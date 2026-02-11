#include "Misc.h"
#include "Entity.h"

#include "Item.h"

#define DEIMPALE_FROBINDEX 7

class idSpearprojectile : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSpearprojectile);

							idSpearprojectile(void);
	virtual					~idSpearprojectile(void);
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	enum
	{
		SPEAR_RAMMING = 0,
		SPEAR_IMPALED,
		SPEAR_CHARGING,
		SPEAR_DONE
	};

	int						spearState;
	idVec3					ramPoint;
	int						ramSpeed;
	idVec3					ramDir;

	const idDeclParticle *	flyParticles = nullptr;
	int						flyParticlesFlyTime;

	bool					hasdoneSpawnBoundCheck;

    idVec3                  FindValidSpawnPosition();
	idVec3					FindValidTrajectory(idVec3 spawnPos);

    idVec3                  collisionNormal;

	void					Launch();
	int						chargeTimer;

	idEntityPtr<idEntity>	enemyPtr;	

	bool					pluckedFromBelly;

	bool					impaledInMonster; //in monster, not player.

};
//#pragma once