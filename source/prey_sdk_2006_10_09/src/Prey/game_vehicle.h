#ifndef __GAME_VEHICLE_H__
#define __GAME_VEHICLE_H__

#define CONTENTS_VEHICLE	(CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_MOVEABLECLIP)
#define CLIPMASK_VEHICLE	(CONTENTS_BODY|CONTENTS_VEHICLECLIP|CONTENTS_SOLID|CONTENTS_FORCEFIELD)

extern const idEventDef EV_Vehicle_FireCannon;
extern const idEventDef EV_VehicleExplode;

class idLight;
class hhVehicle;
class hhPlayer;
class hhDock;
class hhShuttleDock;

enum EFireMode{
	FIREMODE_NOTHING=0,
	FIREMODE_CANNON,
	FIREMODE_TRACTOR,
	FIREMODE_EXIT,
	NUM_FIREMODES
};

// Support for direction masks, as used for thrusters
enum {
	// The order of these is important and is laid out as:
	THRUSTER_FRONT=0,		// x < 0
	THRUSTER_BACK,			// x > 0
	THRUSTER_LEFT,			// y < 0
	THRUSTER_RIGHT,			// y > 0
	THRUSTER_TOP,			// z < 0
	THRUSTER_BOTTOM,		// z > 0
	THRUSTER_DIRECTIONS
};



//==========================================================================
//
//	hhVehicleThruster
//
//==========================================================================

class hhVehicleThruster : public idEntity {
	CLASS_PROTOTYPE( hhVehicleThruster );
public:
							hhVehicleThruster();
	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

	void					SetOwner(hhVehicle *v)		{	owner = v;		}
	void					SetSmoker(bool s, idVec3 &offset, idVec3 &dir);
	void					SetThruster(bool on);
	void					Update( const idVec3 &vel );
	void					SetDying(bool dying);
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

protected:
	void					Event_AssignFxSmoke( hhEntityFx* fx );

protected:
	//rww - changed to idEntityPtr's
	idEntityPtr<hhVehicle>	owner;
	idEntityPtr<idEntityFx>	fxSmoke;
	idVec3					localOffset;
	idVec3					localDirection;
	float					soundDistance;
	bool					bSomeThrusterActive;
	bool					bSoundMaster;
	idVec3					localVelocity;
};


//==========================================================================
//
//	hhPilotVehicleInterface
//
//
//==========================================================================
class hhPilotVehicleInterface : public idClass {
	CLASS_PROTOTYPE( hhPilotVehicleInterface );

	public:
							hhPilotVehicleInterface();
							~hhPilotVehicleInterface();
		void				Save( idSaveGame *savefile ) const;
		void				Restore( idRestoreGame *savefile );

		//Called from vehicle
		virtual void		RetrievePilotInput( usercmd_t& cmds, idAngles& viewAngles );
		virtual idActor*	GetPilot() const;
		virtual void		UnderScriptControl( bool yes ) { underScriptControl = yes; }
		virtual bool		UnderScriptControl() const { return underScriptControl; }
		virtual void		OrientTowards( const idVec3 &loc, float speed ) {}
		virtual void		ThrustTowards( const idVec3 &loc, float speed ) {}
		virtual void		Fire(bool on) {}
		virtual void		AltFire(bool on) {}
		virtual void		ClearBufferedCmds() {}

		//Called from pilot
        virtual bool		ControllingVehicle() const;
		virtual void		ReleaseControl();
		virtual hhVehicle*	GetVehicle() const;
		virtual void		TakeControl( hhVehicle* vehicle, idActor* pilot );
		virtual idVec3		DeterminePilotOrigin() const;
		virtual idMat3		DeterminePilotAxis() const;

		virtual bool		InvalidVehicleImpulse( int impulse );
		int GetVehicleSpawnId() const { return vehicle.GetSpawnId(); }
		bool SetVehicleSpawnId( int id ) { return vehicle.SetSpawnId( id ); }
	protected:
		idEntityPtr<idActor>	pilot;
		idEntityPtr<hhVehicle>	vehicle;
		bool					underScriptControl;
};

class hhAIVehicleInterface : public hhPilotVehicleInterface {
	CLASS_PROTOTYPE( hhAIVehicleInterface );

	public:
		hhAIVehicleInterface(void);

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		//Called from pilot
		void			BufferPilotCmds( const usercmd_t* cmds, const idAngles* viewAngles );
		virtual void	OrientTowards( const idVec3 &loc, float speed );
		virtual void	ThrustTowards( const idVec3 &loc, float speed );
		virtual bool	IsVehicleDocked() const;
		virtual void	TakeControl( hhVehicle* vehicle, idActor* pilot );
		virtual void	Fire(bool on);
		virtual void	AltFire(bool on);

