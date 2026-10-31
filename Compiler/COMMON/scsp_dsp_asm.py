#!/usr/bin/env python3
"""Minimal SCSP DSP assembler (Sega Saturn sound DSP), with a simulator to test the programs on a computer.

Usage:
  python3 scsp_dsp_asm.py PROGRAM.scsp OUTPUT.h NAME     assemble (C header: program, memory addresses, coefficients)
  python3 scsp_dsp_asm.py --test PROGRAM.scsp            assemble, run the simulator on an impulse, print the decay

The DSP runs its 128 steps once per sample (44.1 kHz). A step multiplies X by Y and adds B: the result is in the
accumulator at the next step, where the stores (twt, mwt, ewt) write it (through the shifter: saturated to 24 bits).

Source syntax (';' starts a comment):
  .ring WORDS                the size of the ring buffer in the sound RAM (8192, 16384, 32768 or 65536 words)
  .line NAME LENGTH          a delay line (LENGTH words + 1): its write address is NAME, a value written there is read
                             N samples later at NAME+N (N <= LENGTH)
  .coef NAME                 a coefficient (13 bits, -1 to 1: 4096 = 1), set at run time by the program of the SH-2
  .const NAME VALUE          a coefficient with a fixed value (-1.0 to 0.9997)
  a step: fields separated by spaces:
    ira=mixsN | mems N       the input of the step (INPUTS): the mix of the slots sent to the DSP (ISEL N), or MEMS N
    x=in | tN | tN'          X: INPUTS, the temporary register N of this sample, or its value of the previous sample (tN')
    y=COEF                   Y: a coefficient (default: 0, the product is 0)
    b=acc | tN | tN' | 0     B: the accumulator (the result of the previous step), a temporary register, 0 (default)
    neg                      B is negated
    twt=tN                   the result of the previous step written to the temporary register N
    mwt=LINE[+N]             the result of the previous step written to the ring buffer (float format: 24 bits kept as
                             a 4 bits exponent and an 11 bits mantissa)
    mrd=LINE[+N]>K           a read of the ring buffer: the value is in MEMS K from the step after the next one (the
                             DSP reads in 2 steps); the memory is accessed on the odd steps only (the assembler adds
                             steps that keep the accumulator to place them there)
    ewt=N                    the result of the previous step written to the output N (EFREG N, 16 bits: mixed by the
                             effect level (EFSDL) and pan (EFPAN) of the slot N)
    shift=1                  the result doubled (saturated) by the shifter

The temporary registers turn with the ring buffer (TEMP[(TRA + MDEC_CT) & 0x7F]): tN' is the register N + 1 of this
sample. The assembler rejects a program that writes a register whose number is used by a tN'.

The semantics of the simulator are the ones of Mednafen (ss/scsp.inc, SS_SCSP_RunDSPStep()).
"""
import re
import sys

COEF_ONE = 4096


