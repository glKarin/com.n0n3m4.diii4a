#!/bin/bash
# build armv8-a 64
set -e

export NDK=/opt/ndk
export TOOLCHAIN=$NDK/android_gcc
export ARCH=aarch64-linux-android
export PATH=${TOOLCHAIN}/bin:${TOOLCHAIN}/${ARCH}/bin:${PATH}
export CXX=$ARCH-g++
export CC=$ARCH-gcc

export TARGET=${ARCH}
export API=21 # 14 # android 4.0 ice cream

LIB_PATH="${TOOLCHAIN}/${ARCH}/lib ${TOOLCHAIN}/${ARCH}/lib64 ${TOOLCHAIN}/sysroot/usr/lib"
DEFINES="-D__ANDROID_API__=${API} -I${TOOLCHAIN}/sysroot/usr/include -DD3_SIZEOFPTR=8"

	#BASEFLAGS='-march=armv7-a -fno-builtin-sin -fno-builtin-cos -fno-builtin-sinf -fno-builtin-cosf -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti -I/prog/games/dante-es2/libogg/include -L/prog/games/dante-es2/libogg/lib'\
scons \
	ARCH='arm' \
	BUILD='release' \
	\
	CC=$CC \
	CXX=$CXX \
	\
	NOCURL=0 \
	OPENSLES=1 \
	MULTITHREAD=1 \
	TARGET_ANDROID=1 \
	TARGET_GAME=1 \
	TARGET_D3XP=1 \
	TARGET_CDOOM=1 \
	TARGET_D3LE=1 \
	TARGET_RIVENSIN=1 \
	TARGET_HARDCORPS=1 \
	TARGET_RAVEN=0 \
	TARGET_QUAKE4=0 \
	TARGET_HUMANHEAD=0 \
	TARGET_PREY=0 \
	BASEFLAGS="-march=armv8-a -fno-builtin-sin -fno-builtin-cos -fno-builtin-sinf -fno-builtin-cosf -fexceptions -frtti ${DEFINES}" \
	LIB_PATH="${LIB_PATH}" \
	#$*

echo "${TOOLCHAIN} armv8-a 64";
