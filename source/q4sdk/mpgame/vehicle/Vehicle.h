//----------------------------------------------------------------
// Vehicle.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLE_H__
#define __GAME_VEHICLE_H__

class rvVehiclePosition {
public:

	rvVehiclePosition( void );
	virtual ~rvVehiclePosition( void );
	
	usercmd_t				mInputCmd;
	idAngles				mInputAngles;
	usercmd_t				mOldInputCmd;
	idAngles				mOldInputAngles;	
	int						mOldInputFlags;
	
	int						mCurrentWeapon;

	idEntityPtr<idActor>	mDriver;
	idEntityPtr<rvVehicle>	mParent;

	rvVehiclePartList_t		mParts;
	rvVehiclePartList_t		mWeapons;	
	
	idVec3					mEyeOrigin;
	idMat3					mEyeAxis;

	jointHandle_t			mEyeJoint;
	idVec3					mEyeOffset;
	idMat3					mEyeJointTransform;
	idAngles				mDeltaEyeAxisScale;
	int						mEyeJointAxisMap[3];
	int						mEyeJointDirMap[3];

	idMat3					mAxisOffset;

	jointHandle_t			mDriverJoint;
	idVec3					mDriverOffset;
	idMat3					mDriverJointTransform;
	idAngles				mDeltaDriverAxisScale;
	int						mDriverJointAxisMap[3];
	int						mDriverJointDirMap[3];

	idVec3					mExitPosOffset;
	idMat3					mExitAxisOffset;

	idStr					mDriverAnim;

	idStr					mInternalSurface;

	struct positionFlags_s {
		bool		inputValid			:1;
		bool		engine				:1;
		bool		driverVisible		:1;
		bool		depthHack			:1;
		bool		internalView		:1;
		bool		bindDriver			:1;
		bool		stalled				:1;
	} fl;
		
	void					Init				( rvVehicle* parent, const idDict& args  );			

	int						AddPart				( const idTypeInfo &classdef, const idDict& args );
	rvVehiclePart*			GetPart				( int partIndex );
	
	bool					IsOccupied			( void ) const;
	bool					IsEngine			( void ) const;

	bool					SetDriver			( idActor* driver );
	void					SetInput			( const usercmd_t& cmd, const idAngles& newAngles );
	void					GetInput			( usercmd_t& cmd, idAngles& newAngles ) const;
	
	bool					EjectDriver			( bool force = false );
	
	void					RunPrePhysics		( void );
	void					RunPostPhysics		( void );

	void					SelectWeapon		( int weapon );

	void					UpdateHUD			( idUserInterface* gui );
	void					UpdateCursorGUI		( idUserInterface* gui );
	void					UpdateInternalView	( bool force = false );

	virtual idMat3			GetAxis				( void ) const;
	virtual idVec3			GetOrigin			( const idVec3& offset = vec3_zero ) const;

	const idVec3&			GetEyeOrigin		( void ) const;
	const idMat3&			GetEyeAxis			( void ) const;	
	rvVehicle*				GetParent			( void ) const;
	idActor*				GetDriver			( void ) const;

	void					Save				( idSaveGame* savefile ) const;
	void					Restore				( idRestoreGame* savefile );

	void					WriteToSnapshot		( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot	( const idBitMsgDelta &msg );

	void					GetEyePosition		( idVec3& origin, idMat3& axis ) const;
	void					GetDriverPosition	( idVec3& origin, idMat3& axis ) const;
	void					GetPosition			( const jointHandle_t jointHandle, const idVec3& offset, const idMat3& jointTransform, const idAngles& scale, const int axisMap[], const int dirMap[], idVec3& origin, idMat3& axis ) const;

	void					FireWeapon			( void );

	rvVehicleWeapon *		GetWeapon			( int weaponIndex );
	rvVehicleWeapon *		GetActiveWeapon		( void );

private:

	void					SetParts			( const idDict& args );
	void					ActivateParts		( bool active );
	
	int						mSoundPart;
	float					mSoundMaxSpeed;
};

typedef struct rvVehicleFuncs_s {
	rvScriptFuncUtility		enter;			// script to run when the vehicle becomes occupied
	rvScriptFuncUtility		exit;			// script to run when the vehicle becomes unoccupied
} rvVehicleFuncs_t;

class rvVehicle : public idActor {
public:

	CLASS_PROTOTYPE( rvVehicle );

							rvVehicle				( void );
							~rvVehicle				( void );

	void					Spawn					( void );
	void					Think					( void );
	bool					Give					( const char *statname, const char *value );

	virtual void			Hide					( void );
	virtual void			Show					( void );

