// game_mover.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//=============================================================================
// hhMover
//=============================================================================

CLASS_DECLARATION( idMover, hhMover )
END_CLASS


void hhMover::Spawn(void) {

	if ( GetPhysics()->GetClipModel() && spawnArgs.GetBool( "unblockable", "0" ) ) {
		// HUMANHEAD pdm: forcing unblockable off when noclipmodel is set, not valid to have push without clipmodel
  		//gameLocal.Printf( "Setting to nopushsupported" );
 		physicsObj.SetPusher( 0 | PUSHFL_UNBLOCKABLE );
  	}
  	
  	if ( spawnArgs.GetBool( "nonsolid" ) ) {
		BecomeNonSolid();
	}

}

void hhMover::BecomeNonSolid() {
	// Somewhat copied from idMoveable
	physicsObj.SetContents( CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( 0 );
}


//=============================================================================
// hhMoverWallwalk
//	Mover capable of doing wallwalk fading
//=============================================================================

CLASS_DECLARATION(hhMover, hhMoverWallwalk)
	EVENT(EV_Activate,			hhMoverWallwalk::Event_Activate)
END_CLASS

void hhMoverWallwalk::Spawn(void) {
	wallwalkOn = spawnArgs.GetBool("active");
	flicker	= spawnArgs.GetBool("flicker");

	// Get skin references (already precached)	
	onSkin = declManager->FindSkin( spawnArgs.GetString("skinOn") );
	offSkin = declManager->FindSkin( spawnArgs.GetString("skinOff") );	

	if (wallwalkOn) {
		SetSkin( onSkin );
		alphaOn.Init(gameLocal.time, 0, 1.0f, 1.0f);
		SetShaderParm(5, 1);
	}
	else {
		SetSkin( offSkin );
		alphaOn.Init(gameLocal.time, 0, 0.0f, 0.0f);
		SetShaderParm(5, 0);
	}
	alphaOn.SetHermiteParms(WALLWALK_HERM_S1, WALLWALK_HERM_S2);
	SetShaderParm(4, alphaOn.GetCurrentValue(gameLocal.time));
	UpdateVisuals();
}

void hhMoverWallwalk::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( alphaOn.GetStartTime() );	// hhHermiteInterpolate<float>
	savefile->WriteFloat( alphaOn.GetDuration() );
	savefile->WriteFloat( alphaOn.GetStartValue() );
	savefile->WriteFloat( alphaOn.GetEndValue() );
	savefile->WriteFloat( alphaOn.GetS1() );
	savefile->WriteFloat( alphaOn.GetS2() );

	savefile->WriteBool(wallwalkOn);
	savefile->WriteBool(flicker);
	savefile->WriteSkin(onSkin);
	savefile->WriteSkin(offSkin);
}

void hhMoverWallwalk::Restore( idRestoreGame *savefile ) {
	float set, set2;

	savefile->ReadFloat( set );			// hhHermiteInterpolate<float>
	alphaOn.SetStartTime( set );
	savefile->ReadFloat( set );
	alphaOn.SetDuration( set );
	savefile->ReadFloat( set );
	alphaOn.SetStartValue(set);
	savefile->ReadFloat( set );
	alphaOn.SetEndValue( set );
	savefile->ReadFloat( set );
	savefile->ReadFloat( set2 );
	alphaOn.SetHermiteParms(set, set2);

	savefile->ReadBool(wallwalkOn);
	savefile->ReadBool(flicker);
	savefile->ReadSkin(onSkin);
	savefile->ReadSkin(offSkin);
}

void hhMoverWallwalk::Think() {
	hhMover::Think();
	if (thinkFlags & TH_THINK) {
		SetShaderParm(4, alphaOn.GetCurrentValue(gameLocal.time));
		if (alphaOn.IsDone(gameLocal.time)) {
			BecomeInactive(TH_THINK);
			if (!wallwalkOn) {
				SetSkin( offSkin );
				SetShaderParm(5, 0);	// Not completely on
			}
			else {
				SetShaderParm(5, 1);	// Tell material to stay on
			}
		}
	}
}

