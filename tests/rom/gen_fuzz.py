#!/usr/bin/env python3
"""Jo Engine regression test ROM: the files of the fuzz tests (t_fuzz.c), written in cd/FUZZ.

The seeds are valid files (built here, or copied from the samples), the variants are mutated copies of them. The
output is the same at each run (a fixed seed per file): a crash found by the ROM is reproduced by the same file.

    python3 tests/rom/gen_fuzz.py           write cd/FUZZ (the seeds and the variants)
    python3 tests/rom/gen_fuzz.py --list    the mutations of each variant (to read the trigger of a failure)

Names (8.3): the seeds SKEL.JOM, CAP.JOM, S24.TGA, S32.TGA, S8.TGA, M16.WAV, ST16.WAV, M8.WAV, SEED.MAP, DEFS.JOL,
LEVEL.JOL, the inputs of the known bugs K*.JOM and KTGA.TGA (CAP.JOM, DEFS.JOL and LEVEL.JOL: copies of the files of the sample "demo - first person shooter", in
fuzz_seeds); the variants Jnn.JOM, Tnn.TGA, Wnn.WAV, Mnn.MAP, Lnn.JOL (nn = 00 to FUZZ_COUNT - 1, see T_FUZZ_FILES in
t_fuzz.c).
"""

import os
import random
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "cd", "FUZZ")
SEEDS = os.path.join(HERE, "fuzz_seeds")
FUZZ_COUNT = 30

U32_VALUES = [0, 1, 2, 3, 4, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF, 0x10000, 0x7FFFFFFF, 0x80000000,
              0xFFFFFFFF, 0xFFFFFFF8, 0xFFFFFFFC, 0x40000000, 0x15555556, 0x0CCCCCCD, 0x0AAAAAAB]
U16_VALUES = [0, 1, 2, 7, 8, 9, 0x7F, 0x80, 0xFF, 0x100, 0x1F8, 0x200, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF]


# ----------------------------------------------------------------------------------------------------------------------
# Seeds

def be(fmt, *v):
    return struct.pack(">" + fmt, *v)


def chunk(tag, data):
    assert len(data) % 4 == 0
    return tag.encode() + be("I", len(data)) + data


def pad4(b):
    return b + b"\0" * (-len(b) % 4)


def name16(s):
    return s.encode().ljust(16, b"\0")


