#include "Misc.h"
#include "Entity.h"

#include "Item.h"



class idChembomb: public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idChembomb);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	

private:
//
//	int						nextTouchTime;

	//void					TeleportThingsTo(int listedEntities, idEntity *entityList[], idVec3 sourcePosition, idVec3 targetPosition);

	void					Detonate(idVec3 detonationPos, idMat3 particleDirection);
	const idDeclParticle *	smokeFly = nullptr;
	int						smokeFlyTime;

	void					SpawnChemPuddle(trace_t traceInfo, bool playAudio);
	void					ChemRadiusAttack(idVec3 epicenter);

};
//#pragma once