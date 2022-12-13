#include "../idlib/precompiled.h"
#pragma hdrstop

/*
add particles to bone
make wait: -1 work (removes self now)
*/

#include "prey_local.h"

const idEventDef EV_PlayPainIdle("<ppi>", NULL);
const idEventDef EV_ResetOrgan("<resetorg>", NULL);

CLASS_DECLARATION( hhAnimatedEntity, hhOrganTrigger )
	EVENT( EV_Enable,			hhOrganTrigger::Event_Enable )
	EVENT( EV_Disable,			hhOrganTrigger::Event_Disable )
	EVENT( EV_PlayIdle,			hhOrganTrigger::Event_PlayIdle)
	EVENT( EV_PlayPainIdle,		hhOrganTrigger::Event_PlayPainIdle)
	EVENT( EV_ResetOrgan,		hhOrganTrigger::Event_ResetOrgan)
	EVENT( EV_PostSpawn,		hhOrganTrigger::Event_PostSpawn )
END_CLASS


//--------------------------------
// hhOrganTrigger::~hhOrganTrigger
//--------------------------------
hhOrganTrigger::~hhOrganTrigger() {
	SAFE_REMOVE( trigger );
}

//--------------------------------
// hhOrganTrigger::Event_PostSpawn
//--------------------------------
void hhOrganTrigger::Event_PostSpawn() {
	SpawnTrigger();
}

//--------------------------------
// hhOrganTrigger::Spawn
//--------------------------------
void hhOrganTrigger::Spawn( void ) {
	fl.takedamage = true;
	GetPhysics()->SetContents( CONTENTS_SOLID );

	idleAnim = GetAnimator()->GetAnim( "idle" );
	painAnim = GetAnimator()->GetAnim( "pain" );
	resetAnim = GetAnimator()->GetAnim( "reset" );
	painIdleAnim = GetAnimator()->GetAnim( "painidle" );

	trigger = NULL;
	if (!gameLocal.isClient) {
		if (gameLocal.isMultiplayer) {
			PostEventMS(&EV_PostSpawn, 0);
		} else { //spawn right now. i don't want to break save/load stuff...
			SpawnTrigger();
		}
	}

	ProcessEvent( &EV_PlayIdle );

	ProcessEvent( (spawnArgs.GetBool("enabled", "1")) ? &EV_Enable : &EV_Disable );
}

void hhOrganTrigger::Save(idSaveGame *savefile) const {
	savefile->WriteInt( idleAnim );
	savefile->WriteInt( painAnim );
	savefile->WriteInt( resetAnim );
	savefile->WriteInt( painIdleAnim );
	trigger.Save(savefile);
}

void hhOrganTrigger::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( idleAnim );
	savefile->ReadInt( painAnim );
	savefile->ReadInt( resetAnim );
	savefile->ReadInt( painIdleAnim );
	trigger.Restore(savefile);
}

//--------------------------------
// hhOrganTrigger::SpawnTrigger
//--------------------------------
void hhOrganTrigger::SpawnTrigger() {
	idDict args = spawnArgs;

	args.SetVector( "mins", idVec3(-1.0f, -1.0f, -1.0f) );
	args.SetVector( "maxs", idVec3(1.0f, 1.0f, 1.0f) );
	args.SetBool( "noTouch", true );
	args.SetBool( "enabled", false );
	args.Delete( "spawnclass" );
	args.Delete( "name" );
	args.Delete( "model" );
	trigger = gameLocal.SpawnObject( spawnArgs.GetString("def_trigger"), &args );
	if( trigger.IsValid() ) {
		//Bound entities are removed when their master is removed
		trigger->Bind( this, true );
	}
}

//--------------------------------
// hhOrganTrigger::Killed
//--------------------------------
void hhOrganTrigger::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location ) {
	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, painAnim, gameLocal.GetTime(), 100 );
	int opentime = GetAnimator()->GetAnim( painAnim )->Length();
	PostEventMS( &EV_PlayPainIdle, opentime );

	if( !trigger.IsValid() ) {
		return;
	}

	trigger->TriggerAction( attacker );

	Disable();

	if( trigger->wait >= 0 ) {
		PostEventMS( &EV_ResetOrgan, trigger->nextTriggerTime - gameLocal.GetTime() );
	}
}

//--------------------------------
// hhOrganTrigger::Enable
//--------------------------------
void hhOrganTrigger::Enable() {
	fl.takedamage = true;

	if( trigger.IsValid() ) {
		trigger->Enable();
	}
}

//--------------------------------
// hhOrganTrigger::Disable
//--------------------------------
void hhOrganTrigger::Disable() {
	fl.takedamage = false;

	if( trigger.IsValid() ) {
		trigger->Disable();
	}
}

//--------------------------------
// hhOrganTrigger::Event_Enable
//--------------------------------
void hhOrganTrigger::Event_Enable() {
	Enable();
}

//--------------------------------
// hhOrganTrigger::Event_Disable
//--------------------------------
void hhOrganTrigger::Event_Disable() {
	Disable();
}

//--------------------------------
// hhOrganTrigger::Event_PlayIdle
//--------------------------------
void hhOrganTrigger::Event_PlayIdle( void ) {
	Enable();

	if( idleAnim ) {
		GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
		GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, idleAnim, gameLocal.GetTime(), 0 );
	}
}

//--------------------------------
// hhOrganTrigger::Event_PlayPainIdle
//--------------------------------
void hhOrganTrigger::Event_PlayPainIdle( void ) {
	if( painIdleAnim ) {
		GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
		GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, painIdleAnim, gameLocal.GetTime(), 100 );
	}
}

//--------------------------------
// hhOrganTrigger::Event_ResetOrgan
//--------------------------------
void hhOrganTrigger::Event_ResetOrgan( void ) {
	if( resetAnim ) {
		GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
		GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, resetAnim, gameLocal.GetTime(), 100 );
		int opentime = GetAnimator()->GetAnim( resetAnim )->Length();
		PostEventMS( &EV_PlayIdle, opentime );
	}
}