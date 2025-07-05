#ifndef HK_PHYSICS_LOCAL_CONSTRAINT_SYSTEM_BP
#define HK_PHYSICS_LOCAL_CONSTRAINT_SYSTEM_BP

// IVP_EXPORT_PUBLIC

class hk_Local_Constraint_System_BP  //: public hk_Effector_BP 
{ 
	public:

		hk_real m_damp;
		hk_real m_tau;
        int	m_n_iterations;
        int m_minErrorTicks;
        float m_errorTolerance;
        bool m_active;

        // Ghidra reports the following 3 fields exist:
//        undefined field_0x15;
//        undefined field_0x16;
//        undefined field_0x17;

public:

		hk_Local_Constraint_System_BP()
			:	m_damp( 1.0f ),
				m_tau( 1.0f ),
				m_n_iterations( 0 ),
				m_minErrorTicks( 1 ),
				m_errorTolerance( 0.03 )
		{
			;
		}
};

#endif /* HK_PHYSICS_LOCAL_CONSTRAINT_SYSTEM_BP */