def jom_skeleton():
    """A small model with every chunk: 2 embedded textures (RGB and 16 colors), 2 groups, 2 bones, 2 animations."""
    fx = 0x10000
    points = [(-fx, -fx, 0), (fx, -fx, 0), (fx, fx, 0), (-fx, fx, 0),
              (-fx, -fx, 2 * fx), (fx, -fx, 2 * fx), (fx, fx, 2 * fx), (-fx, fx, 2 * fx)]
    quads = [((0, 0, -fx), (0, 1, 2, 3)), ((0, 0, fx), (4, 5, 6, 7))]
    timg = be("I", 2)
    # RGB 555, 8 x 8
    timg += name16("SKRGB") + be("HHHH", 8, 8, 0, 0) + b"".join(be("H", 0x8000 | (i * 517) & 0x7FFF) for i in range(64))
    # 16 colors, 8 x 8: the table, then 2 pixels per byte
    timg += name16("SK16") + be("HHHH", 8, 8, 1, 0) + b"".join(be("H", 0x8000 | i * 2047 & 0x7FFF) for i in range(16))
    timg += bytes((i * 37) & 0xFF for i in range(32))
    out = b"JOM1" + be("HH", 1, 0) + be("iiii", 0, 0, fx, 2 * fx)
    out += chunk("TIMG", pad4(timg))
    out += chunk("PNTS", be("I", len(points)) + b"".join(be("iii", *p) for p in points))
    out += chunk("QUAD", be("I", len(quads)) + b"".join(be("iii", *n) + be("HHHH", *v) for n, v in quads))
    out += chunk("QMAT", pad4(be("I", 2) + be("HH", 0, 1)))
    # Materials: gouraud + end codes + mipmaps, light + subdivision + half transparent
    mat = name16("M0") + bytes([200, 100, 50, 10]) + be("hH", 0, 0) + be("II", (1 << 3) | (1 << 9) | (1 << 10), 7)
    mat += name16("M1") + bytes([20, 200, 50, 0]) + be("hH", 1, 4) + be("II", (1 << 6) | (1 << 11) | (1 << 4) | 1, 0)
    out += chunk("MATL", be("I", 2) + mat)
    out += chunk("TEXT", be("I", 2) + name16("SKRGB") + name16("SK16"))
    out += chunk("NTAB", be("I", 2) + be("iii", 0, 0, -fx) + be("iii", 0, 0, fx))
    out += chunk("CNRM", pad4(be("I", 8) + be("8H", 0, 0, 0, 0, 1, 1, 1, 1)))
    out += chunk("VCOL", be("I", 8) + b"".join(be("iii", fx, fx // 2, fx // 4) for _ in range(8)))
    out += chunk("GOUR", pad4(be("I", 8) + be("8H", 0x8010, 0x8210, 0x8410, 0x8610, 0x8000, 0x8001, 0x8002, 0x8003)))
    out += chunk("GRUP", be("I", 2) + name16("BODY") + be("HHHH", 0, 4, 0, 1) + name16("ARM") + be("HHHH", 4, 4, 1, 1))
    out += chunk("BONE", be("I", 2) + name16("root") + be("Hh", 0, -1) + be("iii", 0, 0, 0)
                 + name16("arm") + be("Hh", 1, 0) + be("iii", 0, 0, fx))
    # Animations: key = 3 fixed + 2 bones x 3 s16 = 24 bytes
    keys = b""
    for k in range(4):
        keys += be("iii", k * 100, 0, 0) + be("6h", k * 1000, 0, 0, -k * 500, k * 200, 0)
    head = be("I", 2)
    first = 4 + 2 * 28
    head += name16("walk") + be("HHHH", 4, 10, 1, 0) + be("I", first)
    head += name16("jump") + be("HHHH", 2, 30, 0, 0) + be("I", first + 2 * 24)
    out += chunk("ANIM", head + keys)
    out += chunk("END ", b"")
    return out


def jom_replace_chunk(data, tag, new_chunks):
    """A JOM file with a chunk replaced by others (bytes of whole chunks)"""
    out = data[:24]
    for p, t, size in jom_chunks(data):
        out += new_chunks if t == tag.encode() else data[p:p + 8 + size]
    return out


def jom_known_bugs(skel):
    """The inputs of the bugs found by the fuzz and by the review of model.c (K*.JOM, see t_fuzz_known_bugs())"""
    out = {}
    # 2 TEXT chunks, the first one with more names: jo_3d_create_model_in_place() sizes its block with the last one
    names = name16("SKRGB") + name16("SK16")
    # (a model without anything else allocated: only the block of the model in the heap, see t_heap_probe_check())
    good = skel[:24] + b"".join(skel[p:p + 8 + n] for p, t, n in jom_chunks(skel) if t in (b"PNTS", b"QUAD"))
    out["KTEXT.JOM"] = good + chunk("TEXT", be("I", 40) + names * 20) + chunk("TEXT", be("I", 1) + names[:16]) + chunk("END ", b"")
    # The same model with one TEXT chunk (40 names): the bytes after its block are not changed
    out["KTEXT1.JOM"] = good + chunk("TEXT", be("I", 40) + names * 20) + chunk("END ", b"")
    # An animation whose keys are not aligned on 4 bytes (offset + 2): the fixed values of a key read at an odd address
    p = [c for c in jom_chunks(skel) if c[1] == b"ANIM"][0][0]
    k = bytearray(skel)
    off = struct.unpack(">I", k[p + 8 + 4 + 24:p + 8 + 4 + 28])[0]
    k[p + 8 + 4 + 24:p + 8 + 4 + 28] = be("I", off + 2)
    # (3 keys: they fit in the chunk after the offset + 2, see jo_3d_model_animations_fit())
    k[p + 8 + 4 + 16:p + 8 + 4 + 18] = be("H", 3)
    out["KANIM.JOM"] = bytes(k)
    # A long animation (200 keys of 0xFFFF ticks: 13 million ticks): the time x 256 of jo_3d_model_get_pose() overflows
    keys = b"".join(be("iii", 1000, 0, 0) + be("6h", 0, 0, 0, 0, 0, 0) for _ in range(200))
    head = be("I", 1) + name16("long") + be("HHHH", 200, 0xFFFF, 1, 0) + be("I", 4 + 28)
    out["KLONG.JOM"] = jom_replace_chunk(skel, "ANIM", chunk("ANIM", head + keys))
    # A MATL chunk twice: jo_3d_create_model_from_memory() copies both, the first copy is lost
    m = [c for c in jom_chunks(skel) if c[1] == b"MATL"][0]
    out["KDUP.JOM"] = jom_replace_chunk(skel, "MATL", skel[m[0]:m[0] + 8 + m[2]] * 2)
    # An embedded texture of 0xFFFF x 0xFFFF pixels: its size (x 2) wraps around 32 bits, the next entry is before it
    t = bytearray(skel)
    p = [c for c in jom_chunks(skel) if c[1] == b"TIMG"][0][0]
    t[p + 12 + 16:p + 12 + 20] = be("HH", 0xFFFF, 0xFFFF)
    out["KTIMG.JOM"] = bytes(t)
    # An embedded texture of 0x7FFF x 8 pixels (512 KB): accepted by jo_sprite_add() (its size masked by 0x7FFE0)
    t = bytearray(skel)
    t[p + 12 + 16:p + 12 + 20] = be("HH", 0x7FFF, 8)
    out["KBIG.JOM"] = bytes(t)
    return out


def tga(width, height, bits, descriptor=0x20, image_id=b"", palette=None):
    image_type = 1 if bits == 8 else 2
    cmap_len = len(palette) if palette else 0
    header = struct.pack("<BBBHHBHHHHBB", len(image_id), 1 if palette else 0, image_type, 0, cmap_len, 24 if palette else 0,
                         0, 0, width, height, bits, descriptor)
    body = image_id
    if palette:
        body += b"".join(bytes(c) for c in palette)
    bpp = bits // 8
    body += bytes(((x * 31 + y * 7 + c * 50) & 0xFF) for y in range(height) for x in range(width) for c in range(bpp))
    return header + body


def wav(channels, bits, rate, frames, extra_chunk=False):
    data = bytes((i * 13) & 0xFF for i in range(frames * channels * bits // 8))
    fmt = struct.pack("<HHIIHH", 1, channels, rate, rate * channels * bits // 8, channels * bits // 8, bits)
    body = b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt
    if extra_chunk:
        body += b"LIST" + struct.pack("<I", 5) + b"abcde\0"
    body += b"data" + struct.pack("<I", len(data)) + data
    return b"RIFF" + struct.pack("<I", len(body)) + body


def map_seed():
    lines = []
    for i in range(12):
        lines.append("S8.TGA\t%d\t%d%s" % (i * 8, (i % 3) * 8, "\t%d" % (i % 4) if i % 2 else ""))
    lines.append("@0\t100\t50\t2")
    return ("\r\n".join(lines) + "\n").encode()


def read_sample(name):
    """A seed copied from a sample (demo - first person shooter/cd) in fuzz_seeds"""
    path = os.path.join(SEEDS, name)
    with open(path, "rb") as f:
        return f.read()


# ----------------------------------------------------------------------------------------------------------------------
# Mutations

def jom_chunks(data):
    """(offset, tag, size) of the chunks of a JOM file."""
    out = []
    p = 24
    while p + 8 <= len(data):
        tag = data[p:p + 4]
        size = struct.unpack(">I", data[p + 4:p + 8])[0]
        out.append((p, tag, size))
        if tag == b"END " or size > len(data) - p - 8:
            break
        p += 8 + size
    return out


JOM_TAGS = [b"TIMG", b"PNTS", b"QUAD", b"QMAT", b"MATL", b"TEXT", b"NTAB", b"CNRM", b"VCOL", b"GOUR", b"GRUP", b"BONE",
            b"ANIM", b"END ", b"XXXX"]


def mutate_generic(rng, data, log):
    data = bytearray(data)
    op = rng.randrange(5)
    if op == 0 and data:
        i = rng.randrange(len(data))
        bit = rng.randrange(8)
        data[i] ^= 1 << bit
        log.append("bit %d of byte %d" % (bit, i))
    elif op == 1 and data:
        i = rng.randrange(len(data))
        data[i] = rng.randrange(256)
        log.append("byte %d = 0x%02X" % (i, data[i]))
    elif op == 2 and len(data) >= 4:
        i = rng.randrange(len(data) // 4) * 4
        v = rng.choice(U32_VALUES)
        data[i:i + 4] = struct.pack(">I", v)
        log.append("u32 at %d = 0x%X" % (i, v))
    elif op == 3 and len(data) >= 2:
        i = rng.randrange(len(data) // 2) * 2
        v = rng.choice(U16_VALUES)
        data[i:i + 2] = struct.pack(">H", v)
        log.append("u16 at %d = 0x%X" % (i, v))
    elif len(data) > 1:
        n = rng.randrange(1, len(data))
        del data[n:]
        log.append("truncated to %d" % n)
    return bytes(data)


def mutate_jom(rng, data, log):
    chunks = jom_chunks(data)
    data = bytearray(data)
    op = rng.randrange(7)
    if not chunks or op >= 5:
        return mutate_generic(rng, bytes(data), log)
    p, tag, size = rng.choice(chunks)
    name = tag.decode("latin1")
    if op == 0 and size >= 4:
        v = rng.choice(U32_VALUES + [struct.unpack(">I", data[p + 8:p + 12])[0] + 1])
        data[p + 8:p + 12] = struct.pack(">I", v & 0xFFFFFFFF)
        log.append("%s count = 0x%X" % (name, v & 0xFFFFFFFF))
    elif op == 1:
        v = rng.choice(U32_VALUES + [size + 4, size - 4 if size >= 4 else 0, size + 2])
        data[p + 4:p + 8] = struct.pack(">I", v & 0xFFFFFFFF)
        log.append("%s size = 0x%X" % (name, v & 0xFFFFFFFF))
    elif op == 2:
        t = rng.choice(JOM_TAGS)
        data[p:p + 4] = t
        log.append("%s tag = %s" % (name, t.decode("latin1")))
    elif op == 3:
        whole = bytes(data[p:p + 8 + size])
        data[p:p] = whole
        log.append("%s duplicated" % name)
    elif op == 4 and size >= 8:
        # A field inside the elements (u16 or u32, aligned)
        i = p + 12 + rng.randrange((size - 4) // 2) * 2
        if i + 2 <= len(data):
            v = rng.choice(U16_VALUES)
            data[i:i + 2] = struct.pack(">H", v)
            log.append("%s u16 at +%d = 0x%X" % (name, i - p - 12, v))
    else:
        return mutate_generic(rng, bytes(data), log)
    return bytes(data)


TGA_FIELDS = [("id_length", 0, "B"), ("cmap_type", 1, "B"), ("image_type", 2, "B"), ("cmap_length", 5, "<H"),
              ("cmap_depth", 7, "B"), ("width", 12, "<H"), ("height", 14, "<H"), ("bits", 16, "B"),
              ("descriptor", 17, "B")]


def mutate_tga(rng, data, log):
    if rng.randrange(3) == 0 or len(data) < 18:
        return mutate_generic(rng, data, log)
    data = bytearray(data)
    field, off, fmt = rng.choice(TGA_FIELDS)
    if fmt == "B":
        v = rng.choice([0, 1, 2, 3, 8, 9, 10, 16, 24, 32, 0x20, 0x7F, 0x80, 0xFF])
    else:
        v = rng.choice([0, 1, 7, 8, 9, 255, 256, 504, 512, 0x7FFF, 0x8000, 0xFFFF])
    data[off:off + struct.calcsize(fmt)] = struct.pack(fmt, v)
    log.append("%s = %d" % (field, v))
    return bytes(data)


def wav_chunks(data):
    out = []
    p = 12
    while p + 8 <= len(data):
        size = struct.unpack("<I", data[p + 4:p + 8])[0]
        out.append((p, data[p:p + 4], size))
        p += 8 + ((size + 1) & ~1)
    return out


def mutate_wav(rng, data, log):
    data = bytearray(data)
    chunks = wav_chunks(bytes(data))
    op = rng.randrange(6)
    fmt = [c for c in chunks if c[1] == b"fmt "]
    if op == 0 and chunks:
        p, tag, size = rng.choice(chunks)
        v = rng.choice(U32_VALUES + [size + 1, size - 1])
        data[p + 4:p + 8] = struct.pack("<I", v & 0xFFFFFFFF)
        log.append("%s size = 0x%X" % (tag.decode("latin1"), v & 0xFFFFFFFF))
    elif op == 1 and fmt:
        p = fmt[0][0]
        field, off = rng.choice([("format", 8), ("channels", 10), ("bits", 22), ("block_align", 20)])
        v = rng.choice([0, 1, 2, 3, 8, 16, 17, 0xFFFF])
        data[p + off:p + off + 2] = struct.pack("<H", v)
        log.append("fmt %s = %d" % (field, v))
    elif op == 2 and fmt:
        p = fmt[0][0]
        v = rng.choice([0, 1, 0xFFFF, 0x10000, 0xFFFFFFFF])
        data[p + 12:p + 16] = struct.pack("<I", v)
        log.append("fmt rate = 0x%X" % v)
    elif op == 3 and len(chunks) >= 2:
        # data before fmt
        (p0, t0, s0), (p1, t1, s1) = chunks[0], chunks[1]
        a = bytes(data[p0:p1])
        b = bytes(data[p1:p1 + 8 + ((s1 + 1) & ~1)])
        data[p0:p1 + len(b)] = b + a
        log.append("chunks swapped")
    elif op == 4 and chunks:
        p, tag, size = rng.choice(chunks)
        data[p:p + 4] = rng.choice([b"fmt ", b"data", b"LIST", b"fmX "])
        log.append("%s tag = %s" % (tag.decode("latin1"), data[p:p + 4].decode("latin1")))
    else:
        return mutate_generic(rng, bytes(data), log)
    return bytes(data)


def mutate_map(rng, data, log):
    text = data.decode("latin1")
    lines = text.split("\n")
    op = rng.randrange(8)
    i = rng.randrange(len(lines))
    if op == 0:
        lines[i] = "@%d\t1\t2\t3" % rng.choice([0, 1, 7, 63, 64, 254, 255, 256, 65535, -1])
        log.append("line %d = %r" % (i, lines[i]))
    elif op == 1:
        lines[i] = "S8.TGA\t%s\t%s" % (rng.choice(["-32768", "32767", "99999999", "1234567", "-0"]),
                                        rng.choice(["0", "-1", "40000"]))
        log.append("line %d = %r" % (i, lines[i]))
    elif op == 2:
        lines[i] = rng.choice(["A" * 12 + " 1 2", "A" * 13 + " 1 2", "S8.TGA 1", "S8.TGA", "\t\t", "S8.TGA 1 2 3 4 5 6 7 8",
                               "S8.TGA 1 2 1234567", "S8.TGA 1 2 12345678", "@ 1 2", "@@@@ 1 2", "S8.TGA\r1\r2\r"])
        log.append("line %d = %r" % (i, lines[i]))
    elif op == 3:
        lines = lines * rng.choice([20, 120])
        log.append("lines x %d" % (len(lines) // len(text.split("\n"))))
    elif op == 4:
        text = "\n".join(lines)
        k = rng.randrange(len(text))
        text = text[:k] + "\0" + text[k + 1:]
        log.append("NUL at %d" % k)
        return text.encode("latin1")
    elif op == 5:
        text = "\n".join(lines).replace("\t", "")
        log.append("no tabs")
        return text.encode("latin1")
    else:
        return mutate_generic(rng, data, log)
    return "\n".join(lines).encode("latin1")


JOL_DIR_FIELDS = [("version", 4, ">H"), ("flags", 6, ">H"), ("offset", 8, ">I"), ("size", 12, ">I"),
                  ("header_size", 16, ">H"), ("record_size", 18, ">H"), ("record_count", 20, ">I")]


def mutate_jol(rng, data, log):
    data = bytearray(data)
    count = struct.unpack(">I", data[12:16])[0]
    op = rng.randrange(6)
    if op <= 2 and count > 0:
        k = rng.randrange(count)
        base = 16 + 24 * k
        sec = data[base:base + 4].decode("latin1")
        field, off, fmt = rng.choice(JOL_DIR_FIELDS)
        vals = U16_VALUES if fmt == ">H" else U32_VALUES
        v = rng.choice(vals)
        data[base + off:base + off + struct.calcsize(fmt)] = struct.pack(fmt, v)
        log.append("%s %s = 0x%X" % (sec, field, v))
    elif op == 3:
        field, off, fmt = rng.choice([("size", 8, ">I"), ("section_count", 12, ">I"), ("version", 4, ">H")])
        v = rng.choice(U32_VALUES if fmt == ">I" else U16_VALUES)
        data[off:off + struct.calcsize(fmt)] = struct.pack(fmt, v)
        log.append("header %s = 0x%X" % (field, v))
    elif op == 4 and count > 0:
        # A record_size of 0 with many records (the count is not limited by the size of the section)
        k = rng.randrange(count)
        base = 16 + 24 * k
        v = rng.choice([1000, 0x15555556, 0x0AAAAAAB, 0x40000001])
        data[base + 18:base + 20] = struct.pack(">H", 0)
        data[base + 20:base + 24] = struct.pack(">I", v)
        log.append("%s record_size = 0, record_count = 0x%X" % (data[base:base + 4].decode("latin1"), v))
    else:
        return mutate_generic(rng, bytes(data), log)
    return bytes(data)


def variants(seeds, mutator, prefix, ext, salt):
    out = []
    for n in range(FUZZ_COUNT):
        rng = random.Random(salt * 1000 + n)
        seed_name, seed = seeds[n % len(seeds)]
        log = []
        data = seed
        for _ in range(1 + rng.randrange(3)):
            data = mutator(rng, data, log)
        out.append(("%s%02d.%s" % (prefix, n, ext), data, "%s: %s" % (seed_name, "; ".join(log))))
    return out


def build():
    seeds = {}
    seeds["SKEL.JOM"] = jom_skeleton()
    seeds["CAP.JOM"] = read_sample("CAP.JOM")
    seeds["S24.TGA"] = tga(8, 8, 24)
    seeds["S32.TGA"] = tga(16, 4, 32, descriptor=0, image_id=b"jo!")
    seeds["S8.TGA"] = tga(8, 8, 8, palette=[(i * 9 % 256, i * 5 % 256, i * 3 % 256) for i in range(16)])
    seeds["M16.WAV"] = wav(1, 16, 22050, 50)
    seeds["ST16.WAV"] = wav(2, 16, 11025, 40, extra_chunk=True)
    seeds["M8.WAV"] = wav(1, 8, 8000, 33)
    seeds["SEED.MAP"] = map_seed()
    seeds["DEFS.JOL"] = read_sample("DEFS.JOL")
    seeds["LEVEL.JOL"] = read_sample("LEVEL.JOL")
    seeds.update(jom_known_bugs(seeds["SKEL.JOM"]))
    # 0xFFFF x 0x8001 pixels (24 bits): width x height x 2 bytes = 0x1_0000_FFFE, 65534 bytes allocated
    seeds["KTGA.TGA"] = tga(8, 8, 24)[:12] + struct.pack("<HH", 0xFFFF, 0x8001) + tga(8, 8, 24)[16:]
    files = list(seeds.items())
    described = []
    jom = [(k, seeds[k]) for k in ("SKEL.JOM", "CAP.JOM")]
    described += variants(jom, mutate_jom, "J", "JOM", 1)
    described += variants([(k, seeds[k]) for k in ("S24.TGA", "S32.TGA", "S8.TGA")], mutate_tga, "T", "TGA", 2)
    described += variants([(k, seeds[k]) for k in ("M16.WAV", "ST16.WAV", "M8.WAV")], mutate_wav, "W", "WAV", 3)
    described += variants([("SEED.MAP", seeds["SEED.MAP"])], mutate_map, "M", "MAP", 4)
    described += variants([(k, seeds[k]) for k in ("DEFS.JOL", "LEVEL.JOL")], mutate_jol, "L", "JOL", 5)
    return files, described


def main():
    files, described = build()
    if "--list" in sys.argv:
        for name, data, text in described:
            print("%-8s %6d  %s" % (name, len(data), text))
        return
    os.makedirs(OUT, exist_ok=True)
    for f in os.listdir(OUT):
        os.remove(os.path.join(OUT, f))
    total = 0
    for name, data in files + [(n, d) for n, d, _ in described]:
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        total += len(data)
    print("%d files, %d bytes in %s" % (len(files) + len(described), total, OUT))


if __name__ == "__main__":
    main()
