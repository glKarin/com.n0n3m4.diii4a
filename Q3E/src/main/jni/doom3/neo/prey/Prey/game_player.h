
#ifndef __PREY_GAME_PLAYER_H__
#define __PREY_GAME_PLAYER_H__

extern const idEventDef EV_PlayWeaponAnim;
extern const idEventDef EV_Resurrect;
extern const idEventDef EV_PrepareToResurrect;
extern const idEventDef EV_SetOverlayMaterial;
extern const idEventDef EV_SetOverlayTime;
extern const idEventDef EV_SetOverlayColor;
extern const idEventDef EV_ShouldRemainAlignedToAxial;
extern const idEventDef EV_StartHudTranslation;
extern const idEventDef EV_StopSpiritWalk; //rww
extern const idEventDef EV_DamagePlayer; //rww

// HUMANHEAD IMPULSES SHOULD START AT 50
const int IMPULSE_50 = 50;				// Unused
const int IMPULSE_51 = 51;				// Unused
const int IMPULSE_52 = 52;				// Unused
const int IMPULSE_53 = 53;				// Unused
const int IMPULSE_54 = 54;				// Spirit power toggle

//Declared in idPlayer.cpp
extern const int RAGDOLL_DEATH_TIME;
extern const int LADDER_RUNG_DISTANCE;
extern const int HEALTH_PER_DOSE;
extern const int WEAPON_DROP_TIME;
extern const int WEAPON_SWITCH_DELAY;
extern const int SPECTATE_RAISE;
extern const int HEALTHPULSE_TIME;
extern const float MIN_BOB_SPEED;

// Forward declaration
class hhSpiritProxy;
class hhConsole;
class hhTalon;
class hhPossessedTommy;

#define MAX_HEALTH_NORMAL_MP		100 //rww - a probably temporary hack for trying out the pipe-as-armor concept

#define HH_WEAPON_WRENCH			1
#define HH_WEAPON_RIFLE				2
#define HH_WEAPON_CRAWLER			4
#define HH_WEAPON_AUTOCANNON		8
#define HH_WEAPON_HIDERWEAPON		16
#define HH_WEAPON_ROCKETLAUNCHER	32
#define HH_WEAPON_SOULSTRIPPER		64

// Structure to hold all info about the weapons to remove the dictionary lookups every tick
typedef struct weaponInfo_s {
	int			ammoType;
	int			ammoLow;
	int			ammoMax;
	int			ammoRequired;
} weaponInfo_t;

#define MAX_TRACKED_ATTACKERS		3
typedef struct attackInfo_s {
	idEntityPtr<idEntity>		attacker;
	int							time;
	bool						displayed;
} attackInfo_t;


class hhPlayer : public idPlayer {
	CLASS_PROTOTYPE(hhPlayer);

public:
	weaponInfo_t				weaponInfo[15];
	weaponInfo_t				altWeaponInfo[15];
	float						lighterTemperature;			// Temp of the lighter.  0 = cold, 1 = too hot to use
	renderLight_t				lighter;					// lighter
	int							lighterHandle;

	idEntityPtr<hhSpiritProxy>	spiritProxy;				// Proxy player when the player is spiritwalking
	int							lastWeaponSpirit;			// Last weapon the player held before entering spirit mode
	idEntityPtr<hhTalon>		talon;						// Talon, the spirit hawk
	int							nextTalonAttackCommentTime; // Time between when Tommy can make talon comments
	bool						bTalonAttackComment;		// If true, Tommy can randomly make comments when Talon is attacking
	bool						bSpiritWalk;				// True if the player is spirit walking
	bool						bDeathWalk;					// True if the player is dead and DeathWalking
	bool						bReallyDead;				// True if the player truly died (only when in deathwalk mode)
	idEntityPtr<hhConsole>		guiWantsControls;			// Gui console that controls should be routed to
	idEntityPtr<hhPossessedTommy>	possessedTommy;			// ptr to the possessed version of the player

