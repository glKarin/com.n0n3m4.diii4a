#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"


class idCargohide : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idCargohide);

							idCargohide(void);
	virtual					~idCargohide(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	int						maxhealth;


};
//#pragma once