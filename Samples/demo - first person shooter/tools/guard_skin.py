#!/usr/bin/env python3
"""First person shooter demo: the look of the guards (used by tools/make_guard.py).

The colors of a guard are functions of the 3D point of his body (the design units of make_guard.py: Y up, the model
looks toward -Z, +X is his left), so the textures of the polygons that touch each other join: each polygon of the head
and of the front of the jacket gets its own texture, baked from these functions (bake_tile()), several samples per
texel. The faces: FACES, a set of features per guard (skin, hair, eyes, nose, mouth, stubble, moustache)."""

import math

import numpy as np

# ---------------------------------------------------------------------------
# Noise: 3D value noise (smooth), the same everywhere on the body (no seam between 2 textures)
# ---------------------------------------------------------------------------

_LATTICE = np.random.default_rng(1234).random(64 ** 3).astype(np.float32)


def noise3(p, scale, seed=0):
    """Smooth noise about 0..1 at the points p (..., 3), its features scale units wide"""
    q = p / scale + seed * 17.31
    i = np.floor(q).astype(np.int64)
    f = q - i
    f = f * f * (3 - 2 * f)
    out = 0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                idx = (((i[..., 0] + dx) & 63) * 4096 + ((i[..., 1] + dy) & 63) * 64 + ((i[..., 2] + dz) & 63))
                w = (f[..., 0] if dx else 1 - f[..., 0]) * (f[..., 1] if dy else 1 - f[..., 1]) * (f[..., 2] if dz else 1 - f[..., 2])
                out = out + _LATTICE[idx] * w
    return out


def fbm(p, scale, octaves=3, seed=0):
    total = 0
    amp = 1.0
    norm = 0
    for o in range(octaves):
        total = total + noise3(p, scale / (2 ** o), seed + o) * amp
        norm += amp
        amp *= 0.5
    return total / norm


def gauss(x, y, cx, cy, sx, sy):
    return np.exp(-(((x - cx) / sx) ** 2 + ((y - cy) / sy) ** 2))


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def mix(a, b, t):
    t = np.asarray(t)[..., None]
    return a * (1 - t) + b * t


# ---------------------------------------------------------------------------
# The faces of the guards (the design units: the eyes at Y 81.4, the chin at 75.5, the top of the head at 87.3)
# ---------------------------------------------------------------------------

FACES = [
    # A dark haired young guard, clean shaven, brown eyes
    dict(skin=(196, 150, 122), hair=(48, 34, 24), iris=(70, 48, 30), stubble=0.25, moustache=False, brow=0.30,
         hairline=85.0, lips=(150, 92, 82), nose=0.42, eyes=1.52, cheeks=0.20, age=0.1, seed=1),
    # An older guard, a moustache, grey at the temples, blue eyes
    dict(skin=(205, 160, 135), hair=(80, 70, 62), iris=(70, 95, 120), stubble=0.15, moustache=True, brow=0.36,
         hairline=85.4, lips=(155, 98, 90), nose=0.48, eyes=1.58, cheeks=0.35, age=0.6, seed=2),
    # A tanned guard, black hair, a dark stubble
    dict(skin=(170, 120, 90), hair=(25, 20, 18), iris=(50, 34, 22), stubble=0.65, moustache=False, brow=0.34,
         hairline=84.8, lips=(135, 80, 70), nose=0.46, eyes=1.50, cheeks=0.10, age=0.25, seed=3),
    # A fair haired guard, pale skin, a light stubble, grey eyes
    dict(skin=(214, 172, 148), hair=(125, 96, 62), iris=(95, 105, 110), stubble=0.3, moustache=False, brow=0.26,
         hairline=85.2, lips=(165, 100, 95), nose=0.40, eyes=1.55, cheeks=0.40, age=0.15, seed=4),
]

HEAD_EYE_Y = 81.4
HEAD_MOUTH_Y = 77.5