	//rww - keep track of who attacked us in air, so if we die from unnatural causes other than a player,
	//the person who last attacked will be credited.
	idEntityPtr<idEntity>		airAttacker;
	int							airAttackerTime;

	float						deathWalkFlash;
	int							deathWalkTime;				// time the player entered deathwalk
	int							deathWalkPower;				// Power in deathwalk - when full, the player wins deathwalk
	bool						bInDeathwalkTransition;


	bool						bInCinematic;
	bool						lockView;

	bool						bPlayingLowHealthSound;		
	bool						bPossessed;					// True if the player can be possessed
	float						possessionTimer;			// Countdown timer until the player is fully possessed
	float						possessionFOV;				// FOV for possession

	int							preCinematicWeapon;			// selected weapon before cinematic started for restoration
	int							preCinematicWeaponFlags;	// flags to restore after cinematic completes

	int							lastDamagedTime;			// Used to determine when to recharge health

	void						ReportAttack(idEntity *attacker);
	attackInfo_t				lastAttackers[MAX_TRACKED_ATTACKERS];	// Used for tracking attackers for HUD

	//Make these idEntityPtr just to be safe
	hhWeaponHandState			weaponHandState;
	idEntityPtr<hhHand>			hand;
   	idEntityPtr<hhHand>			handNext;					// Next hand to pop up
	hhPlayerVehicleInterface	vehicleInterfaceLocal;

	// Death Variables
	idEntityPtr<idEntity>		deathLookAtEntity;			// nla - Entity to look at when dead
	idStr						deathLookAtBone;			// nla - Entity's bone to look at when dead
	idStr						deathCameraBone;

	hhCameraInterpolator		cameraInterpolator;
	idAngles					oldCmdAngles;				// HUMANHEAD: aob
	bool						bClampYaw;					// HUMANHEAD pdm
	float						maxRelativeYaw;				// HUMANHEAD pdm
	float						maxRelativePitch;			// HUMANHEAD rww

	hhSoundLeadInController		spiritwalkSoundController;
	hhSoundLeadInController		deathwalkSoundController;
	hhSoundLeadInController		wallwalkSoundController;

	bool						bShowProgressBar;			// Whether to display progress bar on HUDs
	float						progressBarValue;			// Current progress (set by script)
	idInterpolate<float>		progressBarGuiValue;		// Interpolates towards progressBarValue
	int							progressBarState;			// 0=none, 1=success, 2=failure

	bool						bScopeView;					// HUMANHEAD rww

	// HUMANHEAD bjk
	float						kickSpring;
	float						kickDamping;
	// HUMANHEAD END

#if GAMEPAD_SUPPORT	// VENOM BEGIN
//SAVETODO: save these
	int							lastAutoLevelTime;
	int							lastAccelTime;
	float						lastAccelFactor;
#endif // VENOM END

	float						bob;
	int							lastAppliedBobCycle;
	int							prevStepUpTime;
	idVec3						prevStepUpOrigin;

	//Crashland
	float						crashlandSpeed_fatal;
	float						crashlandSpeed_soft;
	float						crashlandSpeed_jump;

	// CJR: DDA variables
	int							ddaNumEnemies; // CJR
	float						ddaProbabilityAccum;

	int							forcePredictionButtons; //HUMANHEAD rww

	idScriptBool				AI_ASIDE;

	// Methods
						hhPlayer();
	void				Spawn( void );
	virtual				~hhPlayer();


