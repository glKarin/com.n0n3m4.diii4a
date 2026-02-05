/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

#include "idlib/math/Interpolate.h"

#include "physics/Physics_Player.h"
#include "Item.h"
#include "Actor.h"
#include "Weapon.h"
#include "Projectile.h"
#include "PlayerIcon.h"
#include "GameEdit.h"
#include "ui/DeviceContext.h"
#include "Mover.h"


#include "Misc.h"


class idAI;
class idFuncMountedObject;

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
const int	FOCUS_TIME = 300;
const int	FOCUS_GUI_TIME = 100; //when cursor exits the gui, how long to retain focus on the gui (was 500)

const int MAX_WEAPONS = 64;


const int DEAD_HEARTRATE = 0;			// fall to as you die
const int LOWHEALTH_HEARTRATE_ADJ = 20; //
const int DYING_HEARTRATE = 30;			// used for volumen calc when dying/dead
const int BASE_HEARTRATE = 70;			// default
const int ZEROSTAMINA_HEARTRATE = 115;  // no stamina
const int MAX_HEARTRATE = 130;			// maximum
const int ZERO_VOLUME = -40;			// volume at zero
const int DMG_VOLUME = 5;				// volume when taking damage
const int DEATH_VOLUME = 15;			// volume at death


//BC
const int SAVING_THROW_TIME = 3000;		// maximum one "saving throw" every X seconds

const int SUSPICIONARROW_COUNT = 32;
const int DAMAGEARROW_COUNT = 16;
const int NOISEEVENT_COUNT = 32;

const float QUICKTHROW_OFFSET_RIGHT = 4.0f; //When throwing weapon, offset it to camera right this many units.
const float QUICKTHROW_UPWARD_DIR = .2f;
const float QUICKTHROW_POWER = 1000.0f;
const float QUICKTHROW_POWER_ZEROG = 500.0f;
const float QUICKTHROW_POWER_BROKENITEM = 300.0f;

// SW: This value controls the rate at which the 'physics' update on the throwing prediction arc.
// Because our physics (seemingly) behave differently at different update rates,
// it is important that this interval roughly reflects the update rate that the actual thrown object will experience.
// Thus the value of 16.666666 (~ roughly 60fps)
//
// In an ideal world, our interval would be extrapolated from the previous frame's render time to cope with framerate fluctuations,
// but this is complicated by the slowmo effect, and creates unpleasant 'jittering' in the rendered arc.
// We can engineer this further if we really want, but I think it'll do for 99% of cases.
const float	THROWARC_INTERVAL = 16.666666f;

// SW: We don't want to try to draw *every single interval* because that's horribly expensive and unnecessary.
// Instead, we draw every THROWARC_DRAWINTERVAL intervals, from the position of the last point we drew.
const int THROWARC_DRAWINTERVAL = 4;

const int	THROWARC_BEAMCOUNT = 256;
const float	THROWARC_WIDTH = 2.0f;




const float AIR_DEFAULTAMOUNT = 600.0f; //the amount of third lung air the player always gets for free.
const float AIR_FILL_PER_SECOND = 16.0f * 60.0f;
const float AIR_ZEROG_DECREASE_PER_SECOND = -60.0f;



const int FLYTEXT_COUNT = 32;


#define JOCKEY_REAR_DAMAGE_DOTTHRESHOLD -.1f //when jockeying, ignore damage that comes from in front of me.


extern const int ASYNC_PLAYER_INV_AMMO_BITS;
extern const int ASYNC_PLAYER_INV_CLIP_BITS;

const int IMPACT_SLOWMO_TIME = 30; //Do a short burst of slow motion time; this is the default time. (note: in slow mo, time durations work differently)

#define WOUNDCOUNT_MAX 5

#define MAX_DOORLOCKMARKERS  10  //This is dependent on info_station.gui -- make sure the gui has enough windowdefs to handle this quantity.

#ifdef _D3XP
const int COLOR_BAR_TABLE_MAX = 8;
#else
const int COLOR_BAR_TABLE_MAX = 5;
#endif

enum {
	BEACONTYPE_EXTRACTION,
	BEACONTYPE_SHOP
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

// powerups - the "type" in item .def must match
enum {
	BERSERK = 0,
	INVISIBILITY,
	MEGAHEALTH,
	ADRENALINE,
#ifdef _D3XP
	INVULNERABILITY,
	HELLTIME,
	ENVIROSUIT,
	HASTE,
	ENVIROTIME,
#endif
	MAX_POWERUPS
};


enum
{
	NOISETYPE_FOOTSTEP = 0,
	NOISETYPE_GUNSHOT
};

enum
{
	DEAFEN_NONE = 0,
	DEAFEN_INITIALDELAY,
	DEAFEN_FADINGTONORMAL
};

//BC context menu states.
enum
{
	CONTEXTMENU_OFF = 0,
	CONTEXTMENU_WAITFORINITIALRELEASE,
	CONTEXTMENU_ACTIVE,
	CONTEXTMENU_DEACTIVATING,
	CONTEXTMENU_WAITFORBUTTONRELEASE, //Menu was closed. Wait for contextmenu to release before making it available again.
};

enum
{
	LERPSTATE_OFF = 0,
	LERPSTATE_TRANSITIONON,
	LERPSTATE_ON,
	LERPSTATE_TRANSITIONOFF
};

enum
{
	LEANSTATE_OFF = 0,
	LEANSTATE_TRANSITIONON,
	LEANSTATE_ACTIVE,
	LEANSTATE_TRANSITIONOFF
};

enum
{
	LASTHEALTH_OFF = 0,
	LASTHEALTH_ACTIVE,
	LASTHEALTH_TRANSITIONOFF
};


enum
{
	HEALSTATE_NONE			= 0,
	HEALSTATE_GLASSPULL,	//1
	HEALSTATE_BURNING,		//2
	HEALSTATE_BLEEDOUT,		//3
	HEALSTATE_SHRAPNEL,		//4
	HEALSTATE_BULLETPLUCK,	//5
	HEALSTATE_SPEARPLUCK	//6
};

enum
{
	SNEEZESTATE_ACCUMULATING = 0,
	SNEEZESTATE_SNEEZING, //stay frozen here for a while as the sneeze happens.
	SNEEZESTATE_LOWERING, //when player exits dusty zone, meter gradually lowers.
	SNEEZESTATE_RESETTING //when sneeze is done. Invulnerable to sneezes as it resets.
};

const int SNEEZETRIGGER_UPDATERATE = 100;
const int SNEEZETRIGGER_SNEEZEFREEZETIME = 1000; //When sneeze happens, how long to stay in the maxed-out-bar state.


enum
{
	SAVINGTHROW_AVAILABLE = 0,
	SAVINGTHROW_ACTIVE,
	SAVINGTHROW_COOLDOWN
};

enum
{
	GRENADETHROW_IDLE = 0,
	GRENADETHROW_AIMING,
	GRENADETHROW_FIRINGINPROGRESS,
	GRENADETHROW_NOTREADY //this is for when you cancel out. Wait for player to release LMB before ready again.
};

enum
{
	BLOODBAGSTATE_NONE = 0,
	BLOODBAGSTATE_ACTIVATING,
	BLOODBAGSTATE_TRANSFUSING,
	BLOODBAGSTATE_EXITING,
	BLOODBAGSTATE_EXPLODING
};


enum
{
	CARGOHIDETYPE_ROW = 0,
	CARGOHIDETYPE_STACK,
	CARGOHIDETYPE_LAUNDRYMACHINE
};

enum
{
	LIGHTMETER_NONE,		//0
	LIGHTMETER_SHADOWY,		//1
	LIGHTMETER_UNSEEN		//2
};

// powerup modifiers
enum {
	SPEED = 0,
	PROJECTILE_DAMAGE,
	MELEE_DAMAGE,
	MELEE_DISTANCE
};

// influence levels
enum {
	INFLUENCE_NONE = 0,			// none
	INFLUENCE_LEVEL1,			// no gun or hud
	INFLUENCE_LEVEL2,			// no gun, hud, movement
	INFLUENCE_LEVEL3,			// slow player movement
};

#ifdef _D3XP
typedef struct {
	int ammo;
	int rechargeTime;
	char ammoName[128];
} RechargeAmmo_t;

typedef struct {
	char		name[64];
	idList<int>	toggleList;
} WeaponToggle_t;

typedef struct {
	int weaponType;
	int clip;
	int health;
	bool chambered;
	idEntityPtr<idEntity> carryPtr;
} HotbarSlot_t;
#endif

class idInventory {
public:
	int						maxHealth;
	//int						weapons;
	int						powerups;
	int						armor;
	int						maxarmor;
	int						ammo[ AMMO_NUMTYPES ];
	//int						clip[ MAX_WEAPONS ];
	int						powerupEndTime[ MAX_POWERUPS ];


	int						mushrooms; //this is where we store mushroom count.
	int						bloodbags; //this is where we store bloodbag count.
	int						catkeys;
	int						maintbooks;

	//bool					chambered[MAX_WEAPONS]; //BC
	//int						weaponHealth[MAX_WEAPONS];

