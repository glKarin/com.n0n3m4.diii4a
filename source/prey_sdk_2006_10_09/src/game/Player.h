// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

/*
===============================================================================

	Player entity.
	
===============================================================================
*/

extern const idEventDef EV_Player_GetButtons;
extern const idEventDef EV_Player_GetMove;
extern const idEventDef EV_Player_GetViewAngles;
extern const idEventDef EV_Player_EnableWeapon;
extern const idEventDef EV_Player_DisableWeapon;
extern const idEventDef EV_Player_ExitTeleporter;
extern const idEventDef EV_Player_SelectWeapon;
extern const idEventDef EV_SpectatorTouch;

const float THIRD_PERSON_FOCUS_DISTANCE	= 512.0f;
const int	LAND_DEFLECT_TIME = 150;
const int	LAND_RETURN_TIME = 300;
const int	FOCUS_TIME = 200;		//HUMANHEAD bjk
const int	FOCUS_GUI_TIME = 300;	//HUMANHEAD bjk

const int MAX_WEAPONS = 16;

#define MP_PLAYERNOSHADOW_DEFAULT	true //HUMANEHAD rww - subject to (frequent) change

const int DEAD_HEARTRATE = 0;			// fall to as you die
const int LOWHEALTH_HEARTRATE_ADJ = 20; // 
const int DYING_HEARTRATE = 30;			// used for volumen calc when dying/dead
const int BASE_HEARTRATE = 70;			// default
const int ZEROSTAMINA_HEARTRATE = 115;  // no stamina
const int MAX_HEARTRATE = 130;			// maximum
const int ZERO_VOLUME = -40;			// volume at zero
const int DMG_VOLUME = 5;				// volume when taking damage
const int DEATH_VOLUME = 15;			// volume at death

const int SAVING_THROW_TIME = 5000;		// maximum one "saving throw" every five seconds

const int ASYNC_PLAYER_INV_AMMO_BITS = idMath::BitsForInteger( 999 );	// 9 bits to cover the range [0, 999]
const int ASYNC_PLAYER_INV_CLIP_BITS = -7;								// -7 bits to cover the range [-1, 60]

struct idItemInfo {
	idStr name;
	idStr icon;
// HUMANHEAD
	int		time;
	int		slotZeroTime;
	float	matcolorAlpha;
	bool	bDoubleWide;
// HUMANHEAD END
};

// struct idObjectiveInfo (HUMANHEAD pdm: removed)

struct idLevelTriggerInfo {
	idStr levelName;
	idStr triggerName;
};

// influence levels
enum {
	INFLUENCE_NONE = 0,			// none
	INFLUENCE_LEVEL1,			// no gun or hud
	INFLUENCE_LEVEL2,			// no gun, hud, movement
	INFLUENCE_LEVEL3,			// slow player movement
};

class idInventory {
public:
	int						maxHealth;
	int						weapons;
	int						ammo[ AMMO_NUMTYPES ];
	int						clip[ MAX_WEAPONS ];

	// mp
	int						ammoPredictTime;

	idList<idDict *>		items;

	bool					ammoPulse;
	bool					weaponPulse;

	idList<idLevelTriggerInfo> levelTriggers;

							idInventory() { Clear(); }
							~idInventory() { Clear(); }

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual				// HUMANHEAD nla 
	void					Clear( void );
	virtual				// HUMANHEAD nla 
	void					GetPersistantData( idDict &dict );
	virtual				// HUMANHEAD nla 
	void					RestoreInventory( idPlayer *owner, const idDict &dict );
	virtual				// HUMANHEAD nla 
	bool					Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud );
	void					Drop( const idDict &spawnArgs, const char *weapon_classname, int weapon_index );
	ammo_t					AmmoIndexForAmmoClass( const char *ammo_classname ) const;
	int						MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const;
	int						WeaponIndexForAmmoClass( const idDict & spawnArgs, const char *ammo_classname ) const;
	ammo_t					AmmoIndexForWeaponClass( const char *weapon_classname, int *ammoRequired );
	ammo_t					AltAmmoIndexForWeaponClass( const char *weapon_classname, int *ammoRequired );	//HUMANHEAD bjk
	const char *			AmmoPickupNameForIndex( ammo_t ammonum ) const;
	void					AddPickupName( const char *name, const char *icon );

	virtual				// HUMANHEAD
	int						HasAmmo( ammo_t type, int amount );
	virtual				// HUMANHEAD
	bool					UseAmmo( ammo_t type, int amount );
	virtual				// HUMANHEAD CJR
	int						HasAmmo( const char *weapon_classname );			// looks up the ammo information for the weapon class first

	idList<idItemInfo>		pickupItemNames;
};

