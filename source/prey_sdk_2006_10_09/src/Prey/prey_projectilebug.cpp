#include "../idlib/precompiled.h"
#pragma hdrstop

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#include "prey_local.h"

const idEventDef EV_Guide( "<guide>" );

CLASS_DECLARATION( hhProjectileTracking, hhProjectileBug )
	EVENT( EV_Collision_Flesh,			hhProjectileBug::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_AltMetal,		hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Wood,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Stone,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Glass,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Liquid,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_CardBoard,		hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Tile,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Forcefield,		hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Chaff,			hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Wallwalk,		hhProjectileBug::Event_Collision_Bounce )
	EVENT( EV_Collision_Pipe,			hhProjectileBug::Event_Collision_Bounce )

	EVENT( EV_Guide,					hhProjectileBug::Event_TrackTarget )
END_CLASS

void hhProjectileBug::Spawn() {
	enemyRadius = spawnArgs.GetFloat( "enemy_radius", "200" );
}

idEntity* hhProjectileBug::DetermineEnemy() {
	idEntity *	entityList[ MAX_GENTITIES ];
	idEntity*	possibleEnemy = NULL;
	float		currentEnemyDot = 0.0f;
	float		currentEnemyDist = CM_MAX_TRACE_DIST;
	idEntity*	localEnemy = NULL;

	//find player if within certain radius
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] ) {
			possibleEnemy = gameLocal.entities[i];
			if ( !possibleEnemy || (possibleEnemy->GetOrigin() - GetOrigin()).Length() > enemyRadius ) {
				continue;
			}
			localEnemy = WhosClosest( possibleEnemy, localEnemy, currentEnemyDot, currentEnemyDist );
		}
	}
	if ( localEnemy ) {
		return localEnemy;
	}

	//otherwise look for bug triggers
	float bestDist = 9999999;
	int bestIndex = -1;
	idBounds bounds = idBounds( GetOrigin() ).Expand( spawnArgs.GetFloat( "enemy_check_radius" ));
	int numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );
	for ( int i=0; i<numListedEntities;i++ ) {
		possibleEnemy = entityList[ i ];
		if ( !possibleEnemy || !possibleEnemy->spawnArgs.GetInt( "bug_trigger", "0" ) ) {
			continue;
		}
		float dist = ( GetOrigin() - possibleEnemy->GetOrigin() ).Length();
		if ( dist < bestDist ) {
			bestIndex = i;
			bestDist = dist;
		}
	}

	if ( bestIndex >= 0 ) {
		if ( entityList[bestIndex]->IsType( idActor::Type ) ) {
			physicsObj.SetContents( 0 );
		} else {
			physicsObj.SetContents( CONTENTS_PROJECTILE );
		}
		return entityList[bestIndex];
	}

	return NULL;
}

idVec3 hhProjectileBug::DetermineEnemyPosition( const idEntity* ent ) const {
	float randomOffset = spawnArgs.GetFloat( "offset_max", "70" ) * gameLocal.random.RandomFloat();
	if ( ent && ent->IsType( idActor::Type ) ) {
		const idActor *entActor = static_cast<const idActor*>(ent);
		return entActor->GetEyePosition() + idVec3(0,0,randomOffset);
	}

	return ent->GetOrigin() + idVec3(0,0,randomOffset);
}

void hhProjectileBug::Event_TrackTarget() {
	idEntity *newEnemy = DetermineEnemy();
	if( !newEnemy ) {
		physicsObj.SetLinearVelocity( GetAxis()[ 0 ] * velocity[ 0 ] + GetAxis()[ 1 ] * velocity[ 1 ] + GetAxis()[ 2 ] * velocity[ 2 ] );
		physicsObj.SetAngularVelocity( angularVelocity.ToAngularVelocity() * GetAxis() );
		return;
	}
	enemy = newEnemy;

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

void hhProjectileBug::Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity ) {
	physicsObj.SetLinearVelocity( hhProjectile::GetBounceDirection( physicsObj.GetLinearVelocity(), collision->c.normal ) );
	idThread::ReturnInt( 0 );
}

//================
//hhProjectileBug::Save
//================
void hhProjectileBug::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( enemyRadius );
}

//================
//hhProjectileBug::Restore
//================
void hhProjectileBug::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( enemyRadius );
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build