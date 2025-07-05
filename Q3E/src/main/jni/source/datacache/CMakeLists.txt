cmake_minimum_required(VERSION 3.9)
project (source-datacache)

set(binary_name ${SOURCE_ENGINE_PREFIX}datacache)

set(source_src 
		datacache.cpp
		mdlcache.cpp
		../public/studio.cpp
		../public/studio_virtualmodel.cpp
		../common/studiobyteswap.cpp
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
	) # ANDROID_SUPPORT

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
target_link_libraries(${binary_name} PRIVATE ${source_lib})

