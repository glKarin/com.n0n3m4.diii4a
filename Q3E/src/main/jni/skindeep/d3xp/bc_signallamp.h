
/*
#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idSignallamp : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSignallamp);

							idSignallamp(void);
	virtual					~idSignallamp(void);

	void					Save(idSaveGame *savefile) const;
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

    virtual void			Think(void);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
    
    virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
    virtual bool			DoFrobHold(int index = 0, idEntity* frobber = NULL);

    virtual void			Hide(void);
    virtual void			Show(void);

private:

    renderLight_t			headlight;
    qhandle_t				headlightHandle;

	int						thinkTimer;

    void                    KillLight();

    idAnimatedEntity*       animatedEnt;
    void                    SetLightState(bool activate);
    idVec3                  lastPosition;

};
//#pragma once

*/