"""Models built by a program in Python (no 3D software, no OBJ file) written as JOM files of the Jo Engine (the format
of jo_engine/jo/model.h, the chunks written by jo_model.py).

    import jo_mesh
    mesh = jo_mesh.Mesh()
    paint = jo_mesh.Material('paint', color=(200, 30, 30))
    mesh.quad([(0, 1, 0), (1, 1, 0), (1, 0, 0), (0, 0, 0)], paint)
    open('cd/MODEL.JOM', 'wb').write(jo_mesh.write(mesh))

World of the program: x east, z north, h up (meters as floats). Saturn: X = x, Y = -h, Z = z, in units of UNIT meters
(jo_fixed 16.16). A front face is clockwise seen from its front (the corners are reordered if needed), the first corner
gets the top left corner of the texture, the second one its top right corner.

A game grades its colors (a look: less vivid colors) with its own Texture and Material: subclasses that change the colors
before they are given here.
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jo_model  # noqa: E402

UNIT = 0.1                      # 1 unit of the Saturn = 10 cm

# Material flags (model.h)
DUAL_PLANE = jo_model.MAT_DUAL_PLANE
SORT_MIN = jo_model.MAT_SORT_MIN
SORT_MAX = jo_model.MAT_SORT_MAX
SORT_PREVIOUS = SORT_MIN | SORT_MAX
GOURAUD = jo_model.MAT_GOURAUD
HALF_TRANSPARENT = jo_model.MAT_HALF_TRANSPARENT
SCREEN_DOORS = jo_model.MAT_SCREEN_DOORS
LIGHT = jo_model.MAT_LIGHT
FLIP_H = jo_model.MAT_FLIP_H
FLIP_V = jo_model.MAT_FLIP_V
END_CODES = jo_model.MAT_END_CODES
MIPMAPS = jo_model.MAT_MIPMAPS
SUBDIVISION = jo_model.MAT_SUBDIVISION
SUBDIVISION_NEAR = jo_model.MAT_SUBDIVISION_NEAR
HIGH_SPEED_SHRINK = jo_model.MAT_HIGH_SPEED_SHRINK


def user(n):
    """A flag of the game (n from 0 to 15: the bits 16 to 31 of the flags of a material, read by the game)"""
    return 1 << (16 + n)


def fx(v):
    return int(round(v * 65536.0))


def saturn(p):
    """A point of the world (meters, h up) -> Saturn units"""
    return (p[0] / UNIT, -p[1] / UNIT, p[2] / UNIT)


def rgb555(r, g, b):
    """8 bits color -> RGB 555 of the Saturn (MSB set: opaque)"""
    return 0x8000 | ((int(b) >> 3) << 10) | ((int(g) >> 3) << 5) | (int(r) >> 3)


class Texture:
    """A texture: rgb (h x w x 3 uint8), alpha (h x w bool, False: transparent). colors16: 16 colors (with a table)"""

    def __init__(self, name, rgb, alpha=None, colors16=True):
        assert len(name) < 16
        self.name = name
        self.rgb = np.asarray(rgb, dtype=np.uint8)
        h, w = self.rgb.shape[:2]
        assert w % 8 == 0, '%s: width %d not a multiple of 8' % (name, w)
        self.alpha = np.ones((h, w), bool) if alpha is None else np.asarray(alpha, bool)
        self.colors16 = colors16
        self.palette = None
        self.index = None
        if colors16:
            self.quantize()

    def quantize(self):
        """16 colors: k-means in RGB 555 (the color 0 is transparent)"""
        px = self.rgb[self.alpha].reshape(-1, 3).astype(np.float64)
        # (the color 0 of a 16 colors texture is transparent, the color 15 is the end code of the VDP1: 2 of them on a
        # line of the texture stop the line, the background is seen through it; 14 colors for the pixels)
        n = 14
        rng = np.random.default_rng(len(self.name))
        uniq = np.unique((px // 8).astype(int), axis=0) * 8 + 4
        if len(uniq) <= n:
            centers = uniq.astype(np.float64)
        else:
            centers = px[rng.choice(len(px), n, replace=False)]
            for _ in range(20):
                d = ((px[:, None, :] - centers[None]) ** 2).sum(-1)
                lab = d.argmin(1)
                for k in range(n):
                    sel = px[lab == k]
                    centers[k] = sel.mean(0) if len(sel) else px[rng.integers(len(px))]
        centers = np.clip(np.round(centers), 0, 255)
        flat = self.rgb.reshape(-1, 3).astype(np.float64)
        d = ((flat[:, None, :] - centers[None]) ** 2).sum(-1)
        idx = d.argmin(1).reshape(self.alpha.shape)
        index = idx + 1
        index[~self.alpha] = 0
        pal = [0] + [rgb555(*c) for c in centers]
        while len(pal) < 16:
            pal.append(0x8000)
        self.palette = pal
        self.index = index.astype(np.uint8)

    def crop(self, name, x, y, w, h):
        """A part of the texture (x, w: multiples of 8), the same colors (16 colors: the same table, no new quantization)"""
        part = Texture.__new__(type(self))
        part.name = name
        part.rgb = self.rgb[y:y + h, x:x + w].copy()
        part.alpha = self.alpha[y:y + h, x:x + w].copy()
        part.colors16 = self.colors16
        part.palette = self.palette
        part.index = None if self.index is None else self.index[y:y + h, x:x + w].copy()
        assert w % 8 == 0 and len(name) < 16
        return part

    def preview(self):
        """The texture as the Saturn shows it (RGB 555 colors)"""
        if not self.colors16:
            return (self.rgb // 8 * 8 + 4).astype(np.uint8)
        pal = np.array([[(p & 31) << 3, ((p >> 5) & 31) << 3, ((p >> 10) & 31) << 3] for p in self.palette], np.uint8)
        return pal[self.index]

    def data(self):
        """Its record of the "TIMG" chunk"""
        h, w = self.alpha.shape
        if self.colors16:
            ix = self.index.reshape(-1)
            return jo_model.texture_record(self.name, w, h, ((ix[0::2] << 4) | ix[1::2]).astype(np.uint8), self.palette)
        r, g, b = (self.rgb[..., k].astype(np.uint16) >> 3 for k in range(3))
        v = 0x8000 | (b << 10) | (g << 5) | r
        v[~self.alpha] = 0
        return jo_model.texture_record(self.name, w, h, v.astype('>u2').tobytes())


class Material:
    """A material: color (0 to 255, the polygons without texture), texture (Texture or None), flags (DUAL_PLANE...,
    user()), value (the value of the game)"""

    def __init__(self, name, color=(255, 255, 255), texture=None, flags=GOURAUD, value=0):
        self.name = name
        self.color = tuple(int(round(v)) for v in color) if texture is None else color
        self.texture = texture
        self.flags = flags
        self.value = value


class Mesh:
    """Polygons of a model. A quad: 4 points (world, meters), a material, 4 corner colors (1.0 = the texture as it is,
    up to 1.9), the normal (world: computed if None)"""

    def __init__(self):
        self.points = []
        self.index = {}
        self.quads = []                 # (v0..v3, material, colors, normal)

    def point(self, p):
        key = tuple(int(round(c / UNIT * 16)) for c in p)
        k = self.index.get(key)
        if k is None:
            k = len(self.points)
            self.index[key] = k
            self.points.append(np.array(p, float))
        return k

    def quad(self, corners, material, colors=None, normal=None):
        """corners: 4 points (the texture: 0 top left, 1 top right, 2 bottom right, 3 bottom left), or 3 (a triangle)"""
        c = [np.asarray(p, float) for p in corners]
        if len(c) == 3:
            c.append(c[2])
        if colors is None:
            colors = [(1.0, 1.0, 1.0)] * 4
        colors = [tuple(x) for x in colors]
        if len(colors) == 3:
            colors.append(colors[2])
        if normal is None:
            # (the corners in the order of the texture seen from the front: top left, top right, bottom right;
            # in the world frame (h up, a mirror of the Saturn frame) the front is then cross(1 - 0, 2 - 0))
            normal = np.cross(c[1] - c[0], c[2] - c[0])
            if np.linalg.norm(normal) < 1e-9:
                normal = np.cross(c[2] - c[0], c[3] - c[0])
        n = np.asarray(normal, float)
        ln = np.linalg.norm(n)
        if ln < 1e-12:
            return
        n = n / ln
        idx = [self.point(p) for p in c]
        # (a corner repeated: the quad is a triangle, not dropped: it would be a hole)
        keep = []
        for k in range(4):
            if idx[k] not in [idx[j] for j in keep]:
                keep.append(k)
        if len(keep) < 3:
            return
        if len(keep) == 3:
            tri = [idx[k] for k in keep]
            self.quads.append((tri + [tri[2]], material, [colors[k] for k in keep] + [colors[keep[2]]], n))
            return
        # A quad that is not flat, without texture (the terrain far away): 2 triangles. The VDP1 shows a quad by the
        # winding of its corners on the screen: a folded quad (a crest of a hill) can be skipped while one of its
        # halves faces the camera (a hole). A textured quad is kept whole (a texture is mapped on each polygon)
        if material.texture is None and self.folded(c):
            # (the diagonal of the fold: the one whose triangles are the most flat with each other)
            d02 = self.fold_angle(c[0], c[1], c[2], c[3])
            d13 = self.fold_angle(c[1], c[2], c[3], c[0])
            order = (0, 1, 2, 3) if d02 <= d13 else (1, 2, 3, 0)
            a, b, cc, d = order
            for t in ((a, b, cc), (a, cc, d)):
                pts = [c[k] for k in t]
                nn = np.cross(pts[1] - pts[0], pts[2] - pts[0])
                if np.linalg.norm(nn) < 1e-9:
                    continue
                if np.dot(nn, n) < 0:
                    nn = -nn
                self.triangle(pts, material, [colors[k] for k in t], nn / np.linalg.norm(nn))
            return
        self.quads.append((idx, material, colors, n))

    @staticmethod
    def fold_angle(a, b, c, d):
        """The angle between the triangles (a, b, c) and (a, c, d) (radians)"""
        n1 = np.cross(b - a, c - a)
        n2 = np.cross(c - a, d - a)
        l1, l2 = np.linalg.norm(n1), np.linalg.norm(n2)
        if l1 < 1e-9 or l2 < 1e-9:
            return 0.0
        return float(np.arccos(np.clip(np.dot(n1, n2) / (l1 * l2), -1, 1)))

    def folded(self, c, limit=math.radians(8)):
        return min(self.fold_angle(c[0], c[1], c[2], c[3]), self.fold_angle(c[1], c[2], c[3], c[0])) > limit

    def triangle(self, pts, material, colors, n):
        """A triangle as a quad (A, B, C, C): its shortest edge first (A-B: the VDP1 draws it in fewer lines), the
        winding kept"""
        e = [np.linalg.norm(pts[(k + 1) % 3] - pts[k]) for k in range(3)]
        k = int(np.argmin(e))
        order = [k, (k + 1) % 3, (k + 2) % 3]
        idx = [self.point(pts[j]) for j in order]
        if len(set(idx)) < 3:
            return
        self.quads.append((idx + [idx[2]], material, [colors[j] for j in order] + [colors[order[2]]], n))

    def merge(self, other):
        for idx, m, col, n in other.quads:
            self.quad([other.points[i] for i in idx], m, col, n)

    def bounds(self):
        p = np.array([saturn(q) for q in self.points])
        lo, hi = p.min(0), p.max(0)
        c = (lo + hi) / 2
        r = np.sqrt(((p - c) ** 2).sum(1).max())
        return c, r

    def sort_scene(self, elevations=jo_model.SORT_CHECK_ELEVATIONS, azimuths=jo_model.SORT_CHECK_AZIMUTHS, size=None,
                   holes=True, vdp1=True):
        """The views of the model all around it as the VDP1 draws them and with a z-buffer (jo_model.SortScene: the
        polygons drawn over nearer ones, the holes), in the order of the polygons of the file (after check_winding())"""
        center, radius = self.bounds()
        return jo_model.SortScene([saturn(p) for p in self.points], [q[0] for q in self.quads],
                                  [q[1].flags for q in self.quads], center, radius, elevations, azimuths, size, holes, vdp1)


def fix_pairs(mesh, names):
    """The textured quads of these materials given counterclockwise: their corners swapped by pairs (0-1, 2-3: the
    texture mirrored, not transposed as check_winding() would do)"""
    for k, (idx, m, col, n) in enumerate(mesh.quads):
        if m.name not in names:
            continue
        a, b, c = (np.array(saturn(mesh.points[i])) for i in idx[:3])
        ns = np.array([n[0], -n[1], n[2]])
        if np.dot(np.cross(b - a, c - a), ns) > 0:
            mesh.quads[k] = ([idx[1], idx[0], idx[3], idx[2]], m, [col[1], col[0], col[3], col[2]], n)


def check_winding(mesh):
    """Each quad: clockwise seen from its front (Saturn frame): reordered if not (the texture is then mirrored: the
    generators give the right order)"""
    bad = 0
    for k, (idx, m, col, n) in enumerate(mesh.quads):
        a, b, c = (np.array(saturn(mesh.points[i])) for i in idx[:3])
        ns = np.array([n[0], -n[1], n[2]])
        if np.dot(np.cross(b - a, c - a), ns) > 0:
            bad += 1
            # (the corners swapped by pairs: the texture mirrored; swapping 1 and 3 transposed it, its rows seen as
            # columns: the tail lights were streaks, the lamps of the tunnel dark)
            idx = [idx[1], idx[0], idx[3], idx[2]]
            col = [col[1], col[0], col[3], col[2]]
            mesh.quads[k] = (idx, m, col, n)
    return bad


def gour(c):
    v = [max(0, min(31, int(round(16 * x)))) for x in c]
    return 0x8000 | (v[2] << 10) | (v[1] << 5) | v[0]


def write(mesh, embed=True, embedded_names=(), in_place=False):
    """The JOM file of a mesh. embed: the textures are embedded (TIMG chunk), except the ones in embedded_names
    (already embedded in another model: shared by name). in_place: a model created in place
    (jo_3d_create_model_in_place()): the chunks used only while the model is created (the embedded textures, the names
    of the textures, the gouraud colors) last, the game can read the next model over them; a file loaded by
    jo_3d_load_model() has its embedded textures first"""
    check_winding(mesh)
    mats = []
    for _, m, _, _ in mesh.quads:
        if m not in mats:
            mats.append(m)
    texs = []
    for m in mats:
        if m.texture is not None and m.texture not in texs:
            texs.append(m.texture)
    center, radius = mesh.bounds()
    out = jo_model.jom_header([fx(v) for v in center], fx(radius))
    timg = [t for t in texs if embed and t.name not in embedded_names]
    timg_chunk = jo_model.records_chunk('TIMG', [t.data() for t in timg]) if timg else b''
    if not in_place:
        out += timg_chunk
    out += jo_model.vectors_chunk('PNTS', [[fx(v) for v in saturn(p)] for p in mesh.points])
    out += jo_model.quads_chunk([((fx(n[0]), fx(-n[1]), fx(n[2])), idx) for idx, m, col, n in mesh.quads])
    out += jo_model.values_chunk('QMAT', [mats.index(m) for _, m, _, _ in mesh.quads])
    out += jo_model.records_chunk('MATL', [jo_model.material_record(m.name, m.color, texs.index(m.texture) if m.texture else -1,
                                                                    m.flags, m.value) for m in mats])
    if in_place:
        out += timg_chunk
    if texs:
        out += jo_model.records_chunk('TEXT', [jo_model.name16(t.name) for t in texs])
    # The VDP1 adds the gouraud to the texels without saturation (a channel over 31 or under 0 wraps around): the
    # colors of the corners are limited by the darkest and the brightest texels of the material
    limits = {}
    for m in mats:
        if m.texture is not None:
            # (a family of textures can share the limits of one of them: its attribute gouraud_ref)
            ref = getattr(m.texture, 'gouraud_ref', m.texture)
            pv = ref.preview().reshape(-1, 3).astype(int) >> 3
            pv = pv[ref.alpha.reshape(-1)] if ref.alpha.any() else pv
            lo, hi = pv.min(0), pv.max(0)
        else:
            lo = hi = np.array(m.color, int) >> 3
        limits[id(m)] = (16 - lo, 16 + 31 - hi)
    g = []
    for _, m, col, _ in mesh.quads:
        lo, hi = limits[id(m)]
        for c in col:
            v = [max(0, min(31, int(round(16 * x)))) for x in c]
            # (a channel out of its limits moves the 3 channels: the hue of the light is kept, a little brighter or
            # darker)
            under = max(int(lo[k]) - v[k] for k in range(3))
            if under > 0:
                v = [x + under for x in v]
            over = max(v[k] - int(hi[k]) for k in range(3))
            if over > 0:
                v = [x - over for x in v]
            v = [max(int(lo[k]), min(int(hi[k]), max(0, min(31, v[k])))) for k in range(3)]
            g.append(0x8000 | (v[2] << 10) | (v[1] << 5) | v[0])
    out += jo_model.values_chunk('GOUR', g)
    out += jo_model.chunk('END ', b'')
    return out
