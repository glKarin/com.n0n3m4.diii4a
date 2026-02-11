#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idSecurityStation : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idSecurityStation);

							idSecurityStation(void);
	virtual					~idSecurityStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

    virtual void			DoRepairTick(int amount);

    void                    Unlock();

private:


	enum					{ SECURITYSTAT_LOCKED, SECURITYSTAT_ACTIVE, SECURITYSTAT_DEAD };
	int						securityState;
	

	idFuncEmitter			*idleSmoke = nullptr;

	int						thinkTimer;

    bool                    unlocked;
	

};
//#pragma once