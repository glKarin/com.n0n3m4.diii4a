
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idPhysics_Actor, hhPhysics_Delta )
END_CLASS

//============
// hhPhysics_Delta::hhPhysics_Delta
//============
hhPhysics_Delta::hhPhysics_Delta() {

	delta.Zero();
}		//. hhPhysics_Delta::hhPhysics_Delta()


//============
// hhPhysics_Delta::Evaluate
//============
bool hhPhysics_Delta::Evaluate( int timeStepMSec, int endTimeMSec ) {
	trace_t trace;
	idVec3 dest;
	idRotation rotation;
	float timeStep;
	idVec3 velocity;


	timeStep = MS2SEC( timeStepMSec );
	velocity = delta / timeStep;

	if ( delta == vec3_zero ) {
		Rest();
		return( false );
	}

	clipModel->Unlink();				// Taken from monster

	dest = GetOrigin() + delta;
	rotation.SetOrigin( GetOrigin() );
	rotation.SetAngle( 0 );

	//? Maybe model after the idPhysics_Monster::Slide move, where we 
	//  slide along scholng
	
	// If there was a collision, adjust the dest
	if ( gameLocal.clip.Motion( trace, GetOrigin(), dest, rotation, clipModel,
								GetAxis(), clipMask, self ) ) {
		if ( self->AllowCollision( trace ) ) {
			//gameLocal.Printf( "Hit something %.2f\n", trace.fraction );

			dest = trace.endpos;

			//? Move this elsewhere?
			self->Collide( trace, velocity );
		}		//. We run into what we hit = )

		//? Maybe we apply an impulse?
	}	//. We hit something

	//gameLocal.Printf( "Moving from %s to %s (%s)\n", GetOrigin().ToString(),
	//				  dest.ToString(), delta.ToString() );


	//! What about origin and axis?
	clipModel->Link( gameLocal.clip, self, 0, dest, GetAxis() );	// Taken from monster

	// We are done with delta, so zero it out
	delta.Zero();

	return( true );
}		//. hhPhysics_Delta::Evaluate( int, int );

	
//============
// hhPhysics_Delta::SetDelta
//============
void hhPhysics_Delta::SetDelta( const idVec3 &d ){

	delta = d;

	if ( delta != vec3_origin ) {
		Activate();
	}
}		//. hhPhysics_Delta::SetDelta( const idVec3 & )


//============
// hhPhysics_Delta::Activate
//============
void hhPhysics_Delta::Activate(){

	self->BecomeActive( TH_PHYSICS );
	
}		//. hhPhysics_Delta::Activate( const idVec3 & )


//============
// hhPhysics_Delta::Rest
//============
void hhPhysics_Delta::Rest(){

	self->BecomeInactive( TH_PHYSICS );
	
}		//. hhPhysics_Delta::Rest( const idVec3 & )



//================
//hhPhysics_Delta::SetAxis
//================
void hhPhysics_Delta::SetAxis( const idMat3 &newAxis, int id ) {
	// Ripped from idPhysics_Monster
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), newAxis );
	//? Activate();
}


//================
//hhPhysics_Delta::SetOrigin
//================
void hhPhysics_Delta::SetOrigin( const idVec3 &newOrigin, int id ) {
	// Ripped from idPhysics_Monster
	/*
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}
	*/
	clipModel->Link( gameLocal.clip, self, 0, newOrigin, clipModel->GetAxis() );
	//? Activate();
}

//================
//hhPhysics_Delta::Save
//================
void hhPhysics_Delta::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( delta );
}

//================
//hhPhysics_Delta::Restore
//================
void hhPhysics_Delta::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( delta );
}

