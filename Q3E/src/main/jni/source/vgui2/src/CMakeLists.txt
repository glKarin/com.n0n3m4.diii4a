cmake_minimum_required(VERSION 3.9)
project (source-vgui2)

set(binary_name ${SOURCE_ENGINE_PREFIX}vgui2)

set(source_src 
		Bitmap.cpp
		Border.cpp
		ScalableImageBorder.cpp
		ImageBorder.cpp
		fileimage.cpp
		../../public/filesystem_helpers.cpp
		../../public/filesystem_init.cpp
		InputWin32.cpp
		LocalizedStringTable.cpp
		MemoryBitmap.cpp
		MessageListener.cpp
		Scheme.cpp
		../../public/UnicodeFileHelpers.cpp
		vgui.cpp
		vgui_internal.cpp
		vgui_key_translation.cpp
		VPanel.cpp
		VPanelWrapper.cpp
		keyrepeat.cpp
		../../public/tier0/memoverride.cpp
)
set(source_src
	${source_src}
			system_posix.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
		../../common
)

set(source_def "DONT_PROTECT_FILEIO_FUNCTIONS=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vstdlib
	SDL2
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