	// Overridden Methods
	virtual void		RestorePersistantInfo( void );
	virtual void		SquishedByDoor(idEntity *door);
	virtual void		Init();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	virtual void		SpawnToPoint( const idVec3	&spawn_origin, const idAngles &spawn_angles );
	virtual	void		Think( void );
	virtual void		AdjustBodyAngles( void );
	virtual	void		Move( void );
	virtual void		Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );
	virtual void		Teleport( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& newUntransformedViewAngles, idEntity *destination );
	virtual void		TeleportNoKillBox( const idVec3& origin, const idMat3& bboxAxis ); // HUMANHEAD cjr: A teleport that doesn't telefrag
	virtual void		TeleportNoKillBox( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& newUntransformedViewAngles );
	virtual void		LinkScriptVariables( void );
	virtual void		Present();

	virtual void		UpdateFromPhysics( bool moveBack );
	virtual bool		UpdateAnimationControllers( void );

	virtual bool		Give( const char *statname, const char *value );
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual bool		ServerReceiveEvent( int event, int time, const idBitMsg &msg ); //rww - override idPlayer

	virtual bool		GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ); //rww

	virtual void		WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg );
	//rww - our own prediction function
	virtual void		ClientPredictionThink( void );

	bool				SetWeaponSpawnId( int id ) { return weapon.SetSpawnId( id ); }

	virtual void		NextBestWeapon( void );
	virtual	void		SelectWeapon( int num, bool force );
	virtual void		NextWeapon( void );
	virtual void		PrevWeapon( void );
	virtual void		UpdateWeapon( void );
	virtual	void		UpdateFocus( void );
	virtual	void		UpdateViewAngles( void );
	virtual void		UpdateDeltaViewAngles( const idAngles &angles );
	virtual idAngles	DetermineViewAngles( const usercmd_t& cmd, idAngles& cmdAngles );
	virtual bool		HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);
	virtual	void		GetViewPos( idVec3 &origin, idMat3 &axis );		//HUMANHEAD bjk

	virtual void		UpdateHud( idUserInterface *_hud );
	virtual void		UpdateHudWeapon(bool flashWeapon = true);
	virtual void		UpdateHudAmmo(idUserInterface *_hud);
	virtual void		UpdateHudStats( idUserInterface *_hud );
	virtual void		DrawHUD( idUserInterface *_hud );
	virtual void		DrawHUDVehicle( idUserInterface* _hud );//HUMANHEAD: aob
	virtual void		Weapon_Combat( void );
	virtual	void		Weapon_GUI( void );
	virtual	void		PerformImpulse(int impulse);
	virtual void		ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );

	virtual	int			HasAmmo( ammo_t type, int amount );
	virtual bool		UseAmmo( ammo_t type, int amount );

	virtual void		CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );
	virtual	void		BobCycle( const idVec3 &pushVelocity );
	virtual	void		OffsetThirdPersonView( float angle, float range, float height, bool clip );
	virtual void		CalculateRenderView( void );
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool		IsWallWalking( void ) const;	
	virtual bool		WasWallWalking( void ) const;
	virtual float		GetStepHeight() const;

	virtual int			GetWeaponNum( const char* weaponName ) const;
	virtual const char* GetWeaponName( int num ) const;

	virtual	idAngles	GunTurningOffset( void ) { return idPlayer::GunTurningOffset(); }
	virtual	idVec3		GunAcceleratingOffset( void ) { return idPlayer::GunAcceleratingOffset(); }

	virtual void		SetupWeaponEntity( void );

	virtual void		SetUntransformedViewAngles( const idAngles& newUntransformedViewAngles ) { untransformedViewAngles = newUntransformedViewAngles; }
	virtual void		SetUntransformedViewAxis( const idMat3& newUntransformedViewAxis ) { untransformedViewAxis = newUntransformedViewAxis; }
	virtual const idAngles&	GetUntransformedViewAngles() const;
	virtual const idMat3&	GetUntransformedViewAxis() const;
	virtual idAngles	GetViewAngles() const;
	virtual void		SetEyeAxis( const idMat3 &axis );
	virtual void		SetEyeHeight( float height );
	virtual float		EyeHeight( void ) const;
	virtual idVec3		EyeOffset( void ) const;	
	virtual idMat3		GetEyeAxis() const;
	virtual idAngles	GetEyeAxisAsAngles() const;
	virtual idQuat		GetEyeAxisAsQuat() const;
	void				ShouldRemainAlignedToAxial( bool remainAligned );
	void				OrientToGravity( bool bRotateToGravity );
	void				SetOrientation( const idVec3& origin, const idMat3& bboxAxis, const idVec3& lookDir, const idAngles& newUntransformedViewAngles );
	void				RestoreOrientation( const idVec3& origin, const idMat3& bboxAxis, const idVec3& lookDir, const idAngles& newUntransformedViewAngles );
	void				UpdateOrientation( const idAngles& newUntransformedViewAngles );
	virtual bool		CheckFOV( const idVec3 &pos );
	virtual bool		CheckYawFOV( const idVec3 &pos );
	virtual idVec3		GetEyePosition( void ) const;
	virtual idVec3		TransformToPlayerSpace( const idVec3& origin ) const;
	virtual idVec3		TransformToPlayerSpaceNotInterpolated( const idVec3& origin ) const; //rww
	virtual idMat3		TransformToPlayerSpace( const idMat3& axis ) const;
	virtual idAngles	TransformToPlayerSpace( const idAngles& angles ) const;
	virtual bool		ShouldTouchTrigger( idEntity* entity ) const;

	virtual void		EnterVehicle( hhVehicle* vehicle );
	virtual void		ExitVehicle( hhVehicle* vehicle );
	virtual void		ResetClipModel();
	hhPlayerVehicleInterface*	GetVehicleInterfaceLocal() { return &vehicleInterfaceLocal; }
	virtual void		BecameBound(hhBindController *b);		// Just attached to a bindController
	virtual void		BecameUnbound(hhBindController *b);		// Just detached from a bindController

	virtual void		Possess( idEntity* possessor ); // cjr
	virtual void		Unpossess(); // cjr
	virtual bool		CanBePossessed( void ); // cjr
	void				PossessKilled( void ); // cjr

	virtual idVec3		GetPortalPoint( void ); // cjr:  The entity will portal when this point crosses the portal plane.  Origin for most, eye location for players
	virtual void		Portalled( idEntity *portal );
	void				SetPortalColliding( bool b ) { bCollidingWithPortal = b; }
	bool				IsPortalColliding( void ) { return bCollidingWithPortal; }

	virtual void		UpdateModelTransform( void );//aob

	virtual void		PlayFootstepSound();//aob
	virtual void		PlayPainSound();//aob

	bool				IsCrouching() const { return physicsObj.IsCrouching(); }
	virtual void		ForceCrouching() { physicsObj.ForceCrouching(); }//aob
	virtual void		FillDebugVars( idDict *args, int page );
	virtual bool		ChangingWeapons() const { return idealWeapon != currentWeapon; }
	void				BufferLoggedViewAngles( const idAngles& newUntransformedViewAngles );
	virtual void		GetPilotInput( usercmd_t& pilotCmds, idAngles& pilotViewAngles );

	void				UpdateDDA(); // CJR DDA

	virtual void		UpdateLocation( void );

	// Unique Methods
	void				SetupWeaponInfo();
	void				DialogStart(bool bDisallowPlayerDeath, bool bVoiceDucking, bool bLowerWeapon);
	void				DialogStop();
	void				DoDeathDrop();
	void				GetLocationText( idStr &locationString );
	void				TrySpawnTalon();
	void				TalonAttackComment(); // cjr - comment on Talon attacking
	void				RemoveResources();
	int					GetCurrentWeapon( void ) const;
	void				SetCurrentWeapon( const int _currentWeapon );
	int					GetIdealWeapon( void ) const;
	void				InvalidateCurrentWeapon() { currentWeapon = -1; }
	virtual bool		ShouldRemainAlignedToAxial() const { return physicsObj.ShouldRemainAlignedToAxial(); }
	virtual idVec3		ApplyLandDeflect( const idVec3& pos, float scale );
	virtual idVec3		ApplyBobCycle( const idVec3& pos, const idVec3& velocity );
	virtual void		DetermineOwnerPosition( idVec3 &ownerOrigin, idMat3 &ownerAxis );
	void				StartSpiritWalk( const bool bThrust, bool force = false ); // mdl:  Added force for when the player is possessed
	void				StopSpiritWalk(bool forceAllowance = false); //rww - added forceAllowance
	void				ToggleSpiritWalk( void );

	int					GetDeathWalkPower( void ) { return deathWalkPower; } // cjr: used for determining winning deathwalk
	void				SetDeathWalkPower( int newPower ) { deathWalkPower = newPower; } // cjr: used for determining winning deathwalk

	void				ToggleLighter( void );
	void				LighterOn();
	void				LighterOff();
	bool				IsLighterOn() const; //HUMANHEAD PCF mdl 05/04/06 - Made const
	void				UpdateLighter();
	void				ThrowGrenade();
	void				Event_ReturnToWeapon();
	void				EnableEthereal( const char *proxyName, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );
	void				DisableEthereal( void );
	void				DisableSpiritWalk( int timeout ); // Number of seconds to disable ethereal mode
	void				GetResurrectionPoint( idVec3& origin, idMat3& axis, idVec3& viewDir, idAngles& angles, idMat3& eyeAxis, const idBounds& absBounds, const idVec3& defaultOrigin, const idMat3& defaultAxis, const idVec3& defaultViewDir, const idAngles& defaultAngles );
	void				UpdatePossession( void ); // cjr
	virtual void		Kill( bool delayRespawn, bool nodamage ); // aob
	void				DeathWalk( const idVec3& resurrectOrigin, const idMat3& resurrectAxis, const idMat3& resurrectViewAxis, const idAngles& resurrectAngles, const idMat3& resurrectEyeAxis ); // cjr: entering deathwalk mode
	idEntity *			GetDeathwalkEnergyDestination();
	void				DeathWraithEnergyArived(bool energyHealth);
	void				DeathWalkDamagedByWraith(idEntity *attacker, const char *damageType);
	void				DeathWalkSuccess();
	bool				DeathWalkStage2() { return bDeathWalkStage2; }
	void				ReallyKilled( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ); // cjr: truly dead (killed while in deathwalk mode)
	void				Resurrect( void );
	void				KilledDeathWraith( void ); // cjr:  called when the player kills a deathwraith (determines if the player should resurrect)
	virtual	bool		IsDead() const { return (!bDeathWalk && health <= 0); } // True if the player is really dead
	virtual bool		IsSpiritOrDeathwalking() const { return ( bSpiritWalk || bDeathWalk ); } // True if the player is spiritwalking or deathwalking
	virtual bool		IsSpiritWalking() const { return ( bSpiritWalk && !bDeathWalk ); } // True if the player is spiritwalking, but not deathwalking
	virtual bool		IsDeathWalking() const { return bDeathWalk; } // True if the player is deathwalking
	virtual bool		IsPossessed() const { return bPossessed; }
	void				SnapDownCurrentWeapon();
	void				SnapUpCurrentWeapon();
	void				SelectEtherealWeapon();
	void				PutawayEtherealWeapon();
	bool				SkipWeapon( int weaponNum ) const;
	hhWeapon*			SpawnWeapon( const char* name );
	virtual bool		InGUIMode( );	// nla
	void				CL_UpdateProgress(bool bBar, float value, int state);
	int					GetSpiritPower();
	void				SetSpiritPower(int amount);

	void				ResetZoomFov() { zoomFov.Init( gameLocal.GetTime(), 0, g_fov.GetFloat(), g_fov.GetFloat() ); }
	virtual float		CalcFov( bool honorZoom );	
	virtual void		MangleControls(usercmd_t *cmd);
	virtual	void		UpdateCrosshairs( void );

	void				SetOverlayGui(const char *guiName);

	void				InCinematic( bool cinematic ) { bInCinematic = cinematic; }
	bool				InCinematic() const { return bInCinematic; }

	const char			* GetGuiHandInfo();					// nla

	idInterpolate<float>& GetZoomFov() { return zoomFov; }

	void				SetViewBob(const idVec3 &v)			{viewBob = v;}
	int					GetLastResurrectTime(void)	const	{return lastResurrectTime;} // Time stamp indicating when the player last was resurrected
	virtual idVec3		GetAimPosition() const;	// jrm

	void				SetViewAnglesSensitivity( float factor );
	float				GetViewAnglesSensitivity() const;
	
	//? Is there a better way of doing this?
	// Force the weapon num... 
	virtual void		ForceWeapon( int weaponNum );
	virtual void		ForceWeapon( hhWeapon *newWeapon );
	void				Freeze( float unfreezeDelay = 0.0f );
	void				Unfreeze(void);
	bool				IsFrozen() { return bFrozen; };

	void				LockWeapon( int weaponNum );
	void				UnlockWeapon( int weaponNum );
	bool				IsLocked( int weaponNum );
	hhSpiritProxy*		GetSpiritProxy() { return spiritProxy.IsValid() ? spiritProxy.GetEntity() : NULL; }

	//rww - instead of doing this through the death proxy, this is safer (being an af, the death proxy could be oriented strangely or removed in numerous ways)
	virtual void		RestorePlayerLocationFromDeathwalk( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& angles );

	virtual void		ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	int					mpHitFeedbackTime; //HUMANHEAD rww

	virtual void		Show();

