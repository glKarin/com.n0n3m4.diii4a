#
# LocateSDL2.cmake
#
#---------------------------------------------------------------------------
# Copyright 2018 Braden Obrzut
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#---------------------------------------------------------------------------
#
# Due to a combination of factors there's not a good unified answer to the
# qustion "How do I find SDL2 in CMake."  Ideally one just does
# find_package(SDL2) and either has the sdl2-config.cmake in the magic location
# or will point SDL2_DIR to the directory containing it.
#
# But not only are many people not aware of how this works, the SDL subprojects
# don't always follow the convention. We'd really like to use imported targets,
# we'd like to also support SDL 1.2, and we can just in general do more to make
# this seamless.

# Function converts loose variables to a modern style target
function(sdl_modernize NEW_TARGET LIBS DIRS)
	# Lets be hopeful that eventually a target will appear upstream
	if(NOT TARGET ${NEW_TARGET})
		add_library(${NEW_TARGET} INTERFACE IMPORTED)

		# Strip any extra whitespace per CMP0004
		string(STRIP "${${LIBS}}" "${LIBS}")
		string(STRIP "${${DIRS}}" "${DIRS}")

		# In CMake 3.11 this could just be target_link_libraries and target_include_directories
		set_target_properties(${NEW_TARGET} PROPERTIES
			INTERFACE_LINK_LIBRARIES "${${LIBS}}"
			INTERFACE_INCLUDE_DIRECTORIES "${${DIRS}}"
		)
	endif()
endfunction()

option(FORCE_SDL12 "Use SDL 1.2 instead of SDL2" OFF)
set(SDL_FIND_ERROR NO)
# PKG - Name of directory with vendored library
# LIB - Name of library object file
# HEADER - Name of primary header file
# INTERNAL_TARGET - Target from vendored library to alias
function(find_sdl_library PKG LIB HEADER INTERNAL_TARGET)
	string(TOUPPER "${LIB}" INTERNAL_VAR_NAME)
	string(REPLACE "SDL2" "INTERNAL_SDL" INTERNAL_VAR_NAME "${INTERNAL_VAR_NAME}")

	set(MODERN_TARGET "${LIB}::${LIB}")

	# Detect if we have vendored libraries available
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/${PKG}/CMakeLists.txt")
		option(${INTERNAL_VAR_NAME} "Force build with internal ${LIB}" OFF)
		set(HAS_${INTERNAL_VAR_NAME} ON)
	else()
		set(${INTERNAL_VAR_NAME} OFF)
		set(HAS_${INTERNAL_VAR_NAME} OFF)
	endif()

	if(NOT FORCE_SDL12 OR ${INTERNAL_VAR_NAME})
		# First try to locate an installed system library config
		if(NOT ${INTERNAL_VAR_NAME})
			find_package(${LIB} QUIET)
		endif()

		set(TARGET_TYPE "system")
		if(NOT ${LIB}_FOUND OR ${INTERNAL_VAR_NAME})
			# If we failed to find anything from the system automatically, then
			# manually search for the library files.
			if(NOT ${INTERNAL_VAR_NAME})
				find_library(${LIB}_LIBRARIES ${LIB})
				find_path(${LIB}_INCLUDE_DIRS "${HEADER}" PATH_SUFFIXES SDL2)
			endif()

			# Finally if that fails then use the vendored copy if it's available
			if(${INTERNAL_VAR_NAME} OR NOT ${LIB}_LIBRARIES OR NOT ${LIB}_INCLUDE_DIRS)
				if(HAS_${INTERNAL_VAR_NAME})
					set(ANDROID_NDK ${CMAKE_ANDROID_NDK})
					add_subdirectory(deps/${PKG} EXCLUDE_FROM_ALL)
					if(NOT TARGET "${MODERN_TARGET}")
						add_library("${MODERN_TARGET}" ALIAS ${INTERNAL_TARGET})
					endif()
					set(TARGET_TYPE "internal")

					if(ANDROID)
						# SDL requires C99 or newer for SDL_android.c
						set_target_properties(${INTERNAL_TARGET} PROPERTIES C_STANDARD 99)
					endif()
				else()
					string(TOLOWER "${LIB}" CONF_NAME)
					message(SEND_ERROR "Please set ${LIB}_DIR to the location of ${CONF_NAME}-config.cmake.")
					set(SDL_FIND_ERROR YES PARENT_SCOPE)
				endif()
			endif()
		endif()

		# Tell the user what we're using and finalize the config
		if(TARGET "${MODERN_TARGET}")
			message(STATUS "Using ${TARGET_TYPE} ${LIB}")
		else()
			message(STATUS "Using ${LIB}: ${${LIB}_LIBRARIES}, ${${LIB}_INCLUDE_DIRS}")
		endif()
	else()
		find_package(${PKG})
		string(TOUPPER "${PKG}" SDL1_VAR)

		if(NOT ${SDL1_VAR}_FOUND)
			set(SDL_FIND_ERROR YES PARENT_SCOPE)
		else()
			message(STATUS "Using system ${PKG}")
		endif()

		set(${LIB}_LIBRARIES ${${SDL1_VAR}_LIBRARY})
		set(${LIB}_INCLUDE_DIRS ${${SDL1_VAR}_INCLUDE_DIR})
	endif()

	sdl_modernize("${MODERN_TARGET}" ${LIB}_LIBRARIES ${LIB}_INCLUDE_DIRS)
endfunction()

option(INTERNAL_SDL_MIXER_CODECS "Build SDL_mixer with vendored codec libraries" OFF)

if(_DIII4A) #k: SDL
	set(SDL2::SDL2 SDL2)
	set(SDL2::SDL2_mixer SDL2_mixer)
	set(SDL2::SDL2_net SDL2_net)
else()
find_sdl_library(SDL SDL2 SDL.h SDL2-static)
find_sdl_library(SDL_mixer SDL2_mixer SDL_mixer.h SDL2_mixer)
find_sdl_library(SDL_net SDL2_net SDL_net.h SDL2_net)
endif()

if(SDL_FIND_ERROR)
	message(FATAL_ERROR "One or more required SDL libraries were not found.")
endif()
