//----------------------------------------------------------------
// VehicleStatic.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "VehicleStatic.h"

CLASS_DECLARATION( rvVehicle, rvVehicleStatic )
	EVENT( AI_ScriptedAnim, rvVehicleStatic::Event_ScriptedAnim )
	EVENT( AI_ScriptedDone,	rvVehicleStatic::Event_ScriptedDone )
	EVENT( AI_ScriptedStop,	rvVehicleStatic::Event_ScriptedStop )
END_CLASS

rvVehicleStatic::rvVehicleStatic ( void ) {
}

rvVehicleStatic::~rvVehicleStatic ( void ) {
}

/*
================
rvVehicleStatic::Spawn
================
*/
void rvVehicleStatic::Spawn( void ) {
	BecomeActive( TH_THINK );		
}

/*
================
rvVehicleStatic::AddDriver
================
*/
int rvVehicleStatic::AddDriver ( int position, idActor* driver ) {
	int pos = rvVehicle::AddDriver( position, driver );
	
	if( pos < 0 ) {
		return pos;
	}

	if( GetHud() ) {
		GetHud()->HandleNamedEvent( "hideGunInfo" );
	}

	return pos;
}

/*
================
rvVehicleStatic::RemoveDriver
================
*/
bool rvVehicleStatic::RemoveDriver ( int position, bool force ) {
	bool result = rvVehicle::RemoveDriver( position, force );

	if( !result ) {
		return result;
	}

	if( GetHud() ) {
		GetHud()->HandleNamedEvent( "showGunInfo" );
	}

	return result;
}

/*
================
rvVehicleStatic::UpdateHUD
================
*/
void rvVehicleStatic::UpdateHUD( idActor* driver, idUserInterface* gui ) {
	if( driver && driver->IsType( idPlayer::GetClassType() ) ) {
		static_cast<idPlayer*>(driver)->UpdateHudStats( gui );
	}
}

/*
================
rvVehicleStatic::Event_ScriptedAnim
================
*/
void rvVehicleStatic::Event_ScriptedAnim( const char* animname, int blendFrames, bool loop, bool endWithIdle ) {
	vfl.endWithIdle = endWithIdle;
	if ( loop ) {
		PlayCycle ( ANIMCHANNEL_TORSO, animname, blendFrames );
	} else {
		PlayAnim ( ANIMCHANNEL_TORSO, animname, blendFrames );
	}
	vfl.scripted = true;
}

/*
================
rvVehicleStatic::Event_ScriptedDone
================
*/
void rvVehicleStatic::Event_ScriptedDone( void ) {
	idThread::ReturnInt( !vfl.scripted );
}

/*
================
rvVehicleStatic::Event_ScriptedStop
================
*/
void rvVehicleStatic::Event_ScriptedStop( void ) {
	vfl.scripted = false;

	if ( vfl.endWithIdle ) {
		PlayCycle( ANIMCHANNEL_TORSO, spawnArgs.GetString( "idle" ), 2 );
	}
}
