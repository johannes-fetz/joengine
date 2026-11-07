"""Anti-glitch checks of the 3D (tests/rom/t_glitch.c): absolute checks, each frame is right or wrong by itself.

The test ROM draws known scenes (a closed room seen from inside, a ground of quads, single quads) from known cameras, over
a magenta background that no texture uses (the NBG1 bitmap behind the sprites of the VDP1). The views are in
tests/rom/t_glitch_views.h: written by this module (python3 tests/harness/glitch.py --write-views, the sweeps are in
make_views()) and read again by the checks, the ROM and the checks use the same table.

For each view the expected picture is computed from the scene, with the projection of the engine (see 3d.c,
__jo_3d_look_at(), __jo_3d_set_perspective(): projection distance (width / 2) / tan(45 degrees) = 160 pixels, center
(160, 120), near plane at the projection distance / 2^3 = 20 units, see __jo_vdp1_update_near_clip()):

- cover (the room, the ground): a ray per pixel; a pixel whose ray hits the geometry in front of the near plane must be
  drawn. Hole: a magenta pixel 2 pixels or more inside the region that must be drawn. Spurious: a pixel drawn 2 pixels or
  more outside of it (something behind the near plane drawn)
- quad (one quad): its polygon clipped by the near plane, projected. Deformation: a pixel drawn more than 2 pixels out of
  it, hole: a pixel missing more than 2 pixels inside it. A twisted quad or a quad seen edge on (HULL): only the pixels
  outside of the convex hull of its corners
- mapping (a textured quad): the texture is a gradient (red = column, green = line, blue = 31 - column: every texel has
  its own color, the blue checks the color), the texel of each pixel is compared with the perspective correct texel
  (ray / plane): the VDP1 maps the texture without perspective on each part, the error is measured (fraction of the
  pixels out of MAP_TEXELS + MAP_PIXELS converted into texels, quadrants of the texture swapped; in the room, the pixels
  MARGIN pixels or less from an edge between 2 faces are not measured: the polygons are grown by 1 pixel)
- mip (the ground with smaller copies of its texture, also flipped): each quad drawn as one command is mapped by the VDP1
  bilinearly from its 4 corners on the screen: the texel of each pixel is known exactly (the column, flipped or not, the
  line), within the blur of the copies
- colors: every pixel drawn is a color of the scene (a texel of the gradient or its averages, the flat colors)
- slave: a view drawn by the slave CPU is the same picture as the view drawn by the master CPU

A view flagged DEFECT draws a known defect (a quad missing, a crack, a vertex moved, the texture not flipped) and a view
flagged CONTROL is a known bad case (a textured quad through the near plane without subdivision): their checks must fail,
else the check is broken (a FAIL). See tests/README.md.
"""

import math
import os
import re
import sys

if __package__ in (None, ""):
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from harness import media  # noqa: E402
else:
    from . import media

TESTS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VIEWS_HEADER = os.path.join(TESTS_DIR, "rom", "t_glitch_views.h")

LAYER = "3 glitch"

# Screen of the test ROM 320x224 (its default resolution: 320 x 240, JO_TV_HEIGHT) and projection of the engine (checked by the ROM: t_glitch_init())
SCREEN_W = 320
SCREEN_H = 240
CX = 160.0
CY = 120.0
PD = 160.0
NEAR = 20.0

MARGIN = 2              # pixels: the border of the expected regions not checked (rounding of the VDP1 coordinates)
MAP_TEXELS = 1.5        # texels: error of the texture mapping always counted as good (the gradient decodes 1 texel)
MAP_PIXELS = 10         # pixels: error of the texture mapping counted as good (JO_3D_SUBDIVISION_ERROR 8 + 2)
MAP_MIN_GOOD = 0.95     # fraction of the pixels mapped within the tolerance (subdivided textures)
ALLOWED_Z = 1.0         # units: a quad may be drawn from this depth (the parts through the near plane drawn whole)
MAP_MAX_QUADRANT = 0.01 # fraction of the pixels in the wrong quadrant of the texture
MIP_TOLERANCE = 3.5     # texels: the copies average 2 x 2 and 4 x 4 texels (+ the rounding)
MIP_MAX_BAD = 0.002     # fraction of the pixels of the mipmapped ground with a wrong texel
MIP_MIN_SIZE = 6        # pixels: the smaller quads of the mipmapped ground are not checked
MIP_INSET = 1.5         # pixels: the border of each quad not checked (its neighbor may be drawn over it)

FLAGS = {"QUICK": 1, "SLAVE": 2, "DEFECT": 4, "HULL": 8, "CONTROL": 16}

# ----------------------------------------------------------------------------------------------------------------------
# The views (written into tests/rom/t_glitch_views.h)
# ----------------------------------------------------------------------------------------------------------------------

DEFINES = {
    "G_ROOM_HALF": 200,         # the room: a box of 400 units, the camera inside
    "G_ROOM_N": 8,              # quads per side of a face of ROOM_GRID / ROOM_FLAT (ROOM_BIG: 1)
    "G_GROUND_HALF": 480,       # the ground: 960 x 960 units at y = 0
    "G_GROUND_N": 24,           # quads per side (40 units)
    "G_TEX": 32,                # the gradient texture: 32 x 32
    "G_FRAMES": 9,              # frames of a view (after its marker; the last 3: nothing drawn, black background)
}

# Flat colors (8 bits): 0-5 the faces of ROOM_FLAT, 6-7 the checker of GROUND_FLAT, 8 the flat quads
FLAT_COLORS = [(200, 200, 200), (64, 168, 64), (64, 64, 200), (200, 200, 64), (64, 200, 200), (136, 88, 40),
               (88, 168, 88), (152, 152, 88), (40, 224, 40)]

SCENES = ["ROOM_GRID", "ROOM_FLAT", "ROOM_BIG", "ROOM_BIG_PLAIN", "GROUND_FLAT", "GROUND_TEX", "GROUND_MIP",
          "GROUND_MIP_FLIP", "QUAD"]
MODES = ["NONE", "FLAT", "PLAIN", "SUB", "MIP"]

_SHORT = {"ROOM_GRID": "rg", "ROOM_FLAT": "rf", "ROOM_BIG": "rb", "ROOM_BIG_PLAIN": "rp", "GROUND_FLAT": "gf",
          "GROUND_TEX": "gt", "GROUND_MIP": "gm", "GROUND_MIP_FLIP": "gx"}


def _look(cam, yaw, pitch, dist=100.0):
    """Target of a camera: yaw 0 looks along +z, 90 along +x; pitch > 0 looks up (-y)."""
    y = math.radians(yaw)
    p = math.radians(pitch)
    d = (math.sin(y) * math.cos(p), -math.sin(p), math.cos(y) * math.cos(p))
    return tuple(c + dist * k for c, k in zip(cam, d))


def _rot_y(points, center, degrees):
    a = math.radians(degrees)
    out = []
    for x, y, z in points:
        dx, dz = x - center[0], z - center[2]
        out.append((center[0] + dx * math.cos(a) + dz * math.sin(a), y, center[2] - dx * math.sin(a) + dz * math.cos(a)))
    return out


