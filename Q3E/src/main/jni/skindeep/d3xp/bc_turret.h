
#include "Misc.h"
#include "Entity.h"

class idTurret : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idTurret );

							idTurret(void);
	virtual					~idTurret(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );

	virtual void			Think( void );
	virtual bool			Pain(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	bool					IsOn();

	void					Event_activate(int value);

	bool					IsInCombat();

	void					SetElectricalActive(bool value);

	virtual void			DoHack(); //for the hackgrenade.

	virtual void			DoRepairTick(int amount);

private:

	int						state;
	enum					{ TURRET_OFF, TURRET_OPENING, TURRET_IDLE, TURRET_TARGETING, TURRET_WARMUP, TURRET_VOLLEYING, TURRET_DAMAGESTATE, TURRET_SEARCH, TURRET_DEAD };
	

	int						nextStateTime;
	int						volleyCount;
	int						nextIdleSound;

	
	
	void					Event_isactive();

	void					MuzzleflashOff();

	idMat3					bodyAxis;
	idMat3					turretAxis;
	idVec3					turretSpawnDir;

	idBeam*					beamStart = nullptr;
	idBeam*					beamEnd = nullptr;

	idEntity*				laserdot = nullptr;

	idVec3					laserAimPos;
	int						laserAimtimer;
	int						laserAimMaxtime;
	idVec3					laserAimPosStart, laserAimPosEnd;
	void					MoveLaserAim(idVec3 newPos, int movetime);
	bool					laserAimIsMoving;



	idEntityPtr<idEntity>	targetEnt;		// entity being dragged
	int						lastTargetNum;


	bool					CheckTargetLOS(idEntity *ent, idVec3 offset);
	
	int						nextFindTime;

	bool					FindTargetUpdate();
	idEntity*				FindTarget();
	idVec3					FindTargetPos(idEntity *targetEnt);
	int						IsTargetValid(idEntity *ent);
	enum					{TARG_INVALIDTARGET, TARG_ACTORTARGET, TARG_OBJECTTARGET};


	bool					AimToPositionUpdate(idVec3 newPos, float timeOut = -1);  // return true when position is fully targeted or timeout reached
	idVec3					ConstrainToTurretDir(idVec3 dir, float maxYaw, float maxPitch);

	idVec3					lastTargetPos;

	idDict					brassDict;

	float					targetYaw;
	float					bodyYaw;

	void					UpdateRotation(float _targetYaw);

	void					UpdateLaserCollision();

	void					RotateTowardTarget();
	void					RotateTowardTarget(idVec3 targetPos);

	idVec3					GetEntityCenter(idEntity *ent);
	idVec3					GetMuzzlePos();

	int						idleSwayTimer;

	int						lastTeam;

	void					UpdatePitch();

	int						idleVOTimer;

	void					DoSoundParticle();

	void					ActivateLights();
	void					SetLight(bool value);
	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	bool					electricalActive;

	idLight *				spotlight = nullptr;

	bool					playingRotationSound;

	idVec3					GetDefaultLaserAimAngle(idVec3 facingAngle);

	idVec3					GetFacingAngle();

	idEntityPtr<idEntity>	inflictorEnt;
	idEntityPtr<idEntity>	searchEnt;

	void					SetInspectable(bool value);

};
