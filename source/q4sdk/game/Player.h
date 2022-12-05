// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 07/07/2004

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
extern const idEventDef EV_Player_SetViewAngles;
extern const idEventDef EV_Player_EnableWeapon;
extern const idEventDef EV_Player_DisableWeapon;
extern const idEventDef EV_Player_ExitTeleporter;
extern const idEventDef EV_Player_SelectWeapon;
extern const idEventDef EV_Player_Freeze;
extern const idEventDef EV_SpectatorTouch;
extern const idEventDef EV_Player_SetArmor;
extern const idEventDef EV_Player_SetExtraProjPassEntity;
extern const idEventDef EV_Player_DamageEffect;

const float THIRD_PERSON_FOCUS_DISTANCE	= 512.0f;
const int	LAND_DEFLECT_TIME			= 150;
const int	LAND_RETURN_TIME			= 300;
const int	FOCUS_TIME					= 200;
const int	FOCUS_GUI_TIME				= 300;
const int	FOCUS_USABLE_TIME			= 100;

const int	MAX_WEAPONS					= 16;
const int	MAX_AMMO					= 16;
const int	CARRYOVER_FLAG_AMMO			= 0x40000000;
const int	CARRYOVER_FLAG_ARMOR_LIGHT	= 0x20000000;
const int	CARRYOVER_FLAG_ARMOR_HEAVY	= 0x10000000;
const int	CARRYOVER_WEAPONS_MASK		= 0x0FFFFFFF;
const int	CARRYOVER_FLAGS_MASK		= 0xF0000000;

const int	MAX_SKILL_LEVELS			= 4;

const int	ZERO_VOLUME					= -40;			// volume at zero
const int	DMG_VOLUME					= 5;			// volume when taking damage
const int	DEATH_VOLUME				= 15;			// volume at death

const int	SAVING_THROW_TIME			= 5000;			// maximum one "saving throw" every five seconds

const int	ASYNC_PLAYER_INV_AMMO_BITS = idMath::BitsForInteger( 999 );	// 9 bits to cover the range [0, 999]
const int	ASYNC_PLAYER_INV_CLIP_BITS = -7;							// -7 bits to cover the range [-1, 60]
const int	ASYNC_PLAYER_INV_WPMOD_BITS = 3;							// 3 bits (max of 3 mods per gun)
// NOTE: protocol 69 used 6 bits, but that's only used for client -> server traffic, so doesn't affect backwards protocol replay compat
const int	IMPULSE_NUMBER_OF_BITS		= 8;							// allows for 2<<X impulses

#define MAX_CONCURRENT_VOICES	3

// RAVEN BEGIN
// jnewquist: Xenon weapon combo system
#ifdef _XENON
typedef struct
{
	int up;
	int down;
	int left;
	int right;
} nextWeaponCombo_t;
#endif
// RAVEN END

typedef enum {
	FOCUS_NONE,
	FOCUS_GUI,
	FOCUS_BRACKETS,
	FOCUS_VEHICLE,
	FOCUS_LOCKED_VEHICLE,
	FOCUS_USABLE,
	FOCUS_USABLE_VEHICLE,
	FOCUS_CHARACTER,
	FOCUS_MAX
} playerFocus_t;

struct idItemInfo {
	idStr name;
	idStr icon;
};

struct idObjectiveInfo {
	idStr title;
	idStr text;
	idStr screenshot;
};

struct idLevelTriggerInfo {
	idStr levelName;
	idStr triggerName;
};
/*
struct rvDatabaseEntry {
	idStr title;
	idStr text;
	idStr image;
	idStr filter;
};
*/
typedef struct {
	int		time;
	idVec3	dir;		// scaled larger for running
} loggedAccel_t;

typedef struct {
 	int		areaNum;
 	idVec3	pos;
} aasLocation_t;

// powerups
enum {
	// standard powerups
	POWERUP_QUADDAMAGE = 0, 
	POWERUP_HASTE,
	POWERUP_REGENERATION,
	POWERUP_INVISIBILITY,
		
	// ctf powerups
	POWERUP_CTF_MARINEFLAG,
	POWERUP_CTF_STROGGFLAG,
	POWERUP_CTF_ONEFLAG,

	// persistant powerups, keep these with ammo regen first and scout last or persistance breaks
	POWERUP_AMMOREGEN,
	POWERUP_GUARD,
	POWERUP_DOUBLER,
	POWERUP_SCOUT,	// == 1.2 / protocol 69's POWERUP_MAX-1

	POWERUP_MODERATOR, // Note: This has to be here.  Otherwise, it breaks syncronization with some list elsewhere
		
	POWERUP_DEADZONE,

	// Team Powerups
	POWERUP_TEAM_AMMO_REGEN,
	POWERUP_TEAM_HEALTH_REGEN,
	POWERUP_TEAM_DAMAGE_MOD,
	
	POWERUP_MAX
};

enum {
	PMOD_SPEED = 0,
	PMOD_PROJECTILE_DAMAGE,
	PMOD_MELEE_DAMAGE,
	PMOD_MELEE_DISTANCE,
	PMOD_PROJECTILE_DEATHPUSH,
	PMOD_FIRERATE,
	PMOD_MAX
};

typedef enum {
	PE_NONE,
	PE_GRAB_A,
	PE_GRAB_B,
	PE_SALUTE,
	PE_CHEER,
	PE_TAUNT
} playerEmote_t;