protected:
	idUserInterface *	guiOverlay;
	idClipModel			thirdPersonCameraClipBounds;
	float				viewAnglesSensitivity;
	int					lastResurrectTime;
	int					spiritDrainHeartbeatMS;
	int					ddaHeartbeatMS;
	int					spiritWalkToggleTime;
	bool				bDeathWalkStage2;
	bool				bFrozen;
	int					weaponFlags; // Flags to limit weapons on specific level -mdl
	int					nextSpiritTime; // Disable ethereal mode unless gameLocal.time is greater than this
	idInterpolateAccelDecelSine<float>	cinematicFOV; // HUMANHEAD rdr:  Interpolated FOV for scripter use
	bool				bAllowSpirit;

	bool				bCollidingWithPortal; // CJR
	bool				bLotaTunnelMode;		// Used to disable hud during LOTA tunnel transitions

	// HUMANHEAD: aob
	idAngles			untransformedViewAngles;
	idMat3				untransformedViewAxis;
	// HUMANHEAD END

	//HUMANHEAD rww
	idVec3				deathwalkLastOrigin;
	idMat3				deathwalkLastBBoxAxis;
	idMat3				deathwalkLastViewAxis;
	idAngles			deathwalkLastViewAngles;
	idMat3				deathwalkLastEyeAxis;
	bool				deathwalkLastCrouching;
	//HUMANHEAD END

	// Overridden Methods
	virtual int			GetMaxHealth()	{	return inventory.maxHealth;	}

	// Unique Methods
	virtual void		FireWeapon( void ); 
	virtual void		FireWeaponAlt( void ); 

	void				StopFiring( void );

	void				SetupWeaponFlags( void );

	void				Event_LotaTunnelMode(bool on);
	void				Event_PlayWeaponAnim( const char* animName, int numTries = 0 );
	void				Event_DrainSpiritPower();	// HUMANHEAD pdm

	void				Event_RechargeHealth( void ); // cjr
	void				Event_RechargeRifleAmmo( void ); // cjr

	void				Event_SpawnDeathWraith(); // cjr
	void				Event_AdjustSpiritPowerDeathWalk();
	void				Event_PrepareForDeathWorld();
	void				Event_EnterDeathWorld(); // cjr: set-up for teleporting the player to the deathworld
	void				Event_PrepareToResurrect();
	void				Event_ResurrectScreenFade();
	void				Event_Resurrect();
	virtual void		Event_GetSpiritPower(); //rww
	virtual void		Event_SetSpiritPower(const float s); //rww
	void				Event_OnGround(); // bg

	void				Event_Cinematic( int on, int lockView ); // pdm
	void				Event_DialogStart( int bDisallowPlayerDeath, int bVoiceDucking, int bLowerWeapon );
	void				Event_DialogStop();
	void				Event_ShouldRemainAlignedToAxial( bool remainAligned );
	void				Event_OrientToGravity( bool orient );
	virtual void		Event_ResetGravity();
	void				Event_SetOverlayMaterial( const char *mtrName, const int requiresScratch );
	void				Event_SetOverlayTime( const float newTime, const int requiresScratch );
	void				Event_SetOverlayColor( const float r, const float g, const float b, const float a );

	void				Event_DDAHeartBeat();
	void				Event_StartHUDTranslation();
	void				Event_Unfreeze();
	void				Event_LockWeapon( int weaponNum );
	void				Event_UnlockWeapon( int weaponNum );

	void				Event_SetPrivateCameraView( idEntity *camView, int noHide ); //HUMANHEAD rdr
	void				Event_SetCinematicFOV( float fieldOfView, float accelTime, float decelTime, float duration ); //HUMANHEAD rdr
	void				Event_StopSpiritWalk(); //rww
	void				Event_DamagePlayer(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, char *damageDefName, float damageScale, int location); //rww
	void				Event_GetSpiritProxy();
	void				Event_IsSpiritWalking();
	void				Event_IsDeathWalking();	//bjk
	void				Event_GetDDAValue(); // cjr
	void				Event_AllowLighter( bool allow );
	void				Event_DisableSpirit();
	void				Event_EnableSpirit();
	void				Event_CanAnimateTorso(void); //HUMANHEAD rww

	void				Event_UpdateDDA(); // cjr

	void				Event_AllowDamage(); // mdl
	void				Event_IgnoreDamage(); // mdl

	void				Event_RespawnCleanup(void); //HUMANHEAD rww
};

