
#ifndef __GAME_TALON_H__
#define __GAME_TALON_H__

#define MAX_TALON_PTS	16

typedef enum talontargetType_s {
	TTT_NORMAL,
	TTT_IMPORTANT,
	TTT_BENEFICIAL
} talonTargetType_t;

extern const idEventDef EV_TalonAction;

class hhTalonTarget;

//-----------------------------------------------------------------------------

class hhTalon : public hhMonsterAI {
public:
	CLASS_PROTOTYPE(hhTalon);

	void		Spawn(void);
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

				hhTalon();
				~hhTalon();
	void		SummonTalon(void);
	void		OwnerEnteredVehicle( void );
	void		OwnerExitedVehicle( void );

	void		SetOwner( hhPlayer *newOwner );
	hhPlayer	*GetOwner( void ) { return owner.GetEntity(); }
	void		FindTalonTarget( idEntity *skipEntity, hhTalonTarget *forceTarget, bool bForcePlayer );
	void		CalculateTalonTargetLocation();

	bool		CheckReachedTarget(float distance);
	void		CheckCollisions(float deltaTime);

	void		Portalled( idEntity *portal );
	void		Show();

	void		SetForcedTarget( bool force ) { bForcedTarget = force; }
	void		SetForcedPhysicsFactors( float newVel, float newRot, float newPerch ) { velocityFactor = newVel; rotationFactor = newRot; perchRotationFactor = newPerch; }
	bool		UpdateAnimationControllers();

	bool		FindEnemy( void );

	void		EnterTommyState();
	void		EnterFlyState();
	void		EnterVehicleState();
	void		ExitVehicleState(void);

	void		EnterPerchState();
	void		EnterAttackState();
	void		EndState();

	void		Event_LandAnim(void);
	void		Event_PreLandAnim(void);
	void		Event_IdleAnim(void);
	void		Event_TommyIdleAnim(void);
	void		Event_FlyAnim(void);
	void		Event_GlideAnim(void);
	void		Event_TakeOffAnim(void);
	void		Event_TakeOffAnimB(void);

	void		ReturnToTommy( void );

	virtual	int		HasAmmo( ammo_t type, int amount ) { return 0; }
	virtual	bool	UseAmmo( ammo_t type, int amount ) { return false ; }

	bool		IsAttacking( void ) { return state == StateAttack; }

protected:
	void		Think();
	void		FlyMove( void );
	void		AdjustFlyingAngles();
	bool		GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	void		CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );

	void		Damage( idEntity *inflictor, idEntity *attack, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	void		Event_PerchChatter(void);
	void		Event_PerchSquawk(void); // Perched on a spot and squawking

	void		Event_CheckForTarget(); // Checks for nearby targets to land upon
	void		Event_CheckForEnemy(); // Checks for nearby enemies

	void		PerchTicker(void);	// Perched on a spot
	void		FlyTicker(void);		// Flying to a perch spot or to Tommy
	void		TommyTicker(void);	// Perched on Tommy's shoulder
	void		AttackTicker(void);	// Attacking an enemy
	void		VehicleTicker(void);	// Owner is in a vehicle, so warp from talon point to talon point

	void		StartAttackFX();
	void		StopAttackFX();

protected:
	idEntityPtr<hhPlayer>	owner;
	idVec3			velocity;
	idVec3			acceleration;

	hhTalonTarget	*talonTarget;
	idVec3			talonTargetLoc;
	idMat3			talonTargetAxis;

	idVec3			lastCheckOrigin; // Used to check if the bird flew through a wall

	float			checkTraceTime; // Used to determine when to trace for world collisions
	float			checkFlyTime; // Used to timeout and find a new flying spot
	float			flyStraightTime; // Used to force Talon to fly straight for a short time

	bool			bLanding;
	bool			bReturnToTommy;	// Forces the bird to return to Tommy, no matter what

	bool			bForcedTarget; // True if talon is forced to stay at particular talon target

	bool			bClawingAtEnemy; // True if talon is currently playing his attack anim and clawing at creature's heads

	float			velocityFactor;
	float			rotationFactor;
	float			perchRotationFactor;

	// Anims
	int			flyAnim;
	int			glideAnim;
	int			prelandAnim;
	int			landAnim;
	int			idleAnim;
	int			tommyIdleAnim;
	int			squawkAnim;
	int			stepAnim;
	int			takeOffAnim;
	int			takeOffAnimB;
	int			attackAnim;
	int			preAttackAnim;

	const idDeclSkin		*openWingsSkin;
	const idDeclSkin		*closedWingsSkin;

	// Task variables
	idEntityPtr<idActor>	enemy; // Used for attacking

	idEntityPtr<idEntityFx>	trailFx;

	enum States {
		StateNone = 0,
		StateTommy,
		StateFly,
		StateVehicle,
		StatePerch,
		StateAttack
	} state;
	const idEventDef *stateEvent; // Ticker event for current state
};

//-----------------------------------------------------------------------------

class hhTalonTarget : public idEntity {
	friend class hhTalon;

public:
	CLASS_PROTOTYPE(hhTalonTarget);

	void			Spawn(void);
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
					~hhTalonTarget();

	virtual void	Reached( hhTalon *talon ); // Called when Talon reached this point
	virtual void	Left( hhTalon *talon ); // Called when Talon leaves this point

	static void		ShowTargets( void );
	static void		HideTargets( void );

	void			SetSquawk( bool newSquawk ) { bShouldSquawk = newSquawk; }

protected:
	void			Event_CallTalon( float vel, float rot, float perch );
	void			Event_SetPerchState( float newState );

	void			Event_ReleaseTalon();

	int				priority;	// priority value.  Zero is default, higher numbers is a higher priority
								// -1 skips the target spot completely
	bool			bNotInSpiritWalk; // Talon will skip this target if the player is spiritwalking

	bool			bShouldSquawk;	// if Talon should squawk on this spot or not
	bool			bValidForTalon; // if Talon is attacted to this spot or not
};

#endif /* __GAME_TALON_H__ */
