"""Test of the sound driver of the 68000 (jo_engine/sound_driver, its binary in jo_engine/audio_driver_68k.h): Musashi
(pip install machine68k) with the sound RAM and a model of the registers of the SCSP that the driver uses (the slots,
the key execute, SCIPD: timer A and the sample flag). Run: python3 tools/test_sound_driver.py"""
import os
import re
import sys
import machine68k as m

HDR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'jo_engine', 'audio_driver_68k.h')
CYCLES_PER_SAMPLE = 256        # 11.2896 MHz / 44.1 kHz

QUEUE, HEAD, TAIL, STATE, TICKS = 0x0C00, 0x0B02, 0x0B04, 0x0B00, 0x0B06


class Scsp:
    def __init__(self):
        self.slots = [[0] * 16 for _ in range(32)]
        self.sample = 0
        self.scipd = 0
        self.timer_a = 0
        self.timer_reload = None
        self.key_exec_pending = False
        self.keys = []            # (sample, slot, on)
        self.keybit = [0] * 32
        self.playing = [False] * 32
        self.pitch_log = {}       # slot -> [(sample, value)]

    def read(self, addr):
        off = addr - 0x100000
        if off < 0x400:
            return self.slots[off >> 5][(off & 0x1F) >> 1]
        if off == 0x420:
            return self.scipd
        return 0

    def write(self, addr, val):
        off = addr - 0x100000
        if off < 0x400:
            s, r = off >> 5, (off & 0x1F) >> 1
            if r == 0:
                if val & 0x1000:
                    self.key_exec_pending = True
                self.slots[s][0] = val & 0x0FFF
                self.keybit[s] = (val >> 11) & 1
            else:
                self.slots[s][r] = val
                if r == 8:
                    self.pitch_log.setdefault(s, []).append((self.sample, val))
        elif off == 0x418:
            self.timer_reload = val & 0xFF
        elif off == 0x422:
            self.scipd &= ~val

    def run_sample(self):
        self.sample += 1
        if self.key_exec_pending:
            for s in range(32):
                if self.keybit[s] and not self.playing[s]:
                    self.playing[s] = True
                    self.keys.append((self.sample, s, True))
                elif not self.keybit[s] and self.playing[s]:
                    self.playing[s] = False
                    self.keys.append((self.sample, s, False))
            self.key_exec_pending = False
        if self.timer_reload is not None:
            self.timer_a = self.timer_reload
            self.timer_reload = None
        else:
            self.timer_a = (self.timer_a + 1) & 0xFF
        if self.timer_a == 0xFF:
            self.scipd |= 0x40
        self.scipd |= 0x400


def load_driver():
    text = open(HDR).read()
    body = text[text.index('{') + 1:text.index('}')]
    return bytes(int(v, 16) for v in re.findall(r'0x([0-9A-F]{2})', body))


class Machine:
    def __init__(self):
        self.m = m.Machine(m.CPUType.M68000, 512)
        self.mem = self.m.mem
        self.scsp = Scsp()
        for width in (0, 1, 2):
            self.mem.set_special_range_read_func(0x100000, width, self.rd(width))
            self.mem.set_special_range_write_func(0x100000, width, self.wr(width))
        drv = load_driver()
        assert len(drv) < 0x0A80
        for k, b in enumerate(drv):
            self.mem.w8(k, b)
        self.m.cpu.pulse_reset()

    def rd(self, width):
        def f(addr):
            if width == 2:
                return (self.scsp.read(addr) << 16) | self.scsp.read(addr + 2)
            v = self.scsp.read(addr & ~1)
            if width == 0:
                return (v >> 8) & 0xFF if not addr & 1 else v & 0xFF
            return v
        return f

    def wr(self, width):
        def f(addr, val):
            if width == 2:
                self.scsp.write(addr, val >> 16)
                self.scsp.write(addr + 2, val & 0xFFFF)
            else:
                self.scsp.write(addr & ~1, val)
        return f

    def run(self, samples):
        for _ in range(samples):
            self.m.cpu.execute(CYCLES_PER_SAMPLE)
            self.scsp.run_sample()

    def command(self, words):
        head = self.mem.r16(HEAD)
        base = QUEUE + head * 32
        for k, w in enumerate(words):
            self.mem.w16(base + 2 * k, w & 0xFFFF)
        self.mem.w16(HEAD, (head + 1) & 31)


