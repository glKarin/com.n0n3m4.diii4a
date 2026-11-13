#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
SOURCE_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${ETQW_BUILD_DIR:-$SOURCE_ROOT/build-macos}"
PROFILE="${ETQW_PROFILE:-$HOME/Library/Application Support/idtech4amm/etqw-metal}"
if [[ $# -eq 0 || ! -d "$1/base" ]]; then
  printf 'Usage: %s GAME_DIRECTORY [engine arguments]\nGAME_DIRECTORY must contain your own ETQW 1.5 base/ folder.\n' "$0" >&2
  exit 1
fi
DATA_ROOT="$(cd "$1" && pwd)"
shift
ENGINE="${ETQW_ENGINE:-$BUILD_DIR/doom3/neo/ETQW}"
MODULE="$BUILD_DIR/doom3/neo/libetqwgame.dylib"
AUDIO="$BUILD_DIR/openal/libopenal.1.dylib"
COMPAT="$SOURCE_ROOT/Q3E/src/main/assets/pak/etqwbase/zzz_etqwbase.pk4"
for required in "$ENGINE" "$MODULE" "$AUDIO" "$DATA_ROOT/base/pak008.pk4" "$COMPAT"; do
  if [[ ! -f "$required" ]]; then
    printf 'Missing required file: %s\n' "$required" >&2
    exit 1
  fi
done
mkdir -p "$PROFILE/base"
# Engine compatibility content goes in the writable profile, never in game data.
cp "$COMPAT" "$PROFILE/base/zzz_etqwbase.pk4"
python3 - "$SOURCE_ROOT" "$PROFILE" <<'PY'
from pathlib import Path
import sys, zipfile
source, profile = map(Path, sys.argv[1:])
target = profile / 'base/zzz_mac_renderfixes.pk4'
temporary = target.with_suffix('.partial')
with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
    name = 'renderprogs/imposters.rprog'
    archive.write(source / 'doom3/etqwbase' / name, name)
temporary.replace(target)
PY
if [[ $# -eq 0 ]]; then
  set -- +set net_ip 127.0.0.1 +set net_LANServer 1 +set net_allowCheats 0 \
    +set si_rules sdGameRulesObjective +set bot_minClients 8 +spawnServer valley
fi
exec "$ENGINE" \
  +set fs_basepath "$DATA_ROOT" +set fs_savepath "$PROFILE" \
  +set harm_fs_gameLibPath "$MODULE" +set s_libOpenAL "$AUDIO" +set s_useOpenAL 1 \
  +set harm_r_openglVersion GLES3.0 +set r_metalBackend 1 +set r_useDXT 1 \
  +set r_fullscreen 0 +set r_mode -1 +set r_customWidth 1280 +set r_customHeight 720 \
  +set r_renderWidth 0 +set r_renderHeight 0 +set r_multiSamples 0 \
  +set r_multithread 0 +set harm_r_skipImposter 1 +set com_allowConsole 1 "$@"
