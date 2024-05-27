cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(doom3)

# -DCMAKE_TOOLCHAIN_FILE=D:/project/c/vcpkg/scripts/buildsystems/vcpkg.cmake
set(COMPILE_CURL "1")
set(CORE_DEFINES "__DOOM_DLL__")
if(ANDROID)
	add_definitions(-D__ANDROID__)
	add_definitions(-D_NO_PTHREAD_CANCEL)
	set(CORE_DEFINES "${CORE_DEFINES};NO_ALSA;_NO_GAMMA")
elseif(WIN32)
	find_package(CURL QUIET)
	if(CURL_FOUND)
		set(COMPILE_CURL "1")
		include_directories(${CURL_INCLUDE_DIR})
		message(STATUS "libcurl found and enabled")
	else()
		message(WARNING "libcurl not found, server downloads won't be available (apart from that doom3 will work)")
		set(CURL_LIBRARY "")
		set(COMPILE_CURL "0")
	endif()
	set(CORE_DEFINES "${CORE_DEFINES};NO_ALSA")
else()
	set(CORE_DEFINES "${CORE_DEFINES};_NO_GAMMA")
	# apt-get install libasound2-dev # sound alsa support
	# apt-get install libgles-dev # GLES2/GLES3/EGL headers
endif()

add_definitions(-D_K_CLANG) # TODO
add_definitions(-DID_ENABLE_CURL=${COMPILE_CURL})

set(CMAKE_VERBOSE_MAKEFILE ON)

# set(CMAKE_BUILD_TYPE "Debug")
# TODO

# osx: place game .dylib's in the bundle (next to the binary)
# osx: -weak_framework ?

# maybe add these as options:
# TARGET_MONO
# SETUP
# SDK				-D_D3SDK

# don't add these as options, but document them?
# IDNET_HOST		-DIDNET_HOST=\\"%s\\"' % IDNET_HOST
# DEBUG_MEMORY		-DID_DEBUG_MEMORY', '-DID_REDIRECT_NEWDELETE
# LIBC_MALLOC		-DUSE_LIBC_MALLOC=0
# ID_NOLANADDRESS	-DID_NOLANADDRESS

# fallback for cmake versions without add_compile_options
if(NOT COMMAND add_compile_options)
	function(add_compile_options)
		foreach(arg ${ARGN})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${arg}" PARENT_SCOPE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${arg}" PARENT_SCOPE)
		endforeach()
	endfunction()
endif()

option(LINUX_RELEASE_BINS "Set RPATH to \$ORIGIN/libs/ for Linux binary releases" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sys/cmake")

if(LINUX_RELEASE_BINS)
	message(STATUS "Setting RPATH to \$ORIGIN/libs/ so you can put dependencies in there")
	set(CMAKE_SKIP_RPATH OFF CACHE BOOL "Skip RPATH" FORCE)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/libs")
	set(ldflags "${ldflags} -Wl,-z,origin") # not sure this is even needed, but shouldn't hurt
else()
	set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
endif()

set(IDTECH4LIBRARY "idtech4")

include(CheckCXXCompilerFlag)
include(GNUInstallDirs OPTIONAL RESULT_VARIABLE GNUINSTALLDIRS)

if(MSVC)
	option(TOOLS							"Build the tools game code (Visual Studio+SDL2 only)" OFF)
endif()
option(DEDICATED							"Build the dedicated server" 					OFF)
option(ONATIVE								"Optimize for the host CPU" 					OFF)
option(REPRODUCIBLE_BUILD 					"Replace __DATE__ and __TIME__ by hardcoded values for reproducible builds" OFF)

# Extras engine extensions
if(ANDROID)
	option(OPENSLES							"OpenSLES support"								ON)
endif()
option(OPENGLES3							"OpenGLES3 support"								ON)
option(OPENAL								"OpenAL(soft, OpenSL as backend) support"		ON)
option(MULTITHREAD							"Multi-Thread support"							ON)
option(USING_STB							"Using stb"										ON)
option(SUPPORT_OBJ							"Support obj model"								ON)
option(SUPPORT_DAE							"Support dae model" 							ON)
option(SHADOW_MAPPING						"Shadow mapping support" 						ON)
option(NO_LIGHT								"r_noLight support" 							ON)
option(TRANSLUCENT_STENCIL_SHADOW			"Translucent stencil shadow support" 			ON)
option(EXTRAS_TOOLS							"Add extras commands" 							OFF)

# DOOM3 cores
option(CORE									"Build the core" 								ON)
option(BASE									"Build the base game code" 						ON)
option(D3XP									"Build the d3xp game code" 						ON)

# DOOM3 mods
option(D3_MOD_FULL_BODY_AWARENESS			"Build doom3 full body awareness mod support" 	ON)
option(D3LE									"Build the d3le game code" 						ON)
option(CDOOM								"Build the cdoom game code" 					ON)
option(RIVENSIN								"Build the rivensin game code" 					ON)
option(HARDCORPS							"Build the hardcorps game code" 				ON)
option(OVERTHINKED							"Build the overthinked game code" 				ON)
option(SABOT								"Build the sabot game code" 					ON)
option(HEXENEOC								"Build the hexeneoc game code" 					ON)
option(FRAGGINGFREE							"Build the fraggingfree game code" 				ON)
option(LIBRECOOP							"Build the librecoop game code" 				ON)

# Quake4
option(RAVEN								"Build the raven core" 							ON)
option(RAVEN_BSE_FX							"OpenBSE with Fx in Raven Quake 4" 				ON)
option(QUAKE4								"Build the quake4 game code" 					ON)
option(Q4_MOD_FULL_BODY_AWARENESS			"Build quake4 full body awareness mod support" 	ON)
option(Q4_MOD_BOTS							"Build quake4 bots mod support" 				ON)

# Prey 2006
option(HUMANHEAD							"Build the humanhead core" 						ON)
option(PREY									"Build the prey game code" 						ON)
option(PREY_MOD_FULL_BODY_AWARENESS			"Build prey full body awareness mod support" 	ON)

# Print env
if(OPENSLES)
	add_definitions(-D_OPENSLES)
	set(sys_libs ${sys_libs} OpenSLES)
	message(STATUS "Build OpenSLES support")
endif()

if(MULTITHREAD)
	add_definitions(-D_MULTITHREAD)
	message(STATUS "Build Multi-Thread support")
endif()

if(OPENGLES3)
	add_definitions(-D_OPENGLES3)
	message(STATUS "Build OpenGLES3 support")
endif()

if(EXTRAS_TOOLS)
	add_definitions(-D_EXTRAS_TOOLS)
	message(STATUS "Build extras tools support")
endif()

# Using traditional C++98
set(CMAKE_CXX_STANDARD "98")

if(NOT CMAKE_SYSTEM_PROCESSOR)
	message(FATAL_ERROR "No target CPU architecture set")
endif()

if(NOT CMAKE_SYSTEM_NAME)
	message(FATAL_ERROR "No target OS set")
endif()

# target cpu
set(cpu ${CMAKE_SYSTEM_PROCESSOR})

# Originally, ${CMAKE_SYSTEM_PROCESSOR} was supposed to contain the *target* CPU, according to CMake's documentation.
# As far as I can tell this has always been broken (always returns host CPU) at least on Windows
# (see e.g. https://cmake.org/pipermail/cmake-developers/2014-September/011405.html) and wasn't reliable on
# other systems either, for example on Linux with 32bit userland but 64bit kernel it returned the kernel CPU type
# (e.g. x86_64 instead of i686). Instead of fixing this, CMake eventually updated their documentation in 3.20,
# now it's officially the same as CMAKE_HOST_SYSTEM_PROCESSOR except when cross-compiling (where it's explicitly set)
# So we gotta figure out the actual target CPU type ourselves.. (why am I sticking to this garbage buildsystem?)
if(NOT (CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR))
	# special case: cross-compiling, here CMAKE_SYSTEM_PROCESSOR should be correct, hopefully
	# (just leave cpu at ${CMAKE_SYSTEM_PROCESSOR})
else() # not MSVC and not cross-compiling, assume GCC or clang (-compatible), seems to work for MinGW as well
	execute_process(COMMAND ${CMAKE_C_COMPILER} "-dumpmachine"
	                RESULT_VARIABLE cc_dumpmachine_res
	                OUTPUT_VARIABLE cc_dumpmachine_out)
	if(cc_dumpmachine_res EQUAL 0)
		string(STRIP ${cc_dumpmachine_out} cc_dumpmachine_out) # get rid of trailing newline
		message(STATUS "`${CMAKE_C_COMPILER} -dumpmachine` says: \"${cc_dumpmachine_out}\"")
		# gcc -dumpmachine and clang -dumpmachine seem to print something like "x86_64-linux-gnu" (gcc)
		# or "x64_64-pc-linux-gnu" (clang) or "i686-w64-mingw32" (32bit mingw-w64) i.e. starting with the CPU,
		# then "-" and then OS or whatever - so use everything up to first "-"
		string(REGEX MATCH "^[^-]+" cpu ${cc_dumpmachine_out})
		message(STATUS "  => CPU architecture extracted from that: \"${cpu}\"")
	else()
		message(WARNING "${CMAKE_C_COMPILER} -dumpmachine failed with error (code) ${cc_dumpmachine_res}")
		message(WARNING "will use the (sometimes incorrect) CMAKE_SYSTEM_PROCESSOR (${cpu}) to determine D3_ARCH")
	endif()
endif()

if(cpu STREQUAL "powerpc")
	set(cpu "ppc")
elseif(cpu STREQUAL "aarch64")
	# "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
	set(cpu "arm64")
elseif(cpu MATCHES "i.86")
	set(cpu "x86")
elseif(cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX]64")
	set(cpu "x86_64")
elseif(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
	# On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
	# while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
		set(cpu "arm64")
	else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
		set(cpu "arm")
	endif()
endif()

add_definitions(-DD3_ARCH="${cpu}" -DD3_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})
# Stores in variable <var> either 1 or 0 indicating whether the target architecture is big or little endian.
# test_big_endian(big_endian)
if(CMAKE_CXX_BYTE_ORDER EQUAL BIG_ENDIAN)
	set(big_endian, "1")
else()
	set(big_endian, "0")
endif()
add_definitions(-BUILD_IS_BIG_ENDIAN="${big_endian}")

# target os
if(APPLE)
	set(os "macosx")
else()
	string(TOLOWER "${CMAKE_SYSTEM_NAME}" os)
endif()

add_definitions(-DD3_OSTYPE="${os}")

message(STATUS "Setting -DD3_ARCH=\"${cpu}\" -DD3_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P} -DD3_OSTYPE=\"${os}\" ")

if(cpu MATCHES ".*64.*" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	# tough luck if some CPU architecture has "64" in its name but uses 32bit pointers
	message(SEND_ERROR "CMake thinks sizeof(void*) == 4, but the target CPU looks like a 64bit CPU!")
	message(FATAL_ERROR "If you're building in a 32bit chroot on a 64bit host, switch to it with 'linux32 chroot' or at least call cmake with linux32 (or your OSs equivalent)!")
endif()

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# precompiled libraries from the dhewm3-libs repo
#[[if(DHEWM3LIBS)
	if(CMAKE_CROSSCOMPILING)
		set(CMAKE_FIND_ROOT_PATH ${DHEWM3LIBS})
	else()
		set(ENV{CMAKE_PREFIX_PATH} ${DHEWM3LIBS})
	endif()

	# these are too stupid, give them a hint
	set(ENV{OPENALDIR} ${DHEWM3LIBS})
	set(ENV{SDLDIR} ${DHEWM3LIBS})
	set(ENV{SDL2DIR} ${DHEWM3LIBS})
endif()]]

