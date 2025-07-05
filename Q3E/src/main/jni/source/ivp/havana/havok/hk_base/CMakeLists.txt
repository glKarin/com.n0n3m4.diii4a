cmake_minimum_required(VERSION 3.9)
project (source-hk_base)

set(binary_name ${SOURCE_ENGINE_PREFIX}hk_base)

set(source_src 
		memory/memory.cpp
		memory/memory_util.cpp
		array/array.cpp
		hash/hash.cpp
		stopwatch/stopwatch.cpp
		string/string.cpp
		id_server/id_server.cpp
		base_types.cpp
		console.cpp
)

set(source_inc 
		.
		../
)

set(source_def "VPHYSICS_EXPORTS=1;HAVANA_CONSTRAINTS;HAVOK_MOPP")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

