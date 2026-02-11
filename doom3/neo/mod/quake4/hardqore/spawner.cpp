/*
================

Spawner.cpp

================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "spawner.h"
#include "vehicle/Vehicle.h"
#include "ai/AI.h"
#include "ai/AI_Manager.h"
#include "ai/AI_Util.h"

const idEventDef EV_Spawner_RemoveNullActiveEntities( "removeNullActiveEntities" );
const idEventDef EV_Spawner_NumActiveEntities( "numActiveEntities", "", 'd' );
const idEventDef EV_Spawner_GetActiveEntity( "getActiveEntity", "d", 'e' );

CLASS_DECLARATION( idEntity, rvSpawner )
	EVENT( EV_Activate,								rvSpawner::Event_Activate )
	EVENT( EV_Spawner_RemoveNullActiveEntities,		rvSpawner::Event_RemoveNullActiveEntities )
	EVENT( EV_Spawner_NumActiveEntities,			rvSpawner::Event_NumActiveEntities )
	EVENT( EV_Spawner_GetActiveEntity,				rvSpawner::Event_GetActiveEntity )
END_CLASS

/*
==============
rvSpawner::Spawn
==============
*/
void rvSpawner::Spawn( void ){
	GetPhysics()->SetContents( 0 );

	// TEMP: read max_team_test until we can get it out of all the current maps
	if ( !spawnArgs.GetInt ( "max_active", "4", maxActive ) ) {
		if ( spawnArgs.GetInt ( "max_team_test", "4", maxActive ) ) {
			gameLocal.Warning ( "spawner '%s' using outdated 'max_team_test', please change to 'max_active'", GetName() );
		}
	}

	maxToSpawn		= spawnArgs.GetInt( "count", "-1" );
	skipVisible		= spawnArgs.GetBool ( "skipvisible", "1" );
	spawnWaves		= spawnArgs.GetInt( "waves", "1" );
	spawnDelay		= SEC2MS( spawnArgs.GetFloat( "delay", "2" ) );
	numSpawned		= 0;
	nextSpawnTime	= 0;

	// Spawn waves has to be less than max active
	if ( spawnWaves > maxActive ) {
		spawnWaves = maxActive;
	}

	FindSpawnTypes ( );
}

/*
==============
rvSpawner::Save
==============
*/
void rvSpawner::Save ( idSaveGame *savefile ) const{
	savefile->WriteInt( numSpawned );
	savefile->WriteInt( maxToSpawn );
	savefile->WriteFloat( nextSpawnTime );
	savefile->WriteInt( maxActive );

	int i;
	savefile->WriteInt( currentActive.Num() );
	for( i = 0; i < currentActive.Num(); i++ ) {
		currentActive[i].Save ( savefile );
	}

	savefile->WriteInt( spawnWaves );
	savefile->WriteInt( spawnDelay );
	savefile->WriteBool( skipVisible );

	savefile->WriteInt( spawnPoints.Num() );
    for ( i = 0; i < spawnPoints.Num(); i++ ) {
		spawnPoints[ i ].Save ( savefile );
	}

	savefile->WriteInt ( callbacks.Num() );
	for ( i = 0; i < callbacks.Num(); i ++ ) {
		callbacks[i].ent.Save ( savefile );
		savefile->WriteString ( callbacks[i].event );
	}
}

/*
==============
rvSpawner::Restore
==============
*/
void rvSpawner::Restore ( idRestoreGame *savefile ){
	int num;
	int i;

	savefile->ReadInt( numSpawned );
	savefile->ReadInt( maxToSpawn );
	savefile->ReadFloat( nextSpawnTime );
	savefile->ReadInt( maxActive );
	
	savefile->ReadInt( num );
	currentActive.Clear ( );
	currentActive.SetNum( num );
	for( i = 0; i < num; i++ ) {
		currentActive[i].Restore ( savefile );
	}

	savefile->ReadInt( spawnWaves );
	savefile->ReadInt( spawnDelay );
	savefile->ReadBool( skipVisible );

	savefile->ReadInt( num );
	spawnPoints.SetNum( num);
	for( i = 0; i < num; i ++ ) {
		spawnPoints[i].Restore ( savefile );
	}

	savefile->ReadInt ( num );
	callbacks.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		callbacks[i].ent.Restore ( savefile );
		savefile->ReadString ( callbacks[i].event );
	}

	FindSpawnTypes ();
}

/*
==============
rvSpawner::FindSpawnTypes

Generate the list of classnames to spawn from the spawnArgs.  Anything matching the 
prefix "def_spawn" will be included in the list.
==============
*/
void rvSpawner::FindSpawnTypes ( void ){
	const idKeyValue *kv;	
	for ( kv = spawnArgs.MatchPrefix( "def_spawn", NULL ); kv; kv = spawnArgs.MatchPrefix( "def_spawn", kv ) ) {
		spawnTypes.Append ( kv->GetValue ( ) );

		// precache decls
		declManager->FindType( DECL_AF, kv->GetValue(), true, false );
	}
}