def head_color(p, face):
    """The colors (RGB 0..255) of the head of a guard at the points p (..., 3) of its surface"""
    x = p[..., 0]
    y = p[..., 1]
    z = p[..., 2]
    ang = np.degrees(np.arctan2(x, -z))
    a = np.abs(ang)
    s = np.abs(x)
    front = -z
    skin = np.array(face['skin'], dtype=float)
    hair = np.array(face['hair'], dtype=float)
    # The skin: its pores, red on the cheeks, the nose and the ears, a little yellow on the forehead
    pores = fbm(p, 0.35, 2, face['seed']) - 0.5
    blotch = fbm(p, 1.6, 2, face['seed'] + 5) - 0.5
    col = np.zeros(p.shape[:-1] + (3,)) + skin
    col *= (1 + pores[..., None] * 0.10 + blotch[..., None] * 0.08)
    red = np.array((1.06, 0.93, 0.90))
    cheek = gauss(s, y, 2.2, 79.6, 0.9, 1.0) * (front > 0) * face['cheeks']
    col = mix(col, col * red, cheek + gauss(x, y, 0, 79.3, 0.45, 0.5) * (front > 2) * 0.6)
    col *= 1 + 0.04 * smoothstep(83, 85, y)[..., None]
    # The shapes of the face, drawn by light and shadow (the lights of the scene: the gouraud shading)
    shade = np.ones_like(y)
    frontal = front > 1.0
    # The eye sockets, the brow over them
    ex = face['eyes']
    shade -= 0.30 * gauss(s, y, ex - 0.1, 81.45, 0.95, 0.62) * frontal
    shade -= 0.18 * gauss(s, y, 0.55, 81.6, 0.3, 0.6) * frontal
    shade += 0.10 * gauss(s, y, ex, 82.85, 1.1, 0.4) * frontal
    shade += 0.10 * gauss(s, y, 2.5, 80.3, 0.8, 0.6) * frontal
    shade -= 0.14 * gauss(s, y, 2.9, 78.6, 0.6, 1.1) * frontal
    shade += 0.06 * gauss(x, y, 0, 84.2, 2.2, 1.0) * frontal
    # The nose: its bridge in the light, its sides in the shadow, the tip, the shadow under it
    w = face['nose']
    bridge = (y > 78.9) & (y < 82.2) & frontal
    shade += 0.14 * np.exp(-(x / (w * 0.55)) ** 2) * bridge
    shade -= 0.26 * gauss(s, y, w + 0.3, 79.9, 0.32, 1.2) * frontal
    shade += 0.12 * gauss(x, y, 0, 79.15, w * 0.75, 0.32) * frontal
    shade -= 0.12 * gauss(s, y, w + 0.2, 78.9, 0.3, 0.3) * frontal
    shade -= 0.34 * gauss(x, y, 0, 78.5, w * 1.4, 0.2) * frontal
    shade -= 0.12 * gauss(x, y, 0, 78.0, 0.28, 0.35) * frontal
    # The nostrils
    shade -= 0.45 * gauss(s, y, w * 0.75, 78.75, 0.16, 0.10) * frontal
    # The fold from the nose to the mouth, the age lines
    fold = gauss(s - (w + 0.45 + (78.6 - y) * 0.35), y, 0, 77.9, 0.12, 0.8) * frontal
    shade -= (0.08 + 0.10 * face['age']) * fold
    shade -= 0.06 * face['age'] * gauss(s, y, 2.1, 81.0, 0.5, 0.12) * frontal
    # The chin, the hollow under the lower lip, the jaw in the shadow
    shade += 0.06 * gauss(x, y, 0, 75.9, 0.9, 0.45) * frontal
    shade -= 0.10 * gauss(x, y, 0, 76.6, 0.7, 0.18) * frontal
    shade -= 0.10 * smoothstep(77, 75.3, y) * smoothstep(40, 80, a)
    shade -= 0.06 * smoothstep(2.6, 3.6, s) * (front > 0)
    col *= shade[..., None]
    # The stubble: the jaw, the chin, over the lip (bluish, small dots)
    beard_top = 76.9 + 2.4 * smoothstep(1.2, 3.4, s) + 1.6 * smoothstep(50, 80, a)
    beard_zone = smoothstep(beard_top + 0.3, beard_top - 0.5, y) * smoothstep(100, 82, a)
    beard_zone *= 1 - np.clip(1.4 * gauss(x, y, 0, HEAD_MOUTH_Y - 0.1, 1.5, 0.5), 0, 1)
    beard_zone += gauss(x, y, 0, 78.05, 1.1, 0.22) * frontal * 0.7
    dots = noise3(p, 0.09, face['seed'] + 9)
    stubble = np.clip(beard_zone, 0, 1) * face['stubble'] * (0.55 + 0.45 * dots)
    col = mix(col, np.array((70, 62, 64.0)) * 0.9, np.clip(stubble, 0, 0.8))
    # The mouth: the lips, the line between them, a light on the lower lip, the corners
    lips = np.array(face['lips'], dtype=float)
    mw = 1.3
    upper = gauss(x, y, 0, HEAD_MOUTH_Y + 0.22, mw, 0.16) * (s < mw * 1.15) * frontal
    lower = gauss(x, y, 0, HEAD_MOUTH_Y - 0.28, mw * 0.9, 0.22) * (s < mw * 1.1) * frontal
    col = mix(col, lips * 0.82, np.clip(upper * 1.3, 0, 0.85))
    col = mix(col, lips * 1.02, np.clip(lower * 1.2, 0, 0.8))
    line = np.exp(-((y - HEAD_MOUTH_Y - 0.03 * (s / mw) ** 2) / 0.07) ** 2) * (s < mw) * frontal
    col = mix(col, np.array((60, 30, 28.0)), np.clip(line, 0, 0.9))
    col = mix(col, col * 1.2, gauss(x, y, 0.15, HEAD_MOUTH_Y - 0.32, 0.4, 0.07) * frontal * 0.6)
    col *= (1 - 0.25 * gauss(s, y, mw, HEAD_MOUTH_Y, 0.15, 0.15) * frontal)[..., None]
    # The moustache
    if face['moustache']:
        stache = gauss(x, y, 0, 77.85, 1.15, 0.28) * frontal * (0.6 + 0.4 * noise3(p * np.array((3, 1, 3)), 0.12, 31))
        col = mix(col, hair * 0.85, np.clip(stache * 1.8, 0, 0.95))
    # The eyes: almond shaped, the white, the iris, the pupil, a highlight, the upper lid and its lashes, the lower lid
    ex = face['eyes']
    ey = HEAD_EYE_Y
    dx = (s - ex) / 0.78
    dy = (y - ey) / 0.26
    almond = (dx ** 2 + (dy * (1 + 0.25 * dx)) ** 2) < 1
    eye = almond & frontal
    white = np.array((205, 195, 185.0))
    col = mix(col, white * (0.85 - 0.25 * np.abs(dx))[..., None] / 0.85, eye * 0.95)
    r = np.sqrt(((s - ex + 0.05) / 0.25) ** 2 + ((y - ey + 0.02) / 0.25) ** 2)
    iris = np.array(face['iris'], dtype=float)
    col = mix(col, iris * (0.75 + 0.35 * r[..., None] * 0) , eye * (r < 1) * 0.95)
    col = mix(col, np.array((14, 10, 10.0)), eye * (r < 0.42))
    col = mix(col, np.array((255, 255, 250.0)), eye * (np.sqrt(((s - ex + 0.08) / 0.07) ** 2 + ((y - ey - 0.08) / 0.07) ** 2) < 1) * 0.8)
    lid = np.exp(-((y - ey - 0.27 + 0.18 * dx ** 2) / 0.07) ** 2) * (np.abs(dx) < 1.15) * frontal
    col = mix(col, np.array((40, 28, 24.0)), np.clip(lid * 1.2, 0, 0.9))
    col = mix(col, col * 0.85, np.exp(-((y - ey + 0.3 - 0.1 * dx ** 2) / 0.08) ** 2) * (np.abs(dx) < 1) * frontal)
    col = mix(col, col * 0.88, gauss(s, y, ex, ey + 0.55, 0.65, 0.18) * frontal)
    # The eyebrows: hair, thicker inside, the strands
    bx = (s - ex + 0.05) / 1.0
    brow_y = 82.45 + 0.15 * (1 - bx ** 2) - 0.08 * bx
    strands = noise3(p * np.array((1, 4, 1)), 0.08, face['seed'] + 3)
    brow = np.exp(-((y - brow_y) / (face['brow'] * (1.15 - 0.35 * bx) * 0.5)) ** 2) * (np.abs(bx) < 1.05) * frontal
    col = mix(col, hair * (0.8 + 0.3 * strands[..., None]), np.clip(brow * (0.6 + 0.6 * strands), 0, 0.95))
    # The ears (on the sides): the rim, the hollow
    ear_dy = (y - 80.9) / 1.35
    ear_da = (a - 92) / 13
    ear = (ear_da ** 2 + ear_dy ** 2) < 1
    col = mix(col, col * np.array((1.03, 0.92, 0.88)), ear * 0.8)
    rim = ear & ((ear_da ** 2 + ear_dy ** 2) > 0.55)
    col *= (1 + 0.08 * rim)[..., None]
    col *= (1 - 0.35 * gauss(a, y, 94, 80.6, 5, 0.55))[..., None]
    col *= (1 - 0.25 * np.exp(-(((a - 79) / 2.5) ** 2)) * (np.abs(ear_dy) < 1))[..., None]
    # The hair: short, its hairline (higher at the temples), the sideburns, the nape; darker at the roots
    temple = smoothstep(0.3, 1.8, s) * (front > 0)
    line_y = face['hairline'] + 0.35 * temple - 0.25 * np.cos(np.radians(np.clip(a, 0, 180))) + 0.0 * x
    side = smoothstep(55, 70, a)
    line_y = line_y * (1 - side) + 83.6 * side
    burn = (a > 70) & (a < 84) & (y > 80.6)
    back = smoothstep(105, 130, a)
    line_y = line_y * (1 - back) + 77.2 * back
    hairy = (y > line_y + 0.15 * (noise3(p, 0.2, 7) - 0.5)) | burn
    hairy &= ~(ear & (a < 105))
    edge = smoothstep(0, 0.6, y - line_y)
    strand = noise3(p * np.array((4, 1, 4)), 0.10, face['seed'] + 11)
    strand2 = fbm(p, 0.5, 2, face['seed'] + 13)
    hair_col = hair[None] * (0.75 + 0.35 * strand[..., None] + 0.2 * (strand2[..., None] - 0.5))
    if face['age'] > 0.4:
        grey = smoothstep(60, 95, a) * 0.5
        hair_col = mix(hair_col, np.array((150, 145, 140.0)), grey * (0.5 + 0.5 * strand))
    # (the fade of a short cut: the skin seen through the hair near the line)
    fade = np.clip(0.55 + 0.45 * edge, 0, 1) * hairy
    col = mix(col, hair_col, fade)
    return np.clip(col, 0, 255)


