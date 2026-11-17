#!/usr/bin/env python3
"""Jo Engine regression test ROM: the models of the tests of the vertex animations and of the skin (t_more.c), written in
cd/ (the same output at each run).

    python3 tests/rom/gen_models.py

MRPH12.JOM, MRPH2.JOM   a square (4 points, 1 polygon) and a vertex animation "wave" of 2 frames: the frame 0 has the
                        points of the model (the corners (+-1, +-1) x 2^shift), the frame 1 moves each point by
                        (MOVE_X, MOVE_Y, 0) x 2^shift; shift 12 and 2 (the 2 paths of the engine)
MRPHB1..3.JOM           wrong: a shift over 16, frames past the end of the chunk, an offset not aligned
SKIN.JOM                a skeleton of 2 bones (a parent, a child turning around a joint) and its skin (the converter)
SKINB.JOM               wrong: a point of the skin out of the points
"""

import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "cd")
sys.path.insert(0, os.path.join(HERE, "..", "..", "Compiler", "COMMON"))
import jo_model  # noqa: E402

FRAMES = 2
TICKS = 8                       # T_MORPH_TICKS of t_more.c
MOVE_X = 3                      # T_MORPH_MOVE_X, T_MORPH_MOVE_Y of t_more.c
MOVE_Y = -2
SQUARE = [(-1, -1), (1, -1), (1, 1), (-1, 1)]
LOOP = 1
HEADER = 8
ENTRY = 28
FIXED_1 = 65536
SHIFT_TOO_BIG = 17
UNALIGNED = 2


def chunk(tag, data):
    return jo_model.chunk(tag, data)


def morph_file(shift, offset=HEADER + ENTRY, frames=FRAMES):
    """The square and its vertex animation (see the documentation of the module)"""
    out = bytearray(b"JOM1" + struct.pack(">HH", 1, 0) + struct.pack(">iiii", 0, 0, 0, FIXED_1))
    out += chunk("PNTS", struct.pack(">I", 4) + b"".join(struct.pack(">iii", x << shift, y << shift, 0) for x, y in SQUARE))
    out += chunk("QUAD", struct.pack(">I", 1) + struct.pack(">iiiHHHH", 0, 0, -FIXED_1, 0, 1, 2, 3))
    body = b"".join(struct.pack(">hhh", x + f * MOVE_X, y + f * MOVE_Y, 0) for f in range(FRAMES) for x, y in SQUARE)
    entry = jo_model.name16("wave") + struct.pack(">HHHHI", frames, TICKS, LOOP, 0, offset)
    out += chunk("MRPH", struct.pack(">IHH", 1, shift, 0) + entry + body)
    out += chunk("END ", b"")
    return bytes(out)


def skin_file(path):
    """2 bones: "a" (points 0 to 3, y from 0 to 1), "b" (its child, turning around (0, 1, 0): points 4 to 7 from y 1.2
    to 2), the polygons of "b" joined to "a" (a skin between them)"""
    tmp = tempfile.mkdtemp()
    obj = os.path.join(tmp, "s.obj")
    with open(obj, "w") as f:
        f.write("g a\nv -1 0 0\nv 1 0 0\nv 1 1 0\nv -1 1 0\nf 1 2 3 4\n")
        f.write("g b\nv -1 1.2 0\nv 1 1.2 0\nv 1 2 0\nv -1 2 0\nf 4 3 6 5\nf 5 6 7 8\n")
    rig = os.path.join(tmp, "s.json")
    with open(rig, "w") as f:
        f.write('{"bones": [{"name": "a", "parent": null, "pivot": [0, 0, 0]}, {"name": "b", "parent": "a", '
                '"pivot": [0, 1, 0]}], "animations": [{"name": "bend", "key_time": 1, "keys": [{}, {"b": [0, 0, 90]}]}], '
                '"skin": {"radius": 2}}')
    jo_model.convert(obj, path, saturn_axes=True, quiet=True, skeleton=rig, weld=False, sort=False)


def main():
    for name, data in (("MRPH12.JOM", morph_file(12)), ("MRPH2.JOM", morph_file(2)),
                       ("MRPHB1.JOM", morph_file(SHIFT_TOO_BIG)), ("MRPHB2.JOM", morph_file(4, frames=FRAMES + 1)),
                       ("MRPHB3.JOM", morph_file(4, offset=HEADER + ENTRY + UNALIGNED))):
        open(os.path.join(OUT, name), "wb").write(data)
    skin_file(os.path.join(OUT, "SKIN.JOM"))
    # (wrong: the point of the first point of the skin out of the points)
    data = bytearray(open(os.path.join(OUT, "SKIN.JOM"), "rb").read())
    p = data.index(b"SKIN") + 12
    data[p:p + 2] = struct.pack(">H", 0xFFFF)
    open(os.path.join(OUT, "SKINB.JOM"), "wb").write(bytes(data))


if __name__ == "__main__":
    main()
