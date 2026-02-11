#include "Misc.h"
#include "Entity.h"


class idHanddryer : public idAnimated
{
public:
	CLASS_PROTOTYPE(idHanddryer);

							idHanddryer(void);
	virtual					~idHanddryer(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

private:

	idTraceModel			trm;

	int						frobTimer;

	int						deathTimer;

	bool					hasDied;
};
//#pragma once