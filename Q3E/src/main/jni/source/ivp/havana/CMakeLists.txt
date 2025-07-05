cmake_minimum_required(VERSION 3.9)
project (source-havana_constraints)

set(binary_name ${SOURCE_ENGINE_PREFIX}havana_constraints)

set(source_src 
		havok/hk_physics/constraint/constraint.cpp
		havok/hk_physics/constraint/ball_socket/ball_socket_constraint.cpp
		havok/hk_physics/constraint/limited_ball_socket/limited_ball_socket_constraint.cpp
		havok/hk_physics/constraint/ragdoll/ragdoll_constraint.cpp
		havok/hk_physics/constraint/ragdoll/ragdoll_constraint_bp_builder.cpp
		havok/hk_physics/constraint/local_constraint_system/local_constraint_system.cpp
		havok/hk_physics/constraint/hinge/hinge_bp_builder.cpp
		havok/hk_physics/constraint/hinge/hinge_constraint.cpp
		havok/hk_physics/constraint/breakable_constraint/breakable_constraint.cpp
		havok/hk_physics/constraint/fixed/fixed_constraint.cpp
		havok/hk_physics/constraint/prismatic/prismatic_constraint.cpp
		havok/hk_physics/constraint/pulley/pulley_constraint.cpp
		havok/hk_physics/constraint/stiff_spring/stiff_spring_constraint.cpp
		havok/hk_physics/core/rigid_body_core.cpp
		havok/hk_physics/effector/rigid_body_binary_effector.cpp
)

set(source_inc 
		.
		havok
		../ivp_physics
		../ivp_utility
		../ivp_controller
)

set(source_def "VPHYSICS_EXPORTS;HAVANA_CONSTRAINTS;HAVOK_MOPP;IVP_VERSION_SDK")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