typedef struct {
	int		time;
	idVec3	dir;		// scaled larger for running
} loggedAccel_t;

// HUMANHEAD nla - Has to be here.  Must be after idInventory so can subclass, but before idPlayer so we can instantiate
#include "../Prey/game_inventory.h"
// HUMANHEAD END

typedef struct {
	int		areaNum;
	idVec3	pos;
} aasLocation_t;

class idPlayer : public idActor {
public:
	enum {
		EVENT_IMPULSE = idEntity::EVENT_MAXEVENTS,
		EVENT_EXIT_TELEPORTER,
		EVENT_ABORT_TELEPORTER,
		EVENT_POWERUP,
		EVENT_SPECTATE,
		EVENT_MENUEVENT, //HUMANHEAD rww
		EVENT_HITNOTIFICATION, //HUMANHEAD rww
		EVENT_PORTALLED, //HUMANHEAD rww
		EVENT_REPORTATTACK, //HUMANHEAD rww
		EVENT_MAXEVENTS
	};

	//HUMANHEAD rww - for EVENT_MENUEVENT
	enum {
		MENU_NET_EVENT_HEALTHPULSE,
		MENU_NET_EVENT_MAXHEALTHPULSE,
		MENU_NET_EVENT_SPIRITPULSE,
		MENU_NET_EVENT_AMMOPULSE,
		MENU_NET_EVENT_WEAPONPULSE,
		MENU_NET_EVENT_INVPICKUP,
		MENU_NET_EVENT_NUM
	};
	//HUMANHEAD END

	usercmd_t				usercmd;

	// HUANHEAD: Changed to our version
#ifdef HUMANHEAD
	class hhPlayerView		playerView;			// handles damage kicks and effects
#else
	class idPlayerView		playerView;			// handles damage kicks and effects
#endif
	// HUMANHEAD END

	bool					noclip;
	bool					godmode;

	bool					spawnAnglesSet;		// on first usercmd, we must set deltaAngles
	idAngles				spawnAngles;
	idAngles				viewAngles;			// player view angles
	idAngles				cmdAngles;			// player cmd angles

	int						buttonMask;
	int						oldButtons;
	int						oldFlags;

	int						lastHitTime;			// last time projectile fired by player hit target
	int						lastSndHitTime;			// MP hit sound - != lastHitTime because we throttle
	int						lastSavingThrowTime;	// for the "free miss" effect

	idScriptBool			AI_FORWARD;
	idScriptBool			AI_BACKWARD;
	idScriptBool			AI_STRAFE_LEFT;
	idScriptBool			AI_STRAFE_RIGHT;
	idScriptBool			AI_ATTACK_HELD;
	idScriptBool			AI_WEAPON_FIRED;
	idScriptBool			AI_JUMP;
	idScriptBool			AI_CROUCH;
	idScriptBool			AI_ONGROUND;
	idScriptBool			AI_ONLADDER;
	idScriptBool			AI_DEAD;
	idScriptBool			AI_RUN;
	idScriptBool			AI_PAIN;
	idScriptBool			AI_HARDLANDING;
	idScriptBool			AI_SOFTLANDING;
	idScriptBool			AI_RELOAD;
	idScriptBool			AI_TELEPORT;
	idScriptBool			AI_TURN_LEFT;
	idScriptBool			AI_TURN_RIGHT;
	idScriptBool			AI_REALLYFALL; //HUMANHEAD rww

	// inventory
	hhInventory				inventory;			// HUMANHEAD - Changed to our version

	idEntityPtr<hhWeapon>	weapon;				// HUMANHEAD - Changed to our version
	idUserInterface *		hud;				// MP: is NULL if not local player

	int						weapon_fists;

	int						lastDmgTime;
	int						deathClearContentsTime;
	bool					doingDeathSkin;
	float					stamina;
	float					healthPool;			// amount of health to give over time
	int						nextHealthPulse;
	bool					healthPulse;
	bool					spiritPulse;		// HUMANHEAD pdm
	bool					healthTake;
	int						nextHealthTake;


