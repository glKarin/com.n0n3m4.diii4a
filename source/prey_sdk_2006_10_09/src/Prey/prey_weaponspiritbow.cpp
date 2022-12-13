#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhSpiritBowFireController
	
***********************************************************************/
CLASS_DECLARATION( hhWeaponFireController, hhSpiritBowFireController )
END_CLASS

/*
=================
hhSpiritBowFireController::AmmoRequired
=================
*/
int hhSpiritBowFireController::AmmoRequired() const {
	if( owner->IsDeathWalking() || owner->IsPossessed() ) {
		return 0;
	} 
	else { // CJR: will use any available ammo until the player is at zero ammo
		int ammoAmount = owner->inventory.ammo[ GetAmmoType() ];
		if ( ammoAmount > 0 && ammoAmount < ammoRequired ) {
			return ammoAmount;
		}
	}

	return ammoRequired;
}

/*
=================
hhSpiritBowFireController::GetProjectileDict
=================
*/
const idDict* hhSpiritBowFireController::GetProjectileDict() const
{
	//if( owner->inventory.maxHealth > 100 && !owner->IsDeathWalking() && !gameLocal.isMultiplayer )
	//	return gameLocal.FindEntityDefDict( dict->GetString("def_projectileSuper"), false );

	return hhWeaponFireController::GetProjectileDict();
}

/***********************************************************************

  hhWeaponSpiritBow
	
***********************************************************************/
const idEventDef EV_UpdateBowVision( "updateBowVision" );
const idEventDef EV_StartSeeThroughWalls( "startBowVision" );
const idEventDef EV_FadeOutSeeThroughWalls( "fadeOutBowVision" );
const idEventDef EV_StopSeeThroughWalls( "stopBowVision" );
const idEventDef EV_BowVisionIsEnabled( "visionIsEnabled", NULL, 'd' );

CLASS_DECLARATION( hhWeaponZoomable, hhWeaponSpiritBow )
	EVENT( EV_UpdateBowVision,			hhWeaponSpiritBow::Event_UpdateBowVision )
	EVENT( EV_StartSeeThroughWalls,		hhWeaponSpiritBow::Event_StartSeeThroughWalls )
	EVENT( EV_FadeOutSeeThroughWalls,	hhWeaponSpiritBow::Event_FadeOutSeeThroughWalls )
	EVENT( EV_StopSeeThroughWalls,		hhWeaponSpiritBow::Event_StopSeeThroughWalls )
	EVENT( EV_BowVisionIsEnabled,		hhWeaponSpiritBow::Event_BowVisionIsEnabled )
END_CLASS

/*
=================
hhWeaponSpiritBow::~hhWeaponSpiritBow
=================
*/
hhWeaponSpiritBow::~hhWeaponSpiritBow() {
	if( BowVisionIsEnabled() ) {
		BowVisionIsEnabled( false );
		StopBowVision();
	}
}

/*
=================
hhWeaponSpiritBow::Spawn
=================
*/
void hhWeaponSpiritBow::Spawn() {
	updateRover = 0; //rww - must be initialized! (sync'd over net)
	BowVisionIsEnabled( false );
}

/*
=================
hhWeaponSpiritBow::BeginAltAttack
=================
*/
void hhWeaponSpiritBow::BeginAltAttack( void ) {
	if (owner->inventory.requirements.bCanUseBowVision) {
		hhWeapon::BeginAltAttack();
	}
}

/*
=================
hhWeaponSpiritBow::Ticker
=================
*/
void hhWeaponSpiritBow::Ticker() {
	// Fade out the alt-mode effect
	if (!fadeAlpha.IsDone(gameLocal.GetTime())) {
		float alpha = fadeAlpha.GetCurrentValue(gameLocal.GetTime());
		owner->playerView.SetViewOverlayColor(idVec4(alpha, alpha, alpha, alpha));
	}
}