	void					ClientPredictionThink	( void );
	void					WriteToSnapshot			( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot		( const idBitMsgDelta &msg );

	void					Save					( idSaveGame *savefile ) const;
	void					Restore					( idRestoreGame *savefile );

	virtual	void			Damage					( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			Killed					( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			AddDamageEffect			( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );
	
	virtual bool			GetPhysicsToVisualTransform ( idVec3 &origin, idMat3 &axis ) { return false; }

	virtual const idMat3&	GetAxis					( int id = 0 ) const;
	virtual const idVec3&	GetOrigin				( int id = 0 ) const;

	virtual int				AddDriver				( int position, idActor* driver );
	virtual bool			RemoveDriver			( int position, bool force = false );
	virtual void			EjectAllDrivers			( bool force = false );
	rvVehiclePosition*		GetPosition				( int index );
	const rvVehiclePosition* GetPosition			( int index ) const;
	int						GetPositionIndex		( const rvVehiclePosition* position ) const;

	int						GetNumPositions			( void ) const;
	int						GetNumDrivers			( void ) const;
	bool					HasOpenPositions		( void ) const;
	
	bool					Collide					( const trace_t &collision, const idVec3 &velocity );

	virtual void			UpdateState				();

	float					GetEngineOffTime		( void ) const;
	float					GetEngineOnTime			( void ) const;

	bool					IsFlipped				( void ) const;
	bool					IsFrozen				( void ) const;
	bool					IsLocked				( void ) const;
	bool					HasDrivers				( void ) const;
	
	bool					IsShootingEnabled		( void ) const;
	bool					IsMovementEnabled		( void ) const;

	void					Lock					( void );
	void					Unlock					( void );

	void					AutoRight				( idEntity* activator );

	virtual bool			FindClearExitPoint		( int pos, idVec3& origin, idMat3& axis ) const;
	void					GetDriverPosition		( int position, idVec3& origin, idMat3& axis );
	virtual void			GetEyePosition			( int position, idVec3& origin, idMat3& axis );
	void					DrawHUD					( int position );
	idUserInterface*		GetHud					( void );
	void					UpdateCursorGUI			( int position, idUserInterface* ui );
	virtual void			SetInput				( int position, const usercmd_t& cmd, const idAngles& newAngles );
	void					GetInput				( int position, usercmd_t& cmd, idAngles& newAngles ) const;
	
	void					IssueHazardWarning		( void );
	void					IssueLockedWarning		( void );
	
	void					AddToBounds				( const idVec3& vec );

	virtual void			UpdateHUD				( idActor* driver, idUserInterface* gui ) {}

	float					FocusLength				( void ) const { return spawnArgs.GetFloat("focusLength_enter", "60"); }
	
	bool					IsAutoCorrecting		( void ) const { return autoCorrectionBegin != 0; }
	bool					IsStalled				( void ) const { return vfl.stalled; }

	void					OnEnter					( void ) { funcs.enter.CallFunc( NULL ); fl.exitedVehicle=false; };
	void					OnExit					( void ) { if ( !fl.exitedVehicle ) { fl.exitedVehicle=true; funcs.exit.CallFunc( NULL );} };

	bool					IsStrafing				( void ) const { return vfl.strafe; }
//	void					PlayAnim				( int channel, const char *name, int blendFrames );

	virtual void			GuidedProjectileIncoming( idGuidedProjectile * projectile );
protected:

	void					SelectWeapon			( int weapon );

	void					UpdateDrivers			( int delta );
	virtual void			UpdateHUD				( int position, idUserInterface* gui );
	
	void					SetPositions			( void );
	
	void					SetCombatModel			( void );
	void					LinkCombat				( void );

	// twhitaker:
	friend class rvVehicleDriver;
	virtual void			RunPrePhysics			( void ) { }
	virtual void			RunPostPhysics			( void ) { }


	idList<rvVehiclePosition>	positions;
	int							drivers;

	idUserInterface *			hud;
	
	float						crashSpeedSmall;
	float						crashSpeedMedium;
	float						crashSpeedLarge;
	idStr						crashDamage;
	rvClientEffectPtr			crashEffect;
	int							crashNextSound;
	int							crashTime;
	
	float						autoRightDir;
	bool						autoRight;

	// twhitaker: for rvVehicleDriver, to avoid getting stuck.
	unsigned					autoCorrectionBegin;	// time when obstacle avoidance state began (0 if not correcting)

	struct vehicleFlags_s {
		bool		forward				:1;
		bool		backward			:1;
		bool		left				:1;
		bool		right				:1;
		bool		driver				:1;
		bool		locked				:1;
		bool		godmode				:1;
		bool		frozen				:1;
		bool		disableWeapons		:1;
		bool		disableMovement		:1;
		bool		scripted			:1;
		bool		endWithIdle			:1;
		bool		flipEject			:1;
		bool		dead				:1;
		bool		strafe				:1;
		bool		missileWarningOn	:1;
		bool		stalled				:1;
	} vfl;

	float						damageStaticChance;
	
	// Shields
	float						shieldMaxHealth;
	idInterpolate<int>			shieldHealth;
	int							shieldHitTime;
	float						shieldRegenTime;
	int							shieldRegenDelay;
	idClipModel*				shieldModel;

	int							healthRegenDelay;
	int							healthRegenRate;
	idInterpolate<int>			healthRegenAmount;

	int							hazardWarningTime;
	int							lockWarningTime;
	int							healthMax;
	int							healthLow;
	int							godModeDamage;

	int							cachedContents;
	
	rvVehicleFuncs_t			funcs;

	float						crashVelocitySmall;
	float						crashVelocityMedium;
	float						crashVelocityLarge;

	bool						alwaysImpactDamage;

	void					UpdateIncomingProjectiles( void );
	
public:
	idList< idEntityPtr< idGuidedProjectile > >	incomingProjectiles;
private:
	
	void					Event_Lock				( bool locked );
	void					Event_IsLocked			( void );
	void					Event_EnableWeapon		( void );
	void					Event_DisableWeapon		( void );
	void					Event_EnableMovement	( void );
	void					Event_DisableMovement	( void );
	void					Event_EnableClip		( void );
	void					Event_DisableClip		( void );
	void					Event_Activate			( idEntity* activator );
	void					Event_LaunchProjectiles	( const idList<idStr>* parms );
	void					Event_SetScript			( const char* scriptName, const char* funcName );
	void					Event_SetHealth			( float health );
	void					Event_HUDShockWarningOff( void );
	void					Event_StalledRestart	( float shield, float damage );
	void					Event_GetViewAngles		( void );

	virtual void			OnDeath					( void ) { }

	stateResult_t			State_Wait_Driver		( int blendFrames );

	CLASS_STATES_PROTOTYPE( rvVehicle );
};

/*
===============
rvVehiclePosition inlines
===============
*/

ID_INLINE bool				rvVehiclePosition::IsOccupied ( void ) const	{ return mDriver.IsValid();}
ID_INLINE bool				rvVehiclePosition::IsEngine ( void ) const		{ return fl.engine; }
ID_INLINE const idVec3& 	rvVehiclePosition::GetEyeOrigin ( void ) const	{ return mEyeOrigin; }
ID_INLINE const idMat3& 	rvVehiclePosition::GetEyeAxis ( void ) const	{ return mEyeAxis; }
ID_INLINE rvVehicle*		rvVehiclePosition::GetParent ( void ) const		{ return mParent; }
ID_INLINE idActor*			rvVehiclePosition::GetDriver ( void ) const		{ return mDriver; }
ID_INLINE rvVehiclePart*	rvVehiclePosition::GetPart ( int partIndex )	{ return mParts[partIndex]; }
ID_INLINE rvVehicleWeapon*	rvVehiclePosition::GetWeapon ( int weaponIndex ){ return ( weaponIndex >= 0 && weaponIndex < mWeapons.Num() ) ? static_cast<rvVehicleWeapon*>(mWeapons[weaponIndex]) : 0; }
ID_INLINE rvVehicleWeapon*	rvVehiclePosition::GetActiveWeapon ( void )		{ return GetWeapon( mCurrentWeapon ); }

/*
===============
rvVehicle inlines
===============
*/

ID_INLINE bool rvVehicle::IsFlipped( void ) const {
	return (( !vfl.flipEject ) ? false :
			idMath::Fabs( idMath::AngleNormalize180( renderEntity.axis.ToAngles().roll ) ) > 60.f ||
			idMath::Fabs( idMath::AngleNormalize180( renderEntity.axis.ToAngles().pitch ) > 60.0f ));
}

ID_INLINE bool rvVehicle::IsFrozen( void ) const {
	return vfl.frozen;
}

ID_INLINE idUserInterface* rvVehicle::GetHud ( void ) {
	return hud;
}

ID_INLINE bool rvVehicle::IsLocked ( void ) const {
	return vfl.locked || hazardWarningTime != 0;
}

ID_INLINE rvVehiclePosition* rvVehicle::GetPosition ( int index ) {
	return &positions[index];
}

ID_INLINE const rvVehiclePosition* rvVehicle::GetPosition ( int index ) const {
	return &positions[index];
}

ID_INLINE void rvVehicle::SetInput ( int position, const usercmd_t& cmd, const idAngles& newAngles ) {
	GetPosition(position)->SetInput ( cmd, newAngles );
}

ID_INLINE void rvVehicle::GetInput( int position, usercmd_t& cmd, idAngles& newAngles ) const {
	GetPosition(position)->GetInput( cmd, newAngles );
}

ID_INLINE bool rvVehicle::HasDrivers ( void ) const {
	return drivers > 0;
}

ID_INLINE void rvVehicle::AddToBounds ( const idVec3& vec ) {
	renderEntity.bounds.AddPoint ( (vec-renderEntity.origin) * GetPhysics()->GetAxis().Transpose() );
}

ID_INLINE bool rvVehicle::IsShootingEnabled ( void ) const {
	return !vfl.disableWeapons;
}

ID_INLINE bool rvVehicle::IsMovementEnabled ( void ) const {
	return !vfl.disableMovement;
}

ID_INLINE int rvVehicle::GetNumPositions ( void ) const {
	return positions.Num();
}

ID_INLINE int rvVehicle::GetNumDrivers ( void ) const {
	return drivers;
}

ID_INLINE bool rvVehicle::HasOpenPositions ( void ) const {
	return GetNumPositions() > GetNumDrivers();
}

#endif // __GAME_VEHICLE_H__