class Program:
    def __init__(self):
        self.ring = 8192
        self.lines = {}          # name -> (base address, length)
        self.line_order = []
        self.next_addr = 0
        self.coefs = []          # names (index = order)
        self.consts = {}         # name -> value (float)
        self.steps = []          # raw steps (dicts), before placement
        self.words = []          # the 128 steps: 4 x 16 bits each
        self.madrs = []          # memory addresses (MASA index -> word address)
        self.madrs_names = []

    # ---------------------------------------------------------------------------------------------------------------
    def parse(self, text):
        for number, raw in enumerate(text.splitlines(), 1):
            line = raw.split(';')[0].strip()
            if not line:
                continue
            try:
                self.parse_line(line)
            except ValueError as e:
                raise SystemExit('line %d: %s: %s' % (number, raw.strip(), e))

    def parse_line(self, line):
        tokens = line.split()
        if tokens[0] == '.ring':
            self.ring = int(tokens[1])
            if self.ring not in (8192, 16384, 32768, 65536):
                raise ValueError('ring size: 8192, 16384, 32768 or 65536')
            return
        if tokens[0] == '.line':
            name, length = tokens[1], int(tokens[2])
            self.lines[name] = (self.next_addr, length)
            self.line_order.append(name)
            self.next_addr += length + 1
            if self.next_addr > self.ring:
                raise ValueError('the delay lines need %d words, more than the ring buffer' % self.next_addr)
            return
        if tokens[0] == '.coef':
            self.coefs.append(tokens[1])
            return
        if tokens[0] == '.const':
            self.coefs.append(tokens[1])
            self.consts[tokens[1]] = float(tokens[2])
            return
        step = {'src': line}
        for t in tokens:
            if t == 'neg':
                step['neg'] = True
                continue
            if '=' not in t:
                raise ValueError('unknown field ' + t)
            k, v = t.split('=', 1)
            if k not in ('ira', 'x', 'y', 'b', 'twt', 'mwt', 'mrd', 'ewt', 'shift'):
                raise ValueError('unknown field ' + k)
            step[k] = v
        self.steps.append(step)

    # ---------------------------------------------------------------------------------------------------------------
    def address(self, text):
        """LINE[+N] -> (MADRS index)"""
        m = re.fullmatch(r'(\w+)(?:\+(\d+))?', text)
        if not m or m.group(1) not in self.lines:
            raise ValueError('unknown delay line ' + text)
        base, length = self.lines[m.group(1)]
        offset = int(m.group(2) or 0)
        if offset > length:
            raise ValueError('%s: past the end of the line (%d)' % (text, length))
        value = base + offset
        if value in self.madrs:
            return self.madrs.index(value)
        if len(self.madrs) >= 32:
            raise ValueError('more than 32 memory addresses')
        self.madrs.append(value)
        self.madrs_names.append(text)
        return len(self.madrs) - 1

    @staticmethod
    def temp(text, prev_ok=True):
        m = re.fullmatch(r"t(\d+)('?)", text)
        if not m:
            raise ValueError('a temporary register expected: ' + text)
        n = int(m.group(1))
        if m.group(2) and not prev_ok:
            raise ValueError("a write to " + text)
        return (n + (1 if m.group(2) else 0)) & 0x7F, n, bool(m.group(2))

    def coef_index(self, name):
        if name not in self.coefs:
            raise ValueError('unknown coefficient ' + name)
        return self.coefs.index(name)

    # ---------------------------------------------------------------------------------------------------------------
    def assemble(self):
        if 'zero' not in self.coefs:
            self.coefs.append('zero')
            self.consts['zero'] = 0.0
        if len(self.coefs) > 64:
            raise SystemExit('more than 64 coefficients')
        placed = []
        for s in self.steps:
            # (the memory accesses on the odd steps: a step that keeps the accumulator before)
            if ('mrd' in s or 'mwt' in s) and len(placed) % 2 == 0:
                placed.append({'src': '(keep)', 'b': 'acc', 'keep': True})
            placed.append(dict(s))
        if len(placed) > 128:
            raise SystemExit('%d steps: more than 128' % len(placed))
        reads_prev = set()
        writes = set()
        iwt = {}
        for i, s in enumerate(placed):
            for k in ('x', 'b'):
                if s.get(k, '').startswith('t'):
                    _, n, prev = self.temp(s[k])
                    if prev:
                        reads_prev.add(n + 1)
            if 'twt' in s:
                _, n, _ = self.temp(s['twt'], False)
                writes.add(n)
            if 'mrd' in s:
                target = s['mrd'].split('>')
                if len(target) != 2:
                    raise SystemExit('step %d (%s): mrd=LINE>MEMS' % (i, s['src']))
                if i + 2 >= len(placed):
                    placed.extend({'src': '(keep)', 'b': 'acc', 'keep': True} for _ in range(i + 3 - len(placed)))
                if i + 2 in iwt:
                    raise SystemExit('step %d: 2 reads land on the same step' % i)
                iwt[i + 2] = int(target[1])
        clash = reads_prev & writes
        if clash:
            raise SystemExit("temporary registers %s written and used as t%d' (the previous sample)" %
                             (sorted(clash), min(clash) - 1))
        # (a MEMS read after the step where the memory read lands, a temporary register of this sample read after the
        # step that writes it)
        written_temps = set()
        for i, s in enumerate(placed):
            if s.get('ira', '').startswith('mems'):
                k = int(s['ira'][4:])
                if not any(j < i and v == k for j, v in iwt.items()):
                    raise SystemExit('step %d (%s): MEMS %d read before its memory read lands' % (i, s['src'], k))
            for key in ('x', 'b'):
                if s.get(key, '').startswith('t'):
                    _, n, prev = self.temp(s[key])
                    if not prev and n not in written_temps:
                        raise SystemExit('step %d (%s): t%d read before it is written' % (i, s['src'], n))
            if 'twt' in s:
                written_temps.add(self.temp(s['twt'])[1])
        self.words = []
        self.placed = placed
        for i, s in enumerate(placed):
            w0 = w1 = w2 = w3 = 0
            # word 0: TRA (14-8), TWT (7), TWA (6-0)
            tra = 0
            for k in ('x', 'b'):
                if s.get(k, '').startswith('t'):
                    a, _, _ = self.temp(s[k])
                    if 'tra' in s and s['tra'] != a:
                        raise SystemExit('step %d (%s): X and B read 2 temporary registers' % (i, s['src']))
                    s['tra'] = a
                    tra = a
            w0 |= tra << 8
            if 'twt' in s:
                a, _, _ = self.temp(s['twt'], False)
                w0 |= 0x80 | a
            # word 1: XSEL (15), YSEL (14-13), IRA (11-6), IWT (5), IWA (4-0)
            if s.get('x') == 'in':
                w1 |= 1 << 15
            ysel = 1    # coefficient
            w1 |= ysel << 13
            ira = 0x32  # (nothing: INPUTS kept)
            if 'ira' in s:
                v = s['ira']
                if v.startswith('mixs'):
                    ira = 0x20 + int(v[4:])
                elif v.startswith('mems'):
                    ira = int(v[4:])
                else:
                    raise SystemExit('step %d: ira=mixsN or memsN' % i)
            w1 |= ira << 6
            if i in iwt:
                w1 |= 0x20 | iwt[i]
            # word 2: TABLE (15), MWT (14), MRD (13), EWT (12), EWA (11-8), ADRL (7), FRCL (6), SHIFT (5-4), YRL (3),
            # NEGB (2), ZERO (1), BSEL (0)
            if 'mwt' in s:
                w2 |= 1 << 14
            if 'mrd' in s:
                w2 |= 1 << 13
            if 'ewt' in s:
                w2 |= (1 << 12) | (int(s['ewt']) << 8)
            if s.get('shift') == '1':
                w2 |= 1 << 4
            if s.get('neg'):
                w2 |= 1 << 2
            b = s.get('b', '0')
            if b == '0':
                w2 |= 1 << 1
            elif b == 'acc':
                w2 |= 1
            elif not b.startswith('t'):
                raise SystemExit('step %d: b=acc, b=tN or b=0' % i)
            # word 3: NOFL (15), COEF (14-9), MASA (6-2), ADREB (1), NXADR (0)
            w3 |= self.coef_index(s.get('y', 'zero')) << 9
            if 'mwt' in s or 'mrd' in s:
                if 'mwt' in s and 'mrd' in s:
                    raise SystemExit('step %d: a read and a write' % i)
                w3 |= self.address((s.get('mwt') or s['mrd']).split('>')[0]) << 2
            s['words'] = (w0, w1, w2, w3)
            self.words.append((w0, w1, w2, w3))
        return self

    # ---------------------------------------------------------------------------------------------------------------
    def header(self, name):
        upper = name.upper()
        out = ['/* Generated by Compiler/COMMON/scsp_dsp_asm.py from jo_engine/dsp/%s.scsp: do not edit */' % name, '']
        out.append('/** @brief Steps of the program (%d used, 128 at most), 4 words each */' % len(self.words))
        out.append('# define JO_SCSP_DSP_%s_STEPS (%d)' % (upper, len(self.words)))
        out.append('/** @brief Ring buffer (words): RBL of the common register 0x402 */')
        out.append('# define JO_SCSP_DSP_%s_RING_WORDS (%d)' % (upper, self.ring))
        for k, c in enumerate(self.coefs):
            out.append('# define JO_SCSP_DSP_%s_COEF_%s (%d)' % (upper, c.upper(), k))
        out.append('# define JO_SCSP_DSP_%s_COEF_COUNT (%d)' % (upper, len(self.coefs)))
        out.append('')
        out.append('static const unsigned short jo_scsp_dsp_%s_program[JO_SCSP_DSP_%s_STEPS * 4] =' % (name, upper))
        out.append('{')
        for k, w in enumerate(self.words):
            out.append('    0x%04X, 0x%04X, 0x%04X, 0x%04X, // %3d %s' % (w + (k, self.placed[k]['src'])))
        out.append('};')
        out.append('')
        out.append('/** @brief Memory addresses (words, in the ring buffer) */')
        out.append('static const unsigned short jo_scsp_dsp_%s_addresses[%d] =' % (name, len(self.madrs)))
        out.append('{')
        for v, n in zip(self.madrs, self.madrs_names):
            out.append('    0x%04X, // %s' % (v, n))
        out.append('};')
        out.append('')
        out.append('/** @brief The fixed coefficients (the others are 0 here: set at run time) */')
        out.append('static const short jo_scsp_dsp_%s_coefs[JO_SCSP_DSP_%s_COEF_COUNT] =' % (name, upper))
        out.append('{')
        for c in self.coefs:
            v = to_coef(self.consts[c]) if c in self.consts else 0
            out.append('    %d, // %s' % (v, c))
        out.append('};')
        out.append('')
        return '\n'.join(out)


