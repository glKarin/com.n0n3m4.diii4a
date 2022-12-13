#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhProjectile, hhProjectileWrench )
	EVENT( EV_Collision_Flesh,				hhProjectileWrench::Event_Collision_Impact )
	EVENT( EV_Collision_Metal,				hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_AltMetal,			hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Wood,				hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Stone,				hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Glass,				hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Liquid,				hhProjectileWrench::Event_Collision_DisturbLiquid )
	EVENT( EV_Collision_CardBoard,			hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Tile,				hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Forcefield,			hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Pipe,				hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Wallwalk,			hhProjectileWrench::Event_Collision_Impact_AlertAI )
	EVENT( EV_Collision_Chaff,				hhProjectileWrench::Event_Collision_Impact_AlertAI )

END_CLASS

void hhProjectileWrench::Event_Collision_Impact_AlertAI( const trace_t* collision, const idVec3& velocity ) {
	gameLocal.AlertAI( owner.GetEntity() );

	CancelEvents( &EV_Explode );
	RemoveProjectile( ProcessCollision(collision, velocity) );
	state = COLLIDED;
	idThread::ReturnInt( 1 );
}

void hhProjectileWrench::DamageEntityHit( const trace_t* collision, const idVec3& velocity, idEntity* entHit ) {
	if (!gameLocal.isMultiplayer) {
		hhPlayer* pOwner = (owner->IsType(hhPlayer::Type)) ? static_cast<hhPlayer*>(owner.GetEntity()) : NULL;

		if ( entHit && entHit->IsType( idActor::Type ) ) {
			//pOwner->weapon->SetSkinByName( "skins/weapons/wrench_bloody" );
			pOwner->weapon->SetShaderParm( 7, -MS2SEC(gameLocal.time) );
		}
	}
	hhProjectile::DamageEntityHit( collision, velocity, entHit );
}

