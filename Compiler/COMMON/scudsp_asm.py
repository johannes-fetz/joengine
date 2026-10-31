#!/usr/bin/env python3
"""Minimal SCU DSP assembler (Sega Saturn).

Syntax (one instruction per line, ';' starts a comment, 'label:' defines a label):

  Operation command: any combination of these fields, separated by spaces:
    ALU:   nop and or xor add sub ad2 sr rr sl rl rl8
    X bus: x=M0..M3|MC0..MC3         (MOV [s],X)
           p=mul                     (MOV MUL,P)
           p=M0..MC3                 (MOV [s],P)
    Y bus: y=M0..MC3                 (MOV [s],Y)
           a=0                       (CLR A)
           a=alu                     (MOV ALU,A)
           a=M0..MC3                 (MOV [s],A)
    D1 bus: d1=#imm,DST              (MOV SImm,[d])  imm: -128..127
            d1=SRC,DST               (MOV [s],[d])   SRC: M0..MC3, ALL, ALH
           DST: MC0..MC3 RX PL RA0 WA0 LOP TOP CT0..CT3
  mvi #imm,DST       DST: MC0..MC3 RX PL RA0 WA0 LOP PC   (imm: signed 25 bits)
  dma d0,MCn,#count  (HWRAM / B-bus -> data RAM, RA0 address / 4, +4 bytes)
  dma MCn,d0,#count  (data RAM -> HWRAM, WA0 address / 4)
  dmah ...           (same, but RA0 / WA0 are not updated)
  jmp label
  btm                (loop: if LOP != 0, LOP -= 1 and jump to TOP)
  lps                (repeat the next instruction LOP + 1 times)
  end / endi

Encodings (checked against the disassembly of the DSP program of a commercial game, see segaxtreme):
  c001010f = dma D0,MC1,0x0f   c001133c = dma MC3,D0,0x3c
  00071a0e = mov MC0,A + mov 0x0e,LOP   00001b30 = mov 0x30,TOP
"""
import re
import sys

ALU = {'nop': 0, 'and': 1, 'or': 2, 'xor': 3, 'add': 4, 'sub': 5, 'ad2': 6, 'sr': 8, 'rr': 9, 'sl': 10, 'rl': 11, 'rl8': 15}
SRC = {'m0': 0, 'm1': 1, 'm2': 2, 'm3': 3, 'mc0': 4, 'mc1': 5, 'mc2': 6, 'mc3': 7}
D1SRC = dict(SRC, all=9, alh=10)
D1DST = {'mc0': 0, 'mc1': 1, 'mc2': 2, 'mc3': 3, 'rx': 4, 'pl': 5, 'ra0': 6, 'wa0': 7, 'lop': 10, 'top': 11,
         'ct0': 12, 'ct1': 13, 'ct2': 14, 'ct3': 15}
MVIDST = {'mc0': 0, 'mc1': 1, 'mc2': 2, 'mc3': 3, 'rx': 4, 'pl': 5, 'ra0': 6, 'wa0': 7, 'lop': 10, 'pc': 12}
RAM = {'mc0': 0, 'mc1': 1, 'mc2': 2, 'mc3': 3}


LABELS = {}


def num(s):
    s = s.strip().lstrip('#')
    if s in LABELS:
        return LABELS[s]
    return int(s, 0)


def assemble_op(fields):
    w = 0
    alu_set = x_set = y_set = d1_set = False
    for f in fields:
        if f in ALU:
            assert not alu_set, 'ALU twice'
            w |= ALU[f] << 26
            alu_set = True
            continue
        key, _, val = f.partition('=')
        if key == 'x':
            assert not (w & (1 << 25)), 'x twice'
            w |= (1 << 25) | (SRC[val] << 20)
        elif key == 'p':
            assert not (w & (3 << 23)), 'p twice'
            if val == 'mul':
                w |= 2 << 23
            else:
                assert not (w & (1 << 25)) or ((w >> 20) & 7) == SRC[val], 'x and p sources share the X bus field'
                w |= (3 << 23) | (SRC[val] << 20)
        elif key == 'y':
            w |= (1 << 19) | (SRC[val] << 14)
        elif key == 'a':
            assert not (w & (3 << 17)), 'a twice'
            if val == '0':
                w |= 1 << 17
            elif val == 'alu':
                w |= 2 << 17
            else:
                w |= (3 << 17) | (SRC[val] << 14)
        elif key == 'd1':
            assert not d1_set, 'd1 twice'
            src, dst = val.split(',')
            if src.startswith('#'):
                imm = num(src)
                assert -128 <= imm <= 127
                w |= (1 << 12) | (D1DST[dst] << 8) | (imm & 0xFF)
            else:
                w |= (3 << 12) | (D1DST[dst] << 8) | D1SRC[src]
            d1_set = True
        else:
            raise ValueError('unknown field ' + f)
    return w


