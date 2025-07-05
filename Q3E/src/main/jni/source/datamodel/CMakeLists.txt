cmake_minimum_required(VERSION 3.9)
project (source-datamodel)

set(binary_name ${SOURCE_ENGINE_PREFIX}datamodel)

set(source_src 
		clipboardmanager.cpp
		datamodel.cpp
		dependencygraph.cpp
		dmattribute.cpp
		dmelement.cpp
		dmelementdictionary.cpp
		dmelementfactoryhelper.cpp
		DmElementFramework.cpp
		dmserializerbinary.cpp
		dmserializerkeyvalues.cpp
		dmserializerkeyvalues2.cpp
		undomanager.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_def "DATAMODEL_LIB=1")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