/*
==============
rvSpawner::FindTargets
==============
*/
void rvSpawner::FindTargets ( void ) {
	int i;
	idBounds						bounds( idVec3( -16, -16, 0 ), idVec3( 16, 16, 72 ) );
	trace_t tr;
	
	idEntity::FindTargets ( );

	// Copy the relevant targets to the spawn point list (right now only target_null entities)
	for ( i = targets.Num() - 1; i >= 0; i -- ) {
		idEntity* ent;
		ent = targets[i];
		if ( idStr::Icmp ( ent->spawnArgs.GetString ( "classname" ), "target_null" ) ) {
			continue;
		}
		
		idEntityPtr<idEntity> &entityPtr = spawnPoints.Alloc();
		entityPtr = ent;		

		if( !spawnArgs.GetBool("ignoreSpawnPointValidation") ) {
			gameLocal.TraceBounds( this, tr, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin(), bounds, MASK_MONSTERSOLID, NULL );
			if ( gameLocal.entities[tr.c.entityNum] && !gameLocal.entities[tr.c.entityNum]->IsType ( idActor::GetClassType ( ) ) ) {
				//drop a console warning here
				gameLocal.Warning ( "Spawner '%s' can't spawn at point '%s', the monster won't fit.", GetName(), ent->GetName() );		
			}	
		}
	}
}

/*
==============
rvSpawner::ValidateSpawnPoint
==============
*/
bool rvSpawner::ValidateSpawnPoint ( const idVec3 origin, const idBounds &bounds ){
	trace_t tr;
	if( spawnArgs.GetBool("ignoreSpawnPointValidation") ) {
		return true;
	}

	gameLocal.TraceBounds( this, tr, origin, origin, bounds, MASK_MONSTERSOLID, NULL );
	return tr.fraction >= 1.0f;
}

/*
==============
rvSpawner::AddSpawnPoint
==============
*/
void rvSpawner::AddSpawnPoint ( idEntity* point ) {
	idEntityPtr<idEntity> &entityPtr = spawnPoints.Alloc();
	entityPtr = point;
	
	// If there were no spawnPoints then start with the delay
	if ( spawnPoints.Num () == 1 ) {
		nextSpawnTime = gameLocal.time + spawnDelay;
	}
}

/*
==============
rvSpawner::RemoveSpawnPoint
==============
*/
void rvSpawner::RemoveSpawnPoint ( idEntity* point ) {
	int i;
	for ( i = spawnPoints.Num()-1; i >= 0; i -- ) {
		if ( spawnPoints[i] == point ) {
			spawnPoints.RemoveIndex ( i );
			break;
		}
	}
}

/*
==============
rvSpawner::GetSpawnPoint
==============
*/
void rvSpawner::AddCallback ( idEntity* owner, const idEventDef* ev ) {	
	spawnerCallback_t& callback = callbacks.Alloc ( );
	callback.event = ev->GetName ( );
	callback.ent = owner;
}

/*
==============
rvSpawner::GetSpawnPoint
==============
*/
idEntity *rvSpawner::GetSpawnPoint ( void ) {
	idBounds						bounds( idVec3( -16, -16, 0 ), idVec3( 16, 16, 72 ) );
	idList< idEntityPtr<idEntity> >	spawns;
	int								spawnIndex;
	idEntity*						spawnEnt;

	// Run through all spawnPoints and choose a random one. Each time a spawn point is excluded
	// it will be removed from the list until there are no more items in the list.
	for ( spawns = spawnPoints ; spawns.Num(); spawns.RemoveIndex ( spawnIndex ) ) {		
		spawnIndex = gameLocal.random.RandomInt ( spawns.Num() );
		spawnEnt   = spawns[spawnIndex];
		
		if ( !spawnEnt || !spawnEnt->GetPhysics() ) {
			continue;
		}
		
		// Check to see if something is in the way at this spawn point
		if ( !ValidateSpawnPoint ( spawnEnt->GetPhysics()->GetOrigin(), bounds ) ) {
			continue;
		}
		
		// Skip the spawn point because its currently visible?
		if ( skipVisible && gameLocal.GetLocalPlayer()->CanSee ( spawnEnt, true ) ) {
			continue;
		}

		// Found one!
		return spawnEnt;
	}
		
	return NULL;
}

