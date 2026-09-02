//----------------------------------------------------------------
// Vehicle.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "VehicleRigid.h"

CLASS_DECLARATION( rvVehicle, rvVehicleRigid )
END_CLASS

rvVehicleRigid::rvVehicleRigid ( void ) {
}

rvVehicleRigid::~rvVehicleRigid ( void ) {
	SetPhysics( NULL );
}

/* 
================
rvVehicleRigid::Spawn
================
*/
void rvVehicleRigid::Spawn( void ) {
	physicsObj.SetSelf( this );	

	SetClipModel ( );

	physicsObj.SetOrigin( GetPhysics()->GetOrigin ( ) );
	physicsObj.SetAxis ( GetPhysics()->GetAxis ( ) );
	physicsObj.SetContents( CONTENTS_BODY );
	physicsObj.SetClipMask( MASK_PLAYERSOLID|CONTENTS_VEHICLECLIP );
	physicsObj.SetFriction ( spawnArgs.GetFloat ( "friction_linear", "1" ), spawnArgs.GetFloat ( "friction_angular", "1" ), spawnArgs.GetFloat ( "friction_contact", "1" ) );
	physicsObj.SetBouncyness ( spawnArgs.GetFloat ( "bouncyness", "0.6" ) );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	SetPhysics( &physicsObj );
	
	animator.CycleAnim ( ANIMCHANNEL_ALL, animator.GetAnim( spawnArgs.GetString( "anim", "idle" ) ), gameLocal.time, 0 );	

	BecomeActive( TH_THINK );		
}

/*
================
rvVehicleRigid::SetClipModel
================
*/
void rvVehicleRigid::SetClipModel ( void ) {	
	idStr			clipModelName;
	idTraceModel	trm;
	float			mass;

	// rebuild clipmodel
	spawnArgs.GetString( "clipmodel", "", clipModelName );

	// load the trace model
	if ( clipModelName.Length() ) {
		if ( !collisionModelManager->TrmFromModel( gameLocal.GetMapName(), clipModelName, trm ) ) {
			gameLocal.Error( "rvVehicleRigid '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
			return;
		}

		physicsObj.SetClipModel( new idClipModel( trm ), spawnArgs.GetFloat ( "density", "1" ) );
	} else {
		physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), spawnArgs.GetFloat ( "density", "1" ) );
	}

	if ( spawnArgs.GetFloat ( "mass", "0", mass ) && mass > 0 )	{
		physicsObj.SetMass ( mass );
	}
}


/*
================
rvVehicleRigid::RunPrePhysics
================
*/
void rvVehicleRigid::RunPrePhysics ( void ) {
	storedVelocity = physicsObj.GetLinearVelocity();
}

/*
================
rvVehicleRigid::RunPostPhysics
================
*/
void rvVehicleRigid::RunPostPhysics ( void ) {

	if ( autoCorrectionBegin + 1250 > static_cast<unsigned>( gameLocal.time )) {
		autoCorrectionBegin = 0;

		float lengthSq = physicsObj.GetLinearVelocity().LengthSqr();
		if ( !autoCorrectionBegin && ( ( storedVelocity * 0.4f ).LengthSqr() >= lengthSq ) || ( lengthSq < 0.01f ) ) {
			autoCorrectionBegin = gameLocal.time;
		}
	}

	if ( g_debugVehicle.GetInteger() == 10 ) {
		gameLocal.Printf( "Speed: %f\n", physicsObj.GetLinearVelocity().Length() );
	}
}

/*
================
rvVehicleRigid::WriteToSnapshot
================
*/
void rvVehicleRigid::WriteToSnapshot( idBitMsgDelta &msg ) const {
	rvVehicle::WriteToSnapshot( msg );
	physicsObj.WriteToSnapshot( msg );
}

/*
================
rvVehicleRigid::ReadFromSnapshot
================
*/
void rvVehicleRigid::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	rvVehicle::ReadFromSnapshot( msg );
	physicsObj.ReadFromSnapshot( msg );
}

/*
================
rvVehicleRigid::Save
================
*/
void rvVehicleRigid::Save ( idSaveGame *savefile ) const {

	savefile->WriteVec3 ( storedVelocity ); // cnicholson: Added unsaved var
	savefile->WriteStaticObject ( physicsObj );
}

/*
================
rvVehicleRigid::Restore
================
*/
void rvVehicleRigid::Restore ( idRestoreGame *savefile ) {

	savefile->ReadVec3 ( storedVelocity ); // cnicholson: Added unrestored var

	physicsObj.SetSelf( this );	
	
	SetClipModel ( );

	savefile->ReadStaticObject ( physicsObj );
	RestorePhysics( &physicsObj );
}

/*
=====================
rvVehicleRigid::SkipImpulse
=====================
*/
bool rvVehicleRigid::SkipImpulse( idEntity* ent, int id ) {	
	return false;
}
