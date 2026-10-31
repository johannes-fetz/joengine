#!/usr/bin/env python3
"""Jo Engine 3D model converter: Wavefront OBJ (+ MTL) -> JOM file (see jo_engine/jo/model.h for the file format).

Usage:
    python3 jo_model.py MODEL.OBJ -o cd/MODEL.JOM [options]

Options:
    -o FILE             output file (default: the OBJ name with the .JOM extension)
    --scale S           scale of the points (default 1)
    --saturn-axes       the OBJ is in the axes of the Saturn (X right, Y down, Z forward): no conversion
    --keep-triangles    keep the vertex order of the triangles (by default, the shortest edge becomes A-B)
    --remove-unused     remove the points not used by the polygons (by default, the order of the points is kept)
    --no-weld           keep the points at the same position (by default they are merged: the engine transforms each point
                        once per frame. Only the points used by the same groups are merged; the gouraud colors of the
                        polygons ("GOUR") are kept, "VCOL" gets the color of the first point merged)
    --texture-dir DIR   where the textures converted from PNG are written (default: the directory of the output file)
    --embed-textures    the textures are in the JOM file (one file for the model and its textures, see "TIMG" in
                        jo_engine/jo/model.h): no texture files on the CD
    --skeleton FILE     rigid skeleton and animations (JSON, see below): "BONE" and "ANIM" chunks
    -q                  quiet
    --info FILE.JOM     print the content of a JOM file

What the converter does for the Saturn:
    - the faces become quads (VDP1 polygons). The VDP1 maps the whole texture on a quad (no texture coordinates): the
      corner with the top left of the texture (OBJ vt: u small, v big) becomes A, the texture is drawn from A-B to D-C.
      A triangle is a quad (A, B, C, C) with its shortest edge in A-B (the VDP1
      draws a quad line by line from A-D to B-C: a thin triangle costs about the square of its length otherwise),
      a face of 5 points or more is cut in quads
    - the axes: OBJ (Y up, right handed, Blender) -> Saturn (Y down, left handed): a rotation of 180 degrees around X
      (x, -y, -z) and the vertex order is reversed (the front faces of the OBJ are counterclockwise, the front faces of
      the engine are clockwise on the screen): the model is not mirrored. The points and the normals are 16.16 fixed
      point numbers
    - the normal of each polygon, the bounding sphere of the model
    - the textures (map_Kd) in PNG are converted to jo BIN images (width multiple of 8, RGB 555, alpha -> transparent)
    - the vertex colors (OBJ "v x y z r g b") become gouraud tables (16 = texture unchanged)

Material options (MTL, after newmtl, besides Kd, Ns, d and map_Kd):
    jo_dual 1               drawn on both sides
    jo_sort min|max|center|previous  painter's sort with the nearest point, the farthest point, the center (default) or
                        the depth of the polygon before it in the OBJ (a decal on a face: drawn over it)
    jo_gouraud 0|1          gouraud shading with the vertex colors (default: 1 if the OBJ has vertex colors)
    jo_half_transparent 1   half transparent (also d < 1)
    jo_screen_doors 1       screen doors (mesh)
    jo_light 1              lit by the light of the engine (jo_3d_light())
    jo_flip h|v|hv          texture flipped
    jo_end_codes 1          the texture stops each line after its last visible pixel (jo_sprite_add_end_codes())
    jo_mipmaps 1            smaller copies of the texture far away (jo_3d_set_texture_mipmaps())
    jo_subdivision 1        the polygons near the camera are drawn in parts (jo_3d_set_texture_subdivision())
    jo_subdivision 2        only the polygons that cross the near plane (jo_3d_set_texture_subdivision_near_only())
    jo_subdivision_error N  distortion in pixels over which the polygons are drawn in parts (1 to 255, default 8, see
                            jo_3d_set_texture_subdivision_error()): 4 for a pattern with straight lines (a tiled floor)
    jo_lut 1                the texture is embedded in 16 colors with a color lookup table (4 bits per pixel: 4 times
                            less VRAM, the same rendering as RGB: gouraud, half transparency), see jo_sprite_add_4bits_lut()
    jo_user N [N...]        flags of the program (0 to 15: JO_3D_MATERIAL_USER(N))
    jo_value N              value of the program (jo_3d_material.user)

Groups: each OBJ "o" or "g" is a group, for the animations of the program: its polygons (together in the OBJ) and its
points (the points used only by its polygons, together in the OBJ: the points shared with other groups don't move).

Skeleton (--skeleton FILE.json): rigid parts. Each bone moves one group of the OBJ (its polygons use only its own points:
a mercenary, a jeep with its wheels, a door). The file:
    {"bones": [{"name": "hips", "parent": null, "pivot": [x, y, z]},        (OBJ axes and units, before --scale)
               {"name": "thigh_l", "parent": "hips", "pivot": [...], "group": "thigh_l"}, ...],
     "animations": [{"name": "walk", "key_time": 0.125, "loop": true,
                     "keys": [{"root": [x, y, z], "thigh_l": [rx, ry, rz], ...}, ...]}]}
    - a bone moves the group with its name (or "group": name, null: no polygons) and its children; a parent is
      before its children
    - pivot: the point the bone turns around (the joint)
    - the angles of a key are in degrees, around X (pitch), then Y (yaw), then Z (roll) of the parent; a bone not in a key
      doesn't turn; root: the offset of the whole model (a step, a fall)
    - key_time: seconds between 2 keys (the engine interpolates), loop: the last key goes back to the first one
"""
import json
import math
import hashlib
import os
import struct
import sys

