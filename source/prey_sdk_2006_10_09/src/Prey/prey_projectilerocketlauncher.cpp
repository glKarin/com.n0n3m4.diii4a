#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileRocketLauncher
	
***********************************************************************/
const idEventDef EV_SpawnModelProxyLocal( "<spawnModelProxyLocal>" );

CLASS_DECLARATION( hhProjectile, hhProjectileRocketLauncher )
	EVENT( EV_SpawnModelProxyLocal,		hhProjectileRocketLauncher::Event_SpawnModelProxyLocal )
	EVENT( EV_SpawnFxFlyLocal,			hhProjectileRocketLauncher::Event_SpawnFxFlyLocal )
	
	EVENT( EV_Collision_Flesh,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_AltMetal,		hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Wood,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Stone,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Glass,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Liquid,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_CardBoard,		hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Tile,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Forcefield,		hhProjectileRocketLauncher::Event_Collision_Explode )
	//EVENT( EV_Collision_Chaff,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Pipe,			hhProjectileRocketLauncher::Event_Collision_Explode )
	EVENT( EV_Collision_Wallwalk,		hhProjectileRocketLauncher::Event_Collision_Explode )

	EVENT( EV_AllowCollision_Chaff,		hhProjectileRocketLauncher::Event_AllowCollision_Collide )
END_CLASS

/*
================
hhProjectileRocketLauncher::Spawn
================
*/
void hhProjectileRocketLauncher::Spawn() {
}

/*
================
hhProjectileRocketLauncher::~hhProjectileRocketLauncher
================
*/
hhProjectileRocketLauncher::~hhProjectileRocketLauncher() {
	SAFE_REMOVE( modelProxy );
}

/*
=================
hhProjectileRocketLauncher::Hide
=================
*/
void hhProjectileRocketLauncher::Hide() {
	hhProjectile::Hide();

	if( modelProxy.IsValid() ) {
		modelProxy->Hide();
	}
}

/*
=================
hhProjectileRocketLauncher::Show
=================
*/
void hhProjectileRocketLauncher::Show() {
	hhProjectile::Show();

	if( modelProxy.IsValid() ) {
		modelProxy->Show();
	}
}

/*
=================
hhProjectileRocketLauncher::RemoveProjectile
=================
*/
void hhProjectileRocketLauncher::RemoveProjectile( const int removeDelay ) {
	hhProjectile::RemoveProjectile( removeDelay );

	if( modelProxy.IsValid() ) {
		modelProxy->PostEventMS( &EV_Remove, removeDelay );
	}
}

/*
================
hhProjectileRocketLauncher::Launch
================
*/
void hhProjectileRocketLauncher::Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	hhFxInfo fxInfo;

	ProcessEvent( &EV_SpawnModelProxyLocal ); //rww - was using BroadcastEventDef

	hhProjectile::Launch( start, axis, pushVelocity, timeSinceFire, launchPower, dmgPower );

	if( modelProxy.IsValid() ) {
		fxInfo.SetEntity( this );
		modelProxy->BroadcastFxInfoAlongBonePrefix( &spawnArgs, "fx_blood", "joint_bloodFx", false ); //rww - don't broadcast
	}
}

/*
================
hhProjectileRocketLauncher::Event_SpawnFxFlyLocal
================
*/
void hhProjectileRocketLauncher::Event_SpawnFxFlyLocal( const char* defName ) {
	hhFxInfo fxInfo;

	if( modelProxy.IsValid() ) {
		fxInfo.SetEntity( this );
		modelProxy->SpawnFxAlongBonePrefixLocal( &spawnArgs, "fx_fly", "joint_flyFx", &fxInfo );
	}
}

/*
=================
hhProjectileRocketLauncher::Event_SpawnModelProxyLocal
=================
*/
void hhProjectileRocketLauncher::Event_SpawnModelProxyLocal() {
	idDict args = spawnArgs;

	static const idMat3 pitchedOverAxis( idAngles(-90.0f, 0.0f, 0.0f).ToMat3() );

	args.Delete( "spawnclass" );
	args.Delete( "name" );
	args.Delete( "spawn_entnum" ); //HUMANHEAD rww - yeah, might not be smart to try to spawn in the same entity slot.

	args.Set( "owner", GetName() );
	args.SetVector( "origin", GetOrigin() );
	args.SetMatrix( "rotation", pitchedOverAxis * GetAxis() );
	args.SetBool( "transferDamage", false );
	args.SetBool( "solid", false );
	if (!modelProxy.IsValid()) {
		modelProxy = gameLocal.SpawnEntityTypeClient( hhGenericAnimatedPart::Type, &args ); //rww - proxy now localized
		if( modelProxy.IsValid() ) {
			modelProxy->fl.networkSync = false;
			modelProxy->Bind( this, true );
			modelProxy->CycleAnim( "idle", ANIMCHANNEL_ALL );
		}
	}

	//Get rid of our model.  The modelProxy is our model now.
	if (!gameLocal.isClient) {
		SetModel( "" );
	}
}

