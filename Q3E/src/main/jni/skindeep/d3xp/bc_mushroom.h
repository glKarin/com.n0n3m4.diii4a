#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

const int MAXTRASHFISH = 6;

class idMushroom : public idAnimated
{
public:
	CLASS_PROTOTYPE(idMushroom);

							idMushroom(void);
	virtual					~idMushroom(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

private:

	idFuncEmitter			*splashEnt = nullptr;

	enum					{DRIP_64, DRIP_128, DRIP_192};
	int						dripType;
	idVec3					headPosition;

	int						decalTimer;

	
};
//#pragma once