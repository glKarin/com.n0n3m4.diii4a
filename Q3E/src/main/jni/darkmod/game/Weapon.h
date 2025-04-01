/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

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
	WP_INDICATE
} weaponStatus_t;

class idPlayer;

static const int LIGHTID_WORLD_MUZZLE_FLASH = 1;
static const int LIGHTID_VIEW_MUZZLE_FLASH = 100;

class idMoveableItem;

class idWeapon : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idWeapon );

							idWeapon();
	virtual					~idWeapon() override;

	// Init
	void					Spawn( void );
	void					SetOwner( idPlayer *owner );
	idPlayer*				GetOwner( void );
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const override;

	static void				CacheWeapon( const char *weaponName );

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	// Weapon definition management
	void					Clear( void );
	void					GetWeaponDef( const char *objectname, int ammoinclip );
	bool					IsLinked( void );
	bool					IsWorldModelReady( void );

	// GUIs
	const char *			Icon( void ) const;
	void					UpdateGUI( void );

	virtual void			SetModel( const char *modelname ) override;
	bool					GetGlobalJointTransform( bool viewModel, const jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis );
	void					SetPushVelocity( const idVec3 &pushVelocity );
	bool					UpdateSkin( void );

	// State control/player interface
	virtual void			Think( void ) override;
	void					Raise( void );
	void					PutAway( void );
	void					Reload( void );
	void					LowerWeapon( void );
	void					RaiseWeapon( void );
	void					HideWeapon( void );
	void					ShowWeapon( void );
	void					HideWorldModel( void );
	void					ShowWorldModel( void );
	void					OwnerDied( void );
	void					BeginAttack( void );
	void					EndAttack( void );
	void					BeginBlock( void );
	void					EndBlock( void );
	bool					IsReady( void ) const;
	bool					IsReloading( void ) const;
	bool					IsHolstered( void ) const;
	bool					ShowCrosshair( void ) const;
	idEntity *				DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died );
	bool					CanDrop( void ) const;
	void					WeaponStolen( void );
	void					SetArrow2Arrow( bool state); // grayman #597
	void					Indicate(bool indicate);		// Obsttorte
	void					Perform_KO(void);	// Obsttorte
	bool					canKnockout(void);
	float					getMeleeDistance(void);
	float					getKnockoutRange(void);
	float					getKOBoxSize(void);

	// Script state management
	virtual idThread *		ConstructScriptObject( void ) override;
	virtual void			DeconstructScriptObject( void ) override;
	void					SetState( const char *statename, int blendFrames );
	void					UpdateScript( void );
	void					EnterCinematic( void );
	void					ExitCinematic( void );
	void					NetCatchup( void );

	// Visual presentation
	void					PresentWeapon( bool showViewModel );
	int						GetZoomFov( void );
	void					GetWeaponAngleOffsets( int *average, float *scale, float *max );
	void					GetWeaponTimeOffsets( float *time, float *scale );
	bool					BloodSplat( float size );

	// Ammo
	static const char		*GetAmmoNameForNum( int ammonum );
	static const char		*GetAmmoPickupNameForNum( int ammonum );
	int						AmmoAvailable( void ) const;
	int						AmmoInClip( void ) const;
	void					ResetAmmoClip( void );
	int						ClipSize( void ) const;
	int						LowAmmo( void ) const;
	int						AmmoRequired( void ) const;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	/**
	* TDM: Attach an entity to the weapon's AF.  Different than plain entity attachment
	* Does not use position name for now
	**/
	virtual void			Attach( idEntity *ent, const char *PosName = NULL, const char *AttName = NULL ) override;

	enum {
		EVENT_RELOAD = idEntity::EVENT_MAXEVENTS,
		EVENT_ENDRELOAD,
		EVENT_CHANGESKIN,
		EVENT_MAXEVENTS
	};
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;
	virtual void			ClientPredictionThink( void ) override;
	
	/**
	* TDM: Return true if this is a ranged weapon
	**/
	bool					IsRanged();

protected:
	/**
	* Used internally by the Attach methods.
	* Offset and axis are filled with the correct offset and axis
	* for attaching to a particular joint.
	* Overloaded to call GetJointGlobalTransform on idWeapon entities.
	**/
	virtual void GetAttachingTransform( jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis ) override;

