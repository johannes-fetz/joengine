"""Mednafen runs: a clean configuration in its own directory (Mednafen saves the settings of the command line), the video
recorded with the PNG codec (read while it is written), the sound recorded in a WAV file."""

import glob
import os
import shutil
import signal
import subprocess
import time

from . import barcode
from . import media

IS_WINDOWS = os.name == "nt"

# Same picture on all the computers: no aspect correction, no doubled pixels. The clock is the same at each run (the
# engine seeds jo_random() with it): see Mednafen._prepare_clock()
BASE_SETTINGS = [
    "-video.fs", "0",
    "-video.frameskip", "0",
    "-fps.autoenable", "0",
    "-cheats", "0",
    "-ss.correct_aspect", "0",
    "-qtrecord.w_double_threshold", "0",
    "-qtrecord.h_double_threshold", "0",
    "-qtrecord.vcodec", "png",
]


def find_mednafen(repo, explicit=None):
    candidates = [explicit] if explicit else []
    candidates += [os.environ.get("MEDNAFEN")]
    if IS_WINDOWS:
        candidates.append(os.path.join(repo, "Emulators", "mednafen", "mednafen.exe"))
    else:
        candidates += [shutil.which("mednafen"), "/usr/games/mednafen", "/usr/bin/mednafen"]
    for c in candidates:
        if c and os.path.exists(c):
            return os.path.abspath(c)
    return None


def find_firmware(repo, mednafen):
    candidates = [os.path.join(os.path.dirname(mednafen), "firmware"), os.path.join(repo, "Emulators", "mednafen", "firmware"),
                  os.path.join(os.path.expanduser("~"), ".mednafen", "firmware"), os.path.join(os.path.expanduser("~"), "bios")]
    for c in candidates:
        if os.path.isdir(c) and glob.glob(os.path.join(c, "*.bin")):
            return c
    return None


class Xvfb:
    """A virtual display for Linux without one."""

    def __init__(self):
        self.proc = None
        self.display = os.environ.get("DISPLAY")

    def __enter__(self):
        if IS_WINDOWS or self.display:
            return self
        if shutil.which("Xvfb") is None:
            raise RuntimeError("no DISPLAY and Xvfb not installed")
        self.display = ":%d" % (90 + os.getpid() % 50)
        self.proc = subprocess.Popen(["Xvfb", self.display, "-screen", "0", "1024x768x24"], stdout=subprocess.DEVNULL,
                                     stderr=subprocess.DEVNULL)
        time.sleep(1.0)
        return self

    def __exit__(self, *exc):
        if self.proc:
            self.proc.terminate()
            self.proc.wait()


class RunResult:
    def __init__(self):
        self.reader = None
        self.collector = barcode.Collector()
        self.frames = 0
        self.timed_out = False
        self.exited = False
        self.seconds = 0.0
        self.log = ""
        self.wav = None


