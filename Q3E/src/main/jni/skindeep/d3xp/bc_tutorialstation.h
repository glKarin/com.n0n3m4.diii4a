#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idTutorialStation : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idTutorialStation);

							idTutorialStation(void);
	virtual					~idTutorialStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

private:

	enum					{ TUTSTAT_IDLE, TUTSTAT_BLINKING, TUTSTAT_DORMANT };
	bool					tutState;
	int						stateTimer;

	void					Event_Activate(idEntity *activator);

	idFuncEmitter			*idleSmoke = nullptr;

};
//#pragma once