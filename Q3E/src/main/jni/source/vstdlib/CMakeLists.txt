cmake_minimum_required(VERSION 3.9)
project (source-vstdlib)

set(binary_name ${SOURCE_ENGINE_PREFIX}vstdlib)
set(binary_output_name ${SOURCE_GAME_PREFIX}vstdlib)

set(source_src 
		#xbox\___FirstModule.cpp [$X360]
		coroutine.cpp	#			[!$X360 && !$OSXALL]
		cvar.cpp
		jobthread.cpp
		KeyValuesSystem.cpp
		random.cpp
		vcover.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../public/vstdlib
)

set(source_def "VSTDLIB_DLL_EXPORT=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1
	) # ANDROID_SUPPORT

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

set_target_properties(${binary_name} PROPERTIES OUTPUT_NAME "${binary_output_name}")