// influence levels
enum {
	INFLUENCE_NONE = 0,			// none
	INFLUENCE_LEVEL1,			// no gun or hud
	INFLUENCE_LEVEL2,			// no gun, hud, movement
	INFLUENCE_LEVEL3,			// slow player movement
};

typedef enum { 
	PTS_UNKNOWN = 0,
	PTS_ADVANCED,
	PTS_ELIMINATED,
	PTS_PLAYING,
	PTS_NUM_STATES
} playerTourneyStatus_t;

typedef enum {
	IBS_CAN_BUY = 0,
	IBS_NOT_ALLOWED = 1,
	IBS_ALREADY_HAVE = 2,
	IBS_CANNOT_AFFORD = 3,
} itemBuyStatus_t;

const int	ASYNC_PLAYER_TOURNEY_STATUS_BITS = idMath::BitsForInteger( PTS_NUM_STATES );

class idInventory {
public:
	int						maxHealth;
	int						weapons;
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	int						carryOverWeapons;
// RITUAL END
	int						powerups;
	int						armor;
	int						maxarmor;
	int						ammo[ MAX_AMMO ];
	int						clip[ MAX_WEAPONS ];
	int						powerupEndTime[ POWERUP_MAX ];
	int						weaponMods[ MAX_WEAPONS ];

 	// multiplayer
 	int						ammoPredictTime;
	int						ammoRegenStep[ MAX_WEAPONS ];
	int						ammoRegenTime[ MAX_WEAPONS ];
	int						ammoIndices[ MAX_WEAPONS ];
	int						startingAmmo[ MAX_WEAPONS ];

 	int						lastGiveTime;
 	
	idList<idDict *>		items;
	idStrList				pdas;
	idStrList				pdaSecurity;
	idStrList				videos;

	idList<idLevelTriggerInfo> levelTriggers;

							idInventory() { Clear(); }
							~idInventory() { Clear(); }

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void					Clear( void );
	void					GivePowerUp( idPlayer* player, int powerup, int msec );
	void					ClearPowerUps( void );
	void					GetPersistantData( idDict &dict );
	void					RestoreInventory( idPlayer *owner, const idDict &dict );
	bool					Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud, bool dropped = false, bool checkOnly = false );
	void					Drop( const idDict &spawnArgs, const char *weapon_classname, int weapon_index );
	int						AmmoIndexForAmmoClass( const char *ammo_classname ) const;
	int						MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const;
	int						AmmoIndexForWeaponClass( const char *weapon_classname, int *ammoRequired = NULL );
	const char *			AmmoClassForWeaponClass( const char *weapon_classname);

// RAVEN BEGIN
// mekberg: if the player can pick up the ammo at this time
	bool					DetermineAmmoAvailability( idPlayer* owner, const char *ammoName, int ammoIndex, int ammoAmount, int ammoMax );
// RAVEN END
	
	int						AmmoIndexForWeaponIndex( int weaponIndex );
	int						StartingAmmoForWeaponIndex( int weaponIndex );
	int						AmmoRegenStepForWeaponIndex( int weaponIndex );
	int						AmmoRegenTimeForWeaponIndex( int weaponIndex );

	int						HasAmmo( int index, int amount );
	bool					UseAmmo( int index, int amount );
	int						HasAmmo( const char *weapon_classname );			// looks up the ammo information for the weapon class first

	int						nextItemPickup;
	int						nextItemNum;
	int						onePickupTime;
	idList<idItemInfo>		pickupItemNames;
	idList<idObjectiveInfo>	objectiveNames;
//	idList<rvDatabaseEntry>	database;
	
	int						secretAreasDiscovered;
};

class idPlayer : public idActor {
public:

 	enum {
 		EVENT_IMPULSE = idEntity::EVENT_MAXEVENTS,
 		EVENT_EXIT_TELEPORTER,
 		EVENT_ABORT_TELEPORTER,
 		EVENT_POWERUP,
 		EVENT_SPECTATE,
		EVENT_EMOTE,
 		EVENT_MAXEVENTS
 	};

	friend class idThread;

	usercmd_t				usercmd;

	class idPlayerView		playerView;			// handles damage kicks and effects

	bool					alreadyDidTeamAnnouncerSound;
	bool					noclip;
	bool					godmode;
	int						godmodeDamage;
	bool					undying;

	bool					spawnAnglesSet;		// on first usercmd, we must set deltaAngles
	idAngles				spawnAngles;
	idAngles				viewAngles;			// player view angles
	idAngles				cmdAngles;			// player cmd angles

	int						buttonMask;
	int						oldButtons;
	int						oldFlags;

	int						lastHitTime;			// last time projectile fired by player hit target
	int						lastSavingThrowTime;	// for the "free miss" effect

	struct playerFlags_s {
		bool		forward			:1;
		bool		backward		:1;
		bool		strafeLeft		:1;
		bool		strafeRight		:1;
		bool		attackHeld		:1;
		bool		weaponFired		:1;
		bool		jump			:1;
		bool		crouch			:1;
		bool		onGround		:1;
		bool		onLadder		:1;
		bool		dead			:1;
		bool		run				:1;
		bool		pain			:1;
		bool		hardLanding		:1;
		bool		softLanding		:1;
		bool		reload			:1;
		bool		teleport		:1;
		bool		turnLeft		:1;
		bool		turnRight		:1;
		bool		hearingLoss		:1;
		bool		objectiveFailed	:1;
		bool		noFallingDamage :1;
	} pfl;
		
	// inventory
	idInventory				inventory;

	rvWeapon*						weapon;
	idEntityPtr<rvViewWeapon>		weaponViewModel;
	idEntityPtr<idAnimatedEntity>	weaponWorldModel;
	const idDeclEntityDef*			weaponDef;


