/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"


// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			500
#define	PULSE_TIME			2000
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	cg_weaponselecttime.integer
#define	ITEM_SCALEUP_TIME	cg_itemscaletime.value
#define	ZOOM_TIME			cg_zoomtime.value
#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	75
#define	SINK_TIME			3000		// time for fragments to sink into ground before going away
#define	ATTACKER_HEAD_TIME	10000
#define	REWARD_TIME			2000
#define OBJECTIVES_TIME		2500		//time for objectives updated notification to remain on screen
#define BLACKOUT_TIME		100.000		//time for the screen to remain black at start of game
#define	FADEIN_TIME			1500.000		//amount of time it takes for screen to fade in at start of game
#define TITLE_TIME			5000		//amount of time the level title stays on screen
#define TITLE_FADEIN_TIME	2000.000	//amount of time it takes for level title to fade in
#define TITLE_FADEOUT_TIME	5000.000	//amount of time it takes for level title to fade out
#define SCOREB_TIME			750			//amount of time between each scoreboard item is displayed
#define SCOREB_TIME_LAST	250			//amount of EXTRA time between last scoreboard item and total score

#define MAX_NOTIFICATIONS 8
#define NOTIFICATION_DURATION 10000
#define NOTIFICATION_FADE_TIME 750

#define	PULSE_SCALE			1.15			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	128
#define	MAX_MARK_POLYS		4096

#define STAT_MINUS			10	// num frame for '-' stats digit

/*
#define	ICON_SIZE			48
#define	CHAR_WIDTH			32
#define	CHAR_HEIGHT			48
#define	TEXT_ICON_SPACE		4*/


#define	ICON_SIZE			28
#define	CHAR_WIDTH			19
#define	CHAR_HEIGHT			28
#define	TEXT_ICON_SPACE		2

#define	TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

// very large characters.pk
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#define	NUM_CROSSHAIRS		99

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define	DEFAULT_MODEL			"sarge"
#define	DEFAULT_TEAM_MODEL		"sarge"
#define	DEFAULT_TEAM_HEAD		"sarge"

#define DEFAULT_REDTEAM_NAME		"Vim supporters"
#define DEFAULT_BLUETEAM_NAME		"Emacs supporters"

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

typedef enum {
	LFS_LEVELLOADED,		//the level has just been loaded
	LFS_BLACKOUT,			//busy with the blackout
	LFS_FADEIN,				//busy with the fade in
	LFS_IDLE				//not doing any map change related fades
} levelFadeStatus_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

//Notifications
typedef struct {
    char text[128];
    int type;
    int startTime;
    qboolean active;
} notification_t;


typedef struct {
	lerpFrame_t		legs, torso, flag;
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	int				lightningFiring;

	// railgun trail spawning
	vec3_t			railgunImpact;
	qboolean		railgunFlash;

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;

	// eye stuff...

	vec3_t			eyepos;		// where our eyes at
	vec3_t			eyelookat;	// what we seein'
	lerpFrame_t		head;
} playerEntity_t;

//=================================================



// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;	// from cg.frame
	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int				errorTime;		// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;

	qboolean		extrapolated;	// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;

	int				newcamrunning;	// leilei - determines if we should look in a direction for running
	vec3_t			eyesOrigin;
	vec3_t			eyesAngles;

	vec3_t			eyepos;		// where our eyes at
	vec3_t			eyepos2;	// where our other eyes at
	vec3_t			eyelookat;	// what we seein'

	vec3_t			weapOrigin;	// leilei - for lazy bob
	vec3_t			weapAngles;
} centity_t;

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_FRAGMENT2,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
	LE_KAMIKAZE,
	LE_INVULIMPACT,
	LE_INVULJUICED,
	LE_SHOWREFENTITY,
	LE_GORE
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001,			// do not scale size over time
	LEF_TUMBLE			 = 0x0002,			// tumble over time, used for ejecting shells
	LEF_SOUND1			 = 0x0004,			// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008			// sound 2 for kamikaze
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS,
	LEBS_SHELL
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef enum {
	LETT_NONE,				// does not emit a puff trail
	LETT_BLOOD,				// emits a blood trail
	LETT_DEBRIS_CONCRETE,	// emits a (gray) smoke trail
	LETT_DEBRIS_WOOD		// emits a (brown) dust trail
} leTrailType_t;		// defines bounce behavior and trail on fragment local entities

typedef enum {
	PT_GRAVITY,
	PT_LINEAR_UP,
	PT_LINEAR_DOWN,
	PT_LINEAR_BOTH
} particleType_t;

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;	// sound to play on fragment impact
	leTrailType_t		leTrailType;		// trail to show behind fragment

	refEntity_t		refEntity;
} localEntity_t;

typedef struct {
	int				client;
	int				score;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerUps;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				guantletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	qboolean	perfect;
	int				team;
	int			isDead;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	32

typedef struct {
	qboolean		infoValid;

	char			name[MAX_QPATH];
	team_t			team;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	int				isNPC;		// 0 = not NPC, 1 = NPC

	vec3_t			color1;

	int			helred;
	int			helgreen;
	int			helblue;
	int			tolred;
	int			tolgreen;
	int			tolblue;
	int			plred;
	int			plgreen;
	int			plblue;
	float		pg_red;
	float		pg_green;
	float		pg_blue;
	int			swepid;
	int			vehiclenum;
	int			totex;
	int			hetex;
	int			plradius;

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				health;			// you only get this info about your teammates
	int				armor;
	int				curWeapon;

	int				handicap;
	int				wins, losses;	// in tourney mode

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader
	int				flashlight;

	int				powerups;		// so can display quad/flag status

	int				medkitUsageTime;
	int				invulnerabilityStartTime;
	int				invulnerabilityStopTime;

	int				breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
	char			headModelName[MAX_QPATH];
	char			headSkinName[MAX_QPATH];
	char			legsModelName[MAX_QPATH];
	char			legsSkinName[MAX_QPATH];
	char			redTeam[MAX_TEAMNAME];
	char			blueTeam[MAX_TEAMNAME];
	qboolean		deferred;

	qboolean		newAnims;		// true if using the new mission pack animations
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	gender_t		gender;			// from model

	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	qhandle_t		legsShader;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	qhandle_t		torsoShader;

	qhandle_t		headModel;
	qhandle_t		headSkin;
	qhandle_t		headShader;
	
	qhandle_t		plusModel;
	qhandle_t		plusSkin;

	qhandle_t		modelIcon;

	animation_t		animations[MAX_TOTALANIMATIONS];

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];

	int		isDead;
	vec3_t			eyepos;		// leilei - eye positions loaded from anim cfg
	int		onepiece;		// leilei - g_enableFS meshes
} clientInfo_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	gitem_t			*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;


	qhandle_t		ammoModel;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	void			(*ejectBrassFunc)( centity_t * );

	float			trailRadius;
	float			wiTrailTime;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
	qboolean		loopFireSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define MAX_SKULLTRAIL		20

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;


