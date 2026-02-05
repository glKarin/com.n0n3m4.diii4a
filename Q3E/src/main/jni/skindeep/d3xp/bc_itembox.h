#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

const int MAXTRASHFISH = 6;

class idItembox : public idAnimated
{
public:
	CLASS_PROTOTYPE(idItembox);

							idItembox(void);
	virtual					~idItembox(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

private:

	int						stateTimer;
	int						state;
	enum					{ CLOSED, OPENING, FINISHINGANIMATION, OPENDONE, DEAD  };
	void					SpawnItem();
	idEntity *				PopOffLid();
	idStr					itemSpawnType;
	
};
//#pragma once