if(CMAKE_MAJOR_VERSION LESS 3 OR ( CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION LESS 8 ))
	# cmake < 3.8 doesn't support source_group(TREE ...) so replace it with a dummy
	# (it's only cosmetical anyway, to make source files show up properly in Visual Studio)
	function(source_group)
	endfunction()
	message(STATUS "Using CMake < 3.8, doesn't support source_group(TREE ...), replacing it with a dummy")
	message(STATUS "  (this is only relevants for IDEs, doesn't matter for just compiling doom3)")
#else()
#	message(STATUS "Using CMake >= 3.8, supports source_group(TREE ...)")
endif()

# libs
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

# OpenAL using internal OpenAL-soft
#[[find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})]]


if(WIN32)
	# skip SDL2main
	if(APPLE OR WIN32)
		set(SDL2_BUILDING_LIBRARY TRUE)
	endif()
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})
	set(SDLx_LIBRARY ${SDL2_LIBRARIES})
	add_definitions(-D_SDL)
	include_directories("externlibs/include")
	set(sys_libs ${sys_libs} ${SDLx_LIBRARY})
endif()

if(REPRODUCIBLE_BUILD)
	# don't use __DATE__ and __TIME__ macros so builds are reproducible
	add_definitions(-DID_REPRODUCIBLE_BUILD)
endif()


# cURL using internal
#[[find_package(CURL QUIET)
if(CURL_FOUND)
	set(ID_ENABLE_CURL ON)
	include_directories(${CURL_INCLUDE_DIR})
	message(STATUS "libcurl found and enabled")
else()
	message(WARNING "libcurl not found, server downloads won't be available (apart from that doom3 will work)")
	set(ID_ENABLE_CURL OFF)
	set(CURL_LIBRARY "")
endif()]]

if(MSVC)
	# This is required for tools on windows.
	find_package(MFC)

	if(TOOLS AND NOT MFC_FOUND)
		message(SEND_ERROR "MFC ('Microsoft Foundation Classes for C++') couldn't be found, but is needed for TOOLS!")
		message(FATAL_ERROR "If you're using VS2013, you'll also need the 'Multibyte MFC Library for Visual Studio 2013': https://www.microsoft.com/en-us/download/details.aspx?id=40770 (VS2015 and 2017 include that in the default MFC package)")
	endif()

else() # not MSVC

	if(NOT WIN32 AND NOT ANDROID)
		# libbacktrace support - TODO: might work with MinGW? we don't have a crash handler for win32 though..
		include(CheckCSourceCompiles)
		set(CMAKE_REQUIRED_LIBRARIES backtrace)
		check_c_source_compiles( "#include <backtrace.h>
	int main() { backtrace_create_state(NULL, 0, NULL, NULL); return 0; }" HAVE_LIBBACKTRACE )
		unset(CMAKE_REQUIRED_LIBRARIES)

		if(HAVE_LIBBACKTRACE)
			set(sys_libs ${sys_libs} backtrace)
			add_definitions(-DD3_HAVE_LIBBACKTRACE)
			message(STATUS "Using libbacktrace")
		else()
			message(WARNING "libbacktrace wasn't found. It's not required but recommended, because it provides useful backtraces if doom3 crashes")
		endif()
	endif() # NOT WIN32

endif() # not MSVC

# compiler specific flags
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-pipe)
	#k add_compile_options(-Wall)

	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		add_compile_options(-march=native)
	elseif(NOT APPLE AND cpu STREQUAL "x86")
		add_compile_options(-march=pentium3)
	endif()

	set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -D_DEBUG -O0 -fPIC")
	set(CMAKE_C_FLAGS_DEBUGALL "-g -ggdb -D_DEBUG -fPIC")
	set(CMAKE_C_FLAGS_PROFILE "-g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer -fPIC")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -fno-math-errno -fno-trapping-math -fomit-frame-pointer -fPIC")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -ggdb -O2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer -fPIC")
	set(CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-math-errno -fno-trapping-math -fomit-frame-pointer -fPIC")

	set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})

	add_compile_options(-fno-strict-aliasing)
	# dear idiot compilers, don't fuck up math code with useless FMA "optimizations"
	# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100839)
	add_compile_options(-ffp-contract=off)

	if(NOT AROS)
		CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" cxx_has_fvisibility)
		if(NOT cxx_has_fvisibility)
			message(FATAL_ERROR "Compiler does not support -fvisibility")
		endif()
		add_compile_options(-fvisibility=hidden)
	endif()

	# TODO fix these warnings
	add_compile_options(-Wno-sign-compare)
	add_compile_options(-Wno-switch)
	add_compile_options(-Wno-format-security)

	#k ignore these
	add_compile_options(-Wno-unknown-pragmas)
	add_compile_options(-Wno-unused-private-field)
	add_compile_options(-Wno-unused-variable)
	#add_compile_options(-Wall)
	#add_compile_options(-w)
	add_compile_options(-Wno-local-type-template-args -Wno-parentheses-equality -Wno-unused-value)
	add_compile_options(-Wno-self-assign-field -Wno-logical-not-parentheses -Wno-overloaded-virtual -Wno-c++11-compat-deprecated-writable-strings)
	add_compile_options(-Wno-undefined-inline -Wno-null-conversion -Wno-extra-tokens -Wno-c++11-extensions)
	if(NOT ANDROID)
		CHECK_CXX_COMPILER_FLAG("-Woverloaded-virtual" cxx_has_Woverload_virtual)
		if(cxx_has_Woverload_virtual)
			add_compile_options(-Woverloaded-virtual)
		endif()
	endif()

	# shut up about using memcpy() on classes, in the cases doom3 uses it it seems to be fine
	CHECK_CXX_COMPILER_FLAG("-Wno-class-memaccess" cxx_has_Wno-class-memaccess)
	if(cxx_has_Wno-class-memaccess)
		add_compile_options(-Wno-class-memaccess)
	endif()

	if(AROS)
		set(CMAKE_SHARED_LIBRARY_SUFFIX ".aros-${cpu}")
		add_definitions(-DIOAPI_NO_64)
	elseif(APPLE)
		add_definitions(-DMACOS_X=1)

		if(cpu STREQUAL "x86_64")
			add_compile_options(-arch x86_64 -mmacosx-version-min=10.9)
			set(ldflags "${ldflags} -arch x86_64 -mmacosx-version-min=10.9")
		elseif(cpu STREQUAL "arm64")
			add_compile_options(-arch arm64 -mmacosx-version-min=11.0)
			set(ldflags "${ldflags} -arch arm64 -mmacosx-version-min=11.0")
		elseif(cpu STREQUAL "x86")
			CHECK_CXX_COMPILER_FLAG("-arch i386" cxx_has_arch_i386)
			if(cxx_has_arch_i386)
				add_compile_options(-arch i386)
				set(ldflags "${ldflags} -arch i386")
			endif()

			add_compile_options(-mmacosx-version-min=10.4)
			set(ldflags "${ldflags} -mmacosx-version-min=10.4")
		elseif(cpu STREQUAL "ppc")
			CHECK_CXX_COMPILER_FLAG("-arch ppc" cxx_has_arch_ppc)
			if(cxx_has_arch_ppc)
				add_compile_options(-arch ppc -mone-byte-bool)
				set(ldflags "${ldflags} -arch ppc -mone-byte-bool")
			endif()

			add_compile_options(-mmacosx-version-min=10.4)
			set(ldflags "${ldflags} -mmacosx-version-min=10.4")
		else()
			message(FATAL_ERROR "Unsupported CPU architecture for OSX")
		endif()

		set(sys_libs ${sys_libs} "-framework Carbon -framework Cocoa -framework IOKit")
	elseif(WIN32)
		set(ldflags "${ldflags} -static-libgcc -static-libstdc++")
	elseif(os STREQUAL "linux")
		set(sys_libs ${sys_libs} dl)
	elseif(os STREQUAL "freebsd")
		find_library(EXECINFO_LIBRARIES execinfo /usr/lib /usr/local/lib)
		set(sys_libs ${sys_libs} ${EXECINFO_LIBRARIES})
	endif()
