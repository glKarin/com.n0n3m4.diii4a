//----------------------------------------------------------------
// Vehicle.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "VehicleMonster.h"
#include "../ai/VehicleAI.h"

CLASS_DECLARATION( rvVehicle, rvVehicleMonster )
END_CLASS

rvVehicleMonster::rvVehicleMonster ( void ) {
}

rvVehicleMonster::~rvVehicleMonster ( void ) {
}

/*
================
rvVehicleMonster::Spawn
================
*/
void rvVehicleMonster::Spawn( void ) {
	idDict dict;
	int count = 0;
	const idKeyValue * val = NULL;

	for (;;) {
		val = spawnArgs.MatchPrefix( "target", val );

		if ( !val )
			break;

		dict.Set( "target" + (( count ) ? idStr( count ) : idStr( "" ) ), val->GetValue() );
		count++;
	}

	dict.Set( "bind", name );
	dict.SetBool( "hide", true );

	driver = static_cast<rvVehicleAI *>( gameLocal.SpawnEntityDef( "ai_vehicle_driver", &dict ) );

	if ( driver ) {
		driver->SetVehicle( this );
		driver->GetPhysics()->SetOrigin( vec3_zero );
	} else {
		gameLocal.Warning( "Unable to find \"entityDef ai_vehicle_driver\"." );
	}
}

/*
================
rvVehicleMonster::SetClipModel
================
*/
void rvVehicleMonster::SetClipModel ( idPhysics & physicsObj ) {	
	idStr			clipModelName;
	idTraceModel	trm;
	float			mass;

	// rebuild clipmodel
	spawnArgs.GetString( "clipmodel", "", clipModelName );

	// load the trace model
	if ( clipModelName.Length() ) {
		if ( !collisionModelManager->TrmFromModel( gameLocal.GetMapName(), clipModelName, trm ) ) {
			gameLocal.Error( "rvVehicleMonster '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
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
rvVehicleMonster::Save
================
*/
void rvVehicleMonster::Save ( idSaveGame *savefile ) const {
	driver.Save ( savefile );
}

/*
================
rvVehicleMonster::Restore
================
*/
void rvVehicleMonster::Restore ( idRestoreGame *savefile ) {
	driver.Restore ( savefile );
}

/*
================
rvVehicleMonster::Think
================
*/
void rvVehicleMonster::Think ( void ) {
	if ( !driver ) {
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	stateThread.Execute();
	rvVehicle::Think();
}

/*
================
rvVehicleMonster::GetTargetOrigin
================
*/
const idVec3 & rvVehicleMonster::GetTargetOrigin ( void ) {
	if ( driver && driver->driver && driver->driver->pathTargetInfo.node ) {
		return driver->driver->pathTargetInfo.node->GetPhysics()->GetOrigin();
	}
	return vec3_origin;
}

/*
================
rvVehicleMonster::GetVectorToTarget
================
*/
idVec3 rvVehicleMonster::GetVectorToTarget ( void ) {
	if ( driver && driver->driver && driver->driver->pathTargetInfo.node ) {
		return driver->driver->pathTargetInfo.node->GetPhysics()->GetOrigin() - GetOrigin();
	}
	return vec3_origin;
}

/*
================
rvVehicleMonster::GetEnemyOrigin
================
*/
const idVec3 & rvVehicleMonster::GetEnemyOrigin ( void ) {
	if ( driver && driver->enemy.ent ) {
		return driver->enemy.ent->GetPhysics()->GetOrigin();
	}
	return vec3_origin;
}

/*
================
rvVehicleMonster::GetVectorToEnemy
================
*/
idVec3 rvVehicleMonster::GetVectorToEnemy ( void ) {
	if ( driver && driver->enemy.ent ) {
		//HACK: this is hideous, I'm ashamed.
		return ( driver->enemy.ent->GetPhysics()->GetOrigin() + idVec3( 0, 0, 36 ) )- GetOrigin();
	}
	return vec3_origin;
}

/*
================
rvVehicleMonster::LookAtEntity
================
*/
void rvVehicleMonster::LookAtEntity	( idEntity *ent, float duration ) {
	const idVec3 entOrigin	= ent->GetPhysics()->GetOrigin();
	const idVec3 origin		= GetOrigin();
	idVec3 toEnt			= entOrigin - origin;
	toEnt.Normalize();
	GetPhysics()->SetAxis( toEnt.ToMat3() );
}

/*
================
rvVehicleMonster::SkipImpulse
================
*/
bool rvVehicleMonster::SkipImpulse( idEntity* ent, int id ) {
	return false;
}
