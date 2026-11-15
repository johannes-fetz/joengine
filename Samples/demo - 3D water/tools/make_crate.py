#!/usr/bin/env python3
"""The textures of the crates of the water sample: a wooden crate seen from a side (CRATE.TGA) and from the top
(CRATETOP.TGA, lit by the sun), 64 x 64, 24 bits (the engine makes them 15 bits RGB).

Painted 4 times bigger then made smaller (each pixel: the mean of 4 x 4): weathered pine planks (the grain stretched along
each plank, its knots, a darker gap between the planks), a frame of 4 boards around them and a diagonal brace, rusty
nails, dirt in the corners and the light of the day (a soft gradient, the frame casting a light shadow on the planks).

Usage: python3 make_crate.py (needs numpy, PIL)
"""

import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SIZE = 64
SS = 4
N = SIZE * SS
RNG = np.random.default_rng(11)


def noise(shape, scale_y, scale_x, octaves=4):
    """Smooth value noise (0 to 1), stretched by the scales (pixels of a cell)"""
    out = np.zeros(shape)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        sy, sx = max(1, int(scale_y / 2 ** o)), max(1, int(scale_x / 2 ** o))
        grid = RNG.random((shape[0] // sy + 2, shape[1] // sx + 2))
        img = Image.fromarray((grid * 255).astype(np.uint8)).resize((grid.shape[1] * sx, grid.shape[0] * sy), Image.BICUBIC)
        out += amp * np.asarray(img, float)[:shape[0], :shape[1]] / 255.0
        total += amp
        amp *= 0.5
    return out / total


def wood(shape, along_x, tone):
    """Pine: its grain along the plank (rings of a noise stretched 12 times), a few knots, its tone (r, g, b)"""
    h, w = shape
    n = noise((h, w), 40, 480) if along_x else noise((h, w), 480, 40)
    rings = 0.5 + 0.5 * np.sin(n * 60.0)
    fine = noise((h, w), 3, 60, 2) if along_x else noise((h, w), 60, 3, 2)
    light = 0.78 + 0.16 * rings + 0.10 * (fine - 0.5) + 0.12 * (noise((h, w), 120, 120, 2) - 0.5)
    img = np.array(tone, float)[None, None, :] * light[..., None]
    # (knots: dark rings around a point)
    ys, xs = np.mgrid[0:h, 0:w]
    for _ in range(RNG.integers(0, 2) + (1 if h * w > 30000 else 0)):
        cy, cx = RNG.integers(0, h), RNG.integers(0, w)
        r = np.hypot((ys - cy) * (1.0 if along_x else 3.0), (xs - cx) * (3.0 if along_x else 1.0))
        k = np.exp(-r / 7.0)
        img *= (1 - 0.45 * k)[..., None]
    return img


def side():
    """The side of the crate: 4 planks across, a frame of 4 boards, a diagonal brace, nails"""
    img = np.zeros((N, N, 3))
    plank = N // 4
    for k in range(4):
        tone = np.array((178, 128, 78)) * (0.92 + 0.12 * RNG.random())
        img[k * plank:(k + 1) * plank] = wood((plank, N), True, tone)
        # (the gap between the planks, darker; the edge of the plank lighter)
        img[k * plank:k * plank + 3] *= 0.35
        img[k * plank + 3:k * plank + 6] *= 1.08
    frame = 9 * SS
    ys, xs = np.mgrid[0:N, 0:N]
    # The shadow of the frame and of the brace on the planks (the light from the top left)
    inside = (xs >= frame) & (xs < N - frame) & (ys >= frame) & (ys < N - frame)
    shade = np.clip(1 - 0.25 * np.exp(-(ys - frame) / 10.0) - 0.18 * np.exp(-(xs - frame) / 10.0), 0.6, 1)
    img[inside] *= shade[inside][..., None]
    # The diagonal brace (top left to bottom right) and its shadow
    d = (xs - ys) / np.sqrt(2)
    brace = (np.abs(d) < 5.5 * SS) & inside
    img[(d > 5.5 * SS) & (d < 8.0 * SS) & inside] *= 0.72
    img[brace] = wood((N, N), True, np.array((168, 120, 72)))[brace]
    img[brace & (np.abs(np.abs(d) - 5.5 * SS) < 3)] *= 0.55
    # The frame: 4 boards (top, bottom across; left, right along), their edges
    tone = np.array((160, 112, 64))
    img[:frame] = wood((frame, N), True, tone)
    img[N - frame:] = wood((frame, N), True, tone)
    img[frame:N - frame, :frame] = wood((N - 2 * frame, frame), False, tone)
    img[frame:N - frame, N - frame:] = wood((N - 2 * frame, frame), False, tone)
    edge = ((np.abs(ys - frame) < 3) | (np.abs(ys - (N - frame)) < 3)) | \
        (((np.abs(xs - frame) < 3) | (np.abs(xs - (N - frame)) < 3)) & (ys > frame) & (ys < N - frame))
    img[edge] *= 0.5
    img[(xs < 4) | (ys < 4)] *= 1.15
    img[(xs > N - 5) | (ys > N - 5)] *= 0.6
    # Nails: rusty heads in the corners of the frame
    for cy in (frame // 2, N - frame // 2):
        for cx in (frame // 2, N // 2, N - frame // 2):
            r = np.hypot(ys - cy, xs - cx)
            img[r < 6] = np.array((92, 70, 58)) * (0.8 + 0.4 * RNG.random())
            img[(r < 3) & (ys < cy)] *= 1.4
            img[(r >= 6) & (r < 10) & (ys > cy)] *= 0.8
    # Weathering: grey bleached wood, dirt in the corners, a soft light from the top left
    grey = noise((N, N), 90, 90, 3)
    img = img * (1 - 0.25 * grey[..., None]) + np.array((150, 146, 138)) * 0.25 * grey[..., None]
    corner = np.minimum(np.minimum(xs, N - 1 - xs), np.minimum(ys, N - 1 - ys))
    img *= (1 - 0.18 * np.exp(-corner / 30.0))[..., None]
    img *= (1.06 - 0.12 * (xs + ys) / (2 * N))[..., None]
    return img


def small(img):
    """4 x 4 pixels into one, 8 bits"""
    out = img.reshape(SIZE, SS, SIZE, SS, 3).mean(axis=(1, 3))
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), 'RGB')


def main():
    s = side()
    small(s).save(os.path.join(HERE, '..', 'cd', 'CRATE.TGA'))
    # The top: the same crate in the sun (lighter, warmer)
    top = np.clip(s * np.array((1.22, 1.18, 1.10)), 0, 255)
    small(top).save(os.path.join(HERE, '..', 'cd', 'CRATETOP.TGA'))
    preview = Image.new('RGB', (SIZE * 2 + 4, SIZE), (0, 0, 0))
    preview.paste(small(s), (0, 0))
    preview.paste(small(top), (SIZE + 4, 0))
    preview.resize((preview.width * 4, preview.height * 4), Image.NEAREST).save(os.path.join(HERE, 'crate_preview.png'))
    print('CRATE.TGA, CRATETOP.TGA: %d x %d' % (SIZE, SIZE))


if __name__ == '__main__':
    main()
