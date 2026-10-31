"""VDP1 commands of a frame in a Mednafen save state (tools/savestate.sh): the cost of each texture

Usage: python3 tools/vdp1_commands.py state.mcs [top] [overlay.png]
  Lists the groups of commands (function, texture address and size) from the costliest: the pixels the VDP1 draws
  (about max(lines, texture height) x max(pixels per line, texture width) per command: a texture is read entirely even
  drawn smaller, see JO_PROFILE_3D_PIXELS), the pixels on the screen, the command count.
  With overlay.png: the commands of the top groups drawn over the preview of the state (one image per group, in a grid)
"""
import gzip, struct, sys, collections

def state_vdp1_vram(path):
    """The VDP1 VRAM of the state (512 KB, 16 bits words in little endian) and the preview image (RGB bytes, width, height)"""
    d = gzip.open(path).read()
    w, h = struct.unpack('<II', d[24:32])
    preview = (d[32:32 + w * h * 3], w, h)
    pos = 32 + w * h * 3
    while pos < len(d):
        name = d[pos:pos + 32].split(b'\0')[0]
        size = struct.unpack('<I', d[pos + 32:pos + 36])[0]
        if name == b'VDP1':
            p, e = pos + 36, pos + 36 + size
            while p < e:
                nl = d[p]
                n = d[p + 1:p + 1 + nl]
                s = struct.unpack('<I', d[p + 1 + nl:p + 5 + nl])[0]
                if n == b'VRAM':
                    return d[p + 5 + nl:p + 5 + nl + s], preview
                p += 5 + nl + s
        pos += 36 + size
    raise ValueError('no VDP1 VRAM in the state')

def commands(vram):
    """The commands of the table, from the address 0 (the jumps are followed, the skipped commands are left out)"""
    out, a, seen = [], 0, set()
    while a < len(vram) and a not in seen:
        seen.add(a)
        w = struct.unpack('<16H', vram[a:a + 32])
        ctrl = w[0]
        if ctrl & 0x8000:
            break
        if not (ctrl & 0x4000):
            out.append(dict(addr=a, ctrl=ctrl, pmod=w[2], colr=w[3], srca=w[4] * 8, size=w[5],
                            xy=[struct.unpack('<h', struct.pack('<H', w[6 + i]))[0] for i in range(8)]))
        a = w[1] * 8 if ((ctrl >> 12) & 3) == 1 else a + 32
    return out

def cost(c):
    """(pixels drawn, pixels on the screen, texture width, texture height) of a command"""
    f = c['ctrl'] & 0xF
    tw, th = (((c['size'] >> 8) & 0x3F) * 8, c['size'] & 0xFF) if f < 4 else (0, 0)
    x = c['xy']
    if f == 0:
        lines, width = th, tw
    else:
        ch = lambda i, j: max(abs(x[2 * i] - x[2 * j]), abs(x[2 * i + 1] - x[2 * j + 1]))
        lines, width = max(ch(0, 3), ch(1, 2)), max(ch(0, 1), ch(3, 2))
    return (max(lines, th) + 1) * (max(width, tw) + 1), (lines + 1) * (width + 1), tw, th

def main():
    vram, preview = state_vdp1_vram(sys.argv[1])
    top = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    cs = [c for c in commands(vram) if (c['ctrl'] & 0xF) < 8]
    groups = collections.defaultdict(lambda: [0, 0, 0, []])
    total = [0, 0]
    for c in cs:
        drawn, screen, tw, th = cost(c)
        key = (c['ctrl'] & 0xF, c['srca'] if (c['ctrl'] & 0xF) < 4 else 0, tw, th)
        g = groups[key]
        g[0] += drawn
        g[1] += screen
        g[2] += 1
        g[3].append(c)
        total[0] += drawn
        total[1] += screen
    # (the distorted sprites: about 28 pixels per microsecond measured, the normal sprites are faster)
    print('%d commands: %d pixels drawn, %d on the screen' % (len(cs), total[0], total[1]))
    print('func  texture    size   count   drawn  screen')
    ranked = sorted(groups.items(), key=lambda kv: -kv[1][0])[:top]
    for (f, srca, tw, th), (drawn, screen, n, _) in ranked:
        print('%4d  %06x  %3dx%3d  %5d  %7d  %7d' % (f, srca, tw, th, n, drawn, screen))
    if len(sys.argv) > 3:
        from PIL import Image, ImageDraw
        data, w, h = preview
        base = Image.frombytes('RGB', (w, h), data).resize((320, 224))
        cols = 4
        rows = (min(len(ranked), 12) + cols - 1) // cols
        out = Image.new('RGB', (cols * 320, rows * 224))
        for k, ((f, srca, tw, th), (drawn, screen, n, items)) in enumerate(ranked[:12]):
            im = base.copy()
            d = ImageDraw.Draw(im, 'RGBA')
            for c in items:
                x = c['xy']
                if (c['ctrl'] & 0xF) == 0:
                    pts = [(x[0], x[1]), (x[0] + tw, x[1]), (x[0] + tw, x[1] + th), (x[0], x[1] + th)]
                else:
                    pts = [(x[0], x[1]), (x[2], x[3]), (x[4], x[5]), (x[6], x[7])]
                d.polygon(pts, fill=(255, 0, 0, 70), outline=(255, 255, 0, 255))
            d.text((2, 2), 'func %d %06x %dx%d n=%d drawn=%d' % (f, srca, tw, th, n, drawn), fill=(255, 255, 255))
            out.paste(im, ((k % cols) * 320, (k // cols) * 224))
        out.save(sys.argv[3])

if __name__ == '__main__':
    main()
