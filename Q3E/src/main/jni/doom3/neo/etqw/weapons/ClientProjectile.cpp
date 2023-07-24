// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

/*
===============================================================================

  sdClientProjectile
	
===============================================================================
*/

const int PROJECTILE_REMOVE_TIME = 5000;

CLASS_DECLARATION( sdClientEntity, sdClientProjectile )
END_CLASS

/*
================
sdClientProjectile::sdClientProjectile
================
*/
sdClientProjectile::sdClientProjectile( void ) {
}

/*
=================
sdClientProjectile::~sdClientProjectile
=================
*/
sdClientProjectile::~sdClientProjectile() {
	StopSound( SND_CHANNEL_ANY );
}

/*
================
sdClientProjectile::Spawn
================
*/
void sdClientProjectile::Spawn( void ) {
}

/*
================
sdClientProjectile::Think
================
*/
void sdClientProjectile::Think( void ) {
	Present();
}


/*
=================
sdClientProjectile::Collide
=================
*/
bool sdClientProjectile::Collide( const trace_t &collision, const idVec3 &velocity ) {
	idEntity	*ent;

	// get the entity the projectile collided with
	ent = NULL;

	// play sound based on material type
	const char* surfaceTypeName = "";
	if ( collision.c.material ) {
		surfaceTypeName = collision.c.material->GetSurfaceType();
	}
	if ( !*surfaceTypeName ) {
		surfaceTypeName = "default";
	}
	StartSound( va( "snd_%s", surfaceTypeName ), SND_CHANNEL_ITEM, 0, true );

	return CollideEffect( ent, collision, velocity );
}

/*
=================
sdClientProjectile::CollideEffect
=================
*/
bool sdClientProjectile::CollideEffect( idEntity* ent, const trace_t &collision, const idVec3 &velocity ) {

	SetPosition( collision.endpos, collision.endAxis );

	// if the projectile causes a damage effect
	if ( spawnArgs.GetBool( "impact_blood" ) ) {
		DefaultDamageEffect( collision, velocity );
	}

	Explode( &collision );

	return true;
}

/*
================
sdClientProjectile::Explode
================
*/
void sdClientProjectile::Explode( const trace_t *collision, const char *sndExplode ) {
	const char *fxname;
	idVec3		normal, endpos;
	int			removeTime;

	if ( spawnArgs.GetVector( "detonation_axis", "", normal ) ) {
		SetAxis( normal.ToMat3() );
	} else {
		normal = collision ? collision->c.normal : idVec3( 0, 0, 1 );
	}
	endpos = ( collision ) ? collision->endpos : GetOrigin();

	removeTime = PROJECTILE_REMOVE_TIME;

	// play sound
//	StopSound( SND_CHANNEL_ANY );
	StartSound( sndExplode, SND_CHANNEL_BODY );
	StartSound( "snd_explode_med", SND_CHANNEL_BODY2 );
	StartSound( "snd_explode_far", SND_CHANNEL_BODY3 );

	Hide();
	FreeLightDef();

	SetOrigin( GetOrigin() + 8.0f * normal );

	// change the model
	fxname = NULL;
	if ( g_testParticle.GetInteger() == TEST_PARTICLE_IMPACT ) {
		fxname = g_testParticleName.GetString();
	} else {
		fxname = spawnArgs.GetString( "model_detonate" );
	}

	ClientEntEvent_Remove( removeTime );
}

/*
================
sdClientProjectile::ClientReceiveEvent
================
*/
bool sdClientProjectile::ClientReceiveEvent( const idVec3 &origin, int event, int time, const idBitMsg &msg ) {
	if ( sdClientEntity::ClientReceiveEvent( origin, event, time, msg ) ) {
		return true;
	}

	switch( event ) {
		case EVENT_COLLIDE: {
			trace_t collision;
			idVec3 velocity;
			int index;

			memset( &collision, 0, sizeof( collision ) );
			collision.endpos = collision.c.point = origin;
			collision.c.normal = msg.ReadDir( 24 );

			collision.endAxis = collision.c.normal.ToMat3();

			index = msg.ReadShort();
			if ( index >= -1 && index < declManager->GetNumMaterials() ) {
				if ( index != -1 ) {
					collision.c.material =  declManager->MaterialByIndex( index, false );
				}						
			}

			velocity[0] = msg.ReadFloat( 5, 10 );
			velocity[1] = msg.ReadFloat( 5, 10 );
			velocity[2] = msg.ReadFloat( 5, 10 );
			Collide( collision, velocity );
			return true;
		}
		default: {
			return false;
		}
	}
}

/*
=================
sdClientProjectile::DefaultDamageEffect
=================
*/
void sdClientProjectile::DefaultDamageEffect( const trace_t &collision, const idVec3 &velocity ) {
	const char *decal;

	// project decal
	decal = spawnArgs.RandomPrefix( "mtr_detonate", gameLocal.random );
	if ( decal && *decal ) {
		gameLocal.ProjectDecal( collision.c.point, -collision.c.normal, 8.0f, true, spawnArgs.GetFloat( "decal_size", "6.0" ), decal );
	}
}