/*
=================
hhWeaponSpiritBow::GetProxyOf
=================
*/
hhTargetProxy *hhWeaponSpiritBow::GetProxyOf( const idEntity *target ) {
	hhTargetProxy *proxy;
	// Determine whether this target is proxied by searching their bind list
	for( idEntity* entity = target->GetTeamChain(); entity; entity = entity->GetTeamChain() ) {
		if( entity && entity->IsType(hhTargetProxy::Type) ) {
			proxy = static_cast<hhTargetProxy *>(entity);
			if( owner == proxy->GetOwner() ) {
				return proxy;
			}
		}
	}

	return NULL;
}

/*
=================
hhWeaponSpiritBow::ProxyShouldBeVisible
=================
*/
#define NOTVISIBLE		0
#define ISVISIBLE		1
#define FORCEVISIBILITY	2
bool hhWeaponSpiritBow::ProxyShouldBeVisible( const idEntity* ent ) {
	int visibilityType = ent->spawnArgs.GetInt("bowVisibilityType", "1");

	if( visibilityType == FORCEVISIBILITY ) {
		return true;
	}

	return ent->fl.takedamage && visibilityType >= ISVISIBLE;
}

/*
=================
hhWeaponSpiritBow::StopBowVision
=================
*/
void hhWeaponSpiritBow::StopBowVision() {
	if( !owner.IsValid() || !owner.GetEntity() || !owner->IsType(hhPlayer::Type) ) {
		return;
	}

	// Remove overlay
	if( owner->IsSpiritWalking() ) {
		owner->playerView.SetViewOverlayMaterial( declManager->FindMaterial(owner->spawnArgs.GetString("mtr_Spiritwalk")) );
	}
	else {
		owner->playerView.SetViewOverlayMaterial( NULL );
	}
}

/*
=================
hhWeaponSpiritBow::Event_UpdateBowVision
=================
*/
void hhWeaponSpiritBow::Event_UpdateBowVision() {
	hhTargetProxy *proxy  = NULL;
	idEntity *ent = NULL;

	// In order to hit all possible entities (active or inactive) at low cost, we use a rover
	// to traverse the entity list a little each tick, making a complete traversal about once
	// a second.

	float fuse = altFireController->GetProjectileDict()->GetFloat( "fuse" );
	idVec3 velocity = altFireController->GetProjectileDict()->GetVector( "velocity" );
	float maxDistance = velocity.Length() * fuse;
	float distSquared = maxDistance * maxDistance;

	// for all damageable active entities in the fuse radius
	for (int ix=0; ix<BOWVISION_ROVER_STRIDE; ix++) {
		if (++updateRover >= MAX_GENTITIES) {
			updateRover = 0;
		}
		ent = gameLocal.entities[updateRover];
		if (!ent || owner==ent ) {
			continue;
		}

		if( !ProxyShouldBeVisible(ent) ) {
			continue;
		}

		// PVS Check?

		if ((ent->GetOrigin() - GetOrigin()).LengthSqr() > distSquared) {
			continue;
		}

		// if already has proxy, continue
		proxy = GetProxyOf(ent);
		if (proxy) {
			proxy->StayAlive();
			continue;
		}

		if (!gameLocal.isClient) {
			proxy = static_cast<hhTargetProxy *>( gameLocal.SpawnObject(dict->GetString("def_targetproxy")) );
			if (proxy) {
				proxy->SetOriginal(ent);
				proxy->SetOwner(owner.GetEntity());
				proxy->UpdateVisualState();
			}
		}
	}
}

/*
=================
hhWeaponSpiritBow::Event_StartSeeThroughWalls
=================
*/
void hhWeaponSpiritBow::Event_StartSeeThroughWalls() {
	// Cancel any pending removals/fades
	CancelEvents( &EV_StopSeeThroughWalls );

	updateRover = 0;	// start at begining of list

	fadeAlpha.Init(gameLocal.GetTime(), BOWVISION_FADEIN_DURATION, 0.0f, 1.0f);

	StartSound( "snd_altmodefadein", SND_CHANNEL_ANY, 0, true, NULL );

	// Apply overlay material
	const idMaterial *mat = declManager->FindMaterial( spawnArgs.GetString("mtr_overlay") );
	owner->playerView.SetViewOverlayMaterial( mat );
	owner->playerView.SetViewOverlayColor( idVec4(1.0f, 1.0f, 1.0f, 0.0f) );

	BowVisionIsEnabled( true );
}