/*
================
hhProjectileRocketLauncher::Save
================
*/
void hhProjectileRocketLauncher::Save( idSaveGame *savefile ) const {
	modelProxy.Save( savefile );
}

/*
================
hhProjectileRocketLauncher::Restore
================
*/
void hhProjectileRocketLauncher::Restore( idRestoreGame *savefile ) {
	modelProxy.Restore( savefile );
}

void hhProjectileRocketLauncher::ClientPredictionThink( void ) {
	if (!gameLocal.isNewFrame) { //HUMANHEAD rww
		return;
	}

	//rww - this code is duplicated here since the rocket projectile on the client is a little special-cased
	// HUMANHEAD: cjr - if this projectile recently struck a portal, then attempt to portal it
	if ( (thinkFlags & TH_MISC1) && collidedPortal.IsValid() ) {
		GetPhysics()->SetLinearVelocity( collideVelocity );
		collidedPortal->PortalProjectile( this, collideLocation, collideLocation + collideVelocity );
		collidedPortal = NULL;
		collideLocation = vec3_origin;
		collideVelocity = vec3_origin;
		BecomeInactive(TH_MISC1);
	}
	// HUMANHEAD END

	RunPhysics();

	if ( thinkFlags & TH_MISC2 ) {
		UpdateLight();
	}
}

/*
================
hhProjectileRocketLauncher::Event_AllowCollision_Collide
================
*/
void hhProjectileRocketLauncher::Event_AllowCollision_Collide( const trace_t* collision ) {
	idThread::ReturnInt( 1 );
}

/***********************************************************************

  hhProjectileChaff
	
***********************************************************************/
const idEventDef EV_CollidedWithChaff( "<collidedWithChaff>", "tv", 'd' );
CLASS_DECLARATION( hhProjectile, hhProjectileChaff )
	EVENT( EV_Touch,		hhProjectileChaff::Event_Touch )
	//FIXME: null out all of the events
END_CLASS

/*
================
hhProjectileChaff::Spawn
================
*/
void hhProjectileChaff::Spawn() {
	decelStart = SEC2MS( spawnArgs.GetFloat("decelStart") ) + gameLocal.GetTime();
	decelEnd = SEC2MS( spawnArgs.GetFloat("decelDuration") ) + decelStart;

	BecomeActive( TH_TICKER );
}

/*
================
hhProjectileChaff::Launch
================
*/
void hhProjectileChaff::Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	hhProjectile::Launch( start, axis, pushVelocity, timeSinceFire, launchPower, dmgPower );

	cachedVelocity = GetPhysics()->GetLinearVelocity();

	fl.takedamage = false;
	physicsObj.DisableImpact();
}

/*
================
hhProjectileChaff::Collide
================
*/
bool hhProjectileChaff::Collide( const trace_t& collision, const idVec3& velocity ) {
	// Let the target know
	idEntity *ent = gameLocal.GetTraceEntity( collision );
	if ( ent && ent->RespondsTo( EV_CollidedWithChaff ) ) {
		ent->PostEventMS( &EV_CollidedWithChaff, 0, &collision, velocity );
	}

	return true;//Always stop after collision
}

/*
================
hhProjectileChaff::DetermineContents
================
*/
int hhProjectileChaff::DetermineContents() {
	// Removed PROJECTILE
	return CONTENTS_BLOCK_RADIUSDAMAGE | CONTENTS_OWNER_TO_OWNER | CONTENTS_SHOOTABLE;
}

/*
================
hhProjectileChaff::DetermineClipmask
================
*/
int hhProjectileChaff::DetermineClipmask() {
	// Removed SHOOTABLE, added PROJECTILE
	return CONTENTS_PROJECTILE|CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_CORPSE|CONTENTS_WATER|CONTENTS_FORCEFIELD;
}


/*
================
hhProjectileChaff::Ticker
================
*/
void hhProjectileChaff::Ticker() {
	float scale = 0.0f;
	if( gameLocal.GetTime() > decelStart && gameLocal.GetTime() < decelEnd ) {
		scale = hhMath::Sin( DEG2RAD(hhMath::MidPointLerp( 0.0f, 30.0f, 90.0f, 1.0f - hhUtils::CalculateScale(gameLocal.GetTime(), decelStart, decelEnd))) );

		GetPhysics()->SetLinearVelocity( cachedVelocity * hhMath::ClampFloat(0.05f, 1.0f, scale) );
	}
}

/*
================
hhProjectileChaff::Event_Touch
================
*/
void hhProjectileChaff::Event_Touch( idEntity *other, trace_t *trace ) {
	//Supposed to be empty
}

/*
================
hhProjectileChaff::Save
================
*/
void hhProjectileChaff::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( decelStart );
	savefile->WriteInt( decelEnd );
	savefile->WriteVec3( cachedVelocity );
}

/*
================
hhProjectileChaff::Restore
================
*/
void hhProjectileChaff::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( decelStart );
	savefile->ReadInt( decelEnd );
	savefile->ReadVec3( cachedVelocity );
}
