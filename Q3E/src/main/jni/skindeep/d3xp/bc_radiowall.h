#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idRadioWall : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idRadioWall);

							idRadioWall(void);
	virtual					~idRadioWall(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	//virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	
	virtual void			Think(void);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			DoRepairTick(int amount);

private:

	renderLight_t			headlight;
	qhandle_t				headlightHandle;
	
	bool					isOn;
	void					SetActivate(bool value);

	int						activateTimer;

	idFuncEmitter			*musicNotes = nullptr;
	idFuncEmitter			*soundwaves = nullptr;

	int						interestTimer;
	idEntityPtr<idEntity>	interestPoint;

	idVec3					GetSpeakerPos();

	idFuncEmitter			*idleSmoke = nullptr;
};
//#pragma once