#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16

//unlagged - optimized prediction
#define NUM_SAVED_STATES (CMD_BACKUP + 2)
//unlagged - optimized prediction

typedef struct {
	int			clientFrame;		// incremented each frame

	int			clientNum;

	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t	activeSnapshots[2];

	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking

	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time

	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc
	int			renderingEyesPerson;		// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;
	
	int			swep_listcl[WEAPONS_NUM];
	int			swep_spawncl[WEAPONS_NUM];		//stores spawn weapons

	float		savedSens;						//physgun

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	qboolean	teamoverlay;

	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	score_t		scores[MAX_CLIENTS];
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;

	char			killerName[MAX_NAME_LENGTH];
	char			spectatorList[MAX_STRING_CHARS];		// list of names
	int				spectatorLen;												// length of list
	float			spectatorWidth;											// width in device units
	int				spectatorTime;											// next time to offset
	int				spectatorPaintX;										// current paint x
	int				spectatorPaintX2;										// current paint x
	int				spectatorOffset;										// current offset from start
	int				spectatorPaintLen; 									// current offset from start

	// skull trails
	skulltrail_t	skulltrails[MAX_CLIENTS];

	// centerprinting
	int			centerPrintTime;
	int			centerPrintCharWidth;
	int			centerPrintY;
	char		centerPrint[256];
	int			centerPrintLines;
	int			centerPrintTimeC;

	//notifications
	notification_t notifications[MAX_NOTIFICATIONS];

	// kill timers for carnage reward
	int			lastKillTime;

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;

	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
	qhandle_t	rewardSound[MAX_REWARDSTACK];

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];

	// warmup countdown
	int			warmup;
	int			warmupCount;

	//==========================

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int     nextOrbitTime;
	
	refEntity_t		viewfog[16];
	refEntity_t		viewsky;

	//qboolean cameraMode;		// if rendering from a loaded camera


	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

//unlagged - optimized prediction
	int			lastPredictedCommand;
	int			lastServerTime;
	playerState_t savedPmoveStates[NUM_SAVED_STATES];
	int			stateHead, stateTail;
//unlagged - optimized prediction

    //time that the client will respawn. If 0 = the player is alive.
    int respawnTime;
	//entityplus
	qboolean		footstepSuppressed; //hack to suppress initial footstep after first spawn

	// entityplus objectives
	int				objectivesTime;
	qboolean		objectivesSoundPlayed;

	// entityplus intermission
	int				intermissionTime;	//for timing the intermission scoreboard in entityplus mode
	int				scoreSoundsPlayed;	//number of sounds played during SP intermission scoreboard

	// entityplus death
	qboolean		deathmusicStarted; //true when a background track has been started for the player's death
	qboolean		musicStarted;	   //true when the normal background track has been started

	// entityplus generic fades
	levelFadeStatus_t	levelFadeStatus;	//status for level fade in and -out
	int					levelStartTime;		//cg.time value for when the client loaded cgame
	int					fadeStartTime;		//starting time for the fade
	float				fadeDuration;		//duration of the fade
	vec4_t				fadeStartColor;		//color at the start of fade (r, g, b, a)
	vec4_t				fadeEndColor;		//color at the end of fade (r, g, b, a)

	// entityplus subtitles
	int				subtitlePrintTime;
	int				subtitlePrintCharWidth;
	int				subtitlePrintY;
	char			subtitlePrint[256];
	int				subtitlePrintLines;
	float			subtitlePrintDuration;

        int redObeliskHealth;
        int blueObeliskHealth;
} cg_t;


// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t	defaultFont[3];
	qhandle_t	whiteShader;
	qhandle_t 	corner;

	qhandle_t	redCubeModel;
	qhandle_t	blueCubeModel;
	qhandle_t	redCubeIcon;
	qhandle_t	blueCubeIcon;
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[4];

