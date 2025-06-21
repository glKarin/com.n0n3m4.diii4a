cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-MultiEmulator)

file(GLOB FWGS_MULTIEMULATOR_INCLUDES include/*.h)
file(GLOB FWGS_MULTIEMULATOR_SOURCES src/*.cpp)
add_library (fwgs_MultiEmulator STATIC ${FWGS_MULTIEMULATOR_SOURCES})
target_include_directories(fwgs_MultiEmulator PRIVATE include)
