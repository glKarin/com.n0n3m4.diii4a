//**************************************************************************
//**
//** GAME_SPHEREPART.CPP
//**
//** SphereParts are generic environment objects that animate and can be 
//** interacted with in simple general ways:  Touched, shot / killed.
//**
//** Pulse Tubes should be able to:
//**	- Animate
//**	- Randomly play other anims
//**	- Take Pain
//**	- Die (?)
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// CLASS DECLARATIONS ------------------------------------------------------

const idEventDef EV_Pulse("pulse", NULL);
const idEventDef EV_PlayIdle("<playidle>", NULL);

CLASS_DECLARATION( hhAnimatedEntity, hhSpherePart )
	EVENT( EV_Pulse,					hhSpherePart::Event_Pulse )
	EVENT( EV_Activate,	   				hhSpherePart::Event_Trigger )
	EVENT( EV_PlayIdle,					hhSpherePart::Event_PlayIdle )
END_CLASS

// STATE DECLARATIONS -------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// hhSpherePart::Spawn
//
//==========================================================================

void hhSpherePart::Spawn(void) {
	GetPhysics()->SetContents( CONTENTS_BODY );

	fl.takedamage = true; // Allow the spherepart to be damaged

	spawnArgs.GetFloat("pulsetime", "10", pulseTime); // A pulsetime of zero will ensure that the object never pulses
	spawnArgs.GetFloat("pulserandom", "5", pulseRandom);

	idleAnim		= GetAnimator()->GetAnim("idle");
	painAnim		= GetAnimator()->GetAnim("pain");
	pulseAnim		= GetAnimator()->GetAnim("pulse");
	triggerAnim		= GetAnimator()->GetAnim("trigger");

	BecomeActive(TH_THINK);

	GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 100);
	StartSound( "snd_idle", SND_CHANNEL_ANY );

	if ( pulseAnim && pulseTime ) {
		PostEventSec(&EV_Pulse, pulseTime + gameLocal.random.RandomFloat() * pulseRandom);	
	}
}

void hhSpherePart::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( pulseTime );
	savefile->WriteFloat( pulseRandom );
	savefile->WriteInt( idleAnim );
	savefile->WriteInt( painAnim );
	savefile->WriteInt( pulseAnim );
	savefile->WriteInt( triggerAnim );
}

void hhSpherePart::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( pulseTime );
	savefile->ReadFloat( pulseRandom );
	savefile->ReadInt( idleAnim );
	savefile->ReadInt( painAnim );
	savefile->ReadInt( pulseAnim );
	savefile->ReadInt( triggerAnim );
}

//==========================================================================
//
// hhSpherePart::~hhSpherePart
//
//==========================================================================
hhSpherePart::~hhSpherePart() {
}

//==========================================================================
//
// hhSpherePart::Damage
//
//==========================================================================

void hhSpherePart::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	int startTime;


	if ( painAnim && !GetAnimator()->IsAnimPlaying( GetAnimator()->GetAnim( painAnim ) ) ) {
		StartSound( "snd_pain", SND_CHANNEL_ANY );
		GetAnimator()->ClearAllAnims( gameLocal.time, 100 );
		GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, painAnim, gameLocal.time, 100);

		startTime = GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->Length();
		PostEventMS( &EV_PlayIdle, startTime );
	}
}

//==========================================================================
//
// hhSpherePart::Event_Trigger
//
//==========================================================================

void hhSpherePart::Event_Trigger( idEntity *activator ) {
	int startTime;

	if(triggerAnim) {
		GetAnimator()->ClearAllAnims( gameLocal.time, 500 );
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, triggerAnim, gameLocal.time, 500);

		startTime = GetAnimator()->GetAnim( triggerAnim )->Length();
		PostEventMS( &EV_PlayIdle, startTime );
	}
}

//==========================================================================
//
// hhSpherePart::Event_Pulse
//
//==========================================================================

void hhSpherePart::Event_Pulse(void) {
	int startTime;

	if ( pulseAnim && pulseTime ) {
		StartSound( "snd_pulse", SND_CHANNEL_ANY );

		GetAnimator()->ClearAllAnims( gameLocal.time, 500 );
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, pulseAnim, gameLocal.time, 500);

		startTime = GetAnimator()->GetAnim( pulseAnim )->Length();
		PostEventMS( &EV_PlayIdle, startTime );

		PostEventSec(&EV_Pulse, pulseTime + gameLocal.random.RandomFloat() * pulseRandom);	
	}
}

