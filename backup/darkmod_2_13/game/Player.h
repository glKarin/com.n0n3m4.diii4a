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
#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

#include "ButtonStateTracker.h"
#include "Listener.h"
#include "FrobHelper.h"

class CInventoryItem;
typedef std::shared_ptr<CInventoryItem> CInventoryItemPtr;

class CInventoryWeaponItem;
typedef std::shared_ptr<CInventoryWeaponItem> CInventoryWeaponItemPtr;

/*
===============================================================================

	Player entity.
	
===============================================================================
*/

extern const idEventDef EV_Player_GetButtons;
extern const idEventDef EV_Player_GetMove;
extern const idEventDef EV_Player_GetViewAngles;
extern const idEventDef EV_Player_SetViewAngles;
extern const idEventDef EV_Player_GetMouseGesture;
extern const idEventDef EV_Player_MouseGestureFinished;
extern const idEventDef EV_Player_StartMouseGesture;
extern const idEventDef EV_Player_StopMouseGesture;
extern const idEventDef EV_Player_ClearMouseDeadTime;
extern const idEventDef EV_Player_EnableWeapon;
extern const idEventDef EV_Player_DisableWeapon;
extern const idEventDef EV_Player_ExitTeleporter;
extern const idEventDef EV_Player_SelectWeapon;
extern const idEventDef EV_SpectatorTouch;
extern const idEventDef EV_Player_PlayStartSound;
extern const idEventDef EV_Player_DeathMenu;
extern const idEventDef EV_Player_MissionFailed;
extern const idEventDef EV_Player_GiveHealthPool;
extern const idEventDef EV_Player_WasDamaged;
extern const idEventDef EV_Mission_Success;
extern const idEventDef EV_TriggerMissionEnd;
extern const idEventDef EV_Player_GetLocation;
extern const idEventDef EV_Player_GetFrobbed;
extern const idEventDef EV_Player_SetFrobOnlyUsedByInv;
extern const idEventDef EV_Player_GetCalibratedLightgemValue;
extern const idEventDef EV_Player_SetAirAccelerate;
extern const idEventDef EV_Player_IsAirborne;

// tels: #3282
extern const idEventDef EV_Player_GetShouldered;
extern const idEventDef EV_Player_GetDragged;
extern const idEventDef EV_Player_GetGrabbed;

//Obsttorte:
extern const idEventDef EV_SAVEGAME;
extern const idEventDef EV_setSavePermissions;

// grayman #4882
extern const idEventDef EV_Player_SetPeekView;

const float THIRD_PERSON_FOCUS_DISTANCE	= 512.0f;
const int	LAND_DEFLECT_TIME = 150;
const int	LAND_RETURN_TIME = 300;
const int	FOCUS_TIME = 300;
const int	FOCUS_GUI_TIME = 500;

#define TDM_PLAYER_WEAPON_CATEGORY			"#str_02411"
#define TDM_PLAYER_MAPS_CATEGORY			"#str_02390"

#define ARROW_WEAPON_INDEX_BEGIN 3		// grayman #597 - weapons at or above this index are arrows

// grayman - We aren't using the heartbeat code anymore, so I wrapped
// it in an #ifdef. Uncomment the following line if anyone wants to use it.
// In its current state, it won't play a heartbeat while the player is alive,
// and will only play a couple after the player dies, so there's no point
// using it as is.

//#define PLAYER_HEARTBEAT

#ifdef PLAYER_HEARTBEAT
// Heart rate constants
const int DEAD_HEARTRATE = 0;			// fall to as you die
const int LOWHEALTH_HEARTRATE_ADJ = 20; // 
const int DYING_HEARTRATE = 30;			// used for volume calc when dying/dead
const int BASE_HEARTRATE = 70;			// default
const int ZEROSTAMINA_HEARTRATE = 115;  // no stamina
const int MAX_HEARTRATE = 130;			// maximum
const int ZERO_VOLUME = -40;			// volume at zero
const int DMG_VOLUME = 5;				// volume when taking damage
const int DEATH_VOLUME = 15;			// volume at death
#endif // PLAYER_HEARTBEAT

const int ASYNC_PLAYER_INV_AMMO_BITS = idMath::BitsForInteger( 999 );	// 9 bits to cover the range [0, 999]
const int ASYNC_PLAYER_INV_CLIP_BITS = -7;								// -7 bits to cover the range [-1, 60]

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

/**
* Possible mouse directions, for mouse gestures
**/
typedef enum {
	MOUSEDIR_NONE,
	MOUSEDIR_LEFT,
	MOUSEDIR_UP_LEFT,
	MOUSEDIR_UP,
	MOUSEDIR_UP_RIGHT,
	MOUSEDIR_RIGHT,
	MOUSEDIR_DOWN_RIGHT,
	MOUSEDIR_DOWN,
	MOUSEDIR_DOWN_LEFT
} EMouseDir;

/**
* Possible tests, for mouse gestues
**/
typedef enum 
{
	MOUSETEST_UPDOWN,
	MOUSETEST_LEFTRIGHT,
	MOUSETEST_4DIR, // up down left right
	MOUSETEST_8DIR, // 4 DIR + the diagonals
} EMouseTest;

/**
* Mouse gesture data
**/
typedef struct SMouseGesture_s
{
	bool bActive; // we are currently checking a mouse gesture
	EMouseTest test; // defines which directions we're testing for movement
	bool bInverted; // mouse motions are inverted

	int key; // key being checked
	int thresh; // mouse input threshold required to decide
	int DecideTime; // time in ms before we auto-decide (default -1, wait forever)
	int DeadTime; // time over which the response is dampened, can be greater than decide time

	int started; // time in ms at which we started
	idVec2 StartPos; // mouse position at which we started	
	idVec2 motion; // accumulated mouse motion over the gesture
	EMouseDir result; // result of the last gesture, using the MOUSEDIR_* enum

	SMouseGesture_s( void )
	{
		bActive = false;
		test = MOUSETEST_LEFTRIGHT;
		bInverted = false;
		key = 0;
		thresh = 0;
		DecideTime = -1;
		DeadTime = 0;
		started = 0;
		StartPos = vec2_zero;
		motion = vec2_zero;
		result = MOUSEDIR_RIGHT;
	};

	void	Save(idSaveGame *savefile) const
	{
		savefile->WriteBool( bActive );
		savefile->WriteInt( (int) test );
		savefile->WriteBool( bInverted );
		savefile->WriteInt( key );
		savefile->WriteInt( thresh );
		savefile->WriteInt( DecideTime );
		savefile->WriteInt( started );
		savefile->WriteVec2( StartPos );
		savefile->WriteVec2( motion );
		savefile->WriteInt( (int) result );
	};
	void	Restore(idRestoreGame *savefile)
	{
		int tempInt;

		savefile->ReadBool(bActive);
		savefile->ReadInt( tempInt );
		test = (EMouseTest) tempInt;
		savefile->ReadBool( bInverted );
		savefile->ReadInt( key );
		savefile->ReadInt( thresh );
		savefile->ReadInt( DecideTime );
		savefile->ReadInt( started );
		savefile->ReadVec2( StartPos );
		savefile->ReadVec2( motion );
		savefile->ReadInt( tempInt );
		result = (EMouseDir) tempInt;
	};
} SMouseGesture;

