cmake_minimum_required(VERSION 3.9)
project (source-materialsystem)

set(binary_name ${SOURCE_ENGINE_PREFIX}materialsystem)

set(source_src 
		CColorCorrection.cpp
		cmaterial.cpp
		cmaterial_queuefriendly.cpp
		CMaterialSubRect.cpp
		cmaterialvar.cpp
		cmatnullrendercontext.cpp
		colorspace.cpp
		ctexture.cpp
		../public/filesystem_helpers.cpp
		imagepacker.cpp
		mat_stub.cpp
		materialsystem_global.cpp
		morph.cpp
		occlusionquerymgr.cpp
		shadersystem.cpp
		texturemanager.cpp
		pch_materialsystem.cpp
		cmaterialdict.cpp
		cmaterialsystem.cpp
		cmatlightmaps.cpp
		cmatrendercontext.cpp
		cmatqueuedrendercontext.cpp
		ctexturecompositor.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
		../
)

set(source_def "DEFINE_MATERIALSYSTEM_INTERFACE;MATERIALSYSTEM_EXPORTS;strncpy=use_Q_strncpy_instead;_snprintf=use_Q_snprintf_instead")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}mathlib 
	${SOURCE_ENGINE_PREFIX}bitmap 
	${SOURCE_ENGINE_PREFIX}shaderlib  
	${SOURCE_ENGINE_PREFIX}vtf 
	) # ANDROID_SUPPORT

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

