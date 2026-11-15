"""The sprites of the sun of jo/sun.h: its halo, its core and the pieces of its lens flare, in 16 colors (the section
TX16, see Level.add_textures16()). Needs numpy and Pillow (the rest of jo_level does not).

The halo is see-through: the VDP2 mixes its pixels with the sky behind them (the color calculation of the sprites, at 3
blend ratios only: jo_vdp2_set_sprite_blend_ratios(); the ratio 0: opaque). A pixel of the halo is given the most
see-through ratio that can show its light, and a color that makes the mix that light over the clear sky behind it: the
sky behind each row is given by the game (a function of the height above the horizon, in pixels of the screen). The halo
in strips of 128 x 32 pixels (15 colors each: the colors change with the sky), the core over its middle (32 x 32: the
white disk, antialiased). The pieces of the lens flare: 32 x 32, see-through (dithered between the ratios).

A pixel of the sprites: None (transparent), (r, g, b) opaque or (r, g, b, level): level 0 opaque, 1 to 3 the blend ratio
of the VDP2 (its color with the MSB set). pack16() makes the palette and the textures of a list of such sprites.

Example (the sun 58 pixels above the horizon, the blend ratios 8, 17, 25 of the game):
    from jo_level import sun
    opacity = sun.opacities((8, 17, 25))
    sky = lambda h: (120, 180, 236)                                     # the clear sky h pixels above the horizon
    sprites = [('SUN_%d' % k, s) for k, s in enumerate(sun.halo(sky, 58, opacity))]
    sprites += [('SUN_CORE', sun.core(sky, 58, opacity))]
    sprites += [(name, sun.flare(kind, color, opacity)) for name, kind, color in sun.FLARE_PIECES]
    textures, colors = sun.pack16(sprites)
    lv.add_textures16(textures, colors)
"""

import math

import numpy as np
from PIL import Image

from .level import saturn_color

# The opacity of a pixel at the blend ratios 1, 2, 3 of the sprites: (31 - ratio) / 31 (see opacities())
OPACITY = (23 / 31.0, 14 / 31.0, 6 / 31.0)
# The size of the halo (pixels: a square, the sun in its middle), its strips, the core (the white disk inside)
HALO_SIZE = 128
STRIP_HEIGHT = 32
CORE_SIZE = 32
CORE_RADIUS = 16.5
GLOW = (255, 244, 214)
DISK = (255, 255, 250)
# The pieces of the lens flare: their names (the order of jo_sun.flare[]: ring, disc, hexagon), their shapes, their colors
FLARE_PIECES = (('FLARE_RING', 'ring', (190, 255, 200)), ('FLARE_DISC', 'disc', (255, 200, 150)),
                ('FLARE_HEX', 'hex', (170, 190, 255)))
_BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def opacities(ratios):
    """The opacities of the 3 blend ratios of the sprites (jo_vdp2_set_sprite_blend_ratios(): 0 opaque, 31 almost
    invisible)"""
    return tuple((31 - r) / 31.0 for r in ratios)


def blend_level(want, x, y, opacity=OPACITY):
    """The blend ratio level (0 opaque, 1 to 3, None: nothing) for an opacity, dithered (ordered, 4 x 4) between the 2
    levels around it: no ring"""
    levels = [(1.0, 0)] + [(opacity[k], k + 1) for k in range(3)] + [(0.0, None)]
    for k in range(len(levels) - 1):
        hi, lo = levels[k], levels[k + 1]
        if lo[0] <= want <= hi[0]:
            t = (want - lo[0]) / max(1e-6, hi[0] - lo[0])
            return hi[1] if _BAYER[y & 3][x & 3] / 16.0 < t else lo[1]
    return 0 if want > 1 else None


def _want(r):
    """The opacity of the glow at r pixels from the middle of the sun (> 1: opaque): a bright core (exp, 6 px) over a wide
    soft one (exp, 22 px), to 0 at 63 px"""
    if r > 63.0:
        return 0.0
    w = 0.95 * math.exp(-(r - 8.0) / 6.0) + 0.42 * math.exp(-(r - 8.0) / 22.0)
    return w * min(1.0, (63.0 - r) / 10.0)


