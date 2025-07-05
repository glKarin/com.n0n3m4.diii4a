cmake_minimum_required(VERSION 3.9)
project (source-tier3)

set(binary_name ${SOURCE_ENGINE_PREFIX}tier3)

set(source_src 
		tier3.cpp
		mdlutils.cpp
		choreoutils.cpp
		scenetokenprocessor.cpp
		studiohdrstub.cpp
)

set(source_inc 
		.
		../common
		../public
		../public/tier0
		../public/tier1
		../public/tier2
		../public/tier3
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

