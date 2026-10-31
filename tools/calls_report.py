"""Report of the counts of calls (-finstrument-functions -DJO_PROFILE_CALLS, see profile.c): the functions of each CPU by
calls (the bin of the address of a function: 2 small functions in the same 16 bytes share it)
python3 calls_report.py state elf [top]
nm of the SH-2 (not in Compiler/): sh-elf-nm from the PATH, or the prefix in the variable JO_SH_BINUTILS (see
pcprof_report.py)"""
import gzip, struct, subprocess, sys, collections, os
TC = os.environ.get('JO_SH_BINUTILS', 'sh-elf-')
BIN, BINS, BASE = 16, 16384, 0x06004000
state, elf = sys.argv[1], sys.argv[2]
top = int(sys.argv[3]) if len(sys.argv) > 3 else 40
d = gzip.open(state).read()
o = d.index(b'\x08WorkRAML') + 1 + 8 + 4
lw = d[o:o + 0x100000]
nm = subprocess.run([TC + 'nm', '-n', elf], capture_output=True, text=True).stdout
syms = collections.defaultdict(list)
for line in nm.split('\n'):
    p = line.split()
    if len(p) == 3 and p[1].lower() in ('t', 'w'):
        a = int(p[0], 16)
        if BASE <= a < BASE + BIN * (BINS - 1):
            syms[(a - BASE) // BIN].append(p[2])
for cpu, off in (('master', 0xE0000), ('slave', 0xF0000)):
    words = struct.unpack('<%dH' % (BINS * 2), lw[off:off + BINS * 4])
    h = [(words[2 * i] << 16) | words[2 * i + 1] for i in range(BINS)]
    rows = sorted(((h[b], '/'.join(n)) for b, n in syms.items() if h[b]), reverse=True)
    print('== %s: %d calls' % (cpu, sum(h)))
    for c, n in rows[:top]:
        print('  %9d  %s' % (c, n))