//For Double Domination:
	//qhandle_t	ddPointA;
	//qhandle_t	ddPointB;
	qhandle_t	ddPointSkinA[4]; //white,red,blue,none
        qhandle_t	ddPointSkinB[4]; //white,red,blue,none

	qhandle_t	flagPoleModel;
	qhandle_t	flagFlapModel;

	qhandle_t	redFlagFlapSkin;
	qhandle_t	blueFlagFlapSkin;
	qhandle_t	neutralFlagFlapSkin;

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

	qhandle_t	overloadBaseModel;
	qhandle_t	overloadTargetModel;
	qhandle_t	overloadLightsModel;
	qhandle_t	overloadEnergyModel;

	qhandle_t	harvesterModel;
	qhandle_t	harvesterRedSkin;
	qhandle_t	harvesterBlueSkin;
	qhandle_t	harvesterNeutralModel;

	qhandle_t	armorModel;
	qhandle_t	armorIcon;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

	// gib explosions
	qhandle_t	gibAbdomen;
	qhandle_t	gibArm;
	qhandle_t	gibChest;
	qhandle_t	gibFist;
	qhandle_t	gibFoot;
	qhandle_t	gibForearm;
	qhandle_t	gibIntestine;
	qhandle_t	gibLeg;
	qhandle_t	gibSkull;
	qhandle_t	gibBrain;

	// debris explosions
	qhandle_t	debrislight1;
	qhandle_t	debrislight2;
	qhandle_t	debrislight3;
	qhandle_t	debrislight4;
	qhandle_t	debrislight5;
	qhandle_t	debrislight6;
	qhandle_t	debrislight7;
	qhandle_t	debrislight8;

	qhandle_t	debrisdark1;
	qhandle_t	debrisdark2;
	qhandle_t	debrisdark3;
	qhandle_t	debrisdark4;
	qhandle_t	debrisdark5;
	qhandle_t	debrisdark6;
	qhandle_t	debrisdark7;
	qhandle_t	debrisdark8;

	qhandle_t	debrislightlarge1;
	qhandle_t	debrislightlarge2;
	qhandle_t	debrislightlarge3;

	qhandle_t	debrisdarklarge1;
	qhandle_t	debrisdarklarge2;
	qhandle_t	debrisdarklarge3;

	qhandle_t	debriswood1;
	qhandle_t	debriswood2;
	qhandle_t	debriswood3;
	qhandle_t	debriswood4;
	qhandle_t	debriswood5;

	qhandle_t	debrisglass1;
	qhandle_t	debrisglass2;
	qhandle_t	debrisglass3;
	qhandle_t	debrisglass4;
	qhandle_t	debrisglass5;

	qhandle_t	debrisglasslarge1;
	qhandle_t	debrisglasslarge2;
	qhandle_t	debrisglasslarge3;
	qhandle_t	debrisglasslarge4;
	qhandle_t	debrisglasslarge5;
	
	qhandle_t	debrisstone1;
	qhandle_t	debrisstone2;
	qhandle_t	debrisstone3;
	qhandle_t	debrisstone4;
	qhandle_t	debrisstone5;

	qhandle_t	sparkShader;

	qhandle_t	smoke2;

	qhandle_t	machinegunBrassModel;
	qhandle_t	shotgunBrassModel;

	qhandle_t	railRingsShader;
	qhandle_t	railCoreShader;

	qhandle_t	lightningShader;

	qhandle_t	friendShader;

	qhandle_t	balloonShader;
	qhandle_t	connectionShader;

	qhandle_t	selectShader;
	qhandle_t	viewBloodShader;
	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	crosshairSh3d[NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;

	qhandle_t	smokePuffShader;
	qhandle_t	smokePuffRageProShader;
	qhandle_t	shotgunSmokePuffShader;
	qhandle_t	plasmaBallShader;
	qhandle_t	flameBallShader;
	qhandle_t	antimatterBallShader;
	qhandle_t	waterBubbleShader;
	qhandle_t	bloodTrailShader;



	// LEILEI shaders

	qhandle_t	lsmkShader1;
	qhandle_t	lsmkShader2;
	qhandle_t	lsmkShader3;
	qhandle_t	lsmkShader4;
	qhandle_t	lbumShader1;
	qhandle_t	lfblShader1;
	qhandle_t	lsplShader;
	qhandle_t	lspkShader1;
	qhandle_t	lspkShader2;
	qhandle_t	lbldShader1;
	qhandle_t	lbldShader2;
	qhandle_t	grappleShader;	// leilei - grapple hook
	qhandle_t	lmarkmetal1;
	qhandle_t	lmarkmetal2;
	qhandle_t	lmarkmetal3;
	qhandle_t	lmarkmetal4;
	qhandle_t	lmarkbullet1;
	qhandle_t	lmarkbullet2;
	qhandle_t	lmarkbullet3;
	qhandle_t	lmarkbullet4;


	qhandle_t	nailPuffShader;
	qhandle_t	blueProxMine;


	qhandle_t	numberShaders[11];

	qhandle_t	shadowMarkShader;

	qhandle_t	botSkillShaders[14];

	// wall mark shaders
	qhandle_t	wakeMarkShader;
	qhandle_t	bloodMarkShader;
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// paintball mode shaders
	qhandle_t	bulletMarkPaintShader;
	qhandle_t	burnMarkPaintShader;
	qhandle_t	holeMarkPaintShader;
	qhandle_t	energyMarkPaintShader;

	// powerup shaders
	qhandle_t	quadShader;
	qhandle_t	redQuadShader;
	qhandle_t	quadWeaponShader;
	qhandle_t	invisShader;
	qhandle_t	regenShader;
	qhandle_t	battleSuitShader;
	qhandle_t	battleWeaponShader;
	qhandle_t	hastePuffShader;
	qhandle_t	redKamikazeShader;
	qhandle_t	blueKamikazeShader;

	qhandle_t	ptexShader[2];

	qhandle_t   HudLineBar;           
	qhandle_t   medalFrags;           
	qhandle_t   medalVictory;         
	qhandle_t   healthModel;          
	qhandle_t   healthSphereModel;    
	qhandle_t   healthIcon;          
	qhandle_t   weaponSelectShader;   
	qhandle_t   medalAccuracy;        
	qhandle_t   medalThaws;           
	qhandle_t   medalDeath;          
	qhandle_t   medalSpawn;           

        // player overlays
        qhandle_t       neutralOverlay;
        qhandle_t       redOverlay;
        qhandle_t       blueOverlay;

	// weapon effect models
	qhandle_t	bulletFlashModel;
	qhandle_t	ringFlashModel;
	qhandle_t	dishFlashModel;
	qhandle_t	lightningExplosionModel;

	// weapon effect shaders
	qhandle_t	railExplosionShader;
	qhandle_t	plasmaExplosionShader;
	qhandle_t	bulletExplosionShader;
	qhandle_t	rocketExplosionShader;
	qhandle_t	grenadeExplosionShader;
	qhandle_t	bfgExplosionShader;
	qhandle_t	bloodExplosionShader;

	// special effects models
	qhandle_t	teleportEffectModel;
	qhandle_t	teleportEffectShader;

	qhandle_t	kamikazeEffectModel;
	qhandle_t	kamikazeShockWave;
	qhandle_t	kamikazeHeadModel;
	qhandle_t	kamikazeHeadTrail;
	qhandle_t	guardPowerupModel;
	qhandle_t	scoutPowerupModel;
	qhandle_t	doublerPowerupModel;
	qhandle_t	ammoRegenPowerupModel;
	qhandle_t	invulnerabilityImpactModel;
	qhandle_t	invulnerabilityJuicedModel;
	qhandle_t	medkitUsageModel;
	qhandle_t	dustPuffShader;
	qhandle_t	heartShader;

	qhandle_t	invulnerabilityPowerupModel;

	// objectives screen
	qhandle_t	objectivesOverlay;
	qhandle_t	objectivesUpdated;
	sfxHandle_t	objectivesUpdatedSound;

	// target_effect overlay
	qhandle_t	effectOverlay;
	
	// postprocess
	qhandle_t	postProcess;

	// Icons QS
	qhandle_t	errIcon;
	qhandle_t	notifyIcon;

	// sp intermission scoreboard
	sfxHandle_t	scoreShow;
	sfxHandle_t finalScoreShow;

	// death view image
	qhandle_t	deathImage;

	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;

	// sounds
	sfxHandle_t	quadSound;
	sfxHandle_t	tracerSound;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t	carengine[11];
	sfxHandle_t	sfx_lghit1;
	sfxHandle_t	sfx_lghit2;
	sfxHandle_t	sfx_lghit3;
	sfxHandle_t	sfx_ric1;
	sfxHandle_t	sfx_ric2;
	sfxHandle_t	sfx_ric3;
	sfxHandle_t	sfx_railg;
	sfxHandle_t	sfx_rockexp;
	sfxHandle_t	sfx_plasmaexp;

	sfxHandle_t	sfx_proxexp;
	sfxHandle_t	sfx_nghit;
	sfxHandle_t	sfx_nghitflesh;
	sfxHandle_t	sfx_nghitmetal;
	sfxHandle_t	sfx_chghit;
	sfxHandle_t	sfx_chghitflesh;
	sfxHandle_t	sfx_chghitmetal;
	sfxHandle_t kamikazeExplodeSound;
	sfxHandle_t kamikazeImplodeSound;
	sfxHandle_t kamikazeFarSound;
	sfxHandle_t useInvulnerabilitySound;
	sfxHandle_t invulnerabilityImpactSound1;
	sfxHandle_t invulnerabilityImpactSound2;
	sfxHandle_t invulnerabilityImpactSound3;
	sfxHandle_t invulnerabilityJuicedSound;
	sfxHandle_t obeliskHitSound1;
	sfxHandle_t obeliskHitSound2;
	sfxHandle_t obeliskHitSound3;
	sfxHandle_t	obeliskRespawnSound;
	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;
	sfxHandle_t	youSuckSound;

	sfxHandle_t	gibSound;
	sfxHandle_t	gibBounce1Sound;
	sfxHandle_t	gibBounce2Sound;
	sfxHandle_t	gibBounce3Sound;
	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	noAmmoSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
	sfxHandle_t jumpPadSound;

	sfxHandle_t notifySound;

// LEILEI
	sfxHandle_t	lspl1Sound;
	sfxHandle_t	lspl2Sound; // Blood Splat Noises
	sfxHandle_t	lspl3Sound;

	sfxHandle_t	lbul1Sound;
	sfxHandle_t	lbul2Sound;	// Bullet Drop Noises
	sfxHandle_t	lbul3Sound;

	sfxHandle_t	lshl1Sound;
	sfxHandle_t	lshl2Sound; // Shell Drop Noises
	sfxHandle_t	lshl3Sound;

// LEILEI END

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;
	sfxHandle_t suddenDeathSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

	sfxHandle_t hitSound;
	sfxHandle_t hitSoundHighArmor;
	sfxHandle_t hitSoundLowArmor;
	sfxHandle_t hitTeamSound;
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
	sfxHandle_t assistSound;
	sfxHandle_t defendSound;
	sfxHandle_t firstImpressiveSound;
	sfxHandle_t firstExcellentSound;
	sfxHandle_t firstHumiliationSound;

	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;

	sfxHandle_t voteNow;
	sfxHandle_t votePassed;
	sfxHandle_t voteFailed;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t flightSound;
	sfxHandle_t medkitSound;

	sfxHandle_t weaponHoverSound;

	// teamplay sounds
	sfxHandle_t captureAwardSound;
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;

	sfxHandle_t	captureYourTeamSound;
	sfxHandle_t	captureOpponentSound;
	sfxHandle_t	returnYourTeamSound;
	sfxHandle_t	returnOpponentSound;
	sfxHandle_t	takenYourTeamSound;
	sfxHandle_t	takenOpponentSound;

	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
	sfxHandle_t neutralFlagReturnedSound;
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t	enemyTookTheFlagSound;
	sfxHandle_t yourTeamTookEnemyFlagSound;
	sfxHandle_t yourTeamTookTheFlagSound;
	sfxHandle_t	youHaveFlagSound;
	sfxHandle_t yourBaseIsUnderAttackSound;
	sfxHandle_t holyShitSound;

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
	sfxHandle_t	countPrepareSound;

	sfxHandle_t ammoregenSound;
	sfxHandle_t doublerSound;
	sfxHandle_t guardSound;
	sfxHandle_t scoutSound;

	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;

	sfxHandle_t	regenSound;
	sfxHandle_t	protectSound;
	sfxHandle_t	n_healthSound;
	sfxHandle_t	hgrenb1aSound;
	sfxHandle_t	hgrenb2aSound;
	sfxHandle_t	wstbimplSound;
	sfxHandle_t	wstbimpmSound;
	sfxHandle_t	wstbimpdSound;
	sfxHandle_t	wstbactvSound;

} cgMedia_t;

