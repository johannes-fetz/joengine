#!/usr/bin/env python3
"""First person shooter demo: the guards (the soldiers of the laboratory), made by tools/make_assets.py:

    models/guard.obj, .mtl, .json   a rigid skeleton (11 parts moved by bones) and its animations, converted to
                                    cd/GUARD.JOM (jo_model.py --skeleton: Y up, the model looks toward -Z)
    models/g*.png                   their textures (olive uniform, side cap with a red star, face, boots, the rifle)
    (their cries and the shots of their rifle: tools/make_sounds.py)

The model and the animations: a rigid skeleton of 12 parts, see jo_model.py --skeleton"""

import json
import math
import os

import numpy as np
from PIL import Image

import guard_skin as skin

DEMO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS = os.path.join(DEMO, 'models')
CD = os.path.join(DEMO, 'cd')
KEY = (255, 0, 255)
IMAGES = {}
BAKED = set()      # The textures baked (shared by the models of the guards)


def save_model_texture(img, name):
    """Texture of the guard: models/name.png (embedded in the JOM file by the converter)"""
    rgb = img.convert('RGB')
    IMAGES[name] = rgb
    rgb.save(os.path.join(MODELS, model_texture_file(name)))


def model_texture_file(name):
    """PNG file of a texture (GSHIRT.BIN -> gshirt.png)"""
    return os.path.splitext(name)[0].lower() + '.png'


def smooth_noise(n, octaves, seed, persistence=0.5):
    """Tileable fractal value noise (n x n), values about -1..1"""
    rng = np.random.default_rng(seed)
    out = np.zeros((n, n))
    amp = 1.0
    total = 0.0
    for o in range(octaves):
        cells = 4 << o
        if cells > n:
            break
        g = rng.uniform(-1, 1, (cells, cells))
        # bicubic-ish upscale with wrap
        x = np.arange(n) * cells / n
        i0 = np.floor(x).astype(int)
        f = x - i0
        f = f * f * (3 - 2 * f)
        i1 = (i0 + 1) % cells
        a = g[i0][:, i0] * (1 - f)[None, :] + g[i0][:, i1] * f[None, :]
        b = g[i1][:, i0] * (1 - f)[None, :] + g[i1][:, i1] * f[None, :]
        layer = a * (1 - f)[:, None] + b * f[:, None]
        out += layer * amp
        total += amp
        amp *= persistence
    return out / total




# ---------------------------------------------------------------------------
# The guard: the colors (the light is the gouraud shading), the bones
# ---------------------------------------------------------------------------

GUARD_COLORS = {
    'shirt': (0.33, 0.39, 0.22),        # The olive green uniform of the guards
    'pants': (0.31, 0.36, 0.21),
    'skin': (0.80, 0.62, 0.50),
    'boots': (0.26, 0.11, 0.09),        # Dark red brown leather
    'hair': (0.16, 0.12, 0.09),
    'helmet': (0.30, 0.36, 0.20),       # The side cap
    'belt': (0.26, 0.19, 0.11),
    'gun': (0.09, 0.09, 0.10),
    'wood': (0.45, 0.25, 0.12),         # The rifle: its wooden stock and handguard
}

# The bones: name, parent, pivot (OBJ: world units, Y up, the model looks toward -Z, the feet at Y = 0)
GUARD_BONES = [
    ('hips', None, (0, 46, 0)),
    ('torso', 'hips', (0, 50, 0)),
    ('head', 'torso', (0, 74, 0)),
    ('arm_r', 'torso', (-10.5, 70, 0)),
    ('forearm_r', 'arm_r', (-12.5, 56, -9)),
    ('arm_l', 'torso', (10.5, 70, 0)),
    ('forearm_l', 'arm_l', (13, 57, 0)),
    ('thigh_r', 'hips', (-4.5, 45, 0)),
    ('shin_r', 'thigh_r', (-4.5, 24, 0)),
    ('thigh_l', 'hips', (4.5, 45, 0)),
    ('shin_l', 'thigh_l', (4.5, 24, 0)),
    # (the side cap: a bone of its own, shot off the head it falls, see guards.c)
    ('cap', 'head', (0, 83.5, 0)),
]
GUARD_SHARED = ()
# The guards: 1.25 times the size of the first ones (their eyes at 81 x GUARD_SCALE, the player: EYE_HEIGHT = 85, see demo.h;
# guards.c: GUARD_SIZE())
GUARD_SCALE = 1.35

# The rifle held at the ready by a right handed guard (design units, the model looks toward -Z). In the game the model
# is mirrored (the Y axis of the Saturn goes down): the bones *_l (+X) are on the right of the guard as seen, the
# bones *_r on his left. The axis of the rifle goes through RIFLE_AXIS (over the right hand, on the grip) along RIFLE_DIR
# (across the body toward the front left, a little up); the points of the rifle are distances along it from there
RIFLE_AXIS = np.array((8.0, 54.5, -15.0))
RIFLE_DIR = np.array((-0.42, 0.08, -0.90)) / np.linalg.norm((-0.42, 0.08, -0.90))
RIFLE_HAND = 'forearm_l'        # The bone of the hand on the grip (his right hand in the game, see above)
RIFLE_STOCK = -15.0             # The end of the stock (beside the right hip)
RIFLE_MAGAZINE = 4.0
RIFLE_WOOD = (11.0, 21.0)       # The handguard
RIFLE_MUZZLE = 35.0
RIFLE_LEFT_HAND = 14.0          # The left hand under the handguard
RIFLE_HAND_DROP = np.array((0.0, 2.5, 0.0))     # The hands under the axis


def rifle_point(d):
    """A point of the rifle at the ready (design units): d along its axis from the grip"""
    return RIFLE_AXIS + RIFLE_DIR * d


# (the rifle: a bone of its own turned around the grip, the wrist: the rifle at the ready across the body, at the shoulder
# straight forward when he aims)
GUARD_BONES.append(('rifle', RIFLE_HAND, tuple(float(c) for c in RIFLE_AXIS)))
BONE_PIVOTS = {b: v for b, p, v in GUARD_BONES}




# The textures of the guards: baked from the color functions of tools/guard_skin.py (the uniform, the faces)
FACE_TILES = ['h00', 'h01', 'h02', 'h10', 'h11', 'h12', 'h20', 'h21', 'h22', 'hnape', 'hhairb', 'hhairt', 'ffar', 'fside']
# (the tiles of the faces: the bands of the head (from the chin up) x its sides (the front, the front corner, the side),
# the back of the neck, the back of the head, its top; the far model: its face, its sides)
FACE_SIZES = {'h0': (24, 24), 'h1': (24, 24), 'h2': (24, 24), 'hnape': (16, 16), 'hhairb': (16, 16), 'hhairt': (16, 16),
              'ffar': (16, 16), 'fside': (8, 16)}


def face_size(tile):
    if tile in FACE_SIZES:
        return FACE_SIZES[tile]
    w, h = FACE_SIZES[tile[:2]]
    return (w, h) if tile[2] == '0' else (16, h)


def save_rgb(a, name):
    """models/name.png"""
    save_model_texture(Image.fromarray(np.asarray(a, dtype=np.uint8), 'RGB'), name)


def flat_corners(c, w, h, axis='z'):
    """The corners (top left, top right, bottom right, bottom left) of a vertical patch w x h centered on c, seen from
    -Z (axis z) or from +X (axis x)"""
    c = np.array(c, dtype=float)
    if axis == 'z':
        r = np.array((w / 2, 0, 0))
    elif axis == 'back':
        r = np.array((-w / 2, 0, 0))
    else:
        r = np.array((0, 0, -w / 2))
    u = np.array((0, h / 2, 0))
    return [c - r + u, c + r + u, c + r - u, c - r - u]


