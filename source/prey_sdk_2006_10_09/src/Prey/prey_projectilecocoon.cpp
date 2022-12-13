#include "../idlib/precompiled.h"
#pragma hdrstop

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#include "prey_local.h"

const idEventDef EV_Guide( "<guide>" );

CLASS_DECLARATION( hhProjectileTracking, hhProjectileCocoon )
	EVENT( EV_Collision_Flesh,			hhProjectileCocoon::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,			hhProjectileCocoon::Event_Collision_Proj )
	EVENT( EV_Collision_AltMetal,		hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Wood,			hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Stone,			hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Glass,			hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Liquid,			hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_CardBoard,		hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Tile,			hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Forcefield,		hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Chaff,			hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Wallwalk,		hhProjectileCocoon::Event_Collision_Bounce )
	EVENT( EV_Collision_Pipe,			hhProjectileCocoon::Event_Collision_Bounce )

	EVENT( EV_Guide,					hhProjectileCocoon::Event_TrackTarget )
	EVENT( EV_AllowCollision_Flesh,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Metal,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_AltMetal,		hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Wood,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Stone,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Glass,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Liquid,		hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_CardBoard,		hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Tile,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Forcefield,	hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Pipe,			hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Wallwalk,		hhProjectileCocoon::Event_AllowCollision )
	EVENT( EV_AllowCollision_Spirit,		hhProjectileCocoon::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Chaff,			hhProjectileCocoon::Event_AllowCollision_Collide )

END_CLASS

void hhProjectileCocoon::Spawn() {
	nextBounceTime = 0;
}

void hhProjectileCocoon::Event_TrackTarget() {
	idVec3 newVelocity;
	if( !enemy.IsValid() ) {
		physicsObj.SetLinearVelocity( GetAxis()[ 0 ] * velocity[ 0 ] + GetAxis()[ 1 ] * velocity[ 1 ] + GetAxis()[ 2 ] * velocity[ 2 ] );
		physicsObj.SetAngularVelocity( angularVelocity.ToAngularVelocity() * GetAxis() );
		return;
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
	newVelocity = GetAxis()[ 0 ] * velocity[ 0 ] + GetAxis()[ 1 ] * velocity[ 1 ] + GetAxis()[ 2 ] * velocity[ 2 ];
	newVelocity *= spawnArgs.GetFloat( "bounce", "1.0" );
	physicsObj.SetLinearVelocity( newVelocity );
	physicsObj.SetAngularVelocity( angularVelocity.ToAngularVelocity() * GetAxis() );
}

void hhProjectileCocoon::Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity ) {
	if ( gameLocal.time >= nextBounceTime ) {
		nextBounceTime = gameLocal.time + spawnArgs.GetFloat( "bounce_freq", "200" );

		StopSound( SND_CHANNEL_BODY, true );
		StartSound( "snd_bounce", SND_CHANNEL_BODY, 0, true, NULL );
		idEntity *ent = gameLocal.entities[ collision->c.entityNum ];
		Event_TrackTarget();
		BounceSplat( GetOrigin(), -collision->c.normal );
	}
	idThread::ReturnInt( 0 );
}

void hhProjectileCocoon::Event_Collision_Proj( const trace_t* collision, const idVec3 &velocity ) {
	idEntity *ent = gameLocal.entities[ collision->c.entityNum ];
	if ( ent && ent->IsType( idProjectile::Type ) ) {
		Explode( collision, velocity, 0 );
	} else {
		Event_Collision_Bounce( collision, velocity );
	}

	idThread::ReturnInt( 0 );
}

void hhProjectileCocoon::Event_AllowCollision( const trace_t* collision ) {
	idEntity *ent = gameLocal.entities[ collision->c.entityNum ];
	if ( ent && ent->IsType( idProjectile::Type ) ) {
		int foo = 0;
	}

	idThread::ReturnInt( 1 );
}

void hhProjectileCocoon::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	hhProjectileTracking::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build