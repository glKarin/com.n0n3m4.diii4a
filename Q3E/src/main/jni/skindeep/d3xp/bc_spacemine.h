//#include "physics/Physics_RigidBody.h"
//#include "script/Script_Thread.h"
//#include "gamesys/Event.h"
//#include "Entity.h"
//#include "Player.h"
//#include "Projectile.h"
//#include "Misc.h"
//#include "Target.h"
#include "Moveable.h"
//#include "Mover.h"

class idSpaceMine : public idMoveable
{
public:
	CLASS_PROTOTYPE(idSpaceMine);

							idSpaceMine(void);
	virtual					~idSpaceMine(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	void					Event_Touch(idEntity *other, trace_t *trace);

	virtual void			Event_PostSpawn(void);

private:

	void					WarningState();
	idEntity *				ProximityCheck();

	typedef enum
	{
		IDLE,
		WARNING,
		PURSUING,
		EXPLOSIONDELAY,
		EXPLODED
	} spacemine_state_t;
	spacemine_state_t		state;

	int						timer;

	int						warningCounter;
	int						warningTimer;

	

	void					DoExplodeDelay();
	void					Explode();

	idStr					damageDef;

	
	int						moveSpeed;

	const idDeclParticle *	trailParticles = nullptr;
	int						trailParticlesFlyTime;

	idEntityPtr<idEntity>	pursuitTarget;

	idBeam*					wireOrigin = nullptr;
	idBeam*					wireTarget = nullptr;
	idEntity				*harpoonModel = nullptr;
	bool					harpoonActive;
	void					RemoveHarpoon();

	idVec3					impulseTarget;
	bool					impulseActive;
	int						impulseTimer;
};