/*
==============
rvSpawner::GetSpawnType
==============
*/
const char* rvSpawner::GetSpawnType ( idEntity* spawnPoint ) {
	const idKeyValue* kv;
	
	if ( spawnPoint ) {
		// If the spawn point has any "def_spawn" keys then they override the normal spawn keys
		kv = spawnPoint->spawnArgs.MatchPrefix ( "def_spawn", NULL );
		if ( kv ) {
			const char* types [ MAX_SPAWN_TYPES ];
			int			typeCount;

			for ( typeCount = 0; 
				  typeCount < MAX_SPAWN_TYPES && kv; 
				  kv = spawnPoint->spawnArgs.MatchPrefix ( "def_spawn", kv ) ) {
				types [ typeCount++ ] = kv->GetValue ( ).c_str();
			}
			
			return types[ gameLocal.random.RandomInt( typeCount ) ];
		}
	}
	
	// No spawn types?
	if ( !spawnTypes.Num ( ) ) {
		return "";
	}
	
	// Return from the spawners list of types
	return spawnTypes[ gameLocal.random.RandomInt( spawnTypes.Num() ) ];
}

/*
==============
rvSpawner::CopyPrefixedSpawnArgs
==============
*/
void rvSpawner::CopyPrefixedSpawnArgs( idEntity *src, const char *prefix, idDict &args ){
	const idKeyValue *kv = src->spawnArgs.MatchPrefix( prefix, NULL );
	while( kv ) {
		args.Set( kv->GetKey().c_str() + idStr::Length( prefix ), kv->GetValue() );
		kv = src->spawnArgs.MatchPrefix( prefix, kv );
	}
}

/*
==============
rvSpawner::SpawnEnt
==============
*/
bool rvSpawner::SpawnEnt( void ){
	idDict		args;
	idEntity*	spawnPoint;
	idEntity*	spawnedEnt;
	const char* temp;

	// Find a spawn point to spawn the entity
	spawnPoint = GetSpawnPoint ( );
	if( !spawnPoint ){
		return false;
	}

	// No valid spawn types for this point
	temp = GetSpawnType ( spawnPoint );
	if ( !temp || !*temp ) {
		gameLocal.Warning ( "Spawner '%s' could not find any valid spawn types for spawn point '%s'", GetName(), spawnPoint->GetName() );
		return false;
	}

	// Build the spawn parameters for the entity about to be spawned
	args.Set	   ( "origin",			spawnPoint->GetPhysics()->GetOrigin().ToString() );
	args.SetFloat  ( "angle",			spawnPoint->GetPhysics()->GetAxis().ToAngles()[YAW] );
	args.Set	   ( "classname",		temp );	
	args.SetBool   ( "forceEnemy",		spawnArgs.GetBool ( "auto_target", "1" ) );
	args.SetBool   ( "faceEnemy",		spawnArgs.GetBool ( "faceEnemy", "0" ) );

	// Copy all keywords prefixed with "spawn_" to the entity being spawned.
	CopyPrefixedSpawnArgs( this, "spawn_", args );
	if( spawnPoint != this ) {
		CopyPrefixedSpawnArgs( spawnPoint, "spawn_", args );
	}

	// Spawn the entity
	if ( !gameLocal.SpawnEntityDef ( args, &spawnedEnt ) ) {
		return false;
	}

	// Activate the spawned entity
	spawnedEnt->ProcessEvent( &EV_Activate, this );

	// Play a spawning effect if it has one - do we possibly want some script hooks in here?
	gameLocal.PlayEffect ( spawnArgs, "fx_spawning", spawnPoint->GetPhysics()->GetOrigin(), idVec3(0,0,1).ToMat3() );

	// script function for spawning guys
	if( spawnArgs.GetString( "call", "", &temp ) && *temp ) {
		gameLocal.CallFrameCommand ( this, temp );
	}

	// script function for the guy being spawned
	if ( spawnArgs.GetString( "call_spawned", "", &temp ) && *temp ) {
		gameLocal.CallFrameCommand ( spawnedEnt, temp );
	}

	// Call all of our callbacks
	int c;
	for ( c = callbacks.Num() - 1; c >= 0; c-- ) {
		if ( callbacks[c].ent ) {
			callbacks[c].ent->ProcessEvent ( idEventDef::FindEvent ( callbacks[c].event ), this, spawnedEnt );
		}
	}

	// Activate the spawn point entity when an enemy is spawned there and all of its targets
	if( spawnPoint != this ){
		spawnPoint->ProcessEvent( &EV_Activate, spawnPoint );
		spawnPoint->ActivateTargets( spawnedEnt );
		
		// One time use on this target?
		if ( spawnPoint->spawnArgs.GetBool ( "remove" ) ) {
			spawnPoint->PostEventMS ( &EV_Remove, 0 );
		}
	}

	// Increment the total number spawned
	numSpawned++;

	return true;
}

