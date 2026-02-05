#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idLighter : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idLighter);

							idLighter(void);
	virtual					~idLighter(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

    virtual void			Think(void);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
    
    virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	
    virtual void			JustThrown();

    virtual void			Hide(void);
    virtual void			Show(void);

private:

    renderLight_t			headlight;
    qhandle_t				headlightHandle;

    const idDeclParticle *	trailParticles = nullptr;
    int						trailParticlesFlyTime;

	int						thinkTimer;

    int                     lighterState;
    enum                    { LTR_OFF, LTR_ON, LTR_ON_LINGER };

    void                    KillLight();

    idAnimatedEntity*       animatedEnt = nullptr;
    void                    SetLightState(bool activate);
    idVec3                  lastPosition;

};
//#pragma once