#!/usr/bin/env python3
"""Make the walking character of the vertex animation demo: models/walker.obj (the model), models/frames/walk_NN.obj
(the frames of the walk), models/walk.json (the vertex animation for jo_model.py --morph), models/ground.obj (the
ground), their materials and their textures (PNG), and walker.h (the distance walked in a cycle, for main.c).

    python3 models/make_walker.py
    python3 ../../Compiler/COMMON/jo_model.py models/walker.obj -o cd/WALKER.JOM --scale 40 --embed-textures \
        --morph models/walk.json
    python3 ../../Compiler/COMMON/jo_model.py models/ground.obj -o cd/GROUND.JOM --scale 40 --embed-textures

The character is made of tubes (rings of points around its bones, shaped: the nose, the chin, the cheekbones, the ears,
the locks of the hair) skinned to a skeleton: the points near a joint follow both bones (linear blend skinning), the skin
bends at the knees, the elbows, the hips and the shoulders. Its light is baked in the vertex colors (smooth gouraud
shading, a light fixed on the character): the colors of a point are shared by its polygons, the surface looks round.
The face is a texture of 64 x 80 pixels cut in 4 x 5 parts, one for each quad of the front of the head (the VDP1 maps
a whole texture on a quad). The walk is computed from curves of the angles of the joints; the height of the hips puts
the lowest foot on the ground, the forward motion keeps the foot on the ground still (the program moves the character
at the speed of the walk: the feet don't slide). The painter's sort of each frame is checked (jo_model.check_sort()).
OBJ axes (Y up, the character faces +Z), meters. Needs numpy and Pillow.
"""
import json
import math
import os
import sys
import tempfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SAMPLE = os.path.dirname(HERE)
# The converter of the Jo Engine (its painter's sort check)
sys.path.insert(0, os.path.join(HERE, '..', '..', '..', 'Compiler', 'COMMON'))
import jo_model  # noqa: E402

SCALE = 40                      # Units of the engine per meter (jo_model.py --scale)
FRAMES = 20                     # Frames of a walk cycle (the engine interpolates the points in straight lines between
                                # 2 frames: too few frames, the limbs move in jerks)
FRAME_TICKS = 3                 # 1/60 s between 2 frames: a cycle (2 steps) in 1 s
TICKS_PER_SECOND = 60
FINE_STEPS = 720                # Steps of the integration of the forward motion
TEXTURE = 16                    # Size of the textures of the tubes
FACE_COLUMNS = 4                # The quads of the face (front of the head), a part of 16 x 16 of the face each
FACE_ROWS = 5
FACE_PART = 16
GROUND_TILES = 20               # Tiles of the ground on each side (main.c moves it by less than a tile)
GROUND_TILE = 1.0               # Meters
GROUND_TEXTURE = 32
RNG = np.random.default_rng(1977)
SHOULDER_BLEND = 0.10           # Distance to the middle (m) where the torso starts to follow the arms
SHOULDER_END = 0.20
# Sides of the tubes
HEAD_SIDES = 12                 # (a point in the middle of the front: the ridge of the nose)
TORSO_SIDES = 12
LIMB_SIDES = 8
# The light baked in the vertex colors: a color of 16/31 leaves the texture unchanged (1.0 is 31: +15, the gouraud of
# the VDP1 doesn't saturate), the faces away from the light darker
GOURAUD_NEUTRAL = 16 / 31
LIGHT = np.array([0.35, 0.8, 0.5])     # Toward the light (OBJ axes)
AMBIENT = 0.5
DIFFUSE = 0.5
# The painter's sort check of the frames (jo_model.check_sort()): every SORT_CHECK_STEP frame, these elevations
SORT_CHECK_STEP = 4
SORT_CHECK_ELEVATIONS = (10, 30)
SORT_CHECK_AZIMUTHS = 16
# The part of the pixels of the character drawn wrong over which a view fails: the arms and the legs go into the torso
# at the shoulders and the crotch (the skin is not one surface), their seams of 1 or 2 pixels (0.5 % of a view at most)
SORT_CHECK_MAX_PART = 0.006

# Sorts of some materials (jo_sort, see jo_model.py): none, the converter chooses the sort of each polygon
MATERIAL_FLAGS = {}

# Skeleton: name, parent, joint (rest pose)
BONES = [
    ('pelvis', None, (0.0, 0.95, 0.0)),
    ('spine', 'pelvis', (0.0, 1.02, 0.0)),
    ('chest', 'spine', (0.0, 1.20, 0.0)),
    ('neck', 'chest', (0.0, 1.46, 0.0)),
    ('head', 'neck', (0.0, 1.56, 0.0)),
]
for side, s in (('l', 1), ('r', -1)):
    BONES += [
        ('shoulder_' + side, 'chest', (0.205 * s, 1.40, 0.0)),
        ('elbow_' + side, 'shoulder_' + side, (0.225 * s, 1.13, 0.0)),
        ('wrist_' + side, 'elbow_' + side, (0.23 * s, 0.89, 0.0)),
        ('hip_' + side, 'pelvis', (0.09 * s, 0.90, 0.0)),
        ('knee_' + side, 'hip_' + side, (0.09 * s, 0.50, 0.0)),
        ('ankle_' + side, 'knee_' + side, (0.09 * s, 0.09, 0.0)),
    ]
BONE_INDEX = {b[0]: k for k, b in enumerate(BONES)}


# ---------------------------------------------------------------- textures (procedural, photographic look)

