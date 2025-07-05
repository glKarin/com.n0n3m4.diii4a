cmake_minimum_required(VERSION 3.9)
project (source-filesystem_stdio)

set(binary_name ${SOURCE_ENGINE_PREFIX}filesystem_stdio)

set(source_src 
		basefilesystem.cpp
		packfile.cpp
		filetracker.cpp
		filesystem_async.cpp
		filesystem_stdio.cpp
		../public/kevvaluescompiler.cpp
		../public/zip_utils.cpp
		QueuedLoader.cpp
		../public/tier0/memoverride.cpp
			linux_support.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_def "FILESYSTEM_STDIO_EXPORTS=1;DONT_PROTECT_FILEIO_FUNCTIONS=1;SUPPORT_PACKED_STORE=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}vpklib 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

