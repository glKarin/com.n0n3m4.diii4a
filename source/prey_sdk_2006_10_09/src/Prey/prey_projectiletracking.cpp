#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileTracking
	
***********************************************************************/
const idEventDef EV_Guide( "<guide>" );
const idEventDef EV_Hover( "<hover>" );
const idEventDef EV_StartTracking( "<startTracking>" );
const idEventDef EV_StopTracking( "<stopTracking>" );

CLASS_DECLARATION( hhProjectile, hhProjectileTracking )
	EVENT( EV_Guide,					hhProjectileTracking::Event_TrackTarget )
	EVENT( EV_Hover,					hhProjectileTracking::Event_Hover )
	EVENT( EV_StartTracking,			hhProjectileTracking::Event_StartTracking )
	EVENT( EV_StopTracking,				hhProjectileTracking::Event_StopTracking )

	EVENT( EV_Collision_Flesh,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_AltMetal,		hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Wood,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Stone,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Glass,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Liquid,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_CardBoard,		hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Tile,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Forcefield,		hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Chaff,			hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Wallwalk,		hhProjectileTracking::Event_Collision_Explode )
	EVENT( EV_Collision_Pipe,			hhProjectileTracking::Event_Collision_Explode )

	EVENT( EV_AllowCollision_Chaff,		hhProjectileTracking::Event_AllowCollision_Collide )
END_CLASS



hhProjectileTracking::hhProjectileTracking() {
	spinAngle = 0.0f;	// Needs to be initialized before spawn since GetPhysicsToVisualTransform() is called before spawn
}

/*
================
hhProjectileTracking::Spawn
================
*/
void hhProjectileTracking::Spawn() {
	angularVelocity.Zero();
	velocity.Zero();

	cachedFovCos = idMath::Cos( DEG2RAD(spawnArgs.GetFloat("fov", "90")) );
	turnFactor = spawnArgs.GetFloat("turnfactor");
	updateRate = spawnArgs.GetInt("trackingUpdateRate");
	turnFactorAcc = spawnArgs.GetFloat( "turn_factor_accel", "1.1" );
	spinDelta = spawnArgs.GetFloat( "spin_delta" );

	//rww - some things are hhProjectileTracking when they shouldn't be or don't have proper properties. if this is so, complain.
	if (!updateRate) {
		gameLocal.Warning("hhProjectileTracking with an updateRate of 0 (possible infinite event queue).");
	}
}

/*
================
hhProjectileTracking::IsEnemyValid
================
*/
bool hhProjectileTracking::EnemyIsValid( idEntity* ent ) const {
	if ( ent && ent->IsType( idActor::Type ) ) {
		idActor *entActor = static_cast<idActor*>(ent);
		return ( entActor->GetHealth() > 0 && entActor->team!= 0 && !entActor->IsHidden() && gameLocal.InPlayerPVS(entActor) );
	}
	return ( ent && ent->GetHealth() > 0 && !ent->IsHidden() && gameLocal.InPlayerPVS(ent) );
}

/*
================
hhProjectileTracking::WhosClosest
================
*/
idEntity* hhProjectileTracking::WhosClosest( idEntity* possibleEnemy, idEntity* currentEnemy, float& currentEnemyDot, float& currentEnemyDist ) const {
	if( !possibleEnemy ) {
		return currentEnemy;
	}

	idVec3 enemyDir = DetermineEnemyPosition( possibleEnemy ) - GetOrigin();
	float cachedDist = enemyDir.Normalize();
	float cachedDot = enemyDir * GetAxis()[0];

	//AOB: Think about putting in tolerances for deciding which is better,
	if( cachedDot > cachedFovCos && cachedDot > currentEnemyDot && cachedDist <= currentEnemyDist ) {
		currentEnemyDot = cachedDot;
		currentEnemyDist = cachedDist;
		return possibleEnemy;
	}

	return currentEnemy;
}