#define CONSOLE_MAXHEIGHT 16
#define CONSOLE_WIDTH 80
typedef struct {
	char	msgs[CONSOLE_MAXHEIGHT][CONSOLE_WIDTH*3+1];
	int	msgTimes[CONSOLE_MAXHEIGHT];
	int	insertIdx;
	int	displayIdx;
} console_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenXBias;

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t		gametype;
	int				dmflags;
        int                             videoflags;
        int				elimflags;
	int				teamflags;
	int				fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	char			mapname[MAX_QPATH];
	char			redTeam[MAX_QPATH];
	char			blueTeam[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;

//Forced FFA
	int			ffa_gt;

//Elimination
	int				roundStartTime;
	int				roundtime;

//CTF Elimination
	int				attackingTeam;

//Last Man Standing
	int				lms_mode;

//instantgib + nexuiz style rocket arena:
	int				nopickup;

//Double Domination DD
	int 				timetaken;

//Domination
	int domination_points_count;
	char domination_points_names[MAX_DOMINATION_POINTS][MAX_DOMINATION_POINTS_NAMES];
	int domination_points_status[MAX_DOMINATION_POINTS];

	int				scores1, scores2;		// from configstrings
	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	console_t commonConsole;
	console_t console;
	console_t chat;
	console_t teamChat;

	// teamchat width is *3 because of embedded color codes
	char			teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int				teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int				teamChatPos;
	int				teamLastChatPos;

	int cursorX;
	int cursorY;
	qboolean eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// orders
	int currentOrder;
	qboolean orderPending;
	int orderTime;
	int currentVoiceClient;
	int acceptOrderTime;
	int acceptTask;
	int acceptLeader;
	char acceptVoice[MAX_NAME_LENGTH];

	// media
	cgMedia_t		media;

//unlagged - client options
	// this will be set to the server's g_delagHitscan
	int				delagHitscan;
} cgs_t;

extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];
extern	weaponInfo_t	cg_weapons[WEAPONS_NUM+1];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

extern	int		mod_mgspread;
extern	int		mod_cgspread;
extern	int		mod_lgrange;
extern	int		mod_sgcount;
extern	int		mod_sgspread;
extern	int		mod_jumpheight;
extern 	int		mod_gdelay;
extern 	int		mod_mgdelay;
extern 	int		mod_sgdelay;
extern 	int		mod_gldelay;
extern 	int		mod_rldelay;
extern 	int		mod_lgdelay;
extern 	int		mod_pgdelay;
extern 	int		mod_rgdelay;
extern 	int		mod_bfgdelay;
extern 	int		mod_ngdelay;
extern 	int		mod_pldelay;
extern 	int		mod_cgdelay;
extern 	int		mod_ftdelay;
extern	int		mod_amdelay;
extern	float 	mod_hastefirespeed;
extern	float 	mod_ammoregenfirespeed;
extern	float 	mod_scoutfirespeed;
extern	float		mod_guardfirespeed;
extern	float		mod_doublerfirespeed;
extern	int		mod_noplayerclip;
extern	int		mod_ammolimit;
extern	int		mod_invulmove;
extern	float		mod_teamred_firespeed;
extern	float		mod_teamblue_firespeed;
extern	int 	mod_medkitlimit;
extern	int 	mod_medkitinf;
extern	int 	mod_teleporterinf;
extern	int 	mod_portalinf;
extern	int 	mod_kamikazeinf;
extern	int 	mod_invulinf;
extern	int 	mod_accelerate;
extern	int 	mod_slickmove;
extern	int 	mod_overlay;
extern	int 	mod_gravity;
extern	int 	mod_fogModel;
extern	int 	mod_fogShader;
extern	int 	mod_fogDistance;
extern	int 	mod_fogInterval;
extern	int 	mod_fogColorR;
extern	int 	mod_fogColorG;
extern	int 	mod_fogColorB;
extern	int 	mod_fogColorA;
extern	int 	mod_skyShader;
extern	int 	mod_skyColorR;
extern	int 	mod_skyColorG;
extern	int 	mod_skyColorB;
extern	int 	mod_skyColorA;

extern	vmCvar_t 	g_gametype;

extern	vmCvar_t 	cl_propsmallsizescale;
extern	vmCvar_t 	cl_propheight;
extern	vmCvar_t 	cl_propspacewidth;
extern	vmCvar_t 	cl_propgapwidth;
extern	vmCvar_t 	cl_smallcharwidth;
extern	vmCvar_t 	cl_smallcharheight;
extern	vmCvar_t 	cl_bigcharwidth;
extern	vmCvar_t 	cl_bigcharheight;
extern	vmCvar_t 	cl_giantcharwidth;
extern	vmCvar_t 	cl_giantcharheight;

extern	vmCvar_t	cg_gibjump;
extern	vmCvar_t	cg_gibvelocity;
extern	vmCvar_t	cg_gibmodifier;

extern	vmCvar_t	cg_zoomtime;
extern	vmCvar_t	cg_itemscaletime;
extern	vmCvar_t	cg_weaponselecttime;