	bool					hiddenWeapon;		// if the weapon is hidden ( in noWeapons maps )

	// mp stuff
	static idVec3			colorBarTable[ 5 ];
	int						spectator;
	idVec3					colorBar;			// used for scoreboard and hud display
	int						colorBarIndex;
	bool					scoreBoardOpen;
	bool					forceScoreBoard;
	bool					forceRespawn;
	bool					spectating;
	int						lastSpectateTeleport;
	bool					lastHitToggle;
	bool					forcedReady;
	bool					wantSpectate;		// from userInfo
	bool					weaponGone;			// force stop firing
	bool					useInitialSpawns;	// toggled by a map restart to be active for the first game spawn
	int						latchedTeam;		// need to track when team gets changed
	int						tourneyRank;		// for tourney cycling - the higher, the more likely to play next - server
	int						tourneyLine;		// client side - our spot in the wait line. 0 means no info.
	int						spawnedTime;		// when client first enters the game

	idEntityPtr<idEntity>	teleportEntity;		// while being teleported, this is set to the entity we'll use for exit
	int						teleportKiller;		// entity number of an entity killing us at teleporter exit
	bool					lastManOver;		// can't respawn in last man anymore (srv only)
	bool					lastManPlayAgain;	// play again when end game delay is cancelled out before expiring (srv only)
	bool					lastManPresent;		// true when player was in when game started (spectators can't join a running LMS)
	bool					isLagged;			// replicated from server, true if packets haven't been received from client.
	bool					isChatting;			// replicated from server, true if the player is chatting.

	// timers
	int						minRespawnTime;		// can respawn when time > this, force after g_forcerespawn
	int						maxRespawnTime;		// force respawn after this time

	// the first person view values are always calculated, even
	// if a third person view is used
	idVec3					firstPersonViewOrigin;
	idMat3					firstPersonViewAxis;

	idDragEntity			dragEntity;

	idEntityPtr<idEntity>	MPLastSpawnSpot;	//HUMANHEAD rww - does NOT need to be saved/restored, mp-only

public:
	CLASS_PROTOTYPE( idPlayer );

							idPlayer();
	virtual					~idPlayer();


#if GAMEPAD_SUPPORT	// VENOM BEGIN
	int						GetCurrentRumble(void) {return iCurrentRumble;}
#endif // VENOM END

	void					Spawn( void );
	virtual				// HUMANHEAD
	void					Think( void );

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void			Hide( void );
	virtual void			Show( void );

	virtual				// HUMANHEAD
	void					Init( void );
	void					PrepareForRestart( void );
	virtual void			Restart( void );
	virtual				// HUMANHEAD
	void					LinkScriptVariables( void );
	virtual				// HUMANHEAD
	void					SetupWeaponEntity( void );
	void					SelectInitialSpawnPoint( idVec3 &origin, idAngles &angles, idMat3 *axis = 0 ); //HUMANHEAD rww - added axis
	void					SpawnFromSpawnSpot( void );
	virtual				// HUMANHEAD
	void					SpawnToPoint( const idVec3	&spawn_origin, const idAngles &spawn_angles );
	void					SetClipModel( void );	// spectator mode uses a different bbox size

	void					SavePersistantInfo( void );
	virtual void			RestorePersistantInfo( void );
	void					SetLevelTrigger( const char *levelName, const char *triggerName );

	bool					UserInfoChanged( bool canModify );
	idDict *				GetUserInfo( void );
	bool					BalanceTDM( void );

	virtual				// HUMANHEAD
	void					CacheWeapons( void );

	void					EnterCinematic( void );
	void					ExitCinematic( void );
	bool					HandleESC( void );
	bool					SkipCinematic( void );

	virtual				// HUMANHEAD
	void					UpdateConditions( void );
	void					SetViewAngles( const idAngles &angles );

