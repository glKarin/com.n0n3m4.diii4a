# Script to build FluidSynth for multiple Android ABIs and prepare them for distribution
# via Prefab (github.com/google/prefab)
#
# Ensure that ANDROID_NDK environment variable is set to your Android NDK location
# e.g. /Library/Android/sdk/ndk-bundle

#!/bin/bash

if [ -z "$ANDROID_NDK" ]; then
  echo "Please set ANDROID_NDK to the Android NDK folder"
  exit 1
fi

# Directories, paths and filenames
BUILD_DIR=build

CMAKE_ARGS="-H. \
  -DBUILD_SHARED_LIBS=true \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_TOOLCHAIN=clang \
  -DANDROID_STL=c++_shared \
  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=."

  function build_fluidsynth {

  ABI=$1
  MINIMUM_API_LEVEL=$2
  ABI_BUILD_DIR=build/${ABI}
  STAGING_DIR=staging

  echo "Building FluidSynth for ${ABI}"

  mkdir -p ${ABI_BUILD_DIR} ${ABI_BUILD_DIR}/${STAGING_DIR}

  cmake -B${ABI_BUILD_DIR} \
        -DANDROID_ABI=${ABI} \
        -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${STAGING_DIR}/lib/${ABI} \
        -DANDROID_PLATFORM=android-${MINIMUM_API_LEVEL}\
        ${CMAKE_ARGS}

  pushd ${ABI_BUILD_DIR}
  make -j5
  make install
  popd
}

build_fluidsynth armeabi-v7a 16
build_fluidsynth arm64-v8a 21
build_fluidsynth x86 16
build_fluidsynth x86_64 21