//Noire Set
extern	vmCvar_t	toolgun_mod1;
extern	vmCvar_t	toolgun_mod2;
extern	vmCvar_t	toolgun_mod3;
extern	vmCvar_t	toolgun_mod4;
extern	vmCvar_t	toolgun_mod5;
extern	vmCvar_t	toolgun_mod6;
extern	vmCvar_t	toolgun_mod7;
extern	vmCvar_t	toolgun_mod8;
extern	vmCvar_t	toolgun_mod9;
extern	vmCvar_t	toolgun_mod10;
extern	vmCvar_t	toolgun_mod11;
extern	vmCvar_t	toolgun_mod12;
extern	vmCvar_t	toolgun_mod13;
extern	vmCvar_t	toolgun_mod14;
extern	vmCvar_t	toolgun_mod15;
extern	vmCvar_t	toolgun_mod16;
extern	vmCvar_t	toolgun_mod17;
extern	vmCvar_t	toolgun_mod18;
extern	vmCvar_t	toolgun_mod19;
extern	vmCvar_t	toolgun_tool;
extern	vmCvar_t	toolgun_toolcmd1;
extern	vmCvar_t	toolgun_toolcmd2;
extern	vmCvar_t	toolgun_toolcmd3;
extern	vmCvar_t	toolgun_toolcmd4;
extern	vmCvar_t	toolgun_tooltext;
extern	vmCvar_t	toolgun_tooltip1;
extern	vmCvar_t	toolgun_tooltip2;
extern	vmCvar_t	toolgun_tooltip3;
extern	vmCvar_t	toolgun_tooltip4;
extern	vmCvar_t	toolgun_toolmode1;
extern	vmCvar_t	toolgun_toolmode2;
extern	vmCvar_t	toolgun_toolmode3;
extern	vmCvar_t	toolgun_toolmode4;
extern	vmCvar_t	toolgun_modelst;
extern	vmCvar_t	sb_classnum_view;
extern	vmCvar_t	sb_texture_view;
extern	vmCvar_t	sb_texturename;
extern	vmCvar_t	cg_hide255;

extern	vmCvar_t	ns_haveerror;		//Noire.Script error

extern	vmCvar_t	cg_postprocess;
extern	vmCvar_t	cg_toolguninfo;
extern	vmCvar_t	cl_language;
extern	vmCvar_t	con_notifytime;
extern  vmCvar_t    cg_helightred;
extern  vmCvar_t    cg_helightgreen;
extern  vmCvar_t    cg_helightblue;
extern  vmCvar_t    cg_ptex;
extern  vmCvar_t    cg_totex;
extern  vmCvar_t    cg_hetex;
extern  vmCvar_t    cg_tolightred;
extern  vmCvar_t    cg_tolightgreen;
extern  vmCvar_t    cg_tolightblue;
extern  vmCvar_t    cg_plightred;
extern  vmCvar_t    cg_plightgreen;
extern  vmCvar_t    cg_plightblue;
extern  vmCvar_t    cg_plightradius;
extern  vmCvar_t 	cg_leiChibi;
extern  vmCvar_t    cg_cameraeyes;
extern  vmCvar_t    cl_screenoffset;
extern  vmCvar_t    ui_backcolors;
extern	vmCvar_t	legsskin;
extern	vmCvar_t	team_legsskin;
extern	vmCvar_t	cg_itemstyle;
extern	vmCvar_t	cg_gibtime;
extern	vmCvar_t		cg_centertime;
extern	vmCvar_t		cg_drawsubtitles;
extern	vmCvar_t		cg_drawSyncMessage;
extern	vmCvar_t		cg_runpitch;
extern	vmCvar_t		cg_runroll;
extern	vmCvar_t		cg_bobup;
extern	vmCvar_t		cg_bobpitch;
extern	vmCvar_t		cg_bobroll;
extern	vmCvar_t		cg_swingSpeed;
extern	vmCvar_t		cg_shadows;
extern	vmCvar_t		cg_gibs;
extern	vmCvar_t		cg_drawTimer;
extern	vmCvar_t		cg_drawFPS;
extern	vmCvar_t		cg_draw3dIcons;
extern	vmCvar_t		cg_drawIcons;
extern	vmCvar_t		cg_drawCrosshair;
extern	vmCvar_t		cg_drawCrosshairNames;
extern	vmCvar_t		cg_teamOverlayUserinfo;
extern	vmCvar_t		cg_crosshairX;
extern	vmCvar_t		cg_crosshairY;
extern	vmCvar_t		cg_crosshairScale;
extern	vmCvar_t		cg_drawStatus;
extern	vmCvar_t		cg_draw2D;
extern	vmCvar_t		cg_animSpeed;
extern	vmCvar_t		cg_debugAnim;
extern	vmCvar_t		cg_debugPosition;
extern	vmCvar_t		cg_debugEvents;
extern	vmCvar_t		cg_railTrailTime;
extern	vmCvar_t		cg_paintballMode;
extern	vmCvar_t		cg_disableLevelStartFade;
extern	vmCvar_t		cg_bigheadMode;
extern	vmCvar_t		cg_errorDecay;
extern	vmCvar_t		cg_nopredict;
extern	vmCvar_t		cg_noPlayerAnims;
extern	vmCvar_t		cg_showmiss;
extern	vmCvar_t		cg_footsteps;
extern	vmCvar_t		cg_addMarks;
extern	vmCvar_t		cg_brassTime;
extern	vmCvar_t		cg_gun_x;
extern	vmCvar_t		cg_gun_y;
extern	vmCvar_t		cg_gun_z;
extern	vmCvar_t		cg_drawGun;
extern	vmCvar_t		cg_viewsize;
extern	vmCvar_t		cg_tracerChance;
extern	vmCvar_t		cg_tracerWidth;
extern	vmCvar_t		cg_tracerLength;
extern	vmCvar_t		cg_simpleItems;
extern	vmCvar_t		cg_fov;
extern	vmCvar_t		cg_zoomFov;
extern	vmCvar_t		cg_thirdPersonOffset;
extern	vmCvar_t		cg_thirdPersonRange;
extern	vmCvar_t		cg_thirdPersonAngle;
extern	vmCvar_t		cg_thirdPerson;
extern	vmCvar_t		cg_lagometer;
extern	vmCvar_t		cg_drawSpeed;
extern	vmCvar_t		cg_synchronousClients;
extern	vmCvar_t		cg_teamChatTime;
extern	vmCvar_t		cg_teamChatHeight;
extern 	vmCvar_t 		cg_teamChatY;
extern 	vmCvar_t 		cg_chatY;
extern 	vmCvar_t 		cg_teamChatScaleX;
extern 	vmCvar_t 		cg_teamChatScaleY;
extern	vmCvar_t		cg_stats;
extern	vmCvar_t 		cg_buildScript;
extern	vmCvar_t		cg_paused;
extern	vmCvar_t		cg_blood;
extern	vmCvar_t		cg_predictItems;
extern	vmCvar_t		cg_drawFriend;
extern	vmCvar_t		cg_teamChatsOnly;
extern	vmCvar_t		cg_noVoiceText;
extern  vmCvar_t		cg_scorePlum;
extern vmCvar_t			cg_newFont;
extern vmCvar_t			cg_newConsole;
extern vmCvar_t			cg_chatTime;
extern vmCvar_t			cg_consoleTime;

