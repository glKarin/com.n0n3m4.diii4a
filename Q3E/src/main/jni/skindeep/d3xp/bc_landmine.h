#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idLandmine : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idLandmine);

							idLandmine(void);
	virtual					~idLandmine(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			DoHack();

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	enum					{STATE_ARMING, STATE_ARMED, STATE_RADIUSWARNING, STATE_TRIPDELAY, STATE_EXPLODED };
	int						mineState;
	int						mineTimer;

	int						idleTimer;

	int						thinkTimer;

	void					StartTripDelay();

	void					DoGasSpew();

	bool					readyForWarning;

	void					SetLightColor(int faction);

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	enum					{ LFL_IDLE, LFL_FLASHING };
	int						lightflashState;
	int						lightflashTimer;
	void					DoLightFlash(int duration);
};
//#pragma once