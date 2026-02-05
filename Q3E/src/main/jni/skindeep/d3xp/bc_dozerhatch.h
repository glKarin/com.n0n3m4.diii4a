#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

const int MAXTRASHFISH = 6;

class idDozerhatch : public idAnimated
{
public:
	CLASS_PROTOTYPE(idDozerhatch);

							idDozerhatch(void);
	virtual					~idDozerhatch(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	void					StartSpawnSequence(idEntity * repairableEnt, int botType);

	bool					IsCurrentlySpawning();

	void					SetTeam(int value);

private:

	idEntity * 				SpawnBot(int botType);

	int						stateTimer;
	int						hatchState;
	enum					{ HATCH_IDLE, HATCH_OPENING, HATCH_COOLDOWN  };

	idStr					GetBotDef(int botType);


	
};
//#pragma once