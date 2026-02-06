#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

const int MAXTRASHFISH = 5;

class idTrashfishHive : public idAnimated
{
public:
	CLASS_PROTOTYPE(idTrashfishHive);

							idTrashfishHive(void);
	virtual					~idTrashfishHive(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

private:

	//idEntityPtr<idEntity>	airlockEnt;
	//virtual void			Event_PostSpawn(void);

	idEntityPtr<idEntity>	fishes[MAXTRASHFISH];

	int						fishspawnTimer;

	int						idleTimer;
	
	int						hiveState;
	enum					{ HIVE_IDLE, HIVE_SPAWNING };


	
};
//#pragma once