#pragma once
#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"
#include "bc_healthstation.h"


class idLostandfoundMonitor : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idLostandfoundMonitor);

							idLostandfoundMonitor(void);
	virtual					~idLostandfoundMonitor(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

    virtual void			DoRepairTick(int amount);

	virtual void			Event_PostSpawn(void);


private:

	void					RefreshList();

	enum					{ LFM_ACTIVE, LFM_DEAD };
	int						securityState;	

	idFuncEmitter			*idleSmoke = nullptr;

	int						thinkTimer;

    bool                    unlocked;
	
	idEntityPtr<idEntity>	lostandfoundMachine;
	idListGUI *				itemList = nullptr;

	idProximityAnnouncer	proximityAnnouncer;
};
//#pragma once