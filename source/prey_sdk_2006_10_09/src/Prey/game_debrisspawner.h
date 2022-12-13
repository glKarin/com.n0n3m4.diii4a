#ifndef __PREY_GAME_DEBRISMASS_H__
#define __PREY_GAME_DEBRISMASS_H__

class hhDebrisSpawner : public idEntity {
	CLASS_PROTOTYPE( hhDebrisSpawner );

 public:
						hhDebrisSpawner();
	virtual				~hhDebrisSpawner();
	void				Spawn();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				Activate( idEntity *sourceEntity );

	ID_INLINE float		GetDuration() const { return duration; }
	
 protected:
	void				Event_RemoveAll();

	void				SpawnDebris();
	void				SpawnFX();
	void				SpawnDecals( void );

	bool				GetNextDebrisData( idList<idStr> &entityDefs, int &count,
										   const idKeyValue * &kv, idVec3 &origin, idAngles &angle );
protected:
	idVec3				origin;
	idVec3				orientation;
	idVec3				velocity;
	idVec3				power;
	bool				activated;
	bool				multiActivate;
	bool				useEntity;		// Use an entity to spawn in
	bool				useAFBounds; //rww - use collective AF bodies to produce an appropriate bounds for spawning.
	bool				hasBounds;
	idBounds			bounds;
	// Vars for removing everything after a fixed period
	float				duration;
	
	bool				fillBounds;		// Try to fill in the bounds with the gibs.
	bool				testBounds;		// test to see if debris fits in bounds
	bool				nonSolid;

	idEntity *			sourceEntity;
};

#endif __PREY_GAME_DEBRISMASS_H__
