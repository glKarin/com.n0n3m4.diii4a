cmake_minimum_required(VERSION 3.7)
project(SeriousEngineTSE)

set(SAM_PREFIX "SamTSE_")

if(DIII4A)
    add_subdirectory(External/libogg)
    include_directories(${SAMTSE_OGG_INCLUDE_DIR})
    add_subdirectory(External/libvorbis)

    set(ANDROID_SOUND_IMPL "oboe")
    if(ANDROID_SOUND_IMPL STREQUAL "oboe")
        set(ANDROID_SOUND_LIB oboeaudio)
        add_definitions(-D_ANDROID_SOUND_OBOE)
    else()
        set(ANDROID_SOUND_LIB OpenSLES)
        add_definitions(-D_ANDROID_SOUND_OPENSL)
    endif()
endif()

# set minimal standart
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set @rpath for Mac OS X shared library install names.
#cmake_policy(SET CMP0042 NEW)

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
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/External/SDL2)
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
else()
    add_definitions(-DSE1_ZLIB=1)
endif()

option(USE_ASM "Use ASM code" TRUE)
if (USE_ASM)
    MESSAGE(STATUS "Using assembler code (when available)")
else()
    add_definitions(-DUSE_PORTABLE_C=1)
    MESSAGE(STATUS "Using portable C instead of all ASM")
endif()

################################################################
##### Serious Sam Classic The Second Encounter definitions #####
#####                                                      #####
################################################################

set(INTERNAL_NAME "serioussamse")

###############################################################
###############################################################
###############################################################

# Define Target install dir
string(FIND "${CMAKE_SOURCE_DIR}" "SamTFE/Sources" Found_SamTFE)
string(FIND "${CMAKE_SOURCE_DIR}" "SamTSE/Sources" Found_SamTSE)
if(("${Found_SamTFE}" EQUAL "-1") AND ("${Found_SamTSE}" EQUAL "-1"))
  set(Found_Sam "0")
else()
  set(Found_Sam "10")
endif()

if("${Found_Sam}" EQUAL "0")
  if(INTERNAL_NAME STREQUAL "serioussam")
    set(CMAKE_ADD_TARGET_DIR "${CMAKE_SOURCE_DIR}/SamTFE/Sources")
  else()
    set(CMAKE_ADD_TARGET_DIR "${CMAKE_SOURCE_DIR}/SamTSE/Sources")
  endif()
    message(STATUS "Using CMakeLists.txt from main directory")
else()
    set(CMAKE_ADD_TARGET_DIR "${CMAKE_SOURCE_DIR}")
    message(STATUS "Using CMakeLists.txt from subdirectory")
endif()

option(TFE "Compile a The First Encounter version" FALSE)

if(INTERNAL_NAME STREQUAL "serioussam")
 add_definitions(-DFIRST_ENCOUNTER=1)
 set(MP "")
 message(STATUS "Building Serious Sam Classic The First Encounter")
else()
 set(MP "MP")
 message(STATUS "Building Serious Sam Classic The Second Encounter")
endif()

###############################################################
###############################################################
###############################################################

# Set up some sanity stuff...
# Note:
# "-rpath,$ORIGIN" allows you not to use LD_LIBRARY_PATH and is well suited for local installation
# "-rpath,$ORIGIN" don't like new versions of clang + ninja and crashes during linking of the Engine library
# "-rpath=/tmp" is suitable for linking clang + ninja and installing the game into the system
# "-rpath=/var/tmp/portage" is suitable for Gentoo and installing the game into the system
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME MATCHES "GNU")
    set(LINUX TRUE)
    message(STATUS "Set GNU Linux TRUE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-class-memaccess -Wno-unused-function")
    if(CMAKE_OS_NAME STREQUAL "Gentoo" AND USE_SYSTEM_INSTALL)
      set(RPATH_SETTINGS "-rpath=/var/tmp/portage")
      message(STATUS "Build for Gentoo with '-rpath=/var/tmp/portage'")
    else()
      set(RPATH_SETTINGS "-rpath,$ORIGIN")
    endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable -Wno-uninitialized -Wno-unused-function")
    set(FREEBSD TRUE)
    if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")                                  # for local instal
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86|amd64|AMD64" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        if(USE_ASM)
          set(RPATH_SETTINGS "-z,notext,-rpath,$ORIGIN")
          message(STATUS "Arch i386 using -Wl,-z,notext LD flags")
        else()
          set(RPATH_SETTINGS "-rpath,$ORIGIN")
          message(STATUS "Arch i386 portable C instead of all ASM")
        endif()
      else()
        set(RPATH_SETTINGS "-rpath,$ORIGIN")
      endif()
      message(STATUS "Using cmake generator 'Unix Makefiles'")
    elseif(CMAKE_GENERATOR STREQUAL "Ninja")                                       # defalt for install to the system
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86|amd64|AMD64" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        if(USE_ASM)
          set(RPATH_SETTINGS "-z,notext,-rpath=/tmp")
          message(STATUS "Arch i386 using -Wl,-z,notext LD flags")
        else()
          set(RPATH_SETTINGS "-rpath=/tmp")
          message(STATUS "Arch i386 portable C instead of all ASM")
        endif()
      else()
        set(RPATH_SETTINGS "-rpath=/tmp")
      endif()  
      message(STATUS "Using cmake generator 'Ninja'")
    else()
        message(FATAL_ERROR "Unknown cmake generator")
    endif()
endif()

if(APPLE)
    set(MACOSX TRUE)
    if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
      set(RPATH_SETTINGS "-rpath,$ORIGIN")
      message(STATUS "Using cmake generator 'Unix Makefiles'")
    elseif(CMAKE_GENERATOR STREQUAL "Ninja")
      set(RPATH_SETTINGS "-rpath=/tmp")     
      message(STATUS "Using cmake generator 'Ninja'")
    else()
        message(FATAL_ERROR "Unknown cmake generator")
    endif()
endif()

if(MSVC)
    set(WINDOWS TRUE)
endif()

# RAKE! Where to install the binaries.
if(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" OR CMAKE_INSTALL_PREFIX STREQUAL "") # Only works for linux since I don't
                                                                                   # know what default is for windows/macos/freebsd.
    if("${Found_Sam}" EQUAL "0")
        if(INTERNAL_NAME STREQUAL "serioussam")
            set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/SamTFE/Sources/../")     # set install path to project root directory since
        else()                                                                     # since one wasn't set during config
            set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/SamTSE/Sources/../")     # set install path to project root directory since
        endif()                                                                    # since one wasn't set during config
    else()
        set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/../")                        # set install path to project root directory since
    endif()                                                                        # since one wasn't set during config
    if(NOT USE_SYSTEM_INSTALL)
        set(LOCAL_INSTALL TRUE)
        message(STATUS "Using LOCAL Install")
    endif() 
endif()
if(CMAKE_INSTALL_PREFIX STREQUAL "/usr" OR USE_SYSTEM_INSTALL)
    set(LOCAL_INSTALL FALSE)
    message(STATUS "Using SYSTEM Install")
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
        set(CMAKE_INSTALL_PREFIX "/usr/local")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        set(CMAKE_INSTALL_PREFIX "/usr/pkg")
    else()
        set(CMAKE_INSTALL_PREFIX "/usr")  
    endif()
endif()

# ImageMagic needed for SYSTEM INSTALL
if(NOT LOCAL_INSTALL)
	find_package(ImageMagick COMPONENTS convert)
    if(ImageMagick_convert_FOUND)
        #message(STATUS "ImageMagick convert found: ${ImageMagick_convert_EXECUTABLE}")
    else()
        if(NOT DIII4A)
        message(FATAL_ERROR "Error! LOCAL_INSTAL is set but neccessary ImageMagick convert are missing")
        endif()
    endif()
endif()

# remove clang spam.
if(MACOSX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness -Wno-deprecated")
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

message(STATUS "Operating system name: ${CMAKE_OS_NAME}")

set(CMAKE_ARCH_BITS 32)
set(CMAKE_INSTALL_LIBDIR "/usr/lib")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_ARCH_BITS 64)
    set(CMAKE_INSTALL_LIBDIR "/usr/lib64")
endif()

if(FREEBSD)
    set(CMAKE_INSTALL_LIBDIR "/usr/local/lib")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  include_directories("/usr/pkg/include")
  set(CMAKE_INSTALL_LIBDIR "/usr/pkg/lib")
endif()

message(STATUS "Arch bits: ${CMAKE_ARCH_BITS}")

if(DEBIAN_FOUND OR UBUNTU_FOUND)
    #set libdir for debian based OS
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "riscv64")
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/riscv64-linux-gnu")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")  # 32bit)
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/arm-linux-gnueabihf")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*)")
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/aarch64-linux-gnu")
        if(NOT "${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            message(FATAL_ERROR "invalid pointer size")
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/powerpc64-linux-gnu")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL s390x)
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/s390x-linux-gnu")
    elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86|amd64|AMD64") AND (CMAKE_SIZEOF_VOID_P EQUAL 4))
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/i386-linux-gnu")
    elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64") AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/x86_64-linux-gnu")
    elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "i386") AND (CMAKE_SIZEOF_VOID_P EQUAL 8) AND (APPLE))
        set(CMAKE_INSTALL_LIBDIR "/usr/lib/i386-linux-gnu")
    else()
        message(FATAL_ERROR "Unknown architecture")
    endif()