/*
=====================
hhPlayer::SetEyeAxis
=====================
*/		
ID_INLINE void hhPlayer::SetEyeAxis( const idMat3 &axis ) {
	if (!cameraInterpolator.GetIdealAxis().Compare(axis)) {
		cameraInterpolator.SetTargetAxis( axis, INTERPOLATE_EYEOFFSET );
	}
}

/*
=====================
hhPlayer::SetEyeHeight
=====================
*/		
ID_INLINE void hhPlayer::SetEyeHeight( float height ) {
	idPlayer::SetEyeHeight( height );

	cameraInterpolator.SetTargetEyeOffset( height, INTERPOLATE_EYEOFFSET );
}

/*
=====================
hhPlayer::EyeHeight
=====================
*/
ID_INLINE float hhPlayer::EyeHeight( void ) const {
	return cameraInterpolator.GetCurrentEyeHeight();
}

/*
=====================
hhPlayer::EyeOffset
=====================
*/
ID_INLINE idVec3 hhPlayer::EyeOffset( void ) const {//HUMANHEAD
	return cameraInterpolator.GetCurrentEyeOffset();
}

/*
=====================
hhPlayer::ShouldRemainAlignedToAxial
	HUMANHEAD
=====================
*/
ID_INLINE void hhPlayer::ShouldRemainAlignedToAxial( bool remainAligned ) {//HUMANHEAD
	physicsObj.ShouldRemainAlignedToAxial( remainAligned );
}

