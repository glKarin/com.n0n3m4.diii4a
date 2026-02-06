#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idOxygenbubble : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idOxygenbubble);

							idOxygenbubble(void);
	virtual					~idOxygenbubble(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	int						bubbleState;
	int						bubbleTimer;
	enum					{ OB_SPAWNING, OB_IDLE, OB_DEAD };

	

};
//#pragma once