 	idUserInterface *		hud;				// Common hud
	idUserInterface *		mphud;				// hud overlay containing MP elements
	
	idUserInterface *		objectiveSystem;
	idUserInterface *		cinematicHud;
	bool					objectiveSystemOpen;
	bool					objectiveButtonReleased;
	bool					disableHud;
	bool					showNewObjectives;

	int						lastDmgTime;
	int						deathClearContentsTime;
 	bool					doingDeathSkin;
	int						nextHealthPulse;	// time when health will tick down
	int						nextAmmoRegenPulse[ MAX_AMMO ];	// time when ammo will regenerate
	int						nextArmorPulse;		// time when armor will tick down
	bool					hiddenWeapon;		// if the weapon is hidden ( in noWeapons maps )

	// mp stuff
	int						spectator;

	bool					scoreBoardOpen;
	bool					forceScoreBoard;
	bool					forceRespawn;
	int						forceScoreBoardTime;
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	bool					allowedToRespawn;
// squirrel: Mode-agnostic buymenus
	bool					inBuyZone;
	bool					inBuyZonePrev;
// RITUAL END
	bool					spectating;
	bool					lastHitToggle;
	bool					lastArmorHit;
	bool					forcedReady;
	int						lastArenaChange;
	
	bool					wantSpectate;			// from userInfo
	bool					jumpDuringHitch;

 	bool					weaponGone;				// force stop firing
 	bool					useInitialSpawns;		// toggled by a map restart to be active for the first game spawn
 	bool					isLagged;				// replicated from server, true if packets haven't been received from client.
 	bool					isChatting;				// replicated from server, true if the player is chatting.

	int						lastSpectateTeleport;
	int						latchedTeam;			// need to track when team gets changed
 	int						spawnedTime;			// when client first enters the game
 	int						hudTeam;

 	idEntityPtr<idEntity>	teleportEntity;			// while being teleported, this is set to the entity we'll use for exit
	int						teleportKiller;			// entity number of an entity killing us at teleporter exit

	idEntityPtr<idPlayer>	lastKiller;

	// timers
	int						minRespawnTime;			// can respawn when time > this, force after g_forcerespawn
	int						maxRespawnTime;			// force respawn after this time

	// the first person view values are always calculated, even
	// if a third person view is used
	idVec3					firstPersonViewOrigin;
	idMat3					firstPersonViewAxis;

	idDragEntity			dragEntity;
	idVec3					intentDir;

	rvAASTacticalSensor*	aasSensor;

	idEntityPtr<idEntity>	extraProjPassEntity;
	
	bool					vsMsgState;

	int						lastPickupTime;
//RAVEN BEGIN
// asalmon: the eneny the player is most likely currently aiming at
#ifdef _XBOX
	idEntityPtr<idActor>	bestEnemy;
#endif
//RAVEN END

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	float					buyMenuCash;
// RITUAL END

public:
	CLASS_PROTOTYPE( idPlayer );

							idPlayer();
	virtual					~idPlayer();

	void					Spawn( void );
	void					Think( void );

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	static const char*		GetSpawnClassname ( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	void					Init( void );
 	void					PrepareForRestart( void );
 	virtual void			Restart( void );
	void					SetWeapon ( int weapon );
	void					SetupWeaponEntity( void );
	bool					SelectSpawnPoint( idVec3 &origin, idAngles &angles );
	void					SpawnFromSpawnSpot( void );
	void					SpawnToPoint( const idVec3	&spawn_origin, const idAngles &spawn_angles );
	void					SetClipModel( bool forceSpectatorBBox = false );	// spectator mode uses a different bbox size

	void					SavePersistantInfo( void );
	void					RestorePersistantInfo( void );
	void					SetLevelTrigger( const char *levelName, const char *triggerName );

 	bool					UserInfoChanged( void );
	idDict *				GetUserInfo( void );
// RAVEN BEGIN
// shouchard:  BalanceTDM->BalanceTeam (now used for CTF as well as TDM)
	bool					BalanceTeam( void );
// RAVEN END
	void					CacheWeapons( void );

 	bool					HandleESC( void );
   	void					EnterCinematic( void );
   	void					ExitCinematic( void );
 	bool					SkipCinematic( void );

	void					UpdateConditions( void );
	void					SetViewAngles( const idAngles &angles );

	void					BiasIntentDir			( idVec3 newIntentDir, float prevBias = 199.0f );

							// delta view angles to allow movers to rotate the view of the player
	void					UpdateDeltaViewAngles( const idAngles &angles );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

 	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;
 	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
	void					CalcDamagePoints(  idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health, int *armor );
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual bool			CanPlayImpactEffect ( idEntity* attacker, idEntity* target );
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );
	virtual void			ProjectHeadOverlay( const idVec3 &point, const idVec3 &dir, float size, const char *decal );
 							// use exitEntityNum to specify a teleport with private camera view and delayed exit
 	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

	virtual void			GetDebugInfo ( debugInfoProc_t proc, void* userData );

	virtual bool			CanDamage( const idVec3 &origin, idVec3 &damagePoint, idEntity *ignoreEnt );

 	void					Kill( bool delayRespawn, bool nodamage );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	renderView_t *			GetRenderView( void );
	void					SmoothenRenderView( bool firstPerson );
	void					CalculateRenderView( void );	// called every tic by player code
	void					CalculateFirstPersonView( void );
	
