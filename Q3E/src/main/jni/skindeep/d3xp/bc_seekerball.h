#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idSeekerBall : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSeekerBall);

							idSeekerBall(void);
	virtual					~idSeekerBall(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	void					Event_Touch(idEntity *other, trace_t *trace);		
	virtual void			Think(void);

	void					Fizzle();

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	//virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	enum					{SB_WANDERING, SB_PURSUIT, SB_EXPLOSIONDELAY, SB_DONE};
	int						state;	
	int						stateTimer;

	idEntityPtr<idEntity>	enemyTarget;

	idEntity *				FindEnemyTarget();
	void					LostTarget();
	void					StartExplosionSequence();

	renderLight_t			headlight;
	qhandle_t				headlightHandle;


	enum					{ LIFETIME_IDLE, LIFETIME_COUNTDOWN, LIFETIME_DONE };
	int						lifetimeState;
	int						lifetimeTimer;


};
//#pragma once