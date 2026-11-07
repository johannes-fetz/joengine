"""The barcode drawn by the test ROM (tests/rom/jt.c) and the records of its reports."""

import struct

BLOCK = 4
COLUMNS = 80
LINES = 28
FRAME_BYTES = COLUMNS * LINES // 8
HEADER_BYTES = 10
MAGIC = b"JOT1"

# Size of the screen in pixels of the NBG1 bitmap (the 640 and 704 modes double them): the barcode is in the first lines
_LOGICAL_SIZES = [(320, 224), (352, 240), (320, 240), (352, 224), (640, 448), (704, 480), (640, 480), (704, 448)]


def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else (crc << 1)
            crc &= 0xFFFF
    return crc


def _geometries(width, height):
    for lw, lh in _LOGICAL_SIZES:
        sx = width // lw
        sy = height // lh
        if sx < 1 or sy < 1:
            continue
        yield sx, sy, (width - lw * sx) // 2, (height - lh * sy) // 2


def _read_bits(image, sx, sy, ox, oy, count):
    out = bytearray((count + 7) // 8)
    w = image.width
    data = image.data
    for bit in range(count):
        bx = bit % COLUMNS
        by = bit // COLUMNS
        x = ox + (bx * BLOCK + BLOCK // 2) * sx
        y = oy + (by * BLOCK + BLOCK // 2) * sy
        i = (y * w + x) * 3
        if data[i] + data[i + 1] + data[i + 2] > 384:
            out[bit >> 3] |= 0x80 >> (bit & 7)
    return bytes(out)


def lines_needed(height):
    """Lines of a frame to decode to read the barcode (see media.decode_png(max_lines))."""
    return min(height, 16 + LINES * BLOCK * 2 + 2)


def decode(image, full_height=None):
    """The barcode of a frame: a dict (kind, stage, page, pages, payload) or None."""
    height = full_height or image.height
    for sx, sy, ox, oy in _geometries(image.width, height):
        if oy + LINES * BLOCK * sy > image.height:
            continue
        if _read_bits(image, sx, sy, ox, oy, 32) != MAGIC:
            continue
        frame = _read_bits(image, sx, sy, ox, oy, FRAME_BYTES * 8)
        length = frame[8] | (frame[9] << 8)
        if HEADER_BYTES + length + 2 > FRAME_BYTES:
            return None
        crc = (frame[HEADER_BYTES + length] << 8) | frame[HEADER_BYTES + length + 1]
        if crc != crc16(frame[:HEADER_BYTES + length]):
            return None
        return {"kind": chr(frame[4]), "stage": frame[5], "page": frame[6], "pages": frame[7],
                "payload": frame[HEADER_BYTES:HEADER_BYTES + length], "geometry": (sx, sy, ox, oy)}
    return None


def marker_name(code):
    payload = code["payload"]
    n = payload[0]
    return payload[1:1 + n].decode("latin-1"), payload[1 + n]


def parse_report(data):
    """The records of a report (see tests/rom/jt.c). Returns a dict: checks, failures, failed (list), values, metrics,
    blobs, infos, groups."""
    out = {"checks": 0, "failures": 0, "failed": [], "values": {}, "metrics": {}, "blobs": {}, "infos": [], "groups": []}
    group = ""
    pos = 0

    def string(p):
        n = data[p]
        return data[p + 1:p + 1 + n].decode("latin-1"), p + 1 + n

    while pos < len(data):
        rtype = chr(data[pos])
        if rtype == "G":
            group, pos = string(pos + 1)
            out["groups"].append(group)
        elif rtype == "F":
            line, actual, expected = struct.unpack("<Hii", data[pos + 1:pos + 11])
            text, pos = string(pos + 11)
            out["failed"].append({"group": group, "line": line, "actual": actual, "expected": expected, "text": text})
        elif rtype in "VM":
            (value,) = struct.unpack("<I", data[pos + 1:pos + 5])
            name, pos = string(pos + 5)
            out["values" if rtype == "V" else "metrics"][name] = value
        elif rtype == "B":
            (size,) = struct.unpack("<H", data[pos + 1:pos + 3])
            blob = data[pos + 3:pos + 3 + size]
            name, pos = string(pos + 3 + size)
            out["blobs"][name] = blob.hex()
        elif rtype == "I":
            text, pos = string(pos + 1)
            out["infos"].append(text)
        elif rtype == "S":
            out["checks"], out["failures"] = struct.unpack("<II", data[pos + 1:pos + 9])
            pos += 9
        else:
            out["infos"].append("bad record at %d" % pos)
            break
    return out


class Collector:
    """Gathers the pages of the reports and the markers of the scenes, frame after frame."""

    def __init__(self):
        self.pages = {}         # stage -> {page: payload}
        self.page_counts = {}
        self.markers = []       # (name, last frame of the marker, capture offset)
        self.end_frame = None
        self.geometry = None

    def add(self, index, code):
        self.geometry = code["geometry"]
        kind = code["kind"]
        if kind == "R":
            self.pages.setdefault(code["stage"], {})[code["page"]] = code["payload"]
            self.page_counts[code["stage"]] = code["pages"]
        elif kind == "K":
            name, offset = marker_name(code)
            if self.markers and self.markers[-1][0] == name and self.markers[-1][1] >= index - 8:
                self.markers[-1] = (name, index, offset)
            else:
                self.markers.append((name, index, offset))
        elif kind == "Z" and self.end_frame is None:
            self.end_frame = index

    def stage_complete(self, stage):
        return stage in self.pages and len(self.pages[stage]) == self.page_counts[stage]

    def report(self, stage):
        if not self.stage_complete(stage):
            return None
        pages = self.pages[stage]
        return parse_report(b"".join(pages[i] for i in range(self.page_counts[stage])))