void hhMoverWallwalk::SetWallWalkable(bool on) {
	wallwalkOn = on;

	float curAlpha = alphaOn.GetCurrentValue(gameLocal.time);

	if (wallwalkOn) {	// Turning on
		BecomeActive(TH_THINK);
		SetSkin( onSkin );
		StartSound( "snd_powerup", SND_CHANNEL_ANY );
		alphaOn.Init(gameLocal.time, WALLWALK_TRANSITION_TIME, curAlpha, 1.0f );
		alphaOn.SetHermiteParms(WALLWALK_HERM_S1, WALLWALK_HERM_S2);
		SetShaderParm(5, 0);	// Not completely on
	}
	else {				// Turning off
		BecomeActive(TH_THINK);
		StartSound( "snd_powerdown", SND_CHANNEL_ANY );
		alphaOn.Init(gameLocal.time, WALLWALK_TRANSITION_TIME, curAlpha, 0.0f);
		alphaOn.SetHermiteParms(WALLWALK_HERM_S1, 1.0f);	// no overshoot
		SetShaderParm(5, 0);	// Not completely on
	}
}

void hhMoverWallwalk::Event_Activate(idEntity *activator) {
	SetWallWalkable(!wallwalkOn);
}


//=============================================================================
//=============================================================================
//
// hhExplodeMover
//
// Mover explodes from a specific point to the destination when triggered
//=============================================================================
//=============================================================================

CLASS_DECLARATION( hhMover, hhExplodeMover )
	EVENT( EV_PostSpawn,	hhExplodeMover::Event_PostSpawn )
	EVENT( EV_Activate,	   	hhExplodeMover::Event_Trigger )
END_CLASS

void hhExplodeMover::Spawn(void) {
	moveDelay = spawnArgs.GetFloat( "moveDelay", "0" );
	PostEventMS( &EV_PostSpawn, 0 );
}

void hhExplodeMover::Event_PostSpawn( void ) {
	// Set up the destination location after this object is triggered
	dest_position = spawnArgs.GetVector( "destOrigin", "0 0 0" );
}

void hhExplodeMover::Save(idSaveGame *savefile) const {
	savefile->WriteInt(oldContents);
	savefile->WriteInt(oldClipMask);
	savefile->WriteFloat(moveDelay);
}

void hhExplodeMover::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt(oldContents);
	savefile->ReadInt(oldClipMask);
	savefile->ReadFloat(moveDelay);
}

void hhExplodeMover::Event_Trigger( idEntity *activator ) {
	// When triggered, blast self to the original position
	oldContents = GetPhysics()->GetContents();
	GetPhysics()->SetContents( 0 );

	oldClipMask = GetPhysics()->GetClipMask();
	GetPhysics()->SetClipMask( 0 );

	BeginMove( NULL );
}

void hhExplodeMover::DoneMoving( void ) {
	idMover::DoneMoving();
	GetPhysics()->SetContents( oldContents );
	GetPhysics()->SetClipMask( oldClipMask );
}

//=============================================================================
//=============================================================================
//
// hhExplodeMoverOrigin
//
// Origin object for exploding movers.
//=============================================================================
//=============================================================================

CLASS_DECLARATION( idEntity, hhExplodeMoverOrigin )
	EVENT( EV_Activate,	   	hhExplodeMoverOrigin::Event_Trigger )
END_CLASS

void hhExplodeMoverOrigin::Spawn(void) {
}

void hhExplodeMoverOrigin::Event_Trigger( idEntity *activator ) {
	int count = 0;

/*FIXME: Should use this format instead for speed
for ( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
    if ( ent->IsType( idLight::Type ) ) {
        idLight *light = static_cast<idLight *>(ent);
    }
}*/
	// When triggered, locate explode movers than have this as their origin entity, and then trigger them
	for (int i = 0; i < gameLocal.num_entities; i++ ) {
		idEntity *ent = gameLocal.entities[i];
		if ( !ent || !ent->IsType( hhExplodeMover::Type ) ) {
			continue;
		}

		hhExplodeMover *mover = static_cast<hhExplodeMover *>(ent);
		if( mover->targets.Num() > 0 && mover->targets[0].GetEntity() == this ) { // Trigger all explode movers that have this origin as a target
			mover->PostEventSec( &EV_Activate, mover->GetMoveDelay(), this );
			count++;
		}
	}
}
