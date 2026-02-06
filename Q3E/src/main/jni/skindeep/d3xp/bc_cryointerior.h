#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idCryointerior : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idCryointerior);

							idCryointerior(void);
	virtual					~idCryointerior(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					SetExitPoint(idEntity *ent);
	

private:

	enum					{ IDLE, OPENING, MOVINGHEAD, DONE };
	int						state;
	int						stateTimer;

	idLight *				ceilingLight = nullptr;
	idAnimated*				doorProp = nullptr;

	idEntity*				frobBar = nullptr;

	idEntityPtr<idEntity>	cryospawn;
	
	idEntity				*ventpeekEnt = nullptr;

	int						iceMeltTimer;
	enum					{ ICEMELT_INITIALDELAY, ICEMELT_MELTING, ICEMELT_LIGHTFADE, ICEMELT_DONE };
	int						iceMeltState;

	idFuncEmitter			*dripEmitter = nullptr;

	void					SetCryoFrobbable(int value);


};
//#pragma once