elseif(MSVC)
	add_compile_options(/MP) # parallel build (use all cores, or as many as configured in VS)

	add_compile_options(/W3) # TODO: was /W4, caused trouble with VS2019 (and/or its integrated CMake? or only HarrieVG's setup?)
	add_compile_options(/we4840) # treat as error when passing a class to a vararg-function (probably printf-like)
	# treat several kinds of truncating int<->pointer conversions as errors (for more 64bit-safety)
	add_compile_options(/we4306 /we4311 /we4312 /we4302)
	# ignore the following warnings:
	add_compile_options(/wd4100) # unreferenced formal parameter
	add_compile_options(/wd4127) # conditional expression is constant
	add_compile_options(/wd4244) # possible loss of data
	add_compile_options(/wd4245) # signed/unsigned mismatch
	add_compile_options(/wd4267) # possible loss of data
	add_compile_options(/wd4714) # 'function' marked as __forceinline not inlined
	add_compile_options(/wd4996) # 'function': was declared deprecated
	add_compile_options(/wd4068) # unknown pragma
	add_compile_options(/wd4458) # declaration of 'variable' hides class member
	add_definitions(-D_ALLOW_KEYWORD_MACROS) # because of the "#define private public" and "#define protected public" in TypeInfo.cpp
	set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG /Od /Zi /MDd")
	set(CMAKE_C_FLAGS_RELEASE "/Ox /Oy /MD")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Oy /Zi /MD")
	set(CMAKE_C_FLAGS_MINSIZEREL "/Ox /Oy /Os /MD")
else()
	message(FATAL_ERROR "Unsupported compiler")
endif()

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
# disable assertions in release builds
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG ${CMAKE_C_FLAGS_MINSIZEREL}")

# mingw and msvc
if(WIN32)
	add_definitions(-DWINVER=0x0501)
	add_definitions(-D_WIN32_WINNT=0x0501)

	set(sys_libs ${sys_libs}
			winmm
			iphlpapi
			wsock32
			ole32
			dsound ksuser #k
			)
elseif(ANDROID)
	set(sys_libs ${sys_libs} android dl EGL z)
else() #TODO linux
	set(sys_libs ${sys_libs} X11 dl EGL z pthread)
endif()

# fallback for cmake versions without GNUInstallDirs
if(GNUINSTALLDIRS MATCHES "NOTFOUND")
	set(CMAKE_INSTALL_BINDIR		"bin"
			CACHE PATH "user executables (bin)")
	set(CMAKE_INSTALL_LIBDIR		"lib${LIB_SUFFIX}"
			CACHE PATH "object code libraries (lib${LIB_SUFFIX})")
	set(CMAKE_INSTALL_DATAROOTDIR	"share"
			CACHE PATH "read-only architecture-independent data root (share)")
	set(CMAKE_INSTALL_DATADIR		"${CMAKE_INSTALL_DATAROOTDIR}"
			CACHE PATH "read-only architecture-independent data (DATAROOTDIR)")

	mark_as_advanced(CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_DATAROOTDIR CMAKE_INSTALL_DATADIR)
	foreach(dir BINDIR LIBDIR DATAROOTDIR DATADIR)
		if(NOT IS_ABSOLUTE ${CMAKE_INSTALL_${dir}})
			set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_${dir}}")
		else()
			set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_${dir}}")
		endif()
	endforeach()
endif()

set(bindir		"${CMAKE_INSTALL_FULL_BINDIR}")
set(libdir		"${CMAKE_INSTALL_FULL_LIBDIR}")
set(datadir		"${CMAKE_INSTALL_FULL_DATADIR}")

#[[configure_file(
		"${CMAKE_SOURCE_DIR}/config.h.in"
		"${CMAKE_BINARY_DIR}/config.h"
)]]

message(STATUS "Building ${CMAKE_BUILD_TYPE} for ${os}-${cpu}")

if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
	message(STATUS "The install target will use the following directories:")
	message(STATUS "  Binary directory:  ${bindir}")
	message(STATUS "  Library directory: ${libdir}")
	message(STATUS "  Data directory:    ${datadir}")
endif()

# I'm a bit sloppy with headers and just glob them in..
# they're only handled in CMake at all so they turn up in Visual Studio solutions..

