#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idInfoMap : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idInfoMap);

							idInfoMap(void);
	virtual					~idInfoMap(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	void					SetCurrentNode(int navnode, int nextNavnode);

private:

	virtual void			Event_PostSpawn(void);

	enum					{ INFOMAP_IDLE, INFOMAP_REBOOTING };
	bool					infoState;
	int						stateTimer;

	//void					Event_Activate(idEntity *activator);

	idFuncEmitter			*idleSmoke = nullptr;

	idEntityPtr<idEntity>	FTLDrive_ptr;

};
//#pragma once