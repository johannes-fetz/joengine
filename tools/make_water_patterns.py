#!/usr/bin/env python3
"""The built-in patterns of the water of the engine (jo/water.h): jo_engine/water_patterns.h, compiled with water.c (in the
program only when the game uses the water: the link drops what is not called). A game can give its own patterns in a
level file instead (jo_water_config.patterns_file, the option --jol).

    foam pattern: 128 x 128 values 1 to 15 (4 bits, 2 pixels per byte, the left pixel in the high bits): the foam of the
        water, a pattern that repeats every 64 units (4 m) in both directions (the quads of 8 units: tiles of 16 x 16). The
        half pattern: the same at 64 x 64 (the quads of 16 units), equalized again. The value is how soon a pixel turns
        white when the foam grows: the color lookup tables of the foam (water.c) show the pixels above a threshold in white.
        The values are equalized (as many pixels of each value): a threshold shows a known share of foam. The shapes: lace
        (the edges of cells, the bubbles), in big soft patches (the foam is thick in places, thin around), stretched a bit.
    bottom pattern: the bottom seen through the shallow water (the quads of the level 0, 8 units, take tiles of 16 x 16
        texels: 128 texels for 64 units), values 1 to 15 (4 bits): a phase, the contours of a smooth noise. The color
        lookup tables of the bottom (water.c) show a light band at a phase that moves with the time: the light of the waves
        on the sand moves (the lines move along the contours).
    sky reflection: 64 x 64 values 0 to 255: the clouds reflected by the waves (water.c: the direction reflected by the
        surface toward the sky picks a value), soft streaks; repeats in both directions.

The patterns in a level file (--jol FILE): a section of the game 'foam': u16 the size of the pattern (128), u16 0, the foam
pattern (128 x 128, 4 bits), the half pattern (64 x 64), the bottom pattern (128 x 128). The sky reflection is always the
built-in one.

Usage: python3 tools/make_water_patterns.py [--jol FILE] [--preview FILE.png]   (from the root of the repository; needs
numpy, Pillow)
"""

import argparse
import math
import os
import struct
import sys

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
sys.path.insert(0, os.path.join(ROOT, 'Compiler', 'COMMON'))
import jo_level  # noqa: E402

OUT = os.path.join(ROOT, 'jo_engine', 'water_patterns.h')


def tile_noise(size, cells, rng):
    """Smooth value noise that repeats every size pixels (a random grid of cells x cells, cosine interpolation)"""
    grid = rng.uniform(0, 1, (cells, cells))
    t = np.arange(size) * cells / size
    i0 = np.floor(t).astype(int)
    f = (1 - np.cos((t - i0) * math.pi)) / 2
    i1 = (i0 + 1) % cells
    a = grid[i0][:, i0]
    b = grid[i0][:, i1]
    c = grid[i1][:, i0]
    d = grid[i1][:, i1]
    top = a + (b - a) * f[None, :]
    bottom = c + (d - c) * f[None, :]
    return top + (bottom - top) * f[:, None]


def fbm(size, base, octaves, seed):
    rng = np.random.default_rng(seed)
    out = np.zeros((size, size))
    amp = 1.0
    total = 0.0
    for k in range(octaves):
        out += amp * tile_noise(size, base << k, rng)
        total += amp
        amp *= 0.55
    return out / total


def worley_edges(size, count, seed, stretch=1.0):
    """Edges of the cells of points (F2 - F1 small: on an edge), periodic; stretched along x"""
    rng = np.random.default_rng(seed)
    pts = rng.uniform(0, 1, (count, 2))
    y, x = np.mgrid[0:size, 0:size] / size
    d1 = np.full((size, size), 9.0)
    d2 = np.full((size, size), 9.0)
    for cx, cy in pts:
        dx = np.abs(x - cx)
        dy = np.abs(y - cy)
        dx = np.minimum(dx, 1 - dx) / stretch
        dy = np.minimum(dy, 1 - dy)
        d = np.sqrt(dx * dx + dy * dy)
        d2 = np.where(d < d1, d1, np.minimum(d2, d))
        d1 = np.minimum(d1, d)
    return d2 - d1, d1


def equalize(values, levels):
    """Values 1 to levels, as many pixels of each value (the order of the values kept)"""
    flat = values.ravel()
    order = np.argsort(flat, kind='stable')
    out = np.empty(flat.size, dtype=np.int32)
    out[order] = 1 + (np.arange(flat.size) * levels) // flat.size
    return out.reshape(values.shape)


def make_foam(size=128):
    # Lace: thin bright lines on the edges of small cells (bubbles), and bigger cells
    small, small_d1 = worley_edges(size, 90, 5, 1.3)
    big, _ = worley_edges(size, 22, 9, 1.5)
    lace = np.exp(-small * 26) * 0.75 + np.exp(-big * 14) * 0.45 + (1 - np.clip(small_d1 * 9, 0, 1)) * 0.1
    # Patches: where the foam is thick (soft blobs)
    patches = fbm(size, 4, 4, 13)
    value = lace * 0.55 + patches * 0.9 + fbm(size, 16, 2, 3) * 0.12
    return equalize(value, 15)


def make_reflection(size=64):
    """Soft streaks: cloud noise stretched along x, high values rare (white streaks on the blue)"""
    n = fbm(size, 2, 3, 21)
    m = fbm(size, 4, 2, 33)
    v = np.clip((n * 0.8 + m * 0.3 - 0.5) * 3.0, 0, 1)
    v = v ** 1.2
    return np.round(v * 255).astype(np.int32)


