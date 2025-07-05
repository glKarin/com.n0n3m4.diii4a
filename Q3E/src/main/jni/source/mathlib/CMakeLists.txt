cmake_minimum_required(VERSION 3.9)
project (source-mathlib)

set(binary_name ${SOURCE_ENGINE_PREFIX}mathlib)

set(source_src 
		color_conversion.cpp
		halton.cpp
		lightdesc.cpp
		mathlib_base.cpp
		powsse.cpp
		sparse_convolution_noise.cpp
		sseconst.cpp
		sse.cpp	#				[$WINDOWS||$POSIX]
		ssenoise.cpp				
		3dnow.cpp #					[$WINDOWS||$LINUX]
		anorms.cpp
		bumpvects.cpp
		IceKey.cpp
		imagequant.cpp
		polyhedron.cpp
		quantize.cpp
		randsse.cpp
		spherical.cpp
		simdvectormatrix.cpp
		vector.cpp
		vmatrix.cpp
		almostequal.cpp
)

set(source_inc 
		.
		../common
		../public
		../public/tier0
		../public/mathlib
)

set(source_def "MATHLIB_LIB=1")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

