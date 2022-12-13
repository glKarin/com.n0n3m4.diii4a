#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhProjectile, hhProjectileTrigger )
	EVENT( EV_Collision_Flesh,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_AltMetal,			hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Wood,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Stone,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Glass,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Liquid,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_CardBoard,			hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Tile,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Forcefield,			hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Chaff,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Pipe,				hhProjectileTrigger::Event_Collision_Explode )
	EVENT( EV_Collision_Wallwalk,			hhProjectileTrigger::Event_Collision_Explode )
	
	EVENT( EV_AllowCollision_Chaff,			hhProjectileTrigger::Event_AllowCollision_Collide )
END_CLASS

void hhProjectileTrigger::Event_Collision_Explode( const trace_t* collision, const idVec3& velocity ) {
	ProcessCollision(collision, velocity);
	if ( state == EXPLODED || state == FIZZLED || state == COLLIDED ) {
		return;
	}
	if( !collision ) {
		return;
	}
	int length = 0;
	idEntity *trigger;
	idDict Args;

	Args.Set( "def_damage", spawnArgs.GetString("trigger_damage") );
	Args.Set( "mins", spawnArgs.GetString("trigger_min") );
	Args.Set( "maxs", spawnArgs.GetString("trigger_max") );
	Args.Set( "snd_loop", spawnArgs.GetString("snd_loop") );
	Args.Set( "snd_explode", spawnArgs.GetString("snd_loop") );
	Args.SetVector( "origin", GetOrigin() );
	Args.SetMatrix( "rotation", GetAxis() );
	trigger = gameLocal.SpawnObject( spawnArgs.GetString("def_trigger"), &Args );
	if ( trigger ) {
		trigger->PostEventSec( &EV_Remove, spawnArgs.GetFloat( "remove_time", "5" ) );
		trigger->PostEventSec( &EV_StopSound, spawnArgs.GetFloat( "remove_time", "5" ) - 1, SND_CHANNEL_ANY, 0 );
		trigger->StartSound( "snd_explode", SND_CHANNEL_VOICE, 0, true, &length );
		trigger->StartSound( "snd_loop", SND_CHANNEL_BODY, 0, true, &length );
	}
	SpawnExplosionFx( collision );
	SpawnDebris( collision->c.normal, velocity.ToNormal() );
	state = EXPLODED;
	RemoveProjectile( spawnArgs.GetFloat( "remove_time", "5" ) );
	ProcessCollision(collision, velocity);
	idThread::ReturnInt( 1 );
}
