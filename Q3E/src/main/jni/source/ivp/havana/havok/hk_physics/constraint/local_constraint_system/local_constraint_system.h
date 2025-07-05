#ifndef HK_PHYSICS_LOCAL_CONSTRAINT_SYSTEM_H
#define HK_PHYSICS_LOCAL_CONSTRAINT_SYSTEM_H

#include <hk_physics/constraint/local_constraint_system/local_constraint_system_bp.h>

class hk_Local_Constraint_System_BP;

// IVP_EXPORT_PUBLIC

class IVP_Core;

struct penetratepair_t {
	short   obj0;
	short   obj1;
};

class hk_Local_Constraint_System : public hk_Link_EF
{
public:
	hk_Local_Constraint_System(hk_Environment*, hk_Local_Constraint_System_BP*);
	//: creates a deactivated empty constraint system


	void activate();
	void deactivate();
	void deactivate_silently();

	//: activates the constraint

	virtual ~hk_Local_Constraint_System();

	inline hk_Environment* get_environment() const
	{
		return m_environment;
	};

	void write_to_blueprint(hk_Local_Constraint_System_BP*);

	void set_error_ticks(int);
	void set_error_tolerance(float);
	bool has_error();
	void clear_error();
	void report_square_error(float);

	void solve_penetration(IVP_Real_Object* pivp0, IVP_Real_Object* pivp1);
	void get_constraints_in_system(hk_Array<hk_Constraint*>& constraints_out);

public:	// internal
	virtual void entity_deletion_event(hk_Entity*);

	void core_is_going_to_be_deleted_event(IVP_Core*);

	void constraint_deletion_event(hk_Constraint*);

	virtual hk_effector_priority get_effector_priority() {
		return HK_PRIORITY_LOCAL_CONSTRAINT_SYSTEM;
	}

	void get_effected_entities(hk_Array<hk_Entity*>& ent_out);

	//virtual hk_real get_minimum_simulation_frequency(hk_Array<hk_Entity> *);

	void apply_effector_PSI(hk_PSI_Info&, hk_Array<hk_Entity*>*);

	void apply_effector_collision(hk_PSI_Info&, hk_Array<hk_Entity*>*) { ; }

	hk_real get_epsilon();
	inline bool is_active() const { return m_is_active; }

	inline void set_client_data(void* client_data) { m_client_data = client_data; }
	inline void* get_client_data() const { return m_client_data; }

	//: get the epsilon which defines the softness of the constraint
protected:
	friend class hk_Constraint;

	void add_constraint(hk_Constraint*, int storage_size);

	//: adds a constraint to the constraint system,
	//: Note: if the constraint is deactivated, than adding constraints is much faster

	void recalc_storage_size();
	int							m_n_iterations;
	int							m_size_of_all_vmq_storages;
	hk_Array<hk_Constraint*> 	m_constraints;
	hk_Array<hk_Rigid_Body*>	m_bodies;
	bool						m_is_active;

private:
	void* m_client_data;
	int m_minErrorTicks;
	int m_errorCount;
	int m_penetrationCount;
	struct penetratepair_t m_penetrationPairs[4];
	float m_errorTolerance;
	bool m_errorThisTick;
	bool m_needsSort;
};

//#include <hk_physics/constraint/local_constraint_system/local_constraint_system.inl>

#endif /* HK_PHYSICS_LOCAL_CONSTRAINT_SYSTEM_H */