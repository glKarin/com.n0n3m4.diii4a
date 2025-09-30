#!/bin/sh
# Q3Rally Unix Launcher

PLATFORM=`uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]'|sed -e 's/\//_/g'`
ARCH=`uname -m | sed -e s/i.86/x86/`
BUILD=release

BIN=engine/build/$BUILD-$PLATFORM-$ARCH/q3rally.$ARCH

if [ ! -f $BIN ]; then
	echo "Game binary '$BIN' not found, building it..."
	make -C engine $BUILD BUILD_GAME_SO=0 BUILD_SERVER=0
fi

# Create links to game logic qvms
if [ ! -f baseq3r/vm/ui.qvm ]; then
	DIR=`pwd`
	mkdir -p baseq3r/vm
	ln -st "$DIR/baseq3r/vm/" "$DIR/engine/build/$BUILD-$PLATFORM-$ARCH/baseq3r/vm/cgame.qvm"
	ln -st "$DIR/baseq3r/vm/" "$DIR/engine/build/$BUILD-$PLATFORM-$ARCH/baseq3r/vm/qagame.qvm"
	ln -st "$DIR/baseq3r/vm/" "$DIR/engine/build/$BUILD-$PLATFORM-$ARCH/baseq3r/vm/ui.qvm"
fi

# Run the game
./$BIN +set fs_basepath "." +set vm_game 2 +set vm_cgame 2 +set vm_ui 2 $@