class Mednafen:
    def __init__(self, exe, firmware, work_dir, display=None):
        self.exe = exe
        self.firmware = firmware
        self.home = os.path.join(work_dir, "mednafen_home")
        self.display = display

    def _prepare_home(self):
        os.makedirs(os.path.join(self.home, "firmware"), exist_ok=True)
        for f in glob.glob(os.path.join(self.firmware, "*")):
            dst = os.path.join(self.home, "firmware", os.path.basename(f))
            if os.path.isfile(f) and not os.path.exists(dst):
                shutil.copy2(f, dst)
        # The settings, the backup memory and the clock of the last run
        for d in ("sav", "mcs", "mcm", "snaps"):
            shutil.rmtree(os.path.join(self.home, d), ignore_errors=True)
        for f in ("mednafen.cfg", "mednafen-09x.cfg", "stdout.txt", "stderr.txt"):
            try:
                os.remove(os.path.join(self.home, f))
            except OSError:
                pass

    def _env(self):
        env = dict(os.environ)
        env["MEDNAFEN_HOME"] = self.home
        env["MEDNAFEN_ALLOWMULTI"] = "1"
        if self.display:
            env["DISPLAY"] = self.display
        return env

    def _prepare_clock(self, cue):
        """A short run with the clock of the host writes the clock of the game (sav/*.smpc), then its date becomes
        2000-01-01 12:00:00: the next run starts with this clock (ss.smpc.autortc 0), the same at each run. Without a
        clock set, the BIOS shows its setup screen."""
        args = [self.exe] + BASE_SETTINGS + ["-ss.smpc.autortc", "1", "-sound", "0", "-nothrottle", "1"]
        if not IS_WINDOWS:
            args += ["-video.driver", "sdl"]
        args.append(os.path.abspath(cue))
        with open(os.devnull, "w") as null:
            proc = subprocess.Popen(args, env=self._env(), stdout=null, stderr=subprocess.STDOUT, cwd=self.home)
            time.sleep(1.5)
            self._stop(proc)
        files = glob.glob(os.path.join(self.home, "sav", "*.smpc"))
        for f in glob.glob(os.path.join(self.home, "sav", "*")):
            if not f.endswith(".smpc"):
                os.remove(f)
        if len(files) != 1:
            return False
        with open(files[0], "rb") as f:
            data = bytearray(f.read())
        if len(data) < 8:
            return False
        # Valid, year (BCD), day of the week (6: Saturday) and month, day, hour, minute, second
        data[0:8] = bytes([0x01, 0x20, 0x00, 0x61, 0x01, 0x12, 0x00, 0x00])
        with open(files[0], "wb") as f:
            f.write(data)
        return True

    def run(self, cue, movie, stop, timeout, sound=False, wav=None, decode_barcodes=True, progress=None):
        """Run a game until stop(result) is true (checked as the frames come), the timeout (seconds) or the end of
        Mednafen. sound: the sound is emulated and recorded (wav), the speed is the real speed; else Mednafen runs as
        fast as it can."""
        self._prepare_home()
        fixed_clock = self._prepare_clock(cue)
        for f in (movie, wav):
            if f and os.path.exists(f):
                os.remove(f)
        args = [self.exe] + BASE_SETTINGS + ["-ss.smpc.autortc", "0" if fixed_clock else "1", "-qtrecord", movie]
        if sound:
            args += ["-sound", "1", "-sound.volume", "0"]
            if wav:
                args += ["-soundrecord", wav]
            if not IS_WINDOWS:
                args += ["-sound.driver", "sdl"]
        else:
            args += ["-sound", "0", "-nothrottle", "1"]
        if not IS_WINDOWS:
            args += ["-video.driver", "sdl"]
        args.append(os.path.abspath(cue))
        env = self._env()
        if not IS_WINDOWS and sound:
            env.setdefault("SDL_AUDIODRIVER", "dummy")
        result = RunResult()
        result.wav = wav
        result.fixed_clock = fixed_clock
        log_path = movie + ".log"
        # (the folder of the movie: a run of samples only, --only, did not create it)
        os.makedirs(os.path.dirname(os.path.abspath(movie)), exist_ok=True)
        start = time.time()
        with open(log_path, "w") as log:
            proc = subprocess.Popen(args, env=env, stdout=log, stderr=subprocess.STDOUT, cwd=self.home)
            reader = media.FrameReader(movie)
            result.reader = reader
            decoded = 0
            try:
                while True:
                    time.sleep(0.25)
                    count = reader.update()
                    if decode_barcodes:
                        height = None
                        for i in range(decoded, count):
                            raw = reader.raw(i)
                            height = height or int.from_bytes(raw[20:24], "big")
                            code = barcode.decode(media.decode_png(raw, barcode.lines_needed(height)), height)
                            if code:
                                result.collector.add(i, code)
                        decoded = count
                    result.frames = count
                    if progress:
                        progress(result)
                    if stop(result):
                        break
                    if proc.poll() is not None:
                        result.exited = True
                        break
                    if time.time() - start > timeout:
                        result.timed_out = True
                        break
            finally:
                self._stop(proc)
        result.seconds = time.time() - start
        reader.update()
        result.frames = reader.count()
        try:
            with open(log_path, "r", errors="replace") as f:
                result.log = f.read()
            stdout = os.path.join(self.home, "stdout.txt")
            if os.path.exists(stdout):
                with open(stdout, "r", errors="replace") as f:
                    result.log += f.read()
        except OSError:
            pass
        return result

    @staticmethod
    def _stop(proc):
        if proc.poll() is not None:
            return
        # A clean exit: Mednafen closes the movie and the WAV file
        if IS_WINDOWS:
            subprocess.run(["taskkill", "/PID", str(proc.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            proc.send_signal(signal.SIGINT)
        try:
            proc.wait(timeout=8)
        except subprocess.TimeoutExpired:
            if IS_WINDOWS:
                subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            proc.kill()
            proc.wait()
