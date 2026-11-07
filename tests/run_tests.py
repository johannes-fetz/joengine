#!/usr/bin/env python3
"""Regression tests of the Jo Engine with Mednafen (see tests/README.md).

  run_tests.py              quick: builds (layer 0), checks of the test ROM (layers 1 and 2)
  run_tests.py --full       + build variants, the scenes of the test ROM (golden images), the measures (perf and RAM
                            budgets), the samples run in Mednafen (golden images and sound)
  run_tests.py --approve    record the current results as the references (tests/golden), to commit
  run_tests.py --only shooter   + the checks of that sample (config.json "samples.checks"), also in the quick mode
"""

import argparse
import fnmatch
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from harness import barcode, build, emulator, glitch, media  # noqa: E402
from harness.report import FAIL, NEW, PASS, SKIP, WARN, Results, describe_registers  # noqa: E402

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(TESTS_DIR)
GOLDEN = os.path.join(TESTS_DIR, "golden")
FPS = 59.92     # NTSC frames per second of Mednafen (the sound: 801 samples per frame at 48 kHz)


def load_json(path, default):
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return default


def save_json(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=1, sort_keys=True)
        f.write("\n")


class Run:
    def __init__(self, args):
        self.args = args
        self.full = args.full or args.approve
        self.config = load_json(os.path.join(TESTS_DIR, "config.json"), {})
        self.tol = self.config.get("tolerances", {})
        self.work = os.path.abspath(args.work or os.path.join(TESTS_DIR, "_work"))
        os.makedirs(self.work, exist_ok=True)
        self.results = Results(os.path.join(self.work, "report"))
        self.toolchain = build.Toolchain(REPO, args.compiler_dir)
        self.values_path = os.path.join(GOLDEN, "rom_values.json")
        self.budgets_path = os.path.join(GOLDEN, "budgets.json")
        self.values = load_json(self.values_path, {})
        self.budgets = load_json(self.budgets_path, {})
        self.approved = 0

    def selected(self, name):
        return not self.args.only or any(fnmatch.fnmatch(name.lower(), "*%s*" % p.lower()) for p in self.args.only)

    def approve(self, what):
        self.approved += 1
        print("         approved: %s" % what)

    # Layer 0: builds

    def build_jobs(self):
        rom_cfg = self.config["rom"]
        jobs = []
        variants = rom_cfg["variants"]
        # (the build only variants: every module of the engine compiled, in debug and in release)
        names = list(variants) if self.full else rom_cfg.get("quick_variants", list(variants)[:1]) + rom_cfg.get("build_only", [])
        for v in names:
            if self.selected("rom " + v) or self.selected("rom"):
                jobs.append(build.BuildJob("test rom", v, os.path.join(TESTS_DIR, "rom"), os.path.join(self.work, "build", "rom", v),
                                           variants[v], rom=(rom_cfg["modules"], rom_cfg["sources"])))
        samples_cfg = self.config.get("samples", {})
        samples_dir = os.path.join(REPO, samples_cfg.get("dir", "Samples"))
        skip = set(samples_cfg.get("skip", []))
        sample_variants = {"default": {}}
        if self.full:
            sample_variants.update(samples_cfg.get("full_variants", {}))
        if os.path.isdir(samples_dir) and not self.args.rom_only:
            for name in build.find_samples(samples_dir):
                if name in skip or not self.selected(name):
                    continue
                for v, variables in sample_variants.items():
                    jobs.append(build.BuildJob(name, v, os.path.join(samples_dir, name),
                                               os.path.join(self.work, "build", build.slug(name), v), variables))
                # (the checks of the sample: in the full mode, or when --only names it)
                if self.full or self.args.only:
                    for check in samples_cfg.get("checks", {}).get(name, []):
                        v = "check " + check["name"]
                        jobs.append(build.BuildJob(name, v, os.path.join(samples_dir, name),
                                                   os.path.join(self.work, "build", build.slug(name), build.slug(v)),
                                                   check.get("variables", {})))
        return jobs

    def run_builds(self, jobs):
        print("\n[0] Builds (%d, %d in parallel)" % (len(jobs), self.args.jobs))
        problem = self.toolchain.check()
        if problem:
            self.results.add("0 build", "toolchain", FAIL, problem)
            return
        start = time.time()

        def done(job):
            if job.ok:
                msg = "%.0f s" % job.seconds + (", %d warnings" % len(job.warnings) if job.warnings else "")
                self.results.add("0 build", job.key, WARN if job.warnings and job.name == "test rom" else PASS, msg,
                                 details=job.warnings[:20], quiet=not job.warnings or job.name != "test rom")
            else:
                self.results.add("0 build", job.key, FAIL, job.message, details=["log: %s" % job.log])

        build.run_jobs(jobs, self.toolchain, self.args.jobs, done)
        ok = sum(1 for j in jobs if j.ok)
        print("  %d/%d built in %.0f s" % (ok, len(jobs), time.time() - start))

    def check_sizes(self, jobs):
        """Layer 5: the sizes of the programs (code, data, RAM)."""
        tol = self.tol.get("size_percent", 2.0)
        sizes = self.budgets.setdefault("sizes", {})
        # (absolute limits of config.json: the program of JO_MINIMAL = 1 must stay much smaller than the default engine)
        limits = self.config.get("samples", {}).get("max_bin_size", {})
        for job in jobs:
            limit = limits.get(job.name)
            if job.ok and limit is not None and "bin" in job.sizes:
                self.results.add("5 size", "%s [%s] limit" % (job.name, job.variant), FAIL if job.sizes["bin"] > limit else PASS,
                                 "%d bytes, at most %d" % (job.sizes["bin"], limit), quiet=job.sizes["bin"] <= limit)
            if not job.ok or job.variant != "default" and job.name != "test rom":
                continue
            key = "%s [%s]" % (job.name, job.variant)
            ref = sizes.get(key)
            current = {k: v for k, v in job.sizes.items() if k in ("text", "rodata", "data", "bss", "bin")}
            if self.args.approve:
                if ref != current:
                    sizes[key] = current
                    self.approve("sizes of %s" % key)
                continue
            if ref is None:
                self.results.add("5 size", key, NEW, "no reference", quiet=True)
                continue
            grown = ["%s %d -> %d (%+.1f %%)" % (k, ref[k], v, 100.0 * (v - ref[k]) / max(1, ref[k]))
                     for k, v in current.items() if k in ref and v > ref[k] * (1 + tol / 100.0) and v - ref[k] > 64]
            if grown:
                self.results.add("5 size", key, FAIL if self.full else WARN, "bigger than the reference", details=grown)
            else:
                self.results.add("5 size", key, PASS, "", quiet=True)

    # Layers 1 to 5: the test ROM

    def run_rom(self, med, job):
        variant = job.variant
        print("\n[1-2] Test ROM %s%s" % (variant, " (scenes, measures)" if self.full else ""))
        movie = os.path.join(self.work, "run", "rom_%s.mov" % variant)
        os.makedirs(os.path.dirname(movie), exist_ok=True)
        timeout = self.config["rom"].get("timeout_full" if self.full else "timeout_quick", 300 if self.full else 120)

        if self.full:
            def stop(r):
                return r.collector.end_frame is not None and r.frames > r.collector.end_frame + 2
        else:
            def stop(r):
                return r.collector.stage_complete(0)
        result = med.run(job.cue, movie, stop, timeout)
        layer = "1-2 rom"
        name = "rom %s" % variant
        report = result.collector.report(0)
        if report is None:
            last = self.save_frame(result, result.frames - 1, "rom/%s/last_frame.png" % variant)
            self.results.add(layer, name, FAIL, "no report from the ROM after %.0f s (%d frames): crash or jo_core_error()?"
                             % (result.seconds, result.frames), details=result.log.splitlines()[-8:], images=last)
            return
        failed = ["%s: %s (line %d): %d, expected %d" % (f["group"], f["text"], f["line"], f["actual"], f["expected"])
                  for f in report["failed"]]
        status = FAIL if report["failures"] or report["infos"] else PASS
        self.results.add(layer, name + " checks", status, "%d checks, %d failed" % (report["checks"], report["failures"]),
                         details=failed + report["infos"])
        self.check_values(variant, report)
        # The anti-glitch views of the 3D (absolute checks, no reference: see harness/glitch.py): the quick ones only in
        # the quick mode (shown before the report of the checks)
        hires = int(self.config["rom"]["variants"].get(variant, {}).get("JO_SCREEN_WIDTH", 320)) > 352
        # (the variants of the long fuzz don't show the views, see t_glitch_steps())
        fuzz = "T_FUZZ_LONG" in self.config["rom"]["variants"].get(variant, {})
        if not hires and not fuzz and (self.selected("glitch") or self.selected("rom " + variant)):
            start = time.time()
            glitch.run_checks(self, variant, result, self.results, not self.full)
            print("  (anti-glitch views checked in %.0f s)" % (time.time() - start))
        if not self.full:
            return
        # (the errors of the engine during the anti-glitch views: their report, stage 2)
        errors = result.collector.report(2)
        if errors is not None and (errors["failures"] or errors["infos"]):
            self.results.add(glitch.LAYER, "glitch %s engine errors" % variant, FAIL, "%d errors of the engine during the views"
                             % errors["failures"], details=["%s: %s" % (f["group"], f["text"]) for f in errors["failed"]] +
                             errors["infos"])
        perf = result.collector.report(1)
        if perf is None:
            self.results.add("5 perf", name, FAIL, "no measures (stage 1)")
        else:
            self.check_metrics("rom:%s" % variant, perf["metrics"])
        self.check_scenes(variant, result)
        if result.collector.end_frame is None:
            last = self.save_frame(result, result.frames - 1, "rom/%s/last_frame.png" % variant)
            self.results.add("3 image", name, FAIL, "the ROM did not reach its end (%d frames)" % result.frames, images=last)

    def check_values(self, variant, report):
        golden = self.values.setdefault(variant, {})
        current = dict(("value:" + k, v) for k, v in report["values"].items())
        current.update(("blob:" + k, v) for k, v in report["blobs"].items())
        if self.args.approve:
            if golden != current:
                self.values[variant] = current
                self.approve("values of the ROM %s" % variant)
            return
        if not golden:
            self.results.add("1-2 rom", "rom %s values" % variant, NEW, "%d values, no reference" % len(current))
            return
        details = []
        for k in sorted(set(golden) | set(current)):
            a, b = golden.get(k), current.get(k)
            if a == b:
                continue
            if a is None:
                details.append("%s: new (%s)" % (k, b))
            elif b is None:
                details.append("%s: missing" % k)
            elif k.startswith("blob:"):
                details.append("%s: %s, reference %s" % (k[5:], describe_registers(b), describe_registers(a)))
            else:
                details.append("%s: %d, reference %d (0x%08X / 0x%08X)" % (k[6:], b, a, b, a))
        self.results.add("1-2 rom", "rom %s values" % variant, FAIL if details else PASS,
                         "%d values%s" % (len(current), ", %d different" % len(details) if details else ""), details=details)

    def check_metrics(self, key, metrics):
        tol_perf = self.tol.get("perf_percent", 5.0)
        tol_ram = self.tol.get("ram_percent", 5.0)
        ref = self.budgets.setdefault(key, {})
        if self.args.approve:
            if ref != metrics:
                self.budgets[key] = dict(metrics)
                self.approve("measures of %s" % key)
            return
        if not ref:
            self.results.add("5 perf", key, NEW, "%d measures, no reference" % len(metrics))
            return
        worse = []
        better = []
        for k, v in sorted(metrics.items()):
            r = ref.get(k)
            if r is None:
                continue
            tol = tol_ram if k.startswith("ram.") else tol_perf
            limit = r * (1 + tol / 100.0) + (1 if k.startswith("ram.") else 16)
            if v > limit:
                worse.append("%s: %d, reference %d (%+.1f %%)" % (k, v, r, 100.0 * (v - r) / max(1, r)))
            elif v < r * (1 - tol / 100.0) - 16:
                better.append("%s: %d, reference %d (%+.1f %%)" % (k, v, r, 100.0 * (v - r) / max(1, r)))
        missing = sorted(set(ref) - set(metrics))
        details = worse + ["missing: %s" % ", ".join(missing)] * bool(missing) + ["better: " + b for b in better]
        status = FAIL if worse or missing else PASS
        self.results.add("5 perf", key, status, "%d measures%s" % (len(metrics), ", %d worse" % len(worse) if worse else "")
                         + (", %d better (--approve to keep them)" % len(better) if better else ""), details=details)

    def check_scenes(self, variant, result):
        golden_dir = os.path.join(GOLDEN, "rom", variant)
        seen = set()
        for scene, last, offset in result.collector.markers:
            # (the anti-glitch views: checked by harness/glitch.py, no golden image)
            if scene.startswith("g_"):
                continue
            seen.add(scene)
            self.check_image("3 image", "rom %s %s" % (variant, scene), result, last + offset,
                             os.path.join(golden_dir, "%s.png" % scene), "rom/%s/%s" % (variant, scene))
        if os.path.isdir(golden_dir):
            for f in sorted(os.listdir(golden_dir)):
                if f.endswith(".png") and f[:-4] not in seen:
                    self.results.add("3 image", "rom %s %s" % (variant, f[:-4]), FAIL, "scene not shown by the ROM")

    # Images

    def save_frame(self, result, index, rel):
        if index < 0 or index >= result.reader.count():
            return []
        path = self.results.image_path(rel)
        media.write_png(path, result.reader.frame(index))
        return [("frame %d" % index, path)]

    def find_shift(self, result, golden, index, max_shift):
        """The frame equal to the reference near the frame index (the same pictures sooner or later: the timing changed)."""
        for d in sorted(range(-max_shift, max_shift + 1), key=abs):
            i = index + d
            if d and 0 <= i < result.reader.count():
                frame = result.reader.frame(i)
                if frame.data == golden.data:
                    return d
        return None

    def check_image(self, layer, name, result, index, golden_path, rel, max_shift=0):
        if index >= result.reader.count():
            self.results.add(layer, name, FAIL, "frame %d not recorded (%d frames)" % (index, result.reader.count()))
            return
        image = result.reader.frame(index)
        actual_path = self.results.image_path(rel + ".png")
        media.write_png(actual_path, image)
        if self.args.approve:
            same = os.path.exists(golden_path) and media.compare_images(media.read_png(golden_path), image, 0)[0] == 0
            if not same:
                media.write_png(golden_path, image)
                self.approve(os.path.relpath(golden_path, TESTS_DIR))
            return
        if not os.path.exists(golden_path):
            colors, _ = media.image_stats(image)
            self.results.add(layer, name, WARN if colors <= 1 else NEW, "no reference" + (", uniform picture" if colors <= 1 else ""),
                             images=[("frame %d" % index, actual_path)])
            return
        golden = media.read_png(golden_path)
        threshold = self.tol.get("image_threshold", 24)
        count, diff = media.compare_images(golden, image, threshold)
        allowed = self.tol.get("image_max_pixels_percent", 0.05) * image.width * image.height / 100.0
        if count == 0:
            self.results.add(layer, name, PASS, "identical", quiet=True)
            return
        shift = self.find_shift(result, golden, index, max_shift) if max_shift else None
        if shift is not None:
            self.results.add(layer, name, WARN, "the same picture %d frames %s: the timing changed (--approve if intended)"
                             % (abs(shift), "later" if shift > 0 else "sooner"))
            return
        images = [("reference", golden_path), ("frame %d" % index, actual_path)]
        if diff is not None:
            diff_path = self.results.image_path(rel + ".diff.png")
            media.write_png(diff_path, diff)
            images.append(("%d pixels different" % count, diff_path))
        self.results.add(layer, name, FAIL if count > allowed else PASS,
                         "%d pixels different (%.3f %%)" % (count, 100.0 * count / (image.width * image.height)), images=images)

    # Layer 6: the samples

    def run_sample(self, med, job):
        boot = self.config.get("samples", {}).get("boot", {})
        per_sample = boot.get("per_sample", {}).get(job.name, {})
        frames = per_sample.get("frames", boot.get("frames", [1500]))
        last = max(frames)
        sound = not self.args.no_audio and per_sample.get("audio", True)
        slug = build.slug(job.name)
        movie = os.path.join(self.work, "run", "%s.mov" % slug)
        wav = os.path.join(self.work, "run", "%s.wav" % slug) if sound else None
        timeout = last / FPS * 3 + 40
        shift = boot.get("max_shift_frames", 30)
        result = med.run(job.cue, movie, lambda r: r.frames > last + shift + 2, timeout, sound=sound, wav=wav, decode_barcodes=False)
        layer = "6 sample"
        if result.frames <= last:
            images = self.save_frame(result, result.frames - 1, "samples/%s/last_frame.png" % slug)
            self.results.add(layer, job.name, FAIL, "Mednafen stopped at frame %d (%s)" %
                             (result.frames, "timeout" if result.timed_out else "exited"), details=result.log.splitlines()[-8:],
                             images=images)
        for f in frames:
            self.check_image(layer, "%s frame %d" % (job.name, f), result, f,
                             os.path.join(GOLDEN, "samples", slug, "frame_%05d.png" % f), "samples/%s/frame_%05d" % (slug, f),
                             max_shift=shift)
        if sound and wav:
            self.check_audio(job.name, slug, wav, last, shift)

    # Layer 7: the checks of the samples (config.json "samples.checks")

    @staticmethod
    def verdict_of(image):
        """The verdict shown by a check of a sample: the whole screen green (PASS) or red (FAIL), None: not yet. (A pixel
        of 16: most of the screen of that color, the text of the summary and the sprites on it)"""
        green = red = total = 0
        data = image.data
        for y in range(0, image.height, 4):
            for i in range(y * image.width * 3, (y + 1) * image.width * 3, 12):
                r, g, b = data[i], data[i + 1], data[i + 2]
                total += 1
                if g > 192 and r < 64 and b < 64:
                    green += 1
                elif r > 192 and g < 64 and b < 64:
                    red += 1
        if green * 10 > total * 7:
            return PASS
        if red * 10 > total * 7:
            return FAIL
        return None

    def run_check(self, med, job):
        name = job.variant[len("check "):]
        check = next(c for c in self.config["samples"]["checks"][job.name] if c["name"] == name)
        last = check.get("max_frames", 3000)
        movie = os.path.join(self.work, "run", "%s.mov" % build.slug(job.name + " " + job.variant))
        state = {"seen": 0, "verdict": None, "frame": None}

        def stop(r):
            # (the last frame only, read once per call: the verdict stays on the screen)
            if r.frames > max(state["seen"], check.get("first_frame", 0)):
                state["seen"] = r.frames
                verdict = self.verdict_of(r.reader.frame(r.frames - 1))
                if verdict is not None:
                    state["verdict"], state["frame"] = verdict, r.frames - 1
                    return True
            return r.frames > last

        result = med.run(job.cue, movie, stop, last / FPS * 3 + 60, decode_barcodes=False)
        layer = "7 check"
        key = "%s: %s" % (job.name, name)
        if state["verdict"] is None:
            images = self.save_frame(result, result.frames - 1, "checks/%s/last_frame.png" % build.slug(key))
            self.results.add(layer, key, FAIL, "no verdict after %d frames (%s)" % (result.frames, "timeout" if result.timed_out
                             else "exited" if result.exited else "too long"), details=result.log.splitlines()[-8:], images=images)
            return
        images = self.save_frame(result, state["frame"], "checks/%s/verdict.png" % build.slug(key))
        self.results.add(layer, key, state["verdict"], "%s at frame %d (the counts on the picture)" % (
                         "passed" if state["verdict"] == PASS else "failed", state["frame"]), details=[check.get("what", "")],
                         images=images)

    def check_audio(self, name, slug, wav, last_frame, max_shift=0):
        layer = "4 audio"
        try:
            rate, _, samples = media.read_wav(wav)
        except (OSError, ValueError) as e:
            self.results.add(layer, name, FAIL, "no sound recorded: %s" % e)
            return
        end = int(rate * last_frame / FPS)
        if len(samples) < end:
            self.results.add(layer, name, FAIL, "sound too short: %.1f s, %.1f s expected" % (len(samples) / rate, end / rate))
            return
        envelope = media.audio_envelope(samples, rate, 0, end)
        golden_path = os.path.join(GOLDEN, "samples", slug, "audio.json")
        golden = load_json(golden_path, None)
        loud = sum(1 for v in envelope if v > -50)
        if self.args.approve:
            if golden is None or golden.get("envelope") != envelope:
                save_json(golden_path, {"rate": rate, "block_ms": 100, "envelope": envelope})
                self.approve(os.path.relpath(golden_path, TESTS_DIR))
            return
        if golden is None:
            self.results.add(layer, name, NEW, "no reference (%d blocks with sound of %d)" % (loud, len(envelope)))
            return
        tolerance = self.tol.get("audio_db", 3.0)
        bad, compared = media.compare_envelopes(golden["envelope"], envelope, tolerance)
        allowed = max(1, int(compared * self.tol.get("audio_max_blocks_percent", 5.0) / 100.0))
        if bad > allowed and max_shift:
            # The same sound sooner or later (the timing changed): the envelope shifted by whole blocks of 100 ms
            ref = golden["envelope"]
            blocks = int(max_shift / FPS * 10) + 1
            for d in range(1, blocks + 1):
                for sign in (1, -1):
                    k = sign * d
                    a, b = (ref[:len(ref) - d], envelope[d:]) if k > 0 else (ref[d:], envelope[:len(envelope) - d])
                    shifted_bad, shifted_compared = media.compare_envelopes(a, b, tolerance)
                    if shifted_bad <= max(1, int(shifted_compared * self.tol.get("audio_max_blocks_percent", 5.0) / 100.0)):
                        self.results.add(layer, name, WARN, "the same sound %d ms %s: the timing changed (--approve if intended)"
                                         % (100 * d, "later" if k > 0 else "sooner"))
                        return
        self.results.add(layer, name, FAIL if bad > allowed else PASS,
                         "%d blocks of 100 ms different of %d with sound" % (bad, compared), quiet=bad == 0)