def smooth_noise(size, cells, seed_rng=RNG, height=None):
    """Value noise that tiles: a grid of cells random values, bicubic"""
    height = height or size
    g = np.tile(seed_rng.random((cells, cells)), (3, 3))
    img = Image.fromarray((g * 255).astype(np.uint8)).resize((size * 3, height * 3), Image.BICUBIC)
    return np.asarray(img, dtype=float)[height:2 * height, size:2 * size] / 255


def fractal(size, octaves=((2, 0.5), (4, 0.3), (8, 0.2)), height=None):
    out = np.zeros((height or size, size))
    for cells, w in octaves:
        out += smooth_noise(size, cells, height=height) * w
    return (out - out.mean()) / (out.std() + 1e-9)


def save(name, rgb):
    Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8), 'RGB').save(os.path.join(HERE, name + '.png'))
    return name + '.png'


def colorize(base, field, amount):
    return np.asarray(base, dtype=float)[None, None, :] * (1 + amount * field[:, :, None])


def tex_denim():
    """Blue denim: 2/2 twill, the white weft showing through, faded in patches"""
    n = TEXTURE
    y, x = np.mgrid[0:n, 0:n]
    twill = ((x + y) % 4 < 2).astype(float) - 0.5
    rgb = colorize((58, 84, 126), fractal(n) * 0.10 + twill * 0.08, 1.0)
    rgb += ((RNG.random((n, n)) - 0.5) * 16)[:, :, None] * np.array([0.8, 0.9, 1.0])
    fade = np.clip(smooth_noise(n, 2) - 0.55, 0, 1) * 70
    return save('denim', rgb + fade[:, :, None] * np.array([0.7, 0.8, 0.9]))


def tex_cotton():
    """A white T-shirt worn: jersey knit, light folds"""
    n = TEXTURE
    y, x = np.mgrid[0:n, 0:n]
    knit = np.where(x % 2 == 0, 0.008, -0.008)
    folds = np.sin(y * 0.8 + fractal(n) * 1.5) * 0.015
    return save('cotton', colorize((226, 224, 214), fractal(n) * 0.015 + knit + folds +
                                   (RNG.random((n, n)) - 0.5) * 0.015, 1.0))


def skin_rgb(w, h=None):
    h = h or w
    rgb = colorize((222, 176, 142), fractal(w, height=h) * 0.035 + (RNG.random((h, w)) - 0.5) * 0.04, 1.0)
    rgb[:, :, 0] += smooth_noise(w, 3, height=h) * 8
    return rgb


def tex_skin():
    return save('skin', skin_rgb(TEXTURE))


def tex_hair():
    """Black hair with brown glints: strands along the height of the texture (from the crown to the tips)"""
    n = TEXTURE
    strands = np.repeat(RNG.random((1, n)), n, axis=0)
    strands = (strands - 0.5) * 0.5 + (RNG.random((n, n)) - 0.5) * 0.2
    rgb = colorize((40, 30, 24), strands + fractal(n) * 0.1, 1.0)
    shine = np.clip(np.sin(np.arange(n) * 0.9)[None, :] * 0.5 + smooth_noise(n, 2) - 0.6, 0, 1) * 40
    return save('hair', rgb + shine[:, :, None] * np.array([1.0, 0.85, 0.7]))


def tex_shoe():
    """Dark leather shoes: grain, polish"""
    n = TEXTURE
    rgb = colorize((54, 40, 32), fractal(n) * 0.12 + (RNG.random((n, n)) - 0.5) * 0.1, 1.0)
    shine = np.clip(smooth_noise(n, 2) - 0.55, 0, 1) * 90
    return save('leather', rgb + shine[:, :, None])


