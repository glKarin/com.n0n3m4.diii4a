cmake_minimum_required(VERSION 3.9)
project (source-ivp_physics)

set(binary_name ${SOURCE_ENGINE_PREFIX}ivp_physics)

set(source_src 
		../ivp_collision/ivp_3d_solver.cxx
		../ivp_collision/ivp_clustering_longrange.cxx
		../ivp_collision/ivp_clustering_lrange_hash.cxx
		../ivp_collision/ivp_clustering_visual_hash.cxx
		../ivp_collision/ivp_clustering_visualizer.cxx
		../ivp_collision/ivp_coll_del_root_mindist.cxx
		../ivp_collision/ivp_collision_filter.cxx
		../ivp_collision/ivp_compact_ledge.cxx
		../ivp_collision/ivp_compact_ledge_solver.cxx
		../ivp_collision/ivp_i_collision_vhash.cxx
		../ivp_collision/ivp_mindist.cxx
		../ivp_collision/ivp_mindist_debug.cxx
		../ivp_collision/ivp_mindist_event.cxx
		../ivp_collision/ivp_mindist_mcases.cxx
		../ivp_collision/ivp_mindist_minimize.cxx
		../ivp_collision/ivp_mindist_recursive.cxx
		../ivp_collision/ivp_oo_watcher.cxx
		../ivp_collision/ivp_range_manager.cxx
		../ivp_collision/ivp_ray_solver.cxx
		../ivp_controller/ivp_actuator.cxx
		../ivp_controller/ivp_actuator_spring.cxx
		../ivp_controller/ivp_buoyancy_solver.cxx
		../ivp_controller/ivp_car_system.cxx
		../ivp_controller/ivp_constraint.cxx
		../ivp_controller/ivp_constraint_car.cxx
		../ivp_controller/ivp_constraint_fixed_keyed.cxx
		../ivp_controller/ivp_constraint_local.cxx
		../ivp_controller/ivp_controller_airboat.cpp
		../ivp_controller/ivp_controller_buoyancy.cxx
		../ivp_controller/ivp_controller_floating.cxx
		../ivp_controller/ivp_controller_motion.cxx
		../ivp_controller/ivp_controller_raycast_car.cxx
		../ivp_controller/ivp_controller_stiff_spring.cxx
		../ivp_controller/ivp_controller_world_frict.cxx
		../ivp_controller/ivp_forcefield.cxx
		../ivp_controller/ivp_multidimensional_interp.cxx
		../ivp_controller/ivp_template_constraint.cxx
		../ivp_intern/ivp_ball.cxx
		../ivp_intern/ivp_calc_next_psi_solver.cxx
		../ivp_intern/ivp_controller_phantom.cxx
		../ivp_intern/ivp_core.cxx
		../ivp_intern/ivp_environment.cxx
		../ivp_intern/ivp_friction.cxx
		../ivp_intern/ivp_friction_gaps.cxx
		../ivp_intern/ivp_great_matrix.cxx
		../ivp_intern/ivp_hull_manager.cxx
		../ivp_intern/ivp_i_friction_hash.cxx
		../ivp_intern/ivp_i_object_vhash.cxx
		../ivp_intern/ivp_impact.cxx
		../ivp_intern/ivp_merge_core.cxx
		../ivp_intern/ivp_mindist_friction.cxx
		../ivp_intern/ivp_object.cxx
		../ivp_intern/ivp_object_attach.cxx
		../ivp_intern/ivp_physic.cxx
		../ivp_intern/ivp_physic_private.cxx
		../ivp_intern/ivp_polygon.cxx
		../ivp_intern/ivp_sim_unit.cxx
		../ivp_intern/ivp_solver_core_reaction.cxx
		../ivp_intern/ivp_time.cxx
		ivp_anomaly_manager.cxx
		ivp_betterdebugmanager.cxx
		ivp_betterstatisticsmanager.cxx
		ivp_cache_object.cxx
		ivp_liquid_surface_descript.cxx
		ivp_material.cxx
		ivp_performancecounter.cxx
		ivp_stat_manager_cback_con.cxx
		ivp_surface_manager.cxx
		ivp_templates.cxx
		../ivp_surface_manager/ivp_compact_surface.cxx
		../ivp_surface_manager/ivp_gridbuild_array.cxx
		../ivp_surface_manager/ivp_surman_grid.cxx
		../ivp_surface_manager/ivp_surman_polygon.cxx
		../ivp_utility/ivu_active_value.cxx
		../ivp_utility/ivu_bigvector.cxx
		../ivp_utility/ivu_geometry.cxx
		../ivp_utility/ivu_hash.cxx
		../ivp_utility/ivu_linear.cxx
		../ivp_utility/ivu_memory.cxx
		../ivp_utility/ivu_min_hash.cxx
		../ivp_utility/ivu_min_list.cxx
		../ivp_utility/ivu_os_dep.cxx
		../ivp_utility/ivu_quat.cxx
		../ivp_utility/ivu_string.cxx
		../ivp_utility/ivu_types.cxx
		../ivp_utility/ivu_vector.cxx
		../ivp_utility/ivu_vhash.cxx
)

set(source_inc 
		.
		havok
		../ivp_physics
		../ivp_utility
		../ivp_controller
		../ivp_collision
		../ivp_surface_manager
		../ivp_intern
		../havana/havok
)

set(source_def "HAVOK_MOPP;IVP_VERSION_SDK;VPHYSICS_EXPORTS;HAVANA_CONSTRAINTS")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