def main():
    parser = argparse.ArgumentParser(description="Regression tests of the Jo Engine (Mednafen)")
    parser.add_argument("--full", action="store_true", help="all the layers (long): variants, images, measures, samples")
    parser.add_argument("--approve", action="store_true", help="record the results as the new references (implies --full)")
    parser.add_argument("--only", nargs="+", metavar="PATTERN", help="only the samples (or 'rom') whose name contains a pattern")
    parser.add_argument("--rom-only", action="store_true", help="the test ROM only, not the samples")
    parser.add_argument("--no-build", action="store_true", help="use the builds of the last run")
    parser.add_argument("--no-audio", action="store_true", help="samples without sound (faster: no real time)")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4, help="parallel builds")
    parser.add_argument("--mednafen", help="mednafen executable (default: Emulators/mednafen/mednafen.exe, $MEDNAFEN)")
    parser.add_argument("--compiler-dir", help="Compiler directory (default: Compiler)")
    parser.add_argument("--work", help="work directory (default: tests/_work)")
    args = parser.parse_args()

    run = Run(args)
    mode = "approve" if args.approve else "full" if run.full else "quick"
    print("Jo Engine regression tests (%s)" % mode)
    start = time.time()
    jobs = run.build_jobs()
    if args.no_build:
        for job in jobs:
            cue = os.path.join(job.out_dir, "src", "game.cue")
            job.ok = os.path.exists(cue)
            job.cue = cue if job.ok else None
            job.sizes = build.parse_map(os.path.join(job.out_dir, "src", "game.map"))
            if not job.ok:
                run.results.add("0 build", job.key, FAIL, "no build (run without --no-build)")
    else:
        run.run_builds(jobs)
    run.check_sizes(jobs)

    rom_jobs = [j for j in jobs if j.name == "test rom" and j.ok and j.variant not in run.config["rom"].get("build_only", [])]
    sample_jobs = [j for j in jobs if j.name != "test rom" and j.ok and j.variant == "default"
                   and j.name not in run.config.get("samples", {}).get("boot", {}).get("skip", [])]
    check_jobs = [j for j in jobs if j.name != "test rom" and j.ok and j.variant.startswith("check ")]
    exe = emulator.find_mednafen(REPO, args.mednafen)
    if rom_jobs or (run.full and sample_jobs) or check_jobs:
        firmware = emulator.find_firmware(REPO, exe) if exe else None
        if exe is None or firmware is None:
            run.results.add("1-2 rom", "mednafen", FAIL, "Mednafen not found (Emulators/mednafen/mednafen.exe, option --mednafen)"
                            if exe is None else "Saturn BIOS not found (Emulators/mednafen/firmware)")
        else:
            with emulator.Xvfb() as x:
                med = emulator.Mednafen(exe, firmware, run.work, x.display)
                for job in rom_jobs:
                    run.run_rom(med, job)
                if run.full and sample_jobs:
                    print("\n[6] Samples in Mednafen (%d%s)" % (len(sample_jobs), ", with sound: real time" if not args.no_audio else ""))
                    for job in sample_jobs:
                        run.run_sample(med, job)
                if check_jobs:
                    print("\n[7] Checks of the samples (%d)" % len(check_jobs))
                    for job in check_jobs:
                        run.run_check(med, job)

    if args.approve:
        save_json(run.values_path, run.values)
        save_json(run.budgets_path, run.budgets)
    r = run.results
    summary = ["%s: %d passed, %d failed, %d warnings, %d new, %.0f s" % (mode, r.count(PASS), r.count(FAIL), r.count(WARN),
                                                                         r.count(NEW), time.time() - start)]
    if args.approve:
        summary.append("%d references updated in tests/golden (to commit)" % run.approved)
    elif r.count(NEW):
        summary.append("New results without reference: check them in the report, then run_tests --approve")
    path = r.write("Jo Engine tests (%s)" % mode, summary)
    print("\n" + "\n".join(summary))
    print("Report: %s" % path)
    return 1 if r.count(FAIL) else 0


if __name__ == "__main__":
    sys.exit(main())
