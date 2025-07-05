cmake_minimum_required(VERSION 3.9)
project (source-video_services)

set(binary_name ${SOURCE_ENGINE_PREFIX}video_services)

set(source_src 
		videoservices.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vstdlib
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
target_link_libraries(${binary_name} PRIVATE ${source_lib})

