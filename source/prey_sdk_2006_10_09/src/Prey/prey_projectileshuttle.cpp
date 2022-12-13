#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhProjectile, hhProjectileShuttle )
	EVENT( EV_Collision_Flesh,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_AltMetal,			hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Wood,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Stone,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Glass,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Liquid,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_CardBoard,			hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Tile,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Forcefield,			hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Chaff,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Pipe,				hhProjectileShuttle::Event_Collision_Explode )
	EVENT( EV_Collision_Wallwalk,			hhProjectileShuttle::Event_Collision_Explode )
	
	EVENT( EV_AllowCollision_Chaff,			hhProjectileShuttle::Event_AllowCollision_Collide )
END_CLASS