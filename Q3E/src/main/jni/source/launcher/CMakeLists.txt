cmake_minimum_required(VERSION 3.9)
project (source-launcher)

set(binary_name ${SOURCE_ENGINE_PREFIX}launcher)

set(source_src 
		../public/filesystem_init.cpp
		launcher.cpp
		reslistgenerator.cpp
		../public/tier0/memoverride.cpp
)
set(source_src
	${source_src}
			android/main.cpp
			android/crashhandler.cpp
)

set(source_inc 
		.
		android/libunwind
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_def "LAUNCHERONLY=1;CFLAGS=1;LDFLAGS=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vstdlib
	${SOURCE_ENGINE_PREFIX}steam_api
	${SOURCE_ENGINE_PREFIX}appframework
	SDL2
	${SOURCE_ENGINE_PREFIX}togl
	log
	android
	#UNWIND
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

set_target_properties(${binary_name} PROPERTIES OUTPUT_NAME "source")

