#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idDuper : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idDuper);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:
	
	idEntity *				FindAimedEntity();
	idEntity *				FindMisfireEntity();
	void					DoMisfire();
	void					DoDupe();
	idVec3					FindClearSpawnSpot();

	void					AddConditionalSpawnArgs(idDict* args, idEntity* aimedEnt);

	int						chargesRemaining;
	int						misfireCooldownTimer;
	bool					lastAimState;
	idEntityPtr<idEntity>	aimedEnt;
	int						spawnTime;

	bool					IsValidDupeItemTr(trace_t tr);
	bool					IsValidDupeItemEnt(idEntity *ent);
	

};
//#pragma once