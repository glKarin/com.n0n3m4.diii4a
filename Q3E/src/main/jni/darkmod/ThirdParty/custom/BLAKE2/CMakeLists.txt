cmake_minimum_required(VERSION 3.8)

include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)


project(BLAKE2)
conan_basic_setup()

include_directories(include)

if(SSE STREQUAL "None")
	file(GLOB_RECURSE SourceFiles
		"BLAKE2/ref/blake2*.h"
		"BLAKE2/ref/blake2*.c"
	)
else()
	file(GLOB_RECURSE SourceFiles
		"BLAKE2/sse/blake2*.h"
		"BLAKE2/sse/blake2*.c"
	)
endif()

#TODO check if instruction set is supported (ex: in VC++ SSE2 doesn't work with x64)
if(MSVC)
	if    (SSE STREQUAL "SSE2")
		add_compile_options(/arch:SSE2)
		add_definitions(-D__SSE2__)
	elseif(SSE STREQUAL "AVX")
		add_compile_options(/arch:AVX)
		add_definitions(-D__AVX__)
	elseif(SSE STREQUAL "AVX2")
		add_compile_options(/arch:AVX2)
		add_definitions(-D__AVX2__)
	endif()
else()
	if    (SSE STREQUAL "SSE2")
		add_compile_options(-msse2)
	elseif(SSE STREQUAL "AVX")
		add_compile_options(-mavx)
	elseif(SSE STREQUAL "AVX2")
		add_compile_options(-mavx2)
	endif()
endif()

set(TargetName BLAKE2)
add_library(${TargetName} ${SourceFiles})
conan_target_link_libraries(${TargetName})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TargetName})