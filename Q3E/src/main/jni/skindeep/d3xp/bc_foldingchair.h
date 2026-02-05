#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
//#include "Light.h"


class idFoldingchair : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idFoldingchair);

                            idFoldingchair(void);
	virtual					~idFoldingchair(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

private:
	
	enum					{FCH_CLOSED, FCH_OPEN};
	int						state;
	int						stateTimer;

	idEntity*				frobCube = nullptr;

	idMover*				seatMover = nullptr;
	idVec3					seatPos_open;
};
//#pragma once