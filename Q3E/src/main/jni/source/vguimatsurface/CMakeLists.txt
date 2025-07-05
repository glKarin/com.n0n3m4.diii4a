cmake_minimum_required(VERSION 3.9)
project (source-vguimatsurface)

set(binary_name ${SOURCE_ENGINE_PREFIX}vguimatsurface)

set(source_src 
		Clip2D.cpp
		Cursor.cpp
		../public/filesystem_helpers.cpp
		FontTextureCache.cpp
		Input.cpp
		MatSystemSurface.cpp
		asanstubs.cpp
		TextureDictionary.cpp
		../vgui2/src/vgui_key_translation.cpp
		../public/vgui_controls/vgui_controls.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_def "VGUIMATSURFACE_DLL_EXPORT=1;GAMEUI_EXPORTS=1;DONT_PROTECT_FILEIO_FUNCTIONS=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}bitmap 
	${SOURCE_ENGINE_PREFIX}mathlib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}vgui_controls 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vgui_surfacelib 
	${SOURCE_ENGINE_PREFIX}freetype 
	SDL2 
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