	void					DrawShadow( renderEntity_t *headRenderEnt );
	void					DrawHUD( idUserInterface *hud );
	void					StartRadioChatter ( void );
	void					StopRadioChatter ( void );

	void					WeaponFireFeedback( const idDict *weaponDef );

 	float					DefaultFov( void ) const;
 	float					CalcFov( bool honorZoom );
	void					CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis );
	void					GetViewPos( idVec3 &origin, idMat3 &axis ) const;
 	void					OffsetThirdPersonView( float angle, float range, float height, bool clip );
// RAVEN BEGIN
// jnewquist: option to avoid clipping against world
	void					OffsetThirdPersonVehicleView	( bool clip );
// RAVEN END
	bool					OffsetThirdPersonTargetView		( void );

	bool					Give( const char *statname, const char *value, bool dropped = false );
	bool					GiveItem( idItem *item );
	void					GiveItem( const char *name );
	
	// Inventory
	bool					GiveInventoryItem( idDict *item );
	void					RemoveInventoryItem( idDict *item );
	bool					GiveInventoryItem( const char *name );
	void					RemoveInventoryItem( const char *name );
	idDict *				FindInventoryItem( const char *name );

	// Wrist computer
	void					GiveObjective				( const char *title, const char *text, const char *screenshot );
	void					CompleteObjective			( const char *title );
	void					FailObjective				( const char *title );
	void					GiveDatabaseEntry			( const idDict* dbEntry, bool hudPopup = true );
	bool					IsObjectiveUp				( void ) const { return objectiveUp; }
	idUserInterface *		GetObjectiveHud				( void ) { return objectiveSystem; }

	// Secret Areas
	void					DiscoverSecretArea			( const char *description);
	
	void					StartBossBattle				( idEntity* ent );

	// Powerups
	bool					GivePowerUp					( int powerup, int time, bool team = false );
	void					ClearPowerUps				( void );

	void					StartPowerUpEffect			( int powerup );
	void					StopPowerUpEffect			( int powerup );
	
	bool					PowerUpActive				( int powerup ) const;
	float					PowerUpModifier				( int type );
	void					ClearPowerup				( int i );
	const char*				GetArenaPowerupString		( void );

	// Helper methods to retrieving dictionaries
	const idDeclEntityDef*	GetWeaponDef				( int weaponIndex );
	const idDeclEntityDef*	GetPowerupDef				( int powerupIndex );

	// Weapons
	bool					GiveWeaponMods				( int mods );
	bool					GiveWeaponMods				( int weapon, int mods );
	void					GiveWeaponMod				( const char* weaponmod );

	int						SlotForWeapon				( const char *weaponName );

	idEntity*				DropItem					( const char* itemClass, const idDict& customArgs, const idVec3& velocity = vec3_origin ) const; 
	void					DropPowerups				( void );
	idEntity*				ResetFlag					( const char* itemClass, const idDict& customArgs ) const;
	void					RespawnFlags				( void );
	void					DropWeapon					( void );

// RAVEN BEGIN
// abahr:
	bool					WeaponIsEnabled				( void ) const { return weaponEnabled; }
	void					ShowCrosshair				( void );
	void					HideCrosshair				( void );
// RAVEN END

//RAVEN BEGIN
//asalmon: switch weapon based on d-pad combo
#ifdef _XBOX
	void					ScheduleWeaponSwitch		(int weapon);
#endif
//RAVEN BEGIN

	void					Reload						( void );
	void					NextWeapon					( void );
	void					NextBestWeapon				( void );
	void					PrevWeapon					( void );
	void					LastWeapon					( void );
 	void					SelectWeapon				( int num, bool force );
	void					SelectWeapon				( const char * );
	void					AddProjectilesFired			( int count );
	void					AddProjectileHits			( int count );
	void					SetLastHitTime				( int time, bool armorHit );
 	void					LowerWeapon					( void );
 	void					RaiseWeapon					( void );
 	void					WeaponLoweringCallback		( void );
 	void					WeaponRisingCallback		( void );
	void					RemoveWeapon				( const char *weap );
	void					Flashlight					( bool on );
	void					ToggleFlashlight			( void );
 	bool					CanShowWeaponViewmodel		( void ) const;

	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );
	bool					GuiActive( void ) { return focusType == FOCUS_GUI; }

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	void					GenerateImpulseForBuyAttempt( const char* itemName );
	bool					AttemptToBuyItem( const char* itemName );
	bool					AttemptToBuyTeamPowerup( const char* itemName );
	void					UpdateTeamPowerups( bool isBuying = false );
	bool					CanBuy( void );
	int						CanSelectWeapon				( const char* weaponName );
	int						GetItemCost(const char* itemName);
// RITUAL END
	void					PerformImpulse( int impulse );
	void					Spectate( bool spectate, bool force = false );
 	void					ToggleObjectives ( void );
 	void					ToggleScoreboard( void );
	void					RouteGuiMouse( idUserInterface *gui );
 	void					UpdateHud( void );
	idUserInterface*		GetHud();
	const idUserInterface*	GetHud() const;
	void					SetInfluenceFov( float fov );
 	void					SetInfluenceView( const char *mtr, const char *skinname, float radius, idEntity *ent );
	void					SetInfluenceLevel( int level );
 	int						GetInfluenceLevel( void ) { return influenceActive; };
	void					SetPrivateCameraView( idCamera *camView );
	idCamera *				GetPrivateCameraView( void ) const { return privateCameraView; }
	void					StartFxFov( float duration  );
 	void					UpdateHudWeapon( int displayWeapon=-1 );
