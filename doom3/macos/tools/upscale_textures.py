#!/usr/bin/env python3
"""Build local 4x DDS overrides; preserve original mip levels, alpha and BC5 normals.

Uses Lanczos reconstruction, not generative detail. The .mega virtual terrain
is already 32768 pixels wide and is handled separately by the renderer.
Requires Pillow >= 12 and numpy. No source archives are changed.
"""
import argparse
from concurrent.futures import ProcessPoolExecutor
import hashlib
import io
import json
from pathlib import Path
import struct
import time
import zipfile

import numpy as np
from PIL import Image, ImageFilter



def compress(image, kind):
    stream = io.BytesIO()
    image.save(stream, format='DDS', pixel_format='BC5' if kind == b'ATI2' else kind.decode())
    return stream.getvalue()[148 if kind == b'ATI2' else 128:]


def convert(item):
    name, archive = item
    with zipfile.ZipFile(archive) as z:
        original = z.read(name)
    height, width = struct.unpack_from('<II', original, 12)
    mips = max(1, struct.unpack_from('<I', original, 28)[0])
    caps2 = struct.unpack_from('<I', original, 112)[0]
    kind = original[84:88]
    if caps2 or kind not in (b'DXT1', b'DXT5', b'ATI2'):
        raise ValueError(f'Unsupported DDS: {name}, {kind}, caps2={caps2}')
    image = Image.open(io.BytesIO(original))
    image.load()
    if kind == b'ATI2':
        # BC5 stores signed normal X/Y encoded in [0,255]. Reconstruct Z,
        # interpolate all three components and normalize before encoding XY.
        xy = np.asarray(image.convert('RGB'), dtype=np.float32)[:, :, :2] / 127.5 - 1
        z = np.sqrt(np.maximum(0, 1 - np.sum(xy * xy, axis=2)))
        normal = np.dstack((xy, z))
        channels = [np.asarray(Image.fromarray(normal[:, :, i]).resize((width*4, height*4), Image.Resampling.BILINEAR)) for i in range(3)]
        normal = np.stack(channels, axis=2)
        normal /= np.maximum(np.linalg.norm(normal, axis=2, keepdims=True), 1e-8)
        large = Image.fromarray(np.clip((normal + 1) * 127.5, 0, 255).astype(np.uint8))
    else:
        mode = 'RGBA' if image.mode == 'RGBA' or kind == b'DXT5' else 'RGB'
        large = image.convert(mode).resize((width*4, height*4), Image.Resampling.LANCZOS)
        if mode == 'RGBA':
            alpha = large.getchannel('A')
            large = large.convert('RGB').filter(ImageFilter.UnsharpMask(radius=1.2, percent=35, threshold=3))
            large.putalpha(alpha)
        else:
            large = large.filter(ImageFilter.UnsharpMask(radius=1.2, percent=35, threshold=3))
    medium = large.resize((width*2, height*2), Image.Resampling.BOX)
    top = compress(large, kind)
    mid = compress(medium, kind)
    header = bytearray(original[:128])
    struct.pack_into('<II', header, 12, height*4, width*4)
    struct.pack_into('<I', header, 20, len(top))
    struct.pack_into('<I', header, 28, mips+2)
    struct.pack_into('<I', header, 8, struct.unpack_from('<I', header, 8)[0] | 0x20000 | 0x80000)
    struct.pack_into('<I', header, 108, struct.unpack_from('<I', header, 108)[0] | 0x400008)
    output = bytes(header) + top + mid + original[128:]
    # Decoder verifies the new base level, while the original mips are exact.
    check = Image.open(io.BytesIO(output))
    if check.size != (width*4, height*4):
        raise ValueError(name)
    return name, output, {'source': Path(archive).name, 'size': [width, height],
                          'output_size': [width*4, height*4], 'format': kind.decode(),
                          'source_sha256': hashlib.sha256(original).hexdigest(),
                          'sha256': hashlib.sha256(output).hexdigest()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base', type=Path, required=True, help='Original ETQW base directory')
    parser.add_argument('--output', type=Path, required=True, help='Separate output directory for local overrides')
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--limit', type=int, default=0)
    args = parser.parse_args()
    if args.workers < 1 or args.limit < 0:
        parser.error('workers must be positive and limit cannot be negative')
    base = args.base.resolve()
    if not base.is_dir(): parser.error('base must be a directory')
    if args.output.resolve() == base: parser.error('output must be separate from the original game data')
    sources = {}
    for path in sorted(base.glob('*.pk4')):
        if path.name.startswith('zzz_mac_hd4x_'):
            continue
        with zipfile.ZipFile(path) as z:
            for name in z.namelist():
                if name.lower().endswith('.dds'):
                    sources[name] = str(path)
    items = sorted(sources.items())
    if args.limit:
        items = items[:args.limit]
    if not items: parser.error("No DDS textures found in input PK4 files")
    destination = args.output.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    start = time.monotonic()
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        for index in range(0, len(items), 128):
            group = items[index:index+128]
            output = destination / f'zzz_mac_hd4x_{index//128:03}.pk4'
            manifest_path = output.with_suffix('.json')
            if output.exists() and manifest_path.exists():
                existing = json.loads(manifest_path.read_text())
                if set(existing) != {name for name, _ in group}:
                    raise ValueError(f'Source list changed for {output}; use a fresh output directory')
                print(f'Already built {output.name}', flush=True)
                continue
            partial = output.with_suffix('.partial')
            records = {}
            with zipfile.ZipFile(partial, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=1) as z:
                # Bounded batches keep large atlases from accumulating in RAM.
                for batch in range(0, len(group), args.workers):
                    for name, data, record in pool.map(convert, group[batch:batch+args.workers]):
                        z.writestr(name, data)
                        records[name] = record
                    print(f'{index+len(records)}/{len(items)} textures; {time.monotonic()-start:.0f}s', flush=True)
            partial.replace(output)
            manifest_path.write_text(json.dumps(records, indent=2))
    print(f'Complete: {len(items)} textures. Packs in {destination}', flush=True)


if __name__ == '__main__':
    main()
