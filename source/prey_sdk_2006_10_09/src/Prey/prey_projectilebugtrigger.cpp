#include "../idlib/precompiled.h"
#pragma hdrstop

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#include "prey_local.h"

CLASS_DECLARATION( hhProjectile, hhProjectileBugTrigger )
	EVENT( EV_Collision_Flesh,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Metal,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_AltMetal,			hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Wood,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Stone,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Glass,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Liquid,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_CardBoard,			hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Tile,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Forcefield,			hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Chaff,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Pipe,				hhProjectileBugTrigger::Event_Collision_Impact )
	EVENT( EV_Collision_Wallwalk,			hhProjectileBugTrigger::Event_Collision_Impact )
	
	EVENT( EV_AllowCollision_Chaff,			hhProjectileBugTrigger::Event_AllowCollision_Collide )
	EVENT( EV_Touch,						hhProjectileBugTrigger::Event_Touch )
END_CLASS

void hhProjectileBugTrigger::Event_Collision_Impact( const trace_t* collision, const idVec3& velocity ) {
	ProcessCollision(collision, velocity);
	RemoveProjectile( spawnArgs.GetInt( "remove_time", "1500" ) );
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	idThread::ReturnInt( 1 );
}

void hhProjectileBugTrigger::Event_Touch( idEntity *other, trace_t *trace ) {
	idAI *ownerAI = static_cast<idAI*>(owner.GetEntity());	
	if ( other && ownerAI && ownerAI->GetEnemy() == other ) {
		other->Damage( this, owner.GetEntity(), idVec3( 0,0,1 ), spawnArgs.GetString( "def_damage" ), 1.0, 0 );
		GetPhysics()->SetContents( 0 );
	}
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build