	//idEntityPtr<idEntity>	carryPtrs[MAX_WEAPONS]; //when player stores a carryable, this points toward the item entity.	


#ifdef _D3XP
	RechargeAmmo_t			rechargeAmmo[ AMMO_NUMTYPES ];
#endif

	// mp
	int						ammoPredictTime;

	int						deplete_armor;
	float					deplete_rate;
	int						deplete_ammount;
	int						nextArmorDepleteTime;

	int						pdasViewed[4]; // 128 bit flags for indicating if a pda has been viewed

	int						selPDA;
	int						selEMail;
	int						selVideo;
	int						selAudio;
	bool					pdaOpened;
	bool					turkeyScore;
	idList<idDict *>		items;
	idStrList				pdas;
	idStrList				pdaSecurity;
	idStrList				videos;
	idStrList				emails;
	idStrList				emailsRead;
	idStrList				emailsReplied;

	bool					ammoPulse;
	bool					weaponPulse;
	bool					armorPulse;
	int						lastGiveTime;

	idList<idLevelTriggerInfo> levelTriggers;

							idInventory() { Clear(); }
							~idInventory() { Clear(); }

	// save games
	void					Save( idSaveGame *savefile ) const;	// blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );	

	void					Clear( void );
	void					GivePowerUp( idPlayer *player, int powerup, int msec );
	void					ClearPowerUps( void );
	void					GetPersistantData( idDict &dict );
	void					RestoreInventory( idPlayer *owner, const idDict &dict );
	bool					Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, bool updateHud, const char *carryableName, int health, idEntity *ent );
	void					Drop( int hotbarSlot );
	ammo_t					AmmoIndexForAmmoClass( const char *ammo_classname ) const;
	int						MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const;
	int						WeaponIndexForAmmoClass( const idDict & spawnArgs, const char *ammo_classname ) const;
	ammo_t					AmmoIndexForWeaponClass( const char *weapon_classname, int *ammoRequired );
	const char *			AmmoPickupNameForIndex( ammo_t ammonum ) const;
	void					AddPickupName( const char *name, const char *icon, idPlayer* owner ); //_D3XP

	//BC
	int						HasAmmo( ammo_t type, int amount );
	bool					UseAmmo( ammo_t type, int amount );
	int						HasAmmo( const char *weapon_classname, bool includeClip = false, idPlayer* owner = NULL );			// _D3XP

#ifdef _D3XP
	bool					HasEmptyClipCannotRefill(int hotbarSlot, const char *weapon_classname, idPlayer* owner);
#endif

	void					UpdateArmor( void );

	idList<idObjectiveInfo>	objectiveNames;

#ifdef _D3XP
	void					InitRechargeAmmo(idPlayer *owner);
	void					RechargeAmmo(idPlayer *owner);
	bool					CanGive( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value );
#endif


	//BC INVENTORY PUBLIC
	bool					IsAutoswitchWeapon(const char *weapon_classname, idPlayer* owner);
    bool                    GiveAmmo(idPlayer *owner, const char *statname, const char *statvalue);

	#define MAX_HOTBARSLOTS 5
	HotbarSlot_t			hotbarSlots[MAX_HOTBARSLOTS];
	int						hotbarUnlockedSlots;

	int						GetEmptyHotbarSlot();
	int						GetWeaponIndex(idPlayer *owner, const char *weaponName);

	bool					SetHotbarSlot(idPlayer *owner, int slotIndex, int weaponIndex, int health, idEntity *ent);
	int						GetHotbarslotViaWeaponIndex(int weaponIndex);

	void					UpdateInventoryCollision(idEntity * changedEnt = nullptr);

	int						GetHotbarSelection(){ return hotbarCurrentlySelected; } //What is currently selected.
	void					SetHotbarSelection(int value = 0);
private:
	int						hotbarCurrentlySelected; //What is currently selected.
	void					ResetHotbarSlot(int hotbarSlot);
};

typedef struct {
	int		time;
	idVec3	dir;		// scaled larger for running
} loggedAccel_t;

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
#ifdef _D3XP
		EVENT_PICKUPNAME,
#endif
		EVENT_MAXEVENTS
	};

	usercmd_t				usercmd;

	class idPlayerView		playerView;			// handles damage kicks and effects

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

	// bc heal states
	idScriptBool			AI_HEAL_GLASSWOUND;
	idScriptBool			AI_HEAL_BURNING;
	idScriptBool			AI_HEAL_BLEEDOUT;
	idScriptBool			AI_ACRO_CEILINGHIDE;
	idScriptBool			AI_ACRO_SPLITS;
	idScriptBool			AI_FALLEN;
	idScriptBool			AI_FALLEN_GETUP;
	idScriptBool			AI_FALLEN_ROLL;
	idScriptBool			AI_HEAL_BULLETPLUCK;
	idScriptBool			AI_HEAL_SHRAPNEL;
	idScriptBool			AI_HEAL_HEALWOUND;
	idScriptBool			AI_HEAL_SPEARPLUCK;
	idScriptBool			AI_JOCKEYRIDE;


	// inventory
	idInventory				inventory;

	idEntityPtr<idWeapon>	weapon;
	idUserInterface *		hud = 0;				// MP: is NULL if not local player
	idUserInterface *		objectiveSystem = 0;
	bool					objectiveSystemOpen;

	int						weapon_soulcube;
	int						weapon_pda;
	int						weapon_fists;


	int						heartRate;
	idInterpolate<float>	heartInfo;
	int						lastHeartAdjust;
	int						lastHeartBeat;
	int						lastDmgTime;
	int						deathClearContentsTime;
	bool					doingDeathSkin;
	int						lastArmorPulse;		// lastDmgTime if we had armor at time of hit
	float					stamina;
	float					healthPool;			// amount of health to give over time
	int						nextHealthPulse;
	bool					healthPulse;
	bool					healthTake;
	int						nextHealthTake;


	bool					hiddenWeapon;		// if the weapon is hidden ( in noWeapons maps )
	idEntityPtr<idProjectile> soulCubeProjectile;

	// mp stuff
	static idVec3			colorBarTable[ COLOR_BAR_TABLE_MAX ];

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

#ifdef CTF
	bool					carryingFlag;		// is the player carrying the flag?
#endif

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

#ifdef _D3XP
	idFuncMountedObject	*	mountedObject = 0;
	idEntityPtr<idLight>	enviroSuitLight;



	float					new_g_damageScale;

	bool					bloomEnabled;
	float					bloomSpeed;
	float					bloomIntensity;
#endif

public:
	CLASS_PROTOTYPE( idPlayer );

							idPlayer();
	virtual					~idPlayer();

	void					Spawn( void );
	void					Think( void );

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void			Hide( void );
	virtual void			Show( void );

	void					Init( void );
	void					PrepareForRestart( void );
	virtual void			Restart( void );
	void					LinkScriptVariables( void );
	void					SetupWeaponEntity( void );
	void					SelectInitialSpawnPoint( idVec3 &origin, idAngles &angles );
	void					SpawnFromSpawnSpot( void );
	void					SpawnToPoint( const idVec3	&spawn_origin, const idAngles &spawn_angles );
	void					SetClipModel( void );	// spectator mode uses a different bbox size

	void					SavePersistantInfo( void );
	void					RestorePersistantInfo( void );
	void					SetLevelTrigger( const char *levelName, const char *triggerName );
	bool					SaveProgression(const char* nextMap = nullptr, const char* checkPoint = nullptr);
	void					Event_SaveProgression(const char* nextMap = nullptr, const char* checkPoint = nullptr);

	bool					UserInfoChanged( bool canModify );
	idDict *				GetUserInfo( void );
	bool					BalanceTDM( void );

	void					CacheWeapons( void );

	void					EnterCinematic( void );
	void					ExitCinematic( void );
	bool					HandleESC( void );
	bool					SkipCinematic( void );

	void					UpdateConditions( void );
	void					SetViewAngles( const idAngles &angles );

							// delta view angles to allow movers to rotate the view of the player
	void					UpdateDeltaViewAngles( const idAngles &angles );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;
	virtual void			GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos );
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
	void					CalcDamagePoints(  idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health, int *armor );
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

							// use exitEntityNum to specify a teleport with private camera view and delayed exit
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination, bool forceDuck = false, bool killbox = false );

	void					Kill( bool delayRespawn, bool nodamage );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void					StartFxOnBone(const char *fx, const char *bone);

	renderView_t *			GetRenderView( void );
	void					CalculateRenderView( void );	// called every tic by player code
	void					CalculateFirstPersonView( void );

	void					DrawHUD( idUserInterface *hud );

	void					WeaponFireFeedback( const idDict *weaponDef );

	float					DefaultFov( void ) const;
	float					CalcFov( bool honorZoom );
	void					CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis );
	idVec3					GetEyePosition( void ) const;
	void					GetViewPos( idVec3 &origin, idMat3 &axis ) const;
	void					OffsetThirdPersonView( float angle, float range, float height, bool clip );

	bool					Give( const char *statname, const char *value, int health, const char *carryableName = "", idEntity *ent = NULL );
	bool					GiveItem( idItem *item );
	void					GiveItem( const char *name );
	void					GiveHealthPool( float amt );

	bool					GiveInventoryItem( idDict *item );
	void					RemoveInventoryItem( idDict *item );
	bool					GiveInventoryItem( const char *name );
	void					RemoveInventoryItem( const char *name );
	idDict *				FindInventoryItem( const char *name );
	void					Event_ClearInventory(void);

	void					GivePDA( const char *pdaName, idDict *item );
	void					GiveVideo( const char *videoName, idDict *item );
	void					GiveEmail(const char *emailName );
	void					GiveEmailViaTalker(const char* talkerName, const char* emailName, bool autoSelect = false);
	void					GiveSecurity( const char *security );
	void					GiveObjective( const char *title, const char *text, const char *screenshot );
	void					CompleteObjective( const char *title );

	bool					GivePowerUp( int powerup, int time );
	void					ClearPowerUps( void );
	bool					PowerUpActive( int powerup ) const;
	float					PowerUpModifier( int type );

	int						SlotForWeapon( const char *weaponName );
	void					Reload( void );
	void					NextWeapon( void );
	void					NextBestWeapon( void );
	void					PrevWeapon( void );
	void					SelectWeapon( int hotbarSlot, int weaponNum, bool force );
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
	void					SetSoulCubeProjectile( idProjectile *projectile );

	void					AdjustHeartRate( int target, float timeInSecs, float delay, bool force );
	void					SetCurrentHeartRate( void );
	int						GetBaseHeartRate( void );
	void					UpdateAir( void );

