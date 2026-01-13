#!/bin/bash
# build hard-float vfp armv5
set -e

export NDK=/opt/ndk
export TOOLCHAIN=$NDK/android_4_0_gcc_32_v5
export ARCH=arm-linux-androideabi
export PATH=${TOOLCHAIN}/bin:${TOOLCHAIN}/${ARCH}/bin:${PATH}
export CXX=$ARCH-g++
export CC=$ARCH-gcc

export TARGET=${ARCH}
export API=14 # android 4.0 ice cream

LIB_PATH="${TOOLCHAIN}/arm-linux-androideabi/lib ${TOOLCHAIN}/sysroot/usr/lib"
DEFINES="-D__ANDROID_API__=${API} -DD3_SIZEOFPTR=4"

	#BASEFLAGS='-march=armv5te -fno-builtin-sin -fno-builtin-sinf -fno-builtin-cosf -fno-builtin-cos -mtune=xscale -mfpu=vfp -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti -I/prog/games/dante-es2/libogg/include -L/prog/games/dante-es2/libogg/lib'\
scons \
	ARCH='arm' \
	BUILD='release' \
	\
	CC=$CC \
	CXX=$CXX \
	\
	NOCURL=0 \
	TARGET_ANDROID=1 \
	TARGET_D3XP=1 \
	TARGET_CDOOM=1 \
	TARGET_D3XP=1 \
	TARGET_RIVENSIN=1 \
	TARGET_HARDCORPS=1 \
	BASEFLAGS="-march=armv5te -fno-builtin-sin -fno-builtin-sinf -fno-builtin-cosf -fno-builtin-cos -mtune=xscale -mfpu=vfp -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti ${DEFINES}" \
	LIB_PATH="${LIB_PATH}" \
	#$*

echo "${TOOLCHAIN} armv5 fpu=vfp";
