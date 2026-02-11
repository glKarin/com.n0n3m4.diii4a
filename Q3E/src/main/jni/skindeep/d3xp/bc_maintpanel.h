#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"


class idMaintPanel : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idMaintPanel);

							idMaintPanel(void);
	virtual					~idMaintPanel(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

	//void					Event_Activate(idEntity *activator);
	void					Unlock(bool unlockAll);

	virtual void			Event_PostSpawn(void);

	void					Close();

	bool					IsDone();

	int						GetSystemIndex();


private:

	bool					IsEntTargetingMe(idEntity *ent);
	void					UnlockSystemIndex();

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	idFuncEmitter			*smokeParticle = nullptr;
	idFuncEmitter			*soundParticle = nullptr;

	int						state;
	int						stateTimer;
	enum					
	{
		MPL_JAMMED,
		MPL_WAITINGFORPRESS,
		MPL_CLOSING,
		MPL_DONE
	};

	//Note: we need to ensure this enum list remains in parity with the 'systemindex' values in gameplay.def (enum is in game_local.h)
	int						systemIndex;

	
	idStaticEntity*			videoSignage = nullptr;
	int						videoSignageLifetimer;
	bool					videoSignageActive;

};
//#pragma once