#ifdef _D3XP
	void					UpdatePowerupHud();
#endif

	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );
	bool					GuiActive( void ) { return focusGUIent != NULL; }

	void					PerformImpulse( int impulse );
	void					Spectate( bool spectate );
	void					TogglePDA( void );
	void					ToggleScoreboard( void );
	void					RouteGuiMouse( idUserInterface *gui );
	void					UpdateHud( void );
	const idDeclPDA *		GetPDA( void ) const;
	const idDeclPDA*		GetPDAViaTalker(idStr talkerName) const;
	const idDeclVideo *		GetVideo( int index );
	void					SetInfluenceFov( float fov );
	void					SetInfluenceView( const char *mtr, const char *skinname, float radius, idEntity *ent );
	void					SetInfluenceLevel( int level );
	int						GetInfluenceLevel( void ) { return influenceActive; };
	void					SetPrivateCameraView( idCamera *camView );
	idCamera *				GetPrivateCameraView( void ) const { return privateCameraView; }
	void					StartFxFov( float duration  );
	void					UpdateHudWeapon( bool flashWeapon = true );
	void					UpdateHudStats( idUserInterface *hud );
	void					UpdateHudAmmo( idUserInterface *hud );
	void					Event_StopAudioLog( void );
	void					StartAudioLog( void );
	void					StopAudioLog( void );
	void					ShowTip( const char *title, const char *tip, bool autoHide );
	void					HideTip( void );
	bool					IsTipVisible( void ) { return tipUp; };
	void					ShowObjective( const char *obj );
	void					HideObjective( void );

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	void					WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg );

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

	void					UpdateSkinSetup( bool restart );

	bool					OnLadder( void ) const;

	virtual	void			UpdatePlayerIcons( void );
	virtual	void			DrawPlayerIcons( void );
	virtual	void			HidePlayerIcons( void );
	bool					NeedsIcon( void );

#ifdef _D3XP
	

	idStr					GetCurrentWeapon();

	bool					CanGive( const char *statname, const char *value );

	void					StopHelltime( bool quick = true );
	void					PlayHelltimeStopSound();
#endif

#ifdef CTF
	void					DropFlag( void );	// drop CTF item
	void					ReturnFlag();
	virtual void			FreeModelDef( void );
