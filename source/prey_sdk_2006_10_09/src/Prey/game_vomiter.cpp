// Game_Vomiter.cpp
//
// spews vomit chunks periodically
//	triggering will cause an eruption

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_Erupt("erupt", "d");
const idEventDef EV_UnErupt("unerupt", NULL);
const idEventDef EV_DamagePulse("damagepulse", NULL);
const idEventDef EV_MovablePulse("<movable>");

const idEventDef EV_Broadcast_AssignFxErupt( "<assignFxErupt>", "e" );

CLASS_DECLARATION(hhAnimatedEntity, hhVomiter)
	EVENT( EV_Activate,	   				hhVomiter::Event_Trigger )
	EVENT( EV_Erupt,					hhVomiter::Event_Erupt )
	EVENT( EV_UnErupt,					hhVomiter::Event_UnErupt )
	EVENT( EV_PlayIdle,					hhVomiter::Event_PlayIdle )
	EVENT( EV_DamagePulse,				hhVomiter::Event_DamagePulse )
	EVENT( EV_MovablePulse,				hhVomiter::Event_MovablePulse )
	EVENT( EV_Broadcast_AssignFxErupt,	hhVomiter::Event_AssignFxErupt )
END_CLASS

hhVomiter::hhVomiter(void) {
	// mdl:  Moved this here to prevent uninitialized fxErupt from crashing the destructor
	fxErupt = NULL;
}

void hhVomiter::Spawn(void) {

	detectionTrigger = NULL;

	spawnArgs.GetFloat("minDelay", "3", mindelay);
	spawnArgs.GetFloat("maxDelay", "7", maxdelay);
	secBetweenDamage = spawnArgs.GetFloat("secBetweenDamage");
	secBetweenMovables = spawnArgs.GetFloat("secBetweenMovables");
	minMovableVel = spawnArgs.GetFloat("minMovableVel");
	maxMovableVel = spawnArgs.GetFloat("maxMovableVel");
	removeTime = spawnArgs.GetFloat("movableRemoveTime");
	numMovables = spawnArgs.GetInt("numMovables");
	goAwayChance = spawnArgs.GetFloat("GoAwayChance");

	idleAnim		= GetAnimator()->GetAnim("idle");
	painAnim		= GetAnimator()->GetAnim("pain");
	spewAnim		= GetAnimator()->GetAnim("spew");
	deathAnim		= GetAnimator()->GetAnim("death");

	bErupting = false;
	bIdling = false;

	// setup the clipModel
	GetPhysics()->SetContents( CONTENTS_SOLID );

	// Spawn trigger used for waking it up
	SpawnTrigger();

	//TODO: Spawn this the first time it erupts instead ?
	// Spawn FX system
	hhFxInfo fxInfo;
	fxInfo.SetStart( false );
	fxInfo.SetNormal( GetAxis()[2] );
	fxInfo.RemoveWhenDone( false );
	fxInfo.Triggered( true );
	BroadcastFxInfo(spawnArgs.GetString("fx_vomit"), GetOrigin(), GetAxis(), &fxInfo, &EV_Broadcast_AssignFxErupt );

	StartIdle();
	GoToSleep();
	fl.takedamage = true;
}

hhVomiter::~hhVomiter() {
	SAFE_REMOVE(fxErupt);
}

void hhVomiter::Save(idSaveGame *savefile) const {
	savefile->WriteBool(bErupting);
	savefile->WriteBool(bIdling);
	savefile->WriteBool(bAwake);
	savefile->WriteFloat(mindelay);
	savefile->WriteFloat(maxdelay);

	savefile->WriteFloat(secBetweenDamage);
	savefile->WriteFloat(secBetweenMovables);

	savefile->WriteInt(idleAnim);
	savefile->WriteInt(painAnim);
	savefile->WriteInt(spewAnim);
	savefile->WriteInt(deathAnim);
	savefile->WriteFloat(minMovableVel);
	savefile->WriteFloat(maxMovableVel);
	savefile->WriteFloat(removeTime);

	savefile->WriteInt(numMovables);
	savefile->WriteFloat(goAwayChance);

	savefile->WriteObject(fxErupt);

	detectionTrigger.Save(savefile);
}

