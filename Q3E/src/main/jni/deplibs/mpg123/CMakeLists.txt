cmake_minimum_required (VERSION 3.1..3.18)

project(libmpg123 VERSION 1.28.2)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED TRUE)

enable_language(CXX C ASM)

set (LOCAL_C_INCLUDES android
	src
	src/compat
	src/libmpg123
)

set(DECODER_CFLAGS_NEON "-DOPT_NEON -DREAL_IS_FLOAT")

set(DECODER_SRC_NEON 
    src/libmpg123/stringbuf.c
    src/libmpg123/icy.c
    src/libmpg123/icy2utf8.c
    src/libmpg123/ntom.c
    src/libmpg123/synth.c
    src/libmpg123/synth_8bit.c
    src/libmpg123/layer1.c
    src/libmpg123/layer2.c
    src/libmpg123/layer3.c
    src/libmpg123/dct36_neon.S
    src/libmpg123/dct64_neon_float.S
    src/libmpg123/synth_neon_float.S
    src/libmpg123/synth_neon_s32.S
    src/libmpg123/synth_stereo_neon_float.S
    src/libmpg123/synth_stereo_neon_s32.S
    src/libmpg123/dct64_neon.S
    src/libmpg123/synth_neon.S
    src/libmpg123/synth_stereo_neon.S
    src/libmpg123/synth_s32.c
    src/libmpg123/synth_real.c
    src/libmpg123/feature.c
)

set(DECODER_CFLAGS_NEON64 "-DOPT_MULTI -DOPT_GENERIC -DOPT_GENERIC_DITHER -DOPT_NEON64 -DREAL_IS_FLOAT")

set(DECODER_SRC_NEON64
    src/libmpg123/stringbuf.c
    src/libmpg123/icy.c
    src/libmpg123/icy2utf8.c
    src/libmpg123/ntom.c
    src/libmpg123/synth.c
    src/libmpg123/synth_8bit.c
    src/libmpg123/layer1.c
    src/libmpg123/layer2.c
    src/libmpg123/layer3.c
    src/libmpg123/dct36_neon64.S
    src/libmpg123/dct64_neon64_float.S
    src/libmpg123/synth_neon64_float.S
    src/libmpg123/synth_neon64_s32.S
    src/libmpg123/synth_stereo_neon64_float.S
    src/libmpg123/synth_stereo_neon64_s32.S
    src/libmpg123/dct64_neon64.S
    src/libmpg123/synth_neon64.S
    src/libmpg123/synth_stereo_neon64.S
    src/libmpg123/synth_s32.c
    src/libmpg123/synth_real.c
    src/libmpg123/dither.c
    src/libmpg123/getcpuflags_arm.c
    src/libmpg123/check_neon.S
    src/libmpg123/feature.c
)

set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if(cpu STREQUAL "aarch64")
	# "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
	set(cpu "arm64")
endif()

if(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
	# On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
	# while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
		set(cpu "arm64")
		set(DECODER_CFLAGS ${DECODER_CFLAGS_NEON64})
		set(DECODER_SRC ${DECODER_SRC_NEON64})
	else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
		set(cpu "arm")
		set(DECODER_CFLAGS ${DECODER_CFLAGS_NEON})
		set(DECODER_SRC ${DECODER_SRC_NEON})
	endif()
else()
	message(FATAL_ERROR "Only for arm32 or arm64 build!")
endif()

set(LOCAL_SRC_FILES 
    src/libmpg123/parse.c
    src/libmpg123/frame.c
    src/libmpg123/format.c
    src/libmpg123/dct64.c
    src/libmpg123/equalizer.c
    src/libmpg123/id3.c
    src/libmpg123/optimize.c
    src/libmpg123/readers.c
    src/libmpg123/tabinit.c
    src/libmpg123/libmpg123.c
    src/libmpg123/index.c
    src/compat/compat_str.c
    src/compat/compat.c
    me_rosuh_libmpg123_MPG123.c
    ${DECODER_SRC}
)

set (PACKAGE_NAME ${PROJECT_NAME})
set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

add_definitions(${DECODER_CFLAGS})

add_library (mpg123 SHARED 
    ${LOCAL_SRC_FILES}
	)

target_include_directories (mpg123
	PRIVATE
	${LOCAL_C_INCLUDES}
	)

target_link_libraries(mpg123 log)