#ifdef _XENON
	void					ResetHUDWeaponSwitch( void );
#endif
	void					UpdateHudStats( idUserInterface *hud );
 	void					UpdateHudAmmo( idUserInterface *hud );
 	void					ShowTip( const char *title, const char *tip, bool autoHide );
 	void					HideTip( void );
 	bool					IsTipVisible( void ) { return tipUp; };
	void					ShowObjective( const char *obj );
 	void					HideObjective( void );
	idVec3					GetEyePosition( void ) const;

	void					LocalClientPredictionThink( void );
	void					NonLocalClientPredictionThink( void );
	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	void					WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg );

	virtual bool			ClientStale( void );
	virtual void			ClientUnstale( void );

	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg &msg );

	virtual bool			GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	
	bool					IsBeingTalkedTo	( void );
 	bool					IsReady			( void );
 	bool					IsRespawning	( void );
 	bool					IsInTeleport	( void );
	bool					IsZoomed		( void );
	bool					IsFlashlightOn	( void );
	virtual bool			IsCrouching		( void ) const;
	
	// voice com muting
	bool					IsPlayerMuted	( idPlayer* player ) const;
	bool					IsPlayerMuted	( int clientNum ) const;
	void					MutePlayer		( idPlayer* player, bool mute );
	void					MutePlayer		( int clientNum, bool mute );
	

	// buddy list
	void					SetFriend		( idPlayer* player, bool isFriend );
	void					SetFriend		( int clientNum, bool isFriend );
	bool					IsFriend		( idPlayer* player ) const;
	bool					IsFriend		( int clientNum ) const;

	// time joined server
	int						GetConnectTime	( void ) const;
	void					SetConnectTime	( int time );

	// emotes
	void					SetEmote		( playerEmote_t emote );
    
	// rankings
	int						GetRank			( void ) const;
	void					SetRank			( int newRank );

	// arenas for tourney mode
	int						GetArena		( void ) const;
	void					SetArena		( int newArena );

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

 	void					UpdateModelSetup( bool forceReload = false );

	bool					OnLadder( void ) const;

	rvViewWeapon*			GetWeaponViewModel	( void ) const;
	idAnimatedEntity*		GetWeaponWorldModel ( void ) const;
	int						GetCurrentWeapon	( void ) const;

	bool					IsGibbed			( void ) const;
	const idVec3&			GetGibDir			( void ) const;

	void					SetInitialHud ( void );

	void					RemoveClientModel ( const char* entityDefName );
	void					RemoveClientModels ( void );
	
	rvClientEntityPtr<rvClientModel> AddClientModel ( const char* entityDefName, const char* shaderName = NULL );

	void					ClientGib			( const idVec3& dir );
	void					ClientDamageEffects ( const idDict& damageDef, const idVec3& dir, int damage );


	void					ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash = false );

	void					GUIMainNotice( const char* message, bool persist = false );
	void					GUIFragNotice( const char* message, bool persist = false );

	virtual bool			EnterVehicle		( idEntity* vehicle );
	virtual bool			ExitVehicle			( bool force = false );

	virtual void			SetClipWorld( int newCW );
	virtual int				GetClipWorld( void ) const;

	virtual idEntity*		GetGroundElevator( idEntity* testElevator=NULL ) const;
	
	int						GetWeaponIndex( const char* weaponName ) const;

	virtual void			SetInstance( int newInstance );
	void					JoinInstance( int newInstance );

	void					ClientInstanceJoin( void );
	void					ClientInstanceLeave( void );

	void					SetHudOverlay( idUserInterface* overlay, int duration );

	void					SetShowHud( bool showHud );
	bool					GetShowHud( void );


	// mekberg: wrap saveMessages
	void					SaveMessage( void );

	// mekberg: set pm_ cvars
	void					SetPMCVars( void );

	void					SetTourneyStatus( playerTourneyStatus_t newStatus ) { tourneyStatus = newStatus; }
	playerTourneyStatus_t	GetTourneyStatus( void ) { return tourneyStatus; }
	const char*				GetTextTourneyStatus( void );

	const idVec4&			GetHitscanTint( void );

	void					ForceScoreboard( bool force, int time );

	// call only on the local player
	idUserInterface*		GetCursorGUI( void );

	bool					CanFire( void ) const;

	bool					AllowedVoiceDest( int from );

// RITUAL BEGIN
// squirrel: added DeadZone multiplayer
	itemBuyStatus_t			ItemBuyStatus( const char* itemName );
	bool					CanBuyItem( const char* itemName );
	void					GiveCash( float cashDeltaAmount );
	void					ClampCash( float minCash, float maxCash );
	void					SetCash( float newCashAmount );
	void					ResetCash();
// RITUAL END

protected:
	void					SetupHead( const char* modelKeyName = "", idVec3 headOffset = idVec3(0, 0, 0) );

private:
	float					vehicleCameraDist;

	jointHandle_t			hipJoint;
	jointHandle_t			chestJoint;

	idPhysics_Player		physicsObj;			// player physics

 	idList<aasLocation_t>	aasLocation;		// for AI tracking the player

	idStr					modelName;			// current model name
	const rvDeclPlayerModel* modelDecl;

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
	
	// ddynerman: we read fall deltas from spawnargs, cache them to save some lookups
	float					fatalFallDelta;
	float					hardFallDelta;
	float					softFallDelta;
	float					noFallDelta;

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	int						carryOverCurrentWeapon;
// RITUAL END
	int						currentWeapon;
	int						idealWeapon;
	int						previousWeapon;
	int						weaponSwitchTime;
	bool					weaponEnabled;
 	bool					showWeaponViewModel;

	rvClientEntityPtr<rvClientAFAttachment>	clientHead;
