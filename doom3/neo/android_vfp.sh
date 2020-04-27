#!/bin/bash
set -e

export PATH=/prog/toolchain/android-toolchain-eabi/bin:$PATH
export ARCH=arm-linux-androideabi
export CXX=$ARCH-g++
export CC=$ARCH-gcc

scons \
	ARCH='arm' \
	BUILD='release' \
	\
	CC=$CC \
	CXX=$CXX \
	\
	NOCURL=1 \
	TARGET_ANDROID=1 \
	TARGET_D3XP=0 \
	BASEFLAGS='-march=armv5te -fno-builtin-sin -fno-builtin-sinf -fno-builtin-cosf -fno-builtin-cos -mtune=xscale -mfpu=vfp -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti -I/prog/games/dante-es2/libogg/include -L/prog/games/dante-es2/libogg/lib'\
	$*