// Player control immobilization categories.
enum {
	EIM_ALL					= -1,
	EIM_UPDATE				= BIT( 0),	// For internal use only. True if immobilization needs to be recalculated.
	EIM_VIEW_ANGLE			= BIT( 1),	// Looking around
	EIM_MOVEMENT			= BIT( 2),	// Forwards/backwards, strafing and swimming.
	EIM_CROUCH				= BIT( 3),	// Crouching.
	EIM_CROUCH_HOLD			= BIT( 4),	// Prevent changes to crouching state. (NYI)
	EIM_JUMP				= BIT( 5),	// Jumping.
	EIM_MANTLE				= BIT( 6),	// Mantling (excluding jumping)
	EIM_CLIMB				= BIT( 7),	// Climbing ladders, ropes and mantling.
	EIM_FROB				= BIT( 8),	// Frobbing.
	EIM_FROB_HILIGHT		= BIT( 9),	// Frobbing AND frob hilighting (not sure if needed or if EIM_FROB can disable hilight also)
	EIM_FROB_COMPLEX		= BIT(10),	// Frobbing of "complex" items (not a door, lever, button, etc)
	EIM_ATTACK				= BIT(11),	// Using weapons 
	EIM_ATTACK_RANGED		= BIT(12),	// Using ranged weapons (bows)
	EIM_WEAPON_SELECT		= BIT(13),	// Selecting weapons.
	EIM_ITEM_USE			= BIT(14),	// Using items
	EIM_ITEM_SELECT			= BIT(15),	// Selecting items.
	EIM_ITEM_DROP			= BIT(16),	// Dropping inventory items.
	EIM_LEAN				= BIT(17),  // Leaning
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
		EVENT_SPECTATE,
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

	// The GUI of the lockpick visualisation HUD
	int						lockpickHUD;

	// angua: this is true when the player lands after jumping or a fall
	// to play the jumping footstep sounds
	bool					hasLanded;

	idScriptBool			AI_FORWARD;
	idScriptBool			AI_BACKWARD;
	idScriptBool			AI_STRAFE_LEFT;
	idScriptBool			AI_STRAFE_RIGHT;
	idScriptBool			AI_ATTACK_HELD;
	idScriptBool			AI_BLOCK_HELD;
	idScriptBool			AI_WEAPON_FIRED;
	idScriptBool			AI_WEAPON_BLOCKED;
	idScriptBool			AI_JUMP;
	idScriptBool			AI_CROUCH;
	idScriptBool			AI_ONGROUND;
	idScriptBool			AI_ONLADDER;
	// AI_INWATER holds a value from the enum waterLevel_t to say how deep the player is in 
	// water. Can still be used as a boolean by scripts, because WATERLEVEL_NONE == 0. SteveL #4159
	idScriptInt				AI_INWATER; 
	//idScriptBool			AI_DEAD; // is defined on idActor now
	idScriptBool			AI_RUN;
	idScriptBool			AI_PAIN;
	idScriptBool			AI_HARDLANDING;
	idScriptBool			AI_SOFTLANDING;
	idScriptBool			AI_RELOAD;
	idScriptBool			AI_TELEPORT;
	idScriptBool			AI_TURN_LEFT;
	idScriptBool			AI_TURN_RIGHT;
	/**
	* Leaning
	**/
	idScriptBool			AI_LEAN_LEFT;
	idScriptBool			AI_LEAN_RIGHT;
	idScriptBool			AI_LEAN_FORWARD;


	/**
	* Ishtvan: Set to true for the duration of the frame if the AI takes damage
	* (more reliable than AI_PAIN)
	**/
	bool					m_bDamagedThisFrame;

	/**
	* Set to true if the player is creeping
	**/
	idScriptBool			AI_CREEP;

	/**
	* greebo: Helper class keeping track of which buttons are currently
	*		  held down and which got released.
	*		  calls PerformButtonRelease() on this entity on this occasion.
	*/
	ButtonStateTracker		m_ButtonStateTracker;

	/**
	* Player's current/last mouse gesture:
	**/
	SMouseGesture			m_MouseGesture;

	// ---- Frob-related members (moved from CDarkmodPlayer to here) ----

	/** 
	* Ishtvan: The target that we initially started pressing frob on
	* keep track of this for things that react to frob held, so we don't
	* move from one target to another without first letting go of frob
	**/
	idEntityPtr<idEntity>	m_FrobPressedTarget;

	/**
	 * FrobEntity is NULL when no entity is highlighted. Otherwise it will point 
	 * to the entity which is currently highlighted.
	 */
	idEntityPtr<idEntity>	m_FrobEntity;

	/**
	* Frobbed joint and frobbed clipmodel ID if an AF has been frobbed
	* Set to INVALID and -1 if the frobbed entity is not an AF
	**/
	jointHandle_t	m_FrobJoint;
	int				m_FrobID;

	/**
	* The trace that was done for frobbing
	* Read off by idEntity::UpdateFrob when something has been newly frobbed
	**/
	trace_t			m_FrobTrace;

	/**
	* If true, this only allows frobbing of entities that can be used by
	* the currently selected inventory item.
	* This also disables normal frob actions on pressing frob,
	* only allowing the "used by" action.
	**/
	bool			m_bFrobOnlyUsedByInv;

