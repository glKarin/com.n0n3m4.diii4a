cmake_minimum_required(VERSION 3.9)
project (source-soundemittersystem)

set(binary_name ${SOURCE_ENGINE_PREFIX}soundemittersystem)

set(source_src 
		../game/shared/interval.cpp
		soundemittersystembase.cpp
		../public/SoundParametersInternal.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../game/shared
)

set(source_def "SOUNDEMITTERSYSTEM_EXPORTS=1;_WINDOWS=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	)

	add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

