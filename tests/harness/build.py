"""Builds of the samples and of the test ROM, each one in its own directory (the objects of the engine are written in its
directory: a copy of the engine per build, the builds run in parallel)."""

import os
import re
import shutil
import subprocess
import time

IS_WINDOWS = os.name == "nt"


def slug(name):
    return re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").lower()


class Toolchain:
    def __init__(self, repo, compiler_dir=None):
        self.repo = repo
        self.compiler_dir = os.path.abspath(compiler_dir or os.path.join(repo, "Compiler"))
        self.engine_dir = os.path.join(repo, "jo_engine")
        self.env = dict(os.environ)
        if IS_WINDOWS:
            win = os.path.join(self.compiler_dir, "WINDOWS")
            self.env["PATH"] = os.pathsep.join([os.path.join(win, "Other Utilities"), os.path.join(win, "bin"), self.env.get("PATH", "")])
            make = os.path.join(win, "Other Utilities", "make.exe")
            self.make = make if os.path.exists(make) else "make"
        else:
            self.make = "make"

    def check(self):
        """A message if the toolchain is missing, else None."""
        if not os.path.isdir(self.engine_dir):
            return "jo_engine not found: %s" % self.engine_dir
        common = os.path.join(self.compiler_dir, "COMMON", "jo_engine_makefile")
        if not os.path.exists(common):
            return "Compiler not found: %s (option --compiler-dir)" % common
        if self.make != "make":
            return None
        if shutil.which("make", path=self.env.get("PATH")) is None:
            return "make not found"
        return None


def _ignore_build_files(directory, names):
    return [n for n in names if n.endswith(".o") or n in ("jo_options.txt", "game.iso", "game.cue", "game.elf", "game.raw",
                                                          "game.map", "game.ip.bin", "game.ip.o", "0.bin")]


def generate_rom_makefile(path, modules, sources):
    # (T_* keys are defines of the test ROM itself, as T_FUZZ_LONG: the other keys are variables of the engine's makefile)
    lines = ["%s = %s" % (k, v) for k, v in modules.items() if not k.startswith("T_")]
    lines += ["CCFLAGS += -D%s=%s" % (k, v) for k, v in modules.items() if k.startswith("T_")]
    lines.append("SRCS = %s" % " ".join(sources))
    lines.append("include $(COMPILER_DIR)/COMMON/jo_engine_makefile")
    with open(path, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def parse_map(path):
    """Sizes of the sections of a game.map: text, rodata, data, bss (bytes)."""
    sizes = {}
    try:
        with open(path, "r", errors="replace") as f:
            for line in f:
                m = re.match(r"^\.(text|rodata|data|bss)\s+0x[0-9a-fA-F]+\s+0x([0-9a-fA-F]+)", line)
                if m:
                    sizes[m.group(1)] = sizes.get(m.group(1), 0) + int(m.group(2), 16)
    except OSError:
        pass
    return sizes


class BuildJob:
    def __init__(self, name, variant, source_dir, out_dir, variables, rom=None):
        self.name = name
        self.variant = variant
        self.source_dir = source_dir
        self.out_dir = out_dir
        self.variables = variables
        self.rom = rom              # (modules, sources) for the test ROM: its makefile is generated
        self.ok = False
        self.message = ""
        self.warnings = []
        self.seconds = 0.0
        self.sizes = {}
        self.cue = None
        self.log = os.path.join(out_dir, "build.log")

    @property
    def key(self):
        return "%s [%s]" % (self.name, self.variant)

    def run(self, toolchain):
        start = time.time()
        try:
            self._run(toolchain)
        except Exception as e:  # noqa: BLE001 (a build must not stop the others)
            self.ok = False
            self.message = "harness error: %s" % e
        self.seconds = time.time() - start
        return self

    def _run(self, toolchain):
        src = os.path.join(self.out_dir, "src")
        engine = os.path.join(self.out_dir, "engine")
        if os.path.exists(self.out_dir):
            shutil.rmtree(self.out_dir, ignore_errors=True)
        shutil.copytree(self.source_dir, src, ignore=_ignore_build_files)
        shutil.copytree(toolchain.engine_dir, engine, ignore=_ignore_build_files)
        if self.rom is not None:
            generate_rom_makefile(os.path.join(src, "makefile"), self.rom[0], self.rom[1])
        try:
            compiler = os.path.relpath(os.path.realpath(toolchain.compiler_dir), os.path.realpath(src)).replace("\\", "/")
        except ValueError:
            # (another drive)
            compiler = toolchain.compiler_dir.replace("\\", "/")
        args = [toolchain.make, "JO_ENGINE_SRC_DIR=../engine", "COMPILER_DIR=%s" % compiler]
        args += ["%s=%s" % (k, v) for k, v in self.variables.items()]
        with open(self.log, "w", errors="replace") as log:
            log.write(" ".join(args) + "\n")
            log.flush()
            proc = subprocess.run(args, cwd=src, env=toolchain.env, stdout=log, stderr=subprocess.STDOUT)
        with open(self.log, "r", errors="replace") as f:
            text = f.read()
        self.warnings = sorted(set(l.strip() for l in text.splitlines() if "warning:" in l))
        cue = os.path.join(src, "game.cue")
        if proc.returncode != 0 or not os.path.exists(cue) or not os.path.exists(os.path.join(src, "game.iso")):
            lines = text.splitlines()
            errors = [l.strip() for l in lines if "error:" in l or "undefined reference" in l]
            errors += [l.strip() for l in lines if l.startswith("make") and "***" in l]
            self.message = errors[0][:300] if errors else "make returned %d" % proc.returncode
            return
        self.ok = True
        self.cue = cue
        self.sizes = parse_map(os.path.join(src, "game.map"))
        try:
            self.sizes["bin"] = os.path.getsize(os.path.join(src, "game.raw"))
        except OSError:
            pass


def run_jobs(jobs, toolchain, parallel, on_done=None):
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=max(1, parallel)) as pool:
        for job in pool.map(lambda j: j.run(toolchain), jobs):
            if on_done:
                on_done(job)
    return jobs


def find_samples(samples_dir):
    out = []
    for name in sorted(os.listdir(samples_dir)):
        d = os.path.join(samples_dir, name)
        if os.path.isdir(d) and any(os.path.exists(os.path.join(d, m)) for m in ("makefile", "Makefile")):
            out.append(name)
    return out
