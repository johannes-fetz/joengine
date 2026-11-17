#!/usr/bin/env python3
"""Make the house of the 3D model demo: models/house.obj, models/house.mtl and their textures (PNG).

    python3 models/make_house.py
    python3 ../../Compiler/COMMON/jo_model.py models/house.obj -o cd/HOUSE.JOM --scale 6 --embed-textures

A cottage on its lawn: rendered walls with a stone plinth, windows with shutters, a wooden door and its step, wooden
boards in the gables, a roof of clay tiles, a brick chimney. The VDP1 maps a whole texture on each quad: the windows and the
door are painted in the textures of the panels of the walls (a quad over a wall would flicker with it), each panel is 2 m
wide and has its own texture. The sun is baked in the gouraud colors (vertex colors: 16/31 leaves a texture unchanged,
the faces in the shadow are darker; never brighter: the gouraud of the VDP1 doesn't saturate).
OBJ axes (Y up, the front of the house faces +Z), meters. Needs numpy and Pillow.
"""
import math
import os
import sys
import tempfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
# The converter of the Jo Engine (its painter's sort check)
sys.path.insert(0, os.path.join(HERE, '..', '..', '..', 'Compiler', 'COMMON'))
import jo_model  # noqa: E402
RNG = np.random.default_rng(1886)

# Sizes (meters)
WIDTH = 8.0                     # Along X
DEPTH = 6.0                     # Along Z
WALL = 3.0                      # Height of the walls
RIDGE = 5.0                     # Height of the ridge
EAVE = 0.4                      # Overhang of the roof over the front and the back walls
VERGE = 0.3                     # Overhang of the roof over the gables
PANEL = 2.0                     # Width of a panel of wall (one texture)
ROOF_PARTS = 4                  # Quads of each side of the roof along X
ROOF_THICKNESS = 0.12           # Of the overhangs of the roof (its underside, the boards along its edges)
ROOF_ROWS = 3                   # Quads of each side of the roof from the wall to the ridge (the eave: one more row)
LAWN = 2.0                      # Size of a quad of the lawn
LAWN_BORDER = 2                 # Quads of lawn around the house
CHIMNEY = (1.9, 2.5, -0.3, 0.3, 5.9)    # x0, x1, z0, z1, top (across the ridge)
STEP = (-1.75, -0.25, 0.55, 0.2)        # x0, x1, depth, height (in front of the door)
# The scale of the converter (jo_model.py --scale: units of the engine per meter), the views of the sort check (degrees
# above the house: main.c tilts it by HOUSE_TILT)
JOM_SCALE = 6
SORT_CHECK_ELEVATIONS = (10, 25, 40, 60)
# Textures: 16 pixels per meter for the walls
PANEL_TEXTURE = (32, 48)
TILE_TEXTURE = 32
TILE_ROW = 8                    # Pixels of a row of tiles (the eave: one row)
# The sun (baked in the gouraud colors): direction toward the sun, the light of a face in the shadow and in the sun
SUN = np.array([0.45, 0.75, 0.5])
GOURAUD_NEUTRAL = 16 / 31     # A vertex color: 1 is 31 in the gouraud table (+15), 16/31 leaves the texture unchanged
SHADE = 0.62 * GOURAUD_NEUTRAL
SUNLIT = GOURAUD_NEUTRAL
# Colors
STUCCO = (214, 200, 170)
STONE = (150, 144, 132)
FRAME = (232, 230, 222)
SHUTTER = (64, 96, 74)
DOOR = (96, 58, 34)
BOARDS = (132, 110, 84)
CLAY = (168, 84, 52)
BRICK = (150, 70, 48)
MORTAR = (176, 168, 152)
CONCRETE = (150, 148, 142)
GRASS = (78, 112, 46)
SOFFIT = (196, 186, 164)
FASCIA = (92, 66, 48)


# ---------------------------------------------------------------- textures (procedural, photographic look)