def tex_face():
    """The face (64 x 80, the angles of the front of the head across: 30 to 150 degrees, from the forehead to the chin),
    cut in FACE_COLUMNS x FACE_ROWS parts. The face of a young man: thick eyebrows, almond eyes, a straight nose, the
    shading of the eye sockets, of the cheekbones and under the lower lip painted (the light of the vertex colors only
    changes from a quad to the next one)"""
    w, h = FACE_COLUMNS * FACE_PART, FACE_ROWS * FACE_PART
    rgb = skin_rgb(w, h)
    y, x = np.mgrid[0:h, 0:w].astype(float) + 0.5
    cx = w / 2

    def ell(ex, ey, rx, ry):
        return np.clip(1.4 - (((x - ex) / rx) ** 2 + ((y - ey) / ry) ** 2), 0, 1)

    def paint(mask, color, alpha=1.0):
        a = np.clip(mask, 0, 1)[:, :, None] * alpha
        rgb[:] = rgb * (1 - a) + np.asarray(color, dtype=float) * a

    def shade(mask, amount):
        rgb[:] = rgb * (1 - np.clip(mask, 0, 1)[:, :, None] * amount)

    # Hair: the fringe over the forehead (locks down to its middle), the temples
    locks = 6 + 3 * np.sin(x * 0.55) + 2 * np.sin(x * 1.7 + 1)
    paint(np.clip((locks - y) / 1.5, 0, 1), (40, 30, 24))
    paint(np.clip((np.abs(x - cx) - 27) / 2, 0, 1) * np.clip((30 - y) / 3, 0, 1), (40, 30, 24))
    # Shading: the eye sockets, the sides of the nose, the cheekbones, under the nose, under the lower lip, the jaw
    for ex in (cx - 11, cx + 11):
        shade(ell(ex, 32, 9, 5), 0.16)
        shade(ell(ex + (4 if ex > cx else -4), 52, 6, 8), 0.07)
    shade(ell(cx - 3.5, 42, 1.6, 8), 0.12)
    shade(ell(cx + 3.5, 42, 1.6, 8), 0.06)
    shade(ell(cx, 57, 5, 1.6), 0.18)
    shade(ell(cx, 69, 6, 1.8), 0.14)
    shade(np.clip((np.abs(x - cx) - 22) / 8, 0, 1) * np.clip((y - 55) / 10, 0, 1), 0.18)
    # Eyebrows (thick, rising to the outside), eyes (white, iris, pupil, glint, upper lid), the crease under them
    for sgn in (-1, 1):
        ex = cx + sgn * 11
        brow = np.clip(1.3 - np.abs(y - (26.5 - (x - ex) * sgn * 0.18)) / 1.3, 0, 1) * np.clip(1 - np.abs(x - ex) / 7.5, 0, 1)
        paint(brow * 1.4, (34, 26, 22))
        eye = ell(ex, 33, 5.2, 1.8)
        paint(eye, (232, 226, 218))
        paint(ell(ex + sgn * 0.5, 33, 2.0, 2.0) * eye, (78, 52, 34))
        paint(ell(ex + sgn * 0.5, 33, 0.9, 0.9) * eye, (14, 10, 8))
        paint(ell(ex + sgn * 0.5 - 0.7, 32.2, 0.4, 0.4), (250, 250, 250), 0.9)
        paint(np.clip(1.2 - np.abs(y - (31.3 + ((x - ex) / 5.2) ** 2 * 1.4)) / 0.8, 0, 1) * np.clip(1 - np.abs(x - ex) / 5.6, 0, 1),
              (40, 28, 24), 0.95)
        paint(np.clip(1 - np.abs(y - 35.6) / 0.7, 0, 1) * np.clip(1 - np.abs(x - ex) / 4.5, 0, 1), (176, 126, 104), 0.5)
    # Nose: the ridge lit, the tip, the nostrils
    paint(ell(cx, 44, 1.4, 9), (238, 196, 164), 0.5)
    paint(ell(cx - 2.6, 52.5, 1.3, 0.8), (110, 64, 52), 0.8)
    paint(ell(cx + 2.6, 52.5, 1.3, 0.8), (110, 64, 52), 0.8)
    # Mouth: the upper lip, the line between the lips, the lower lip (lit)
    paint(ell(cx, 61, 6.5, 1.3), (176, 104, 92), 0.8)
    paint(np.clip(1 - np.abs(y - 62.6) / 0.6, 0, 1) * np.clip(1 - np.abs(x - cx) / 7, 0, 1), (88, 44, 40), 0.9)
    paint(ell(cx, 64.4, 5.5, 1.4), (196, 126, 110), 0.8)
    names = []
    for row in range(FACE_ROWS):
        for col in range(FACE_COLUMNS):
            part = rgb[row * FACE_PART:(row + 1) * FACE_PART, col * FACE_PART:(col + 1) * FACE_PART]
            names.append(('face%d%d' % (row, col), save('face%d%d' % (row, col), part)))
    return names


def tex_cobbles():
    """Old cobblestones: Voronoi stones (one gray-beige tone each, rounded), dark sand joints; tiles"""
    n = GROUND_TEXTURE
    seeds = RNG.random((14, 2)) * n
    tones = RNG.random(14)
    y, x = np.mgrid[0:n, 0:n].astype(float) + 0.5
    d = []
    for sx, sy in seeds:
        dx = np.minimum(abs(x - sx), n - abs(x - sx))
        dy = np.minimum(abs(y - sy), n - abs(y - sy))
        d.append(np.sqrt(dx * dx + dy * dy))
    d = np.array(d)
    order = np.argsort(d, axis=0)
    first = np.take_along_axis(d, order[:1], 0)[0]
    second = np.take_along_axis(d, order[1:2], 0)[0]
    edge = second - first                                    # distance to the joint
    tone = tones[order[0]]
    base = np.array([128, 120, 108], dtype=float)
    rgb = base[None, None, :] * (0.78 + 0.36 * tone[:, :, None])
    rgb *= (1 + 0.07 * fractal(n))[:, :, None]
    dome = np.clip(edge / 4, 0, 1)                             # rounded stone: darker near the joint
    rgb *= (0.72 + 0.28 * dome)[:, :, None]
    joint = np.clip(1.6 - edge, 0, 1)[:, :, None]
    rgb = rgb * (1 - joint) + np.array([70, 62, 52]) * joint
    return save('cobbles', rgb)


# ---------------------------------------------------------------- mesh

