#include "physics/Physics_RigidBody.h"
#include "script/Script_Thread.h"
#include "gamesys/Event.h"
#include "Entity.h"
#include "Player.h"
#include "Projectile.h"
#include "Misc.h"
#include "Target.h"
#include "Moveable.h"

class idFlyingBarrel : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idFlyingBarrel);

							idFlyingBarrel( void );
	virtual					~idFlyingBarrel( void );

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );

	
	virtual void			AddDamageEffect(const trace_t &collision, const idVec3 &velocity, const char *damageDefName);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);

private:

	typedef enum
	{
		NORMAL = 0,
		PREDEATH,
		DEATH
	} explode_state_t;

	explode_state_t			state;

	//enum					{ IDLE, PRESSED };
	//int						state;

	int						fireSpewIntervalTimer;
	bool					isSpewingFire;
	int						fireSpewTimer;
	idVec3					fireSpewDirection;

	idEntity *				jetNode = nullptr;

	void					Event_PreDeath();

	idLight *				barrelLight = nullptr;

	void					Explode(idEntity *attacker);
};
