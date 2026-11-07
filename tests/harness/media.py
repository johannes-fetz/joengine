"""Pictures and sounds without dependencies: PNG read/write, the frames of a Mednafen QuickTime recording (PNG codec),
WAV files, image comparison."""

import os
import struct
import zlib

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


class Image:
    """RGB picture: `data` is bytes, 3 per pixel, line after line."""

    def __init__(self, width, height, data):
        self.width = width
        self.height = height
        self.data = data

    def pixel(self, x, y):
        i = (y * self.width + x) * 3
        return self.data[i], self.data[i + 1], self.data[i + 2]

    def crop(self, x, y, w, h):
        rows = []
        for yy in range(y, y + h):
            i = (yy * self.width + x) * 3
            rows.append(self.data[i:i + w * 3])
        return Image(w, h, b"".join(rows))


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def _unfilter(raw, width, height, bpp):
    stride = width * bpp
    out = bytearray(stride * height)
    prev = bytearray(stride)
    pos = 0
    for y in range(height):
        ftype = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        if ftype == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                upleft = prev[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + _paeth(left, prev[i], upleft)) & 0xFF
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return bytes(out)


def decode_png(png, max_lines=None):
    """Decode a PNG (8 bits RGB or RGBA). max_lines: only the first lines (faster)."""
    if png[:8] != PNG_SIGNATURE:
        raise ValueError("not a PNG")
    pos = 8
    idat = []
    width = height = 0
    color_type = 2
    while pos < len(png):
        length, ctype = struct.unpack(">I4s", png[pos:pos + 8])
        body = png[pos + 8:pos + 8 + length]
        if ctype == b"IHDR":
            width, height, depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if depth != 8 or color_type not in (2, 6) or interlace:
                raise ValueError("unsupported PNG format")
        elif ctype == b"IDAT":
            idat.append(body)
        elif ctype == b"IEND":
            break
        pos += 12 + length
    bpp = 3 if color_type == 2 else 4
    lines = height if max_lines is None else min(height, max_lines)
    stride = width * bpp + 1
    decomp = zlib.decompressobj()
    raw = decomp.decompress(b"".join(idat), stride * lines)
    if any(raw[y * stride] != 0 for y in range(lines)):
        data = _unfilter(raw, width, lines, bpp)
    else:
        data = b"".join(raw[y * stride + 1:(y + 1) * stride] for y in range(lines))
    if bpp == 4:
        data = bytes(b for i, b in enumerate(data) if i % 4 != 3)
    return Image(width, lines, data)


def encode_png(image):
    """Encode an RGB picture (the "up" filter: smaller files than no filter)."""
    stride = image.width * 3
    rows = []
    prev = bytes(stride)
    for y in range(image.height):
        line = image.data[y * stride:(y + 1) * stride]
        rows.append(b"\x02" + bytes((a - b) & 0xFF for a, b in zip(line, prev)))
        prev = line

    def chunk(ctype, body):
        return struct.pack(">I", len(body)) + ctype + body + struct.pack(">I", zlib.crc32(ctype + body) & 0xFFFFFFFF)

    return (PNG_SIGNATURE + chunk(b"IHDR", struct.pack(">IIBBBBB", image.width, image.height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b""))


def read_png(path):
    with open(path, "rb") as f:
        return decode_png(f.read())


def write_png(path, image):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "wb") as f:
        f.write(encode_png(image))


