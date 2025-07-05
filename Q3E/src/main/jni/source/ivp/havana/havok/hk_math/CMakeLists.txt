cmake_minimum_required(VERSION 3.9)
project (source-hk_math)

set(binary_name ${SOURCE_ENGINE_PREFIX}hk_math)

set(source_src 
		lcp/lcp_solver.cpp
		incr_lu/incr_lu.cpp
		gauss_elimination/gauss_elimination.cpp
		quaternion/quaternion.cpp
		quaternion/quaternion_util.cpp
		vector3/vector3.cpp
		vector3/vector3_util.cpp
		densematrix.cpp
		densematrix_util.cpp
		eulerangles.cpp
		math.cpp
		matrix3.cpp
		odesolve.cpp
		plane.cpp
		rotation.cpp
		spatial_matrix.cpp
		spatial_vector.cpp
		transform.cpp
)

set(source_inc 
		.
		../
		../../../ivp_utility/
)

set(source_def "VPHYSICS_EXPORTS;HAVANA_CONSTRAINTS;HAVOK_MOPP")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

