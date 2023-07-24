// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_VEHICLES_SOUNDCONTROL_H__
#define __GAME_VEHICLES_SOUNDCONTROL_H__

class sdVehicleSoundControlBase {
public:
							sdVehicleSoundControlBase( void ) { owner = NULL; }
	virtual					~sdVehicleSoundControlBase( void ) { ; }

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition ) { }
	virtual void			OnPlayerExited( idPlayer* player, int position ) { }
	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) { }
	virtual void			OnEMPStateChanged( void ) { }
	virtual void			OnWeaponEMPStateChanged( void ) { }

	virtual void			Init( sdTransport* transport );

	virtual void			Update( void ) = 0;

	static void							Startup( void );
	static void							Shutdown( void );
	static sdVehicleSoundControlBase*	Alloc( const char* name );

	bool					InWater( void ) const;
	bool					Submerged( void ) const;

protected:
	sdTransport*			owner;

	typedef sdFactory< sdVehicleSoundControlBase > soundControlFactory_t;
	static soundControlFactory_t s_factory;
};

class sdVehicleSoundControl_Simple : public sdVehicleSoundControlBase {
public:
	sdVehicleSoundControl_Simple() {
		simpleSoundFlags.playingHornSound = false;
		simpleSoundFlags.playingCockpitSound = false;
	}

	struct soundParms_t {
		bool inWater		: 1;
		bool submerged		: 1;
		float speedKPH;
		float absSpeedKPH;
		float newSoundLevel;
		float newSoundPitch;
	};

	void					CalcSoundParms( soundParms_t& parms ) const;

	virtual void			Init( sdTransport* transport );

	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType );

	virtual void			StartCockpitSound( void );
	virtual void			StopCockpitSound( void );

	virtual void			StartHornSound( void );
	virtual void			StopHornSound( void );

	virtual void			StopOffRoadSound( void ) { }

	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void );

protected:
	float					lowPitch;
	float					highPitch;
	float					lowSpeed;
	float					highSpeed;

	float					maxSoundLevel;

	float					maxHornWaterLevel;

	idStr					groundSurfaceType;
	bool					groundIsOffRoad;

	struct simpleSoundFlags_t {
		bool				playingHornSound		: 1;
		bool				playingCockpitSound		: 1;
	};

	simpleSoundFlags_t		simpleSoundFlags;
};

class sdVehicleSoundControl_CrossFade : public sdVehicleSoundControl_Simple {
public:

	struct soundParmsAdvanced_t {
		soundParms_t simple;

		float idleVolume;		// volume of the idle sound
		float driveVolume;		// volume of the driving sound
		float accelVolume;		// volume of the acceleration sound
		float accelPitch;		// pitch of the acceleration sound
	};

	void					CalcSoundParmsAdvanced( soundParmsAdvanced_t& parms );

	virtual void			Init( sdTransport* transport );

protected:
	void		StartEngineSounds();
	void		StopEngineSounds();
	void		UpdateEngineSounds( soundParmsAdvanced_t& parms );

	float		engineSpeed;
	float		lastVolumeIncreaseValue;
	int			soundKillTime;

	// tuning
	float		accelSpoolTime;
	float		decelSpoolTime;

	float		idleMinSpeed;
	float		idleMaxSpeed;
	float		idleMinVol;
	float		idleMaxVol;
	float		idlePower;
	float		idleFadeTime;

	float		driveMinSpeed;
	float		driveMaxSpeed;
	float		driveMinVol;
	float		driveMaxVol;
	float		drivePower;
	float		driveFadeTime;

	float		accelPitchMultiplier;
	float		accelPitchOffset;
	
	float		accelMinSpeed;
	float		accelMidSpeed;
	float		accelMaxSpeed;
	float		accelMinVol;
	float		accelMidVol;
	float		accelMaxVol;
	float		accelPowerLow;
	float		accelPowerHigh;
	float		accelFadeTime;

	float		accelYawVolume;
	float		accelYawVolumeMultiplier;
	float		accelYawPitch;
	float		accelYawPitchMultiplier;
};

class sdVehicleSoundControl_Wheeled : public sdVehicleSoundControl_CrossFade {
public:
							sdVehicleSoundControl_Wheeled( void );

