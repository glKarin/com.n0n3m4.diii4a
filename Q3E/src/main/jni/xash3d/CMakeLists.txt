cmake_minimum_required(VERSION 2.8)

set(DIII4A True)
add_definitions(-D_DIII4A)
include_directories(${DIII4A_DEPEND_COMMON_PATH})
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif ()

option(BUILD_CS16_CLIENT "Build CS1.6 library" ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hlsdk-portable/CMakeLists.txt") # is git submodule now, so check is pull
add_subdirectory(hlsdk-portable) # Jul 18, 2025 | https://github.com/FWGS/hlsdk-portable/commit/895b28d0379f8902051d2b733029fc6a0fa9989c
else()
	message(WARNING "hlsdk-portable source code not exists")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xash3d-fwgs/CMakeLists.txt") # is git submodule now, so check is pull
add_subdirectory(xash3d-fwgs) # Jul 18, 2025 | https://github.com/FWGS/xash3d-fwgs/commit/74b196d2c362ea195967be6f139faeed18795a53
else()
	message(WARNING "xash3d-fwgs source code not exists")
endif()

# https://github.com/Velaron/cs16-client/commit/65ccebe56af34bc9086e60d83745794f76e578ac
if(BUILD_CS16_CLIENT)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cs16-client/CMakeLists.txt") # is git submodule now, so check is pull
add_subdirectory(cs16-client)
else()
	message(WARNING "cs16-client source code not exists")
endif()
endif()