#endif

	bool					SelfSmooth( void );
	void					SetSelfSmooth( bool b );

	bool					airless;

	void					Event_SelectWeapon(const char *weaponName); //moved to public.

	int						currentWeaponSlot; //moved to public.

	//BC PUBLIC START =============================================
	void					SetViewPosActive(bool activate, idVec3 _offset);
	void					SetStaminaDelta(float deltaValue);
	void					SetStaminaRechargeDelay();

	void					SetViewYawLerp(float targetYaw, int transitiontime = 250);
	void					SetViewPitchLerp(float targetPitch, int transitiontime = 250 /*default transition time, MAKE SURE this syncs with VIEWPOS_OFFSET_MOVETIME*/  );
	void					SetViewRollLerp(float targetPitch, int transitiontime = 250);
	void					SetViewLerp(const idVec3 &targetPosition, int moveTime);
	void					SetViewLerpAngles(idAngles angles, int moveTime);

	void					SetConfinedState(float baseAngle, float sneezeMultiplier, bool isConfined);
	void					SetSneezeDelta(bool movementBased, float multiplier = 1.0f);

	idVec3					autoaimPos;

	void					Event_AddGlassWound(void);
	void					SetGlasswound(int delta);
	int						GetLastGlasswoundTime();

	void					SetSpearwound();

	void					DoGlassYankShard(void);
	void					DoHealPluck(const char *defName);
	void					EjectBodyDebris(const char *defName, idVec3 spawnPosition);
	void					HealAllWoundEjectDebris(const char *defName, int count);
	
	bool					inDownedState;

	void					SetNoiseEvent(idVec3 position, int noiseType);

	void					SetAmmoDelta(const char *ammoName, int delta);

	void					SetFallState(bool value, bool hardFall);
	void					SetFallGetupState();
	int						fallenRollTimer;
	
	void					SetArmVisibility(bool value);

	

	void					ZippingTo(idVec3 destination, idWinding* aperture, float forceDuckDuration = 0.0f);
	void					Cableswoop(idEntity * startPoint, idEntity * endPoint);
	void					SpaceCableswoop(idEntity * startPoint, idEntity * endPoint);

	

	bool					GetSwoopState();

	void					HealWound(const char *woundName);

	void					FlashScreen();
	void					FlashScreenCustom(idVec4 color, int time);

	void					SetHideState(idEntity * hideEnt, int verticalOffset);

	idEntityPtr<class idMech> mountedMech;
	void					EnterMech(idEntity * mechEnt);
	idAnimated *			mechCockpit = 0;
	void					ExitMech(bool flashWhite);
	bool					IsInMech() const;
	bool					IsInMechEvent();

	idEntity *				UpdateAutoAim(void);
	idEntityPtr<idEntity>	autoaimEnt;

	void					SetViewFade(float r, float g, float b, float a, int time);


	idBodyDragger			bodyDragger;
	idRuler					ruler;

	void					SetAcroHide(idEntity * hideEnt);

	void					StartMovelerp(idVec3 destination, int moveTime = 150);

	void					KillFeedback();

	bool					IsInHealState();

	bool					inConfinedState; //is player in vent.
	bool					confinedStealthActive;
	bool					lastConfinedState;
	bool					confinedAngleLock;
	int						confinedType; //this refers to enum in game_local.h
	

	void					SetBodyAngleLock(bool activate);

	void					SetHudNamedEvent(const char * eventName);
	void					SetHudParmInt(const char * parmName, int value);

	void					SetViewposAbsLerp(const idVec3 &_target, int _duration);
	void					SetViewposAbsLerp2(const idVec3 &_target, int _duration);


	idEntityPtr<idEntity>	peekObject;

	void					SetSmelly(bool value, bool showMessage = true);
	bool					GetSmelly();

	bool					SetOnFire(bool value);
	bool					GetOnFire();
	int						GetShrapnelCount();
	int						GetBulletwoundCount();

	void					SetCarryable(idEntity * ent, bool value, bool gentleDrop = false);
	idEntity *				GetCarryable();
	void					DropCurrentCarryable(idEntity* dropEntTo = nullptr, bool showMessage = true);
	bool					PlayerFrobbedCarryable(idEntity *itemEnt);
	bool					DropCarryable(idEntity *ent);



	const char *			reverbLocation = 0;


	bool					GetDefibState();
	void					SetDefibAvailable(bool value);
	bool					GetDefibAvailable();
	void					Event_SetDefibAvailable(int value);

	bool					listenmodeActive;	
	idVec3					listenmodePos;	//for listening through doors.
	int						listenmodeVisualizerTime;

	void					contextMenuToggle(); // SM: Even though this is called toggle, it only turns it on
	void					CloseContextMenu();
	bool					CanEnterContextMenu();
	void					UpdateContextMenuSound(void);	

	void					Event_SetDropAmmoMsg(int value);

	void					AssignHotbarSlot(const char *weaponName, const char *displayname, int health, bool autoSwitch, idEntity *ent);
	int						GetCurrentWeaponType();

	idVec3					GetThrowAngularVelocity(); // SW

							// SW: The player code normally decides the frob ent for itself, and nothing else needs to know about it.
							// The only current exception to this rule is if the frob entity is destroyed, and needs to null out the pointer.
	idEntity*				GetFrobEnt(void);
	void					SetFrobEnt(idEntity* ent);

	// SW: Lets the player periodically consult lights they are interacting with and update their known 'luminance' level
	static void				UpdateTrackedInteractions(renderEntity_s* renderEnt, idList<trackedInteraction_t>* interactions); 

	void					SetLastLuminanceUpdate(int);
	int						GetLastLuminanceUpdate(void);
	void					SetLuminance(float);
	float					GetLuminance(void);


	//Returns: 0 = player in darkness, 1 = middle, 2 = in light.
	int						GetDarknessValue();
	
	idUserInterface *		eventlogMenu = 0; //BC for eventlog.
	idUserInterface *		spectatorMenu = 0;

	idUserInterface*		levelselectMenu = 0; //BC for the level select menu.
	idListGUI*				levelselectGuiList = 0;


	idUserInterface*		emailFullscreenMenu = 0; //BC for the fullscreen email interface.
	int						emailFullscreenState;
	enum					{EFS_OFF, EFS_TRANSITIONON, EFS_ACTIVE, EFS_TRANSITIONOFF};
	void					UpdateEmailFullscreen();
	void					SetEmailFullscreen(bool active);
	int						emailFullscreenTimer;
	idEntityPtr<idEntity>	emailFullscreenEnt;
	idAngles				emailFullscreenOriginalPlayerViewangle;
	bool					emailInSubMenu;


	void					DrawSpectatorHUD();

	bool					HasWeaponInInventory(const char *weaponName, bool ignoreMultiCarry = false);
	void					Event_HasWeaponInInventory(const char* weaponName, int ignoreMultiCarry); // SW 14th April 2025
	bool					HasEntityInCarryableInventory(idEntity *ent);
	idEntity *				HasEntityInCarryableInventory_ViaInvName(idStr invName);
	int						GetEmptyInventorySlots();
	
	bool					RemoveCarryableFromInventory(const char *weaponName);
	bool					RemoveCarryableFromInventory(idEntity *ent);
	idEntity*				GetCarryableFromInventory(const char *weaponName);

	bool					HasItemViaClassname(const char *itemClassname);
	bool					RemoveItemViaClassname(const char *itemClassname);

	// SW 12th March 2025: added support for getting items directly by name
	// (There are some scenarios where the player has multiple of the same entity class in their inventory,
	// but we still want to handle them independently)
	bool					HasItemViaEntityname(const char* itemname);
	void					Event_HasItemViaEntityname(const char* itemname);

	void					Event_HasItemViaClassname(const char *itemClassname);
	void					Event_RemoveItemViaClassname(const char *itemClassname);
	void					Event_RemoveItemViaEntity(idEntity *ent);
	void					Event_GetItemViaClassname(const char *itemClassname);

	int						GiveAirtics(int airticsToGive);
	int						GetAirtics();

	int						GetBloodbagCount();
	int						GetMushroomCount();
	void					AddBloodMushroom(int value, idVec3 _pos);
	void					SetCenterMessage(const char *text);
	void					SetFanfareMessage(const char *text);

	//int						GetCatkeyCount();
	//void					SetCatkeyDelta(int value);

	
	bool					GiveHealthAdjustedMax(); //fill up healthbar as far as possible (respect the X'd out blocks).
	void					WoundRegenerateHealthblock(); //fill up one health pip.	
	void					GiveHealthGranular(int amount);
	void					GiveBleedoutHealth(int amount);

	void					SetCamSpurt(const char *particleName);

	void					SetImpactSlowmo(bool value, int slowmoTime = IMPACT_SLOWMO_TIME);

	idVec3					GetPlacerNormal();

	void					SetFlytextEvent(idVec3 position, const char *text, int textAlign = 0);

	//camerasplice.
	bool					StartCameraSplice(idEntity *cameraEnt, bool isNewUnlock = false);
	bool					IsCameraSpliceActive( idEntity* cameraEnt );

	void					DoEliminationMessage(const char *enemyName, bool extendedTime);

	idEntity *				GetFocusedGUI();

	bool					IsContextmenuActive();
	bool					IsBlurActive();

	idVec3					GetCameraRelativeDesiredMovement();

	bool					IsJockeying();
	void					SetJockeyMode(bool value);
	void					ResetJockeyTimer();

	void					SetSpectateVictoryFanfare();

	void					ClearMeleeTarget();
	idEntityPtr<idAI>		meleeTarget;

	int						GetCatsAvailable();
	int						GetCatsRescued();
	int						GetCatsTotal();

	void					ForceShowHealthbar(int extraDisplayTime);
	void					ForceShowOxygenbar();

	bool					IsCrouching();

	void					SetGascloudState(bool value);
	bool					cond_gascloud;

	int						GetWoundcount_Burn();
	void					HealWound_Burn();

	idVec3					GetClosestPointOnLine(idVec3 observerPoint, idVec3 lineStart, idVec3 lineEnd);

	bool					SetHotbarslotsUnlockDelta(int delta);

	void					StartGrabringGrab(idVec3 destination, idEntity *grabring);

    int                     GetCoinCount();
    void                    SetCoinDelta(int delta);

	void					Event_SetFOVLerp(float amount, int timeMS);

    void					HealAllWounds(bool silent = false);

	bool					GetFallenState();
	void					Event_GetFallState(void);
	void					Event_SetFallState(bool enable, bool hardFall, bool immediateExit);

	void					ForceDuck(int duration);

	void					DebugPrintInventory(int index);

	void					AddLostInSpace( int lostEntityDefNum );
	void					RemoveLostInSpace( int foundEntityDefNum );
	bool					IsEntityLostInSpace(const idStr& entityDefName);

	idList<int>				lostInSpaceEntityDefNums;

	void					LightEdit_OpenMenu(bool value);

	void					DoDoorlockDelayedUpdate();

	bool					IsPlayerNearSpacenudgeEnt();

	void					Event_SetObjectiveText(const char* text, bool newObjective = true);
	void					SetObjectiveText(const char* text, bool newObjective = true, idStr objectiveIcon = "");

	void					DoZoominspectEntity(idEntity *ent);

	idVec2					GetWorldToScreen(const idVec3& worldPos);
	idVec3					GetWorldToDeviceCoords(const idVec3& worldPos);

	int						GetAcroType();
	bool					wasCaughtEnteringCargoHide;
	bool					IsCurrentlySeenByAI();

	bool					isInZoomMode();
	bool					isInLabelInspectMode();

	void					SetDrawIngressPoints(bool value);

	int						GetHiddenStatus();

	bool					playerTouchedByCenterLight;

	bool					DoInspectCurrentItem();

	bool					SayVO_WithIntervalDelay(const char *sndName, int lineCategory = VO_CATEGORY_BARK);
	void					SayVO_WithIntervalDelay_msDelayed(const char *sndName, int msDelay);

	void					JustPickedupItem(idEntity *ent);

	bool					isInVignette;

	void					Event_HasEmailsCritical();
	bool					HasEmailsCritical();
	bool					HasEmailsUnread();
	
	void					UpdateLevelProgressionIndex();
	void					DebugSetLevelProgressionIndex(int value);
	int						GetLevelProgressionIndex();

	void					SetArmstatsFuseboxNeedUpdate();

	void					EnemyHasRespawned();

	idVec3					GetZoominspectAdjustedPosition(idEntity* ent);

	void					Event_setPlayerFrozen(int value);
	void					Event_TeleportToEnt(idEntity* entity); //BC

	void					SetROQVideoState(int value);
	bool					GetROQVideoStateActive();

	void					DoLocalSoundwave(idStr particlename);

	void					DoPickpocketSuccess(idEntity* ent);
	void					DoPickpocketFail(bool doFanfare);
	bool					IsFullscreenUIActive() const;
	void					LevelselectMenuOpen(int value);

	bool					GetInjured();
	void					UpdatePDAInfo(bool updatePDASel);

	void					UnsetToggleCrouch(); // blendo eric: moved to public function to help player physics acro logic

	bool					escapedFullScreenMenu;

	bool					isTitleFlyMode;

	// SW: Manual control over the fullscreen blood spatter FX (for vignettes, etc).
	bool					doForceSpatter;
	float					forceSpatterAmount; // Range of 0 to 1. 

	// Used for tracking our cassette tapes
	void					PickupTape(int tapeNumber);
	idList<int>				tapesCollected;

	void					SetupArmstatRoomLabels();

	void					Event_IsFullscreenUIActive(void);

	int						GetWoundCount(void);
	void					JockeyAnimTransform(idAI* enemy, bool initRef = false);

	// SW 17th Feb 2025
	void					SetBeingVacuumSplined(bool enable, idMover* mover);
	bool					GetBeingVacuumSplined(void);

	bool					IsPickpocketing();
	idEntity*				GetPickpocketEnt();

	// ====================== BC PUBLIC END ======================