def to_coef(value):
    v = int(round(value * COEF_ONE))
    return max(-4096, min(4095, v))


# -------------------------------------------------------------------------------------------------------------------
# Simulator (the semantics of Mednafen)
# -------------------------------------------------------------------------------------------------------------------
def sx(bits, v):
    v &= (1 << bits) - 1
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def lzcount32(v):
    v &= 0xFFFFFFFF
    return 32 - v.bit_length()


def int_to_dspfloat(inv):
    """24 bits -> the float format of the ring buffer (Mednafen int_to_dspfloat())"""
    invsl8 = (inv << 8) & 0xFFFFFFFF
    sign_xor = (s32(invsl8) >> 31) & 0xFFFFFFFF
    exp = lzcount32((((invsl8 ^ sign_xor) << 1) & 0xFFFFFFFF) | (1 << 19))
    shift = exp - (1 if exp == 12 else 0)
    ret = (s32(invsl8) >> (19 - shift)) & 0xFFFFFFFF
    ret &= 0x87FF
    ret |= exp << 11
    return ret & 0xFFFF


def dspfloat_to_int(inv):
    """The float format of the ring buffer -> 24 bits (Mednafen dspfloat_to_int())"""
    sign_xor = (s32((inv & 0x8000) << 16) >> 1) & 0xFFFFFFFF
    exp = (inv >> 11) & 0xF
    ret = inv & 0x7FF
    if exp < 12:
        ret |= 0x800
    ret = (ret << (11 + 8)) & 0xFFFFFFFF
    ret ^= sign_xor
    return s32(ret) >> (8 + min(11, exp))


