cmake_minimum_required(VERSION 3.9)
project (source-shaderapidx9)

set(binary_name ${SOURCE_ENGINE_PREFIX}shaderapidx9)

set(source_src 
		colorformatdx8.cpp
		../../public/filesystem_helpers.cpp
		hardwareconfig.cpp
		meshbase.cpp
		meshdx8.cpp
		shaderapidx8.cpp
		shaderdevicebase.cpp
		shaderapibase.cpp
		shaderdevicedx8.cpp
		shadershadowdx8.cpp
		texturedx8.cpp
		TransitionTable.cpp
		cvballoctracker.cpp
		vertexdecl.cpp
		vertexshaderdx8.cpp
		#textureheap.cpp							[$X360]
		../../public/tier0/memoverride.cpp
)
set(source_src 
	${source_src}
	winutils.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
		../../common
		../
)

set(source_def "SHADERAPIDX9;SHADER_DLL_EXPORT;strncpy=use_Q_strncpy_instead;_snprintf=use_Q_snprintf_instead")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}togl  
	${SOURCE_ENGINE_PREFIX}bitmap 
	${SOURCE_ENGINE_PREFIX}mathlib  
	) # ANDROID_SUPPORT

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

