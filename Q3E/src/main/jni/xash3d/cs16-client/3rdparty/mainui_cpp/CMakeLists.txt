#
# Copyright (c) 2015 Pavlo Lavrenenko
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

cmake_minimum_required(VERSION 3.5)
project(cs16_MAINUI)

set(CMAKE_CXX_STANDARD 11)

set(MAINUI_LIBRARY menu_cs)

option(MAINUI_USE_CUSTOM_FONT_RENDER "Use custom font rendering" ON)
option(MAINUI_USE_STB "Use stb_truetype.h for rendering(*nix-only)" OFF)
option(MAINUI_FONT_SCALE "Scale fonts by height" OFF)

if(NOT XASH_SDK)
	set(XASH_SDK "sdk_includes/")
endif()

file(GLOB MAINUI_CONTROLS_SOURCES "controls/*.cpp")
file(GLOB MAINUI_MENUS_SOURCES "menus/*.cpp" "menus/dynamic/*.cpp")
file(GLOB MAINUI_FONT_RENDER_SOURCES "font/*.cpp")
file(GLOB MAINUI_SOURCES "miniutl/*.cpp" "*.cpp")

add_library(${MAINUI_LIBRARY} SHARED ${MAINUI_CONTROLS_SOURCES} ${MAINUI_MENUS_SOURCES} ${MAINUI_FONT_RENDER_SOURCES} ${MAINUI_SOURCES})

if(NOT WIN32 AND NOT MINGW)
	add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-macro-redefined)
endif()

if(MAINUI_FONT_SCALE)
	add_definitions(-DMAINUI_FONT_SCALE)
endif()

add_definitions(-DSTDINT_H=<cstdint>)

# Android uses stb_truetype by default for rendering fonts
if(ANDROID)
	set(MAINUI_USE_STB TRUE)
endif()

include_directories(${XASH_SDK}/common ${XASH_SDK}/engine ${XASH_SDK}/pm_shared ${XASH_SDK}/public . controls/ menus/ miniutl/ font/ model/)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Font Rendering(FreeType or WinAPI)
if(MAINUI_USE_CUSTOM_FONT_RENDER)
	# Win32 will always use GDI font renderer
	if(NOT WIN32)
		if(MAINUI_USE_STB)
			# Use stbtt
			add_definitions(-DMAINUI_USE_STB)
		else()
			# Use freetype2
			find_package(PkgConfig)
			pkg_check_modules(FT2 REQUIRED freetype2)
			include_directories(${FT2_INCLUDE_DIRS})
			target_link_libraries(${MAINUI_LIBRARY} ${FT2_LIBRARIES})
			add_definitions(-DMAINUI_USE_FREETYPE)
		endif()
	endif()

	add_definitions(-DMAINUI_USE_CUSTOM_FONT_RENDER)
endif()

install(TARGETS ${MAINUI_LIBRARY} DESTINATION "${GAME_DIR}/${CLIENT_INSTALL_DIR}/")

if(MSVC)
	install(FILES $<TARGET_PDB_FILE:${MAINUI_LIBRARY}> DESTINATION "${GAME_DIR}/${CLIENT_INSTALL_DIR}/" OPTIONAL)
endif()