private:
	jointHandle_t			hipJoint;
	jointHandle_t			chestJoint;
	jointHandle_t			headJoint;

	idPhysics_Player		physicsObj;			// player physics

	idList<aasLocation_t>	aasLocation;		// for AI tracking the player

	int						bobFoot;
	float					bobFrac;
	float					bobfracsin;
	int						bobCycle;			// for view bobbing and footstep generation
	float					xyspeed;
	int						stepUpTime;
	float					stepUpDelta;
	float					stepUpCorrection;
	float					idealLegsYaw;
	float					legsYaw;
	bool					legsForward;
	float					oldViewYaw;
	idAngles				viewBobAngles;
	idVec3					viewBob;
	int						landChange;
	int						landTime;

	
	int						idealWeaponSlot;
	int						previousWeaponSlot;
	int						weaponSwitchTime;
	bool					weaponEnabled;
	bool					showWeaponViewModel;

	const idDeclSkin *		skin = 0;
	const idDeclSkin *		powerUpSkin = 0;
	idStr					baseSkinName;

	int						numProjectilesFired;	// number of projectiles fired
	int						numProjectileHits;		// number of hits on mobs

	
	float					airTics;				// set to pm_airTics at start, drops in vacuum
	int						lastAirDamage;

	bool					gibDeath;
	bool					gibsLaunched;
	idVec3					gibsDir;

	idInterpolate<float>	zoomFov;
	idInterpolate<float>	centerView;
	bool					fxFov;

	float					influenceFov;
	int						influenceActive;		// level of influence.. 1 == no gun or hud .. 2 == 1 + no movement
	idEntity *				influenceEntity = 0;
	const idMaterial *		influenceMaterial = 0;
	float					influenceRadius;
	const idDeclSkin *		influenceSkin = 0;

	idCamera *				privateCameraView = 0;

	static const int		NUM_LOGGED_VIEW_ANGLES = 64;		// for weapon turning angle offsets
	idAngles				loggedViewAngles[NUM_LOGGED_VIEW_ANGLES];	// [gameLocal.framenum&(LOGGED_VIEW_ANGLES-1)]
	static const int		NUM_LOGGED_ACCELS = 16;			// for weapon turning angle offsets
	loggedAccel_t			loggedAccel[NUM_LOGGED_ACCELS];	// [currentLoggedAccel & (NUM_LOGGED_ACCELS-1)]
	int						currentLoggedAccel;

	// if there is a focusGUIent, the attack button will be changed into mouse clicks
	idEntity *				focusGUIent = 0;
	idUserInterface *		focusUI = 0;				// focusGUIent->renderEntity.gui, gui2, or gui3
	idAI *					focusCharacter = 0;
	int						talkCursor;				// show the state of the focusCharacter (0 == can't talk/dead, 1 == ready to talk, 2 == busy talking)
	int						focusTime;
	idAFEntity_Vehicle *	focusVehicle = 0;
	idUserInterface *		cursor = 0;

	// full screen guis track mouse movements directly
	int						oldMouseX;
	int						oldMouseY;

	idStr					pdaAudio;
	idStr					pdaVideo;
	idStr					pdaVideoWave;

	bool					tipUp;
	bool					objectiveUp;

	int						lastDamageDef;
	idVec3					lastDamageDir;
	int						lastDamageLocation;
	int						smoothedFrame;
	bool					smoothedOriginUpdated;
	idVec3					smoothedOrigin;
	idAngles				smoothedAngles;

#ifdef _D3XP
	idHashTable<WeaponToggle_t>	weaponToggles;

	int						hudPowerup;
	int						lastHudPowerup;
	int						hudPowerupDuration;
#endif

	// mp
	bool					ready;					// from userInfo
	bool					respawning;				// set to true while in SpawnToPoint for telefrag checks
	bool					leader;					// for sudden death situations
	int						lastSpectateChange;
	int						lastTeleFX;
	unsigned int			lastSnapshotSequence;	// track state hitches on clients
	bool					weaponCatchup;			// raise up the weapon silently ( state catchups )
	int						MPAim;					// player num in aim
	int						lastMPAim;
	int						lastMPAimTime;			// last time the aim changed
	int						MPAimFadeTime;			// for GUI fade
	bool					MPAimHighlight;
	bool					isTelefragged;			// proper obituaries

	idPlayerIcon			playerIcon;

	bool					selfSmooth;

	void					LookAtKiller( idEntity *inflictor, idEntity *attacker );

	void					StopFiring( void );
	void					FireWeapon( void );
	void					Weapon_Combat( void );
	void					Weapon_NPC( void );
	void					Weapon_GUI( void );
	void					UpdateWeapon( void );
	bool					UpdateSpectating( void ); // returns TRUE if a level change occurs
	void					SpectateFreeFly( bool force );	// ignore the timeout to force when followed spec is no longer valid
	void					SpectateCycle( void );
	idAngles				GunTurningOffset( void );
	idVec3					GunAcceleratingOffset( void );

	void					UseObjects( void );
	void					CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );
	void					BobCycle( const idVec3 &pushVelocity );
	void					UpdateViewAngles( void );
	void					EvaluateControls( void );
	void					AdjustSpeed( void );
	void					AdjustBodyAngles( void );
	void					InitAASLocation( void );
	void					SetAASLocation( void );
	void					Move( void );
	void					UpdatePowerUps( void );
	void					UpdateDeathSkin( bool state_hitch );
	void					ClearPowerup( int i );
	void					SetSpectateOrigin( void );

	void					ClearFocus( void );
	void					UpdateFocus( void );
	void					UpdateLocation( void );
	idUserInterface *		ActiveGui( void );
	int						AddGuiPDAData( const declType_t dataType, const char *listName, const idDeclPDA *src, idUserInterface *gui );
	void					ExtractEmailInfo( const idStr &email, const char *scan, idStr &out );
	void					CurrentEmailReplied();
	void					UpdateObjectiveInfo( void );

#ifdef _D3XP
	bool					WeaponAvailable( const char* name );
#endif

	void					UseVehicle( void );

	void					Event_GetButtons( void );
	void					Event_GetMove( void );
	void					Event_GetViewAngles( void );
	void					Event_StopFxFov( void );
	void					Event_EnableWeapon( void );
	void					Event_DisableWeapon( void );
	void					Event_GetCurrentWeapon( void );
	void					Event_GetPreviousWeapon( void );
	
	void					Event_GetWeaponEntity( void );
	void					Event_OpenPDA( void );
	void					Event_PDAAvailable( void );
	void					Event_InPDA( void );
	void					Event_ExitTeleporter( void );
	void					Event_HideTip( void );
	void					Event_LevelTrigger( void );
	void					Event_Gibbed( void );

	
	void					Event_Teleport(const idVec3 &destination);
	

#ifdef _D3XP //BSM: Event to remove inventory items. Useful with powercells.
	void					Event_GiveInventoryItem( const char* name );
	void					Event_RemoveInventoryItem( const char* name );

	void					Event_GetIdealWeapon( void );
	void					Event_WeaponAvailable( const char* name );
	void					Event_SetPowerupTime( int powerup, int time );
	void					Event_IsPowerupActive( int powerup );
	void					Event_StartWarp();
	void					Event_StopHelltime( int mode );
	void					Event_ToggleBloom( int on );
	void					Event_SetBloomParms( float speed, float intensity );
