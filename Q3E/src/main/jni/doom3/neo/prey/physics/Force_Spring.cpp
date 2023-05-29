// Copyright (C) 2004 Id Software, Inc.
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( idForce, idForce_Spring )
END_CLASS

/*
================
idForce_Spring::idForce_Spring
================
*/
idForce_Spring::idForce_Spring( void ) {
	Kstretch		= 100.0f;
	Kcompress		= 100.0f;
	damping			= 0.0f;
	restLength		= 0.0f;
	physics1		= NULL;
	id1				= 0;
	p1				= vec3_zero;
	physics2		= NULL;
	id2				= 0;
	p2				= vec3_zero;
}

/*
================
idForce_Spring::~idForce_Spring
================
*/
idForce_Spring::~idForce_Spring( void ) {
}

/*
================
idForce_Spring::InitSpring
================
*/
void idForce_Spring::InitSpring( float Kstretch, float Kcompress, float damping, float restLength ) {
	this->Kstretch = Kstretch;
	this->Kcompress = Kcompress;
	this->damping = damping;
	this->restLength = restLength;
}

/*
================
idForce_Spring::SetPosition
================
*/
void idForce_Spring::SetPosition( idEntity *physics1, int id1, const idVec3 &p1, idEntity *physics2, int id2, const idVec3 &p2 ) { // HUMANHEAD mdl:  Changed physics1 and physics2 to idEntity instead of the entity's physics object
	this->physics1 = physics1;
	this->id1 = id1;
	this->p1 = p1;
	this->physics2 = physics2;
	this->id2 = id2;
	this->p2 = p2;
}

/*
================
idForce_Spring::Evaluate
================
*/
void idForce_Spring::Evaluate( int time ) {
	float length;
	idMat3 axis;
	idVec3 pos1, pos2, velocity1, velocity2, force, dampingForce;
	impactInfo_t info;

	pos1 = p1;
	pos2 = p2;
	velocity1 = velocity2 = vec3_origin;

	if ( physics1.IsValid() ) { // HUMANHEAD mdl:  Added IsValid()
		axis = physics1->GetPhysics()->GetAxis( id1 ); // HUMANHEAD:  Added GetPhysics()
		pos1 = physics1->GetPhysics()->GetOrigin( id1 ); // HUMANHEAD:  Added GetPhysics()
		pos1 += p1 * axis;
		if ( damping > 0.0f ) {
			physics1->GetPhysics()->GetImpactInfo( id1, pos1, &info ); // HUMANHEAD:  Added GetPhysics()
			velocity1 = info.velocity;
		}
	}

	if ( physics2.IsValid() ) { // HUMANHEAD mdl:  Added IsValid()
		axis = physics2->GetPhysics()->GetAxis( id2 ); // HUMANHEAD:  Added GetPhysics()
		pos2 = physics2->GetPhysics()->GetOrigin( id2 ); // HUMANHEAD:  Added GetPhysics()
		pos2 += p2 * axis;
		if ( damping > 0.0f ) {
			physics2->GetPhysics()->GetImpactInfo( id2, pos2, &info ); // HUMANHEAD:  Added GetPhysics()
			velocity2 = info.velocity;
		}
	}

	force = pos2 - pos1;

#ifdef _HH_FLOAT_PROTECTION //HUMANHEAD rww
	if (FLOAT_IS_NAN(velocity1.x) || FLOAT_IS_NAN(velocity1.y) || FLOAT_IS_NAN(velocity1.z) ||
		FLOAT_IS_NAN(velocity2.x) || FLOAT_IS_NAN(velocity2.y) || FLOAT_IS_NAN(velocity2.z)) {
		gameLocal.DWarning( "idForce_Spring::Evaluate: NaN velocity." );
		return;
	}
#endif //HUMANHEAD END
	if (force == vec3_origin) { //HUMANHEAD rww
		//gameLocal.Warning( "idForce_Spring::Evaluate: force equal to zero." );
		dampingForce = vec3_origin;
	}
	else { //HUMANHEAD END
		dampingForce = ( damping * ( ((velocity2 - velocity1) * force) / (force * force) ) ) * force;
	}
	length = force.Normalize();

	// if the spring is stretched
	if ( length > restLength ) {
		if ( Kstretch > 0.0f ) {
			force = ( Square( length - restLength ) * Kstretch ) * force - dampingForce;
			if ( physics1.IsValid() ) { // HUMANHEAD mdl:  Added IsValid()
				physics1->GetPhysics()->AddForce( id1, pos1, force ); // HUMANHEAD:  Added GetPhysics()
			}
			if ( physics2.IsValid() ) { // HUMANHEAD mdl:  Added IsValid()
				physics2->GetPhysics()->AddForce( id2, pos2, -force ); // HUMANHEAD:  Added GetPhysics()
			}
		}
	}
	else {
		if ( Kcompress > 0.0f ) {
			force = ( Square( length - restLength ) * Kcompress ) * force - dampingForce;
			if ( physics1.IsValid() ) { // HUMANHEAD mdl:  Added IsValid()
				physics1->GetPhysics()->AddForce( id1, pos1, -force ); // HUMANHEAD:  Added GetPhysics()
			}
			if ( physics2.IsValid() ) { // HUMANHEAD mdl:  Added IsValid()
				physics2->GetPhysics()->AddForce( id2, pos2, force ); // HUMANHEAD:  Added GetPhysics()
			}
		}
	}
}

/*
================
idForce_Spring::RemovePhysics
================
*/
void idForce_Spring::RemovePhysics( const idEntity *phys ) { // HUMANHEAD mdl:  Changed to take entity instead of its physics object
	if ( physics1.GetEntity() == phys ) { // HUMANHEAD mdl:  Added GetEntity()
		physics1 = NULL;
	}
	if ( physics2.GetEntity() == phys ) { // HUMANHEAD mdl:  Added GetEntity()
		physics2 = NULL;
	}
}

// HUMANHEAD mdl
void idForce_Spring::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( Kstretch );
	savefile->WriteFloat( Kcompress );
	savefile->WriteFloat( damping );
	savefile->WriteFloat( restLength );
	physics1.Save( savefile );
	savefile->WriteInt( id1 );
	savefile->WriteVec3( p1 );
	physics2.Save( savefile );
	savefile->WriteInt( id2 );
	savefile->WriteVec3( p2 );
}

void idForce_Spring::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( Kstretch );
	savefile->ReadFloat( Kcompress );
	savefile->ReadFloat( damping );
	savefile->ReadFloat( restLength );
	physics1.Restore( savefile );
	savefile->ReadInt( id1 );
	savefile->ReadVec3( p1 );
	physics2.Restore( savefile );
	savefile->ReadInt( id2 );
	savefile->ReadVec3( p2 );
}
// HUMANHEAD END