extern vmCvar_t			cg_fontScale;
extern vmCvar_t			cg_fontShadow;

extern vmCvar_t			cg_consoleSizeX;
extern vmCvar_t			cg_consoleSizeY;
extern vmCvar_t			cg_chatSizeX;
extern vmCvar_t			cg_chatSizeY;
extern vmCvar_t			cg_teamChatSizeX;
extern vmCvar_t			cg_teamChatSizeY;

extern vmCvar_t			cg_consoleLines;
extern vmCvar_t			cg_commonConsoleLines;
extern vmCvar_t			cg_chatLines;
extern vmCvar_t			cg_teamChatLines;

extern vmCvar_t			cg_commonConsole;
extern	vmCvar_t		pmove_fixed;
extern	vmCvar_t		pmove_msec;
extern	vmCvar_t		pmove_float;
extern	vmCvar_t		cg_timescaleFadeEnd;
extern	vmCvar_t		cg_timescaleFadeSpeed;
extern	vmCvar_t		cg_timescale;
extern	vmCvar_t		cg_cameraMode;
extern	vmCvar_t		cg_noProjectileTrail;

extern	vmCvar_t		cg_leiEnhancement;			// LEILEI'S LINE!
extern	vmCvar_t		cg_leiGoreNoise;			// LEILEI'S LINE!
extern	vmCvar_t		cg_leiBrassNoise;			// LEILEI'S LINE!
extern	vmCvar_t		cg_cameramode;
extern	vmCvar_t		cg_cameraEyes;
extern	vmCvar_t		cg_cameraEyes_Fwd;
extern	vmCvar_t		cg_cameraEyes_Up;
extern	vmCvar_t		cg_trueLightning;
extern	vmCvar_t		cg_music;
//Sago: Moved outside
extern	vmCvar_t		cg_obeliskRespawnDelay;
extern	vmCvar_t		cg_enableDust;
extern	vmCvar_t		cg_enableBreath;

//unlagged - client options
extern	vmCvar_t		cg_delag;
extern	vmCvar_t		cg_cmdTimeNudge;
extern	vmCvar_t		sv_fps;
extern	vmCvar_t		cg_projectileNudge;
extern	vmCvar_t		cg_optimizePrediction;
extern	vmCvar_t		cl_timeNudge;
//unlagged - client options

//extra CVARS elimination
extern	vmCvar_t		cg_alwaysWeaponBar;
extern  vmCvar_t                cg_voteflags;

extern  vmCvar_t                cg_autovertex;

extern	vmCvar_t		cg_atmosphericLevel;

extern	vmCvar_t		cg_crosshairPulse;

extern	vmCvar_t                cg_crosshairColorRed;
extern	vmCvar_t                cg_crosshairColorGreen;
extern	vmCvar_t                cg_crosshairColorBlue;

extern vmCvar_t			cg_weaponBarStyle;

extern vmCvar_t                 cg_weaponOrder;
extern vmCvar_t			cg_chatBeep;
extern vmCvar_t			cg_teamChatBeep;

//unlagged - cg_unlagged.c
void CG_PredictWeaponEffects( centity_t *cent );
//void CG_AddBoundingBox( centity_t *cent );
qboolean CG_Cvar_ClampInt( const char *name, vmCvar_t *vmCvar, int min, int max );
//unlagged - cg_unlagged.c

//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

void QDECL CG_Printf( const char *msg, ... );
void QDECL CG_Error( const char *msg, ... ) __attribute__((noreturn));

void CG_StartMusic( void );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type);
void CG_RankRunFrame( void );
void CG_SetScoreSelection(void *menu);
void CG_BuildSpectatorString( void );
qboolean CG_IsTeamGame();
void CG_RegisterOverlay( void );

//unlagged, sagos modfication
void SnapVectorTowards( vec3_t v, vec3_t to );

void CG_FairCvars( void );

//
// cg_view.c
//
void CG_CloadMap_f (void);
void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);
void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );
void CG_AddBufferedSound( sfxHandle_t sfx);

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );


//
// cg_drawtools.c
//
void CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_FillRect2( float x, float y, float width, float height, const float *color );
void CG_DrawRoundedRect(float x, float y, float width, float height, float radius, const float *color);
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars, float offset );
void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawGiantString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color );

int CG_DrawStrlen( const char *str );

float	*CG_FadeColor( int startMsec, int totalMsec );
float *CG_TeamColor( int team );
void CG_TileClear( void );
void CG_ColorForHealth( vec4_t hcolor );
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );

void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);
qboolean CG_InsideBox( vec3_t mins, vec3_t maxs, vec3_t pos );


//
// cg_draw.c, cg_newDraw.c
//
extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;
extern	int drawTeamOverlayModificationCount;
extern  char systemChat[256];
extern  char teamChat1[256];
extern  char teamChat2[256];