// RAVEN BEGIN
// mekberg: allow disabling of objectives during non-cinematic time periods
	bool					objectivesEnabled;
// jshepard: false when player is looking at an npc or gui
	bool					flagCanFire;
// RAVEN END

	bool					flashlightOn;
	bool					zoomed;

	bool					reloadModel;

	const idDeclSkin *		skin;
	const idDeclSkin *		weaponViewSkin;
	const idDeclSkin *		headSkin;

	const idDeclSkin *		powerUpSkin;			// active powerup skin
	const idDeclSkin *		gibSkin;

	const idMaterial*		quadOverlay;
	const idMaterial*		hasteOverlay;
	const idMaterial*		regenerationOverlay;
	const idMaterial*		invisibilityOverlay;
	const idMaterial*		powerUpOverlay;

	int						numProjectilesFired;	// number of projectiles fired
	int						numProjectileHits;		// number of hits on mobs

	bool					airless;
	int						airTics;				// set to pm_airTics at start, drops in vacuum
	int						lastAirDamage;

	bool					gibDeath;
	bool					gibsLaunched;
	idVec3					gibDir;

	playerTourneyStatus_t	tourneyStatus;
	bool					isStrogg;

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

	int						demoViewAngleTime;
	idAngles				demoViewAngles;

	// if there is a focusGUIent, the attack button will be changed into mouse clicks
	idUserInterface *		focusUI;
	int						focusTime;
	playerFocus_t			focusType;
	idEntityPtr<idEntity>	focusEnt;
	idUserInterface *		focusBrackets;	
	int						focusBracketsTime;

	bool					targetFriendly;

	idEntityPtr<idAI>		talkingNPC;				// NPC who's currently talking to us
 	int						talkCursor;				// show the state of the focusCharacter (0 == can't talk/dead, 1 == ready to talk, 2 == busy talking)
	idUserInterface *		cursor;

	idUserInterface *		overlayHud;			// a temporary hud overlay
	int						overlayHudTime;
	
	// full screen guis track mouse movements directly
	int						oldMouseX;
	int						oldMouseY;

	bool					tipUp;
	bool					objectiveUp;

	float					dynamicProtectionScale;	// value to scale damage by due to dynamic protection
	int						lastDamageDef;
	idVec3					lastDamageDir;
	int						lastDamageLocation;

	int						predictedFrame;
	idVec3					predictedOrigin;
	idAngles				predictedAngles;
	bool					predictedUpdated;
	idVec3					predictionOriginError;
	idAngles				predictionAnglesError;
	int						predictionErrorTime;

	// mp
 	bool					ready;					// from userInfo
 	bool					respawning;				// set to true while in SpawnToPoint for telefrag checks
 	bool					leader;					// for sudden death situations
 	bool					weaponCatchup;			// raise up the weapon silently ( state catchups )
	bool					isTelefragged;			// proper obituaries
	
	int						lastSpectateChange;
 	int						lastTeleFX;
 	unsigned int			lastSnapshotSequence;	// track state hitches on clients
 	
	int						aimClientNum;			// player num in aim

	idPlayer*				lastImpulsePlayer;		// the last player who gave me an impulse, may be null
	
	int						arena;					// current arena for tourney gameplay

	int						connectTime;
	int						mutedPlayers;			// bitfield set to which clients this player wants muted
	int						friendPlayers;			// bitfield set to which clients this player has marked as friends
	
	int						voiceDest[MAX_CONCURRENT_VOICES];
	int						voiceDestTimes[MAX_CONCURRENT_VOICES];

	int						rank;

	int						deathSkinTime;
	bool					deathStateHitch;

	playerEmote_t			emote;

	int						powerupEffectTime;
	const idDecl			*powerupEffect;
	int						powerupEffectType;
	idList<jointHandle_t>	powerupEffectJoints;
	rvClientEffectPtr		hasteEffect;
	rvClientEffectPtr		flagEffect;
	rvClientEffectPtr		arenaEffect;

	rvClientEffectPtr		teamHealthRegen;
	bool					teamHealthRegenPending;
	rvClientEffectPtr		teamAmmoRegen;
	bool					teamAmmoRegenPending;
	rvClientEffectPtr		teamDoubler;
	bool					teamDoublerPending;

	idVec4					hitscanTint;
	// end mp

	int						lastImpulseTime;		// time of last impulse
	idEntityPtr<idEntity>	bossEnemy;

	const idDeclEntityDef*	cachedWeaponDefs[ MAX_WEAPONS ];
	const idDeclEntityDef*	cachedPowerupDefs[ POWERUP_MAX ];

	bool					weaponChangeIconsUp;

	int						oldInventoryWeapons;

	const idDeclEntityDef*	itemCosts;

	bool					WantSmoothing( void ) const;
	void					PredictionErrorDecay( void );

	bool					CanZoom(void);

	void					LookAtKiller( idEntity *inflictor, idEntity *attacker );

	void					StopFiring( void );
	void					FireWeapon( void );
	void					Weapon_Combat( void );
	void					Weapon_NPC( void );
	void					Weapon_GUI( void );
	void					Weapon_Vehicle ( void );
	void					Weapon_Usable ( void );
	void					SpectateFreeFly( bool force );	// ignore the timeout to force when followed spec is no longer valid
	void					SpectateCycle( void );
	idAngles				GunTurningOffset( void );
	idVec3					GunAcceleratingOffset( void );

	void					CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );
	void					BobCycle( const idVec3 &pushVelocity );
	void					EvaluateControls( void );
	void					AdjustSpeed( void );
	void					AdjustBodyAngles( void );
	void					Move( void );
	void					SetSpectateOrigin( void );

 	void					InitAASLocation( void );
 	void					SetAASLocation( void );

	idUserInterface *		ActiveGui( void );

	void					UpdateWeapon				( void );
	void					UpdateSpectating			( void );
	void					UpdateAir					( void );