def make_views():
    """The views of the ROM: (name, scene, mode, flags, camera, target, roll, quad), and the quads."""
    H = DEFINES["G_ROOM_HALF"]
    views = []

    def add(name, scene, mode, flags, cam, target, roll=0, quad=-1):
        views.append((name, scene, mode, flags, tuple(cam), tuple(target), roll, quad))

    # The room: from the center to the walls, the edges and the corners, nearer than the near plane
    room = [("c_fwd", (0, 5, 0), 0, 0, 0), ("c_corner", (0, 5, 0), None, None, 0), ("c_up", (0, 5, 0), 30, 70, 0),
            ("c_roll", (0, 5, 0), 20, 10, 30)]
    for d in (60, 25, 21, 19, 12, 6, 2, 0.5):
        tag = ("%g" % d).replace(".", "p")
        cam = (10, 5, H - d)
        room += [("w%s_n" % tag, cam, 0, 0, 0), ("w%s_o" % tag, cam, 50, -15, 0), ("w%s_g" % tag, cam, 85, 0, 0)]
    for d in (30, 10, 3):
        cam = (H - d, 5, H - d)
        room += [("e%d_in" % d, cam, 45, 0, 0), ("e%d_up" % d, cam, 45, 40, 0)]
    for d in (30, 8, 2):
        room += [("k%d" % d, (H - d, -(H - d), H - d), None, None, 0)]
    for d in (10, 2):
        room += [("f%d_dn" % d, (0, H - d, 0), 0, -20, 0), ("f%d_fw" % d, (0, H - d, 0), 30, 0, 0)]
    room += [("w10_roll", (0, 0, H - 10), 30, 20, 45)]
    near_poses = set(n for n, cam, *_ in room if n.startswith(("w6", "w2", "w0p5", "e3", "k2", "f2")))
    quick = {("ROOM_GRID", "c_fwd"), ("ROOM_GRID", "w6_o"), ("ROOM_BIG", "w0p5_g")}
    for scene in ("ROOM_GRID", "ROOM_BIG", "ROOM_FLAT", "ROOM_BIG_PLAIN"):
        for name, cam, yaw, pitch, roll in room:
            if scene in ("ROOM_FLAT", "ROOM_BIG_PLAIN") and name not in near_poses and name != "c_fwd":
                continue
            target = (H, -H, H) if yaw is None else _look(cam, yaw, pitch)
            add("g_%s_%s" % (_SHORT[scene], name), scene, "NONE", ["QUICK"] if (scene, name) in quick else [],
                cam, target, roll)
    # The ground: very low cameras over a vertex, an edge, the center of a quad; toward and below the horizon
    ground = []
    for h in (40, 10, 2.5, 0.75):
        tag = ("%g" % h).replace(".", "p")
        ground += [("h%s_v" % tag, (0, -h, 0), 0, 0), ("h%s_e" % tag, (20, -h, 0), 30, -15),
                   ("h%s_c" % tag, (20, -h, 20), 90, -45), ("h%s_up" % tag, (0, -h, 0), 45, 8)]
    for scene in ("GROUND_FLAT", "GROUND_TEX"):
        for name, cam, yaw, pitch in ground:
            add("g_%s_%s" % (_SHORT[scene], name), scene, "NONE",
                ["QUICK"] if (scene, name) == ("GROUND_TEX", "h2p5_e") else [], cam, _look(cam, yaw, pitch))
    # The mipmapped ground, flipped or not (far enough: the quads drawn whole)
    mip = [("a", (0, -30, -300), 0, -10), ("b", (40, -30, -300), 20, -12), ("c", (0, -60, -300), 0, -25)]
    for scene in ("GROUND_MIP", "GROUND_MIP_FLIP"):
        for name, cam, yaw, pitch in mip:
            add("g_%s_%s" % (_SHORT[scene], name), scene, "NONE",
                ["QUICK"] if (scene, name) == ("GROUND_MIP_FLIP", "a") else [], cam, _look(cam, yaw, pitch))
    # Single quads (camera at the origin looking along +z, unless given)
    quads = []

    def quad(points):
        quads.append([tuple(p) for p in points])
        return len(quads) - 1

    origin = ((0, 0, 0), (0, 0, 100), 0)
    q = {}
    q["q0"] = quad([(-60, -40, 200), (60, -40, 200), (60, 40, 200), (-60, 40, 200)])
    q["q1"] = quad(_rot_y([(-60, -40, 200), (60, -40, 200), (60, 40, 200), (-60, 40, 200)], (0, 0, 200), -60))
    q["q2"] = quad(_rot_y([(-60, -40, 200), (60, -40, 200), (60, 40, 200), (-60, 40, 200)], (0, 0, 200), -86))
    q["q3"] = quad([(-50, 40, 300), (50, 40, 300), (50, 40, 2), (-50, 40, 2)])
    q["q4"] = quad([(-50, 40, 300), (50, 40, 300), (50, 40, -100), (-50, 40, -100)])
    q["q5"] = quad([(30, -60, 400), (30, -60, -50), (30, 60, -50), (30, 60, 400)])
    q["q6"] = quad([(-20, -15, 21), (20, -15, 21), (20, 15, 21), (-20, 15, 21)])
    q["q7"] = quad([(-30, -25, 40), (30, -25, 40), (30, 25, 10), (-30, 25, 10)])
    q["q8"] = quad([(-2000, -30, 100), (2000, -30, 100), (2000, 30, 100), (-2000, 30, 100)])
    q["q10"] = quad([(-50, -40, 200), (50, -40, 200), (50, 40, 260), (-50, 40, 200)])
    q["q11"] = quad([(0, -40, 100), (0, -40, 300), (0, 40, 300), (0, 40, 100)])
    q["q12"] = quad([(-10, -10, 2000), (10, -10, 2000), (10, 10, 2000), (-10, 10, 2000)])
    cams = {k: origin for k in q}
    cams["q9"] = ((0, 0, 0), _look((0, 0, 0), 20, -10), 30)
    q["q9"] = q["q3"]
    cams["q13"] = ((25, 0, 0), _look((25, 0, 0), 30, 0), 0)
    q["q13"] = q["q5"]
    cams["q14"] = ((0, -5, 0), _look((0, -5, 0), -25, -30), 0)
    q["q14"] = q["q4"]
    order = ["q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q13", "q14", "q10", "q11", "q12"]
    for name in order:
        cam, target, roll = cams[name]
        hull = ["HULL"] if name in ("q10", "q11") else []
        modes = ["FLAT", "SUB"] if name not in ("q12",) else ["FLAT"]
        for mode in modes:
            flags = list(hull)
            if (name, mode) in (("q3", "SUB"), ("q5", "SUB")):
                flags.append("QUICK")
            add("g_%s_%s" % (name, mode.lower()), "QUAD", mode, flags, cam, target, roll, q[name])
    # Without subdivision: the quads in front of the near plane (informative), the quads through it (CONTROL: the whole
    # texture in the visible part, the mapping check must fail)
    for name in ("q0", "q1"):
        cam, target, roll = cams[name]
        add("g_%s_plain" % name, "QUAD", "PLAIN", [], cam, target, roll, q[name])
    for name in ("q3", "q7", "q13"):
        cam, target, roll = cams[name]
        add("g_%s_plain_ctl" % name, "QUAD", "PLAIN", ["CONTROL"] + (["QUICK"] if name == "q3" else []), cam, target,
            roll, q[name])
    # A mipmapped quad far away and flipped
    add("g_q0_mip", "QUAD", "MIP", [], (0, 0, -900), (0, 0, 100), 0, q["q0"])
    # The known defects (the checks must fail)
    add("g_q0_flat_defect", "QUAD", "FLAT", ["DEFECT", "QUICK"], origin[0], origin[1], 0, q["q0"])
    add("g_rg_defect", "ROOM_GRID", "NONE", ["DEFECT"], (0, 5, 0), _look((0, 5, 0), 0, 0))
    add("g_gf_defect", "GROUND_FLAT", "NONE", ["DEFECT"], (0, -10, 0), _look((0, -10, 0), 0, -15))
    add("g_gx_defect", "GROUND_MIP_FLIP", "NONE", ["DEFECT"], mip[0][1], _look(mip[0][1], mip[0][2], mip[0][3]))
    # The slave CPU: the same views as the master CPU
    for scene, pose in (("ROOM_GRID", "c_fwd"), ("ROOM_GRID", "w2_o"), ("ROOM_GRID", "k2"), ("GROUND_TEX", "h2p5_e")):
        master = [v for v in views if v[0] == "g_%s_%s" % (_SHORT[scene], pose)][0]
        add(master[0] + "_slave", scene, "NONE", ["SLAVE"], master[4], master[5], master[6])
    return views, quads