	/**
	* Set to true if the player is holding an item with the Grabber
	**/
	bool					m_bGrabberActive;

	/**
	* Set to true if the player is shouldering a body
	**/
	bool					m_bShoulderingBody;


	// angua: whether the player should be crouching
	bool					m_IdealCrouchState;

	// angua: this is true when the player is holding the crocuh button
	// or toggle crouch is active
	bool					m_CrouchIntent;

	// Daft Mugi: For new toggle crouch, this is set to true when the
	// player has pressed toggle crouch while on a rope or ladder/vine.
	bool					m_CrouchToggleBypassed;

	// Daft Mugi: Keep track of forced-crouch mantle state. This is needed
	// to ensure the player view does not clip through the ceiling.
	idVec3					m_prevMantleOrigin;
	bool					m_bMantleViewAtCrouchView;

	// Daft Mugi: For new toggle creep, this is set to true when the
	// player is holding the creep button or toggle creep is active.
	bool					m_CreepIntent;

	// STiFU: FrobHelper alpha calculation
	CFrobHelper				m_FrobHelper;


	/**
	* Hack to fix the leaning test of key-releases
	* Timestamp to wait a few frames before testing for button release
	**/
	int						m_LeanButtonTimeStamp;

	idEntityPtr<idWeapon>	weapon;
	idUserInterface *		hud;				// MP: is NULL if not local player

	// greebo: This is true if the inventory HUD needs a refresh
	bool					inventoryHUDNeedsUpdate;

	// greebo: A list of HUD messages which are displayed one after the other
	idList<idStr>			hudMessages;
	idList<idStr>			inventoryPickedUpMessages;

	int						weapon_fists;

#ifdef PLAYER_HEARTBEAT
	bool					m_HeartBeatAllow; // disable hearbeat except when dying or drowning - Need this to track state
	int						heartRate;
	idInterpolate<float>	heartInfo;
	int						lastHeartAdjust;
	int						lastHeartBeat;
#endif // PLAYER_HEARTBEAT

	int						lastDmgTime;
	int						deathClearContentsTime;
	bool					doingDeathSkin;
	float					stamina;
	float					healthPool;			// amount of health to give over time
	int						nextHealthPulse;
	bool					healthPulse;
	// greebo: added these to make the interval customisable
	int						healthPoolStepAmount;			// The amount of healing in each pulse
	int						healthPoolTimeInterval;			// The time between health pulses
	float					healthPoolTimeIntervalFactor;	// The factor to increase the time interval after each pulse


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

	// A pointer to our weaponslot.
	CInventoryCursorPtr		m_WeaponCursor;
	// An index of the current map/floorplan.
	int						m_MapCursorIdx;
	// The name of the item that was current before pressing inventory clear
	idStr					m_LastItemNameBeforeClear;

	// The currently active inventory map entity
	idEntityPtr<idEntity>	m_ActiveInventoryMapEnt;

	int						m_WaitUntilReadyGuiHandle;
	int						m_WaitUntilReadyGuiTime;

	// grayman #3424 - keep track of intruder evidence when meeting friends
	int						timeEvidenceIntruders;

	// Obsttorte: 
	int						savePermissions;

	/**
	* Pointer to an idListener entity if one is active
	**/
	idEntityPtr<idListener>		m_Listener; // grayman #4620

	int						usePeekView;    // grayman #4882
	idVec3					normalViewOrigin;  // grayman #4882
	idVec3					peekEntityViewOrigin; // grayman #4882

public:
	CLASS_PROTOTYPE( idPlayer );

							idPlayer();
	virtual					~idPlayer() override;

	void					Spawn( void );
	virtual void			Think( void ) override;

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;

	idPlayerView&			GetPlayerView() { return playerView; }

	void					Init( void );
	void					PrepareForRestart( void );
	virtual void			Restart( void ) override;
	virtual void			LinkScriptVariables( void ) override;
	void					SetupWeaponEntity( void );
	void					SelectInitialSpawnPoint( idVec3 &origin, idAngles &angles );
	void					SpawnFromSpawnSpot( void );
	void					SpawnToPoint( const idVec3	&spawn_origin, const idAngles &spawn_angles );
	void					SetClipModel( void );	// spectator mode uses a different bbox size

	void					SavePersistentInfo( void );
	void					RestorePersistentInfo( void );

	bool					UserInfoChanged( bool canModify );
	idDict *				GetUserInfo( void );
	bool					BalanceTDM( void );

	void					CacheWeapons( void );

	void					EnterCinematic( void );
	void					ExitCinematic( void );
	bool					HandleESC( void );
	bool					SkipCinematic( void );

	int						GetImmobilization();
	int						GetImmobilization( const char *source );
	void					SetImmobilization( const char *source, int type );

	// greebo: Consolidate these three hinderances into one, selectable via an enum?
	float					GetHinderance();
	float					GetTurnHinderance();
	float					GetJumpHinderance();

	/**
	* Sets the linear movement hinderance.  
	* This should be a fraction relative to max movement speed
	* mCap values multiply together with those of other hinderances, aCap is an absolute cap
	* for this particular hinderance.
	**/
	void					SetHinderance( const char *source, float mCap, float aCap );
	/**
	* Same as SetHinderance, but applies to angular view turning speed
	**/
	void					SetTurnHinderance( const char *source, float mCap, float aCap );

	void					SetJumpHinderance( const char *source, float mCap, float aCap );

	// greebo: Sets the "player is pushing something heavy" state to the given bool (virtual override)
	virtual void			SetIsPushing(bool isPushing) override;
	// Returns whether the player is currently pushing something heavy (virtual override)
	virtual bool			IsPushing() override;

	// Called by the grabber to signal that we start/stop shouldering a body
	void					OnStartShoulderingBody(idEntity* body);
	void					OnStopShoulderingBody(idEntity* body);

	/**
	 * @brief		greebo:	Plays the footstep sound according to the current movement type.
	 */
	virtual void			PlayFootStepSound() override;
	/**
	 * STiFU #4947: Refactored to give a clearer structure and added further arguments
	 * @param		pPointOfContact If not NULL, the material type is checked at this given position instead of the player position. Furthermore, the waterlevel is ignored because it is invalid at this position.
	 * @param		skipTimeCheck If true, the sound is played regardless of the last playtime
	 */
	void					PlayPlayerFootStepSound(idVec3* pPointOfContact = NULL, const bool skipTimeCheck = false);

