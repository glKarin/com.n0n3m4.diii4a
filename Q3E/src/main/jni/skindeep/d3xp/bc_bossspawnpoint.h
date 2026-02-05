#include "Misc.h"
#include "Entity.h"


class idBossSpawnpoint : public idEntity
{
public:
	CLASS_PROTOTYPE(idBossSpawnpoint);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	

//private:
//	int						state;

	

};
//#pragma once