def corner_slots(p, n):
    """The texture corner of each point of a face (0: top left, 1: top right, 2: bottom right, 3: bottom left; "up": the
    Y axis, or the front for a horizontal face, "right" for someone in front): the 4 corners in the order of the points
    (a turn of the corners, the one closest to the shape of the face: a face nearly a triangle gets 4 different corners)"""
    up = np.array((0.0, 1.0, 0.0)) if abs(n[1]) < 0.9 else np.array((0.0, 0.0, -1.0))
    up = up - n * np.dot(up, n)
    up /= np.linalg.norm(up)
    right = np.cross(up, n)
    center = np.mean(p, axis=0)
    targets = [(-1, 1), (1, 1), (1, -1), (-1, -1)]
    best = None
    for order in ((0, 1, 2, 3), (0, 3, 2, 1)):
        for r in range(4):
            slots = [order[(k + r) % 4] for k in range(4)]
            score = sum(float(np.dot(q - center, right)) * targets[sl][0] + float(np.dot(q - center, up)) * targets[sl][1]
                        for q, sl in zip(p, slots))
            if best is None or score > best[0]:
                best = (score, slots)
    return best[1]


def guard_textures(prefix='G', colors=None):
    """The textures shared by the guards (the uniform, the boots, the cap, the rifle): baked on patches of the body"""
    face = skin.FACES[0]
    def bake(name, corners, fn, w, h):
        save_rgb(skin.bake_tile(corners, fn, w, h), prefix + name.upper() + '.BIN')
    bake('shirt', flat_corners((9.6, 64, 0), 6, 6, 'x'), skin.jacket_color, 16, 16)
    bake('jacketb', flat_corners((0, 60, 5.6), 9, 12, 'back'), skin.jacket_color, 16, 16)
    bake('pants', flat_corners((4.5, 36, -3.6), 7, 12), skin.pants_color, 16, 16)
    bake('boots', flat_corners((4.5, 14, -3.2), 6.4, 16), skin.boots_color, 16, 16)
    bake('boott', flat_corners((4.5, 3, -6), 6, 6), skin.boots_color, 16, 8)
    bake('skin', flat_corners((0, 60, -10), 3, 3), lambda p: skin.skin_color(p, face), 8, 8)
    bake('belt', flat_corners((6, 48.75, -3), 6, 3.5), skin.belt_color, 16, 8)
    bake('beltf', flat_corners((0, 48.75, -5.2), 3.4, 3.5), skin.belt_color, 16, 16)
    bake('cap', flat_corners((4, 88.5, 0), 10, 7, 'x'), skin.cap_color, 16, 16)
    # The front of the cap: the red star
    a = skin.bake_tile(flat_corners((0, 88.5, -5.5), 8, 7), skin.cap_color, 16, 16).astype(float)
    star = []
    for k in range(10):
        t = math.pi / 2 + k * math.pi / 5
        r = 4.6 if k % 2 == 0 else 1.9
        star.append((7.5 + r * math.cos(t), 8.6 - r * math.sin(t)))
    from PIL import ImageDraw
    im = Image.new('L', (64, 64), 0)
    ImageDraw.Draw(im).polygon([(x * 4, y * 4) for x, y in star], fill=255)
    mask = np.asarray(im.resize((16, 16), Image.LANCZOS), float)[..., None] / 255
    a = a * (1 - mask) + np.array((200, 40, 32)) * mask
    save_rgb(np.clip(a, 0, 255), prefix + 'CAPF.BIN')
    # The shoulder boards: red, a stripe
    a = np.zeros((8, 16, 3)) + skin.RED_TABS
    a *= 0.9 + 0.2 * np.random.default_rng(5).random((8, 16, 1))
    a[:, 0] *= 0.7
    a[:, -1] *= 0.7
    a[0] *= 0.7
    a[-1] *= 0.7
    save_rgb(np.clip(a, 0, 255), prefix + 'EPAUL.BIN')
    rng = np.random.default_rng(71)
    a = np.zeros((16, 16, 3)) + np.array((24, 24, 26.0)) + rng.normal(0, 4, (16, 16, 1))
    a[:, ::5] *= 1.4
    save_rgb(np.clip(a, 0, 255), prefix + 'GUN.BIN')
    a = np.zeros((16, 16, 3)) + np.array((112, 62, 30.0))
    a *= (0.9 + 0.1 * np.sin(np.arange(16) * 1.7))[:, None, None]
    save_rgb(np.clip(a, 0, 255), prefix + 'WOOD.BIN')