# globs all the headers from ${PATHPREFIX}/ and adds them to ${SRCLIST}
function(add_globbed_headers SRCLIST PATHPREFIX)
	file(GLOB_RECURSE tmp_hdrs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${PATHPREFIX}/*.h")
	set(${SRCLIST} ${tmp_hdrs} ${${SRCLIST}})
endfunction()

set(local_libs static_ogg)
if(COMPILE_CURL STREQUAL "1")
	if(NOT WIN32)
		set(local_libs ${local_libs} static_curl)
	else()
		set(sys_libs ${sys_libs} ${CURL_LIBRARY})
	endif()
	message(STATUS "Build cURL")
endif()

if(USING_STB)
	set(CORE_DEFINES "${CORE_DEFINES};_USING_STB")
	message(STATUS "Using stb jpeg image")
endif()
if(SUPPORT_OBJ)
	set(CORE_DEFINES "${CORE_DEFINES};_MODEL_OBJ")
	message(STATUS "Support obj model")
endif()
if(SUPPORT_DAE)
	set(CORE_DEFINES "${CORE_DEFINES};_MODEL_DAE")
	message(STATUS "Support dae model")
endif()
if(SHADOW_MAPPING)
	set(CORE_DEFINES "${CORE_DEFINES};_SHADOW_MAPPING")
	message(STATUS "Add shadow mapping support")
endif()
if(NO_LIGHT)
	set(CORE_DEFINES "${CORE_DEFINES};_NO_LIGHT")
	message(STATUS "Add r_noLight support")
endif()
if(TRANSLUCENT_STENCIL_SHADOW)
	set(CORE_DEFINES "${CORE_DEFINES};_TRANSLUCENT_STENCIL_SHADOW")
	message(STATUS "Add translucent stencil shadow support")
endif()

if(OPENAL)
	if(ANDROID)
		add_subdirectory (externlibs/openal-soft)
	endif()
	add_definitions(-D_OPENAL -D_OPENAL_EFX -D_OPENAL_SOFT)
	message(STATUS "Build OpenAL(soft) support")
endif()

#//k jpeg
set(src_jpeg
		externlibs/jpeg-8d/jaricom.c
		externlibs/jpeg-8d/jcapimin.c
		externlibs/jpeg-8d/jcapistd.c
		externlibs/jpeg-8d/jcarith.c
		externlibs/jpeg-8d/jccoefct.c
		externlibs/jpeg-8d/jccolor.c
		externlibs/jpeg-8d/jcdctmgr.c
		externlibs/jpeg-8d/jchuff.c
		externlibs/jpeg-8d/jcinit.c
		externlibs/jpeg-8d/jcmainct.c
		externlibs/jpeg-8d/jcmarker.c
		externlibs/jpeg-8d/jcmaster.c
		externlibs/jpeg-8d/jcomapi.c
		externlibs/jpeg-8d/jcparam.c
		externlibs/jpeg-8d/jcprepct.c
		externlibs/jpeg-8d/jcsample.c
		externlibs/jpeg-8d/jctrans.c
		externlibs/jpeg-8d/jdapimin.c
		externlibs/jpeg-8d/jdapistd.c
		externlibs/jpeg-8d/jdarith.c
		externlibs/jpeg-8d/jdatadst.c
		externlibs/jpeg-8d/jdatasrc.c
		externlibs/jpeg-8d/jdcoefct.c
		externlibs/jpeg-8d/jdcolor.c
		externlibs/jpeg-8d/jddctmgr.c
		externlibs/jpeg-8d/jdhuff.c
		externlibs/jpeg-8d/jdinput.c
		externlibs/jpeg-8d/jdmainct.c
		externlibs/jpeg-8d/jdmarker.c
		externlibs/jpeg-8d/jdmaster.c
		externlibs/jpeg-8d/jdmerge.c
		externlibs/jpeg-8d/jdpostct.c
		externlibs/jpeg-8d/jdsample.c
		externlibs/jpeg-8d/jdtrans.c
		externlibs/jpeg-8d/jerror.c
		externlibs/jpeg-8d/jfdctflt.c
		externlibs/jpeg-8d/jfdctfst.c
		externlibs/jpeg-8d/jfdctint.c
		externlibs/jpeg-8d/jidctflt.c
		externlibs/jpeg-8d/jidctfst.c
		externlibs/jpeg-8d/jidctint.c
		externlibs/jpeg-8d/jmemmgr.c
		externlibs/jpeg-8d/jmemname.c
		externlibs/jpeg-8d/jquant1.c
		externlibs/jpeg-8d/jquant2.c
		externlibs/jpeg-8d/jutils.c
		)

add_globbed_headers(src_jpeg "externlibs/jpeg-8d")
include_directories("externlibs/jpeg-8d")
add_library(static_jpeg STATIC ${src_jpeg})

set(local_libs ${local_libs} static_jpeg)

set(src_renderer
	renderer/Cinematic.cpp
	renderer/GuiModel.cpp
	renderer/Image_files.cpp
	renderer/Image_init.cpp
	renderer/Image_load.cpp
	renderer/Image_process.cpp
	renderer/Image_program.cpp
	renderer/Interaction.cpp
	renderer/Material.cpp
	renderer/Model.cpp
	renderer/ModelDecal.cpp
	renderer/ModelManager.cpp
	renderer/ModelOverlay.cpp
	renderer/Model_beam.cpp
	renderer/Model_ase.cpp
	renderer/Model_liquid.cpp
	renderer/Model_lwo.cpp
	renderer/Model_ma.cpp
	renderer/Model_md3.cpp
	renderer/Model_md5.cpp
	renderer/Model_prt.cpp
	renderer/Model_sprite.cpp
	renderer/RenderEntity.cpp
	renderer/RenderSystem.cpp
	renderer/RenderSystem_init.cpp
	renderer/RenderWorld.cpp
	renderer/RenderWorld_demo.cpp
	renderer/RenderWorld_load.cpp
	renderer/RenderWorld_portals.cpp
	renderer/VertexCache.cpp
	renderer/draw_common.cpp
	renderer/draw_glsl.cpp
	renderer/tr_backend.cpp
	renderer/tr_deform.cpp
	renderer/tr_font.cpp
	renderer/tr_guisurf.cpp
	renderer/tr_light.cpp
	renderer/tr_lightrun.cpp
	renderer/tr_main.cpp
	renderer/tr_orderIndexes.cpp
	renderer/tr_polytope.cpp
	renderer/tr_render.cpp
	renderer/tr_shadowbounds.cpp
	renderer/tr_stencilshadow.cpp
	renderer/tr_subview.cpp
	renderer/tr_trace.cpp
	renderer/tr_trisurf.cpp
	renderer/tr_turboshadow.cpp
	renderer/etc/etc_android.cpp
	renderer/etc/etc_rg_etc1.cpp
	renderer/tr_rendertools.cpp
    renderer/matrix/esTransform.c
)

add_globbed_headers(src_renderer "renderer")

set(src_framework
	framework/CVarSystem.cpp
	framework/CmdSystem.cpp
	framework/Common.cpp
	framework/Compressor.cpp
	framework/Console.cpp
	framework/DemoFile.cpp
	framework/DeclAF.cpp
	framework/DeclEntityDef.cpp
	framework/DeclFX.cpp
	framework/DeclManager.cpp
	framework/DeclParticle.cpp
	framework/DeclPDA.cpp
	framework/DeclSkin.cpp
	framework/DeclTable.cpp
	framework/EditField.cpp
	framework/EventLoop.cpp
	framework/File.cpp
	framework/FileSystem.cpp
	framework/KeyInput.cpp
	framework/UsercmdGen.cpp
	framework/Session_menu.cpp
	framework/Session.cpp
	framework/Unzip.cpp
	framework/async/AsyncClient.cpp
	framework/async/AsyncNetwork.cpp
	framework/async/AsyncServer.cpp
	framework/async/MsgChannel.cpp
	framework/async/NetworkSystem.cpp
	framework/async/ServerScan.cpp
)

add_globbed_headers(src_framework "framework")

set(src_cm
	cm/CollisionModel_contacts.cpp
	cm/CollisionModel_contents.cpp
	cm/CollisionModel_debug.cpp
	cm/CollisionModel_files.cpp
	cm/CollisionModel_load.cpp
	cm/CollisionModel_rotate.cpp
	cm/CollisionModel_trace.cpp
	cm/CollisionModel_translate.cpp
)

add_globbed_headers(src_cm "cm")

set(src_dmap
	tools/compilers/dmap/dmap.cpp
	tools/compilers/dmap/facebsp.cpp
	tools/compilers/dmap/gldraw.cpp
	tools/compilers/dmap/glfile.cpp
	tools/compilers/dmap/leakfile.cpp
	tools/compilers/dmap/map.cpp
	tools/compilers/dmap/optimize.cpp
	tools/compilers/dmap/output.cpp
	tools/compilers/dmap/portals.cpp
	tools/compilers/dmap/shadowopt3.cpp
	tools/compilers/dmap/tritjunction.cpp
	tools/compilers/dmap/tritools.cpp
	tools/compilers/dmap/ubrush.cpp
	tools/compilers/dmap/usurface.cpp
	tools/compilers/dmap/optimize_gcc.cpp
)

add_globbed_headers(src_dmap "tools/compilers/dmap")

set(src_aas
	tools/compilers/aas/AASBuild.cpp
	tools/compilers/aas/AASBuild_file.cpp
	tools/compilers/aas/AASBuild_gravity.cpp
	tools/compilers/aas/AASBuild_ledge.cpp
	tools/compilers/aas/AASBuild_merge.cpp
	tools/compilers/aas/AASCluster.cpp
	tools/compilers/aas/AASFile.cpp
	tools/compilers/aas/AASFile_optimize.cpp
	tools/compilers/aas/AASFile_sample.cpp
	tools/compilers/aas/AASReach.cpp
	tools/compilers/aas/AASFileManager.cpp
	tools/compilers/aas/Brush.cpp
	tools/compilers/aas/BrushBSP.cpp
)

add_globbed_headers(src_aas "tools/compilers/aas")

set(src_roq
	tools/compilers/roqvq/NSBitmapImageRep.cpp
	tools/compilers/roqvq/codec.cpp
	tools/compilers/roqvq/roq.cpp
	tools/compilers/roqvq/roqParam.cpp
)

add_globbed_headers(src_roq "tools/compilers/roqvq")

set(src_renderbump
	tools/compilers/renderbump/renderbump.cpp
)

add_globbed_headers(src_renderbump "tools/compilers/renderbump")

set(src_snd
	sound/snd_cache.cpp
	sound/snd_decoder.cpp
	sound/snd_efxfile.cpp
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_wavefile.cpp
	sound/snd_world.cpp
)

add_globbed_headers(src_snd "sound")

set(src_ui
	ui/BindWindow.cpp
	ui/ChoiceWindow.cpp
	ui/DeviceContext.cpp
	ui/EditWindow.cpp
	ui/FieldWindow.cpp
	ui/GameBearShootWindow.cpp
	ui/GameBustOutWindow.cpp
	ui/GameSSDWindow.cpp
	ui/GuiScript.cpp
	ui/ListGUI.cpp
	ui/ListWindow.cpp
	ui/MarkerWindow.cpp
	ui/RegExp.cpp
	ui/RenderWindow.cpp
	ui/SimpleWindow.cpp
	ui/SliderWindow.cpp
	ui/UserInterface.cpp
	ui/Window.cpp
	ui/Winvar.cpp
)

add_globbed_headers(src_ui "ui")

set(src_idlib
	idlib/bv/Bounds.cpp
	idlib/bv/Frustum.cpp
	idlib/bv/Sphere.cpp
	idlib/bv/Box.cpp
	idlib/geometry/DrawVert.cpp
	idlib/geometry/Winding2D.cpp
	idlib/geometry/Surface_SweptSpline.cpp
	idlib/geometry/Winding.cpp
	idlib/geometry/Surface.cpp
	idlib/geometry/Surface_Patch.cpp
	idlib/geometry/TraceModel.cpp
	idlib/geometry/JointTransform.cpp
	idlib/hashing/CRC32.cpp
	idlib/hashing/MD4.cpp
	idlib/hashing/MD5.cpp
	idlib/math/Angles.cpp
	idlib/math/Lcp.cpp
	idlib/math/Math.cpp
	idlib/math/Matrix.cpp
	idlib/math/Ode.cpp
	idlib/math/Plane.cpp
	idlib/math/Pluecker.cpp
	idlib/math/Polynomial.cpp
	idlib/math/Quat.cpp
	idlib/math/Rotation.cpp
	idlib/math/Simd.cpp
	idlib/math/Simd_Generic.cpp
	idlib/math/Vector.cpp
	idlib/BitMsg.cpp
	idlib/LangDict.cpp
	idlib/Lexer.cpp
	idlib/Lib.cpp
	idlib/containers/HashIndex.cpp
	idlib/Dict.cpp
	idlib/Str.cpp
	idlib/Parser.cpp
	idlib/MapFile.cpp
	idlib/CmdArgs.cpp
	idlib/Token.cpp
	idlib/Base64.cpp
	idlib/Timer.cpp
	idlib/Heap.cpp
	idlib/bv/Frustum_gcc.cpp
)

add_globbed_headers(src_idlib "idlib")

set(base_game_src
		AF.cpp
		AFEntity.cpp
		Actor.cpp
		Camera.cpp
		Entity.cpp
		BrittleFracture.cpp
		Fx.cpp
		GameEdit.cpp
		Game_local.cpp
		Game_network.cpp
		Item.cpp
		IK.cpp
		Light.cpp
		Misc.cpp
		Mover.cpp
		Moveable.cpp
		MultiplayerGame.cpp
		Player.cpp
		PlayerIcon.cpp
		PlayerView.cpp
		Projectile.cpp
		Pvs.cpp
		SecurityCamera.cpp
		SmokeParticles.cpp
		Sound.cpp
		Target.cpp
		Trigger.cpp
		Weapon.cpp
		WorldSpawn.cpp
		ai/AAS.cpp
		ai/AAS_debug.cpp
		ai/AAS_pathing.cpp
		ai/AAS_routing.cpp
		ai/AI.cpp
		ai/AI_events.cpp
		ai/AI_pathing.cpp
		ai/AI_Vagary.cpp
		gamesys/DebugGraph.cpp
		gamesys/Class.cpp
		gamesys/Event.cpp
		gamesys/SaveGame.cpp
		gamesys/SysCmds.cpp
		gamesys/SysCvar.cpp
		gamesys/TypeInfo.cpp
		anim/Anim.cpp
		anim/Anim_Blend.cpp
		anim/Anim_Import.cpp
		anim/Anim_Testmodel.cpp
		script/Script_Compiler.cpp
		script/Script_Interpreter.cpp
		script/Script_Program.cpp
		script/Script_Thread.cpp
		physics/Clip.cpp
		physics/Force.cpp
		physics/Force_Constant.cpp
		physics/Force_Drag.cpp
		physics/Force_Field.cpp
		physics/Force_Spring.cpp
		physics/Physics.cpp
		physics/Physics_AF.cpp
		physics/Physics_Actor.cpp
		physics/Physics_Base.cpp
		physics/Physics_Monster.cpp
		physics/Physics_Parametric.cpp
		physics/Physics_Player.cpp
		physics/Physics_RigidBody.cpp
		physics/Physics_Static.cpp
		physics/Physics_StaticMulti.cpp
		physics/Push.cpp
		)

function(set_src_dir DSTLIST PATHPREFIX SRCLIST)
	set(list, "")
	math(EXPR last_index "${ARGC}-1")
	foreach(index RANGE 2 ${last_index})
		list(APPEND list "${PATHPREFIX}/${ARGV${index}}")
	endforeach()
	set(${DSTLIST} ${list} PARENT_SCOPE)
endfunction()

set(MOD_DIR "mod")
set(DOOM3_MOD_DIR "${MOD_DIR}/doom3")
set(QUAKE4_MOD_DIR "${MOD_DIR}/quake4")
set(PREY_MOD_DIR "${MOD_DIR}/prey")

include_directories("externlibs")

if(NOT WIN32)
	if(COMPILE_CURL STREQUAL "1")
		#//k curl
		set(src_curl
			externlibs/curl/lib/base64.c
			externlibs/curl/lib/connect.c
			externlibs/curl/lib/cookie.c
			externlibs/curl/lib/dict.c
			externlibs/curl/lib/easy.c
			externlibs/curl/lib/escape.c
			externlibs/curl/lib/file.c
			externlibs/curl/lib/formdata.c
			externlibs/curl/lib/ftp.c
			externlibs/curl/lib/getdate.c
			externlibs/curl/lib/getenv.c
			externlibs/curl/lib/getinfo.c
			externlibs/curl/lib/hash.c
			externlibs/curl/lib/hostip.c
			externlibs/curl/lib/http.c
			externlibs/curl/lib/http_chunks.c
			externlibs/curl/lib/if2ip.c
			externlibs/curl/lib/krb4.c
			externlibs/curl/lib/ldap.c
			externlibs/curl/lib/llist.c
			externlibs/curl/lib/memdebug.c
			externlibs/curl/lib/mprintf.c
			externlibs/curl/lib/multi.c
			externlibs/curl/lib/netrc.c
			externlibs/curl/lib/progress.c
			externlibs/curl/lib/security.c
			externlibs/curl/lib/sendf.c
			externlibs/curl/lib/share.c
			externlibs/curl/lib/speedcheck.c
			externlibs/curl/lib/ssluse.c
			externlibs/curl/lib/strequal.c
			externlibs/curl/lib/strtok.c
			externlibs/curl/lib/telnet.c
			externlibs/curl/lib/timeval.c
			externlibs/curl/lib/transfer.c
			externlibs/curl/lib/url.c
			externlibs/curl/lib/version.c
			externlibs/curl/lib/http_digest.c
			externlibs/curl/lib/http_ntlm.c
			externlibs/curl/lib/inet_pton.c
			externlibs/curl/lib/md5.c
			externlibs/curl/lib/content_encoding.c
		)

		add_globbed_headers(src_curl "externlibs/curl/lib")
		include_directories("externlibs/curl/lib")
		include_directories("externlibs/curl/include")
		add_library(static_curl STATIC ${src_curl})
		set(CURL_DEFINES "HAVE_CONFIG_H") # without autoconf
		INCLUDE(CheckFunctionExists)
		check_function_exists(strlcat HAVE_STRLCAT)
		if(NOT HAVE_STRLCAT)
			set(CURL_DEFINES "${CURL_DEFINES};_NOT_HAVE_STRLCAT")
			message(STATUS "No strlcat for cURL")
		else()
			message(STATUS "Have strlcat for cURL")
		endif()
		set_target_properties(static_curl PROPERTIES COMPILE_DEFINITIONS "${CURL_DEFINES}")
		target_link_libraries(static_curl z)
	endif()
endif()

# Build internal ogg/vorbis
set(src_ogg
	externlibs/libogg/src/bitwise.c
	externlibs/libogg/src/framing.c
	externlibs/libvorbis/lib/analysis.c
	externlibs/libvorbis/lib/floor1.c
	externlibs/libvorbis/lib/psy.c
	# externlibs/libvorbis/lib/tone.c
	# externlibs/libvorbis/lib/barkmel.c
	externlibs/libvorbis/lib/info.c
	externlibs/libvorbis/lib/vorbisenc.c
	externlibs/libvorbis/lib/bitrate.c
	externlibs/libvorbis/lib/lookup.c
	externlibs/libvorbis/lib/registry.c
	externlibs/libvorbis/lib/vorbisfile.c
	externlibs/libvorbis/lib/block.c
	externlibs/libvorbis/lib/lpc.c
	externlibs/libvorbis/lib/res0.c
	externlibs/libvorbis/lib/window.c
	externlibs/libvorbis/lib/codebook.c
	externlibs/libvorbis/lib/lsp.c
	externlibs/libvorbis/lib/sharedbook.c
	externlibs/libvorbis/lib/envelope.c
	externlibs/libvorbis/lib/mapping0.c
	externlibs/libvorbis/lib/smallft.c
	externlibs/libvorbis/lib/floor0.c
	externlibs/libvorbis/lib/mdct.c
	externlibs/libvorbis/lib/synthesis.c
)

add_globbed_headers(src_ogg "externlibs/libogg")
add_globbed_headers(src_ogg "externlibs/libvorbis")
include_directories("externlibs/libogg/include")
include_directories("externlibs/libvorbis/include")
include_directories("sys/android")
add_library(static_ogg STATIC ${src_ogg})
target_include_directories(static_ogg PRIVATE "externlibs/libogg/include/ogg")
target_include_directories(static_ogg PRIVATE "externlibs/libvorbis/include/vorbis")
target_include_directories(static_ogg PRIVATE "externlibs/libvorbis/lib")

set(src_core
	${src_renderer}
	${src_framework}
	${src_cm}
	${src_aas}
	${src_roq}
	${src_snd}
	${src_ui}
	${src_tools}
	${src_renderbump}
	${src_dmap}
)

set(src_stub_openal		sys/stub/openal_stub.cpp)
set(src_stub_gl			sys/stub/stub_gl.cpp)


set(src_tools
	tools/guied/GEWindowWrapper_stub.cpp
)

# TODO: add_globbed_headers(src_tools "tools/guied")

# Begin normal tools code

set(src_sys_base
	sys/linux/stack.cpp
	sys/stub/util_stub.cpp
	sys/sys_local.cpp
	sys/posix/posix_net.cpp
	sys/posix/posix_main.cpp
	sys/posix/posix_threads.cpp
	sys/posix/posix_signal.cpp
	sys/posix/posix_input.cpp
	sys/linux/gl_extensions.cpp
)

if(ANDROID)
	set(src_sys_base
		${src_sys_base}
		sys/android/main.cpp
		sys/android/sound.cpp
		sys/android/input.cpp
	)
	set(src_sys_core
		sys/android/glimp.cpp
	)
elseif(WIN32)
	set(src_sys_base
			sys/sys_local.cpp
			sys/sdl/threads.cpp
			sys/sdl/events.cpp
			sys/sdl/cpu.cpp
			sys/sdl/sdl.cpp
			)
	set(src_sys_base
			${src_sys_base}
			sys/win32/win_input.cpp
			sys/win32/win_main.cpp
			sys/win32/win_net.cpp
			sys/win32/win_shared.cpp
			sys/win32/win_syscon.cpp
			sys/win32/SDL_win32_main.c
			sys/win32/win_aros.cpp
			sys/win32/win_cpu.cpp
			sys/win32/win_snd.cpp
			)
	set(src_sys_core
			sys/sdl/glimp.cpp
			)
else()
	set(src_sys_base
		${src_sys_base}
		sys/linux/main.cpp
		sys/linux/input.cpp
		sys/linux/sound.cpp
		sys/linux/sound_alsa.cpp
	)
	set(src_sys_core
		sys/linux/gles2.cpp
	)
endif()

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Build idlib static
# idSoftware
add_library(idlib STATIC ${src_idlib})
if(NOT MSVC)
	set_target_properties(idlib PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_idlib})

# raven
if(RAVEN OR QUAKE4)
	set(raven_idlib_src
			raven/idlib/LexerFactory.cpp
			raven/idlib/TextCompiler.cpp
			raven/idlib/math/Radians.cpp
			)
	set(src_idlib_raven
			${src_idlib}
			${raven_idlib_src}
			)
	add_library(idlib_raven STATIC ${src_idlib_raven})
	if(NOT MSVC)
		set_target_properties(idlib_raven PROPERTIES COMPILE_FLAGS "-fPIC")
	endif()
	set_target_properties(idlib_raven PROPERTIES COMPILE_DEFINITIONS "_RAVEN")

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_idlib_raven})
endif()

# humanhead
if(HUMANHEAD OR PREY)
	set(humanhead_idlib_src
			humanhead/idlib/math/prey_math.cpp
			)
	set(src_idlib_humanhead
			${src_idlib}
			${humanhead_idlib_src}
			)
	add_library(idlib_humanhead STATIC ${src_idlib_humanhead})
	if(NOT MSVC)
		set_target_properties(idlib_humanhead PROPERTIES COMPILE_FLAGS "-fPIC")
	endif()
	set_target_properties(idlib_humanhead PROPERTIES COMPILE_DEFINITIONS "_HUMANHEAD;HUMANHEAD")

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_idlib_humanhead})
endif()

if(CORE)
	message(STATUS "Build core")
endif()
if(BASE)
	message(STATUS "Build base")
endif()
if(D3XP)
	message(STATUS "Build d3xp")
endif()
if(D3LE)
	message(STATUS "Build d3le")
endif()
if(CDOOM)
	message(STATUS "Build cdoom")
endif()
if(RIVENSIN)
	message(STATUS "Build rivensin")
endif()
if(HARDCORPS)
	message(STATUS "Build HARDCORPS")
endif()
if(OVERTHINKED)
	message(STATUS "Build overthinked")
endif()
if(SABOT)
	message(STATUS "Build sabot")
endif()
if(HEXENEOC)
	message(STATUS "Build hexeneoc")
endif()
if(FRAGGINGFREE)
	message(STATUS "Build fraggingfree")
endif()
if(LIBRECOOP)
	message(STATUS "Build librecoop")
endif()
if(RAVEN)
	message(STATUS "Build raven core")
endif()
if(QUAKE4)
	message(STATUS "Build q4base")
endif()
if(HUMANHEAD)
	message(STATUS "Build humanhead core")
endif()
if(PREY)
	message(STATUS "Build prey")
endif()

message(STATUS "C++ STL: ${CMAKE_ANDROID_STL_TYPE}")
message(STATUS "arch ABI: ${CMAKE_ANDROID_ARCH_ABI}")
message(STATUS "API level: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "arm mode: ${CMAKE_ANDROID_ARM_MODE}")
message(STATUS "armv7-neon: ${CMAKE_ANDROID_ARM_NEON}")
message(STATUS "build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "-l: ${sys_libs}")
message(STATUS "-l(static): ${local_libs}")

message(STATUS "pointer size: ${CMAKE_SIZEOF_VOID_P}")
message(STATUS "CORE_DEFINES: ${CORE_DEFINES}")
message(STATUS "ldflags: ${ldflags}")

#message(FATAL_ERROR "STOP")

if(CORE)
	if(ANDROID)
		set(DOOM3BINARY ${IDTECH4LIBRARY})
		add_library(${IDTECH4LIBRARY} SHARED
				${src_core}
				${src_sys_base}
				${src_sys_core}
				${src_editor_tools}
				)
	else()
		set(DOOM3BINARY "Doom3")
		add_executable(${DOOM3BINARY}
				${src_core}
				${src_sys_base}
				${src_sys_core}
				${src_editor_tools}
				)
	endif()

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_sys_core} ${src_editor_tools})

	set_target_properties(${DOOM3BINARY} PROPERTIES COMPILE_DEFINITIONS "${CORE_DEFINES}")
	set_target_properties(${DOOM3BINARY} PROPERTIES LINK_FLAGS "${ldflags}")

	target_link_libraries(${DOOM3BINARY}
		idlib
		${sys_libs}
		${local_libs}
	)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS ${DOOM3BINARY}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(BASE)
	# base
	set(src_game ${base_game_src})
	set_src_dir(src_game "game" ${src_game})
	add_globbed_headers(src_game "game")

	add_library(game SHARED ${src_game})
	
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_game})
	
	#set_target_properties(game PROPERTIES PREFIX "")
	set(d3_compile_definitions "GAME_DLL")
	if(D3_MOD_FULL_BODY_AWARENESS)
		set(d3_compile_definitions "${d3_compile_definitions};_MOD_FULL_BODY_AWARENESS")
		message(STATUS "Build doom3 full body awareness mod support")
	endif()
	set_target_properties(game PROPERTIES COMPILE_DEFINITIONS "${d3_compile_definitions}")
	target_include_directories(game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/game")
	set_target_properties(game PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(game PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(game idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS game
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(D3XP)
	# d3xp
	set(src_d3xp ${base_game_src})
	set(d3xp_game_src
			Grabber.cpp
			physics/Force_Grab.cpp
			)
	list(APPEND src_d3xp ${d3xp_game_src})
	set_src_dir(src_d3xp "d3xp" ${src_d3xp})
	add_globbed_headers(src_d3xp "d3xp")

	add_library(d3xp SHARED ${src_d3xp})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_d3xp})
	#set_target_properties(d3xp PROPERTIES PREFIX "")
	set_target_properties(d3xp PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_D3XP;CTF")
	target_include_directories(d3xp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/d3xp")
	set_target_properties(d3xp PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(d3xp PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(d3xp idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS d3xp
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(D3LE)
	# d3le
	set(src_d3le ${base_game_src})
	set(d3le_game_src
			Grabber.cpp
			physics/Force_Grab.cpp
			)
	list(APPEND src_d3le ${d3le_game_src})
	set_src_dir(src_d3le "${DOOM3_MOD_DIR}/d3le" ${src_d3le})
	add_globbed_headers(src_d3le "${DOOM3_MOD_DIR}/d3le")

	add_library(d3le SHARED ${src_d3le})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_d3le})
	#set_target_properties(d3le PROPERTIES PREFIX "")
	set_target_properties(d3le PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_D3XP;CTF;_D3LE")
	target_include_directories(d3le PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/d3le")
	set_target_properties(d3le PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(d3le PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(d3le idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS d3le
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(CDOOM)
	# cdoom
	set(src_cdoom ${base_game_src})
	set_src_dir(src_cdoom "${DOOM3_MOD_DIR}/cdoom" ${src_cdoom})
	add_globbed_headers(src_cdoom "${DOOM3_MOD_DIR}/cdoom")

	add_library(cdoom SHARED ${src_cdoom})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_cdoom})
	#set_target_properties(cdoom PROPERTIES PREFIX "")
	set_target_properties(cdoom PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_CDOOM")
	target_include_directories(cdoom PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/cdoom")
	set_target_properties(cdoom PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(cdoom PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(cdoom idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS cdoom
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(RIVENSIN)
	# rivensin
	set(src_rivensin ${base_game_src})
	set(rivensin_game_src
			Grabber.cpp
			tracer.cpp
			TrailGenerator.cpp
			PlayerCursor.cpp
			ai/AI_bot.cpp
			physics/Force_Grab.cpp
			)
	list(APPEND src_rivensin ${rivensin_game_src})
	set_src_dir(src_rivensin "${DOOM3_MOD_DIR}/rivensin" ${src_rivensin})
	add_globbed_headers(src_rivensin "${DOOM3_MOD_DIR}/rivensin")

	add_library(rivensin SHARED ${src_rivensin})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_rivensin})
	#set_target_properties(rivensin PROPERTIES PREFIX "")
	set_target_properties(rivensin PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_RIVENSIN;_DENTONMOD;_PORTALSKY;_HDR")
	target_include_directories(rivensin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/rivensin")
	set_target_properties(rivensin PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(rivensin PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(rivensin idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS rivensin
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(HARDCORPS)
	# hardcorps
	set(src_hardcorps ${base_game_src})
	set(hardcorps_game_src
			Liquid.cpp
			tracer.cpp
			TrailGenerator.cpp
			ai/AI_bot.cpp
			physics/Physics_Liquid.cpp
			)
	list(APPEND src_hardcorps ${hardcorps_game_src})
	set_src_dir(src_hardcorps "${DOOM3_MOD_DIR}/hardcorps" ${src_hardcorps})
	add_globbed_headers(src_hardcorps "${DOOM3_MOD_DIR}/hardcorps")

	add_library(hardcorps SHARED ${src_hardcorps})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_hardcorps})
	#set_target_properties(hardcorps PROPERTIES PREFIX "")
	set_target_properties(hardcorps PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_HARDCORPS;_DENTONMOD;_PORTALSKY;_WATER_PHYSICS")
	target_include_directories(hardcorps PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/hardcorps")
	set_target_properties(hardcorps PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(hardcorps PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(hardcorps idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS hardcorps
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(OVERTHINKED)
	# overthinked
	set(src_overthinked ${base_game_src})
	set(overthinked_game_src
			tracer.cpp
			)
	list(APPEND src_overthinked ${overthinked_game_src})
	set_src_dir(src_overthinked "${DOOM3_MOD_DIR}/overthinked" ${src_overthinked})
	add_globbed_headers(src_overthinked "${DOOM3_MOD_DIR}/overthinked")

	add_library(overthinked SHARED ${src_overthinked})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_overthinked})
	#set_target_properties(overthinked PROPERTIES PREFIX "")
	set_target_properties(overthinked PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_OVERTHINKED;_DENTONMOD;_PORTALSKY")
	target_include_directories(overthinked PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/overthinked")
	set_target_properties(overthinked PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(overthinked PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(overthinked idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS overthinked
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(SABOT)
	# sabot
	set(src_sabot ${base_game_src})
	set(sabot_game_src
			Grabber.cpp
			physics/Force_Grab.cpp
			bots/BotAASBuild.cpp
			bots/BotAI.cpp
			bots/BotSabot.cpp
			)
	list(APPEND src_sabot ${sabot_game_src})
	set_src_dir(src_sabot "${DOOM3_MOD_DIR}/sabot" ${src_sabot})
	add_globbed_headers(src_sabot "${DOOM3_MOD_DIR}/sabot")

	add_library(sabot SHARED ${src_sabot})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_sabot})
	#set_target_properties(sabot PROPERTIES PREFIX "")
	set_target_properties(sabot PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_SABOT;MOD_BOTS;_PORTALSKY;_D3XP;CTF")
	target_include_directories(sabot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/sabot")
	set_target_properties(sabot PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(sabot PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(sabot idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS sabot
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(HEXENEOC)
	# hexeneoc
	set(src_hexeneoc ${base_game_src})
	set(hexeneoc_game_src
			PlayerArtifacts.cpp
			PlayerScriptEvents.cpp
			ai/AI_Golem.cpp
			ai/AI_Shadowspawn.cpp
			ai/AI_Veloxite.cpp
			objects/Leaf.cpp
			objects/LeafEmitter.cpp
			objects/Tree.cpp
			projectiles/FireStorm.cpp
			projectiles/Soul.cpp
			projectiles/Wraithverge.cpp
			)
	list(APPEND src_hexeneoc ${hexeneoc_game_src})
	set_src_dir(src_hexeneoc "${DOOM3_MOD_DIR}/hexeneoc" ${src_hexeneoc})
	add_globbed_headers(src_hexeneoc "${DOOM3_MOD_DIR}/hexeneoc")

	add_library(hexeneoc SHARED ${src_hexeneoc})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_hexeneoc})
	#set_target_properties(hexeneoc PROPERTIES PREFIX "")
	set_target_properties(hexeneoc PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_HEXENEOC")
	target_include_directories(hexeneoc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/hexeneoc")
	set_target_properties(hexeneoc PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(hexeneoc PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(hexeneoc idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS hexeneoc
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(FRAGGINGFREE)
	# fraggingfree
	set(src_fraggingfree ${base_game_src})
	set(fraggingfree_game_src
			Grabber.cpp
			physics/Force_Grab.cpp
			tracer.cpp
			)
	list(APPEND src_fraggingfree ${fraggingfree_game_src})
	set_src_dir(src_fraggingfree "${DOOM3_MOD_DIR}/fraggingfree" ${src_fraggingfree})
	add_globbed_headers(src_fraggingfree "${DOOM3_MOD_DIR}/fraggingfree")

	add_library(fraggingfree SHARED ${src_fraggingfree})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_fraggingfree})
	#set_target_properties(fraggingfree PROPERTIES PREFIX "")
	set_target_properties(fraggingfree PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_D3XP;CTF;_FRAGGINGFREE;_DENTONMOD;_PORTALSKY")
	target_include_directories(fraggingfree PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/fraggingfree")
	set_target_properties(fraggingfree PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(fraggingfree PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(fraggingfree idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS fraggingfree
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(LIBRECOOP)
	# librecoop
	set(src_librecoop ${base_game_src})
	set_src_dir(src_librecoop "${DOOM3_MOD_DIR}/librecoop" ${src_librecoop})
	add_globbed_headers(src_librecoop "${DOOM3_MOD_DIR}/librecoop")

	add_library(librecoop SHARED ${src_librecoop})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_librecoop})
	#set_target_properties(librecoop PROPERTIES PREFIX "")
	set_target_properties(librecoop PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_LIBRECOOP")
	target_include_directories(librecoop PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DOOM3_MOD_DIR}/librecoop")
	set_target_properties(librecoop PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(librecoop PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(librecoop idlib)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS librecoop
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(RAVEN)
	# raven engine source
	set(raven_renderer_src
			raven/bse/Model_bse.cpp
			raven/renderer/NewShaderStage.cpp
			)
	set(raven_framework_src
			raven/framework/DeclLipSync.cpp
			raven/framework/DeclPlayback.cpp
			raven/framework/DeclMatType.cpp
			raven/framework/DeclPlayerModel.cpp
			)
	set(raven_fx_bse_src
			raven/bse/BSE_Fx.cpp
			raven/bse/BSE_Manager.cpp
			raven/bse/BSE_Decl.cpp
			raven/bse/BSE_Particle.cpp
			)
	set(raven_null_bse_src
			raven/bse/BSE_Null.cpp
			)
	set(src_core_raven
			${src_renderer}
			${raven_renderer_src}
			${src_framework}
			${raven_framework_src}
			${src_cm}
			${src_aas}
			${src_roq}
			${src_snd}
			${src_ui}
			${src_tools}
			${src_renderbump}
			${src_dmap}
			)
	if(RAVEN_BSE_FX)
		list(APPEND src_core_raven ${raven_fx_bse_src})
	else()
		list(APPEND src_core_raven ${raven_null_bse_src})
	endif()
	add_globbed_headers(raven_renderer_src "raven")
	add_globbed_headers(raven_framework_src "raven")
	add_globbed_headers(raven_fx_bse_src "raven")
	add_globbed_headers(raven_null_bse_src "raven")

	if(ANDROID)
		set(QUAKE4BINARY "${IDTECH4LIBRARY}_raven")
		add_library(${QUAKE4BINARY} SHARED
				${src_core_raven}
				${src_sys_base}
				${src_sys_core}
				${src_editor_tools}
				)
	else()
		set(QUAKE4BINARY "Quake4")
		add_executable(${QUAKE4BINARY}
				${src_core_raven}
				${src_sys_base}
				${src_sys_core}
				${src_editor_tools}
		)
	endif()

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core_raven} ${src_sys_base} ${src_sys_core} ${src_editor_tools})

	set(raven_compile_definitions "_RAVEN")
	if(RAVEN_BSE_FX)
		set(raven_compile_definitions "${raven_compile_definitions};_RAVEN_FX")
	endif()
	set_target_properties(${QUAKE4BINARY} PROPERTIES COMPILE_DEFINITIONS "${raven_compile_definitions};${CORE_DEFINES}")
	set_target_properties(${QUAKE4BINARY} PROPERTIES LINK_FLAGS "${ldflags}")

	target_link_libraries(${QUAKE4BINARY}
			idlib_raven
			${sys_libs}
			${local_libs}
			)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS ${QUAKE4BINARY}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(QUAKE4)
	# quake4 game source
	set(src_quake4 ${base_game_src})
	set(quake4_game_src
			Effect.cpp
			FreeView.cpp
			Game_Debug.cpp
			Game_Log.cpp
			Healing_Station.cpp
			IconManager.cpp
			Instance.cpp
			Playback.cpp
			Player_Cheats.cpp
			Player_States.cpp
			spawner.cpp
			SplineMover.cpp
			Icon.cpp
			LipSync.cpp
			TramGate.cpp
			gamesys/State.cpp
			script/ScriptFuncUtility.cpp
			vehicle/Vehicle.cpp
			vehicle/VehicleAnimated.cpp
			vehicle/VehicleController.cpp
			vehicle/VehicleDriver.cpp
			vehicle/VehicleMonster.cpp
			vehicle/VehicleParts.cpp
			vehicle/VehiclePosition.cpp
			vehicle/VehicleRigid.cpp
			vehicle/VehicleSpline.cpp
			vehicle/VehicleStatic.cpp
			vehicle/Vehicle_DropPod.cpp
			vehicle/Vehicle_Walker.cpp
			client/ClientAFEntity.cpp
			client/ClientEffect.cpp
			client/ClientEntity.cpp
			client/ClientModel.cpp
			client/ClientMoveable.cpp
			ai/AAS_Find.cpp
			ai/Monster_Berserker.cpp
			ai/Monster_BossBuddy.cpp
			ai/Monster_BossMakron.cpp
			ai/Monster_ConvoyGround.cpp
			ai/Monster_ConvoyHover.cpp
			ai/Monster_FailedTransfer.cpp
			ai/Monster_Fatty.cpp
			ai/Monster_Gladiator.cpp
			ai/Monster_Grunt.cpp
			ai/Monster_Gunner.cpp
			ai/Monster_Harvester.cpp
			ai/Monster_HarvesterDispersal.cpp
			ai/Monster_HeavyHoverTank.cpp
			ai/Monster_IronMaiden.cpp
			ai/Monster_LightTank.cpp
			ai/Monster_NetworkGuardian.cpp
			ai/Monster_RepairBot.cpp
			ai/Monster_Scientist.cpp
			ai/Monster_Sentry.cpp
			ai/Monster_SlimyTransfer.cpp
			ai/Monster_StreamProtector.cpp
			ai/Monster_StroggFlyer.cpp
			ai/Monster_StroggHover.cpp
			ai/Monster_StroggMarine.cpp
			ai/Monster_TeleportDropper.cpp
			ai/Monster_Turret.cpp
			ai/Monster_TurretFlying.cpp
			ai/AI_Actions.cpp
			ai/AI_Announcements.cpp
			ai/AI_Debug.cpp
			ai/AI_Manager.cpp
			ai/AI_Medic.cpp
			ai/AI_Move.cpp
			ai/AI_States.cpp
			ai/AI_Tactical.cpp
			ai/AI_Util.cpp
			ai/VehicleAI.cpp
			ai/AAS_tactical.cpp
			weapon/WeaponBlaster.cpp
			weapon/WeaponGrenadeLauncher.cpp
			weapon/WeaponMachinegun.cpp
			weapon/WeaponRailgun.cpp
			weapon/WeaponDarkMatterGun.cpp
			weapon/WeaponHyperblaster.cpp
			weapon/WeaponNailgun.cpp
			weapon/WeaponRocketLauncher.cpp
			weapon/WeaponGauntlet.cpp
			weapon/WeaponLightningGun.cpp
			weapon/WeaponNapalmGun.cpp
			weapon/WeaponShotgun.cpp
			mp/Buying.cpp
			mp/CTF.cpp
			mp/GameState.cpp
			mp/Tourney.cpp
			mp/VoiceComms.cpp
			mp/stats/StatEvent.cpp
			mp/stats/StatManager.cpp
			mp/stats/StatWindow.cpp
			physics/Physics_Particle.cpp
			physics/Physics_VehicleMonster.cpp
			bots/BotAASBuild.cpp
			bots/BotAI.cpp
			bots/BotSabot.cpp
			)
	list(REMOVE_ITEM src_quake4
			Fx.cpp
			PlayerIcon.cpp
			SmokeParticles.cpp
			ai/AI_Vagary.cpp
			#gamesys/TypeInfo.cpp
			)
	list(APPEND src_quake4 ${quake4_game_src})
	set_src_dir(src_quake4 "quake4" ${src_quake4})
	add_globbed_headers(src_quake4 "quake4")

	add_library(q4game SHARED ${src_quake4})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_quake4})
	#set_target_properties(quake4 PROPERTIES PREFIX "")
	set(q4_compile_definitions "GAME_DLL;_RAVEN;_QUAKE4")
	if(Q4_MOD_FULL_BODY_AWARENESS)
		set(q4_compile_definitions "${q4_compile_definitions};_MOD_FULL_BODY_AWARENESS")
		message(STATUS "Build quake4 full body awareness mod support")
	endif()
	if(Q4_MOD_BOTS)
		set(q4_compile_definitions "${q4_compile_definitions};MOD_BOTS")
		message(STATUS "Build quake4 bots mod support")
	endif()
	set_target_properties(q4game PROPERTIES COMPILE_DEFINITIONS "${q4_compile_definitions}")
	target_include_directories(q4game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/quake4")
	set_target_properties(q4game PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(q4game PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(q4game idlib_raven)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS q4game
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()



if(HUMANHEAD)
	# humanhead engine source
	set(humanhead_renderer_src
			humanhead/renderer/Model_hhBeam.cpp
			)
	set(humanhead_framework_src
			humanhead/framework/declPreyBeam.cpp
			)
	set(humanhead_ui_src
			humanhead/ui/TabWindow.cpp
			humanhead/ui/TabContainerWindow.cpp
			)

	set(src_core_humanhead
			${src_renderer}
			${humanhead_renderer_src}
			${src_framework}
			${humanhead_framework_src}
			${humanhead_ui_src}
			${src_cm}
			${src_aas}
			${src_roq}
			${src_snd}
			${src_ui}
			${src_tools}
			${src_renderbump}
			${src_dmap}
			)
	add_globbed_headers(humanhead_renderer_src "humanhead")
	add_globbed_headers(humanhead_framework_src "humanhead")

	if(ANDROID)
		set(PREYBINARY "${IDTECH4LIBRARY}_humanhead")
		add_library(${PREYBINARY} SHARED
				${src_core_humanhead}
				${src_sys_base}
				${src_sys_core}
				${src_editor_tools}
				)
	else()
		set(PREYBINARY "Prey")
		add_executable(${PREYBINARY}
				${src_core_humanhead}
				${src_sys_base}
				${src_sys_core}
				${src_editor_tools}
				)
	endif()

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core_humanhead} ${src_sys_base} ${src_sys_core} ${src_editor_tools})

	set_target_properties(${PREYBINARY} PROPERTIES COMPILE_DEFINITIONS "_HUMANHEAD;${CORE_DEFINES}")
	set_target_properties(${PREYBINARY} PROPERTIES LINK_FLAGS "${ldflags}")

	target_link_libraries(${PREYBINARY}
			idlib_humanhead
			${sys_libs}
			${local_libs}
			)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS ${PREYBINARY}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()

if(PREY)
	# prey game source
	set(src_prey ${base_game_src})
	set(prey_game_src
			EntityAdditions.cpp
			physics/Physics_PreyPlayer.cpp
			Prey/ai_centurion.cpp
			Prey/ai_crawler.cpp
			Prey/ai_creaturex.cpp
			Prey/ai_droid.cpp
			Prey/ai_gasbag_simple.cpp
			Prey/ai_harvester_simple.cpp
			Prey/ai_hunter_simple.cpp
			Prey/ai_inspector.cpp
			Prey/ai_jetpack_harvester_simple.cpp
			Prey/ai_keeper_simple.cpp
			Prey/ai_mutate.cpp
			Prey/ai_mutilatedhuman.cpp
			Prey/ai_passageway.cpp
			Prey/ai_possessedTommy.cpp
			Prey/ai_reaction.cpp
			Prey/ai_spawncase.cpp
			Prey/ai_speech.cpp
			Prey/ai_sphereboss.cpp
			Prey/anim_baseanim.cpp
			Prey/force_converge.cpp
			Prey/game_afs.cpp
			Prey/game_alarm.cpp
			Prey/game_anim.cpp
			Prey/game_animBlend.cpp
			Prey/game_animDriven.cpp
			Prey/game_animatedentity.cpp
			Prey/game_animatedgui.cpp
			Prey/game_animator.cpp
			Prey/game_arcadegame.cpp
			Prey/game_barrel.cpp
			Prey/game_bindController.cpp
			Prey/game_blackjack.cpp
			Prey/game_cards.cpp
			Prey/game_cilia.cpp
			Prey/game_console.cpp
			Prey/game_damagetester.cpp
			Prey/game_dda.cpp
			Prey/game_deathwraith.cpp
			Prey/game_debrisspawner.cpp
			Prey/game_dock.cpp
			Prey/game_dockedgun.cpp
			Prey/game_door.cpp
			Prey/game_eggspawner.cpp
			Prey/game_energynode.cpp
			Prey/game_entityfx.cpp
			Prey/game_entityspawner.cpp
			Prey/game_events.cpp
			Prey/game_fixedpod.cpp
			Prey/game_forcefield.cpp
			Prey/game_fxinfo.cpp
			Prey/game_gibbable.cpp
			Prey/game_gravityswitch.cpp
			Prey/game_guihand.cpp
			Prey/game_gun.cpp
			Prey/game_hand.cpp
			Prey/game_handcontrol.cpp
			Prey/game_healthbasin.cpp
			Prey/game_healthspore.cpp
			Prey/game_inventory.cpp
			Prey/game_itemautomatic.cpp
			Prey/game_itemcabinet.cpp
			Prey/game_jukebox.cpp
			Prey/game_jumpzone.cpp
			Prey/game_light.cpp
			Prey/game_lightfixture.cpp
			Prey/game_mine.cpp
			Prey/game_misc.cpp
			Prey/game_modeldoor.cpp
			Prey/game_modeltoggle.cpp
			Prey/game_monster_ai.cpp
			Prey/game_monster_ai_events.cpp
			Prey/game_mountedgun.cpp
			Prey/game_moveable.cpp
			Prey/game_mover.cpp
			Prey/game_note.cpp
			Prey/game_organtrigger.cpp
			Prey/game_player.cpp
			Prey/game_playerview.cpp
			Prey/game_pod.cpp
			Prey/game_podspawner.cpp
			Prey/game_poker.cpp
			Prey/game_portal.cpp
			Prey/game_portalframe.cpp
			Prey/game_proxdoor.cpp
			Prey/game_rail.cpp
			Prey/game_railshuttle.cpp
			Prey/game_renderentity.cpp
			Prey/game_safeDeathVolume.cpp
			Prey/game_securityeye.cpp
			Prey/game_shuttle.cpp
			Prey/game_shuttledock.cpp
			Prey/game_shuttletransport.cpp
			Prey/game_skybox.cpp
			Prey/game_slots.cpp
			Prey/game_sphere.cpp
			Prey/game_spherepart.cpp
			Prey/game_spring.cpp
			Prey/game_sunCorona.cpp
			Prey/game_talon.cpp
			Prey/game_targetproxy.cpp
			Prey/game_targets.cpp
			Prey/game_trackmover.cpp
			Prey/game_trigger.cpp
			Prey/game_tripwire.cpp
			Prey/game_utils.cpp
			Prey/game_vehicle.cpp
			Prey/game_vomiter.cpp
			Prey/game_weaponHandState.cpp
			Prey/game_woundmanager.cpp
			Prey/game_wraith.cpp
			Prey/game_zone.cpp
			Prey/particles_particles.cpp
			Prey/physics_delta.cpp
			Prey/physics_preyai.cpp
			Prey/physics_preyparametric.cpp
			Prey/physics_simple.cpp
			Prey/physics_vehicle.cpp
			Prey/prey_animator.cpp
			Prey/prey_baseweapons.cpp
			Prey/prey_beam.cpp
			Prey/prey_bonecontroller.cpp
			Prey/prey_camerainterpolator.cpp
			Prey/prey_firecontroller.cpp
			Prey/prey_game.cpp
			Prey/prey_items.cpp
			Prey/prey_liquid.cpp
			Prey/prey_local.cpp
			Prey/prey_projectile.cpp
			Prey/prey_projectileautocannon.cpp
			Prey/prey_projectilebounce.cpp
			Prey/prey_projectilebug.cpp
			Prey/prey_projectilebugtrigger.cpp
			Prey/prey_projectilecocoon.cpp
			Prey/prey_projectilecrawlergrenade.cpp
			Prey/prey_projectilefreezer.cpp
			Prey/prey_projectilegasbagpod.cpp
			Prey/prey_projectilehiderweapon.cpp
			Prey/prey_projectilemine.cpp
			Prey/prey_projectilerifle.cpp
			Prey/prey_projectilerocketlauncher.cpp
			Prey/prey_projectileshuttle.cpp
			Prey/prey_projectilesoulcannon.cpp
			Prey/prey_projectilespiritarrow.cpp
			Prey/prey_projectiletracking.cpp
			Prey/prey_projectiletrigger.cpp
			Prey/prey_projectilewrench.cpp
			Prey/prey_script_thread.cpp
			Prey/prey_sound.cpp
			Prey/prey_soundleadincontroller.cpp
			Prey/prey_spiritbridge.cpp
			Prey/prey_spiritproxy.cpp
			Prey/prey_spiritsecret.cpp
			Prey/prey_vehiclefirecontroller.cpp
			Prey/prey_weapon.cpp
			Prey/prey_weaponautocannon.cpp
			Prey/prey_weaponcrawlergrenade.cpp
			Prey/prey_weaponfirecontroller.cpp
			Prey/prey_weaponhider.cpp
			Prey/prey_weaponrifle.cpp
			Prey/prey_weaponrocketlauncher.cpp
			Prey/prey_weaponsoulstripper.cpp
			Prey/prey_weaponspiritbow.cpp
			Prey/sys_debugger.cpp
			Prey/sys_preycmds.cpp
			)
	list(REMOVE_ITEM src_prey
			ai/AI_Vagary.cpp
			)
	list(APPEND src_prey ${prey_game_src})
	set_src_dir(src_prey "prey" ${src_prey})
	add_globbed_headers(src_prey "prey")

	add_library(preygame SHARED ${src_prey})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_prey})
	#set_target_properties(quake4 PROPERTIES PREFIX "")
	set(prey_compile_definitions "GAME_DLL;_HUMANHEAD;HUMANHEAD;_PREY")
	if(PREY_MOD_FULL_BODY_AWARENESS)
		set(prey_compile_definitions "${prey_compile_definitions};_MOD_FULL_BODY_AWARENESS")
		message(STATUS "Build prey full body awareness mod support")
	endif()
	set_target_properties(preygame PROPERTIES COMPILE_DEFINITIONS "${prey_compile_definitions}")
	target_include_directories(preygame PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/prey")
	set_target_properties(preygame PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(preygame PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(preygame idlib_humanhead)

	if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
		install(TARGETS preygame
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
				)
	endif()
endif()
