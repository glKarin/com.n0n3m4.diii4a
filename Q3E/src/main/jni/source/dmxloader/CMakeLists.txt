cmake_minimum_required(VERSION 3.9)
project (source-dmxloader)

set(binary_name ${SOURCE_ENGINE_PREFIX}dmxloader)

set(source_src 
		dmxattribute.cpp
		dmxelement.cpp
		dmxloader.cpp
                dmxloadertext.cpp
		dmxserializationdictionary.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_def "DMXLOADER_LIB=1")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

