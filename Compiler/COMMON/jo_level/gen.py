"""Make the documentation and the C header of the level format from schema.py

    python3 Compiler/COMMON/jo_level gen        (from the root of the repository)
"""

import os

from . import schema
from .schema import TYPES, SECTIONS, FILE_HEADER, DIRECTORY_ENTRY
from .level import _layout

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
DOC = os.path.join(ROOT, 'Documentation', 'level_format.md')
HEADER = os.path.join(ROOT, 'jo_engine', 'jo', 'level_format.h')


def _table(fields):
    offsets, size = _layout(fields)
    rows = ['| Offset | Field | Type | Default | Since | Description |', '|---|---|---|---|---|---|']
    for f, o in zip(fields, offsets):
        t = f.type + ('[%d]' % f.count if f.count > 1 else '')
        d = f.doc + (' (%s)' % f.unit if f.unit else '')
        default = '0x%X' % f.default if f.default > 9 else str(f.default)
        rows.append('| %d | `%s` | %s | %s | v%d | %s |' % (o, f.name, t, default, f.since, d))
    rows.append('')
    rows.append('Size: %d bytes.' % size)
    return '\n'.join(rows)


def make_doc():
    out = ['# Jo Engine level file (JOL) {#level_format}', '',
           '*This file is made by `Compiler/COMMON/jo_level/gen.py` from `Compiler/COMMON/jo_level/schema.py`: change the '
           'schema, not this file.*', '',
           'A level (`.JOL`) holds the data of a level of a game: terrain, models and their collisions, plants, decals... '
           'The engine loads it with `jo_level_load()` (module `JO_COMPILE_WITH_LEVEL_MODULE = 1`); the tools of '
           '`Compiler/COMMON/jo_level` write it (Python), check it and print it.', '',
           '## Rules of the format', '', schema.FORMAT_RULES, '',
           '## Types', '',
           '| Type | Size | Description |', '|---|---|---|',
           '| u8, i8, u16, i16, u32, i32 | 1, 2, 4 | Integers (big endian) |',
           '| fixed | 4 | Q16.16 fixed point number (jo_fixed) |',
           '| string | 4 | Offset of a string in the STRS section, 0x%X: no string |' % schema.NO_STRING,
           '| offset | 4 | Offset of an array from the start of the section (0: empty) |',
           '| index | 2 | Index of a record of another section, 0x%X: none |' % schema.NO_INDEX, '',
           '## File header', '', _table(FILE_HEADER), '',
           '## Directory entry', '', 'One entry per section, after the file header.', '', _table(DIRECTORY_ENTRY), '',
           '## Sections', '']
    for s in SECTIONS:
        out += ['### %s: %s (version %d%s)' % (s.id, s.title, s.version, ', required' if s.flags & schema.FLAG_REQUIRED else ''),
                '', s.doc + '.', '']
        if s.raw:
            continue
        if s.header:
            out += ['Header:', '', _table(s.header), '']
        if s.record:
            out += ['Record:', '', _table(s.record), '']
        if s.arrays:
            out += ['Arrays (after the header and the records, aligned on 4 bytes):', '']
            for a in s.arrays:
                count = a.count_field or a.count_expr
                out.append('- `%s`: %s, %s elements (offset: `%s`). %s.' % (a.name, a.type, count, a.offset_field, a.doc))
            out.append('')
    out += ['## Enumerations', '']
    for e in schema.ENUMS:
        out += ['### %s' % e.name, '', e.doc + '.', '', '| Value | Name | Description |', '|---|---|---|']
        for k, (name, doc) in enumerate(e.values):
            out.append('| %d | `%s_%s` | %s |' % (k, e.name, name, doc))
        out.append('')
    out += ['### ENTITY_FLAG', '', 'Flags of the entities (ENTI.flags).', '', '| Value | Name | Description |', '|---|---|---|']
    for name, value, doc in schema.ENTITY_FLAGS:
        out.append('| 0x%04X | `ENTITY_FLAG_%s` | %s |' % (value, name, doc))
    out += ['', '## Classes of entities of the engine', '',
            'The value of `ENTI.cls`. A game registers its own classes with `jo_entity_register_class()` (a class with the '
            'name of a class of the engine replaces it). A property missing in the level gets its default value.', '']
    for name, doc, props in schema.ENTITY_CLASSES:
        out += ['### %s' % name, '', doc + '.', '']
        if props:
            out += ['| Property | Type | Default | Description |', '|---|---|---|---|']
            for key, type, default, doc in props:
                out.append('| `%s` | %s | %s | %s |' % (key, type, '"%s"' % default if type == 'STRING' else default, doc))
            out.append('')
    out += ['## Reserved identifiers', '', 'For the next versions of the engine:', '']
    for k, v in schema.RESERVED_IDS.items():
        out.append('- `%s`: %s' % (k, v))
    out += ['', '## Changing the format', '',
            '1. Edit `Compiler/COMMON/jo_level/schema.py`:',
            '   - a new field: at the end of the header or of the record, with `default=` and `since=` (the new version of '
            'the section), then the version of the section + 1;',
            '   - a new section: a new `Section` (identifier of 4 characters, version 1). `REQUIRED` only if an older engine '
            'would draw the level wrongly without it.',
            '2. `python3 Compiler/COMMON/jo_level gen`: this documentation and `jo_engine/jo/level_format.h` are made again.',
            '3. The engine reads the new fields (`jo_level_get_header()`, `jo_level_get_records()` give the default values '
            'of the fields missing in an older file).',
            '4. The tools that write the levels fill the new fields.', '',
            'An engine reads the levels written for an older engine (the missing fields get their default values) and, '
            'when no section is required, the levels written for a newer engine (it ignores what it doesn\'t know).', '',
            '## Tools', '',
            '```', 'python3 Compiler/COMMON/jo_level dump cd/LEVEL.JOL      # print the sections',
            'python3 Compiler/COMMON/jo_level check cd/LEVEL.JOL     # check the file',
            'python3 Compiler/COMMON/jo_level gen                    # make this file and jo_engine/jo/level_format.h', '```', '',
            'Writing a level in Python:', '', '```python', 'import sys', "sys.path.insert(0, 'Compiler/COMMON')",
            'import jo_level', "lv = jo_level.Level('My level')",
            "lv.add('TERR', header=dict(n=3, cell=128, origin_x=0, origin_z=0),",
            "       arrays=dict(heights=[0] * 9, colors=[0x8000] * 9, holes=[]))",
            "lv.add('PLNT', records=[dict(x=0, y=0, z=0, kind=0, angle=0, scale=256)])",
            "lv.add_entities([dict(name='barrel', cls='explosive', model='BARREL.JOM', x=0, y=0, z=200, health=40,",
            "                      flags=['SOLID'], radius=14, height=40, props=dict(sound='EXPLODE.PCM'),",
            "                      links=[dict(event='DEATH', target='fuse', action='TRIGGER', delay=400)]),",
            "                 dict(name='fuse', cls='relay', flags=['HIDDEN'],",
            "                      links=[dict(event='TRIGGER', target='barrel2', action='KILL')]),",
            "                 dict(name='barrel2', cls='explosive', model='BARREL.JOM', x=300, z=200, health=40,",
            "                      flags=['SOLID'], radius=14, height=40)])",
            "lv.write('cd/LEVEL.JOL')", '```', '']
    return '\n'.join(out)


