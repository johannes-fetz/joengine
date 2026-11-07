#!/usr/bin/env python3
"""Preview of a guard (models/NAME.obj + .mtl + .json) in a pose, without the console: the painter's sort of the engine
(one depth per polygon, its center), the front faces only (the sign of the cross product of the diagonals on the screen,
as the engine), the mean color of the textures. The back faces seen through a hole of the body are drawn in red: a red
pixel is a hole (a joint open when a limb bends). The poses: the animations of the JSON as the engine plays them (the
same interpolation between 2 keys, the bones turned around their pivots: X, then Y, then Z of the parent).

Usage (from the demo folder):
    python3 tools/guard_view.py models guard out.png [--anim NAME --time SECONDS] [--yaw DEG] [--pitch DEG] [--zoom F]
    python3 tools/guard_view.py models guard strip.png --strip NAME DURATION STEP     a frame every STEP seconds, 2 views
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont


def load_obj(models, name):
    pts = []
    cols = []
    faces = []   # (group, material, [point indexes])
    point_groups = []   # (the group each point is declared in, like jo_model.py: it moves with that bone)
    group = None
    mat = None
    with open(os.path.join(models, name + '.obj')) as f:
        for line in f:
            w = line.split()
            if not w:
                continue
            if w[0] == 'v':
                pts.append([float(x) for x in w[1:4]])
                cols.append([float(x) for x in w[4:7]] if len(w) >= 7 else [0.5, 0.5, 0.5])
                point_groups.append(group)
            elif w[0] == 'g':
                group = w[1]
            elif w[0] == 'usemtl':
                mat = w[1]
            elif w[0] == 'f':
                faces.append((group, mat, [int(x.split('/')[0]) - 1 for x in w[1:]]))
    mats = {}
    cur = None
    with open(os.path.join(models, name + '.mtl')) as f:
        for line in f:
            w = line.split()
            if not w:
                continue
            if w[0] == 'newmtl':
                cur = w[1]
                mats[cur] = (0.6, 0.6, 0.6)
            elif w[0] == 'map_Kd' and cur:
                p = os.path.join(models, w[1])
                if os.path.exists(p):
                    a = np.asarray(Image.open(p).convert('RGBA'), float)
                    m = a[..., 3] > 0
                    if m.any():
                        mats[cur] = tuple(a[m][:, :3].mean(axis=0) / 255)
            elif w[0] == 'Kd' and cur and mats[cur] == (0.6, 0.6, 0.6):
                mats[cur] = tuple(float(x) for x in w[1:4])
    with open(os.path.join(models, name + '.json')) as f:
        rig = json.load(f)
    rig['point_groups'] = point_groups
    return np.array(pts), np.array(cols), faces, mats, rig


def rot(axis, deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    if axis == 0:
        return np.array(((1, 0, 0), (0, c, -s), (0, s, c)))
    if axis == 1:
        return np.array(((c, 0, s), (0, 1, 0), (-s, 0, c)))
    return np.array(((c, -s, 0), (s, c, 0), (0, 0, 1)))


def pose_at(rig, anim, time):
    """The pose (root, {bone: angles}) of an animation at a time (seconds): the interpolation of the engine (linear
    between 2 keys)"""
    if anim is None:
        return np.zeros(3), {}
    if isinstance(anim, dict):
        # (a pose given directly: {bone: [x, y, z]}, 'root': [x, y, z])
        return np.array(anim.get('root', [0, 0, 0]), float), {b['name']: np.array(anim.get(b['name'], [0, 0, 0]), float) for b in rig['bones']}
    a = next(x for x in rig['animations'] if x['name'] == anim)
    keys = a['keys']
    kt = max(1, int(round(a.get('key_time', 0.1) * 60))) / 60.0
    n = len(keys)
    if a.get('loop'):
        time = time % (n * kt)
        k = int(time / kt)
        f = time / kt - k
        nxt = (k + 1) % n
    elif time >= (n - 1) * kt:
        k = nxt = n - 1
        f = 0
    else:
        k = int(time / kt)
        f = time / kt - k
        nxt = k + 1
    def val(key, c):
        return np.array(key.get(c, [0, 0, 0]), float)
    root = val(keys[k], 'root') * (1 - f) + val(keys[nxt], 'root') * f
    angles = {}
    for b in rig['bones']:
        c = b['name']
        a0, a1 = val(keys[k], c), val(keys[nxt], c)
        d = ((a1 - a0 + 180) % 360) - 180
        angles[c] = a0 + d * f
    return root, angles


def bone_matrices(rig, root, angles):
    """The matrix (3x3, offset) of each bone: around its pivot, X then Y then Z of the parent"""
    out = {}
    bones = {b['name']: b for b in rig['bones']}
    def mat(name):
        if name in out:
            return out[name]
        b = bones[name]
        ang = angles.get(name, np.zeros(3))
        r = rot(2, ang[2]) @ rot(1, ang[1]) @ rot(0, ang[0])
        pv = np.array(b['pivot'], float)
        # p' = r (p - pv) + pv
        local = (r, pv - r @ pv)
        if b['parent'] is None:
            pr, po = np.eye(3), root
        else:
            pr, po = mat(b['parent'])
        out[name] = (pr @ local[0], pr @ local[1] + po)
        return out[name]
    for b in rig['bones']:
        mat(b['name'])
    return out


def render(models, name, out, anim=None, time=0.0, yaw=30, pitch=10, size=400, zoom=1.0, label=None, mirror=False,
           hidden=(), focus=None):
    pts, cols, faces, mats, rig = load_obj(models, name)
    root, angles = pose_at(rig, anim, time)
    m = bone_matrices(rig, root, angles)
    world = pts.copy()
    # (the points of a group: moved by its bone)
    groups = {}
    for g, mt, idx in faces:
        for i in idx:
            groups.setdefault(i, g)
    for i in range(len(pts)):
        g = rig['point_groups'][i] or groups.get(i)
        if g in m:
            r, o = m[g]
            world[i] = r @ pts[i] + o
    # Camera: around the center of the model, pitch down
    scale = rig['bones'][0]['pivot'][1] / 46.0 if rig['bones'] else 1.0
    center = (world.min(axis=0) + world.max(axis=0)) / 2 if focus is None else np.array(focus, float) * scale
    cam = rot(0, pitch) @ rot(1, yaw)     # world -> view
    dist = 240 * scale / zoom
    view = (world - center) @ cam.T
    view[:, 2] += dist
    img = Image.new('RGB', (size, size), (230, 230, 240))
    draw = ImageDraw.Draw(img)
    fl = size * 0.95
    def proj(p):
        z = max(p[2], 1.0)
        return (size / 2 + fl * p[0] / z, size / 2 - fl * p[1] / z)
    polys = []
    for g, mt, idx in faces:
        if g in hidden:
            continue
        p = view[idx]
        s = [proj(q) for q in p]
        # (the engine: the sign of the cross product of the diagonals on the screen)
        d1 = (s[2][0] - s[0][0], s[2][1] - s[0][1])
        d2 = (s[3][0] - s[1][0], s[3][1] - s[1][1])
        cross = d1[0] * d2[1] - d1[1] * d2[0]
        front = cross > 0
        if mirror:
            front = not front
        depth = p[:, 2].mean()
        c = np.array(mats.get(mt, (0.6, 0.6, 0.6)))
        shade = cols[idx].mean() * 2.0
        color = tuple(int(min(255, x * 255 * shade)) for x in c) if front else (255, 0, 0)
        polys.append((depth, s, color, front))
    polys.sort(key=lambda t: -t[0])
    for depth, s, color, front in polys:
        draw.polygon(s, fill=color, outline=None if front else (255, 0, 0))
    if label:
        draw.rectangle((0, 0, size, 14), fill=(0, 0, 0))
        draw.text((3, 1), label, fill=(255, 255, 255))
    if out:
        img.save(out)
    return img


def sheet(models, name, out, items, size=220, zoom=1.0, cols=6, mirror=False):
    """items: (anim, time, yaw, pitch, label[, zoom, focus])"""
    imgs = [render(models, name, None, it[0], it[1], it[2], it[3], size, it[5] if len(it) > 5 else zoom, label=it[4], mirror=mirror,
                   focus=it[6] if len(it) > 6 else None) for it in items]
    rows = (len(imgs) + cols - 1) // cols
    img = Image.new('RGB', (cols * size, rows * size), (0, 0, 0))
    for k, im in enumerate(imgs):
        img.paste(im, ((k % cols) * size, (k // cols) * size))
    img.save(out)


def strip(models, name, anim, duration, step, out, yaws=(70, -50), pitch=12, size=230, zoom=2.6):
    """The frames of an animation (every step seconds) from 2 sides, the camera on the hips of the rest pose"""
    n = int(round(duration / step)) + 1
    img = Image.new('RGB', (n * size, len(yaws) * size), (0, 0, 0))
    for r, yaw in enumerate(yaws):
        for k in range(n):
            t = k * step
            im = render(models, name, None, anim, t, yaw, pitch, size, zoom, label='%s %.2fs' % (anim, t), focus=(0, 40, 0))
            img.paste(im, (k * size, r * size))
    img.save(out)


if __name__ == '__main__':
    args = sys.argv[1:]
    models, name, out = args[0], args[1], args[2]
    if '--strip' in args:
        k = args.index('--strip')
        strip(models, name, args[k + 1], float(args[k + 2]), float(args[k + 3]), out)
        sys.exit(0)
    opts = {}
    k = 3
    while k < len(args):
        opts[args[k].lstrip('-')] = args[k + 1] if k + 1 < len(args) and not args[k + 1].startswith('--') else '1'
        k += 2 if k + 1 < len(args) and not args[k + 1].startswith('--') else 1
    render(models, name, out, opts.get('anim'), float(opts.get('time', 0)), float(opts.get('yaw', 30)),
           float(opts.get('pitch', 10)), int(opts.get('size', 400)), float(opts.get('zoom', 1)))
