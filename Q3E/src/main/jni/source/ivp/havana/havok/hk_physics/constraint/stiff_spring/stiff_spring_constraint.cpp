#include <hk_physics/physics.h>

#include <hk_physics/constraint/stiff_spring/stiff_spring_constraint.h>
#include <hk_physics/constraint/stiff_spring/stiff_spring_bp.h>

#include <hk_physics/simunit/psi_info.h>

#include <hk_physics/core/vm_query_builder/vm_query_builder.h>

#ifdef HK_ARCH_PPC
#include <stddef.h> // for size_t
#endif

class hk_Stiff_Spring_Work
{
	public:
#ifdef HK_ARCH_PPC
		static inline void *operator new (size_t size, void *addr){
			return addr;
		}
#else
		static inline void *operator new (size_t size, void *addr){
			return addr;
		}

		static inline void operator delete (void *, void *){;}
#endif

		hk_VM_Query_Builder< hk_VMQ_Storage<1> > query_engine;
		hk_real current_dist;
		hk_bool skip_solve;
};

void hk_Stiff_Spring_Constraint::init_constraint( const void* vbp )
{
	const hk_Stiff_Spring_BP* bp = static_cast<const hk_Stiff_Spring_BP*>(vbp);
	this->init_stiff_spring_constraint(bp);
}

void hk_Stiff_Spring_Constraint::init_stiff_spring_constraint( const hk_Stiff_Spring_BP *bp )
{
	m_tau		= bp->m_tau;
	m_strength	= bp->m_strength;
	m_stiff_spring_length = bp->m_length;
	m_translation_os_ks[0]	= bp->m_translation_os_ks[0];
	m_translation_os_ks[1]	= bp->m_translation_os_ks[1];
	m_min_length = bp->m_min_length;
}

void hk_Stiff_Spring_Constraint::write_to_blueprint( hk_Stiff_Spring_BP *bp )
{
	bp->m_tau = m_tau;
	bp->m_strength = m_strength;
	bp->m_length = m_stiff_spring_length;
	bp->m_translation_os_ks[0] = m_translation_os_ks[0];
	bp->m_translation_os_ks[1] = m_translation_os_ks[1];
	bp->m_min_length = m_min_length;
}

hk_Stiff_Spring_Constraint::hk_Stiff_Spring_Constraint(
		hk_Environment *env,
		const hk_Stiff_Spring_BP *bp,
		hk_Rigid_Body* a,
		hk_Rigid_Body* b )
	: hk_Constraint( env, a, b, HK_PRIORITY_LOCAL_CONSTRAINT)
{
	init_stiff_spring_constraint( bp );
}



hk_Stiff_Spring_Constraint::hk_Stiff_Spring_Constraint( 
	hk_Local_Constraint_System* constraint_system,
	const hk_Stiff_Spring_BP* bp,
	hk_Rigid_Body* a ,
	hk_Rigid_Body* b )
	: hk_Constraint( constraint_system, a, b, HK_PRIORITY_LOCAL_CONSTRAINT, HK_NEXT_MULTIPLE_OF(16,sizeof(hk_Stiff_Spring_Work)))
{
	init_stiff_spring_constraint( bp );
}

void hk_Stiff_Spring_Constraint::set_length( hk_real length)
{
	m_stiff_spring_length = length;
}

int hk_Stiff_Spring_Constraint::get_vmq_storage_size()
{
	return HK_NEXT_MULTIPLE_OF(16, sizeof(hk_Stiff_Spring_Work));
}

float IntervalDistance(float a1, float a2, float a3)
{
	if ( a2 > a1 )
		return a1 - a2;
	if ( a1 <= a3 )
		return 0.f;
	return a1 - a3;
}