							// delta view angles to allow movers to rotate the view of the player
	virtual //HUMANHEAD
	void					UpdateDeltaViewAngles( const idAngles &angles );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;
	virtual void			GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos );
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
	void					CalcDamagePoints(  idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health, int *armor );
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

							// use exitEntityNum to specify a teleport with private camera view and delayed exit
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

	virtual				// HUMANHEAD
	void					Kill( bool delayRespawn, bool nodamage );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void					StartFxOnBone(const char *fx, const char *bone);

	renderView_t *			GetRenderView( void );
	virtual				//HUMANHEAD
	void					CalculateRenderView( void );	// called every tic by player code
	void					CalculateFirstPersonView( void );

	virtual				// HUMANHEAD
	void					DrawHUD( idUserInterface *hud );

	void					WeaponFireFeedback( const idDict *weaponDef );

	float					DefaultFov( void ) const;
	virtual				// HUMANHEAD
	float					CalcFov( bool honorZoom );
	void					CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis );
	idVec3					GetEyePosition( void ) const;
	virtual				// HUMANHEAD
	void					GetViewPos( idVec3 &origin, idMat3 &axis );	// HUMANHEAD
	virtual				// HUMANHEAD
	void					OffsetThirdPersonView( float angle, float range, float height, bool clip );

	virtual				// HUMANHEAD
	bool					Give( const char *statname, const char *value );
	virtual				// HUMANHEAD
	bool					GiveItem( idItem *item );
	void					GiveItem( const char *name );
	void					GiveHealthPool( float amt );
	
	virtual				// HUMANHEAD
	bool					GiveInventoryItem( idDict *item );
	void					RemoveInventoryItem( idDict *item );
	bool					GiveInventoryItem( const char *name );
	void					RemoveInventoryItem( const char *name );
	idDict *				FindInventoryItem( const char *name );

	int						SlotForWeapon( const char *weaponName );
	virtual				// HUMANHEAD
	void					Reload( void );
	virtual				// HUMANHEAD
	void					NextWeapon( void );
	virtual				// HUMANHEAD
	void					NextBestWeapon( void );
	virtual				// HUMANHEAD
	void					PrevWeapon( void );
	virtual				// HUMANHEAD
	void					SelectWeapon( int num, bool force );
	void					DropWeapon( bool died ) ;
	void					StealWeapon( idPlayer *player );
	void					AddProjectilesFired( int count );
	void					AddProjectileHits( int count );
	void					SetLastHitTime( int time );
	void					LowerWeapon( void );
	void					RaiseWeapon( void );
	void					WeaponLoweringCallback( void );
	void					WeaponRisingCallback( void );
	void					RemoveWeapon( const char *weap );
	bool					CanShowWeaponViewmodel( void ) const;

	void					AddAIKill( void );

	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );
	bool					GuiActive( void ) { return focusGUIent != NULL; }

	virtual				// HUMANHEAD
	void					PerformImpulse( int impulse );
	void					Spectate( bool spectate );
	void					ToggleScoreboard( void );
	void					RouteGuiMouse( idUserInterface *gui );
	virtual				// HUMANHEAD
	void					UpdateHud( idUserInterface *_hud );
	int						GetInfluenceLevel( void ) { return influenceActive; };
	void					SetPrivateCameraView( idCamera *camView, bool noHide = false );
	idCamera *				GetPrivateCameraView( void ) const { return privateCameraView; }
	void					StartFxFov( float duration  );
	virtual				// HUMANHEAD
	void					UpdateHudWeapon( bool flashWeapon = true );
	virtual				// HUMANHEAD
	void					UpdateHudStats( idUserInterface *hud );
	virtual				// HUMANHEAD
	void					UpdateHudAmmo( idUserInterface *hud );
	void					ShowTip( const char *keyMaterial, const char *tip, const char *key, const char *topMaterial, bool keywide, int xOffset=0, int yOffset=0 );
	void					HideTip( void );
	bool					IsTipVisible( void ) { return tipUp; };
	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	//HUMANHEAD rww - made virtual
	virtual void			WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg );
	//HUMANHEAD END

	//HUMANHEAD
	virtual bool			DoThirdPersonDeath(void);					// HUMANHEAD rww
	virtual	bool			IsDead() const { return (health <= 0); }	// HUMANHEAD cjr:  Deathwalk
	virtual idVec4			GetTeamColor();								// HUMANHEAD pdm
	bool					InDialogDamageMode() const { return bDialogDamageMode;	}
	//HUMANHEAD END

	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg &msg );

	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	bool					IsReady( void );
	bool					IsRespawning( void );
	bool					IsInTeleport( void );

	idEntity				*GetInfluenceEntity( void ) { return influenceEntity; };
	const idMaterial		*GetInfluenceMaterial( void ) { return influenceMaterial; };
	float					GetInfluenceRadius( void ) { return influenceRadius; };

	// server side work for in/out of spectate. takes care of spawning it into the world as well
	void					ServerSpectate( bool spectate );
	// for very specific usage. != GetPhysics()
	idPhysics				*GetPlayerPhysics( void );
	void					TeleportDeath( int killer );
	void					SetLeader( bool lead );
	bool					IsLeader( void );

	//HUMANHEAD rww
	void					SetPlayerModel( bool preserveChannels );
	void					SetSkin( const idDeclSkin *skin );
	const char				*GetModelPortraitName(bool bThumb = false);
	//HUMANHEAD END
	void					UpdateSkinSetup( bool restart );

	bool					OnLadder( void ) const;

	virtual	void			UpdatePlayerIcons( void );
	virtual	void			DrawPlayerIcons( void );
	virtual	void			HidePlayerIcons( void );
	bool					NeedsIcon( void );

	bool					SelfSmooth( void );
	void					SetSelfSmooth( bool b );

	idUserInterface *		ActiveGui( void ); //HUMANHEAD rww - made public
	void					ClearFocus( void ); //HUMANHEAD rww - made public

	//HUMANHEAD PCF rww 09/15/06 - female mp sounds
	bool					IsFemale(void);
	//HUMANHEAD END