VERSION = 1

MAT_DUAL_PLANE = 1 << 0
MAT_SORT_MIN = 1 << 1
MAT_SORT_MAX = 1 << 2
MAT_GOURAUD = 1 << 3
MAT_HALF_TRANSPARENT = 1 << 4
MAT_SCREEN_DOORS = 1 << 5
MAT_LIGHT = 1 << 6
MAT_FLIP_H = 1 << 7
MAT_FLIP_V = 1 << 8
MAT_END_CODES = 1 << 9
MAT_MIPMAPS = 1 << 10
MAT_SUBDIVISION = 1 << 11
MAT_SUBDIVISION_NEAR = 1 << 12


class ModelError(Exception):
    pass


class Material:
    def __init__(self, name):
        self.name = name
        self.color = (1.0, 1.0, 1.0)
        self.shine = 0
        self.texture = None
        self.flags = 0
        self.user = 0
        self.subdivision_error = 0  # 0: the default of the engine
        self.gouraud = None     # None: default (vertex colors)
        self.lut = False        # The texture in 16 colors (jo_lut)


def fixed(v):
    f = int(round(v * 65536.0))
    if f < -0x80000000 or f > 0x7fffffff:
        raise ModelError('value out of the 16.16 fixed point range: %g' % v)
    return f


def rgb555(c):
    r, g, b = [max(0, min(31, int(round(x * 31)))) for x in c]
    return 0x8000 | (b << 10) | (g << 5) | r


def short_name(name):
    """8.3 file name of the CD (upper case)"""
    stem, ext = os.path.splitext(os.path.basename(name))
    return (stem[:8] + ext[:4]).upper()


def read_mtl(path, materials, warnings):
    current = None
    with open(path, encoding='utf-8', errors='replace') as f:
        for number, raw in enumerate(f, 1):
            line = raw.split('#', 1)[0].strip()
            if not line:
                continue
            words = line.split()
            key = words[0].lower()
            args = words[1:]
            if key == 'newmtl':
                current = Material(' '.join(args))
                materials[current.name] = current
                continue
            if current is None:
                continue
            try:
                if key == 'kd':
                    current.color = tuple(float(v) for v in args[:3])
                elif key == 'ns':
                    current.shine = max(0, min(255, int(round(float(args[0])))))
                elif key == 'd' and float(args[0]) < 1.0:
                    current.flags |= MAT_HALF_TRANSPARENT
                elif key == 'tr' and float(args[0]) > 0.0:
                    current.flags |= MAT_HALF_TRANSPARENT
                elif key == 'map_kd':
                    current.texture = os.path.join(os.path.dirname(path), args[-1])
                elif key == 'jo_dual':
                    current.flags = (current.flags | MAT_DUAL_PLANE) if int(args[0]) else (current.flags & ~MAT_DUAL_PLANE)
                elif key == 'jo_sort':
                    current.flags &= ~(MAT_SORT_MIN | MAT_SORT_MAX)
                    current.flags |= {'min': MAT_SORT_MIN, 'max': MAT_SORT_MAX, 'center': 0,
                                      'previous': MAT_SORT_MIN | MAT_SORT_MAX}[args[0].lower()]
                elif key == 'jo_gouraud':
                    current.gouraud = bool(int(args[0]))
                elif key == 'jo_half_transparent':
                    current.flags = (current.flags | MAT_HALF_TRANSPARENT) if int(args[0]) else (current.flags & ~MAT_HALF_TRANSPARENT)
                elif key == 'jo_screen_doors' and int(args[0]):
                    current.flags |= MAT_SCREEN_DOORS
                elif key == 'jo_light' and int(args[0]):
                    current.flags |= MAT_LIGHT
                elif key == 'jo_flip':
                    current.flags |= (MAT_FLIP_H if 'h' in args[0].lower() else 0) | (MAT_FLIP_V if 'v' in args[0].lower() else 0)
                elif key == 'jo_end_codes' and int(args[0]):
                    current.flags |= MAT_END_CODES
                elif key == 'jo_mipmaps' and int(args[0]):
                    current.flags |= MAT_MIPMAPS
                elif key == 'jo_subdivision' and int(args[0]):
                    current.flags |= MAT_SUBDIVISION_NEAR if int(args[0]) == 2 else MAT_SUBDIVISION
                elif key == 'jo_subdivision_error':
                    if not 0 <= int(args[0]) <= 255:
                        raise ValueError('jo_subdivision_error: 0 to 255')
                    current.subdivision_error = int(args[0])
                elif key == 'jo_lut':
                    current.lut = bool(int(args[0]))
                elif key == 'jo_user':
                    for a in args:
                        if not 0 <= int(a) <= 15:
                            raise ValueError('jo_user: 0 to 15')
                        current.flags |= 1 << (16 + int(a))
                elif key == 'jo_value':
                    current.user = int(args[0], 0) & 0xffffffff
            except (ValueError, IndexError, KeyError) as e:
                warnings.append('%s:%d: %s (%s)' % (path, number, line, e))


