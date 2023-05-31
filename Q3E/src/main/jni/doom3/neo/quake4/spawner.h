/*
================

Spawner.h

================
*/

#ifndef __GAME_SPAWNER_H__
#define __GAME_SPAWNER_H__

const int MAX_SPAWN_TYPES	= 32;

class rvSpawner;

typedef void (*spawnerCallbackProc_t) ( rvSpawner* spawner, idEntity* spawned, int userdata );

typedef struct {
	idEntityPtr<idEntity>	ent;
	idStr					event;
} spawnerCallback_t;

/*
===============================================================================

  rvSpawner

===============================================================================
*/
class rvSpawner : public idEntity {
public:
	CLASS_PROTOTYPE( rvSpawner );

	void				Spawn					( void );
	void				Think					( void );

	void				Attach					( idEntity* ent );
	void				Detach					( idEntity* ent );

	void				Save					( idSaveGame *savefile ) const;
	void				Restore					( idRestoreGame *savefile );

	void				AddSpawnPoint			( idEntity* point );
	void				RemoveSpawnPoint		( idEntity* point );

	int					GetNumSpawnPoints		( void ) const;
	int					GetNumActive			( void ) const;
	int					GetMaxActive			( void ) const;
	idEntity*			GetSpawnPoint			( int index );

	virtual void		FindTargets				( void );
	bool				ActiveListChanged		( void );

	void				CallScriptEvents		( const char* prefixKey, idEntity* parm );

	void				AddCallback				( idEntity* owner, const idEventDef* ev );

protected:

	int								numSpawned;
	int								maxToSpawn;
	float							nextSpawnTime;
	int								maxActive;
	idList< idEntityPtr<idEntity> >	currentActive;
	int								spawnWaves;
	int								spawnDelay;
	bool							skipVisible;
	idStrList						spawnTypes;

	idList< idEntityPtr<idEntity> >	spawnPoints;
	
	idList< spawnerCallback_t >		callbacks;

	// Check to see if its time to spawn
	void				CheckSpawn				( void );
	
	// Spawn a new entity
	bool				SpawnEnt				( void );

	// Populate the spawnType list with the available spawn types
	void				FindSpawnTypes			( void );

	// Get a random spawnpoint to spawn at
	idEntity*			GetSpawnPoint			( void );
	
	// Get a random spawn type
	const char*			GetSpawnType			( idEntity* spawnPoint );
	
	// Validate the given spawn point for spawning
	bool				ValidateSpawnPoint		( const idVec3 origin, const idBounds &bounds );

	// Copy key/values from the given entity to the given dictionary using the specified prefix
	void				CopyPrefixedSpawnArgs	( idEntity *src, const char *prefix, idDict &args );

private:

	void				Event_Activate			( idEntity *activator );
	void				Event_RemoveNullActiveEntities( void );
	void				Event_NumActiveEntities	( void );
	void				Event_GetActiveEntity	( int index );
};


ID_INLINE int rvSpawner::GetNumSpawnPoints( void ) const {
	return spawnPoints.Num ( );
}

ID_INLINE idEntity* rvSpawner::GetSpawnPoint( int index ) {
	return spawnPoints[index];
}

ID_INLINE int rvSpawner::GetNumActive( void ) const {
	return currentActive.Num();
}

ID_INLINE int rvSpawner::GetMaxActive( void ) const {
	return maxActive;
}

#endif // __GAME_SPAWNER_H__
