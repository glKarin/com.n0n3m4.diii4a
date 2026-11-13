#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
SOURCE_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${ETQW_BUILD_DIR:-$SOURCE_ROOT/build-macos}"
PROFILE="${ETQW_PROFILE:-$HOME/Library/Application Support/idtech4amm/etqw-retail}"
if [[ $# -lt 2 || ! -d "$1/base" ]]; then
  printf 'Usage: %s GAME_DIRECTORY HOST:PORT [engine arguments]\n' "$0" >&2
  exit 1
fi
DATA_ROOT="$(cd "$1" && pwd)"
SERVER="$2"
shift 2
ENGINE="${ETQW_ENGINE:-$BUILD_DIR/doom3/neo/ETQW}"
MODULE="$BUILD_DIR/doom3/neo/libetqwgame.dylib"
AUDIO="$BUILD_DIR/openal/libopenal.1.dylib"
for required in "$ENGINE" "$MODULE" "$AUDIO" "$DATA_ROOT/base/pak008.pk4" "$DATA_ROOT/base/game001.pk4"; do
  if [[ ! -f "$required" ]]; then
    printf 'Missing required file: %s\n' "$required" >&2
    exit 1
  fi
done
mkdir -p "$PROFILE/base"
# Use a separate profile: pure retail servers require the original archives.
# In particular, do not install the local compatibility/HD overrides here.
exec "$ENGINE" \
  +set fs_basepath "$DATA_ROOT" +set fs_savepath "$PROFILE" +set fs_userpath "$PROFILE" \
  +set harm_fs_gameLibPath "$MODULE" +set s_libOpenAL "$AUDIO" +set s_useOpenAL 1 \
  +set harm_r_openglVersion GLES3.0 +set r_metalBackend 1 +set r_useDXT 1 \
  +set image_usePrecompressedTextures 1 +set image_useCompression 1 \
  +set r_fullscreen 0 +set r_mode -1 +set r_customWidth 1280 +set r_customHeight 720 \
  +set r_renderWidth 0 +set r_renderHeight 0 +set r_multiSamples 0 \
  +set r_multithread 0 +set harm_r_skipImposter 1 +set com_allowConsole 1 \
  +set net_ip 0.0.0.0 +set net_LANServer 0 +set net_serverDedicated 0 \
  +set net_retailProtocol 1 "$@" +connect "$SERVER"