def obj_index(token, count, number):
    i = int(token)
    i = i - 1 if i > 0 else count + i
    if not 0 <= i < count:
        raise ModelError('line %d: index %s out of range' % (number, token))
    return i


def read_obj(path, warnings):
    points = []
    colors = []
    normals = []
    uvs = []
    faces = []          # (corner points, corner normals or None, material name, group name, corner uvs or None)
    materials = {}
    material = None
    group = None
    with open(path, encoding='utf-8', errors='replace') as f:
        for number, raw in enumerate(f, 1):
            line = raw.split('#', 1)[0].strip()
            if not line:
                continue
            words = line.split()
            key = words[0]
            if key == 'v':
                v = [float(x) for x in words[1:]]
                points.append(tuple(v[:3]))
                colors.append(tuple(v[3:6]) if len(v) >= 6 else None)
            elif key == 'vn':
                normals.append(tuple(float(x) for x in words[1:4]))
            elif key == 'vt':
                uvs.append(tuple(float(x) for x in (words[1:3] + ['0'])[:2]))
            elif key == 'f':
                corners = []
                corner_normals = []
                corner_uvs = []
                for token in words[1:]:
                    parts = token.split('/')
                    corners.append(obj_index(parts[0], len(points), number))
                    corner_uvs.append(obj_index(parts[1], len(uvs), number) if len(parts) >= 2 and parts[1] else None)
                    corner_normals.append(obj_index(parts[2], len(normals), number) if len(parts) >= 3 and parts[2] else None)
                if len(corners) < 3:
                    warnings.append('line %d: face of %d points ignored' % (number, len(corners)))
                    continue
                faces.append((corners, None if None in corner_normals else corner_normals, material, group,
                              None if None in corner_uvs else [uvs[i] for i in corner_uvs]))
            elif key in ('o', 'g'):
                group = ' '.join(words[1:]) or None
            elif key == 'usemtl':
                material = ' '.join(words[1:])
            elif key == 'mtllib':
                mtl = os.path.join(os.path.dirname(path), ' '.join(words[1:]))
                if os.path.exists(mtl):
                    read_mtl(mtl, materials, warnings)
                else:
                    warnings.append('%s not found' % mtl)
    return points, colors, normals, faces, materials


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def length(a):
    return math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def to_quads(corners, normals, points, keep_triangles, uv=None):
    """Faces of the OBJ -> quads (4 corners, a triangle repeats its last corner)"""
    n = len(corners)
    if n == 4:
        if uv is not None:
            # The VDP1 maps the whole texture: its top left corner on A, its top right corner on B (the front faces are
            # clockwise on the screen). The corner with the top left of the texture (OBJ: u small, v big) becomes A
            r = min(range(4), key=lambda k: uv[k][0] - uv[k][1])
            corners = corners[r:] + corners[:r]
            normals = None if normals is None else normals[r:] + normals[:r]
        return [(corners, normals)]
    if n == 3:
        r = 0
        if not keep_triangles:
            # The shortest edge in A-B (the orientation is kept)
            edges = [length(sub(points[corners[k]], points[corners[(k + 1) % 3]])) for k in range(3)]
            r = edges.index(min(edges))
        c = [corners[(r + k) % 3] for k in range(3)]
        nn = None if normals is None else [normals[(r + k) % 3] for k in range(3)]
        return [(c + [c[2]], None if nn is None else nn + [nn[2]])]
    # Fan of quads (and a triangle at the end)
    out = []
    k = 1
    while k + 1 < n:
        if k + 2 < n:
            idx = [0, k, k + 1, k + 2]
            k += 2
        else:
            idx = [0, k, k + 1, k + 1]
            k += 1
        out.append(([corners[i] for i in idx], None if normals is None else [normals[i] for i in idx]))
    return out


def face_normal(p):
    """Newell normal of a quad (outward for a counterclockwise order)"""
    nx = ny = nz = 0.0
    for k in range(4):
        a = p[k]
        b = p[(k + 1) % 4]
        nx += (a[1] - b[1]) * (a[2] + b[2])
        ny += (a[2] - b[2]) * (a[0] + b[0])
        nz += (a[0] - b[0]) * (a[1] + b[1])
    d = math.sqrt(nx * nx + ny * ny + nz * nz)
    return (0.0, 0.0, 0.0) if d < 1e-12 else (nx / d, ny / d, nz / d)