def _light(x, y, glow, disk):
    """The light of the pixel (x, y) of the halo: its color, its opacity (the mean of 4 x 4 samples: the edge of the disk
    antialiased)"""
    acc, wsum = np.zeros(3), 0.0
    for j in range(4):
        for i in range(4):
            r = math.hypot(x + (i + 0.5) / 4 - 64.0, y + (j + 0.5) / 4 - 64.0)
            if r < 6.5:
                c, w = disk, 1.0
            else:
                w = min(1.0, _want(r))
                c = glow if r > 9.0 else disk + (glow - disk) * (r - 6.5) / 2.5
            acc += c * w
            wsum += w
    return acc / max(wsum, 1e-6), wsum / 16.0


def _entry(color, w, sky, opacity):
    """The pixel (r, g, b, level) for a light (color, opacity w) over the sky: the most see-through ratio that can show
    it, its color such as the mix of the VDP2 (c x o + sky x (1 - o)) is color x w + sky x (1 - w); None: nothing"""
    if w < 0.02:
        return None
    if w >= opacity[0]:
        c = color * w + sky * (1 - w)
        return tuple(np.clip(c, 0, 255)) + (0,)
    for k in (2, 1, 0):
        o = opacity[k]
        if o >= w:
            c = (color * w + sky * (o - w)) / o
            return tuple(np.clip(c, 0, 255)) + (k + 1,)
    return None