def bottom_phase(size):
    """The phase of the bottom (0 to 1) on size x size texels for 64 units, the same function at any size: the contours of
    a smooth noise that repeats (2 octaves), warped a bit (rounder lines)"""
    rng = np.random.default_rng(41)
    cells = [(3, rng.uniform(0, 1, (3, 3))), (6, rng.uniform(0, 1, (6, 6)))]
    t = (np.arange(size) + 0.5) / size

    def noise(cells_n, grid, u, v):
        x = u * cells_n
        y = v * cells_n
        i0 = np.floor(x).astype(int) % cells_n
        j0 = np.floor(y).astype(int) % cells_n
        fx = (1 - np.cos((x - np.floor(x)) * math.pi)) / 2
        fy = (1 - np.cos((y - np.floor(y)) * math.pi)) / 2
        i1 = (i0 + 1) % cells_n
        j1 = (j0 + 1) % cells_n
        a = grid[j0, i0]
        b = grid[j0, i1]
        c = grid[j1, i0]
        d = grid[j1, i1]
        return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy

    v, u = np.meshgrid(t, t, indexing='ij')
    warp_u = u + 0.06 * np.sin(2 * math.pi * (v * 2 + 0.3))
    warp_v = v + 0.06 * np.sin(2 * math.pi * (u * 2 + 0.7))
    n = noise(cells[0][0], cells[0][1], warp_u, warp_v) * 0.7 + noise(cells[1][0], cells[1][1], warp_u, warp_v) * 0.3
    # (3 bands across the range of the noise: the phase 0 to 1, 3 times)
    return np.mod(n * 3.0, 1.0)


def c_array(name, data, per_line=24):
    lines = []
    flat = [int(v) for v in data]
    for k in range(0, len(flat), per_line):
        lines.append('    ' + ', '.join('%d' % v for v in flat[k:k + per_line]) + ',')
    return 'static const unsigned char %s[%d] __attribute__((aligned(4))) =\n{\n%s\n};\n' % (name, len(flat), '\n'.join(lines))


def patterns():
    """The section data of the patterns (see above) and the sky reflection"""
    foam = make_foam()
    refl = make_reflection()
    packed = (foam[:, 0::2] << 4) | foam[:, 1::2]
    half = equalize(foam.reshape(64, 2, 64, 2).mean(axis=(1, 3)) + np.arange(64 * 64).reshape(64, 64) * 1e-9, 15)
    packed_half = (half[:, 0::2] << 4) | half[:, 1::2]
    values = 1 + np.minimum(14, np.floor(bottom_phase(128) * 15)).astype(np.int32)
    packed_bottom = (values[:, 0::2] << 4) | values[:, 1::2]
    data = struct.pack('>HH', 128, 0) + bytes(packed.ravel().astype(np.uint8)) + bytes(packed_half.ravel().astype(np.uint8)) + \
        bytes(packed_bottom.ravel().astype(np.uint8))
    return data, refl, foam


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--jol', help='write the patterns in a level file (the section foam) instead of the header')
    parser.add_argument('--preview', help='a picture of the foam at 3 thresholds and of the reflections')
    args = parser.parse_args()
    data, refl, foam = patterns()
    if args.jol:
        level = jo_level.Level('Water patterns')
        level.add('foam', data=data)
        print('%s: %d bytes' % (args.jol, level.write(args.jol)))
    else:
        text = ['/*', '** Jo Sega Saturn Engine: the built-in patterns of the water (see jo/water.h)',
                '** Generated by tools/make_water_patterns.py: do not edit', '*/', '',
                '#ifndef __JO_WATER_PATTERNS_H__', '# define __JO_WATER_PATTERNS_H__', '',
                '/** @brief The size of the patterns (the half pattern: half of it) and of the sky reflection */',
                '# define JO_WATER_PATTERN_SIZE          (128)', '# define JO_WATER_SKY_REFLECTION_SIZE   (64)', '',
                '/** @brief The patterns: u16 the size of the pattern, u16 0, the foam pattern (4 bits), the half pattern, the',
                ' *  bottom pattern (see tools/make_water_patterns.py) */',
                c_array('__jo_water_patterns', data),
                '/** @brief The clouds reflected by the waves (0 to 255) */',
                c_array('__jo_water_sky_reflection', refl.ravel()),
                '#endif /* !__JO_WATER_PATTERNS_H__ */', '']
        with open(OUT, 'w', newline='\r\n') as f:
            f.write('\n'.join(text))
        print('%s: %d bytes of patterns, %d of sky reflection' % (OUT, len(data), refl.size))
    if args.preview:
        water = np.array((40, 168, 168))
        white = np.array((235, 240, 240))
        tiles = []
        for t in (13, 10, 6):
            rgb = np.where((foam >= t)[..., None], white[None, None, :], water[None, None, :])
            tiles.append(np.tile(rgb, (2, 2, 1)))
        r = np.tile(refl, (4, 4))
        tiles.append(water[None, None, :] + (white - water)[None, None, :] * (r[..., None] / 255.0))
        img = np.concatenate(tiles, axis=1)
        Image.fromarray(np.clip(img, 0, 255).astype(np.uint8)).save(args.preview)


if __name__ == '__main__':
    main()
