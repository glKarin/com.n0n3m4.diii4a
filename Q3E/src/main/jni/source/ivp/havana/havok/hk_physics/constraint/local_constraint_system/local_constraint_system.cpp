	#include <ivp_physics.hxx>
#include <ivp_template_constraint.hxx>

#include <hk_physics/physics.h>

#include <hk_physics/constraint/constraint.h>
#include <hk_physics/constraint/local_constraint_system/local_constraint_system.h>

#include <hk_physics/constraint/limited_ball_socket/limited_ball_socket_bp.h>
#include <hk_physics/constraint/limited_ball_socket/limited_ball_socket_constraint.h>
#include <hk_physics/constraint/ragdoll/ragdoll_constraint_bp.h>
#include <hk_physics/constraint/ragdoll/ragdoll_constraint.h>
#include <hk_physics/constraint/ragdoll/ragdoll_constraint_bp_builder.h>

hk_Local_Constraint_System::hk_Local_Constraint_System(hk_Environment* env, hk_Local_Constraint_System_BP* bp)
	: hk_Link_EF(env)
{
	m_environment = env;
	m_size_of_all_vmq_storages = 0;
	m_is_active = 0;
	m_errorCount = 0;
	m_client_data = NULL;
	//m_scErrorThisTick = Four_Zeros;
	m_n_iterations = bp->m_n_iterations + 2;
	m_errorTolerance = bp->m_errorTolerance;
	m_minErrorTicks = bp->m_minErrorTicks;
	m_needsSort = 0;
	m_penetrationCount = 0;
	m_size_of_all_vmq_storages = 0;
	m_is_active = false;
	m_errorThisTick = 0;

	clear_error();
}

hk_Local_Constraint_System::~hk_Local_Constraint_System()
{
	for ( hk_Array<hk_Constraint *>::iterator i = m_constraints.start();
			m_constraints.is_valid(i);
			i = m_constraints.next( i ) )
	{
		hk_Constraint *constraint = m_constraints.get_element(i);
		constraint->constraint_system_deleted_event( this );
	}

	if ( m_is_active )
	{
		m_environment->get_controller_manager()->remove_controller_from_environment(this,IVP_TRUE); //silently
	}
}

void hk_Local_Constraint_System::get_constraints_in_system(hk_Array<hk_Constraint*>& constraints_out)
{
	for (hk_Array<hk_Constraint*>::iterator i = m_constraints.start();
		m_constraints.is_valid(i);
		i = m_constraints.next(i))
	{
		constraints_out.add_element(m_constraints.get_element(i));
	}
}

//@@CB
void hk_Local_Constraint_System::entity_deletion_event(hk_Entity* entity)
{
	m_bodies.search_and_remove_element(entity);

	if (!entity->get_core()->physical_unmoveable)
	{
		actuator_controlled_cores.remove(entity->get_core());
	}

	//	HK_BREAK;
}


//@@CB
void hk_Local_Constraint_System::core_is_going_to_be_deleted_event(IVP_Core* my_core)
{
	hk_Rigid_Body* rigid_body;

	if (m_bodies.length())
	{
		for (hk_Array<hk_Rigid_Body*>::iterator i = m_bodies.start();
			m_bodies.is_valid(i);
			i = m_bodies.next(i))
		{
			rigid_body = m_bodies.get_element(i);

			if (rigid_body->get_core() == my_core)
			{
				this->entity_deletion_event(rigid_body);
			}
		}
	}
}


void hk_Local_Constraint_System::constraint_deletion_event(hk_Constraint* constraint)
{
	m_constraints.search_and_remove_element_sorted(constraint);
	if (m_constraints.length())
	{
		recalc_storage_size();
	}
}


void hk_Local_Constraint_System::recalc_storage_size()
{
	m_size_of_all_vmq_storages = 0;
	for (hk_Array<hk_Constraint*>::iterator i = m_constraints.start();
		m_constraints.is_valid(i);
		i = m_constraints.next(i))
	{
		m_size_of_all_vmq_storages += m_constraints.get_element(i)->get_vmq_storage_size();
	}
}


