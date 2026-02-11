#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idTablet : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idTablet);

							idTablet(void);
	virtual					~idTablet(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual void			Hide(void);

	void					SetRead();
	bool					GetRead();

private:
		
	enum					{ TBLT_LOCKED, TBLT_UNLOCKED };
	int						state;
	int						thinkTimer;

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	void					CreateMemorypalaceClone();
};
//#pragma once