#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
SOURCE_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$SOURCE_ROOT"
mkdir -p build-macos-tests
clang++ -std=c++14 -pthread -Idoom3/neo doom3/macos/tests/atomic_probe.cpp -o build-macos-tests/atomic_probe
build-macos-tests/atomic_probe
python3 doom3/macos/tests/verify_render_state.py
python3 doom3/macos/tests/verify_gui_clip.py
python3 doom3/macos/tests/verify_hud_bars.py
python3 doom3/macos/tests/verify_startup_args.py
python3 doom3/macos/tests/verify_embedded_renderprogs.py
clang++ -std=c++17 -fsanitize=address,undefined -fno-omit-frame-pointer \
  doom3/macos/tests/retail_huffman_probe.cpp -o build-macos-tests/retail_huffman_probe
build-macos-tests/retail_huffman_probe
clang++ -std=c++17 -O1 -fobjc-arc \
  $(pkg-config --cflags sdl2 shaderc spirv-cross-c) \
  -Idoom3/neo/sys/sdl -Idoom3/neo/externlibs/include \
  doom3/macos/tests/metal_backend_probe.mm \
  doom3/neo/sys/sdl/metal_backend.mm doom3/neo/sys/sdl/metal_shader.cpp \
  $(pkg-config --libs sdl2 shaderc) \
  -lspirv-cross-msl -lspirv-cross-glsl -lspirv-cross-core \
  -framework Metal -framework QuartzCore -framework Cocoa \
  -o build-macos-tests/metal_backend_probe
MTL_DEBUG_LAYER=1 build-macos-tests/metal_backend_probe
