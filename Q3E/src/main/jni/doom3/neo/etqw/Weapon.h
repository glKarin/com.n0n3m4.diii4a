// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_WEAPON_H__
#define __GAME_WEAPON_H__

#include "interfaces/NetworkInterface.h"
#include "client/ClientEntity.h"

/*
===============================================================================

	Player Weapon
	
===============================================================================
*/

#include "AnimatedEntity.h"

typedef enum {
	WP_READY,
	WP_RELOAD,
	WP_HOLSTERED,
	WP_RISING,
	WP_LOWERING
} weaponStatus_t;

class idWeapon;

class sdWeaponNetworkInterface : public sdNetworkInterface {
public:
										sdWeaponNetworkInterface( void ) { owner = NULL; }

	void								Init( idWeapon* _owner ) { owner = _owner; }

	virtual void						HandleNetworkMessage( idPlayer* player, const char* message );
	virtual void						HandleNetworkEvent( const char* message );

protected:
	idWeapon*							owner;
};

class sdWeaponLockInfo {
public:
							sdWeaponLockInfo( void ) { lockingSound = NULL; lockedSound = NULL; lockDistance = 0.f; supported = false; lockFriendly = false; sticky = false; }

	void					Load( const idDict& dict );

	void					SetSupported( bool value ) { supported = value; }

	bool					IsSupported( void ) const { return supported; }
	bool					IsSticky( void ) const { return sticky; }
	float					GetLockDistance( void ) const { return lockDistance; }
	const idSoundShader*	GetLockingSound( void ) const { return lockingSound; }
	const idSoundShader*	GetLockedSound( void ) const { return lockedSound; }
	int						GetLockDuration( void ) const { return lockDuration; }
	bool					LockFriendly( void ) const { return lockFriendly; }

	const sdDeclTargetInfo*	GetLockFilter( void ) const { return lockFilter; }

private:
	const idSoundShader*	lockingSound;
	const idSoundShader*	lockedSound;
	float					lockDistance;
	int						lockDuration;
	bool					supported;
	bool					lockFriendly;
	bool					sticky;
	const sdDeclTargetInfo*	lockFilter;
};

class idPlayer;
class idProjectile;
class idMoveableItem;
class sdPlayerStatEntry;

extern const idEventDef EV_Weapon_LaunchProjectiles;

typedef struct modInfo_s {
	const idDeclEntityDef*	projectileDef;
	const char*				clientProjectileScript;
	ammoType_t				ammoType;
	int						ammoRequired;		// amount of ammo to use each shot.  0 means weapon doesn't need ammo.
	int						clipSize;			// 0 means no reload
	int						lowAmmo;			// if ammo in clip hits this threshold, snd_
} modInfo_t;

struct weaponSpreadValues_t {
	float					min;
	float					max;
	float					inc;
	int						numIgnoreRounds;	// e.g., the assualt rifle's burst fire should all have the same cone
	int						maxSettleTime;

	// movement dependant
	float					viewRateMin;
	float					viewRateMax;
	float					viewRateInc;
};

struct weaponAimValues_t {
	float					bobscaleyaw;
	float					bobscalepitch;
	float					lagscaleyaw;
	float					lagscalepitch;
	float					speedlr;
};

struct weaponFeedback_t {
	int						recoilTime;
	idAngles				recoilAngles;
	float					kickback;
	float					kickbackProne;
};

enum weaponSpreadValueIndex_t {
	WSV_STANDING,
	WSV_CROUCHING,
	WSV_PRONE,
	WSV_JUMPING,
	WSV_NUM,
};

enum weaponAimValueIndex_t {
	WAV_NORMAL,
	WAV_IRONSIGHTS,
	WAV_NUM,
};