void hhVomiter::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool(bErupting);
	savefile->ReadBool(bIdling);
	savefile->ReadBool(bAwake);
	savefile->ReadFloat(mindelay);
	savefile->ReadFloat(maxdelay);

	savefile->ReadFloat(secBetweenDamage);
	savefile->ReadFloat(secBetweenMovables);

	savefile->ReadInt(idleAnim);
	savefile->ReadInt(painAnim);
	savefile->ReadInt(spewAnim);
	savefile->ReadInt(deathAnim);
	savefile->ReadFloat(minMovableVel);
	savefile->ReadFloat(maxMovableVel);
	savefile->ReadFloat(removeTime);

	savefile->ReadInt(numMovables);
	savefile->ReadFloat(goAwayChance);

	savefile->ReadObject( reinterpret_cast<idClass *&>(fxErupt) );

	detectionTrigger.Restore(savefile);
}

void hhVomiter::SpawnTrigger() {
	idDict Args;

	hhUtils::PassArgs( spawnArgs, Args, "triggerpass_" );

	Args.Set( "target", name.c_str() );
	Args.Set( "bind", name.c_str() );

	Args.SetVector( "origin", GetOrigin() + spawnArgs.GetVector("offset_trigger") );
	Args.SetMatrix( "rotation", GetAxis() );
	detectionTrigger = gameLocal.SpawnObject( spawnArgs.GetString("def_trigger"), &Args );
}

void hhVomiter::DormantBegin( void ) {
}

void hhVomiter::DormantEnd( void ) {
}

void hhVomiter::Erupt(int repeat) {
	if (bErupting || health <= 0 || fl.isDormant) {
		if (fl.isDormant) {
			UnErupt();	// go to sleep, etc., so we can wake up again later
		}
		return;
	}

	bErupting = true;
	StartSound( "snd_erupt", SND_CHANNEL_ANY );
	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, spewAnim, gameLocal.time, 0);

	int startTime = GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->Length();
	PostEventMS( &EV_PlayIdle, startTime );

	PostEventMS( &EV_MovablePulse, 0 );	// Self repeating moveable spawn
	PostEventMS( &EV_DamagePulse, 0 );	// Self repeating damage pulse

	// Activate FX
	if (fxErupt) {
		fxErupt->PostEventMS( &EV_Activate, 0, this );
	}

	// Stop erupting in a while
	PostEventSec( &EV_UnErupt, spawnArgs.GetFloat("vomitTime") );
}

void hhVomiter::UnErupt() {
	CancelEvents(&EV_Erupt);			// Stop any pending events
	CancelEvents(&EV_DamagePulse);
	CancelEvents(&EV_MovablePulse);

	if (bErupting) {
		bErupting = false;
		// Deactivate FX
		if (fxErupt) {
			fxErupt->PostEventMS( &EV_Fx_KillFx, 0 );
		}
	}

	GoToSleep();
}

void hhVomiter::GoToSleep() {
	bAwake = false;
}

void hhVomiter::WakeUp() {
	if (!bAwake && !bErupting && health > 0) {
		bAwake = true;
		StartSound( "snd_wakeup", SND_CHANNEL_ANY );
		PostEventSec( &EV_Erupt, 0.1+mindelay + gameLocal.random.RandomFloat()*(maxdelay-mindelay), true);
	}
}

bool hhVomiter::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if (painAnim) {
		StartSound( "snd_pain", SND_CHANNEL_ANY );
		GetAnimator()->ClearAllAnims( gameLocal.time, 200 );
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, painAnim, gameLocal.time, 200);

		int startTime = GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->Length();
		PostEventMS( &EV_PlayIdle, startTime );
	}
	return( idEntity::Pain(inflictor, attacker, damage, dir, location) );
}

