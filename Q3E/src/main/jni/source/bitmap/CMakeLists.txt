cmake_minimum_required(VERSION 3.9)
project (source-bitmap)

set(binary_name ${SOURCE_ENGINE_PREFIX}bitmap)

set(source_src 
		ImageByteSwap.cpp
		colorconversion.cpp
		float_bm.cpp
		float_bm2.cpp
		float_bm3.cpp
		float_bm_bilateral_filter.cpp
		float_cube.cpp
		imageformat.cpp
		psd.cpp
		resample.cpp
		tgaloader.cpp
		tgawriter.cpp
		bitmap.cpp
)

set(source_inc 
	.
	../public
	../public/tier0
	../public/tier1
	../thirdparty/stb
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

