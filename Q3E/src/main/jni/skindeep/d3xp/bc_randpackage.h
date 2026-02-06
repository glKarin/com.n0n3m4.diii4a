#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idRandPackage : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idRandPackage);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			Collide(const trace_t& collision, const idVec3& velocity);

	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	//
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	int						spawnTime;
	bool					hasBeenOpened;

	idStr					GetRandomSpawnDef();
	
	void					OpenPackage();
};
//#pragma once