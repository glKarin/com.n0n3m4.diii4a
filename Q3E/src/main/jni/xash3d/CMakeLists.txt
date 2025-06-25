cmake_minimum_required(VERSION 2.8)

set(DIII4A True)
add_definitions(-D_DIII4A)
include_directories(${DIII4A_DEPEND_COMMON_PATH})
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif ()

option(BUILD_CS16_CLIENT "Build CS1.6 library" ON)

add_subdirectory(hlsdk-portable)
add_subdirectory(xash3d-fwgs)

if(BUILD_CS16_CLIENT)
add_subdirectory(cs16-client)
endif()