/*
==============
rvSpawner::Think
==============
*/
void rvSpawner::Think( void ){
	if ( thinkFlags & TH_THINK ) {
		if( ActiveListChanged() ) {// If an entity has been removed and we have not been informed via Detach
			nextSpawnTime = gameLocal.GetTime() + spawnDelay;
		}

		CheckSpawn ( );
	}
}

/*
==============
rvSpawner::CheckSpawn
==============
*/
void rvSpawner::CheckSpawn ( void ) {
	int count;

	// Any spawn points?
	if ( !spawnPoints.Num ( ) ) {
		return;
	}

	// Is it time to spawn yet?
	if ( nextSpawnTime == 0 || gameLocal.time < nextSpawnTime ) {
		return;
	}

	// Any left to spawn?
	if ( maxToSpawn > -1 && numSpawned >= maxToSpawn ){
		return;
	}

	// Spawn in waves?
	for ( count = 0; count < spawnWaves; count ++ ) {
		// Too many active?
		if( currentActive.Num() >= maxActive ) {
			return;
		}

		// Spawn a new entity
		SpawnEnt ( );

		// Are we at the limit now?
		if ( maxToSpawn > -1 && numSpawned >= maxToSpawn ) {
			CallScriptEvents( "script_used_up", this );
			PostEventMS ( &EV_Remove, 0 );
			break;
		}
	}

	// Dont spawn again until after the delay
	nextSpawnTime = gameLocal.time + spawnDelay;
}

/*
==============
rvSpawner::CallScriptEvents
==============
*/
void rvSpawner::CallScriptEvents( const char* prefixKey, idEntity* parm ) {
	if( !prefixKey || !prefixKey[0] ) {
		return;
	}

	rvScriptFuncUtility func;
	for( const idKeyValue* kv = spawnArgs.MatchPrefix(prefixKey); kv; kv = spawnArgs.MatchPrefix(prefixKey, kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}

		if( func.Init(kv->GetValue()) <= SFU_ERROR ) {
			continue;
		}

		func.InsertEntity( parm, 0 );
		func.CallFunc( &spawnArgs );
	}
}

/*
==============
rvSpawner::ActiveListChanged
==============
*/
bool rvSpawner::ActiveListChanged() {
	int previousCount = currentActive.Num();

	currentActive.RemoveNull();

	return previousCount > currentActive.Num();
}

/*
==============
rvSpawner::Attach

Attach the given AI to the spawner.  This will increase the active count of the spawner and
set the spawner pointer in the ai.
==============
*/
void rvSpawner::Attach ( idEntity* ent ) {
	currentActive.AddUnique( ent );
}

/*
==============
rvSpawner::Detach

Detaches the given AI from the spawner which will free up an active spot for spawning.
Attach the given AI to the spawner.  This will increase the active count of the spawner and
set the spawner pointer in the ai.
==============
*/
void rvSpawner::Detach ( idEntity* ent ){
	currentActive.Remove( ent );

	nextSpawnTime = gameLocal.GetTime() + spawnDelay;
}

/*
==============
rvSpawner::Event_Activate
==============
*/
void rvSpawner::Event_Activate ( idEntity *activator ) {
	
	// "trigger_only" spawners will attempt to spawn when triggered
	if ( spawnArgs.GetBool ( "trigger_only" ) ) {
		// Update next spawn time to follo CheckSpawn into thinking its time to spawn again
		nextSpawnTime = gameLocal.time;
		CheckSpawn ( );
		return;
	}
	
	// If nextSpawnTime is zero then the spawner is currently deactivated
	if ( nextSpawnTime == 0 ) {
		// Start thinking
		BecomeActive( TH_THINK );
		
		// Allow immediate spawn
		nextSpawnTime = gameLocal.time;
		
		// Spawn any ai targets and add them to the current count
		ActivateTargets ( this );
	} else {
		nextSpawnTime = 0;
		BecomeInactive( TH_THINK );
		
		// Remove the spawner if need be
		if ( spawnArgs.GetBool ( "remove", "1" ) ) {
			PostEventMS ( &EV_Remove, 0 );
		}
	}
}

/*
==============
rvSpawner::Event_RemoveNullActiveEntities
==============
*/
void rvSpawner::Event_RemoveNullActiveEntities( void ) {
	for( int ix = currentActive.Num() - 1; ix >= 0; --ix ) {
		if( !currentActive[ix].IsValid() ) {
			currentActive.RemoveIndex( ix );
		}
	}
}

/*
==============
rvSpawner::Event_NumActiveEntities
==============
*/
void rvSpawner::Event_NumActiveEntities( void ) {
	idThread::ReturnInt( currentActive.Num() );
}

/*
==============
rvSpawner::Event_GetActiveEntity
==============
*/
void rvSpawner::Event_GetActiveEntity( int index ) {
	idThread::ReturnEntity( (index < 0 || index >= currentActive.Num()) ? NULL : currentActive[index] );
}

