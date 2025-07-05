cmake_minimum_required(VERSION 3.9)
project (source-vgui_surfacelib)

set(binary_name ${SOURCE_ENGINE_PREFIX}vgui_surfacelib)

set(source_src 
		BitmapFont.cpp
		FontAmalgam.cpp
		FontManager.cpp
		FontEffects.cpp
		#Win32Font_x360.cpp	[$X360]
		#osxfont.cpp			[$OSXALL]
)
set(source_src
	${source_src}
			linuxfont.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
		../../common
)
set(source_inc
	${source_inc}
			../../thirdparty/freetype/include
			../../thirdparty/fontconfig
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

