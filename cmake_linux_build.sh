#!/bin/sh

# need install tools: cmake, gcc/g++ | clang/clang++, make
# need install libraries: SDL2, curl, OpenAL-soft, zlib, ALSA


# Setup build type: Release | Debug
BUILD_TYPE=Release

echo "Configure and generate MSVC project ......";
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -CMakeLists.txt;

echo "Build ${BUILD_TYPE} ......";
make;

# clean: make clean;

TARGET_PATH="`pwd`/Q3E/src/main/jni/doom3/neo";
echo "Target directory: ${TARGET_PATH} ......"

echo "Done";

exit 0;
