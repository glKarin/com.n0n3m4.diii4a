cmake_minimum_required(VERSION 3.9)
project (source-tier2)

set(binary_name ${SOURCE_ENGINE_PREFIX}tier2)

set(source_src 
		beamsegdraw.cpp
		defaultfilesystem.cpp
		dmconnect.cpp
		fileutils.cpp
		keybindings.cpp
		../public/map_utils.cpp
		../public/materialsystem/MaterialSystemUtil.cpp
		camerautils.cpp
		meshutils.cpp
		p4helpers.cpp
		renderutils.cpp
		riff.cpp
		soundutils.cpp
		tier2.cpp
		util_init.cpp
		utlstreambuffer.cpp
		vconfig.cpp
		keyvaluesmacros.cpp
)

set(source_inc 
		.
		../common
		../public
		../public/tier0
		../public/tier1
		../public/tier2
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

