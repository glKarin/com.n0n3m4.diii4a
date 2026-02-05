#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idSmokegrenade : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSmokegrenade);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
    virtual void			Think(void);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	
    idFuncEmitter			*nozzleParticle = nullptr;
    int                     spewtimer;

    int                     state;
    enum                    { SG_SMOKEDELAY, SG_SMOKING, SG_DONE };
    int                     stateTimer;

    int                     intervalTimer;

    void                    DoRandomSmokeSpew();

	
	int						inflictTimer;


};
//#pragma once