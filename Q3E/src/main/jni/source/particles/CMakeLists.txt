cmake_minimum_required(VERSION 3.9)
project (source-particles)

set(binary_name ${SOURCE_ENGINE_PREFIX}particles)

set(source_src 
		builtin_constraints.cpp
		builtin_initializers.cpp
		builtin_particle_emitters.cpp
		builtin_particle_forces.cpp
		addbuiltin_ops.cpp
		builtin_particle_ops.cpp
		builtin_particle_render_ops.cpp
		particle_sort.cpp		
		particles.cpp
		psheet.cpp
)

set(source_inc 
		.
		../public
		../public/tier1
		../public/tier0
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

