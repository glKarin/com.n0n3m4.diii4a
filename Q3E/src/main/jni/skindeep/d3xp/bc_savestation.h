#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idSaveStation : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idSaveStation);

							idSaveStation(void);
	virtual					~idSaveStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			DoRepairTick(int amount);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	

private:


	enum					{ SV_READY, SV_LOCKDOWN, SV_BROKEN };
	int						state;
	int						stateTimer;
	idFuncEmitter			*idleSmoke = nullptr;

	bool					savebuttonDelayActive;

	void					DoSave();

	




};
//#pragma once