/*
=====================
hhPlayer::OrientToGravity
	HUMANHEAD
=====================
*/
ID_INLINE void hhPlayer::OrientToGravity( bool orientToGravity ) {//HUMANHEAD
	physicsObj.OrientToGravity( orientToGravity );
}

/*
=====================
hhPlayer::EyePosition
	HUMANHEAD
=====================
*/
ID_INLINE idVec3 hhPlayer::GetEyePosition( void ) const {//HUMANHEAD
	return cameraInterpolator.GetEyePosition();
}

/*
=====================
hhPlayer::GetEyeAxis
	HUMANHEAD
=====================
*/
ID_INLINE idMat3 hhPlayer::GetEyeAxis() const {
	return cameraInterpolator.GetCurrentAxis();
}

/*
=====================
hhPlayer::GetEyeAxisAsAngles
	HUMANHEAD
=====================
*/
ID_INLINE idAngles hhPlayer::GetEyeAxisAsAngles() const {
	return cameraInterpolator.GetCurrentAngles();
}

/*
=====================
hhPlayer::GetEyeAxisAsQuat
	HUMANHEAD
=====================
*/
ID_INLINE idQuat hhPlayer::GetEyeAxisAsQuat() const {
	return cameraInterpolator.GetCurrentRotation();
}

