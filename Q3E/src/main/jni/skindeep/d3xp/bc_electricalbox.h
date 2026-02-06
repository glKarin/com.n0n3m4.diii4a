#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idElectricalBox : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idElectricalBox);

							idElectricalBox(void);
	virtual					~idElectricalBox(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	
	
	virtual void			DoRepairTick(int amount);


	bool					IsElectricalboxAlive();

	virtual bool			Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location);

private:

	enum					{ IDLE, DEAD };
	int						state;

	virtual void			Event_PostSpawn(void);

	idFuncEmitter			*idleSmoke = nullptr;

	idList<int>				pipeIndexes;
	idList<int>				lightIndexes;
	idList<int>				turretIndexes;
	idList<int>				securitycameraIndexes;

	int						hazardType;


	bool					interestDelayActive;
	int						interestDelayTimer;

};
//#pragma once