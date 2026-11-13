#!/usr/bin/env python3
"""Compile the real codec in isolation and verify Valley luminance residuals."""
from pathlib import Path
import subprocess
import argparse
ROOT = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument('mega', type=Path, help='Path to your own valley_lit.mega')
args = parser.parse_args()
(ROOT/'build-macos-tests').mkdir(exist_ok=True)
p = ROOT / 'doom3/neo/splashdamage/renderer/megatexture'
prefix = '#include <cmath>\n#include <cstring>\n#include <algorithm>\n#include <cstdlib>\ntypedef unsigned char byte;\nstruct idMath {static int ClampInt(int a,int b,int v){return std::max(a,std::min(b,v));} static int FtoiFast(float a){return int(a);} static float Fabs(float a){return fabsf(a);} static int Abs(int a){return abs(a);}};\nconst int MEGA_TEXTURE_TILE_SIZE=128, MEGA_TEXTURE_MIN_MIP_SIZE=4;\n'
body = '\n'.join(line for line in (p/'MegaTextureCodec.cpp').read_text().splitlines() if not line.startswith(('#include', '#pragma')))
source = ROOT/'build-macos-tests/mega_codec_probe.cpp'
source.write_text('\n'.join([prefix, (p/'MegaTextureCodec.h').read_text(), body, (Path(__file__).with_name('mega_codec_probe.cpp')).read_text()]))
binary = ROOT/'build-macos-tests/mega_codec_probe'
subprocess.run(['clang++', '-std=c++17', '-O2', str(source), '-o', str(binary)], check=True)
result = subprocess.run([str(binary), str(args.mega)], check=True, capture_output=True, text=True)
print(result.stdout, end='')
