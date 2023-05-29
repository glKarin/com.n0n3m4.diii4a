// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 9/30/2004

#ifndef __GAME_WEAPON_H__
#define __GAME_WEAPON_H__

/*
===============================================================================

	Player Weapon
	
===============================================================================
*/

typedef enum {
	WP_READY,
	WP_OUTOFAMMO,
	WP_RELOAD,
	WP_HOLSTERED,
	WP_RISING,
	WP_LOWERING,
	WP_FLASHLIGHT,
} weaponStatus_t;

static const int MAX_WEAPONMODS	= 4;
static const int MAX_AMMOTYPES	= 16;

class idPlayer;
class idItem;
class idAnimatedEntity;
class idProjectile;
class rvWeapon;

class rvViewWeapon : public idAnimatedEntity {
public:

	CLASS_PROTOTYPE( rvViewWeapon );

							rvViewWeapon( void );
	virtual					~rvViewWeapon( void );

	// Init
	void					Spawn						( void );

	// save games
	void					Save						( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore						( idRestoreGame *savefile );					// unarchives object from save game file


	// Weapon definition management
	void					Clear						( void );

	// GUIs
	void					PostGUIEvent				( const char* event );

	virtual void			SetModel					( const char *modelname, int mods = 0 );
	void					SetPowerUpSkin				( const char *name );
	void					UpdateSkin					( void );

	// State control/player interface
	void					Think						( void );

	// Visual presentation
	void					PresentWeapon				( bool showViewModel );

	// Networking
	virtual void			WriteToSnapshot				( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot			( const idBitMsgDelta &msg );
	virtual bool			ClientReceiveEvent			( int event, int time, const idBitMsg &msg );
	virtual void			ClientPredictionThink		( void );
	virtual bool			ClientStale					( void );

	virtual void			ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis );
	virtual void			UpdateModelTransform		( void );

	// Debugging
	virtual void			GetDebugInfo				( debugInfoProc_t proc, void* userData );


	void					SetSkin						( const char *skinname );
	void					SetSkin						( const idDeclSkin* skin );

	void					SetOverlayShader			( const idMaterial* material );

	virtual void			GetPosition					( idVec3& origin, idMat3& axis ) const;

private:

	idStrList				pendingGUIEvents;

	// effects
	const idDeclSkin *		saveSkin;
	const idDeclSkin *		invisSkin;
	const idDeclSkin *		saveWorldSkin;
	const idDeclSkin *		worldInvisSkin;
	const idDeclSkin *		saveHandsSkin;
	const idDeclSkin *		handsSkin;
		
	void					Event_CallFunction			( const char* function );
	
	friend		class rvWeapon;
	rvWeapon*	weapon;
};

class rvWeapon : public idClass {
public:

	CLASS_PROTOTYPE( rvWeapon );

	rvWeapon( void );
	virtual ~rvWeapon( void );
	
	enum {
		WPLIGHT_MUZZLEFLASH,
		WPLIGHT_MUZZLEFLASH_WORLD,
		WPLIGHT_FLASHLIGHT,
		WPLIGHT_FLASHLIGHT_WORLD,
		WPLIGHT_GUI,
		WPLIGHT_MAX
	};

	enum {
		EVENT_RELOAD = idEntity::EVENT_MAXEVENTS,
		EVENT_ENDRELOAD,
		EVENT_CHANGESKIN,
		EVENT_MAXEVENTS
	};
	
	void				Init						( idPlayer* _owner, const idDeclEntityDef* def, int weaponIndex, bool isStrogg = false );