def _c_struct(name, fields, doc):
    offsets, size = _layout(fields)
    lines = ['/** @brief %s (%d bytes) */' % (doc, size), 'typedef struct', '{']
    pos = 0
    pad = 0
    for f, o in zip(fields, offsets):
        if o > pos:
            lines.append('    unsigned char               __pad%d[%d];' % (pad, o - pos))
            pad += 1
        ctype = TYPES[f.type][2]
        member = f.name + ('[%d]' % f.count if f.count > 1 else '')
        lines.append('    %-27s %-20s // v%d: %s%s' % (ctype, member + ';', f.since, f.doc, ' (%s)' % f.unit if f.unit else ''))
        pos = o + f.size
    if size > pos:
        lines.append('    unsigned char               __pad%d[%d];' % (pad, size - pos))
    lines.append('}                               %s;' % name)
    lines.append('')
    return lines


def _c_defaults(name, fields):
    values = []
    for f in fields:
        if f.default:
            v = '0x%X' % f.default if f.default > 9 else str(f.default)
            values.append('.%s = %s' % (f.name, '{' + ', '.join([v] * f.count) + '}' if f.count > 1 else v))
    return ['/** @brief Default values (the fields missing in an older file, see jo_level_get_records()) */',
            '# define %-34s {%s}' % (name, ', '.join(values)), '']


