"""OCR of the text of jo_printf() on a screenshot of Mednafen (the font of jo_engine/vdp2.c, white text over the game).

Usage: python3 tools/ocr_screen.py IMAGE [first_row last_row]: prints the rows of 40 characters (the text layer of 8 x 8
cells), a character not recognized is a space."""
import os
import re
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'jo_engine', 'vdp2.c'), encoding='utf-8').read()
BODY = SRC[SRC.index('__jo_vdp2_printf_font[128 * 8] ='):]
BODY = BODY[BODY.index('{') + 1:BODY.index('};')]
FONT = [int(v, 16) for v in re.findall(r'0x[0-9A-Fa-f]{2}', BODY)]
CHARS = ' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-%:/()'
GLYPHS = {c: [[(FONT[ord(c) * 8 + y] >> (7 - x)) & 1 for x in range(8)] for y in range(8)] for c in CHARS}


def read_screen(path, rows=(0, 28)):
    im = Image.open(path).convert('RGB')
    w, h = im.size
    s = 2 if w > 400 else 1
    px = im.load()

    def bit(x, y):
        return 1 if sum(px[x * s, y * s][:3]) > 600 else 0

    def cell(ox, oy, col, row):
        x0, y0 = ox + col * 8, oy + row * 8
        if x0 + 8 > w // s or y0 + 8 > h // s:
            return ' ', 0
        bits = [[bit(x0 + x, y0 + y) for x in range(8)] for y in range(8)]
        best, score = ' ', -1
        for c, g in GLYPHS.items():
            sc = sum(1 for y in range(8) for x in range(8) if g[y][x] == bits[y][x])
            if sc > score:
                best, score = c, sc
        return best, score

    best = None
    for oy in range(0, 8):
        for ox in range(0, 12):
            total = sum(cell(ox, oy, c, r)[1] for r in range(1, 6) for c in range(0, 12))
            if best is None or total > best[0]:
                best = (total, ox, oy)
    _, ox, oy = best
    out = []
    for r in range(rows[0], rows[1]):
        out.append(''.join(cell(ox, oy, c, r)[0] for c in range(40)))
    return out


if __name__ == '__main__':
    rows = (int(sys.argv[2]), int(sys.argv[3])) if len(sys.argv) > 3 else (0, 28)
    for line in read_screen(sys.argv[1], rows):
        print(line)
