//----------------------------------------------------------------
// VehicleParts.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLEPARTS_H__
#define __GAME_VEHICLEPARTS_H__

class idWeapon;
class rvVehicle;
class rvVehiclePosition;
class rvEffect;

//----------------------------------------------------------------
//							Base Part
//----------------------------------------------------------------

class rvVehiclePart : public idClass
{
public:

	ABSTRACT_PROTOTYPE( rvVehiclePart );

	virtual ~rvVehiclePart( void ) { }

	void						Spawn				( void );
	void						Save				( idSaveGame* saveFile ) const;
	void						Restore				( idRestoreGame* saveFile );

	virtual bool				Init				( rvVehiclePosition* position, const idDict& args, s_channelType soundChannel );

	virtual void				RunPrePhysics		( void ) { }
	virtual void				RunPhysics			( void ) { }
	virtual void				RunPostPhysics		( void ) { }

	bool						IsLeft				( void ) const;
	bool						IsFront				( void ) const;
	bool						IsUsingCenterMass	( void ) const;		
	bool						IsActive			( void ) const;
	
	virtual void				Activate			( bool activate ) { fl.active = activate; }

	virtual void				Impulse				( int impulse ) { }
	
	virtual rvVehiclePosition*	GetPosition			( void ) const;
	const idVec3&				GetOrigin			( void ) const;
	const idMat3&				GetAxis				( void ) const;

protected:

	void						UpdateOrigin		( void );

	typedef struct partFlags_s {
		bool				active			: 1;	// Is part active
		bool				left			: 1;	// Is part on left side of vehicle
		bool				front			: 1;	// Is part on right side of vehicle
		bool				useCenterMass	: 1;	// Use center of mass for origin
		bool				useViewAxis		: 1;	// Uses parent->viewAxis instead of the physics axis (::UpdateOrigin())
	} partFlags_t;

	partFlags_t				fl;

	idDict					spawnArgs;
	idEntityPtr<rvVehicle>	parent;
	jointHandle_t			joint;
	s_channelType			soundChannel;
	rvVehiclePosition*		position;

	idVec3					worldOrigin;
	idMat3					worldAxis;
	idVec3					localOffset;
};

ID_INLINE rvVehiclePosition* rvVehiclePart::GetPosition ( void ) const	{ return position; }
ID_INLINE const idVec3&		 rvVehiclePart::GetOrigin ( void ) const	{ return worldOrigin; }
ID_INLINE const idMat3&		 rvVehiclePart::GetAxis ( void ) const		{ return worldAxis; }

ID_INLINE bool				 rvVehiclePart::IsLeft ( void ) const				{ return fl.left; }
ID_INLINE bool				 rvVehiclePart::IsFront ( void ) const				{ return fl.front; }
ID_INLINE bool				 rvVehiclePart::IsUsingCenterMass ( void ) const	{ return fl.useCenterMass; }
ID_INLINE bool				 rvVehiclePart::IsActive ( void ) const				{ return fl.active; }

//----------------------------------------------------------------
//							Sound
//----------------------------------------------------------------

class rvVehicleSound : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( rvVehicleSound );

	rvVehicleSound ( void );
	~rvVehicleSound ( void );
	
	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPostPhysics		( void );
	virtual void	Activate			( bool active );
	
	bool			IsPlaying			( void ) const;

	void			Play				( void );
	void			Stop				( void );
	void			Update				( bool force = false );

	void			Fade				( int time, float toVolume, float toFreq );
	void			Attenuate			( float volumeAttenuate, float freqAttenuate );
	
	void			SetAutoActivate		( bool activate );
	
protected:

	idVec2					volume;
	idVec2					freqShift;
	idStr					soundName;
	refSound_t				refSound;
	idInterpolate<float>	currentVolume;
	idInterpolate<float>	currentFreqShift;
	bool					fade;
	bool					autoActivate;
};

ID_INLINE bool rvVehicleSound::IsPlaying ( void ) const {
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if( emitter ) {
		return ( emitter->CurrentlyPlaying ( ) );
	}
	return( false );
}

