cmake_minimum_required(VERSION 3.9)
project (source-vtf)

set(binary_name ${SOURCE_ENGINE_PREFIX}vtf)

set(source_src 
		convert_x360.cpp
		vtf.cpp
		#$File	"vtf_x360.cpp"#		[$X360]
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