def _num(x):
    s = "%.3f" % x
    return s if "." in s else s + ".0"


def write_views(path=VIEWS_HEADER):
    views, quads = make_views()
    lines = ["/*",
             "** Jo Sega Saturn Engine - regression test ROM: the views of the anti-glitch checks (t_glitch.c)",
             "**",
             "** Written by tests/harness/glitch.py --write-views (edit make_views() there): the checks read this file",
             "*/", "", "#ifndef __T_GLITCH_VIEWS_H__", "# define __T_GLITCH_VIEWS_H__", ""]
    for k, v in DEFINES.items():
        lines.append("# define %-24s(%d)" % (k, v))
    lines.append("")
    lines.append("/* Flat colors (8 bits): the faces of ROOM_FLAT, the checker of GROUND_FLAT, the flat quads */")
    lines.append("static const unsigned char        g_flat_colors[][3] =")
    lines.append("{")
    for c in FLAT_COLORS:
        lines.append("    G_COLOR(%d, %d, %d)," % c)
    lines.append("};")
    lines.append("")
    lines.append("/* Quads: 4 corners (world units), clockwise seen from the front (Y down) */")
    lines.append("static const jo_fixed             g_quads[][4][3] =")
    lines.append("{")
    for i, qd in enumerate(quads):
        lines.append("    G_QUAD(%s), /* %d */" % (", ".join(_num(c) for p in qd for c in p), i))
    lines.append("};")
    lines.append("")
    lines.append("/* Views: name, scene, mode of the quad, flags, camera, target, roll (degrees), quad (-1: none) */")
    lines.append("static const g_view               g_views[] =")
    lines.append("{")
    for name, scene, mode, flags, cam, target, roll, quad in views:
        lines.append("    G_VIEW(\"%s\", %s, %s, %s, %s, %s, %d, %d)," % (
            name, scene, mode, "|".join("G_F_" + f for f in flags) if flags else "0",
            ", ".join(_num(c) for c in cam), ", ".join(_num(c) for c in target), roll, quad))
    lines.append("};")
    lines += ["", "#endif /* !__T_GLITCH_VIEWS_H__ */", ""]
    with open(path, "w", newline="\n") as f:
        f.write("\n".join(lines))
    return len(views)


def _fixed(s):
    """The value of JO_FLOAT_TO_FIXED() (truncated toward 0), in units."""
    return int(float(s) * 65536.0) / 65536.0


def read_views(path=VIEWS_HEADER):
    """The views of the header: (defines, {name: view}, quads, flat colors 5 bits)."""
    with open(path, "r") as f:
        text = f.read()
    defines = {m.group(1): int(m.group(2)) for m in re.finditer(r"#\s*define\s+(G_\w+)\s+\((-?\d+)\)", text)}
    colors = [tuple(int(c) >> 3 for c in m.groups()) for m in re.finditer(r"G_COLOR\((\d+), (\d+), (\d+)\)", text)]
    quads = []
    for m in re.finditer(r"G_QUAD\(([^)]*)\)", text):
        v = [_fixed(x) for x in m.group(1).split(",")]
        quads.append([tuple(v[i:i + 3]) for i in range(0, 12, 3)])
    views = {}
    for i, m in enumerate(re.finditer(r'G_VIEW\("([^"]+)", (\w+), (\w+), ([\w|]+), ([^)]*)\)', text)):
        rest = [x.strip() for x in m.group(5).split(",")]
        flags = 0 if m.group(4) == "0" else sum(FLAGS[f[4:]] for f in m.group(4).split("|"))
        views[m.group(1)] = {"index": i, "name": m.group(1), "scene": m.group(2), "mode": m.group(3), "flags": flags,
                             "cam": tuple(_fixed(x) for x in rest[0:3]), "target": tuple(_fixed(x) for x in rest[3:6]),
                             "roll": int(rest[6]), "quad": int(rest[7])}
    return defines, views, quads, colors


# ----------------------------------------------------------------------------------------------------------------------
# Geometry (the same as the engine)
# ----------------------------------------------------------------------------------------------------------------------

def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _norm(a):
    n = math.sqrt(_dot(a, a))
    return (a[0] / n, a[1] / n, a[2] / n)


class Camera:
    """The matrix of jo_3d_camera_look_at() (see __jo_3d_look_at()): rows right, down, forward, then the roll."""

    def __init__(self, cam, target, roll):
        f = _norm(_sub(target, cam))
        up = (0.0, -1.0, 0.0) if (f[0] != 0 or f[2] != 0) else (0.0, 0.0, 1.0)
        r = _norm(_cross(f, up))
        u = _cross(f, r)
        if roll:
            a = math.radians(roll)
            s, c = math.sin(a), math.cos(a)
            r, u = (tuple(c * r[i] - s * u[i] for i in range(3)), tuple(s * r[i] + c * u[i] for i in range(3)))
        self.rows = (r, u, f)
        self.pos = tuple(cam)

    def view(self, p):
        d = _sub(p, self.pos)
        return (_dot(self.rows[0], d), _dot(self.rows[1], d), _dot(self.rows[2], d))

    def ray(self, px, py):
        """World direction of the ray of a pixel (its view space z is 1: the distance along it is the depth)."""
        dx = (px + 0.5 - CX) / PD
        dy = (py + 0.5 - CY) / PD
        r, u, f = self.rows
        return (r[0] * dx + u[0] * dy + f[0], r[1] * dx + u[1] * dy + f[1], r[2] * dx + u[2] * dy + f[2])


def project_floor(v):
    """Screen position of a view space point as the engine computes it (floor(x * pd / z) + center)."""
    k = PD / v[2]
    return (math.floor(v[0] * k) + CX, math.floor(v[1] * k) + CY)


def clip_z(points, z):
    """Sutherland-Hodgman against z >= z (view space)."""
    out = []
    n = len(points)
    for i in range(n):
        a = points[i]
        b = points[(i + 1) % n]
        ina = a[2] >= z
        inb = b[2] >= z
        if ina:
            out.append(a)
        if ina != inb:
            t = (z - a[2]) / (b[2] - a[2])
            out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, z))
    return out


