#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idSink : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idSink);

							idSink(void);
	virtual					~idSink(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	idEntityPtr<idAnimated>	button1Anim;
	idEntityPtr<idEntity>	frobbutton1;
	int						button1Timer;

	idEntityPtr<idAnimated>	button2Anim;
	idEntityPtr<idEntity>	frobbutton2;

	

	idEntityPtr<idFuncEmitter> faucetEmitter;
	bool					sinkIsOn;
	idVec3					GetFaucetPos();

	int						interestTimer;

	void					CreateCloud();
	idVec3					GetSoapPos();
	int						soapCooldownTimer;


	void					Event_SetSinkActive(int value);
	void					Event_GetSinkActive(void);
};
//#pragma once