class Mesh:
    def __init__(self):
        self.points = []        # rest position
        self.weights = []       # [(bone index, weight)]
        self.faces = []         # (4 point indexes, material, 4 uv)

    def ring(self, center, ax, bx, sides, weights, angle0, shape=None):
        """Points around center: ax, bx the 2 half axes of the ellipse (angle 0 along ax), x shape(angle) (a nose, an
        ear: 1 by default)"""
        out = []
        for j in range(sides):
            t = angle0 + 2 * math.pi * j / sides
            k = shape(t) if shape else 1.0
            p = np.array(center) + k * (math.cos(t) * np.array(ax) + math.sin(t) * np.array(bx))
            out.append(len(self.points))
            self.points.append(p)
            # (weights: the same for all the points, or a function of the point)
            self.weights.append(weights(p) if callable(weights) else weights)
        return out

    def quad(self, idx, material, outward_hint, up):
        """A quad, its front outward (counterclockwise in the OBJ seen from outside), the top left of its texture on
        the corner at the left of its top edge seen from outside (up: the up direction of the texture)"""
        p = [self.points[i] for i in idx]
        n = np.cross(p[1] - p[0], p[2] - p[0]) + np.cross(p[2] - p[0], p[3] - p[0])
        if np.dot(n, outward_hint) < 0:
            idx = idx[::-1]
            p = p[::-1]
            n = -n
        n = n / (np.linalg.norm(n) + 1e-12)
        right = np.cross(up, n)
        c = sum(p) / 4
        uv = []
        for q in p:
            uv.append((1.0 if np.dot(q - c, right) > 0 else 0.0, 1.0 if np.dot(q - c, up) > 0 else 0.0))
        self.faces.append((list(idx), material, uv))

    def tube(self, rings, sides, materials, axis_up=(0, 1, 0), caps=(False, False), angle0=None):
        """rings: [(center, ax, bx, weights[, shape])] from the bottom to the top; materials(row, side) -> material name
        (row -1: the bottom cap, len(rings) - 1: the top cap)"""
        if angle0 is None:
            angle0 = math.pi / sides                 # a quad centered on each axis
        ids = [self.ring(r[0], r[1], r[2], sides, r[3], angle0, r[4] if len(r) > 4 else None) for r in rings]
        up = np.array(axis_up, dtype=float)
        for r in range(len(ids) - 1):
            for j in range(sides):
                k = (j + 1) % sides
                idx = [ids[r][j], ids[r][k], ids[r + 1][k], ids[r + 1][j]]
                mid = (self.points[ids[r][j]] + self.points[ids[r + 1][k]]) / 2
                center = (np.array(rings[r][0]) + np.array(rings[r + 1][0])) / 2
                self.quad(idx, materials(r, j), mid - center, up)
        for end, (cap, r) in enumerate(zip(caps, (0, len(ids) - 1))):
            if not cap:
                continue
            ring = ids[r]
            outward = -up if end == 0 else up
            # The polygon of the end cut in quads (a fan of quads: 12 sides -> 5 quads, 8 sides -> 3 quads)
            k = 1
            while k + 1 < sides:
                corners = [ring[0], ring[k], ring[k + 1], ring[min(k + 2, sides - 1)]]
                k += 2
                cap_up = np.array(rings[r][2], dtype=float)
                self.quad(corners, materials(-1 if end == 0 else len(ids) - 1, -1), outward, cap_up)


def w(*pairs):
    return [(BONE_INDEX[b], x) for b, x in pairs]


def shoulder_weights(amount):
    """Weights of a point of the shoulders of the torso: the chest, and the shoulder of its side toward the arm (amount
    at the end of the shoulder): the shoulder follows the arm, the arm stays attached"""
    def weights(p):
        side = 'shoulder_l' if p[0] > 0 else 'shoulder_r'
        t = min(1.0, max(0.0, (abs(p[0]) - SHOULDER_BLEND) / (SHOULDER_END - SHOULDER_BLEND))) * amount
        return w(('chest', 1 - t), (side, t)) if t > 0 else w(('chest', 1))
    return weights


def angle_bump(center, width, height):
    """A shape of a ring (see Mesh.ring()): 1 + height around the angle center (radians)"""
    def shape(t):
        d = (t - center + math.pi) % (2 * math.pi) - math.pi
        return 1 + height * math.exp(-(d / width) ** 2)
    return shape


def shapes(*fs):
    """Several shapes of a ring together"""
    def shape(t):
        k = 1.0
        for f in fs:
            k *= f(t)
        return k
    return shape


FRONT = math.pi / 2             # The angle of the front of a ring (+Z), the sides: 0 (left, +X) and pi (right)
HEAD_STEP = 2 * math.pi / HEAD_SIDES


def head_material(r, j):
    """The material of a quad of the head: row r from the jaw, side j from the left (angle j x HEAD_STEP). The face
    on the 4 quads of the front of the rows 1 to 5 (the face parts: column 0 at the left of the picture: the right
    of the character), the hair at the back and on the top, the ears on the sides"""
    if r < 0:
        return 'skin'
    a = (j + 0.5) * HEAD_STEP
    front = 1 <= j <= FACE_COLUMNS
    if 1 <= r <= FACE_ROWS and front:
        return 'face%d%d' % (FACE_ROWS - r, FACE_COLUMNS - j)
    if r > FACE_ROWS:
        return 'hair'
    back = math.sin(a) < -0.3
    if back and r >= 2:
        return 'hair'
    if r == FACE_ROWS:
        return 'hair'
    return 'skin'