protected:				// HUMANHEAD nla - need to be protected for access by hhPlayer
	jointHandle_t			hipJoint;
	jointHandle_t			chestJoint;
	jointHandle_t			headJoint;

	// HUMANHEAD - Changed to our version
	//idPhysics_Player		physicsObj;			// player physics
	hhPhysics_Player		physicsObj;			// player physics
	// HUMANHEAD END

	idList<aasLocation_t>	aasLocation;		// for AI tracking the player

	int						bobFoot;
	float					bobFrac;
	float					bobfracsin;
	int						bobCycle;			// for view bobbing and footstep generation
	float					xyspeed;
	int						stepUpTime;
	float					stepUpDelta;
	float					idealLegsYaw;
	float					legsYaw;
	bool					legsForward;
	float					oldViewYaw;
	idAngles				viewBobAngles;
	idVec3					viewBob;
	int						landChange;
	int						landTime;

	int						currentWeapon;
	int						idealWeapon;
	int						previousWeapon;
	int						weaponSwitchTime;
	bool					weaponEnabled;
	bool					showWeaponViewModel;

	const idDeclSkin *		skin;
	idStr					baseSkinName;

	int						numProjectilesFired;	// number of projectiles fired
	int						numProjectileHits;		// number of hits on mobs

	bool					airless;
	int						airTics;				// set to pm_airTics at start, drops in vacuum
	int						lastAirDamage;

	bool					gibDeath;
	bool					gibsLaunched;
	idVec3					gibsDir;

	idInterpolate<float>	zoomFov;
	idInterpolate<float>	centerView;
	bool					fxFov;

	float					influenceFov;
	int						influenceActive;		// level of influence.. 1 == no gun or hud .. 2 == 1 + no movement
	idEntity *				influenceEntity;
	const idMaterial *		influenceMaterial;
	float					influenceRadius;
	const idDeclSkin *		influenceSkin;

	idCamera *				privateCameraView;

	static const int		NUM_LOGGED_VIEW_ANGLES = 64;		// for weapon turning angle offsets
	idAngles				loggedViewAngles[NUM_LOGGED_VIEW_ANGLES];	// [gameLocal.framenum&(LOGGED_VIEW_ANGLES-1)]
	static const int		NUM_LOGGED_ACCELS = 16;			// for weapon turning angle offsets
	loggedAccel_t			loggedAccel[NUM_LOGGED_ACCELS];	// [currentLoggedAccel & (NUM_LOGGED_ACCELS-1)]
	int						currentLoggedAccel;

	// if there is a focusGUIent, the attack button will be changed into mouse clicks
	idEntity *				focusGUIent;
	idUserInterface *		focusUI;				// focusGUIent->renderEntity.gui, gui2, or gui3
	int						focusTime;
	idUserInterface *		cursor;
	
	// full screen guis track mouse movements directly
	int						oldMouseX;
	int						oldMouseY;

#if GAMEPAD_SUPPORT	// VENOM BEGIN
	int						iCurrentRumble;