	void					UpdateConditions( void );
	void					SetViewAngles( const idAngles &angles );

							// delta view angles to allow movers to rotate the view of the player
	void					UpdateDeltaViewAngles( const idAngles &angles );

	/**
	* Get or set the listener location for the player, in world coordinates
	**/
#if 1 // grayman #4882
	void					SetPrimaryListenerLoc(idVec3 loc);
	idVec3					GetPrimaryListenerLoc( void );
	void					SetSecondaryListenerLoc(idVec3 loc);
	idVec3					GetSecondaryListenerLoc(void);
#else
	void					SetListenerLoc(idVec3 loc);
	idVec3					GetListenerLoc(void);
#endif

	/**
	* Set/Get the door listening location
	**/
	void					SetListenLoc( idVec3 loc );
	idVec3					GetListenLoc( void );

	void					CrashLand( const idVec3 &savedOrigin, const idVec3 &savedVelocity );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const override;
	virtual void			GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) override;
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) override;
	void					CalcDamagePoints(  idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health );
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
							const char *damageDefName, const float damageScale, const int location,
							trace_t *collision = NULL ) override;

							// use exitEntityNum to specify a teleport with private camera view and delayed exit
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) override;

	void					Kill( bool delayRespawn, bool nodamage );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	void					StartFxOnBone(const char *fx, const char *bone);

	virtual renderView_t *	GetRenderView( void ) override;
	void					CalculateRenderView( void );	// called every tic by player code
	void					CalculateFirstPersonView( void );

	void					DrawHUD( idUserInterface *hud );

	void					WeaponFireFeedback( const idDict *weaponDef );

	float					DefaultFov( void ) const;
	float					CalcFov( bool honorZoom );
	void					CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis );
	virtual idVec3			GetEyePosition( void ) const override;
	virtual bool			CanGreet() override; // grayman #3338

	virtual void			GetViewPos( idVec3 &origin, idMat3 &axis ) const override;
	void					OffsetThirdPersonView( float angle, float range, float height, bool clip );

	bool					Give( const char *statname, const char *value );
	void					GiveItem( const char *name );
	void					GiveHealthPool( float amt );

	// Unused. Returns always 1.0. Will be replaced by the new effects system eventually.	
	float					PowerUpModifier( int type );

	int						SlotForWeapon( const char *weaponName );
	void					Reload( void );
	void					NextWeapon( void );
	void					NextBestWeapon( void );
	void					PrevWeapon( void );

	// greebo: Returns the highest weapon index in the weapon inventory category (-1 if empty/error)
	// Traverses the entire category, so this is not the fastest code
	int						GetHightestWeaponIndex();

	// returns FALSE if the weapon with the requested index could not be selected
	bool					SelectWeapon( int num, bool force );

	/**
	 * greebo: This returns the current weapon being focused at by the weapon inventory cursor.
	 * Can return NULL, but should not in 99% of the cases.
	 */
	CInventoryWeaponItemPtr	GetCurrentWeaponItem();

	/** 
	 * greebo: Returns the inventory weapon item with the given name (e.g. "shortsword" or "broadhead").
	 * This can actually return NULL if no weapon item with the given name exists.
	 */
	CInventoryWeaponItemPtr GetWeaponItem(const idStr& weaponName);

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
	// greebo: This method updates the player's movement hinderance when weapons are drawn
	void					UpdateWeaponEncumbrance();

#ifdef PLAYER_HEARTBEAT
	void					AdjustHeartRate( int target, float timeInSecs, float delay, bool force );
#endif // PLAYER_HEARTBEAT

	void					SetCurrentHeartRate( void );
	int						GetBaseHeartRate( void );
	void					UpdateAir( void );
	void					PlaySwimmingSplashSound( const char *soundName ); // grayman #3413
	
	/**
	* This updates the audiovisual effects when the player is underwater
	*/
	void					UpdateUnderWaterEffects();

	/**
	* greebo: Accessor methods for the airTicks member variable. 
	*/
	int						getAirTicks() const;
	void					setAirTicks(int airTicks);

	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) override;
#if 0
	bool					GuiActive( void ) { return focusGUIent != NULL; }
