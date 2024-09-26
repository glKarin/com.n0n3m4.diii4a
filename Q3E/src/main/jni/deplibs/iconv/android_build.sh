#!/bin/sh
# fftw3/build.sh
# Compiles fftw3 for Android
# Make sure you have NDK_ROOT defined in .bashrc or .bash_profile
#export
  #PATH="$NDK/android-ndk-r11c/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin:$PATH"
#export SYS_ROOT="$NDK/android-ndk-r11c/platforms/android-21/arch-arm/"
export CC="arm-linux-androideabi-gcc --sysroot=$SYSROOT"
export LD="arm-linux-androideabi-ld"
export AR="arm-linux-androideabi-ar"
export RANLIB="arm-linux-androideabi-ranlib"
export STRIP="arm-linux-androideabi-strip"
export CPP="arm-linux-androideabi-gcc -E"

cd main/cpp

sh ./configure --prefix="/Users/vnetoo/AndroidStudioProjects/VLiveAdpaterVersion/vliveSDK/src/main/cpp/yuv_convert/libs" \
--host=arm-linux-eabi CPPFLAGS="-I${SYSROOT}/usr/include" CFLAGS="--sysroot $SYSROOT" \
LDFLAGS="-Wl,-rpath-link=$NDK/platforms/android-21/arch-arm/usr/lib/ -L$NDK/platforms/android-21/arch-arm/usr/lib/" LIBS="-lc"

make


make install