def build_mesh():
    m = Mesh()
    # Torso: jeans up to the belt, then the T-shirt (chest and shoulder blades shaped); closed at the bottom (the legs
    # under it) and at the top (the neck on it). The shoulders follow the arms
    chest = angle_bump(FRONT, 0.9, 0.06)
    blades = angle_bump(-FRONT, 0.8, 0.04)
    torso = [((0, 0.86, 0), (0.165, 0, 0), (0, 0, 0.11), w(('pelvis', 1))),
             ((0, 0.95, 0.005), (0.172, 0, 0), (0, 0, 0.115), w(('pelvis', 1)), angle_bump(-FRONT, 0.7, 0.08)),
             ((0, 1.02, 0.005), (0.16, 0, 0), (0, 0, 0.105), w(('pelvis', 0.5), ('spine', 0.5))),
             ((0, 1.10, 0.01), (0.152, 0, 0), (0, 0, 0.102), w(('spine', 1))),
             ((0, 1.19, 0.012), (0.158, 0, 0), (0, 0, 0.108), w(('spine', 0.5), ('chest', 0.5))),
             ((0, 1.28, 0.015), (0.17, 0, 0), (0, 0, 0.115), w(('chest', 1)), shapes(chest, blades)),
             ((0, 1.36, 0.01), (0.192, 0, 0), (0, 0, 0.11), shoulder_weights(0.3), shapes(chest, blades)),
             ((0, 1.425, 0.0), (0.205, 0, 0), (0, 0, 0.092), shoulder_weights(0.5)),
             ((0, 1.475, 0.0), (0.11, 0, 0), (0, 0, 0.075), w(('chest', 1)))]
    m.tube(torso, TORSO_SIDES, lambda r, j: 'denim' if r < 2 else 'cotton', caps=(True, True))
    # Neck (its ends against the torso and the head)
    m.tube([((0, 1.47, 0.005), (0.048, 0, 0), (0, 0, 0.05), w(('chest', 1))),
            ((0, 1.51, 0.008), (0.046, 0, 0), (0, 0, 0.048), w(('neck', 1))),
            ((0, 1.55, 0.01), (0.046, 0, 0), (0, 0, 0.047), w(('neck', 0.4), ('head', 0.6)))], LIMB_SIDES,
           lambda r, j: 'skin')
    # Head (12 sides, a point in the middle of the front): the jaw and the chin, the nose, the brow, the ears, the locks
    # of the hair on the top
    hw = w(('head', 1))
    ears = shapes(angle_bump(0, 0.25, 0.16), angle_bump(math.pi, 0.25, 0.16))
    locks = lambda t: 1 + 0.1 * (1 + math.sin(t * 5 + 1)) / 2  # noqa: E731
    head = [((0, 1.548, 0.016), (0.048, 0, 0), (0, 0, 0.055), hw),
            ((0, 1.568, 0.014), (0.064, 0, 0), (0, 0, 0.078), hw, angle_bump(FRONT, 0.35, 0.12)),
            ((0, 1.606, 0.009), (0.073, 0, 0), (0, 0, 0.088), hw, angle_bump(FRONT, 0.3, 0.05)),
            ((0, 1.648, 0.004), (0.078, 0, 0), (0, 0, 0.093), hw, shapes(angle_bump(FRONT, 0.22, 0.24), ears)),
            ((0, 1.695, 0.001), (0.081, 0, 0), (0, 0, 0.096), hw, shapes(angle_bump(FRONT, 0.2, 0.09), ears)),
            ((0, 1.740, 0.0), (0.083, 0, 0), (0, 0, 0.097), hw, angle_bump(FRONT, 0.6, 0.03)),
            ((0, 1.778, -0.003), (0.086, 0, 0), (0, 0, 0.1), hw, angle_bump(FRONT, 0.5, 0.06)),
            ((0, 1.812, -0.008), (0.078, 0, 0), (0, 0, 0.09), hw, locks),
            ((0, 1.838, -0.012), (0.056, 0, 0), (0, 0, 0.066), hw, locks),
            ((0, 1.852, -0.014), (0.026, 0, 0), (0, 0, 0.03), hw)]
    m.tube(head, HEAD_SIDES, head_material, caps=(True, True), angle0=0.0)
    for side, s in (('l', 1), ('r', -1)):
        sh, el, wr = 'shoulder_' + side, 'elbow_' + side, 'wrist_' + side
        x = 0.215 * s
        # Arm: the shoulder (a dome on the end of the shoulder of the torso: the arm attached), the sleeve of the
        # T-shirt, the biceps, the forearm, the hand (flat, its palm toward the body, the knuckles)
        arm = [((x, 1.465, 0), (0.026, 0, 0), (0, 0, 0.03), w((sh, 0.6), ('chest', 0.4))),
               ((x + 0.004 * s, 1.44, 0), (0.05, 0, 0), (0, 0, 0.054), w((sh, 0.7), ('chest', 0.3))),
               ((x + 0.008 * s, 1.37, 0), (0.054, 0, 0), (0, 0, 0.056), w((sh, 1))),
               ((x + 0.009 * s, 1.29, 0), (0.05, 0, 0), (0, 0, 0.052), w((sh, 1))),
               ((x + 0.009 * s, 1.235, 0), (0.048, 0, 0), (0, 0, 0.05), w((sh, 1))),
               ((x + 0.009 * s, 1.222, 0), (0.04, 0, 0), (0, 0, 0.044), w((sh, 1))),
               ((x + 0.01 * s, 1.17, 0.004), (0.039, 0, 0), (0, 0, 0.042), w((sh, 1))),
               ((x + 0.01 * s, 1.13, 0), (0.035, 0, 0), (0, 0, 0.037), w((sh, 0.5), (el, 0.5))),
               ((x + 0.011 * s, 1.07, 0.003), (0.037, 0, 0), (0, 0, 0.039), w((el, 1))),
               ((x + 0.013 * s, 0.98, 0.006), (0.031, 0, 0), (0, 0, 0.033), w((el, 1))),
               ((x + 0.015 * s, 0.9, 0.01), (0.024, 0, 0), (0, 0, 0.027), w((el, 0.6), (wr, 0.4))),
               ((x + 0.015 * s, 0.865, 0.012), (0.018, 0, 0), (0, 0, 0.044), w((wr, 1))),
               ((x + 0.015 * s, 0.82, 0.014), (0.02, 0, 0), (0, 0, 0.047), w((wr, 1))),
               ((x + 0.013 * s, 0.775, 0.016), (0.017, 0, 0), (0, 0, 0.04), w((wr, 1))),
               ((x + 0.011 * s, 0.75, 0.017), (0.01, 0, 0), (0, 0, 0.024), w((wr, 1)))]
        # (from the bottom to the top: the hand, then the shoulder; the sleeve from the row of its hem)
        sleeve = len(arm) - 6
        m.tube(list(reversed(arm)), LIMB_SIDES, lambda r, j: 'skin' if r < sleeve else 'cotton', caps=(True, True))
        hp, kn, an = 'hip_' + side, 'knee_' + side, 'ankle_' + side
        x = 0.09 * s
        # Leg: the jeans from the ankle to the hip (the calf, the knee, the thigh)
        calf = angle_bump(-FRONT, 0.8, 0.12)
        leg = [((x, 0.10, 0.0), (0.052, 0, 0), (0, 0, 0.054), w((an, 0.3), (kn, 0.7))),
               ((x, 0.18, 0.0), (0.048, 0, 0), (0, 0, 0.05), w((kn, 1))),
               ((x, 0.30, -0.004), (0.052, 0, 0), (0, 0, 0.056), w((kn, 1)), calf),
               ((x, 0.40, 0.0), (0.054, 0, 0), (0, 0, 0.056), w((kn, 1))),
               ((x, 0.50, 0.01), (0.056, 0, 0), (0, 0, 0.06), w((kn, 0.5), (hp, 0.5))),
               ((x, 0.58, 0.01), (0.06, 0, 0), (0, 0, 0.064), w((hp, 0.85), (kn, 0.15))),
               ((x, 0.72, 0.01), (0.068, 0, 0), (0, 0, 0.072), w((hp, 1))),
               ((x * 1.02, 0.865, 0.005), (0.072, 0, 0), (0, 0, 0.076), w((hp, 0.5), ('pelvis', 0.5)))]
        m.tube(leg, LIMB_SIDES, lambda r, j: 'denim')
        # Shoe: a tube from the heel to the toe (flat under: the sole), closed at both ends
        fw = w((an, 1))
        shoe = [((x, 0.045, -0.065), (0.04, 0, 0), (0, 0.045, 0), fw),
                ((x, 0.055, -0.03), (0.046, 0, 0), (0, 0.055, 0), fw),
                ((x, 0.05, 0.04), (0.049, 0, 0), (0, 0.05, 0), fw),
                ((x, 0.038, 0.12), (0.048, 0, 0), (0, 0.038, 0), fw),
                ((x, 0.03, 0.17), (0.04, 0, 0), (0, 0.03, 0), fw),
                ((x, 0.024, 0.195), (0.026, 0, 0), (0, 0.022, 0), fw)]
        m.tube(shoe, LIMB_SIDES, lambda r, j: 'leather', axis_up=(0, 0, 1), caps=(True, True))
    return m