void hk_Local_Constraint_System::add_constraint(hk_Constraint* constraint, int storage_size)
{
	bool isActive = m_is_active;
	if ( isActive )
	{
		deactivate();
	}
	m_constraints.add_element( constraint );
	
	int i = 1;
	do 
	{
		hk_Rigid_Body *b = constraint->get_rigid_body(i);
		if ( m_bodies.index_of( b ) <0)
		{
			m_bodies.add_element(b);
			if ( !b->get_core()->physical_unmoveable )
			{
				actuator_controlled_cores.add( b->get_core());
			}
		}
	} while (--i>=0);

	if ( isActive )
	{
		activate();
	}
	m_size_of_all_vmq_storages += storage_size;	
}

void hk_Local_Constraint_System::activate()
{
	if (!m_is_active && m_bodies.length())
	{
		m_environment->get_controller_manager()->announce_controller_to_environment(this);
		m_is_active = true;
	}
}

void hk_Local_Constraint_System::deactivate()
{
	if (m_is_active && actuator_controlled_cores.len())
	{
		m_environment->get_controller_manager()->remove_controller_from_environment(this, IVP_FALSE);
		m_is_active = false;
	}
}

void hk_Local_Constraint_System::deactivate_silently()
{
	if (m_is_active && actuator_controlled_cores.len())
	{
		m_is_active = false;
		m_environment->get_controller_manager()->remove_controller_from_environment(this, IVP_TRUE);
	}
}

void hk_Local_Constraint_System::write_to_blueprint(hk_Local_Constraint_System_BP* bpOut)
{
	bpOut->m_damp = 1.0f;
	bpOut->m_tau = 1.0f;
	bpOut->m_n_iterations = m_n_iterations - 2;
	bpOut->m_minErrorTicks = m_minErrorTicks;
	bpOut->m_errorTolerance = m_errorTolerance;
	bpOut->m_active = m_is_active;
}

void hk_Local_Constraint_System::set_error_ticks(int error_ticks)
{
	m_minErrorTicks = error_ticks;
}

void hk_Local_Constraint_System::set_error_tolerance(float tolerance)
{
	m_errorTolerance = tolerance;
}

bool hk_Local_Constraint_System::has_error()
{
	return m_errorCount >= m_minErrorTicks;
}

void hk_Local_Constraint_System::clear_error()
{
	m_errorCount = 0;
}

void hk_Local_Constraint_System::report_square_error(float errSq)
{
//	m_errorThisTick = ((m_errorTolerance * m_errorTolerance) < errSq);
}

void hk_Local_Constraint_System::solve_penetration(IVP_Real_Object* pivp0, IVP_Real_Object* pivp1)
{
#if 0
	if (m_penetrationCount >= 4)
		return;

	hk_Rigid_Body *b0 = (hk_Rigid_Body*)pivp0;
	hk_Rigid_Body *b1 = (hk_Rigid_Body*)pivp1;

	m_penetrationPairs[m_penetrationCount].obj0 = m_bodies.index_of(b0);
	m_penetrationPairs[m_penetrationCount].obj1 = m_bodies.index_of(b1);

	if (m_penetrationPairs[m_penetrationCount].obj0 && m_penetrationPairs[m_penetrationCount].obj1)
		m_penetrationCount++;
#endif
}

void hk_Local_Constraint_System::get_effected_entities(hk_Array<hk_Entity*>& ent_out)
{
	for (hk_Array<hk_Entity*>::iterator i = m_bodies.start();
		m_bodies.is_valid(i);
		i = m_bodies.next(i))
	{
		ent_out.add_element(m_bodies.get_element(i));
	}
}

//virtual hk_real get_minimum_simulation_frequency(hk_Array<hk_Entity> *);

IVP_FLOAT GetMoveableMass(IVP_Core* pCore)
{
	if (pCore->movement_state & (IVP_MT_STATIC | IVP_MT_SLOW)) {
		return pCore->get_rot_inertia()->hesse_val;
	}

	return 0;
}

