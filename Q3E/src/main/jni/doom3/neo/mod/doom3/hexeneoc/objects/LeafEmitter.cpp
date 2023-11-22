
#pragma hdrstop

#include "Entity.h"
#include "Game_local.h"
#include "LeafEmitter.h"

CLASS_DECLARATION( idEntity, idEntity_LeafEmitter )

END_CLASS

void idEntity_LeafEmitter::Spawn() {
	nextLeaf = 0;
	interval = spawnArgs.GetFloat( "emitter_leaf_interval" );
	maxLeaf = spawnArgs.GetFloat( "emitter_leaf_max" );

	leaf.SetFloat( "leaf_liveTime",		spawnArgs.GetFloat( "leaf_liveTime" ) );
	leaf.SetFloat( "leaf_spread",		spawnArgs.GetFloat( "leaf_spread" ) );
	leaf.SetFloat( "leaf_windPower",	spawnArgs.GetFloat( "leaf_windPower" ) );
	leaf.SetVector( "leaf_windDir",		spawnArgs.GetVector( "leaf_windDir" ) );
	leaf.SetFloat( "leaf_moveSpeed",	spawnArgs.GetFloat( "leaf_moveSpeed" ) );
	leaf.SetVector( "leaf_gravity",		spawnArgs.GetVector( "leaf_gravity" ) );
	leaf.SetFloat( "leaf_maxSpinSpeed",	spawnArgs.GetFloat( "leaf_maxSpinSpeed" ) );
	leaf.SetVector( "leaf_origin",		GetPhysics()->GetOrigin() );
}

void idEntity_LeafEmitter::Think() {
	if ( ( gameLocal.time / 1000 ) > nextLeaf) {

		float i = gameLocal.random.RandomFloat() * 4;

		if ( gameLocal.random.RandomInt(1) > 1 ) {
			leaf.Set("classname", "object_leaf_lg");
		} else {
			leaf.Set("classname", "object_leaf_sm");
		}

		gameLocal.SpawnEntityDef( leaf );

		nextLeaf = ( gameLocal.time / 1000 ) + interval;
	}
}
