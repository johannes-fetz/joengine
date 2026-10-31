"""Report of the program counter sampling (jo_profile_sampling_start(), histograms at the end of the LWRAM in a Mednafen
save state): python3 pcprof_report.py state elf [top] [function [lines|asm]]
- no function: the functions (symbols of game.elf) of each CPU, by sample count
- function: the source lines of its address range (game.elf linked with -g: make CC="... -g" _CCFLAGS="-g")
- function asm: its disassembly with the samples of each 16 bytes block
The binutils of the SH-2 (nm, objdump, addr2line: not in Compiler/): from the PATH (sh-elf-nm...), or the prefix in
the variable JO_SH_BINUTILS (for example C:/saturn/bin/sh-elf- or /opt/toolchain/bin/sh-none-elf-)"""
import gzip, struct, subprocess, sys, collections, bisect, os
TC = os.environ.get('JO_SH_BINUTILS', 'sh-elf-')
ADDR2LINE = TC + 'addr2line'
BIN = 16
BINS = 16384
BASE = 0x06004000
OFFSETS = (('master', 0xE0000), ('slave', 0xF0000))
state, elf = sys.argv[1], sys.argv[2]
top = int(sys.argv[3]) if len(sys.argv) > 3 else 25
func = sys.argv[4] if len(sys.argv) > 4 else None
mode = sys.argv[5] if len(sys.argv) > 5 else 'lines'
d = gzip.open(state).read()
o = d.index(b'\x08WorkRAML') + 1 + 8 + 4
lw = d[o:o + 0x100000]

def hist(off):
    words = struct.unpack('<%dH' % (BINS * 2), lw[off:off + BINS * 4])
    return [(words[2 * i] << 16) | words[2 * i + 1] for i in range(BINS)]

nm = subprocess.run([TC + 'nm', '-n', '-S', elf], capture_output=True, text=True).stdout
syms = []
for line in nm.split('\n'):
    p = line.split()
    if len(p) >= 3 and p[-2].lower() in ('t', 'w'):
        syms.append((int(p[0], 16), int(p[1], 16) if len(p) >= 4 else 0, p[-1]))
syms.sort()
addrs = [a for a, _, _ in syms]

def name(a):
    i = bisect.bisect_right(addrs, a) - 1
    return syms[i][2] if i >= 0 else '?'

if func:
    m = [s for s in syms if s[2] == func or s[2] == '_' + func or s[2].startswith('_' + func + '.')]
    if not m:
        m = [s for s in syms if func in s[2]]
    if not m:
        sys.exit('no symbol ' + func)
    lo, hi = m[0][0], m[0][0] + m[0][1]
    if m[0][1] == 0:
        # No size (assembly): up to the next symbol
        hi = min([a for a, _, _ in syms if a > lo] or [lo + 4096])
    print('%s: 0x%x-0x%x (%d bytes)' % (m[0][2], lo, hi, m[0][1]))
for cpu, off in OFFSETS:
    h = hist(off)
    total = sum(h)
    print('== %s: %d samples' % (cpu, total))
    if not total:
        continue
    if not func:
        by = collections.Counter()
        for i, c in enumerate(h[:-1]):
            if c:
                by[name(BASE + i * BIN)] += c
        by['(outside code / idle loop?)'] += h[-1]
        for n, c in by.most_common(top):
            print('  %5.1f %%  %6d  %s' % (100.0 * c / total, c, n))
        continue
    idx = [i for i in range(BINS - 1) if h[i] and lo <= BASE + i * BIN < hi]
    counts = [h[i] for i in idx]
    print('  %d samples in the range (%.1f %%)' % (sum(counts), 100.0 * sum(counts) / total))
    if mode == 'asm':
        dis = subprocess.run([TC + 'objdump', '-d', '--start-address=0x%x' % lo, '--stop-address=0x%x' % hi, elf],
                             capture_output=True, text=True).stdout.split('\n')
        for line in dis:
            p = line.split(':')
            try:
                a = int(p[0].strip(), 16)
            except ValueError:
                continue
            c = h[(a - BASE) // BIN] if (a - BASE) % BIN == 0 else 0
            print('  %6s  %s' % (str(c) if c else '', line.strip()[:110]))
        continue
    r = subprocess.run([ADDR2LINE, '-f', '-e', elf] + ['0x%x' % (BASE + i * BIN + BIN // 2) for i in idx],
                       capture_output=True, text=True).stdout.split('\n')
    res = collections.Counter()
    for k, c in enumerate(counts):
        fn, loc = r[2 * k], r[2 * k + 1]
        res[(fn, loc.split('/')[-1].split(' ')[0])] += c
    for (fn, loc), c in res.most_common(top):
        print('  %5.2f %%  %6d  %-40s %s' % (100.0 * c / total, c, fn[:40], loc))
