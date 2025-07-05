cmake_minimum_required(VERSION 3.9)
project (source-inputsystem)

set(binary_name ${SOURCE_ENGINE_PREFIX}inputsystem)

set(source_src 
		inputsystem.cpp
		joystick_sdl.cpp
		touch_sdl.cpp
		key_translation.cpp
		steamcontroller.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../common
		../public
		../public/tier0
)

set(source_def "VERSION_SAFE_STEAM_API_INTERFACES=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	SDL2 
	${SOURCE_ENGINE_PREFIX}steam_api
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