#endif

	void					PerformImpulse( int impulse );

	/**
	* greebo: This gets called by the ButtonStateTracker helper class
	*		  whenever a key is released.
	*
	* @impulse: The impulse number
	* @holdTime: The time the button has been held down
	*/
	void					PerformKeyRelease(int impulse, int holdTime);

	/**
	* sparhawk: This gets called by the ButtonStateTracker helper class
	*			whenever a key is held.
	*
	* @impulse: The impulse number
	* @holdTime: The time the button has been held down
	*/
	void					PerformKeyRepeat(int impulse, int holdTime);

	/**
	* Ishtvan: Start tracking a mouse gesture that started when the key "impulse" was pressed
	* Discretizes analog mouse movement into a few different gesture possibilities
	* "Impulse" arg can also be a "button," see the UB_* enum in usercmdgen.h
	* 
	* Waits until the threshold mouse input is reached before deciding
	* The test argument determines what to test for (up/down, left/right, etc)
	*	determined by the enum EMouseTest
	* TurnHinderane sets the max palyer view turn rate when checking this mouse gesture
	*	0 => player view locked, 1.0 => no effect on view turning
	* DecideTime is the time in milliseconds after which the mouse gesture is auto-decided,
	*	in the event that the mouse movement threshold was not reached.
	* For now, only one mouse gesture check at a time.
	**/
	void					StartMouseGesture( int impulse, int thresh, EMouseTest test, bool bInverted, float TurnHinderance, int DecideTime = -1, int DeadTime = 0 );
	void					UpdateMouseGesture( void );
	void					StopMouseGesture( void );
	/**
	* Returns the result of the last mouse gesture (MOUSEDIR_* enum)
	**/
	EMouseDir				GetMouseGesture( void );

	void					Spectate( bool spectate );
	void					ToggleScoreboard( void );

	void					RouteGuiMouse( idUserInterface *gui );
	void					UpdateHUD();

	// greebo: Checks if any messages are still pending.
	void					UpdateHUDMessages();

	// Updates the HUD for the inventory items
	void					UpdateInventoryHUD();
	void					UpdateInventoryPickedUpMessages();

	void					SetInfluenceFov( float fov );
	void					SetInfluenceView( const char *mtr, const char *skinname, float radius, idEntity *ent );
	void					SetInfluenceLevel( int level );
	int						GetInfluenceLevel( void ) { return influenceActive; };
	void					SetPrivateCameraView( idCamera *camView );
	idCamera *				GetPrivateCameraView( void ) const { return privateCameraView; }
	void					StartFxFov( float duration  );
	void					UpdateHudWeapon( bool flashWeapon = true );
	void					UpdateHudStats( idUserInterface *hud );
	void					UpdateHudAmmo();

	virtual void			ClientPredictionThink( void ) override;
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;
	void					WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg );

	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg &msg ) override;
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) override;
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) override;
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;

	bool					IsReady( void );
	bool					IsRespawning( void );
	bool					IsInTeleport( void );

	idEntity				*GetInfluenceEntity( void ) { return influenceEntity; };
	const idMaterial		*GetInfluenceMaterial( void ) { return influenceMaterial; };
	float					GetInfluenceRadius( void ) { return influenceRadius; };

	// server side work for in/out of spectate. takes care of spawning it into the world as well
	void					ServerSpectate( bool spectate );
	// for very specific usage. != GetPhysics()
	idPhysics_Player		*GetPlayerPhysics( void );
	void					TeleportDeath( int killer );
	void					SetLeader( bool lead );
	bool					IsLeader( void );

	void					UpdateSkinSetup( bool restart );

	bool					IsShoulderingBody( void ) { return m_bShoulderingBody; };

	bool					IsForcedCrouch( void );
	void					ResetForcedCrouchMantle( void );

	// Daft Mugi #6316: Hold Frob for alternate interaction
	bool					IsHoldFrobEnabled( void );
	bool					CanHoldFrobAction( void );
	void					SetHoldFrobView( void );
	float					HoldFrobViewDistance( void );
	bool					IsAdditionalHoldFrobDraggableType(idEntity* target);
	bool					IsUsedItemOrJunk(idEntity* target);

	virtual bool			OnLadder( void ) const override;
	virtual CMultiStateMover* OnElevator(bool mustBeMoving) const override;

	bool					SelfSmooth( void );

	void					SetSelfSmooth( bool b );


	/**
	* Do the frobbing trace and bounds intersection to mark entities as frobable
	**/
	void					PerformFrobCheck();
	void					PerformFrobCheckInternal();

	/**
	 * greebo: Performs a frob action on the given entity. The above method
	 * PerformFrob() without arguments redirects the call to this one.
	 * This method might be invoked by scripts as well to simulate 
	 * a frob action without having the player to hit any buttons.
	 * 
	 * @impulseState: the button state of the frob key. Pass EPressed if you
	 * want to simulate a one-time frob event.
	 *
	 * @allowUseCurrentInvItem (#5542): if true, then also try to use currently selected inventory item
	 * if "Use-by-frobbing" (tdm_inv_use_on_frob) player setting is on.
	 * This happens e.g. when frobbing door while having lockpick/key selected.
	 * When run from game script, it must always be false!
	 *
	 * Hold time: The amount of time the button has been held, if applicable (0 by default)
	 */
	void					PerformFrob(EImpulseState impulseState, idEntity* frobbed, bool allowUseCurrentInvItem);

	// Gets called when the player hits the frob button.
	void					PerformFrob();
	// Gets repeatedly called when the player holds down the frob button
	void					PerformFrobKeyRepeat(int holdTime);
	// Gets called when the player releases the frob button
	void					PerformFrobKeyRelease(int holdTime);

	// Obsttorte: #5984 (multilooting)
	bool					multiloot;
	int						multiloot_lastfrob;

	// Daft Mugi #6316: Hold Frob for alternate interaction
	idEntityPtr<idEntity>   holdFrobEntity;
	idEntityPtr<idEntity>   holdFrobDraggedEntity;
	int                     holdFrobStartTime;
	idMat3                  holdFrobStartViewAxis;

	// angua: Set ideal crouch state
	void					EvaluateCrouch();

	ID_INLINE bool			GetIdealCrouchState()
	{
		return m_IdealCrouchState;
	}


	/**
	 * CalculateWeakLightgem (formerly AdjustLightgem) will do the "weak" 
	 * lightgem calculation, based on a list of lights, not using the rendershot.
	 */
	float	CalculateWeakLightgem();

	// Register/unregister a light for the weak lightgem calculation
	int		AddLight(idLight* lightToAdd);
	int		RemoveLight(idLight* lightToRemove);

	/**
	 * GetHeadEntity will return the entity for the head of the playermodel
	 */
	idEntity *GetHeadEntity(void) { return head.GetEntity(); };

	/**
	* Update movement volumes: Reads the movement volume
	* modifiers from cvars (for now)
	**/
	virtual void UpdateMoveVolumes( void ) override;

	/**
	* Get the volume modifier for a given movement type
	**/
	virtual float GetMovementVolMod( void ) override;

	/**
	 * greebo: Cycles through the inventory and opens the next map.
	 * If no map is displayed currently, the first map is toggled.
	 * If there is a map currently on the HUD, the next one is chosen.
	 * If there is no next map, the map is closed again.
	 */
	void					NextInventoryMap();

	// Shows/hides the in-game objectives GUI
	void					ToggleObjectivesGUI();

	// Shows/hides the in-game inventory grid GUI -- Durandall #4286
	void					ToggleInventoryGridGUI();

	/**
	* Physically test whether an item would fit in the world when
	* dropped with its center of mass (not origin!) at the specified point
	* Initially tries the supplied orientation, and tries all possible 90 degree
	* rotations, overwriting the supplied orientation with the one that fits
	**/
	bool TestDropItemRotations( idEntity *ent, idVec3 viewPoint, idVec3 DropPoint, idMat3 &DropAxis );

	/**
	* Uses the currently held/selected item.
	**/
	void UseInventoryItem();

	/**
	* Tries to drop the currently held/selected item.
	**/
	void DropInventoryItem();

	/**
	* Physically put the item in the hands, returns true if it fits
	* Called by DropInventoryItem, also called from Grabber::Dequip
	* When dropping an item from the inventory, item argument should be supplied
	* When dropping other objects (e.g., equipped junk item), it need not be
	**/
	bool DropToHands( idEntity *ent, CInventoryItemPtr item = CInventoryItemPtr() );

	// Performs the inventory action for onButtonRepeat
	void InventoryUseKeyRepeat(int holdTime);

	// Performs the inventory action for onButtonRelease
	void InventoryUseKeyRelease(int holdTime);

	// Uses a specific inventory item
	bool UseInventoryItem(EImpulseState nState, const CInventoryItemPtr& item, int holdTime, bool isFrobUse);

	// Changes the inventory selection to the item with the given name
	// Returns false if there is no such item
	bool SelectInventoryItem(const idStr& name);

	// Override idEntity method to get notified upon item changes
	virtual void OnInventoryItemChanged() override;

	// Override idEntity method: the "selection changed" event is called after the inventory cursor has changed its position
	virtual void OnInventorySelectionChanged(const CInventoryItemPtr& prevItem) override;

	/**
	 * Overload the idEntity::AddToInventory method to catch weapon items.
	 */
	virtual CInventoryItemPtr AddToInventory(idEntity *ent) override;

	/**
	 * greebo: Attempts to put the current grabber item back into the inventory.
	 *
	 * @returns: TRUE if an item was put back, FALSE if the grabber hands are empty.
	 */
	bool AddGrabberEntityToInventory();

	// Returns the current lightgem value
	int GetCurrentLightgemValue() { return m_LightgemValue; }
	float GetCalibratedLightgemValue();

	// Runs the (strong) lightgem calculation, returns the resulting value
	int		ProcessLightgem(bool processing);
	
	/**
	 * greebo: Returns the lightgem modifier value according to the currently selected inventory items
	 * and other factors (like crouching). Returns a value between 0 and DARKMOD_LG_MAX. 
	 * This value is added to the calculated lightgem value.
	 */
	int GetLightgemModifier(int curLightgemValue);

	/// Am I a ranged threat to the given entity (or entities in general if target is NULL)?
	virtual float	RangedThreatTo(idEntity* target) override;
	
	// greebo: Sends a message to the HUD (used for "Game Saved" and such).
	void			SendHUDMessage(const idStr& text);

	// greebo: Sends a "picked up so and so" message to the inventory HUD
	void			SendInventoryPickedUpMessage(const idStr& text);

	// Updates the in-game Objectives GUI, if visible (otherwise nothing happens)
	void			UpdateObjectivesGUI();

	// Updates the in-game Inventory Grid GUI, if visible (otherwise nothing happens) -- Durandall #4286
	void			UpdateInventoryGridGUI();

	void			PrintDebugHUD();

	// Runs the "Click when ready" GUI, returns TRUE if the player is ready
	bool			WaitUntilReady();

	// stgatilov #2454: enable/disable subtitles overlay and update active text to be displayed
	void			UpdateSubtitlesGUI();

	/**
	* greebo: Sets the time between health "pulses" if the healthPool > 0
	*
	* @newTimeInterval: the new value for the time interval
	* @factor: The factor that the time interval is being multiplied with after each pulse.
	*          This can be used to increase the time between pulses gradually.
	* @stepAmount: The amount of health to be taken from the healthpool at each step
	*/
	void			setHealthPoolTimeInterval(int newTimeInterval, float factor, int stepAmount);


	/** 
	* Get the current idLocation entity for the location the player is in 
	**/
	idLocationEntity *GetLocation( void );

	bool			CheckPushEntity(idEntity *entity); // grayman #4603
	void			ClearPushEntity(); // grayman #4603