		//Called from vehicle
		virtual void	RetrievePilotInput( usercmd_t& cmds, idAngles& viewAngles );

	protected:
		void			ClearBufferedCmds();

	protected:
		usercmd_t		bufferedCmds;
		idAngles		bufferedViewAngles;
		bool			stateFiring;
		bool			stateAltFiring;
		idVec3			stateOrientDestination;
		float			stateOrientSpeed;
		idVec3			stateThrustDestination;
		float			stateThrustSpeed;
};

class hhPlayerVehicleInterface : public hhPilotVehicleInterface {
	CLASS_PROTOTYPE( hhPlayerVehicleInterface );

	public:
						hhPlayerVehicleInterface();
		virtual			~hhPlayerVehicleInterface();
		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		//Called from vehicle
		virtual void	RetrievePilotInput( usercmd_t& cmds, idAngles& viewAngles );

		//Called from pilot
		virtual void	ReleaseControl();
		virtual void	TakeControl( hhVehicle* vehicle, idActor* pilot );
		void			StartHUDTranslation();	// HUMANHEAD mdl: Moved these into hhPlayer events to prevent loadgame crash
												// (events must be on entities, not just idClasses, in order to save properly)
		int GetHandSpawnId() const { return controlHand.GetSpawnId(); }
		bool SetHandSpawnId( int id ) { return controlHand.SetSpawnId( id ); }
		hhControlHand *GetHandEntity() const { return controlHand.GetEntity(); }
		const hhWeaponHandState *GetWeaponHandState() const { return &weaponHandState; }
		hhWeaponHandState *GetWeaponHandState() { return &weaponHandState; }

		//Called from playerView
		virtual idUserInterface*	GetHUD() const { return hud; }
		virtual void	DrawHUD( idUserInterface* _hud );

	protected:
		void			UpdateControlHand( const usercmd_t& cmds );
		void			CreateControlHand( hhPlayer* pilot, const char* handName );
		void			RemoveHand();

	protected:
		hhWeaponHandState	weaponHandState;
		idEntityPtr<hhControlHand> controlHand;

		idUserInterface*	hud;
		bool				uniqueHud; //rww
		idInterpolate<float>	translationAlpha;				// interpolator for hud translation
};

//==========================================================================
//
//	hhVehicle
//
//==========================================================================

class hhVehicle : public hhRenderEntity {
	ABSTRACT_PROTOTYPE( hhVehicle );

public:
							hhVehicle() : fireController(NULL), pilotInterface(NULL) {}
							~hhVehicle();
	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think();
	virtual void			ClientPredictionThink( void );
	virtual void			Present();
	enum {
		EVENT_EJECT_PILOT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );
	virtual const idMat3&	GetAxis( int id = 0 ) const { return modelAxis; }
	virtual void			SetAxis( const idMat3& axis );
	virtual void			Portalled(idEntity *portal);
	virtual idVec3			GetPortalPoint( void );
	virtual idVec3			DeterminePilotOrigin() const;
	virtual idMat3			DeterminePilotAxis() const;

	virtual void			FireThrusters( const idVec3& impulse ) {}
	virtual void			ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse );
	virtual void			ApplyImpulse( const idVec3& impulse );

	virtual void			SetDock( const hhDock* dock ) { this->dock = dock; }
	virtual void			Undock() { dock.Clear(); }
	virtual bool			IsDocked() const { return dock.IsValid(); }
	virtual idActor*		GetPilot() const { return (GetPilotInterface()) ? GetPilotInterface()->GetPilot() : NULL; }
	virtual hhPilotVehicleInterface* GetPilotInterface() const { return pilotInterface; }
	virtual void			SetPilotInterface(hhPilotVehicleInterface *pInterface) { pilotInterface = pInterface; }

	bool					IsNoClipping() const;
	bool					InGodMode() const;
	bool					InDialogMode() const;

	virtual float			GetThrustFactor() const { return thrustFactor; }

	void					GiveHealth( int amount );
	void					GivePower( int amount );
	bool					ConsumePower( int amount );
	bool					HasPower( int amount ) const;
	bool					NeedsPower() const { return currentPower < spawnArgs.GetFloat("maxPower");	}

	virtual bool			WillAcceptPilot( idActor *act ) = 0;
	virtual void			AcceptPilot( hhPilotVehicleInterface* pilotInterface );
	void					RestorePilot( hhPilotVehicleInterface* pilotInterface);
	virtual void			ReleaseControl() {}
	virtual void			EjectPilot();
	virtual bool			CanBecomeVehicle(idActor *pilot);
	virtual void			BecomeVehicle();
	virtual void			BecomeConsole();
	virtual bool			IsVehicle() const;
	virtual bool			IsConsole() const;

	virtual idVec3			GetFireOrigin()	{	return GetOrigin();	}	// Called by firecontroller
	virtual idMat3			GetFireAxis()	{	return GetAxis();	}

	virtual void			ResetGravity();

	virtual void			ProcessPilotInput( const usercmd_t* cmds, const idAngles* viewAngles );

	virtual float			GetWeaponRecoil() const { return (fireController) ? fireController->GetRecoil() : 0.0f; }
	virtual float			GetWeaponFireDelay() const { return (fireController) ? fireController->GetFireDelay() : 0.0f; }

	virtual void			DrawHUD( idUserInterface* _hud );

	virtual void			DoPlayerImpulse(int impulse); //rww - seperated into its own function because of networked impulse events

