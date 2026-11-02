#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise shader declaration extraction and safe recursive include expansion."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile

script = Path(__file__).resolve().parents[1] / 'tools/embed_renderprogs.py'
spec = importlib.util.spec_from_file_location('embed_renderprogs', script)
embed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(embed)

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp) / 'renderprogs'
    root.mkdir()
    (root / 'valid.rprog').write_text('''
// renderProgram commented { }
/* renderProgram alsoCommented { } */
renderProgram fixture {
  // $include "does-not-exist.inc"
  program vertex glsl { $include "vertex.inc" }
}
renderProgram incomplete { $include "missing.inc" }
template fixtureTemplate { text { <% vec4 tint = vec4(1.0); %> } }
''')
    (root / 'vertex.inc').write_text('void main() { /* } */ gl_Position = vec4(1.0); }')
    output = Path(temp) / 'programs.h'
    embed.generate(root, output)
    text = output.read_text()
    assert 'fixture' in text and 'gl_Position' in text
    assert '$include' not in text and 'incomplete' not in text and 'Commented' not in text
    assert 'fixturetemplate' in text and ', true}' in text
    source = Path(temp) / 'probe.cpp'
    source.write_text('#include "programs.h"\nint main() { return sizeof(metalRenderPrograms) / sizeof(metalRenderPrograms[0]) != 2 || metalRenderPrograms[0].isTemplate || !metalRenderPrograms[1].isTemplate; }\n')
    binary = Path(temp) / 'probe'
    subprocess.run(['clang++', '-std=c++98', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    for include in ('../outside.inc', 'vertex.inc'):
        (root / 'vertex.inc').write_text(f'$include "{include}"')
        try:
            embed.generate(root, output)
        except ValueError:
            pass
        else:
            raise AssertionError('Include escape or recursion was accepted')
print('Embedded shader extraction, C++98 strings and include bounds passed')
