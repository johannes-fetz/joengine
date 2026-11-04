#!/usr/bin/env python3
"""Assets of the first person shooter demo: a first person game in a secret laboratory.

    python3 tools/make_assets.py          (from the directory of the demo)

Writes:
    models/*.png            textures (procedural: nothing is taken from another game)
    models/*.obj, *.mtl     the models (converted to cd/*.JOM by Compiler/COMMON/jo_model.py, textures embedded)
    cd/LEVEL.JOL            the level: the cells (rooms) and their portals (doors), the entities (doors, start)
    cd/DEFS.JOL             the weapons
    cd/*.WAV                the sounds (16 bits mono, synthesized: tools/make_sounds.py)

World: the axes of the Saturn (X right, Y DOWN, Z forward), 1 unit = 2 cm. The floors are at y = 0, the ceilings at
y = -WALL_HEIGHT. The models are written in these axes (jo_model.py --saturn-axes): a front face is clockwise seen from
its front, the first corner gets the top left corner of the texture, the second one its top right corner.

Design rules (the limits of the VDP1, see claude/geometrie_ztreme.md of the project):
    - the walls, floors and ceilings are cut on a grid of GRID units: one quad = one whole texture, the quads share their
      corners (no T-junction: no crack, no overlap). The rooms, the doors and the openings are designed on this grid
    - the furniture is made of boxes (whole textures, stretched a little when needed)
    - the light is baked in the colors of the vertices (gouraud: 0.5 = the texture as it is, 1.0 = twice brighter)
"""
import math
import os
import re
import struct
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
DEMO = os.path.dirname(HERE)
MODELS = os.path.join(DEMO, 'models')
CD = os.path.join(DEMO, 'cd')
COMMON = os.path.normpath(os.path.join(DEMO, '..', '..', 'Compiler', 'COMMON'))
sys.path.insert(0, COMMON)
import jo_level  # noqa: E402

# ---------------------------------------------------------------------------
# Dimensions
# ---------------------------------------------------------------------------
GRID = 80                       # The grid of the walls (1.6 m): one texture cell
WALL_HEIGHT = 180               # The rooms are 3.6 m high (2 cells of tiles and a strip of stone above them)
DUCT_SIZE = 80                  # The ventilation duct: 1.6 m square
DUCT_CELL = 40                  # Its quads: the camera is 45 units from its walls, big quads would be distorted
DOOR_WIDTH = 100                # (wider than a cell: easier to go through)
DOOR_HEIGHT = 160
FRAME_WIDTH = 8                 # The posts and the lintel of the frames of the doors
TILE_TOP = 108                  # The tiles of the washroom go up to 3/5 of the walls, anthracite concrete above
# The textures with straight lines (the joints of the tiles): the distortion in pixels over which the engine draws their
# quads in parts (jo_subdivision_error, the default is 8: the joints bent where a part met a whole quad). About 1 ms more for
# each CPU in the washroom
STRAIGHT_LINE_TEXTURES = {'floor': 4}
EYE_HEIGHT = 85                 # 1.7 m (the same as main.c)

rng = np.random.default_rng(1964)

# ---------------------------------------------------------------------------
# Textures
# ---------------------------------------------------------------------------
TEXTURES = {}


def save_texture(name, rgb, key_transparent=False):
    """A texture of the models: models/name.png (RGB, 8 bits), embedded in the JOM files by the converter.
    key_transparent: the KEY color is transparent in the model (RGBA, its alpha 0: transparent for the converter)"""
    a = np.clip(rgb, 0, 255).astype(np.uint8)
    if key_transparent:
        alpha = np.where(np.all(a == np.array(KEY, dtype=np.uint8), axis=2), 0, 255).astype(np.uint8)
        Image.fromarray(np.dstack([a, alpha]), 'RGBA').save(os.path.join(MODELS, name + '.png'))
    else:
        Image.fromarray(a, 'RGB').save(os.path.join(MODELS, name + '.png'))
    TEXTURES[name] = a


def noise(n, scale, seed, octaves=3):
    """Smooth noise in [-1, 1], n x n, tiling"""
    r = np.random.default_rng(seed)
    out = np.zeros((n, n))
    amp = 1.0
    total = 0.0
    for o in range(octaves):
        m = max(2, scale >> o)
        g = r.uniform(-1, 1, (m, m))
        # (bilinear, tiling)
        ys = np.linspace(0, m, n, endpoint=False)
        xs = np.linspace(0, m, n, endpoint=False)
        y0 = np.floor(ys).astype(int)
        x0 = np.floor(xs).astype(int)
        fy = (ys - y0)[:, None]
        fx = (xs - x0)[None, :]
        y1 = (y0 + 1) % m
        x1 = (x0 + 1) % m
        v = (g[y0][:, x0] * (1 - fx) * (1 - fy) + g[y0][:, x1] * fx * (1 - fy) +
             g[y1][:, x0] * (1 - fx) * fy + g[y1][:, x1] * fx * fy)
        out += v * amp
        total += amp
        amp *= 0.5
    return out / total


def gray(v):
    return np.stack([v, v, v], axis=2)


def tint(v, color):
    """Gray levels (0..255) tinted by a color (0..1 per component)"""
    return np.stack([v * color[0], v * color[1], v * color[2]], axis=2)


def texture_tiles():
    """White ceramic tiles for a quad of 80 x TILE_TOP units: 4 x 5 square tiles of about 20 units (40 cm), light gray
    grout, each tile a little different, a soft shine (64 x 88 pixels)"""
    w, h = 64, 88
    cols = [0, 16, 32, 48, 64]
    rows = [0, 18, 35, 53, 70, 88]
    t = np.full((h, w), 208.0)
    yy, xx = np.mgrid[0:h, 0:w]
    lx = np.zeros((h, w))
    ly = np.zeros((h, w))
    for j in range(5):
        for i in range(4):
            t[rows[j]:rows[j + 1], cols[i]:cols[i + 1]] += rng.uniform(-7, 7)
            lx[rows[j]:rows[j + 1], cols[i]:cols[i + 1]] = xx[rows[j]:rows[j + 1], cols[i]:cols[i + 1]] - cols[i]
            ly[rows[j]:rows[j + 1], cols[i]:cols[i + 1]] = yy[rows[j]:rows[j + 1], cols[i]:cols[i + 1]] - rows[j]
    t += noise(64, 8, 11)[np.arange(h) % 64][:, :w] * 5
    # Shine: brighter toward the top left of each tile
    t += (12 - (lx + ly) / 2.5).clip(-6, 12) * 0.7
    # Grout: dark at the top and the left of each tile, light at the bottom and the right
    for r in rows[:-1]:
        t[r, :] = 148
    for r in rows[1:]:
        t[r - 1, :] = np.minimum(t[r - 1, :], 178)
    for c in cols[:-1]:
        t[:, c] = 148
    for c in cols[1:]:
        t[:, c - 1] = np.minimum(t[:, c - 1], 178)
    save_texture('tiles', gray(t))


def texture_stone():
    """Anthracite concrete, rough: the upper walls of the washroom (over the tiles)"""
    n = 64
    t = np.full((n, n), 62.0) + noise(n, 16, 32, 4) * 14 + rng.normal(0, 5, (n, n))
    save_texture('stone', tint(t, (0.95, 0.97, 1.0)))


def texture_porcelain():
    """The basins and the toilets: pale blue porcelain"""
    n = 32
    t = np.full((n, n), 190.0) + noise(n, 8, 33) * 6
    save_texture('porcelain', tint(t, (0.80, 0.88, 1.0)))


def sign_image(size, draw_fn, ss=4):
    """A sign drawn with antialiasing: drawn at ss times its size by draw_fn(draw, scale), reduced (Lanczos)"""
    w, h = size
    im = Image.new('RGB', (w * ss, h * ss), (255, 255, 255))
    draw_fn(ImageDraw.Draw(im), ss)
    return np.asarray(im.resize((w, h), Image.LANCZOS)).astype(float)


def texture_sign_nosmoking():
    """A no smoking sign: a red ring with a bar over a cigarette, on a white plate (64 x 64)"""
    def d(dr, k):
        dr.rectangle([0, 0, 64 * k - 1, 64 * k - 1], fill=(236, 236, 232), outline=(150, 150, 150), width=2 * k)
        dr.rectangle([14 * k, 29 * k, 44 * k, 35 * k], fill=(120, 120, 120))
        dr.rectangle([44 * k, 29 * k, 50 * k, 35 * k], fill=(235, 140, 50))
        dr.ellipse([7 * k, 7 * k, 57 * k, 57 * k], outline=(205, 30, 30), width=6 * k)
        dr.line([16 * k, 16 * k, 48 * k, 48 * k], fill=(205, 30, 30), width=6 * k)
    save_texture('nosmoke', sign_image((64, 64), d))


def texture_sign_tap():
    """A mandatory sign over the basins: a white tap and a drop on a blue disc, on a white plate (64 x 64)"""
    def d(dr, k):
        dr.rectangle([0, 0, 64 * k - 1, 64 * k - 1], fill=(236, 236, 232), outline=(150, 150, 150), width=2 * k)
        dr.ellipse([5 * k, 5 * k, 59 * k, 59 * k], fill=(30, 90, 200))
        white = (245, 245, 245)
        dr.rectangle([18 * k, 20 * k, 42 * k, 26 * k], fill=white)          # the spout
        dr.rectangle([36 * k, 20 * k, 42 * k, 34 * k], fill=white)
        dr.rectangle([26 * k, 13 * k, 32 * k, 20 * k], fill=white)          # the stem
        dr.rectangle([20 * k, 11 * k, 38 * k, 14 * k], fill=white)          # the handle
        dr.ellipse([35 * k, 38 * k, 43 * k, 48 * k], fill=white)            # the drop
        dr.polygon([(39 * k, 34 * k), (35 * k, 42 * k), (43 * k, 42 * k)], fill=white)
    save_texture('tapsign', sign_image((64, 64), d))


def texture_sign_exit():
    """A green exit sign: a white arrow (64 x 32)"""
    def d(dr, k):
        dr.rectangle([0, 0, 64 * k - 1, 32 * k - 1], fill=(40, 185, 90), outline=(25, 120, 60), width=2 * k)
        white = (245, 245, 245)
        dr.rectangle([12 * k, 13 * k, 38 * k, 19 * k], fill=white)
        dr.polygon([(36 * k, 6 * k), (52 * k, 16 * k), (36 * k, 26 * k)], fill=white)
    save_texture('exit', sign_image((64, 32), d))


def texture_sign_biohazard():
    """A yellow warning sign: a black ring and 3 discs (64 x 64)"""
    def d(dr, k):
        dr.rectangle([0, 0, 64 * k - 1, 64 * k - 1], fill=(230, 200, 40), outline=(40, 40, 40), width=3 * k)
        dr.ellipse([18 * k, 18 * k, 46 * k, 46 * k], outline=(30, 30, 30), width=4 * k)
        dr.ellipse([27 * k, 27 * k, 37 * k, 37 * k], fill=(30, 30, 30))
        for a in (90, 210, 330):
            px = 32 + 16 * math.cos(math.radians(a))
            py = 32 - 16 * math.sin(math.radians(a))
            dr.ellipse([(px - 8) * k, (py - 8) * k, (px + 8) * k, (py + 8) * k], fill=(30, 30, 30))
    save_texture('biohaz', sign_image((64, 64), d))


def texture_water():
    """The inside of the bowl of a toilet: white porcelain seen through clear water (white with a soft blue tint), the
    slope a little shaded toward the water, a light reflection, the drain a soft gray under the water (32 x 32, a circle)"""
    n = 32
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx - 15.5, (yy - 15.5) * 1.15) / 15.5
    rgb = tint(225 - 35 * np.clip((1 - d) / 0.45, 0, 1), (0.95, 0.97, 1.0))
    water = d < 0.6
    w = 200 + 15 * np.clip(1 - d / 0.6, 0, 1)
    rgb[water] = tint(w, (0.90, 0.95, 1.0))[water]
    rgb[water & (d > 0.53)] = (165, 180, 200)            # the edge of the water
    rgb[(np.hypot(xx - 12, yy - 12) < 2.6) & water] = (245, 250, 255)
    rgb[(np.hypot(xx - 16, (yy - 19) * 1.2) < 3.0)] = (150, 160, 178)
    rgb[d >= 1.0] = KEY
    save_texture('water', rgb)


def texture_urinal():
    """The front of a urinal over its lip: pale blue porcelain, a tall dark oval (the inside of the
    bowl, darker in its middle), 3 slots of the drain at its bottom (32 x 64)"""
    w, h = 32, 64
    yy, xx = np.mgrid[0:h, 0:w]
    # (the back of the inside of the bowl: in its shadow, darker than the outside, no white like the tiles around it)
    t = np.full((h, w), 150.0) + noise(64, 8, 35)[:h, :w] * 4
    # (a soft shade on the sides: the front is rounded)
    t -= (np.abs(xx - 15.5) / 15.5) ** 2 * 25
    d = np.hypot((xx - 15.5) / 8.5, (yy - 30) / 27)
    inside = d < 1
    t[inside] = 95 + 70 * d[inside] ** 2
    rgb = tint(t, (0.82, 0.90, 1.0))
    for k in range(3):
        rgb[47:56, 12 + k * 3] = (40, 44, 52)
    save_texture('urinal', rgb)


def texture_drain():
    """The bottom of a basin: the porcelain getting darker toward a small dark drain (32 x 32, a circle)"""
    n = 32
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx - 15.5, yy - 15.5) / 15.5
    rgb = tint(175 - 50 * np.clip(1 - d, 0, 1), (0.80, 0.88, 1.0))
    rgb[d < 0.22] = (90, 95, 105)
    rgb[d < 0.14] = (35, 38, 44)
    rgb[d >= 1.0] = KEY
    save_texture('drain', rgb)


def texture_switch():
    """The flush plate of the stalls: a gray plate with a red button"""
    w, h = 32, 16
    rgb = np.zeros((h, w, 3))
    rgb[:, :] = (150, 150, 155)
    rgb[1:-1, 1:-1] = (205, 205, 210)
    rgb[4:12, 9:17] = (200, 40, 40)
    rgb[4:12, 20:24] = (90, 90, 95)
    save_texture('switch', rgb)


def texture_floor():
    """Floor of the washroom: small dark green tiles (4 x 4 per cell), dirty joints"""
    n = 64
    t = np.full((n, n), 110.0) + noise(n, 16, 12) * 14
    for j in range(4):
        for i in range(4):
            t[j * 16:(j + 1) * 16, i * 16:(i + 1) * 16] += rng.uniform(-9, 9)
    t[::16, :] = 70
    t[:, ::16] = 70
    save_texture('floor', tint(t, (0.72, 0.86, 0.80)))


def texture_panel():
    """The doors of the stalls: teal painted metal on the whole door (no frame), lighter at the top,
    soft vertical streaks; the lock on the left at the middle (seen from the room): a small rounded steel plate with the
    red sign "occupied" and a screw (64 x 128: the door is 94 x 120)"""
    w, h = 64, 128
    yy, xx = np.mgrid[0:h, 0:w].astype(float)
    # (streaks: a noise stretched along Y, the light from the top)
    streaks = np.tile(noise(w, 4, 13)[:1, :], (h, 1)) * 10 + np.kron(noise(64, 8, 14), np.ones((2, 1)))[:h, :w] * 3
    t = 118 + 52 * (1 - yy / h) ** 1.4 + streaks
    rgb = tint(np.clip(t, 0, 255), (0.48, 0.86, 0.86))
    # (the base of the strips of the door, see build_hinged_door_model(): the whole door is painted on them; a strip is a
    # quarter of the door: 16 pixels wide, the copies painted have its size (the texture of 64 x 128 for each strip cost 4
    # times the VRAM and the tiles near the camera, 2 more sprites each))
    save_texture('panelp', rgb[:, :16].copy())
    # The lock: a plate 11 x 6 with rounded corners, light steel lit from the top, the red sign, a dark screw on the right
    x0, y0, pw, ph = 3, 61, 11, 6
    for y in range(ph):
        for x in range(pw):
            if (x in (0, pw - 1)) and (y in (0, ph - 1)):
                continue
            v = 214 - y * 12
            rgb[y0 + y, x0 + x] = (v, v, v + 4)
    rgb[y0 + ph, x0 + 1:x0 + pw - 1] = (60, 90, 92)
    rgb[y0 + 2:y0 + 4, x0 + 3:x0 + 6] = (205, 30, 35)
    rgb[y0 + 2:y0 + 4, x0 + 8] = (40, 40, 45)
    save_texture('panel', rgb)


def texture_white():
    """The partitions of the stalls: plain light gray"""
    n = 32
    t = np.full((n, n), 196.0) + noise(n, 4, 25) * 2
    save_texture('white', tint(t, (1.0, 1.0, 1.0)))


def texture_hdoor():
    """A door of the rooms: light gray brushed steel (vertical streaks), a lighter push plate on the side
    of the handle at the middle, a ventilation grille at the bottom"""
    w, h = 64, 128
    yy, xx = np.mgrid[0:h, 0:w]
    streaks = rng.normal(0, 6, w)[None, :].repeat(h, axis=0)
    t = np.full((h, w), 165.0) + streaks + noise(64, 16, 26).repeat(2, axis=0)[:h, :w] * 5
    # A soft shine toward the top
    t += (20 - yy * 0.25).clip(-10, 20)
    # The edges of the panel
    t[(xx < 2) | (xx >= w - 2) | (yy < 2)] -= 35
    # The push plate, on the free edge of the door (the hinge is the left of the texture, see build_hinged_door_model())
    plate = (xx >= 42) & (xx < 56) & (yy >= 50) & (yy < 70)
    t[plate] += 28
    t[((xx == 42) | (xx == 55)) & (yy >= 50) & (yy < 70)] -= 30
    t[((yy == 50) | (yy == 69)) & (xx >= 42) & (xx < 56)] -= 30
    # The grille at the bottom: dark slats
    grille = (yy >= 110) & (yy < 124) & (xx >= 10) & (xx < w - 10)
    t[grille & (yy % 3 == 0)] = 70
    t[grille & (yy % 3 != 0)] = 130
    save_texture('hdoor', tint(t, (0.97, 0.98, 1.0)))


