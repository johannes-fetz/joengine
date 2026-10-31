"""Compression of the data kept in RAM by a game (jo_lz_decompress() of the Jo Engine): LZSS, the size first (4 bytes,
big endian), then groups of a byte of 8 flags (the high bit first) and 8 items: flag 1 a byte as it is, flag 0 a copy
of 3 to 18 bytes from 1 to 4096 bytes before (2 bytes: 12 bits distance - 1, 4 bits length - 3).

    python3 jo_lz.py IN OUT
    import jo_lz; data = jo_lz.compress(raw)
"""
import struct
import sys

WINDOW = 4096
MIN_MATCH = 3
MAX_MATCH = 18
CANDIDATES = 64


def compress(raw):
    raw = bytes(raw)
    n = len(raw)
    out = bytearray(struct.pack('>I', n))
    heads = {}
    i = 0
    flags_at = -1
    count = 8
    while i < n:
        if count == 8:
            flags_at = len(out)
            out.append(0)
            count = 0
        best_len, best_dist = 0, 0
        if i + MIN_MATCH <= n:
            key = raw[i:i + MIN_MATCH]
            for j in reversed(heads.get(key, ())[-CANDIDATES:]):
                if i - j > WINDOW:
                    break
                k = MIN_MATCH
                limit = min(MAX_MATCH, n - i)
                while k < limit and raw[j + k] == raw[i + k]:
                    k += 1
                if k > best_len:
                    best_len, best_dist = k, i - j
                    if k == limit:
                        break
        if best_len >= MIN_MATCH:
            v = ((best_dist - 1) << 4) | (best_len - MIN_MATCH)
            out += struct.pack('>H', v)
            step = best_len
        else:
            out[flags_at] |= 0x80 >> count
            out.append(raw[i])
            step = 1
        for p in range(i, min(i + step, n - MIN_MATCH + 1)):
            heads.setdefault(raw[p:p + MIN_MATCH], []).append(p)
        i += step
        count += 1
    return bytes(out)


def decompress(data):
    n = struct.unpack('>I', data[:4])[0]
    out = bytearray()
    s = 4
    flags, bits = 0, 0
    while len(out) < n:
        if bits == 0:
            flags, bits = data[s], 8
            s += 1
        if flags & 0x80:
            out.append(data[s])
            s += 1
        else:
            v = (data[s] << 8) | data[s + 1]
            s += 2
            m = len(out) - ((v >> 4) + 1)
            for k in range((v & 15) + MIN_MATCH):
                if len(out) >= n:
                    break
                out.append(out[m + k])
        flags = (flags << 1) & 0xFF
        bits -= 1
    return bytes(out)


if __name__ == '__main__':
    data = open(sys.argv[1], 'rb').read()
    open(sys.argv[2], 'wb').write(compress(data))
