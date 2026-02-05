#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idHandSanitizer : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idHandSanitizer);

							idHandSanitizer(void);
	virtual					~idHandSanitizer(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:
		
	idFuncEmitter			*faucetEmitter = nullptr;
	idVec3					GetFaucetPos();

	bool					sinkIsOn;
	int						sinkTimer;

	void					CreateCloud();

	renderLight_t			headlight;
	qhandle_t				headlightHandle;
};
//#pragma once