def make_header():
    out = ['/*', '** Jo Sega Saturn Engine: level file format (JOL)', '**',
           '** Made by Compiler/COMMON/jo_level/gen.py from Compiler/COMMON/jo_level/schema.py: change the schema, not this file.',
           '** Documentation: Documentation/level_format.md', '*/', '',
           '#ifndef __JO_LEVEL_FORMAT_H__', '# define __JO_LEVEL_FORMAT_H__', '',
           '/** @brief File magic */', "# define JO_LEVEL_MAGIC                     (0x%08X)" % int.from_bytes(schema.MAGIC, 'big'),
           '/** @brief Version of the file format (header and directory) */',
           '# define JO_LEVEL_FORMAT_VERSION            (%d)' % schema.FORMAT_VERSION,
           '/** @brief A reader that doesn\'t know this section must refuse the file */',
           '# define JO_LEVEL_FLAG_REQUIRED             (0x%04X)' % schema.FLAG_REQUIRED,
           '/** @brief No string (field of type string) */',
           '# define JO_LEVEL_NO_STRING                 (0x%08XU)' % schema.NO_STRING,
           '/** @brief No record (field of type index) */',
           '# define JO_LEVEL_NO_INDEX                  (0x%04X)' % schema.NO_INDEX, '',
           '/** @brief Identifier of a section from its 4 characters */',
           "# define JO_LEVEL_ID(A, B, C, D)            (((unsigned int)(A) << 24) | ((unsigned int)(B) << 16) | ((unsigned int)(C) << 8) | (unsigned int)(D))", '']
    out += _c_struct('jo_level_file_header', FILE_HEADER, 'File header')
    out += _c_struct('jo_level_directory_entry', DIRECTORY_ENTRY, 'Directory entry (one per section, after the file header)')
    for s in SECTIONS:
        lower = s.id.lower().strip()
        out.append('/*')
        out.append('** %s: %s (version %d)' % (s.id, s.title, s.version))
        out.append('*/')
        out.append('# define JO_LEVEL_SECTION_%-18s JO_LEVEL_ID(%s)' % (s.id.strip(), ', '.join("'%s'" % c for c in s.id)))
        out.append('# define JO_LEVEL_%s_VERSION%s (%d)' % (s.id.strip(), ' ' * (18 - len(s.id.strip())), s.version))
        out.append('')
        if s.header:
            out += _c_struct('jo_level_%s_header' % lower, s.header, '%s: header' % s.title)
            out += _c_defaults('JO_LEVEL_%s_HEADER_DEFAULTS' % s.id.strip(), s.header)
        if s.record:
            out += _c_struct('jo_level_%s_record' % lower, s.record, '%s: record' % s.title)
            out += _c_defaults('JO_LEVEL_%s_RECORD_DEFAULTS' % s.id.strip(), s.record)
    for e in schema.ENUMS:
        out.append('/** @brief %s */' % e.doc)
        for k, (name, doc) in enumerate(e.values):
            out.append('# define %-34s (%d)' % ('JO_LEVEL_%s_%s' % (e.name, name), k))
        out.append('')
    out.append('/** @brief Flags of the entities (ENTI.flags) */')
    for name, value, doc in schema.ENTITY_FLAGS:
        out.append('# define %-34s (0x%04X)' % ('JO_LEVEL_ENTITY_FLAG_%s' % name, value))
    out.append('')
    out += ['#endif /* !__JO_LEVEL_FORMAT_H__ */', '', '/*', '** END OF FILE', '*/', '']
    return '\n'.join(out)


def write_all():
    for path, text in ((DOC, make_doc()), (HEADER, make_header())):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', newline='\r\n') as f:
            f.write(text)
        print('written', os.path.relpath(path, ROOT))