def _fit_colors(pixels, count=15):
    """At most count colors for the pixels (r, g, b, level) of a sprite: a few per level (as many as its share of the
    pixels, 1 at least), each pixel the nearest of its level"""
    groups = {}
    for p in pixels:
        if p is not None:
            groups.setdefault(p[3], []).append(p[:3])
    if not groups:
        return {}
    total = sum(len(g) for g in groups.values())
    shares = {lv: max(1, int(round(count * len(g) / total))) for lv, g in groups.items()}
    while sum(shares.values()) > count:
        lv = max(shares, key=lambda k: shares[k])
        shares[lv] -= 1
    palettes = {}
    for lv, g in groups.items():
        q = Image.new('RGB', (len(g), 1))
        q.putdata([tuple(int(round(v)) for v in c) for c in g])
        n = min(shares[lv], len(set(map(tuple, np.asarray(q).reshape(-1, 3)))))
        pal = q.quantize(colors=max(1, n), method=Image.MEDIANCUT).getpalette()[:max(1, n) * 3]
        palettes[lv] = np.array([pal[3 * k:3 * k + 3] for k in range(len(pal) // 3)], float)
    return palettes


def _pixels(x0, y0, w, h, keep, sky, height, opacity, glow, disk):
    """The pixels of a part of the halo (its place in the 128 x 128 square, its size), keep(r): the pixels drawn; in 15
    colors"""
    glow, disk = np.array(glow, float), np.array(disk, float)
    raw = []
    for y in range(y0, y0 + h):
        behind = np.array(sky(height - (y - 63.5)), float)
        row = []
        for x in range(x0, x0 + w):
            r = math.hypot(x + 0.5 - 64.0, y + 0.5 - 64.0)
            if not keep(r):
                row.append(None)
                continue
            c, op = _light(x, y, glow, disk)
            row.append(_entry(c, op, behind, opacity))
        raw.append(row)
    palettes = _fit_colors([p for row in raw for p in row])
    out = []
    for row in raw:
        o = []
        for p in row:
            if p is None:
                o.append(None)
                continue
            pal = palettes[p[3]]
            k = int(np.argmin(((pal - np.array(p[:3])) ** 2).sum(axis=1)))
            o.append(tuple(int(v) for v in pal[k]) + (p[3],))
        out.append(o)
    return out


def halo(sky, height, opacity=OPACITY, strips=HALO_SIZE // STRIP_HEIGHT, glow=GLOW, disk=DISK):
    """The halo of the sun: strips of 128 x 32 pixels, from the top (jo_sun.halo[], drawn over each other from the top)
    @param sky The clear sky behind a row: sky(h) the color (r, g, b) h pixels above the horizon (h < 0: under it)
    @param height The middle of the sun above the horizon (pixels of the screen: tan(elevation) x projection distance)"""
    return [_pixels(0, STRIP_HEIGHT * k, HALO_SIZE, STRIP_HEIGHT, lambda r: CORE_RADIUS - 1.0 <= r < 64.0, sky, height,
                    opacity, glow, disk) for k in range(strips)]


def core(sky, height, opacity=OPACITY, glow=GLOW, disk=DISK):
    """The middle of the sun (32 x 32, jo_sun.core: drawn over the halo): the white disk, its edge, the bright glow around
    it"""
    return _pixels(48, 48, CORE_SIZE, CORE_SIZE, lambda r: r < CORE_RADIUS, sky, height, opacity, glow, disk)


def flare(kind, color, opacity=OPACITY):
    """A piece of the lens flare (32 x 32): 'ring', 'disc' (lighter in its middle) or 'hex', see-through (the VDP2 mixes it
    with the sky; the same pixels in RGB drawn half transparent by the VDP1 over the rest, see jo_sun.flare_rgb[])"""
    out = []
    for y in range(32):
        row = []
        for x in range(32):
            dx, dy = x - 15.5, y - 15.5
            r = math.hypot(dx, dy)
            if kind == 'ring':
                want = 0.55 * max(0.0, 1 - abs(r - 12.5) / 2.6)
            elif kind == 'disc':
                want = 0.0 if r > 14.5 else 0.22 + 0.25 * (1 - r / 14.5) ** 2
            else:
                # (a hexagon: the largest of the 3 distances to the pairs of sides)
                a = max(abs(dx), abs(dx * 0.5 + dy * 0.866), abs(dx * 0.5 - dy * 0.866))
                want = 0.0 if a > 13.5 else 0.30 + (0.12 if a > 11.5 else 0.0)
            level = blend_level(min(want, opacity[0]), x, y, opacity) if want > 0.02 else None
            row.append(None if level is None or level == 0 else tuple(color) + (level,))
        out.append(row)
    return out


def pack16(sprites):
    """The textures of the section TX16 for a list of sprites (name, rows of pixels: None, (r, g, b) or (r, g, b, level)),
    15 colors each at most, all of them in one palette (255 colors at most): (textures, colors) for
    Level.add_textures16(textures, colors). A color is see-through (its MSB set: mixed by the VDP2) when its level is above
    0, else opaque (MSB cleared); a texture: the color of the palette of each value, its blend ratio level"""
    def key(c):
        return c[:3] + (1 if len(c) == 4 and c[3] > 0 else 0,)

    palette = []
    luts = []
    for name, img in sprites:
        local = []
        for row in img:
            for c in row:
                if c is not None:
                    c = c if len(c) == 4 else c + (0,)
                    if c not in local:
                        local.append(c)
        if len(local) > 15:
            raise ValueError('%s: %d colors (15 at most)' % (name, len(local)))
        for c in local:
            if key(c) not in palette:
                palette.append(key(c))
        luts.append(local)
    if len(palette) > 255:
        raise ValueError('%d colors in the palette (255 at most)' % len(palette))
    textures = []
    for n, (name, img) in enumerate(sprites):
        local = luts[n]
        idx = [[0 if c is None else local.index(c if len(c) == 4 else c + (0,)) + 1 for c in row] for row in img]
        colors = [0] + [palette.index(key(c)) + 1 for c in local]
        levels = [0] + [c[3] for c in local]
        textures.append(dict(width=len(img[0]), height=len(img), colors=colors, ratios=levels, pixels=idx))
    return textures, [saturn_color(r, g, b, msb=see) for r, g, b, see in palette]
