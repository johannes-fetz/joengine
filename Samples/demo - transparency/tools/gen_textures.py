#!/usr/bin/env python3
"""Generate the textures of the transparency test (photorealistic procedural stone, tiles and bricks).

Writes ../textures.h (RGB 555 arrays for jo_sprite_add()) and PNG previews in ./preview.
Usage: python3 tools/gen_textures.py   (from the folder of the demo)
"""
import os
import sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_H = os.path.join(HERE, '..', 'textures.h')
PREVIEW = os.path.join(HERE, 'preview')

rng = np.random.default_rng(1234)


def value_noise(w, h, cells_x, cells_y, seed):
    """Tileable smooth value noise in [0, 1]."""
    r = np.random.default_rng(seed)
    grid = r.random((cells_y, cells_x))
    ys = np.arange(h) * cells_y / h
    xs = np.arange(w) * cells_x / w
    y0 = np.floor(ys).astype(int)
    x0 = np.floor(xs).astype(int)
    fy = ys - y0
    fx = xs - x0
    fy = fy * fy * (3 - 2 * fy)
    fx = fx * fx * (3 - 2 * fx)
    y1 = (y0 + 1) % cells_y
    x1 = (x0 + 1) % cells_x
    a = grid[np.ix_(y0, x0)]
    b = grid[np.ix_(y0, x1)]
    c = grid[np.ix_(y1, x0)]
    d = grid[np.ix_(y1, x1)]
    top = a + (b - a) * fx[None, :]
    bot = c + (d - c) * fx[None, :]
    return top + (bot - top) * fy[:, None]