ID_INLINE void rvVehicleSound::SetAutoActivate ( bool activate ){
	autoActivate = activate;
}

//----------------------------------------------------------------
//							Hoverpad
//----------------------------------------------------------------

class rvVehicleHoverpad : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( rvVehicleHoverpad );

	rvVehicleHoverpad ( void );
	~rvVehicleHoverpad ( void );
	
	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPrePhysics		( void );
	virtual void	RunPhysics			( void );
	virtual void	Activate			( bool active ); 
		
protected:

	void			UpdateDustEffect	( const idVec3& origin, const idMat3& axis, float attenuation, const rvDeclMatType* mtype );

	float					height;
	float					dampen;
	bool					atRest;
		
	idClipModel*			clipModel;
	
	idInterpolate<float>	currentForce;
	float					force;
	const idDeclTable*		forceTable;
	float					forceRandom;

	float					fadeTime;
	idVec3					velocity;
	
	float					thrustForward;
	float					thrustLeft;
	
	float					maxRestAngle;
	
	int						soundPart;
	
	int						forceUpTime;
	int						forceDownTime;

	// Dust effects
	rvClientEntityPtr<rvClientEffect>	effectDust;
	const rvDeclMatType*				effectDustMaterialType;	
};

//----------------------------------------------------------------
//							Thruster
//----------------------------------------------------------------

class rvVehicleThruster : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( rvVehicleThruster );

	rvVehicleThruster ( void );
	~rvVehicleThruster ( void );
	
	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPhysics			( void );
		
protected:

	enum EThrusterKey
	{
		KEY_FORWARD,
		KEY_RIGHT,
		KEY_UP
	};

	float			force;	
	int				forceAxis;
	EThrusterKey	key;	
};

//----------------------------------------------------------------
//							Light
//----------------------------------------------------------------

class rvVehicleLight : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( rvVehicleLight );

	rvVehicleLight ( void );
	~rvVehicleLight ( void );
	
	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPostPhysics		( void );
	virtual void	Activate			( bool active );

	virtual void	Impulse				( int impulse );
	
protected:

	void			TurnOff				( void );
	void			TurnOn				( void );
	
	void			UpdateLightDef		( void );
	
	renderLight_t	renderLight;
	int				lightHandle;	
	bool			lightOn;

	idStr			soundOn;
	idStr			soundOff;		
};

//----------------------------------------------------------------
//							Weapon
//----------------------------------------------------------------
class rvVehicleWeapon : public rvVehiclePart {
public:
	CLASS_PROTOTYPE( rvVehicleWeapon );

	rvVehicleWeapon ( void );
	~rvVehicleWeapon ( void );
	
	void					Spawn				( void );
	void					Save				( idSaveGame* saveFile ) const;
	void					Restore				( idRestoreGame* saveFile );

	virtual void			RunPostPhysics		( void );
	virtual void			Activate			( bool activate );
	
	int						GetCurrentAmmo		( void ) const;
	float					GetCurrentCharge	( void ) const;

	void					UpdateCursorGUI		( idUserInterface* gui ) const;

	bool					Fire				();

	int						GetZoomFov			( void ) const;
	idUserInterface *		GetZoomGui			( void ) const;
	float					GetZoomTime			( void ) const;
	bool					CanZoom				( void ) const;

	void					StopTargetEffect	( void );

protected:
#ifdef _XENON
	idActor*				bestEnemy;
	void					AutoAim( idPlayer* player, const idVec3& origin, idVec3& dir );
#endif
	void					LaunchHitScan		( const idVec3& origin, const idVec3& dir, const idVec3& jointOrigin );
	void					LaunchProjectile	( const idVec3& origin, const idVec3& dir, const idVec3& pushVelocity );
	void					LaunchProjectiles	();

	void					UpdateLock			( void );
	void					GetLockInfo			( const idVec3& eyeOrigin, const idMat3& eyeAxis );

	void					EjectBrass			( void );

	void					MuzzleFlashLight	( const idVec3& origin, const idMat3& axis );
	void					WeaponFeedback		( const idDict* dict );

