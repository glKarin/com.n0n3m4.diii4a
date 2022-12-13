#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhProjectile, hhProjectileAutoCannonGrenade )
	EVENT( EV_Collision_Flesh,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Metal,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_AltMetal,			hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Wood,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Stone,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Glass,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Liquid,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_CardBoard,			hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Tile,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Forcefield,			hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Pipe,				hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Wallwalk,			hhProjectileAutoCannonGrenade::Event_Collision_Explode )
	
	EVENT( EV_AllowCollision_Chaff,			hhProjectileAutoCannonGrenade::Event_AllowCollision_Collide )
END_CLASS