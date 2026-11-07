"""Results of a run: the console, tests/_work/report.html and tests/_work/results.json."""

import html
import json
import os
import shutil

PASS = "PASS"
FAIL = "FAIL"
NEW = "NEW"         # no reference yet: --approve records it
WARN = "WARN"
SKIP = "SKIP"

_COLORS = {PASS: "#2e7d32", FAIL: "#c62828", NEW: "#1565c0", WARN: "#ef6c00", SKIP: "#757575"}

VDP2_REGISTERS = dict(enumerate([
    "TVMD", "EXTEN", "TVSTAT", "VRSIZE", "HCNT", "VCNT", "-", "RAMCTL", "CYCA0L", "CYCA0U", "CYCA1L", "CYCA1U", "CYCB0L",
    "CYCB0U", "CYCB1L", "CYCB1U", "BGON", "MZCTL", "SFSEL", "SFCODE", "CHCTLA", "CHCTLB", "BMPNA", "BMPNB", "PNCN0",
    "PNCN1", "PNCN2", "PNCN3", "PNCR", "PLSZ", "MPOFN", "MPOFR", "MPABN0", "MPCDN0", "MPABN1", "MPCDN1", "MPABN2",
    "MPCDN2", "MPABN3", "MPCDN3", "MPABRA", "MPCDRA", "MPEFRA", "MPGHRA", "MPIJRA", "MPKLRA", "MPMNRA", "MPOPRA",
    "MPABRB", "MPCDRB", "MPEFRB", "MPGHRB", "MPIJRB", "MPKLRB", "MPMNRB", "MPOPRB", "SCXIN0", "SCXDN0", "SCYIN0",
    "SCYDN0", "ZMXIN0", "ZMXDN0", "ZMYIN0", "ZMYDN0", "SCXIN1", "SCXDN1", "SCYIN1", "SCYDN1", "ZMXIN1", "ZMXDN1",
    "ZMYIN1", "ZMYDN1", "SCXN2", "SCYN2", "SCXN3", "SCYN3", "ZMCTL", "SCRCTL", "VCSTAU", "VCSTAL", "LSTA0U", "LSTA0L",
    "LSTA1U", "LSTA1L", "LCTAU", "LCTAL", "BKTAU", "BKTAL", "RPMD", "RPRCTL", "KTCTL", "KTAOF", "OVPNRA", "OVPNRB",
    "RPTAU", "RPTAL", "WPSX0", "WPSY0", "WPEX0", "WPEY0", "WPSX1", "WPSY1", "WPEX1", "WPEY1", "WCTLA", "WCTLB", "WCTLC",
    "WCTLD", "LWTA0U", "LWTA0L", "LWTA1U", "LWTA1L", "SPCTL", "SDCTL", "CRAOFA", "CRAOFB", "LNCLEN", "SFPRMD", "CCCTL",
    "SFCCMD", "PRISA", "PRISB", "PRISC", "PRISD", "PRINA", "PRINB", "PRIR", "-", "CCRSA", "CCRSB", "CCRSC", "CCRSD",
    "CCRNA", "CCRNB", "CCRR", "CCRLB", "CLOFEN", "CLOFSL", "COAR", "COAG", "COAB", "COBR", "COBG", "COBB"]))


def describe_registers(blob_hex):
    """'CCCTL=0002 CCRNA=1000' from the pairs (index, value) of a blob of tests/rom/t_state.c."""
    data = bytes.fromhex(blob_hex)
    out = []
    for p in range(0, len(data) - 3, 4):
        # (big endian: the shorts of the SH-2)
        index = (data[p] << 8) | data[p + 1]
        value = (data[p + 2] << 8) | data[p + 3]
        out.append("%s=%04X" % (VDP2_REGISTERS.get(index, "reg%02X" % (2 * index)), value))
    return " ".join(out) if out else "(no register changed)"


class Results:
    def __init__(self, report_dir):
        self.items = []
        self.report_dir = report_dir
        shutil.rmtree(report_dir, ignore_errors=True)
        os.makedirs(report_dir, exist_ok=True)

    def add(self, layer, name, status, message="", details=None, images=None, quiet=False):
        item = {"layer": layer, "name": name, "status": status, "message": message, "details": details or [],
                "images": images or []}
        self.items.append(item)
        if not quiet or status in (FAIL, WARN):
            line = "  %-4s %-48s %s" % (status, name, message)
            print(line.rstrip())
            for d in (details or [])[:12]:
                print("         " + d)
        return item

    def image_path(self, name):
        path = os.path.join(self.report_dir, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        return path

    def count(self, status):
        return sum(1 for i in self.items if i["status"] == status)

    def write(self, title, summary_lines):
        with open(os.path.join(os.path.dirname(self.report_dir), "results.json"), "w") as f:
            json.dump({"title": title, "summary": summary_lines, "items": self.items}, f, indent=1)
        rows = []
        order = {FAIL: 0, WARN: 1, NEW: 2, PASS: 3, SKIP: 4}
        for item in sorted(self.items, key=lambda i: (order[i["status"]], i["layer"], i["name"])):
            details = "".join("<div class='d'>%s</div>" % html.escape(d) for d in item["details"][:200])
            images = "".join("<figure><img src='%s'><figcaption>%s</figcaption></figure>" %
                             (html.escape(os.path.relpath(p, os.path.dirname(self.report_dir)).replace("\\", "/")), html.escape(label))
                             for label, p in item["images"])
            rows.append("<tr><td><b style='color:%s'>%s</b></td><td>%s</td><td>%s</td><td>%s%s%s</td></tr>" %
                        (_COLORS[item["status"]], item["status"], html.escape(item["layer"]), html.escape(item["name"]),
                         html.escape(item["message"]), details, ("<div class='imgs'>%s</div>" % images) if images else ""))
        page = """<!doctype html><html><head><meta charset="utf-8"><title>%s</title><style>
body{font-family:system-ui,sans-serif;margin:16px;background:#fafafa;color:#222}
table{border-collapse:collapse;width:100%%}td{border-bottom:1px solid #ddd;padding:6px;vertical-align:top;font-size:14px}
.d{font-family:monospace;font-size:12px;color:#555}.imgs{display:flex;gap:8px;flex-wrap:wrap;margin-top:6px}
figure{margin:0}img{max-width:360px;image-rendering:pixelated;border:1px solid #ccc}figcaption{font-size:12px;color:#666}
</style></head><body><h1>%s</h1>%s<table>%s</table></body></html>""" % (
            html.escape(title), html.escape(title), "".join("<p>%s</p>" % html.escape(l) for l in summary_lines), "".join(rows))
        path = os.path.join(os.path.dirname(self.report_dir), "report.html")
        with open(path, "w", encoding="utf-8") as f:
            f.write(page)
        return path