class idWeapon : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idWeapon );

							idWeapon();
	virtual					~idWeapon();

	// Init
	void					Spawn( void );
	void					SetOwner( idPlayer* _owner );
	idPlayer*				GetOwner( void ) { return owner; }
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;

	virtual bool			StartSynced( void ) const { return true; }

	void					UpdateVisibility( void );

	virtual bool			NoThink( void ) const { return true; }

	bool					OnActivate( idPlayer* player, float distance );
	bool					OnActivateHeld( idPlayer* player, float distance );
	bool					OnUsed( idPlayer* player, float distance );
	bool					OnWeapNext();
	bool					OnWeapPrev();

	virtual sdTeamInfo*		GetGameTeam( void ) const;

	// Weapon definition management
	void					Clear( void );
	void					LinkScriptVariables( void );
	void					GetWeaponDef( const sdDeclInvItem* item );
	bool					IsLinked( void );

	void					MakeReady( void ) { Event_WeaponReady(); }
	void					OnProxyEnter( void );
	void					OnProxyExit( void );

	void					LogTimeUsed( void );

	virtual void			SetModel( const char *modelname );
	bool					GetGlobalJointTransform( bool viewModel, const jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis );

	void					UpdateShadows( void );

	// State control/player interface
	void					Think( void );
	void					Raise( void );
	void					PutAway( void );
	void					Reload( void );
	void					LowerWeapon( void );
	void					RaiseWeapon( void );
	void					HideWorldModel( void );
	void					ShowWorldModel( void );
	void					OwnerDied( void );
	void					BeginAttack( void );
	void					EndAttack( void );
	
	void					UpdateSpreadValue( void );
	void					UpdateSpreadValue( const idVec3& velocity, const idAngles& angles, const idAngles& oldAngles );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual	bool			ClientReceiveUnreliableEvent( int event, int time, const idBitMsg &msg );

	void					DoStanceTransition( weaponSpreadValueIndex_t oldStanceState );

	void					BeginModeSwitch( void );
	void					EndModeSwitch( void );

	void					BeginAltFire( void );
	void					EndAltFire( void );

	bool					IsReady( void ) const;
	bool					IsReloading( void ) const;
	bool					IsHolstered( void ) const;
	void					InstantSwitch( void );
	bool					CanAttack( void );

	bool					IsAttacking( void ) const;

	const idVec3&			GetViewOffset( void ) const { return viewOffset; }

	float					GetLargeFOVScale( void ) const { return largeFOVScale; }

	float					GetSpreadValue() const;
	float					GetSpreadValueNormalized( bool useGlobalMax = true ) const;
	float					GetCrosshairSpreadMin() const { return crosshairSpreadMin; }
	float					GetCrosshairSpreadMax() const { return crosshairSpreadMax; }
	float					GetCrosshairSpreadScale() const { return crosshairSpreadScale; }

	int						GetGrenadeFuseTime( void );

	void					SetOwnerStanceState( weaponSpreadValueIndex_t state );

	// Script state management
	virtual sdProgramThread* ConstructScriptObject( void );
	virtual void			DeconstructScriptObject( void );
	void					SetState( const char *statename, int blendFrames );
	void					UpdateScript( void );

	// Visual presentation
	virtual void			Present( void );
	virtual void			FreeModelDef( void );
	void					PresentWeapon( void );
	void					SwayAngles( idAngles& angles ) const;

	// Ammo
	static ammoType_t		GetAmmoType( const char *ammoname );
	ammoType_t				GetAmmoType( int modIndex ) const;
	int						ShotsAvailable( int modIndex ) const;
	int						GetClipSize( int modIndex ) const;
	int						LowAmmo( int modIndex ) const;
	int						AmmoRequired( int modIndex ) const;
	bool					IsClipBased() const;
	bool					UsesStroyent() const;

	void							UpdatePlayerWeaponInfo(); //mal: update what this player's current weapon is for the bots.
	const playerWeaponTypes_t&		GetPlayerWeaponNum( void ) const { return playerWeaponNum; }
	bool							IsIronSightsEnabled( void );

	const weaponAimValues_t&		GetAimValues( weaponAimValueIndex_t index ) { return aimValues[ index ]; }

	virtual sdNetworkInterface*			GetNetworkInterface( void ) { return &networkInterface; }

	const sdWeaponLockInfo*	GetLockInfo( void ) const { return &lockInfo; }

	void					ClampAngles( idAngles& angles, const idAngles& oldAngles ) const;

	float					GetDriftScale( void ) const { return driftScale; }

	bool					GetFov( float& fov ) const;

	static void				RegisterCVarCallback( void );
	static void				UnRegisterCVarCallback( void );
	static void				UpdateWeaponVisibility( void );

	const sdDeclInvItem*	GetInvItemDecl( void ) const { return weaponItem; }

	void					SetNumWorldModels( int count );

	// allow scripts to cleanup their state before we do something like lower from code (e.g. allow the script to exit iron-sights)
	void					ScriptCleanup( void );

	bool					ActivateAttack( void ) const { return activateAttack; }

