cmake_minimum_required(VERSION 3.9)
project (source-ivp_compactbuilder)

set(binary_name ${SOURCE_ENGINE_PREFIX}ivp_compactbuilder)

set(source_src 
		ivp_compact_ledge_gen.cxx
		ivp_compact_modify.cxx
		ivp_compact_recursive.cxx
		ivp_convex_decompositor.cxx
		ivp_halfspacesoup.cxx
		ivp_i_fpoint_vhash.cxx
		ivp_i_point_vhash.cxx
		ivp_object_polygon_tetra.cxx
		ivp_rot_inertia_solver.cxx
		ivp_surbuild_halfspacesoup.cxx
		ivp_surbuild_ledge_soup.cxx
		ivp_surbuild_pointsoup.cxx
		ivp_surbuild_polygon_convex.cxx
		ivp_surbuild_polyhdrn_cncv.cxx
		ivp_surbuild_q12.cxx
		ivp_templates_intern.cxx
		ivp_tetra_intrude.cxx
		ivv_cluster_min_hash.cxx
		qhull.cxx
		qhull_geom.cxx
		qhull_geom2.cxx
		qhull_global.cxx
		qhull_io.cxx
		qhull_mem.cxx
		qhull_merge.cxx
		qhull_poly.cxx
		qhull_poly2.cxx
		qhull_qset.cxx
		qhull_stat.cxx
		qhull_user.cxx
		geompack_cutfac.cxx
		geompack_cvdec3.cxx
		geompack_drdec3.cxx
		geompack_dsphdc.cxx
		geompack_edght.cxx
		geompack_initcb.cxx
		geompack_insed3.cxx
		geompack_insfac.cxx
		geompack_insvr3.cxx
		geompack_prime.cxx
		geompack_ptpolg.cxx
		geompack_resedg.cxx
		3dsimport_co.cxx
)

set(source_inc 
		.
		havok
		../havana/havok
		../ivp_physics
		../ivp_utility
		../ivp_controller
		../ivp_surface_manager
		../ivp_collision
)

set(source_def "IVP_VERSION_SDK;HAVOK_MOPP")

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")

