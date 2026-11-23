#!/usr/bin/env python3
"""Jo Engine 3D model converter: Wavefront OBJ (+ MTL) -> JOM file (see jo_engine/jo/model.h for the file format).

Usage:
    python3 jo_model.py MODEL.OBJ -o cd/MODEL.JOM [options]

A model built by a program in Python (no OBJ file): jo_mesh.py, which writes the chunks with the functions of this module
(chunk(), vectors_chunk(), quads_chunk()... see "The writing of the chunks").

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
    --morph FILE        vertex animations (JSON, see below): the points of each frame, "MRPH" chunk
    --add-skin FILE.JOM add a skin to the skeleton of a JOM file (see "skin" below), rewritten in place
    --no-auto-sort      keep the sort of the materials (by default the converter chooses the sort of each polygon that
                        has no jo_sort in the MTL file, see "Sort" below; needs numpy)
    -q                  quiet
    --info FILE.JOM     print the content of a JOM file
    --check-sort FILE.JOM  views all around the model: the painter's sort of the VDP1 (no z-buffer: one depth per
                        polygon) against a z-buffer (a polygon drawn over a nearer one: a wall over the edge of a roof),
                        and the holes (the back of a face seen: the edge of a roof without thickness, a face turned the
                        wrong way). Fails (exit code 1) over 0.2 % of the model in a view. Needs numpy
    --check-twist FILE.JOM  the quads twisted more than MAX_TWIST (0.15, see quad_twist(): holes, the VDP1 draws a
                        twisted quad as a bow tie). Fails (exit code 1) if there is one. The converter cuts them in 2
                        triangles (with a warning)

What the converter does for the Saturn:
    - the faces become quads (VDP1 polygons). The VDP1 maps the whole texture on a quad (no texture coordinates): the
      corner with the top left of the texture (OBJ vt: u small, v big) becomes A, the texture is drawn from A-B to D-C.
      A triangle is a quad (A, B, C, C) with its shortest edge in A-B (the VDP1
      draws a quad line by line from A-D to B-C: a thin triangle costs about the square of its length otherwise),
      a face of 5 points or more is cut in quads. A twisted quad (see --check-twist) is cut in 2 triangles along its
      shorter diagonal, with a warning: a row of points more in the model is better (the texture is squashed in each
      triangle)
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
    jo_high_speed_shrink 1  the VDP1 reads every other texel of the texture drawn smaller than it is: faster far away,
                            a little less sharp (all the textures: jo_3d_set_high_speed_shrink())
    jo_subdivision 2        only the polygons that cross the near plane (jo_3d_set_texture_subdivision_near_only())
    jo_subdivision_error N  distortion in pixels over which the polygons are drawn in parts (1 to 255, default 8, see
                            jo_3d_set_texture_subdivision_error()): 4 for a pattern with straight lines (a tiled floor)
    jo_lut 1                the texture is embedded in 16 colors with a color lookup table (4 bits per pixel: 4 times
                            less VRAM, the same rendering as RGB: gouraud, half transparency), see jo_sprite_add_4bits_lut()
    jo_user N [N...]        flags of the program (0 to 15: JO_3D_MATERIAL_USER(N))
    jo_value N              value of the program (jo_3d_material.user)

Sort: the VDP1 has no z-buffer, the engine draws the polygons from the farthest to the nearest with one depth for each
(its center by default): a big face sorted on its center can be drawn over a smaller one against it (a gable over the
edge of a roof, a door over its step). By default the converter draws the model in views all around it like the VDP1 and
with a z-buffer (SortScene), and sorts a polygon drawn over a nearer one on its farthest point, or the nearer one on its
nearest point, while it removes pixels drawn wrong (auto_sort(): a copy of the material with this sort). A material with
jo_sort keeps its sort. --check-sort FILE.JOM prints the views still drawn wrong.

Groups: each OBJ "o" or "g" is a group, for the animations of the program: its polygons (together in the OBJ) and its
points. A point belongs to the group it is declared in (the "v" lines after the "g" line, together); in an OBJ whose
points are all declared before the groups, a group owns the points used only by its polygons (the points shared with
other groups don't move).

Skeleton (--skeleton FILE.json): rigid parts. Each bone moves one group of the OBJ (a mercenary, a jeep with its wheels,
a door). A polygon of a group may use points of other groups: a skin stretched between 2 parts (the polygons around an
elbow use the points of the upper arm and of the forearm, each point moves with its own bone: no gap at the joint). The
file:
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
    - the skin ("SKIN" chunk, by default): where polygons join 2 bones (a bone and its parent), the points near the joint
      follow both bones (the bone of their group, and up to half the other one at the joint): the skin bends instead of
      folding. "skin": false: a rigid skeleton; "skin": {"radius": r}: the radius of the bend around a joint (OBJ units,
      before --scale; default 7 % of the height of the model)

Vertex animations (--morph FILE.json): the points of each frame (morph targets), for a deformed skin (a character
skinned in Blender, a flag, a face): one OBJ per frame, exported with the same topology as the model (the same points in
the same order, the same faces). The file:
    {"animations": [{"name": "walk", "frames": "walk_%04d.obj", "first": 1, "last": 24, "frame_time": 0.0667,
                     "loop": true}, ...]}
    - frames: the OBJ files of the frames (relative to the JSON file): a pattern with first and last (printf style,
      like the export of Blender), or a list of files
    - frame_time: seconds between 2 frames (the engine interpolates the points), loop: the last frame goes back to the
      first one
    - the model (the OBJ converted) is the pose drawn without animation, its bounding sphere includes all the frames
    - the values of the points are 16 bits (x 2^shift: the smallest shift for the largest value of all the frames), the
      normals of the polygons (if a material is lit: jo_light) are indexes in a table of 256 directions
      (morph_normal_table()); the frames of the polygons not lit don't need them
    - the program draws a time of an animation with jo_3d_model_draw_morph() (see jo_engine/jo/model.h)
"""
import copy
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
MAT_HIGH_SPEED_SHRINK = 1 << 13
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
        self.sort_set = False   # jo_sort in the MTL file (else the converter chooses the sort of each polygon)


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
                    current.sort_set = True
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
                elif key == 'jo_high_speed_shrink' and int(args[0]):
                    current.flags |= MAT_HIGH_SPEED_SHRINK
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
    point_groups = []   # The group each point is declared in (None: before the first group)
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
                point_groups.append(group)
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
    return points, colors, normals, faces, materials, point_groups


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


# The writing of the chunks of a JOM file (the converter, and the programs that build their models in Python: see
# jo_engine/jo/model.h). The values are already in the format of the file: 16.16 fixed, indexes, RGB 555

