#!/bin/sh
# Q3Rally (debug) Unix Launcher

PLATFORM=`uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]'|sed -e 's/\//_/g'`
ARCH=`uname -m | sed -e s/i.86/x86/`
BUILD=debug

BIN=engine/build/$BUILD-$PLATFORM-$ARCH/q3rally.$ARCH

if [ ! -f $BIN ]; then
	echo "Game binary '$BIN' not found, building it..."
	make -C engine $BUILD BUILD_GAME_QVM=0 BUILD_SERVER=0
fi

# Create links to game logic natives
if [ ! -f baseq3r/ui$ARCH.so ]; then
	DIR=`pwd`
	ln -st "$DIR/baseq3r/" "$DIR/engine/build/$BUILD-$PLATFORM-$ARCH/baseq3r/cgame$ARCH.so"
	ln -st "$DIR/baseq3r/" "$DIR/engine/build/$BUILD-$PLATFORM-$ARCH/baseq3r/qagame$ARCH.so"
	ln -st "$DIR/baseq3r/" "$DIR/engine/build/$BUILD-$PLATFORM-$ARCH/baseq3r/ui$ARCH.so"
fi

# Run the game
./$BIN +set fs_basepath "." +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 $@