/*
=====================
hhPlayer::GetStepHeight
	HUMANHEAD
=====================
*/
ID_INLINE float hhPlayer::GetStepHeight() const {
	if( IsWallWalking() ) {
		return pm_wallwalkstepsize.GetFloat();
	}

	return pm_stepsize.GetFloat();
}

/*
=====================
hhPlayer::TransformToPlayerSpace
	HUMANHEAD
=====================
*/
ID_INLINE idVec3 hhPlayer::TransformToPlayerSpace( const idVec3& origin ) const {
	return cameraInterpolator.GetCurrentPosition() + origin * GetEyeAxis();
}

/*
=====================
hhPlayer::TransformToPlayerSpaceNotInterpolated
	HUMANHEAD rww
=====================
*/
ID_INLINE idVec3 hhPlayer::TransformToPlayerSpaceNotInterpolated( const idVec3& origin ) const {
	return cameraInterpolator.GetIdealPosition() + origin * cameraInterpolator.GetIdealAxis();
}

/*
=====================
hhPlayer::TransformToPlayerSpace
	HUMANHEAD
=====================
*/
ID_INLINE idMat3 hhPlayer::TransformToPlayerSpace( const idMat3& axis ) const {
	return axis * GetEyeAxis();
}

/*
=====================
hhPlayer::TransformToPlayerSpace
	HUMANHEAD
=====================
*/
ID_INLINE idAngles hhPlayer::TransformToPlayerSpace( const idAngles& angles ) const {
	return (angles.ToMat3() * GetEyeAxis()).ToAngles();
}

