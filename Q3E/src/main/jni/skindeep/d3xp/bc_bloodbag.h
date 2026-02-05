#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idBloodbag : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idBloodbag);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	bool					hasExploded;

	void					BloodExplosion();

	

};
//#pragma once