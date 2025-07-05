cmake_minimum_required(VERSION 3.9)
project (source-vphysics)

set(binary_name ${SOURCE_ENGINE_PREFIX}vphysics)

set(source_src 
		convert.cpp
		../public/filesystem_helpers.cpp
		ledgewriter.cpp
		main.cpp
		physics_airboat.cpp
		physics_collide.cpp
		physics_constraint.cpp
		physics_controller_raycast_vehicle.cpp
		physics_environment.cpp
		physics_fluid.cpp
		physics_friction.cpp
		physics_material.cpp
		physics_motioncontroller.cpp
		physics_object.cpp
		physics_shadow.cpp
		physics_spring.cpp
		physics_vehicle.cpp
		physics_virtualmesh.cpp
		trace.cpp
		vcollide_parse.cpp
		vphysics_saverestore.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../ivp/ivp_intern
		../ivp/ivp_collision
		../ivp/ivp_physics
		../ivp/ivp_surface_manager
		../ivp/ivp_utility
		../ivp/ivp_controller
		../ivp/ivp_compact_builder
		../ivp/havana/havok
		../ivp/havana
)

set(source_def "VPHYSICS_EXPORTS;HAVANA_CONSTRAINTS;HAVOK_MOPP")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}havana_constraints 
	${SOURCE_ENGINE_PREFIX}hk_math 
	${SOURCE_ENGINE_PREFIX}hk_base 
	${SOURCE_ENGINE_PREFIX}ivp_compactbuilder
	${SOURCE_ENGINE_PREFIX}ivp_physics 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib  
	${SOURCE_ENGINE_PREFIX}mathlib  
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