def p_reg(p):
    return (((p >> 10) & 0xF) << 11) | (p & 0x3FF)


def main():
    t = Machine()
    t.run(50)
    assert t.mem.r16(STATE) == 0x4A6F, 'the driver does not run (state %04X)' % t.mem.r16(STATE)
    print('driver running (state %04X)' % t.mem.r16(STATE))
    # 2 slots keyed on in one batch
    regs = [0x0120, 0x1000, 0, 63, 0x001F, 0x001F, 0x0010, 0, p_reg(1024 + 300), 0, 0x0007, 0xE000]
    t.command([(1 << 8) | 3] + regs)
    regs2 = list(regs)
    regs2[11] = 0x0000
    t.command([(1 << 8) | 4] + regs2)
    t.run(20)
    assert t.mem.r16(TAIL) == 2, 'tail %d' % t.mem.r16(TAIL)
    ons = [k for k in t.scsp.keys if k[2]]
    assert {k[1] for k in ons} == {3, 4} and ons[0][0] == ons[1][0], ons
    assert t.scsp.slots[3][1:12] == regs[1:12] and t.scsp.slots[3][0] == (0x0120 | 0x0800), t.scsp.slots[3]
    print('key on of 2 slots together at sample %d, registers ok' % ons[0][0])
    # The same slot again (it played to its end, still keyed on): keyed off, then on 2 samples later at least
    t.scsp.keys.clear()
    t.command([(1 << 8) | 3] + regs)
    t.run(20)
    ev = [k for k in t.scsp.keys if k[1] == 3]
    assert len(ev) == 2 and not ev[0][2] and ev[1][2] and ev[1][0] - ev[0][0] >= 1, ev
    print('replay: key off at %d, key on at %d' % (ev[0][0], ev[1][0]))
    # Glide: 300 ms, from 2 octaves up to the base pitch, speed 1/64 of the difference per ms
    start, target, speed = 2048 + 500, 0, 512
    n0 = len(t.scsp.pitch_log[3])
    t.command([(4 << 8) | 3, target, speed, start])
    ms0 = t.mem.r16(TICKS)
    t.run(44 * 400)
    log = t.scsp.pitch_log[3][n0:]
    ticks = t.mem.r16(TICKS) - ms0
    vals = [v for _, v in log]
    def to_p(v):
        o = (v >> 11) & 0xF
        o = o - 16 if o & 8 else o
        return o * 1024 + (v & 0x3FF)
    ps = [to_p(v) for v in vals]
    assert all(b <= a for a, b in zip(ps, ps[1:])), 'not monotonic'
    assert ps[-1] == target, ps[-5:]
    # (the model: p -= p * speed / 32768 each ms: 1/e after 64 ms)
    k = next(i for i, p in enumerate(ps) if p <= start / 2.718)
    print('glide: %d ms ticks in %.0f ms of samples; %d pitch writes; 1/e reached after %d ms (model 64); end %d' %
          (ticks, 400 * 44 / 44.1, len(ps), k, ps[-1]))
    assert 55 <= k <= 75
    # A pitch write stops the glide
    t.command([(4 << 8) | 3, 3000, 100, 0])
    t.run(44 * 5)
    t.command([(3 << 8) | 3, 0x10, 0x1234])
    t.run(44 * 5)
    n = len(t.scsp.pitch_log[3])
    t.run(44 * 20)
    assert len(t.scsp.pitch_log[3]) == n and t.scsp.slots[3][8] == 0x1234
    print('a write of the pitch stops the glide')
    # Key off
    t.scsp.keys.clear()
    t.command([(2 << 8) | 4])
    t.run(10)
    assert t.scsp.keys == [(t.scsp.keys[0][0], 4, False)], t.scsp.keys
    # A full queue: 31 commands written at once
    for k in range(31):
        t.command([(3 << 8) | (k % 32), 0x0C, k])
    t.run(200)
    assert t.mem.r16(TAIL) == t.mem.r16(HEAD)
    print('key off ok; 31 commands in a batch ok; driver: %d bytes' % len(load_driver()))
    print('ALL OK')


if __name__ == '__main__':
    main()