#endif


	//BC PRIVATE START =============================================================

	bool					IsElectricityOnInLocation(idLocationEntity* locationEnt);	
	idHashTable<int>		voIntervalTable;

	//context menu.
	idUserInterface *		contextMenu = 0;
	bool					contextMenuActive;
	float					contextMenuSoundSpeed; // 0.0 to 1.0, ramps rapidly down to 0 after opening menu
	int						soundSlowmoHandle;
	bool					soundSlowmoActive;

	
	int						nextContextMenuAvailableTime; //next time contextmenu can be opened. Prevents menu from opening immediately after closing it.
	int						nextContextMenuCloseTime; //next time to auto close the context menu.
	int						contextMenuForbiddenAlertTimer; // When did we last tell the player 'no, you can't open the menu right now'?
	int						contextMenuState;
	void					ContextmenuConfirm(bool isCommand);
	void					UpdateContextConditions(void);
	int						contextmenuStartTime;




	
	//view position lerping (offset).
	idVec3					viewposTargetOffset;
	idVec3					viewposOffset;
	int						viewposState;
	int						viewposStarttime;

	//view position lerping (absolute).
	idVec3					viewposAbsStart;
	idVec3					viewposAbsTarget;
	bool					viewposAbsActive;
	int						viewposAbsTimer;
	int						viewposAbsDuration;


	//view yaw lerping.
	float					viewyawStartAngle;
	float					viewyawTargetAngle;	
	float					viewyawStarttime;
	int						viewyawState;
	int						viewyawMovetime;

	//view pitch lerping.
	float					viewpitchStartAngle;
	float					viewpitchTargetAngle;
	float					viewpitchStarttime;
	int						viewpitchState;

	//view roll lerp
	float					viewrollStartAngle;
	float					viewrollTargetAngle;
	float					viewrollStarttime;
	int						viewrollState;
	int						viewrollMovetime;

	//leaning.
	trace_t					GetWallCorner();
	trace_t					GetWallCornerTrace(int startRightDistance, idVec3 right, idVec3 forward, idVec3 up);
	int						leanState;
	float					leanAmountX;
	float					leanAmountY;
	float					lastLeanMouseX;
	float					lastLeanMouseY;
	float					lastLeanTime;
	float					lastLeanRoll;
	idVec3					lastLeanOffset;

	int						staminaChargeDelayTimer; //delay before stamina starts recharging.
	int						staminaHideTimer;
	int						staminaHideDelayMode;
	enum					{STAMBAR_HIDDEN, STAMBAR_VISIBLE, STAMBAR_HIDEDELAY};

	int						confinedstateTimer;
	float					confinedAngle;
	

	int						confinedDustTimer;
	

	bool					viewYawLocked;
	float					viewYawLockAngle;
	float					viewYawLockArcsize;
	void					SetViewYawLock(bool activate, float baseYaw, float arcSize);
	bool					viewPitchLocked;
	float					viewPitchLockAngle;
	float					viewPitchLockArcsize;
	void					SetViewPitchLock( bool activate, float basePitch, float arcSize );

	bool					bodyAngleLocked;
	

	float					ClampViewArc(float baseAngle, float arcSize, bool playSounds = true);

	void					SetListenModeActive(bool value);
	bool					UpdateListenStatus(idVec3 headPosition);
	

	void					ThrowCurrentWeapon(void);
	void					ThrowCarryable(void);
	idEntityPtr<idEntity>	carryableItem;
	void					UpdateCarryableItem();
	idVec3					GetCarryableOffset();
	
	//For handling the little animation of hiding the carryable during leaning state.
	int						lastLeanState;
	int						leanCarryableTimer;
	bool					IsLeaning();



	void					UpdateFrobCursor(void);
	void					UpdateFrob(void);
	void					DeselectFrobItem(void);
	bool					IsItemValidFrobbable(idEntity *ent);
	idEntity *				UpdateMemoryPalaceFrobCursor();
	idEntity *				frobEnt = 0;
	int						frobState;
	int						lastFrobNumber;
	idVec3					frobHitpos;
	int						frobFlashTimer;
	float					frobHoldTimer;
	

	enum
	{
		FROBSTATE_NONE = 0,
		FROBSTATE_HOVERCURSOR,
		FROBSTATE_HOLDING
	};

	//Autoaim.
	
	idEntity*				FindAimAssistTarget(idVec3& targetPos);
	float					GetAutoaimScore(const idVec3& targetPos, const idVec3& cameraPos, const idMat3& cameraAxis);
	bool					ComputeTargetPos(idEntity* entity, idVec3& primaryTargetPos, idVec3& secondaryTargetPos);
	idVec2					autoaimDefaultPosition;
	float					autoaimDotTimer;
	int						lastAutoaimEntNum;	
	int						autoaimDotState;
	int						lastAutoaimDotState;

	enum
	{
		AUTOAIMDOTSTATE_IDLE = 0,
		AUTOAIMDOTSTATE_MOVING_TO_LOCK,
		AUTOAIMDOTSTATE_LOCKED,
		AUTOAIMDOTSTATE_MOVING_TO_IDLE
	};

	idVec2					autoaimDotPosition;
	idVec2					autoaimDotStartPos;
	idVec2					autoaimDotEndPos;


	void					DrawAutoAimUI(void);	


	idVec3					damageArrowPosition[DAMAGEARROW_COUNT];
	int						damageArrowTimers[DAMAGEARROW_COUNT];
	void					DoDamageArrow(idVec3 worldPosition);


	
	



	

	int						deathTimer;

	void					StartBashAttack();
	bool					DoPhysicalMelee();
	bool					DoThrowItemAttack(int physicalAttackType);
	enum
	{
		PHYSICALATTACKTYPE_HANDS = 0,
		PHYSICALATTACKTYPE_FEET		
	};


	int						rackslideButtonTimer;
	int						reloadButtonTimer;
	void					RackTheSlide(void);
	void					InspectMagazine(bool active);
	void					InspectChamber(bool active);
	void					InspectWeapon(bool active);

	void					UpdateHealthRecharge(void);
	int						healthrechargeTimer;
	int						healthDeclineTimer;
	bool					readyForHealthrechargeSound;

	void					UpdateHealthbarAnimation(void);
	int						lastHealthValue;
	int						lastHealthTimer;
	int						lastHealthState;
	float					lastHealthDisplayvalue;


	void					UpdatePlayerConditions(void);
	int						cond_burnwound; //how many Fire Wounds does
	int						burningTimer;
	int						burningDOTTimer;
	int						lastBurnwoundTime;
	

	int						lastChemDamageTime;
	int						playerInChemTimer;

	int						cond_glasswound; //how many shards of glass are stuck in foot.
	int						glasswoundMoveTimer;
	int						glasswoundFoot; //for decal reasons, keep track if left or right foot.
	int						lastGlasswoundTime;

	int						cond_spearwound; //HOW Many spears are stuck in our belly.

	int						cond_bulletwound; //how many bullets are inside me.
	int						lastBulletwoundTimer; //Limit how many bullets can hit me at a given time.

	int						cond_shrapnel; //how many shrapnel pieces are inside me.

	int						cond_smelly;
	int						smellyTimer;
	idVec3					lastSmellPosition;


	int						sneezeValue; //tracks how much sneeze is built up in your body.
	idVec3					lastPlayerPosition; //sneeze builds up when we detect player has moved.
	int						nextSneezeTimer;
	int						sneezeState;
	void					DoSneeze(int sneezeType);
	int						sneezeUpdateTimer;
	int						sneezeVOTimer;

	void					Event_SetSpearwoundHeal(void);
	void					Event_SetGlasswoundHeal(void);
	void					Event_GetGlasswounds(void);
	void					Event_SetBurningHeal(void);
	void					Event_GetBulletwounds(void);
	void					Event_GetShrapnelwounds(void);

	int						healthFadeTimer;
	bool					healthFadeMaxedBool;
	bool					healthbarIsFaded;


	int						healthDamageFlashTimer;


	int						healState;
	void					StartHealState(int healstateIndex);
	int						healbarTotalTime;
	int						healbarStartTime;


	

	int						footBloodDecalCount;
	
	

	int						lastAcroState;


	void					DoSpit(int spitType);
	const idDict *			spitDef = 0;
	const idDict *			bloodspitDef = 0;

	
	int						downedTickTimer;
	int						downedDecalTimer;
	idVec3					lastDownedPlayerPosition;


	idVec3					noiseEventPos[NOISEEVENT_COUNT];
	int						noiseEventTimer[NOISEEVENT_COUNT];
	int						noiseEventType[NOISEEVENT_COUNT];

	int						deafenState;
	int						deafenTimer;

	idVec3					lastAttackDir;


	int						lastTargetHeartRate;

	int						lastHeartVolumeAdjust;
	float					heartVolume;
	bool					heartbeatEnabled; // SW 11th Feb 2025: adding this for scripted sequences

	int						savingthrowState;

	int						grenadeButtonTimer;
	int						grenadeThrowState;
	bool					shouldDrawThrowArc;
	bool					lastThrowArcState;
	idBeam*					throwBeamOrigin[THROWARC_BEAMCOUNT] = {};
	idBeam*					throwBeamTarget[THROWARC_BEAMCOUNT] = {};
	idEntity*				throwdisc = 0;
	idODE*					throwPredictionIntegrator = 0;
	idPhysics_RigidBody*	throwWeaponPhysicsObj = 0;
	idEntity*				throwPredictionModel = 0;
	void					UpdateThrownWeapon(const idDeclEntityDef* weapDef);
	void					DrawThrowArc(void);
	bool					GetThrowArcVisible(void);
	void					SetThrowArcVisible(bool);
	rigidBodyPState_t		PredictNextThrowArc(rigidBodyPState_t currentState, idVec3 gravityVector, float deltaTime, float mass, idVec3 centerOfMass);
    trace_t                 throwarcCollision;


	void					Event_getPlacerAngle(void);
	void					Event_getPlacerPos(void);
	void					Event_getPlacerValid(void);
	void					Event_getPlacerRotation(void);
	idEntity				*placerEnt = 0;
	idEntity				*placerEntLine = 0;
	idEntity				*placerEntArrow = 0;
	bool					placerClearance = 0;
	void					UpdatePlacerSticky(void);
	int						placerEntityNumber;
	int						placerEntityCollisionID;

	void					Event_SetButtonPrompt(int prompttype);
	int						buttonpromptType;
	int						buttonpromptTimer;
	enum					{ BUTTONPROMPT_RACKSLIDE, BUTTONPROMPT_RELOAD };

	


	void					SetHudEvent(const char *eventName);
	int						viewpitchTransitionTime;

	bool					defibAvailable;
	enum					{ DEFIB_NONE, DEFIB_SETUP, DEFIB_ZOOMIN, DEFIB_HEARTANIM, DEFIB_ZOOMOUT, DEFIB_ZOOMTOHEAD, DEFIB_CLEANUP };
	int						defibState;
	void					UpdateDefib();
	int						defibTimer;
	idEntity *				defibCamera = 0;
	idEntity *				defibMover = 0;
	idEntity *				defibCameraTarget = 0;
	bool					defibFadeDone;
	float					defibYaw;

	bool					defibParticleDone;
	int						defibParticleTimer;

	int						defibButtonTimer;
	int						defibButtonCount;
	void					ActivateDefib();
	void					Event_HasDefib(void);


	bool					defibButtonState; //this is to handle the sound that plays when holding down the defib button.





	idUserInterface *		videoMenu = 0;
	idUserInterface*		cameraGuiMenu = 0;

	void					BindStickyItem(idEntity *stickyItem);

	void					DrawFrobHUD(int time);

	int						lastBaffleState;

	void					GetYawDelta();
	float					currentYawDelta;


	void					AttachBloodbag();
	int						bloodbagState;
	idAnimated *			bloodbagMesh = 0;
	int						bloodbagTimer;
	int						bloodbagMaxTime;
	int						bloodbagHealth;
	const idDeclParticle *	bloodbagParticles = 0;
	int						bloodbagParticleFlytime;
	jointHandle_t			bloodbagJoint;
	idFuncEmitter			*bloodbagEmitter = 0;
	int						bloodbagDamageTimer;
	void					HideBloodbag();
	int						bloodbagHealthFXState;
	
	


	float					luminance; // SW: Used for light-dark detection. Ranges from 0.0 to 1.0. Only updates once every g_luminance_updateRate milliseconds.
	int						lastLuminanceUpdate;
	idEntityPtr<idStaticEntity>	lightProbe; // SW: Invisible volume that generates interactions when the renderer detects light falling on it.
	
	int						lastLuminanceState;
	int						luminanceState;

	

	

	int						mechTransitionState;
	enum
	{
		MECHTRANSITION_NONE,
		MECHTRANSITION_MOVINGTOBEHIND,
		MECHTRANSITION_ENTERING,
		MECHTRANSITION_PILOTING
	};
	int						mechTransitionTimer;
	idVec3					mechplayerStartPos;
	bool					mechStartupstate;
	int						mechStartupTimer;


	void					UpdateMechCockpit();
	
	int						mechSpeedTimer;
	
	

	void					RemovePlayerWeapon(int switchToEmptyWeapon);



	void					EnableBeaconUI();
	void					DrawBeaconUI(void);
	void					StartEnteringCode(void);
	void					UpdateBeaconSignalLock(void);
	void					UpdateSignalCode(void);
	bool					CanSeeSpace(void);
	int						beaconUITimer;
	void					SetBeaconCode(const char * code);
	idStr					beaconCode;
	int						codeStartTime; // Start time for the current dot/dash
	bool					isEnteringCode; // Currently entering morse code string
	bool					signalLock; // Is signal locked on?
	int						lastSignalLockUpdate; // When did we last update the signal lock-on state?
	idVec3					lastBeaconPosition; // What is the beacon vector's position?
	void					ActivateBeacon(int beaconType, const idVec3 &beaconSpawnPos);
	bool					beaconFlashState;
	int						beaconFlashTimer;
	bool					beaconUIBlink;
	int						beaconUIBlinkTimer;
	int						beaconLingerTimer;
	idVec3					FindSuitableBeaconSpawn(idVec3 beaconPos);
	int						beaconLastStrength;
	bool					beaconLastVertArrows;
	

	void					DrawGrabUI();
	void					DrawEnemyHealth();

	int						nextAttackTime;

	void					Lightedit_reset();
	void					Lightedit_resize(idVec3 value);
	void					Lightedit_colordelta(float value);
	void					Lightedit_reposition(idVec3 delta);
	

	void					DrawSuspicionArrows();


	void					UpdateMeleeTarget(void);	
	void					DrawMeleeCursor(void);
	
	int						confinedlimitsoundTimer;
	int						confinedRustlesoundTimer;
	idVec3					confinedRustlesoundLastPos;

	bool					playingSuspicionSound;

	int						ventpeekRustleTimer;

	idEntityPtr<idEntity>	smellyInterestpoint;

	
	
	

	void					weaponDropCheck();

	bool					lastListenmodeActive;
	bool					listenProbemodeActive;
	int						listenProbeTimer;

    int                     lastLocationEntityNum;

	bool					DoCryoSpawnLogic();
	bool					didInitialSpawn;


    int                     spaceparticleTimer;
	bool					forceSpaceParticles;
	
	bool					SetHotbarSlot(int slotIndex, int weaponIndex, const char *weaponName, int health, idEntity *ent);
	bool					SelectHotbarslot(int slotIndex);
	int						lastSelectedHotbarslot;
	void					UpdateHotbarHighlight(bool activate);

	void					SwapDropWeapon(int hotbarSlot, const char *newWeaponToSelect, idEntity* newEnt);
	void					RemoveCarryableFromHotbar(int hotbarIndex);
	
	bool					eventlogMenuActive;

	bool					levelselectMenuActive; //BC is the level select menu active

	bool					carryableBashActive;
	int						carryableBashTimer;

	void					UpdateZoomMode();
	bool					zoommodeActive;
	int						zoombuttonTimer;
	bool					zoomWaitingForInitialRelease;
	bool					zoomManualInspect;

	int						impactslowmoTimer;
	bool					impactslowmoActive;

	enum					{CURSOR_FROB, CURSOR_SWAP, CURSOR_CARRY, CURSOR_ADDMORE, };

	void					DropCurrentCarryableOrWeapon(); //_impulse21
	void					DropCurrentWeapon( idEntity* dropEntTo = nullptr );
	idEntity *				DropCurrentWeapon_SingleItem(int index, idEntity* dropEntTo = nullptr );
	

	bool					lastRulerState;

	idEntity*				IsPlayerLookingATrashchute();
	idEntityPtr<idEntity>	trashchuteFocus;
	
	int						weaponselectPauseTimer;
	bool					isWeaponselectPaused;

	bool					healthcloudActive;
	int						healthcloudTimer;

	int						mushroomDisplayTimer;
	
	int						zoomCurrentFOV;

	bool					scriptedBlur;
	void					Event_EnableBlur(void);
	void					Event_DisableBlur(void);

	bool					IsEntityEnemyTarget(idEntity *ent);



	//flytext events.
	struct flyTextEvent_t {
		idVec3 pos;
		int timer;
		idStr text;
		int textAlign;
	};

	flyTextEvent_t			flytext[FLYTEXT_COUNT];
	void					UpdateFlytext();



	//player wristwatch.
	bool					armstatsActive;
	idAnimatedEntity*		armstatsModel = 0;
	enum					{ARMST_DORMANT, ARMST_RAISING, ARMST_ACTIVE, ARMST_LOWERING};
	int						armstatsState;
	int						armstatsTimer;
	void					SetArmStatsActive(bool value);

	int						armstatMode;
	enum					{AMOD_MAP, AMOD_OBJECTIVES};


	//oxygen meter fadeout.
	bool					oxygenmeterIsFaded;
	int						oxygenmeterFadeTimer;

	int						oxygenFreezeTimer; //We sometimes want to freeze air consumption to make it clear/visible that the airtank got filled up.
	

	

	//camerasplice.
	bool					cameraspliceActive;
	void					SwitchCameraSpliceChannel(int index);
	int						cameraspliceChannel;
	idEntity *				GetAnyAvailableCamera();
	idEntity *				GetSecurityCameraViaIndex(int index);
	int						nextCameraspliceChannelTime;


	//Jockey.
	bool					inputReadyForTakedown;
	int						jockeyState;
	enum					{ JCK_INACTIVE, JCK_ATTACHED };
	idVec3					GetJockeyDismountLocation();
	bool					CheckDismountViability(idVec3 _position, bool doFloorCheck);
	idEntity *				jockeyArrow = 0;
	idVec3					jockeyJointIdlePos;
	idQuat					jockeyJointIdleRot;
	idVec3					jockeyJointModPos;
	idQuat					jockeyJointModRot;

	void					Weapon_Jockey(void);
	void					DrawAIDodgeUI();
	bool					DrawJockeyUI();
	//idEntity*				jockeyDisc;
	int						jockeyTimer;
	int						lastAvailableJockeyAttacktype;
	bool					ShouldDrawJockeyWallUI(bool baseValue);



	int						spectateTimer;
	int						spectateState;
	enum					{SPC_NONE, SPC_FANFAREREADY, SPC_FANFAREDELAY, SPC_FANFAREPOST};

	int						eliminationCounter;

	bool					toggleCrouch;
	int						teleportCrouchTime; // time of last teleport into crouch position, to help with clipping

	

	

	idEntity*				tele_ui_entity = 0;
    idEntity*				tele_ui_disc = 0;

	int						gascloud_timer;
	int						gascloud_coughtimer;

	bool					CanPlayerStandHere(idVec3 _pos);

	

	enum					{ FROZ_NONE, FROZ_LEGSONLY, FROZ_LEGSANDCAMERA };
	int						isFrozen;
	
	
	float					fovLerpStart;
	float					fovLerpEnd;
	float					fovLerpCurrent;
	int						fovLerpTimer;
	int						fovLerpStartTime;
	int						fovLerpState;
	enum					{FOVLERP_NONE, FOVLERP_LERPING, FOVLERP_LOCKED};

	void					SetCanFrob(int value);
	bool					playerCanFrob;
	
	int						inspectLerpTimer;
	int						inspectLerpState;
	enum					{ INSP_NONE, INSP_LERPON, INSP_INSPECTING, INSP_LERPOFF };

	bool					ShouldLowerWeapon();

    int                     coins;

	int						exitLevelHoldTimer; //when player holds BUTTON_FROB to exit the level during the after-mission report.
	bool					exitLevelButtonAvailable;

	bool					postgameLoadButtonAvailable;

	

	void					UpdateThrowableAIDodge(idVec3 _throwOrigin, bool doAIDodge);
	bool					CheckThrowableArcHitOnAI(idVec3 _throwOrigin, idEntity *aiEnt, idVec3 &nearestPos);

	bool					AcceptJockeyDamage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName);

	int						knockeddownLastSpeed;

    void                    Event_SetBloodRage(float value, float seconds);

	void					Event_ForceSpaceParticles(int enable);

	bool					DoCarryablePlacerLogic();

	idEntityPtr<idEntity>	lastPlayerLookTrigger;
	void					UpdatePlayerLookTriggers();
	void					DebugUnassignedLocations();

	void					Event_GiveEmail(const char *emailName);
	void					Event_GiveEmailViaTalker(const char *talkerName, const char* emailName, int autoSelect);
	void					Event_HasReadEmail(const char* emailname);
	void					Event_HasEmail(const char* emailname);
	bool					HasUnreadEmail_ExceptSpecifiedInbox(int inboxIndex);
	
	int						currentlyActiveEmailInbox;
	int						activeEmailInboxTimer;
	bool					emailInboxCheckActive;
	void					UpdateEmailInboxCheck();
	idStr					emailReplyFunc1;
	idStr					emailReplyFunc2;

	void					UpdateZoomInspect();
	idEntityPtr<idEntity>	zoominspectEntityPtr;
	void					DrawZoomInspect();
	void					HandleZoomInspectPressFire();
	bool					zoominspect_LabelmodeActive;
	idVec3					zoominspect_LabelmodePosition;
	idAngles				zoominspect_LabelmodeAngle;
	int						zoominspect_previousEntNum;
	bool					zoominspect_lerpDone;


	int						zoominspect_lerptimer;
	idVec3					zoominspect_lerpStartPos;
	idAngles				zoominspect_lerpStartAngle;	
	idVec3					GetAdjustedLabelInspectionPosition(idEntityPtr<idEntity> entPtr);


	bool					woundArray[WOUNDCOUNT_MAX];
	void					AddToWoundArray(int woundType);

	bool					HasInventoryItem(idStr invname);

	bool					IsThrowarcAtDropPosition();
	bool					throwarcDropMode;
	bool					lastThrowarcDropMode;

	int						vo_chamberchecktimer;
	bool					hasSaidChambercheckVO;

	int						vo_reloadchecktimer;
	bool					hasSaidReloadcheckVO;

	void					Event_flytext(const idVec3 &position, const char *text);

	
	void					EventlogMenuOpen(bool value);
	void					UpdateLevelselectMenu();
	bool					LevelHasAnyMilestones(const idDeclEntityDef* mapDef);

	
	int						pickpocketPipCount;
	int						pickpocketPipTimer;
	bool					pickpocketGoodRange;
	bool					lastPickpocketGoodRange;

	int						pickpocketState;
	enum					{PP_NONE, PP_PICKING, PP_OUTOFRANGE_DELAY};
	idEntityPtr<idEntity>	pickpocketEnt;
	void					DrawPickpocketUI();	
	float					GetPickpocketDistance();

	int						statsPickpocketAttempts;
	int						statsPickpocketSuccesses;

	

	void					UpdateArmstats();
	void					UpdateArmstatsDoorlocks();
	int						armstatDoorlockUpdateTimer;
	bool					armstatDoorlockNeedsUpdate;
	
	int						armstatFuseboxUpdateTimer;
	bool					armstatFuseboxNeedsUpdate;
	void					UpdateArmstatsFuseboxes();

	void					Event_HasInventoryItem(const char *itemname);

	int						GetThrowPower(idEntity *ent);

	int						cryoexitEntNum;

	idEntity *				FindBashTarget(trace_t& outputTr);

	void					DoMemoryPalace();
	void					StartMemoryPalace();
	enum					{MEMP_NONE, MEMP_SPAWNDELAY, MEMP_ACTIVE};
	int						memorypalaceState;
	int						memorypalaceTimer;
	idVec3					memorypalacePlayerPos;
	idVec3					memorypalaceForwardView;
	void					ExitMemoryPalace();

	
	bool					UpdateConfinedDust();

	bool					ingresspointsDrawActive;
	void					DrawIngressPoints();

	bool					IsZoominspectEntInLOS(idEntity *ent);

	

	

	bool					CallCustomMorsesignal(idStr _playercode);

	bool					Event_IsAirless(void);

    int                     staminaThrowTimer;

    void                    UpdateGrenadethrowStamina();

	void					PlayerLanded();

	void					ExitLabelinspectMode();
	

	bool					IsEntityFuseboxLocked(idEntity *ent);

	//For the mission stats at the end.
	int						damageTaken;
	int						enemiesPounced;
	int						brutalslams;
	int						enemyrespawns;

	bool					iteminspectActive;
	void					ExitIteminspectMode();
	int						iteminspectFOV;

	bool					ShouldHideLegend();
	
	void					DebugPlayerConnectedArea();
	void					UpdateMemoryPalace();
	void					MarkMemorypalacenoteDone(idEntity *ent);
	void					DrawMemoryPalace();

	

	//This value determines how far the player is in the campaign. The level index values live inside the file maps.def
	int						levelProgressIndex;

	idStr					GetNextMap();
	void					Event_GetNextMap();	
	void					Event_GetMapIndex();
	void					Event_GetNextMapName();
	void					Event_GetNextMapDesc();
	void					Event_GetIsNextmapShip();

	void					Event_SetFallDamageImmune(int status);
	bool					isFallDamageImmune;

	bool					CanZoom();

	void					OnSelectLevelselectIndex(int index);
	void					OnStartLevelselectIndex(int index);

	bool					spectateUIActive;
	bool					spectateStatsActive;

	idEntity*				spectateTimelineEnt = 0;

	void					UpdateSpectatenodes();

	bool					IsLookingatKeypad();

	void					SetupSpectatorMilestones(idUserInterface *statsGui);

	void					Event_IsBurning(void);
	void					Event_ClearWounds(int silent);
	void					Event_IsDowned(void);

	void					Event_SetDownedState(int toggle);
	void					EnterDownedState(const idDeclEntityDef* damageDef);
	void					CancelDownedState(void);

	idAnimatedEntity*		ninaOrgansModel = 0;
	idVec3					ninaOrgansPosStart;
	idVec3					ninaOrgansPosEnd;
	int						ninaOrgansTimer;

	void					Event_SetJockeyMode(float toggle, idEntity* target);

	void					Event_GetLocationLocID();


	void					DrawThrowUI();

	void					Event_ForceDuck(int value);
	void					Event_IsEntityLostInSpace(const char* entityDefName);
	void					Event_SetVignetteMode(int value);
	void					Event_GetVignetteMode(void);
	void					UpdateHudVignetteState(bool isVignette);

	bool					roqVideoStateActive;

	void					Event_SetSmelly(int value);

	void					Event_DropInventoryItem(const char* inventoryItemName);

	bool					IsReadableRead(idEntity *ent);

	bool					IsSavingthrowAvailable(const idDict* damageDef, idEntity* inflictor);

	void					DoExitZoomMode();

	void					Event_SetTitleFlyMode(int value);

	void					SetCameraGuiEvent(const char* eventName);

	void					Event_ResetMaterialTimer(const char* materialName);



	void					Event_ParticleStream(const char* particlename, const idVec3 &destination, int duration);
	int						particlestreamTimer;
	idVec3					particleStreamDestination;
	idEntity*				particleStreamEnt = 0;

	void					Event_ForceSpatterFx(int enable, float amount);

	void					Event_SetPlayerBody(int value);
	bool					showPlayerBody_Scripted;


	void					HandleItemLocboxInspect(idDict &spawnargs);
	void					DisplayLocbox(idStr _text);

	bool					ShouldShowLocBox();

	void					UpdatePlayerLocboxTriggers();

	// SW 2nd April: converting this from a bool to an entity pointer so we know *what* the previous locbox was,
	// not just *whether* a locbox was active. This lets us handle switching directly from one locbox to another.
	idEntityPtr<idEntity>	lastLocboxTriggered; 

	void					DisplayCatBox(idStr name, idStr title, idStr blurb);
	void					Event_DisplayCatBox(const char* name, const char* title, const char* blurb);

	bool					drawCinematicHUD; //do we force hud to draw during cinematics.
	void					Event_SetCinematicHUD(int value);

	void					Event_EnableHeartBeat(int value);
	void					RestorePDAEmails(bool clean = false);

	// SW 17th Feb 2025
	idMover*				vacuumSplineMover = 0;

	//BC PRIVATE END ===============================================================

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
