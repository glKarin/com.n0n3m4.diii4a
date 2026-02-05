#include "Misc.h"
#include "Entity.h"


class idEnviroSpawner : public idEntity
{
public:
	CLASS_PROTOTYPE(idEnviroSpawner);

							idEnviroSpawner(void);
	virtual					~idEnviroSpawner(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	virtual void			Event_PostSpawn(void);

private:

	class idAsteroidParms
	{
		public:
			idStr			asteroidArgsName;
			const idDict*	asteroidSpawnArgs;
			idBounds		asteroidBox;
			float			chanceOfSpawning; // Probability (0 to 1) of this asteroid type overriding the default asteroid type. Ignored for the default (index 0)

			idAsteroidParms(const idDict* spawnArgs, idBounds box, float chanceOfSpawn) 
			{
				asteroidSpawnArgs = spawnArgs;
				asteroidBox = box;
				chanceOfSpawning = chanceOfSpawn;
			}

			idAsteroidParms(void) {};
			~idAsteroidParms(void) {};
	};

	idBounds				spawnBox;
	idList<idBounds>		occluderBoxes; // we will refrain from spawning asteroids inside these bounds

	idList<idAsteroidParms>	asteroidEntries; // list of all asteroid types spawned by this envirospawner. Index 0 is the 'default' asteroid.

	void					SpawnElement();

	int						timer;
	int						spawnRate;
	int						despawnThreshold; // if an asteroid's coordinates pass this value on the Y-axis, it despawns
	
	idVec3					lastSpawnPos;

	bool					state;
	enum					{ESP_OFF, ESP_ON};

	

};
//#pragma once