void hhVomiter::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {

	if (health + damage > 0) {	// Only do this the first death
		// Clear Event Queue of all eruptions
		CancelEvents(&EV_Erupt);
		CancelEvents(&EV_DamagePulse);
		if (bErupting) {
			UnErupt();
		}
		
		StartSound( "snd_die", SND_CHANNEL_ANY );

		// Spawn dead effect -- currently an explosion
		hhFxInfo fxInfo;
		fxInfo.SetNormal( GetAxis()[2] );
		fxInfo.RemoveWhenDone( true );
		BroadcastFxInfoPrefixed( "fx_death", GetOrigin(), -GetAxis(), &fxInfo );	

		ActivateTargets( attacker );

		StopIdle();
		GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, deathAnim, gameLocal.time, 0);
	}
	else {	// Post-death pain
//		GetAnimator()->ClearAllAnims( gameLocal.time, 200 );
//		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, painAnim, gameLocal.time, 200);
	}
}

void hhVomiter::StartIdle() {
	hhFxInfo fxInfo;

	if (idleAnim && !bIdling && health > 0) {
		PostEventMS(&EV_PlayIdle, 0);
		StartSound( "snd_idle", SND_CHANNEL_IDLE );
		bIdling = true;
	}
}

void hhVomiter::StopIdle() {
	CancelEvents(&EV_PlayIdle);
	StopSound( SND_CHANNEL_IDLE );
	bIdling = false;
}

//
// Events
//

void hhVomiter::Event_AssignFxErupt( hhEntityFx* fx ) {
	fxErupt = fx;
}

void hhVomiter::Event_PlayIdle() {
	GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 100);
}

void hhVomiter::Event_Trigger( idEntity *activator ) {
	WakeUp();
}

void hhVomiter::Event_Erupt(int repeat) {
	Erupt(repeat);
}

void hhVomiter::Event_UnErupt() {
	UnErupt();
}

void hhVomiter::Event_DamagePulse() {
	idEntity		*touch[ MAX_GENTITIES ];
	int				num;
	// Damage all entities touching trigger's clipmodel
	num = hhUtils::EntitiesTouchingClipmodel( detectionTrigger->GetPhysics()->GetClipModel(), touch, MAX_GENTITIES, MASK_SHOT_BOUNDINGBOX );
	for (int i = 0; i < num; i++ ) {
		if( !touch[i] || touch[i] == this ) {
			continue;
		}
		touch[i]->Damage(this, this, GetAxis()[2], spawnArgs.GetString("def_damage"), 1.0f, 0);
	}
	PostEventSec( &EV_DamagePulse, secBetweenDamage );
}

void hhVomiter::Event_MovablePulse() {
	// Spawn movables
	idDict args;
	idEntity *ent;
	idVec3 vel, dir;
	
	const char *movableName = spawnArgs.RandomPrefix("def_movable", gameLocal.random);
	if (movableName && *movableName) {
		args.Clear();
		args.SetVector("origin", GetOrigin());

		if (gameLocal.random.RandomFloat() < goAwayChance) {
			args.SetBool("removeOnCollision", true);
		}
		else {
			args.SetFloat("removeTime", removeTime);
		}

		for (int ix=0; ix<numMovables; ix++) {
			dir = GetAxis()[2];
			dir.x += gameLocal.random.CRandomFloat();
			dir.y += gameLocal.random.CRandomFloat();
			dir.z += gameLocal.random.RandomFloat()*3.0f;
			dir.NormalizeFast();
			vel = dir * (minMovableVel + gameLocal.random.RandomFloat()*(maxMovableVel-minMovableVel));
			ent = gameLocal.SpawnObject( movableName, &args );
			ent->GetPhysics()->SetLinearVelocity( vel );
			ent->fl.takedamage = false;		// don't let the radius damage affect them
		}
	}
	PostEventSec( &EV_MovablePulse, secBetweenMovables );	// Self repeating event
}
