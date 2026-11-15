"""Pictures in a palette of 256 colors for the textures of the section TX08 (see Level.add_textures8()): one palette for
several pictures (the median cut and k-means: the colors where the pictures have them), each picture in it (the nearest
color, a light error diffusion: a natural grain, no bands), its smaller copies (the means of 2 x 2 pixels, then in the
palette). Needs numpy and Pillow (the rest of jo_level does not).

Example:
    from jo_level import quantize
    palette = quantize.build_palette([sand, palm])                      # RGBA pictures (numpy, alpha 0 or 255)
    textures = [dict(width=64, height=64, **quantize.encode(sand, palette)), ...]
    lv.add_textures8(textures, [saturn_color(*c, msb=False) for c in palette])
"""

import numpy as np
from PIL import Image


def build_palette(images, count=255, reserved=()):
    """count colors (reserved first: fixed colors, the plain colors of a game) for the opaque pixels of RGBA pictures:
    a list of (r, g, b), the pixel value n is the color n - 1 of the list"""
    reserved = [tuple(int(v) for v in c) for c in reserved]
    learn = count - len(reserved)
    pix = np.concatenate([im[im[:, :, 3] > 0][:, :3] for im in images if (im[:, :, 3] > 0).any()]).astype(np.uint8)
    q = Image.fromarray(pix.reshape(1, -1, 3), 'RGB')
    pal = q.quantize(colors=learn, method=Image.Quantize.MEDIANCUT, kmeans=4, dither=Image.Dither.NONE).getpalette()
    learned = [tuple(pal[3 * k:3 * k + 3]) for k in range(learn)]
    return reserved + learned


def index_image(img, palette, diffusion=0.6):
    """A picture (RGBA, alpha 0 or 255) in the palette: rows of pixel values (1 to len(palette), 0: transparent), the
    nearest color, the error diffused (Floyd and Steinberg x diffusion; 0: none)"""
    pal = np.array(palette, float)
    h, w = img.shape[:2]
    out = np.zeros((h, w), np.uint8)
    err = np.zeros((h + 1, w + 2, 3))
    for y in range(h):
        for x in range(w):
            if img[y, x, 3] == 0:
                continue
            c = img[y, x, :3] + err[y, x + 1] * diffusion
            k = int(np.argmin(((pal - c) ** 2).sum(axis=1)))
            out[y, x] = k + 1
            e = c - pal[k]
            err[y, x + 2] += e * 7 / 16
            err[y + 1, x] += e * 3 / 16
            err[y + 1, x + 1] += e * 5 / 16
            err[y + 1, x + 2] += e * 1 / 16
    return out


def halve(img):
    """A picture (RGBA) half as wide and as high: each pixel the mean of the opaque pixels of 2 x 2, opaque if 2 of them
    are"""
    h, w = img.shape[0] // 2, img.shape[1] // 2
    q = img[:h * 2, :w * 2].reshape(h, 2, w, 2, 4)
    a = (q[..., 3] > 0).astype(float)
    count = a.sum(axis=(1, 3))
    out = np.zeros((h, w, 4))
    out[:, :, :3] = (q[..., :3] * a[..., None]).sum(axis=(1, 3)) / np.maximum(count, 1)[..., None]
    out[:, :, 3] = np.where(count >= 2, 255, 0)
    return out


def encode(img, palette, copies=2, diffusion=0.6):
    """A texture of the section TX08 from a picture (RGBA, alpha 0 or 255): dict(pixels=..., copies=[...]) (its smaller
    copies while their width is a multiple of 8, copies at most)"""
    levels = []
    small = img
    while len(levels) < copies and (img.shape[1] >> (len(levels) + 1)) % 8 == 0 and (img.shape[0] >> (len(levels) + 1)) >= 1:
        small = halve(small)
        levels.append(index_image(small, palette, 0.0))
    return dict(pixels=index_image(img, palette, diffusion), copies=levels)