	// Virtual overrides
	void				Spawn						( void );
	virtual void		Think						( void );
	virtual void		CleanupWeapon				( void ) {}
	virtual void		WriteToSnapshot				( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot			( const idBitMsgDelta &msg );
	virtual bool		ClientReceiveEvent			( int event, int time, const idBitMsg &msg );
	virtual void		ClientStale					( void );
	virtual void		ClientUnstale				( void ) { }
	virtual void		Attack						( bool altFire, int num_attacks, float spread, float fuseOffset, float power );
	virtual void		GetDebugInfo				( debugInfoProc_t proc, void* userData );
	virtual void		SpectatorCycle				( void ) { }
	virtual bool		NoFireWhileSwitching		( void ) const { return false; }

	void				Save						( idSaveGame *savefile ) const;
	void				Restore						( idRestoreGame *savefile );
	virtual void		PreSave						( void );
	virtual void		PostSave					( void );


	// Visual presentation
	bool				BloodSplat					( float size );
	void				MuzzleFlash					( void );
	void				MuzzleRise					( idVec3 &origin, idMat3 &axis );
	float				GetMuzzleFlashLightParm		( int parm );
	void				SetMuzzleFlashLightParm		( int parm, float value );
	void				GetAngleOffsets				( int *average, float *scale, float *max );
	void				GetTimeOffsets				( float *time, float *scale );
	bool				GetGlobalJointTransform		( bool viewModel, const jointHandle_t jointHandle, idVec3 &origin, idMat3 &axis, const idVec3& offset = vec3_origin );

	// State control/player interface
	void				LowerWeapon					( void );
	void				RaiseWeapon					( void );
	void				Raise						( void );
	void				PutAway						( void );
	void				Hide						( void );
	void				Show						( void );
	void				HideWorldModel				( void );
	void				ShowWorldModel				( void );
	void				SetFlashlight				( bool on = true );
	void				Flashlight					( void );
	void				SetPushVelocity				( const idVec3 &pushVelocity );
	void				Reload						( void );
	void				OwnerDied					( void );
	void				BeginAttack					( void );
	void				EndAttack					( void );
	bool				IsReady						( void ) const;
	bool				IsReloading					( void ) const;
	bool				IsHolstered					( void ) const;
	bool				ShowCrosshair				( void ) const;
	bool				CanDrop						( void ) const;
	bool				CanZoom						( void ) const;
	void				CancelReload				( void );
	void				SetStatus					( weaponStatus_t status );
	bool				AutoReload					( void );
	bool				IsHidden					( void ) const;
	void				EjectBrass					( void );

	// Network helpers
	void				NetReload					( void );
	void				NetEndReload				( void );
	void				NetCatchup					( void );

	// Ammo
	static int			GetAmmoIndexForName			( const char *ammoname );
	static const char*	GetAmmoNameForIndex			( int index );
	int					GetAmmoType					( void ) const;
	int					AmmoAvailable				( void ) const;
	int					AmmoInClip					( void ) const;
	void				ResetAmmoClip				( void );
	int					ClipSize					( void ) const;
	int					LowAmmo						( void ) const;
	int					AmmoRequired				( void ) const;
	void				AddToClip					( int amount );
	void				UseAmmo						( int amount );
	void				SetClip						( int amount );
	int					TotalAmmoCount				( void ) const;

	// Attack
	bool				PerformAttack				( idVec3& muzzleOrigin, idMat3& muzzleAxis, float dmgPower );
	void				LaunchProjectiles			( idDict& dict, const idVec3& muzzleOrigin, const idMat3& muzzleAxis, int num_projectiles, float spread, float fuseOffset, float power );
	void				Hitscan						( const idDict& dict, const idVec3& muzzleOrigin, const idMat3& muzzleAxis, int num_hitscans, float spread, float power );
	void				AlertMonsters				( void );

	// Mods
	int					GetMods						( void ) const;

	// Zoom
	idUserInterface*	GetZoomGui					( void ) const;
	float				GetZoomTime					( void ) const;
	int					GetZoomFov					( void ) const;

	rvViewWeapon*		GetViewModel				( void ) const;
	idAnimatedEntity*	GetWorldModel				( void ) const;
	idPlayer*			GetOwner					( void ) const;
	const char *		GetIcon						( void ) const;
	renderLight_t&		GetLight					( int light );
	const idAngles&		GetViewModelAngles			( void ) const;
	const idVec3&		GetViewModelOffset			( void ) const;

 	static void			CacheWeapon					( const char *weaponName );
	static void			SkipFromSnapshot			( const idBitMsgDelta &msg );

	void				EnterCinematic				( void );
	void				ExitCinematic				( void );

protected:

	virtual void		OnLaunchProjectile			( idProjectile* proj );

	void				SetState					( const char *statename, int blendFrames );
	void				PostState					( const char *statename, int blendFrames );
	void				ExecuteState				( const char *statename );

	void				PlayAnim					( int channel, const char *animname, int blendFrames );
	void				PlayCycle					( int channel, const char *animname, int blendFrames );
	bool				AnimDone					( int channel, int blendFrames );
	bool				StartSound					( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	void				StopSound					( const s_channelType channel, bool broadcast );
	rvClientEffect*		PlayEffect					( const char* effectName, jointHandle_t joint, bool loop = false, const idVec3& endOrigin = vec3_origin, bool broadcast = false );

	void				FindViewModelPositionStyle	( idVec3& viewOffset, idAngles& viewAngles ) const;

public:

	void				InitLights					( void );
	void				InitWorldModel				( void );
	void				InitViewModel				( void );
	void				InitDefs					( void );

	void				FreeLight					( int lightID );
	void				UpdateLight					( int lightID );

	void				UpdateMuzzleFlash			( void );
	void				UpdateFlashlight			( void );	

	void				UpdateGUI					( void );
	void				UpdateCrosshairGUI			( idUserInterface* gui ) const;

	idMat3				ForeshortenAxis				( const idMat3& axis ) const;

	// Script state management
	struct weaponStateFlags_s {
		bool		attack				:1;
		bool		reload				:1;
		bool		netReload			:1;
		bool		netEndReload		:1;
		bool		raiseWeapon			:1;
		bool		lowerWeapon			:1;
		bool		flashlight			:1;
		bool		zoom				:1;
	} wsfl;		
	
	// Generic flags
	struct weaponFlags_s {
		bool		attackAltHitscan	:1;
		bool		attackHitscan		:1;
		bool		hide				:1;
		bool		disabled			:1;
		bool		hasBloodSplat		:1;
		bool		silent_fire			:1;
		bool		zoomHideCrosshair	:1;
		bool		flashlightOn		:1;
		bool		hasWindupAnim		:1;
	} wfl;
	
	// joints from models
	jointHandle_t					barrelJointView;
	jointHandle_t					flashJointView;
	jointHandle_t					ejectJointView;
	jointHandle_t					guiLightJointView;
	jointHandle_t					flashlightJointView;

	jointHandle_t					flashJointWorld;
	jointHandle_t					ejectJointWorld;
	jointHandle_t					flashlightJointWorld;
	
	weaponStatus_t					status;
	int								lastAttack;

	// hiding weapon
	int								hideTime;
 	float							hideDistance;
 	int								hideStartTime;
	float							hideStart;
	float							hideEnd;
	float							hideOffset;

	// Attack
	idVec3							pushVelocity;
	int								kick_endtime;
	int								muzzle_kick_time;
	int								muzzle_kick_maxtime;
	idAngles						muzzle_kick_angles;
	idVec3							muzzle_kick_offset;
	idVec3							muzzleOrigin;
	idMat3							muzzleAxis;
	float							muzzleOffset;
	idEntityPtr<idEntity>			projectileEnt;
	idVec3							ejectOffset;

	int								fireRate;
	int								altFireRate;
	float							spread;
	int								nextAttackTime;

	// we maintain local copies of the projectile and brass dictionaries so they
	// do not have to be copied across the DLL boundary when entities are spawned
	idDict							attackAltDict;
	idDict							attackDict;
	idDict							brassDict;

	// Melee
	const idDeclEntityDef *			meleeDef;
	float							meleeDistance;

	// zoom
    int								zoomFov;				// variable zoom fov per weapon (-1 is no zoom)
	idUserInterface*				zoomGui;				// whether or not to overlay a zoom scope
	float							zoomTime;				// time it takes to zoom in

	// lights
	renderLight_t					lights[WPLIGHT_MAX];
	int								lightHandles[WPLIGHT_MAX];
	idVec3							guiLightOffset;	
	int								muzzleFlashEnd;
	int								muzzleFlashTime;
	idVec3							muzzleFlashViewOffset;
	bool							flashlightOn;
	idVec3							flashlightViewOffset;	

	// ammo management
	int								ammoType;
	int								ammoRequired;		// amount of ammo to use each shot.  0 means weapon doesn't need ammo.
	int								clipSize;			// 0 means no reload
	int								ammoClip;
	int								lowAmmo;			// if ammo in clip hits this threshold, snd_
	int								maxAmmo;		

 	// multiplayer
 	int								clipPredictTime;

	// these are the player render view parms, which include bobbing
	idVec3							playerViewOrigin;
	idMat3							playerViewAxis;


	// View Model
	idVec3							viewModelOrigin;
	idMat3							viewModelAxis;
	idAngles						viewModelAngles;
	idVec3							viewModelOffset;

	// weighting for viewmodel offsets 
	int								weaponAngleOffsetAverages;
	float							weaponAngleOffsetScale;
	float							weaponAngleOffsetMax;
	float							weaponOffsetTime;
	float							weaponOffsetScale;

	// General
	idStr							icon;
	bool							isStrogg;
	
	bool							forceGUIReload;

public:

	idDict							spawnArgs;

protected:

	idEntityPtr<rvViewWeapon>		viewModel;
	idAnimator*						viewAnimator;
	idEntityPtr<idAnimatedEntity>	worldModel;
	idAnimator*						worldAnimator;
	const idDeclEntityDef*			weaponDef;
	idScriptObject*					scriptObject;
	idPlayer *						owner;
	int								weaponIndex;
	int								mods;

	float							viewModelForeshorten;

	rvStateThread					stateThread;
	int								animDoneTime[ANIM_NumAnimChannels];

private:

	stateResult_t			State_Raise				( const stateParms_t& parms );
	stateResult_t			State_Lower				( const stateParms_t& parms );
	stateResult_t			State_ExitCinematic		( const stateParms_t& parms );
	stateResult_t			State_NetCatchup		( const stateParms_t& parms );

	stateResult_t			Frame_EjectBrass		( const stateParms_t& parms );	

	// store weapon index information for death messages
	int						methodOfDeath;

	// multiplayer hitscans
	int						hitscanAttackDef;

	CLASS_STATES_PROTOTYPE ( rvWeapon );
};

ID_INLINE rvViewWeapon* rvWeapon::GetViewModel ( void ) const {
	return viewModel.GetEntity();
}

ID_INLINE idAnimatedEntity* rvWeapon::GetWorldModel ( void ) const {
	return worldModel;
}

ID_INLINE idPlayer* rvWeapon::GetOwner ( void ) const {
	return owner;
}

ID_INLINE const char* rvWeapon::GetIcon ( void ) const {
	return icon;
}

ID_INLINE renderLight_t& rvWeapon::GetLight ( int light ) {
	assert ( light < WPLIGHT_MAX );
	return lights[light];	
}

ID_INLINE const idAngles& rvWeapon::GetViewModelAngles( void ) const {
	return viewModelAngles;
}

ID_INLINE const idVec3& rvWeapon::GetViewModelOffset ( void ) const {
	return viewModelOffset;
}

ID_INLINE int rvWeapon::GetZoomFov ( void ) const {
	return zoomFov;
}

ID_INLINE idUserInterface* rvWeapon::GetZoomGui ( void ) const {
	return zoomGui;
}

ID_INLINE float rvWeapon::GetZoomTime ( void ) const {
	return zoomTime;
}

ID_INLINE int rvWeapon::GetMods ( void ) const {
	return mods;
}

ID_INLINE void rvWeapon::PreSave ( void ) {
}

ID_INLINE void rvWeapon::PostSave ( void ) {
}


#endif /* !__GAME_WEAPON_H__ */

// RAVEN END
