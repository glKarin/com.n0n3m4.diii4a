#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idRadio : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idRadio);

							idRadio(void);
	virtual					~idRadio(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	//virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual bool			DoFrobHold(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);


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
};
//#pragma once