class Simulator:
    def __init__(self, prog, coefs):
        self.p = prog
        self.coef = [0] * 64
        for k, c in enumerate(prog.coefs):
            v = coefs.get(c, prog.consts.get(c, 0.0))
            self.coef[k] = to_coef(v)
        self.temp = [0] * 128
        self.mems = [0] * 32
        # (0x6000: 0 in the float format of the ring buffer; 0x0000 is +0.5: the engine fills the ring buffer with 0x6000)
        self.ram = [0x6000] * prog.ring
        self.mdec = 0
        self.sft = 0
        self.inputs = 0
        self.read_value = 0
        self.pending = None
        self.efreg = [0] * 16
        self.max_abs = 0

    def run(self, mixs0):
        mixs = [0] * 16
        mixs[0] = mixs0
        for step, (w0, w1, w2, w3) in enumerate(self.p.words):
            tra = (w0 >> 8) & 0x7F
            twt = (w0 >> 7) & 1
            twa = w0 & 0x7F
            xsel = (w1 >> 15) & 1
            ira = (w1 >> 6) & 0x3F
            iwt = (w1 >> 5) & 1
            iwa = w1 & 0x1F
            mwt = (w2 >> 14) & 1
            mrd = (w2 >> 13) & 1
            ewt = (w2 >> 12) & 1
            ewa = (w2 >> 8) & 0xF
            shift = (w2 >> 4) & 3
            negb = (w2 >> 2) & 1
            zero = (w2 >> 1) & 1
            bsel = w2 & 1
            coef = (w3 >> 9) & 0x3F
            masa = (w3 >> 2) & 0x1F
            if ira & 0x20:
                if not (ira & 0x10):
                    self.inputs = sx(20, mixs[ira & 0xF]) << 4
            else:
                self.inputs = self.mems[ira & 0x1F]
            shft0 = shift & 1
            shft1 = (shift >> 1) & 1
            so = sx(26, self.sft) << (shft0 ^ shft1)
            if not shft1:
                so = max(-0x800000, min(0x7FFFFF, so))
            so = sx(24, so)
            t = self.temp[(tra + self.mdec) & 0x7F]
            x = self.inputs if xsel else t
            y = sx(13, self.coef[coef])
            product = (y * x) >> 12
            sga = self.sft if bsel else t
            sga = sx(26, sga)
            if negb:
                sga = -sga
            if zero:
                sga = 0
            self.sft = (product + sga) & 0x3FFFFFF
            if ewt:
                self.efreg[ewa] = so >> 8
            if twt:
                self.temp[(twa + self.mdec) & 0x7F] = so
            if iwt:
                self.mems[iwa] = self.read_value
            if self.pending is not None:
                kind, addr, value = self.pending
                if kind == 'r':
                    self.read_value = dspfloat_to_int(self.ram[addr])
                else:
                    self.ram[addr] = value
                self.pending = None
            addr = (self.p.madrs[masa] if masa < len(self.p.madrs) else 0) + self.mdec
            addr &= self.p.ring - 1
            if mrd:
                self.pending = ('r', addr, 0)
            if mwt:
                self.pending = ('w', addr, int_to_dspfloat(so))
            self.max_abs = max(self.max_abs, abs(sx(24, so)))
        if self.mdec == 0:
            self.mdec = self.p.ring
        self.mdec -= 1
        return list(self.efreg)


def load(path):
    p = Program()
    with open(path) as f:
        p.parse(f.read())
    return p.assemble()


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == '--test':
        p = load(sys.argv[2])
        print('%d steps, %d addresses, %d coefficients, %d words of delay lines (ring %d)' %
              (len(p.words), len(p.madrs), len(p.coefs), p.next_addr, p.ring))
        return
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    p = load(sys.argv[1])
    with open(sys.argv[2], 'w', newline='\r\n') as f:
        f.write(p.header(sys.argv[3]))
    print('%s: %d steps, %d addresses, %d coefficients' % (sys.argv[2], len(p.words), len(p.madrs), len(p.coefs)))


if __name__ == '__main__':
    main()
