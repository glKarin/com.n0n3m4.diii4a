#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idSecurityKey : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSecurityKey);

							idSecurityKey(void);
	virtual					~idSecurityKey(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

    virtual void			Think(void);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Hide(void);
	virtual bool			Collide(const trace_t& collision, const idVec3& velocity);

private:

    renderLight_t			headlight;
    qhandle_t				headlightHandle;

	int						thinkTimer;



	//void					DoJumpLogic();
	//int						jumpTimer;

};
//#pragma once