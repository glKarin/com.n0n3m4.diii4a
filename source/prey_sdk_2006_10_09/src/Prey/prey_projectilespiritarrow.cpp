#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileSpiritArrow
	
***********************************************************************/
CLASS_DECLARATION( hhProjectile, hhProjectileSpiritArrow )
	EVENT( EV_AllowCollision_Spirit,	hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Chaff,		hhProjectile::Event_AllowCollision_PassThru )

	EVENT( EV_AllowCollision_Flesh,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Metal,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_AltMetal,	hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Wood,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Stone,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Glass,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Pipe,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Tile,		hhProjectileSpiritArrow::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_CardBoard,	hhProjectileSpiritArrow::Event_AllowCollision_Collide )

	EVENT( EV_Collision_Flesh,			hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Metal,			hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_AltMetal,		hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Wood,			hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Stone,			hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Glass,			hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Wallwalk,		hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Pipe,			hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_CardBoard,		hhProjectileSpiritArrow::Event_Collision_Stick )
	EVENT( EV_Collision_Spirit,			hhProjectileSpiritArrow::Event_Collision_Impact )
	EVENT( EV_Collision_Chaff,			hhProjectileSpiritArrow::Event_Collision_Impact )
END_CLASS


int hhProjectileSpiritArrow::DetermineClipmask() {
	return MASK_SPIRITARROW;
}

/*
=================
hhProjectileSpiritArrow::BindToCollisionObject
=================
*/
void hhProjectileSpiritArrow::BindToCollisionObject( const trace_t* collision ) {
	if( !collision || collision->fraction == 1.0f ) {
		return;
	}

	idEntity*	pEntity = gameLocal.entities[collision->c.entityNum];
	HH_ASSERT( pEntity );
	
	if( !pEntity->fl.applyDamageEffects ) {
		PostEventMS( &EV_Fizzle, 0 );
		return;
	}

	if ( pEntity->spawnArgs.GetBool( "no_arrow_stick" ) ) {
		RemoveProjectile( 0 );
		return;
	}

	jointHandle_t jointHandle = CLIPMODEL_ID_TO_JOINT_HANDLE( collision->c.id );
	idVec3 penetrationVector( collision->endAxis[0] * spawnArgs.GetFloat("penetrationDepth", "10") ); // Push the arrow into the hit object a bit
	if ( jointHandle != INVALID_JOINT ) {
		SetOrigin( collision->endpos + penetrationVector );
		BindToJoint( pEntity, jointHandle, true );
	} else {
		SetOrigin( collision->endpos + penetrationVector );
		Bind( pEntity, true );
	}
}

/*
=================
hhProjectileSpiritArrow::Event_Collision_Stick
=================
*/
void hhProjectileSpiritArrow::Event_Collision_Stick( const trace_t* collision, const idVec3 &velocity ) {

	ProcessCollision( collision, velocity );//Assuming that EV_Fizzle is canceled in ProcessCollision
	
	if (gameLocal.isMultiplayer) { //rww - in mp we don't stick to players.
		idEntity *pEntity = gameLocal.entities[collision->c.entityNum];
		if (pEntity && pEntity->IsType(hhPlayer::Type)) {
			PostEventMS( &EV_Fizzle, 0 );

			idThread::ReturnInt( 1 );
			return;
		}
	}

	PostEventMS( &EV_Fizzle, spawnArgs.GetInt("remove_time") );

	BindToCollisionObject( collision );

	fl.ignoreGravityZones = true;
	SetGravity( idVec3(0.f, 0.f, 0.f) );
	spawnArgs.SetVector("gravity", idVec3(0.f, 0.f, 0.f) );

	idThread::ReturnInt( 1 );
}