/*
================
hhProjectileTracking::DetermineEnemy
================
*/
idEntity* hhProjectileTracking::DetermineEnemy() {
	idEntity*	possibleEnemy = NULL;
	float		currentEnemyDot = 0.0f;
	float		currentEnemyDist = CM_MAX_TRACE_DIST;
	idEntity*	localEnemy = NULL;
	
	if ( spawnArgs.GetInt( "trackPlayersOnly", "0" ) ) {
		for( int i = 0; i < gameLocal.numClients; i++ ) {
			if ( gameLocal.entities[ i ] ) {
				possibleEnemy = gameLocal.entities[i];
				if ( possibleEnemy && possibleEnemy->IsType( hhPlayer::Type ) ) {
					hhPlayer *player = static_cast<hhPlayer*>(possibleEnemy);
					if ( player && player->IsSpiritOrDeathwalking() ) {
						possibleEnemy = player->GetSpiritProxy();
					}
				}
				localEnemy = WhosClosest( possibleEnemy, localEnemy, currentEnemyDot, currentEnemyDist );
			}
		}

		return localEnemy;
	}
	int num = hhMonsterAI::allSimpleMonsters.Num();
	for( int index = 0; index < num; ++index ) {
		possibleEnemy = hhMonsterAI::allSimpleMonsters[ index ];
		if( EnemyIsValid(possibleEnemy) ) {
			localEnemy = WhosClosest( possibleEnemy, localEnemy, currentEnemyDot, currentEnemyDist );
		}
	}

	return localEnemy;
}

/*
================
hhProjectileTracking::Launch
================
*/
void hhProjectileTracking::Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	hhProjectile::Launch( start, axis, pushVelocity, timeSinceFire, launchPower, dmgPower );
	
	enemy = DetermineEnemy();
	
	float randomStartSpread = spawnArgs.GetFloat( "randomStartSpread", "0" );
	if ( randomStartSpread > 0.0 ) {
		velocity = spawnArgs.GetVector( "velocity" ).Length() * hhUtils::RandomSpreadDir( GetPhysics()->GetAxis(), 1.0 );
	} else {
		velocity = spawnArgs.GetVector( "velocity" );
	}
	angularVelocity = spawnArgs.GetAngles( "angular_velocity" );
	float trackDelay = spawnArgs.GetFloat( "trackDelay", "0" );
	if( trackDelay > 0.0 ) {
		PostEventSec( &EV_Hover, spawnArgs.GetFloat( "trackStop", "0" ) );
		PostEventSec( &EV_StartTracking, trackDelay );
	} else if ( enemy.IsValid() ) {
		PostEventMS( &EV_Guide, updateRate );
	}
	float trackDuration = spawnArgs.GetFloat( "trackDuration", "0" );
	if ( trackDuration > 0.0 ) {
		PostEventSec( &EV_StopTracking, trackDuration );
	}
}

void hhProjectileTracking::Event_Hover() {
	physicsObj.SetLinearVelocity( velocity * spawnArgs.GetFloat( "hoverScale", "0.15" ) );
}

void hhProjectileTracking::Event_StartTracking() {
	float randomStartSpread = spawnArgs.GetFloat( "randomStartSpread", "0" );
	if ( randomStartSpread > 0.0 ) {
		velocity = spawnArgs.GetVector( "velocity" );
		velocity.y = gameLocal.random.CRandomFloat() * randomStartSpread;
		velocity.z = gameLocal.random.CRandomFloat() * randomStartSpread;
	}
	PostEventMS( &EV_Guide, updateRate );
}

void hhProjectileTracking::Event_StopTracking() {
	CancelEvents(&EV_Guide);
}

/*
================
hhProjectileTracking::DetermineEnemyPosition
================
*/
idVec3 hhProjectileTracking::DetermineEnemyPosition( const idEntity* ent ) const {
	if ( ent && ent->IsType( idActor::Type ) ) {
		const idActor *entActor = static_cast<const idActor*>(ent);
		return entActor->GetEyePosition();
	}

	return ent->GetOrigin();
}