private:
	// script control
	idScriptBool			WEAPON_ATTACK;
	idScriptBool			WEAPON_BLOCK;
	idScriptBool			WEAPON_RELOAD;
	idScriptBool			WEAPON_NETRELOAD;
	idScriptBool			WEAPON_NETENDRELOAD;
	idScriptBool			WEAPON_NETFIRING;
	idScriptBool			WEAPON_RAISEWEAPON;
	idScriptBool			WEAPON_LOWERWEAPON;
	idScriptBool			WEAPON_INDICATE; // Obsttorte
	weaponStatus_t			status;
	idThread *				thread;
	idStr					state;
	idStr					idealState;
	int						animBlendFrames;
	/**
	* TDM: animDoneTime no longer used, but keep it around
	* for compatibility with future patches/engines
	**/
	int						animDoneTime;
	bool					isLinked;

	// precreated projectile
	idEntity				*projectileEnt;

	idPlayer *				owner;
	idEntityPtr<idAnimatedEntity>	worldModel;

	// hiding (for GUIs and NPCs)
	int						hideTime;
	float					hideDistance;
	int						hideStartTime;
	float					hideStart;
	float					hideEnd;
	float					hideOffset;
	bool					hide;
	bool					disabled;

	// these are the player render view parms, which include bobbing
	idVec3					playerViewOrigin;
	idMat3					playerViewAxis;

	// the view weapon render entity parms
	idVec3					viewWeaponOrigin;
	idMat3					viewWeaponAxis;
	
	// the muzzle bone's position, used for launching projectiles and trailing smoke
	idVec3					muzzleOrigin;
	idMat3					muzzleAxis;

	idVec3					pushVelocity;

	// weapon definition
	// we maintain local copies of the projectile and brass dictionaries so they
	// do not have to be copied across the DLL boundary when entities are spawned
	const idDeclEntityDef *	weaponDef;
	const idDeclEntityDef *	meleeDef;

	// greebo: This is not needed anymore - the projectile dictionary is requested when it's actually needed
	//idDict					projectileDict;
	float					meleeDistance;
	float					knockoutRange; // Obsttorte (#4289)
	float					KOBoxSize;
	idStr					meleeDefName;
	idDict					brassDict;
	int						brassDelay;
	idStr					icon;

	// view weapon gui light
	renderLight_t			guiLight;
	int						guiLightHandle;

	// muzzle flash
	renderLight_t			muzzleFlash;		// positioned on view weapon bone
	int						muzzleFlashHandle;

	renderLight_t			worldMuzzleFlash;	// positioned on world weapon bone
	int						worldMuzzleFlashHandle;

	idVec3					flashColor;
	int						muzzleFlashEnd;
	int						flashTime;
	bool					lightOn;
	bool					allowDrop;

	int						hideUntilTime; // grayman #597 - keep hidden until timer expires (for arrow spawning)

	// effects
	bool					hasBloodSplat;

	// weapon kick
	int						kick_endtime;
	int						muzzle_kick_time;
	int						muzzle_kick_maxtime;
	idAngles				muzzle_kick_angles;
	idVec3					muzzle_kick_offset;

	// ammo management
	int						ammoRequired;		// amount of ammo to use each shot.  0 means weapon doesn't need ammo.
	int						clipSize;			// 0 means no reload
	int						ammoClip;
	int						lowAmmo;			// if ammo in clip hits this threshold, snd_
	bool					powerAmmo;			// true if the clip reduction is a factor of the power setting when
												// a projectile is launched
	// mp client
	bool					isFiring;

	// zoom
    int						zoomFov;			// variable zoom fov per weapon

	// joints from models
	jointHandle_t			barrelJointView;
	jointHandle_t			flashJointView;
	jointHandle_t			ejectJointView;
	jointHandle_t			guiLightJointView;
	jointHandle_t			ventLightJointView;

	jointHandle_t			flashJointWorld;
	jointHandle_t			barrelJointWorld;
	jointHandle_t			ejectJointWorld;

	// sound
	const idSoundShader *	sndHum;

	// new style muzzle smokes
	const idDeclParticle *	weaponSmoke;			// null if it doesn't smoke
	int						weaponSmokeStartTime;	// set to gameLocal.time every weapon fire
	bool					continuousSmoke;		// if smoke is continuous ( chainsaw )
	const idDeclParticle *  strikeSmoke;			// striking something in melee
	int						strikeSmokeStartTime;	// timing	
	idVec3					strikePos;				// position of last melee strike	
	idMat3					strikeAxis;				// axis of last melee strike
	int						nextStrikeFx;			// used for sound and decal ( may use for strike smoke too )

	// nozzle effects
	bool					nozzleFx;			// does this use nozzle effects ( parm5 at rest, parm6 firing )
										// this also assumes a nozzle light atm
	int						nozzleFxFade;		// time it takes to fade between the effects
	int						lastAttack;			// last time an attack occurred
	int						lastBlock;			// last time a block occurred
	renderLight_t			nozzleGlow;			// nozzle light
	int						nozzleGlowHandle;	// handle for nozzle light

	idVec3					nozzleGlowColor;	// color of the nozzle glow
	const idMaterial *		nozzleGlowShader;	// shader for glow light
	float					nozzleGlowRadius;	// radius of glow light

	// weighting for viewmodel angles
	int						weaponAngleOffsetAverages;
	float					weaponAngleOffsetScale;
	float					weaponAngleOffsetMax;
	float					weaponOffsetTime;
	float					weaponOffsetScale;

	// weapon switching
	bool					arrow2Arrow;		// grayman #597

	// flashlight
	void					AlertMonsters( void );


	// Visual presentation
	void					InitWorldModel( const idDeclEntityDef *def );
	void					MuzzleFlashLight( void );
	void					MuzzleRise( idVec3 &origin, idMat3 &axis );
	void					UpdateNozzleFx( void );
	void					UpdateFlashPosition( void );

	// script events
	void					Event_Clear( void );
	void					Event_GetOwner( void );
	void					Event_WeaponState( const char *statename, int blendFrames );
	void					Event_SetWeaponStatus( float newStatus );
	void					Event_WeaponReady( void );
	void					Event_WeaponOutOfAmmo( void );
	void					Event_WeaponReloading( void );
	void					Event_WeaponHolstered( void );
	void					Event_WeaponRising( void );
	void					Event_WeaponLowering( void );
	void					Event_UseAmmo( int amount );
	void					Event_AddToClip( int amount );
	void					Event_AmmoInClip( void );
	void					Event_AmmoAvailable( void );
	void					Event_TotalAmmoCount( void );
	void					Event_ClipSize( void );
	void					Event_PlayAnim( int channel, const char *animname );
	void					Event_PauseAnim( int channel, bool bPause );
	void					Event_AnimIsPaused( int channel );
	void					Event_PlayCycle( int channel, const char *animname );
	void					Event_AnimDone( int channel, int blendFrames );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_Next( void );
	void					Event_SetSkin( const char *skinname );
/**
* TDM: Show or hide the weapon attachments
* The first argument is an index to the attachments list, starting at 1
* Second argument is set to true if it should be shown, false for hiding.
**/
	void					Event_ShowAttachment( int id, bool bShow );

	void					Event_Flashlight( int enable );
	void					Event_GetLightParm( int parmnum );
	void					Event_SetLightParm( int parmnum, float value );
	void					Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower );
	void					Event_CreateProjectile( void );
	void					Event_EjectBrass( void );
	void					Event_Melee( void );
	void					Event_GetWorldModel( void );
	void					Event_AllowDrop( int allow );
	void					Event_NetReload( void );
	void					Event_IsInvisible( void );
	void					Event_NetEndReload( void );
};

ID_INLINE bool idWeapon::IsLinked( void ) {
	return isLinked;
}

ID_INLINE bool idWeapon::IsWorldModelReady( void ) {
	return ( worldModel.GetEntity() != NULL );
}

ID_INLINE idPlayer* idWeapon::GetOwner( void ) {
	return owner;
}

#endif /* !__GAME_WEAPON_H__ */