def chunk(tag, data):
    """A chunk: its tag (4 characters), the size of its data, its data (4 bytes aligned)"""
    data = bytes(data)
    if len(data) % 4:
        data += b'\0' * (4 - len(data) % 4)
    return (tag.encode('ascii') if isinstance(tag, str) else bytes(tag)) + struct.pack('>I', len(data)) + data


def name16(s):
    b = s.encode('ascii', 'replace')[:15]
    return b + b'\0' * (16 - len(b))


def jom_header(center, radius):
    """The header of a JOM file: the bounding sphere of the model (16.16 fixed)"""
    return b'JOM1' + struct.pack('>HH', VERSION, 0) + struct.pack('>iiii', center[0], center[1], center[2], radius)


def vectors_chunk(tag, vectors):
    """A count and 3 values of 16.16 fixed each: "PNTS" (the points), "NTAB" (the normals of the corners), "VCOL" (the
    colors of the points)"""
    return chunk(tag, struct.pack('>I', len(vectors)) + b''.join(struct.pack('>iii', *v) for v in vectors))


def quads_chunk(quads):
    """"QUAD": the polygons, (normal (16.16 fixed), the indexes of their 4 points) each"""
    return chunk('QUAD', struct.pack('>I', len(quads)) + b''.join(struct.pack('>iiiHHHH', *n, *c) for n, c in quads))


def values_chunk(tag, values):
    """A count and 16 bits values: "QMAT" (the material of each polygon), "GOUR" (the 4 gouraud colors of each polygon),
    "CNRM" (the 4 normals of the corners of each polygon)"""
    return chunk(tag, struct.pack('>I', len(values)) + struct.pack('>%dH' % len(values), *values))


def records_chunk(tag, records):
    """A count and records: "MATL" (material_record()), "TIMG" (texture_record()), "GRUP" (group_record()), "TEXT"
    (name16())"""
    return chunk(tag, struct.pack('>I', len(records)) + b''.join(records))


def material_record(name, rgb, texture=-1, flags=0, user=0, shine=0, subdivision_error=0):
    """A material of "MATL": rgb 0 to 255, texture: its index in "TEXT" (-1: none), flags: MAT_xxx, user: the value of
    the game"""
    return name16(name) + struct.pack('>BBBBhHII', rgb[0], rgb[1], rgb[2], shine, texture, subdivision_error, flags, user)


def texture_record(name, width, height, pixels, lut=None):
    """A texture of "TIMG": RGB 555 (pixels: their values, or their bytes big endian), or 16 colors (lut: the 16
    colors, pixels: the indexes packed 2 per byte, the left pixel in the high bits). 4 bytes aligned"""
    if lut is not None:
        data = struct.pack('>16H', *lut) + bytes(pixels)
    elif isinstance(pixels, (bytes, bytearray)):
        data = bytes(pixels)
    else:
        data = struct.pack('>%dH' % len(pixels), *pixels)
    data += b'\0' * (-len(data) % 4)
    return name16(name) + struct.pack('>HHHH', width, height, 0 if lut is None else 1, 0) + data


def group_record(name, first_point, point_count, first_polygon, polygon_count):
    """A group of "GRUP": its points, its polygons"""
    return name16(name) + struct.pack('>HHHH', first_point, point_count, first_polygon, polygon_count)


def skeleton_chunks(path, groups, quads, points, scale, s_yz, point_groups):
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
            # Each point of the polygons of a bone belongs to the group of a bone (its own, or another one: a skin
            # between 2 parts), or it would not move
            _, first, end, used = groups[gi]
            bone_groups = {bb.get('group', bb['name']) for bb in bones}
            owners = {}
            for k, g in enumerate(groups):
                for i in group_points(groups, k, point_groups):
                    owners[i] = g[0]
            for i in used:
                if owners.get(i) not in bone_groups:
                    raise ModelError('bone %s: a polygon uses a point that belongs to no bone (declared in the group %s)'
                                     % (b['name'], owners.get(i)))
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
    skin = skin_chunk(rig.get('skin', True), bones, names, groups, quads, points, scale, s_yz, point_groups)
    return chunk('BONE', data), chunk('ANIM', header + body), len(bones), len(anims), skin


SKIN_ONE = 32768                # JO_3D_SKIN_ONE: a weight of 1.0
SKIN_RADIUS = 0.07              # The radius of the bend around a joint, x the height of the model (by default)
SKIN_MAX_WEIGHT = 0.5           # The weight of the other bone at the joint (both bones the same)


def skin_chunk(spec, bones, names, groups, quads, points, scale, s_yz, point_groups):
    """The "SKIN" chunk of a skeleton (the points near a joint follow both bones) and its point count, from the OBJ (see
    skin_points()). spec (JSON "skin"): true (default), false (a rigid skeleton) or {"radius": r} (in the units of the
    OBJ)"""
    if spec is False:
        return None, 0
    owner = {}
    for k, b in enumerate(bones):
        g = b.get('group', b['name'])
        for gi, grp in enumerate(groups):
            if grp[0] == g:
                for i in group_points(groups, gi, point_groups):
                    owner[i] = k
    pivots = []
    parents = []
    for b in bones:
        px, py, pz = b.get('pivot', [0, 0, 0])
        pivots.append((px * scale, py * scale * s_yz, pz * scale * s_yz))
        parents.append(-1 if b.get('parent') is None else names.index(b['parent']))
    radius = spec.get('radius') * scale if isinstance(spec, dict) and 'radius' in spec else None
    return skin_points(points, [q[0] for q in quads], owner, parents, pivots, radius)


def skin_points(points, quads, owner, parents, pivots, radius=None):
    """The "SKIN" chunk and its point count: the joints that bend are between a bone and its parent when polygons use
    points of both (a skin stretched between them, not a cap or a weapon). A point of the child nearer to such a joint
    than the radius follows the parent too, up to SKIN_MAX_WEIGHT at the joint. points: the points of the model, quads: their
    corners, owner: the bone of each point (its group), parents and pivots: the bones. radius: default SKIN_RADIUS x the
    height of the model"""
    joined = set()
    for c in quads:
        owners = {owner[i] for i in c if i in owner}
        for a in owners:
            for b in owners:
                if a != b:
                    joined.add((a, b))
    joints = []                 # (bone, other, pivot of the joint)
    for k, pk in enumerate(parents):
        if pk >= 0 and (k, pk) in joined:
            # (the points of the child follow its parent near the joint; the parent stays rigid: a torso whose points
            # followed an arm raised to the front stuck out at the shoulder)
            joints.append((k, pk, pivots[k]))
    if not joints:
        return None, 0
    if radius is None:
        ys = [p[1] for p in points]
        radius = SKIN_RADIUS * (max(ys) - min(ys))
    data = bytearray()
    count = 0
    for i, p in enumerate(points):
        if i not in owner:
            continue
        near = [(length(sub(p, pivot)), other) for bone, other, pivot in joints if bone == owner[i]]
        if not near:
            continue
        d, other = min(near)
        if d >= radius:
            continue
        t = d / radius
        w = int(round(SKIN_MAX_WEIGHT * (1 - t * t * (3 - 2 * t)) * SKIN_ONE))
        if w <= 0:
            continue
        data += struct.pack('>HBBHH', i, owner[i], other, w, 0)
        count += 1
    return chunk('SKIN', struct.pack('>I', count) + data), count


