#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idAloe : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idAloe);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	
	void					DropLeaves();

	

};
//#pragma once