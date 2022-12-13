
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_SpawnEntity("spawnEntity", NULL, 'd');

CLASS_DECLARATION( idEntity, hhEntitySpawner )
	EVENT(EV_Activate,	   		hhEntitySpawner::Event_Activate)
	EVENT(EV_SpawnEntity,  		hhEntitySpawner::Event_SpawnEntity)
END_CLASS

//
// Spawn()
//
void hhEntitySpawner::Spawn( void ) {

	maxSpawnCount		= spawnArgs.GetInt("max_spawn", "-1"); // Default: Infinite
	currSpawnCount		= 0;	
	if(!spawnArgs.GetString("def_entity", "", entDefName)) {
		gameLocal.Error("def_entity not specified for hhEntitySpawner");
	}
	entSpawnArgs.Clear();
	idStr tmpStr;
	idStr realKeyName;

	// Copy keys for monster
	const idKeyValue *kv = spawnArgs.MatchPrefix("ent_", NULL);
	while(kv) {
		tmpStr = kv->GetKey();
		int usIndex = tmpStr.FindChar("ent_", '_');
		realKeyName = tmpStr.Mid(usIndex+1, strlen(kv->GetKey())-usIndex-1);
		entSpawnArgs.Set(realKeyName, kv->GetValue());
		
		kv = spawnArgs.MatchPrefix("ent_", kv);
	}
}

void hhEntitySpawner::Save(idSaveGame *savefile) const {
	savefile->WriteString( entDefName );
	savefile->WriteDict( &entSpawnArgs );
	savefile->WriteInt( maxSpawnCount );
	savefile->WriteInt( currSpawnCount );
}

void hhEntitySpawner::Restore( idRestoreGame *savefile ) {
	savefile->ReadString( entDefName );
	savefile->ReadDict( &entSpawnArgs );
	savefile->ReadInt( maxSpawnCount );
	savefile->ReadInt( currSpawnCount );
}

//
// Event_Activate()
//
void hhEntitySpawner::Event_Activate(idEntity *activator) {
	Event_SpawnEntity();
}

//
// Event_Activate()
//
void hhEntitySpawner::Event_SpawnEntity( void ) {
	idVec3 entSize;

	// Can't spawn anymore
	if(maxSpawnCount >= 0 && currSpawnCount >= maxSpawnCount) {
		return;
	}

	idDict args = entSpawnArgs;;
	
	args.SetVector( "origin", GetPhysics()->GetOrigin());	
	args.SetMatrix("rotation", GetAxis());

	// entity collision checks for seeing if we are going to collide with another entity on spawn
	const idDict *entDef = gameLocal.FindEntityDefDict( entDefName, false );
	if ( !entDef ) {
		if (!entDefName) { //HUMANHEAD rww
			gameLocal.Error("NULL entDefName in hhEntitySpawner::Event_SpawnEntity\n");
		}
		else {
			gameLocal.Warning( "Unknown Def '%s'", entDefName );
		}
		return;
	}
	entDef->GetVector( "size", "0", entSize );
	idBounds bounds = idBounds( GetPhysics()->GetOrigin() ).Expand( max( entSize.x, entSize.y ) );
	idEntity* ents[MAX_GENTITIES];
	int numModels = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, ents, MAX_GENTITIES );
	for ( int i = 0; i < numModels ; i++ ) {
		if( ents[i] && ents[i]->IsType(idActor::Type) ) {
			idThread::ReturnInt( false );
			return;
		}
	}

	idEntity *e = gameLocal.SpawnObject(entDefName, &args);
	
	if(!e) {
		gameLocal.Error("hhEntitySpawner: Failed to spawn entity def named: %s", entDefName);
		return;
	}		
	
	// Copy the targets that our case has to our newly spawned entity
	for( int i = 0; i < targets.Num(); i++ ) {
		e->targets.AddUnique(targets[i]);			
	}

	if(e->IsType(idAI::Type)) {
		static_cast<idAI*>(e)->viewAxis = GetAxis();
	}

	currSpawnCount++;	
	idThread::ReturnInt( true );
}