	int						nextFireTime;
	int						fireDelay;
	int						count;
	const idDict*			projectileDef;
	const idDict*			hitScanDef;
	float					spread;
	bool					launchFromJoint;
	bool					lockScanning;
	int						lastLockTime;
	float					lockRange;

	idEntityPtr<idEntity>	targetEnt;
	jointHandle_t			targetJoint;
	idVec3					targetPos;
	rvClientEntityPtr<rvClientEffect>	targetEffect;
		
	idList<jointHandle_t>	joints;
	int						jointIndex;
	
	idVec3					force;
	
	int						ammoPerCharge;
	int						chargeTime;
	int						currentAmmo;
	idInterpolate<float>	currentCharge;	
	
	renderLight_t			muzzleFlash;
	int						muzzleFlashHandle;
	int						muzzleFlashEnd;
	int						muzzleFlashTime; 
	idVec3					muzzleFlashOffset;
	
	int						animNum;
	int						animChannel;
	
	const idSoundShader*	shaderFire;
	const idSoundShader*	shaderReload;

	const idDict*			brassDict;
	jointHandle_t			brassEjectJoint;
	idVec3					brassEjectOffset;
	int						brassEjectNext;
	int						brassEjectDelay;

	int						zoomFov;
	idUserInterface *		zoomGui;
	float					zoomTime;
};

ID_INLINE int	rvVehicleWeapon::GetCurrentAmmo		( void ) const { return currentAmmo; }
ID_INLINE float rvVehicleWeapon::GetCurrentCharge	( void ) const { return currentCharge.IsDone(gameLocal.time) ? 1.0f : currentCharge.GetCurrentValue(gameLocal.time); }
ID_INLINE int rvVehicleWeapon::GetZoomFov				( void ) const { return zoomFov; }
ID_INLINE idUserInterface* rvVehicleWeapon::GetZoomGui	( void ) const { return zoomGui; }
ID_INLINE float rvVehicleWeapon::GetZoomTime			( void ) const { return zoomTime; }
ID_INLINE bool rvVehicleWeapon::CanZoom					( void ) const { return zoomFov != -1; }


//----------------------------------------------------------------
//							Turret
//----------------------------------------------------------------

class rvVehicleTurret : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( rvVehicleTurret );

	rvVehicleTurret ( void );
	
	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPrePhysics		( void );		
	virtual void	RunPostPhysics		( void );		
	virtual void	Activate			( bool active );

protected:

	idBounds		angles;
	int				axisMap[3];
	
	float			invert[3];
		
	idAngles		currentAngles;
	
	int				moveTime;	
	float			turnRate;
	
	int				soundPart;

	bool			parentStuck;
};

//----------------------------------------------------------------
//				Animated Vehicle Part
//----------------------------------------------------------------

class rvVehicleUserAnimated : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( rvVehicleUserAnimated );

	rvVehicleUserAnimated ( void );
	
	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPrePhysics		( void );		

	void			Event_PostSpawn		( void );

protected:

	typedef struct rvUserAnimatedAnim_s {
		int					index;
		float				frame;
		short				length;
		short				channel;
		float				rate;
		bool				loop;
	} rvUserAnimatedAnim_t;

	enum { VUAA_Forward, VUAA_Strafe, VUAA_Crouch, VUAA_Attack, VUAA_Count };
	enum { VUAF_Forward, VUAF_Strafe, VUAF_Crouch, VUAF_Attack, VUAF_Count };

	rvUserAnimatedAnim_t	anims[ VUAA_Count ];
	rvScriptFuncUtility		funcs[ VUAF_Count ];

	void InitAnim( const char * action, rvUserAnimatedAnim_t & anim );
	void InitFunc( const char * action, rvScriptFuncUtility & func );
	void SetFrame( const rvUserAnimatedAnim_t & anim );
	void LateralMove( signed char input, int anim, int func );
};

typedef idList<rvVehiclePart*> rvVehiclePartList_t;

#endif // __GAME_VEHICLEPARTS_H__
