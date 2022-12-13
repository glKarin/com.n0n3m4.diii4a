#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idPhysics_RigidBody, hhPhysics_RigidBodySimple )
END_CLASS

/*
================
SimpleRigidBodyDerivatives
================
*/
void SimpleRigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives ) {
	const hhPhysics_RigidBodySimple *p = (hhPhysics_RigidBodySimple *) clientData;
	rigidBodyIState_t *s = (rigidBodyIState_t *) state;
	// NOTE: this struct should be build conform rigidBodyIState_t
	struct rigidBodyDerivatives_s {
		idVec3				linearVelocity;
		idMat3				angularMatrix;
		idVec3				force;
		idVec3				torque;
	} *d = (struct rigidBodyDerivatives_s *) derivatives;

	// derivatives
	d->linearVelocity = p->inverseMass * s->linearMomentum;
	d->angularMatrix.Zero();
	//d->angularMatrix = SkewSymmetric( vec3_zero ) * s->orientation;
	d->force = - p->linearFriction * s->linearMomentum + p->current.externalForce;
	d->torque.Zero();
}

/*
================
hhPhysics_RigidBodySimple::hhPhysics_RigidBodySimple
================
*/
hhPhysics_RigidBodySimple::hhPhysics_RigidBodySimple() {
	SAFE_DELETE_PTR( integrator );
	integrator = new idODE_Euler( sizeof(rigidBodyIState_t) / sizeof(float), SimpleRigidBodyDerivatives, this );
}

/*
================
hhPhysics_RigidBodySimple::Integrate
================
*/
void hhPhysics_RigidBodySimple::Integrate( const float deltaTime, rigidBodyPState_t &next ) {
	idPhysics_RigidBody::Integrate( deltaTime, next );
}