//==========================================================================
//
// hhSpherePart::Event_PlayIdle
//
//==========================================================================

void hhSpherePart::Event_PlayIdle() {
	// We are playing the idle, cancel any pending idles
	CancelEvents( &EV_PlayIdle );

	GetAnimator()->ClearAllAnims( gameLocal.time, 100 );
	GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 100);
}

/**********************************************************************

hhGenericAnimatedPart

**********************************************************************/
CLASS_DECLARATION( hhAnimatedEntity, hhGenericAnimatedPart )
	EVENT( EV_PostSpawn,				hhGenericAnimatedPart::Event_PostSpawn )
END_CLASS

/*
============
hhGenericAnimatedPart::Spawn
============
*/
void hhGenericAnimatedPart::Spawn() {
	if( spawnArgs.GetBool("solid", "1") ) {
		fl.takedamage = true;
		GetPhysics()->SetContents( CONTENTS_SOLID );
	} else {
		fl.takedamage = false;
		GetPhysics()->SetContents( 0 );
		GetPhysics()->UnlinkClip();
	}

	//rww - make sure it's sent
	fl.networkSync = true;

	PostEventMS( &EV_PostSpawn, 0 );
}

void hhGenericAnimatedPart::Save(idSaveGame *savefile) const {
	owner.Save(savefile);
}

void hhGenericAnimatedPart::Restore( idRestoreGame *savefile ) {
	owner.Restore(savefile);
}

void hhGenericAnimatedPart::WriteToSnapshot( idBitMsgDelta &msg ) const
{
	GetPhysics()->WriteToSnapshot(msg);
	msg.WriteBits(owner.GetSpawnId(), 32);
}

void hhGenericAnimatedPart::ReadFromSnapshot( const idBitMsgDelta &msg )
{
	GetPhysics()->ReadFromSnapshot(msg);
	owner.SetSpawnId(msg.ReadBits(32));
}

void hhGenericAnimatedPart::ClientPredictionThink( void )
{
	Think();
}


/*
============
hhGenericAnimatedPart::SetOwner
============
*/
void hhGenericAnimatedPart::SetOwner( idEntity* pOwner ) {
	owner = pOwner;
}

/*
============
hhGenericAnimatedPart::LinkCombatModel
============
*/
void hhGenericAnimatedPart::LinkCombatModel( idEntity* self, const int renderModelHandle ) {
	hhAnimatedEntity::LinkCombatModel( (owner.IsValid()) ? owner.GetEntity() : this, renderModelHandle );
}

/*
============
hhGenericAnimatedPart::Damage
============
*/
void hhGenericAnimatedPart::Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location ) {
	if( owner.IsValid() && spawnArgs.GetBool("transferDamage") ) {
		owner->Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	} else {
		hhAnimatedEntity::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	}
}

/*
================
hhGenericAnimatedPart::PlayAnim
================
*/
int hhGenericAnimatedPart::PlayAnim( const char* animName, int channel ) {
	ClearAllAnims();

	int anim = GetAnimator()->GetAnim( animName );
	if( !anim ) {
		return 0;
	}

	GetAnimator()->PlayAnim( channel, anim, gameLocal.GetTime(), 0 );
	return GetAnimator()->CurrentAnim( channel )->GetEndTime() - gameLocal.GetTime();
}

/*
================
hhGenericAnimatedPart::CycleAnim
================
*/
void hhGenericAnimatedPart::CycleAnim( const char* animName, int channel ) {
	ClearAllAnims();

	int anim = GetAnimator()->GetAnim( animName );
	if( !anim ) {
		return;
	}

	GetAnimator()->CycleAnim( channel, anim, gameLocal.GetTime(), 0 );
}

/*
================
hhGenericAnimatedPart::ClearAllAnims
================
*/
void hhGenericAnimatedPart::ClearAllAnims() {
	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
}

/*
============
hhGenericAnimatedPart::Event_PostSpawn
============
*/
void hhGenericAnimatedPart::Event_PostSpawn() {
	idStr ownerName = spawnArgs.GetString( "owner" );
	if( ownerName.Length() ) {
		SetOwner( gameLocal.FindEntity(ownerName.c_str()) );
	}
}
