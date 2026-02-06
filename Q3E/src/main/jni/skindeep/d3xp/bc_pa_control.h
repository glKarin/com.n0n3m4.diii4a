#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idPA_Control : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idPA_Control);

							idPA_Control(void);
	virtual					~idPA_Control(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			DoRepairTick(int amount);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	

private:

	bool					IsEnemyNear();

	enum					{ PA_NONE, PA_PLAYERSPEAKING };
	int						state;
	int						stateTimer;

	void					InterruptAllclear();

	idVec3					GetMicPosition();

	idFuncEmitter			*idleSmoke = nullptr;

	idAnimatedEntity*		flagModel;
	int						flagCheckTimer;
	int						flagState;
	enum                    {FLG_INACTIVE, FLG_ACTIVATING, FLG_ACTIVE};

	idFuncEmitter*			soundwaveEmitter;


};
//#pragma once