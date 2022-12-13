//**************************************************************************
//**
//** hhAnimated
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_PositionDefaultPose("<positionDefaultPose>");
const idEventDef EV_StartDefaultAnim("<startDefaultAnim>");
const idEventDef EV_SetAnim( "setAnim", "s" );	// nla
const idEventDef EV_IsAnimDone( "isAnimDone", "d", 'd' );

CLASS_DECLARATION( idAnimated, hhAnimated )
	EVENT( EV_Activate,				hhAnimated::Event_Activate )
	EVENT( EV_SetAnim,				hhAnimated::Event_SetAnim )
	EVENT( EV_IsAnimDone,			hhAnimated::Event_IsAnimDone )
	EVENT( EV_AnimDone,				hhAnimated::Event_AnimDone )
	EVENT( EV_Animated_Start,		hhAnimated::Event_Start )
	EVENT( EV_PositionDefaultPose,	hhAnimated::Event_PositionDefaultPose )
	EVENT( EV_StartDefaultAnim,		hhAnimated::Event_StartDefaultAnim )
	EVENT( EV_Footstep,				hhAnimated::Event_Footstep )
	EVENT( EV_FootstepLeft,			hhAnimated::Event_Footstep )
	EVENT( EV_FootstepRight,		hhAnimated::Event_Footstep )
END_CLASS

/*
===============
hhAnimated::Spawn
================
*/
void hhAnimated::Spawn() {
	isAnimDone = true;

	fl.takedamage = health > 0;

	const char* startAnimName = spawnArgs.GetString( "start_anim" );
	if( !startAnimName || !startAnimName[0] ) {
		PostEventMS( &EV_PositionDefaultPose, 0 );
	}
}

void hhAnimated::Save(idSaveGame *savefile) const {
	savefile->WriteBool( isAnimDone );
}

void hhAnimated::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( isAnimDone );
}

/*
===============
hhAnimated::Damage
================
*/
void hhAnimated::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( CheckRagdollDamage( inflictor, attacker, dir, damageDefName, location ) ) {
		return;
	}

	idAnimated::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

/*
===============
hhAnimated::Killed
================
*/
void hhAnimated::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	idAnimBlend* pAnim = NULL;//GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, "death", gameLocal.time, 0, false );
	
	if( !af.IsActive() ) {
		PostEventMS( &EV_StartRagdoll, (pAnim) ? pAnim->Length() : 0 );
	}
}

/*
===============
hhAnimated::StartRagdoll
================
*/
bool hhAnimated::StartRagdoll( void ) {
	bool	parentAns = false;
	float 	slomoStart, slomoEnd;

	// NOTE: These first two calls are ripped from idAnimated::StartRagdoll()
	// if no AF loaded
	if ( !af.IsLoaded() ) {
		return false;
	}

	// if the AF is already active
	if ( af.IsActive() ) {
		return true;
	}
	
	// HUMANHEAD nla - Added to fix the issue with inproper offsets for fixed constraints.  Taken from idAFEntity::LoadAF (Caused ragdolls to be 'fixed' to the center of the world/0,0,0)
	af.GetPhysics()->Rotate( GetPhysics()->GetAxis().ToRotation() );
	af.GetPhysics()->Translate( GetPhysics()->GetOrigin() );
	// HUMANHEAD END

	parentAns = idAnimated::StartRagdoll();
	
	// HUMANHEAD nla - Allow the ragdolls to slow into the ragdoll
	slomoStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoStart", "-1.6" );
	slomoEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoEnd", "0.8" );

	// do the first part of the death in slow motion
	af.GetPhysics()->SetTimeScaleRamp( slomoStart, slomoEnd );

	// Allow ragdolls to be active when first started
	//   This logic ripped from idAFEntity::LoadAF
	af.GetPhysics()->PutToRest();
	af.GetPhysics()->Activate();

	af.UpdateAnimation();
	GetAnimator()->CreateFrame( gameLocal.time, true );
	UpdateVisuals();
	// HUMANHEAD END
	
	return( parentAns );	
}

/*
================
hhAnimated::UpdateAnimationControllers
================
*/
bool hhAnimated::UpdateAnimationControllers( void ) {
	bool retValue = idAnimated::UpdateAnimationControllers();

	JawFlap(GetAnimator());

	return retValue;
}

/*
================
hhAnimated::Event_SetAnim
================
*/
void hhAnimated::Event_SetAnim( const char *animname ) {
	assert( animname );
	
	anim = GetAnimator()->GetAnim( animname );
	HH_ASSERT( anim );

	ProcessEvent( &EV_Activate, this );
}

/*
================
hhAnimated::Event_Start
================
*/
void hhAnimated::Event_Start( void ) {
	idAnimated::Event_Start();
	
	isAnimDone = false;
}

/*
===============
hhAnimated::Event_AnimDone
================
*/
void hhAnimated::Event_AnimDone( int animIndex ) {
	idAnimated::Event_AnimDone( animIndex );

	if( !spawnArgs.GetBool("resetDefaultAnim") ) {
		isAnimDone = true;
	} else {
		PostEventMS( &EV_StartDefaultAnim, 0 );
	}
}

/*
================
hhAnimated::Event_IsAnimDone
================
*/
void hhAnimated::Event_IsAnimDone( int timeMS ) {
	idThread::ReturnInt( (int) isAnimDone );
}

/*
===============
hhAnimated::Event_PositionDefaultPose
================
*/
void hhAnimated::Event_PositionDefaultPose() {
	const char* animName = spawnArgs.GetString( "defaultPose" );

	if( !animName || !animName[0] ) {
		return;
	}
	
	int pAnim = GetAnimator()->GetAnim( animName );
	GetAnimator()->SetFrame( ANIMCHANNEL_ALL, pAnim, spawnArgs.GetInt("pose_frame", "1"), gameLocal.GetTime(), 0 );

	GetAnimator()->ForceUpdate();
	UpdateModel();
	UpdateAnimation();
}

/*
===============
hhAnimated::Event_StartDefaultAnim
================
*/
void hhAnimated::Event_StartDefaultAnim() {
	const char* animName = spawnArgs.GetString( "start_anim" );

	if( !animName || animName[0] ) {
		return;
	}

	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
	int pAnim = GetAnimator()->GetAnim( animName );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, pAnim, gameLocal.time, FRAME2MS(blendFrames) );
}


/*
===============
hhAnimated::Event_Activate
================
*/
void hhAnimated::Event_Activate( idEntity *_activator ) {
	if (spawnArgs.GetBool("ragdollOnTrigger")) {
		StartRagdoll();
		return;
	}

	idAnimated::Event_Activate(_activator);
}

/*
===============
hhAnimated::Event_Footstep
================
*/
void hhAnimated::Event_Footstep() {
	StartSound( "snd_footstep", SND_CHANNEL_BODY3, 0, false, NULL );
}
