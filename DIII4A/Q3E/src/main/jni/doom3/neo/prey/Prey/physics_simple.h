#ifndef __HH_PHYSICS_SIMPLE_H__
#define __HH_PHYSICS_SIMPLE_H__

class hhPhysics_RigidBodySimple : public idPhysics_RigidBody {
	CLASS_PROTOTYPE( hhPhysics_RigidBodySimple );
	public:
							hhPhysics_RigidBodySimple();

		virtual void		Integrate( const float deltaTime, rigidBodyPState_t &next );

	protected:
		friend void			SimpleRigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives );
};

#endif