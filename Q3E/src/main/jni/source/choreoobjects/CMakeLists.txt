cmake_minimum_required(VERSION 3.9)
project (source-choreoobjects)

set(binary_name ${SOURCE_ENGINE_PREFIX}choreoobjects)

set(source_src 
		../game/shared/choreoactor.cpp
		../game/shared/choreochannel.cpp
		../game/shared/choreoevent.cpp
		../game/shared/choreoscene.cpp
		../game/shared/sceneimage.cpp
)

set(source_inc 
		.
		../game/shared
		../utils/common
		../public/tier0
		../public/tier1
		../public
		../common
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