	virtual void			Update( void );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );
	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType );
	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void );

	virtual void			Init( sdTransport* transport );

	virtual void			StartOffRoadSound( bool force );
	virtual void			StopOffRoadSound( void );

	virtual void			EnterWater( float absSpeedKPH );
	virtual void			ExitWater( float absSpeedKPH );

	virtual void			FadeSkidSoundIn( float volume );
	virtual void			FadeSkidSoundOut( void );

	virtual void			StartSkidSound( bool force );
	virtual void			StopSkidSound( void );

	virtual void			StartDriveSound( void );
	virtual void			StopDriveSound( void );

protected:
	struct soundFlags_t {
		bool				playingOffRoadSound		: 1;
		bool				skidSoundFadedOut		: 1;
		bool				playingSkidSound		: 1;
		bool				inWater					: 1;
		bool				playingDriveSound		: 1;
	};

	soundFlags_t			soundFlags;
};

class sdVehicleSoundControl_Tracked : public sdVehicleSoundControl_CrossFade {
public:
							sdVehicleSoundControl_Tracked( void );

	virtual void			Update( void );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );
	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType );
	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void );

	virtual void			Init( sdTransport* transport );

	virtual void			StartOffRoadSound( void );
	virtual void			StopOffRoadSound( void );

	virtual void			EnterWater( float absSpeedKPH );
	virtual void			ExitWater( float absSpeedKPH );

	virtual void			StartDriveSound( void );
	virtual void			StopDriveSound( void );

protected:
	struct soundFlags_t {
		bool				playingOffRoadSound		: 1;
		bool				playingDriveSound		: 1;
		bool				inWater					: 1;
	};
	soundFlags_t			soundFlags;

	int						nextRevSoundTime;
};

class sdVehicleSoundControl_Helicopter : public sdVehicleSoundControl_Simple {
public:
							sdVehicleSoundControl_Helicopter( void );

	virtual void			Update( void );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );
	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType );
	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void );

	virtual void			Init( sdTransport* transport );

	virtual void			StartTurbineSound( void );
	virtual void			StopTurbineSound( void );

	virtual void			StartRotorSound( void );
	virtual void			StopRotorSound( void );

	virtual void			StartThrottleSound( void );
	virtual void			StopThrottleSound( void );

	virtual void			FadeTailRotorIn( void );
	virtual void			FadeTailRotorOut( void );

	virtual void			EnterWater( void );
	virtual void			ExitWater( void );

protected:
	struct soundFlags_t {
		bool				playingRotorSound		: 1;
		bool				playingTurbineSound		: 1;
		bool				playingThrottleSound	: 1;
		bool				playingTailRotorSound	: 1;
		bool				inWater					: 1;
	};
	soundFlags_t			soundFlags;
};

class sdJetPack;

class sdVehicleSoundControl_JetPack : public sdVehicleSoundControlBase {
public:
							sdVehicleSoundControl_JetPack( void );

	virtual void			Update( void );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );
	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType );
	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void );

	virtual void			Init( sdTransport* transport );

	virtual void			StartIdleSound( void );
	virtual void			StopIdleSound( void );

	virtual void			StartJetSound( void );
	virtual void			StopJetSound( void );

protected:
	struct soundFlags_t {
		bool				playingJetSound		: 1;
		bool				playingIdleSound	: 1;
	};
	soundFlags_t			soundFlags;
	int						nextJetStartSoundTime;
	float					soundSpeedMultiplier;
	float					soundSpeedOffset;
	float					soundPitchMax;
	float					soundRampRate;

	float					lastSoundPitch;

	sdJetPack*				jetPack;
};

class sdVehicleSoundControl_SpeedBoat : public sdVehicleSoundControl_CrossFade {
public:
							sdVehicleSoundControl_SpeedBoat( void );

	virtual void			Update( void );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );
	virtual void			OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType );
	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void );

	virtual void			Init( sdTransport* transport );

	virtual void			StartDriveSound( void );
	virtual void			StopDriveSound( void );

	virtual void			EnterWater( void );
	virtual void			ExitWater( void );

protected:
	struct soundFlags_t {
		bool				playingDriveSound	: 1;
		bool				inWater				: 1;
	};
	soundFlags_t			soundFlags;
};

#endif // __GAME_VEHICLES_SOUNDCONTROL_H__