def add_skin(path, radius=None):
    """Add a skin to the skeleton of a JOM file (see skin_points(); radius in the units of the model): the file is
    rewritten with a "SKIN" chunk before its end (a former one replaced). Returns the number of points of the skin"""
    data = open(path, 'rb').read()
    m = read_jom(path)
    if 'BONE' not in m['chunks'] or 'GRUP' not in m['chunks']:
        raise ModelError('%s: no skeleton' % path)
    points = [tuple(v / 65536 for v in p) for p in m['points']]
    quads = [p[3:] for p in m['polygons']]
    c = m['chunks']['BONE']
    parents, pivots, groups = [], [], []
    for k in range(struct.unpack('>I', c[:4])[0]):
        g, parent, x, y, z = struct.unpack('>Hhiii', c[4 + k * 32 + 16:4 + k * 32 + 32])
        groups.append(g)
        parents.append(parent)
        pivots.append((x / 65536, y / 65536, z / 65536))
    owner = {}
    for k, g in enumerate(groups):
        if g != 0xFFFF:
            _, first, count, _, _ = m['groups'][g]
            for i in range(first, first + count):
                owner[i] = k
    skin, count = skin_points(points, quads, owner, parents, pivots, radius)
    # The chunks without "SKIN" and "END ", then the skin, then the end
    out = bytearray(data[:24])
    p = 24
    while p + 8 <= len(data):
        tag = data[p:p + 4]
        size = struct.unpack('>I', data[p + 4:p + 8])[0]
        if tag == b'END ':
            break
        if tag != b'SKIN':
            out += data[p:p + 8 + size]
        p += 8 + size
    if skin:
        out += skin
    out += chunk('END ', b'')
    with open(path, 'wb') as f:
        f.write(out)
    return count


MORPH_LOOP = 1 << 0
MORPH_FLAG_NORMALS = 1 << 0
MORPH_NORMALS = 256
MORPH_GOLDEN_ANGLE = 25033      # The golden angle in jo_angle (JO_3D_MORPH_GOLDEN_ANGLE of model.c)
ANGLE_TURN = 65536              # A turn in jo_angle
MORPH_VALUE_MAX = 32767
MORPH_SHIFT_MAX = 16
MORPH_HEADER = 8
MORPH_ENTRY = 28
TICKS_PER_SECOND = 60


def morph_normal_table():
    """The directions of the normals of the vertex animations (the same as jo_3d_morph_normal() of jo_engine/model.c): a
    golden angle spiral on the sphere, y from 1 to -1 (unit vectors)"""
    table = []
    for i in range(MORPH_NORMALS):
        y = 1 - (i + 0.5) * 2 / MORPH_NORMALS
        r = math.sqrt(max(0.0, 1 - y * y))
        phi = 2 * math.pi * (i * MORPH_GOLDEN_ANGLE % ANGLE_TURN) / ANGLE_TURN
        table.append((math.cos(phi) * r, y, math.sin(phi) * r))
    return table


def nearest_normal(n, table):
    """Index of the direction of the table nearest to the normal n"""
    return max(range(len(table)), key=lambda k: n[0] * table[k][0] + n[1] * table[k][1] + n[2] * table[k][2])


def read_morph_frames(path, faces, point_count, scale, s_yz):
    """The frames of the vertex animations of a JSON file (see the documentation of the module): a list of
    (name, frame ticks, loop, [the points of each frame, in the axes of the Saturn])"""
    spec = json.load(open(path))
    base = os.path.dirname(os.path.abspath(path))
    anims = spec.get('animations', [])
    if not anims:
        raise ModelError('%s: no animations' % path)
    corners = [f[0] for f in faces]
    out = []
    for a in anims:
        files = a.get('frames')
        if isinstance(files, str):
            files = [files % k for k in range(a.get('first', 0), a.get('last', a.get('first', 0)) + 1)]
        if not files:
            raise ModelError('animation %s: no frames' % a.get('name'))
        frames = []
        for f in files:
            fp = os.path.join(base, f)
            pts, _, _, fc, _, _ = read_obj(fp, [])
            # The same topology as the model: the points of a frame replace the points of the model
            if len(pts) != point_count or [x[0] for x in fc] != corners:
                raise ModelError('%s: not the same points and faces as the model' % fp)
            frames.append([(x * scale, y * scale * s_yz, z * scale * s_yz) for x, y, z in pts])
        ticks = max(1, int(round(a.get('frame_time', 1.0 / 15) * TICKS_PER_SECOND)))
        out.append((a['name'], ticks, bool(a.get('loop')), frames))
    return out


