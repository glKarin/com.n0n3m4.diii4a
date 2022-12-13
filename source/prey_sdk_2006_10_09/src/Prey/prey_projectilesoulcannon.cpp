#include "../idlib/precompiled.h"
#pragma hdrstop

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#include "prey_local.h"

/***********************************************************************

  hhProjectileSoulCannon
	
***********************************************************************/

const idEventDef EV_FindSoulEnemy( "<findSoulEnemy>", NULL );

CLASS_DECLARATION( hhProjectile, hhProjectileSoulCannon )
	EVENT( EV_FindSoulEnemy,		hhProjectileSoulCannon::Event_FindEnemy )
END_CLASS

//=============================================================================
//
// hhProjectileSoulCannon::Spawn
//
//=============================================================================

void hhProjectileSoulCannon::Spawn() {
	hhProjectile::Spawn();

	BecomeActive( TH_THINK );

	PostEventSec( &EV_FindSoulEnemy, 0.5f ); // Find an enemy shortly after being launched

	maxVelocity = spawnArgs.GetFloat( "maxVelocity", "400" );
	maxEnemyDist = spawnArgs.GetFloat( "maxEnemyDist", "4096" );

	thrustDir = vec3_origin;
}

//=============================================================================
//
// hhProjectileSoulCannon::Think
//
//=============================================================================

void hhProjectileSoulCannon::Think( void ) {
	// run physics
	RunPhysics();

	// Thrust toward enemy
	if ( thinkFlags & TH_THINK && thrustDir != vec3_origin ) {
		idVec3 vel = GetPhysics()->GetLinearVelocity();
		vel += thrustDir * spawnArgs.GetFloat( "soulThrust", "5.0" );

		if ( vel.Length() > maxVelocity ) { // Cap the velocity
			vel.Normalize();
			vel *= maxVelocity;
		}

		GetPhysics()->SetLinearVelocity( vel );
		GetPhysics()->SetAxis( vel.ToMat3() );
	}

	//HUMANHEAD: aob
	if (thinkFlags & TH_TICKER) {
		Ticker();
	}
	//HUMANHEAD

	Present();
}

//=============================================================================
//
// hhProjectileSoulCannon::Event_FindEnemy
//
// Finds a new enemy every second
//=============================================================================

void hhProjectileSoulCannon::Event_FindEnemy( void ) {
	int				i;
	idEntity		*ent;
	idActor			*actor;	
	trace_t			tr;

	PostEventSec( &EV_FindSoulEnemy, 1.0f );

	for ( i = 0; i < gameLocal.num_entities; i++ ) {
		ent = gameLocal.entities[ i ];

		if ( !ent || !ent->IsType( idActor::Type ) ) {
			continue;
		}

		if ( ent == this || ent == this->GetOwner() ) {
			continue;
		}

		// Ignore dormant entities!
		if( ent->IsHidden() || ent->fl.isDormant ) { // HUMANHEAD JRM - changed to fl.isDormant
			continue;			
		}

		actor = static_cast<idActor *>( ent );
		if ( ( actor->health <= 0 ) ) {
			continue;
		}

		idVec3 center = actor->GetPhysics()->GetAbsBounds().GetCenter();

		// Cannot see this enemy because he is too far away
		if ( maxEnemyDist > 0.0f && ( center - GetOrigin() ).LengthSqr() > maxEnemyDist * maxEnemyDist )
			continue;

		// Quick trace to the center of the potential enemy
		if ( !gameLocal.clip.TracePoint( tr, GetOrigin(), center, 1.0f, this ) ) {
			thrustDir = center - GetOrigin();
			thrustDir.Normalize();
			break;
		}
	}
}

/*
================
hhProjectileSoulCannon::Save
================
*/
void hhProjectileSoulCannon::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( maxVelocity );
	savefile->WriteFloat( maxEnemyDist );
	savefile->WriteVec3( thrustDir );
}

/*
================
hhProjectileSoulCannon::Restore
================
*/
void hhProjectileSoulCannon::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( maxVelocity );
	savefile->ReadFloat( maxEnemyDist );
	savefile->ReadVec3( thrustDir );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build