/*
================
hhProjectileTracking::DetermineEnemyDir
================
*/
idVec3 hhProjectileTracking::DetermineEnemyDir( const idEntity* actor ) const {
	idVec3 enemyPos = DetermineEnemyPosition( actor );
	idVec3 enemyDir = enemyPos - GetOrigin();
	enemyDir.Normalize();

	return enemyDir;
}

/*
================
hhProjectileTracking::Explode
================
*/
void hhProjectileTracking::Explode( const trace_t *collision, const idVec3& velocity, int removeDelay ) {
	hhProjectile::Explode( collision, velocity, removeDelay );
	CancelEvents(&EV_Guide);
}

/*
================
hhProjectileTracking::GetPhysicsToVisualTransform
================
*/
bool hhProjectileTracking::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( spinDelta != 0.0f ) {
		axis = idAngles(0,0,spinAngle).ToMat3();
		spinAngle += spinDelta;
		if ( spinAngle > 360.0f ) {
			spinAngle = 0.0f;
		}
		return true;
	}

	return hhProjectile::GetPhysicsToVisualTransform( origin, axis );
}

/*
================
hhProjectileTracking::Event_TrackTarget
================
*/
void hhProjectileTracking::Event_TrackTarget() {
	if ( !spawnArgs.GetInt( "constantEnemy", "1" ) ) {
		enemy = DetermineEnemy();
	}

	if( !enemy.IsValid() ) {
		physicsObj.SetLinearVelocity( GetAxis()[ 0 ] * velocity[ 0 ] + GetAxis()[ 1 ] * velocity[ 1 ] + GetAxis()[ 2 ] * velocity[ 2 ] );
		physicsObj.SetAngularVelocity( angularVelocity.ToAngularVelocity() * GetAxis() );
		return;
	}

	if( !enemy->GetHealth() ) {
		//if enemy is dead, just explode
		trace_t collision;
		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		Explode( &collision, idVec3(0,0,0), 3 );
	}

	if ( turnFactor < 1.0 && turnFactorAcc > 1.0 ) {
		turnFactor *= turnFactorAcc;		// Accelerate the turn as time goes on so we don't get stuck in any orbits
	}
	idVec3 enemyDir = DetermineEnemyDir( enemy.GetEntity() );
	idVec3 currentDir = GetAxis()[0];
	idVec3 newDir = currentDir*(1-turnFactor) + enemyDir*turnFactor;
	newDir.Normalize();
	if ( driver.IsValid() ) {
		driver->SetAxis(newDir.ToMat3());
	} else {
		SetAxis(newDir.ToMat3());
	}

	physicsObj.SetLinearVelocity( GetAxis()[ 0 ] * velocity[ 0 ] + GetAxis()[ 1 ] * velocity[ 1 ] + GetAxis()[ 2 ] * velocity[ 2 ] );
	physicsObj.SetAngularVelocity( angularVelocity.ToAngularVelocity() * GetAxis() );

	PostEventMS( &EV_Guide, updateRate );
}

/*
================
hhProjectileTracking::Save
================
*/
void hhProjectileTracking::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( turnFactor );
	savefile->WriteInt( updateRate );

	enemy.Save( savefile );

	savefile->WriteAngles( angularVelocity );
	savefile->WriteVec3( velocity );
	savefile->WriteFloat( cachedFovCos );
	savefile->WriteFloat( spinAngle );
	savefile->WriteFloat( turnFactorAcc );
	savefile->WriteFloat( spinDelta );
}

/*
================
hhProjectileTracking::Restore
================
*/
void hhProjectileTracking::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( turnFactor );
	savefile->ReadInt( updateRate );

	enemy.Restore( savefile );

	savefile->ReadAngles( angularVelocity );
	savefile->ReadVec3( velocity );
	savefile->ReadFloat( cachedFovCos );
	savefile->ReadFloat( spinAngle );
	savefile->ReadFloat( turnFactorAcc );
	savefile->ReadFloat( spinDelta );
}

void hhProjectileTracking::StartTracking() {
	Event_StartTracking();
}