def assemble(text):
    lines = []
    labels = {}
    for raw in text.splitlines():
        line = raw.split(';')[0].strip().lower()
        if not line:
            continue
        m = re.match(r'^([a-z_][a-z0-9_]*):\s*(.*)$', line)
        if m:
            labels[m.group(1)] = len(lines)
            line = m.group(2).strip()
            if not line:
                continue
        lines.append(line)
    LABELS.clear()
    LABELS.update(labels)
    out = []
    for pc, line in enumerate(lines):
        parts = line.split(None, 1)
        op = parts[0]
        arg = parts[1].replace(' ', '') if len(parts) > 1 else ''
        if op == 'mvi':
            imm, dst = arg.split(',')
            v = num(imm)
            assert -(1 << 24) <= v < (1 << 24), 'mvi immediate out of range'
            w = (2 << 30) | (MVIDST[dst] << 26) | (v & 0x1FFFFFF)
        elif op in ('dma', 'dmah'):
            a, b, c = arg.split(',')
            if c in SRC:
                # Count read from the data RAM: bit 13, bank in bits 1-0, CT increment in bit 2
                w = (3 << 30) | (2 << 15) | (1 << 13) | SRC[c]
            else:
                count = num(c)
                assert 0 <= count <= 255
                w = (3 << 30) | (2 << 15) | count
            if op == 'dmah':
                w |= 1 << 14
            if a == 'd0':
                w |= RAM[b] << 8
            else:
                assert b == 'd0'
                w |= (1 << 12) | (RAM[a] << 8)
        elif op == 'jmp':
            cond = 0
            if ',' in arg:
                c, arg = arg.split(',')
                # Condition: 0x40 conditional, 0x20 jump when the flag is set, Z 1 S 2 C 4 T0 8
                flags = {'z': 1, 's': 2, 'c': 4, 't0': 8}
                if c.startswith('n'):
                    cond = 0x40 | flags[c[1:]]
                else:
                    cond = 0x60 | flags[c]
            w = 0xD0000000 | (cond << 19) | (labels[arg] if arg in labels else num(arg))
        elif op == 'btm':
            w = 0xE0000000
        elif op == 'lps':
            w = 0xE8000000
        elif op == 'end':
            w = 0xF0000000
        elif op == 'endi':
            w = 0xF8000000
        else:
            w = assemble_op(line.split())
        out.append(w)
    assert len(out) <= 256, 'program too long'
    return out, labels


def to_c(name, words):
    body = ',\n'.join('    0x%08X' % w for w in words)
    return 'static const unsigned int %s[%d] =\n{\n%s\n};\n' % (name, len(words), body)


if __name__ == '__main__':
    # Self test with the known encodings
    assert assemble('dma d0,mc1,#0x0f')[0] == [0xC001010F]
    assert assemble('dma mc3,d0,#0x3c')[0] == [0xC001133C]
    assert assemble('a=mc0 d1=#0x0e,lop')[0] == [0x00071A0E]
    assert assemble('d1=#0x30,top')[0] == [0x00001B30]
    out = []
    for path in sys.argv[2:]:
        name = re.sub(r'\W', '_', path.rsplit('/', 1)[-1].rsplit('.', 1)[0])
        words, labels = assemble(open(path).read())
        out.append(to_c('dsp_' + name, words))
        for k, v in labels.items():
            out.append('#define DSP_%s_%s (%d)\n' % (name.upper(), k.upper(), v))
    open(sys.argv[1], 'w').write('/* Generated by tools/scudsp_asm.py */\n' + '\n'.join(out))
    print('ok')
