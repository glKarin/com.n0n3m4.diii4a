#include "../idlib/precompiled.h"
#pragma hdrstop

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#include "prey_local.h"

CLASS_DECLARATION( hhProjectile, hhProjectileBounce )
	EVENT( EV_Collision_Flesh,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Metal,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_AltMetal,		hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Wood,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Stone,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Glass,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_CardBoard,		hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Tile,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Forcefield,		hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Pipe,			hhProjectileBounce::Event_Collision_Bounce )
	EVENT( EV_Collision_Wallwalk,		hhProjectileBounce::Event_Collision_Bounce )

END_CLASS

void hhProjectileBounce::Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity ) {
	if( !collision || collision->fraction == 1.0f ) {
		return;
	}
	StartSound( "snd_bounce", SND_CHANNEL_BODY, 0, true, NULL );
	float dot = velocity * collision->c.normal;
	idVec3 normal = collision->c.normal * dot;
	idVec3 tangent = (velocity - normal).ToNormal() - normal.ToNormal();
	idVec3 newVelocity  = tangent.ToNormal() * velocity.Length();
	physicsObj.SetLinearVelocity( newVelocity );
	idThread::ReturnInt( 0 );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build