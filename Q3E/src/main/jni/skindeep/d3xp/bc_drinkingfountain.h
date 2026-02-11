#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idDrinkingFountain : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idDrinkingFountain);

							idDrinkingFountain(void);
	virtual					~idDrinkingFountain(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:
		
	idFuncEmitter			*faucetEmitter = nullptr;
	idVec3					GetFaucetPos();
	idVec3					GetDrinkPos();

	bool					sinkIsOn;
	int						sinkTimer;

	int						slurpTimer;
	bool					isSlurping;

	idFuncEmitter*			drinkEmitter = nullptr;

};
//#pragma once