/*
=====================
hhPlayer::GetCurrentWeapon
	HUMANHEAD
=====================
*/
ID_INLINE int hhPlayer::GetCurrentWeapon( void ) const {
	return currentWeapon;
}

/*
=====================
hhPlayer::SetCurrentWeapon
	HUMANHEAD
=====================
*/
ID_INLINE void hhPlayer::SetCurrentWeapon( const int _currentWeapon ) {
	if( _currentWeapon >= 0 && _currentWeapon < MAX_WEAPONS ) {
		currentWeapon = _currentWeapon;
	}
}

/*
=====================
hhPlayer::GetIdealWeapon
	HUMANHEAD
=====================
*/
ID_INLINE int hhPlayer::GetIdealWeapon( void ) const {
	return idealWeapon;
}

/*
=====================
hhPlayer::GetUntransformedViewAngles
	HUMANHEAD
=====================
*/
ID_INLINE const idAngles& hhPlayer::GetUntransformedViewAngles() const {
	return untransformedViewAngles;
}

/*
=====================
hhPlayer::GetUntransformedViewAxis
	HUMANHEAD
=====================
*/
ID_INLINE const idMat3& hhPlayer::GetUntransformedViewAxis() const {
	return untransformedViewAxis;
}

/*
=====================
hhPlayer::GetViewAngles
	HUMANHEAD
=====================
*/
ID_INLINE idAngles hhPlayer::GetViewAngles() const {
	return viewAngles;
}

/*
=====================
hhPlayer::GetPortalPoint
	HUMANHEAD
=====================
*/
ID_INLINE idVec3 hhPlayer::GetPortalPoint( void ) {
	return idEntity::GetPortalPoint();
/*
	idVec3 origin;
	idMat3 axis;

	GetViewPos( origin, axis );
	return origin; // Compensate for the near-clip plane
*/
}

//HUMANHEAD rww
class hhArtificialPlayer : public hhPlayer {
	CLASS_PROTOTYPE(hhArtificialPlayer);
public:
						hhArtificialPlayer(void);

	void				Spawn( void );
	virtual	void		Think( void );

	virtual void		ClientPredictionThink( void );
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	usercmd_t			lastAICmd;

	bool				testCrouchActive;
	int					testCrouchTime;
};
//HUMANHEAD END
#endif /* __PREY_GAME_PLAYER_H__ */
