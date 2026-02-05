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

class idAsteroid : public idMoveable
{
public:
	CLASS_PROTOTYPE(idAsteroid);

							idAsteroid(void);
	virtual					~idAsteroid(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	void					Event_Touch(idEntity *other, trace_t *trace);

private:

	typedef enum
	{
		SPAWNING,
		MOVING,
		DESPAWNING,
		DORMANT
	} asteroid_state_t;

	asteroid_state_t		state;

	int						timer;

	void					Shatter();
	void					BeginDespawn();

	void					InflictDamage(idEntity *other);
	idStr					damageDef;

	int						angularSpeed;
	int						moveSpeed;

	const idDeclParticle *	trailParticles = nullptr;
	int						trailParticlesFlyTime;

	int						despawnThreshold; // if an asteroid's coordinates pass this value on the Y-axis, it despawns
};