private:
	void					SetupAnimClass( const char* prefix );

	// script control
	idScriptBool			WEAPON_ATTACK;
	idScriptBool			WEAPON_ALTFIRE;
	idScriptBool			WEAPON_MODESWITCH;
	idScriptBool			WEAPON_RELOAD;
	idScriptBool			WEAPON_RAISEWEAPON;
	idScriptBool			WEAPON_LOWERWEAPON;
	idScriptBool			WEAPON_HIDE;

	weaponStatus_t			status;

	sdProgramThread*		thread;

	idStr					state;
	idStr					idealState;

	int						animBlendFrames;
	int						animDoneTime;
	bool					isLinked;
	bool					modelDisabled;
	bool					worldModelDisabled;
	bool					activateAttack;

	sdWeaponLockInfo		lockInfo;

	struct stats_t {
		sdPlayerStatEntry*	shotsFired;
		sdPlayerStatEntry*	timeUsed;
	};
	stats_t					stats;

	const sdProgram::sdFunction*		onActivateFunc;
	const sdProgram::sdFunction*		onActivateFuncHeld;
	const sdProgram::sdFunction*		onUsedFunc;
	const sdProgram::sdFunction*		onWeapNextFunc;
	const sdProgram::sdFunction*		onWeapPrevFunc;
	const sdProgram::sdFunction*		cleanupFunc;
	const sdProgram::sdFunction*		getGrenadeFuseStartFunc;
	const sdProgram::sdFunction*		ironSightsEnabledFunc;

	playerWeaponTypes_t					playerWeaponNum;

	// precreated projectile
	idEntity				*projectileEnt;

	idPlayer*										owner;

	idList< rvClientEntityPtr< sdClientAnimated > >	worldModels;
	idList< jointHandle_t >							barrelJointsWorld;

	// hiding (for GUIs and NPCs)
	int						hideTime;
	float					hideDistance;
	int						hideStartTime;
	float					hideStart;
	float					hideEnd;
	float					hideOffset;
	bool					hide;
	bool					disabled;

	float					largeFOVScale;

	// some weapons aren't clip-based (e.g. grenades) and this affects the GUI
	bool					clipBased;

	// some strogg weapons' ammo is not based on stroyent
	bool					usesStroyent;

	// these are the player render view parms, which include bobbing
	idVec3					playerViewOrigin;
	idMat3					playerViewAxis;

	// the view weapon render entity parms
	idVec3					viewWeaponOrigin;
	idMat3					viewWeaponAxis;

	idVec3					viewOffset;

	float					viewForeShorten;
	idMat3					viewForeShortenAxis;

	idInterpolate< float >	zoomFov;
	
	weaponSpreadValueIndex_t	ownerStanceState;

	weaponSpreadValues_t	spreadValues[ WSV_NUM ];
	weaponAimValues_t		aimValues[ WAV_NUM ];
	float					spreadValueMax;				// used to normalize against
	bool					spreadEvalVelocity;
	bool					spreadEvalView;

	float					spreadCurrentValue;
	float					spreadModifier;

	float					crosshairSpreadMin;
	float					crosshairSpreadMax;
	float					crosshairSpreadScale;

	float					driftScale;

	float					zoomPitchAmplitude;
	float					zoomPitchFrequency;
	float					zoomPitchMinAmplitude;
	float					zoomYawAmplitude;
	float					zoomYawFrequency;
	float					zoomYawMinAmplitude;
	bool					swayEnabled;

	// weapon definition
	const sdDeclInvItem*	weaponItem;
	
	const sdDeclDamage*		meleeDamage;
	const sdDeclDamage*		meleeSpecialDamage;

	// weapon kick
	int						kick_endtime;
	int						muzzle_kick_time;
	int						muzzle_kick_maxtime;
	idAngles				muzzle_kick_angles;
	idVec3					muzzle_kick_offset;

	// zoom

	// joints from models
	jointHandle_t			barrelJointView;

	trace_t					meleeTrace;			// FIXME: static?

	sdWeaponNetworkInterface	networkInterface;

	angleClamp_t			clampPitch;
	angleClamp_t			clampYaw;
	bool					clampEnabled;
	idAngles				clampBaseAngles;

	int						tracerCounter;

	int						timeStartedUsing;

	rvClientEntityPtr< rvClientEffect > lastTracer;

	renderEntity_t			dofRenderEntity;
	int						dofModelDefHandle;
	const idDeclSkin*		dofSkin;

	const idDeclSkin*		climateSkin;

	weaponFeedback_t		feedback;

	// Visual presentation
	void					InitWorldModel( int index );
	void					MuzzleRise( idVec3 &origin, idMat3 &axis );

	void					SetupStats( const char* statName );

	virtual void			SetSkin( const idDeclSkin* skin );

	// script events
	void					Event_Clear( void );
	void					Event_GetOwner( void );
	void					Event_GetWeaponState( void );
	void					Event_WeaponState( const char *statename, int blendFrames );
	void					Event_SetWeaponStatus( float newStatus );
	void					Event_WeaponReady( void );
	void					Event_WeaponReloading( void );
	void					Event_WeaponHolstered( void );
	void					Event_WeaponRising( void );
	void					Event_WeaponLowering( void );
	void					Event_AddToClip( int modIndex, int amount );
	void					Event_AmmoInClip( int modIndex );
	void					Event_NeedsAmmo( int modIndex );
	void					Event_AmmoAvailable( int modIndex );
	void					Event_AmmoRequired( int modIndex );
	void					Event_AmmoType( int modIndex );
	void					Event_UseAmmo( ammoType_t type, int amount );
	void					Event_ClipSize( int modIndex );
	void					Event_PlayAnim( animChannel_t channel, const char *animname );
	void					Event_PlayCycle( animChannel_t channel, const char *animname );
	void					Event_AnimDone( animChannel_t channel, int blendFrames );
	void					Event_SetBlendFrames( animChannel_t channel, int blendFrames );
	void					Event_GetBlendFrames( animChannel_t channel );
	void					Event_Next( void );
	void					Event_LaunchProjectiles( int numProjectiles, int projectileIndex, float spread, float fuseOffset, float launchPower, float dmgPower );
	void					Event_DoProjectileTracer( int projectileIndex, const idVec3& start, const idVec3& end );
	void					Event_CreateProjectile( int projectileIndex );
	void					Event_Melee( int contentMask, float distance, bool ignoreOwner, bool useAntiLag );
	void					Event_MeleeAttack( float damageScale );
	void					Event_SaveMeleeTrace( void );
	void					Event_GetMeleeFraction( void );
	void					Event_GetMeleeEndPos( void );
	void					Event_GetMeleePoint( void );
	void					Event_GetMeleeNormal( void );
	void					Event_GetMeleeEntity( void );
	void					Event_GetMeleeSurfaceFlags( void );
	void					Event_GetMeleeSurfaceType( void );
	void					Event_GetMeleeSurfaceColor( void );
	void					Event_GetMeleeJoint( void );
	void					Event_GetMeleeBody( void );

	void					Event_AutoReload( void );
	void					Event_NetReload( void );
	void					Event_NetEndReload( void );
	void					Event_EnableTargetLock( bool enable );
	void					Event_SetFov( float startFov, float endFov, float duration );
	void					Event_SetFovStart( float startFov, float endFov, float startTime, float duration );
	void					Event_ClearFov( void );
	void					Event_GetFov( void );
	void					Event_EnableClamp( const idAngles& baseAngles );
	void					Event_DisableClamp( void );
	void					Event_GetSpreadValue( void );
	void					Event_IncreaseSpreadValue( void );
	void					Event_SetSpreadModifier( float modifier );
	void					Event_Hide( void );
	void					Event_Show( void );
							// weapons that don't do melee or projectile attacks still need to indicate they've been fired to their owner
	void					Event_Fired( void );
	void					Event_GetWorldModel( int index );
	void					Event_EnableSway( bool enable );
	void					Event_SetDriftScale( float scale );
	void					Event_ResetTracerCounter( void );
	void					Event_GetLastTracer( void );
	void					Event_SetupAnimClass( const char* prefix );
	void					Event_HasWeaponAnim( const char* anim );
	void					Event_SetStatName( const char* statName );

	void					Event_SendTracerMessage( const idVec3& start, const idVec3& end, float strength );

	enum {
		EVENT_RELOAD = idEntity::EVENT_MAXEVENTS,
		EVENT_TRACER,		// unreliable event
		EVENT_MAXEVENTS
	};
};

ID_INLINE bool idWeapon::IsLinked( void ) {
	return isLinked;
}

#endif /* !__GAME_WEAPON_H__ */