# ---------------------------------------------------------------------------
# The uniform (shared by the guards): the jacket, its collar with the red tabs, the pockets, the buttons, the folds
# ---------------------------------------------------------------------------

UNIFORM = (106, 128, 72)          # The light olive green of the uniform of the guards
UNIFORM_DARK = (92, 110, 62)      # The trousers
RED_TABS = (165, 32, 26)
BRASS = (150, 128, 70)


def cloth(p, base, seed):
    """Cloth: the weave (fine lines), the folds (large soft shadows)"""
    base = np.array(base, dtype=float)
    weave = noise3(p * np.array((1, 6, 1)), 0.25, seed) * 0.5 + noise3(p * np.array((6, 1, 6)), 0.25, seed + 1) * 0.5
    folds = fbm(p * np.array((1.6, 0.6, 1.6)), 3.0, 3, seed + 2)
    col = np.zeros(p.shape[:-1] + (3,)) + base
    col *= (0.94 + 0.10 * weave[..., None]) * (0.86 + 0.26 * folds[..., None])
    return col


def jacket_color(p):
    """The jacket of the guards at the points p (..., 3)"""
    x = p[..., 0]
    y = p[..., 1]
    z = p[..., 2]
    s = np.abs(x)
    ang = np.degrees(np.arctan2(x, -z))
    a = np.abs(ang)
    front = a < 70
    col = cloth(p, UNIFORM, 41)
    # The folds from the belt up, under the chest
    col *= (1 - 0.10 * smoothstep(52, 49, y))[..., None]
    col *= (1 - 0.08 * gauss(s, y, 3.5, 58.5, 3.0, 1.0) * front)[..., None]
    # The front: the placket (lighter edge, a seam), the buttons
    placket = (s < 0.55) & front
    col *= (1 + 0.05 * placket)[..., None]
    col *= (1 - 0.30 * np.exp(-((x - 0.55) / 0.08) ** 2) * front)[..., None]
    for by in (52.0, 56.0, 60.0, 64.0, 68.0):
        d = np.sqrt((x / 0.42) ** 2 + ((y - by) / 0.42) ** 2)
        col = mix(col, np.array(BRASS) * (1.15 - 0.4 * d[..., None]), (d < 1) & front)
        col *= (1 - 0.3 * ((d > 1) & (d < 1.4) & front & (y < by)))[..., None]
    # The chest pockets with their flaps (and a button)
    for sx in (-1, 1):
        px = sx * 4.4
        inside = (np.abs(x - px) < 2.1) & (y > 59.5) & (y < 65.5) & front
        flap = inside & (y > 64.0)
        col *= (1 - 0.06 * inside)[..., None]
        col *= (1 + 0.08 * flap)[..., None]
        border = inside & ((np.abs(np.abs(x - px) - 2.1) < 0.18) | (np.abs(y - 59.6) < 0.18))
        col *= (1 - 0.30 * border)[..., None]
        col *= (1 - 0.40 * (inside & (np.abs(y - 64.0) < 0.15)))[..., None]
        d = np.sqrt(((x - px) / 0.32) ** 2 + ((y - 64.5) / 0.32) ** 2)
        col = mix(col, np.array(BRASS), (d < 1) & front)
    # The back: the seam down the middle, the shoulder blades
    col *= (1 - 0.25 * np.exp(-((x) / 0.12) ** 2) * (a > 150))[..., None]
    # The collar: standing, the red tabs on its front
    collar = y > 70.9
    col *= (1 - 0.18 * np.exp(-((y - 70.9) / 0.25) ** 2))[..., None]
    tab = collar & (s > 0.6) & (s < 2.7) & (a < 75)
    col = mix(col, np.array(RED_TABS) * (0.9 + 0.2 * noise3(p, 0.3, 5)[..., None]), tab)
    # The shadow under the arms, at the sides
    col *= (1 - 0.15 * smoothstep(60, 90, a) * smoothstep(64, 69, y))[..., None]
    return np.clip(col, 0, 255)


