cmake_minimum_required(VERSION 3.9)
project (source-studiorender)

set(binary_name ${SOURCE_ENGINE_PREFIX}studiorender)

set(source_src 
		studiorender.cpp
		studiorendercontext.cpp
		flexrenderdata.cpp
		r_studio.cpp
		r_studiodecal.cpp
		r_studiodraw.cpp
		r_studiodraw_computeflexedvertex.cpp
		r_studioflex.cpp
		r_studiogettriangles.cpp
		r_studiolight.cpp
		r_studiostats.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
)

set(source_def "STUDIORENDER_EXPORTS=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}mathlib 
	${SOURCE_ENGINE_PREFIX}bitmap 
	)

	add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

