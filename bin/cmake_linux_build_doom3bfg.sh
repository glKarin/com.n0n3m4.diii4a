#!/bin/sh

# need install tools: cmake, gcc/g++ | clang/clang++, make
# need install libraries: SDL2, curl, OpenAL-soft, zlib, FFMPEG


# Setup build type: Release | Debug
BUILD_TYPE=Release
TARGET_PATH="`pwd`/Q3E/src/main/jni/doom3bfg/neo";

WORK_DIR=build

cd ${TARGET_PATH};

echo "Configure and generate GNU makefile ......";
cmake -B build -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_CXX_FLAGS="-Werror=nonnull" CMakeLists.txt;

echo "Build ${BUILD_TYPE} ......";
cmake --build ${WORK_DIR} --config ${BUILD_TYPE}

# clean: make clean;

echo "Target directory: ${TARGET_PATH} ......"

echo "Done";

exit 0;
