#!/bin/bash
# build hard-float vfp
set -e

export NDK=/opt/ndk
export TOOLCHAIN=$NDK/android_4_0_gcc_32$1
export ARCH=arm-linux-androideabi
export PATH=${TOOLCHAIN}/bin:${TOOLCHAIN}/${ARCH}/bin:${PATH}
export CXX=$ARCH-g++
export CC=$ARCH-gcc

export TARGET=${ARCH}
export API=14 # android 4.0 ice cream

LIB_PATH="-L${NDK}/arm-linux-androideabi/lib/armv7-a -L${NDK}/sysroot/usr/lib"
DEFINES="-D__ANDROID_API__=${API}"

	#BASEFLAGS='-march=armv5te -fno-builtin-sin -fno-builtin-sinf -fno-builtin-cosf -fno-builtin-cos -mtune=xscale -mfpu=vfp -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti -I/prog/games/dante-es2/libogg/include -L/prog/games/dante-es2/libogg/lib'\
scons \
	ARCH='arm' \
	BUILD='release' \
	\
	CC=$CC \
	CXX=$CXX \
	\
	NOCURL=1 \
	TARGET_ANDROID=1 \
	TARGET_D3XP=1 \
	TARGET_CDOOM=1 \
	TARGET_D3XP=1 \
	BASEFLAGS="-march=armv5te -fno-builtin-sin -fno-builtin-sinf -fno-builtin-cosf -fno-builtin-cos -mtune=xscale -mfpu=vfp -mfloat-abi=softfp -Wl,--no-undefined -fexceptions -frtti ${DEFINES}"\

	#$*

echo "${TOOLCHAIN} fpu=vfp";
