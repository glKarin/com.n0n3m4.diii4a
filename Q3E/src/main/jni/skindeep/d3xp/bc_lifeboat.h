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

class idLifeboat : public idMoveable
{
public:
	CLASS_PROTOTYPE(idLifeboat);

							idLifeboat(void);
	virtual					~idLifeboat(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	void					Launch(idVec3 _targetPos);
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);

	void					Event_Touch(idEntity *other, trace_t *trace);
	
	virtual void			DoGenericImpulse(int index);

	void					StartTakeoff();

	void					SpawnGoodie(const char *defName);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	// SW 10th March 2025
	void					EjectItems(idList<idMoveableItem*> items);

protected:

	virtual void			Think_Landed(void);
	virtual void			OnLanded(void);
	virtual void			OnTakeoff(void);


protected:

	void				Event_LaunchPod(const idVec3 &originPos, const idVec3 &destinationPos);

	typedef enum
	{
		INITIALIZING,
		THRUSTING,
		LANDED,
		EXITWARMUP,
		EXITING,
		DORMANT,
		CATPOD_EQUIPPING
	} lifeboat_state_t;

	lifeboat_state_t		state;

	idVec3					targetDirection;
	idVec3					despawnPosition;

	int						thrustTimer;

	int						stateTimer;
	int						lastSecondDisplay;

	idFuncEmitter			*idleSmoke = nullptr;

	idAnimated*				animatedThrusters = nullptr;
	
	void					Despawn();

	bool					damageSmokeDone;

	int						damageTimer;

	renderLight_t			boatlight;
	qhandle_t				boatlightHandle;

	//int						lastDamageFXTime;

	int						storingCount; //how many skulls am I storing

	void					UpdateBodyPull();
	int						bodypullTimer;

	idBeam*					tractorbeam = nullptr;
	idBeam*					tractorbeamTarget = nullptr;
	idEntityPtr<idEntity>	tractorPtr;

	idMat3					displayAngle;

	idVec3					FindGoodieSpawnposition();

	bool					hasTractorBeam;

	idEntity *				shopMonitor = nullptr;
	void					SetupShopMonitor();
	void					SpawnShopMonitor(idVec3 pos);
	void					UpdateShopMonitor(int secondsRemaining);
	int						shopState;
	enum					{SHOP_UNSPAWNED, SHOP_IDLE};

	int						lifeboatStayTime;
};