// RAVEN BEGIN
// abahr
	void					UpdateGravity				( void );
// nrausch: common handling for objective screen toggle	
	void					HandleObjectiveInput		( void );
	void					HandleCheats				( void );
	void					ClearCheatState				( void );
// RAVEN END
	void					UpdateViewAngles			( void );
	void					UpdatePowerUps				( void );
 	void					UpdateDeathSkin				( bool state_hitch );
	void					UpdateFocus					( void );
 	void					UpdateLocation				( void );
	void					UpdateObjectiveInfo			( void );
//	void					UpdateDatabaseInfo			( void );
	void					UpdateIntentDir				( void );	

	void					LoadDeferredModel			( void );

	void					ClearFocus					( void );
	void					UpdateFocusCharacter		( idEntity* newEnt );
	void					SetFocus					( playerFocus_t type, int focusTime, idEntity* ent, idUserInterface* ui );

	void					Event_GetButtons			( void );
	void					Event_GetMove				( void );
	void					Event_GetViewAngles			( void );
	void					Event_SetViewAngles			( const idVec3 &vec );
	void					Event_StopFxFov				( void );
	void					Event_EnableWeapon			( void );
	void					Event_DisableWeapon			( void );
	void					Event_GetCurrentWeapon		( void );
	void					Event_GetPreviousWeapon		( void );
	void					Event_SelectWeapon			( const char *weaponName );
	void					Event_GetWeaponEntity		( void );
	void					Event_ExitTeleporter		( void );
	void					Event_HideTip				( void );
	void					Event_LevelTrigger			( void );
	void					Event_GetViewPos			( void );
	void					Event_TeleportPlayer		( idVec3 &newPos, idVec3 &newAngles );
	void					Event_Freeze				( float f );
	void					Event_HideDatabaseEntry		( void );	
	void					Event_ZoomIn				( void );
	void					Event_ZoomOut				( void );
	void					Event_FinishHearingLoss		( float fadeTime );
	void					Event_GetAmmoData			( const char *ammoClass );
	void					Event_RefillAmmo			( void );
	void					Event_AllowFallDamage		( int toggle );
	
	void					Event_EnableTarget			( void );
	void					Event_DisableTarget			( void );
	virtual void			Event_DamageOverTimeEffect	( int endTime, int interval, const char *damageDefName );
	
	// RAVEN BEGIN
	// twhitaker: added Event_ApplyImpulse
	void					Event_ApplyImpulse			( idEntity* ent, idVec3 &point, idVec3 &impulse	);

	// mekberg:	added sethealth
	void					Event_SetHealth					( float newHealth );
	void					Event_SetArmor					( float newArmor );

	void					Event_SetExtraProjPassEntity( idEntity* _extraProjPassEntity );
	void					Event_DamageEffect			( const char *damageDefName, idEntity* _damageFromEnt  );

	// mekberg: allow enabling/disabling of objectives
	void					Event_EnableObjectives		( void );
	void					Event_DisableObjectives		( void );

	//  mekberg: don't supress showing new objectives anymore
	void					Event_AllowNewObjectives	( void );

	// twhitaker: death shader
	void					UpdateDeathShader			( bool state_hitch );
	
	bool doInitWeapon;
	void					InitWeapon			( void );
	// RAVEN END

	bool					IsLegsIdle						( bool crouching ) const;
	
	stateResult_t			State_Wait_Alive				( const stateParms_t& parms );
	stateResult_t			State_Wait_ReloadAnim			( const stateParms_t& parms );
	
	stateResult_t			State_Torso_Idle				( const stateParms_t& parms );
	stateResult_t			State_Torso_IdleThink			( const stateParms_t& parms );
	stateResult_t			State_Torso_Teleport			( const stateParms_t& parms );
	stateResult_t			State_Torso_RaiseWeapon			( const stateParms_t& parms );
	stateResult_t			State_Torso_LowerWeapon			( const stateParms_t& parms );
	stateResult_t			State_Torso_Fire				( const stateParms_t& parms );
	stateResult_t			State_Torso_Fire_Windup			( const stateParms_t& parms );

	stateResult_t			State_Torso_Reload				( const stateParms_t& parms );
	stateResult_t			State_Torso_Pain				( const stateParms_t& parms );
	stateResult_t			State_Torso_Dead				( const stateParms_t& parms );
	stateResult_t			State_Torso_Emote				( const stateParms_t& parms );
	
	stateResult_t			State_Legs_Idle					( const stateParms_t& parms );
	stateResult_t			State_Legs_Run_Forward			( const stateParms_t& parms );
	stateResult_t			State_Legs_Run_Backward			( const stateParms_t& parms );
	stateResult_t			State_Legs_Run_Left				( const stateParms_t& parms );
	stateResult_t			State_Legs_Run_Right			( const stateParms_t& parms );
	stateResult_t			State_Legs_Walk_Forward			( const stateParms_t& parms );
	stateResult_t			State_Legs_Walk_Backward		( const stateParms_t& parms );
	stateResult_t			State_Legs_Walk_Left			( const stateParms_t& parms );
	stateResult_t			State_Legs_Walk_Right			( const stateParms_t& parms );
	stateResult_t			State_Legs_Crouch				( const stateParms_t& parms );
	stateResult_t			State_Legs_Uncrouch				( const stateParms_t& parms );
	stateResult_t			State_Legs_Crouch_Idle			( const stateParms_t& parms );
	stateResult_t			State_Legs_Crouch_Forward		( const stateParms_t& parms );
	stateResult_t			State_Legs_Crouch_Backward		( const stateParms_t& parms );
	stateResult_t			State_Legs_Jump					( const stateParms_t& parms );
	stateResult_t			State_Legs_Fall					( const stateParms_t& parms );
	stateResult_t			State_Legs_Land					( const stateParms_t& parms );
	stateResult_t			State_Legs_Dead					( const stateParms_t& parms );
	
 	CLASS_STATES_PROTOTYPE( idPlayer );
};