def guard_model(name='guard', colors=None, far=False, scale=GUARD_SCALE, prefix='G', helmet=True, rifle=True):
    """A guard holding a rifle (about 1.75 m: 87 design units, times scale), 12 parts moved by the bones.
    The body is made of rings (8 sides for the head, the torso, the belt; 6 for the limbs): its shape is round, the
    proportions are the ones of a man (the head 1/7.5 of his height, the legs half of it, the shoulders 3 heads wide),
    the trousers tucked into high boots, red shoulder boards and collar tabs. Smooth
    shading: the vertex colors come from the vertex normals. The head and the front of the jacket: a texture per polygon
    baked from tools/guard_skin.py (the face of FACES[0]; the other faces: the textures of cd/GFACES.JOM, swapped by
    guards.c). models/NAME.obj, .mtl, .json (the skeleton and the animations, see Compiler/COMMON/jo_model.py --skeleton)"""
    light = np.array((0.25, 1.0, -0.35))
    light /= np.linalg.norm(light)
    groups = {b[0]: [] for b in GUARD_BONES}     # (4 points, material, flat)
    textures = {}                                 # material -> (texture name, flipped)

    def face(group, pts, mat, flat=False):
        groups[group].append(([tuple(float(x) for x in v) for v in pts], mat, flat))

    def orient(pts, outward):
        p = [np.array(v, dtype=float) for v in pts]
        n = np.cross(p[1] - p[0], p[3] - p[0])
        if np.linalg.norm(n) < 1e-9:
            n = np.cross(p[2] - p[0], p[3] - p[1])
        if np.dot(n, outward) < 0:
            p = p[::-1]
            n = -n
        return p, n / np.linalg.norm(n)

    def add_face(group, pts, mat, outward, flat=False):
        p, n = orient(pts, outward)
        face(group, p, mat, flat)

    def tile_corners(p, n):
        """The corners top left, top right, bottom right, bottom left of a face (the same as the texture coordinates of
        the OBJ below)"""
        out = [None] * 4
        for q, sl in zip(p, corner_slots(p, n)):
            out[sl] = q
        return out

    def baked_face(group, pts, outward, mat, fn, size, flat=False, flip=False, variants=None):
        """A face with its own texture (baked from fn; flip: the texture of mat flipped, mat baked elsewhere);
        variants: the face textures of the guards (fn(points, face))"""
        p, n = orient(pts, outward)
        key = mat + ('_flip' if flip else '')
        textures[key] = (prefix + mat.upper() + '.BIN', flip)
        if not flip and mat not in BAKED:
            c = tile_corners(p, n)
            if variants is not None:
                for v, f in enumerate(skin.FACES):
                    a = skin.bake_tile(c, lambda q: fn(q, f), size[0], size[1])
                    if v == 0:
                        save_rgb(a, prefix + mat.upper() + '.BIN')
                    else:
                        save_rgb(a, 'V%d%s.BIN' % (v, mat.upper()))
            else:
                save_rgb(skin.bake_tile(c, fn, size[0], size[1]), prefix + mat.upper() + '.BIN')
            BAKED.add(mat)
        face(group, p, key, flat)

    def ring(y, rx, rz, n, cx=0.0, cz=0.0):
        """n points around the axis Y (the first face looks toward -Z: the front)"""
        out = []
        for k in range(n):
            t = -math.pi / n + k * 2 * math.pi / n
            out.append(np.array((cx + rx * math.sin(t), y, cz - rz * math.cos(t))))
        return out

    def loft(group, rings, mat, top=None, bottom=None):
        """The bands between the rings (the same number of points); mat(band, side, points, outward) adds the face, or a
        material; the top closed (a dome: quads to the point top), the bottom closed (flat)"""
        n = len(rings[0])
        for i in range(len(rings) - 1):
            center = (sum(rings[i]) + sum(rings[i + 1])) / (2 * n)
            for k in range(n):
                pts = [rings[i][k], rings[i][(k + 1) % n], rings[i + 1][(k + 1) % n], rings[i + 1][k]]
                outward = np.mean(pts, axis=0) - center
                outward[1] = 0
                if isinstance(mat, str):
                    add_face(group, pts, mat, outward)
                else:
                    mat(i, k, pts, outward)
        if top is not None:
            last = rings[-1]
            c = sum(last) / n
            apex = np.array((c[0], top, c[2]))
            for k in range(0, n, 2):
                pts = [last[k], last[(k + 1) % n], last[(k + 2) % n], apex]
                outward = np.mean(pts, axis=0) - (c - np.array((0, 3, 0)))
                if isinstance(mat, str):
                    add_face(group, pts, mat, outward)
                else:
                    mat(len(rings) - 1, -1 - k // 2, pts, outward)
        if bottom is not None:
            first = rings[0]
            for k in range(1, n - 1, 2):
                pts = [first[0], first[k], first[k + 1], first[(k + 2) % n]]
                add_face(group, pts, bottom, np.array((0, -1.0, 0)), True)

    def fan(group, pts, mat, outward):
        """A flat end on a ring of points: n / 2 - 1 quads (the texture flat)"""
        n = len(pts)
        for k in range(1, n - 1, 2):
            add_face(group, [pts[0], pts[k], pts[k + 1], pts[(k + 2) % n]], mat, outward, True)

    def dome(group, pts, center, axis, mat, r=None):
        """The ball of a joint: a dome on a ring of points (the end of a limb) around center (the pivot of the joint of
        the part next to it), out along axis (unit): a band to a ring at 0.55 r (radius 0.83 r: a sphere), then the
        quads to its pole at r. With the part next to it starting at the pivot with a ring of the same radius, the ball
        fills the gap when the joint bends (its material: the one of the part next to it, hidden inside it when the
        joint is straight: a wrong order of the painter's sort shows the same color)"""
        pts = [np.array(v, dtype=float) for v in pts]
        center = np.array(center, dtype=float)
        axis = np.array(axis, dtype=float)
        n = len(pts)
        if r is None:
            r = float(np.mean([np.linalg.norm(q - center) for q in pts]))
        mid = []
        for q in pts:
            d = q - center
            d -= axis * np.dot(d, axis)
            mid.append(center + axis * 0.55 * r + d * 0.83)
        pole = center + axis * r
        for k in range(n):
            face_pts = [pts[k], pts[(k + 1) % n], mid[(k + 1) % n], mid[k]]
            add_face(group, face_pts, mat, np.mean(face_pts, axis=0) - center)
        for k in range(0, n, 2):
            face_pts = [mid[k], mid[(k + 1) % n], mid[(k + 2) % n], pole]
            add_face(group, face_pts, mat, np.mean(face_pts, axis=0) - center)

    def tube(group, a, b, ra, rb, n, mat, up=(0, 0, -1), cap_a=False, cap_b=False, ball_b=None):
        """A tapered tube (n sides) from the point a (radius ra) to the point b"""
        a = np.array(a, dtype=float)
        b = np.array(b, dtype=float)
        axis = b - a
        axis /= np.linalg.norm(axis)
        u = np.array(up, dtype=float)
        u = u - axis * np.dot(u, axis)
        if np.linalg.norm(u) < 1e-6:
            u = np.array((0.0, 1.0, 0.0)) - axis * axis[1]
        u /= np.linalg.norm(u)
        side = np.cross(axis, u)
        def rg(c, r):
            if isinstance(r, tuple):
                rs, ru = r
            else:
                rs = ru = r
            return [c + side * rs * math.sin(-math.pi / n + k * 2 * math.pi / n) + u * ru * math.cos(-math.pi / n + k * 2 * math.pi / n)
                    for k in range(n)]
        r0 = rg(a, ra)
        r1 = rg(b, rb)
        center = (a + b) / 2
        for k in range(n):
            pts = [r0[k], r0[(k + 1) % n], r1[(k + 1) % n], r1[k]]
            add_face(group, pts, mat, np.mean(pts, axis=0) - center)
        for cap, rr, out in ((cap_a, r0, -axis), (cap_b, r1, axis)):
            if cap:
                for k in range(1, n - 1, 2):
                    pts = [rr[0], rr[k], rr[k + 1], rr[(k + 2) % n]]
                    add_face(group, pts, mat, out, True)
        if ball_b is not None:
            dome(group, r1, ball_b[0], axis, ball_b[1])

    def prism(group, bottom, top, mats, flat=False):
        bottom = [np.array(v, dtype=float) for v in bottom]
        top = [np.array(v, dtype=float) for v in top]
        center = (sum(bottom) + sum(top)) / 8
        if not isinstance(mats, dict):
            mats = {k: mats for k in 'zXZx'}
        for k, key in enumerate('zXZx'):
            if key in mats:
                pts = [bottom[k], bottom[(k + 1) % 4], top[(k + 1) % 4], top[k]]
                add_face(group, pts, mats[key], np.mean(pts, axis=0) - center, flat)
        if 'y' in mats:
            add_face(group, bottom, mats['y'], np.mean(bottom, axis=0) - center, True)
        if 'Y' in mats:
            add_face(group, top, mats['Y'], np.mean(top, axis=0) - center, True)

    def frustum(group, y0, w0, d0, y1, w1, d1, mats, cx=0.0, cz=0.0, cx1=None, cz1=None):
        cx1 = cx if cx1 is None else cx1
        cz1 = cz if cz1 is None else cz1
        def rg(y, w, d, x, z):
            return [(x - w / 2, y, z - d / 2), (x + w / 2, y, z - d / 2), (x + w / 2, y, z + d / 2), (x - w / 2, y, z + d / 2)]
        prism(group, rg(y0, w0, d0, cx, cz), rg(y1, w1, d1, cx1, cz1), mats)

    def box(group, x0, y0, z0, x1, y1, z1, mat, faces='xXyYzZ'):
        mats = {k: mat for k in faces}
        frustum(group, y0, x1 - x0, z1 - z0, y1, x1 - x0, z1 - z0, mats, (x0 + x1) / 2, (z0 + z1) / 2)

    # --- The legs: the breeches (fuller over the knee), tucked into the high boots (to the knee), the feet forward
    for side, x in (('r', -4.4), ('l', 4.4)):
        # (the joints closed: the top of the thigh under the belt (a flat end, the color of the belt), the knee (the ball
        # of the boot at the end of the thigh: the boot wraps the knee when it bends), the sole of the boot (he lies on the
        # floor): a bent joint showed the inside of the parts, see dome())
        if far:
            frustum('thigh_' + side, 23, 6.0, 6.4, 48.5, 7.8, 7.8, {'z': 'pants', 'X': 'pants', 'Z': 'pants', 'x': 'pants', 'Y': 'belt', 'y': 'boots'},
                    x, 0.2, x, 0)
            frustum('shin_' + side, 0, 5.6, 6.0, 25, 6.4, 6.6, {'z': 'boots', 'X': 'boots', 'Z': 'boots', 'x': 'boots', 'Y': 'boots'}, x, 0)
            box('shin_' + side, x - 2.8, 0, -9.0, x + 2.8, 5, -2.8, 'boott', faces='Yyzx X'.replace(' ', ''))
            continue
        knee = ring(24.0, 3.0, 3.2, 6, x, 0.1)
        hip_top = ring(48.5, 4.0, 4.1, 6, x, 0)
        loft('thigh_' + side, [knee, hip_top], 'pants')
        fan('thigh_' + side, hip_top, 'belt', np.array((0, 1.0, 0)))
        dome('thigh_' + side, knee, (x, 24.0, 0.1), (0, -1.0, 0), 'boots')
        # (the boot: its shaft from the ankle to the knee, its top edge a little wider)
        loft('shin_' + side, [ring(6.0, 2.6, 2.9, 6, x, 0.0), ring(24.0, 3.2, 3.4, 6, x, 0)], 'boots')
        frustum('shin_' + side, 0, 5.6, 12.6, 6.5, 5.6, 6.6, {'z': 'boott', 'X': 'boott', 'Z': 'boots', 'x': 'boott', 'Y': 'boott', 'y': 'boots'},
                x, -3.0, x, -0.1)
    # --- The hips (the breeches), the belt with its buckle
    if far:
        frustum('hips', 42, 15.5, 9.0, 50.5, 16.0, 9.4, {'z': 'beltf', 'X': 'belt', 'Z': 'belt', 'x': 'belt'})
    else:
        def belt(i, k, pts, outward):
            if k == 0:
                baked_face('hips', pts, outward, 'beltf', skin.belt_color, (16, 16))
            else:
                add_face('hips', pts, 'belt', outward)
        belt_bottom = ring(46.8, 8.35, 5.25, 8)
        loft('hips', [belt_bottom, ring(50.6, 8.35, 5.25, 8)], belt)
        fan('hips', belt_bottom, 'pants', np.array((0, -1.0, 0)))
    # --- The torso: the waist, the chest, the shoulders sloping to the neck; the collar standing with its red tabs
    # (the shoulders: the widest ring, then the slope to the neck, the shoulder boards lie on it)
    SHOULDER_Y, SHOULDER_X = 68.3, 11.0
    torso_rings = [ring(48.0, 7.9, 4.9, 8), ring(62.0, 9.6, 5.9, 8), ring(SHOULDER_Y, SHOULDER_X, 5.3, 8), ring(72.8, 2.75, 2.6, 8, 0, 0.3)]
    if far:
        frustum('torso', 48, 15.8, 9.6, 71, 18.4, 10.4, {'z': 'jfar', 'X': 'shirt', 'Z': 'jacketb', 'x': 'shirt', 'Y': 'shirt'})
        if 'jfar' not in BAKED:
            save_rgb(skin.bake_tile(flat_corners((0, 59.5, -5.6), 16, 23), skin.jacket_color, 16, 24), prefix + 'JFAR.BIN')
            BAKED.add('jfar')
        textures['jfar'] = (prefix + 'JFAR.BIN', False)
    else:
        def jacket(i, k, pts, outward):
            if k in (0, 1, 7):
                mat = 'j%d%d' % (i, 0 if k == 0 else 1)
                baked_face('torso', pts, outward, mat, skin.jacket_color, (24, 24) if k == 0 else (16, 24), flip=(k == 7))
            elif k in (2, 6):
                add_face('torso', pts, 'shirt', outward)
            else:
                add_face('torso', pts, 'jacketb', outward)
        loft('torso', torso_rings, jacket)
        # The red shoulder boards on the slope of the shoulders (the slope: from the shoulder ring to the neck ring)
        def slope_y(x):
            return SHOULDER_Y + (SHOULDER_X - abs(x)) / (SHOULDER_X - 2.75) * (72.8 - SHOULDER_Y)
        for sx in (-1, 1):
            lo, hi = sorted((sx * 6.0, sx * 9.4))
            prism('torso', [(lo, slope_y(lo) - 0.2, -1.6), (hi, slope_y(hi) - 0.2, -1.6), (hi, slope_y(hi) - 0.2, 1.6), (lo, slope_y(lo) - 0.2, 1.6)],
                  [(lo, slope_y(lo) + 0.5, -1.6), (hi, slope_y(hi) + 0.5, -1.6), (hi, slope_y(hi) + 0.5, 1.6), (lo, slope_y(lo) + 0.5, 1.6)],
                  {'z': 'epaul', 'Z': 'epaul', 'Y': 'epaul', ('X' if sx > 0 else 'x'): 'epaul'})
        def collar(i, k, pts, outward):
            if k in (0, 1, 7):
                baked_face('torso', pts, outward, 'c%d' % (0 if k == 0 else 1), skin.jacket_color, (16, 8), flip=(k == 7))
            else:
                add_face('torso', pts, 'collar', outward)
        # The neck
        tube('head', (0, 71.5, 0.3), (0, 76.6, 0.5), 2.45, 2.35, 4, 'neck', cap_a=True)
        if 'neck' not in BAKED:
            save_rgb(skin.bake_tile(flat_corners((0, 74, -2.3), 3, 5), lambda q: skin.skin_color(q, skin.FACES[0]) * 0.92, 8, 8),
                     prefix + 'NECK.BIN')
            BAKED.add('neck')
    # --- The head: the chin, the cheeks, the brow, the top (a dome): 8 sides, a texture per polygon in front
    head_rings = [ring(75.4, 2.9, 3.6, 8, 0, -0.35), ring(78.5, 3.55, 4.35, 8, 0, -0.15), ring(82.0, 3.75, 4.55, 8),
                  ring(85.6, 3.6, 4.4, 8, 0, 0.1)]
    if far:
        frustum('head', 75.4, 6.4, 7.6, 86.0, 7.2, 8.8, {'z': 'ffar', 'X': 'fside', 'Z': 'hhairb', 'x': 'fside'}, 0, -0.1, 0, 0.1)
        for t, c, sz, axis in (('ffar', flat_corners((0, 80.7, -4.4), 6.8, 10.6), (16, 16), 'z'),
                               ('fside', flat_corners((3.6, 80.7, 0), 8.2, 10.6, 'x'), (8, 16), 'x')):
            textures[t] = (prefix + t.upper() + '.BIN', False)
            if t not in BAKED:
                for v, f in enumerate(skin.FACES):
                    a = skin.bake_tile(c, lambda q: skin.head_color(q, f), sz[0], sz[1])
                    save_rgb(a, (prefix + t.upper() if v == 0 else 'V%d%s' % (v, t.upper())) + '.BIN')
                BAKED.add(t)
        textures['hhairb'] = (prefix + 'HHAIRB.BIN', False)
        frustum('cap' if helmet else 'head', 84.9, 7.5, 10.4, 90.2, 2.6, 9.8, {'z': 'capf', 'X': 'cap', 'Z': 'cap', 'x': 'cap', 'Y': 'cap', 'y': 'cap'})
    else:
        def head(i, k, pts, outward):
            if k < 0:
                baked_face('head', pts, outward, 'hhairt', skin.head_color, face_size('hhairt'), variants=True)
            elif k in (0, 1, 2, 6, 7):
                kk = {0: 0, 1: 1, 7: 1, 2: 2, 6: 2}[k]
                mat = 'h%d%d' % (i, kk)
                baked_face('head', pts, outward, mat, skin.head_color, face_size(mat), flip=k in (6, 7), variants=True)
            else:
                # (the back of the head: the same texture on the 3 sides)
                mat = 'hnape' if i == 0 else 'hhairb'
                if mat in BAKED or k == 4:
                    baked_face('head', pts, outward, mat, skin.head_color, face_size(mat), variants=True)
                else:
                    later.append((i, k, pts, outward))
        later = []
        loft('head', head_rings, head, top=87.6)
        for i, k, pts, outward in later:
            head(i, k, pts, outward)
        if helmet:
            # The side cap of the guards: narrow across, long, higher in front (the red star)
            frustum('cap', 84.9, 7.5, 10.6, 90.2, 2.6, 10.0, {'z': 'capf', 'X': 'cap', 'Z': 'cap', 'x': 'cap', 'Y': 'cap', 'y': 'cap'}, 0, 0.0, 0, -0.2)
    # --- The arms: the shoulders (the sleeve around the joint, the red shoulder board on it), the upper arms, the
    # forearms, the hands. A right handed guard holds the rifle at the ready (see RIFLE_AXIS): the right hand on the grip
    # beside the right hip, the stock along the right side, the barrel across the body toward the front left, the left
    # hand under the handguard
    def back(a, b, d):
        a = np.array(a, dtype=float)
        b = np.array(b, dtype=float)
        return tuple(a + (a - b) / np.linalg.norm(a - b) * d)
    grip, left = rifle_point(0) - RIFLE_HAND_DROP, rifle_point(RIFLE_LEFT_HAND) - RIFLE_HAND_DROP
    for sx, arm, fore, hand in ((-1, 'arm_r', 'forearm_r', left), (1, 'arm_l', 'forearm_l', grip)):
        shoulder = (sx * 10.5, 70.5, 0)
        elbow = np.array(BONE_PIVOTS[fore], dtype=float)
        along = (hand - elbow) / np.linalg.norm(hand - elbow)
        wrist = hand - along * 2.2
        if far:
            tube(arm, back(shoulder, elbow, 1.5), elbow, 3.0, 2.4, 4, 'shirt', cap_a=True, cap_b=True)
            tube(fore, elbow, wrist, 2.4, 2.0, 4, 'shirt', cap_a=True)
            continue
        # (the shoulder closed by a flat end inside the torso; the elbow: the ball at the end of the arm, the forearm
        # starts at the joint with a ring a little wider than the ball)
        tube(arm, back(shoulder, elbow, 0.5), elbow, (2.9, 3.1), (2.25, 2.4), 6, 'shirt', up=(0, 0, -1), cap_a=True,
             ball_b=(elbow, 'shirt'))
        tube(fore, elbow, wrist, 2.45, 2.0, 4, 'shirt', up=(0, 1, 0))
        tube(fore, wrist - along * 0.6, hand + along * 1.8, (1.9, 2.3), (1.7, 2.1), 4, 'skin', up=(0, 1, 0), cap_b=True)
    if far and rifle:
        tube('rifle', rifle_point(RIFLE_STOCK), rifle_point(RIFLE_MUZZLE), (1.2, 2.2), (1.2, 2.2), 4, 'gun', up=(0, 1, 0))
    if not far and rifle:
        # The rifle (its bone: it moves with the right hand): the stock and the receiver, the wooden
        # handguard, the barrel, the magazine under the receiver in front of the grip (rifle=False: the dead guard, his
        # rifle dropped on the floor)
        tube('rifle', rifle_point(RIFLE_STOCK), rifle_point(RIFLE_WOOD[0]), (1.3, 2.5), (1.3, 2.5), 4, 'gun', up=(0, 1, 0), cap_a=True)
        tube('rifle', rifle_point(RIFLE_WOOD[0]), rifle_point(RIFLE_WOOD[1]), (1.4, 2.6), (1.4, 2.6), 4, 'wood', up=(0, 1, 0), cap_b=True)
        tube('rifle', rifle_point(RIFLE_WOOD[1]), rifle_point(RIFLE_MUZZLE), 0.6, 0.6, 4, 'gun', up=(0, 1, 0), cap_b=True)
        mag = rifle_point(RIFLE_MAGAZINE)
        tube('rifle', mag, mag + np.array((0, -5.5, 0)) + RIFLE_DIR * 2, (0.9, 1.5), (0.9, 1.5), 4, 'gun', up=tuple(RIFLE_DIR), cap_b=True)
    # The OBJ: the groups in the order of the bones, their points together (shared by the faces of the group, never by 2
    # groups). The texture of each face is drawn upright as seen from outside (vt: the corner at the top left of the
    # texture is the corner up and left for someone in front of the face, "up" = the Y axis, or the front for a
    # horizontal face). The vertex colors: grey levels, about 0.5 leaves the texture unchanged
    out = ['# Guard (Y up, world units, looks toward -Z): generated by tools/make_assets.py, converted to cd/%s.JOM'
           % name.upper(), 'mtllib %s.mtl' % name, '', 'vt 0 1', 'vt 1 1', 'vt 1 0', 'vt 0 0', '']
    body = []
    shades = {}
    count = 0
    for bone, parent, pivot in GUARD_BONES:
        if not groups[bone]:
            continue
        body.append('g ' + bone)
        # The normals of the faces, the normals of the points (the mean of the faces around the point, the flat faces
        # keep their own normal)
        face_normals = []
        around = {}
        for pts, mat, flat in groups[bone]:
            p = [np.array(v) for v in pts]
            n = np.cross(p[1] - p[0], p[3] - p[0])
            if np.linalg.norm(n) < 1e-9:
                n = np.cross(p[2] - p[0], p[3] - p[1])
            n /= max(np.linalg.norm(n), 1e-9)
            face_normals.append(n)
            if not flat:
                for v in pts:
                    around.setdefault(v, []).append(n)
        def grey(n):
            level = 0.70 + 0.30 * max(0.0, float(np.dot(n, light))) - 0.10 * max(0.0, -n[1])
            return round(0.5 * level / 0.92, 3)
        index = {}
        current = None
        for (pts, mat, flat), n in zip(groups[bone], face_normals):
            shade = 'guard_%s' % mat
            shades[shade] = textures.get(mat, (prefix + mat.upper() + '.BIN', False))
            # The texture corners: "up" along Y (the front for a horizontal face), "right" for someone in front
            up = np.array((0.0, 1.0, 0.0)) if abs(n[1]) < 0.9 else np.array((0.0, 0.0, -1.0))
            up = up - n * np.dot(up, n)
            up /= np.linalg.norm(up)
            right = np.cross(up, n)
            p = [np.array(v) for v in pts]
            center = np.mean(p, axis=0)
            ids = []
            slots = corner_slots(p, n)
            for v, q, sl in zip(pts, p, slots):
                vn = n if flat else sum(around[v])
                vn = vn / max(np.linalg.norm(vn), 1e-9)
                g = grey(vn)
                key = tuple(round(float(x) * scale, 2) for x in v) + (g, g, g)
                if key not in index:
                    count += 1
                    index[key] = count
                    out.append('v %.2f %.2f %.2f %.3f %.3f %.3f' % key)
                uv = sl + 1
                ids.append('%d/%d' % (index[key], uv))
            if shade != current:
                body.append('usemtl ' + shade)
                current = shade
            body.append('f %s %s %s %s' % tuple(ids))
    with open(os.path.join(MODELS, name + '.obj'), 'w') as f:
        f.write('\n'.join(out + body) + '\n')
    # (no subdivision: the agent never comes 5 units near a guard, his radius and the one of the guard; the tiles of the
    # textures near the camera would cost 2 more sprites per texture)
    # (the uniform and the gear in 16 colors: 4 times less VRAM, the table of colors in the place of the texture; the faces
    # stay RGB, their skin and their eyes need more colors)
    mtl = ['# Materials of the guard: a texture per part, the light: the gouraud shading (the vertex colors)', '']
    for shade, (texture, flip) in shades.items():
        face = texture.upper().split('.')[0][1:].lower() in FACE_TILES
        mtl += ['newmtl ' + shade, 'Kd 1 1 1', 'map_Kd ' + model_texture_file(texture), 'jo_gouraud 1', 'jo_lut %d' % (0 if face else 1)] + \
               (['jo_flip h'] if flip else []) + ['']
    with open(os.path.join(MODELS, name + '.mtl'), 'w') as f:
        f.write('\n'.join(mtl) + '\n')
    anims = guard_animations()
    # (the root moves scaled in copies of the keys: a pose is shared by several keys and animations (crouch_aim in kneel,
    # crouch_fire...), scaled in place it was scaled once per use: the guard on one knee went up and down between 2 shots)
    for a in anims:
        a['keys'] = [dict(key, root=[v * scale for v in key['root']]) if 'root' in key else dict(key) for key in a['keys']]
    rig = {'bones': [dict({'name': b, 'parent': p, 'pivot': [c * scale for c in v]}, **({} if groups[b] else {'group': None}))
                     for b, p, v in GUARD_BONES],
           'animations': anims}
    with open(os.path.join(MODELS, name + '.json'), 'w') as f:
        json.dump(rig, f, indent=1)



def guard_animations():
    """The animations of the guard (degrees: + around X moves a leg or an arm forward, the torso backward)"""
    anims = []
    # Idle: he breathes (4 s), his weight goes from a leg to the other, he looks around slowly (16 keys, smooth)
    keys = []
    for k in range(16):
        t = k / 16
        c, sn = math.cos(2 * math.pi * t), math.sin(2 * math.pi * t)
        keys.append({'torso': [-1.5 * (1 + math.sin(4 * math.pi * t)) / 2, 3 * sn, 0], 'head': [1.5 * c, -14 * sn + 4 * math.sin(6 * math.pi * t), 0],
                     'hips': [0, -2 * sn, 2.5 * c], 'thigh_r': [0, 0, -2.5 * c], 'thigh_l': [0, 0, -2.5 * c],
                     'shin_r': [-3 - 3 * max(0.0, c), 0, 0], 'shin_l': [-3 - 3 * max(0.0, -c), 0, 0],
                     'arm_r': [1.5 * math.sin(4 * math.pi * t), 0, 0], 'arm_l': [1.5 * math.sin(4 * math.pi * t), 0, 0],
                     'root': [0.8 * c, -0.3 * (1 + math.sin(4 * math.pi * t)), 0]})
    anims.append({'name': 'idle', 'key_time': 0.25, 'loop': True, 'keys': keys})

    # Walk and run: a gait from the curves of the human walk (as measured by motion capture): the hip swings the leg
    # forward and back, the knee bends a little when the foot lands (it takes the weight) and a lot in the swing, the
    # pelvis turns and tilts with the steps, the body goes up and down twice per cycle (low when both feet are on the
    # floor), the chest turns against the pelvis, the head stays steady, the rifle bobs in the hands. 16 keys per cycle
    def bump(t, center, width):
        d = (t - center + 0.5) % 1.0 - 0.5
        return math.exp(-(d / width) ** 2)

    def gait(name, period, hip, knee_load, knee_swing, bob, lean, turn, arms):
        keys = []
        for k in range(16):
            t = k / 16
            pose = {}
            for side, phase in (('r', 0.0), ('l', 0.5)):
                u = (t + phase) % 1.0
                # (the hip: forward at the landing (u = 0), back at the push (u = 0.5); faster forward in the swing)
                h = hip * math.cos(2 * math.pi * u) + hip * 0.18 * math.sin(4 * math.pi * u)
                kn = 4 + knee_load * bump(u, 0.12, 0.07) + knee_swing * bump(u, 0.72, 0.11)
                pose['thigh_' + side] = [h, 0, 0]
                pose['shin_' + side] = [-kn, 0, 0]
            c2 = math.cos(4 * math.pi * t)
            pose['hips'] = [lean, turn * math.cos(2 * math.pi * t), 3 * math.sin(2 * math.pi * t)]
            pose['torso'] = [lean * 0.4, -turn * 1.2 * math.cos(2 * math.pi * t), -1.5 * math.sin(2 * math.pi * t)]
            pose['head'] = [-lean * 0.8 + 1.5 * c2, turn * 0.3 * math.cos(2 * math.pi * t), 0]
            pose['arm_r'] = [arms * c2, 0, 0]
            pose['arm_l'] = [arms * c2 - arms * 0.5 * math.cos(2 * math.pi * t), 0, 0]
            pose['root'] = [0.7 * math.sin(2 * math.pi * t), -bob * (1 + c2) / 2, 0]
            keys.append(pose)
        anims.append({'name': name, 'key_time': period / 16, 'loop': True, 'keys': keys})
    gait('walk', 1.05, 22, 14, 58, 1.4, -3, 5, 2)
    gait('run', 0.66, 36, 22, 95, 3.0, -12, 8, 4)
    # Aim: the rifle up to the shoulder (the arms up, the head down a little), then the shots (recoil). The rifle turns at
    # the wrist (its bone) from across the body to straight forward: the stock on the right shoulder, the cheek on it, the
    # barrel level, the left hand under the handguard (the angles found by a solver from RIFLE_*: the distance of the
    # stock to the shoulder, of the left hand to the handguard, the direction of the barrel, the limits of the joints)
    aim = {'arm_r': [20.1, -8.0, 10.3], 'forearm_r': [-4.2, 7.5, -4.5], 'arm_l': [21.9, 5.2, -22.0], 'forearm_l': [17.8, -5.6, 7.0],
           'rifle': [-35.2, 2.4, 34.6], 'head': [8, 0, 6], 'torso': [-2, 0, 0]}
    anims.append({'name': 'aim', 'key_time': 0.25, 'loop': False, 'keys': [{}, aim]})
    # (the recoil: the shoulder pushed back, the barrel up a little)
    recoil = dict(aim, torso=[-6, 0, 0], arm_r=[aim['arm_r'][0] + 4] + aim['arm_r'][1:], arm_l=[aim['arm_l'][0] + 4] + aim['arm_l'][1:],
                  head=[5, 0, 6])
    anims.append({'name': 'fire', 'key_time': 0.05, 'loop': False, 'keys': [aim, recoil, aim]})
    # Hit: he bends forward
    anims.append({'name': 'hit', 'key_time': 0.1, 'loop': False, 'keys': [
        {}, {'torso': [14, 0, 0], 'head': [10, 0, 0], 'hips': [-4, 0, 0], 'root': [0, -1, 0]}, {}]})
    # Death: the knees bend, he falls on his back (the hips turn of 90 degrees: the body lies on the ground)
    anims.append({'name': 'death', 'key_time': 0.18, 'loop': False, 'keys': [
        {},
        {'torso': [18, 0, 0], 'head': [12, 0, 0], 'thigh_r': [30, 0, 0], 'shin_r': [-55, 0, 0], 'thigh_l': [24, 0, 0],
         'shin_l': [-48, 0, 0], 'arm_r': [-10, 0, 0], 'arm_l': [-20, 0, -12], 'root': [0, -10, 0]},
        {'hips': [40, 0, 0], 'torso': [6, 0, 0], 'thigh_r': [36, 0, 0], 'shin_r': [-60, 0, 0], 'thigh_l': [20, 0, 0],
         'shin_l': [-40, 0, 0], 'arm_r': [-40, 0, 20], 'arm_l': [-50, 0, -30], 'head': [-10, 0, 0], 'root': [0, -24, 4]},
        {'hips': [88, 0, 0], 'torso': [-4, 0, 0], 'thigh_r': [8, 0, 0], 'shin_r': [-14, 0, 0], 'thigh_l': [-4, 0, 0],
         'shin_l': [-8, 0, 0], 'arm_r': [-80, 0, 40], 'arm_l': [-90, 0, -40], 'forearm_l': [-30, 0, 0],
         'head': [-14, 20, 0], 'root': [0, -41, 10]},
        {'hips': [92, 0, 0], 'torso': [-4, 0, 0], 'thigh_r': [6, 0, 0], 'shin_r': [-10, 0, 0], 'thigh_l': [-4, 0, 0],
         'shin_l': [-6, 0, 0], 'arm_r': [-84, 0, 44], 'arm_l': [-92, 0, -44], 'forearm_l': [-30, 0, 0],
         'head': [-14, 24, 0], 'root': [0, -42, 10]}]})
    # Hit in the head: it snaps back, he staggers
    anims.append({'name': 'hit_head', 'key_time': 0.08, 'loop': False, 'keys': [
        {}, {'head': [-30, 10, 0], 'torso': [-10, 6, 0], 'arm_l': [10, 0, -10], 'root': [0, 0, 2]},
        {'head': [-12, 4, 0], 'torso': [-4, 0, 0]}, {}]})
    # Hit on a side (the shoulder): he turns with the impact
    anims.append({'name': 'hit_side', 'key_time': 0.09, 'loop': False, 'keys': [
        {}, {'torso': [6, 24, 8], 'head': [6, -10, 0], 'arm_l': [-10, 0, -20], 'hips': [0, 6, 0]}, {}]})
    # Hit in a leg: the knee gives way, he limps a moment; in an arm: the arm thrown back, the shoulder turns
    anims.append({'name': 'hit_leg', 'key_time': 0.1, 'loop': False, 'keys': [
        {}, {'thigh_r': [-10, 0, 0], 'shin_r': [-45, 0, 0], 'hips': [6, 0, -8], 'torso': [16, 0, 6], 'head': [8, 0, 0], 'root': [0, -8, 0]},
        {'thigh_r': [10, 0, 0], 'shin_r': [-30, 0, 0], 'hips': [4, 0, -5], 'torso': [10, 0, 4], 'root': [0, -5, 0]}, {}]})
    for name, sign, arm in (('hit_arm_r', -1, 'arm_r'), ('hit_arm_l', 1, 'arm_l')):
        anims.append({'name': name, 'key_time': 0.09, 'loop': False, 'keys': [
            {}, {arm: [-25, 0, sign * 25], 'torso': [-4, sign * 28, 0], 'head': [4, sign * -12, 0], 'hips': [0, sign * 6, 0]},
            {arm: [-8, 0, sign * 8], 'torso': [0, sign * 10, 0]}, {}]})
    # Crouched: on one knee, the rifle up (he fights crouched behind the furniture), and its shots
    crouch = {'hips': [0, 0, 0], 'root': [0, -20, 0], 'thigh_r': [80, 0, 0], 'shin_r': [-80, 0, 0], 'thigh_l': [10, 0, 0],
              'shin_l': [-100, 0, 0], 'torso': [6, 0, 0]}
    crouch_aim = dict(crouch, arm_r=aim['arm_r'], forearm_r=aim['forearm_r'], arm_l=aim['arm_l'], forearm_l=aim['forearm_l'], rifle=aim['rifle'],
                      head=[4, 0, -6], torso=[2, 0, 0])
    # (down on one knee in 0.5 s: the left foot forward, the weight on it, the torso leans, the right knee goes down; up in
    # 0.55 s: he pushes on his legs, the torso forward, then straight with the rifle up)
    # (the feet on the floor: the hips lower by the bent legs, thigh 21 and shin 24 units long)
    down = {'root': [0, -5.5, 0], 'hips': [4, 0, 0], 'thigh_r': [42, 0, 0], 'shin_r': [-58, 0, 0], 'thigh_l': [30, 0, 0],
            'shin_l': [-50, 0, 0], 'torso': [14, 0, 0], 'head': [-6, 0, -4], 'arm_r': aim['arm_r'], 'forearm_r': aim['forearm_r'],
            'arm_l': aim['arm_l'], 'forearm_l': aim['forearm_l'], 'rifle': aim['rifle']}
    up = dict(down, root=[0, -7.5, 0], thigh_r=[50, 0, 0], shin_r=[-62, 0, 0], thigh_l=[46, 0, 0], shin_l=[-60, 0, 0], torso=[20, 0, 0],
              head=[-10, 0, -4])
    anims.append({'name': 'crouch', 'key_time': 0.25, 'loop': False, 'keys': [aim, down, crouch_aim]})
    anims.append({'name': 'stand', 'key_time': 0.27, 'loop': False, 'keys': [crouch_aim, up, aim]})
    # Hit on one knee: he stays on it, the torso and the head thrown back, the rifle up again
    anims.append({'name': 'hit_kneel', 'key_time': 0.1, 'loop': False, 'keys': [
        crouch_aim, dict(crouch_aim, torso=[-12, 8, 0], head=[-14, 6, -6], arm_r=[aim['arm_r'][0] - 15] + aim['arm_r'][1:],
                         arm_l=[aim['arm_l'][0] - 15] + aim['arm_l'][1:]), dict(crouch_aim, torso=[-4, 2, 0]), crouch_aim]})
    # (the poses held after them, the rifle up: guards.c, hold_rifle())
    anims.append({'name': 'aim_hold', 'key_time': 0.25, 'loop': True, 'keys': [aim, aim]})
    anims.append({'name': 'kneel', 'key_time': 0.25, 'loop': True, 'keys': [crouch_aim, crouch_aim]})
    anims.append({'name': 'crouch_fire', 'key_time': 0.05, 'loop': False, 'keys': [
        crouch_aim, dict(crouch_aim, torso=[-3, 0, 0], arm_r=recoil['arm_r'], arm_l=recoil['arm_l']), crouch_aim]})
    # Surprised: he jumps a little, looks at the player (before he raises his rifle)
    anims.append({'name': 'surprise', 'key_time': 0.12, 'loop': False, 'keys': [
        {}, {'torso': [-8, 0, 0], 'head': [-10, 0, 0], 'arm_l': [20, 0, -10], 'root': [0, 2, 0]}, {'torso': [-2, 0, 0]}]})
    # Death, falling forward: the head thrown back by the shot, the knees give way, he falls on his face (the hips turn
    # of -90 degrees: the body lies face down), the arms in front of him
    anims.append({'name': 'death_front', 'key_time': 0.16, 'loop': False, 'keys': [
        {},
        {'head': [-28, 6, 0], 'torso': [-10, 4, 0], 'arm_r': [-12, 0, 10], 'arm_l': [-16, 0, -12], 'root': [0, 0, 2]},
        {'torso': [16, 0, 0], 'head': [6, 0, 0], 'thigh_r': [30, 0, 0], 'shin_r': [-60, 0, 0], 'thigh_l': [26, 0, 0],
         'shin_l': [-55, 0, 0], 'arm_r': [10, 0, 14], 'arm_l': [6, 0, -14], 'root': [0, -12, -2]},
        {'hips': [-40, 0, 0], 'torso': [10, 0, 0], 'thigh_r': [34, 0, 0], 'shin_r': [-50, 0, 0], 'thigh_l': [26, 0, 0],
         'shin_l': [-40, 0, 0], 'arm_r': [40, 0, 20], 'arm_l': [36, 0, -24], 'head': [10, 0, 0], 'root': [0, -26, -6]},
        {'hips': [-86, 0, 0], 'torso': [2, 0, 0], 'thigh_r': [4, 0, 0], 'shin_r': [-12, 0, 0], 'thigh_l': [-2, 0, 0],
         'shin_l': [-8, 0, 0], 'arm_r': [80, 0, 40], 'arm_l': [76, 0, -44], 'forearm_l': [-20, 0, 0], 'head': [-20, 28, 0],
         'root': [0, -41, -10]},
        {'hips': [-90, 0, 0], 'torso': [2, 0, 0], 'thigh_r': [2, 0, 0], 'shin_r': [-8, 0, 0], 'thigh_l': [-2, 0, 0],
         'shin_l': [-6, 0, 0], 'arm_r': [84, 0, 44], 'arm_l': [80, 0, -46], 'forearm_l': [-20, 0, 0], 'head': [-22, 32, 0],
         'root': [0, -42, -10]}]})
    # The falls (deaths), the hits: their keys made smooth (Catmull-Rom between the poses, eased: they accelerate when
    # they fall, they settle with a small bounce on the floor) at 30 keys per second
    for a in anims:
        if a['name'] in ('death', 'death_front', 'hit', 'hit_side', 'hit_head', 'hit_leg', 'hit_arm_r', 'hit_arm_l', 'surprise', 'aim',
                         'crouch', 'stand', 'hit_kneel'):
            a['keys'], a['key_time'] = smooth_keys(a['keys'], a['key_time'], fall=a['name'].startswith('death'))
    return anims


def smooth_keys(keys, key_time, fall=False, rate=30):
    """Keys at rate per second between the keys of an animation (each one key_time long): Catmull-Rom on each angle and
    on the root; a fall accelerates (gravity) and bounces a little at its end"""
    channels = sorted({c for k in keys for c in k})
    def value(k, c):
        return np.array(keys[k].get(c, [0, 0, 0]), dtype=float)
    n = len(keys)
    out = []
    total = (n - 1) * key_time
    steps = max(2, int(round(total * rate)))
    for i in range(steps + 1):
        t = i / steps
        if fall:
            # (gravity: slow at first, fast at the end)
            t = t ** 1.6
        x = t * (n - 1)
        k = min(int(x), n - 2)
        f = x - k
        pose = {}
        for c in channels:
            p0, p1, p2, p3 = value(max(k - 1, 0), c), value(k, c), value(k + 1, c), value(min(k + 2, n - 1), c)
            v = 0.5 * (2 * p1 + (-p0 + p2) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f * f + (-p0 + 3 * p1 - 3 * p2 + p3) * f ** 3)
            pose[c] = [round(float(e), 2) for e in v]
        out.append(pose)
    if fall:
        # (the bounce on the floor: the body goes up a little and falls back)
        last = out[-1]
        for b in (1.5, 0.5, 0.0):
            out.append({c: (list(v[:1]) + [v[1] + b] + list(v[2:]) if c == 'root' else v) for c, v in last.items()})
    return out, 1.0 / rate




def guard_faces(jo_model):
    """The faces of the other guards: cd/GFACES.JOM (a quad per texture, only to load them in the VRAM), guard_faces.h
    (the names of the textures of each face, in the order of FACE_TILES: guards.c swaps them on the polygons of the head)"""
    out = ['# The faces of the guards (their textures only): generated by tools/make_guard.py', 'mtllib gfaces.mtl', '',
           'vt 0 1', 'vt 1 1', 'vt 1 0', 'vt 0 0', '']
    mtl = []
    faces = []
    names = []
    for v in range(len(skin.FACES)):
        row = []
        for t in FACE_TILES:
            png = model_texture_file(('G' + t.upper() if v == 0 else 'V%d%s' % (v, t.upper())) + '.BIN')
            row.append(jo_model.embedded_texture_name(os.path.join(MODELS, png), False, []))
            if v == 0:
                continue
            k = len(faces) // 2
            out += ['v %d 0 0' % (k * 2), 'v %d 0 0' % (k * 2 + 1), 'v %d 1 0' % (k * 2 + 1), 'v %d 1 0' % (k * 2)]
            faces += ['usemtl face%d' % k, 'f %d/4 %d/3 %d/2 %d/1' % (k * 4 + 1, k * 4 + 2, k * 4 + 3, k * 4 + 4)]
            mtl += ['newmtl face%d' % k, 'Kd 1 1 1', 'map_Kd ' + png, '']
        names.append(row)
    with open(os.path.join(MODELS, 'gfaces.obj'), 'w') as f:
        f.write('\n'.join(out + faces) + '\n')
    with open(os.path.join(MODELS, 'gfaces.mtl'), 'w') as f:
        f.write('\n'.join(mtl) + '\n')
    stats = jo_model.convert(os.path.join(MODELS, 'gfaces.obj'), os.path.join(CD, 'GFACES.JOM'), keep_triangles=True,
                             embed_textures=True, quiet=True)
    print('  GFACES.JOM: %s bytes' % stats.get('bytes'))
    h = ['/*', '** Jo Sega Saturn Engine: first person shooter demo, the faces of the guards (generated by tools/make_guard.py)', '*/', '',
         '#ifndef __GUARD_FACES_H__', '# define __GUARD_FACES_H__', '',
         '/** @brief The faces, the textures of a face (the bands of the head from the chin up: its front, its front corner, its side;',
         ' *  the back of the neck, the back of the head, its top; the far model: its face, its sides) */',
         '# define GUARD_FACE_COUNT (%d)' % len(names), '# define GUARD_FACE_TILES (%d)' % len(FACE_TILES), '',
         '/** @brief The names of the embedded textures (cd/GUARD.JOM: the first face, cd/GFACES.JOM: the others) */',
         'static const char * const guard_face_textures[GUARD_FACE_COUNT][GUARD_FACE_TILES] =', '{']
    for row in names:
        h.append('    {' + ', '.join('"%s"' % n for n in row) + '},')
    h += ['};', '', '#endif /* !__GUARD_FACES_H__ */', '']
    with open(os.path.join(DEMO, 'guard_faces.h'), 'w') as f:
        f.write('\n'.join(h))


def build_guards():
    """The textures, the model (models/guard.obj, .mtl, .json)"""
    os.makedirs(MODELS, exist_ok=True)
    guard_textures()
    guard_model()
    guard_model('guardn', rifle=False)
    # (far away: fewer polygons, the same skeleton and animations)
    guard_model('guardf', far=True)
    guard_model('guardnf', far=True, rifle=False)
    # (the converter of the engine: the skeleton and the animations, the textures embedded)
    import sys
    sys.path.insert(0, os.path.join(DEMO, '..', '..', 'Compiler', 'COMMON'))
    import jo_model
    guard_faces(jo_model)
    for name in ('guard', 'guardn', 'guardf', 'guardnf'):
        stats = jo_model.convert(os.path.join(MODELS, name + '.obj'), os.path.join(CD, name.upper() + '.JOM'), keep_triangles=True,
                                 remove_unused=True, embed_textures=True, quiet=True, skeleton=os.path.join(MODELS, name + '.json'))
        for w in stats.get('warnings', []):
            print('  %s: %s' % (name, w))
        print('  %s.JOM: %s polygons, %s bytes' % (name.upper(), stats.get('polygons'), stats.get('bytes')))