def texture_concrete():
    """Concrete of the corridors: gray, rough, a few stains"""
    n = 64
    t = np.full((n, n), 128.0) + noise(n, 16, 14, 4) * 22 + rng.normal(0, 3, (n, n))
    for k in range(3):
        cx, cy = rng.integers(0, n, 2)
        yy, xx = np.mgrid[0:n, 0:n]
        d = np.hypot((xx - cx + n // 2) % n - n // 2, (yy - cy + n // 2) % n - n // 2)
        t -= np.clip(14 - d, 0, 14) * 1.2
    save_texture('concrete', tint(t, (1.0, 0.98, 0.94)))


def texture_ceiling():
    """Ceiling of the rooms: dark panels with a grid of joints"""
    n = 64
    t = np.full((n, n), 78.0) + noise(n, 8, 15) * 8
    t[::32, :] = 50
    t[:, ::32] = 50
    save_texture('ceiling', tint(t, (0.90, 0.92, 0.96)))


def texture_lamp_cells():
    """The cells of the ceilings with a lamp: the ceiling with a white panel in the middle (40 units: the middle half of a
    cell of 80, a quarter of a cell of 160). The cell is the quad of the ceiling itself (a panel just under the ceiling, a
    polygon of its own, was cut by the parts of the ceiling near the camera). The cell is lit like the ceiling (the halo of
    the lamp, see bake_light(): the same colors on the corners as the cells around it, no edge)"""
    n = 64
    for name, half in (('lampcell', 16), ('lampcell2', 8)):
        t = np.full((n, n), 78.0) + noise(n, 8, 15) * 8
        t[::32, :] = 50
        t[:, ::32] = 50
        yy, xx = np.mgrid[0:n, 0:n]
        inside = (np.abs(xx + 0.5 - n / 2) < half) & (np.abs(yy + 0.5 - n / 2) < half)
        edge = inside & ~((np.abs(xx + 0.5 - n / 2) < half - 2) & (np.abs(yy + 0.5 - n / 2) < half - 2))
        t[inside] = 250
        t[edge] = 170
        save_texture(name, tint(t, (1.0, 1.0, 0.93)))
    # The alarm lamp of the gallery (see ALARM_ROOMS): the ceiling with a red dome in the middle, seen from under it: a dark
    # round base, the red glass bright in its middle, the bars of its cage across it
    base = np.full((n, n), 78.0) + noise(n, 8, 15) * 8
    base[::32, :] = 50
    base[:, ::32] = 50
    rgb = np.dstack([base * 0.9, base * 0.92, base * 0.96])
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx + 0.5 - n / 2, yy + 0.5 - n / 2)
    rgb[d < 11] = (40, 38, 36)
    glass = d < 9
    light = np.clip(1.0 - d / 9.0, 0, 1)
    red = np.dstack([150 + 105 * light, 20 + 120 * light ** 3, 15 + 90 * light ** 3])
    rgb[glass] = red[glass]
    cage = glass & ((np.abs(xx + 0.5 - n / 2) < 0.8) | (np.abs(yy + 0.5 - n / 2) < 0.8) | (np.abs(d - 5.5) < 0.6))
    rgb[cage] = (30, 28, 26)
    save_texture('alarmcell', rgb)


def tile_blur(a, sigma):
    """Gaussian blur of a 2D array that tiles (the edges wrap)"""
    r = max(1, int(sigma * 3))
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    p = np.pad(a, r, mode='wrap')
    p = np.apply_along_axis(lambda v: np.convolve(v, k, mode='valid'), 0, p)
    return np.apply_along_axis(lambda v: np.convolve(v, k, mode='valid'), 1, p)


def tile_noise(r, n, cells):
    """Smooth noise (about -1..1) of n x n pixels that tiles, cells features across"""
    g = np.tile(r.normal(0, 1, (cells, cells)), (3, 3))
    big = np.asarray(Image.fromarray(g.astype(np.float32), 'F').resize((n * 3, n * 3), Image.BICUBIC))
    return big[n:2 * n, n:2 * n]


def tile_fbm(r, n, cells, octaves=4):
    """Noise of several octaves that tiles (the biggest features: cells across)"""
    out = np.zeros((n, n))
    amp = 1.0
    for _ in range(octaves):
        out += tile_noise(r, n, cells) * amp
        cells *= 2
        amp *= 0.5
    return out / 1.9


DUCT_PAINT = (0.70, 0.80, 0.74)    # The gray green paint of the duct
DUCT_SCALE = 8                     # The duct is rendered 8 times bigger than its textures, then filtered


def duct_bake(name, hgt, col, rough, n, target):
    """Light a texture of the duct like a photo of today and save it (see locker_bake()): the height map (texels) and the
    materials (gray level, roughness) of a picture DUCT_SCALE times bigger that tiles; soft light from several directions,
    occlusion in the hollows, the sheen of the paint; all the colors on the ramp of the paint (16 colors in the game); the
    mean brightness target (0 to 255); filtered down to n x n"""
    s = DUCT_SCALE
    gy, gx = np.gradient(hgt * s)
    nrm = np.dstack([-gx, -gy, np.ones_like(gx)])
    nrm /= np.linalg.norm(nrm, axis=2, keepdims=True)
    diffuse = np.zeros(hgt.shape)
    spec = np.zeros(hgt.shape)
    for L, weight in (((-0.3, -0.6, 0.74), 0.4), ((0.3, -0.5, 0.8), 0.3), ((0.0, 0.0, 1.0), 0.3)):
        L = np.array(L) / np.linalg.norm(L)
        diffuse += np.clip((nrm * L).sum(axis=2), 0, 1) * weight
        hv = L + np.array([0, 0, 1.0])
        hv /= np.linalg.norm(hv)
        p = 2 / np.maximum(rough, 0.05) ** 2
        spec += np.clip((nrm * hv).sum(axis=2), 0, 1) ** p * weight * (p + 2) / 8
    cav = np.clip(tile_blur(hgt, 1.0 * s) - hgt, 0, None)
    ao = np.clip(1 - cav * 1.5, 0.25, 1)
    lum = col * (0.35 + diffuse * 0.9) * ao + spec * 0.05 * ao
    for exposure in np.linspace(0.3, 4.0, 371):
        e = lum * exposure
        e = e / (1 + e * 0.15)
        srgb = np.where(e <= 0.0031308, e * 12.92, 1.055 * np.power(np.clip(e, 0, None), 1 / 2.4) - 0.055)
        if srgb.mean() * 255 >= target:
            break
    hue = np.array(DUCT_PAINT) ** (1 / 2.2)
    hue = hue / (hue @ np.array([0.299, 0.587, 0.114]))
    big = np.clip(srgb[..., None] * hue[None, None, :] * 255, 0, 255)
    small = np.dstack([np.asarray(Image.fromarray(big[..., k].astype(np.float32), 'F').resize((n, n), Image.LANCZOS))
                       for k in range(3)])
    save_texture(name, small)


def duct_plates(floor, seed):
    """The height map and the materials of the plates of the duct (see texture_duct())"""
    n, s = 64, DUCT_SCALE
    M = n * s
    r = np.random.default_rng(seed)
    yy, xx = (np.mgrid[0:M, 0:M].astype(float) + 0.5) / s
    e = np.minimum(np.minimum(xx, n - xx), np.minimum(yy, n - yy))        # texels from the edge of the plate
    # The plates butt against each other: a groove between them
    hgt = np.clip(e / 1.0, 0, 1) * 0.5 - np.exp(-e ** 2 / 0.1) * 0.4
    # Rivets in rows along the edges
    rivet = np.zeros((M, M))
    for k in np.arange(4, n, 8):
        for cy, cx in ((2.5, k), (n - 2.5, k), (k, 2.5), (k, n - 2.5)):
            d = np.hypot(xx - cx, yy - cy)
            rivet = np.maximum(rivet, np.sqrt(np.clip(1 - (d / 1.1) ** 2, 0, 1)))
    hgt += rivet * 0.7
    if not floor:
        hgt += np.exp(-((yy - n / 2) ** 2) / 3.0) * 0.6                 # a stiffening rib across the middle of the plate
    for _ in range(4):
        cx, cy, rad = r.uniform(6, 58), r.uniform(6, 58), r.uniform(3, 7)
        hgt -= np.exp(-((xx - cx) ** 2 + (yy - cy) ** 2) / (2 * (rad / 2) ** 2)) * r.uniform(0.1, 0.25)
    curv = np.clip(hgt - tile_blur(hgt, 0.6 * s), 0, None)
    col = 0.30 + tile_fbm(r, M, 3) * 0.02 + tile_fbm(r, M, 12) * 0.01
    rough = np.full((M, M), 0.6)
    # The paint worn on the edges and the rivets: bare steel
    wear = np.clip((curv * 7 + tile_fbm(r, M, 6) * 0.4 + rivet * 0.6 - 0.9) * 3, 0, 1)
    col = col * (1 - wear) + 0.45 * wear
    rough = rough * (1 - wear) + 0.3 * wear
    # Rust running down from the rivets
    runs = np.zeros((M, M))
    for k in np.arange(4, n, 8):
        runs += np.exp(-((xx - k) ** 2) / 0.3) * np.clip((yy - 3) / 20, 0, 1) * np.exp(-(yy - 3) / 25) * (yy > 3)
    runs *= np.clip(tile_noise(r, M, 8) + 0.3, 0, 1)
    col = col * (1 - runs * 0.5) + 0.18 * runs * 0.5
    # Dust, more on the floor and in the grooves
    dust = np.clip(tile_fbm(r, M, 3) * 0.5 + (0.6 if floor else 0.15) + np.exp(-e ** 2 / 2) * 0.3, 0, 1)
    col = col * (1 - dust * 0.3) + 0.26 * dust * 0.3
    return hgt, col, rough


def texture_duct():
    """The ventilation duct: gray green painted steel plates riveted along their edges, a stiffening rib across each plate of
    the walls and the ceiling, the paint worn on the edges and the rivets, rust running down from the rivets, dust (more on
    the floor). Rendered like a photo of today from a height map 8 times bigger (see duct_bake()), 64 x 64, tiling"""
    duct_bake('duct', *duct_plates(False, 2), 64, 118)
    duct_bake('ductfl', *duct_plates(True, 2), 64, 105)


def texture_frame():
    """The frames of the doors and the dark metal: brushed steel, dark"""
    n = 32
    t = np.full((n, n), 70.0) + noise(n, 4, 18) * 6
    t += rng.normal(0, 2, (n, n))
    save_texture('frame', tint(t, (0.9, 0.92, 1.0)))


def texture_crate():
    """A wooden crate: planks, a frame, stenciled marks"""
    n = 64
    t = np.full((n, n), 150.0) + noise(n, 16, 19) * 14
    yy, xx = np.mgrid[0:n, 0:n]
    t += np.sin(yy * 0.9 + noise(n, 8, 20) * 3) * 7      # the grain
    for y in (0, 21, 42, 63):
        t[y, :] -= 45
    edge = (xx < 6) | (xx >= n - 6) | (yy < 6) | (yy >= n - 6)
    t[edge] += 12
    t[(xx == 6) | (xx == n - 7) | (yy == 6) | (yy == n - 7)] -= 30
    t[28:36, 18:46] -= 50                                 # a dark stencil band
    save_texture('crate', tint(t, (1.0, 0.78, 0.48)))


def texture_hazard():
    """Yellow and black stripes (the frames of the security doors)"""
    n = 32
    yy, xx = np.mgrid[0:n, 0:n]
    s = ((xx + yy) // 8) % 2
    t = np.where(s == 0, 220.0, 30.0) + noise(n, 4, 21) * 6
    rgb = np.stack([t, np.where(s == 0, t * 0.85, t), np.where(s == 0, t * 0.1, t)], axis=2)
    save_texture('hazard', rgb)


LOCKER_PAINT = (0.16, 0.25, 0.42)     # Blue paint of the lockers (linear)
LOCKER_SCALE = 8                       # The lockers are rendered 8 times bigger than their textures, then filtered


def blur(a, sigma):
    """Gaussian blur of a 2D array (separable, edges clamped)"""
    r = max(1, int(sigma * 3))
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    p = np.pad(a, r, mode='edge')
    p = np.apply_along_axis(lambda v: np.convolve(v, k, mode='valid'), 0, p)
    return np.apply_along_axis(lambda v: np.convolve(v, k, mode='valid'), 1, p)


def locker_fbm(r, H, W, scale, octaves=4):
    """Smooth noise (about -1..1) of H x W pixels, the biggest features of scale pixels"""
    out = np.zeros((H, W))
    amp = 1.0
    for _ in range(octaves):
        g = r.normal(0, 1, (max(2, int(H / scale)), max(2, int(W / scale))))
        out += np.asarray(Image.fromarray(g.astype(np.float32), 'F').resize((W, H), Image.BICUBIC)) * amp
        scale /= 2
        amp *= 0.5
    return out / 1.9


def locker_bake(name, hgt, col, rough, yy, w, h):
    """Light a locker texture like a photo of today and save it: the height map (texels) and the materials (linear color,
    roughness) of a picture LOCKER_SCALE times bigger; soft light of the fluorescent tubes of the ceiling (several
    directions from above), occlusion in the hollows, the sheen of the paint; all the colors on the ramp of the paint (the
    game draws it in 16 colors: 14 levels of one hue, a mix of hues was dithered into colored grain); filtered down to w x h"""
    s = LOCKER_SCALE
    gy, gx = np.gradient(hgt * s)
    n = np.dstack([-gx, -gy, np.ones_like(gx)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    diffuse = np.zeros(hgt.shape)
    spec = np.zeros(hgt.shape)
    for L, weight in (((-0.35, -0.60, 0.72), 0.25), ((0.0, -0.65, 0.76), 0.30), ((0.35, -0.60, 0.72), 0.25),
                      ((0.0, -0.25, 0.97), 0.20)):
        L = np.array(L) / np.linalg.norm(L)
        diffuse += np.clip((n * L).sum(axis=2), 0, 1) * weight
        hv = L + np.array([0, 0, 1.0])
        hv /= np.linalg.norm(hv)
        p = 2 / np.maximum(rough, 0.05) ** 2
        spec += np.clip((n * hv).sum(axis=2), 0, 1) ** p * weight * (p + 2) / 8
    cav = np.clip(blur(hgt, 1.0 * s) - hgt, 0, None)
    cav_wide = np.clip(blur(hgt, 3.0 * s) - hgt, 0, None)
    ao = np.clip(1 - cav * 1.4 - cav_wide * 0.6, 0.2, 1) * (1 - 0.25 * np.clip((yy - 96) / 32, 0, 1))
    lin = col * ((0.30 + diffuse * 0.95) * ao)[..., None] + (spec * 0.045 * ao)[..., None]
    lum = lin @ np.array([0.299, 0.587, 0.114])
    # (exposure: the same mean brightness as the other textures of the level)
    for exposure in np.linspace(0.3, 3.0, 271):
        e = lum * exposure
        e = e / (1 + e * 0.15)
        srgb = np.where(e <= 0.0031308, e * 12.92, 1.055 * np.power(np.clip(e, 0, None), 1 / 2.4) - 0.055)
        if srgb.mean() * 255 >= 116:
            break
    hue = np.array(LOCKER_PAINT) ** (1 / 2.2)
    hue = hue / (hue @ np.array([0.299, 0.587, 0.114]))
    big = np.clip(srgb[..., None] * hue[None, None, :] * 255, 0, 255)
    small = np.dstack([np.asarray(Image.fromarray(big[..., k].astype(np.float32), 'F').resize((w, h), Image.LANCZOS))
                       for k in range(3)])
    save_texture(name, small)


def texture_locker():
    """The front of the steel lockers, 2 doors (the model: 80 x 140 units, 1.6 m x 2.8 m): a frame, raised doors with
    rounded edges, louvered vents at the top and the bottom, aluminium number plates, recessed lift handles and padlock
    hasps, hinges, a dark base; the paint worn on the edges and near the handles, scratches, fingerprints, dents, dust in the
    hollows, grime near the floor. Rendered from a height map and layered materials (see locker_bake())"""
    w, h, s = 64, 128, LOCKER_SCALE
    W, H = w * s, h * s
    r = np.random.default_rng(22)
    yy, xx = (np.mgrid[0:H, 0:W].astype(float) + 0.5) / s
    lx = xx % 32
    di = (xx // 32).astype(int)

    def box(x0, x1, y0, y1, radius):
        """1 inside the rectangle of a door (texels), 0 outside, a rounded edge"""
        fx = np.clip(np.minimum(lx - x0, x1 - lx) / radius, 0, 1)
        fy = np.clip(np.minimum(yy - y0, y1 - yy) / radius, 0, 1)
        return np.sin(np.minimum(fx, fy) * np.pi / 2)

    # The shape (heights in texels)
    hgt = np.zeros((H, W))
    door = box(0.7, 31.3, 3.8, 115.8, 1.1)
    hgt += door * 0.9
    hgt -= np.clip((yy - 116.6) * 3, 0, 1) * 0.5                         # the base, set back
    for top, rows in ((12.5, 6), (97.5, 4)):
        y0, y1 = top, top + rows * 3
        hgt -= box(7.3, 24.7, y0 - 0.5, y1 + 0.5, 0.8) * 0.35             # the stamped window of the vent
        f = ((yy - y0) % 3) / 3
        hgt += box(8.6, 23.4, y0 + 0.3, y1 - 0.3, 0.5) * (np.minimum(f * 1.6, (1 - f) * 6) * 0.55 - 0.65)
    hgt += box(10.6, 20.4, 36.6, 41.4, 0.35) * 0.18                      # the number plate
    hgt -= box(22.4, 27.8, 53.4, 70.6, 1.4) * 0.95                       # the recess of the handle
    lever = box(23.7, 26.5, 56.6, 66.4, 0.8)
    hgt += lever * 1.15
    hasp = np.clip(box(23.8, 26.4, 72.0, 75.6, 0.5) - box(24.7, 25.5, 72.9, 74.7, 0.3) * 1.5, 0, 1)
    hgt += hasp * 0.55
    for y0 in (18.0, 96.0):
        hgt += box(0.3, 2.3, y0, y0 + 6.5, 1.0) * 0.55                    # the hinges
    for _ in range(7):
        cx, cy, rad = r.uniform(3, 61), r.uniform(8, 110), r.uniform(1.5, 4.0)
        hgt -= np.exp(-((xx - cx) ** 2 + (yy - cy) ** 2) / (2 * (rad / 2) ** 2)) * r.uniform(0.1, 0.3) * door
    hgt += locker_fbm(r, H, W, 48, 3) * 0.012 * door                      # the orange peel of the paint
    # Masks
    cav = np.clip(blur(hgt, 1.0 * s) - hgt, 0, None)
    cav_wide = np.clip(blur(hgt, 3.0 * s) - hgt, 0, None)
    curv = np.clip(hgt - blur(hgt, 0.6 * s), 0, None)                     # the convex edges, worn first
    up = np.clip(-np.gradient(hgt * s)[0], 0, None)                       # the slopes facing up (dust)
    n_big, n_mid, n_fine = locker_fbm(r, H, W, 192), locker_fbm(r, H, W, 48), locker_fbm(r, H, W, 12, 2)
    # Materials: the paint (the 2 doors slightly different), the steel where it is worn
    col = np.array(LOCKER_PAINT)[None, None, :] * np.where(di == 0, 1.0, 0.96)[..., None] * \
        (1 + n_big[..., None] * 0.05 + n_mid[..., None] * 0.02)
    rough = np.full((H, W), 0.55)
    near_handle = np.exp(-((lx - 25.5) ** 2 / 18 + (yy - 61) ** 2 / 160))
    wear = np.clip((curv * 6 + near_handle * 0.9 + n_mid * 0.35 + n_fine * 0.25 - 0.95) * 4, 0, 1) * (yy < 116.5)
    steel = np.array([0.50, 0.51, 0.52]) * (1 + n_fine[..., None] * 0.08)
    col = col * (1 - wear[..., None]) + steel * wear[..., None]
    rough = rough * (1 - wear) + 0.30 * wear
    frame = np.clip(1 - door * 1.3, 0, 1) * (yy < 116.6)
    col = col * (1 - frame[..., None] * 0.35)
    col = np.where((yy > 116.6)[..., None], np.array([0.035, 0.037, 0.04]) * (1 + n_mid[..., None] * 0.15), col)
    metal = np.clip(lever * 1.5, 0, 1) + hasp                             # the chromed lever and hasp
    col = col * (1 - metal[..., None]) + np.array([0.55, 0.56, 0.57]) * metal[..., None]
    rough = rough * (1 - metal) + 0.2 * metal
    plate = box(10.9, 20.1, 36.9, 41.1, 0.3)
    col = col * (1 - plate[..., None]) + np.array([0.80, 0.80, 0.78]) * plate[..., None]
    # (the digits engraved on the plates: segments)
    seg = {'1': 'bc', '4': 'bcfg', '5': 'acdfg'}
    pts = {'a': ((0, 0), (1, 0)), 'b': ((1, 0), (1, 0.5)), 'c': ((1, 0.5), (1, 1)), 'd': ((0, 1), (1, 1)),
           'e': ((0, 0.5), (0, 1)), 'f': ((0, 0), (0, 0.5)), 'g': ((0, 0.5), (1, 0.5))}
    ink = np.zeros((H, W))
    for d, text in enumerate(('14', '15')):
        for c, ch in enumerate(text):
            x0, y0 = d * 32 + 12.9 + c * 3.4, 37.7
            for sg in seg[ch]:
                (ax, ay), (bx, by) = pts[sg]
                for k in np.linspace(0, 1, 30):
                    px, py = int((x0 + (ax + (bx - ax) * k) * 2.1) * s), int((y0 + (ay + (by - ay) * k) * 2.6) * s)
                    ink[py - 2:py + 3, px - 2:px + 3] = 1
    col = col * (1 - ink[..., None] * 0.8)
    # Scratches (in groups), fingerprints near the handles, dust in the hollows and on the slats, grime near the floor
    scratch = np.zeros((H, W))
    for _ in range(18):
        cx, cy = r.uniform(4, 60), r.uniform(10, 108)
        for _ in range(r.integers(1, 4)):
            a, length = r.normal(0.25, 0.3), r.uniform(1.5, 4.5)
            x0, y0 = cx + r.normal(0, 0.6), cy + r.normal(0, 0.6)
            for k in np.linspace(0, length, int(length * s * 1.5)):
                px, py = int((x0 + np.cos(a) * k) * s), int((y0 + np.sin(a) * k) * s)
                if 0 <= px < W and 0 <= py < H:
                    scratch[py, px] = 1
    scratch *= door > 0.9
    col = col * (1 - scratch[..., None] * 0.6) + steel * scratch[..., None] * 0.6
    rough = rough * (1 - scratch * 0.6) + 0.3 * scratch * 0.6
    smudge = np.clip(near_handle * 1.2 * (0.5 + n_mid * 0.8), 0, 1) * 0.35
    col = col * (1 - smudge[..., None] * 0.3)
    rough = rough * (1 - smudge) + 0.25 * smudge
    dust = np.clip(cav * 2.5 + up * 0.15 + cav_wide * 0.8, 0, 1) * (0.7 + n_mid * 0.4)
    col = col * (1 - dust[..., None] * 0.45) + np.array([0.30, 0.29, 0.27]) * dust[..., None] * 0.45
    runs = np.clip(np.asarray(Image.fromarray(blur(r.normal(0, 1, (H // 24, W)), 1.2).astype(np.float32), 'F')
                              .resize((W, H), Image.BILINEAR)), 0, None)
    grime = np.clip(np.clip((yy - 88) / 28, 0, 1) ** 1.5 * (0.6 + n_big * 0.3 + n_mid * 0.3) +
                    runs * np.clip((yy - 30) / 80, 0, 1) * 0.25, 0, 1) * (yy < 116.5)
    col = col * (1 - grime[..., None] * 0.35) + np.array([0.16, 0.14, 0.12]) * grime[..., None] * 0.35
    rough = rough * (1 - grime * 0.5) + 0.85 * grime * 0.5
    # The dark gaps between the doors
    gap = np.exp(-np.minimum(lx, 32.0 - lx) ** 2 / 0.5) * (yy < 116.5)
    col = col * (1 - gap[..., None] * 0.7)
    locker_bake('locker', hgt, col, rough, yy, w, h)


def texture_lockerside():
    """The sides of the rows of lockers (the model: 24 x 140 units, 48 cm deep): a flat steel panel folded on its front and
    back edges, the top rail and the base of the front, the same paint, worn on the folds, scratches, grime near the floor
    (symmetric: the front edge can be on either side)"""
    w, h, s = 32, 128, LOCKER_SCALE
    W, H = w * s, h * s
    r = np.random.default_rng(23)
    yy, xx = (np.mgrid[0:H, 0:W].astype(float) + 0.5) / s
    edge = np.minimum(xx, w - xx)                                         # texels from the nearest vertical edge
    hgt = np.clip(edge / 1.2, 0, 1) * 0.5                                 # the folds of the panel
    hgt += np.clip((edge - 1.6) / 0.6, 0, 1) * -0.12                      # (a slight hollow inside the folds)
    hgt -= np.clip((yy - 116.6) * 3, 0, 1) * 0.5                          # the base, set back
    hgt -= np.clip((3.6 - yy) * 3, 0, 1) * 0.2                            # the top rail
    for _ in range(3):
        cx, cy, rad = r.uniform(5, 27), r.uniform(10, 110), r.uniform(2.0, 4.0)
        hgt -= np.exp(-((xx - cx) ** 2 + (yy - cy) ** 2) / (2 * (rad / 2) ** 2)) * r.uniform(0.1, 0.25)
    hgt += locker_fbm(r, H, W, 48, 3) * 0.012
    curv = np.clip(hgt - blur(hgt, 0.6 * s), 0, None)
    n_big, n_mid, n_fine = locker_fbm(r, H, W, 192), locker_fbm(r, H, W, 48), locker_fbm(r, H, W, 12, 2)
    col = np.array(LOCKER_PAINT)[None, None, :] * (1 + n_big[..., None] * 0.05 + n_mid[..., None] * 0.02)
    rough = np.full((H, W), 0.55)
    wear = np.clip((curv * 6 + n_mid * 0.35 + n_fine * 0.25 - 0.95) * 4, 0, 1) * (yy < 116.5)
    steel = np.array([0.50, 0.51, 0.52]) * (1 + n_fine[..., None] * 0.08)
    col = col * (1 - wear[..., None]) + steel * wear[..., None]
    rough = rough * (1 - wear) + 0.30 * wear
    col = np.where((yy < 3.6)[..., None], col * 0.65, col)
    col = np.where((yy > 116.6)[..., None], np.array([0.035, 0.037, 0.04]) * (1 + n_mid[..., None] * 0.15), col)
    scratch = np.zeros((H, W))
    for _ in range(8):
        cx, cy = r.uniform(4, 28), r.uniform(10, 108)
        a, length = r.normal(0.1, 0.3), r.uniform(1.5, 4.0)
        for k in np.linspace(0, length, int(length * s * 1.5)):
            px, py = int((cx + np.cos(a) * k) * s), int((cy + np.sin(a) * k) * s)
            if 0 <= px < W and 0 <= py < H:
                scratch[py, px] = 1
    col = col * (1 - scratch[..., None] * 0.6) + steel * scratch[..., None] * 0.6
    runs = np.clip(np.asarray(Image.fromarray(blur(r.normal(0, 1, (H // 24, W)), 1.2).astype(np.float32), 'F')
                              .resize((W, H), Image.BILINEAR)), 0, None)
    grime = np.clip(np.clip((yy - 88) / 28, 0, 1) ** 1.5 * (0.6 + n_big * 0.3 + n_mid * 0.3) +
                    runs * np.clip((yy - 30) / 80, 0, 1) * 0.25, 0, 1) * (yy < 116.5)
    col = col * (1 - grime[..., None] * 0.35) + np.array([0.16, 0.14, 0.12]) * grime[..., None] * 0.35
    rough = rough * (1 - grime * 0.5) + 0.85 * grime * 0.5
    locker_bake('lockerside', hgt, col, rough, yy, w, h)


def texture_bdoor():
    """The doors of the lower floor: brown painted steel, wide horizontal ribs, darker edges, a push
    plate on the free edge (the hinge is the left of the texture)"""
    w, h = 64, 128
    yy, xx = np.mgrid[0:h, 0:w]
    t = np.full((h, w), 120.0) + noise(64, 16, 31).repeat(2, axis=0)[:h, :w] * 6 + rng.normal(0, 2, (h, w))
    # (the ribs: a light top edge, a dark bottom edge, every 16 pixels)
    rib = yy % 16
    t[rib == 1] += 26
    t[rib == 2] += 14
    t[rib == 13] -= 22
    t[rib == 14] -= 34
    t[(xx < 2) | (xx >= w - 2) | (yy < 2) | (yy >= h - 2)] -= 40
    plate = (xx >= 46) & (xx < 56) & (yy >= 52) & (yy < 68)
    t[plate] = 150
    save_texture('bdoor', tint(t, (0.86, 0.62, 0.44)))


def texture_glass():
    """The glass of the windows: a pale blue gray with soft diagonal reflections (half transparent in the game)"""
    n = 32
    yy, xx = np.mgrid[0:n, 0:n]
    t = np.full((n, n), 150.0) + np.clip(np.sin((xx + yy) * 0.35) * 30 - 10, 0, 30) + noise(n, 8, 34) * 4
    save_texture('glass', tint(t, (0.80, 0.92, 1.0)))


def texture_sdoor():
    """The doors of the airlock: light gray steel without handle (they slide), a band of yellow and black stripes at the
    middle, dark edges"""
    w, h = 64, 128
    yy, xx = np.mgrid[0:h, 0:w]
    t = np.full((h, w), 160.0) + rng.normal(0, 4, w)[None, :].repeat(h, axis=0) + noise(64, 16, 35).repeat(2, axis=0)[:h, :w] * 4
    t[(xx < 2) | (xx >= w - 2) | (yy < 2) | (yy >= h - 2)] -= 40
    rgb = tint(t, (0.96, 0.98, 1.0))
    band = (yy >= 58) & (yy < 70)
    stripe = ((xx + yy) // 6) % 2 == 0
    rgb[band & stripe] = (215, 180, 25)
    rgb[band & ~stripe] = (30, 30, 30)
    save_texture('sdoor', rgb)


def texture_casing():
    """The plastic of the computers: a light beige gray, a little noise"""
    n = 16
    t = np.full((n, n), 176.0) + noise(n, 4, 36) * 4
    save_texture('casing', tint(t, (1.0, 0.97, 0.90)))


def texture_screen():
    """The screen of a computer: lines of green text on a dark green background, a gray border"""
    w, h = 32, 32
    rgb = np.zeros((h, w, 3))
    rgb[:, :] = (16, 40, 22)
    lines = np.random.default_rng(37)
    for y in range(5, h - 4, 3):
        x = 5
        while x < w - 6:
            length = int(lines.integers(2, 7))
            rgb[y, x:min(x + length, w - 5)] = (90, 230, 110)
            x += length + int(lines.integers(1, 3))
            if lines.random() < 0.2:
                break
    rgb[:3, :] = rgb[-3:, :] = (120, 118, 112)
    rgb[:, :3] = rgb[:, -3:] = (120, 118, 112)
    save_texture('screen', rgb)


def screen_text(rgb, seed, cursor_row, cursor_on, scroll=0):
    """Lines of green text on a screen (rgb: the image, 5 pixels of margin), the lines shifted up by scroll pixels, a
    cursor block at the end of the row cursor_row when cursor_on"""
    h, w = rgb.shape[:2]
    lines = np.random.default_rng(seed)
    for y in range(5 - scroll, h - 4, 3):
        if y < 4:
            # (a line scrolled out: its random numbers drawn anyway, the lines below keep their text)
            x = 5
            while x < w - 6:
                x += int(lines.integers(2, 7)) + int(lines.integers(1, 3))
                if lines.random() < 0.2:
                    break
            continue
        x = 5
        while x < w - 6:
            length = int(lines.integers(2, 7))
            rgb[y, x:min(x + length, w - 5)] = (90, 230, 110)
            x += length + int(lines.integers(1, 3))
            if lines.random() < 0.2:
                break
    if cursor_on and 4 <= cursor_row < h - 4:
        rgb[cursor_row, w - 10:w - 7] = (150, 255, 170)


def texture_screen_frames():
    """The screen of a computer animated (3 frames swapped by the game, see tools/screen_frames.h): the same text with a
    cursor blinking on the last line, then the text scrolled up by a line; and the screen broken (dark, a star of cracks,
    the glass gone white at the impact)"""
    w, h = 32, 32
    for k, (cursor_on, scroll) in enumerate(((False, 0), (True, 0), (False, 3))):
        rgb = np.zeros((h, w, 3))
        rgb[:, :] = (16, 40, 22)
        screen_text(rgb, 37, 26, cursor_on, scroll)
        rgb[:3, :] = rgb[-3:, :] = (120, 118, 112)
        rgb[:, :3] = rgb[:, -3:] = (120, 118, 112)
        save_texture('screen%d' % k, rgb)
    rgb = np.zeros((h, w, 3))
    rgb[:, :] = (22, 24, 26)
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy = 14, 17
    a = np.arctan2(yy - cy, xx - cx)
    r = np.hypot(yy - cy, xx - cx)
    for k in range(7):
        angle = k * 2 * np.pi / 7 + 0.4
        crack = (np.abs(((a - angle + np.pi) % (2 * np.pi)) - np.pi) * r < 0.9) & (r > 1) & (r < 9 + 4 * ((k * 5) % 3))
        rgb[crack] = (150, 156, 160)
    rgb[r < 2.2] = (200, 204, 206)
    rgb[:3, :] = rgb[-3:, :] = (120, 118, 112)
    rgb[:, :3] = rgb[:, -3:] = (120, 118, 112)
    save_texture('screenx', rgb)


def texture_glassware():
    """The glass of the test tubes and the flasks (8 x 16: a pale glass with a liquid in the lower part: green, red,
    blue; the empty glass), drawn in screen doors in the game"""
    w, h = 8, 16
    for name, color, level in (('tubeg', (110, 220, 120), 9), ('tuber', (220, 90, 80), 7), ('tubeb', (100, 150, 230), 10),
                               ('tubee', None, 0), ('flaskg', (120, 230, 130), 10), ('flaskb', (110, 160, 235), 8)):
        rgb = np.zeros((h, w, 3))
        rgb[:, :] = (200, 220, 230)
        rgb[:, 0] = rgb[:, -1] = (150, 170, 185)
        rgb[1:h - 1, 2:w - 2] += 12
        if color is not None:
            rgb[h - level:, 1:w - 1] = color
            rgb[h - level, 1:w - 1] = tuple(c * 0.8 + 50 for c in color)
        save_texture(name, rgb)


def texture_keyboard():
    """The keys of a keyboard seen from above: light keys, dark gaps, a space bar"""
    w, h = 32, 16
    t = np.full((h, w), 70.0)
    for row in range(4):
        y0 = 2 + row * 3
        for col in range(14):
            x0 = 1 + col * 2 + (row % 2)
            if x0 + 1 < w - 1:
                t[y0:y0 + 2, x0] = 205
    t[14:16, 9:22] = 205
    save_texture('keyboard', tint(t, (1.0, 0.98, 0.92)))


def texture_rifle_top():
    """The rifle seen from above (dropped on the floor: one quad, see build_rifle_floor()): the wooden stock, the black
    receiver, the grip and the curved magazine on its side, the wooden handguard, the barrel and its front sight; the rest
    transparent. Along the width: from the stock to the muzzle"""
    w, h = 64, 16
    rgb = np.zeros((h, w, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:h, 0:w]
    wood = tint(150.0 + noise(16, 4, 26).repeat(4, axis=1)[:h, :w] * 8 + np.sin(xx * 0.9) * 6, (0.86, 0.58, 0.33))
    metal = np.full((h, w, 3), 42.0) + noise(16, 4, 23).repeat(4, axis=1)[:h, :w, None] * 6
    # (the stock: wider at its butt)
    stock = (xx < 19) & (np.abs(yy - 7.5) < 2.2 + (19 - xx) * 0.06)
    receiver = (xx >= 18) & (xx < 37) & (yy >= 4) & (yy <= 10)
    grip = (xx >= 20) & (xx < 24) & (yy > 10) & (yy <= 13)
    magazine = (xx >= 29) & (xx < 37) & (yy > 10) & (yy <= 10 + (xx - 28) * 0.7)
    handguard = (xx >= 37) & (xx < 49) & (yy >= 5) & (yy <= 9)
    barrel = (xx >= 49) & (yy >= 7) & (yy <= 8)
    sight = (xx >= 59) & (xx < 61) & (yy >= 6) & (yy <= 9)
    rgb[stock | handguard] = wood[stock | handguard]
    for part in (receiver, grip, magazine, barrel, sight):
        rgb[part] = metal[part]
    # (a light edge on the side of the light)
    rgb[receiver & (yy == 4)] += 30
    save_texture('riflef', rgb, key_transparent=True)


def texture_mouse():
    """A mouse seen from above: beige plastic, its 2 buttons, rounded (the corners transparent)"""
    w, h = 8, 8
    rgb = np.zeros((h, w, 3))
    rgb[:, :] = KEY
    rgb[1:7, 1:7] = (200, 194, 180)
    rgb[0, 2:6] = rgb[7, 2:6] = (200, 194, 180)
    rgb[1:3, 3:5] = (120, 116, 108)
    save_texture('mouse', rgb)


def texture_barrel():
    """The explosive barrels: gray steel with brown rust running down, a rim at the top and the bottom,
    2 ridges; on some sides yellow and black hazard stripes near the top and the bottom, on the front and the back a red
    ring with a white skull and crossbones (drawn here). A side of the barrel: 2 bands (the top half, the bottom half: the
    textures *_t and *_b); the lid; the barrel burnt after the explosion"""
    w, h = 16, 64
    yy, xx = np.mgrid[0:h, 0:w]
    streaks = rng.normal(0, 1, w)[None, :].repeat(h, axis=0)
    base = np.full((h, w), 128.0) + streaks * 6 + noise(16, 4, 41).repeat(4, axis=0)[:h, :w] * 10
    # (the rims and the ridges: a light line over a dark line)
    for y in (2, 22, 41, 61):
        base[yy == y] += 34
        base[yy == y + 1] -= 46
    rgb = tint(np.clip(base, 0, 255), (0.86, 0.84, 0.82))
    # (the rust: brown, running down from the ridges)
    rust = (noise(16, 4, 42).repeat(4, axis=0)[:h, :w] + 0.25 * np.sin(xx * 2.1 + streaks)) > 0.62
    rgb[rust] = rgb[rust] * 0.55 + np.array((112, 64, 34)) * 0.45

    def hazard(img, y0, y1):
        # (stripes at 45 degrees on the middle of the side)
        zone = (yy >= y0) & (yy < y1) & (xx >= 3) & (xx < 13)
        yellow = ((xx + yy) // 3) % 2 == 0
        img[zone & yellow] = (226, 186, 24)
        img[zone & ~yellow] = (26, 24, 20)

    def skull(img):
        cx, cy = 7.5, 31.5
        d = np.hypot(xx - cx, yy - cy)
        img[d < 7.6] = (196, 30, 26)
        img[d < 6.0] = (236, 232, 224)
        # (the crossbones: 2 diagonals under the skull, then the skull: the head, the jaw, the eyes)
        bones = (np.abs((xx - cx) - (yy - cy - 1.5)) < 0.8) | (np.abs((xx - cx) + (yy - cy - 1.5)) < 0.8)
        img[bones & (d < 5.2) & (yy > cy - 1)] = (30, 28, 26)
        head = np.hypot((xx - cx) / 1.0, (yy - (cy - 1.5)) / 1.05) < 2.8
        jaw = (np.abs(xx - cx) < 1.6) & (yy >= cy + 0.5) & (yy < cy + 2.5)
        img[head | jaw] = (30, 28, 26)
        eyes = (np.hypot(xx - (cx - 1.1), yy - (cy - 1.5)) < 0.8) | (np.hypot(xx - (cx + 1.1), yy - (cy - 1.5)) < 0.8)
        img[eyes] = (236, 232, 224)

    plain = rgb.copy()
    stripes = rgb.copy()
    hazard(stripes, 8, 16)
    hazard(stripes, 48, 56)
    front = stripes.copy()
    skull(front)
    for name, img in (('barrel', plain), ('barrelh', stripes), ('barrels', front)):
        save_texture(name + '_t', img[:h // 2].copy())
        save_texture(name + '_b', img[h // 2:].copy())
    n = 16
    yy2, xx2 = np.mgrid[0:n, 0:n]
    t = np.full((n, n), 118.0) + noise(n, 4, 43) * 8
    t[np.abs(np.hypot(xx2 - 7.5, yy2 - 7.5) - 6.5) < 0.8] -= 40
    t[(np.hypot(xx2 - 11, yy2 - 5) < 1.6)] = 50
    save_texture('barrelt', tint(t, (0.86, 0.84, 0.82)))
    t = np.full((h // 2, w), 40.0) + noise(16, 4, 44).repeat(2, axis=0)[:h // 2, :w] * 20
    save_texture('burnt', tint(np.clip(t, 0, 255), (1.0, 0.75, 0.6)))


def texture_grate():
    """The metal grating of the walkway of the bottling room: a grid of dark holes in gray steel"""
    n = 32
    yy, xx = np.mgrid[0:n, 0:n]
    t = np.full((n, n), 128.0) + noise(n, 8, 32) * 8
    holes = ((xx % 8) >= 2) & ((yy % 8) >= 2)
    t[holes] = 42
    t[((xx % 8) == 1) | ((yy % 8) == 1)] -= 18
    save_texture('grate', tint(t, (0.92, 0.95, 1.0)))


def texture_tank():
    """The tanks of the bottling room: light painted steel, almost white, soft vertical streaks, a band
    of rivets at the top and at the bottom"""
    w, h = 32, 64
    yy, xx = np.mgrid[0:h, 0:w]
    t = np.full((h, w), 222.0) + rng.normal(0, 3, w)[None, :].repeat(h, axis=0) + noise(32, 8, 33).repeat(2, axis=0)[:h, :w] * 4
    t[(yy == 8) | (yy == 56)] -= 40
    t[((yy == 6) | (yy == 58)) & (xx % 6 == 2)] -= 55
    save_texture('tank', tint(np.clip(t, 0, 255), (0.97, 0.99, 1.0)))


def texture_light():
    """A ceiling light: a white panel (the glow: the color of the vertices)"""
    n = 32
    yy, xx = np.mgrid[0:n, 0:n]
    t = np.full((n, n), 240.0)
    t[(xx < 3) | (xx >= n - 3) | (yy < 3) | (yy >= n - 3)] = 150
    save_texture('light', tint(t, (1.0, 1.0, 0.92)))


def texture_gun():
    """The pistol: dark gun metal (the colors of the vertices give the shape)"""
    n = 32
    t = np.full((n, n), 120.0) + noise(n, 4, 23) * 6
    save_texture('gun', gray(t))


def texture_wood():
    """The rifle: light orange wood (the stock, the grip, the handguard), its grain along it"""
    n = 16
    yy, xx = np.mgrid[0:n, 0:n]
    t = np.full((n, n), 150.0) + noise(n, 4, 26) * 6 + np.sin(xx * 1.3 + noise(n, 4, 27) * 2) * 10
    save_texture('kwood', tint(t, (0.86, 0.58, 0.33)))


def texture_skin():
    n = 16
    t = np.full((n, n), 200.0) + noise(n, 4, 24) * 6
    save_texture('skin', tint(t, (1.0, 0.80, 0.66)))


def all_textures():
    texture_tiles()
    texture_wood()
    texture_stone()
    texture_porcelain()
    texture_sign_nosmoking()
    texture_sign_tap()
    texture_switch()
    texture_water()
    texture_drain()
    texture_urinal()
    texture_floor()
    texture_panel()
    texture_white()
    texture_hdoor()
    texture_concrete()
    texture_ceiling()
    texture_duct()
    texture_frame()
    texture_crate()
    texture_hazard()
    texture_sign_exit()
    texture_sign_biohazard()
    texture_locker()
    texture_lockerside()
    texture_light()
    texture_bdoor()
    texture_grate()
    texture_tank()
    texture_glass()
    texture_sdoor()
    texture_casing()
    texture_lamp_cells()
    texture_barrel()
    texture_mouse()
    texture_rifle_top()
    texture_screen()
    texture_screen_frames()
    texture_glassware()
    texture_keyboard()
    texture_gun()
    texture_skin()


# ---------------------------------------------------------------------------
# Meshes
# ---------------------------------------------------------------------------
class Mesh:
    """Quads with a texture and a color per corner (Saturn axes). A quad: 4 corners clockwise seen from its front, the first
    one gets the top left corner of the texture"""

    def __init__(self):
        self.quads = []         # (corners (4 x (x, y, z)), texture, options dict, colors (4 x (r, g, b)) or None)
        self.decals = []        # (corners, texture): painted into the textures of the quads under them (apply_decals())
        self.attached = []      # (x0, x1, y0, y1, z0, z1): the objects against the walls (fit_walls())

    def quad(self, corners, tex, **options):
        self.quads.append([list(map(tuple, corners)), tex, options, None])

    def attach(self, x0, y0, z0, x1, y1, z1):
        """An object against a wall (its box): the cells of the wall under it are cut on its edges (see fit_walls())"""
        self.attached.append((min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1), min(z0, z1), max(z0, z1)))

    def decal(self, corners, tex):
        """A sign, a plate, a hole... on a surface of the mesh: 4 corners like a quad (clockwise seen from the front, the
        first one gets the top left corner of the texture), on the plane of a quad of the mesh (3 units at most from it).
        The VDP1 has no depth buffer: a decal drawn as its own polygon over a wall is sorted against the wall (and against
        the parts of the wall cut near the camera) and disappears or cuts the wall. Here it is painted into a copy of the
        texture of each quad it lies on: no polygon, nothing to sort"""
        self.decals.append((list(map(tuple, corners)), tex))

    def split_face(self, corners, tex, **options):
        """A rectangular face cut on the grid of the world (multiples of GRID) when it is bigger than GRID: the painter's
        sort (the center of each polygon) works when the polygons have the size of the cells of the walls; a big face in
        front of a wall (the lockers) has its center farther than the cells of the wall near the camera: the wall was
        drawn over it"""
        c = [np.array(v, dtype=float) for v in corners]
        u, v = c[1] - c[0], c[3] - c[0]

        def cuts(start, vec):
            axis = int(np.argmax(np.abs(vec)))
            a, b = start[axis], start[axis] + vec[axis]
            length = abs(b - a)
            if length <= GRID + 0.01:
                return [0.0, 1.0]
            lo, hi = min(a, b), max(a, b)
            ts = [0.0, 1.0]
            g = math.floor(lo / GRID + 1) * GRID
            while g < hi - 0.01:
                if g > lo + 0.01:
                    ts.append((g - a) / (b - a))
                g += GRID
            return sorted(ts)

        tu, tv = cuts(c[0], u), cuts(c[0], v)
        for j in range(len(tv) - 1):
            for i in range(len(tu) - 1):
                def p(s, t):
                    return tuple(c[0] + u * s + v * t)
                self.quad([p(tu[i], tv[j]), p(tu[i + 1], tv[j]), p(tu[i + 1], tv[j + 1]), p(tu[i], tv[j + 1])], tex, **options)

    def wall(self, a, b, y_top, y_bottom, tex, period=GRID, **options):
        """A vertical wall from a (x, z) to b (x, z) seen from the side where a is on the RIGHT (the wall turns clockwise
        around the room seen from above: the inside faces), cut in cells of period x period (the texture repeated), from
        y_top (smaller Y: up) to y_bottom"""
        bx, bz = a
        ax, az = b
        length = math.hypot(bx - ax, bz - az)
        n = max(1, int(round(length / period)))
        rows = max(1, int(round((y_bottom - y_top) / period)))
        for i in range(n):
            x0, z0 = ax + (bx - ax) * i / n, az + (bz - az) * i / n
            x1, z1 = ax + (bx - ax) * (i + 1) / n, az + (bz - az) * (i + 1) / n
            for r in range(rows):
                yt = y_top + (y_bottom - y_top) * r / rows
                yb = y_top + (y_bottom - y_top) * (r + 1) / rows
                self.quad([(x0, yt, z0), (x1, yt, z1), (x1, yb, z1), (x0, yb, z0)], tex, **options)

    def floor(self, x0, z0, x1, z1, y, tex, period=GRID, up=True, **options):
        """A horizontal surface (up: seen from above, a floor; else a ceiling) cut in cells"""
        nx = max(1, int(round((x1 - x0) / period)))
        nz = max(1, int(round((z1 - z0) / period)))
        for j in range(nz):
            for i in range(nx):
                a, b = x0 + (x1 - x0) * i / nx, x0 + (x1 - x0) * (i + 1) / nx
                c, d = z0 + (z1 - z0) * j / nz, z0 + (z1 - z0) * (j + 1) / nz
                if up:
                    # Seen from above (Y down: the camera is at a smaller Y): clockwise seen from above
                    self.quad([(a, y, d), (b, y, d), (b, y, c), (a, y, c)], tex, **options)
                else:
                    # Seen from below: the opposite order
                    self.quad([(a, y, c), (b, y, c), (b, y, d), (a, y, d)], tex, **options)

    def box(self, x0, y0, z0, x1, y1, z1, tex, faces='xXyYzZ', **options):
        """A box seen from outside (y0 < y1: y0 is the top). faces: x (the side at x0), X (at x1), y (top), Y (bottom),
        z (at z0), Z (at z1)"""
        # (split=False: a model drawn as one object, the texture on each whole face: the doors)
        q = self.split_face if options.pop('split', True) else self.quad
        if options.get('sort') == AGAINST_WALL:
            self.attach(x0, y0, z0, x1, y1, z1)
        if 'x' in faces:
            q([(x0, y0, z1), (x0, y0, z0), (x0, y1, z0), (x0, y1, z1)], tex, **options)
        if 'X' in faces:
            q([(x1, y0, z0), (x1, y0, z1), (x1, y1, z1), (x1, y1, z0)], tex, **options)
        if 'z' in faces:
            q([(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)], tex, **options)
        if 'Z' in faces:
            q([(x1, y0, z1), (x0, y0, z1), (x0, y1, z1), (x1, y1, z1)], tex, **options)
        if 'y' in faces:
            q([(x0, y0, z1), (x1, y0, z1), (x1, y0, z0), (x0, y0, z0)], tex, **options)
        if 'Y' in faces:
            q([(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)], tex, **options)

    def extend(self, other):
        self.quads.extend(other.quads)
        self.decals.extend(other.decals)
        self.attached.extend(other.attached)


def wall_axis(corners):
    """A vertical quad on a plane x = constant (0) or z = constant (2), its corners along the other axis; None: not a wall"""
    xs = {round(c[0], 2) for c in corners}
    zs = {round(c[2], 2) for c in corners}
    ys = {round(c[1], 2) for c in corners}
    if len(ys) < 2:
        return None
    if len(xs) == 1 and len(zs) > 1:
        return 0
    if len(zs) == 1 and len(xs) > 1:
        return 2
    return None


def fit_walls(mesh, name):
    """The rule of the objects against the walls: a cell of a wall behind an object has its center behind it.
    The VDP1 has no depth buffer: the polygons are drawn from the farthest (their sort key). An object against a wall is
    sorted with its nearest point (AGAINST_WALL): nearer than any point of the wall behind it. But a cell of the wall that
    goes past its side has its center beside the object: seen at a low angle, that center is nearer than the object and
    the cell is drawn over it (the basins and the urinals on 2 cells, the inside of the urinals). A cell of the wall under
    an object (Mesh.attach(), the boxes with sort=AGAINST_WALL) with its center beside it is cut on the edge of the object,
    along the wall: each part gets the part of the texture (a copy, the same parts are shared), the colors of its corners
    are interpolated. A cell centered behind the object stays whole (an object in the middle of a cell costs nothing;
    across 2 cells: 2 copies of textures per row of cells)"""
    def crop(tex, a, b):
        base = TEXTURES[tex]
        h, w = base.shape[:2]
        c0 = int(round(a * w))
        c1 = max(c0 + 1, int(round(b * w)))
        new = '%s_u%d_%d' % (tex, c0, c1)
        if new not in TEXTURES:
            part = base[:, c0:c1]
            width = max(8, (c1 - c0 + 7) // 8 * 8)
            save_texture(new, np.asarray(Image.fromarray(part.astype(np.uint8), 'RGB').resize((width, h), Image.NEAREST)))
        return new

    changed = True
    while changed:
        changed = False
        out = []
        for q in mesh.quads:
            corners, tex, options, colors = q
            axis = wall_axis(corners)
            cuts = []
            if axis is not None and tex is not None and options.get('sort') != AGAINST_WALL:
                along = 2 if axis == 0 else 0
                plane = corners[0][axis]
                c0, c1 = np.array(corners[0], dtype=float), np.array(corners[1], dtype=float)
                lo, hi = sorted((c0[along], c1[along]))
                ytop, ybottom = min(c[1] for c in corners), max(c[1] for c in corners)
                for box in mesh.attached:
                    b0, b1 = (box[0], box[1]) if axis == 0 else (box[4], box[5])
                    s0, s1 = (box[4], box[5]) if axis == 0 else (box[0], box[1])
                    # (the object on this plane, beside the quad's span and its height)
                    if not (b0 - 0.5 <= plane <= b1 + 0.5) or box[3] <= ytop + 0.5 or box[2] >= ybottom - 0.5 or s1 <= lo + 0.5 or s0 >= hi - 0.5:
                        continue
                    # (a cell centered behind the object is safe: its center is farther than the nearest point of the object)
                    if s0 - 0.5 <= (lo + hi) / 2 <= s1 + 0.5:
                        continue
                    for cut in (s0, s1):
                        if lo + 0.5 < cut < hi - 0.5:
                            t = (cut - c0[along]) / (c1[along] - c0[along])
                            cuts.append(t)
            if not cuts:
                out.append(q)
                continue
            changed = True
            ts = [0.0] + sorted(set(round(t, 6) for t in cuts)) + [1.0]
            p = [np.array(c, dtype=float) for c in corners]
            for a, b in zip(ts, ts[1:]):
                pc = [p[0] + (p[1] - p[0]) * a, p[0] + (p[1] - p[0]) * b, p[3] + (p[2] - p[3]) * b, p[3] + (p[2] - p[3]) * a]
                pcol = None
                if colors:
                    col = [np.array(c, dtype=float) for c in colors]
                    pcol = [tuple(col[0] + (col[1] - col[0]) * a), tuple(col[0] + (col[1] - col[0]) * b),
                            tuple(col[3] + (col[2] - col[3]) * b), tuple(col[3] + (col[2] - col[3]) * a)]
                out.append([[tuple(float(x) for x in v) for v in pc], crop(tex, a, b), dict(options), pcol])
        mesh.quads = out
    # The check: no cell of a wall goes past the side of an object (the build stops: the wall would be drawn over it)
    for corners, tex, options, colors in mesh.quads:
        axis = wall_axis(corners)
        if axis is None or tex is None or options.get('sort') == AGAINST_WALL:
            continue
        along = 2 if axis == 0 else 0
        plane = corners[0][axis]
        lo, hi = min(c[along] for c in corners), max(c[along] for c in corners)
        ytop, ybottom = min(c[1] for c in corners), max(c[1] for c in corners)
        for box in mesh.attached:
            b0, b1 = (box[0], box[1]) if axis == 0 else (box[4], box[5])
            s0, s1 = (box[4], box[5]) if axis == 0 else (box[0], box[1])
            if not (b0 - 0.5 <= plane <= b1 + 0.5) or box[3] <= ytop + 0.5 or box[2] >= ybottom - 0.5 or s1 <= lo + 0.5 or s0 >= hi - 0.5:
                continue
            if not (s0 - 0.5 <= (lo + hi) / 2 <= s1 + 0.5):
                raise SystemExit('%s: the cell of the wall at %s is behind the object %s, its center beside it: it would be drawn '
                                 'over it' % (name, corners, box))


def apply_decals(mesh, name):
    """Paint the decals of a mesh into copies of the textures of the quads under them (see Mesh.decal()). Each quad
    touched gets its own texture, in 16 colors like the one it replaces (the converter picks the 14 colors of the copy:
    the colors of the sign are kept; in RGB the copies filled the VRAM of the VDP1)"""
    count = 0
    # (the copies with the same texels are shared: the same object on the same cell of a wall, a basin beside another one)
    copies = {}
    for corners, dtex in mesh.decals:
        d0 = [np.array(v, dtype=float) for v in corners]
        dn = quad_normal(corners)
        decal = TEXTURES[dtex]
        dh, dw = decal.shape[:2]
        hit = 0
        for q in mesh.quads:
            qc = [np.array(v, dtype=float) for v in q[0]]
            if q[1] is None or float(np.dot(quad_normal(q[0]), dn)) < 0.98 or abs(float(np.dot(d0[0] - qc[0], dn))) > 3:
                continue
            base = TEXTURES[q[1]]
            th, tw = base.shape[:2]
            # Texels of the quad inside the decal (the quads are rectangles: bilinear = affine)
            u, v = qc[1] - qc[0], qc[3] - qc[0]
            # (the decal moved to the nearest texel of this quad, at most half a texel: the same decal at the same place
            # of 2 copies of a quad paints the same texels, the copies are shared)
            rel0 = d0[0] - qc[0]
            fs = round(float(np.dot(rel0, u) / np.dot(u, u)) * tw) / tw - float(np.dot(rel0, u) / np.dot(u, u))
            ft = round(float(np.dot(rel0, v) / np.dot(v, v)) * th) / th - float(np.dot(rel0, v) / np.dot(v, v))
            d = [c + u * fs + v * ft for c in d0]
            e1, e2 = d[1] - d[0], d[3] - d[0]
            ss, tt = np.meshgrid((np.arange(tw) + 0.5) / tw, (np.arange(th) + 0.5) / th)
            pts = qc[0][None, None, :] + ss[..., None] * u[None, None, :] + tt[..., None] * v[None, None, :]
            rel = pts - d[0][None, None, :]
            du = rel @ e1 / float(np.dot(e1, e1))
            dv = rel @ e2 / float(np.dot(e2, e2))
            inside = (du >= -0.05) & (du < 1.05) & (dv >= -0.05) & (dv < 1.05)
            if not inside.any():
                continue
            # 4 x 4 samples in each texel (the decal is finer than the texture under it: no pixelated edges), each one
            # bilinear in the decal; the transparent parts of the decal let the texture through
            out = base.astype(float).copy()
            acc = np.zeros(base.shape[:2] + (3,))
            cover = np.zeros(base.shape[:2])
            n = 4
            for sj in range(n):
                for si in range(n):
                    p = qc[0][None, None, :] + (ss + (si + 0.5) / n / tw - 0.5 / tw)[..., None] * u[None, None, :] + \
                        (tt + (sj + 0.5) / n / th - 0.5 / th)[..., None] * v[None, None, :]
                    r = p - d[0][None, None, :]
                    su = r @ e1 / float(np.dot(e1, e1))
                    sv = r @ e2 / float(np.dot(e2, e2))
                    ok = (su >= 0) & (su < 1) & (sv >= 0) & (sv < 1)
                    fx = np.clip(su * dw - 0.5, 0, dw - 1)
                    fy = np.clip(sv * dh - 0.5, 0, dh - 1)
                    x0, y0 = np.floor(fx).astype(int), np.floor(fy).astype(int)
                    x1, y1 = np.minimum(x0 + 1, dw - 1), np.minimum(y0 + 1, dh - 1)
                    ax, ay = (fx - x0)[..., None], (fy - y0)[..., None]
                    col = (decal[y0, x0] * (1 - ax) + decal[y0, x1] * ax) * (1 - ay) + \
                          (decal[y1, x0] * (1 - ax) + decal[y1, x1] * ax) * ay
                    key = np.all(decal[np.clip(np.rint(fy).astype(int), 0, dh - 1), np.clip(np.rint(fx).astype(int), 0, dw - 1)]
                                 == np.array(KEY), axis=-1)
                    ok &= ~key
                    acc[ok] += col[ok]
                    cover[ok] += 1
            c = cover[..., None] / (n * n)
            painted = cover > 0
            out[painted] = (out * (1 - c) + acc / np.maximum(cover, 1)[..., None] * c)[painted]
            key = (q[1], np.clip(out, 0, 255).astype(np.uint8).tobytes())
            if key not in copies:
                copies[key] = '%s_%s%d' % (name, q[1], count)
                count += 1
                save_texture(copies[key], out)
            q[1] = copies[key]
            hit += 1
        if hit == 0:
            raise SystemExit('the decal %s of %s is on no quad: %s' % (dtex, name, corners))


def check_no_overlap(mesh, name):
    """Stop the build on 2 quads of a mesh on the same plane that overlap (the VDP1 has no depth buffer: they flicker,
    one cuts the other). Decals are not polygons (see Mesh.decal())"""
    # (the rooms, with their floors: 3 units; the small models (the weapons, at their own scale): the same plane only)
    near = 3 if any(q[2].get('sort') == 'max' for q in mesh.quads) else 0.05
    info = []
    for k, q in enumerate(mesh.quads):
        c = np.array(q[0], dtype=float)
        n = quad_normal(q[0])
        info.append((n, float(np.dot(n, c[0])), c))
    for i in range(len(info)):
        ni, di, ci = info[i]
        for j in range(i + 1, len(info)):
            nj, dj, cj = info[j]
            # (2 quads facing the same way less than 3 units apart: the one in front of the other is sorted against it
            # and against its parts cut near the camera: it is cut, it flickers; a light panel under the ceiling lost a
            # corner. Paint it into the texture (a decal) or make it the quad itself)
            if float(np.dot(ni, nj)) < 0.999 or abs(di - dj) > near:
                continue
            # (the furniture against a wall: sorted with its nearest point, the cells of the wall cut on its edges, see
            # fit_walls(); only vertical: a light panel sorted so under the ceiling still lost a corner)
            if abs(di - dj) > 0.05 and abs(ni[1]) < 0.01 and AGAINST_WALL in (mesh.quads[i][2].get('sort'), mesh.quads[j][2].get('sort')):
                continue
            # The same plane: do the 2 convex quads overlap (in 2D, separating axes; touching edges don't count)?
            axis = int(np.argmax(np.abs(ni)))
            pi, pj = np.delete(ci, axis, axis=1), np.delete(cj, axis, axis=1)
            separated = False
            for poly in (pi, pj):
                for k in range(4):
                    e = poly[(k + 1) % 4] - poly[k]
                    if np.hypot(*e) < 1e-6:
                        continue
                    nrm = np.array((-e[1], e[0])) / np.hypot(*e)
                    a, b = pi @ nrm, pj @ nrm
                    if a.max() <= b.min() + 0.05 or b.max() <= a.min() + 0.05:
                        separated = True
                        break
                if separated:
                    break
            if not separated:
                raise SystemExit('%s: the quads %d and %d overlap on the same plane (or less than 3 units apart): %s / %s' % (name, i, j, mesh.quads[i][0],
                                                                                                 mesh.quads[j][0]))


def quad_normal(c):
    """Normal of a quad toward its front (clockwise seen from the front, Saturn axes)"""
    a, b, d = np.array(c[0]), np.array(c[1]), np.array(c[3])
    n = np.cross(d - a, b - a)
    length = np.linalg.norm(n)
    return n / length if length > 1e-9 else np.array((0.0, -1.0, 0.0))


# Lamps: (x, y, z, radius, (r, g, b)) in world units, the light falls to half at the radius
# The halo of the lamps on the ceilings (see bake_light()): its strength (times the color of the lamp), the lamps at most
# this far under the ceiling
CEILING_HALO = 0.9
CEILING_HALO_DEPTH = 80


def bake_light(mesh, lamps, ambient=(0.12, 0.12, 0.14), occluders=()):
    """The light of the lamps on the corners of the quads: colors of the vertices (0.5 = the texture as it is).
    occluders: boxes (x0, y0, z0, x1, y1, z1) that stop the light (the furniture: a soft shadow under it)"""
    for q in mesh.quads:
        corners, tex, options, _ = q
        if options.get('unlit'):
            q[3] = [options['unlit']] * 4
            continue
        n = quad_normal(corners)
        # A ceiling (facing down: Saturn Y): the halo of its lamps (the panel of the lamp lights the
        # ceiling around it, a gradient over about 2 cells of the ceiling; the grazing light alone left it dark around
        # the bright panel). The size of a cell: the quad
        ceiling = n[1] > 0.7
        cell = float(np.ptp(np.array(corners, dtype=float)[:, [0, 2]], axis=0).max()) if ceiling else 0.0
        cols = []
        for c in corners:
            p = np.array(c, dtype=float)
            # (a little in front of the surface: its own plane doesn't shadow it)
            p -= n * 2
            color = np.array(ambient, dtype=float)
            for lx, ly, lz, radius, lc in lamps:
                d = np.array((lx, ly, lz)) - p
                dist = np.linalg.norm(d)
                if dist < 1e-6:
                    facing = 1.0
                else:
                    facing = max(0.15, float(np.dot(d / dist, n)))
                    # (the lamps on the ceiling light the ceiling around them a little: grazing light)
                fall = 1.0 / (1.0 + (dist / radius) ** 2)
                shadow = 1.0
                for bx0, by0, bz0, bx1, by1, bz1 in occluders:
                    if segment_hits_box(p, np.array((lx, ly, lz)), (bx0, by0, bz0, bx1, by1, bz1)):
                        shadow = 0.35
                        break
                color += np.array(lc) * fall * facing * shadow
                if ceiling and 0 <= ly - p[1] < CEILING_HALO_DEPTH and cell > 0:
                    # (1 at the corners of the cell of the lamp, 0 two cells farther. The red alarms: a softer and wider
                    # halo, 0 four cells farther, the light of the dome all around: a halo of 2 cells on the narrow
                    # gallery was a dark ceiling with a red diamond in it)
                    flat = math.hypot(p[0] - lx, p[2] - lz)
                    if lc == ALARM_BAKED:
                        halo = min(1.0, max(0.0, 1.0 - (flat - cell * 0.75) / (cell * 3.25)))
                        color += np.array(lc) * CEILING_HALO * halo
                    else:
                        halo = min(1.0, max(0.0, 1.0 - (flat - cell * 0.75) / (cell * 1.25)))
                        color += np.array(lc) * CEILING_HALO * halo ** 1.5
            cols.append(tuple(float(min(0.62, v)) for v in color))
        q[3] = cols
    check_ceiling_halos(mesh, lamps, ambient)


def check_ceiling_halos(mesh, lamps, ambient):
    """Stop the build on a lamp without its halo on the ceiling (see bake_light()): the corners of the ceiling around a lamp
    (the ones of the cell of its panel) must be lit by it. (A sign of Y turned around left the ceilings dark around the
    bright panels)"""
    for lx, ly, lz, radius, lc in lamps:
        near = []
        for corners, tex, options, cols in mesh.quads:
            if options.get('unlit') or quad_normal(corners)[1] <= 0.7:
                continue
            cell = float(np.ptp(np.array(corners, dtype=float)[:, [0, 2]], axis=0).max())
            for c, col in zip(corners, cols):
                if 0 <= ly - c[1] < CEILING_HALO_DEPTH and math.hypot(c[0] - lx, c[2] - lz) <= cell * 0.75:
                    near.append(float(np.mean(col)))
        if not near:
            continue
        expected = float(np.mean(ambient)) + 0.5 * CEILING_HALO * float(np.mean(lc))
        if max(near) < min(expected, 0.6):
            raise SystemExit('check_ceiling_halos: the lamp at (%d, %d, %d) has no halo on the ceiling (%.2f, at least %.2f)'
                             % (lx, ly, lz, max(near), min(expected, 0.6)))


def segment_hits_box(a, b, box):
    """Check if the segment a-b goes through the box (slab test)"""
    x0, y0, z0, x1, y1, z1 = box
    tmin, tmax = 0.0, 1.0
    for k, (lo, hi) in enumerate(((x0, x1), (y0, y1), (z0, z1))):
        d = b[k] - a[k]
        if abs(d) < 1e-9:
            if a[k] < lo or a[k] > hi:
                return False
            continue
        t1 = (lo - a[k]) / d
        t2 = (hi - a[k]) / d
        if t1 > t2:
            t1, t2 = t2, t1
        tmin = max(tmin, t1)
        tmax = min(tmax, t2)
        if tmin > tmax:
            return False
    return True


def check_on_top(mesh, name):
    """Stop the build on a thing put on a piece of furniture (its bottom on the top of the furniture, inside it) that is not
    sorted with its nearest point (sort='min'). The VDP1 has no depth buffer: the top of the desk, sorted with its center,
    was drawn over the computers on it (their faces disappeared). The floors (sort='max') are drawn first: no rule"""
    # (the rooms: the models with a floor; the weapons and the doors are models of their own)
    if not any(options.get('sort') == 'max' for corners, tex, options, colors in mesh.quads):
        return
    tops = []
    for corners, tex, options, colors in mesh.quads:
        n = quad_normal(corners)
        if n[1] < -0.99 and options.get('sort') != 'max':
            c = np.array(corners, dtype=float)
            tops.append((c[0][1], c[:, 0].min(), c[:, 0].max(), c[:, 2].min(), c[:, 2].max()))
    for corners, tex, options, colors in mesh.quads:
        if options.get('sort') == 'min':
            continue
        c = np.array(corners, dtype=float)
        bottom = c[:, 1].max()
        if bottom - c[:, 1].min() < 0.5:
            # (a horizontal quad: a top, not a thing standing)
            continue
        cx, cz = c[:, 0].mean(), c[:, 2].mean()
        for y, x0, x1, z0, z1 in tops:
            if abs(bottom - y) < 0.5 and x0 + 0.5 < cx < x1 - 0.5 and z0 + 0.5 < cz < z1 - 0.5:
                raise SystemExit('%s: the quad %s stands on a piece of furniture: sort it with its nearest point (sort=\'min\')'
                                 % (name, corners))


def check_overhangs(mesh, name):
    """Stop the build on a side of furniture under the top of another piece (a top wider than the body under it): the
    VDP1 has no depth buffer, the side, sorted with its center, was drawn over the top near the camera (the benches of
    the laboratory). Make the piece one box. The things sorted with their nearest point and the rooms are not checked"""
    if not any(options.get('sort') == 'max' for corners, tex, options, colors in mesh.quads):
        return
    tops = []
    for corners, tex, options, colors in mesh.quads:
        if quad_normal(corners)[1] < -0.99 and options.get('sort') not in ('max', 'min'):
            c = np.array(corners, dtype=float)
            tops.append((c[0][1], c[:, 0].min(), c[:, 0].max(), c[:, 2].min(), c[:, 2].max()))
    for corners, tex, options, colors in mesh.quads:
        if options.get('sort') in ('max', 'min') or abs(quad_normal(corners)[1]) > 0.01:
            continue
        c = np.array(corners, dtype=float)
        top = c[:, 1].min()
        x0, x1, z0, z1 = c[:, 0].min(), c[:, 0].max(), c[:, 2].min(), c[:, 2].max()
        for y, tx0, tx1, tz0, tz1 in tops:
            # (the side starts under the top, and it is inside its footprint, not on its edge)
            if y + 0.5 < top < y + 20 and tx0 + 0.5 < x0 and x1 < tx1 - 0.5 and tz0 + 0.5 < z0 and z1 < tz1 - 0.5:
                raise SystemExit('%s: the quad %s is under the top of a piece of furniture (an overhang): make it one box'
                                 % (name, corners))


def write_model(mesh, name, description, lut=True):
    """Write models/name.obj + .mtl and convert them to cd/NAME.JOM (textures embedded)"""
    check_on_top(mesh, name)
    check_overhangs(mesh, name)
    fit_walls(mesh, name)
    apply_decals(mesh, name)
    check_no_overlap(mesh, name)
    obj = ['# %s: generated by tools/make_assets.py (Saturn axes), converted to cd/%s.JOM' % (description, name.upper()),
           'mtllib %s.mtl' % name, '', 'vt 0 1', 'vt 1 1', 'vt 1 0', 'vt 0 0', '']
    mats = {}
    index = {}
    body = []
    current = None
    for corners, tex, options, colors in mesh.quads:
        key = (tex, tuple(sorted((k, str(v)) for k, v in options.items() if k != 'unlit')))
        if key not in mats:
            mats[key] = 'm%d' % len(mats)
        mat = mats[key]
        ids = []
        for k, c in enumerate(corners):
            col = colors[k] if colors else (0.5, 0.5, 0.5)
            vkey = (round(c[0], 2), round(c[1], 2), round(c[2], 2), round(col[0], 3), round(col[1], 3), round(col[2], 3))
            if vkey not in index:
                index[vkey] = len(index) + 1
                obj.append('v %.2f %.2f %.2f %.3f %.3f %.3f' % vkey)
            ids.append(index[vkey])
        if mat != current:
            body.append('usemtl ' + mat)
            current = mat
        body.append('f %d/1 %d/2 %d/3 %d/4' % tuple(ids))
    with open(os.path.join(MODELS, name + '.obj'), 'w') as f:
        f.write('\n'.join(obj + body) + '\n')
    mtl = ['# Materials of %s' % name, '']
    for (tex, opts), mat in mats.items():
        options = dict((k, v) for k, v in opts)
        # (kd: the color of a quad without texture, 'r g b' from 0 to 1)
        mtl += ['newmtl ' + mat, 'Kd ' + options.get('kd', '1 1 1')]
        if tex is not None:
            mtl.append('map_Kd %s.png' % tex)
            mtl.append('jo_lut %d' % (1 if (lut and options.get('lut', 'True') != 'False') else 0))
        mtl.append('jo_gouraud 1')
        # (the rooms are cut in cells of GRID; near the camera the engine draws the quads in 2 x 2 or 4 x 4 parts, each with
        # its tile of the texture: the VDP1 maps a texture without perspective, a big quad near the camera is distorted)
        # The furniture and the small models (subdivision 0 in the code): their quads are only cut when they cross the near
        # plane (a quad that crosses it isn't drawn at all otherwise: the counter vanished when the agent stood against it)
        # The furniture and the models of the entities: only cut on the near plane (cut near the camera like the walls,
        # the washroom fell to 22 frames per second). The furniture against a wall is sorted with its nearest point
        # (sort='min' where it is made, see AGAINST_WALL): the parts of the wall behind it, cut near the camera, are
        # sorted with their centers, as near as the center of a face of the cupboard (the wall was drawn over it). The
        # furniture in the middle of the room (the partitions) keeps the center: sorted with its nearest point, a long
        # partition was drawn over the door of a stall in front of it
        sub = str(options.get('subdivision', '1'))
        if sub == '0':
            sub = '2'
        mtl.append('jo_subdivision %s' % sub)
        # (a pattern with straight lines: drawn in parts from a smaller distortion, else its lines bend where a part meets a
        # whole quad, see STRAIGHT_LINE_TEXTURES)
        if sub == '1' and tex in STRAIGHT_LINE_TEXTURES:
            mtl.append('jo_subdivision_error %d' % STRAIGHT_LINE_TEXTURES[tex])
        # (no smaller copies of the textures far away (mipmaps): tried on the 64 x 64 textures of the rooms, the VDP1 time
        # of Mednafen did not move (20.8 to 20.0 ms in the hall) while the CPUs spent 1.5 ms more choosing the copies)
        mtl.append('jo_mipmaps 0')
        if options.get('sort'):
            mtl.append('jo_sort ' + options['sort'])

        if options.get('dual') == 'True':
            mtl.append('jo_dual 1')
        if options.get('end_codes') == 'True':
            mtl.append('jo_end_codes 1')
        if options.get('half') == 'True':
            mtl.append('jo_half_transparent 1')
        if options.get('mesh') == 'True':
            mtl.append('jo_screen_doors 1')
        mtl.append('')
    with open(os.path.join(MODELS, name + '.mtl'), 'w') as f:
        f.write('\n'.join(mtl) + '\n')
    out = os.path.join(CD, name.upper() + '.JOM')
    cmd = [sys.executable, os.path.join(COMMON, 'jo_model.py'), os.path.join(MODELS, name + '.obj'), '-o', out,
           '--saturn-axes', '--embed-textures', '-q']
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout, r.stderr)
        raise SystemExit('conversion of %s failed' % name)
    print('  %s: %d quads, %d bytes' % (os.path.basename(out), len(mesh.quads), os.path.getsize(out)))


# ---------------------------------------------------------------------------
# The level
# ---------------------------------------------------------------------------
# Rooms: name -> (x0, z0, x1, z1): the box of the cell (world units, Y down, the floor at y = 0). All on the grid
H = WALL_HEIGHT
ROOMS = {
    # The washroom where the player lands (12 x 10 cells): stalls along the south wall, basins
    # along the north wall. Its only way out: the door to the corridor
    'washroom': (0, 0, 960, 800),
    # The tiled corridor east of the washroom (8 x 4 cells)
    'corridor': (960, 240, 1600, 560),
    # The storage room at the end of the corridor (10 x 8 cells)
    'storage': (1600, 160, 2400, 800),
    # The tiled hall going north from the corridor to the laboratory (2 x 10 cells)
    'hall': (1200, 560, 1360, 1360),
    # The laboratory of the scientists (10 x 7 cells): benches, a desk
    'lab': (960, 1360, 1760, 1920),
    # The room of the guards between the laboratory and the storage room (8 x 10 cells): lockers, crates, the switch of
    # the security door
    'guards': (1760, 800, 2400, 1600),
    # The office of the laboratory, seen from the hall through a long window (5 x 6 cells)
    'office': (1360, 880, 1760, 1360),
    # The concrete corridor east of the room of the guards, to the airlock of the bottling room (9 x 3 cells)
    'gallery': (2400, 1360, 3120, 1600),
    # The airlock before the walkway of the bottling room: 2 doors sliding to the left, in pockets of the walls (2 x 3 cells)
    'airlock': (3120, 1360, 3280, 1600),
    # The stairs from the storage room down to the lower floor (8 x 2 cells, along +X)
    'stairs': (2400, 320, 3040, 480),
    # The gray concrete hall of the lower floor (3 x 13 cells): brown doors
    'lowhall': (3040, 160, 3280, 1200),
    # The bottling room (12 x 12 cells, 2 floors high: the floor of the lower floor, the ceiling of the upper one): tanks
    'bottling': (3280, 400, 4240, 1360),
    # Along its north wall: the walkway at the level of the upper floor, the stairs down to its floor (along +X), the
    # landing at their bottom
    'walkway': (3280, 1360, 3680, 1600),
    'stairs2': (3680, 1360, 4080, 1600),
    'landing': (4080, 1360, 4240, 1600),
}
# The lower floor: 4.8 m under the upper one (Saturn Y: down); the floors and the ceilings of the rooms (default: 0, -H)
LOWER = 240
ROOM_FLOOR = {'lowhall': LOWER, 'bottling': LOWER, 'landing': LOWER}
ROOM_TOP = {'lowhall': LOWER - H, 'bottling': -H, 'walkway': -H, 'stairs2': -H, 'landing': -H}
# The stairs: (axis of the slope, floor at x0, floor at x1, flat landing at each end): 12 steps; the landings of the
# stairs a door opens on are half a step deep (the slope of the cell goes through the middle of the treads: it is the
# floor of the landing at the door, see stairs_slope())
STAIRS = {'stairs': ('x', 0, LOWER, 25), 'stairs2': ('x', 0, LOWER, 0)}
STAIRS_STEPS = 12
# The cells across the openings between the walkway, its stairs and the landing (the circle of the agent must be in a
# cell: from one box to the next, it is in this one): (cell a, cell b, x of the opening, half depth, floor)
LINKS = [('walkway', 'stairs2', 'x', 3680, 40, 0), ('stairs2', 'landing', 'x', 4080, 40, LOWER),
         ('landing', 'bottling', 'z', 1360, 40, LOWER)]
# Doors: (room a, room b, axis of the wall ('x': the wall is at x = pos), pos, c0, c1, floor, style): the opening from c0
# to c1 (not in a corner of a room: the frame of the door would be on the plane of the side wall). style: 'steel' (the
# hinged doors of the upper floor), 'brown' (the lower floor), 'slide' (the doors of the airlock: they slide to the left
# seen from room a, into a pocket of the wall), 'slide_hazard' (the same, yellow and black stripes around it)
DOORS = [
    ('washroom', 'corridor', 'x', 960, 350, 450, 0, 'steel'),
    ('corridor', 'storage', 'x', 1600, 350, 450, 0, 'steel'),
    ('corridor', 'hall', 'z', 560, 1230, 1330, 0, 'steel'),
    ('hall', 'lab', 'z', 1360, 1230, 1330, 0, 'steel'),
    ('lab', 'guards', 'x', 1760, 1440, 1540, 0, 'steel'),
    ('storage', 'guards', 'z', 800, 1900, 2000, 0, 'steel'),
    ('guards', 'gallery', 'x', 2400, 1390, 1490, 0, 'steel'),
    ('lab', 'office', 'z', 1360, 1560, 1660, 0, 'steel'),
    ('gallery', 'airlock', 'x', 3120, 1380, 1480, 0, 'slide'),
    ('airlock', 'walkway', 'x', 3280, 1380, 1480, 0, 'slide_hazard'),
    ('storage', 'stairs', 'x', 2400, 350, 450, 0, 'steel'),
    ('stairs', 'lowhall', 'x', 3040, 350, 450, LOWER, 'brown'),
    ('lowhall', 'bottling', 'x', 3280, 600, 700, LOWER, 'brown'),
]
# The openings between the parts of the bottling room (no door): (cell a, cell b, axis, pos, c0, c1, bottom, top)
OPENINGS = [
    ('walkway', 'bottling', 'z', 1360, 3280, 3680, LOWER, -H),
    ('stairs2', 'bottling', 'z', 1360, 3680, 4080, LOWER, -H),
    ('landing', 'bottling', 'z', 1360, 4080, 4240, LOWER, -H),
    ('walkway', 'stairs2', 'x', 3680, 1360, 1600, LOWER, -H),
    ('stairs2', 'landing', 'x', 4080, 1360, 1600, LOWER, -H),
]
# The windows (half transparent glass) between 2 rooms: (room a, room b, axis, pos, c0, c1, bottom, top): the hall sees
# the office of the laboratory
WINDOWS = [('hall', 'office', 'x', 1360, 960, 1280, -40, -140)]
# The tanks of the bottling room: (center x, center z), their radius, the bottom and the top of their body (Saturn Y)
TANKS = [(x, z) for z in (660, 1060) for x in (3540, 3800, 4040)]
TANK_RADIUS = 70
TANK_BOTTOM = LOWER - 50
TANK_TOP = LOWER - 300
RAIL_HEIGHT = 50
# The ventilation duct over the washroom: a cell of its own (x0, z0, x1, z1), its floor at DUCT_FLOOR (Y down: up)
DUCT = (110, -480, 190, 100)
DUCT_FLOOR = -H                 # The floor of the duct is the ceiling of the washroom
DUCT_TOP = DUCT_FLOOR - DUCT_SIZE
# The hole of the duct in the ceiling of the washroom: the end of the duct, over the second stall, a cell of its own (its
# floor is the one of the washroom: the agent falls when he enters it, in front of the toilet, behind the door; the camera
# falling in it still sees the end of the duct, its model)
DUCT_HOLE = (110, 40, 190, 100)
# The branch of the duct: on the right (+X) before the washroom, 240 long, a dead end (x0, z0, x1, z1)
DUCT_BRANCH = (190, -240, 430, -160)

# The stalls of the washroom (south wall): 5 stalls of 100 x 200 (room between the toilet and the door to close it), white partitions 4 units thick, 150 high, open toward
# the north with a teal hinged door (the hinge on the left post seen from the room, 20 units over the floor)
STALL_WIDTH = 100
STALL_DEPTH = 200
STALL_HEIGHT = 150
STALL_DOOR_BOTTOM = 20
# The hinge of a door of a stall: from the left side of the stall (the post is 2 wide), and the width of the panel
STALL_HINGE = 4
STALL_DOOR_WIDTH = STALL_WIDTH - STALL_HINGE - 2
STALLS = [(x, 0, x + STALL_WIDTH, STALL_DEPTH) for x in range(0, 5 * STALL_WIDTH, STALL_WIDTH)]
# The mirrors: (room, wall, from c0 to c1 along the wall (on the grid of the wall), top y, bottom y (Saturn Y)): a hole in the
# wall and a portal MIRROR of the cell, the engine draws the room again, reflected (see jo_cells_draw_mirror())
MIRRORS = []
# The sort of the furniture against the walls (see write_model()): its nearest point
AGAINST_WALL = 'min'
# Solid furniture for the collisions: (center x, center z, half x, half z), see build_level()
SOLIDS = []

# Lamps of the rooms: (x, y, z, radius, color)
# The rooms with red alarm lights: their lamps baked dark red, their panels
# red, a point light that pulses at each lamp (the class lamp with alarm=1, see doors.c)
ALARM_ROOMS = ('gallery',)
ALARM_BAKED = (0.30, 0.05, 0.04)
LAMPS = {
    'washroom': [(x, -170, z, 380, (0.50, 0.52, 0.55)) for x in (240, 720) for z in (260, 600)],
    'corridor': [(x, -170, 400, 300, (0.50, 0.48, 0.42)) for x in (1120, 1440)],
    'storage': [(x, -170, z, 400, (0.48, 0.48, 0.44)) for x in (1840, 2160) for z in (320, 640)],
    'hall': [(1280, -170, z, 280, (0.48, 0.50, 0.50)) for z in (760, 1160)],
    'lab': [(x, -170, z, 380, (0.52, 0.54, 0.56)) for x in (1160, 1560) for z in (1520, 1760)],
    'guards': [(x, -170, z, 380, (0.46, 0.46, 0.42)) for x in (1920, 2240) for z in (1000, 1400)],
    'office': [(x, -170, 1120, 380, (0.52, 0.54, 0.56)) for x in (1480, 1660)],
    'gallery': [(x, -170, 1480, 300, ALARM_BAKED) for x in (2600, 2960)],
    'airlock': [(3200, -170, 1480, 220, (0.50, 0.46, 0.30))],
    'stairs': [(2420, -170, 400, 260, (0.44, 0.44, 0.42)), (2980, LOWER - 170, 400, 260, (0.40, 0.40, 0.38))],
    'lowhall': [(3160, LOWER - 170, z, 300, (0.42, 0.42, 0.40)) for z in (340, 680, 1020)],
    'bottling': [(x, -170, z, 600, (0.44, 0.48, 0.46)) for x in (3520, 4000) for z in (640, 1120)],
}
LAMPS['walkway'] = LAMPS['stairs2'] = LAMPS['landing'] = LAMPS['bottling']
AMBIENT = {'washroom': (0.16, 0.17, 0.18), 'corridor': (0.10, 0.10, 0.11), 'storage': (0.11, 0.11, 0.12),
           'hall': (0.12, 0.12, 0.13), 'office': (0.15, 0.16, 0.17), 'airlock': (0.11, 0.11, 0.10), 'lab': (0.15, 0.16, 0.17), 'guards': (0.11, 0.11, 0.12), 'gallery': (0.10, 0.10, 0.11),
           'stairs': (0.09, 0.09, 0.10), 'lowhall': (0.09, 0.09, 0.10), 'bottling': (0.10, 0.12, 0.11)}
AMBIENT['walkway'] = AMBIENT['stairs2'] = AMBIENT['landing'] = AMBIENT['bottling']
# The cells of the ceilings (units): a lamp is in the middle of a cell (its panel is the cell, see lamp_cells())
CEILING_PERIOD = {'bottling': GRID * 2}
for _room in ('washroom', 'corridor', 'storage', 'hall', 'lab', 'office', 'guards', 'gallery', 'airlock', 'lowhall', 'bottling'):
    _p = CEILING_PERIOD.get(_room, GRID)
    _x0, _z0 = ROOMS[_room][0], ROOMS[_room][1]
    LAMPS[_room] = [(_x0 + (math.floor((lx - _x0) / _p) + 0.5) * _p, ly, _z0 + (math.floor((lz - _z0) / _p) + 0.5) * _p, r, c)
                    for lx, ly, lz, r, c in LAMPS[_room]]
LAMPS['walkway'] = LAMPS['stairs2'] = LAMPS['landing'] = LAMPS['bottling']


def room_floor(name):
    """The floor of a room (Saturn Y), at its west end for stairs"""
    if name in STAIRS:
        return STAIRS[name][1]
    return ROOM_FLOOR.get(name, 0)


def room_top(name):
    """The ceiling of a room (Saturn Y), at its west end for stairs"""
    return ROOM_TOP.get(name, room_floor(name) - H)


def room_openings(name):
    """The openings of the doors in the walls of a room: {'x0': [(c0, c1, floor)], 'x1': ..., 'z0': ..., 'z1': ...}"""
    x0, z0, x1, z1 = ROOMS[name]
    holes = {'x0': [], 'x1': [], 'z0': [], 'z1': []}
    for a, b, axis, pos, c0, c1, fy, style in DOORS:
        if name not in (a, b):
            continue
        if axis == 'x':
            holes['x0' if pos == x0 else 'x1'].append((c0, c1, fy))
        else:
            holes['z0' if pos == z0 else 'z1'].append((c0, c1, fy))
    return holes


def room_side(m, name, key, top, floor, tex_wall, tex_upper=None, period=GRID):
    """A wall of a room (key: x0, x1, z0, z1) from top to floor (Saturn Y), with the openings of its doors. tex_upper: the
    top row of the walls (the tiles end at TILE_TOP, stone above)"""
    x0, z0, x1, z1 = ROOMS[name]
    cuts = sorted(room_openings(name)[key])
    if key[0] == 'x':
        fixed = x0 if key == 'x0' else x1
        lo, hi = z0, z1
    else:
        fixed = z0 if key == 'z0' else z1
        lo, hi = x0, x1

    def ends(c0, c1):
        # The wall faces the inside of the room: the left end seen from inside
        if key == 'x0':
            return (fixed, c1), (fixed, c0)
        if key == 'x1':
            return (fixed, c0), (fixed, c1)
        if key == 'z0':
            return (c0, fixed), (c1, fixed)
        return (c1, fixed), (c0, fixed)

    def seg(c0, c1, bottom, over_door=False):
        a, b = ends(c0, c1)
        if over_door:
            m.wall(a, b, top, bottom, tex_upper if tex_upper is not None else tex_wall, period=period * 2)
        elif tex_upper is None:
            m.wall(a, b, top, bottom, tex_wall, period=period)
        else:
            # The tiles up to TILE_TOP (a row of quads), the stone above them
            m.wall(a, b, bottom - TILE_TOP, bottom, tex_wall, period=period)
            m.wall(a, b, top, bottom - TILE_TOP, tex_upper, period=period)

    windows = []
    for a, b, axis, wpos, w0, w1, wbottom, wtop in WINDOWS:
        if name in (a, b) and axis == key[0] and wpos == fixed:
            windows.append((w0, w1, wbottom, wtop))

    def plain(c0, c1, bottom):
        # A part of the wall without door: the windows cut in it (the wall over them and under them)
        start = c0
        for w0, w1, wbottom, wtop in sorted(windows):
            if w1 <= c0 or w0 >= c1:
                continue
            a, b = ends(w0 - FRAME_WIDTH, w1 + FRAME_WIDTH)
            if start < w0 - FRAME_WIDTH:
                seg(start, w0 - FRAME_WIDTH, bottom)
            m.wall(a, b, top, wtop - FRAME_WIDTH, tex_upper if tex_upper is not None else tex_wall, period=period)
            m.wall(a, b, wbottom + FRAME_WIDTH, bottom, tex_wall, period=period)
            start = w1 + FRAME_WIDTH
        if c1 > start:
            seg(start, c1, bottom)

    pos = lo
    for c0, c1, fy in cuts + [(hi, hi, floor)]:
        if c1 > c0:
            # (the hole goes to the outer sides of the posts of the frame, up to the top of its lintel: the frame fills it
            # around the opening, no cell of the wall goes behind it, see door_frames())
            c0, c1 = c0 - FRAME_WIDTH, c1 + FRAME_WIDTH
        if c0 > pos:
            plain(pos, c0, floor)
        if c1 > c0:
            # Over the door and its frame (the rooms are higher than the doors)
            seg(c0, c1, fy - DOOR_HEIGHT - FRAME_WIDTH, over_door=True)
        pos = max(pos, c1)


# The glows of the lamps (the engine glows: a halo of light added to the image by the VDP2, see jo/glow.h; 48 at most):
# all the lamps of the rooms (not the stairs: their ceilings slope)
GLOW_LAMPS = {'washroom': 4, 'corridor': 2, 'storage': 4, 'hall': 2, 'lab': 4, 'office': 2, 'guards': 4, 'gallery': 2,
              'airlock': 1, 'lowhall': 3, 'bottling': 4}
LAMP_PANEL = 20                 # Half size of the light panels on the ceilings (units)
# The size of the tiles of the floors (a texture cell, units): the big bottling room has bigger quads (fewer polygons), the
# treads of its stairs the same tiles
FLOOR_PERIOD = {'bottling': GRID * 2, 'stairs2': GRID * 2, 'landing': GRID * 2, 'walkway': GRID}


def lamp_cells(m, name, first):
    """The lamps of a room (see LAMPS): the quads of its ceiling (from the index first) under a lamp get the texture of a
    cell with a panel, lit by itself"""
    if name not in CEILING_PERIOD and name not in ('washroom', 'corridor', 'storage', 'hall', 'lab', 'office', 'guards',
                                                   'gallery', 'airlock', 'lowhall'):
        return
    tex = 'lampcell2' if CEILING_PERIOD.get(name, GRID) > GRID else 'lampcell'
    if name in ALARM_ROOMS:
        tex = 'alarmcell'
    for q in m.quads[first:]:
        c = np.array(q[0], dtype=float)
        for lx, ly, lz, radius, color in LAMPS.get(name, []):
            if c[:, 0].min() < lx < c[:, 0].max() and c[:, 2].min() < lz < c[:, 2].max():
                q[1] = tex


def build_room(name, tex_wall, tex_floor, tex_ceiling, tex_upper=None, sides=('x0', 'x1', 'z0', 'z1'), period=GRID):
    """The walls (with the openings of the doors), the floor and the ceiling of a room"""
    m = Mesh()
    x0, z0, x1, z1 = ROOMS[name]
    floor, top = room_floor(name), room_top(name)
    for key in sides:
        room_side(m, name, key, top, floor, tex_wall, tex_upper, period)
    # (the floors with the tiles of the treads of their stairs, see FLOOR_PERIOD)
    m.floor(x0, z0, x1, z1, floor, tex_floor, period=FLOOR_PERIOD.get(name, GRID), sort='max')
    first = len(m.quads)
    if name != 'washroom':
        m.floor(x0, z0, x1, z1, top, tex_ceiling, period=period, up=False)
        lamp_cells(m, name, first)
        return m
    # The ceiling, without the hole of the duct (the cells it crosses keep the part out of it)
    hx0, hz0, hx1, hz1 = DUCT_HOLE
    for j in range(int((z1 - z0) / GRID)):
        for i in range(int((x1 - x0) / GRID)):
            a, c = x0 + i * GRID, z0 + j * GRID
            if a < hx1 and a + GRID > hx0 and c < hz1 and c + GRID > hz0:
                # The cell minus the hole: the strips in front of it and behind it, then on its sides
                b, d = a + GRID, c + GRID
                if c < hz0:
                    m.floor(a, c, b, hz0, -H, tex_ceiling, up=False)
                if d > hz1:
                    m.floor(a, hz1, b, d, -H, tex_ceiling, up=False)
                if a < hx0:
                    m.floor(a, max(c, hz0), hx0, min(d, hz1), -H, tex_ceiling, up=False)
                if b > hx1:
                    m.floor(hx1, max(c, hz0), b, min(d, hz1), -H, tex_ceiling, up=False)
                continue
            m.floor(a, c, a + GRID, c + GRID, -H, tex_ceiling, up=False)
    lamp_cells(m, name, first)
    return m


def door_frames(m, name):
    """The frames of the doors of a room (dark metal, 6 units out of the wall, 8 wide; yellow and black stripes around the
    security door). They fill the hole of the wall around the opening (room_side()): the cells of the wall end on their
    sides, whole (no copy of a part of a texture, see fit_walls())"""
    x0, z0, x1, z1 = ROOMS[name]
    d, w = 6, FRAME_WIDTH
    for a, b, axis, pos, c0, c1, fy, style in DOORS:
        if name not in (a, b):
            continue
        tex = 'hazard' if style in ('hazard', 'slide_hazard') else 'frame'
        top = fy - DOOR_HEIGHT
        if axis == 'x':
            inside = 1 if pos == x0 else -1       # toward the inside of the room along x
            fx = pos + inside * d
            for cc0, cc1 in ((c0 - w, c0), (c1, c1 + w)):
                m.box(min(pos, fx), top - w, cc0, max(pos, fx), fy, cc1, tex, faces='xXzZ', subdivision=0, sort=AGAINST_WALL)
            m.box(min(pos, fx), top - w, c0, max(pos, fx), top, c1, tex, faces='xXY', subdivision=0, sort=AGAINST_WALL)
            if style.startswith('slide'):
                # The pocket where the panel slides (to +Z: on the left going east): a casing as deep as the frame on
                # each side of the wall, the panel (4 thick) inside it
                m.box(min(pos, fx), top - w, c1 + w, max(pos, fx), fy, c1 + w + DOOR_WIDTH, 'frame', faces='xXZ' if inside > 0 else 'xXZ',
                      subdivision=0, sort=AGAINST_WALL)
        else:
            inside = 1 if pos == z0 else -1
            fz = pos + inside * d
            for cc0, cc1 in ((c0 - w, c0), (c1, c1 + w)):
                m.box(cc0, top - w, min(pos, fz), cc1, fy, max(pos, fz), tex, faces='xXzZ', subdivision=0, sort=AGAINST_WALL)
            m.box(c0, top - w, min(pos, fz), c1, top, max(pos, fz), tex, faces='zZY', subdivision=0, sort=AGAINST_WALL)


def windows(m, name):
    """The windows of a room (see WINDOWS): a frame around the hole of the wall, the glass on the plane of the wall seen
    from the room (half transparent: the room behind it seen through it; each room has its side of the glass)"""
    x0, z0, x1, z1 = ROOMS[name]
    d, w = 6, FRAME_WIDTH
    for a, b, axis, pos, c0, c1, bottom, top in WINDOWS:
        if name not in (a, b):
            continue
        inside = 1 if pos == (x0 if axis == 'x' else z0) else -1
        p1 = pos + inside * d
        lo, hi = min(pos, p1), max(pos, p1)
        if axis == 'x':
            for cc0, cc1 in ((c0 - w, c0), (c1, c1 + w)):
                m.box(lo, top - w, cc0, hi, bottom + w, cc1, 'frame', faces='xXzZ', subdivision=0, sort=AGAINST_WALL)
            m.box(lo, top - w, c0, hi, top, c1, 'frame', faces='xXY', subdivision=0, sort=AGAINST_WALL)
            m.box(lo, bottom, c0, hi, bottom + w, c1, 'frame', faces='xXy', subdivision=0, sort=AGAINST_WALL)
            a_, b_ = ((pos, c1), (pos, c0)) if inside > 0 else ((pos, c0), (pos, c1))
        else:
            for cc0, cc1 in ((c0 - w, c0), (c1, c1 + w)):
                m.box(cc0, top - w, lo, cc1, bottom + w, hi, 'frame', faces='xXzZ', subdivision=0, sort=AGAINST_WALL)
            m.box(c0, top - w, lo, c1, top, hi, 'frame', faces='zZY', subdivision=0, sort=AGAINST_WALL)
            m.box(c0, bottom, lo, c1, bottom + w, hi, 'frame', faces='zZy', subdivision=0, sort=AGAINST_WALL)
            a_, b_ = ((c0, pos), (c1, pos)) if inside > 0 else ((c1, pos), (c0, pos))
        # (the glass is an entity: the class glass breaks when it is shot, see build_glass_models())


BARREL_RADIUS = 30                      # 1.2 m
BARREL_HEIGHT = 88
# The explosive barrels (the class explosive of the engine: fire, smoke, flash, shake, damage around, chain reactions):
# (x, z, floor)
BARRELS = [(1755, 300, 0), (1818, 282, 0), (2150, 1562, 0), (3245, 300, LOWER), (3320, 1300, LOWER), (3385, 1292, LOWER)]
# The fire, the smoke and the sound of the explosions: kept in the cd (not generated here)
EXPLOSION_FILES = ['FIRE1.BIN', 'FIRE2.BIN', 'FIRE3.BIN', 'FIRE4.BIN', 'SMOKE1.BIN', 'SMOKE2.BIN', 'EXPLODE.PCM']
# The HUD (see hud.c): the digits and the icons of the health and of the ammunition, kept in the cd (not generated here)
HUD_FILES = (['HUDD%d.BIN' % k for k in range(10)] + ['HUDS%d.BIN' % k for k in range(10)] +
             ['HUDSSL.BIN', 'HUDRLD.BIN', 'HUDHEAL.BIN', 'HUDAMMO.BIN'])


BARREL_DOOR_CLEARANCE = 130             # A barrel this far at least from a doorway (it doesn't block it)


def check_barrels():
    """Stop the build on a barrel in the way: in front of a door (nearer than BARREL_DOOR_CLEARANCE to its doorway), in a
    wall, a piece of furniture or another barrel (the barrel of the room of the guards stood in front of its door)"""
    r = BARREL_RADIUS
    for k, (bx, bz, fy) in enumerate(BARRELS):
        name = 'barrel%d (%d, %d)' % (k + 1, bx, bz)
        rooms = [n for n, (x0, z0, x1, z1) in ROOMS.items() if x0 + r <= bx <= x1 - r and z0 + r <= bz <= z1 - r]
        if not rooms:
            raise SystemExit('check_barrels: %s is in a wall' % name)
        for a, b, axis, pos, c0, c1, floor, style in DOORS:
            if abs(floor - fy) > 100:
                continue
            # (the distance to the doorway: the segment of the door on the floor)
            if axis == 'x':
                d = math.hypot(bx - pos, bz - min(max(bz, c0), c1))
            else:
                d = math.hypot(bx - min(max(bx, c0), c1), bz - pos)
            if d < BARREL_DOOR_CLEARANCE:
                raise SystemExit('check_barrels: %s is in front of the door %s-%s (%d units, at least %d)'
                                 % (name, a, b, d, BARREL_DOOR_CLEARANCE))
        for solid in SOLIDS:
            sx, sz, hx, hz = solid[:4]
            if abs(bx - sx) < hx + r and abs(bz - sz) < hz + r:
                raise SystemExit('check_barrels: %s is in a piece of furniture at (%d, %d)' % (name, sx, sz))
        for j, (ox, oz, ofy) in enumerate(BARRELS[:k]):
            if ofy == fy and math.hypot(bx - ox, bz - oz) < 2 * r + 2:
                raise SystemExit('check_barrels: %s is in the barrel%d' % (name, j + 1))


def check_guard_paths(entities):
    """Stop the build on a patrol that goes through a piece of furniture, a barrel or a wall (the guard of the hall of the
    lower floor walked into a crate at each patrol, walking on the spot): the circle of the guard along the patrol"""
    r = 20
    doorways = [(pos - 50, c0, pos + 50, c1) if axis == 'x' else (c0, pos - 50, c1, pos + 50)
                for a, b, axis, pos, c0, c1, fy, style in DOORS]
    for e in entities:
        if e.get('cls') != 'guard':
            continue
        px, pz = e['props'].get('patrol_x', 0), e['props'].get('patrol_z', 0)
        x0, z0 = e['x'], e['z']
        steps = max(1, int(math.hypot(px, pz) // 10))
        for k in range(steps + 1):
            x, z = x0 + px * k / steps, z0 + pz * k / steps
            inside = any(bx0 + r <= x <= bx1 - r and bz0 + r <= z <= bz1 - r for bx0, bz0, bx1, bz1 in ROOMS.values())
            inside = inside or any(bx0 <= x <= bx1 and bz0 <= z <= bz1 for bx0, bz0, bx1, bz1 in doorways)
            if not inside:
                raise SystemExit('check_guard_paths: the patrol of %s goes into a wall at (%d, %d)' % (e['name'], x, z))
            for solid in SOLIDS:
                sx, sz, hx, hz = solid[:4]
                sy = solid[4] if len(solid) > 4 else 0
                if abs(sy - e.get('y', 0)) < 100 and abs(x - sx) < hx + r and abs(z - sz) < hz + r:
                    raise SystemExit('check_guard_paths: the patrol of %s goes through the furniture at (%d, %d)'
                                     % (e['name'], sx, sz))
            for bx, bz, fy in BARRELS:
                if abs(fy - e.get('y', 0)) < 100 and math.hypot(x - bx, z - bz) < BARREL_RADIUS + r:
                    raise SystemExit('check_guard_paths: the patrol of %s goes through a barrel at (%d, %d)' % (e['name'], bx, bz))


def build_barrel_models():
    """The explosive barrel: near (8 sides in 2 bands, the lid), far (4 sides), after the explosion (torn, burnt, both
    faces). The origin at the bottom in the middle (Saturn Y: up is negative)"""
    r, hgt = BARREL_RADIUS, BARREL_HEIGHT
    models = []
    for sides, rs in ((8, r), (4, r * 1.05)):
        m = Mesh()
        ring = [(rs * math.cos(2 * math.pi * (k + 0.5) / sides), rs * math.sin(2 * math.pi * (k + 0.5) / sides)) for k in range(sides)]
        for k in range(sides):
            (ax, az), (bx, bz) = ring[k], ring[(k + 1) % sides]
            # (the skull on 2 opposite sides, the stripes on the sides between them; far: 4 sides, the skull on 2)
            if sides == 8:
                tex = ('barrels', 'barrel', 'barrelh', 'barrel')[k % 4]
            else:
                tex = ('barrels', 'barrelh')[k % 2]
            for y0, y1, half in ((-hgt, -hgt / 2, '_t'), (-hgt / 2, 0, '_b')):
                quad_out(m, [(ax, y0, az), (bx, y0, bz), (bx, y1, bz), (ax, y1, az)], (0, -hgt / 2, 0), tex + half, subdivision=0)
        top = [(x, -hgt, z) for x, z in ring]
        for q in (([0, 1, 2, 3], [0, 3, 4, 7], [4, 5, 6, 7]) if sides == 8 else ([0, 1, 2, 3],)):
            face(m, [top[i] for i in q], (0, -1, 0), 'barrelt', subdivision=0)
        for k, q in enumerate(m.quads):
            q[3] = [(0.50, 0.50, 0.50)] * 4 if quad_normal(q[0])[1] < -0.5 else [(0.46, 0.46, 0.46)] * 4
        models.append(m)
    rng = np.random.default_rng(77)
    d = Mesh()
    tops = [hgt * (0.45 + 0.3 * rng.random()) for k in range(8)]
    for k in range(8):
        a0, a1 = 2 * math.pi * k / 8, 2 * math.pi * (k + 1) / 8
        d.quad([(r * math.cos(a0), 0, r * math.sin(a0)), (r * math.cos(a1), 0, r * math.sin(a1)),
                (r * 1.18 * math.cos(a1), -tops[(k + 1) % 8], r * 1.18 * math.sin(a1)), (r * 1.18 * math.cos(a0), -tops[k], r * 1.18 * math.sin(a0))],
               'burnt', subdivision=0, dual=True)
    for q in d.quads:
        if float(np.dot(quad_normal(q[0]), np.mean(np.array(q[0]), axis=0) * np.array((1, 0, 1)))) < 0:
            q[0] = q[0][::-1]
        q[3] = [(0.45, 0.45, 0.45)] * 4
    models.append(d)
    return models


def build_monitor_models():
    """The monitor of a computer: a box of beige plastic, the screen on its front (toward -Z: the model turned by the yaw
    of the entity), its stand; the origin on the desk under the stand. The monitor broken: the screen dark and cracked"""
    models = []
    for screen in ('screen0', 'screenx'):
        m = Mesh()
        o = dict(subdivision=0)
        m.box(-18, -40, -15, 18, -8, 15, 'casing', faces='xXZy', split=False, **o)
        # (the screen: its texture swapped by the game for the animation (16 colors: a texture carries its own table))
        face(m, [(-18, -40, -15), (18, -40, -15), (18, -8, -15), (-18, -8, -15)], (0, 0, -1), screen, **o)
        m.box(-6, -8, -5, 6, 0, 5, 'frame', faces='xXzZ', split=False, **o)
        for q in m.quads:
            n = quad_normal(q[0])
            q[3] = [(0.50, 0.50, 0.50) if n[1] < -0.5 else (0.56, 0.56, 0.56) if q[1].startswith('screen') else
                    (0.46, 0.46, 0.46)] * 4
            if screen == 'screenx' and q[1] == screen:
                q[3] = [(0.40, 0.40, 0.40)] * 4
        models.append(m)
    return models


def build_screen_frames():
    """The other frames of the screens of the monitors: cd/SCRFRM.JOM (a quad per texture, only to load them in the
    VRAM) and screen_frames.h (the names of the embedded textures of the frames, in their order: props.c swaps them on
    the polygon of the screen, see build_monitor_models())"""
    import jo_model
    frames = ['screen0', 'screen1', 'screen2']
    names = [jo_model.embedded_texture_name(os.path.join(MODELS, f + '.png'), True, []) for f in frames]
    m = Mesh()
    for k, f in enumerate(frames[1:]):
        m.quad([(k * 10, -10, 0), (k * 10 + 8, -10, 0), (k * 10 + 8, 0, 0), (k * 10, 0, 0)], f, subdivision=0)
    for q in m.quads:
        q[3] = [(0.5, 0.5, 0.5)] * 4
    write_model(m, 'scrfrm', 'The frames of the screens of the computers (their textures only)')
    out = ['/*', '** Jo Sega Saturn Engine: first person shooter demo, the screens of the computers (generated by tools/make_assets.py)',
           '*/', '', '#ifndef __SCREEN_FRAMES_H__', '# define __SCREEN_FRAMES_H__', '',
           '/** @brief The frames of the animation of a screen (the first one: in cd/MONITOR.JOM, the others in cd/SCRFRM.JOM) */',
           '# define SCREEN_FRAME_COUNT (%d)' % len(frames), '',
           '/** @brief The names of the embedded textures of the frames */',
           'static const char * const screen_frame_textures[SCREEN_FRAME_COUNT] =', '{',
           '    ' + ', '.join('"%s"' % n for n in names), '};', '', '#endif /* !__SCREEN_FRAMES_H__ */']
    with open(os.path.join(DEMO, 'screen_frames.h'), 'w') as f:
        f.write('\n'.join(out) + '\n')


def build_glassware_models():
    """The glassware of the benches (screen doors: the glass half transparent), the origin at the bottom in the middle:
    a rack of 5 test tubes (a dark bar with the tubes standing in it), the rack with the tubes broken (stubs); an
    Erlenmeyer flask (a 6-sided cone and its neck), its bottom broken (a jagged ring)"""
    o = dict(subdivision=0)
    g = dict(subdivision=0, mesh=True)
    models = {}
    for broken in (False, True):
        m = Mesh()
        m.box(-14, -3, -3, 14, 0, 3, 'frame', faces='xXzZy', split=False, **o)
        for k in range(5):
            cx = -11 + k * 5.5
            tex = ('tubeg', 'tuber', 'tubee', 'tubeb', 'tubeg')[k]
            top = -6 if broken else -17
            if broken and k % 2 == 1:
                continue
            for a0, a1 in ((-1, 1), (1, 1), (1, -1), (-1, -1)):
                pass
            r = 1.8
            ring = [(cx - r, -r), (cx + r, -r), (cx + r, r), (cx - r, r)]
            for i in range(4):
                (ax, az), (bx, bz) = ring[i], ring[(i + 1) % 4]
                quad_out(m, [(ax, -3, az), (bx, -3, bz), (bx, top, bz), (ax, top, az)], (cx, -10, 0), tex if not broken else 'tubee', **g)
        for q in m.quads:
            q[3] = [(0.46, 0.46, 0.46) if q[1] == 'frame' else (0.58, 0.58, 0.60)] * 4
        models['tubesd' if broken else 'tubes'] = m
    for broken in (False, True):
        m = Mesh()
        sides = 6
        base = [(7 * math.cos(2 * math.pi * (k + 0.5) / sides), 7 * math.sin(2 * math.pi * (k + 0.5) / sides)) for k in range(sides)]
        if broken:
            rng_b = np.random.default_rng(91)
            tops = [-(3 + 4 * rng_b.random()) for k in range(sides)]
            for k in range(sides):
                (ax, az), (bx, bz) = base[k], base[(k + 1) % sides]
                quad_out(m, [(ax, 0, az), (bx, 0, bz), (bx, tops[(k + 1) % sides], bz), (ax, tops[k], az)], (0, -2, 0), 'flaskg', **g)
        else:
            neck = [(2.5 * math.cos(2 * math.pi * (k + 0.5) / sides), 2.5 * math.sin(2 * math.pi * (k + 0.5) / sides)) for k in range(sides)]
            for k in range(sides):
                (ax, az), (bx, bz) = base[k], base[(k + 1) % sides]
                (nx, nz), (mx, mz) = neck[k], neck[(k + 1) % sides]
                quad_out(m, [(ax, 0, az), (bx, 0, bz), (mx, -18, mz), (nx, -18, nz)], (0, -9, 0), 'flaskg', **g)
                quad_out(m, [(nx, -18, nz), (mx, -18, mz), (mx, -25, mz), (nx, -25, nz)], (0, -21, 0), 'tubee', **g)
        for q in m.quads:
            q[3] = [(0.58, 0.58, 0.60)] * 4
        models['flaskd' if broken else 'flask'] = m
    return models


def check_kept_files():
    """The fire, the smoke and the sound of the explosions, the HUD: files of the cd that this script does not make"""
    for name in EXPLOSION_FILES + HUD_FILES:
        if not os.path.exists(os.path.join(CD, name)):
            raise SystemExit('cd/%s missing: it is not generated, get it from the repository' % name)


def build_glass_models():
    """The pane of a window (its width and height: the first window; the X axis of the model along the width, Y up from its
    bottom, both faces) and a piece of broken glass. Screen doors (a pixel in 2, the mesh of the VDP1): half transparent,
    the VDP1 reads the frame buffer under each pixel, the window in front of the agent fell to 20 frames per second"""
    a, b, axis, pos, c0, c1, bottom, top = WINDOWS[0]
    w, h = c1 - c0, bottom - top
    pane = Mesh()
    o = dict(mesh=True, subdivision=0)
    n = max(1, int(round(w / GRID)))
    for k in range(n):
        x0, x1 = -w / 2 + w * k / n, -w / 2 + w * (k + 1) / n
        face(pane, [(x0, -h, 0), (x1, -h, 0), (x1, 0, 0), (x0, 0, 0)], (0, 0, -1), 'glass', **o)
        face(pane, [(x0, -h, 0), (x1, -h, 0), (x1, 0, 0), (x0, 0, 0)], (0, 0, 1), 'glass', **o)
    for q in pane.quads:
        q[3] = [(0.56, 0.58, 0.60)] * 4
    shard = Mesh()
    corners = [(-7, -1, 0), (5, -6, 0), (8, 4, 0), (-3, 7, 0)]
    face(shard, corners, (0, 0, -1), 'glass', **o)
    face(shard, corners, (0, 0, 1), 'glass', **o)
    for q in shard.quads:
        q[3] = [(0.60, 0.62, 0.64)] * 4
    return pane, shard


def toilet(m, cx, z0, facing_z):
    """A toilet: a pale blue pedestal bowl against the back wall (z0), 3D; the black seat is a sprite
    (an entity of the class sprite, see build_level()). The bowl: a hexagonal rim 30 x 36 at 38 high, narrowing to a foot"""
    rim = [(cx - 15, -38, z0 + 2), (cx - 15, -38, z0 + 22), (cx - 9, -38, z0 + 36), (cx + 9, -38, z0 + 36), (cx + 15, -38, z0 + 22),
           (cx + 15, -38, z0 + 2)]
    foot = [(cx - 8, 0, z0 + 2), (cx - 8, 0, z0 + 14), (cx - 5, 0, z0 + 22), (cx + 5, 0, z0 + 22), (cx + 8, 0, z0 + 14),
            (cx + 8, 0, z0 + 2)]
    for k in range(len(rim) - 1):
        # (seen from outside: the rim goes around counter clockwise seen from above, from the left of the wall)
        m.quad([rim[k + 1], rim[k], foot[k], foot[k + 1]], 'porcelain', subdivision=0)
    # The top of the rim: 2 quads with a dark water hole in the middle
    # (the inside of the bowl is white, the lamps of the room are far from the stalls)
    m.quad([rim[0], rim[1], rim[4], rim[5]], 'porcelain', subdivision=0, unlit=(0.56, 0.56, 0.56))
    m.quad([rim[1], rim[2], rim[3], rim[4]], 'porcelain', subdivision=0, unlit=(0.56, 0.56, 0.56))
    m.decal([(cx - 7, -38, z0 + 28), (cx + 7, -38, z0 + 28), (cx + 7, -38, z0 + 10), (cx - 7, -38, z0 + 10)], 'water')
    SOLIDS.append((cx, z0 + 18, 15, 18))


def quad_out(m, corners, center, tex, **options):
    """A quad facing away from a point (the center of its object): its corners are turned if needed"""
    c = np.mean(np.array(corners, dtype=float), axis=0)
    if float(np.dot(quad_normal(corners), c - np.array(center, dtype=float))) < 0:
        corners = corners[::-1]
    m.quad(corners, tex, **options)


def urinal(m, cz, wall_x=0):
    """A urinal hung on the west wall (x = wall_x), seen from the side: a wedge of pale blue porcelain,
    thin at the top against the wall, its front leaning out down to a thick lip, then going back to the wall under it.
    Hollow over the lip: a thin rim around the opening, the inside (its back, its sides, the bottom of the bowl with
    its drain) seen through it"""
    # The rings from the top to the bottom: (y, depth out of the wall, half width)
    rings = [(-104, 9, 15), (-51, 32, 19), (-43, 30, 18), (-34, 10, 13)]
    rim = 1.5
    center = (wall_x + 14, -70, cz)
    (yt, dt, ht), (yl, dl, hl) = rings[0], rings[1]
    wx = wall_x
    opts = dict(subdivision=0, sort=AGAINST_WALL)
    for side in (-1, 1):
        # The outer sides over the lip, the thin rim on the front edge, the inner sides
        quad_out(m, [(wx, yt, cz + side * ht), (wx + dt, yt, cz + side * ht), (wx + dl, yl, cz + side * hl), (wx, yl, cz + side * hl)],
                 center, 'porcelain', **opts)
        quad_out(m, [(wx + dt, yt, cz + side * ht), (wx + dl, yl, cz + side * hl), (wx + dl - 1, yl, cz + side * (hl - rim)),
                     (wx + dt - 1, yt, cz + side * (ht - rim))], (wx, (yt + yl) / 2, cz + side * (ht - rim)), 'porcelain', **opts)
        quad_out(m, [(wx + rim, yt + rim, cz + side * (ht - rim)), (wx + dt - 1, yt + rim, cz + side * (ht - rim)),
                     (wx + dl - 1, yl, cz + side * (hl - rim)), (wx + rim, yl, cz + side * (hl - rim))],
                 (wx + 15, (yt + yl) / 2, cz + side * 100), 'porcelain', unlit=(0.40, 0.42, 0.45), **opts)
    # The rim on the top edge, the top
    quad_out(m, [(wx + dt, yt, cz - ht), (wx + dt, yt, cz + ht), (wx + dt - 1, yt + rim, cz + ht - rim), (wx + dt - 1, yt + rim, cz - ht + rim)],
             (wx, yt + 5, cz), 'porcelain', **opts)
    quad_out(m, [(wx, yt, cz - ht), (wx + dt, yt, cz - ht), (wx + dt, yt, cz + ht), (wx, yt, cz + ht)], (wx + dt / 2, yt + 10, cz), 'porcelain', **opts)
    # The inside: its back (on the wall), the bottom of the bowl (the drain painted on it)
    # (the back is painted on the wall, a decal: a polygon 1.5 units in front of the wall was sorted behind the parts of
    # the wall cut near the camera, it disappeared at some angles)
    m.decal([(wx, yt + rim, cz - ht + rim), (wx, yt + rim, cz + ht - rim), (wx, yl, cz + hl - rim), (wx, yl, cz - hl + rim)], 'urinal')
    bottom = [(wx + rim, yl, cz - hl + rim), (wx + rim, yl, cz + hl - rim), (wx + dl - 1, yl, cz + hl - rim), (wx + dl - 1, yl, cz - hl + rim)]
    # (the inside in the shadow of the bowl: darker than the outside)
    quad_out(m, bottom, (wx + 15, yl + 10, cz), 'porcelain', unlit=(0.42, 0.44, 0.47), **opts)
    # (the decal: clockwise seen from above, the first corner the top left of the texture)
    m.decal([(wx + 6, yl, cz - 7), (wx + 6, yl, cz + 7), (wx + 20, yl, cz + 7), (wx + 20, yl, cz - 7)], 'drain')
    # Under the lip: closed (the front, the sides, the bottom), square in front
    for j in range(1, len(rings) - 1):
        (ya, da, ha), (yb, db, hb) = rings[j], rings[j + 1]
        quad_out(m, [(wx + da, ya, cz - ha), (wx + da, ya, cz + ha), (wx + db, yb, cz + hb), (wx + db, yb, cz - hb)], center, 'porcelain', **opts)
        for side in (-1, 1):
            quad_out(m, [(wx, ya, cz + side * ha), (wx + da, ya, cz + side * ha), (wx + db, yb, cz + side * hb), (wx, yb, cz + side * hb)],
                     center, 'porcelain', **opts)
    # The lip: the top of the ring under the bottom of the bowl (its front edge)
    quad_out(m, [(wx + dl - 1, yl, cz - hl + rim), (wx + dl - 1, yl, cz + hl - rim), (wx + dl, yl, cz + hl), (wx + dl, yl, cz - hl)],
             (wx + dl - 1, yl + 10, cz), 'porcelain', **opts)
    yb, db, hb = rings[-1]
    quad_out(m, [(wx, yb, cz - hb), (wx + db, yb, cz - hb), (wx + db, yb, cz + hb), (wx, yb, cz + hb)], (wx + db / 2, yb - 10, cz), 'porcelain', **opts)
    m.attach(wall_x, rings[0][0], cz - 19, wall_x + 32, rings[-1][0], cz + 19)
    SOLIDS.append((wall_x + 16, cz, 16, 19))


def basin(m, cx, wall_z):
    """A basin hung on the north wall (z = wall_z, the wall behind it): a hexagonal bowl of pale blue porcelain, 36 wide,
    26 deep, from 70 to 56 high, a dark drain"""
    top, bottom = -70, -56
    front = wall_z - 26
    rim = [(cx - 18, top, wall_z), (cx - 18, top, front + 10), (cx - 10, top, front), (cx + 10, top, front), (cx + 18, top, front + 10),
           (cx + 18, top, wall_z)]
    under = [(cx - 9, bottom, wall_z), (cx - 9, bottom, front + 16), (cx - 5, bottom, front + 12), (cx + 5, bottom, front + 12),
             (cx + 9, bottom, front + 16), (cx + 9, bottom, wall_z)]
    for k in range(len(rim) - 1):
        m.quad([rim[k], rim[k + 1], under[k + 1], under[k]], 'porcelain', subdivision=0, sort=AGAINST_WALL)
    m.quad([rim[5], rim[4], rim[1], rim[0]], 'porcelain', subdivision=0, sort=AGAINST_WALL)
    m.quad([rim[4], rim[3], rim[2], rim[1]], 'porcelain', subdivision=0, sort=AGAINST_WALL)
    m.decal([(cx - 9, top, wall_z - 3), (cx + 9, top, wall_z - 3), (cx + 9, top, front + 5), (cx - 9, top, front + 5)], 'drain')
    # The tap: a small dark block on the wall
    m.box(cx - 2, top - 14, wall_z - 6, cx + 2, top - 2, wall_z, 'frame', faces='xXzy', subdivision=0, sort=AGAINST_WALL)
    m.attach(cx - 18, top - 14, front, cx + 18, bottom, wall_z)
    SOLIDS.append((cx, (wall_z + front) / 2, 18, (wall_z - front) / 2))


def build_washroom():
    m = build_room('washroom', 'tiles', 'floor', 'ceiling', tex_upper='stone')
    wx0, wz0, wx1, wz1 = ROOMS['washroom']
    # The stalls: white partitions between them (both sides seen), a lintel over the doors, a toilet in the middle of
    # each one against the back wall
    for sx0, sz0, sx1, sz1 in STALLS:
        # The partition on the right of each stall (the last one too: it closes the row of stalls, there was a hole)
        # (up to the posts of the front: 2 faces on the same plane would flicker)
        m.box(sx1 - 2, -STALL_HEIGHT, sz0, sx1 + 2, 0, sz1 - 4, 'white', faces='xXy', subdivision=0)
        SOLIDS.append((sx1, (sz0 + sz1) / 2, 2, (sz1 - sz0) / 2))
        # The front: a post on each side of the door, the lintel
        # (the first post against the west wall: sorted with its nearest point, see check_no_overlap())
        m.box(sx0, -STALL_HEIGHT, sz1 - 4, sx0 + 2, 0, sz1, 'white', faces='xXZ' if sx0 == wx0 else 'XZ', subdivision=0,
              **(dict(sort=AGAINST_WALL) if sx0 == wx0 else {}))
        # (the side toward the next stall: against its left post (the last one: the end post), hidden, not drawn)
        m.box(sx1 - 2, -STALL_HEIGHT, sz1 - 4, sx1, 0, sz1, 'white', faces='xZ', subdivision=0)
        if sx1 == STALLS[-1][2]:
            # (the end of the last partition: the post the next stall would have)
            m.box(sx1, -STALL_HEIGHT, sz1 - 4, sx1 + 2, 0, sz1, 'white', faces='XZy', subdivision=0)
        m.box(sx0 + 2, -STALL_HEIGHT, sz1 - 4, sx1 - 2, -STALL_HEIGHT + 8, sz1, 'white', faces='zZY', subdivision=0)
        toilet(m, (sx0 + sx1) / 2, sz0, 1)
    # The basins hung on the north wall, a tap sign over the first one, a no smoking sign on the east wall
    # (the basins and the urinals in the middle of a cell of the wall (GRID): sorted with their nearest point, they are
    # nearer than the cell behind them; on 2 cells, the center of the other cell was nearer at some angles: the wall was
    # drawn over them)
    for bx in (360, 440, 520, 600, 680):
        basin(m, bx, wz1)
    # 4 urinals on the west wall
    for cz in (360, 440, 520, 600):
        urinal(m, cz, wx0)
    # The long mirror over the basins (a hole in the wall and a portal, see MIRRORS): its frame, thin bars of metal
    for room, key, m0, m1, mtop, mbottom in MIRRORS:
        m.box(m0 - 3, mtop - 3, wz1 - 2, m1 + 3, mtop, wz1, 'frame', faces='zyY', subdivision=0, sort=AGAINST_WALL)
        m.box(m0 - 3, mbottom, wz1 - 2, m1 + 3, mbottom + 3, wz1, 'frame', faces='zyY', subdivision=0, sort=AGAINST_WALL)
        m.box(m0 - 3, mtop, wz1 - 2, m0, mbottom, wz1, 'frame', faces='zxX', subdivision=0, sort=AGAINST_WALL)
        m.box(m1, mtop, wz1 - 2, m1 + 3, mbottom, wz1, 'frame', faces='zxX', subdivision=0, sort=AGAINST_WALL)
    m.decal([(196, -150, wz1), (228, -150, wz1), (228, -118, wz1), (196, -118, wz1)], 'tapsign')
    m.decal([(wx1, -150, 700), (wx1, -150, 660), (wx1, -110, 660), (wx1, -110, 700)], 'nosmoke')
    # The hole of the duct: its 4 inner sides go up to the floor of the duct (dark)
    hx0, hz0, hx1, hz1 = DUCT_HOLE
    m.box(hx0, DUCT_FLOOR - 1, hz0, hx1, -H, hz1, 'frame', faces='', subdivision=0)
    door_frames(m, 'washroom')
    # A sign over the door: exit
    # (on the right of the door seen from the room, on the concrete: over the door the strip of wall is too low)
    m.decal([(960, -152, 336), (960, -152, 296), (960, -132, 296), (960, -132, 336)], 'exit')
    occluders = [((sx0 + sx1) / 2 - 15, -38, sz0, (sx0 + sx1) / 2 + 15, 0, sz0 + 36) for sx0, sz0, sx1, sz1 in STALLS]
    bake_light(m, LAMPS['washroom'], AMBIENT['washroom'], occluders)
    return m


LOCKER_HEIGHT = 140             # The lockers: 2.8 m high (as high as before)
LOCKER_DEPTH = 24               # 48 cm deep
LOCKER_WIDTH = 80               # 2 doors of 80 cm: one copy of the texture of the front (64 x 128 texels: texels about square)


def lockers(m, wall, plane, start, count):
    """A row of steel lockers against a wall: count blocks of 2 doors, each front one quad with the whole texture (80 x 140
    units), the 2 ends with the texture of the sides, the top without texture in the blue of the paint. wall 'x': the wall is the plane
    x = plane, the row along Z from z = start, on the side x < plane; 'z': the wall is z = plane, the row along X from x = start,
    on the side z < plane. The row is attached to the wall as one object (see fit_walls(): the cells of the wall are only
    cut at its ends, not at each block). Its collision box is added to SOLIDS"""
    o = dict(subdivision=0, sort=AGAINST_WALL)
    # (the top: the mean color of the paint of the doors, see texture_locker())
    paint = TEXTURES['locker'][45:90, 3:20].reshape(-1, 3).mean(axis=0) / 255
    top = dict(o, kd='%.3f %.3f %.3f' % tuple(paint))
    end = start + count * LOCKER_WIDTH
    y0, y1 = -LOCKER_HEIGHT, 0
    if wall == 'x':
        x0, x1 = plane - LOCKER_DEPTH, plane
        m.attach(x0, y0, start, x1, y1, end)
        for k in range(count):
            a, b = start + k * LOCKER_WIDTH, start + (k + 1) * LOCKER_WIDTH
            m.quad([(x0, y0, b), (x0, y0, a), (x0, y1, a), (x0, y1, b)], 'locker', **o)
            m.quad([(x0, y0, b), (x1, y0, b), (x1, y0, a), (x0, y0, a)], None, **top)
        m.quad([(x0, y0, start), (x1, y0, start), (x1, y1, start), (x0, y1, start)], 'lockerside', **o)
        m.quad([(x1, y0, end), (x0, y0, end), (x0, y1, end), (x1, y1, end)], 'lockerside', **o)
        SOLIDS.append((plane - LOCKER_DEPTH // 2, (start + end) // 2, LOCKER_DEPTH // 2, (end - start) // 2))
    else:
        z0, z1 = plane - LOCKER_DEPTH, plane
        m.attach(start, y0, z0, end, y1, z1)
        for k in range(count):
            a, b = start + k * LOCKER_WIDTH, start + (k + 1) * LOCKER_WIDTH
            m.quad([(a, y0, z0), (b, y0, z0), (b, y1, z0), (a, y1, z0)], 'locker', **o)
            m.quad([(a, y0, z1), (b, y0, z1), (b, y0, z0), (a, y0, z0)], None, **top)
        m.quad([(start, y0, z1), (start, y0, z0), (start, y1, z0), (start, y1, z1)], 'lockerside', **o)
        m.quad([(end, y0, z0), (end, y0, z1), (end, y1, z1), (end, y1, z0)], 'lockerside', **o)
        SOLIDS.append(((start + end) // 2, plane - LOCKER_DEPTH // 2, (end - start) // 2, LOCKER_DEPTH // 2))


def crates(m, items, floor=0):
    """Wooden crates (center x, center z, size): solid, their shadows; returns the boxes (the occluders of the light)"""
    boxes = []
    for cx, cz, size in items:
        m.box(cx - size / 2, floor - size, cz - size / 2, cx + size / 2, floor, cz + size / 2, 'crate', faces='xXzZy', subdivision=0)
        boxes.append((cx - size / 2, floor - size, cz - size / 2, cx + size / 2, floor, cz + size / 2))
        SOLIDS.append((cx, cz, size / 2, size / 2, floor, size + 10))
    return boxes


def face(m, corners, toward, tex, **options):
    """A quad facing a direction (its corners turned if needed)"""
    if float(np.dot(quad_normal(corners), np.array(toward, dtype=float))) < 0:
        corners = corners[::-1]
    m.quad(corners, tex, **options)


def build_corridor():
    # (tiled like the corridor near the washroom: white tiles, stone above)
    m = build_room('corridor', 'tiles', 'floor', 'ceiling', tex_upper='stone')
    door_frames(m, 'corridor')
    # A biohazard sign on the north wall (inside one cell of the wall, on the stone: a decal across 2 cells makes 2 copies
    # of the texture, 4 at a corner of cells)
    m.decal([(1480, -150, 560), (1520, -150, 560), (1520, -110, 560), (1480, -110, 560)], 'biohaz')
    bake_light(m, LAMPS['corridor'], AMBIENT['corridor'])
    return m


def build_storage():
    m = build_room('storage', 'stone', 'floor', 'ceiling')
    door_frames(m, 'storage')
    boxes = crates(m, [(1680, 640, 60), (1740, 640, 60), (1680, 580, 60), (2200, 240, 70), (2120, 240, 50), (2290, 700, 60),
                       (1900, 260, 60)])
    # A crate on top of another one
    m.box(1680 - 25, -110, 640 - 25, 1680 + 25, -60, 640 + 25, 'crate', faces='xXzZy', subdivision=0, sort='min')
    # Lockers along the north wall
    lockers(m, 'z', 800, 2080, 3)
    # The way down: a sign over the door of the stairs (on the stone, on the right of the door)
    m.decal([(2400, -152, 336), (2400, -152, 296), (2400, -132, 296), (2400, -132, 336)], 'exit')
    bake_light(m, LAMPS['storage'], AMBIENT['storage'], boxes)
    return m


def build_hall():
    """The tiled hall from the corridor to the laboratory"""
    m = build_room('hall', 'tiles', 'floor', 'ceiling', tex_upper='stone')
    door_frames(m, 'hall')
    windows(m, 'hall')
    bake_light(m, LAMPS['hall'], AMBIENT['hall'])
    return m


def bench(m, x0, z0, x1, z1, height=45):
    """A bench of the laboratory: a white top, dark steel sides. One box: its faces never cover each other (a top over a
    narrower body: the sides of the body, sorted with their centers, were drawn over the top; see check_overhangs())"""
    m.box(x0, -height, z0, x1, 0, z1, 'white', faces='y', subdivision=0)
    m.box(x0, -height, z0, x1, 0, z1, 'frame', faces='xXzZ', subdivision=0)
    SOLIDS.append(((x0 + x1) / 2, (z0 + z1) / 2, (x1 - x0) / 2, (z1 - z0) / 2, 0, height + 10))
    return (x0, -height, z0, x1, 0, z1)


def build_lab():
    """The laboratory: white tiles, 2 long benches, a desk, a crate (the room of the scientists)"""
    m = build_room('lab', 'tiles', 'floor', 'ceiling', tex_upper='stone')
    door_frames(m, 'lab')
    boxes = [bench(m, 1040, 1600, 1440, 1660), bench(m, 1040, 1760, 1440, 1820), bench(m, 1560, 1800, 1720, 1880, 40)]
    boxes += crates(m, [(1000, 1440, 60)])
    # The racks of test tubes and the flasks on the benches (the class breakable: broken by a shot), 2 computers on the
    # second bench facing the door of the hall (the keyboards painted, their monitors are entities)
    GLASSWARE.extend([('tubes', 1090, -45, 1632, 0), ('flask', 1160, -45, 1626, 0), ('tubes', 1300, -45, 1628, 15),
                      ('flask', 1395, -45, 1636, 0), ('flask', 1070, -45, 1792, 0), ('tubes', 1380, -45, 1788, 0),
                      ('flask', 1640, -40, 1840, 0), ('tubes', 1690, -40, 1850, 90)])
    for cx in (1180, 1260):
        m.decal([(cx - 20, -45, 1806), (cx + 20, -45, 1806), (cx + 20, -45, 1786), (cx - 20, -45, 1786)], 'keyboard')
        MONITORS.append((cx, -45, 1774, 0))
    bake_light(m, LAMPS['lab'], AMBIENT['lab'], boxes)
    return m


def build_guards():
    """The room of the guards: lockers along the east wall, crates, the switch of the security door"""
    m = build_room('guards', 'stone', 'floor', 'ceiling')
    door_frames(m, 'guards')
    lockers(m, 'x', 2400, 880, 5)
    boxes = crates(m, [(2280, 1540, 60), (2220, 1540, 60), (1840, 1260, 60), (1840, 1200, 50)])
    bake_light(m, LAMPS['guards'], AMBIENT['guards'], boxes)
    return m


def computer(m, x_back, cz, top, yaw=90):
    """A computer on a desk (its top at the height top), facing -X: the keyboard and the mouse in front of its place,
    painted on the top of the desk (flat things on a surface: polygons a few units over it were cut by it, see
    check_no_overlap()). The monitor is an entity (the class breakable of props.c: its screen animated, broken by a shot),
    placed in the level, see MONITORS"""
    m.decal([(x_back - 70, top, cz + 20), (x_back - 50, top, cz + 20), (x_back - 50, top, cz - 20), (x_back - 70, top, cz - 20)],
            'keyboard')
    m.decal([(x_back - 64, top, cz - 26), (x_back - 56, top, cz - 26), (x_back - 56, top, cz - 32), (x_back - 64, top, cz - 32)],
            'mouse')
    MONITORS.append((x_back - 15, top, cz, yaw))


# The monitors of the computers: (x, y of the desk top, z, yaw) filled by computer(); the glassware on the benches:
# (model, x, y, z, yaw)
MONITORS = []
GLASSWARE = []


def build_office():
    """The office of the laboratory, seen from the hall through its window: a desk with computers, cupboards"""
    m = build_room('office', 'tiles', 'floor', 'ceiling', tex_upper='stone')
    door_frames(m, 'office')
    windows(m, 'office')
    # The desk along the east wall, 2 computers on it facing the window of the hall
    boxes = [bench(m, 1600, 960, 1700, 1160, 40)]
    for cz in (1010, 1110):
        computer(m, 1696, cz, -40)
    lockers(m, 'z', 1360, 1672, 1)
    boxes += crates(m, [(1420, 1300, 50)])
    bake_light(m, LAMPS['office'], AMBIENT['office'], boxes)
    return m


def build_airlock():
    """The airlock before the walkway of the bottling room: 2 sliding doors, a yellow lamp"""
    m = build_room('airlock', 'concrete', 'grate', 'ceiling')
    door_frames(m, 'airlock')
    bake_light(m, LAMPS['airlock'], AMBIENT['airlock'])
    return m


def build_gallery():
    """The concrete corridor to the security door of the bottling room (yellow and black stripes around it)"""
    m = build_room('gallery', 'concrete', 'floor', 'ceiling')
    door_frames(m, 'gallery')
    bake_light(m, LAMPS['gallery'], AMBIENT['gallery'])
    return m


def stairs_steps(name):
    """The steps of stairs along +X: [(x0, x1, y of the tread)], the landings included"""
    x0, z0, x1, z1 = ROOMS[name]
    axis, ya, yb, landing = STAIRS[name]
    n = STAIRS_STEPS
    xs, xe = x0 + landing, x1 - landing
    steps = []
    if landing:
        steps.append((x0, xs, ya))
    for k in range(n):
        steps.append((xs + (xe - xs) * k / n, xs + (xe - xs) * (k + 1) / n, ya + (yb - ya) * (k + 1) / n))
    if landing:
        steps.append((xe, x1, yb))
    return steps


def stairs_slope(name, x):
    """The floor of the cell of stairs at x: the line through the middles of the treads (see stairs_steps())"""
    x0, z0, x1, z1 = ROOMS[name]
    axis, ya, yb, landing = STAIRS[name]
    xs, xe = x0 + landing, x1 - landing
    run, rise = (xe - xs) / STAIRS_STEPS, (yb - ya) / STAIRS_STEPS
    return ya + rise * ((x - xs) / run + 0.5)


def tread_texture(depth, scale=GRID):
    """The floor texture cut to a tread (depth: world units along X, its columns; GRID along Z, its rows), the tiles at
    the scale of the floor of the room around (scale: world units of the texture)"""
    base = TEXTURES['floor']
    h, w = base.shape[:2]
    cols = max(8, int(round(w * depth / scale)) // 8 * 8)
    rows = max(8, int(round(h * GRID / scale)))
    name = 'tread%d_%d' % (cols, rows)
    if name not in TEXTURES:
        save_texture(name, base[:rows, :cols].copy())
    return name


def stairs_treads(m, name, z0, z1):
    """The treads and the risers of stairs (see stairs_steps())"""
    # (the first riser: from the floor at the top of the stairs, the walkway without landing)
    previous = STAIRS[name][1]
    for xa, xb, y in stairs_steps(name):
        # (the tiles of the floors at their scale: a texture cell is GRID; a tread is narrower: its strip of the texture)
        tex = tread_texture(xb - xa, FLOOR_PERIOD.get(name, GRID))
        for za in range(int(z0), int(z1), GRID):
            m.quad([(xa, y, za + GRID), (xb, y, za + GRID), (xb, y, za), (xa, y, za)], tex, sort='max')
        if previous is not None and y > previous:
            face(m, [(xa, previous, z1), (xa, previous, z0), (xa, y, z0), (xa, y, z1)], (1, 0, 0), 'concrete')
        previous = y


def side_strip(m, x0, x1, z, top0, top1, bottom, toward, tex):
    """A part of a side wall of stairs (on the plane z): from a sloped top (top0 at x0, top1 at x1) down to a flat bottom,
    in 2 rows"""
    mid0, mid1 = (top0 + bottom) / 2, (top1 + bottom) / 2
    face(m, [(x0, top0, z), (x1, top1, z), (x1, mid1, z), (x0, mid0, z)], toward, tex)
    face(m, [(x0, mid0, z), (x1, mid1, z), (x1, bottom, z), (x0, bottom, z)], toward, tex)


def build_stairs():
    """The stairs from the storage room (upper floor) down to the hall of the lower floor: landings at both ends (the
    cells of the doors), 12 steps, side walls, a sloped ceiling"""
    m = Mesh()
    x0, z0, x1, z1 = ROOMS['stairs']
    axis, ya, yb, landing = STAIRS['stairs']
    stairs_treads(m, 'stairs', z0, z1)
    xs, xe = x0 + landing, x1 - landing

    def ceiling(x):
        return ya - H + (yb - ya) * min(max((x - xs) / (xe - xs), 0), 1)

    for xa, xb, y in stairs_steps('stairs'):
        for z, toward in ((z0, (0, 0, 1)), (z1, (0, 0, -1))):
            side_strip(m, xa, xb, z, ceiling(xa), ceiling(xb), y, toward, 'concrete')
        face(m, [(xa, ceiling(xa), z0), (xb, ceiling(xb), z0), (xb, ceiling(xb), z1), (xa, ceiling(xa), z1)], (0, 1, 0), 'ceiling')
    room_side(m, 'stairs', 'x0', ya - H, ya, 'concrete')
    room_side(m, 'stairs', 'x1', yb - H, yb, 'concrete')
    door_frames(m, 'stairs')
    bake_light(m, LAMPS['stairs'], AMBIENT['stairs'])
    return m


def build_lowhall():
    """The gray concrete hall of the lower floor (brown doors)"""
    m = build_room('lowhall', 'concrete', 'concrete', 'ceiling')
    door_frames(m, 'lowhall')
    boxes = crates(m, [(3200, 240, 50), (3220, 1120, 60), (3160, 1140, 50)], LOWER)
    m.decal([(3280, LOWER - 152, 760), (3280, LOWER - 152, 720), (3280, LOWER - 112, 720), (3280, LOWER - 112, 760)], 'biohaz')
    bake_light(m, LAMPS['lowhall'], AMBIENT['lowhall'], boxes)
    return m


def tank(m, cx, cz):
    """A tank of the bottling room: an octagonal body on 4 legs, a flat top (the rendezvous with 006 among them)"""
    r = TANK_RADIUS
    ring = [(cx + r * math.cos(math.radians(22.5 + 45 * k)), cz + r * math.sin(math.radians(22.5 + 45 * k))) for k in range(8)]
    center = (cx, (TANK_TOP + TANK_BOTTOM) / 2, cz)
    mid = (TANK_TOP + TANK_BOTTOM) / 2
    for k in range(8):
        (ax, az), (bx, bz) = ring[k], ring[(k + 1) % 8]
        for yt, yb in ((TANK_TOP, mid), (mid, TANK_BOTTOM)):
            # (furniture: cut only on the near plane, the whole cut near the camera cost 4 ms of the master CPU)
            quad_out(m, [(ax, yt, az), (bx, yt, bz), (bx, yb, bz), (ax, yb, az)], center, 'tank', subdivision=0)
    # The top: 3 quads of the octagon
    top = [(x, TANK_TOP, z) for x, z in ring]
    for a, b, c, d in ((0, 1, 2, 3), (3, 4, 7, 0), (4, 5, 6, 7)):
        face(m, [top[a], top[b], top[c], top[d]], (0, -1, 0), 'tank', subdivision=0)
    for k in range(4):
        angle = math.radians(45 + 90 * k)
        lx, lz = cx + r * 0.7 * math.cos(angle), cz + r * 0.7 * math.sin(angle)
        m.box(lx - 4, TANK_BOTTOM, lz - 4, lx + 4, LOWER, lz + 4, 'frame', faces='xXzZ', subdivision=0)
    SOLIDS.append((cx, cz, r * 0.92, r * 0.92, LOWER, LOWER - TANK_TOP))
    return (cx - r, TANK_TOP, cz - r, cx + r, TANK_BOTTOM, cz + r)


def build_bottling():
    """The bottling room: 2 floors high, the tanks; on the north side the front of the walkway and the side under the
    stairs (the walkway, the stairs and the landing are cells of their own: their models)"""
    m = build_room('bottling', 'stone', 'floor', 'ceiling', sides=('x0', 'x1', 'z0'), period=GRID * 2)
    door_frames(m, 'bottling')
    wx0, wz0, wx1, wz1 = ROOMS['walkway']
    m.wall((wx1, wz0), (wx0, wz0), 0, LOWER, 'stone', period=GRID * 2)
    for xa, xb, y in stairs_steps('stairs2'):
        if y < LOWER:
            face(m, [(xa, y, wz0), (xb, y, wz0), (xb, LOWER, wz0), (xa, LOWER, wz0)], (0, 0, -1), 'stone')
    boxes = [tank(m, cx, cz) for cx, cz in TANKS]
    bake_light(m, LAMPS['bottling'], AMBIENT['bottling'], boxes)
    return m


def rail(m, x0, y0, x1, y1, z):
    """A railing along X on the plane z: posts, a top bar and a middle bar following the floor (y0 at x0, y1 at x1). The
    bars go from a post to the next one (through the posts, their faces 0.5 unit from the faces of the posts were sorted
    against them: see check_no_overlap())"""
    n = max(1, int(round((x1 - x0) / 120)))
    posts = [min(max(x0 + (x1 - x0) * k / n, x0 + 3), x1 - 3) for k in range(n + 1)]

    def floor_y(x):
        return y0 + (y1 - y0) * (x - x0) / (x1 - x0)

    for x in posts:
        y = floor_y(x)
        m.box(x - 2, y - RAIL_HEIGHT, z - 2, x + 2, y, z + 2, 'frame', faces='xXzZ', subdivision=0)
    for h, size in ((RAIL_HEIGHT, 3), (RAIL_HEIGHT / 2, 2)):
        for k in range(n):
            xa, xb = posts[k] + 2, posts[k + 1] - 2
            ya, yb = floor_y(xa) - h, floor_y(xb) - h
            face(m, [(xa, ya - size, z - 1.5), (xb, yb - size, z - 1.5), (xb, yb, z - 1.5), (xa, ya, z - 1.5)], (0, 0, -1), 'frame', subdivision=0)
            face(m, [(xa, ya - size, z + 1.5), (xb, yb - size, z + 1.5), (xb, yb, z + 1.5), (xa, ya, z + 1.5)], (0, 0, 1), 'frame', subdivision=0)
            face(m, [(xa, ya - size, z - 1.5), (xb, yb - size, z - 1.5), (xb, yb - size, z + 1.5), (xa, ya - size, z + 1.5)], (0, -1, 0), 'frame', subdivision=0)


def build_walkway():
    """The walkway of the bottling room at the level of the upper floor: a grating, the railing, the security door"""
    m = Mesh()
    x0, z0, x1, z1 = ROOMS['walkway']
    m.floor(x0, z0, x1, z1, 0, 'grate', sort='max')
    m.floor(x0, z0, x1, z1, -H, 'ceiling', up=False)
    m.wall((x1, z1), (x0, z1), -H, 0, 'stone')
    room_side(m, 'walkway', 'x0', -H, 0, 'stone')
    door_frames(m, 'walkway')
    rail(m, x0, 0, x1, 0, z0 + 2)
    # (as high as the railing: the shots over it at the floor of the bottling room went into an invisible box)
    SOLIDS.append(((x0 + x1) / 2, z0 + 2, (x1 - x0) / 2, 4, 0, RAIL_HEIGHT))
    bake_light(m, LAMPS['walkway'], AMBIENT['walkway'])
    return m


def build_stairs2():
    """The stairs from the walkway down to the floor of the bottling room, along its north wall: their railing"""
    m = Mesh()
    x0, z0, x1, z1 = ROOMS['stairs2']
    stairs_treads(m, 'stairs2', z0, z1)
    m.floor(x0, z0, x1, z1, -H, 'ceiling', up=False)
    for xa, xb, y in stairs_steps('stairs2'):
        side_strip(m, xa, xb, z1, -H, -H, y, (0, 0, -1), 'stone')
    rail(m, x0, 0, x1, LOWER, z0 + 2)
    # (the railing blocks in 4 parts, each one from the floor at its low end to the top of the railing at its high end)
    for k in range(4):
        xa, xb = x0 + (x1 - x0) * k / 4, x0 + (x1 - x0) * (k + 1) / 4
        y = LOWER * (k + 1) / 4
        SOLIDS.append(((xa + xb) / 2, z0 + 2, (xb - xa) / 2, 4, y, LOWER / 4 + RAIL_HEIGHT))
    bake_light(m, LAMPS['stairs2'], AMBIENT['stairs2'])
    return m


def build_landing():
    """The landing at the bottom of the stairs of the bottling room (the corner of the room)"""
    m = Mesh()
    x0, z0, x1, z1 = ROOMS['landing']
    m.floor(x0, z0, x1, z1, LOWER, 'floor', period=FLOOR_PERIOD['landing'], sort='max')
    m.floor(x0, z0, x1, z1, -H, 'ceiling', up=False)
    m.wall((x1, z1), (x0, z1), -H, LOWER, 'stone', period=GRID * 2)
    m.wall((x1, z0), (x1, z1), -H, LOWER, 'stone', period=GRID * 2)
    bake_light(m, LAMPS['landing'], AMBIENT['landing'])
    return m


def build_duct():
    """The ventilation duct up to its hole: 4 sides, the far end closed, open on the hole (the cell ducthole) and on the
    branch (on the right, the cell ductend)"""
    m = Mesh()
    x0, z0, x1, z1 = DUCT
    top, floor = DUCT_TOP, DUCT_FLOOR
    hx0, hz0, hx1, hz1 = DUCT_HOLE
    bx0, bz0, bx1, bz1 = DUCT_BRANCH
    # Floor and ceiling up to the hole, the 2 sides (the right one cut by the branch), the far end (z0)
    m.floor(x0, z0, x1, hz0, floor, 'ductfl', period=DUCT_CELL)
    m.floor(x0, z0, x1, hz0, top, 'duct', period=DUCT_CELL, up=False)
    m.wall((x0, hz0), (x0, z0), top, floor, 'duct', period=DUCT_CELL)
    m.wall((x1, z0), (x1, bz0), top, floor, 'duct', period=DUCT_CELL)
    m.wall((x1, bz1), (x1, hz0), top, floor, 'duct', period=DUCT_CELL)
    m.wall((x0, z0), (x1, z0), top, floor, 'duct', period=DUCT_CELL)
    duct_light(m)
    return m


def build_duct_hole():
    """The end of the duct over the hole: its ceiling, its 2 sides and its closed end (no floor: the hole)"""
    m = Mesh()
    x0, z0, x1, z1 = DUCT
    top, floor = DUCT_TOP, DUCT_FLOOR
    hx0, hz0, hx1, hz1 = DUCT_HOLE
    m.floor(x0, hz0, x1, z1, top, 'duct', period=DUCT_CELL, up=False)
    m.wall((x0, z1), (x0, hz0), top, floor, 'duct', period=DUCT_CELL)
    m.wall((x1, hz0), (x1, z1), top, floor, 'duct', period=DUCT_CELL)
    m.wall((x1, z1), (x0, z1), top, floor, 'duct', period=DUCT_CELL)
    duct_light(m)
    return m


def build_duct_branch():
    """The branch of the duct, on the right before the washroom: a dead end (floor, ceiling, 2 sides, the closed end)"""
    m = Mesh()
    top, floor = DUCT_TOP, DUCT_FLOOR
    bx0, bz0, bx1, bz1 = DUCT_BRANCH
    m.floor(bx0, bz0, bx1, bz1, floor, 'ductfl', period=DUCT_CELL)
    m.floor(bx0, bz0, bx1, bz1, top, 'duct', period=DUCT_CELL, up=False)
    m.wall((bx0, bz0), (bx1, bz0), top, floor, 'duct', period=DUCT_CELL)
    m.wall((bx1, bz0), (bx1, bz1), top, floor, 'duct', period=DUCT_CELL)
    m.wall((bx1, bz1), (bx0, bz1), top, floor, 'duct', period=DUCT_CELL)
    duct_light(m)
    return m


def duct_light(m):
    """The light of the duct, the same for its 2 models (no seam at the branch): bright cold steel, a
    lamp at the start, one at the branch, the light of the washroom through the hole"""
    x0, z0, x1, z1 = DUCT
    bx0, bz0, bx1, bz1 = DUCT_BRANCH
    top = DUCT_TOP
    bake_light(m, [(150, top + 10, z0 + 60, 220, (0.30, 0.31, 0.33)),
                   (190, top + 10, (bz0 + bz1) / 2, 220, (0.26, 0.27, 0.29)),
                   (150, -H + 30, 50, 160, (0.30, 0.31, 0.33))], (0.30, 0.31, 0.33))


def build_hinged_door_model(name, width, top, bottom, tex, shade, mirror_back=True, mirror_front=False, strips=1, lock=None):
    """A hinged door: the hinge is the Y axis of the model (x = 0), the panel goes to x = width, 4 thick, from top to
    bottom (Saturn Y: top < bottom <= 0). Turned by the game around its hinge"""
    m = Mesh()
    # (the 2 faces: the texture of the back one mirrored, the push plate on the free edge on both sides)
    # (mirror_front=True: the doors of the stalls, their lock on the free edge of both faces: on the left seen from the
    # room (the face Z, as the texture is), on the right seen from inside the stall (the face z, mirrored))
    back = tex
    front = tex
    if mirror_back or mirror_front:
        if tex + '_m' not in TEXTURES:
            save_texture(tex + '_m', TEXTURES[tex][:, ::-1])
    if mirror_back:
        back = tex + '_m'
    if mirror_front:
        front = tex + '_m'
    if strips <= 1:
        m.box(0, top, -2, width, bottom, 2, front, faces='z', subdivision=0, split=False)
        m.box(0, top, -2, width, bottom, 2, back, faces='Z', subdivision=0, split=False)
    else:
        # (strips: each one sorted by its own center. A door of a stall open along a partition: the whole panel had its
        # center farther than the front part of the partition, drawn over it, the door went into the partition. lock:
        # the texture of the whole door (its lock on the free edge), painted on the strips as a decal)
        for k in range(strips):
            xa, xb = width * k / strips, width * (k + 1) / strips
            m.box(xa, top, -2, xb, bottom, 2, tex, faces='zZ', subdivision=0, split=False)
        if lock is not None:
            # (the whole door painted over its strips: each strip gets its part of the texture; seen from the room (the
            # face Z, toward +Z) the lock on the free edge is on the left, the texture as it is; from inside (the face
            # z) it is on the right, the texture mirrored)
            if lock + '_m' not in TEXTURES:
                save_texture(lock + '_m', TEXTURES[lock][:, ::-1])
            m.decal([(width, top, 2), (0, top, 2), (0, bottom, 2), (width, bottom, 2)], lock)
            m.decal([(0, top, -2), (width, top, -2), (width, bottom, -2), (0, bottom, -2)], lock + '_m')
    # (the edges sorted with their farthest point: seen from above (from the duct), the long thin edge of a door sorted with
    # its center was drawn over the face of the partition in front of it, a dark slit on the partition)
    m.box(0, top, -2, width, bottom, 2, 'frame', faces='xX', subdivision=0, split=False, sort='max')
    m.box(0, top, -2, width, bottom, 2, 'frame', faces='yY', subdivision=0, split=False)
    for q in m.quads:
        q[3] = [shade] * 4
    return m


def check_outward(m, parts):
    """Check that the quads of closed parts face outward: the normal of each quad goes away from the center of its part.
    parts: (first quad, end, center). Stops the build on a face turned inward (it would not be drawn: a hole)"""
    for first, end, center in parts:
        for k in range(first, end):
            corners = m.quads[k][0]
            d = np.mean(np.array(corners, dtype=float), axis=0) - np.array(center, dtype=float)
            if float(np.dot(quad_normal(corners), d)) <= 0:
                raise SystemExit('quad %d of the model faces inward: %s' % (k, corners))


def build_pistol():
    """The silenced pistol in the hand of the agent (first person): a dark blue
    steel slide with beveled edges, a long silencer, the grip going out of the screen, the right hand low on the grip.
    The model is in front of the camera (X right, Y down, Z forward, units of the world); the origin is the trigger"""
    m = Mesh()
    gun = 'gun'
    skin = 'skin'
    parts = []

    def box(x0, y0, z0, x1, y1, z1, tex, faces):
        first = len(m.quads)
        m.box(x0, y0, z0, x1, y1, z1, tex, faces=faces, subdivision=0)
        parts.append((first, len(m.quads), ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)))

    def extrude(profile, z0, z1, tex, back=True, front=True, unlit=None):
        """A prism along Z: the profile is a list of (x, y) clockwise seen from behind (from the camera). The sides face
        outward, the back cap (z0) faces the camera, the front cap (z1) faces away"""
        n = len(profile)
        first = len(m.quads)
        opts = dict(subdivision=0)
        if unlit is not None:
            opts['unlit'] = unlit
        for k in range(n):
            (ax, ay), (bx, by) = profile[k], profile[(k + 1) % n]
            m.quad([(bx, by, z0), (ax, ay, z0), (ax, ay, z1), (bx, by, z1)], tex, **opts)
        # The caps: fans of quads (the profile has 4, 6 or 8 points)
        for k in range(0, n - 2, 2):
            a, b, c, d = profile[0], profile[k + 1], profile[k + 2], profile[(k + 3) % n]
            if back:
                m.quad([(a[0], a[1], z0), (b[0], b[1], z0), (c[0], c[1], z0), (d[0], d[1], z0)], tex, **opts)
            if front:
                m.quad([(d[0], d[1], z1), (c[0], c[1], z1), (b[0], b[1], z1), (a[0], a[1], z1)], tex, **opts)
        parts.append((first, len(m.quads), (sum(x for x, _ in profile) / n, sum(y for _, y in profile) / n, (z0 + z1) / 2)))

    def ring(x, y, r, sides=8):
        return [(x + r * math.cos(2 * math.pi * (k + 0.5) / sides), y + r * math.sin(2 * math.pi * (k + 0.5) / sides)) for k in range(sides)]

    # The slide: 3.4 wide, from y -4.6 (top) to -1.6, beveled top edges, z -4 (the back, toward the camera) to 14
    slide = [(-1.7, -1.6), (-1.7, -3.9), (-1.0, -4.6), (1.0, -4.6), (1.7, -3.9), (1.7, -1.6)]
    extrude(slide, -4, 14, gun)
    # The frame under the slide (the trigger at the origin), the front of the frame holds the barrel
    extrude([(-1.5, -1.6), (-1.5, 1.0), (1.5, 1.0), (1.5, -1.6)][::-1], -4, 10, gun)
    # The rear sight and the front sight
    box(-0.6, -5.4, -4, 0.6, -4.6, -2.5, gun, 'xXyz')
    box(-0.4, -5.4, 12, 0.4, -4.6, 13.5, gun, 'xXyz')
    # The barrel, then the silencer (a long tube, a little darker at its end)
    extrude(ring(0, -3.1, 0.9), 14, 16.5, gun, back=False)
    extrude(ring(0, -3.1, 1.6), 16.5, 34, gun, unlit=None)
    # The trigger guard (thin bars) and the trigger, in front of the hand
    box(-1.0, 1.0, 3.0, 1.0, 3.5, 3.8, gun, 'xXzZ')
    box(-1.0, 3.5, 0.6, 1.0, 4.2, 3.8, gun, 'xXYzZ')
    box(-0.4, 1.0, 0.6, 0.4, 3.2, 1.3, gun, 'xXzZ')
    # The hand: a short block of skin around the grip, under the frame, little of it seen (in the aiming
    # mode the weapon goes up, a tall block filled the bottom of the screen; the
    # grip is not modeled: inside the hand, its faces were sorted over the hand when the agent aims up, black holes in it)
    # (down to y 16: out of the bottom of the screen at any pitch, the bottom of a shorter block was seen when the agent
    # looked down, a hand cut off)
    box(-2.4, 1.0, -6.0, 2.8, 16.0, 0.6, skin, 'xXyYzZ')
    # Every face must look outward (the back faces are not drawn): checked against the center of its part
    check_outward(m, parts)
    # (half size: the pistol is thin and long on the screen)
    for q in m.quads:
        q[0] = [(x * 0.5, y * 0.5, z * 0.5) for x, y, z in q[0]]
    for q in m.quads:
        if q[3] is not None:
            continue
        n = quad_normal(q[0])
        # A light from the top, a little from the left and behind (the room behind the agent): the blued steel is dark
        # with a cold highlight on the top, the skin lighter
        shade = max(0.0, float(np.dot(n, np.array((-0.45, -0.75, -0.5)))))
        if q[1] == gun:
            # (glossy black: very dark, a gray highlight on the faces toward the light)
            light = 0.05 + 0.40 * shade ** 3
            q[3] = [(light, light, light * 1.04)] * 4
        else:
            light = 0.34 + 0.24 * shade
            q[3] = [(light, light * 0.98, light * 0.95)] * 4
    return m



def build_rifle_floor():
    """The rifle dropped on the floor by a dead guard: one quad lying on the floor, the rifle seen from above in its texture
    (the 3D rifle on the floor, about 100 polygons each, up to 8 of them, made the frame rate fall). The size and the axes of
    the lying 3D rifle: along Z from the stock (-17) to the muzzle (34.5), its width along X"""
    m = Mesh()
    y = -0.6
    face(m, [(-7.2, y, -17.0), (-7.2, y, 34.5), (2.3, y, 34.5), (2.3, y, -17.0)], (0, -1, 0), 'riflef', subdivision=0, sort='min')
    for q in m.quads:
        q[3] = [(0.5, 0.5, 0.5)] * 4
    return m


def build_rifle(world=False):
    """The rifle (an AK-47), slim and rounded: the black receiver with its
    rounded cover, the wooden handguard around the barrel and the wooden cover of the gas tube over it (round, light
    wood), the long thin barrel and its front sight, the gas block, the curved magazine, the grip. First person
    (world=False): in front of the camera like the pistol (X right, Y down, Z forward, the origin at the grip), no hand
    seen. world=True: the same rifle lying on the floor (dropped by a dead guard),
    fewer sides"""
    m = Mesh()
    parts = []
    sides = 6 if world else 8
    gun, wood = 'gun', 'kwood'

    def ring(x, y, rx, ry, n):
        # (clockwise seen from behind, from the camera: x right, y down)
        return [(x + rx * math.cos(2 * math.pi * (k + 0.5) / n), y + ry * math.sin(2 * math.pi * (k + 0.5) / n)) for k in range(n)]

    def extrude(profile, z0, z1, tex, back=True, front=True, scale1=1.0):
        """A prism along Z (the profile clockwise seen from behind), its front scaled by scale1 around its center (a
        taper). The sides face outward, the back cap faces the camera, the front cap away"""
        n = len(profile)
        first = len(m.quads)
        cx = sum(x for x, _ in profile) / n
        cy = sum(y for _, y in profile) / n
        front_profile = [(cx + (x - cx) * scale1, cy + (y - cy) * scale1) for x, y in profile]
        for k in range(n):
            (ax, ay), (bx, by) = profile[k], profile[(k + 1) % n]
            (fx, fy), (gx, gy) = front_profile[k], front_profile[(k + 1) % n]
            m.quad([(bx, by, z0), (ax, ay, z0), (fx, fy, z1), (gx, gy, z1)], tex, subdivision=0)
        for k in range(0, n - 2, 2):
            ids = (0, k + 1, k + 2, (k + 3) % n)
            if back:
                m.quad([(profile[i][0], profile[i][1], z0) for i in ids], tex, subdivision=0)
            if front:
                m.quad([(front_profile[i][0], front_profile[i][1], z1) for i in ids[::-1]], tex, subdivision=0)
        parts.append((first, len(m.quads), (cx, cy, (z0 + z1) / 2)))

    def loft(levels, tex, top=False, bottom=True):
        """A bent bar going down: levels = [(y, z_back, z_front)] (its width 1.8): the sides between 2 levels, the bottom
        cap (the magazine, the grip)"""
        first = len(m.quads)
        w = 0.9
        for (ya, ba, fa), (yb, bb, fb) in zip(levels, levels[1:]):
            m.quad([(-w, ya, fa), (-w, ya, ba), (-w, yb, bb), (-w, yb, fb)], tex, subdivision=0)
            m.quad([(w, ya, ba), (w, ya, fa), (w, yb, fb), (w, yb, bb)], tex, subdivision=0)
            m.quad([(-w, ya, ba), (w, ya, ba), (w, yb, bb), (-w, yb, bb)], tex, subdivision=0)
            m.quad([(w, ya, fa), (-w, ya, fa), (-w, yb, fb), (w, yb, fb)], tex, subdivision=0)
        y, b, f = levels[-1]
        if bottom:
            m.quad([(-w, y, b), (w, y, b), (w, y, f), (-w, y, f)], tex, subdivision=0)
        if top:
            y, b, f = levels[0]
            m.quad([(-w, y, f), (w, y, f), (w, y, b), (-w, y, b)], tex, subdivision=0)
        cy = sum(l[0] for l in levels) / len(levels)
        cz = sum((l[1] + l[2]) / 2 for l in levels) / len(levels)
        parts.append((first, len(m.quads), (0.0, cy, cz)))

    # The receiver: narrow, its cover rounded on top
    extrude([(-1.05, 1.4), (-1.05, -0.4), (-0.75, -1.1), (0.0, -1.35), (0.75, -1.1), (1.05, -0.4), (1.05, 1.4), (0.0, 1.55)],
            -5, 9, gun)
    # The rear sight on the cover, at its front
    extrude([(-0.5, -1.3), (-0.5, -1.9), (0.5, -1.9), (0.5, -1.3)], 6.5, 8.5, gun, back=True, front=True)
    # The wooden handguard around the barrel (round, a little thinner at its front) and the wooden cover of the gas tube
    extrude(ring(0, 0.25, 1.15, 1.2, sides), 9, 18, wood, back=False, scale1=0.9)
    extrude(ring(0, -1.35, 0.72, 0.62, sides - 2), 9.3, 17, wood, scale1=0.92)
    # The gas tube out of its cover, the gas block, the barrel, the front sight, the muzzle
    extrude(ring(0, -1.35, 0.36, 0.36, 6), 17, 21.5, gun, back=False)
    extrude([(-0.55, 0.4), (-0.55, -1.8), (0.55, -1.8), (0.55, 0.4)], 21.5, 23, gun)
    extrude(ring(0, 0.0, 0.34, 0.34, 6), 18, 33, gun, back=False, front=False)
    extrude([(-0.2, -0.2), (-0.2, -2.3), (0.2, -2.3), (0.2, -0.2)], 31, 32, gun)
    extrude(ring(0, 0.0, 0.48, 0.48, 6), 33, 34.5, gun, back=True)
    # The curved magazine: forward as it goes down
    loft([(1.4, 3.6, 6.6), (3.4, 4.1, 7.3), (5.4, 5.0, 8.4), (7.2, 6.3, 9.8)], gun)
    # The grip (slanted back) and the trigger guard
    loft([(1.5, -1.6, 0.6), (4.0, -2.5, -0.2), (6.2, -3.2, -0.8)], wood)
    extrude([(-0.35, 2.4), (-0.35, 1.5), (0.35, 1.5), (0.35, 2.4)], 0.6, 3.6, gun)
    # The stock behind (out of the view in the hands)
    extrude([(-0.9, 2.4), (-0.9, -0.9), (0.9, -0.9), (0.9, 2.4)], -17, -5, wood, front=False)
    check_outward(m, parts)
    for q in m.quads:
        if q[3] is not None:
            continue
        n = quad_normal(q[0])
        # (the light from the top left, over the shoulder: the round parts get a gradient, the black steel a sheen)
        shade = max(0.0, float(np.dot(n, np.array((-0.45, -0.75, -0.5)))))
        if q[1] == gun:
            light = 0.06 + 0.42 * shade ** 2.5
            q[3] = [(light, light, light * 1.04)] * 4
        else:
            light = 0.40 + 0.32 * shade
            q[3] = [(light, light, light)] * 4
    if world:
        # (lying on its side on the floor: turned around Z (the barrel along Z), the bottom of the model at y = 0)
        for q in m.quads:
            q[0] = [(-y, x - 1.6, z) for x, y, z in q[0]]
    return m


def demo_define(name):
    """A value of demo.h (#define NAME (value))"""
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'demo.h')) as f:
        found = re.search(r'#\s*define\s+%s\s+\(?(\d+)\)?' % name, f.read())
    if not found:
        raise SystemExit('demo.h: %s not found' % name)
    return int(found.group(1))


def check_cd_sort():
    """Every file of the disc in cd.sort (the order of the files on the disc: the order the game reads them at the start,
    read ahead by the engine without a seek; in the order of the names, a file read back was a seek: 14 s of loading
    instead of 4). Its order: the files opened by the game, see JO_FS_TRACE in jo_engine/fs.c (build with
    _CCFLAGS=-DJO_FS_TRACE=300, read __jo_fs_trace in a save state). A new file: put it where the game reads it"""
    path = os.path.join(DEMO, 'cd.sort')
    listed = set()
    with open(path) as f:
        for line in f:
            if line.strip():
                listed.add(line.split()[0].replace('./cd/', ''))
    missing = sorted(n for n in os.listdir(CD) if n not in listed and not n.endswith('.TXT'))
    # (0.bin: the program, made by the build after the assets, see the makefile of the engine)
    gone = sorted(n for n in listed if not os.path.exists(os.path.join(CD, n)) and n != '0.bin')
    if missing or gone:
        raise SystemExit('cd.sort: not listed %s, listed but not on the disc %s' % (missing, gone))


def check_stairs_walk(cells):
    """The agent walks up and down every stairs (see move() in player.c and __jo_cells_find() in cell.c): from the cell
    before it to the cell after it, at several speeds and starting points. The build stops if he is stuck somewhere: the
    slope of the stairs at the edge of a flat cell higher than his step (he stood at the top of the stairs of the bottling
    room, the walkway one step too high)"""
    radius, step, halvings = demo_define('PLAYER_RADIUS'), demo_define('PLAYER_STEP'), demo_define('MOVE_HALVINGS')
    walk = demo_define('WALK_SPEED')
    fps = 30

    def floor_of(c, x, z):
        x0, z0, x1, z1 = c['box']
        f0 = c['floor0']
        f1 = c.get('floor1', f0)
        if c.get('slope') == 'X':
            return int(f0 + (f1 - f0) * (min(max(int(x), x0), x1) - x0) // (x1 - x0))
        if c.get('slope') == 'Z':
            return int(f0 + (f1 - f0) * (min(max(int(z), z0), z1) - z0) // (z1 - z0))
        return f0

    def find(x, y, z, r):
        best = None
        for c in cells:
            x0, z0, x1, z1 = c['box']
            if int(x) - r < x0 or int(x) + r > x1 or int(z) - r < z0 or int(z) + r > z1:
                continue
            f = floor_of(c, x, z)
            if f < int(y) - step or (best is not None and f >= best):
                continue
            best = f
        return best

    for stairs in cells:
        if not stairs.get('slope'):
            continue
        axis = 0 if stairs['slope'] == 'X' else 1
        sx0, sz0, sx1, sz1 = stairs['box']
        lo, hi = (sx0, sx1) if axis == 0 else (sz0, sz1)
        across = ((sz0 + sz1) / 2.0) if axis == 0 else ((sx0 + sx1) / 2.0)
        for speed in (walk / fps, walk * 0.5 / fps, walk * 0.7 / fps):
            for start_offset in range(0, 22, 3):
                for direction in (1, -1):
                    pos = (lo - 60 - start_offset) if direction > 0 else (hi + 60 + start_offset)
                    goal = hi + 40 if direction > 0 else lo - 40

                    def at(p):
                        return (p, across) if axis == 0 else (across, p)
                    x, z = at(pos)
                    y = find(x, -10000, z, 0)
                    if y is None:
                        raise SystemExit('check_stairs_walk: %s: no floor at the start %s' % (stairs['name'], at(pos)))
                    for _ in range(400):
                        if (pos - goal) * direction >= 0:
                            break
                        moved = False
                        part = speed * direction
                        for k in range(halvings + 1):
                            x, z = at(pos + part)
                            if find(x, y, z, radius) is not None:
                                pos += part
                                moved = True
                                break
                            part /= 2.0
                        if not moved:
                            raise SystemExit('check_stairs_walk: the agent is stuck in %s at %s (floor %d, speed %.1f): the '
                                             'slope at the edge of the next cell is more than a step (%d) over the floor'
                                             % (stairs['name'], at(pos), y, speed, step))
                        x, z = at(pos)
                        y = find(x, y, z, 0)
                    else:
                        raise SystemExit('check_stairs_walk: %s: the agent never reached the end' % stairs['name'])


def build_level():
    """cd/LEVEL.JOL: the cells, their portals, the entities"""
    level = jo_level.Level('Laboratory')
    names = list(ROOMS.keys()) + ['duct', 'ducthole', 'ductend']
    cells = []
    for name in ROOMS:
        x0, z0, x1, z1 = ROOMS[name]
        cell = dict(name=name, model=name.upper()[:8] + '.JOM', box=(x0, z0, x1, z1), floor0=room_floor(name), portals=[])
        if name in STAIRS:
            # (the whole room: it overlaps the cells of the doors, the agent goes from one box to the other; where both
            # are under him, the higher floor: the flat one of the door at the top, the slope over the bottom landing)
            axis = STAIRS[name][0]
            cell.update(floor0=int(round(stairs_slope(name, x0))), floor1=int(round(stairs_slope(name, x1))), slope=axis.upper())
        cells.append(cell)
    x0, z0, x1, z1 = DUCT
    hx0, hz0, hx1, hz1 = DUCT_HOLE
    cells.append(dict(name='duct', model='DUCT.JOM', box=(x0, z0, x1, hz0), floor0=DUCT_FLOOR, portals=[]))
    # (its floor is the one of the washroom: the camera there is in the smallest model around it, see
    # __jo_cells_find_object(): falling through the hole, in it; standing in the washroom under it, in the washroom)
    cells.append(dict(name='ducthole', model='DUCTHOLE.JOM', box=(hx0, hz0, hx1, hz1), floor0=0, portals=[]))
    bx0, bz0, bx1, bz1 = DUCT_BRANCH
    # (its box goes over the duct: the agent walks from one box to the other, his circle in one of them)
    cells.append(dict(name='ductend', model='DUCTEND.JOM', box=(x0, bz0, bx1, bz1), floor0=DUCT_FLOOR, portals=[]))
    by_name = {c['name']: c for c in cells}
    # The doors: a portal on each side, and a small cell across the wall for the player to walk through
    depth = 50
    for a, b, axis, pos, c0, c1, fy, style in DOORS:
        if axis == 'x':
            quad = [(pos, fy, c0), (pos, fy, c1), (pos, fy - DOOR_HEIGHT, c1), (pos, fy - DOOR_HEIGHT, c0)]
            box = (pos - depth, c0, pos + depth, c1)
        else:
            quad = [(c0, fy, pos), (c1, fy, pos), (c1, fy - DOOR_HEIGHT, pos), (c0, fy - DOOR_HEIGHT, pos)]
            box = (c0, pos - depth, c1, pos + depth)
        by_name[a]['portals'].append(dict(cell=b, points=quad))
        by_name[b]['portals'].append(dict(cell=a, points=quad))
        cells.append(dict(name='door_%s_%s' % (a, b), box=box, floor0=fy,
                          portals=[dict(cell=a, points=quad), dict(cell=b, points=quad)]))
    for a, b, axis, pos, depth, fy in LINKS:
        if axis == 'x':
            za, zb = max(ROOMS[a][1], ROOMS[b][1]), min(ROOMS[a][3], ROOMS[b][3])
            quad = [(pos, LOWER, za), (pos, LOWER, zb), (pos, -H, zb), (pos, -H, za)]
            box = (pos - depth, za, pos + depth, zb)
        else:
            xa, xb = max(ROOMS[a][0], ROOMS[b][0]), min(ROOMS[a][2], ROOMS[b][2])
            quad = [(xa, LOWER, pos), (xb, LOWER, pos), (xb, -H, pos), (xa, -H, pos)]
            box = (xa, pos - depth, xb, pos + depth)
        cells.append(dict(name='link_%s_%s' % (a, b), box=box, floor0=fy,
                          portals=[dict(cell=a, points=quad), dict(cell=b, points=quad)]))
    # The windows: a portal between the 2 rooms (no cell: nobody goes through)
    for a, b, axis, pos, c0, c1, bottom, top in WINDOWS:
        if axis == 'x':
            quad = [(pos, bottom, c0), (pos, bottom, c1), (pos, top, c1), (pos, top, c0)]
        else:
            quad = [(c0, bottom, pos), (c1, bottom, pos), (c1, top, pos), (c0, top, pos)]
        by_name[a]['portals'].append(dict(cell=b, points=quad))
        by_name[b]['portals'].append(dict(cell=a, points=quad))
    # The openings between the parts of the bottling room (the walkway, the stairs, the landing over its floor)
    for a, b, axis, pos, c0, c1, bottom, top in OPENINGS:
        if axis == 'x':
            quad = [(pos, bottom, c0), (pos, bottom, c1), (pos, top, c1), (pos, top, c0)]
        else:
            quad = [(c0, bottom, pos), (c1, bottom, pos), (c1, top, pos), (c0, top, pos)]
        by_name[a]['portals'].append(dict(cell=b, points=quad))
        by_name[b]['portals'].append(dict(cell=a, points=quad))
    # The hole of the duct: its end (the cell ducthole) sees the washroom through it and the duct through its section; the
    # washroom sees the end of the duct, then the duct
    hole = [(hx0, -H, hz0), (hx1, -H, hz0), (hx1, -H, hz1), (hx0, -H, hz1)]
    section = [(hx0, DUCT_FLOOR, hz0), (hx1, DUCT_FLOOR, hz0), (hx1, DUCT_TOP, hz0), (hx0, DUCT_TOP, hz0)]
    by_name['ducthole']['portals'].append(dict(cell='washroom', points=hole))
    by_name['washroom']['portals'].append(dict(cell='ducthole', points=hole))
    by_name['ducthole']['portals'].append(dict(cell='duct', points=section))
    by_name['duct']['portals'].append(dict(cell='ducthole', points=section))
    # The opening of the branch, on the right side of the duct
    side = [(bx0, DUCT_FLOOR, bz0), (bx0, DUCT_FLOOR, bz1), (bx0, DUCT_TOP, bz1), (bx0, DUCT_TOP, bz0)]
    by_name['duct']['portals'].append(dict(cell='ductend', points=side))
    by_name['ductend']['portals'].append(dict(cell='duct', points=side))
    for room, key, m0, m1, mtop, mbottom in MIRRORS:
        rx0, rz0, rx1, rz1 = ROOMS[room]
        pos = {'x0': rx0, 'x1': rx1, 'z0': rz0, 'z1': rz1}[key]
        if key[0] == 'x':
            quad = [(pos, mbottom, m0), (pos, mbottom, m1), (pos, mtop, m1), (pos, mtop, m0)]
        else:
            quad = [(m0, mbottom, pos), (m1, mbottom, pos), (m1, mtop, pos), (m0, mtop, pos)]
        by_name[room]['portals'].append(dict(cell=room, kind='MIRROR', points=quad))
    check_stairs_walk(cells)
    check_barrels()
    level.add_cells(cells)
    entities = [
        # (on the right of the duct: the drop at its end lands beside the toilet of the stall)
        dict(name='start', cls='spawn', x=150, y=DUCT_FLOOR, z=-440, yaw=0, flags=['HIDDEN']),
    ]
    # The doors between the rooms: hinged doors (the hinge on the north post, they open away from the agent; the class
    # door also makes sliding doors: the property slide, no hinge). The entities are not SOLID: the class door blocks the
    # agent itself when the door is closed
    # (the doors: a recording, opening and closing; the closing sound starts so that the door hits its frame when it is
    # closed, see tools/make_sounds.py; the doors of the stalls: the same sound higher, lighter doors)
    import make_sounds
    close_hit = make_sounds.door_close_hit_ms() if os.path.exists(make_sounds.DOOR_RECORDING) else 331
    for a, b, axis, pos, c0, c1, fy, style in DOORS:
        # The standard door (brushed steel, hinged; brown on the lower floor); the panel fills the
        # opening: no light between it and the frame. The hinge: on the north post of a wall along Z (the panel goes
        # south, yaw 90), on the west post of a wall along X (the panel goes east, yaw 0)
        if style.startswith('slide'):
            # The doors of the airlock: the middle of the doorway, the panel along Z (yaw 90: the X of the model is -Z),
            # it slides to +Z (side -1), into the pocket of the wall
            entities.append(dict(name='door_%s_%s' % (a, b), cls='door', model='SDOOR.JOM', x=pos, y=fy, z=(c0 + c1) // 2, yaw=90,
                                 radius=DOOR_WIDTH // 2, height=DOOR_HEIGHT,
                                 props=dict(slide=DOOR_WIDTH, side=-1, time=1200, sound='SLIDE.WAV')))
            continue
        x, z, yaw = (pos, c1, 90) if axis == 'x' else (c0, pos, 0)
        entities.append(dict(name='door_%s_%s' % (a, b), cls='door', model='BDOOR.JOM' if style == 'brown' else 'HDOOR.JOM',
                             x=x, y=fy, z=z, yaw=yaw, radius=DOOR_WIDTH, height=DOOR_HEIGHT,
                             props=dict(hinge=1, time=600, sound='DOOROPEN.WAV', sound_close='DOORCLOS.WAV', close_hit=close_hit)))
    # The doors of the stalls: hinged on the left post seen from the room (x = sx0 + STALL_HINGE), closed along the front (z = STALL_DEPTH - 2).
    # The yaw of a hinged door is the direction of its closed panel from the hinge: the panel is the X axis of the model,
    # yaw 0 puts it along +X, yaw 90 along -Z (the washroom door hangs from the north post and goes south)
    for k, (sx0, sz0, sx1, sz1) in enumerate(STALLS):
        # (the hinge 2 units from the post: the panel is 4 thick, turned its back edge went 2 units into the post)
        entities.append(dict(name='stall%d' % (k + 1), cls='door', model='STALLD.JOM', x=sx0 + STALL_HINGE, y=0, z=sz1 - 2, yaw=0,
                             radius=STALL_DOOR_WIDTH, height=STALL_HEIGHT, props=dict(hinge=1, angle=75, time=500, sound='DOOROPEN.WAV', sound_close='DOORCLOS.WAV',
                                                                                    close_hit=close_hit, pitch=125)))
    # The glows of the lamps (the class lamp, see doors.c)
    for room, count in GLOW_LAMPS.items():
        for k, (lx, ly, lz, radius, color) in enumerate(LAMPS[room][:count]):
            r, g, b = (int(min(255, 120 + c * 260)) for c in color)
            props = dict(r=r, g=g, b=b, size=46, intensity=170)
            if room in ALARM_ROOMS:
                props = dict(r=255, g=36, b=20, size=55, intensity=230, alarm=1)
            entities.append(dict(name='lamp_%s%d' % (room, k + 1), cls='lamp', x=int(lx), y=int(room_top(room) + 4), z=int(lz),
                                 props=props))
    # The explosive barrels (the class explosive of the engine)
    for k, (bx, bz, fy) in enumerate(BARRELS):
        entities.append(dict(name='barrel%d' % (k + 1), cls='explosive', model='BARREL.JOM', x=bx, y=fy, z=bz, yaw=(k * 37) % 360,
                             health=40, flags=['SOLID'], radius=BARREL_RADIUS, height=BARREL_HEIGHT,
                             props=dict(damage=120, radius=230, delay=160, size=180, sound='EXPLODE.PCM', dead_model='BARRELD.JOM',
                                        far_model='BARRELF.JOM', far_distance=1000, draw_distance=1800)))
    # The monitors of the computers (the class breakable of props.c: the screen animated, broken by a shot, see computer())
    # and the glassware of the benches (broken by a shot: the pieces fly)
    for k, (mx, my, mz, yaw) in enumerate(MONITORS):
        entities.append(dict(name='monitor%d' % (k + 1), cls='breakable', model='MONITOR.JOM', x=int(mx), y=int(my), z=int(mz), yaw=yaw,
                             health=1, flags=['SOLID'], radius=18, height=40,
                             props=dict(dead_model='MONITORD.JOM', frames=3, shards=10, draw_distance=1400)))
    for k, (model, gx, gy, gz, yaw) in enumerate(GLASSWARE):
        entities.append(dict(name='%s%d' % (model, k + 1), cls='breakable', model=model.upper() + '.JOM', x=int(gx), y=int(gy), z=int(gz),
                             yaw=yaw, health=1, flags=['SOLID'], radius=14 if model == 'tubes' else 7, height=17 if model == 'tubes' else 25,
                             props=dict(dead_model=model.upper() + 'D.JOM', shards=6, draw_distance=900)))
    # The panes of the windows (the class glass: broken by a shot)
    for k, (a, b, axis, pos, c0, c1, bottom, top) in enumerate(WINDOWS):
        x, z, yaw = (pos, (c0 + c1) // 2, 90) if axis == 'x' else ((c0 + c1) // 2, pos, 0)
        entities.append(dict(name='glass%d' % (k + 1), cls='glass', model='GLASS.JOM', x=x, y=bottom, z=z, yaw=yaw,
                             radius=(c1 - c0) // 2, height=bottom - top, props=dict(width=c1 - c0, height=bottom - top)))
    # The seats of the toilets: black rings standing on the back of the bowls, sprites turned toward the agent (the class
    # billboard of doors.c)
    for k, (sx0, sz0, sx1, sz1) in enumerate(STALLS):
        entities.append(dict(name='seat%d' % (k + 1), cls='billboard', x=(sx0 + sx1) // 2, y=-38, z=sz0 + 8, radius=20,
                             height=36, props=dict(sprite='SEAT.BIN', size_x=30, size_y=36)))
    # The guards (tools/make_guard.py, guards.c; the model looks toward +Z turned by the yaw): one in the stall on the right
    # of the one where the agent lands (he waits, facing his door), one at the basins (his back to the room), 2 in the
    # storage room (one patrols)
    # (each guard his own face: make_guard.FACES, see guard_faces.h)
    import make_guard
    faces = iter(range(len(make_guard.skin.FACES) * 8))
    def guard(name, x, z, yaw, patrol=(0, 0), y=0):
        return dict(name=name, cls='guard', model='GUARD.JOM', x=x, y=y, z=z, yaw=yaw, health=100, flags=['SOLID'], radius=20,
                    height=132, props=dict(animation='idle', draw_distance=2000, far_model='GUARDF.JOM', far_distance=320, patrol_x=patrol[0],
                                           patrol_z=patrol[1], face=next(faces) % len(make_guard.skin.FACES)))
    sx0, sz0, sx1, sz1 = STALLS[2]
    entities += [guard('guard_stall', (sx0 + sx1) // 2, sz0 + 80, 0),
                 guard('guard_basins', 560, 730, 0),
                 guard('guard_storage1', 1950, 450, 270, patrol=(300, 100)),
                 guard('guard_storage2', 2150, 660, 300),
                 # The new rooms: the hall, the laboratory, the room of the guards, the gallery, the lower floor, the bottling
                 # room (its floor and its walkway)
                 guard('guard_hall', 1280, 1100, 180, patrol=(0, 300)),
                 guard('guard_lab1', 1500, 1720, 270),
                 guard('guard_lab2', 1120, 1500, 90, patrol=(240, 0)),
                 guard('guard_room1', 2000, 1100, 0, patrol=(200, 0)),
                 guard('guard_room2', 2200, 1440, 200),
                 guard('guard_gallery', 2900, 1440, 270),
                 guard('guard_low', 3160, 900, 180, patrol=(0, 150), y=LOWER),
                 guard('guard_bottling1', 3660, 860, 0, y=LOWER),
                 guard('guard_bottling2', 3920, 860, 300, patrol=(200, 0), y=LOWER),
                 guard('guard_bottling3', 4120, 520, 90, y=LOWER),
                 guard('guard_walkway', 3560, 1440, 90),
                 guard('guard_office', 1600, 1200, 270)]
    # The scientists (the class scientist of guards.c: they walk, raise their hands when the agent aims at them, run away
    # from the shots): between the benches of the laboratory, along its east side, in the office
    def scientist(name, x, z, yaw, patrol=(0, 0), y=0):
        return dict(name=name, cls='scientist', model='SCIENT.JOM', x=x, y=y, z=z, yaw=yaw, health=60, flags=['SOLID'], radius=20,
                    height=132, props=dict(animation='idle', draw_distance=2000, far_model='SCIENTF.JOM', far_distance=320, patrol_x=patrol[0],
                                           patrol_z=patrol[1], face=next(faces) % len(make_guard.skin.FACES)))
    entities += [scientist('scientist_lab1', 1100, 1710, 90, patrol=(280, 0)),
                 scientist('scientist_lab2', 1600, 1440, 0, patrol=(0, 260)),
                 scientist('scientist_office', 1460, 1000, 0, patrol=(0, 220))]
    # The furniture: invisible solid boxes
    for k, solid in enumerate(SOLIDS):
        # (center x, center z, half x, half z[, floor, height])
        sx, sz, hx, hz = solid[:4]
        sy, height = solid[4:6] if len(solid) > 4 else (0, 80)
        entities.append(dict(name='solid%d' % (k + 1), cls='entity', x=int(sx), y=int(sy), z=int(sz), flags=['SOLID', 'HIDDEN'],
                             radius=int(max(hx, hz)), height=int(height), props=dict(box_x=int(hx), box_z=int(hz))))
    check_guard_paths(entities)
    level.add_entities(entities)
    level.write(os.path.join(CD, 'LEVEL.JOL'))
    print('  LEVEL.JOL: %d cells, %d entities' % (len(cells), len(entities)))


def build_defs():
    """cd/DEFS.JOL: the weapons"""
    lv = jo_level.Level('Definitions')
    lv.add('WEAP', records=[dict(
        name=lv.string('Silenced pistol'), model=lv.string('PISTOL.JOM'),
        x=int(3 * 65536), y=int(6.1 * 65536), z=int(15 * 65536), muzzle_y=int(-1.55 * 65536), muzzle_z=int(17 * 65536),
        convergence=600, automatic=0, fire_delay=4, damage=40, spread=6, recoil=8, kick=1, magazine=7, ammo_type=0,
        ammo_start=93, ammo_max=800, reload_time=50, switch_time=12, flash=lv.string(''), flash_size=0, casing=1,
        sound_shot=lv.string('SHOT.WAV'), sound_reload=lv.string('RELOAD.WAV'), shot_volume=115),
        # The rifle of the guards (picked up on their bodies): automatic, 30 rounds, louder
        dict(name=lv.string('AK-47'), model=lv.string('RIFLE.JOM'),
             x=int(5.5 * 65536), y=int(7.0 * 65536), z=int(12 * 65536), muzzle_y=int(0 * 65536), muzzle_z=int(34.5 * 65536),
             convergence=3000, automatic=1, fire_delay=3, damage=34, spread=10, recoil=6, kick=1, magazine=30, ammo_type=1,
             ammo_start=0, ammo_max=400, reload_time=60, switch_time=12, flash=lv.string(''), flash_size=0, casing=1,
             sound_shot=lv.string('RIFLE.WAV'), sound_reload=lv.string('RELOAD.WAV'), shot_volume=120)])
    lv.write(os.path.join(CD, 'DEFS.JOL'))
    print('  DEFS.JOL: 2 weapons')


# ---------------------------------------------------------------------------
# Sprites of the HUD and the flashes (jo BIN images: width, height, RGB 555 little endian; the KEY color is transparent)
# ---------------------------------------------------------------------------
KEY = (255, 0, 255)


def save_bin(name, rgb):
    a = np.asarray(rgb).astype(np.uint16)
    h, w = a.shape[:2]
    px = 0x8000 | ((a[..., 2] >> 3) << 10) | ((a[..., 1] >> 3) << 5) | (a[..., 0] >> 3)
    key = 0x8000 | ((KEY[2] >> 3) << 10) | ((KEY[1] >> 3) << 5) | (KEY[0] >> 3)
    px[px == key] = 0
    with open(os.path.join(CD, name), 'wb') as f:
        f.write(struct.pack('<HH', w, h))
        f.write(px.astype('<u2').tobytes())


# The flame at the muzzle (some shots): its look. Its light (0: nothing) goes through the colors FLAME_COLORS, from the
# edge to the core; the pixels with less light than FLAME_CUT are transparent. It is drawn half transparent by the VDP1
# (twice: the whole flame, then its core smaller, the core is brighter), turned along the barrel seen: FLAME_ANGLES
# images turned beforehand (a turned sprite of the VDP1 draws some pixels twice: half transparent, dotted lines). To
# draw the flame by hand:
# models/flame_custom.png (its colors, black is transparent; the base of the flame at the bottom, the tip at the top),
# used instead of the flame made from these numbers
FLAME_SIZE = (24, 48)           # Pixels (width, height): the base at the bottom middle
FLAME_ANGLES = list(range(-80, 41, 20))  # Turns of the images (degrees, clockwise from up): cd/FLAME0.BIN ...
FLAME_BOX = 56                  # Size of the turned images (pixels, a multiple of 8, over the diagonal of FLAME_SIZE)
FLAME_COLORS = [(230, 120, 30), (250, 190, 60), (255, 235, 130), (255, 250, 210), (255, 255, 245)]
FLAME_CUT = 0.12
FLAME_TONGUES = 3               # Tongues of fire at the tip (their lengths at random, FLAME_SEED)
FLAME_WIDTH = 0.95              # Width of the body / width of the image
FLAME_SEED = 11
FLAME_LUT_COLORS = 14            # Colors of the images (16 colors sprites: 14 colors, the transparent one, the end code)


def flame_levels():
    """The levels of light (0 to 1) of the flame, height x width"""
    w, h = FLAME_SIZE
    rng = np.random.RandomState(FLAME_SEED)
    yy, xx = np.mgrid[0:h, 0:w].astype(float)
    # (v: 0 at the base, 1 at the tip; u: -1 to 1 across)
    v = 1 - (yy + 0.5) / h
    u = (xx + 0.5 - w / 2) / (w / 2)
    # The body: wide near the base (a short bright neck at the muzzle), narrowing to the tip, tongues near the tip
    width = FLAME_WIDTH * (np.sqrt(np.clip(v * 6, 0, 1)) * (1 - v) ** 0.7)
    tongue = np.zeros_like(v)
    for k in range(FLAME_TONGUES):
        center = rng.uniform(-0.45, 0.45)
        length = rng.uniform(0.55, 1.0)
        tongue = np.maximum(tongue, np.clip(1 - np.abs(u - center * v) / (0.12 + 0.2 * (1 - v)), 0, 1) * np.clip((length - v) / 0.25, 0, 1))
    body = np.clip(1 - np.abs(u) / np.maximum(width, 1e-3), 0, 1)
    light = np.maximum(body, tongue * 0.8 * (v > 0.3))
    # Brighter near the base, fading to the tip
    light *= np.clip(1.15 - v * 0.9, 0, 1)
    return np.clip(light, 0, 1) ** 1.2


def flame_sprite():
    """cd/FLAME0.BIN ...: the flame turned by each of FLAME_ANGLES (its middle at the middle of the image), 14 colors
    (loaded as 16 colors sprites, drawn half transparent); models/flame.png: a preview of the flame"""
    w, h = FLAME_SIZE
    custom = os.path.join(MODELS, 'flame_custom.png')
    if os.path.exists(custom):
        rgb = np.asarray(Image.open(custom).convert('RGB').resize((w, h), Image.LANCZOS), dtype=float)
        mask = rgb.max(axis=2) >= 24
    else:
        light = flame_levels()
        n = len(FLAME_COLORS) - 1
        f = np.clip((light - FLAME_CUT) / (1 - FLAME_CUT), 0, 1) * n
        i = np.minimum(f.astype(int), n - 1)
        a_ = np.array(FLAME_COLORS, dtype=float)
        t = (f - i)[..., None]
        rgb = a_[i] * (1 - t) + a_[i + 1] * t
        mask = light >= FLAME_CUT
    # (14 colors: the game loads the images in 16 colors, jo_sprite_add_bin_4bits_lut(), 4 times less VRAM; the turns keep
    # the colors, NEAREST)
    pixels = Image.fromarray(rgb[mask][None].astype(np.uint8), 'RGB').quantize(colors=FLAME_LUT_COLORS, method=Image.Quantize.MAXCOVERAGE,
                                                                                dither=Image.Dither.NONE)
    palette = np.array(pixels.getpalette()[:FLAME_LUT_COLORS * 3], dtype=float).reshape(-1, 3)
    rgb = rgb.copy()
    rgb[mask] = palette[np.asarray(pixels)[0]]
    Image.fromarray(np.where(mask[..., None], rgb, 0).astype(np.uint8)).save(os.path.join(MODELS, 'flame.png'))
    rgba = np.dstack([rgb, mask * 255.0]).astype(np.uint8)
    for k, angle in enumerate(FLAME_ANGLES):
        canvas = Image.new('RGBA', (FLAME_BOX, FLAME_BOX), (0, 0, 0, 0))
        canvas.paste(Image.fromarray(rgba, 'RGBA'), ((FLAME_BOX - w) // 2, (FLAME_BOX - h) // 2))
        # (PIL turns counter clockwise: the angle is clockwise on the screen; no blending of the edge, it would be dark)
        turned = np.asarray(canvas.rotate(-angle, resample=Image.NEAREST), dtype=float)
        out = turned[..., :3].copy()
        out[turned[..., 3] < 128] = KEY
        save_bin('FLAME%d.BIN' % k, out)


def hud_sprites():
    # (the HUD: the health and the ammunition, see HUD_FILES and hud.c)
    # The crosshair of the aiming mode: a red ring and a cross whose 4 arms go out of the ring (32 x 32)
    n = 32
    rgb = np.zeros((n, n, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx - 15.5, yy - 15.5)
    red = (225, 30, 30)
    rgb[(d >= 7.0) & (d < 8.6)] = red
    arm = ((np.abs(xx - 15.5) < 1) & (np.abs(yy - 15.5) > 3) & (np.abs(yy - 15.5) < 15)) | \
          ((np.abs(yy - 15.5) < 1) & (np.abs(xx - 15.5) > 3) & (np.abs(xx - 15.5) < 15))
    rgb[arm] = red
    save_bin('AIM.BIN', rgb)
    # The seat of the toilets: a black ring (the lid up, seen from the front), a soft highlight (32 x 32)
    n = 32
    rgb = np.zeros((n, n, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot((xx - 15.5) / 14.5, (yy - 15.5) / 15.5)
    ring = (d < 1.0) & (d > 0.52)
    rgb[ring] = (22, 22, 26)
    rgb[ring & (d > 0.9) & (yy < 12)] = (70, 70, 78)
    rgb[ring & (d < 0.62)] = (48, 48, 54)
    save_bin('SEAT.BIN', rgb)
    # The impact of a bullet on a wall: a round dark hole, a soft dark gray rim, a light edge at the
    # bottom (the lit side of the hole) (16 x 16, drawn 7 units wide)
    n = 16
    rgb = np.zeros((n, n, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx - 7.5, yy - 7.5)
    rgb[d < 7.2] = (95, 92, 90)
    rgb[d < 6.0] = (60, 58, 57)
    rgb[d < 4.6] = (22, 21, 21)
    rgb[(d >= 4.6) & (d < 6.0) & (yy > 9)] = (130, 128, 126)
    rgb[d < 2.6] = (8, 8, 8)
    save_bin('IMPACT.BIN', rgb)
    flame_sprite()
    # The bullet in flight (some shots): a small bright yellow dot with a white middle (8 x 8)
    n = 8
    rgb = np.zeros((n, n, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx - 3.5, yy - 3.5)
    rgb[d < 3.8] = (240, 200, 80)
    rgb[d < 2.2] = (255, 250, 220)
    save_bin('TRACER.BIN', rgb)
    # A drop of blood (8 x 8): dark red, a lighter middle
    n = 8
    rgb = np.zeros((n, n, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:n, 0:n]
    d = np.hypot(xx - 3.5, yy - 3.5)
    rgb[d < 3.8] = (120, 10, 8)
    rgb[d < 2.4] = (175, 20, 15)
    save_bin('BLOOD.BIN', rgb)
    # The flash of the rifles of the guards (16 x 16: a bright star, white in the middle)
    n = 16
    rgb = np.zeros((n, n, 3))
    rgb[:, :] = KEY
    yy, xx = np.mgrid[0:n, 0:n]
    dx, dy = xx - 7.5, yy - 7.5
    r = np.hypot(dx, dy)
    a = np.arctan2(dy, dx)
    star = r < 3.0 + 4.5 * np.abs(np.cos(a * 2.5)) ** 4
    rgb[star] = (250, 200, 70)
    rgb[star & (r < 3.4)] = (255, 245, 190)
    rgb[r < 1.8] = (255, 255, 255)
    save_bin('GFLASH.BIN', rgb)
    print('  sprites: AIM SEAT IMPACT TRACER BLOOD GFLASH')


# ---------------------------------------------------------------------------
def clean_models(started):
    """Remove from models/ what the build did not write (the models and textures of an older version) and the textures
    used by no material (the decals, the parts of other textures: they are painted in the copies): models/ holds only the
    sources of the JOM files of the disc. Kept: the preview of the flame and its custom image (see flame_sprite())"""
    used = set(MODELS_KEEP)
    for name in os.listdir(MODELS):
        if name.endswith('.mtl'):
            with open(os.path.join(MODELS, name)) as f:
                used.update(line.split()[1] for line in f if line.startswith('map_Kd '))
    removed = []
    for name in sorted(os.listdir(MODELS)):
        path = os.path.join(MODELS, name)
        if not os.path.isfile(path) or name in MODELS_KEEP:
            continue
        if os.path.getmtime(path) < started or (name.endswith('.png') and name not in used):
            os.remove(path)
            removed.append(name)
    if removed:
        print('  removed from models/: %s' % ' '.join(removed))


MODELS_KEEP = ('flame.png', 'flame_custom.png')


if __name__ == '__main__':
    import time
    started = time.time() - 1
    os.makedirs(MODELS, exist_ok=True)
    os.makedirs(CD, exist_ok=True)
    print('Guards')
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import make_guard
    make_guard.build_guards()
    print('Textures')
    all_textures()
    print('Models')
    write_model(build_washroom(), 'washroom', 'The washroom')
    write_model(build_corridor(), 'corridor', 'The corridor')
    write_model(build_storage(), 'storage', 'The storage room')
    write_model(build_hall(), 'hall', 'The hall to the laboratory')
    write_model(build_lab(), 'lab', 'The laboratory')
    write_model(build_guards(), 'guards', 'The room of the guards')
    write_model(build_office(), 'office', 'The office of the laboratory')
    write_model(build_gallery(), 'gallery', 'The gallery to the bottling room')
    write_model(build_airlock(), 'airlock', 'The airlock of the bottling room')
    write_model(build_stairs(), 'stairs', 'The stairs down to the lower floor')
    write_model(build_lowhall(), 'lowhall', 'The hall of the lower floor')
    write_model(build_bottling(), 'bottling', 'The bottling room')
    write_model(build_walkway(), 'walkway', 'The walkway of the bottling room')
    write_model(build_stairs2(), 'stairs2', 'The stairs of the bottling room')
    write_model(build_landing(), 'landing', 'The landing of the stairs of the bottling room')
    write_model(build_duct(), 'duct', 'The ventilation duct')
    write_model(build_duct_hole(), 'ducthole', 'The end of the duct, over its hole')
    write_model(build_duct_branch(), 'ductend', 'The branch of the duct (a dead end)')
    write_model(build_hinged_door_model('hdoor', DOOR_WIDTH, -DOOR_HEIGHT, 0, 'hdoor', (0.46, 0.46, 0.48)), 'hdoor',
                'A hinged door')
    sdoor = build_hinged_door_model('sdoor', DOOR_WIDTH, -DOOR_HEIGHT, 0, 'sdoor', (0.46, 0.46, 0.48))
    for q in sdoor.quads:
        # (centered: the middle of the doorway)
        q[0] = [(x - DOOR_WIDTH / 2, y, z) for x, y, z in q[0]]
    write_model(sdoor, 'sdoor', 'A sliding door (the airlock)')
    barrel, barrelf, barreld = build_barrel_models()
    write_model(barrel, 'barrel', 'An explosive barrel')
    write_model(barrelf, 'barrelf', 'An explosive barrel seen from far away')
    write_model(barreld, 'barreld', 'An explosive barrel after the explosion')
    check_kept_files()
    pane, shard = build_glass_models()
    write_model(pane, 'glass', 'The pane of a window')
    write_model(shard, 'shard', 'A piece of broken glass')
    monitor, monitord = build_monitor_models()
    write_model(monitor, 'monitor', 'The monitor of a computer')
    write_model(monitord, 'monitord', 'The monitor of a computer, its screen broken')
    build_screen_frames()
    for name, mesh in build_glassware_models().items():
        write_model(mesh, name, {'tubes': 'A rack of test tubes', 'tubesd': 'A rack of test tubes, broken', 'flask': 'A flask',
                                 'flaskd': 'A flask, broken'}[name])
    write_model(build_hinged_door_model('bdoor', DOOR_WIDTH, -DOOR_HEIGHT, 0, 'bdoor', (0.46, 0.46, 0.48)), 'bdoor',
                'A brown hinged door (the lower floor)')
    write_model(build_hinged_door_model('stalld', STALL_DOOR_WIDTH, -STALL_HEIGHT + 10, -STALL_DOOR_BOTTOM, 'panelp', (0.50, 0.50, 0.50),
                                        mirror_back=False, strips=4, lock='panel'), 'stalld',
                'The door of a stall')
    write_model(build_pistol(), 'pistol', 'The pistol (first person)')
    write_model(build_rifle(), 'rifle', 'The rifle (first person)')
    write_model(build_rifle_floor(), 'riflew', 'The rifle on the floor (dropped by a guard): one quad')
    print('Level')
    build_level()
    build_defs()
    print('Sounds')
    import make_sounds
    if os.path.exists(make_sounds.DOOR_RECORDING):
        make_sounds.build_sounds()
    hud_sprites()
    # The texts of the disc
    for name, text in (('ABS.TXT', 'Abstract'), ('BIB.TXT', 'Bibliographic'), ('CPY.TXT', 'Copyright')):
        with open(os.path.join(CD, name), 'w') as f:
            f.write(text + '\n')
    clean_models(started)
    check_cd_sort()
    print('Done')
