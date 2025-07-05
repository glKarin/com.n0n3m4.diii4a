cmake_minimum_required(VERSION 3.9)
project (source-vaudio_minimp3)

set(binary_name ${SOURCE_ENGINE_PREFIX}vaudio_minimp3)

set(source_src 
		mp3codecs.cpp
		../../../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../../../public/tier1
		../../../public/
		../../../thirdparty/minimp3
)

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}vstdlib
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
target_link_libraries(${binary_name} PRIVATE ${source_lib})

