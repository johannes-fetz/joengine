#!/usr/bin/env python3
"""The functions of a game that call the slow routines of libgcc: the SH-2 has no FPU, no barrel shifter and only a step
of division, so the compiler calls a routine for each float or double operation, each division or modulo by a variable,
each shift by a variable count (and some signed shifts by a constant), each 64 bits division.

    python3 tools/slow_calls.py game.elf

Reads game.elf alone (no tool of the SH-2 needed). For each routine, the functions that call it: their own code, or a
function inlined in them (the link time optimization inlines a lot: a function of the engine can show up under the
function of the game that calls it). The functions of the engine called once (initialization, loading) are fine: look
at the functions of the game, and at the functions called at each frame (see the profiler, profile.h, to know which ones
take time). The fixes: jo_fixed instead of float (jo/math.h), an inverse computed once or jo_fixed_div() instead of a
division in a loop, a constant shift, a table or a multiplication instead of a shift by a variable count.
"""

import struct
import sys

# The routines of libgcc, by kind (the names of the ELF: an underscore before the C name)
KINDS = (
    ('float or double (emulated in software)', (
        'addsf3', 'subsf3', 'mulsf3', 'divsf3', 'negsf2', 'cmpsf2', 'eqsf2', 'nesf2', 'gtsf2', 'gesf2', 'ltsf2', 'lesf2',
        'unordsf2', 'adddf3', 'subdf3', 'muldf3', 'divdf3', 'negdf2', 'cmpdf2', 'eqdf2', 'nedf2', 'gtdf2', 'gedf2', 'ltdf2',
        'ledf2', 'unorddf2', 'fixsfsi', 'fixunssfsi', 'fixdfsi', 'fixunsdfsi', 'fixsfdi', 'fixunssfdi', 'fixdfdi',
        'fixunsdfdi', 'floatsisf', 'floatunsisf', 'floatsidf', 'floatunsidf', 'floatdisf', 'floatundisf', 'floatdidf',
        'floatundidf', 'extendsfdf2', 'truncdfsf2', 'powisf2', 'powidf2')),
    ('division or modulo by a variable (32 bits)', ('sdivsi3', 'udivsi3', 'sdivsi3_i4', 'udivsi3_i4', 'modsi3', 'umodsi3')),
    ('64 bits division, modulo or product', ('divdi3', 'udivdi3', 'moddi3', 'umoddi3', 'muldi3', 'udiv_qrnnd_16')),
    ('shift by a variable count', ('ashlsi3', 'ashlsi3_r0', 'ashrsi3', 'ashrsi3_r0', 'lshrsi3', 'lshrsi3_r0', 'ashldi3',
                                   'ashrdi3', 'lshrdi3')),
    ('signed shift right by a constant (___ashiftrt_r4_N: shifts by N, one bit at a time)', ()),
)


def kind_of(name):
    """The index of the kind of a routine of libgcc (its ELF name), None if it is not one"""
    if not name.startswith('___'):
        return None
    base = name[3:]
    if base.startswith('ashiftrt_r4_'):
        return len(KINDS) - 1
    for k, (_, names) in enumerate(KINDS):
        if base in names:
            return k
    return None


def read_elf(path):
    """The executable sections (address, bytes) and the functions (address, size, name) of a big endian ELF of 32 bits"""
    data = open(path, 'rb').read()
    if data[:4] != b'\x7fELF' or data[4] != 1 or data[5] != 2:
        sys.exit('%s: not an ELF of the SH-2 (32 bits, big endian)' % path)
    shoff, = struct.unpack_from('>I', data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('>HHH', data, 0x2E)
    sections = [struct.unpack_from('>IIIIIIIIII', data, shoff + i * shentsize) for i in range(shnum)]
    code = []
    functions = []
    for name, stype, flags, addr, offset, size, link, info, align, entsize in sections:
        # SHT_PROGBITS with SHF_EXECINSTR: code
        if stype == 1 and flags & 4 and size > 0:
            code.append((addr, data[offset:offset + size]))
        # SHT_SYMTAB: the symbols, their names in the section of its link
        if stype == 2:
            strings = sections[link]
            for k in range(size // 16):
                st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from('>IIIBBH', data, offset + k * 16)
                if st_info & 0xF != 2 and not (st_info & 0xF == 0 and st_shndx != 0):
                    continue
                start = strings[4] + st_name
                end = data.index(b'\0', start)
                functions.append((st_value, st_size, data[start:end].decode('ascii', 'replace')))
    return code, functions


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    code, symbols = read_elf(sys.argv[1])
    routines = {}
    for addr, size, name in symbols:
        k = kind_of(name)
        if k is not None:
            routines[addr] = (k, name)
    # (not the routines of libgcc themselves: __pack_f, __unpack_f... call the others)
    functions = sorted((a, s, n) for a, s, n in symbols if s > 0 and kind_of(n) is None)
    library = set(n for a, s, n in functions if n.startswith('___') and not n.startswith('___jo_'))
    starts = [a for a, _, _ in functions]

    def function_at(address):
        lo, hi = 0, len(starts)
        while lo < hi:
            mid = (lo + hi) // 2
            if starts[mid] <= address:
                lo = mid + 1
            else:
                hi = mid
        if lo == 0:
            return None
        a, s, n = functions[lo - 1]
        return n if address < a + s else None

    callers = {}
    for base, blob in code:
        # The addresses in the constant pools (mov.l @(disp, pc) then jsr) and the near calls (bsr, after --relax)
        for off in range(0, len(blob) - 3, 2):
            word, = struct.unpack_from('>I', blob, off)
            if word in routines and (base + off) % 4 == 0:
                caller = function_at(base + off)
                if caller is not None and caller not in library:
                    callers.setdefault(word, set()).add(caller)
        for off in range(0, len(blob) - 1, 2):
            op, = struct.unpack_from('>H', blob, off)
            if op & 0xF000 == 0xB000:
                disp = op & 0xFFF
                if disp & 0x800:
                    disp -= 0x1000
                target = base + off + 4 + disp * 2
                if target in routines:
                    caller = function_at(base + off)
                    if caller is not None and caller not in library:
                        callers.setdefault(target, set()).add(caller)
    found = False
    for k, (title, _) in enumerate(KINDS):
        rows = sorted((routines[a][1], sorted(c)) for a, c in callers.items() if routines[a][0] == k)
        if not rows:
            continue
        found = True
        print('%s:' % title)
        for name, names in rows:
            print('  %s: %s' % (name[1:], ', '.join(n[1:] if n.startswith('_') else n for n in names)))
    if not found:
        print('No call of a slow routine of libgcc.')


if __name__ == '__main__':
    main()
