//**************************************************************************
//**
//** GAME_CILIA.CPP
//**
//** Specific type of SpherePart.  When shot, they retract for a while,
//** and trigger nearby cilia to retract as well.
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// CLASS DECLARATIONS ------------------------------------------------------

const idEventDef EV_StickOut( "stickout", NULL );
const idEventDef EV_TriggerNearby( "triggernearby", NULL );
const idEventDef EV_Idle( "idle", NULL );

CLASS_DECLARATION( hhSpherePart,	hhSphereCilia )
	EVENT( EV_Activate,	   			hhSphereCilia::Event_Trigger )
	EVENT( EV_TriggerNearby,   		hhSphereCilia::Event_TriggerNearby )
	EVENT( EV_StickOut,	   			hhSphereCilia::Event_StickOut )
	EVENT( EV_Idle,					hhSphereCilia::Event_Idle )	// JRM	
	EVENT( EV_Touch,				hhSphereCilia::Event_Touch )
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
// hhSphereCilia::Spawn
//
//==========================================================================

void hhSphereCilia::Spawn(void) {
	fl.takedamage	= true; // Allow the spherepart to be damaged
	fl.allowSpiritWalkTouch = true;
	bRetracted		= false;	// Start sticking out
	bAlreadyActivated = false; // Hasn't been touched yet

	//HUMANHEAD jsh PCF 4/26/06 allow creatures to trigger cilia
	GetPhysics()->SetContents( CONTENTS_TRIGGER | CONTENTS_RENDERMODEL );

	idleAnim		= GetAnimator()->GetAnim("idle");
	pullInAnim		= GetAnimator()->GetAnim("pullin");
	stickOutAnim	= GetAnimator()->GetAnim("stickout");

	spawnArgs.GetFloat( "nearbySize", "128", nearbySize );
	spawnArgs.GetFloat( "idleDelay", "-1", idleDelay );

	if( idleDelay < 0 ) { // negative number denotes a random start delay
		idleDelay = gameLocal.random.RandomFloat();
	}

	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time + idleDelay * 1000, 100 );
}

void hhSphereCilia::Save(idSaveGame *savefile) const {

	savefile->WriteInt( pullInAnim );
	savefile->WriteInt( stickOutAnim );
	savefile->WriteFloat( nearbySize );
	savefile->WriteFloat( idleDelay );
	savefile->WriteBool( bRetracted );
	savefile->WriteBool( bAlreadyActivated );
}

void hhSphereCilia::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( pullInAnim );
	savefile->ReadInt( stickOutAnim );
	savefile->ReadFloat( nearbySize );
	savefile->ReadFloat( idleDelay );
	savefile->ReadBool( bRetracted );
	savefile->ReadBool( bAlreadyActivated );
}

//==========================================================================
//
// hhSphereCilia::Damage
//
// Currently similar to 
//==========================================================================

void hhSphereCilia::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location ) {

	hhFxInfo fxInfo;

	if( bRetracted ) { // Don't trigger if already retracted
		return;
	}

	bRetracted = true;

	// Remove collision on the model
	GetPhysics()->SetContents(0);

	// Switch to the broken model
	SetModel( spawnArgs.GetString( "model_broken", "" ) );
	
	// Play an explode sound
	StartSound( "snd_explode", SND_CHANNEL_ANY, 0, true, NULL );

	fl.takedamage = false;
	fl.applyDamageEffects = false; // Cilia don't accept damage wounds, since they swap out models

	PostEventSec( &EV_TriggerNearby, 0.2f + gameLocal.random.RandomFloat() * 0.1 );

	ApplyEffect();
}

//==========================================================================
//
// hhSphereCilia::Event_Touch
//
//==========================================================================
void hhSphereCilia::Event_Touch( idEntity *other, trace_t *trace ) {
	Trigger( other );
}

//==========================================================================
//
// hhSphereCilia::Trigger
//
//==========================================================================