void hk_Local_Constraint_System::apply_effector_PSI(hk_PSI_Info& pi, hk_Array<hk_Entity*>*)
{
	const int buffer_size = 150000;
	const int max_constraints = 1000;
	void* vmq_buffers[max_constraints];
	char buffer[buffer_size];
	HK_ASSERT(m_size_of_all_vmq_storages < buffer_size);

	m_errorThisTick = 0;

	hk_real taus[] = { 1.0f, 1.0f, 0.8f, 0.6f, 0.4f, 0.4f, 0.4f, 0.4f, 0.4f, 0.0f };
	hk_real damps[] = { 1.0f, 1.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.0f };

	//first do the setup
	{
		char* p_buffer = &buffer[0];
		for (int i = 0; i < m_constraints.length(); i++) {
			vmq_buffers[i] = (void*)p_buffer;

			hk_Rigid_Body *b0 = m_constraints.element_at(i)->get_rigid_body(0);
			hk_Rigid_Body *b1 = m_constraints.element_at(i)->get_rigid_body(1);

			int b_size = m_constraints.element_at(i)->setup_and_step_constraint(pi, (void*)p_buffer, 1.0f, 1.0f);
			p_buffer += b_size;
		}
	}

	for (int i = 0; i < m_n_iterations; i++)
	{
		// do the steps
		for (int x = 0; x < 2 && taus[x] != 0; x++)
		{
			for (int i = m_constraints.length() - 1; i >= 0; i--) {
				m_constraints.element_at(i)->step_constraint(pi, (void*)vmq_buffers[i], taus[x], damps[x]);
			}
			for (int j = 0; j < m_constraints.length(); j++) {
				m_constraints.element_at(j)->step_constraint(pi, (void*)vmq_buffers[j], taus[x], damps[x]);
			}
		}
	}

#if 0
	if (m_penetrationCount)
	{
		IVP_DOUBLE d_time = m_environment->get_delta_PSI_time();
		IVP_DOUBLE grav = m_environment->get_gravity()->real_length();
		IVP_DOUBLE speed_change = (grav * 2.0f) * d_time;
		IVP_FLOAT mass = 0;

		for (hk_Array<hk_Entity*>::iterator i = m_bodies.start();
			m_bodies.is_valid(i);
			i = m_bodies.next(i))
		{
			mass += GetMoveableMass(m_bodies.get_element(i)->get_core());
		}

		for (int i = 0; i < m_penetrationCount; i++)
		{
			IVP_Real_Object* obj0 = m_bodies.element_at(m_penetrationPairs[i].obj0);
			IVP_Real_Object* obj1 = m_bodies.element_at(m_penetrationPairs[i].obj1);

			IVP_Core* core0 = obj0->get_core();
			IVP_Core* core1 = obj1->get_core();

			const IVP_U_Matrix* m_world_f_core0 = core0->get_m_world_f_core_PSI();
			const IVP_U_Matrix* m_world_f_core1 = core1->get_m_world_f_core_PSI();

			IVP_U_Float_Point vec01;
			vec01.subtract(&m_world_f_core1->vv, &m_world_f_core0->vv);
			float deltaLength = vec01.real_length_plus_normize();
			if (deltaLength <= 0.01)
			{
				// if the objects are exactly on top of each other just push down X the axis
				vec01.set(1.0, 0.0, 0.0);
			}

			IVP_U_Float_Point p0;
			p0.set_multiple(&vec01, -speed_change * mass);

			if (IVP_MTIS_SIMULATED(core0->movement_state) && !core0->pinned)
			{
				obj0->async_push_object_ws(&m_world_f_core0->vv, &p0);
				IVP_U_Float_Point rot_speed_object(-d_time, 0, 0);
				obj0->async_add_rot_speed_object_cs(&rot_speed_object);
			}

			if (IVP_MTIS_SIMULATED(core1->movement_state) && !core1->pinned)
			{
				obj1->async_push_object_ws(&m_world_f_core0->vv, &p0);
				IVP_U_Float_Point rot_speed_object(d_time, 0, 0);
				obj1->async_add_rot_speed_object_cs(&rot_speed_object);
			}
		}
		m_penetrationCount = 0;
	}

	if( !m_errorThisTick )
	{
		m_errorCount = 0;
		return;
	}

	if( m_errorCount <= m_minErrorTicks )
		m_errorCount++;
#endif
}

hk_real hk_Local_Constraint_System::get_epsilon()
{
	return 0.2f;
}