int hk_Stiff_Spring_Constraint::setup_and_step_constraint( hk_PSI_Info& pi, void *mem, hk_real tau_factor, hk_real damp_factor )
{
	hk_Stiff_Spring_Work& work = *new (mem) hk_Stiff_Spring_Work;
	hk_VM_Query_Builder< hk_VMQ_Storage<1> > &query_engine = work.query_engine;

	hk_Rigid_Body *b0 = get_rigid_body(0);
	hk_Rigid_Body *b1 = get_rigid_body(1);

	hk_Vector3 translation_ws_ks[2];

	translation_ws_ks[0]._set_transformed_pos( b0->get_cached_transform(), m_translation_os_ks[0]);
	translation_ws_ks[1]._set_transformed_pos( b1->get_cached_transform(), m_translation_os_ks[1]);

	hk_Vector3 dir;
	dir.set_sub( translation_ws_ks[1], translation_ws_ks[0] );

	hk_real norm_length = dir.normalize_with_length();

	work.current_dist = IntervalDistance(norm_length, m_min_length, m_stiff_spring_length);

	if (this->m_min_length == this->m_stiff_spring_length || work.current_dist != 0.f)
		work.skip_solve = false;
	else
	{
		hk_Vector3 next_translation_ws_ks[2];
		next_translation_ws_ks[0]._set_transformed_pos(b0->get_transform_next_PSI(pi.get_delta_time()), m_translation_os_ks[0]);
		next_translation_ws_ks[1]._set_transformed_pos(b1->get_transform_next_PSI(pi.get_delta_time()), m_translation_os_ks[1]);

		hk_Vector3 next_dir;
		next_dir.set_sub(next_translation_ws_ks[1], next_translation_ws_ks[0]);

		hk_real next_norm_length = next_dir.normalize_with_length();

		hk_real next_dist = IntervalDistance(next_norm_length, m_min_length, m_stiff_spring_length);
		work.skip_solve = next_dist == 0.f;
		if (work.skip_solve)
			return HK_NEXT_MULTIPLE_OF(16, sizeof(hk_Stiff_Spring_Work));
	}

	query_engine.begin(1);
	{
		query_engine.begin_entries(1);
		{
			query_engine.add_linear( 0, HK_BODY_A, b0, translation_ws_ks[0], dir,  1.0f );
			query_engine.add_linear( 0, HK_BODY_B, b1, translation_ws_ks[0], dir, -1.0f );
		}
		query_engine.commit_entries(1);
	}
	query_engine.commit(HK_BODY_A, b0);
	query_engine.commit(HK_BODY_B, b1);

	hk_Dense_Matrix& mass_matrix = query_engine.get_vmq_storage().get_dense_matrix();
	mass_matrix(0, 0) = 1.0f / mass_matrix(0, 0); // invert in place

	{ // step
		hk_real *approaching_velocity = query_engine.get_vmq_storage().get_velocities();
		hk_real delta_dist = tau_factor * m_tau * pi.get_inv_delta_time() * work.current_dist - damp_factor * m_strength * approaching_velocity[0];

		hk_Vector3 impulses;
		impulses(0) = delta_dist * mass_matrix(0,0);

		query_engine.apply_impulses( HK_BODY_A, b0, (hk_real *)&impulses(0) );
		query_engine.apply_impulses( HK_BODY_B, b1, (hk_real *)&impulses(0) );
	}
	return HK_NEXT_MULTIPLE_OF(16, sizeof(hk_Stiff_Spring_Work));
}

void hk_Stiff_Spring_Constraint::step_constraint( hk_PSI_Info& pi, void *mem, hk_real tau_factor, hk_real damp_factor )
{
	hk_Stiff_Spring_Work& work = *(hk_Stiff_Spring_Work*)mem;

	if (!work.skip_solve)
	{
		hk_VM_Query_Builder< hk_VMQ_Storage<1> > &query_engine = work.query_engine;

		hk_real *approaching_velocity = query_engine.get_vmq_storage().get_velocities();
		approaching_velocity[0] = 0.0f;

		hk_Rigid_Body *b0 = get_rigid_body(0);
		hk_Rigid_Body *b1 = get_rigid_body(1);

		query_engine.update_velocities(HK_BODY_A, b0);
		query_engine.update_velocities(HK_BODY_B, b1);

		hk_real delta_dist = tau_factor * m_tau * pi.get_inv_delta_time() * work.current_dist - damp_factor * m_strength * approaching_velocity[0];

		hk_Vector3 impulses;
		hk_Dense_Matrix& mass_matrix = query_engine.get_vmq_storage().get_dense_matrix();
		impulses(0) = delta_dist * mass_matrix(0,0);

		query_engine.apply_impulses( HK_BODY_A, b0, (hk_real *)&impulses(0) );
		query_engine.apply_impulses( HK_BODY_B, b1, (hk_real *)&impulses(0) );
	}
}


void hk_Stiff_Spring_Constraint::apply_effector_PSI( hk_PSI_Info& pi, hk_Array<hk_Entity*>* )
{
	hk_Stiff_Spring_Work work_mem;
	hk_Stiff_Spring_Constraint::setup_and_step_constraint( pi,(void *)&work_mem, 1.0f, 1.0f );
}

// HAVOK DO NOT EDIT
