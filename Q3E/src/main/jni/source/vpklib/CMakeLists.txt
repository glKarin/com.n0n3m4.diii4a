cmake_minimum_required(VERSION 3.9)
project (source-vpklib)

set(binary_name ${SOURCE_ENGINE_PREFIX}vpklib)

set(source_src 
		packedstore.cpp
		../common/simplebitstring.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

