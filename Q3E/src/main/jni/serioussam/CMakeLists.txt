cmake_minimum_required(VERSION 3.7)
project(SeriousEngine)

set(DIII4A TRUE)
if (DIII4A)
    add_definitions(-D_DIII4A) # Android
    add_definitions(-D_GLES) # OpenGLES 1.1
    option(STATICALLY_LINKED "static link" ON) # build a single so
    if (STATICALLY_LINKED)
        add_definitions(-DSTATICALLY_LINKED)
    endif ()
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "RelWithDebInfo")
    endif ()
    set(CROSS_COMPILE TRUE) # don't call ecc, bison, flex if cross compile
    if(CMAKE_CROSSCOMPILING)
        set(CROSS_COMPILE TRUE)
    endif()
    add_compile_options(-fsigned-char)
    add_compile_options(-Wno-register -Wno-enum-constexpr-conversion)

    # override subdirectory
    option(USE_ASM "Use ASM code" FALSE)
    option(USE_SYSTEM_SDL2 "Use systems sdl2 development files" OFF)
    option(BUILD_DEDICATED_SERVER "Compile the dedicated server, too" FALSE)
    option(BUILD_MAKEFONT "Compile the MakeFONT, too" FALSE)
    option(BUILD_TEXConv "Compile the TEXConv, too" FALSE)

    #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set(DIII4A_DEPEND_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../deplibs")
    set(DIII4A_DEPEND_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../depincs")
    set(DIII4A_DEPEND_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../common")

    include_directories(${DIII4A_DEPEND_COMMON_PATH})
endif ()
# Set @rpath for Mac OS X shared library install names.

# Use system SDL2 is on by default
option(USE_SYSTEM_SDL2 "Use systems sdl2 development files" On)
option(USE_SYSTEM_ZLIB "Use systems zlib development files" On)
option(USE_CCACHE "Set to ON to use ccache if present in the system" ${USE_CCACHE})
option(USE_SYSTEM_INSTALL "Install to systems directories" Off)

# fallback for cmake versions without add_compile_options # RAKE! Borrowed from dhewm3 project
if(NOT COMMAND add_compile_options)
	function(add_compile_options)
		foreach(arg ${ARGN})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${arg}" PARENT_SCOPE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${arg}" PARENT_SCOPE)
		endforeach()
	endfunction()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CheckCXXCompilerFlag)

# SeriousSam expects the libs to be in Debug/
if(NOT DIII4A)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Debug)
endif()

if(USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif()
endif()


# Use systemwide SDL2 or custom build
# RAKE!: Find a way to use their custom built library if
# they want to use that instead or if their system only
# allows for a setup like this. Maybe use a SDL2_DIR var or
# some thing set in the system enviroment.
if(NOT USE_SYSTEM_SDL2)
    include_directories(${CMAKE_SOURCE_DIR}/External/SDL2)
else()
    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
	include_directories(${SDL2_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Error USE_SYSTEM_SDL2 is set but neccessary developer files are missing")
    endif()
endif()

if(USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
    if(ZLIB_FOUND)
        include_directories(${ZLIB_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Error! USE_SYSTEM_ZLIB is set but neccessary developer files are missing")
    endif()
endif()


# Set up some sanity stuff...
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME MATCHES "GNU")
    set(LINUX TRUE)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(FREEBSD TRUE)
endif()
if(APPLE)
    set(MACOSX TRUE)
endif()
if(MSVC)
    set(WINDOWS TRUE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()
set(DEBUG FALSE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG TRUE)
endif()

if(LINUX)
    set(CMAKE_OS_NAME "GNU/Linux" CACHE STRING "Operating system name" FORCE)
    ## Check for Debian GNU/Linux
    find_file(DEBIAN_FOUND debian_version debconf.conf
        PATHS /etc
    )
    if(DEBIAN_FOUND)
        set(CMAKE_OS_NAME "Debian" CACHE STRING "Operating system name" FORCE)
    endif(DEBIAN_FOUND)
    ##  Check for Fedora
    find_file(FEDORA_FOUND fedora-release
        PATHS /etc
    )
    if(FEDORA_FOUND)
        set(CMAKE_OS_NAME "Fedora" CACHE STRING "Operating system name" FORCE)
    endif(FEDORA_FOUND)
    ##  Check for RedHat
    find_file(REDHAT_FOUND redhat-release inittab.RH
        PATHS /etc
    )
    if(REDHAT_FOUND)
        set(CMAKE_OS_NAME "RedHat" CACHE STRING "Operating system name" FORCE)
    endif(REDHAT_FOUND)
    ##  Check for Gentoo
    find_file(OS_Release_FOUND os-release
        PATHS /usr/lib
    )
    if (OS_Release_FOUND)
        ## Scan contents of file
        file(STRINGS ${OS_Release_FOUND} Gentoo_FOUND
            REGEX Gentoo
        )
        ## Check result of string search
        if(Gentoo_FOUND)
            set(CMAKE_OS_NAME "Gentoo" CACHE STRING "Operating system name" FORCE)
            set(DEBIAN_FOUND FALSE)
        endif(Gentoo_FOUND)
    endif(OS_Release_FOUND)
    ## Extra check for Ubuntu    
    if(DEBIAN_FOUND)
      ## At its core Ubuntu is a Debian system, with
      ## a slightly altered configuration; hence from
      ## a first superficial inspection a system will
      ## be considered as Debian, which signifies an
      ## extra check is required.
      find_file(UBUNTU_EXTRA legal issue
        PATHS /etc
      )
      if(UBUNTU_EXTRA)
        ## Scan contents of file
        file(STRINGS ${UBUNTU_EXTRA} UBUNTU_FOUND
          REGEX Ubuntu
        )
        ## Check result of string search
        if(UBUNTU_FOUND)
          set(CMAKE_OS_NAME "Ubuntu" CACHE STRING "Operating system name" FORCE)
          set(DEBIAN_FOUND FALSE)
        endif(UBUNTU_FOUND)
      endif(UBUNTU_EXTRA)      
    endif(DEBIAN_FOUND)
endif(LINUX)

if(GAME_SAMTFE)
add_subdirectory(SamTFE/Sources)
endif()
if(GAME_SAMTSE)
add_subdirectory(SamTSE/Sources)
endif()