ID_INLINE bool idPlayer::IsBeingTalkedTo( void ) {
	return talkingNPC!=NULL;
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

ID_INLINE bool idPlayer::IsZoomed( void ) {
	return zoomed;
}

ID_INLINE bool idPlayer::IsFlashlightOn( void ) {
	return flashlightOn;
}

ID_INLINE rvViewWeapon* idPlayer::GetWeaponViewModel( void ) const {
	return weaponViewModel;
}

ID_INLINE idAnimatedEntity* idPlayer::GetWeaponWorldModel( void ) const {
	return weaponWorldModel;
}

ID_INLINE int idPlayer::GetCurrentWeapon( void ) const {
	return currentWeapon;
}

ID_INLINE bool idPlayer::IsGibbed( void ) const {
	return gibDeath;
}

ID_INLINE const idVec3& idPlayer::GetGibDir( void ) const {
	return gibDir;
}

ID_INLINE int idPlayer::GetClipWorld( void ) const {
	return clipWorld;
}

ID_INLINE void idPlayer::SetClipWorld( int newCW ) {
	idEntity::SetClipWorld( newCW );
	
	if( head.GetEntity() ) {
		head.GetEntity()->SetClipWorld( newCW );

		head.GetEntity()->UnlinkCombat();
		head.GetEntity()->LinkCombat();
	}

	if( weapon ) {
		if( weapon->GetViewModel() ) {
			weapon->GetViewModel()->SetClipWorld( newCW );
		}

		if( weapon->GetWorldModel() ) {
			weapon->GetWorldModel()->SetClipWorld( newCW );
		}
	}

	UnlinkCombat();
	LinkCombat();
}

ID_INLINE int idPlayer::GetWeaponIndex( const char* weaponName ) const {
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		if( !idStr::Icmp( spawnArgs.GetString( va( "def_weapon%d", i ), "" ), weaponName ) ) {
			return i;
		}
	}

	return 0;
}

ID_INLINE idUserInterface* idPlayer::GetHud() {
	return vehicleController.IsDriving() ? vehicleController.GetHud() : hud;
}

ID_INLINE const idUserInterface* idPlayer::GetHud() const {
	return vehicleController.IsDriving() ? vehicleController.GetHud() : hud;
}

ID_INLINE bool idPlayer::IsPlayerMuted( idPlayer* player ) const {
	return !!( mutedPlayers & ( 1 << player->entityNumber ) );
}

ID_INLINE bool idPlayer::IsPlayerMuted( int clientNum ) const {
	return !!( mutedPlayers & ( 1 << clientNum ) );
}

ID_INLINE void idPlayer::MutePlayer( idPlayer* player, bool mute ) {
	if( mute ) {
		mutedPlayers |= ( 1 << player->entityNumber );
	} else {
		mutedPlayers &= ~( 1 << player->entityNumber );
	}
}

ID_INLINE void idPlayer::MutePlayer( int clientNum, bool mute ) {
	if( mute ) {
		mutedPlayers |= ( 1 << clientNum );
	} else {
		mutedPlayers &= ~( 1 << clientNum );
	}
}

ID_INLINE bool idPlayer::IsFriend( idPlayer* player ) const {
	return !!( friendPlayers & ( 1 << player->entityNumber ) );
}

ID_INLINE bool idPlayer::IsFriend( int clientNum ) const {
	return !!( friendPlayers & ( 1 << clientNum ) );
}

ID_INLINE void idPlayer::SetFriend( idPlayer* player, bool isFriend ) {
	if( isFriend ) {
		friendPlayers |= ( 1 << player->entityNumber ); 
	} else {
		friendPlayers &= ~( 1 << player->entityNumber );
	}
}

ID_INLINE void idPlayer::SetFriend( int clientNum, bool isFriend ) {
	if( isFriend ) {
		friendPlayers |= ( 1 << clientNum );
	} else {
		friendPlayers &= ~( 1 << clientNum );
	}
}

ID_INLINE int idPlayer::GetConnectTime( void ) const {
	return connectTime;
}

ID_INLINE void idPlayer::SetConnectTime( int time ) {
	connectTime = time;
}

ID_INLINE int idPlayer::GetRank( void ) const {
	return rank;
}

ID_INLINE void idPlayer::SetRank( int newRank ) {
	rank = newRank;
}

ID_INLINE int idPlayer::GetArena( void ) const {
	return arena;
}

ID_INLINE bool idPlayer::CanFire( void ) const {
	return flagCanFire;
}

#endif /* !__GAME_PLAYER_H__ */

// RAVEN END
