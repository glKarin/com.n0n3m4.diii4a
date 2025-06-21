#
# Copyright (c) 2016 Alibek Omarov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# 3.9 added captures in if(MATCHES)
cmake_minimum_required(VERSION 3.9)

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.15.0")
	cmake_policy(SET CMP0091 NEW)
endif()

# Install custom module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(VSForceXPToolchain) # Force XP toolchain for Visual Studio
include(CheckIPOSupported)

project (HLSDK-PORTABLE)

#--------------
# USER DEFINES \
################\
option(USE_VGUI "Enable VGUI1." OFF)
option(USE_NOVGUI_MOTD "Prefer non-VGUI MOTD when USE_VGUI is enabled" OFF)
option(USE_NOVGUI_SCOREBOARD "Prefer non-VGUI Scoreboard when USE_VGUI is enabled" OFF)
option(USE_VOICEMGR "Enable VOICE MANAGER." OFF)
option(BUILD_CLIENT "Build client dll" ON)
option(BUILD_SERVER "Build server dll" ON)
option(LTO "Enable interprocedural optimization" OFF)
option(POLLY "Enable pollyhedral optimization" OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4 OR
	((WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	AND (CMAKE_SYSTEM_PROCESSOR STREQUAL "x64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "X64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "X86_64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "EM64T")))
	option(64BIT "Disable auto -m32 appending to compiler flags" OFF)
else()
	option(64BIT "Disable auto -m32 appending to compiler flags" ON)
endif()

# It seems CMAKE_SYSTEM_PROCESSOR parameter completely useless for APPLE platform,
# so may need to set options here manually.
if((WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	AND (((CMAKE_SYSTEM_PROCESSOR STREQUAL "x64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "X64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "X86_64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "EM64T") AND NOT 64BIT)
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "X86"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i386"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i486"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i586"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i686"))
	option(GOLDSOURCE_SUPPORT "Build goldsource compatible client library" ON)
else()
	option(GOLDSOURCE_SUPPORT "Build goldsource compatible client library" OFF)
endif()

#-----------------
# MAIN BUILD CODE \
###################\

CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)

execute_process(COMMAND "git" "describe" "--dirty" "--always"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "git" "rev-parse" "--abbrev-ref" "HEAD"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if(GIT_VERSION)
	add_definitions(-DXASH_BUILD_COMMIT="${GIT_VERSION}")
endif()

if(GIT_BRANCH)
	add_definitions(-DXASH_BUILD_BRANCH="${GIT_BRANCH}")
endif()

file(STRINGS "mod_options.txt" MOD_OPTIONS_STRINGS REGEX "^([A-Za-z0-9_-]+)=([A-Za-z0-9_-]+)\ \#\ (.*)$")
foreach(LINE IN LISTS MOD_OPTIONS_STRINGS)
	# file() itself doesn't populate CMAKE_MATCH_<n>, so
	# reparse the string

	if(${LINE} MATCHES "^([A-Za-z0-9_-]+)=([A-Za-z0-9_-]+)\ \#\ (.*)$")
		# detect boolean options
		if(${CMAKE_MATCH_2} STREQUAL "ON" OR ${CMAKE_MATCH_2} STREQUAL "OFF")
			option(${CMAKE_MATCH_1} "${CMAKE_MATCH_3}" ${CMAKE_MATCH_2})

			# let's check it here as well
			if(${CMAKE_MATCH_1})
				message(STATUS ${CMAKE_MATCH_3} " is enabled")
				add_definitions(-D${CMAKE_MATCH_1})
			else()
				message(STATUS ${CMAKE_MATCH_3} " is disabled")
			endif()
		else()
			set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE STRING "${CMAKE_MATCH_3}")
		endif()
	endif()
endforeach()

if(HLDEMO_BUILD AND OEM_BUILD)
	message(FATAL_ERROR "Don't mix Demo and OEM builds!")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT 64BIT)
	if(MSVC)
		message(FATAL_ERROR "Add \"-D64BIT=ON\" to build 64-bit version of SDK or add \"-A Win32\" if you want to build a 32-bit version")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
	endif()
	set(CMAKE_SIZEOF_VOID_P 4)
endif()

if(64BIT AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "You enabled XASH_64BIT, but compiler can't create 64 bit code!")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	find_program(TERMUX_INFO termux-info)
	if(NOT TERMUX_INFO)
		# no-op
	else()
		# set through flags because otherwise macro won't be seen in tests
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__TERMUX__=1")
		set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -D__TERMUX__=1")
	endif()
endif()

# Xash3D FWGS Library Naming Scheme compliance
# see documentation: https://github.com/FWGS/xash3d-fwgs/blob/master/Documentation/extensions/library-naming.md
include(LibraryNaming)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Building for 64 Bit")
else()
	message(STATUS "Building for 32 Bit")
endif()

if (MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--add-stdcall-alias")
endif()

if(NOT MSVC)
	#add_compile_options(-Wempty-body) # GCC/Clang flag
	add_compile_options(-Wreturn-type) # GCC/Clang flag
else()
	add_definitions(-D_CRT_SILENCE_NONCONFORMING_TGMATH_H)
endif()

if(VITA)
	add_compile_options(-fno-use-cxa-atexit)
endif()

if(LTO)
	check_ipo_supported(RESULT HAVE_LTO OUTPUT LTO_ERROR)
	if(HAVE_LTO)
		message(STATUS "IPO / LTO enabled")
	else()
		message(STATUS "IPO / LTO not supported: ${LTO_ERROR}")
	endif()
endif()

check_include_file("tgmath.h" HAVE_TGMATH_H)
if(HAVE_TGMATH_H)
	if(NOT MSVC)
		set(CMAKE_REQUIRED_LIBRARIES "m")
	endif()
	check_c_source_compiles("#include <tgmath.h>
	const float val = 2, val2 = 3;
	int main(){ return (int)(-asin(val) + cos(val2)); }" HAVE_VALID_TGMATH_H )
	if(${HAVE_VALID_TGMATH_H})
		add_definitions(-DHAVE_TGMATH_H)
	endif()
endif()

if(BUILD_CLIENT)
	message(STATUS "Building client enabled")
	add_subdirectory(cl_dll)
endif()

if(BUILD_SERVER)
	message(STATUS "Building server enabled")
	add_subdirectory(dlls)
endif()

if(NOT BUILD_SERVER AND NOT BUILD_CLIENT)
	message(FATAL_ERROR "Nothing to build")
endif()

if(POLLY)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(-mllvm -polly)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-fgraphite-identity -floop-interchange -floop-block)
	endif()
endif()