protected:
	/**
	* greebo: This creates all the default inventory items and adds the weapons.
	*/
	void SetupInventory();

	void AddPersistentInventoryItems();

	// greebo: Methods used to manage the GUI layer for the in-game objectives
	void CreateObjectivesGUI();
	void DestroyObjectivesGUI();

	// Methods used to manage the GUI layer for the in-game inventory grid -- Durandall #4286
	void CreateInventoryGridGUI();
	void DestroyInventoryGridGUI();

	/**
	* greebo: Parses the spawnargs for any weapon definitions and adds them
	* to the inventory. Expects the weapon category to exist.
	*/
	void AddWeaponsToInventory();

	// Sorts the weapon item category by weapon index
	void SortWeaponItems();

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
	float					idealLegsYaw;
	float					legsYaw;
	bool					legsForward;
	float					oldViewYaw;
	idAngles				viewBobAngles;
	idVec3					viewBob;
	int						landChange;
	int						landTime;
	int						lastFootstepPlaytime;
	bool					isPushing;			// is true while the player is pushing something heavy

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
	int						airTics;				// set to pm_airTics at start, drops in water
	int						lastAirDamage, lastAirCheck;
	
	bool					underWaterEffectsActive; // True, if the under water effects are in charge
	int						underWaterGUIHandle;	 // The handle of the GUI underwater overlay

	bool					gibDeath;
	bool					gibsLaunched;
	idVec3					gibsDir;
	int						m_InventoryOverlay;

	// The GUI handle used by the in-game objectives display 
	int						objectivesOverlay;

	// The GUI handle used by the in-game inventory grid display -- Durandall #4286
	int						inventoryGridOverlay;

	// The GUI handle used by the in-game subtitles -- stgatilov #2454
	int						subtitlesOverlay;

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

	/**
	* Location of the player's ears for sound rendering
	**/
	idVec3					m_PrimaryListenerLoc; // grayman #4882

	/**
	* Location of the player's ear point when the player is leaning against
	* a door (i.e., a point on the other side of the door)
	**/
	idVec3					m_ListenLoc;

	/**
	* Location of an activated Listener entity. (grayman #4882)
	**/
	idVec3					m_SecondaryListenerLoc;

	/**
	* m_immobilization keeps track of sources of immobilization.
	* m_immobilizationCache caches the total immobilization so it
	* only gets recalculated when something is changed.
	**/
	idDict					m_immobilization;
	int						m_immobilizationCache;

	/**
	* m_hinderance keeps track of sources of hinderance. (slowing the player)
	* m_hinderanceCache caches the current hinderance level so it
	* only gets recalculated when something is changed.
	**/
	idDict					m_hinderance;
	float					m_hinderanceCache;

	/**
	* m_TurnHinderance keeps track of sources of turn hinderance.
	* These slow down the rate at which the player can turn their view
	* m_TurnHinderanceCache works the same as m_hinderanceCache above
	**/
	idDict					m_TurnHinderance;
	float					m_TurnHinderanceCache;

	/**
	* m_JumpHinderance keeps track of sources of jump height hinderance.
	* These limit the height which the player can jump.
	* m_JumpHinderanceCache works the same as m_hinderanceCache above
	**/
	idDict					m_JumpHinderance;
	float					m_JumpHinderanceCache;

	/**
	 * greebo: This is the list of named lightgem modifier values. These can be accessed
	 *         via script events to allow several modifiers to be active at the same time.
	 *         To save for performance, the sum of these values is stored in m_LightgemModifier
	 */
	std::map<std::string, int>	m_LightgemModifierList;

	// greebo: The sum of the values in the above list
	int						m_LightgemModifier;

	/**
	 * Each light entity must register here itself. This is used
	 * to calculate the value for the weak lightgem.
	 */
	idList<idLight*>		m_LightList;

	/**
	 * LightgemValue determines the level of visibility of the player.
	 * This value is used to light up the lightgem and is defined as
	 * DARKMOD_LG_MIN (1) <= N <= DARKMOG_LG_MAX (32)
	 */
	int							m_LightgemValue;

	/**
	 * Contains the last lightgem value. This is stored for interleaving.
	 */
	float						m_fColVal;
	float						m_fBlendColVal;						// Store the result for smooth fading of lightgem - J.C.Denton

	// An integer keeping track of the lightgem interleaving
	int							m_LightgemInterleave;
	
	// grayman #597 - ignore attack button if depressed, but weapon has been lowered
	bool						ignoreWeaponAttack;

	bool					displayAASAreas;	// grayman #3032 - no need to save/restore

	static const int		NUM_LOGGED_VIEW_ANGLES = 64;		// for weapon turning angle offsets
	idAngles				loggedViewAngles[NUM_LOGGED_VIEW_ANGLES];	// [gameLocal.framenum&(LOGGED_VIEW_ANGLES-1)]
	static const int		NUM_LOGGED_ACCELS = 16;			// for weapon turning angle offsets
	loggedAccel_t			loggedAccel[NUM_LOGGED_ACCELS];	// [currentLoggedAccel & (NUM_LOGGED_ACCELS-1)]
	int						currentLoggedAccel;

	idUserInterface *		cursor;
	
	// full screen guis track mouse movements directly
	int						oldMouseX;
	int						oldMouseY;

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
	unsigned int			lastSnapshotSequence;	// track state hitches on clients
	bool					weaponCatchup;			// raise up the weapon silently ( state catchups )

	bool					selfSmooth;


	void					LookAtKiller( idEntity *inflictor, idEntity *attacker );

	void					StopFiring( void );
	void					FireWeapon( void );
	void					BlockWeapon( void );
	void					Weapon_Combat( void );
	void					Weapon_GUI( void );
	void					UpdateWeapon( void );

	/**
	 * greebo: Changes the projectileDef name of the weapon inventory item with the given name.
	 * The name is defined in the "inv_weapon_name" spawnarg in the weaponDef.
	 */
	void					ChangeWeaponProjectile(const idStr& weaponName, const idStr& projectileDefName);

	// greebo: Resets the weapon projectile as originally defined in the weaponDef
	void					ResetWeaponProjectile(const idStr& weaponName);

	/**
	 * greebo: Changes the name of the given weapon item to a new string. Pass an empty string "" to this
	 * function to reset the name to the value defined in the weaponDef.
	 */
	void					ChangeWeaponName(const idStr& weaponName, const idStr& displayName);

	void					UpdateSpectating( void );
	void					SpectateFreeFly( bool force );	// ignore the timeout to force when followed spec is no longer valid
	void					SpectateCycle( void );
	idAngles				GunTurningOffset( void );
	idVec3					GunAcceleratingOffset( void );

	void					UseObjects( void );
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
	void					SetSpectateOrigin( void );

	void					UpdateLocation( void );
	idUserInterface *		ActiveGui( void );
	void					ExtractEmailInfo( const idStr &email, const char *scan, idStr &out );

	void					UseVehicle( void );

	void					ClearActiveInventoryMap();

	// Considers item limits as defined in atdm:campaign_info entities placed in the map
	// All items exceeding the defined limits are removed from the player's inventory
	void					EnforcePersistentInventoryItemLimits();

	void					CollectItemsAndCategoriesForInventoryGrid( idList<CInventoryItemPtr> &items, idStr &categoryNames, idStr &categoryValues );

	void					Event_GetButtons( void );
	void					Event_GetMove( void );
	void					Event_GetViewAngles( void );
	void					Event_SetViewAngles( const idVec3* vec );
	void					Event_StopFxFov( void );
	void					Event_EnableWeapon( void );
	void					Event_DisableWeapon( void );
	void					Event_GetCurrentWeapon( void );
	void					Event_GetPreviousWeapon( void );
	void					Event_SelectWeapon( const char *weaponName );
	void					Event_GetWeaponEntity( void );
	void					Event_ExitTeleporter( void );
	void					Event_Gibbed( void );
	void					Event_GetIdealWeapon( void );
	void					Event_RopeRemovalCleanup( idEntity *RopeEnt );
	void					Event_GetCalibratedLightgemValue( void );
	void					Event_SetAirAccelerate( float newAccel );
	void					Event_IsAirborne( void );
	