def belt_color(p):
    """The belt: brown leather, its brass buckle at the front"""
    x = p[..., 0]
    y = p[..., 1]
    z = p[..., 2]
    a = np.abs(np.degrees(np.arctan2(x, -z)))
    leather = np.array((92, 60, 36.0))
    col = np.zeros(p.shape[:-1] + (3,)) + leather
    col *= (0.88 + 0.2 * fbm(p, 0.8, 2, 61)[..., None])
    edge = np.minimum(np.abs(y - 47.0), np.abs(y - 50.5))
    col *= (1 - 0.35 * np.exp(-(edge / 0.25) ** 2))[..., None]
    col *= (1 + 0.1 * np.exp(-((y - 49.8) / 0.25) ** 2))[..., None]
    buckle = (np.abs(x) < 1.6) & (y > 47.4) & (y < 50.1) & (a < 40)
    inner = (np.abs(x) < 1.0) & (y > 47.9) & (y < 49.6)
    col = mix(col, np.array(BRASS) * (0.95 + 0.25 * np.exp(-((y - 49.5) / 0.4) ** 2))[..., None], buckle & ~inner)
    return np.clip(col, 0, 255)


def pants_color(p):
    """The trousers (breeches): the cloth, the folds"""
    col = cloth(p, UNIFORM_DARK, 71)
    y = p[..., 1]
    col *= (1 - 0.1 * gauss(np.abs(p[..., 0]) * 0, y, 0, 44, 1, 2))[..., None]
    return np.clip(col, 0, 255)