/*
=================
hhWeaponSpiritBow::Event_FadeOutSeeThroughWalls
	Fading the effect out, notify proxies to start fading too
=================
*/
void hhWeaponSpiritBow::Event_FadeOutSeeThroughWalls() {
	idEntity *ent;
	hhTargetProxy *proxy;

	fadeAlpha.Init(gameLocal.GetTime(), BOWVISION_FADEOUT_DURATION, 1.0f, 0.0f);
	owner->playerView.SetViewOverlayColor(colorWhite);

	StartSound( "snd_altmodefadeout", SND_CHANNEL_ANY, 0, true, NULL );

/*FIXME: Should use this format instead for speed
for ( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
    if ( ent->IsType( idLight::Type ) ) {
        idLight *light = static_cast<idLight *>(ent);
    }
}*/
	//TODO: Could store all created(owned) proxies in a list, for faster freeing here
	// Unmark all proxies that were set for this owner
	for (int ix=0; ix<gameLocal.num_entities; ix++) {
		ent = gameLocal.entities[ix];
		if (!ent || !ent->IsType(hhTargetProxy::Type)) {
			continue;
		}

		proxy = static_cast<hhTargetProxy *>(ent);
		if (owner==proxy->GetOwner()) {
			proxy->ProxyFinished();
		}
	}

	PostEventMS( &EV_StopSeeThroughWalls, BOWVISION_FADEOUT_DURATION );

	BowVisionIsEnabled( false );
}

/*
=================
hhWeaponSpiritBow::Event_StopSeeThroughWalls
=================
*/
void hhWeaponSpiritBow::Event_StopSeeThroughWalls() {
	BowVisionIsEnabled( false );
	StopBowVision();
}

/*
=================
hhWeaponSpiritBow::Event_BowVisionIsEnabled
=================
*/
void hhWeaponSpiritBow::Event_BowVisionIsEnabled() {
	idThread::ReturnInt( BowVisionIsEnabled() );
}

/*
================
hhWeaponSpiritBow::Save
================
*/
void hhWeaponSpiritBow::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( fadeAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( fadeAlpha.GetDuration() );
	savefile->WriteFloat( fadeAlpha.GetStartValue() );
	savefile->WriteFloat( fadeAlpha.GetEndValue() );

	savefile->WriteInt( updateRover );
	savefile->WriteBool( visionEnabled );
}

/*
================
hhWeaponSpiritBow::Restore
================
*/
void hhWeaponSpiritBow::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadFloat( set );	// idInterpolate<float>
	fadeAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetStartValue( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetEndValue( set );

	savefile->ReadInt( updateRover );
	savefile->ReadBool( visionEnabled );
}

/*
=================
hhWeaponSpiritBow::WriteToSnapshot
rww - write applicable weapon values to snapshot
=================
*/
void hhWeaponSpiritBow::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(visionEnabled, 1);
	msg.WriteBits(updateRover, GENTITYNUM_BITS);

//	msg.WriteFloat(fadeAlpha.GetCurrentValue(gameLocal.time));
	msg.WriteFloat(fadeAlpha.GetDuration());
//	msg.WriteFloat(fadeAlpha.GetEndTime());
	msg.WriteFloat(fadeAlpha.GetEndValue());
	msg.WriteFloat(fadeAlpha.GetStartTime());
	msg.WriteFloat(fadeAlpha.GetStartValue());

	hhWeapon::WriteToSnapshot(msg);
}

/*
=================
hhWeaponSpiritBow::ReadFromSnapshot
rww - read applicable weapon values from snapshot
=================
*/
void hhWeaponSpiritBow::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	visionEnabled = !!msg.ReadBits(1);
	updateRover = msg.ReadBits(GENTITYNUM_BITS);

	fadeAlpha.SetDuration(msg.ReadFloat());
	fadeAlpha.SetEndValue(msg.ReadFloat());
	fadeAlpha.SetStartTime(msg.ReadFloat());
	fadeAlpha.SetStartValue(msg.ReadFloat());

	hhWeapon::ReadFromSnapshot(msg);
}