endif()

message(STATUS "Installed libraries dir: ${CMAKE_INSTALL_LIBDIR}")

## ** RAKE! start compiler specific flags section **
## ** RAKE! Borrowed from dhewm3 project, need to **
## ** RAKE! clean up for SeriousEngine use. Also  **
## ** RAKE! need to make this pandora safe.	  **
# compiler specific flags
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	# This section and the like are for flags/defines that can be shared between 
	# c and c++ compile options
	add_compile_options(-Wall)
	add_compile_options(-pipe)
	add_compile_options(-fPIC)
	add_compile_options(-fms-extensions)
	if(NOT ANDROID)
	if(NOT PANDORA AND NOT PYRA AND NOT RPI4 AND NOT (MACOSX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64"))
		if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "riscv64")
			add_compile_options(-march=rv64gc)
		    add_compile_options(-ftree-vectorize)
		    add_compile_options(-fomit-frame-pointer)
			message(STATUS "Architecture RISCV64, -march=rv64gc")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
			add_compile_options(-mpowerpc64)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64")
			add_compile_options(-march=loongarch64)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "mips|mips64|mipsel|mips64el")
			add_compile_options(-march=mips32r2)
		else()
            if(LOCAL_INSTALL)
			    add_compile_options(-march=native)
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i586|i686|x86|amd64|AMD64|x86_64")
			    add_compile_options(-march=native) # set mtune=generic if nedded
		    endif()
		endif()
	endif()
	if((PYRA OR PANDORA)  AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm.*"))
		if(PYRA)
		add_compile_options(-mfpu=neon-vfpv4)
		add_compile_options(-mcpu=cortex-a15)
		add_compile_options(-mfloat-abi=hard)
		add_compile_options(-marm)
                elseif(NOT (MACOSX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64"))
                    add_compile_options(-mfpu=neon)
		elseif(PANDORA)
		add_compile_options(-mfpu=neon)
		endif()
		add_compile_options(-fsigned-char)
	endif()
    if(NOT PYRA AND NOT PANDORA AND ${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm*")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")  # 32bit
            add_compile_options(-march=armv8-a+crc)
		    add_compile_options(-mfpu=neon-fp-armv8)
            if(RPI4) 
		        add_compile_options(-mcpu=cortex-a72)
            endif()
		    add_compile_options(-mfloat-abi=hard)
		    add_compile_options(-ftree-vectorize)
		    add_compile_options(-fomit-frame-pointer)
            message(STATUS "ARM 32bit, -march=armv8-a+crc, adding -mfpu=neon-fp-armv8")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*)")  # NEON is implied on aarch64
            if(NOT "${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                message(FATAL_ERROR "invalid pointer size")
            endif()
            add_compile_options(-march=armv8-a+crc+fp+simd)
            if(RPI4) 
		        add_compile_options(-mcpu=cortex-a72)
            endif()
		    add_compile_options(-ftree-vectorize)
		    add_compile_options(-fomit-frame-pointer)
            message(STATUS "AARCH64, -march=armv8-a+crc+fp+simd")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64")
            message(STATUS "Loongsoon 3 5000 series processor")
        else()
            message(FATAL_ERROR "Unknown ARM processor '${CMAKE_SYSTEM_PROCESSOR}'")
        endif()
    endif()
    endif()
    add_compile_options(-fno-strict-aliasing)
	add_definitions(-D_REENTRANT=1)
	add_definitions(-D_MT=1)

	## Add your custom C and CXX flags on the command line aka -DCMAKE_C_FLAGS=-std=c98 or -DCMAKE_CXX_FLAGS=-std=c++11

	## For C flags
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -D_DEBUG=1 -DDEBUG=1 -O0")
        if(PANDORA)
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -faligned-new -ffast-math")
            set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2  -faligned-new -ffast-math")
            set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -Os -ffast-math")
        elseif(PYRA)
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -marm -faligned-new -ffast-math")
            set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -marm -faligned-new -ffast-math")
            set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -Os -marm -ffast-math")
        elseif(MACOSX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O0 -fno-unsafe-math-optimizations")
            set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O0 -fno-unsafe-math-optimizations")
            set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -O0 -fno-unsafe-math-optimizations")
        else()
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -fno-delete-null-pointer-checks -fno-unsafe-math-optimizations")
            set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -fno-delete-null-pointer-checks -fno-unsafe-math-optimizations")
            set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -Os -fno-delete-null-pointer-checks -fno-unsafe-math-optimizations")
        endif()

	## For C++ flags
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -D_DEBUG=1 -DDEBUG=1 -O0")
        if(PANDORA)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -O2  -faligned-new -ffast-math")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2  -faligned-new -ffast-math")
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -Os -ffast-math")
        elseif(PYRA)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -O2 -marm -faligned-new -ffast-math")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -marm -faligned-new -ffast-math")
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -Os -marm -ffast-math")
        elseif(MACOSX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -O0 -fno-unsafe-math-optimizations")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O0 -fno-unsafe-math-optimizations") ## RAKE! Does -DNDEBUG=1 and -D_NDEBUG=1 mess with RelWithDebInfo?
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -O0 -fno-unsafe-math-optimizations")
        else()
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -O2 -fno-delete-null-pointer-checks -fno-unsafe-math-optimizations")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -g -O2 -fno-delete-null-pointer-checks -fno-unsafe-math-optimizations") ## RAKE! Does -DNDEBUG=1 and -D_NDEBUG=1 mess with RelWithDebInfo?
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -D_NDEBUG=1 -Os -fno-delete-null-pointer-checks -fno-unsafe-math-optimizations")
        endif()

	# TODO fix these warnings
	add_compile_options(-Wno-format-security)
	add_compile_options(-Wno-switch)
	add_compile_options(-Wno-char-subscripts)
	add_compile_options(-Wno-unknown-pragmas)
	add_compile_options(-Wno-unused-variable) # TODO: maybe only enable this for Entities
	add_compile_options(-Wno-unused-value)    # same here (the Scripts generate tons of unused variables and values)
	add_compile_options(-Wno-missing-braces)
	add_compile_options(-Wno-overloaded-virtual)
	add_compile_options(-Wno-invalid-offsetof)
	#MESSAGE(WARNING, "re-enable some of the warnings some day!")

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
		# !!! FIXME: turn a bunch of these off, this is just for now. I hope.  --ryan.
		add_compile_options(-Wno-c++11-compat-deprecated-writable-strings)
	endif()

	if(MACOSX)
		add_definitions(-DPLATFORM_UNIX=1)
    		add_definitions(-DPLATFORM_MACOSX=1)
    		add_definitions(-DPRAGMA_ONCE=1)
    		include_directories("/usr/local/include")
		include_directories("/usr/X11/include/")
	elseif(WINDOWS)
		add_definitions(-DPLATFORM_WIN32=1)
    		add_definitions(-DPRAGMA_ONCE=1)
                add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
                add_definitions(-D_CRT_SECURE_NO_DEPRECATE=1)
	elseif(LINUX)
	    	set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
    		add_definitions(-DPLATFORM_UNIX=1)
    		add_definitions(-D_FILE_OFFSET_BITS=64)
    		add_definitions(-D_LARGEFILE_SOURCE=1)
    		add_definitions(-DPRAGMA_ONCE=1)
	elseif(FREEBSD)
    		set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
    		add_definitions(-DPLATFORM_UNIX=1)
    		add_definitions(-DPLATFORM_FREEBSD=1)
    		add_definitions(-D_FILE_OFFSET_BITS=64)
    		add_definitions(-D_LARGEFILE_SOURCE=1)
    		add_definitions(-DPRAGMA_ONCE=1)
    		include_directories("/usr/local/include")
	endif()
	
        if(MACOSX OR LINUX OR FREEBSD)
		add_compile_options(-pthread)
                add_compile_options(-fsigned-char)
	endif()

	if(CMAKE_COMPILER_IS_GNUCC)
	    	# !!! FIXME: turn a bunch of these off, this is just for now. I hope.  --ryan.
    		add_compile_options(-Wno-invalid-offsetof)
	endif()

elseif(MSVC) # RAKE! I don't know if this will build with MSVC
	add_compile_options(/W4)
	add_compile_options(/wd4100) # unreferenced formal parameter
	add_compile_options(/wd4127) # conditional expression is constant
	add_compile_options(/wd4244) # possible loss of data
	add_compile_options(/wd4245) # signed/unsigned mismatch
	add_compile_options(/wd4267) # possible loss of data
	add_compile_options(/wd4714) # 'function' marked as __forceinline not inlined
	add_compile_options(/wd4996) # 'function': was declared deprecated
	add_compile_options(/wd4068) # unknown pragma
	set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG /Od /Zi /MDd")
	set(CMAKE_C_FLAGS_RELEASE "/Ox /Oy /MD")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Oy /Zi /MD")
	set(CMAKE_C_FLAGS_MINSIZEREL "/Ox /Oy /Os /MD")
else()
	message(FATAL_ERROR "Unsupported compiler")
endif()

## ** RAKE! end compiler specific flags section **

if(DEBUG)
    set(DEBUGSUFFIX "D")
else()
    set(DEBUGSUFFIX "")
endif()

option(USE_I386_NASM_ASM "Use i386 nasm ASM code" FALSE)

if (USE_ASM AND USE_I386_NASM_ASM)
    # You need the Netwide Assembler (NASM) to build this on Intel systems.
    #   http://nasm.sf.net/
    add_definitions(-DUSE_I386_NASM_ASM=1)
    if (MACOSX)
        set(ASMOBJFMT "macho")
        list(APPEND ASMFLAGS --prefix _)
    elseif(WINDOWS)
        set(ASMOBJFMT "win32")
    else()
        set(ASMOBJFMT "elf")
    endif()
    MESSAGE(STATUS "Using i386 nasm ASM")
else()
    MESSAGE(STATUS "Not using i386 nasm ASM")
endif()

option(PANDORA "Compile for Pandora" FALSE)
if (PANDORA)
    add_definitions(-DPLATFORM_PANDORA=1)
endif()

option(PYRA "Compile for Pyra" FALSE)
if (PYRA)
    add_definitions(-DPLATFORM_PYRA=1)
endif()

option(RPI4 "Raspberry PI 4" FALSE)
if(RPI4)
    add_definitions(-DPLATFORM_RPI4=1)
endif()

option(USE_TREMOR "Use Tremor instead of Vorbis" FALSE)
if (USE_TREMOR)
    add_definitions(-DUSE_TREMOR=1)
endif()

option(XPLUS "Compile a XPLUS modification" FALSE)
if(XPLUS)
    execute_process (
        COMMAND bash -c "cp -fr ${CMAKE_ADD_TARGET_DIR}/Entities${MP}/PlayerWeaponsHD.es ${CMAKE_ADD_TARGET_DIR}/Entities${MP}/PlayerWeapons.es;"
        OUTPUT_VARIABLE outVar
    )
    message(STATUS "Compile a XPLUS modification")
else()
    execute_process (
        COMMAND bash -c "cp -fr ${CMAKE_ADD_TARGET_DIR}/Entities${MP}/PlayerWeapons_old.es ${CMAKE_ADD_TARGET_DIR}/Entities${MP}/PlayerWeapons.es;"
        OUTPUT_VARIABLE outVar
    )
    message(STATUS "Compile a standard game")
endif()

# !!! FIXME: I currently force this, but you shouldn't _have_ to.
option(USE_SINGLE_THREAD "Use Single Threaded version" FALSE)
if(USE_SINGLE_THREAD)
 add_definitions(-DSINGLE_THREADED=1)
endif()


include_directories(
    .
	${CMAKE_CURRENT_SOURCE_DIR}/External/libogg/include
)
if(USE_TREMOR)
if(PANDORA)
    include_directories(/mnt/utmp/codeblocks/usr/include/tremor)
else()
    # !!!Do something here!
endif()
else()
    include_directories(External/libvorbis/include)
endif()

# We build ECC, then use it to generate C++ code for the game entities...
macro(add_parser_and_scanner _PARSER _SCANNER)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${_SCANNER}.cpp"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${_SCANNER}.l"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND flex
        ARGS -o${_SCANNER}.cpp ${_SCANNER}.l
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${_PARSER}.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${_PARSER}.hpp"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${_PARSER}.y"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND bison
        ARGS -o${_PARSER}.cpp ${_PARSER}.y -d
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${_PARSER}.h"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${_PARSER}.hpp"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${_PARSER}.hpp ${_PARSER}.h
    )
endmacro()

# Build ECC from source if there wasn't a prebuilt-one specified on the command line.
#  Normally we build it here, but we might need a prebuilt, native binary if
#  we're cross-compiling the rest of the game.
if(NOT CROSS_COMPILE)
if(NOT ECC)
    add_parser_and_scanner("Ecc/Parser" "Ecc/Scanner")
    add_executable(ecc-se Ecc/Main.cpp Ecc/Parser.cpp Ecc/Parser.h Ecc/Scanner.cpp)
	if(DIII4A)
	set_target_properties(ecc-se PROPERTIES COMPILE_DEFINITIONS "PLATFORM_UNIX")
	set(ECC-SE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ecc")
	else()
    set(ECC-SE "ecc-se")
	endif()
endif()
else()
	message(STATUS "not need compile ECC when cross compile")
endif()

macro(entity _NAME)
	if(NOT CROSS_COMPILE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.h" "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}_tables.h"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.es"
        DEPENDS ${ECC-SE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ${ECC-SE}
        ARGS "${_NAME}.es"
    )
	else()
	message(STATUS "not need run ECC when cross compile")
	endif()
    list(APPEND ENTITIES_CPP "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.cpp")
    list(APPEND ENTITIES_H "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.h")
    list(APPEND ENTITIES_H "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}_tables.h")
endmacro()

set(ENTITIES_CPP "")
set(ENTITIES_H "")
entity(Engine/Classes/BaseEvents)
entity(Engine/Classes/MovableBrushEntity)
entity(Engine/Classes/MovableEntity)
entity(Engine/Classes/MovableModelEntity)
entity(Engine/Classes/PlayerEntity)
set(ENGINE_ENTITIES_CPP ${ENTITIES_CPP})

set(ENTITIES_CPP "")
if(INTERNAL_NAME STREQUAL "serioussam")
 entity(Entities/Acid)
 entity(Entities/AirWave)
 entity(Entities/AmmoItem)
 entity(Entities/AmmoPack)
 entity(Entities/AnimationChanger)
 entity(Entities/AnimationHub)
 entity(Entities/ArmorItem)
 entity(Entities/BackgroundViewer)
 entity(Entities/BasicEffects)
 entity(Entities/Beast)
 entity(Entities/BigHead)
 entity(Entities/BlendController)
 entity(Entities/BloodSpray)
 entity(Entities/Boneman)
 entity(Entities/Bouncer)
 entity(Entities/Bullet)
 entity(Entities/Camera)
 entity(Entities/CameraMarker)
 entity(Entities/CannonBall)
 entity(Entities/Catman)
 entity(Entities/Copier)
 entity(Entities/Counter)
 entity(Entities/CrateRider)
 entity(Entities/CyborgBike)
 entity(Entities/Cyborg)
 entity(Entities/Damager)
 entity(Entities/Debris)
 entity(Entities/DestroyableArchitecture)
 entity(Entities/Devil)
 entity(Entities/DevilMarker)
 entity(Entities/DevilProjectile)
 entity(Entities/DoorController)
 entity(Entities/Dragonman)
 entity(Entities/EffectMarker)
 entity(Entities/Effector)
 entity(Entities/Elemental)
 entity(Entities/EnemyBase)
 entity(Entities/EnemyCounter)
 entity(Entities/EnemyDive)
 entity(Entities/EnemyFly)
 entity(Entities/EnemyMarker)
 entity(Entities/EnemyRunInto)
 entity(Entities/EnemySpawner)
 entity(Entities/EnvironmentBase)
 entity(Entities/EnvironmentMarker)
 entity(Entities/Eruptor)
 entity(Entities/Eyeman)
 entity(Entities/Fish)
 entity(Entities/Fishman)
 entity(Entities/Flame)
 entity(Entities/FogMarker)
 #entity(Entities/GhostBusterRay)
 entity(Entities/Gizmo)
 entity(Entities/Global)
 entity(Entities/GradientMarker)
 entity(Entities/GravityMarker)
 entity(Entities/GravityRouter)
 entity(Entities/HazeMarker)
 entity(Entities/Headman)
 entity(Entities/HealthItem)
 entity(Entities/Huanman)
 entity(Entities/Item)
 entity(Entities/KeyItem)
 entity(Entities/Light)
 entity(Entities/Lightning)
 entity(Entities/LightStyle)
 entity(Entities/Mamut)
 entity(Entities/Mamutman)
 entity(Entities/Mantaman)
 entity(Entities/Marker)
 entity(Entities/MessageHolder)
 entity(Entities/MessageItem)
 entity(Entities/MirrorMarker)
 entity(Entities/ModelDestruction)
 entity(Entities/ModelHolder2)
 entity(Entities/ModelHolder)
 entity(Entities/MovingBrush)
 entity(Entities/MovingBrushMarker)
 entity(Entities/MusicChanger)
 entity(Entities/MusicHolder)
 entity(Entities/NavigationMarker)
 entity(Entities/ParticlesHolder)
 entity(Entities/Pendulum)
 entity(Entities/Pipebomb)
 entity(Entities/PlayerActionMarker)
 entity(Entities/PlayerAnimator)
 entity(Entities/Player)
 entity(Entities/PlayerMarker)
 entity(Entities/PlayerView)
 entity(Entities/PlayerWeaponsEffects)
 entity(Entities/PlayerWeapons)
 entity(Entities/Projectile)
 entity(Entities/PyramidSpaceShip)
 entity(Entities/PyramidSpaceShipMarker)
 entity(Entities/Reminder)
 entity(Entities/RobotDriving)
 entity(Entities/RobotFixed)
 entity(Entities/RobotFlying)
 entity(Entities/RollingStone)
 entity(Entities/Scorpman)
 entity(Entities/Ship)
 entity(Entities/ShipMarker)
 entity(Entities/SoundHolder)
 entity(Entities/StormController)
 entity(Entities/Switch)
 entity(Entities/Teleport)
 entity(Entities/TouchField)
 entity(Entities/Trigger)
 entity(Entities/Twister)
 entity(Entities/VoiceHolder)
 entity(Entities/Walker)
 entity(Entities/Watcher)
 entity(Entities/WatchPlayers)
 entity(Entities/Water)
 entity(Entities/WeaponItem)
 entity(Entities/Werebull)
 entity(Entities/Woman)
 entity(Entities/WorldBase)
 entity(Entities/WorldLink)
 entity(Entities/WorldSettingsController)
else()
 entity(EntitiesMP/AirElemental)
 entity(EntitiesMP/AirShockwave)
 entity(EntitiesMP/AmmoItem)
 entity(EntitiesMP/AmmoPack)
 entity(EntitiesMP/AnimationChanger)
 entity(EntitiesMP/AnimationHub)
 entity(EntitiesMP/AreaMarker)
 entity(EntitiesMP/ArmorItem)
 entity(EntitiesMP/BackgroundViewer)
 entity(EntitiesMP/BasicEffects)
 entity(EntitiesMP/Beast)
 entity(EntitiesMP/BigHead)
 entity(EntitiesMP/BlendController)
 entity(EntitiesMP/BloodSpray)
 entity(EntitiesMP/Boneman)
 entity(EntitiesMP/Bouncer)
 entity(EntitiesMP/Bullet)
 entity(EntitiesMP/Camera)
 entity(EntitiesMP/CameraMarker)
 entity(EntitiesMP/CannonBall)
 entity(EntitiesMP/CannonRotating)
 entity(EntitiesMP/CannonStatic)
 entity(EntitiesMP/ChainsawFreak)
 entity(EntitiesMP/Copier)
 entity(EntitiesMP/Counter)
 entity(EntitiesMP/CrateBus)
 entity(EntitiesMP/CrateRider)
 entity(EntitiesMP/CreditsHolder)
 entity(EntitiesMP/Damager)
 entity(EntitiesMP/Debris)
 entity(EntitiesMP/DebugEntityStatesDisplay)
 entity(EntitiesMP/Demon)
 entity(EntitiesMP/DestroyableArchitecture)
 entity(EntitiesMP/Devil)
 entity(EntitiesMP/DevilMarker)
 entity(EntitiesMP/DevilProjectile)
 entity(EntitiesMP/DoorController)
 entity(EntitiesMP/Dragonman)
 entity(EntitiesMP/EffectMarker)
 entity(EntitiesMP/Effector)
 entity(EntitiesMP/Elemental)
 entity(EntitiesMP/EnemyBase)
 entity(EntitiesMP/EnemyCounter)
 entity(EntitiesMP/EnemyDive)
 entity(EntitiesMP/EnemyFly)
 entity(EntitiesMP/EnemyMarker)
 entity(EntitiesMP/EnemyRunInto)
 entity(EntitiesMP/EnemySpawner)
 entity(EntitiesMP/EnvironmentBase)
 entity(EntitiesMP/EnvironmentMarker)
 entity(EntitiesMP/EnvironmentParticlesHolder)
 entity(EntitiesMP/Eruptor)
 entity(EntitiesMP/ExotechLarva)
 entity(EntitiesMP/ExotechLarvaBattery)
 entity(EntitiesMP/ExotechLarvaCharger)
 entity(EntitiesMP/Eyeman)
 entity(EntitiesMP/Fireworks)
 entity(EntitiesMP/Fish)
 entity(EntitiesMP/Flame)
 entity(EntitiesMP/FogMarker)
 entity(EntitiesMP/Gizmo)
 entity(EntitiesMP/Global)
 entity(EntitiesMP/GradientMarker)
 entity(EntitiesMP/GravityMarker)
 entity(EntitiesMP/GravityRouter)
 entity(EntitiesMP/Grunt)
 entity(EntitiesMP/GruntSka)
 entity(EntitiesMP/Guffy)
 entity(EntitiesMP/HazeMarker)
 entity(EntitiesMP/Headman)
 entity(EntitiesMP/HealthItem)
 entity(EntitiesMP/HudPicHolder)
 entity(EntitiesMP/Item)
 entity(EntitiesMP/KeyItem)
 entity(EntitiesMP/LarvaOffspring)
 entity(EntitiesMP/Light)
 entity(EntitiesMP/Lightning)
 entity(EntitiesMP/Marker)
 entity(EntitiesMP/MessageHolder)
 entity(EntitiesMP/MessageItem)
 entity(EntitiesMP/MeteorShower)
 entity(EntitiesMP/MirrorMarker)
 entity(EntitiesMP/ModelDestruction)
 entity(EntitiesMP/ModelHolder)
 entity(EntitiesMP/ModelHolder2)
 entity(EntitiesMP/ModelHolder3)
 entity(EntitiesMP/MovingBrush)
 entity(EntitiesMP/MovingBrushMarker)
 entity(EntitiesMP/MusicChanger)
 entity(EntitiesMP/MusicHolder)
 entity(EntitiesMP/NavigationMarker)
 entity(EntitiesMP/ParticlesHolder)
 entity(EntitiesMP/Pendulum)
 entity(EntitiesMP/PhotoAlbum)
 entity(EntitiesMP/Pipebomb)
 entity(EntitiesMP/Player)
 entity(EntitiesMP/PlayerActionMarker)
 entity(EntitiesMP/PlayerAnimator)
 entity(EntitiesMP/PlayerMarker)
 entity(EntitiesMP/PlayerView)
 entity(EntitiesMP/PlayerWeapons)
 entity(EntitiesMP/PlayerWeaponsEffects)
 entity(EntitiesMP/PowerUpItem)
 entity(EntitiesMP/Projectile)
 entity(EntitiesMP/PyramidSpaceShip)
 entity(EntitiesMP/PyramidSpaceShipMarker)
 entity(EntitiesMP/Reminder)
 entity(EntitiesMP/RollingStone)
 entity(EntitiesMP/Santa)
 entity(EntitiesMP/Scorpman)
 entity(EntitiesMP/ScrollHolder)
 entity(EntitiesMP/SeriousBomb)
 entity(EntitiesMP/Ship)
 entity(EntitiesMP/ShipMarker)
 entity(EntitiesMP/Shooter)
 entity(EntitiesMP/SoundHolder)
 entity(EntitiesMP/SpawnerProjectile)
 entity(EntitiesMP/Spinner)
 entity(EntitiesMP/StormController)
 entity(EntitiesMP/Summoner)
 entity(EntitiesMP/SummonerMarker)
 entity(EntitiesMP/Switch)
 entity(EntitiesMP/TacticsChanger)
 entity(EntitiesMP/TacticsHolder)
 entity(EntitiesMP/Teleport)
 entity(EntitiesMP/Terrain)
 entity(EntitiesMP/TextFXHolder)
 entity(EntitiesMP/TimeController)
 entity(EntitiesMP/TouchField)
 entity(EntitiesMP/Trigger)
 entity(EntitiesMP/Twister)
 entity(EntitiesMP/VoiceHolder)
 entity(EntitiesMP/Walker)
 entity(EntitiesMP/WatchPlayers)
 entity(EntitiesMP/Watcher)
 entity(EntitiesMP/Water)
 entity(EntitiesMP/WeaponItem)
 entity(EntitiesMP/Werebull)
 entity(EntitiesMP/Woman)
 entity(EntitiesMP/WorldBase)
 entity(EntitiesMP/WorldLink)
 entity(EntitiesMP/WorldSettingsController)
endif()

add_custom_target(ParseEntities${MP} DEPENDS ${ENTITIES_H})

if(STATICALLY_LINKED)
	set(LIB_LINK_TYPE STATIC)
else()
	set(LIB_LINK_TYPE SHARED)
endif()

set(ENTITIESMPLIB "${SAM_PREFIX}Entities${MP}${DEBUGSUFFIX}")
if(INTERNAL_NAME STREQUAL "serioussam")
	add_library(${ENTITIESMPLIB} ${LIB_LINK_TYPE}
     ${ENTITIES_CPP}
     Entities/Common/Common.cpp
     Entities/Common/Debris.cpp
     Entities/Common/Particles.cpp
     Entities/Common/Stats.cpp
     Entities/Common/PathFinding.cpp
     Entities/Common/HUD.cpp
 )
else()
	add_library(${ENTITIESMPLIB} ${LIB_LINK_TYPE}
     ${ENTITIES_CPP}
     EntitiesMP/Common/Common.cpp
 	 EntitiesMP/Common/Particles.cpp
 	 EntitiesMP/Common/EmanatingParticles.cpp
 	 EntitiesMP/Common/PathFinding.cpp
 	 EntitiesMP/Common/HUD.cpp
 )
endif()

if(MACOSX)
    target_link_libraries(${ENTITIESMPLIB} "-undefined dynamic_lookup")
endif()
add_dependencies(${ENTITIESMPLIB} ParseEntities${MP})

set(GAMEMPLIB "${SAM_PREFIX}Game${MP}${DEBUGSUFFIX}")
add_library(${GAMEMPLIB} ${LIB_LINK_TYPE}
    GameMP/Camera.cpp
    GameMP/CompMessage.cpp
    GameMP/CompModels.cpp
    GameMP/Computer.cpp
    GameMP/Console.cpp
    GameMP/Controls.cpp
    GameMP/Game.cpp
    GameMP/LCDDrawing.cpp
    GameMP/LoadingHook.cpp
    GameMP/Map.cpp
    GameMP/SessionProperties.cpp
    GameMP/WEDInterface.cpp
)
if(MACOSX)
    target_link_libraries(${GAMEMPLIB} "-undefined dynamic_lookup")
endif()
add_dependencies(${GAMEMPLIB} ParseEntities${MP})

if(NOT XPLUS)
	set(SHADERSLIB "${SAM_PREFIX}Shaders${MP}${DEBUGSUFFIX}")
add_library(${SHADERSLIB} ${LIB_LINK_TYPE}
    Shaders/AddShader.cpp
    Shaders/AddShaderDS.cpp
    Shaders/BaseShader.cpp
    Shaders/BaseShaderDS.cpp
    Shaders/BaseTransparent.cpp
    Shaders/BaseTransparentDS.cpp
    Shaders/ColorShader.cpp
    Shaders/Common.cpp
    Shaders/DetailShader.cpp
    Shaders/DisplaceShader.cpp
    Shaders/InvisibleShader.cpp
    Shaders/MultiLayerShader.cpp
    Shaders/Reflection.cpp
    Shaders/ReflectionDS.cpp
    Shaders/ReftectionAndSpecular.cpp
    Shaders/ReftectionAndSpecularDS.cpp
    Shaders/Specular.cpp
    Shaders/SpecularDS.cpp
    Shaders/StdH.cpp
    Shaders/Translucent.cpp
)
if(MACOSX)
    target_link_libraries(${SHADERSLIB} "-undefined dynamic_lookup")
endif()
add_dependencies(${SHADERSLIB} ParseEntities${MP})
endif() # NOT XPLUS

if(NOT CROSS_COMPILE)
add_parser_and_scanner("Engine/Base/Parser" "Engine/Base/Scanner")
add_parser_and_scanner("Engine/Ska/smcPars" "Engine/Ska/smcScan")
else()
	message(STATUS "not need generate source code when cross compile")
endif()

if(NOT XPLUS)
if (USE_I386_NASM_ASM)
    add_custom_command(
        OUTPUT "SoundMixer386.o"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Sound/SoundMixer386.asm"
        COMMAND nasm
        ARGS ${ASMFLAGS} -f ${ASMOBJFMT} -o SoundMixer386.o ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Sound/SoundMixer386.asm
    )
    list(APPEND ADDITIONAL_ENGINE_SRCS SoundMixer386.o)
endif()

if(USE_SINGLE_THREAD)
 set(SYNCHRO_SRCS
    Engine/Base/NullSynchronization.cpp  # single threaded.
 )
else()
 #!! TODO Win32/Linux case
 set(SYNCHRO_SRCS
    Engine/Base/Unix/UnixSynchronization.cpp  # multithreaded Unix.
    #Engine/Base/SDL/SDLSynchronization.cpp
    #Engine/Base/SDL/SDLThreadLocalStorage.cpp  # multithreaded Unix.
 )
    #Engine/Base/Registry.cpp  # Windows only.
    #Engine/Base/StackDump.cpp  # Windows only.
    #Engine/Base/Win32/Win32Synchronization.cpp # Windows only.
endif()

set(ZLIB_SRCS "")
if(NOT USE_SYSTEM_ZLIB)
    set(ZLIB_SRCS
        Engine/zlib/inflate.c
        Engine/zlib/adler32.c
        Engine/zlib/infblock.c
        Engine/zlib/inffast.c
        Engine/zlib/inftrees.c
        Engine/zlib/trees.c
        Engine/zlib/compress.c
        Engine/zlib/zutil.c
        Engine/zlib/deflate.c
        Engine/zlib/infcodes.c
        Engine/zlib/infutil.c
        Engine/zlib/uncompr.c)
endif()

set(ENGINE_SAFEMATH_SRCS
    Engine/Brushes/Brush.cpp
    Engine/Brushes/BrushPolygon.cpp
    Engine/Brushes/BrushSector.cpp
    Engine/Entities/Entity.cpp
    Engine/Entities/EntityClass.cpp
    Engine/Entities/EntityCollision.cpp
    Engine/Entities/PlayerCharacter.cpp
    Engine/Terrain/TerrainMisc.cpp
    Engine/Terrain/TerrainRayCasting.cpp
    Engine/World/WorldCSG.cpp
    Engine/World/WorldRayCasting.cpp
    Engine/World/WorldCollision.cpp
    Engine/World/WorldCollisionGrid.cpp

    Engine/Math/Projection_Simple_DOUBLE.cpp
    Engine/Math/Geometry_DOUBLE.cpp
)
add_library(${SAM_PREFIX}engine_safemath${MP} STATIC
    ${ENGINE_SAFEMATH_SRCS}
)
target_compile_options(${SAM_PREFIX}engine_safemath${MP} PRIVATE "-fno-unsafe-math-optimizations")
if(PANDORA)
	target_compile_options(${SAM_PREFIX}engine_safemath${MP} PRIVATE "-mfpu=vfpv3")
endif()
add_dependencies(${SAM_PREFIX}engine_safemath${MP} ParseEntities${MP})

set(ENGINE_SRCS
    ${ENGINE_ENTITIES_CPP}
    Engine/Engine.cpp
    Engine/Base/Anim.cpp
    Engine/Base/CRC.cpp
    Engine/Base/CRCTable.cpp
    Engine/Base/Changeable.cpp
    Engine/Base/Console.cpp
    Engine/Base/Directory.cpp
    Engine/Base/ErrorReporting.cpp
    Engine/Base/FileName.cpp
    Engine/Base/Input.cpp
    Engine/Base/Lists.cpp
    Engine/Base/Memory.cpp
    Engine/Base/Profiling.cpp
    Engine/Base/ProgressHook.cpp
    Engine/Base/Protection.cpp
    Engine/Base/Relations.cpp
    Engine/Base/ReplaceFile.cpp
    Engine/Base/Serial.cpp
    Engine/Base/Shell.cpp
    Engine/Base/ShellTypes.cpp
    Engine/Base/Statistics.cpp
    Engine/Base/Stream.cpp
    Engine/Base/Timer.cpp
    Engine/Base/Translation.cpp
    Engine/Base/Unzip.cpp
    Engine/Base/Updateable.cpp
    Engine/Base/CTString.cpp
    Engine/Base/Scanner.cpp
    Engine/Base/Parser.cpp
    Engine/Base/Parser.h
    Engine/Base/IFeel.cpp
    Engine/Base/iconvlite.cpp
    Engine/Base/Unix/UnixFileSystem.cpp
    Engine/Base/Unix/UnixDynamicLoader.cpp
    Engine/Base/SDL/SDLTimer.cpp
    Engine/Base/SDL/SDLInput.cpp
    Engine/Base/SDL/SDLEvents.cpp
    ${SYNCHRO_SRCS}
    Engine/Brushes/BrushIO.cpp
    Engine/Brushes/BrushShadows.cpp
    Engine/Brushes/BrushTriangularize.cpp
    Engine/Brushes/BrushArchive.cpp
    Engine/Brushes/BrushImport.cpp
    Engine/Brushes/BrushMip.cpp
    Engine/Brushes/BrushExport.cpp
    Engine/Entities/NearestPolygon.cpp
    Engine/Entities/EntityProperties.cpp
    Engine/Entities/FieldBSPTesting.cpp
    Engine/Entities/EntityCopying.cpp
    Engine/Entities/LastPositions.cpp
    Engine/Math/Projection_Isometric.cpp
    Engine/Math/Object3D.cpp
    Engine/Math/Projection_Parallel.cpp
    Engine/Math/Projection_Perspective.cpp
    Engine/Math/Float.cpp
    Engine/Math/Object3D_CSG.cpp
    Engine/Math/Projection_Simple.cpp
    Engine/Math/Functions.cpp
    Engine/Math/ObjectSector.cpp
    Engine/Math/Placement.cpp
    Engine/Math/TextureMapping.cpp
    Engine/Math/Geometry.cpp
    Engine/Math/Projection.cpp
    #Engine/Math/Object3D_IO.cpp  # Exploration 3D support.
    #Engine/Models/EditModel.cpp
    Engine/Models/Model.cpp
    Engine/Models/RenderModel_View.cpp
    Engine/Models/Normals.cpp
    Engine/Models/VertexGetting.cpp
    Engine/Models/RenderModel.cpp
    Engine/Models/MipMaker.cpp
    Engine/Models/ModelProfile.cpp
    Engine/Models/RenderModel_Mask.cpp
    Engine/Light/LayerMaker.cpp
    Engine/Light/LayerMixer.cpp
    Engine/Light/LightSource.cpp
    Engine/Graphics/Adapter.cpp
    Engine/Graphics/Raster.cpp
    Engine/Graphics/GfxLibrary.cpp
    Engine/Graphics/Benchmark.cpp
    Engine/Graphics/GfxProfile.cpp
    Engine/Graphics/Color.cpp
    Engine/Graphics/ShadowMap.cpp
    Engine/Graphics/DepthCheck.cpp
    Engine/Graphics/Texture.cpp
    Engine/Graphics/DisplayMode.cpp
    Engine/Graphics/Gfx_OpenGL.cpp
    Engine/Graphics/Gfx_OpenGL_Textures.cpp
    Engine/Graphics/TextureEffects.cpp
    Engine/Graphics/DrawPort.cpp
    Engine/Graphics/Gfx_wrapper.cpp
    Engine/Graphics/DrawPort_Particles.cpp
    Engine/Graphics/Graphics.cpp
    Engine/Graphics/ViewPort.cpp
    Engine/Graphics/DrawPort_RenderScene.cpp
    Engine/Graphics/ImageInfo.cpp
    Engine/Graphics/Fog.cpp
    Engine/Graphics/MultiMonitor.cpp
    Engine/Graphics/Font.cpp
    Engine/Graphics/Shader.cpp
    Engine/Graphics/Stereo.cpp
    Engine/Graphics/SDL/SDLOpenGL.cpp
    Engine/Graphics/SDL/SDLAdapter.cpp
    Engine/Network/ActionBuffer.cpp
    Engine/Network/NetworkMessage.cpp
    Engine/Network/Server.cpp
    Engine/Network/Buffer.cpp
    Engine/Network/NetworkProfile.cpp
    Engine/Network/SessionState.cpp
    Engine/Network/PlayerBuffer.cpp
    Engine/Network/MessageDispatcher.cpp
    Engine/Network/PlayerSource.cpp
    Engine/Network/Compression.cpp
    Engine/Network/Network.cpp
    Engine/Network/PlayerTarget.cpp
    Engine/Network/CPacket.cpp
    Engine/Network/ClientInterface.cpp
    Engine/Network/CommunicationInterface.cpp
    Engine/Network/Diff.cpp
    Engine/GameAgent/GameAgent.cpp
    Engine/Terrain/ArrayHolder.cpp
    Engine/Terrain/Terrain.cpp
    Engine/Terrain/TerrainArchive.cpp
    Engine/Terrain/TerrainEditing.cpp
    Engine/Terrain/TerrainLayer.cpp
    Engine/Terrain/TerrainRender.cpp
    Engine/Terrain/TerrainTile.cpp
    Engine/Rendering/Render.cpp
    Engine/Rendering/RenderProfile.cpp
    Engine/Rendering/SelectOnRender.cpp
    Engine/Ska/AnimSet.cpp
    Engine/Ska/RMRender.cpp
    Engine/Ska/Skeleton.cpp
    Engine/Ska/ModelInstance.cpp
    Engine/Ska/StringTable.cpp
    Engine/Ska/Mesh.cpp
    Engine/Ska/RMRenderMask.cpp
    Engine/Ska/smcPars.cpp
    Engine/Ska/smcPars.h
    Engine/Ska/smcScan.cpp
    Engine/Sound/SoundDecoder.cpp
    Engine/Sound/SoundObject.cpp
    Engine/Sound/SoundLibrary.cpp
    Engine/Sound/SoundProfile.cpp
    Engine/Sound/SoundData.cpp
    Engine/Sound/Wave.cpp
    Engine/Sound/SoundMixer.cpp
    Engine/Templates/Stock_CAnimData.cpp
    Engine/Templates/Stock_CAnimSet.cpp
    Engine/Templates/Stock_CEntityClass.cpp
    Engine/Templates/Stock_CMesh.cpp
    Engine/Templates/Stock_CModelData.cpp
    Engine/Templates/Stock_CSkeleton.cpp
    Engine/Templates/Stock_CSoundData.cpp
    Engine/Templates/Stock_CTextureData.cpp
    Engine/Templates/Stock_CShader.cpp
    Engine/Templates/NameTable_CTFileName.cpp
    Engine/Templates/NameTable_CTranslationPair.cpp
    Engine/Templates/BSP.cpp
    Engine/World/PhysicsProfile.cpp
    Engine/World/World.cpp
    Engine/World/WorldEditingProfile.cpp
    Engine/World/WorldIO.cpp
    ${ADDITIONAL_ENGINE_SRCS}
    ${ZLIB_SRCS}
)
if(NOT DIII4A)
	set(ENGINELIB "${SAM_PREFIX}Engine${MP}${DEBUGSUFFIX}")
add_library(${ENGINELIB} ${LIB_LINK_TYPE}
     ${ENGINE_SRCS}
)
if(MACOSX)
    target_link_libraries(${ENGINELIB} "-undefined dynamic_lookup")
endif()
add_dependencies(${ENGINELIB} ParseEntities${MP})
target_link_libraries(${ENGINELIB} ${SAM_PREFIX}engine_safemath${MP})
if(FREEBSD)
target_link_libraries(${ENGINELIB} ${SDL2_LIBRARY})
target_link_libraries(${ENGINELIB} ${ZLIB_LIBRARIES})
endif()
if(MACOSX)
    if(USE_SYSTEM_SDL2) # use sdl2 framework on system
      target_link_libraries(${ENGINELIB} ${SDL2_LIBRARY})
    else() # use local libsdl2
	  find_library(COCOA_FRAMEWORK Cocoa)
      target_link_libraries(${ENGINELIB} "${COCOA_FRAMEWORK}")
      target_link_libraries(${ENGINELIB} "${CMAKE_CURRENT_SOURCE_DIR}/lib/macosx/libSDL2-2.0.0.dylib")
    endif()
target_link_libraries(${ENGINELIB} ${ZLIB_LIBRARIES})
endif()
if(PANDORA)
    target_link_libraries(${ENGINELIB} "rt")
endif()
if(RPI4)
    target_link_libraries(${ENGINELIB} "rt")
endif()
set_target_properties(${ENGINELIB} PROPERTIES ENABLE_EXPORTS ON LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
endif()

if(ANDROID)
	add_library(SeriousSamTSE${MP} SHARED
    ${ENGINE_SRCS}
    SeriousSam/LevelInfo.cpp
    SeriousSam/CmdLine.cpp
    SeriousSam/SeriousSam.cpp
    SeriousSam/VarList.cpp
    SeriousSam/Credits.cpp
    SeriousSam/GLSettings.cpp
    SeriousSam/LCDDrawing.cpp
    SeriousSam/SplashScreen.cpp
    SeriousSam/MainWindow.cpp
    SeriousSam/Menu.cpp
    SeriousSam/MenuGadgets.cpp
    SeriousSam/MenuPrinting.cpp
)
else()
	add_executable(SeriousSamTSE${MP}
    #${ENGINE_SRCS}
    SeriousSam/LevelInfo.cpp
    SeriousSam/CmdLine.cpp
    SeriousSam/SeriousSam.cpp
    SeriousSam/VarList.cpp
    SeriousSam/Credits.cpp
    SeriousSam/GLSettings.cpp
    SeriousSam/LCDDrawing.cpp
    SeriousSam/SplashScreen.cpp
    SeriousSam/MainWindow.cpp
    SeriousSam/Menu.cpp
    SeriousSam/MenuGadgets.cpp
    SeriousSam/MenuPrinting.cpp
)
endif()
if(DIII4A)
	target_link_libraries(SeriousSamTSE${MP} ${ENGINELIB} EGL android z ${ANDROID_SOUND_LIB} ${SAM_PREFIX}engine_safemath${MP} -Wl,--whole-archive ${ENTITIESMPLIB} ${GAMEMPLIB} ${SAM_PREFIX}amp11lib${MP} samtse_ogg samtse_vorbis -Wl,--no-whole-archive )
else()
	target_link_libraries(SeriousSamTSE${MP} ${ENGINELIB})
endif()
add_dependencies(SeriousSamTSE${MP} ParseEntities${MP})
# Make symbols in the main executable available to dynamic objects
set_target_properties(SeriousSamTSE${MP} PROPERTIES ENABLE_EXPORTS ON)


# !!! FIXME: this is an option because you have to recompile the entire engine twice.
# !!! FIXME:  If we can put the engine in a static library and not lose symbols,
# !!! FIXME:  that's a better plan and we can remove the toggle here.
option(BUILD_DEDICATED_SERVER "Compile the dedicated server, too" TRUE)
if(BUILD_DEDICATED_SERVER)
    add_executable(DedicatedServer${MP} DedicatedServer/DedicatedServer.cpp)
    target_link_libraries(DedicatedServer${MP} ${ENGINELIB})
    add_dependencies(DedicatedServer${MP} ParseEntities${MP})
    # Make symbols in the main executable available to dynamic objects
    set_target_properties(DedicatedServer${MP} PROPERTIES ENABLE_EXPORTS ON)
endif()

option(BUILD_MAKEFONT "Compile the MakeFONT, too" TRUE)
if(BUILD_MAKEFONT)
    add_executable(MakeFONT${MP} MakeFONT/MakeFONT.cpp)
    target_link_libraries(MakeFONT${MP} ${ENGINELIB})
    add_dependencies(MakeFONT${MP} ParseEntities${MP})
    # Make symbols in the main executable available to dynamic objects
    set_target_properties(MakeFONT${MP} PROPERTIES ENABLE_EXPORTS ON)
endif()

option(BUILD_TEXConv "Compile the TEXConv, too" TRUE)
if(BUILD_TEXConv)
    add_executable(TEXConv${MP} TEXConv/TEXConv.cpp)
    target_link_libraries(TEXConv${MP} ${ENGINELIB})
    add_dependencies(TEXConv${MP} ParseEntities${MP})
    # Make symbols in the main executable available to dynamic objects
    set_target_properties(TEXConv${MP} PROPERTIES ENABLE_EXPORTS ON)
endif()

option(BUILD_AMP11LIB "Compile the AMP11LIB, too" TRUE)
if(BUILD_AMP11LIB)
add_definitions(-DFASTBITS)
add_definitions(-DUNIX)
add_definitions(-DLINUX)
add_definitions(-DOSS)
add_definitions(-DAMP11LIB_EXPORTS)

set(AMP11LIB_SOURCE_FILES
    amp11lib/binfile/binfarc.cpp
    amp11lib/binfile/binfarc.h
    amp11lib/binfile/binfile.cpp
    amp11lib/binfile/binfile.h
    amp11lib/binfile/binfstd.cpp
    amp11lib/binfile/binfstd.h
    amp11lib/binfile/ptypes.h
    amp11lib/amp11lib.cpp
    amp11lib/amp11lib.h
    amp11lib/amp1dec.cpp
    amp11lib/amp2dec.cpp
    amp11lib/amp3dec.cpp
    amp11lib/ampdec.cpp
    amp11lib/ampdec.h
    amp11lib/ampsynth.cpp)

add_library(${SAM_PREFIX}amp11lib${MP} ${LIB_LINK_TYPE} ${AMP11LIB_SOURCE_FILES})
endif()

if(MACOSX)
	target_link_libraries(SeriousSamTSE${MP} ${ZLIB_LIBRARIES})
    if(USE_SYSTEM_SDL2) # use sdl2 framework on system
		target_link_libraries(SeriousSamTSE${MP} ${SDL2_LIBRARY})
    else() # use local libsdl2
	  find_library(COCOA_FRAMEWORK Cocoa)
	  target_link_libraries(SeriousSamTSE${MP} "${COCOA_FRAMEWORK}")
	  target_link_libraries(SeriousSamTSE${MP} "${CMAKE_CURRENT_SOURCE_DIR}/lib/macosx/libSDL2-2.0.0.dylib")
    endif()
    
    if(BUILD_DEDICATED_SERVER)
		target_link_libraries(DedicatedServer${MP} ${ZLIB_LIBRARIES})
		if(USE_SYSTEM_SDL2)
			target_link_libraries(DedicatedServer${MP} ${SDL2_LIBRARY})
		else()
			target_link_libraries(DedicatedServer${MP} "${COCOA_FRAMEWORK}")
			target_link_libraries(DedicatedServer${MP} "${CMAKE_CURRENT_SOURCE_DIR}/lib/macosx/libSDL2-2.0.0.dylib")
		endif()
    endif()
endif()

if(LINUX)
	set_target_properties(SeriousSamTSE${MP} PROPERTIES LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
	target_link_libraries(SeriousSamTSE${MP} "m")
	target_link_libraries(SeriousSamTSE${MP} "dl")
	target_link_libraries(SeriousSamTSE${MP} "pthread")
	target_link_libraries(SeriousSamTSE${MP} ${SDL2_LIBRARY})
	target_link_libraries(SeriousSamTSE${MP} ${ZLIB_LIBRARIES})
    if(BUILD_DEDICATED_SERVER)
        set_target_properties(DedicatedServer${MP} PROPERTIES LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
        target_link_libraries(DedicatedServer${MP} "m")
        target_link_libraries(DedicatedServer${MP} "dl")
        target_link_libraries(DedicatedServer${MP} "pthread")
	    target_link_libraries(DedicatedServer${MP} ${SDL2_LIBRARY})
        target_link_libraries(DedicatedServer${MP} ${ZLIB_LIBRARIES})
    endif()
    if(BUILD_MAKEFONT)
        set_target_properties(MakeFONT${MP} PROPERTIES LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
        target_link_libraries(MakeFONT${MP} "m")
        target_link_libraries(MakeFONT${MP} "dl")
        target_link_libraries(MakeFONT${MP} "pthread")
	    target_link_libraries(MakeFONT${MP} ${SDL2_LIBRARY})
        target_link_libraries(MakeFONT${MP} ${ZLIB_LIBRARIES})
    endif()
    if(BUILD_TEXConv)
        set_target_properties(TEXConv${MP} PROPERTIES LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
        target_link_libraries(TEXConv${MP} "m")
        target_link_libraries(TEXConv${MP} "dl")
        target_link_libraries(TEXConv${MP} "pthread")
	    target_link_libraries(TEXConv${MP} ${SDL2_LIBRARY})
        target_link_libraries(TEXConv${MP} ${ZLIB_LIBRARIES})
    endif()
endif()

if(FREEBSD)
	set_target_properties(SeriousSamTSE${MP} PROPERTIES LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
	target_link_libraries(SeriousSamTSE${MP} "m")
	target_link_libraries(SeriousSamTSE${MP} "pthread")
	target_link_libraries(SeriousSamTSE${MP} ${SDL2_LIBRARY})
	target_link_libraries(SeriousSamTSE${MP} ${ZLIB_LIBRARIES})
    if(BUILD_DEDICATED_SERVER)
        set_target_properties(DedicatedServer${MP} PROPERTIES LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
        target_link_libraries(DedicatedServer${MP} "m")
        target_link_libraries(DedicatedServer${MP} "pthread")
        target_link_libraries(DedicatedServer${MP} ${SDL2_LIBRARY})
        target_link_libraries(DedicatedServer${MP} ${ZLIB_LIBRARIES})
    endif()
endif()

# Set output name for not local installation
if(LOCAL_INSTALL)
	set_target_properties(SeriousSamTSE${MP} PROPERTIES OUTPUT_NAME "SeriousSamTSE")
 if(NOT DIII4A)
 set_target_properties(DedicatedServer${MP}  PROPERTIES OUTPUT_NAME "DedicatedServer")
 set_target_properties(MakeFONT${MP} PROPERTIES OUTPUT_NAME "MakeFONT")
 set_target_properties(TEXConv${MP} PROPERTIES OUTPUT_NAME "TEXConv")
 endif()
 set_target_properties(${SAM_PREFIX}amp11lib${MP} PROPERTIES OUTPUT_NAME "${SAM_PREFIX}amp11lib${DEBUGSUFFIX}")
 if(NOT DIII4A)
 set_target_properties(${SHADERSLIBSE} PROPERTIES OUTPUT_NAME "Shaders${DEBUGSUFFIX}")
 endif()
else()
    if(DIII4A)
		set_target_properties(SeriousSamTSE${MP} PROPERTIES OUTPUT_NAME "SeriousSamTSE")
     else()
		 set_target_properties(SeriousSamTSE${MP} PROPERTIES OUTPUT_NAME "${INTERNAL_NAME}")
    endif()
 if(NOT DIII4A)
 set_target_properties(DedicatedServer${MP}  PROPERTIES OUTPUT_NAME "${INTERNAL_NAME}-ded")
 set_target_properties(MakeFONT${MP} PROPERTIES OUTPUT_NAME "${INTERNAL_NAME}-mkfont")
 set_target_properties(TEXConv${MP} PROPERTIES OUTPUT_NAME "${INTERNAL_NAME}-texconv")
 endif()
 set_target_properties(${SAM_PREFIX}amp11lib${MP} PROPERTIES OUTPUT_NAME "${SAM_PREFIX}amp11lib")
 if(NOT DIII4A)
 set_target_properties(${SHADERSLIB} PROPERTIES OUTPUT_NAME "Shaders")
 endif()
endif()
endif() # NOT XPLUS

# RAKE! Install Section.
if(LOCAL_INSTALL AND NOT XPLUS)
if(DEBUG) # RAKE! Will this work with TFE?
	install(TARGETS SeriousSamTSE${MP} ${SHADERSLIB} ${GAMEMPLIB} ${ENTITIESMPLIB}
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
            LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    install(TARGETS ${ENGINELIB}
            LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    if(BUILD_DEDICATED_SERVER)
        install(TARGETS DedicatedServer${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_MAKEFONT)
        install(TARGETS MakeFONT${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_TEXConv)
        install(TARGETS TEXConv${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_AMP11LIB)
        install(TARGETS amp11lib${MP}
                LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
else()
	install(TARGETS SeriousSamTSE${MP} ${SHADERSLIB} ${GAMEMPLIB} ${ENTITIESMPLIB} ${ENGINELIB}
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
            LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    if(BUILD_DEDICATED_SERVER)
        install(TARGETS DedicatedServer${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_MAKEFONT)
        install(TARGETS MakeFONT${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_TEXConv)
        install(TARGETS TEXConv${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_AMP11LIB)
		install(TARGETS ${SAM_PREFIX}amp11lib${MP}
                LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
endif()
elseif(LOCAL_INSTALL AND XPLUS)
    install(TARGETS ${GAMEMPLIB} ${ENTITIESMPLIB}
            LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Mods/XPLUS/Bin"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
# NOT LOCAL INSTALL
elseif(NOT LOCAL_INSTALL AND NOT XPLUS)
if(DEBUG) # RAKE! Will this work with TFE?
	install(TARGETS SeriousSamTSE${MP} ${SHADERSLIB} ${GAMEMPLIB} ${ENTITIESMPLIB} ${ENGINELIB}
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
            LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    if(BUILD_DEDICATED_SERVER)
        install(TARGETS DedicatedServer${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_MAKEFONT)
        install(TARGETS MakeFONT${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_TEXConv)
        install(TARGETS TEXConv${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_AMP11LIB)
        install(TARGETS ${SAM_PREFIX}amp11lib${MP}
                LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
else()
	install(TARGETS SeriousSamTSE${MP} ${SHADERSLIB} ${GAMEMPLIB} ${ENTITIESMPLIB}
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${INTERNAL_NAME}"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    install(TARGETS ${ENGINELIB}
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    if(BUILD_DEDICATED_SERVER)
        install(TARGETS DedicatedServer${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_MAKEFONT)
        install(TARGETS MakeFONT${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_TEXConv)
        install(TARGETS TEXConv${MP}
                RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(BUILD_AMP11LIB)
        install(TARGETS ${SAM_PREFIX}amp11lib${MP}
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${INTERNAL_NAME}"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
endif()
elseif(NOT LOCAL_INSTALL AND XPLUS)
    install(TARGETS ${GAMEMPLIB} ${ENTITIESMPLIB}
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${INTERNAL_NAME}/Mods/XPLUS"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# RAKE! If CMAKE_INSTALL_PREFIX was set during config then its not a local install
# and SE1_10.gro needs to be installed to Games root dir.
if(NOT LOCAL_INSTALL AND NOT XPLUS)
    install(FILES ${CMAKE_ADD_TARGET_DIR}/../SE1_10b.gro
            DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${INTERNAL_NAME}"
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    install(FILES "${CMAKE_ADD_TARGET_DIR}/../${INTERNAL_NAME}.desktop"
            DESTINATION "${CMAKE_INSTALL_PREFIX}//share/applications"
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    set(A 16)
    set(B 32)
    set(C 48)
    set(D 64)
    set(E 128)
    foreach(X IN LISTS A B C D E)
        #message(STATUS "Create icon ${X}x${X}")
        execute_process (
            COMMAND bash -c "${ImageMagick_convert_EXECUTABLE} ${CMAKE_ADD_TARGET_DIR}/../${INTERNAL_NAME}.png -scale ${X}x${X} ${CMAKE_ADD_TARGET_DIR}/../${X}.png;"
            OUTPUT_VARIABLE outVar
       )
       install(FILES ${CMAKE_ADD_TARGET_DIR}/../${X}.png
               DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${X}x${X}/apps" RENAME "${INTERNAL_NAME}.png"
               PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
	endforeach()
endif()

# end of CMakeLists.txt ...