def boots_color(p):
    """The boots: dark red brown leather, a shine down the front, the creases at the ankle, the top edge, the sole"""
    x = p[..., 0]
    y = p[..., 1]
    z = p[..., 2]
    leather = np.array((96, 48, 34.0))
    col = np.zeros(p.shape[:-1] + (3,)) + leather
    col *= (0.85 + 0.25 * fbm(p, 1.2, 2, 81)[..., None])
    creases = noise3(p * np.array((3, 0.5, 3)), 0.5, 83)
    col *= (1 - 0.2 * smoothstep(9, 6, y) * smoothstep(3, 5, y) * creases)[..., None]
    col *= (1 - 0.3 * np.exp(-((y - 22.6) / 0.4) ** 2))[..., None]
    col *= (1 - 0.55 * smoothstep(1.4, 0.6, y))[..., None]
    return np.clip(col, 0, 255)


def skin_color(p, face):
    """The hands"""
    skin = np.array(face['skin'], dtype=float)
    col = np.zeros(p.shape[:-1] + (3,)) + skin
    col *= (0.9 + 0.15 * fbm(p, 0.5, 2, 91)[..., None])
    return np.clip(col, 0, 255)


# The scientists: a white lab coat over grey trousers, black shoes
COAT = (226, 226, 222)
TROUSERS = (92, 94, 100)


