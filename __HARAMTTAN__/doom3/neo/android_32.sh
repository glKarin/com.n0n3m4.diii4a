#!/bin/bash
set -e

export TOOLCHAIN=/opt/ndk/android_gcc_32
export ARCH=arm-linux-androideabi
export PATH=${TOOLCHAIN}/bin:${TOOLCHAIN}/${ARCH}/bin:${PATH}
export CXX=$ARCH-g++
export CC=$ARCH-gcc
#export CXX=g++
#export CC=gcc

	#BASEFLAGS='-march=armv7-a -fno-builtin-sin -fno-builtin-cos -fno-builtin-sinf -fno-builtin-cosf -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti -I/prog/games/dante-es2/libogg/include -L/prog/games/dante-es2/libogg/lib'\
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
	BASEFLAGS='-march=armv7-a -fno-builtin-sin -fno-builtin-cos -fno-builtin-sinf -fno-builtin-cosf -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp -fexceptions -frtti -I/data/data/com.termux/files/usr/include/android'\
	$*