def vertex_colors(mesh):
    """The light baked in the vertex colors (see GOURAUD_NEUTRAL): the normal of a point is the mean of the normals of
    its polygons in the rest pose"""
    normals = np.zeros((len(mesh.points), 3))
    for idx, mat, uv in mesh.faces:
        p = [mesh.points[i] for i in idx]
        n = np.cross(p[2] - p[0], p[3] - p[1])
        for i in idx:
            normals[i] += n
    normals /= np.linalg.norm(normals, axis=1, keepdims=True) + 1e-12
    light = LIGHT / np.linalg.norm(LIGHT)
    k = AMBIENT + DIFFUSE * np.clip(normals @ light, 0, 1)
    return [(v * GOURAUD_NEUTRAL,) * 3 for v in k]


# ---------------------------------------------------------------- walk

def rot_x(deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])


def rot_y(deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


def rot_z(deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


def bump(phase, center, width):
    d = (phase - center + math.pi) % (2 * math.pi) - math.pi
    return math.exp(-(d / width) ** 2)


# The walk: angles in degrees, phase 0 to 2 pi (pi / 2: the heel of the left foot touches the ground)
HIP_FLEX = 6                    # Hips: forward on average (the thigh a little ahead), swing around it
HIP_SWING = 23
KNEE_STRAIGHT = 4               # Knees: almost straight, bent in the swing and when the foot lands (it absorbs the weight)
KNEE_SWING = 60
KNEE_SWING_PHASE = -0.6
KNEE_SWING_WIDTH = 0.8
KNEE_LOADING = 16
KNEE_LOADING_DELAY = 0.5
KNEE_LOADING_WIDTH = 0.45
HEEL_STRIKE_TOES_UP = 14        # Feet: the toes up when the heel lands, the push of the toes, up again in the swing
TOE_OFF = 28
TOE_OFF_PHASE = 3 * math.pi / 2 + 0.3
SWING_TOES_UP = 7
FOOT_WIDTH = 0.35               # (width of these bumps, radians)
PELVIS_YAW = 6                  # Pelvis: turns with the leg forward, drops on the side of the swing, shifts over the foot
PELVIS_ROLL = 4
PELVIS_SWAY = 0.022             # (m)
LEAN = 4                        # Forward lean of the body
CHEST_COUNTER = 1.6             # Chest: turns against the pelvis (x its yaw)
HEAD_STEADY = 0.8               # Head: keeps looking ahead (x the yaw of the chest against the pelvis)
ARM_SWING = 22                  # Arms: swing against the legs, a little late, the elbow bends more forward
ARM_LAG = 0.35
ARM_OUT = 7
ELBOW_BENT = 16
ELBOW_SWING = 20
ELBOW_LAG = 0.6
WRIST_BENT = 8


def leg_angles(phase):
    """Hip forward, knee flexion, ankle (toes up) of a leg"""
    hip = HIP_FLEX + HIP_SWING * math.sin(phase)
    knee = KNEE_STRAIGHT + KNEE_SWING * bump(phase, KNEE_SWING_PHASE, KNEE_SWING_WIDTH) + \
        KNEE_LOADING * bump(phase, math.pi / 2 + KNEE_LOADING_DELAY, KNEE_LOADING_WIDTH)
    toes_up = HEEL_STRIKE_TOES_UP * bump(phase, math.pi / 2, FOOT_WIDTH) - TOE_OFF * bump(phase, TOE_OFF_PHASE, FOOT_WIDTH) \
        + SWING_TOES_UP * bump(phase, 0.0, 2 * FOOT_WIDTH)
    # (the foot flat on the ground when toes_up is 0: the ankle undoes the hip and the knee)
    return hip, knee, knee - hip + toes_up


def pose(phase):
    """Local rotations of the bones and the offset of the pelvis (before the contact with the ground)"""
    rot = {b[0]: np.eye(3) for b in BONES}
    for side, ph in (('l', phase), ('r', phase + math.pi)):
        h, k, a = leg_angles(ph)
        rot['hip_' + side] = rot_x(-h)
        rot['knee_' + side] = rot_x(k)
        rot['ankle_' + side] = rot_x(-a)
    yaw = PELVIS_YAW * math.sin(phase)
    rot['pelvis'] = rot_y(-yaw) @ rot_z(PELVIS_ROLL * math.sin(phase)) @ rot_x(-LEAN)
    rot['spine'] = rot_y(yaw * CHEST_COUNTER / 2)
    rot['chest'] = rot_y(yaw * CHEST_COUNTER / 2)
    rot['neck'] = rot_y(-yaw * (CHEST_COUNTER - 1) * HEAD_STEADY) @ rot_x(LEAN / 2)
    rot['head'] = rot_x(LEAN / 2)
    for side, s, ph in (('l', 1, phase), ('r', -1, phase + math.pi)):
        late = ph - ARM_LAG
        rot['shoulder_' + side] = rot_x(ARM_SWING * math.sin(late)) @ rot_z(ARM_OUT * s)
        rot['elbow_' + side] = rot_x(-(ELBOW_BENT + ELBOW_SWING * (1 - math.sin(late + ELBOW_LAG)) / 2))
        rot['wrist_' + side] = rot_x(-WRIST_BENT)
    sway = np.array([-PELVIS_SWAY * math.cos(phase), 0.0, 0.0])
    return rot, sway


def skin(mesh, rot, offset):
    world_r = {}
    world_p = {}
    for name, parent, joint in BONES:
        j = np.array(joint)
        if parent is None:
            world_r[name] = rot[name]
            world_p[name] = j + offset
        else:
            pj = np.array(BONES[BONE_INDEX[parent]][2])
            world_r[name] = world_r[parent] @ rot[name]
            world_p[name] = world_p[parent] + world_r[parent] @ (j - pj)
    out = []
    for p, ws in zip(mesh.points, mesh.weights):
        v = np.zeros(3)
        for b, x in ws:
            name, _, joint = BONES[b]
            v += x * (world_p[name] + world_r[name] @ (p - np.array(joint)))
        out.append(v)
    return np.array(out)


def foot_points(mesh):
    """The points of each shoe (bone ankle_l, ankle_r)"""
    return [[i for i, ws in enumerate(mesh.weights) if ws == [(BONE_INDEX['ankle_' + side], 1)]] for side in 'lr']


def walk(mesh):
    """The points of each frame of the cycle, and the distance walked in a cycle (meters)"""
    feet = foot_points(mesh)
    steps = []
    for k in range(FINE_STEPS):
        phase = 2 * math.pi * k / FINE_STEPS
        rot, sway = pose(phase)
        pts = skin(mesh, rot, sway)
        lows = [pts[f][:, 1].min() for f in feet]
        stance = int(np.argmin(lows))
        # (the height: the lowest foot on the ground)
        pts[:, 1] -= min(lows)
        f = feet[stance]
        contact = f[int(np.argmin(pts[f][:, 1]))]
        steps.append((pts, contact))
    # Forward motion: the point of the foot on the ground stays still (it moves back in the model: the model moves
    # forward; a foot that rolls on its heel or its toes touches the ground with another point at the next step)
    forward = [0.0]
    for k in range(1, FINE_STEPS + 1):
        a = steps[k - 1]
        b = steps[k % FINE_STEPS]
        forward.append(forward[-1] + a[0][a[1], 2] - b[0][a[1], 2])
    distance = forward[-1]
    # In place: the forward motion minus the motion at constant speed (the program moves the model)
    offsets = [forward[k] - distance * k / FINE_STEPS for k in range(FINE_STEPS)]
    mean = sum(offsets) / FINE_STEPS
    frames = []
    for i in range(FRAMES):
        k = i * FINE_STEPS // FRAMES
        pts = steps[k][0].copy()
        pts[:, 2] += offsets[k] - mean
        frames.append(pts)
    return frames, distance


# ---------------------------------------------------------------- files

def write_mtl(path, materials, extra='', flags=None):
    """flags: the options of some materials (see jo_model.py), extra: the options of all of them"""
    with open(path, 'w', newline='\n') as f:
        for name, texture in materials:
            f.write('newmtl %s\nKd 1 1 1\nmap_Kd %s\n%s\n' % (name, texture, (flags or {}).get(name, extra)))


def write_obj(path, points, faces, mtl, colors=None):
    with open(path, 'w', newline='\n') as f:
        f.write('# Made by make_walker.py\nmtllib %s\n' % mtl)
        for k, p in enumerate(points):
            if colors:
                f.write('v %.5f %.5f %.5f %.4f %.4f %.4f\n' % (tuple(p) + tuple(colors[k])))
            else:
                f.write('v %.5f %.5f %.5f\n' % tuple(p))
        n = 0
        for idx, mat, uv in faces:
            for u, v in uv:
                f.write('vt %g %g\n' % (u, v))
        current = None
        for idx, mat, uv in faces:
            if mat != current:
                f.write('usemtl %s\n' % mat)
                current = mat
            f.write('f %s\n' % ' '.join('%d/%d' % (i + 1, n + k + 1) for k, i in enumerate(idx)))
            n += 4


def ground():
    tex = tex_cobbles()
    pts = []
    faces = []
    half = GROUND_TILES * GROUND_TILE / 2
    for j in range(GROUND_TILES + 1):
        for i in range(GROUND_TILES + 1):
            pts.append((i * GROUND_TILE - half, 0.0, j * GROUND_TILE - half))
    row = GROUND_TILES + 1
    for j in range(GROUND_TILES):
        for i in range(GROUND_TILES):
            a = j * row + i
            # (counterclockwise seen from above: the front up; the top left of the texture on the far left corner)
            faces.append(([a + row, a + row + 1, a + 1, a], 'cobbles', [(0, 1), (1, 1), (1, 0), (0, 0)]))
    write_mtl(os.path.join(HERE, 'ground.mtl'), [('cobbles', tex)], 'jo_sort max\njo_subdivision 2')
    write_obj(os.path.join(HERE, 'ground.obj'), pts, faces, 'ground.mtl')


def check_frames(frames):
    """The views of the character around it (jo_model.sort_views()) every SORT_CHECK_STEP frame, with the sorts the
    converter chose for the model: a polygon drawn over a nearer one or a hole stops here"""
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        jom = os.path.join(tmp, 'WALKER.JOM')
        jo_model.convert(os.path.join(HERE, 'walker.obj'), jom, scale=SCALE, quiet=True, texture_dir=tmp,
                         morph=os.path.join(HERE, 'walk.json'))
        for k in range(0, len(frames), SORT_CHECK_STEP):
            views = jo_model.sort_views(jom, SORT_CHECK_ELEVATIONS, SORT_CHECK_AZIMUTHS,
                                        points=jo_model.morph_points(jom, 0, k))
            bad = [v for v in views if v[2] and v[3] + v[7] > v[2] * SORT_CHECK_MAX_PART]
            worst = max(views, key=lambda v: (v[3] + v[7]) / max(1, v[2]))
            print('frame %2d: %d views of %d fail, the worst %.2f %% drawn wrong, %.2f %% holes (elevation %d, azimuth '
                  '%.0f)' % (k, len(bad), len(views), worst[3] * 100.0 / max(1, worst[2]),
                             worst[7] * 100.0 / max(1, worst[2]), worst[0], worst[1]))
            failed += len(bad)
    return failed


def main():
    textures = {'denim': tex_denim(), 'cotton': tex_cotton(), 'skin': tex_skin(), 'hair': tex_hair(), 'leather': tex_shoe()}
    for name, png in tex_face():
        textures[name] = png
    mesh = build_mesh()
    used = sorted({f[1] for f in mesh.faces})
    write_mtl(os.path.join(HERE, 'walker.mtl'), [(m, textures[m]) for m in used], flags=MATERIAL_FLAGS)
    colors = vertex_colors(mesh)
    frames, distance = walk(mesh)
    # The model: the first frame (standing on the ground)
    write_obj(os.path.join(HERE, 'walker.obj'), frames[0], mesh.faces, 'walker.mtl', colors)
    os.makedirs(os.path.join(HERE, 'frames'), exist_ok=True)
    for k, pts in enumerate(frames):
        write_obj(os.path.join(HERE, 'frames', 'walk_%02d.obj' % k), pts, mesh.faces, '../walker.mtl', colors)
    json.dump({'animations': [{'name': 'walk', 'frames': 'frames/walk_%02d.obj', 'first': 0, 'last': FRAMES - 1,
                               'frame_time': FRAME_TICKS / TICKS_PER_SECOND, 'loop': True}]},
              open(os.path.join(HERE, 'walk.json'), 'w'), indent=1)
    ground()
    with open(os.path.join(SAMPLE, 'walker.h'), 'w', newline='\n') as f:
        f.write('/* Made by models/make_walker.py: the walk of cd/WALKER.JOM */\n\n#ifndef __WALKER_H__\n# define __WALKER_H__\n\n')
        f.write('/* Distance walked in a cycle of the animation "walk" (units of the model, 16.16 fixed): the feet on the '
                'ground stay still\n   when the model moves at this speed */\n')
        f.write('# define WALKER_CYCLE_DISTANCE      (%d)\n\n' % round(distance * SCALE * 65536))
        f.write('/* Size of a tile of cd/GROUND.JOM (units, 16.16 fixed): the ground repeats itself every tile */\n')
        f.write('# define WALKER_GROUND_TILE         (%d)\n\n#endif /* !__WALKER_H__ */\n' % round(GROUND_TILE * SCALE * 65536))
    print('%d points, %d polygons, %d frames, %.2f m per cycle' % (len(mesh.points), len(mesh.faces), FRAMES, distance))
    if check_frames(frames):
        sys.exit('the painter\'s sort draws polygons over nearer ones (see above)')


if __name__ == '__main__':
    main()