def clip_near(points):
    return clip_z(points, NEAR)


def convex_hull(points):
    pts = sorted(set(points))
    if len(pts) <= 2:
        return pts

    def turn(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower = []
    for p in pts:
        while len(lower) >= 2 and turn(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper = []
    for p in reversed(pts):
        while len(upper) >= 2 and turn(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def signed_distance(poly, x, y):
    """Distance of a point to a convex polygon (positive inside). A polygon of 1 or 2 points: minus the distance."""
    n = len(poly)
    best = 1e30
    inside = n >= 3
    sign = 0
    for i in range(n):
        ax, ay = poly[i]
        bx, by = poly[(i + 1) % n]
        ex, ey = bx - ax, by - ay
        L = ex * ex + ey * ey
        t = 0.0 if L == 0 else max(0.0, min(1.0, ((x - ax) * ex + (y - ay) * ey) / L))
        dx, dy = ax + ex * t - x, ay + ey * t - y
        best = min(best, dx * dx + dy * dy)
        if n >= 3:
            c = ex * (y - ay) - ey * (x - ax)
            if c != 0:
                s = 1 if c > 0 else -1
                if sign == 0:
                    sign = s
                elif s != sign:
                    inside = False
    d = math.sqrt(best)
    return d if inside else -d


def _scene_geometry(defines, scene):
    """The quads of a room or ground scene (world corners, clockwise from the front) and their colors (None: texture)."""
    out = []
    if scene.startswith("ROOM"):
        H = float(defines["G_ROOM_HALF"])
        n = 1 if scene.startswith("ROOM_BIG") else defines["G_ROOM_N"]
        faces = [((-H, -H, H), (1, 0, 0), (0, 1, 0)), ((H, -H, -H), (-1, 0, 0), (0, 1, 0)),
                 ((H, -H, H), (0, 0, -1), (0, 1, 0)), ((-H, -H, -H), (0, 0, 1), (0, 1, 0)),
                 ((-H, H, H), (1, 0, 0), (0, 0, -1)), ((-H, -H, -H), (1, 0, 0), (0, 0, 1))]
        step = 2 * H / n
        for fi, (o, a, b) in enumerate(faces):
            for j in range(n):
                for i in range(n):
                    def p(ii, jj):
                        return tuple(o[k] + a[k] * ii * step + b[k] * jj * step for k in range(3))
                    out.append(([p(i, j), p(i + 1, j), p(i + 1, j + 1), p(i, j + 1)], fi))
    else:
        G = float(defines["G_GROUND_HALF"])
        n = defines["G_GROUND_N"]
        step = 2 * G / n
        for j in range(n):
            for i in range(n):
                def p(ii, jj):
                    return (-G + ii * step, 0.0, G - jj * step)
                out.append(([p(i, j), p(i + 1, j), p(i + 1, j + 1), p(i, j + 1)], 6 + ((i + j) & 1)))
    return out


# ----------------------------------------------------------------------------------------------------------------------
# Pictures
# ----------------------------------------------------------------------------------------------------------------------

class Frame:
    """The 320 x 240 picture of the ROM in a frame of Mednafen (scaled and centered, see barcode._geometries()): 5 bits
    components per pixel."""

    def __init__(self, image):
        sx = image.width // SCREEN_W
        sy = image.height // SCREEN_H
        ox = (image.width - SCREEN_W * sx) // 2
        oy = (image.height - SCREEN_H * sy) // 2
        self.image = image
        self.rgb = []
        data = image.data
        w = image.width
        for y in range(SCREEN_H):
            row = []
            base = ((oy + y * sy) * w + ox) * 3
            for x in range(SCREEN_W):
                i = base + x * sx * 3
                row.append((data[i] >> 3, data[i + 1] >> 3, data[i + 2] >> 3))
            self.rgb.append(row)
        self.magenta = [[c[0] >= 28 and c[1] <= 3 and c[2] >= 28 for c in row] for row in self.rgb]

    def picture(self):
        data = bytearray()
        for row in self.rgb:
            for r, g, b in row:
                data += bytes(((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)))
        return media.Image(SCREEN_W, SCREEN_H, bytes(data))


def _erode(mask, radius):
    """True where the mask is true in all the square of the radius (the edges of the screen repeated)."""
    h = len(mask)
    w = len(mask[0])
    tmp = []
    for row in mask:
        out = []
        for x in range(w):
            out.append(all(row[min(w - 1, max(0, x + d))] for d in range(-radius, radius + 1)))
        tmp.append(out)
    res = []
    for y in range(h):
        res.append([all(tmp[min(h - 1, max(0, y + d))][x] for d in range(-radius, radius + 1)) for x in range(w)])
    return res


def _crop_zoom(frame, points, color=(255, 255, 0), zoom=4, margin=8, max_size=80):
    """A zoomed crop of the picture around the points (their pixels marked) next to the picture itself."""
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    x0 = max(0, min(xs) - margin)
    y0 = max(0, min(ys) - margin)
    x1 = min(SCREEN_W, max(xs) + margin + 1)
    y1 = min(SCREEN_H, max(ys) + margin + 1)
    if x1 - x0 > max_size:
        cx = sorted(xs)[len(xs) // 2]
        x0 = max(0, min(SCREEN_W - max_size, cx - max_size // 2))
        x1 = x0 + max_size
    if y1 - y0 > max_size:
        cy = sorted(ys)[len(ys) // 2]
        y0 = max(0, min(SCREEN_H - max_size, cy - max_size // 2))
        y1 = y0 + max_size
    marked = set(points)
    w = (x1 - x0) * zoom
    rows = []
    for y in range(y0, y1):
        line = bytearray()
        right = bytearray()
        for x in range(x0, x1):
            r, g, b = frame.rgb[y][x]
            px = bytes(((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)))
            mk = bytes(color) if (x, y) in marked else bytes((v // 3 for v in px))
            line += px * zoom
            right += mk * zoom
        line += right
        for _ in range(zoom):
            rows.append(bytes(line))
    return media.Image(w * 2, len(rows), b"".join(rows)), (x0, y0, x1, y1)


# ----------------------------------------------------------------------------------------------------------------------
# Checks
# ----------------------------------------------------------------------------------------------------------------------

def _valid_gradient(c):
    return 29 <= c[0] + c[2] <= 31


class Check:
    """The result of a view: problems (each one: a message and its pixels), measures."""

    def __init__(self, view):
        self.view = view
        self.problems = []      # (kind, message, pixels)
        self.measures = []
        self.worst = {}

    def problem(self, kind, message, pixels):
        self.problems.append((kind, message, pixels))

    def measure(self, text):
        self.measures.append(text)


def _bbox_text(points):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return "x %d-%d, y %d-%d" % (min(xs), max(xs), min(ys), max(ys))


class Mapping:
    """The texels drawn against the perspective correct texels. The tolerance of a pixel: MAP_TEXELS + MAP_PIXELS pixels
    (the distortion of the parts allowed by the engine, JO_3D_SUBDIVISION_ERROR = 8, + the rounding) converted into
    texels at this pixel (the texels per pixel there)."""

    def __init__(self, extra=0.0, wrap=False):
        self.total = 0
        self.good = 0
        self.quadrant = 0
        self.err_sum = 0.0
        self.bad = []
        self.extra = extra
        self.wrap = wrap        # the cells of a grid have the same texture: a texel of the cell next to it is near

    def add(self, x, y, c, uv, uv_dx, uv_dy, tex=32.0):
        eu, ev = uv[0] * tex, uv[1] * tex
        tpp = tex * max(abs(uv_dx[0] - uv[0]), abs(uv_dx[1] - uv[1]), abs(uv_dy[0] - uv[0]), abs(uv_dy[1] - uv[1]))
        tol = MAP_TEXELS + MAP_PIXELS * tpp + self.extra
        if self.wrap:
            err = max(min(abs(c[0] + 0.5 + k - eu) for k in (-tex, 0, tex)), min(abs(c[1] + 0.5 + k - ev) for k in (-tex, 0, tex)))
        else:
            err = max(abs(c[0] + 0.5 - eu), abs(c[1] + 0.5 - ev))
        self.total += 1
        self.err_sum += err / max(tpp, 1e-3)
        if err <= tol:
            self.good += 1
        else:
            self.bad.append((x, y))
            # (a quadrant of the texture swapped: a pixel out of the tolerance and farther than it from the middle lines of
            # the texture)
            if (abs(eu - 16) > tol and (c[0] >= 16) != (eu >= 16)) or (abs(ev - 16) > tol and (c[1] >= 16) != (ev >= 16)):
                self.quadrant += 1

    def report(self, chk, enforce):
        if self.total == 0:
            return
        fgood = self.good / self.total
        fquad = self.quadrant / self.total
        chk.measure("mapping: %d pixels, %.1f %% within %.1f texels + %d pixels, mean error %.1f pixels, %.1f %% in a "
                    "wrong quadrant of the texture" % (self.total, 100 * fgood, MAP_TEXELS + self.extra, MAP_PIXELS,
                                                         self.err_sum / self.total, 100 * fquad))
        chk.worst["map_good"] = fgood
        chk.worst["map_quadrant"] = fquad
        if enforce and (fgood < MAP_MIN_GOOD or fquad > MAP_MAX_QUADRANT):
            chk.problem("mapping", "texture mapping: %.1f %% of the pixels within the tolerance (%.0f %% needed), %.1f %% in "
                        "a wrong quadrant (%.0f %% at most)" % (100 * fgood, 100 * MAP_MIN_GOOD, 100 * fquad,
                                                                100 * MAP_MAX_QUADRANT), self.bad)


def _ray_hit(scene_room, cam, x, y, H, G):
    """The point of the room (face index, point) or of the ground hit by the ray of a pixel center: (t, face, point) or None.
    x, y: floats (the center: + 0.5 added by Camera.ray())."""
    w = cam.ray(x, y)
    if scene_room:
        t = 1e30
        face = None
        for k in range(3):
            if w[k] > 1e-12:
                tk = (H - cam.pos[k]) / w[k]
                if tk < t:
                    t, face = tk, (k, 1)
            elif w[k] < -1e-12:
                tk = (-H - cam.pos[k]) / w[k]
                if tk < t:
                    t, face = tk, (k, -1)
        return t, face, tuple(cam.pos[i] + w[i] * t for i in range(3))
    if w[1] > 1e-12 and cam.pos[1] < 0:
        t = -cam.pos[1] / w[1]
        p = tuple(cam.pos[i] + w[i] * t for i in range(3))
        if -G <= p[0] <= G and -G <= p[2] <= G:
            return t, None, p
    return None


# The faces of the room (see _scene_geometry()): (axis, side) -> origin, A, B (unit vectors)
def _room_faces(H):
    return {(2, 1): ((-H, -H, H), (1, 0, 0), (0, 1, 0)), (2, -1): ((H, -H, -H), (-1, 0, 0), (0, 1, 0)),
            (0, 1): ((H, -H, H), (0, 0, -1), (0, 1, 0)), (0, -1): ((-H, -H, -H), (0, 0, 1), (0, 1, 0)),
            (1, 1): ((-H, H, H), (1, 0, 0), (0, 0, -1)), (1, -1): ((-H, -H, -H), (1, 0, 0), (0, 0, 1))}


def check_cover(view, frame, defines, flat_colors, chk):
    """The room and the ground: no hole inside the region that must be drawn (the geometry in front of the near plane),
    nothing drawn where there is no geometry, colors, texture mapping."""
    cam = Camera(view["cam"], view["target"], view["roll"])
    scene = view["scene"]
    room = scene.startswith("ROOM")
    H = float(defines["G_ROOM_HALF"])
    G = float(defines["G_GROUND_HALF"])
    n = (1 if scene.startswith("ROOM_BIG") else defines["G_ROOM_N"]) if room else defines["G_GROUND_N"]
    faces = _room_faces(H)
    required = []
    exists = []
    hits = []
    for y in range(SCREEN_H):
        row_r = []
        row_e = []
        row_h = []
        for x in range(SCREEN_W):
            h = _ray_hit(room, cam, x, y, H, G)
            row_h.append(h)
            row_e.append(h is not None)
            row_r.append(h is not None and h[0] >= NEAR)
        required.append(row_r)
        exists.append(row_e)
        hits.append(row_h)
    inner = _erode(required, MARGIN)
    outer = _erode([[not v for v in row] for row in exists], MARGIN)
    # (the room: the pixels MARGIN pixels or less from an edge between 2 faces are not measured for the mapping, as the edges of
    # the regions: the polygons are grown by 1 pixel on their edges and rounded (jo_3d_grow_borders()), the face next to it can
    # be drawn there, its texture is not the one of the face. The faces out of the screen too: an edge just out of it)
    near_edge = set()
    if room:
        faces_px = {}
        for y in range(-MARGIN, SCREEN_H + MARGIN):
            for x in range(-MARGIN, SCREEN_W + MARGIN):
                h = hits[y][x] if 0 <= x < SCREEN_W and 0 <= y < SCREEN_H else _ray_hit(room, cam, x, y, H, G)
                faces_px[(x, y)] = h[1] if h is not None else None
        for y in range(-MARGIN, SCREEN_H + MARGIN):
            for x in range(-MARGIN, SCREEN_W + MARGIN):
                f = faces_px[(x, y)]
                if faces_px.get((x + 1, y), f) != f or faces_px.get((x, y + 1), f) != f:
                    for yy in range(y - MARGIN + 1, y + MARGIN + 1):
                        for xx in range(x - MARGIN + 1, x + MARGIN + 1):
                            near_edge.add((xx, yy))
    holes = []
    spurious = []
    garbage = []
    near_drawn = 0
    flat = scene in ("ROOM_FLAT", "GROUND_FLAT")
    palette = set(flat_colors)
    textured = not flat and not scene.startswith("GROUND_MIP")
    mapping = Mapping(wrap=n > 1)
    covered = 0

    def uv_of(p, face):
        if room:
            o, a, b = faces[face]
            d = _sub(p, o)
            return _dot(d, a) / (2 * H) * n, _dot(d, b) / (2 * H) * n
        return (p[0] + G) / (2 * G) * n, (G - p[2]) / (2 * G) * n

    for y in range(SCREEN_H):
        for x in range(SCREEN_W):
            m = frame.magenta[y][x]
            if inner[y][x]:
                covered += 1
                if m:
                    holes.append((x, y))
                elif textured and _valid_gradient(frame.rgb[y][x]) and (x, y) not in near_edge:
                    h = hits[y][x]
                    su, sv = uv_of(h[2], h[1])
                    ci, cj = math.floor(su), math.floor(sv)
                    # (the neighbors: the same plane, the same cell)
                    h1 = _ray_hit(room, cam, x + 1, y, H, G) or h
                    h2 = _ray_hit(room, cam, x, y + 1, H, G) or h
                    if room and (h1[1] != h[1] or h2[1] != h[1]):
                        h1 = h2 = h
                    u1 = uv_of(h1[2], h[1])
                    u2 = uv_of(h2[2], h[1])
                    mapping.add(x, y, frame.rgb[y][x], (su - ci, sv - cj), (u1[0] - ci, u1[1] - cj), (u2[0] - ci, u2[1] - cj))
            elif outer[y][x] and not m:
                spurious.append((x, y))
            elif not m and not required[y][x]:
                near_drawn += 1
            if not m:
                c = frame.rgb[y][x]
                if not (c in palette if flat else _valid_gradient(c)):
                    garbage.append((x, y))
    chk.measure("%d pixels must be drawn, %d holes, %d drawn where there is no geometry, %d wrong colors, %d drawn nearer "
                "than the near plane (allowed)" % (covered, len(holes), len(spurious), len(garbage), near_drawn))
    chk.worst["holes"] = len(holes)
    if holes:
        chk.problem("hole", "%d hole pixels (background seen) in %s" % (len(holes), _bbox_text(holes)), holes)
    if spurious:
        chk.problem("spurious", "%d pixels drawn where there is no geometry in %s" % (len(spurious), _bbox_text(spurious)),
                    spurious)
    if garbage:
        chk.problem("color", "%d pixels of a color not in the scene in %s (first %s)" %
                    (len(garbage), _bbox_text(garbage), frame.rgb[garbage[0][1]][garbage[0][0]]), garbage)
    if textured:
        # (the subdivided textures: the perspective is expected; the plain big quads: measured)
        mapping.report(chk, scene in ("ROOM_GRID", "ROOM_BIG", "GROUND_TEX"))


def _project_poly(points):
    return [(p[0] * PD / p[2] + CX, p[1] * PD / p[2] + CY) for p in points]


def check_quad(view, frame, quads, flat_colors, chk):
    """One quad: the pixels that must be drawn (the quad clipped by the near plane), the pixels that may be drawn (the quad
    in front of the camera: the engine draws the parts of a subdivided polygon through the near plane whole, see
    jo_3d_emit_near_tile()), the mapping of its texture."""
    cam = Camera(view["cam"], view["target"], view["roll"])
    world = quads[view["quad"]]
    vpts = [cam.view(p) for p in world]
    hull = bool(view["flags"] & FLAGS["HULL"])
    normal = _cross(_sub(vpts[1], vpts[0]), _sub(vpts[3], vpts[0]))
    facing = _dot(normal, vpts[0]) > 0
    if hull:
        allowed = convex_hull(_project_poly(clip_near(vpts)))
        required = []
    elif facing:
        required = _project_poly(clip_near(vpts))
        allowed = _project_poly(clip_z(vpts, ALLOWED_Z))
    else:
        required = allowed = []
    deform = []
    holes = []
    inside = []
    garbage = []
    mode = view["mode"]
    flat = mode == "FLAT"
    flat_color = flat_colors[8]

    def box(poly):
        if not poly:
            return (1, 0, 1, 0)
        return (max(0, int(math.floor(min(p[0] for p in poly))) - 3), min(SCREEN_W - 1, int(math.ceil(max(p[0] for p in poly))) + 3),
                max(0, int(math.floor(min(p[1] for p in poly))) - 3), min(SCREEN_H - 1, int(math.ceil(max(p[1] for p in poly))) + 3))

    ab = box(allowed)
    rb = box(required)
    for y in range(SCREEN_H):
        for x in range(SCREEN_W):
            drawn = not frame.magenta[y][x]
            in_r = rb[0] <= x <= rb[1] and rb[2] <= y <= rb[3]
            sr = signed_distance(required, x + 0.5, y + 0.5) if in_r and required else -1e9
            if drawn:
                c = frame.rgb[y][x]
                if not (c == flat_color if flat else _valid_gradient(c)):
                    garbage.append((x, y))
                sa = signed_distance(allowed, x + 0.5, y + 0.5) if allowed and ab[0] <= x <= ab[1] and ab[2] <= y <= ab[3] else -1e9
                if sa < -MARGIN:
                    deform.append((x, y))
                elif sr > MARGIN:
                    inside.append((x, y))
            elif sr > MARGIN:
                holes.append((x, y))
    chk.measure("expected polygon: %s" % (" ".join("(%.1f, %.1f)" % p for p in required) if required else
                                          "none (back face)" if not hull else "hull " + " ".join("(%.1f, %.1f)" % p for p in allowed)))
    chk.worst["deform"] = len(deform)
    chk.worst["holes"] = len(holes)
    if deform:
        chk.problem("deformation", "%d pixels drawn more than %d pixels out of the quad in %s" %
                    (len(deform), MARGIN, _bbox_text(deform)), deform)
    if holes:
        chk.problem("hole", "%d pixels missing more than %d pixels inside the quad (clipped by the near plane) in %s" %
                    (len(holes), MARGIN, _bbox_text(holes)), holes)
    if garbage:
        chk.problem("color", "%d pixels of a color not in the scene in %s (first %s)" %
                    (len(garbage), _bbox_text(garbage), frame.rgb[garbage[0][1]][garbage[0][0]]), garbage)
    if mode in ("PLAIN", "SUB", "MIP") and not hull and inside:
        check_mapping(view, frame, cam, world, inside, chk)


def check_mapping(view, frame, cam, world, pixels, chk):
    """The texel of each pixel against the perspective correct texel (ray / plane of the quad)."""
    a = _sub(world[1], world[0])
    b = _sub(world[3], world[0])
    n = _cross(a, b)
    aa, bb, ab = _dot(a, a), _dot(b, b), _dot(a, b)
    det = aa * bb - ab * ab
    d0 = _dot(n, _sub(world[0], cam.pos))
    mapping = Mapping(MIP_TOLERANCE if view["mode"] == "MIP" else 0.0)

    def uv(x, y):
        w = cam.ray(x, y)
        dn = _dot(n, w)
        if abs(dn) < 1e-12:
            return None
        t = d0 / dn
        p = _sub((cam.pos[0] + w[0] * t, cam.pos[1] + w[1] * t, cam.pos[2] + w[2] * t), world[0])
        pa, pb = _dot(p, a), _dot(p, b)
        return ((pa * bb - pb * ab) / det, (pb * aa - pa * ab) / det)

    for x, y in pixels:
        c = frame.rgb[y][x]
        if not _valid_gradient(c):
            continue
        e = uv(x, y)
        e1 = uv(x + 1, y)
        e2 = uv(x, y + 1)
        if e is None or e1 is None or e2 is None:
            continue
        mapping.add(x, y, c, e, e1, e2)
    # (only the subdivided textures are expected to be mapped with the perspective; the plain ones: measured)
    mapping.report(chk, view["mode"] in ("SUB", "MIP") or bool(view["flags"] & FLAGS["CONTROL"]))


def _inverse_bilinear(p, q):
    """(u, v) of a point in a quad (bilinear: q0 + u (q1 - q0) on the first edge... the way the VDP1 draws, lines from
    the edge 0-3 to the edge 1-2), Newton iterations."""
    u, v = 0.5, 0.5
    for _ in range(12):
        x = (1 - v) * ((1 - u) * q[0][0] + u * q[1][0]) + v * ((1 - u) * q[3][0] + u * q[2][0]) - p[0]
        y = (1 - v) * ((1 - u) * q[0][1] + u * q[1][1]) + v * ((1 - u) * q[3][1] + u * q[2][1]) - p[1]
        dxu = (1 - v) * (q[1][0] - q[0][0]) + v * (q[2][0] - q[3][0])
        dyu = (1 - v) * (q[1][1] - q[0][1]) + v * (q[2][1] - q[3][1])
        dxv = (1 - u) * (q[3][0] - q[0][0]) + u * (q[2][0] - q[1][0])
        dyv = (1 - u) * (q[3][1] - q[0][1]) + u * (q[2][1] - q[1][1])
        det = dxu * dyv - dxv * dyu
        if abs(det) < 1e-9:
            break
        du = (x * dyv - y * dxv) / det
        dv = (y * dxu - x * dyu) / det
        u -= du
        v -= dv
        if abs(du) < 1e-6 and abs(dv) < 1e-6:
            break
    return u, v


def _is_sliver(q):
    """jo_3d_is_sliver() of 3d.c: extent >= 64 pixels and cross < 2 x 4 x extent."""
    extent = max(abs(q[2][0] - q[0][0]), abs(q[2][1] - q[0][1]), abs(q[3][0] - q[1][0]), abs(q[3][1] - q[1][1]))
    cross = (q[2][0] - q[0][0]) * (q[3][1] - q[1][1]) - (q[3][0] - q[1][0]) * (q[2][1] - q[0][1])
    return extent >= 64 and cross < 2 * 4 * extent


def check_mip(view, frame, defines, chk):
    """The mipmapped ground: each quad in front of the near plane is drawn as one command, mapped bilinearly by the VDP1
    from its corners on the screen: the column (flipped or not) and the line of each pixel are known."""
    cam = Camera(view["cam"], view["target"], view["roll"])
    # (DEFECT: the ROM draws the ground of GROUND_MIP_FLIP not flipped, the check expects it flipped)
    flip = view["scene"] == "GROUND_MIP_FLIP"
    tex = defines["G_TEX"] - 1
    checked = 0
    bad = []
    slivers = 0
    for corners, _ in _scene_geometry(defines, "GROUND"):
        vp = [cam.view(p) for p in corners]
        if min(p[2] for p in vp) <= NEAR * 1.05:
            continue
        q = [project_floor(p) for p in vp]
        cross = (q[2][0] - q[0][0]) * (q[3][1] - q[1][1]) - (q[3][0] - q[1][0]) * (q[2][1] - q[0][1])
        if cross <= 0:
            continue
        # (the quads of a few pixels: the rounding of the VDP1 (the texels of its lines) is as big as them)
        if min(max(abs(q[1][0] - q[0][0]), abs(q[1][1] - q[0][1])), max(abs(q[3][0] - q[0][0]), abs(q[3][1] - q[0][1]))) < MIP_MIN_SIZE:
            continue
        sliver = _is_sliver(q)
        slivers += sliver
        poly = [(p[0] + 0.5, p[1] + 0.5) for p in q]
        x0 = max(0, int(min(p[0] for p in q)))
        x1 = min(SCREEN_W - 1, int(max(p[0] for p in q)))
        y0 = max(0, int(min(p[1] for p in q)))
        y1 = min(SCREEN_H - 1, int(max(p[1] for p in q)))
        if x1 < x0 or y1 < y0:
            continue
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if signed_distance(poly, x + 0.5, y + 0.5) < MIP_INSET:
                    continue
                c = frame.rgb[y][x]
                if frame.magenta[y][x]:
                    continue    # (the holes: check_cover())
                checked += 1
                if sliver and abs(c[0] - 15) <= 1 and abs(c[1] - 15) <= 1:
                    continue
                u, v = _inverse_bilinear((x, y), q)
                col = (1 - u) * tex if flip else u * tex
                row = v * tex
                # (the tolerance: the blur of the copies, + 1 pixel converted into texels at this pixel: the rounding of
                # the VDP1, a quad of 5 lines on the screen has 6 texels per pixel)
                u1, v1 = _inverse_bilinear((x + 1, y), q)
                u2, v2 = _inverse_bilinear((x, y + 1), q)
                tol = MIP_TOLERANCE + tex * max(abs(u1 - u), abs(v1 - v), abs(u2 - u), abs(v2 - v))
                if abs(c[0] - col) > tol or abs(c[1] - row) > tol or not _valid_gradient(c):
                    bad.append((x, y))
    frac = len(bad) / max(1, checked)
    chk.measure("mipmaps: %d pixels of whole quads checked (%d slivers), %d with a wrong texel (%.2f %%)" %
                (checked, slivers, len(bad), 100 * frac))
    chk.worst["mip_bad"] = frac
    if frac > MIP_MAX_BAD:
        chk.problem("mipmap", "%d pixels (%.2f %%) with a texel more than %.1f texels from the expected one (%s) in %s" %
                    (len(bad), 100 * frac, MIP_TOLERANCE, "flipped" if flip else "not flipped", _bbox_text(bad)), bad)


def _describe(view):
    flags = [k for k, v in FLAGS.items() if view["flags"] & v]
    return "scene %s%s, camera (%s) target (%s) roll %d%s" % (
        view["scene"], "" if view["mode"] == "NONE" else " mode " + view["mode"],
        ", ".join("%g" % c for c in view["cam"]), ", ".join("%g" % c for c in view["target"]), view["roll"],
        (", flags " + "|".join(flags)) if flags else "")


def _has_black(raw):
    """A frame of the end of a view or of a marker: black background (see g_draw() in t_glitch.c). The pictures of the
    views have no black pixel (magenta background, the colors of the scenes)."""
    image = media.decode_png(raw)
    sx = image.width // SCREEN_W
    sy = image.height // SCREEN_H
    ox = (image.width - SCREEN_W * sx) // 2
    oy = (image.height - SCREEN_H * sy) // 2
    d = image.data
    for y in range(SCREEN_H):
        base = ((oy + y * sy) * image.width + ox) * 3
        for i in range(base, base + SCREEN_W * sx * 3, sx * 3):
            if d[i] < 8 and d[i + 1] < 8 and d[i + 2] < 8:
                return True
    return False


def _view_frame(reader, start, end):
    """The picture of a view: the last frame before the black frames of its end (see g_draw() in t_glitch.c; a heavy
    view is shown during a few frames only: the frames before are the background alone, the VDP1 still drawing).
    Returns (index, count of the identical frames that end there)."""
    best = (None, 0)
    previous = None
    length = 0
    for i in range(start, min(end, reader.count())):
        raw = reader.raw(i)
        if raw == previous:
            length += 1
        elif _has_black(raw):
            if best[0] is not None and i > best[0] + 1:
                break
            previous = None
            length = 0
            continue
        else:
            length = 1
        previous = raw
        best = (i, length)
    return best


def run_checks(run, variant, result, results, quick):
    """Check the views of the ROM seen in a run (the markers 'g_...'). run: tests/run_tests.py Run (images)."""
    try:
        defines, views, quads, colors = read_views()
    except OSError as e:
        results.add(LAYER, "glitch %s" % variant, "FAIL", "no views: %s" % e)
        return
    markers = sorted((last, name) for name, last, _ in result.collector.markers if name.startswith("g_"))
    others = sorted([last for name, last, _ in result.collector.markers if not name.startswith("g_")] +
                    ([result.collector.end_frame] if result.collector.end_frame is not None else []))
    if not markers:
        if not quick:
            results.add(LAYER, "glitch %s" % variant, "FAIL", "no view of the anti-glitch checks shown by the ROM")
        return
    frames = {}
    summary = {}
    count = 0
    seen = set()
    for k, (last, name) in enumerate(markers):
        view = views.get(name)
        if view is None:
            results.add(LAYER, "glitch %s %s" % (variant, name), "FAIL", "view not in t_glitch_views.h (rebuild the ROM)")
            continue
        seen.add(name)
        # The frames of the view: after its marker, before the next one (the frames of a step are not a fixed count of
        # frames of the video: a heavy view takes 2 or 3 frames per step)
        end = min([m for m, _ in markers[k + 1:k + 2]] + [m for m in others if m > last] + [last + 120,
                                                                                          result.reader.count()])
        index, run_length = _view_frame(result.reader, last + 1, end)
        if index is None:
            results.add(LAYER, "glitch %s %s" % (variant, name), "FAIL", "no frame recorded after the marker")
            continue
        image = result.reader.frame(index)
        frame = Frame(image)
        frames[name] = frame
        chk = Check(view)
        if run_length < 2:
            chk.measure("the picture of the view was shown during 1 frame only (a slow view)")
        if view["scene"] == "QUAD":
            check_quad(view, frame, quads, colors, chk)
        else:
            check_cover(view, frame, defines, colors, chk)
            if view["scene"].startswith("GROUND_MIP"):
                check_mip(view, frame, defines, chk)
        if view["flags"] & FLAGS["SLAVE"]:
            master = frames.get(name[:-len("_slave")])
            if master is None:
                chk.problem("slave", "the view of the master CPU was not checked before", [])
            else:
                diff = [(x, y) for y in range(SCREEN_H) for x in range(SCREEN_W) if master.rgb[y][x] != frame.rgb[y][x]]
                chk.measure("%d pixels different from the master CPU" % len(diff))
                if diff:
                    chk.problem("slave", "%d pixels different from the same view drawn by the master CPU in %s" %
                                (len(diff), _bbox_text(diff)), diff)
        count += 1
        _report(run, variant, view, frame, chk, index, results)
        group = view["scene"] + ("" if view["mode"] in ("NONE",) else " " + view["mode"])
        s = summary.setdefault(group, {"views": 0, "failed": 0, "worst": {}})
        s["views"] += 1
        expected_fail = view["flags"] & (FLAGS["DEFECT"] | FLAGS["CONTROL"])
        if bool(chk.problems) != bool(expected_fail):
            if not expected_fail and _known(view, chk):
                s["known"] = s.get("known", 0) + 1
            else:
                s["failed"] += 1
        if not expected_fail:
            for k, v in chk.worst.items():
                better_low = k not in ("map_good",)
                old = s["worst"].get(k)
                if old is None or (v > old[0] if better_low else v < old[0]):
                    s["worst"][k] = (v, name)
    lines = []
    for group, s in sorted(summary.items()):
        worst = ", ".join("%s %s (%s)" % (k, ("%.3f" % v) if isinstance(v, float) else v, n) for k, (v, n) in
                          sorted(s["worst"].items()))
        lines.append("%s: %d views, %d failed%s%s" % (group, s["views"], s["failed"],
                                                       ", %d known limitations (WARN)" % s["known"] if s.get("known") else "",
                                                       "; worst: " + worst if worst else ""))
    failed = sum(s["failed"] for s in summary.values())
    results.add(LAYER, "glitch %s summary" % variant, "FAIL" if failed else "PASS",
                "%d views checked, %d failed" % (count, failed), details=lines)


# Glitches of the engine known and not fixed (see the report of the tests in tests/README.md): (view name pattern, kinds
# of problems, reason). A view whose problems are all of these kinds is a WARN, not a FAIL. Remove an entry when the
# engine is fixed: the views must then pass.
# (view name patterns, kinds of problems, most pixels of each problem (None: any), reason)
KNOWN = [
    (("g_q5_sub",), ("deformation",), 8,
     "the edges of the polygon are grown by 1 pixel (jo_3d_grow_borders(): no line of background between 2 polygons), "
     "plus the rounding of the lines of the VDP1: at the far corner of the quad, between its edge seen edge on and its "
     "steep top edge, 5 pixels 2.2 to 2.5 pixels out of the quad"),
]


def _known(view, chk):
    """The reason when all the problems of a view are known limitations of the engine, else None."""
    import fnmatch
    for patterns, kinds, most, reason in KNOWN:
        if not any(fnmatch.fnmatch(view["name"], p) for p in patterns):
            continue
        if all(kind in kinds and (most is None or kind == "mapping" or len(pixels) <= most) for kind, _, pixels in chk.problems):
            return reason
    return None


def _report(run, variant, view, frame, chk, index, results):
    name = view["name"]
    expected_fail = view["flags"] & (FLAGS["DEFECT"] | FLAGS["CONTROL"])
    details = [_describe(view), "frame %d" % index] + chk.measures + [m for _, m, _ in chk.problems]
    label = "glitch %s %s" % (variant, name)
    if expected_fail:
        if chk.problems:
            results.add(LAYER, label, "PASS", "known defect detected: %s" % chk.problems[0][1], details=details, quiet=True)
            return
        status, message = "FAIL", "the known defect of this view was NOT detected: the check is broken"
    elif not chk.problems:
        results.add(LAYER, label, "PASS", chk.measures[0] if chk.measures else "", details=details, quiet=True)
        return
    else:
        known = _known(view, chk)
        if known:
            status, message = "WARN", ("known limitation of the engine (%s): " % known) + "; ".join(m for _, m, _ in chk.problems)
            message = message[:400]
        else:
            status, message = "FAIL", "; ".join(m for _, m, _ in chk.problems)[:300]
    rel = "glitch/%s/%s" % (variant, name)
    images = []
    path = run.results.image_path(rel + ".png")
    media.write_png(path, frame.picture())
    images.append(("frame %d" % index, path))
    for kind, _, pixels in chk.problems[:3]:
        if not pixels:
            continue
        crop, box = _crop_zoom(frame, pixels)
        p = run.results.image_path("%s.%s.png" % (rel, kind))
        media.write_png(p, crop)
        images.append(("%s x4: x %d-%d, y %d-%d (marked right)" % (kind, box[0], box[2] - 1, box[1], box[3] - 1), p))
    results.add(LAYER, label, status, message, details=details, images=images)


if __name__ == "__main__":
    if "--write-views" in sys.argv:
        print("%d views written in %s" % (write_views(), VIEWS_HEADER))
    else:
        print(__doc__)
