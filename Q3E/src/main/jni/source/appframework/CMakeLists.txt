cmake_minimum_required(VERSION 3.9)
project (source-appframework)

set(binary_name ${SOURCE_ENGINE_PREFIX}appframework)

set(source_src 
	AppSystemGroup.cpp
	../public/filesystem_init.cpp
	sdlmgr.cpp
	posixapp.cpp #		[$POSIX]
)

set(source_inc 
	.
	../public
	../public/tier0
	../public/tier1
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