#endif // VENOM END

	bool					tipUp;

	int						lastDamageDef;
	idVec3					lastDamageDir;
	int						lastDamageLocation;
	int						smoothedFrame;
	bool					smoothedOriginUpdated;
	idVec3					smoothedOrigin;
	idAngles				smoothedAngles;

	// mp
	bool					ready;					// from userInfo
	bool					respawning;				// set to true while in SpawnToPoint for telefrag checks
	bool					leader;					// for sudden death situations
	int						lastSpectateChange;
	int						lastTeleFX;
	unsigned int			lastSnapshotSequence;	// track state hitches on clients
	bool					weaponCatchup;			// raise up the weapon silently ( state catchups )
	int						MPAim;					// player num in aim
// HUMANHEAD pdm
	int						lastMPAim;
	int						lastMPAimTime;			// last time the aim changed
	int						MPAimFadeTime;			// for GUI fade
	bool					MPAimHighlight;
	bool					bDialogDamageMode;		// Disallow health from dropping below 1
	bool					bDialogWeaponMode;		// lock weapon during dialog
// HUMANHEAD END
	bool					isTelefragged;			// proper obituaries

	idPlayerIcon			playerIcon;

	hhPlayerTeamIcon		playerTeamIcon; //HUMANHEAD rww

	bool					selfSmooth;

	bool					bBufferNextSnapAngles; //HUMANHEAD rww

	void					LookAtKiller( idEntity *inflictor, idEntity *attacker );

	void					StopFiring( void );
	virtual				// HUMANHEAD
	void					FireWeapon( void );
	virtual				// HUMANHEAD
	void					Weapon_Combat( void );
	virtual				// HUMANHEAD
	void					Weapon_GUI( void );
	virtual				// HUMANHEAD
	void					UpdateWeapon( void );
	void					UpdateSpectating( void );
	void					SpectateFreeFly( bool force );	// ignore the timeout to force when followed spec is no longer valid
	void					SpectateCycle( void );
	virtual				// HUMANHEAD
	idAngles				GunTurningOffset( void );
	virtual				// HUMANHEAD
	idVec3					GunAcceleratingOffset( void );

	void					UseObjects( void );
	virtual				// HUMANHEAD
	void					CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );
	virtual				// HUMANHEAD
	void					BobCycle( const idVec3 &pushVelocity );
	virtual				// HUMANHEAD
	void					UpdateViewAngles( void );
	virtual				// HUMANHEAD
	void					EvaluateControls( void );
	void					AdjustSpeed( void );
	virtual				// HUMANHEAD
	void					AdjustBodyAngles( void );
	void					InitAASLocation( void );
	void					SetAASLocation( void );
	virtual				// HUMANHEAD
	void					Move( void );
	void					UpdateDeathSkin( bool state_hitch );
	void					SetSpectateOrigin( void );

	virtual				// HUMANHEAD
	void					UpdateFocus( void );
	virtual				// HUMANHEAD
	void					UpdateLocation( void );

	void					UseVehicle( void );

	void					Event_GetButtons( void );
	void					Event_GetMove( void );
	void					Event_GetViewAngles( void );
	void					Event_StopFxFov( void );
	void					Event_EnableWeapon( void );
	void					Event_DisableWeapon( void );
	void					Event_GetCurrentWeapon( void );
	void					Event_GetPreviousWeapon( void );
	void					Event_SelectWeapon( const char *weaponName );
	void					Event_GetWeaponEntity( void );
	void					Event_ExitTeleporter( void );
	void					Event_HideTip( void );
	void					Event_LevelTrigger( void );
	void					Event_Gibbed( void );
};

ID_INLINE bool idPlayer::IsReady( void ) {
	return ready || forcedReady;
}

ID_INLINE bool idPlayer::IsRespawning( void ) {
	return respawning;
}

ID_INLINE idPhysics* idPlayer::GetPlayerPhysics( void ) {
	return &physicsObj;
}

ID_INLINE bool idPlayer::IsInTeleport( void ) {
	return ( teleportEntity.GetEntity() != NULL );
}

ID_INLINE void idPlayer::SetLeader( bool lead ) {
	leader = lead;
}

ID_INLINE bool idPlayer::IsLeader( void ) {
	return leader;
}

ID_INLINE bool idPlayer::SelfSmooth( void ) {
	return selfSmooth;
}

ID_INLINE void idPlayer::SetSelfSmooth( bool b ) {
	selfSmooth = b;
}

#endif /* !__GAME_PLAYER_H__ */

