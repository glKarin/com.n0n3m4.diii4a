#!/usr/bin/env python3
"""Verify every override hash, 4x dimensions, and byte-identical original mips."""
from contextlib import ExitStack
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('--base', type=Path, required=True)
parser.add_argument('--packs', type=Path, required=True)
args = parser.parse_args()
count = 0
formats = {}
with ExitStack() as stack:
    sources = {}
    for pack in sorted(args.packs.glob('*.pk4')):
        records = json.loads(pack.with_suffix('.json').read_text())
        with zipfile.ZipFile(pack) as archive:
            assert set(records) == set(archive.namelist()), pack
            for name, record in records.items():
                key = record['source']
                if key not in sources:
                    sources[key] = stack.enter_context(zipfile.ZipFile(args.base/key))
                original = sources[key].read(name)
                data = archive.read(name)
                assert hashlib.sha256(data).hexdigest() == record['sha256'], name
                assert hashlib.sha256(original).hexdigest() == record['source_sha256'], name
                h, w = struct.unpack_from('<II', original, 12)
                assert struct.unpack_from('<II', data, 12) == (h*4, w*4), name
                kind = original[84:88]
                block_bytes = 8 if kind == b'DXT1' else 16
                new_bytes = sum(((w*s+3)//4)*((h*s+3)//4)*block_bytes for s in (4, 2))
                assert data[128+new_bytes:] == original[128:], name
                assert data[84:88] == kind, name
                count += 1
                formats[kind.decode()] = formats.get(kind.decode(), 0)+1
result = {'verified_textures': count, 'formats': formats, 'scale': 4,
          'hashes_match': True, 'original_mips_identical': True}
if not count: raise SystemExit('No texture packs verified')
print(json.dumps(result))
