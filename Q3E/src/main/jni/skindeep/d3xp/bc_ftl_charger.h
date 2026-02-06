#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idFTLCharger : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idFTLCharger);

							idFTLCharger(void);
	virtual					~idFTLCharger(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity* frobber = NULL);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	//virtual void			DoRepairTick(int amount);

	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);


private:

	enum					{ FC_NONE, FC_CHARGING, FC_POSTMESSAGE };
	int						state;
	int						stateTimer;

	void					StopCharge();

	idFuncEmitter*			particleEmitter = nullptr;
	idVec3					GetEmitterOrigin();



};
//#pragma once