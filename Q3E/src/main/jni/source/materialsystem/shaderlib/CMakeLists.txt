cmake_minimum_required(VERSION 3.9)
project (source-shaderlib)

set(binary_name ${SOURCE_ENGINE_PREFIX}shaderlib)

set(source_src 
		BaseShader.cpp
		ShaderDLL.cpp
		shaderlib_cvar.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
		../../common
		../
)

set(source_def "FAST_MATERIALVAR_ACCESS=1")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

