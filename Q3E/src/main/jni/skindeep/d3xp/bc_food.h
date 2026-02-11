#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idFood : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idFood);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual bool			Collide(const trace_t& collision, const idVec3& velocity);

private:

	bool					collideSpawn; //does it spawn a new object when it collides.





};
//#pragma once