def morph_chunk(morphs, quads, lit):
    """The "MRPH" chunk: the vertex animations (morphs: (name, frame ticks, loop, frames) with the points of the model,
    welded), the normals of the polygons if some are lit. Returns (chunk, shift, frame count)"""
    largest = max(abs(fixed(v)) for _, _, _, frames in morphs for f in frames for p in f for v in p)
    shift = 0
    while shift < MORPH_SHIFT_MAX and (largest + (1 << shift) // 2) >> shift > MORPH_VALUE_MAX:
        shift += 1
    if (largest + (1 << shift) // 2) >> shift > MORPH_VALUE_MAX:
        raise ModelError('vertex animations: a point too far from the origin')
    table = morph_normal_table() if lit else None
    header = bytearray(struct.pack('>IHH', len(morphs), shift, MORPH_FLAG_NORMALS if lit else 0))
    body = bytearray()
    offset = MORPH_HEADER + MORPH_ENTRY * len(morphs)
    count = 0
    for name, ticks, loop, frames in morphs:
        header += name16(name) + struct.pack('>HHHHI', len(frames), ticks, MORPH_LOOP if loop else 0, 0, offset + len(body))
        for f in frames:
            for p in f:
                body += struct.pack('>hhh', *[(fixed(v) + (1 << shift) // 2) >> shift for v in p])
            if lit:
                for c, n, m, g in quads:
                    body.append(nearest_normal(face_normal([f[i] for i in c[::-1]]), table))
            body += b'\0' * (-len(body) % 4)
            count += 1
    return chunk('MRPH', header + body), shift, count


def group_points(groups, gi, point_groups):
    """The points of a group (its index gi): the points declared in it; none declared (the points of the OBJ before its
    groups): the points used only by its polygons"""
    name = groups[gi][0]
    own = [i for i, g in enumerate(point_groups) if g == name]
    if own:
        return own
    users = {}
    for k, (n, first, end, used) in enumerate(groups):
        for i in used:
            users.setdefault(i, set()).add(k)
    return [i for i in groups[gi][3] if users[i] == {gi}]


def weld_points(points, colors, quads, point_groups, frames=()):
    """Merge the points at the same position (same fixed point values) used by the same groups and declared in the same
    group (a group can move its points: see "GRUP"), and at the same position in all the frames of the vertex animations
    (frames: lists of points). A point is replaced by the first one of its kind: the order is kept.
    Returns (points, colors, quads, point_groups, frames)"""
    users = {}
    for c, n, m, g in quads:
        for i in c:
            users.setdefault(i, set()).add(g)
    first = {}
    remap = []
    out_points = []
    out_colors = []
    out_groups = []
    sources = []        # The point of each point kept
    for i, p in enumerate(points):
        if i not in users:
            key = ('unused', i)
        else:
            key = (tuple(fixed(v) for v in p), frozenset(users[i]), point_groups[i],
                   tuple(fixed(v) for f in frames for v in f[i]))
        if key not in first:
            first[key] = len(out_points)
            sources.append(i)
            out_points.append(p)
            out_colors.append(colors[i])
            out_groups.append(point_groups[i])
        remap.append(first[key])
    out_frames = [[f[i] for i in sources] for f in frames]
    return out_points, out_colors, [([remap[i] for i in c], n, m, g) for c, n, m, g in quads], out_groups, out_frames


def convert(obj_path, out_path=None, scale=1.0, saturn_axes=False, keep_triangles=False, remove_unused=False,
            texture_dir=None, quiet=False, embed_textures=False, skeleton=None, weld=True, morph=None, sort=True):
    """Convert an OBJ file to a JOM file. Returns a dictionary of statistics (and 'warnings')"""
    warnings = []
    if out_path is None:
        out_path = os.path.splitext(obj_path)[0] + '.JOM'
    if texture_dir is None:
        texture_dir = os.path.dirname(os.path.abspath(out_path))
    points, colors, normals, faces, materials, point_groups = read_obj(obj_path, warnings)
    if not points or not faces:
        raise ModelError('%s: no points or no faces' % obj_path)
    # Saturn axes: rotation of 180 degrees around X, the vertex order is reversed (front faces: clockwise on the screen)
    s_yz = 1.0 if saturn_axes else -1.0
    points = [(x * scale, y * scale * s_yz, z * scale * s_yz) for x, y, z in points]
    normals = [(x, y * s_yz, z * s_yz) for x, y, z in normals]
    # The frames of the vertex animations (the same points and faces as the model)
    morphs = read_morph_frames(morph, faces, len(points), scale, s_yz) if morph else []
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
    quads = split_twisted(quads, points, warnings)
    if len(quads) > 65535 or len(points) > 65535:
        raise ModelError('too many points or polygons (65535 at most)')
    if remove_unused:
        used = sorted({i for q in quads for i in q[0]})
        remap = {old: new for new, old in enumerate(used)}
        if len(used) < len(points):
            points = [points[i] for i in used]
            colors = [colors[i] for i in used]
            point_groups = [point_groups[i] for i in used]
            morphs = [(n, t, l, [[f[i] for i in used] for f in frames]) for n, t, l, frames in morphs]
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
        # (all the frames together: 2 points merged are at the same place in every frame)
        all_frames = [f for _, _, _, frames in morphs for f in frames]
        points, colors, quads, point_groups, all_frames = weld_points(points, colors, quads, point_groups, all_frames)
        welded = count - len(points)
        k = 0
        for a, (n, t, l, frames) in enumerate(morphs):
            morphs[a] = (n, t, l, all_frames[k:k + len(frames)])
            k += len(frames)
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
    # Bounding sphere (of all the frames of the vertex animations)
    every = points + [p for _, _, _, frames in morphs for f in frames for p in f]
    xs, ys_, zs = zip(*every)
    center = ((min(xs) + max(xs)) / 2, (min(ys_) + max(ys_)) / 2, (min(zs) + max(zs)) / 2)
    radius = max(length(sub(p, center)) for p in every)
    # The sort of each polygon (auto_sort(): the polygons drawn over nearer ones in views around the model sorted on
    # another point), unless its material has jo_sort: the materials are copied with the other sorts
    poly_mat = [mat_index[q[2]] for q in quads]
    sort_stats = None
    sorted_count = 0
    if sort:
        try:
            import numpy  # noqa: F401
        except ImportError:
            numpy = None
            warnings.append('numpy not found: no automatic sort of the polygons (pip install numpy)')
        if numpy is not None:
            sorts, sort_stats = auto_sort(points, [q[0] for q in quads], [mats[k].flags for k in poly_mat],
                                          [mats[k].sort_set for k in poly_mat], center, radius)
            if sort_stats is None:
                warnings.append('more than %d polygons: no automatic sort' % AUTO_SORT_MAX_POLYGONS)
            variants = {}
            for i, (k, mode) in enumerate(zip(list(poly_mat), sorts)):
                if mode == mats[k].flags & SORT_MAT_SORT:
                    continue
                if (k, mode) not in variants:
                    v = copy.copy(mats[k])
                    v.flags = (v.flags & ~SORT_MAT_SORT) | mode
                    variants[(k, mode)] = len(mats)
                    mats.append(v)
                poly_mat[i] = variants[(k, mode)]
                sorted_count += 1
    out = bytearray(jom_header([fixed(v) for v in center], fixed(radius)))
    embedded = 0
    if embed_textures and textures:
        # The textures in the file (first chunk: the loader copies them to the VRAM one at a time)
        records = []
        for t in textures:
            px = texture_pixels(texture_sources[t], warnings)
            if px is None:
                px = (8, 1, [0] * 8)
            w, h, pixels = px
            if t in lut_textures:
                lut, packed = texture_16_colors(w, h, pixels)
                records.append(texture_record(t, w, h, packed, lut))
                embedded += 32 + len(packed) + (-(32 + len(packed)) % 4)
                continue
            records.append(texture_record(t, w, h, pixels))
            embedded += w * h * 2
        out += records_chunk('TIMG', records)
    out += vectors_chunk('PNTS', [[fixed(v) for v in p] for p in points])
    # The normal of each polygon: front faces clockwise in the Saturn axes (Y down, left handed)
    out += quads_chunk([([fixed(v) for v in face_normal([points[i] for i in c[::-1]])], c) for c, n, m, g in quads])
    out += values_chunk('QMAT', poly_mat)
    out += records_chunk('MATL', [material_record(m.name, [max(0, min(255, int(round(v * 255)))) for v in m.color],
                                                  m.texture_index, m.flags, m.user, m.shine, m.subdivision_error)
                                  for m in mats])
    if textures:
        out += records_chunk('TEXT', [name16(t) for t in textures])
    if normals and all(q[1] is not None for q in quads):
        out += vectors_chunk('NTAB', [[fixed(v) for v in n] for n in normals])
        out += values_chunk('CNRM', [i for q in quads for i in q[1]])
    elif normals:
        warnings.append('some faces have no normals: the normals are not written')
    if has_colors:
        out += vectors_chunk('VCOL', [[fixed(v) for v in c] for c in colors])
        if any(m.flags & MAT_GOURAUD for m in mats):
            out += values_chunk('GOUR', [rgb555(color) if mats[mat_index[m]].flags & MAT_GOURAUD else 0
                                         for (c, n, m, g), cc in zip(quads, corner_colors) for color in cc])
    if groups:
        # The points of a group (they must be together in the OBJ): the points declared in it; an OBJ with its points
        # before its groups: the points used only by its polygons (a point shared with another group doesn't move with it)
        records = []
        for gi, (name, first, end, used) in enumerate(groups):
            own = group_points(groups, gi, point_groups)
            lo, hi = (min(own), max(own)) if own else (0, -1)
            # (the range includes points of other groups if its points are not together in the OBJ, see --info)
            if own and hi - lo + 1 != len(own):
                warnings.append('group %s: its points are not together in the OBJ' % name)
            records.append(group_record(name, lo, hi - lo + 1, first, end - first))
        out += records_chunk('GRUP', records)
    bone_count = animation_count = skin_count = 0
    if skeleton:
        bones, anims, bone_count, animation_count, (skin, skin_count) = skeleton_chunks(skeleton, groups, quads, points, scale,
                                                                                        s_yz, point_groups)
        out += bones + anims
        if skin:
            out += skin
    morph_frames = 0
    if morphs:
        lit = any(m.flags & MAT_LIGHT for m in mats)
        data, shift, morph_frames = morph_chunk(morphs, quads, lit)
        out += data
    out += chunk('END ', b'')
    with open(out_path, 'wb') as f:
        f.write(out)
    stats = {'points': len(points), 'welded': welded, 'polygons': len(quads), 'triangles': triangles, 'materials': len(mats),
             'textures': textures, 'groups': len(groups), 'bones': bone_count, 'animations': animation_count, 'skin': skin_count,
             'morphs': len(morphs), 'morph_frames': morph_frames, 'sorted': sorted_count, 'sort_wrong': sort_stats,
             'bytes': len(out), 'warnings': warnings}
    if not quiet:
        print('%s: %d points (%d merged), %d polygons (%d triangles), %d materials, %d textures%s, %d groups, %s%d bytes'
              % (os.path.basename(out_path), len(points), welded, len(quads), triangles, len(mats), len(textures),
                 ' (embedded: %d bytes)' % embedded if embedded else '', len(groups),
                 '%d vertex animations (%d frames), ' % (len(morphs), morph_frames) if morphs else '', len(out)))
        if sort_stats:
            print('  sort: %d polygons on their nearest or farthest point, pixels drawn wrong in %d views around the '
                  'model: %d -> %d' % (sorted_count, len(AUTO_SORT_ELEVATIONS) * AUTO_SORT_AZIMUTHS, sort_stats[0],
                                       sort_stats[1]))
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
    if 'SKIN' in m['chunks']:
        print('  skin: %d points follow 2 bones' % struct.unpack('>I', m['chunks']['SKIN'][:4])[0])
    if 'MRPH' in m['chunks']:
        c = m['chunks']['MRPH']
        count, shift, flags = struct.unpack('>IHH', c[:MORPH_HEADER])
        print('  vertex animations: values x 2^%d%s' % (shift, ', normals' if flags & MORPH_FLAG_NORMALS else ''))
        for k in range(count):
            a = c[MORPH_HEADER + k * MORPH_ENTRY:MORPH_HEADER + (k + 1) * MORPH_ENTRY]
            frames, ticks, flags, _, offset = struct.unpack('>HHHHI', a[16:28])
            print('  vertex animation %-16s %d frames, %d/60 s%s, offset %d' % (a[:16].split(b'\0')[0].decode(), frames,
                                                                               ticks, ', loop' if flags & MORPH_LOOP else '',
                                                                               offset))


# Painter's sort check (--check-sort): the views of the model drawn like the VDP1 and with a z-buffer
SORT_CHECK_SIZE = (512, 384)            # Pixels of a view (a line of 1 pixel on the Saturn: 1.5 pixels)
SORT_CHECK_FOCAL = 1.2                  # Focal length (x half width of the view): about the field of the engine
SORT_CHECK_DISTANCE = 2.5               # Distance of the camera to the center, x the radius of the bounding sphere
SORT_CHECK_AZIMUTHS = 24                # Views around the model
SORT_CHECK_ELEVATIONS = (10, 25, 40, 60)    # Degrees above the model (default)
SORT_CHECK_MAX_PART = 0.002             # Part of the pixels of the model drawn wrong over which a view fails
SORT_MAT_SORT = MAT_SORT_MIN | MAT_SORT_MAX
# The sorts chosen by the converter (auto_sort()): fewer views, smaller (it draws them many times)
AUTO_SORT_SIZE = (320, 240)
AUTO_SORT_ELEVATIONS = (10, 25, 40, 60)
AUTO_SORT_AZIMUTHS = 24
AUTO_SORT_ROUNDS = 4                    # Rounds of changes
AUTO_SORT_CANDIDATES = 12               # Pairs of polygons tried in a round (the ones with the most pixels drawn wrong)
AUTO_SORT_REPAIRS = 3                   # Pairs drawn wrong by a change tried with it
AUTO_SORT_MAX_POLYGONS = 4000           # Over this, the sorts of the MTL file only (too slow)


def morph_points(path, morph, frame):
    """The points of a frame of a vertex animation of a JOM file (16.16 fixed, like read_jom()['points'])"""
    m = read_jom(path)
    c = m['chunks']['MRPH']
    count, shift, flags = struct.unpack('>IHH', c[:MORPH_HEADER])
    entry = c[MORPH_HEADER + morph * MORPH_ENTRY:MORPH_HEADER + (morph + 1) * MORPH_ENTRY]
    frames, _, _, _, offset = struct.unpack('>HHHHI', entry[16:28])
    points = len(m['points'])
    size = points * 6 + (len(m['polygons']) if flags & MORPH_FLAG_NORMALS else 0)
    size += -size % 4
    base = offset + min(frame, frames - 1) * size
    values = struct.unpack('>%dh' % (points * 3), c[base:base + points * 6])
    return [tuple(v * (1 << shift) for v in values[k * 3:k * 3 + 3]) for k in range(points)]


# The most a quad can be twisted (see quad_twist()): measured (Mednafen, 2026-11-20), a quad of a hull twisted 0.41 seen in
# close-up had a hole a third of its size; computed for the VDP1 all around a quad: at 0.14 the holes are at most 5 % of
# the quad seen from the front, and grow quickly beyond
MAX_TWIST = 0.15
# A triangle of 3 corners of a quad without area: less than 1 / TWIST_FLAT_TRIANGLE of the largest one
TWIST_FLAT_TRIANGLE = 20


def quad_twist(corners):
    """How twisted a quad is: the distance of its corner farthest from
    the plane of the 3 others, over the length of its longest diagonal (0: flat, a triangle too). The VDP1 draws a quad
    in lines from its edge A-D to its edge B-C: twisted, it is a bow tie on the screen from some angles and a part of it
    is not drawn (a hole in the model). Over MAX_TWIST: a row of points more where it is twisted"""
    p = [tuple(float(v) for v in c) for c in corners]
    v = [sub(q, p[0]) for q in p]
    det = abs(sum(a * b for a, b in zip(v[1], cross(v[2], v[3]))))
    if det < 1e-12:
        return 0.0
    # (the planes of 3 corners: not the ones of a triangle without area, 2 corners almost the same: a quad that is a
    # triangle is flat, its corners a unit apart made its plane anything)
    areas = [length(cross(sub(o[1], o[0]), sub(o[2], o[0]))) for o in ([p[(k + j) % 4] for j in (1, 2, 3)] for k in range(4))]
    largest = max(areas)
    diagonal = max(length(sub(p[2], p[0])), length(sub(p[3], p[1])))
    if largest < 1e-12 or diagonal < 1e-12:
        return 0.0
    return det / min(a for a in areas if a >= largest / TWIST_FLAT_TRIANGLE) / diagonal


def split_twisted(quads, points, warnings):
    """The quads (corners, normals, material, group) with each twisted one (see quad_twist(): a hole from some angles) cut
    along its shorter diagonal into 2 triangles (their 2 same corners last, the winding kept), and a warning: the texture is
    squashed in each triangle, a row of points more in the model is better"""
    out = []
    for k, (c, n, material, group) in enumerate(quads):
        t = quad_twist([points[i] for i in c])
        if t <= MAX_TWIST:
            out.append((c, n, material, group))
            continue
        warnings.append('polygon %d (material %s) twisted %.2f (at most %.2f): cut in 2 triangles (a texture squashed in each '
                        'one): a row of points more in the model is better' % (k, material, t, MAX_TWIST))
        p = [points[i] for i in c]
        first = 0 if length(sub(p[2], p[0])) <= length(sub(p[3], p[1])) else 1
        for start in (first, first + 2):
            order = [(start + j) & 3 for j in (0, 1, 2, 2)]
            out.append(([c[j] for j in order], None if n is None else [n[j] for j in order], material, group))
    return out


def twisted_quads(points, quads, limit=MAX_TWIST):
    """The quads (4 indexes of points each) twisted more than limit: [(index, twist)]"""
    out = []
    for i, q in enumerate(quads):
        t = quad_twist([points[k] for k in q])
        if t > limit:
            out.append((i, t))
    return out


def check_twist(path, limit=MAX_TWIST, quiet=False):
    """The quads of a JOM file twisted more than limit (see quad_twist()): printed; true if there is none"""
    m = read_jom(path)
    bad = twisted_quads(m['points'], [p[3:7] for p in m.get('polygons', [])], limit)
    if not quiet:
        for i, t in bad:
            print('%s: polygon %d twisted %.2f (at most %.2f): a row of points more in the model' % (path, i, t, limit))
        print('%s: %d polygons twisted' % (path, len(bad)))
    return not bad


def triangle_pixels(np, x, y, xx, yy, w, h, margin=0.0):
    """The pixels of a triangle on the screen (corners x, y: 3 values each; xx, yy: the centers of the pixels): their
    indexes (y * w + x)"""
    area = (x[1] - x[0]) * (y[2] - y[0]) - (y[1] - y[0]) * (x[2] - x[0])
    if abs(area) < 1e-9:
        return np.zeros(0, dtype=int)
    lo_x, hi_x = max(0, int(min(x))), min(w, int(max(x)) + 1)
    lo_y, hi_y = max(0, int(min(y))), min(h, int(max(y)) + 1)
    if lo_x >= hi_x or lo_y >= hi_y:
        return np.zeros(0, dtype=int)
    px, py = xx[lo_y:hi_y, lo_x:hi_x], yy[lo_y:hi_y, lo_x:hi_x]
    l1 = ((x[2] - x[1]) * (py - y[1]) - (y[2] - y[1]) * (px - x[1])) / area
    l2 = ((x[0] - x[2]) * (py - y[2]) - (y[0] - y[2]) * (px - x[2])) / area
    inside = (l1 >= -margin) & (l2 >= -margin) & (1 - l1 - l2 >= -margin)
    return (py[inside] - 0.5).astype(int) * w + (px[inside] - 0.5).astype(int)


def vdp1_quad_pixels(np, xs, ys, xx, yy, w, h):
    """The pixels of a quad drawn like the VDP1: lines from its edge A-D to its edge B-C (strips between them here)"""
    a, b, c, d = (np.array((xs[k], ys[k])) for k in range(4))
    n = int(max(2, min(64, max(np.linalg.norm(d - a), np.linalg.norm(c - b)))))
    out = []
    for k in range(n):
        l0, l1 = a + (d - a) * k / n, a + (d - a) * (k + 1) / n
        r0, r1 = b + (c - b) * k / n, b + (c - b) * (k + 1) / n
        for t in ((l0, r0, r1), (l0, r1, l1)):
            out.append(triangle_pixels(np, [p[0] for p in t], [p[1] for p in t], xx, yy, w, h, 1e-3))
    return np.unique(np.concatenate(out))


class SortScene:
    """Views of a model all around it (its points in the axes of the Saturn, its quads, the flags of their materials),
    drawn like the VDP1 (no z-buffer: the painter's sort with one key per polygon, its center or with jo_sort min / max
    its nearest or farthest point, the faces turned away not drawn unless jo_dual) and with a z-buffer. The z-buffer and
    the pixels of each polygon are computed once: evaluate() draws the painter's sort of other sorts quickly. vdp1: the
    pixels of a quad in the painter's view are its lines from the edge A-D to the edge B-C, as the VDP1 draws them (a
    twisted quad is a bow tie: where it misses pixels, the painter's view has no polygon and the z-buffer has one); else
    its 2 triangles (0 1 2, 0 2 3), the same pixels as the z-buffer. Needs numpy"""

    def __init__(self, points, quads, flags, center, radius, elevations=SORT_CHECK_ELEVATIONS,
                 azimuths=SORT_CHECK_AZIMUTHS, size=None, holes=False, vdp1=False):
        import numpy as np
        self.np = np
        pts = np.asarray(points, dtype=float)
        w, h = size or SORT_CHECK_SIZE
        self.size = (w, h)
        focal = SORT_CHECK_FOCAL * w / 2
        center = np.asarray(center, dtype=float)
        self.views = []         # (elevation, azimuth, [(polygon, z min, z max, z mean, pixels)], z-buffer ids, holes)
        yy, xx = np.mgrid[0:h, 0:w] + 0.5
        for el in elevations:
            for k in range(azimuths):
                az = 2 * math.pi * k / azimuths
                e = math.radians(el)
                # Camera: Saturn axes (X right, Y down, Z forward, left handed): above the model is -Y
                fwd = np.array([-math.sin(az) * math.cos(e), math.sin(e), -math.cos(az) * math.cos(e)])
                eye = center - fwd * radius * SORT_CHECK_DISTANCE
                right = np.cross(fwd, np.array([0.0, -1.0, 0.0]))
                right /= np.linalg.norm(right)
                down = np.cross(fwd, right)
                v = pts - eye
                vx, vy, vz = v @ right, v @ down, v @ fwd
                sx = w / 2 + focal * vx / vz
                sy = h / 2 + focal * vy / vz
                polys = []
                zbuf = np.full(w * h, np.inf)
                zid = np.full(w * h, -1)
                # (holes: the faces turned away too; a pixel where the nearest face is turned away shows its back:
                # nothing is drawn there, the model is seen through)
                zall = np.full(w * h, np.inf)
                back = np.zeros(w * h, dtype=bool)
                for i, (q, f) in enumerate(zip(quads, flags)):
                    q = list(q)
                    xs, ys, zs = sx[q], sy[q], vz[q]
                    if zs.min() <= 0:
                        continue
                    # (front faces: clockwise on the screen, Y down; a quad is kept when one of its 2 triangles faces)
                    t1 = (xs[1] - xs[0]) * (ys[2] - ys[0]) - (ys[1] - ys[0]) * (xs[2] - xs[0])
                    t2 = (xs[2] - xs[0]) * (ys[3] - ys[0]) - (ys[2] - ys[0]) * (xs[3] - xs[0])
                    away = not (f & MAT_DUAL_PLANE) and t1 <= 0 and t2 <= 0
                    if away and not holes:
                        continue
                    pix = []
                    for a, b, c in ((0, 1, 2), (0, 2, 3)):
                        x0, x1, x2 = xs[a], xs[b], xs[c]
                        y0, y1, y2 = ys[a], ys[b], ys[c]
                        area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0)
                        if abs(area) < 1e-9:
                            continue
                        lo_x, hi_x = max(0, int(min(x0, x1, x2))), min(w, int(max(x0, x1, x2)) + 1)
                        lo_y, hi_y = max(0, int(min(y0, y1, y2))), min(h, int(max(y0, y1, y2)) + 1)
                        if lo_x >= hi_x or lo_y >= hi_y:
                            continue
                        px, py = xx[lo_y:hi_y, lo_x:hi_x], yy[lo_y:hi_y, lo_x:hi_x]
                        l1 = ((x2 - x1) * (py - y1) - (y2 - y1) * (px - x1)) / area
                        l2 = ((x0 - x2) * (py - y2) - (y0 - y2) * (px - x2)) / area
                        l3 = 1 - l1 - l2
                        inside = (l1 >= 0) & (l2 >= 0) & (l3 >= 0)
                        idx = ((py[inside] - 0.5).astype(int) * w + (px[inside] - 0.5).astype(int))
                        # (1/z is linear on the screen)
                        z = 1 / (l1[inside] / zs[a] + l2[inside] / zs[b] + l3[inside] / zs[c])
                        if holes:
                            near_all = z < zall[idx]
                            zall[idx[near_all]] = z[near_all]
                            back[idx[near_all]] = away
                        if away:
                            continue
                        nearer = z < zbuf[idx]
                        zbuf[idx[nearer]] = z[nearer]
                        zid[idx[nearer]] = i
                        pix.append(idx)
                    if pix and vdp1:
                        pix = [vdp1_quad_pixels(np, xs, ys, xx, yy, w, h)]
                    if pix:
                        polys.append((i, zs.min(), zs.max(), zs.mean(), np.concatenate(pix)))
                self.views.append((el, math.degrees(az), polys, zid, back if holes else None))

    def draw(self, view, sorts):
        """The painter's view of a view (ids of the polygons, -1: none) with the sort of each polygon (its flags:
        MAT_SORT_MIN, MAT_SORT_MAX or 0: its center)"""
        np = self.np
        w, h = self.size
        polys = view[2]
        keys = []
        for i, zmin, zmax, zmean, pix in polys:
            f = sorts[i]
            # (the keys of the engine are whole units; the same key: the order of the model)
            keys.append((-int(zmin if f & MAT_SORT_MIN else zmax if f & MAT_SORT_MAX else zmean), i, pix))
        painter = np.full(w * h, -1)
        for _, i, pix in sorted(keys, key=lambda k: (k[0], k[1])):
            painter[pix] = i
        return painter

    def wrong(self, view, painter, opening=True):
        """The pixels drawn wrong of a view: another polygon than the nearest one covers them, without the edges of 1
        pixel between 2 polygons (an opening 2 x 2) unless opening is False"""
        np = self.np
        w, h = self.size
        zid = view[3]
        bad = (painter != zid) & (zid >= 0)
        return self.opening(bad) if opening else bad

    def opening(self, mask):
        """A mask of the pixels of a view without its lines of 1 pixel (an opening 2 x 2)"""
        np = self.np
        w, h = self.size
        bad = mask.reshape(h, w)
        core = bad[:-1, :-1] & bad[1:, :-1] & bad[:-1, 1:] & bad[1:, 1:]
        kept = np.zeros_like(bad)
        for dy in (0, 1):
            for dx in (0, 1):
                kept[dy:h - 1 + dy, dx:w - 1 + dx] |= core
        return kept.reshape(-1)

    def evaluate(self, sorts, opening=True):
        """Pixels drawn wrong in all the views with these sorts, and how many by (polygon drawn, nearest polygon)"""
        total = 0
        pairs = {}
        for view in self.views:
            painter = self.draw(view, sorts)
            kept = self.wrong(view, painter, opening)
            n = int(kept.sum())
            if n:
                total += n
                for a, b in zip(painter[kept], view[3][kept]):
                    pairs[(int(a), int(b))] = pairs.get((int(a), int(b)), 0) + 1
        return total, pairs


def sort_moves(pairs, sorts, fixed, count):
    """The changes of sort to try for the pairs (polygon drawn, nearest polygon) with the most pixels drawn wrong: the
    polygon drawn on its farthest point (it goes behind), the nearest one on its nearest point (it goes in front), or
    back to their centers"""
    moves = []
    for (a, b), n in sorted(pairs.items(), key=lambda p: -p[1])[:count]:
        for poly, mode in ((a, MAT_SORT_MAX), (b, MAT_SORT_MIN), (a, 0), (b, 0)):
            if poly >= 0 and not fixed[poly] and sorts[poly] != mode and (poly, mode) not in moves:
                moves.append((poly, mode))
    return moves


def auto_sort(points, quads, flags, fixed, center, radius):
    """Choose the sort of each polygon (the converter, by default): from their centers, a polygon drawn over a nearer
    one in the views around the model is sorted on its farthest point (it goes behind: a big face with smaller faces
    against it, a gable under the edge of a roof), or the nearer one on its nearest point (it goes in front: a step, a
    sign against a wall), the change that removes the most pixels drawn wrong first, while it removes some. A change
    that draws other polygons wrong (a door behind its step, but then behind the lawn too) is tried with the change
    that repairs them. fixed: the polygons whose material has jo_sort (kept). Returns the sorts (MAT_SORT_MIN,
    MAT_SORT_MAX or 0) and the pixels drawn wrong before and after (None: too many polygons)"""
    sorts = [f & SORT_MAT_SORT for f in flags]
    if len(quads) > AUTO_SORT_MAX_POLYGONS:
        return sorts, None
    scene = SortScene(points, quads, flags, center, radius, AUTO_SORT_ELEVATIONS, AUTO_SORT_AZIMUTHS, AUTO_SORT_SIZE)
    # (all the pixels drawn wrong: a line of 1 pixel of a face over a nearer one shows on the Saturn)
    total, pairs = scene.evaluate(sorts, False)
    first = total
    for _ in range(AUTO_SORT_ROUNDS):
        better = False
        for poly, mode in sort_moves(pairs, sorts, fixed, AUTO_SORT_CANDIDATES):
            if not total:
                break
            trial = list(sorts)
            trial[poly] = mode
            t, p = scene.evaluate(trial, False)
            if t >= total:
                # (with the change that repairs the most pixels it drew wrong)
                new = {k: n - pairs.get(k, 0) for k, n in p.items() if n > pairs.get(k, 0)}
                for poly2, mode2 in sort_moves(new, trial, fixed, AUTO_SORT_REPAIRS):
                    trial2 = list(trial)
                    trial2[poly2] = mode2
                    t2, p2 = scene.evaluate(trial2, False)
                    if t2 < t:
                        trial, t, p = trial2, t2, p2
            if t < total:
                sorts, total, pairs, better = trial, t, p, True
        if not better or not total:
            break
    return sorts, (first, total)


def sort_views(path, elevations=SORT_CHECK_ELEVATIONS, azimuths=SORT_CHECK_AZIMUTHS, points=None):
    """The views of the model of a JOM file (see SortScene), with its sorts. points: other points than the ones of the
    file (a frame of a vertex animation). Returns a list of (elevation, azimuth, pixels of the model, pixels drawn
    wrong, painter view, z-buffer view, pixels drawn wrong), the views as arrays of the ids of the polygons"""
    m = read_jom(path)
    pts = [[v / 65536 for v in p] for p in (points if points is not None else m['points'])]
    quads = [p[3:] for p in m['polygons']]
    c = m['chunks']['QMAT']
    qmat = [struct.unpack('>H', c[4 + 2 * k:6 + 2 * k])[0] for k in range(len(quads))]
    flags = [m['materials'][k]['flags'] for k in qmat]
    scene = SortScene(pts, quads, flags, m['center'], m['radius'], elevations, azimuths, holes=True)
    w, h = scene.size
    out = []
    for view in scene.views:
        painter = scene.draw(view, flags)
        kept = scene.wrong(view, painter)
        # (the holes: the back of a face seen, without the edges of 1 pixel, like the pixels drawn wrong)
        holes = scene.opening(view[4])
        out.append((view[0], view[1], int((view[3] >= 0).sum()), int(kept.sum()), painter.reshape(h, w),
                    view[3].reshape(h, w), kept.reshape(h, w), int(holes.sum()), holes.reshape(h, w)))
    return out


def check_sort(path, elevations=SORT_CHECK_ELEVATIONS, image=None, points=None, quiet=False):
    """The check of a JOM file in views all around the model (see sort_views()): the polygons drawn over nearer ones
    (the painter's sort of the VDP1 against a z-buffer) and the holes (the back of a face seen: the model is seen through,
    an edge of a roof without thickness, a face turned the wrong way). Prints the worst views. image: a PNG of the worst
    views (the polygons in false colors, the pixels drawn wrong in magenta, the holes in cyan). Returns the number of
    views that fail"""
    import numpy as np
    views = sort_views(path, elevations, points=points)
    failed = [v for v in views if v[2] and (v[3] > v[2] * SORT_CHECK_MAX_PART or v[7] > v[2] * SORT_CHECK_MAX_PART)]
    worst = sorted(views, key=lambda v: -(v[3] + v[7]) / max(1, v[2]))
    if not quiet:
        print('%s: %d views, %d fail (drawn wrong or holes over %.1f %% of the model)' % (
            path, len(views), len(failed), SORT_CHECK_MAX_PART * 100))
        for v in worst[:5]:
            el, az, n, bad, holes = v[0], v[1], v[2], v[3], v[7]
            if bad or holes:
                print('  elevation %3d, azimuth %5.1f: %d pixels drawn wrong, %d pixels of holes, of %d (%.2f %%)' % (
                    el, az, bad, holes, n, (bad + holes) * 100.0 / n))
    if image:
        from PIL import Image
        rng = np.random.default_rng(1)
        palette = rng.integers(60, 230, (65536, 3))
        rows = []
        for v in worst[:6]:
            painter, zid, kept, holes = v[4], v[5], v[6], v[8]
            views_rgb = []
            for ids in (painter, zid):
                views_rgb.append(np.where(ids[:, :, None] >= 0, palette[np.maximum(ids, 0)], 0))
            views_rgb[0] = np.where(kept[:, :, None], np.array([255, 0, 255]), views_rgb[0])
            views_rgb[0] = np.where(holes[:, :, None], np.array([0, 255, 255]), views_rgb[0])
            rows.append(np.concatenate(views_rgb, axis=1))
        Image.fromarray(np.concatenate(rows, axis=0).astype(np.uint8)).save(image)
    return len(failed)


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
        elif a == '--morph':
            opts['morph'] = args.pop(0)
        elif a == '--no-auto-sort':
            opts['sort'] = False
        elif a == '--add-skin':
            print('%d points of the skin' % add_skin(args.pop(0)))
            return 0
        elif a == '--info':
            info(args.pop(0))
            return 0
        elif a == '--check-sort':
            return 1 if check_sort(args.pop(0)) else 0
        elif a == '--check-twist':
            return 0 if check_twist(args.pop(0)) else 1
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