def texture_pixels(source, warnings):
    """Pixels of a texture: (width, height, RGB 555 values, 0 is transparent), None if not readable.
    PNG (and the other formats of Pillow: alpha < 128 is transparent), jo BIN image, TGA"""
    if not os.path.exists(source):
        warnings.append('%s not found' % source)
        return None
    if source.upper().endswith('.BIN'):
        data = open(source, 'rb').read()
        w, h = struct.unpack('<HH', data[:4])
        return w, h, list(struct.unpack('<%dH' % (w * h), data[4:4 + w * h * 2]))
    try:
        from PIL import Image
    except ImportError:
        warnings.append('%s: Pillow is required to convert the textures' % source)
        return None
    img = Image.open(source).convert('RGBA')
    w, h = img.size
    if w % 8:
        w8 = (w + 7) // 8 * 8
        warnings.append('%s: width %d -> %d (multiple of 8 for the VDP1)' % (source, w, w8))
        img = img.resize((w8, h), Image.BILINEAR)
        w = w8
    if w * h > 512 * 256:
        warnings.append('%s: big texture (%d x %d)' % (source, w, h))
    raw = img.tobytes()
    pixels = []
    for k in range(0, len(raw), 4):
        r, g, b, a = raw[k], raw[k + 1], raw[k + 2], raw[k + 3]
        pixels.append(0 if a < 128 else 0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3))
    return w, h, pixels


