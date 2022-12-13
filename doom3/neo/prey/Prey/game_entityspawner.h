#ifndef GAME_ENTITYSPAWNER_H
#define GAME_ENTITYSPAWNER_H

//
// hhEntitySpawner
//
class hhEntitySpawner : public idEntity {
public:
	CLASS_PROTOTYPE( hhEntitySpawner );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	
protected:
	void				Event_Activate(idEntity *activator);	
	void				Event_SpawnEntity( void );		

	idStr				entDefName;
	idDict				entSpawnArgs;
	int					maxSpawnCount;
	int					currSpawnCount;
};

#endif