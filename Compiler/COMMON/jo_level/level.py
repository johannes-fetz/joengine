"""Jo Engine level file (JOL): writer, reader, validator and dump (made from schema.py).

Example:
    import jo_level
    lv = jo_level.Level('My level')
    lv.add('TERR', header=dict(n=3, cell=128, origin_x=0, origin_z=0),
           arrays=dict(heights=[0] * 9, colors=[0x8000] * 9, holes=[]))
    lv.add('PLNT', records=[dict(x=0, y=40, z=0, kind=0, angle=30, scale=256)])
    lv.write('cd/LEVEL.JOL')

    python3 -m jo_level dump cd/LEVEL.JOL      (or: python3 Compiler/COMMON/jo_level dump FILE)
"""

import struct

from . import schema
from .schema import TYPES, SECTION_BY_ID, NO_STRING


def _layout(fields):
    """Offsets of the fields (natural alignment, like the C structures of jo_engine/jo/level_format.h)"""
    offsets = []
    size = 0
    for f in fields:
        a = TYPES[f.type][1]
        size = (size + a - 1) // a * a
        offsets.append(size)
        size += f.size
    return offsets, ((size + 3) // 4 * 4 if fields else 0)


def _pack(fields, values, where):
    offsets, size = _layout(fields)
    out = bytearray(size)
    names = {f.name for f in fields}
    for k in values:
        if k not in names:
            raise ValueError('%s: unknown field %r' % (where, k))
    for f, o in zip(fields, offsets):
        v = values.get(f.name, f.default)
        fmt = TYPES[f.type][0]
        if f.count > 1:
            v = list(v) if isinstance(v, (list, tuple)) else [v] * f.count
            if len(v) != f.count:
                raise ValueError('%s.%s: %d values expected' % (where, f.name, f.count))
            struct.pack_into('>%d%s' % (f.count, fmt), out, o, *[int(x) for x in v])
        else:
            try:
                struct.pack_into('>' + fmt, out, o, int(v))
            except struct.error as e:
                raise ValueError('%s.%s = %r: %s' % (where, f.name, v, e))
    return bytes(out)


def _unpack(fields, data, version):
    """Fields of a header or a record: the fields missing in the data (older file) get their default value"""
    offsets, size = _layout(fields)
    values = {}
    for f, o in zip(fields, offsets):
        fmt = TYPES[f.type][0]
        if o + f.size <= len(data):
            v = struct.unpack_from('>%d%s' % (f.count, fmt), data, o)
            values[f.name] = list(v) if f.count > 1 else v[0]
        else:
            values[f.name] = f.default
    return values


def _align(buf, n=4):
    while len(buf) % n:
        buf.append(0)


class Section:
    def __init__(self, id, header=None, records=None, arrays=None, data=None, version=None, flags=None):
        self.id = id
        self.schema = SECTION_BY_ID.get(id)
        self.header = dict(header or {})
        self.records = list(records or [])
        self.arrays = dict(arrays or {})
        self.data = data            # Raw sections, or a section unknown by these tools (kept as it is)
        self.version = version if version is not None else (self.schema.version if self.schema else 1)
        self.flags = flags if flags is not None else (self.schema.flags if self.schema else 0)


class Level:
    def __init__(self, name=None):
        self.sections = []
        self._strings = bytearray()
        self._string_offsets = {}
        self.add('INFO', header=dict(name=self.string(name) if name else NO_STRING))

    # Building

    def string(self, s):
        """Offset of a string in the STRS section (the same string is written once)"""
        if s not in self._string_offsets:
            self._string_offsets[s] = len(self._strings)
            self._strings += s.encode('ascii') + b'\0'
        return self._string_offsets[s]

    def add(self, id, header=None, records=None, arrays=None, data=None, flags=None):
        """Add a section. A game section (identifier with a lowercase letter) has no schema: give its raw data"""
        if id in [s.id for s in self.sections]:
            raise ValueError('%s: the section is already in the level' % id)
        if id not in SECTION_BY_ID and data is None:
            raise ValueError('%s: unknown section (a game section is given as raw data)' % id)
        self.sections.append(Section(id, header, records, arrays, data, flags=flags))
        return self.sections[-1]

    def section(self, id):
        for s in self.sections:
            if s.id == id:
                return s
        return None

    def add_entities(self, entities):
        """Add the entities of the level (sections ENTI, PROP and LINK) from a list of dictionaries:

            dict(name='barrel1', cls='explosive', model='BARREL.JOM', x=0, y=-40, z=100, yaw=0, health=50,
                 flags=['SOLID'], radius=14, height=40,
                 props=dict(damage=120, radius=260, sound='EXPLODE.PCM', size=1.5),    # int: INT, float: FIXED, str: STRING
                 links=[dict(event='DEATH', target='relay1', action='TRIGGER', delay=500, param=-1)])

        The events, the actions and the flags are the names of the enumerations of the schema (or their values). A link
        without a target is sent to the entity itself
        """
        names = [e['name'] for e in entities]
        if len(set(names)) != len(names):
            raise ValueError('ENTI: two entities have the same name')
        flag_values = {n: v for n, v, d in schema.ENTITY_FLAGS}

        def enum(kind, v):
            values = [n for n, d in schema.ENUM_BY_NAME[kind].values]
            if isinstance(v, str):
                if v.upper() not in values:
                    raise ValueError('%s: unknown %s (%s)' % (v, kind, ', '.join(values)))
                return values.index(v.upper())
            return int(v)
        records, props, links = [], [], []
        for e in entities:
            known = {'name', 'cls', 'model', 'x', 'y', 'z', 'yaw', 'health', 'flags', 'radius', 'height', 'props', 'links'}
            for k in e:
                if k not in known:
                    raise ValueError('ENTI %s: unknown field %r' % (e['name'], k))
            flags = e.get('flags', 0)
            if isinstance(flags, (list, tuple)):
                flags = sum(flag_values[f.upper()] for f in flags)
            r = dict(name=self.string(e['name']), cls=self.string(e['cls']),
                     model=self.string(e['model']) if e.get('model') else schema.NO_STRING,
                     x=e.get('x', 0), y=e.get('y', 0), z=e.get('z', 0), yaw=e.get('yaw', 0), health=e.get('health', 0),
                     flags=flags, radius=e.get('radius', 0), height=e.get('height', 0),
                     first_prop=len(props), prop_count=len(e.get('props', {})),
                     first_link=len(links), link_count=len(e.get('links', [])))
            for key, v in e.get('props', {}).items():
                if isinstance(v, str):
                    props.append(dict(key=self.string(key), type=2, value=self.string(v)))
                elif isinstance(v, float):
                    props.append(dict(key=self.string(key), type=1, value=int(round(v * 65536))))
                else:
                    props.append(dict(key=self.string(key), type=0, value=int(v)))
            for l in e.get('links', []):
                t = l.get('target')
                if t is not None and t not in names:
                    raise ValueError('ENTI %s: link to an unknown entity %r' % (e['name'], t))
                links.append(dict(event=enum('EVENT', l['event']), action=enum('ACTION', l['action']),
                                  target=names.index(t) if t is not None else schema.NO_INDEX,
                                  delay=l.get('delay', 0), param=l.get('param', -1)))
            records.append(r)
        self.add('ENTI', records=records)
        if props:
            self.add('PROP', records=props)
        if links:
            self.add('LINK', records=links)

    def add_cells(self, cells):
        """Add the cells of an inside level and their portals (sections CELL and PRTL) from a list of dictionaries:

            dict(name='hall', model='HALL.JOM', box=(x0, z0, x1, z1), floor0=0, floor1=0, slope='FLAT', yaw=0, reflection='HALLR.JOM',
                 portals=[dict(cell='corridor', kind='DOOR', points=[(x, y, z), (x, y, z), (x, y, z), (x, y, z)])])

        The positions are world units, Saturn axes (Y down). A portal leads to the cell named by its cell (a name or an
        index, None: the outside); the stairs: slope 'X' or 'Z', the floor goes from floor0 (at x0 or z0) to floor1 (at x1
        or z1); yaw: the box is turned around its center (degrees); reflection (optional): a simpler model drawn in the
        mirrors of the cell (a polished floor)
        """
        names = [c['name'] for c in cells]
        if len(set(names)) != len(names):
            raise ValueError('CELL: two cells have the same name')

        def enum(kind, v):
            values = [n for n, d in schema.ENUM_BY_NAME[kind].values]
            return values.index(v.upper()) if isinstance(v, str) else int(v)
        records, portals = [], []
        for c in cells:
            x0, z0, x1, z1 = c['box']
            r = dict(model=self.string(c['model']) if c.get('model') else schema.NO_STRING, x0=x0, z0=z0, x1=x1, z1=z1,
                     floor0=c.get('floor0', 0), floor1=c.get('floor1', c.get('floor0', 0)),
                     slope=enum('CELL_SLOPE', c.get('slope', 'FLAT')), first_portal=len(portals),
                     portal_count=len(c.get('portals', [])), yaw=int(round(c.get('yaw', 0))),
                     reflection=self.string(c['reflection']) if c.get('reflection') else schema.NO_STRING)
            for p in c.get('portals', []):
                target = p['cell']
                if target is None:
                    target = schema.NO_INDEX
                elif isinstance(target, str):
                    if target not in names:
                        raise ValueError('CELL %s: portal to an unknown cell %r' % (c['name'], target))
                    target = names.index(target)
                pts = [int(round(v)) for pt in p['points'] for v in pt]
                if len(pts) != 12:
                    raise ValueError('CELL %s: a portal has 4 points' % c['name'])
                portals.append(dict(cell=target, kind=enum('PORTAL', p.get('kind', 'DOOR')), points=pts))
            records.append(r)
        self.add('CELL', records=records)
        if portals:
            self.add('PRTL', records=portals)

    # Writing

    def _section_bytes(self, s):
        sc = s.schema
        if s.id == 'STRS':
            return bytes(self._strings), 0, 0, 0
        if sc is None or sc.raw:
            return bytes(s.data or b''), 0, 0, 0
        header = dict(s.header)
        # Arrays after the header and the records (their offsets and counts are fields of the header)
        tail = bytearray()
        base = sc.header_size + sc.record_size * len(s.records)
        for a in sc.arrays:
            values = s.arrays.get(a.name, [])
            _align(tail)
            header[a.offset_field] = base + len(tail) if values else 0
            if a.count_field:
                header[a.count_field] = len(values)
            elif a.count_expr:
                expected = eval(a.count_expr, {}, dict(header))
                if len(values) != expected:
                    raise ValueError('%s.%s: %d elements, %s = %d expected' % (s.id, a.name, len(values), a.count_expr, expected))
            fmt = TYPES[a.type][0]
            tail += struct.pack('>%d%s' % (len(values), fmt), *[int(v) for v in values])
        _align(tail)
        out = bytearray(_pack(sc.header, header, s.id) if sc.header else b'')
        for k, r in enumerate(s.records):
            out += _pack(sc.record, r, '%s[%d]' % (s.id, k))
        out += tail
        return bytes(out), sc.header_size, sc.record_size, len(s.records)

    def to_bytes(self):
        if self._strings and not self.section('STRS'):
            self.add('STRS')
        _, hsize = _layout(schema.FILE_HEADER)
        _, esize = _layout(schema.DIRECTORY_ENTRY)
        blobs = [self._section_bytes(s) for s in self.sections]
        offset = hsize + esize * len(self.sections)
        entries = []
        body = bytearray()
        for s, (data, header_size, record_size, record_count) in zip(self.sections, blobs):
            _align(body)
            entries.append(dict(id=struct.unpack('>I', s.id.encode('ascii'))[0], version=s.version, flags=s.flags,
                                offset=offset + len(body), size=len(data), header_size=header_size,
                                record_size=record_size, record_count=record_count))
            body += data
        _align(body)
        total = offset + len(body)
        out = bytearray(_pack(schema.FILE_HEADER, dict(magic=struct.unpack('>I', schema.MAGIC)[0], version=schema.FORMAT_VERSION,
                                                        size=total, section_count=len(self.sections)), 'header'))
        for e in entries:
            out += _pack(schema.DIRECTORY_ENTRY, e, 'directory')
        out += body
        return bytes(out)

    def write(self, path):
        data = self.to_bytes()
        validate(data)
        with open(path, 'wb') as f:
            f.write(data)
        return len(data)


# Reading

def read(data):
    """Sections of a JOL file: list of (directory entry, Section)"""
    if isinstance(data, str):
        with open(data, 'rb') as f:
            data = f.read()
    _, hsize = _layout(schema.FILE_HEADER)
    _, esize = _layout(schema.DIRECTORY_ENTRY)
    head = _unpack(schema.FILE_HEADER, data[:hsize], 1)
    if struct.pack('>I', head['magic']) != schema.MAGIC:
        raise ValueError('not a JOL file')
    result = []
    for k in range(head['section_count']):
        e = _unpack(schema.DIRECTORY_ENTRY, data[hsize + k * esize:hsize + (k + 1) * esize], 1)
        e['name'] = struct.pack('>I', e['id']).decode('ascii', 'replace')
        raw = data[e['offset']:e['offset'] + e['size']]
        sc = SECTION_BY_ID.get(e['name'])
        s = Section(e['name'], version=e['version'], flags=e['flags'])
        if sc is None or sc.raw:
            s.data = raw
        else:
            s.header = _unpack(sc.header, raw[:e['header_size']], e['version'])
            for r in range(e['record_count']):
                o = e['header_size'] + r * e['record_size']
                s.records.append(_unpack(sc.record, raw[o:o + e['record_size']], e['version']))
            for a in sc.arrays:
                off = s.header.get(a.offset_field, 0)
                n = s.header[a.count_field] if a.count_field else eval(a.count_expr, {}, dict(s.header))
                fmt = TYPES[a.type][0]
                s.arrays[a.name] = list(struct.unpack_from('>%d%s' % (n, fmt), raw, off)) if n and off else []
        result.append((e, s))
    return head, result


def validate(data):
    """Check a JOL file: header, directory, sections in the file, known sections, indices and strings. Raises ValueError"""
    head, sections = read(data)
    if head['size'] != len(data):
        raise ValueError('size of the file: %d in the header, %d bytes' % (head['size'], len(data)))
    ids = [e['name'] for e, s in sections]
    if not ids or ids[0] != 'INFO':
        raise ValueError('the first section must be INFO')
    strs = next((s.data for e, s in sections if e['name'] == 'STRS'), b'')
    counts = {e['name']: e['record_count'] for e, s in sections}
    for e, s in sections:
        if e['offset'] % 4 or e['offset'] + e['size'] > len(data):
            raise ValueError('%s: section outside of the file or not aligned' % e['name'])
        sc = s.schema
        if sc is None:
            if e['name'] == e['name'].upper() and e['flags'] & schema.FLAG_REQUIRED:
                raise ValueError('%s: unknown required section' % e['name'])
            continue
        if e['version'] > sc.version and e['flags'] & schema.FLAG_REQUIRED:
            raise ValueError('%s: version %d, these tools know version %d' % (e['name'], e['version'], sc.version))
        for f in sc.header + sc.record:
            if f.type == 'string':
                for v in [s.header.get(f.name)] + [r.get(f.name) for r in s.records]:
                    if v is not None and v != NO_STRING and v >= len(strs):
                        raise ValueError('%s.%s: string outside of STRS' % (e['name'], f.name))
    # Indices between sections
    def check_index(section, field, target):
        for k, r in enumerate(next((s.records for e, s in sections if e['name'] == section), [])):
            v = r[field]
            if v != schema.NO_INDEX and v >= counts.get(target, 0):
                raise ValueError('%s[%d].%s = %d: only %d records in %s' % (section, k, field, v, counts.get(target, 0), target))
    check_index('INST', 'model', 'MODL')
    check_index('INST', 'near_model', 'MODL')
    check_index('INST', 'inside_model', 'MODL')
    for k, r in enumerate(next((s.records for e, s in sections if e['name'] == 'MODL'), [])):
        if r['first_box'] + r['box_count'] > counts.get('MBOX', 0):
            raise ValueError('MODL[%d]: boxes outside of MBOX' % k)
    check_index('LINK', 'target', 'ENTI')
    check_index('PRTL', 'cell', 'CELL')
    for k, r in enumerate(next((s.records for e, s in sections if e['name'] == 'CELL'), [])):
        if r['first_portal'] + r['portal_count'] > counts.get('PRTL', 0):
            raise ValueError('CELL[%d]: portals outside of PRTL' % k)
    for k, r in enumerate(next((s.records for e, s in sections if e['name'] == 'ENTI'), [])):
        if r['first_prop'] + r['prop_count'] > counts.get('PROP', 0):
            raise ValueError('ENTI[%d]: properties outside of PROP' % k)
        if r['first_link'] + r['link_count'] > counts.get('LINK', 0):
            raise ValueError('ENTI[%d]: links outside of LINK' % k)
    for k, r in enumerate(next((s.records for e, s in sections if e['name'] == 'PROP'), [])):
        if r['type'] == 2 and not 0 <= r['value'] < len(strs):
            raise ValueError('PROP[%d]: string outside of STRS' % k)
    terr = next((s for e, s in sections if e['name'] == 'TERR'), None)
    if terr is not None:
        n = terr.header['n']
        for v in terr.arrays['holes']:
            if (v & 0x7FFF) >= n * n:
                raise ValueError('TERR: hole %d outside of the height map' % v)
    return True


def dump(data):
    head, sections = read(data)
    strs = next((s.data for e, s in sections if e['name'] == 'STRS'), b'')

    def text(v):
        if v == NO_STRING:
            return '-'
        return strs[v:strs.index(b'\0', v)].decode('ascii', 'replace')
    lines = ['JOL file version %d, %d bytes, %d sections' % (head['version'], head['size'], head['section_count'])]
    for e, s in sections:
        sc = s.schema
        lines.append('%s v%d%s  %7d bytes at %7d  header %d, %d records of %d bytes%s' % (
            e['name'], e['version'], ' (required)' if e['flags'] & schema.FLAG_REQUIRED else '', e['size'], e['offset'],
            e['header_size'], e['record_count'], e['record_size'], '' if sc else ('  (game section)' if e['name'] != e['name'].upper() else '  (unknown section)')))
        if sc is None or sc.raw:
            continue
        for f in sc.header:
            if f.type != 'offset':
                v = s.header[f.name]
                lines.append('    %-14s %s' % (f.name, text(v) if f.type == 'string' else v))
        for name, values in s.arrays.items():
            lines.append('    [%s] %d elements' % (name, len(values)))
        for k, r in enumerate(s.records[:8]):
            lines.append('    %3d: %s' % (k, ', '.join('%s=%s' % (f.name, text(r[f.name]) if f.type == 'string' else r[f.name])
                                                     for f in sc.record)))
        if len(s.records) > 8:
            lines.append('    ... (%d more)' % (len(s.records) - 8))
    return '\n'.join(lines)