def smooth_noise(w, h, cells):
    """Value noise (bicubic): cells random values across the width. It tiles (the texture of a quad is repeated on the
    next quad): the values repeated around, the middle kept"""
    g = np.tile(RNG.random((max(2, cells * h // w), cells)), (3, 3))
    img = Image.fromarray((g * 255).astype(np.uint8)).resize((w * 3, h * 3), Image.BICUBIC)
    return np.asarray(img, dtype=float)[h:2 * h, w:2 * w] / 255


def fractal(w, h, octaves=((2, 0.5), (4, 0.3), (8, 0.2))):
    out = np.zeros((h, w))
    for cells, weight in octaves:
        out += smooth_noise(w, h, cells) * weight
    return (out - out.mean()) / (out.std() + 1e-9)


def grain(w, h, amount):
    return (RNG.random((h, w)) - 0.5) * amount


def colored(color, field):
    return np.asarray(color, dtype=float)[None, None, :] * (1 + field[:, :, None])


def save(name, rgb):
    Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8), 'RGB').save(os.path.join(HERE, name + '.png'))
    return name + '.png'


def stucco_wall(w, h):
    """Rendered wall, darker toward the ground (splashes of rain), a stone plinth at the bottom"""
    rgb = colored(STUCCO, fractal(w, h) * 0.04 + grain(w, h, 0.06))
    y = np.arange(h)[:, None] / h
    rgb *= (1 - 0.08 * y ** 3)[:, :, None]
    plinth = h - h // 8
    stones = colored(STONE, fractal(w, h // 8 + 1)[:h - plinth] * 0.08 + grain(w, h - plinth, 0.1))
    # (courses of stones: dark joints)
    stones[(np.arange(h - plinth) % 3 == 0)] *= 0.8
    for x in range(0, w, 5):
        stones[1:, x] *= 0.82
    rgb[plinth:] = stones
    rgb[plinth - 1] *= 0.75                     # (the shadow under the edge of the render)
    return rgb


def box(rgb, x0, y0, x1, y1, color, field_amount=0.0):
    h = y1 - y0
    w = x1 - x0
    rgb[y0:y1, x0:x1] = colored(color, grain(w, h, field_amount))


def window_panel(name):
    w, h = PANEL_TEXTURE
    rgb = stucco_wall(w, h)
    x0, x1, y0, y1 = 10, 22, 10, 31
    # Shutters (wooden slats)
    for sx in (x0 - 6, x1):
        box(rgb, sx, y0, sx + 6, y1, SHUTTER, 0.08)
        rgb[y0:y1:2, sx:sx + 6] *= 0.82
        rgb[y0:y1, sx + 5 if sx < x0 else sx] *= 0.85
    # Frame, glass (the sky reflected: lighter at the top, a diagonal glint), mullions
    box(rgb, x0, y0, x1, y1, FRAME, 0.03)
    yy, xx = np.mgrid[y0 + 1:y1 - 1, x0 + 1:x1 - 1]
    t = (yy - y0) / (y1 - y0)
    glass = np.stack([60 + 50 * (1 - t), 74 + 56 * (1 - t), 92 + 60 * (1 - t)], axis=-1)
    glint = np.exp(-((xx - x0 - (yy - y0) * 0.6 - 2) / 1.5) ** 2) * 40
    rgb[y0 + 1:y1 - 1, x0 + 1:x1 - 1] = glass + glint[:, :, None]
    rgb[y0 + 1:y1 - 1, (x0 + x1) // 2] = FRAME
    rgb[(y0 + y1) // 2 - 2, x0 + 1:x1 - 1] = FRAME
    # Sill (stone, its shadow under it), the lintel
    box(rgb, x0 - 1, y1, x1 + 1, y1 + 2, (196, 190, 176), 0.05)
    rgb[y1 + 2, x0 - 1:x1 + 1] *= 0.7
    box(rgb, x0 - 1, y0 - 2, x1 + 1, y0, (186, 180, 166), 0.05)
    return save(name, rgb)


def door_panel(name):
    w, h = PANEL_TEXTURE
    rgb = stucco_wall(w, h)
    x0, x1, y0 = 9, 23, 13
    # Frame, planks (vertical, grain), a small window, the handle, the stone threshold
    box(rgb, x0 - 1, y0 - 1, x1 + 1, h, FRAME, 0.03)
    yy, xx = np.mgrid[y0:h - 1, x0:x1]
    wood = colored(DOOR, fractal(x1 - x0, h - 1 - y0) * 0.06 + grain(x1 - x0, h - 1 - y0, 0.08))
    wood *= (1 + 0.08 * np.sin(yy * 0.9 + xx * 2.1) * 0.5)[:, :, None]
    wood[:, ::3] *= 0.78
    rgb[y0:h - 1, x0:x1] = wood
    box(rgb, x0 + 3, y0 + 3, x1 - 3, y0 + 8, (78, 92, 110), 0.05)
    rgb[y0 + 3, x0 + 3:x1 - 3] += 30
    rgb[y0 + 21:y0 + 23, x1 - 3] = (200, 168, 80)
    box(rgb, x0 - 2, y0 - 3, x1 + 2, y0 - 1, (186, 180, 166), 0.05)
    box(rgb, x0 - 1, h - 1, x1 + 1, h, (170, 166, 156), 0.05)
    return save(name, rgb)


def plain_panel(name):
    return save(name, stucco_wall(*PANEL_TEXTURE))


def boards(name):
    """Weathered wooden boards (horizontal, each overlapping the one under it)"""
    n = TILE_TEXTURE
    rgb = colored(BOARDS, fractal(n, n) * 0.05 + grain(n, n, 0.08))
    yy, xx = np.mgrid[0:n, 0:n]
    rgb *= (1 + 0.05 * np.sin(xx * 0.7 + RNG.random() * 6 + yy // 4 * 1.3))[:, :, None]
    for y in range(0, n, 4):
        rgb[y] *= 1.12                          # (the edge of a board)
        rgb[(y + 3) % n] *= 0.72                # (its shadow on the board under it)
    return save(name, rgb)


def painted_wood(name, color, w, h):
    """Painted boards (the underside of the roof: lengthwise, the fascia and the barge boards), the paint worn"""
    rgb = colored(color, fractal(w, h) * 0.04 + grain(w, h, 0.06))
    rgb[::4] *= 0.86
    return save(name, rgb)


def roof_tiles(name):
    """Clay tiles: rows of rounded tiles, the joints shifted from a row to the next, a tone for each tile"""
    n = TILE_TEXTURE
    rgb = np.zeros((n, n, 3))
    tile_w, tile_h = 8, 8
    for row in range(n // tile_h):
        shift = (row % 2) * tile_w // 2
        for col in range(-1, n // tile_w + 1):
            x0 = col * tile_w + shift
            tone = 1 + (RNG.random() - 0.5) * 0.16
            for y in range(tile_h):
                for x in range(tile_w):
                    px = x0 + x
                    if not 0 <= px < n:
                        continue
                    # (a rounded tile: lit in the middle, the lower edge in the shadow of the next row)
                    curve = 0.82 + 0.22 * math.sin(math.pi * (x + 0.5) / tile_w)
                    edge = 0.7 if y == tile_h - 1 else 1.0
                    rgb[row * tile_h + y, px] = np.asarray(CLAY) * tone * curve * edge
    rgb *= (1 + fractal(n, n) * 0.05 + grain(n, n, 0.08))[:, :, None]
    lichen = (RNG.random((n, n)) > 0.985)
    rgb[lichen] = rgb[lichen] * 0.6 + np.array([130, 128, 96]) * 0.4
    # The eave: the last row of tiles (one row on a quad of the height of a row); the verges: a column of the width of
    # a row (the edge of the roof over a gable), the corner: both
    save('eave', rgb[-TILE_ROW:])
    save('verge', rgb[:, :TILE_ROW])
    save('eave_verge', rgb[-TILE_ROW:, :TILE_ROW])
    return save(name, rgb)


def bricks(name):
    n = TILE_TEXTURE
    rgb = colored(MORTAR, grain(n, n, 0.08))
    for row in range(n // 4):
        shift = (row % 2) * 4
        for col in range(-1, n // 8 + 1):
            x0 = col * 8 + shift
            tone = 1 + (RNG.random() - 0.5) * 0.25
            for y in range(row * 4, row * 4 + 3):
                for x in range(x0, x0 + 7):
                    if 0 <= x < n:
                        rgb[y, x] = np.asarray(BRICK) * tone * (1 + (RNG.random() - 0.5) * 0.12)
    rgb *= (0.8 + 0.2 * np.arange(n)[:, None, None] / n)    # (soot at the top)
    return save(name, rgb)


def concrete(name):
    n = TILE_TEXTURE // 2
    return save(name, colored(CONCRETE, fractal(n, n) * 0.05 + grain(n, n, 0.1)))


def grass(name):
    """Lawn: large patches of light and darker green, fine blades at a low contrast"""
    n = TILE_TEXTURE
    rgb = colored(GRASS, fractal(n, n) * 0.08 + grain(n, n, 0.14))
    yy, xx = np.mgrid[0:n, 0:n]
    rgb[:, :, 1] *= 1 + 0.05 * np.sin(xx * 1.7 + yy * 0.4)
    dry = np.clip(smooth_noise(n, n, 3) - 0.65, 0, 1) * 1.6
    rgb = rgb * (1 - dry[:, :, None]) + np.array([118, 116, 62]) * dry[:, :, None]
    return save(name, rgb)


# ---------------------------------------------------------------- mesh

class Mesh:
    def __init__(self):
        self.faces = []         # (4 points, material, 4 uv, gouraud color)

    def quad(self, pts, material, outward, up):
        """A quad: its front outward (counterclockwise in the OBJ seen from outside), the top left of its texture on the
        corner at the left of its top edge seen from outside (up: the up direction of the texture)"""
        p = [np.asarray(q, dtype=float) for q in pts]
        n = np.cross(p[1] - p[0], p[2] - p[0]) + np.cross(p[2] - p[0], p[3] - p[0])
        if np.dot(n, outward) < 0:
            p = p[::-1]
            n = -n
        n /= np.linalg.norm(n)
        up = np.asarray(up, dtype=float)
        right = np.cross(up, n)
        c = sum(p) / 4
        uv = [(1.0 if np.dot(q - c, right) > 0 else 0.0, 1.0 if np.dot(q - c, up) > 0 else 0.0) for q in p]
        light = SHADE + (SUNLIT - SHADE) * max(0.0, float(np.dot(n, SUN / np.linalg.norm(SUN))))
        self.faces.append((p, material, uv, light))


def roof_height(z):
    return RIDGE - (RIDGE - WALL) * abs(z) / (DEPTH / 2)


def build(textures):
    m = Mesh()
    hx, hz = WIDTH / 2, DEPTH / 2
    up = (0, 1, 0)
    # Walls: panels of PANEL meters, a texture each (front: window, door, window, window)
    walls = [((0, 0, 1), ['window_a', 'door', 'window_b', 'window_a']),
             ((0, 0, -1), ['window_b', 'plain', 'window_a', 'window_b']),
             ((1, 0, 0), ['plain', 'window_a', 'plain']),
             ((-1, 0, 0), ['plain', 'window_b', 'plain'])]
    for normal, panels in walls:
        nx, _, nz = normal
        for k, mat in enumerate(panels):
            a = -len(panels) * PANEL / 2 + k * PANEL
            b = a + PANEL
            if nz:
                pts = [(a, 0, nz * hz), (b, 0, nz * hz), (b, WALL, nz * hz), (a, WALL, nz * hz)]
            else:
                pts = [(nx * hx, 0, a), (nx * hx, 0, b), (nx * hx, WALL, b), (nx * hx, WALL, a)]
            m.quad(pts, mat, normal, up)
    # Gables: the triangle under the roof in 2 quads (from the middle of its base to the ridge)
    for sx in (-1, 1):
        x = sx * hx
        mid_y = (WALL + RIDGE) / 2
        m.quad([(x, WALL, -hz), (x, WALL, 0), (x, RIDGE, 0), (x, mid_y, -hz / 2)], 'boards', (sx, 0, 0), up)
        m.quad([(x, WALL, 0), (x, WALL, hz), (x, mid_y, hz / 2), (x, RIDGE, 0)], 'boards', (sx, 0, 0), up)
    # Roof: each side in ROOF_PARTS quads along X between the gables, and in rows from the wall to the ridge (the top of the
    # texture toward the ridge). The parts over the walls (the eave, the verges over the gables) are quads of their own,
    # with a strip of the texture: sorted on their own centers, they are drawn after the wall under them (a quad from the
    # eave to the ridge was sorted behind the front wall, the wall was drawn over the edge of the roof)
    # (the vertical drop of the underside of the roof for a thickness ROOF_THICKNESS across the slope)
    slope_length = math.hypot(1, (RIDGE - WALL) / hz)
    columns = [-hx - VERGE] + [-hx + WIDTH * k / ROOF_PARTS for k in range(ROOF_PARTS + 1)] + [hx + VERGE]
    rows = [hz + EAVE, hz] + [hz * (1 - r / ROOF_ROWS) for r in range(1, ROOF_ROWS + 1)]
    for sz in (-1, 1):
        for k in range(len(columns) - 1):
            a, b = columns[k], columns[k + 1]
            verge = k == 0 or k == len(columns) - 2
            for r in range(len(rows) - 1):
                z0, z1 = sz * rows[r], sz * rows[r + 1]
                y0, y1 = roof_height(z0), roof_height(z1)
                mat = ('eave_verge' if verge else 'eave') if r == 0 else ('verge' if verge else 'tiles')
                top = [(a, y0, z0), (b, y0, z0), (b, y1, z1), (a, y1, z1)]
                slope = (0, 1, sz * (RIDGE - WALL) / hz)
                m.quad(top, mat, slope, (0, y1 - y0, z1 - z0))
                if not (verge or r == 0):
                    continue
                # The overhang has a thickness: its underside (seen from below, and from above when the slope is
                # turned away: a roof without thickness disappears there) and its edges (the barge boards along the
                # verges, the fascia along the eave). The underside lowered vertically: the 2 slopes meet at the ridge
                under = [(x, y - ROOF_THICKNESS * slope_length, z) for x, y, z in top]
                m.quad(under, 'soffit', tuple(-v for v in slope), (0, y1 - y0, z1 - z0))
                if k == 0:
                    m.quad([top[0], top[3], under[3], under[0]], 'fascia', (-1, 0, 0), (0, 1, 0))
                if k == len(columns) - 2:
                    m.quad([top[1], top[2], under[2], under[1]], 'fascia', (1, 0, 0), (0, 1, 0))
                if r == 0:
                    m.quad([top[0], top[1], under[1], under[0]], 'fascia', (0, 0, sz), (0, 1, 0))
    # Chimney on the ridge: its sides from the roof (their bottom edges follow the slopes, a side across the ridge in 2
    # quads), its top. No part of it under the roof (a part hidden by the roof was drawn over it: one depth per polygon)
    x0, x1, z0, z1, top = CHIMNEY
    m.quad([(x0, roof_height(z0), z0), (x1, roof_height(z0), z0), (x1, top, z0), (x0, top, z0)], 'brick', (0, 0, -1), up)
    m.quad([(x0, roof_height(z1), z1), (x1, roof_height(z1), z1), (x1, top, z1), (x0, top, z1)], 'brick', (0, 0, 1), up)
    for x, sx in ((x0, -1), (x1, 1)):
        m.quad([(x, roof_height(z0), z0), (x, RIDGE, 0), (x, top, 0), (x, top, z0)], 'brick', (sx, 0, 0), up)
        m.quad([(x, RIDGE, 0), (x, roof_height(z1), z1), (x, top, z1), (x, top, 0)], 'brick', (sx, 0, 0), up)
    m.quad([(x0, top, z0), (x1, top, z0), (x1, top, z1), (x0, top, z1)], 'concrete', up, (0, 0, -1))
    # Step of the door (its back against the wall: no face)
    s0, s1, depth, height = STEP
    front = hz + depth
    m.quad([(s0, height, hz), (s1, height, hz), (s1, height, front), (s0, height, front)], 'step', up, (0, 0, -1))
    m.quad([(s0, 0, front), (s1, 0, front), (s1, height, front), (s0, height, front)], 'step', (0, 0, 1), up)
    for x, sx in ((s0, -1), (s1, 1)):
        m.quad([(x, 0, hz), (x, 0, front), (x, height, front), (x, height, hz)], 'step', (sx, 0, 0), up)
    # Lawn around the house (none under it): quads of LAWN meters
    cx = int(WIDTH / LAWN) // 2 + LAWN_BORDER
    cz = int(DEPTH / LAWN) // 2 + LAWN_BORDER
    for i in range(-cx, cx):
        for j in range(-cz, cz):
            a, b = i * LAWN, (i + 1) * LAWN
            c, d = j * LAWN, (j + 1) * LAWN
            if -hx <= a and b <= hx and -hz <= c and d <= hz:
                continue
            m.quad([(a, 0, c), (b, 0, c), (b, 0, d), (a, 0, d)], 'lawn', up, (0, 0, -1))
    return m


def write(m, textures):
    # Sorts of some materials (jo_sort, see jo_model.py): none, the converter chooses the sort of each polygon (the
    # gables and the door behind what is against them, the step and the chimney in front of what they touch...)
    flags = {}
    with open(os.path.join(HERE, 'house.mtl'), 'w', newline='\n') as f:
        for name in sorted({face[1] for face in m.faces}):
            f.write('newmtl %s\nKd 1 1 1\nmap_Kd %s\n%s\n' % (name, textures[name], flags.get(name, '')))
    with open(os.path.join(HERE, 'house.obj'), 'w', newline='\n') as f:
        f.write('# Made by make_house.py\nmtllib house.mtl\n')
        # (the points of each face: its own gouraud color, the sun baked)
        for p, mat, uv, light in m.faces:
            for q in p:
                f.write('v %.4f %.4f %.4f %.3f %.3f %.3f\n' % (q[0], q[1], q[2], light, light, light))
        for p, mat, uv, light in m.faces:
            for u, v in uv:
                f.write('vt %g %g\n' % (u, v))
        current = None
        for k, (p, mat, uv, light) in enumerate(m.faces):
            if mat != current:
                f.write('usemtl %s\n' % mat)
                current = mat
            f.write('f %s\n' % ' '.join('%d/%d' % (k * 4 + i + 1, k * 4 + i + 1) for i in range(4)))


def main():
    textures = {'window_a': window_panel('window_a'), 'window_b': window_panel('window_b'), 'door': door_panel('door'),
                'plain': plain_panel('plain'), 'boards': boards('boards'), 'tiles': roof_tiles('tiles'),
                'brick': bricks('brick'), 'concrete': concrete('concrete'), 'lawn': grass('lawn')}
    for name in ('eave', 'verge', 'eave_verge'):
        textures[name] = name + '.png'
    textures['step'] = textures['concrete']
    textures['soffit'] = painted_wood('soffit', SOFFIT, TILE_TEXTURE, TILE_TEXTURE)
    textures['fascia'] = painted_wood('fascia', FASCIA, TILE_TEXTURE, TILE_ROW)
    m = build(textures)
    write(m, textures)
    print('%d polygons' % len(m.faces))
    # The painter's sort of the VDP1 against a z-buffer all around the house (the camera of main.c looks at it from
    # above): a polygon drawn over a nearer one stops here
    with tempfile.TemporaryDirectory() as tmp:
        jom = os.path.join(tmp, 'HOUSE.JOM')
        jo_model.convert(os.path.join(HERE, 'house.obj'), jom, scale=JOM_SCALE, embed_textures=True, quiet=True)
        if jo_model.check_sort(jom, SORT_CHECK_ELEVATIONS, image=os.path.join(tmp, 'sort.png')):
            sys.exit('the painter\'s sort draws polygons over nearer ones (see above)')


if __name__ == '__main__':
    main()