void CG_Fade( float duration, vec4_t startColor, vec4_t endColor );
void CG_DrawFade( void );
void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_DrawHead( float x, float y, float w, float h, int clientNum );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
int CG_Text_Width(const char *text, float scale, int limit);
int CG_Text_Height(const char *text, float scale, int limit);
void CG_SelectPrevPlayer( void );
void CG_SelectNextPlayer( void );
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
void CG_ShowResponseHead( void );
void CG_SetPrintString(int type, const char *p);
void CG_InitTeamChat( void );
void CG_GetTeamColor(vec4_t *color);
const char *CG_GetGameStatusText( void );
const char *CG_GetKillerText( void );
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
void CG_Draw3DModelToolgun(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_CheckOrderPending( void );
const char *CG_GameTypeString( void );
qboolean CG_YourTeamHasFlag( void );
qboolean CG_OtherTeamHasFlag( void );
qhandle_t CG_StatusHandle(int task);
void CG_AddToGenericConsole( const char *str, console_t *console );
void CG_AddNotify(const char *text, int type);


//
// cg_player.c
//
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team, qboolean isMissile );
void CG_NewClientInfo( int clientNum );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );

//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask );
void CG_PredictPlayerState( void );
void CG_ReloadPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health );
void CG_PainVehicleEvent( centity_t *cent, int health );


//
// cg_ents.c
//
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( void );
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out );

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );



//
// cg_weapons.c
//
void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );

void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent );
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType );
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum );
void CG_ShotgunFire( entityState_t *es );
void CG_Bullet( vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum );

void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end );
void CG_PhysgunTrail( clientInfo_t *ci, vec3_t start, vec3_t end );
void CG_GravitygunTrail( clientInfo_t *ci, vec3_t start, vec3_t end );
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
void CG_AddViewWeapon (playerState_t *ps);
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, clientInfo_t *ci );
void CG_DrawWeaponSelect( void );
void CG_DrawWeaponBarNew2(int count);

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir,
					float orientation,
				    float r, float g, float b, float a,
					qboolean alphaFade,
					float radius, qboolean temporary );
void    CG_LeiSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void    CG_LeiSparks2 (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void    CG_LeiPuff (vec3_t org, vec3_t vel, int duration, float x, float y, float speed, float size);


//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
localEntity_t *CG_SmokePuff( const vec3_t p,
				   const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing );
void CG_SpawnEffect( vec3_t org );

void CG_KamikazeEffect( vec3_t org );
void CG_ObeliskExplode( vec3_t org, int entityNum );
void CG_ObeliskPain( vec3_t org );
void CG_InvulnerabilityImpact( vec3_t org, vec3_t angles );
void CG_InvulnerabilityJuiced( vec3_t org );
void CG_LightningBoltBeam( vec3_t start, vec3_t end );
void CG_ScorePlum( int client, vec3_t org, int score, int dmgf );

void CG_GibPlayer( vec3_t playerOrigin );
void CG_BigExplode( vec3_t playerOrigin );

void CG_Bleed( vec3_t origin, int entityNum );

localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, qhandle_t shader, int msec,
								qboolean isSprite );

void CG_SpurtBlood( vec3_t origin, vec3_t velocity, int hard );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );
//unlagged - early transitioning
void CG_TransitionEntity( centity_t *cent );
//unlagged - early transitioning

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation( void );
void CG_DrawInformationRus( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawScoreboard( void );
void CG_DrawTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );
void CG_SetConfigValues( void );
void CG_ShaderStateChanged(void);

//
// cg_playerstate.c
//
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );
extern float teamcolormodels[TEAM_NUM_TEAMS][3];


//
// cg_atmospheric.c
//
void CG_AddAtmosphericEffects( void );
void CG_Atmospheric_SetParticles( int type, int numParticles, qboolean diableSplashes );
//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print( const char *fmt );

// abort the game
void		trap_Error( const char *fmt )  __attribute__((noreturn));

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds( void );

// console variable interaction
void		trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void		trap_Cvar_Update( vmCvar_t *vmCvar );
void		trap_Cvar_Set( const char *var_name, const char *value );
void		trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc( void );
void		trap_Argv( int n, char *buffer, int bufferLength );
void		trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );
int			trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
void 		trap_System( const char *command );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand( const char *cmdName );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points,
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void		trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
sfxHandle_t	trap_S_RegisterSound_SourceTech( const char *sample, qboolean compressed );		// returns buzz if not found
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop );	// empty name stops music
void	trap_S_StopBackgroundTrack( void );


void		trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterModel_SourceTech( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void		trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void		trap_R_AddLinearLightToScene( const vec3_t start, const vec3_t end, float intensity, float r, float g, float b );
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic( float x, float y, float w, float h,
			float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
					   float frac, const char *tagName );
void		trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale );

// aids for VM testing
void		testPrintInt( char *string, int i );
void		testPrintFloat( char *string, float f );

int			trap_MemoryRemaining( void );
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown( int keynum );
int			trap_Key_GetCatcher( void );
void		trap_Key_SetCatcher( int catcher );
int			trap_Key_GetKey( const char *binding );


typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t; // bk001201 - warning: useless keyword or type name in empty declaration


int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void trap_CIN_DrawCinematic (int handle);
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

void trap_SnapVector( float *v );

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

void	CG_ClearParticles (void);
void	CG_AddParticles (void);
void	CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void	CG_ParticleSmoke (qhandle_t pshader, centity_t *cent);
void	CG_AddParticleShrapnel (localEntity_t *le);
void	CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent);
void	CG_ParticleBulletDebris (vec3_t	org, vec3_t vel, int duration);
void	CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void	CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir);
void	CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void	CG_ParticleExplosion (char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
void	CG_LaunchFragment( vec3_t origin, vec3_t velocity, leTrailType_t trailType, qhandle_t hModel );
extern qboolean		initparticles;
int CG_NewParticleArea ( int num );
extern int wideAdjustX;


// LEILEI ENHANCEMENT
