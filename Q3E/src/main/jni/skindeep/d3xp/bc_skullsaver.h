#include "Misc.h"
#include "Entity.h"
#include "Light.h"

#include "Item.h"

const int CONVEY_MAX_PATH = 16;

class idSkullsaver : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSkullsaver);

							idSkullsaver(void);
	virtual					~idSkullsaver(void);


	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	virtual void			Think(void);
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual void			Event_PostSpawn(void);
	
	idEntityPtr<idEntity>	bodyOwner; //the body that the skull belongs to.

	void					doRespawnJump(idVec3 respawnPosition);

	idEntityPtr<idEntity>	springPartner;

	virtual void			Hide(void);

	void					SetInEscapePod();
	void					StoreSkull();

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					DetachFromEnemy();

	void					ResetConveyTime();
	bool					IsConveying() const { return state == SKULLSAVER_CONVEYING || state == SKULLSAVER_CONVEYJUMP; }
	bool					IsRespawning() const { return state == SKULLSAVER_STARTRESPAWN; }

	void					SetBodyOwner(idEntity* ent);

	void					SetSpawnLocOrig(idLocationEntity * locEnt){ aiSpawnLocOrig = locEnt; }

	virtual void			JustThrown();

	//public end

private:


	int						nextJumpTime;
	int						lifetime;

	int						state;
	enum					{ SKULLSAVE_JUSTSPAWNED, SKULLSAVE_IDLE, SKULLSAVE_DESPAWNJUMPING, SKULLSAVE_DONE, SKULLSAVER_INESCAPEPOD, SKULLSAVER_ENEMYCARRIED, SKULLSAVER_CONVEYJUMP, SKULLSAVER_CONVEYING, SKULLSAVER_STARTRESPAWN, SKULLSAVER_LOSTANDFOUNDRESPAWN };

	

    renderLight_t			headlight;
	qhandle_t				headlightHandle;

	const idDeclParticle *	idleSmoke = nullptr;
	int						idleSmokeFlyTime;

	int						heybarkTimer;

	idEntityPtr<idFuncEmitter> soundwaves;

	idEntityPtr<idFuncEmitter> regnerationParticle;


	
	int						conveyorDelayTime; //Skullsaver 3.0: when spawned/dropped, how long before I levitate away to respawnpoint.
	idEntityPtr<idEntity>	conveyorRespawnpoint;
	idEntity*				GetNearestRespawnpoint(idLocationEntity * restrictLocation = nullptr);
	float					conveyorTotalMoveTime;
	idVec3					conveyorStartPosition;
	int						conveyorStartMoveTime;
	idVec3					conveyorDestPosition;
	idVec3					conveyorRespawnPos;
	idAAS*					aas = nullptr;
	int						destinationArea;

	void					SetupPath();
	idList<idVec3>			pathPoints;
	idBeam*					pathBeamOrigin[CONVEY_MAX_PATH] = {};
	idBeam*					pathBeamTarget[CONVEY_MAX_PATH] = {};

	bool					hasStoredSkull;


	bool					isLowHealthState;
	const idDeclParticle *	damageParticle = nullptr;
	int						damageParticleFlyTime;
	idEntityPtr<idFuncEmitter> damageEmitter;

	idEntityPtr<idEntity>	lostandfoundMachine;
	idEntity*				FindLostandfoundMachine();


	idEntityPtr<idLocationEntity>	aiSpawnLocOrig;

	bool					waitingForNinaReply;
	int						ninaReplyTimer;

	bool					rantcheckDone;

	//private end
};
//#pragma once