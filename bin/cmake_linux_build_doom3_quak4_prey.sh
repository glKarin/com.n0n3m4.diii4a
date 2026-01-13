#!/bin/sh

# need install tools: cmake, gcc/g++ | clang/clang++, make
# need install libraries: SDL2, OpenAL-soft, zlib, ALSA # curl


# Setup build type: Release | Debug
BUILD_TYPE=Release

PROJECT_PATH=doom3/neo

WORK_DIR=build

echo "Configure and generate GNU makefile ......";
cmake -B ${WORK_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} CMakeLists.txt; # -DBUILD_D3_MOD=OFF -DBUILD_Q4=OFF -DBUILD_PREY=OFF -DBUILD_Q4_MOD=OFF

echo "Build ${BUILD_TYPE} ......";
cmake --build ${WORK_DIR} --config ${BUILD_TYPE}

# clean: make clean;

TARGET_PATH="`pwd`/${WORK_DIR}/${PROJECT_PATH}";
echo "Target directory: ${TARGET_PATH} ......"

echo "Done";

exit 0;
