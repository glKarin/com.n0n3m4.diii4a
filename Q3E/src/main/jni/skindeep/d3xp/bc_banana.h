#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idBanana : public idFood
{
public:
	CLASS_PROTOTYPE(idBanana);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			Collide(const trace_t& collision, const idVec3& velocity);

	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	//
	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	int						spawnTime;
	bool					hasBecomePeel;



	

};
//#pragma once