protected:
	virtual void			UpdateModel();
	virtual const idEventDef* GetAttackFunc( const char* funcName );
	virtual void			InitializeAttackFuncs();

	virtual void			InitPhysics();
	virtual void			SetConsolePhysics();
	virtual void			SetVehiclePhysics();

	virtual void			SetConsoleModel();
	virtual void			SetVehicleModel();

	void					ProcessButtons( const usercmd_t& cmds );
	void					ProcessImpulses( const usercmd_t& cmds );
	float					CmdScale( const usercmd_t* cmd ) const;
	void					SetThrustBooster( float minBooster, float maxBooster, float accelBooster );

	hhVehicleFireController *	CreateFireController() {	return new hhVehicleFireController;	}

	virtual void			CreateHeadLight();
	virtual void			FreeHeadLight();
	virtual void			Headlight( bool on );
	virtual void			CreateDomeLight();
	virtual void			FreeDomeLight();

	virtual void			RemoveVehicle();
	virtual void			PerformDeathAction( int deathAction, idActor *savedPilot, idEntity *attacker, idVec3 &dir );
	virtual void			Explode( idEntity *attacker, idVec3 dir );
	virtual void			Event_FireCannon();
	void					Event_Explode( idEntity *attacker, float dx, float dy, float dz );
	void					Event_GetIn( idEntity *ent );
	void					Event_GetOut();
	void					Event_ResetGravity();
	void					Event_Fire( bool start );
	void					Event_AltFire( bool start );
	void					Event_OrientTowards( idVec3 &point, float speed );
	void					Event_StopOrientingTowards();
	void					Event_ThrustTowards( idVec3 &point, float speed );
	void					Event_StopThrustingTowards();
	void					Event_ReleaseScriptControl();
	void					Event_EjectPilot();


protected:
	hhPhysics_Vehicle		physicsObj;				// physics object for vehicle
	idMat3					modelAxis;

	hhPilotVehicleInterface* pilotInterface;

	hhVehicleFireController*	fireController;
	usercmd_t				oldCmds;

	//FIXME: make these renderLight structs
	idEntityPtr<idLight>	headlight;				// Head light
	idEntityPtr<idLight>	domelight;				// Dome light
	bool					bHeadlightOn;
	
	int						currentPower;
	float					thrustFactor;
	float					thrustMin;
	float					thrustMax;
	float					thrustAccel;
	float					thrustScale;
	float					dockBoostFactor;

	idEntityPtr<hhDock>		dock;

	idEntityPtr<idEntity>	lastAttacker;
	int						lastAttackTime;

	int						vehicleClipMask;		// clipmask of vehicle when not noclipping
	int						vehicleContents;		// contents of vehicle when not noclipping
	
	bool					bDamageSelfOnCollision;	// Vehicle takes damage when colliding
	bool					bDamageOtherOnCollision;// Vehicle takes damage when colliding

	bool					bDisallowAttackUntilRelease;
	bool					bDisallowAltAttackUntilRelease;

	int						thrusterCost;
	int						validThrustTime;				// Delay thrust capability a couple seconds after entering vehicle

	const idEventDef*		attackFunc;
	const idEventDef*		finishedAttackingFunc;
	const idEventDef*		altAttackFunc;
	const idEventDef*		finishedAltAttackingFunc;

	bool					noDamage;
};

#endif

