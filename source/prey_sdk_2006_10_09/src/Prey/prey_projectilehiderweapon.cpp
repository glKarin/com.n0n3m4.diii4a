#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileHider
	
***********************************************************************/
CLASS_DECLARATION( hhProjectile, hhProjectileHider )
END_CLASS

void hhProjectileHider::ApplyDamageEffect( idEntity* hitEnt, const trace_t& collision, const idVec3& velocity, const char* damageDefName ) {
	//This is used to allow the hider weapon shots to streak when colliding on the ground
	if( hitEnt ) {
		hitEnt->AddDamageEffect( collision, velocity.ToNormal(), damageDefName, (!fl.networkSync || netSyncPhysics) );
	}
}

/***********************************************************************

  hhProjectileHiderCanister
	
***********************************************************************/
const idEventDef EV_CollidedWithChaff( "<collidedWithChaff>", "tv", 'd' );
CLASS_DECLARATION( hhProjectileHider, hhProjectileHiderCanister )
	EVENT( EV_Collision_Flesh,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_AltMetal,	hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Wood,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Stone,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Glass,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_CardBoard,	hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Forcefield,	hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Pipe,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Wallwalk,	hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_Collision_Tile,		hhProjectileHiderCanister::Event_Collision_Explode )
	EVENT( EV_CollidedWithChaff,	hhProjectileHiderCanister::Event_Collision_ExplodeChaff )
END_CLASS

void hhProjectileHiderCanister::Spawn() {
	numSubProjectiles = spawnArgs.GetInt( "numSubProjectiles" );
	const idDict *dict = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_subProjectile"), false );
	if (!dict) {
		gameLocal.Error( "No def_subProjectile defined for entity '%s'.\n", GetName() );
	}
	subProjectileDict = *dict;
	subSpread = DEG2RAD( spawnArgs.GetFloat("spread") );
	subBounce = spawnArgs.GetFloat( "subBounce", "1" );
	bScatter = spawnArgs.GetBool( "subScatter", "0" );
}

void hhProjectileHiderCanister::SpawnRicochetSpray( const idVec3& bounceVector ) {
	hhProjectile*	projectile = NULL;
	idVec3			dir;
	idMat3			projAxis;
	idAngles		projAngle;
	idMat3			bounceAxis;

	if( !bScatter ) {
		bounceAxis = bounceVector.ToNormal().ToMat3();
		subProjectileDict.SetVector( "velocity", idVec3(bounceVector.Length() * subBounce, 0.0f, 0.0f) );
	}

	for( int iIndex = 0; iIndex < numSubProjectiles; ++iIndex ) {
		if( bScatter ) {
			bounceAxis = hhUtils::RandomVector().ToNormal().ToMat3();
			subProjectileDict.SetVector( "velocity", idVec3(bounceVector.Length() * subBounce, 0.0f, 0.0f) );
		}
		dir = hhUtils::RandomSpreadDir( bounceAxis, subSpread );
		projAngle = dir.ToAngles();
		projAngle[2] = GetAxis().ToAngles()[2];
		projAxis = projAngle.ToMat3();

		//HUMANHEAD rww - now local
		//projectile = hhProjectile::SpawnProjectile( &subProjectileDict );
		projectile = hhProjectile::SpawnClientProjectile( &subProjectileDict );
		projectile->spawnArgs.Set( "weapontype", spawnArgs.GetString("weapontype", "NONE1") );
		projectile->Create( owner.GetEntity(), GetOrigin(), bounceAxis );
		projectile->Launch( GetOrigin(), projAxis, vec3_zero );
		projectile->SetParentProjectile( this );
	}
}

void hhProjectileHiderCanister::SpawnDebris( const idVec3& collisionNormal, const idVec3& collisionDir ) {
	//Spawn debris along ground with respect to our current direction
	idVec3 dir = collisionDir;
	dir.ProjectOntoPlane( -GetPhysics()->GetGravityNormal() );
	dir.Normalize();
	hhProjectileHider::SpawnDebris( dir, collisionDir );
}

void hhProjectileHiderCanister::Event_Collision_Explode( const trace_t* collision, const idVec3& velocity ) {
	hhProjectile::Event_Collision_Explode( collision, velocity );

	idEntity *entityHit = gameLocal.entities[ collision->c.entityNum ];
	if (entityHit->IsType(idAI::Type) || entityHit->IsType(idPlayer::Type))
		numSubProjectiles = 0;

	SetOrigin(collision->endpos+collision->c.normal*8.0f);

	//rww - these are purely local now, because they cause bandwidth murder.
	SpawnRicochetSpray( hhProjectile::GetBounceDirection(velocity, collision->c.normal, this, NULL) );

	idThread::ReturnInt( 1 );
}

void hhProjectileHiderCanister::Event_Collision_ExplodeChaff( const trace_t* collision, const idVec3& velocity ) {
	fl.takedamage = false;
	hhProjectile::Event_Collision_Explode( collision, velocity );
	idThread::ReturnInt( 1 );
}


void hhProjectileHiderCanister::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( numSubProjectiles );
	savefile->WriteDict( &subProjectileDict );
	savefile->WriteFloat( subSpread );
	savefile->WriteBool( bScatter );
}

void hhProjectileHiderCanister::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( numSubProjectiles );
	savefile->ReadDict( &subProjectileDict );
	savefile->ReadFloat( subSpread );
	savefile->ReadBool( bScatter );

	subBounce = spawnArgs.GetFloat( "subBounce", "1" );
}

void hhProjectileHiderCanister::Killed( idEntity *inflicter, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	hhProjectileHider::Killed( inflicter, attacker, damage, dir, location );
	//bScatter = true;
	SpawnRicochetSpray( dir * (damage * 10.0f) );
}
