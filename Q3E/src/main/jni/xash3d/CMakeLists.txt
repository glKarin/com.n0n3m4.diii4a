cmake_minimum_required(VERSION 2.8)

set(DIII4A True)
add_definitions(-D_DIII4A)
include_directories(${DIII4A_DEPEND_COMMON_PATH})
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif ()

option(BUILD_CS16_CLIENT "Build CS1.6 library" ON)

add_subdirectory(hlsdk-portable) # Jul 18, 2025 | 343b09bc4de15ecf310ab97e759bfdef6e883bd8 | Copy the latest character.
add_subdirectory(xash3d-fwgs) # Jul 18, 2025 | eeca965b5d65d8b43d15096cac7fa5803559713b | 3rdparty: update libbacktrace and mainui submodules
if(BUILD_CS16_CLIENT)
add_subdirectory(cs16-client)
endif()
