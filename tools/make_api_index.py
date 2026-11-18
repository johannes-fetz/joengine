#!/usr/bin/env python3
"""The index of the API of the Jo Engine (Documentation/api_index.md): each function, macro and type of the headers
(jo_engine/jo/*.h) on one line, with its brief (the @brief of its comment), by header. Made to be searched (grep) before
writing a function: the engine probably has it, faster on the Saturn (fixed point, tables, the hardware).

    python3 tools/make_api_index.py           write Documentation/api_index.md
    python3 tools/make_api_index.py --check   exit code 1 if the index is not the one of the headers

The names that start with "__" (internal) are not listed. A declaration under #ifdef JO_DEBUG or
JO_COMPILE_WITH_xxx_SUPPORT says so.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HEADERS = os.path.join(HERE, '..', 'jo_engine', 'jo')
OUT = os.path.join(HERE, '..', 'Documentation', 'api_index.md')
BRIEF_MAX = 200
# The headers of the engine itself (not an API of the game)
SKIPPED = {'jo.h', 'conf.h'}
# The modules of the makefile on by default (see Compiler/COMMON/jo_engine_makefile)
DEFAULT_MODULES = {'FS', 'DUAL_CPU', 'PRINTF', 'RAM_CARD'}


def option_text(define):
    """The setting of the makefile of a game for a define of the engine"""
    if define == 'JO_DEBUG':
        return 'JO_DEBUG only'
    m = re.match(r'JO_COMPILE_WITH_(\w+)_SUPPORT$', define)
    if m and m.group(1) in ('KEYBOARD', 'CONSOLE'):
        return '%s = 1' % define
    if m:
        return 'JO_COMPILE_WITH_%s_MODULE%s' % (m.group(1), ', on by default' if m.group(1) in DEFAULT_MODULES else ' = 1')
    return define


def clean(text):
    return re.sub(r'\s+', ' ', text).strip()


def brief_of(comment):
    """The brief of a doc comment: the text of @brief up to the next tag, else the first sentence"""
    body = re.sub(r'^\s*/\*\*|\*/\s*$', '', comment)
    body = '\n'.join(re.sub(r'^\s*\*\s?', '', line) for line in body.split('\n'))
    m = re.search(r'@brief\s+(.*?)(?=\n\s*@\w|\Z)', body, re.S)
    text = clean(m.group(1) if m else body.split('@')[0])
    if len(text) > BRIEF_MAX:
        text = text[:BRIEF_MAX].rsplit(' ', 1)[0] + '...'
    return text


def declaration_after(text, pos):
    """The declaration after a doc comment: (kind, name, signature) or None"""
    rest = text[pos:].lstrip()
    if rest.startswith('/*') or rest.startswith('//'):
        return None
    m = re.match(r'#\s*define\s+(\w+)(\([^)]*\))?', rest)
    if m:
        return 'macro', m.group(1), m.group(1) + (m.group(2) or '')
    if rest.startswith('#'):
        return None
    # (up to the end of the declaration: ";" or the body of an inline function)
    end = len(rest)
    for stop in (';', '{'):
        k = rest.find(stop)
        if k >= 0:
            end = min(end, k)
    decl = clean(rest[:end])
    if decl.startswith('typedef'):
        if rest[end:end + 1] == '{':
            # (typedef struct { ... } name;)
            depth = 0
            for k in range(end, len(rest)):
                if rest[k] == '{':
                    depth += 1
                elif rest[k] == '}':
                    depth -= 1
                    if depth == 0:
                        m = re.match(r'\s*(\w+)\s*;', rest[k + 1:])
                        if m:
                            return 'type', m.group(1), m.group(1)
                        return None
            return None
        m = re.search(r'\(\s*\*\s*(\w+)\s*\)\s*\(', decl) or re.search(r'(\w+)\s*(\[[^\]]*\])?$', decl)
        return ('type', m.group(1), decl) if m else None
    if re.match(r'(struct|enum|union)\s+\w+$', decl):
        return 'type', decl.split()[1], decl
    m = re.search(r'(\w+)\s*\((.*)\)$', decl, re.S)
    if m:
        name = m.group(1)
        ret = clean(decl[:m.start(1)].replace('static', '').replace('__jo_force_inline', '').replace('inline', ''))
        return 'function', name, '%s(%s) -> %s' % (name, clean(m.group(2)), ret or 'void')
    m = re.search(r'(\w+)\s*(\[[^\]]*\])?$', decl)
    if m and decl.startswith('extern'):
        return 'variable', m.group(1), decl
    return None


def conditions_at(text, pos):
    """The #ifdef of the options around a position (JO_DEBUG, JO_COMPILE_WITH_xxx)"""
    stack = []
    for m in re.finditer(r'^\s*#\s*(ifdef|ifndef|if|else|elif|endif)\b(.*)$', text[:pos], re.M):
        kind, arg = m.group(1), clean(m.group(2))
        if kind in ('ifdef', 'ifndef', 'if'):
            stack.append((kind, arg))
        elif kind in ('else', 'elif') and stack:
            k, a = stack[-1]
            stack[-1] = ('else', a if k == 'ifdef' else '')
        elif kind == 'endif' and stack:
            stack.pop()
    out = []
    for kind, arg in stack:
        names = re.findall(r'\b(JO_DEBUG|JO_COMPILE_WITH_\w+)\b', arg)
        if kind == 'ifdef' or (kind == 'if' and 'defined' in arg):
            out += names
    return out


