#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

//#include "bc_frobcube.h"


class idToilet : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idToilet);

							idToilet(void);
	virtual					~idToilet(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	idAnimated*				handle = nullptr;
	int						flushTimer;

	int						flushMode;

	idVec3					GetWaterjetPosition();
	int						waterjetTimer;


	
	void					GrabAndFlushObjects();

	int						state;
	enum					{TLT_IDLE, TLT_STRAINING, TLT_STRAINDONE};
	int						stateTimer;


	int						grabTimer;

	void					PullObjectsCloser();
};
//#pragma once