void hhSphereCilia::Trigger( idEntity *activator ) {
	if( bRetracted || !activator ) { // Don't trigger if already retracted
		return;
	}

	bRetracted = true;

	// Remove collision on the model
	GetPhysics()->SetContents(0);

	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, pullInAnim, gameLocal.time, 100);

	StartSound( "snd_in", SND_CHANNEL_ANY, 0, true, NULL );

	PostEventSec( &EV_TriggerNearby, 0.2f + gameLocal.random.RandomFloat() * 0.1 );
	PostEventSec( &EV_StickOut, 5.0f + idleDelay );

	// Trigger this cilia's targets the first time it is touched
	if ( !bAlreadyActivated ) {
		ActivateTargets( activator );
		bAlreadyActivated = true;
	}

	ApplyEffect();
}

//==========================================================================
//
// hhSphereCilia::Event_Trigger
//
//==========================================================================

void hhSphereCilia::Event_Trigger( idEntity *activator ) {
	Trigger( activator );
}

//==========================================================================
//
// hhSphereCilia::Event_TriggerNearby
//
// Trigger nearby cilia (to create a cascading retract effect)
//==========================================================================

void hhSphereCilia::Event_TriggerNearby( void ) {
	int				i;
	int				e;
	hhSphereCilia	*ent;
	idEntity		*entityList[ MAX_GENTITIES ];
	int				numListedEntities;
	idBounds		bounds;
	idVec3			org;

	org = GetPhysics()->GetOrigin();
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = org[i] - nearbySize;
		bounds[1][i] = org[i] + nearbySize;
	}

	// Find the first closest neighbor cilia to trigger
	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		if( !entityList[e]->IsType( hhSphereCilia::Type ) ) {
			continue;
		}

		ent = static_cast< hhSphereCilia * >( entityList[e] );
	
		if( ent->bRetracted ) {
			continue;
		}

		ent->Trigger( this );	
	}	
}

//==========================================================================
//
// hhSphereCilia::Event_StickOut
//
// Return the cilia to its original state
//==========================================================================

#define MAX_NEARBY_CILIA	8

void hhSphereCilia::Event_StickOut( void ) {
	int			i;
	int			num;
	idEntity	*touch[MAX_NEARBY_CILIA];
	idEntity	*hit;
	bool		canFit;

	// Check if the cilia can fit when sticking out
	num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), -1, touch, MAX_NEARBY_CILIA );

	canFit = true;
	for( i = 0; i < num; i++ ) {
		hit = touch[ i ];
		assert( hit );

		if(( hit == this )) {
			continue;
		}

		// Hit an entity, so reset the cilia to attempt to emerge in a bit
		PostEventSec( &EV_StickOut, 5.0f );
		return;
	}

	//HUMANHEAD jsh PCF 4/26/06 allow creatures to trigger cilia
	// Restore the proper collision
	GetPhysics()->SetContents( CONTENTS_TRIGGER | CONTENTS_RENDERMODEL );

	// Play the stick out anims and blend the idle back in
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, stickOutAnim, gameLocal.time, 100);
	PostEventMS(&EV_Idle, 100); // JRM

	StartSound( "snd_out", SND_CHANNEL_ANY, 0, true, NULL);

	bRetracted = false;
}

//
// Event_Idle
// 
// JRM
void hhSphereCilia::Event_Idle(void) { 

	assert(idleAnim);
	GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 200);
};

void hhSphereCilia::ApplyEffect( void ) {
	hhFxInfo fxInfo;
	fxInfo.SetNormal( GetAxis()[0] );
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_explode", GetOrigin() + (GetAxis()[0] * 16.0f), GetAxis(), &fxInfo );

	int num;
	int i;
	idEntity* ents[MAX_GENTITIES];

	num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_SHOT_BOUNDINGBOX|CONTENTS_RENDERMODEL, ents, MAX_GENTITIES );

	const char *damageType = spawnArgs.GetString("damageType", "damage_cilia");
	for( i = 0; i < num; i++ ) {
		if( ents[ i ] != this && !ents[ i ]->IsType(hhSphereCilia::Type) && !ents[ i ]->IsType(idAFAttachment::Type) ) {
			ents[ i ]->Damage( this, this, vec3_origin, damageType, 1.0f, INVALID_JOINT );
		}
	}

}

