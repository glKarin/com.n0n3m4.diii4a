#!/bin/bash

# Show commands that are executed.
set -x

ROOT_DIR="$(cd `dirname $0`/.. && pwd)"
BOTRIX_DIR="$ROOT_DIR/botrix"
BOTRIX_LIB="$BOTRIX_DIR/build/libbotrix.so"

STEAM_DIR=$(echo `cat /usr/bin/steam | grep "STEAMCONFIG=" | sed -e "s/STEAMCONFIG=//"`)
if [ "$STEAM_DIR" == "" ]; then
    STEAM_DIR="$HOME/.steam"
fi

# Fail on error.
set -e

# Check out command line parameters.
if [ "$1" == "hl2dm" ]; then
    GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Half-Life 2 Deathmatch"
    MOD_DIR="$STEAM_DIR/steam/SteamApps/common/Half-Life 2 Deathmatch/hl2mp"
elif [ "$1" == "srcds" ]; then
    GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Source SDK Base 2013 Dedicated Server"
    MOD_DIR="$STEAM_DIR/steam/SteamApps/common/Half-Life 2 Deathmatch/hl2mp"
else
    GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Source SDK Base 2013 Multiplayer"
    MOD_DIR="$ROOT_DIR/source-sdk-2013/mp/game/mod_hl2mp"
fi

# Steam is i386 (32bit).
STEAM_RUNTIME="$STEAM_DIR/ubuntu12_32/steam-runtime"

export LD_LIBRARY_PATH="$GAME_DIR/bin:$MOD_DIR/bin:$STEAM_RUNTIME/i386/lib/i386-linux-gnu:$STEAM_RUNTIME/i386/lib:$STEAM_RUNTIME/i386/usr/lib/i386-linux-gnu:$STEAM_RUNTIME/i386/usr/lib:$STEAM_RUNTIME/amd64/lib/x86_64-linux-gnu:$STEAM_RUNTIME/amd64/lib:$STEAM_RUNTIME/amd64/usr/lib/x86_64-linux-gnu:$STEAM_RUNTIME/amd64/usr/lib:$LD_LIBRARY_PATH"

# Prepare for launch: copy addons, libbotrix.so and botrix.cfg.
cp -r "$BOTRIX_DIR/runtime/linux"/* "$MOD_DIR/"
cp "$BOTRIX_LIB" "$MOD_DIR/addons/"

# Launch the game.
cd "$GAME_DIR"
if [ "$1" == "srcds" ]; then
    ./srcds_linux -game "$MOD_DIR" -debug
else
    ./hl2_linux -insecure -allowdebug -novid -console -game "$MOD_DIR"
fi

