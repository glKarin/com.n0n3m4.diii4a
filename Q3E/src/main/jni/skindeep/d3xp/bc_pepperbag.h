#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idPepperBag : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idPepperBag);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:



	int						nextFallingDamageTime;
	int						spewType;

	



};
//#pragma once