def index_of(path):
    text = open(path, encoding='utf-8', errors='replace').read().replace('\r\n', '\n')
    m = re.search(r'@file.*?@brief\s+(.*?)(?=\n\s*\*\s*@|\*/)', text, re.S)
    title = clean(re.sub(r'^\s*\*\s?', '', m.group(1), flags=re.M)) if m else ''
    # (the first sentence of the description of the module)
    title = re.split(r'(?<=[a-z0-9)])[.:] | - ', title)[0]
    if len(title) > BRIEF_MAX:
        title = title[:BRIEF_MAX].rsplit(' ', 1)[0] + '...'
    entries = []
    seen = set()
    for c in re.finditer(r'/\*\*(?!<).*?\*/', text, re.S):
        if '@file' in c.group(0):
            continue
        # (a comment inside a struct or an enum: a field)
        before = text[:c.start()]
        if before.count('{') > before.count('}'):
            continue
        d = declaration_after(text, c.end())
        if d is None:
            continue
        kind, name, signature = d
        if name.startswith('__') or name in seen:
            continue
        seen.add(name)
        cond = conditions_at(text, c.start())
        cond = [x for x in cond if not x.startswith('__')]
        entries.append((kind, name, signature, brief_of(c.group(0)), cond))
    return title, entries


def build():
    lines = ['# Jo Engine API index {#api_index}', '',
             'Generated by `tools/make_api_index.py` from `jo_engine/jo/*.h` (do not edit: run the script after a change of '
             'the headers). Each function, macro and type of the engine on one line, with its brief: **search it (grep) '
             'before writing a function**, the engine probably has it, made for the Saturn (fixed point, tables, the '
             'hardware). The full documentation of each one is its comment in the header.', '']
    total = 0
    for name in sorted(os.listdir(HEADERS)):
        if not name.endswith('.h') or name in SKIPPED:
            continue
        title, entries = index_of(os.path.join(HEADERS, name))
        if not entries:
            continue
        # (the options of the whole header in its title, the other ones on their declarations)
        common = [c for c in entries[0][4] if all(c in e[4] for e in entries)]
        lines.append('## %s: %s' % (name, title) if title else '## %s' % name)
        lines.append('')
        if common:
            lines.append('In the makefile of the game: %s.' % '; '.join(option_text(c) for c in common))
            lines.append('')
        for kind, n, signature, brief, cond in entries:
            notes = [option_text(c) for c in cond if c not in common]
            if kind == 'function' and re.search(r'\b(float|double)\b', signature):
                notes.append('float: emulated in software, slow')
            note = ' (%s)' % ', '.join(notes) if notes else ''
            lines.append('- `%s`%s: %s' % (signature, note, brief))
            total += 1
        lines.append('')
    lines[4:4] = ['%d entries.' % total, '']
    return '\n'.join(lines).rstrip('\n') + '\n'


def main():
    text = build()
    if '--check' in sys.argv[1:]:
        try:
            old = open(OUT, encoding='utf-8').read().replace('\r\n', '\n')
        except OSError:
            old = None
        if old != text:
            print('%s is not the index of the headers: run python3 tools/make_api_index.py' % os.path.normpath(OUT))
            sys.exit(1)
        return
    with open(OUT, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(text)
    print('%s: %d lines' % (os.path.normpath(OUT), text.count('\n')))


if __name__ == '__main__':
    main()
