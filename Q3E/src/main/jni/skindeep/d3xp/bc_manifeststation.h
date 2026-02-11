#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idManifestStation : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idManifestStation);

							idManifestStation(void);
	virtual					~idManifestStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

    virtual void			DoRepairTick(int amount);

    void                    Unlock();

	void					UpdateInfos();

private:

	virtual void			Event_PostSpawn(void);

	enum					{ STAT_ACTIVE, STAT_AWAITINGUPDATE, STAT_DEAD };
	int						state;	

	idFuncEmitter			*idleSmoke = nullptr;

	int						thinkTimer;

    bool                    unlocked;
	

};
//#pragma once