def fbm(w, h, base, octaves, seed, gain=0.5):
    total = np.zeros((h, w))
    amp = 1.0
    norm = 0.0
    for o in range(octaves):
        cx = base * (2 ** o)
        total += amp * value_noise(w, h, min(cx, w), min(max(1, cx * h // w), h), seed + o)
        norm += amp
        amp *= gain
    return total / norm


def supersample(fn, w, h, s=4):
    """Render at s x, then reduce with a box filter (no aliasing in the fine details)."""
    big = fn(w * s, h * s)
    img = Image.fromarray(np.clip(big * 255, 0, 255).astype(np.uint8), 'RGB')
    return np.asarray(img.resize((w, h), Image.LANCZOS)).astype(np.float64) / 255.0


def floor_stone(W, H):
    """Worn dark flagstones (2 x 2 slabs), mortar joints, grime."""
    slab = W // 2
    y, x = np.mgrid[0:H, 0:W]
    lx = (x % slab) / slab
    ly = (y % slab) / slab
    edge = np.minimum(np.minimum(lx, 1 - lx), np.minimum(ly, 1 - ly))
    joint = np.clip((edge - 0.012) / 0.03, 0, 1)
    # Each slab its own tint
    sid = (x // slab) + 2 * (y // slab)
    tint = np.array([0.95, 1.05, 1.0, 0.9])[sid]
    n = fbm(W, H, 4, 6, 11)
    fine = fbm(W, H, 32, 3, 21)
    stains = fbm(W, H, 2, 4, 31)
    base = np.stack([0.43, 0.40, 0.36]) * 1.0
    lum = (0.75 + 0.35 * (n - 0.5) * 2 * 0.5 + 0.12 * (fine - 0.5)) * tint
    lum *= 1.0 - 0.25 * np.clip((stains - 0.55) * 4, 0, 1)
    # Edges of the slabs slightly worn (darker near the joints)
    lum *= 0.82 + 0.18 * np.clip(edge / 0.08, 0, 1)
    col = base[None, None, :] * lum[..., None]
    mortar = np.array([0.18, 0.17, 0.15])
    col = col * joint[..., None] + mortar[None, None, :] * (1 - joint[..., None])
    return col


def pool_tiles(W, H):
    """Small white and aqua ceramic tiles with grout, faint caustic light."""
    t = W // 8
    y, x = np.mgrid[0:H, 0:W]
    lx = (x % t) / t
    ly = (y % t) / t
    edge = np.minimum(np.minimum(lx, 1 - lx), np.minimum(ly, 1 - ly))
    grout = np.clip((edge - 0.04) / 0.08, 0, 1)
    tid = (x // t) * 7 + (y // t) * 13
    r = np.random.default_rng(5).random(64)
    tint = 0.92 + 0.08 * r[tid % 64]
    glaze = 0.95 + 0.05 * np.clip(1 - ((lx - 0.3) ** 2 + (ly - 0.3) ** 2) * 6, 0, 1)
    n = fbm(W, H, 4, 4, 41)
    # Caustics: thin bright lines from the ridges of a noise
    c = fbm(W, H, 3, 4, 51)
    caustic = np.clip(1 - np.abs(c - 0.5) * 14, 0, 1) ** 2
    base = np.array([0.55, 0.80, 0.86])
    lum = tint * glaze * (0.9 + 0.1 * n)
    col = base[None, None, :] * lum[..., None]
    col += caustic[..., None] * np.array([0.25, 0.25, 0.22])[None, None, :]
    groutc = np.array([0.42, 0.50, 0.52])
    col = col * grout[..., None] + groutc[None, None, :] * (1 - grout[..., None])
    return np.clip(col, 0, 1)


def sandstone_bricks(W, H):
    """Weathered sandstone bricks (running bond), mortar, erosion."""
    bh = H // 8
    bw = W // 2
    y, x = np.mgrid[0:H, 0:W]
    row = y // bh
    xs = (x + (row % 2) * (bw // 2)) % W
    lx = (xs % bw) / bw
    ly = (y % bh) / bh
    edge_x = np.minimum(lx, 1 - lx) * bw
    edge_y = np.minimum(ly, 1 - ly) * bh
    edge = np.minimum(edge_x, edge_y)
    joint = np.clip((edge - 0.5 * W / 32) / (0.9 * W / 32), 0, 1)
    bid = (xs // bw) + row * 3
    r = np.random.default_rng(7).random(256)
    tint = 0.85 + 0.25 * r[bid % 256]
    n = fbm(W, H, 4, 6, 61)
    grain = fbm(W, H, 32, 2, 71)
    base = np.array([0.66, 0.50, 0.34])
    lum = tint * (0.8 + 0.3 * (n - 0.5) + 0.1 * (grain - 0.5))
    # Rounded edges of the bricks (light from the top)
    lum *= 0.85 + 0.15 * np.clip(edge / (2.0 * W / 32), 0, 1)
    col = base[None, None, :] * lum[..., None]
    mortar = np.array([0.30, 0.26, 0.22])
    col = col * joint[..., None] + mortar[None, None, :] * (1 - joint[..., None])
    return np.clip(col, 0, 1)


def water_surface(W, H):
    """Deep blue-green water: ripples lit from the top-left, small specular glints. Kept below 0.75: the gouraud of
    the demo adds up to + 4 units (31 would wrap to dark)."""
    n1 = fbm(W, H, 3, 5, 81)
    n2 = fbm(W, H, 6, 4, 91)
    h = 0.6 * n1 + 0.4 * n2
    gy, gx = np.gradient(h)
    shade = np.clip(0.5 + (-gx - gy) * W * 0.12 + (h - 0.5) * 0.6, 0, 1)
    deep = np.array([0.04, 0.22, 0.36])
    shallow = np.array([0.10, 0.45, 0.55])
    col = deep[None, None, :] + (shallow - deep)[None, None, :] * shade[..., None]
    glint = np.clip((shade - 0.82) * 6, 0, 1) ** 2
    col += glint[..., None] * np.array([0.25, 0.25, 0.22])[None, None, :]
    return np.clip(col, 0, 0.75)


FLAME_FRAMES = 4


def flame(W, H, frame):
    """One frame of a looping flame (FLAME_FRAMES frames): noise rising through a teardrop shape, black outside (no
    effect in the additive mode). At most 0.87: the bottom of the flames is drawn with a neutral gouraud."""
    y, x = np.mgrid[0:H, 0:W]
    u = (x + 0.5) / W - 0.5                  # -0.5 .. 0.5
    v = 1.0 - (y + 0.5) / H                  # 0 at the bottom, 1 at the top
    # Rising noise: the noise is tileable vertically, shifted by a quarter per frame (a loop of 4 frames)
    shift = int(round(frame * H / FLAME_FRAMES))
    n = np.roll(fbm(W, H, 2, 5, 101), -shift, axis=0)
    n2 = np.roll(fbm(W, H, 4, 4, 111), -2 * shift % H, axis=0)
    turb = 0.65 * n + 0.35 * n2
    # Teardrop: wide at the bottom (but rounded), narrow at the top, bent by the noise
    width = 0.42 * (1 - v) ** 0.8 * np.clip(v * 6, 0, 1) ** 0.5 + 0.02
    du = u + (turb - 0.5) * 0.25 * v
    body = np.clip(1 - np.abs(du) / np.maximum(width, 1e-3), 0, 1)
    heat = body * (1.15 - v) * (0.55 + 0.9 * turb)
    heat = np.clip(heat, 0, 1.2)
    # Color ramp: dark red -> orange -> yellow -> pale yellow (the hot core)
    stops = np.array([0.0, 0.25, 0.5, 0.8, 1.2])
    cols = np.array([[0, 0, 0], [0.45, 0.06, 0.01], [0.85, 0.30, 0.03], [0.87, 0.65, 0.18], [0.87, 0.82, 0.55]])
    out = np.zeros((H, W, 3))
    for c in range(3):
        out[..., c] = np.interp(heat, stops, cols[:, c])
    return np.clip(out, 0, 0.87)


def halo(W, H):
    """Glow of a lamp (additive): a white hot core, a warm falloff, faint soft rays; black at the edge (transparent)."""
    y, x = np.mgrid[0:H, 0:W]
    dx = (x + 0.5) / W - 0.5
    dy = (y + 0.5) / H - 0.5
    r = np.sqrt(dx * dx + dy * dy) * 2          # 0 at the center, 1 at the edge
    a = np.arctan2(dy, dx)
    rays = 1 + 0.04 * np.cos(a * 6) + 0.025 * np.cos(a * 11 + 1.3)
    glow = np.exp(-(r * 2.4 / rays) ** 1.4)
    glow *= np.clip((1 - r) / 0.25, 0, 1)        # nothing at the edge
    core = np.exp(-(r * 11) ** 2)
    col = glow[..., None] * np.array([0.92, 0.62, 0.26])[None, None, :]
    col += core[..., None] * np.array([0.10, 0.30, 0.45])[None, None, :]
    return np.clip(col, 0, 0.9)


# Textures whose black (after the quantization to 5 bits) is transparent (color 0: the VDP1 doesn't draw these pixels)
TRANSPARENT_BLACK = ('halo',)


TEXTURES = [
    ('floor', 64, 64, floor_stone),
    ('pool', 64, 64, pool_tiles),
    ('bricks', 32, 64, sandstone_bricks),
    ('water', 64, 64, water_surface),
    ('halo', 64, 64, halo),
] + [('flame%d' % f, 32, 64, (lambda w, h, f=f: flame(w, h, f))) for f in range(FLAME_FRAMES)]


def to555(img, transparent_black=False):
    q = np.clip(np.round(img * 31), 0, 31).astype(int)
    v = 0x8000 | (q[..., 2] << 10) | (q[..., 1] << 5) | q[..., 0]
    if transparent_black:
        v = np.where(v == 0x8000, 0, v)
    return v


def main():
    os.makedirs(PREVIEW, exist_ok=True)
    lines = ['/* Generated by tools/gen_textures.py: do not edit */',
             '#ifndef TEXTURES_H', '# define TEXTURES_H', '']
    for name, w, h, fn in TEXTURES:
        img = supersample(fn, w, h)
        q = np.clip(np.round(img * 31), 0, 31) / 31.0
        Image.fromarray((q * 255).astype(np.uint8)).resize((w * 4, h * 4), Image.NEAREST).save(
            os.path.join(PREVIEW, name + '.png'))
        data = to555(img, name in TRANSPARENT_BLACK or name.startswith('flame')).flatten()
        lines.append('# define TEX_%s_W (%d)' % (name.upper(), w))
        lines.append('# define TEX_%s_H (%d)' % (name.upper(), h))
        lines.append('static jo_color tex_%s[%d] = {' % (name, w * h))
        for i in range(0, len(data), 16):
            lines.append('    ' + ', '.join('0x%04X' % v for v in data[i:i + 16]) + ',')
        lines.append('};')
        lines.append('')
    lines.append('#endif')
    with open(OUT_H, 'w', newline='\r\n') as f:
        f.write('\n'.join(lines) + '\n')
    print('wrote', os.path.normpath(OUT_H))


if __name__ == '__main__':
    sys.exit(main())