def coat_color(p):
    """The lab coat of the scientists at the points p (..., 3): white cloth, the placket and its buttons, a breast pocket
    with a pen, the collar; no shoulder boards, no tabs"""
    x = p[..., 0]
    y = p[..., 1]
    z = p[..., 2]
    s = np.abs(x)
    ang = np.degrees(np.arctan2(x, -z))
    a = np.abs(ang)
    front = a < 70
    col = cloth(p, COAT, 141)
    col *= (1 - 0.08 * smoothstep(52, 49, y))[..., None]
    # The front: the placket (a seam), the buttons (white)
    col *= (1 - 0.22 * np.exp(-((x - 0.7) / 0.1) ** 2) * front)[..., None]
    for by in (52.0, 57.0, 62.0, 67.0):
        d = np.sqrt((x / 0.45) ** 2 + ((y - by) / 0.45) ** 2)
        col = mix(col, np.array((200, 200, 196.0)) * (1.1 - 0.4 * d[..., None]), (d < 1) & front)
        col *= (1 - 0.25 * ((d > 1) & (d < 1.4) & front & (y < by)))[..., None]
    # The breast pocket on the left (his left: +X), a pen in it
    px = 4.4
    inside = (np.abs(x - px) < 2.2) & (y > 58.5) & (y < 64.5) & front
    border = inside & ((np.abs(np.abs(x - px) - 2.2) < 0.18) | (np.abs(y - 58.6) < 0.18))
    col *= (1 - 0.04 * inside)[..., None]
    col *= (1 - 0.25 * border)[..., None]
    pen = (np.abs(x - (px + 1.0)) < 0.25) & (y > 62.5) & (y < 66.0) & front
    col = mix(col, np.array((40, 60, 150.0)), pen)
    # The back: the seam, the collar folded (a darker line under it)
    col *= (1 - 0.18 * np.exp(-((x) / 0.12) ** 2) * (a > 150))[..., None]
    col *= (1 - 0.22 * np.exp(-((y - 70.6) / 0.3) ** 2))[..., None]
    col *= (1 - 0.12 * smoothstep(60, 90, a) * smoothstep(64, 69, y))[..., None]
    return np.clip(col, 0, 255)


def trousers_color(p):
    """The trousers of the scientists: grey cloth, a crease down the front"""
    col = cloth(p, TROUSERS, 151)
    x = p[..., 0]
    z = p[..., 2]
    col *= (1 + 0.06 * np.exp(-((x - 4.5) / 0.3) ** 2) * (z < 0))[..., None]
    return np.clip(col, 0, 255)


def shoes_color(p):
    """The legs of the scientists below the knee: the trousers down to the ankle, black shoes"""
    y = p[..., 1]
    col = cloth(p, TROUSERS, 151)
    shoe = np.zeros(p.shape[:-1] + (3,)) + np.array((28, 26, 26.0))
    shoe *= (0.85 + 0.3 * fbm(p, 1.2, 2, 81)[..., None])
    col = mix(col, shoe, y < 5.5)
    col *= (1 - 0.5 * smoothstep(1.4, 0.6, y))[..., None]
    col *= (1 - 0.3 * np.exp(-((y - 5.5) / 0.3) ** 2))[..., None]
    return np.clip(col, 0, 255)


def cap_color(p, front=False):
    """The side cap: olive cloth, its folded edge; at the front, the red star"""
    col = cloth(p, UNIFORM, 101)
    y = p[..., 1]
    col *= (1 - 0.25 * np.exp(-((y - 86.3) / 0.3) ** 2))[..., None]
    return np.clip(col, 0, 255)


# ---------------------------------------------------------------------------
# Baking: the texture of a polygon from a color function
# ---------------------------------------------------------------------------

def bake_tile(corners, color, width, height, ss=4):
    """The texture (height x width RGB, uint8) of a quad: corners = top left, top right, bottom right, bottom left (3D),
    color(points) -> RGB; ss x ss samples per texel (the mean)"""
    tl, tr, br, bl = [np.array(c, dtype=float) for c in corners]
    u = (np.arange(width * ss) + 0.5) / (width * ss)
    v = (np.arange(height * ss) + 0.5) / (height * ss)
    uu, vv = np.meshgrid(u, v)
    top = tl[None, None] * (1 - uu[..., None]) + tr[None, None] * uu[..., None]
    bottom = bl[None, None] * (1 - uu[..., None]) + br[None, None] * uu[..., None]
    pts = top * (1 - vv[..., None]) + bottom * vv[..., None]
    c = color(pts)
    c = c.reshape(height, ss, width, ss, 3).mean(axis=(1, 3))
    return np.clip(c, 0, 255).astype(np.uint8)