/**
* TDM Events
**/
	void					Event_GetEyePos( void );
	void					Event_SetImmobilization( const char *source, int type );
	void					Event_GetImmobilization( const char *source );
	void					Event_GetNextImmobilization( const char *prefix, const char *lastMatch );
	void					Event_SetHinderance( const char *source, float mCap, float aCap );
	void					Event_GetHinderance( const char *source );
	void					Event_GetNextHinderance( const char *prefix, const char *lastMatch );
	void					Event_SetTurnHinderance( const char *source, float mCap, float aCap );
	void					Event_GetTurnHinderance( const char *source );
	void					Event_GetNextTurnHinderance( const char *prefix, const char *lastMatch );


	void					Event_SetGui( int handle, const char *guiFile );
	void					Event_GetInventoryOverlay(void);

	void					Event_PlayStartSound( void );

	// greebo: Gets posted when the player is dead and the "custom_death_delay" spawnarg is set to a positive value on worldspawn
	void					Event_CustomDeath();

	void					Event_MissionFailed( void );
	void					Event_LoadDeathMenu( void );

	void					Event_HoldEntity( idEntity *ent );
	void					Event_HeldEntity( void );

	/**
	* Return the last mouse gesture result to the script
	**/
	void					Event_GetMouseGesture( void );
	/**
	* Return to script whether we are currently waiting for a mouse gesture to finish
	**/
	void					Event_MouseGestureFinished( void );
	/**
	* Clear the mouse dead time if it extends beyond the time used to determine the gesture
	**/
	void					Event_ClearMouseDeadTime( void );

	/**
	 * greebo: Sets the named lightgem modifier to a particular value. 
	 * Setting the modifier to 0 removes it from the internal list.
	 */
	void					Event_SetLightgemModifier(const char* modifierName, int amount);

	/**
	 * greebo: Reads the lightgem modifier setting from the worldspawn entity (defaults to 0).
	 */
	void					Event_ReadLightgemModifierFromWorldspawn();

	/**
	* NOTE: The following objective functions all take the "user" objective indices
	* That is, the indices start at 1 instead of 0
	*
	* If the objective/component for that index was not found
	* The getters return -1 for completion state and FALSE for component state
	**/
	void					Event_SetObjectiveState( int ObjIndex, int State );
	void					Event_GetObjectiveState( int ObjIndex );
	void					Event_SetObjectiveComp( int ObjIndex, int CompIndex, int bState );
	void					Event_GetObjectiveComp( int ObjIndex, int CompIndex );
	void					Event_ObjectiveUnlatch( int ObjIndex );
	void					Event_ObjectiveComponentUnlatch( int ObjIndex, int CompIndex );
	void					Event_SetObjectiveVisible( int ObjIndex, bool bVal );
	void					Event_GetObjectiveVisible( int ObjIndex );
	void					Event_SetObjectiveOptional( int ObjIndex, bool bVal );
	void					Event_SetObjectiveOngoing( int ObjIndex, bool bVal );
	void					Event_SetObjectiveEnabling( int ObjIndex, const char *strIn );
	void					Event_SetObjectiveText( int ObjIndex, const char *descr );

	// Obsttorte: (#5967) Enable/Disable Objective State Notifications
	void					Event_SetObjectiveNotification(bool ObjNote);

	/**
	* greebo: This scriptevent routes the call to the member method "GiveHealthPool".
	*/
	void					Event_GiveHealthPool( float amount );

	/** Returns true if we were damaged this frame **/
	void					Event_WasDamaged( void );

	/**
	 * greebo: These scriptevents handle the player zoom in/out behaviour.
	 */
	void					Event_StartZoom(float duration, float startFOV, float endFOV);
	void					Event_EndZoom(float duration);
	void					Event_ResetZoom();
	void					Event_GetFov();

	// Objectives GUI-related events
	void					Event_Pausegame();
	void					Event_Unpausegame();

	// Ends the game (fade out, success.gui, etc.)
	void					Event_MissionSuccess();

	// greebo: This event prepares the running map for mission success.
	// Basically waits for any HUD messages and fades out the screen, afterwards
	// the Mission Success event is called.
	void					Event_TriggerMissionEnd();

	// Disconnects the player from the mission, this is the final action during gameplay
	void					Event_DisconnectFromMission();

	/** Returns to script the current idLocation of the player **/
	void					Event_GetLocation();

	// Gets called in the first few frames
	void					Event_StartGamePlayTimer();

	// Checks the AAS status and displays the HUD warning
	void					Event_CheckAAS();

	void					Event_SetSpyglassOverlayBackground(); // grayman #3807

	void					Event_SetPeekOverlayBackground(); // grayman #4882

	// Changes the projectile def name of the given weapon inventory item
	void					Event_ChangeWeaponProjectile(const char* weaponName, const char* projectileDefName);
	void					Event_ResetWeaponProjectile(const char* weaponName);
	void					Event_ChangeWeaponName(const char* weaponName, const char* newName);
	void					Event_GetCurWeaponName();

	// Clears any active inventory maps
	void					Event_ClearActiveInventoryMap();

	// Sets the currently active map (feedback method for inventory map scripts)
	void					Event_SetActiveInventoryMapEnt(idEntity* mapEnt);

	void					Event_ClearActiveInventoryMapEnt(); // grayman #3164
	
	// return the frobbed entity
	void					Event_GetFrobbed() const;
	// enables "frob only ents used by active inventory item" mode
	void					Event_SetFrobOnlyUsedByInv( bool Value );

	// tels: #3282
	void					Event_GetShouldered() const;
	void					Event_GetDragged() const;
	void					Event_GetGrabbed() const;

	// Call gameLocal.ProcessInterMissionTriggers
	void					Event_ProcessInterMissionTriggers();

	// Obsttorte: event to save the game
	void					Event_saveGame(const char* name);


	void					Event_setSavePermissions(int sp);

	void					Event_SetPeekView(int state, idVec3 peekViewOrigin); // grayman #4882

	void					Event_IsLeaning(); // grayman #4882
	void					Event_IsPeekLeaning(); // Obsttorte
	void					Event_GetListenLoc(); // Obsttorte #5899

	//stgatilov: testing script-cpp interop
	void Event_TestEvent1(float float_pi, int int_beef, float float_exp, const char *string_tdm, float float_exp10, int int_food);
	void Event_TestEvent2(int int_prevres, idVec3 *vec_123, int int_food, idEntity *ent_player, idEntity *ent_null, float float_pi, float float_exp);
	void Event_TestEvent3(idEntity *ent_prevres, idVec3 *vec_123, float float_pi, idEntity *ent_player);
};

ID_INLINE bool idPlayer::IsReady( void ) {
	return ready || forcedReady;
}

ID_INLINE bool idPlayer::IsRespawning( void ) {
	return respawning;
}

ID_INLINE idPhysics_Player* idPlayer::GetPlayerPhysics( void ) {
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

