//#include "Player.h"

#include "Light.h"
#include "ai/AI.h"

class idAI_Repairbot : public idAI
{
public:
	CLASS_PROTOTYPE(idAI_Repairbot);
	
							idAI_Repairbot(void);
	virtual					~idAI_Repairbot(void);

	void					Spawn(void);
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);
	virtual void			Think(void);

	bool					SetRepairTask(idEntity * thingToRepair);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	idEntityPtr<idEntity>	repairEnt;

	idEntityPtr<idEntity>	queuedRepairEnt;

private:

	//void					Event_PostSpawn(void);

	//Don't change this order. The IsBotRepairingSomethingNearby() function expects this enum in this order...
	enum
	{
		REPAIRBOT_IDLE,
		REPAIRBOT_SEEKINGREPAIR,
		REPAIRBOT_REPAIRING,
		REPAIRBOT_DEATHCOUNTDOWN,
		REPAIRBOT_PAIN,
	};

	
	idVec3					repairPosition;
	int						updateTimer;


	void					GotoIdle();

	void					BlowUp();

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	idLight *				headLight = nullptr;
	idFuncEmitter			*repairParticles = nullptr;

	int						patrolCooldownTimer;

	void					Event_Touch(idEntity* other, trace_t* trace);
	int						blockVoTimer;

	bool					isMovingToDespawn;
	int						despawnTimer;
	idEntityPtr<idEntity>	despawnHatch;
	int						despawnProximityTimer;

	idFuncEmitter			*talkParticles = nullptr;

	idVec3					AttemptGoToRepairPoint(idEntity *ent);

	renderLight_t			selfglowLight;
	qhandle_t				selfglowlightHandle;
};
//#pragma once