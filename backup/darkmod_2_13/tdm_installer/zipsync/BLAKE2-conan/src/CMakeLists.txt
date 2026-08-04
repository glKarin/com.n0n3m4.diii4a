cmake_minimum_required(VERSION 3.8)

project(BLAKE2)

include_directories(include)

if(SSE STREQUAL "None")
	file(GLOB_RECURSE SourceFiles
		"ref/blake2*.h"
		"ref/blake2*.c"
	)
else()
	file(GLOB_RECURSE SourceFiles
		"sse/blake2*.h"
		"sse/blake2*.c"
	)
endif()

#TODO check if instruction set is supported (ex: in VC++ SSE2 doesn't work with x64)
if(MSVC)
	if (SSE STREQUAL "SSE2")
		add_compile_options(/arch:SSE2)
		add_definitions(-D__SSE2__)
	endif()
else()
	if (SSE STREQUAL "SSE2")
		add_compile_options(-msse2)
	endif()
endif()

add_library(BLAKE2 ${SourceFiles})