class FrameReader:
    """The frames of a recording being written by Mednafen (-qtrecord, PNG codec): each frame is a complete PNG file in
    the data of the movie, found by its signature (the index of the movie is written at the end only)."""

    def __init__(self, path):
        self.path = path
        self.offsets = []      # (start, end) of each frame
        self._scan_pos = 0

    def update(self):
        """Find the new complete frames. Returns the count of frames."""
        if not os.path.exists(self.path):
            return 0
        with open(self.path, "rb") as f:
            f.seek(self._scan_pos)
            data = f.read()
        base = self._scan_pos
        pos = 0
        while True:
            start = data.find(PNG_SIGNATURE, pos)
            if start < 0:
                break
            end = data.find(b"IEND", start)
            if end < 0 or end + 8 > len(data):
                break
            end += 8
            self.offsets.append((base + start, base + end))
            pos = end
        self._scan_pos = base + pos
        return len(self.offsets)

    def count(self):
        return len(self.offsets)

    def raw(self, index):
        start, end = self.offsets[index]
        with open(self.path, "rb") as f:
            f.seek(start)
            return f.read(end - start)

    def frame(self, index, max_lines=None):
        return decode_png(self.raw(index), max_lines)


def compare_images(a, b, threshold=24):
    """Pixels whose difference (the biggest of the 3 components) is above threshold. Returns (count, diff image)."""
    if (a.width, a.height) != (b.width, b.height):
        return a.width * a.height, None
    da, db = a.data, b.data
    if da == db:
        return 0, None
    out = bytearray(len(da))
    count = 0
    for i in range(0, len(da), 3):
        d = max(abs(da[i] - db[i]), abs(da[i + 1] - db[i + 1]), abs(da[i + 2] - db[i + 2]))
        if d > threshold:
            count += 1
            out[i] = 255
            out[i + 1] = 0
            out[i + 2] = 0
        else:
            # The reference, darker
            g = (da[i] + da[i + 1] + da[i + 2]) // 9
            out[i] = out[i + 1] = out[i + 2] = g
    return count, Image(a.width, a.height, bytes(out))


def image_stats(image):
    """(count of different colors among some samples, average brightness)."""
    colors = set()
    total = 0
    n = 0
    step = max(1, (image.width * image.height) // 4096)
    for p in range(0, image.width * image.height, step):
        i = p * 3
        c = image.data[i:i + 3]
        colors.add(c)
        total += c[0] + c[1] + c[2]
        n += 1
    return len(colors), total / (3 * max(1, n))


def read_wav(path):
    """16 bits PCM WAV (also a file not closed by Mednafen: the sizes of its header are not trusted).
    Returns (rate, channels, samples of the first channel)."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("not a WAV file")
    pos = 12
    rate = channels = 0
    while pos + 8 <= len(data):
        ctype, length = struct.unpack("<4sI", data[pos:pos + 8])
        if ctype == b"fmt ":
            _, channels, rate = struct.unpack("<HHI", data[pos + 8:pos + 16])
        elif ctype == b"data":
            body = data[pos + 8:]
            if length < len(body) and length != 0:
                body = body[:length]
            n = len(body) // (2 * channels)
            samples = struct.unpack("<%dh" % (n * channels), body[:n * 2 * channels])
            return rate, channels, samples[::channels]
        pos += 8 + length + (length & 1)
    raise ValueError("no audio data")


def audio_envelope(samples, rate, start=0, end=None, block_ms=100):
    """Loudness per block of block_ms (dB, -90 = silence) from the sample start to the sample end."""
    import math
    block = max(1, rate * block_ms // 1000)
    end = len(samples) if end is None else min(end, len(samples))
    out = []
    for p in range(start, end - block + 1, block):
        chunk = samples[p:p + block]
        power = sum(s * s for s in chunk[::4]) / max(1, len(chunk[::4]))
        out.append(round(10 * math.log10(power / (32768.0 * 32768.0)) if power > 0 else -90.0, 1))
    return [max(-90.0, v) for v in out]


def compare_envelopes(golden, current, tolerance_db, floor_db=-50.0):
    """Blocks whose loudness differs by more than tolerance_db (the blocks quieter than floor_db in both are ignored).
    Returns (bad blocks, compared blocks)."""
    n = min(len(golden), len(current))
    bad = compared = 0
    for a, b in zip(golden[:n], current[:n]):
        if a < floor_db and b < floor_db:
            continue
        compared += 1
        if abs(max(a, floor_db) - max(b, floor_db)) > tolerance_db:
            bad += 1
    return bad + abs(len(golden) - len(current)), compared
