#!/usr/bin/env python3
"""Embed the repository's renderer translations without replacing retail declarations."""
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import json
import re
import sys


def without_comments(text):
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/', re.S)
    return tokens.sub(lambda match: re.sub(r'[^\n]', ' ', match[0])
                      if match[0].startswith('/') else match[0], text)


def program_bodies(text, kind='renderProgram'):
    # Skip comments and strings when counting braces (including GLSL bodies).
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|[{}]', re.S)
    cursor = 0
    declaration = re.compile(r'\b' + kind + r'\s+([^\s{]+)\s*\{')
    searchable = without_comments(text)
    while match := declaration.search(searchable, cursor):
        depth = 1
        for token in tokens.finditer(text, match.end()):
            if token[0] == '{':
                depth += 1
            elif token[0] == '}':
                depth -= 1
            if depth == 0:
                yield match[1], text[match.start():token.end()]
                cursor = token.end()
                break
        else:
            raise ValueError(f'Unclosed program {match[1]}')


def generate(root, output):
    def expand(text, source, stack):
        def include(match):
            name = match[1]
            candidate = source.parent / name
            if not candidate.is_file():
                candidate = root / name
            candidate = candidate.resolve()
            if not candidate.is_relative_to(root.resolve()) or candidate in stack:
                raise ValueError(f'Invalid shader include: {name}')
            return expand(candidate.read_text(), candidate, stack + (candidate,))
        return re.sub(r'\$include\s+"([^"]+)"', include, without_comments(text))

    programs = {}
    templates = {}
    for source in sorted(root.rglob('*.rprog')):
        for name, body in program_bodies(source.read_text(), 'template'):
            if name.lower() in templates:
                raise ValueError(f'Duplicate template {name}')
            templates[name.lower()] = expand(body, source, (source.resolve(),))
        for name, body in program_bodies(source.read_text()):
            if name.lower() in programs:
                raise ValueError(f'Duplicate program {name}')
            try:
                programs[name.lower()] = expand(body, source, (source.resolve(),))
            except FileNotFoundError as error:
                # Some upstream declarations are incomplete. Keep their existing
                # runtime fallback instead of embedding a broken translation.
                print(f'Skipping incomplete renderer translation {name}: {Path(error.filename).name}')
    lines = ['// Generated from doom3/etqwbase/renderprogs; do not edit.',
             '// Existing shader sources retain their upstream license.',
             'static const struct { const char *name; const char *source; bool isTemplate; } metalRenderPrograms[] = {']
    for entries, is_template in ((programs, False), (templates, True)):
        for name, body in sorted(entries.items()):
            # The shared engine still builds as C++98: use adjacent escaped strings.
            lines.append('{' + json.dumps(name) + ',')
            lines.extend(json.dumps(line) for line in body.splitlines(keepends=True))
            lines.append(', ' + str(is_template).lower() + '},')
    lines.append('};\n')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines))
    print(f'Embedded {len(programs)} renderer translations')


if __name__ == '__main__':
    generate(Path(sys.argv[1]), Path(sys.argv[2]))
