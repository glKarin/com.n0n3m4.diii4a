cmake_minimum_required(VERSION 3.9)
project (source-steam_api)

set(binary_name ${SOURCE_ENGINE_PREFIX}steam_api)
set(binary_output_name ${SOURCE_GAME_PREFIX}steam_api)

set(source_src 
		steam_api.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

set_target_properties(${binary_name} PROPERTIES OUTPUT_NAME "${binary_output_name}")