def texture_16_colors(w, h, pixels):
    """A texture in 16 colors: (16 RGB 555 colors (the color 0: transparent), pixels packed 2 per byte, the left one in the
    high bits). The opaque colors are reduced to 14 colors (median cut, Floyd-Steinberg dithering): the index 15 is never
    used, it is the end code of the VDP1 (16 colors mode) when the end codes are enabled (ECD = 0, the polygons of the
    models): 2 texels of index 15 on a line of the texture stop the VDP1, the rest of the line is not drawn (dashes of
    the background across the hull of the wreck: a few texels of the 15th color scattered by the dithering)"""
    from PIL import Image
    rgb = bytearray()
    for p in pixels:
        rgb += bytes((((p & 0x1f) << 3) | 4, (((p >> 5) & 0x1f) << 3) | 4, (((p >> 10) & 0x1f) << 3) | 4)) if p else b'\0\0\0'
    img = Image.frombytes('RGB', (w, h), bytes(rgb))
    opaque = [p for p in pixels if p]
    if not opaque:
        return [0] * 16, bytes(w * h // 2)
    q = img.quantize(colors=14, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
    pal = q.getpalette()[:14 * 3]
    lut = [0] + [0x8000 | ((pal[k * 3 + 2] >> 3) << 10) | ((pal[k * 3 + 1] >> 3) << 5) | (pal[k * 3] >> 3) for k in range(len(pal) // 3)]
    # (the index 15: the end code, never in the pixels; its color is the last one in case)
    lut += [lut[-1]] * (16 - len(lut))
    assert max(q.getdata()) + 1 <= 14
    indexes = list(q.getdata())
    packed = bytearray(w * h // 2)
    for k, p in enumerate(pixels):
        index = 0 if p == 0 else indexes[k] + 1
        packed[k >> 1] |= index << (0 if k & 1 else 4)
    return lut, bytes(packed)


def convert_texture(source, texture_dir, warnings, embed=False):
    """Texture of a material: name (on the CD, or embedded in the JOM file). A PNG is converted to a jo BIN image
    (written in texture_dir when the texture is not embedded)"""
    name = short_name(source)
    ext = os.path.splitext(name)[1]
    if ext in ('.BIN', '.TGA') or embed:
        return name if ext in ('.BIN', '.TGA') else os.path.splitext(name)[0] + '.BIN'
    name = os.path.splitext(name)[0] + '.BIN'
    t = texture_pixels(source, warnings)
    if t is None:
        return name
    w, h, pixels = t
    os.makedirs(texture_dir, exist_ok=True)
    with open(os.path.join(texture_dir, name), 'wb') as f:
        f.write(struct.pack('<HH', w, h) + struct.pack('<%dH' % len(pixels), *pixels))
    return name


def embedded_texture_name(source, lut, warnings):
    """Name of an embedded texture (15 characters at most): the start of the file name and a hash of the pixels and of the
    format, unique for each texture of all the models"""
    px = texture_pixels(source, warnings)
    stem = os.path.splitext(os.path.basename(source))[0].upper()
    digest = hashlib.sha1(repr((px, lut)).encode()).hexdigest()[:6].upper()
    return '%s_%s.BIN' % (stem[:4], digest)


def chunk(tag, data):
    data = bytes(data)
    if len(data) % 4:
        data += b'\0' * (4 - len(data) % 4)
    return tag.encode('ascii') + struct.pack('>I', len(data)) + data


def name16(s):
    b = s.encode('ascii', 'replace')[:15]
    return b + b'\0' * (16 - len(b))


def skeleton_chunks(path, groups, quads, points, scale, s_yz):
    """The "BONE" and "ANIM" chunks of a skeleton file (see the documentation of the module)"""
    rig = json.load(open(path))
    bones = rig.get('bones', [])
    if not bones or len(bones) > 32:
        raise ModelError('%s: 1 to 32 bones' % path)
    names = [b['name'] for b in bones]
    group_index = {g[0]: k for k, g in enumerate(groups)}
    data = bytearray(struct.pack('>I', len(bones)))
    for k, b in enumerate(bones):
        parent = b.get('parent')
        pi = -1 if parent is None else names.index(parent)
        if pi >= k:
            raise ModelError('bone %s: its parent must be before it' % b['name'])
        gname = b.get('group', b['name'])
        gi = 0xFFFF
        if gname is not None and gname in group_index:
            gi = group_index[gname]
            # A bone draws its group alone: its polygons use only its points (together)
            _, first, end, used = groups[gi]
            lo, hi = min(used), max(used)
            for q in quads[first:end]:
                for i in q[0]:
                    if not lo <= i <= hi:
                        raise ModelError('bone %s: a polygon uses a point of another group' % b['name'])
            for other in groups:
                if other[0] != gname and any(lo <= i <= hi for i in other[3]):
                    raise ModelError('bone %s: its points are shared with the group %s' % (b['name'], other[0]))
        elif gname is not None:
            raise ModelError('bone %s: no group %s in the OBJ' % (b['name'], gname))
        px, py, pz = b.get('pivot', [0, 0, 0])
        data += name16(b['name']) + struct.pack('>Hhiii', gi, pi, fixed(px * scale), fixed(py * scale * s_yz),
                                                fixed(pz * scale * s_yz))
    anims = rig.get('animations', [])
    header = bytearray(struct.pack('>I', len(anims)))
    body = bytearray()
    offset = 4 + 28 * len(anims)
    for a in anims:
        keys = a['keys']
        if not keys:
            raise ModelError('animation %s: no keys' % a['name'])
        ticks = max(1, int(round(a.get('key_time', 0.1) * 60)))
        header += name16(a['name']) + struct.pack('>HHHHI', len(keys), ticks, 1 if a.get('loop') else 0, 0,
                                                  offset + len(body))
        for key in keys:
            for n in key:
                if n != 'root' and n not in names:
                    raise ModelError('animation %s: no bone %s' % (a['name'], n))
            rx, ry, rz = key.get('root', [0, 0, 0])
            body += struct.pack('>iii', fixed(rx * scale), fixed(ry * scale * s_yz), fixed(rz * scale * s_yz))
            for n in names:
                ax, ay, az = key.get(n, [0, 0, 0])
                # (the OBJ axes: a rotation of 180 degrees around X, the angles around Y and Z change of sign)
                body += struct.pack('>hhh', *[(int(round(v / 360.0 * 65536)) + 32768) % 65536 - 32768
                                              for v in (ax, ay * s_yz, az * s_yz)])
            if len(names) % 2:
                body += b'\0\0'
    return chunk('BONE', data), chunk('ANIM', header + body), len(bones), len(anims)


def weld_points(points, colors, quads):
    """Merge the points at the same position (same fixed point values) used by the same groups (a group can move its
    points: see "GRUP"). A point is replaced by the first one of its kind: the order is kept. Returns (points, colors, quads)"""
    users = {}
    for c, n, m, g in quads:
        for i in c:
            users.setdefault(i, set()).add(g)
    first = {}
    remap = []
    out_points = []
    out_colors = []
    for i, p in enumerate(points):
        if i not in users:
            key = ('unused', i)
        else:
            key = (tuple(fixed(v) for v in p), frozenset(users[i]))
        if key not in first:
            first[key] = len(out_points)
            out_points.append(p)
            out_colors.append(colors[i])
        remap.append(first[key])
    return out_points, out_colors, [([remap[i] for i in c], n, m, g) for c, n, m, g in quads]


def convert(obj_path, out_path=None, scale=1.0, saturn_axes=False, keep_triangles=False, remove_unused=False,
            texture_dir=None, quiet=False, embed_textures=False, skeleton=None, weld=True):
    """Convert an OBJ file to a JOM file. Returns a dictionary of statistics (and 'warnings')"""
    warnings = []
    if out_path is None:
        out_path = os.path.splitext(obj_path)[0] + '.JOM'
    if texture_dir is None:
        texture_dir = os.path.dirname(os.path.abspath(out_path))
    points, colors, normals, faces, materials = read_obj(obj_path, warnings)
    if not points or not faces:
        raise ModelError('%s: no points or no faces' % obj_path)
    # Saturn axes: rotation of 180 degrees around X, the vertex order is reversed (front faces: clockwise on the screen)
    s_yz = 1.0 if saturn_axes else -1.0
    points = [(x * scale, y * scale * s_yz, z * scale * s_yz) for x, y, z in points]
    normals = [(x, y * s_yz, z * s_yz) for x, y, z in normals]
    if not saturn_axes:
        faces = [(c[::-1], None if n is None else n[::-1], m, g, None if uv is None else uv[::-1]) for c, n, m, g, uv in faces]
    has_colors = any(c is not None for c in colors)
    colors = [c if c is not None else (1.0, 1.0, 1.0) for c in colors]
    # Quads
    quads = []
    triangles = 0
    for corners, corner_normals, material, group, uv in faces:
        if len(corners) == 3:
            triangles += 1
        for c, n in to_quads(corners, corner_normals, points, keep_triangles, uv):
            quads.append((c, n, material, group))
    if len(quads) > 65535 or len(points) > 65535:
        raise ModelError('too many points or polygons (65535 at most)')
    if remove_unused:
        used = sorted({i for q in quads for i in q[0]})
        remap = {old: new for new, old in enumerate(used)}
        if len(used) < len(points):
            points = [points[i] for i in used]
            colors = [colors[i] for i in used]
            quads = [([remap[i] for i in c], n, m, g) for c, n, m, g in quads]
    else:
        unused = len(points) - len({i for q in quads for i in q[0]})
        if unused:
            warnings.append('%d points not used by the polygons (--remove-unused)' % unused)
    # The gouraud colors of the corners of the polygons, before the points are merged
    corner_colors = [[colors[i] for i in q[0]] for q in quads]
    welded = 0
    if weld:
        count = len(points)
        points, colors, quads = weld_points(points, colors, quads)
        welded = count - len(points)
    # Materials (in the order of their first use), the polygons without material use a white one
    names = []
    for q in quads:
        if q[2] not in names:
            names.append(q[2])
    mats = []
    for n in names:
        if n is not None and n in materials:
            mats.append(materials[n])
        else:
            if n is not None:
                warnings.append('material %s not found in the MTL file' % n)
            mats.append(Material(n or 'default'))
    textures = []
    texture_sources = {}
    lut_textures = set()
    for m in mats:
        if m.gouraud is None:
            m.gouraud = has_colors
        if m.gouraud and has_colors:
            m.flags |= MAT_GOURAUD
        m.texture_index = -1
        if m.texture:
            t = convert_texture(m.texture, texture_dir, warnings, embed_textures)
            if embed_textures:
                # The engine shares the embedded textures of all the models by name (see jo_3d_model_find_texture()): the name
                # is made from the content (pixels and format), 2 different textures never get the same name (the 8.3 names
                # of "washroom_stone2.png" and "washroom_upper1.png" were the same: one texture replaced the other)
                t = embedded_texture_name(m.texture, bool(m.lut), warnings)
            elif t in texture_sources and os.path.abspath(texture_sources[t]) != os.path.abspath(m.texture):
                raise SystemExit('%s and %s have the same file name on the CD (%s): rename one of them' %
                                 (texture_sources[t], m.texture, t))
            if t not in textures:
                textures.append(t)
                texture_sources[t] = m.texture
            m.texture_index = textures.index(t)
            if m.lut:
                lut_textures.add(t)
    mat_index = {n: k for k, n in enumerate(names)}
    # Groups: ranges of points and of polygons (in the order of the OBJ)
    groups = []
    for k, q in enumerate(quads):
        g = q[3]
        if g is None:
            continue
        if not groups or groups[-1][0] != g:
            if any(x[0] == g for x in groups):
                warnings.append('group %s: its polygons are not together' % g)
            groups.append([g, k, k + 1, set(q[0])])
        else:
            groups[-1][2] = k + 1
            groups[-1][3].update(q[0])
    # Bounding sphere
    xs, ys_, zs = zip(*points)
    center = ((min(xs) + max(xs)) / 2, (min(ys_) + max(ys_)) / 2, (min(zs) + max(zs)) / 2)
    radius = max(length(sub(p, center)) for p in points)
    out = bytearray(b'JOM1')
    out += struct.pack('>HH', VERSION, 0)
    out += struct.pack('>iiii', fixed(center[0]), fixed(center[1]), fixed(center[2]), fixed(radius))
    embedded = 0
    if embed_textures and textures:
        # The textures in the file (first chunk: the loader copies them to the VRAM one at a time)
        data = bytearray(struct.pack('>I', len(textures)))
        for t in textures:
            px = texture_pixels(texture_sources[t], warnings)
            if px is None:
                px = (8, 1, [0] * 8)
            w, h, pixels = px
            if t in lut_textures:
                # 16 colors: the table, then 2 pixels per byte (4 bytes aligned)
                lut, packed = texture_16_colors(w, h, pixels)
                block = struct.pack('>16H', *lut) + packed
                block += b'\0' * (-len(block) % 4)
                data += name16(t) + struct.pack('>HHHH', w, h, 1, 0) + block
                embedded += len(block)
                continue
            data += name16(t) + struct.pack('>HHHH', w, h, 0, 0) + struct.pack('>%dH' % len(pixels), *pixels)
            if len(pixels) % 2:
                data += b'\0\0'
            embedded += w * h * 2
        out += chunk('TIMG', data)
    data = bytearray(struct.pack('>I', len(points)))
    for p in points:
        data += struct.pack('>iii', *[fixed(v) for v in p])
    out += chunk('PNTS', data)
    data = bytearray(struct.pack('>I', len(quads)))
    for c, n, m, g in quads:
        # The normal of the polygon: front faces clockwise in the Saturn axes (Y down, left handed)
        fn = face_normal([points[i] for i in c[::-1]])
        data += struct.pack('>iiiHHHH', fixed(fn[0]), fixed(fn[1]), fixed(fn[2]), *c)
    out += chunk('QUAD', data)
    data = bytearray(struct.pack('>I', len(quads)))
    for q in quads:
        data += struct.pack('>H', mat_index[q[2]])
    out += chunk('QMAT', data)
    data = bytearray(struct.pack('>I', len(mats)))
    for m in mats:
        r, g, b = [max(0, min(255, int(round(v * 255)))) for v in m.color]
        data += name16(m.name) + struct.pack('>BBBBhHII', r, g, b, m.shine, m.texture_index, m.subdivision_error, m.flags, m.user)
    out += chunk('MATL', data)
    if textures:
        data = bytearray(struct.pack('>I', len(textures)))
        for t in textures:
            data += name16(t)
        out += chunk('TEXT', data)
    if normals and all(q[1] is not None for q in quads):
        data = bytearray(struct.pack('>I', len(normals)))
        for n in normals:
            data += struct.pack('>iii', *[fixed(v) for v in n])
        out += chunk('NTAB', data)
        data = bytearray(struct.pack('>I', len(quads) * 4))
        for q in quads:
            data += struct.pack('>HHHH', *q[1])
        out += chunk('CNRM', data)
    elif normals:
        warnings.append('some faces have no normals: the normals are not written')
    if has_colors:
        data = bytearray(struct.pack('>I', len(points)))
        for c in colors:
            data += struct.pack('>iii', *[fixed(v) for v in c])
        out += chunk('VCOL', data)
        if any(m.flags & MAT_GOURAUD for m in mats):
            data = bytearray(struct.pack('>I', len(quads) * 4))
            for (c, n, m, g), cc in zip(quads, corner_colors):
                gouraud = mats[mat_index[m]].flags & MAT_GOURAUD
                for color in cc:
                    data += struct.pack('>H', rgb555(color) if gouraud else 0)
            out += chunk('GOUR', data)
    if groups:
        # The points of a group: the points used only by its polygons (a point shared with another group doesn't move
        # with it), they must be together in the OBJ
        users = {}
        for gi, (name, first, end, used) in enumerate(groups):
            for i in used:
                users.setdefault(i, set()).add(gi)
        data = bytearray(struct.pack('>I', len(groups)))
        for gi, (name, first, end, used) in enumerate(groups):
            own = [i for i in used if users[i] == {gi}]
            lo, hi = (min(own), max(own)) if own else (0, -1)
            # (the range includes points of other groups if its points are not together in the OBJ, see --info)
            data += name16(name) + struct.pack('>HHHH', lo, hi - lo + 1, first, end - first)
        out += chunk('GRUP', data)
    bone_count = animation_count = 0
    if skeleton:
        bones, anims, bone_count, animation_count = skeleton_chunks(skeleton, groups, quads, points, scale, s_yz)
        out += bones + anims
    out += chunk('END ', b'')
    with open(out_path, 'wb') as f:
        f.write(out)
    stats = {'points': len(points), 'welded': welded, 'polygons': len(quads), 'triangles': triangles, 'materials': len(mats),
             'textures': textures, 'groups': len(groups), 'bones': bone_count, 'animations': animation_count,
             'bytes': len(out), 'warnings': warnings}
    if not quiet:
        print('%s: %d points (%d merged), %d polygons (%d triangles), %d materials, %d textures%s, %d groups, %d bytes'
              % (os.path.basename(out_path), len(points), welded, len(quads), triangles, len(mats), len(textures),
                 ' (embedded: %d bytes)' % embedded if embedded else '', len(groups), len(out)))
        for w in warnings:
            print('  warning: ' + w)
    return stats


def read_jom(path):
    """Read a JOM file (to check it): dictionary of the header values and of the chunks"""
    data = open(path, 'rb').read()
    if data[:4] != b'JOM1':
        raise ModelError('%s: not a JOM file' % path)
    version = struct.unpack('>H', data[4:6])[0]
    cx, cy, cz, r = struct.unpack('>iiii', data[8:24])
    out = {'version': version, 'center': (cx / 65536, cy / 65536, cz / 65536), 'radius': r / 65536, 'chunks': {}}
    p = 24
    while p + 8 <= len(data):
        tag = data[p:p + 4].decode('ascii', 'replace')
        size = struct.unpack('>I', data[p + 4:p + 8])[0]
        if tag == 'END ':
            break
        out['chunks'][tag] = data[p + 8:p + 8 + size]
        p += 8 + size
    c = out['chunks']
    if 'PNTS' in c:
        n = struct.unpack('>I', c['PNTS'][:4])[0]
        out['points'] = [struct.unpack('>iii', c['PNTS'][4 + k * 12:16 + k * 12]) for k in range(n)]
    if 'QUAD' in c:
        n = struct.unpack('>I', c['QUAD'][:4])[0]
        out['polygons'] = [struct.unpack('>iiiHHHH', c['QUAD'][4 + k * 20:24 + k * 20]) for k in range(n)]
    if 'MATL' in c:
        n = struct.unpack('>I', c['MATL'][:4])[0]
        out['materials'] = []
        for k in range(n):
            b = c['MATL'][4 + k * 32:36 + k * 32]
            r, g, bl, shine, tex, error, flags, user = struct.unpack('>BBBBhHII', b[16:32])
            out['materials'].append({'name': b[:16].split(b'\0')[0].decode(), 'color': (r, g, bl), 'shine': shine,
                                     'texture': tex, 'flags': flags, 'user': user, 'subdivision_error': error})
    if 'TEXT' in c:
        n = struct.unpack('>I', c['TEXT'][:4])[0]
        out['textures'] = [c['TEXT'][4 + k * 16:20 + k * 16].split(b'\0')[0].decode() for k in range(n)]
    if 'GRUP' in c:
        n = struct.unpack('>I', c['GRUP'][:4])[0]
        out['groups'] = [(c['GRUP'][4 + k * 24:20 + k * 24].split(b'\0')[0].decode(),) +
                         struct.unpack('>HHHH', c['GRUP'][20 + k * 24:28 + k * 24]) for k in range(n)]
    return out


def info(path):
    m = read_jom(path)
    print('%s: version %d, %d points, %d polygons, bounding sphere (%.1f, %.1f, %.1f) radius %.1f' % (
        path, m['version'], len(m.get('points', [])), len(m.get('polygons', [])), m['center'][0], m['center'][1],
        m['center'][2], m['radius']))
    print('  chunks: ' + ' '.join(sorted(m['chunks'])))
    for t in m.get('textures', []):
        print('  texture ' + t)
    if 'TIMG' in m['chunks']:
        c = m['chunks']['TIMG']
        n = struct.unpack('>I', c[:4])[0]
        p = 4
        for k in range(n):
            w, h, fmt = struct.unpack('>HHH', c[p + 16:p + 22])
            print('  embedded texture %s %d x %d%s' % (c[p:p + 16].split(b'\0')[0].decode(), w, h, ' (16 colors)' if fmt == 1 else ''))
            p += 24 + ((32 + w * h // 2 + 3) // 4 * 4 if fmt == 1 else (w * h * 2 + 3) // 4 * 4)
    for mat in m.get('materials', []):
        print('  material %-16s color %s texture %d flags 0x%08x value %d' % (mat['name'], mat['color'], mat['texture'],
                                                                             mat['flags'], mat['user']))
    for g in m.get('groups', []):
        print('  group %-16s points %d (%d) polygons %d (%d)' % g)
    if 'BONE' in m['chunks']:
        c = m['chunks']['BONE']
        for k in range(struct.unpack('>I', c[:4])[0]):
            b = c[4 + k * 32:36 + k * 32]
            g, parent, x, y, z = struct.unpack('>Hhiii', b[16:32])
            print('  bone %-16s group %d parent %d pivot (%.1f, %.1f, %.1f)' % (b[:16].split(b'\0')[0].decode(),
                                                                          -1 if g == 0xFFFF else g, parent,
                                                                          x / 65536, y / 65536, z / 65536))
    if 'ANIM' in m['chunks']:
        c = m['chunks']['ANIM']
        for k in range(struct.unpack('>I', c[:4])[0]):
            a = c[4 + k * 28:32 + k * 28]
            keys, ticks, flags, _, _ = struct.unpack('>HHHHI', a[16:28])
            print('  animation %-16s %d keys, %d/60 s%s' % (a[:16].split(b'\0')[0].decode(), keys, ticks,
                                                           ', loop' if flags & 1 else ''))


def main(argv):
    args = list(argv)
    opts = {}
    obj = None
    while args:
        a = args.pop(0)
        if a == '-o':
            opts['out_path'] = args.pop(0)
        elif a == '--scale':
            opts['scale'] = float(args.pop(0))
        elif a == '--saturn-axes':
            opts['saturn_axes'] = True
        elif a == '--keep-triangles':
            opts['keep_triangles'] = True
        elif a == '--remove-unused':
            opts['remove_unused'] = True
        elif a == '--no-weld':
            opts['weld'] = False
        elif a == '--texture-dir':
            opts['texture_dir'] = args.pop(0)
        elif a == '-q':
            opts['quiet'] = True
        elif a == '--embed-textures':
            opts['embed_textures'] = True
        elif a == '--skeleton':
            opts['skeleton'] = args.pop(0)
        elif a == '--info':
            info(args.pop(0))
            return 0
        elif a in ('-h', '--help'):
            print(__doc__)
            return 0
        else:
            obj = a
    if obj is None:
        print(__doc__)
        return 1
    try:
        convert(obj, **opts)
    except (ModelError, OSError, ValueError) as e:
        print('error: %s' % e, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
