cmake_minimum_required(VERSION 3.9)
project (source-vtex_dll)

set(binary_name ${SOURCE_ENGINE_PREFIX}vtex_dll)

set(source_src 
		../common/cmdlib.cpp
		../../public/filesystem_helpers.cpp
		../../public/filesystem_init.cpp
		../common/filesystem_tools.cpp
		vtex.cpp
		../../common/imageutils.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
		../common
		../../common
		../../thirdparty
)

set(source_def "VTEX_DLL=1;UTILS=1;VTEX_DLL_EXPORTS=1;PROTECTED_THINGS_DISABLE=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}bitmap 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}mathlib 
	${SOURCE_ENGINE_PREFIX}vtf 
	${SOURCE_ENGINE_PREFIX}jpeg 
	${SOURCE_ENGINE_PREFIX}png_static 
	${SOURCE_ENGINE_PREFIX}zlibstatic 
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

