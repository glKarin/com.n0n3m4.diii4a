/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/
#include "ui_local.h"
#include "ui_startserver_q3.h"

#define ID_SERVER_GAMETYPE 300
#define ID_SERVER_HOSTNAME 301
#define ID_SERVER_RESPAWN 302
#define ID_SERVER_WARMUP 303
#define ID_SERVER_FRIENDLY 304
#define ID_SERVER_AUTOJOIN 305
#define ID_SERVER_TEAMBALANCE 306
#define ID_SERVER_PURE 307
#define ID_SERVER_DEDICATED 308
#define ID_SERVER_INACTIVITY 309
#define ID_SERVER_SAVE 310
#define ID_SERVER_LOAD 311
#define ID_SERVER_PMOVEFIXED 312
#define ID_SERVER_PMOVEMSEC 313
#define ID_SERVER_SMOOTHCLIENTS 314
#define ID_SERVER_MAXRATE 315
#define ID_SERVER_ALLOWDOWNLOAD 316
#define ID_SERVER_PASSWORD 317
#define ID_SERVER_ALLOWPASS 318
#define ID_SERVER_ALLOWMAXRATE 319
#define ID_SERVER_ALLOWWARMUP 320
#define ID_SERVER_SYNCCLIENTS 321
#define ID_SERVER_MINPING 322
#define ID_SERVER_MAXPING 323
#define ID_SERVER_ALLOWMINPING 324
#define ID_SERVER_ALLOWMAXPING 325
#define ID_SERVER_CONFIGBUG 326
#define ID_SERVER_NETPORT 327
#define ID_SERVER_SVFPS 328
#define ID_SERVER_ALLOWPRIVATECLIENT 329
#define ID_SERVER_PRIVATECLIENT 330
#define ID_SERVER_PRIVATECLIENTPWD 331
#define ID_SERVER_STRICTAUTH 332
#define ID_SERVER_LANFORCERATE 333
#define ID_SERVERTAB_CONNECT 334
#define ID_SERVERTAB_ADMIN 335
#define ID_SERVERTAB_GENERAL 336
#define ID_SERVERTAB_GAMEMODE 337
#define ID_SERVERTAB_PHYSICS 338
#define ID_SERVERTAB_RULES 339
#define ID_SERVERTAB_RUNE1 340
#define ID_SERVERTAB_RUNE2 341
#define ID_SERVERTAB_REDTEAM 342
#define ID_SERVERTAB_BLUETEAM 343
#define ID_SERVERTAB_REDTEAMWEAPONS 344
#define ID_SERVERTAB_BLUETEAMWEAPONS 345
#define ID_SERVERTAB_TEAMOTHER 346
#define ID_SERVERTAB_OTHER 347
#define ID_SERVERTAB_ENVIROMENT 348
#define ID_SERVERTAB_ITEM 349
#define ID_SERVERTAB_TIME 350
#define ID_SERVER_MAXENTITIES 351
#define ID_SERVER_DRAWDISTANCE 352
#define ID_SERVER_SINGLESKILL 353
#define ID_SERVER_KILL 354
#define ID_SERVER_DAMAGEMODIFIER           355
#define ID_SERVER_ELIMINATION               356
#define ID_SERVER_OBELISKHEALTH             357
#define ID_SERVER_OBELISKREGENPERIOD       358
#define ID_SERVER_OBELISKREGENAMOUNT       359
#define ID_SERVER_OBELISKRESPAWNDELAY      360
#define ID_SERVER_CUBETIMEOUT               361
#define ID_SERVER_FLAGRESPAWN               362
#define ID_SERVER_WEAPONTEAMRESPAWN         363
#define ID_SERVER_ELIMINATION_CTF_ONEWAY    364
#define ID_SERVER_ELIMINATION_SELFDAMAGE     365
#define ID_SERVER_ELIMINATION_ROUNDTIME      366
#define ID_SERVER_ELIMINATION_WARMUP         367
#define ID_SERVER_ELIMINATION_ACTIVEWARMUP   368
#define ID_SERVER_LMS_LIVES                  369
#define ID_SERVER_LMS_MODE                   370
#define ID_SERVER_ACCELERATE                 371
#define ID_SERVER_SPECTATORSPEED             372
#define ID_SERVER_SPEED                      373
#define ID_SERVER_GRAVITY                    374
#define ID_SERVER_GRAVITYMODIFIER            375
#define ID_SERVER_KNOCKBACK                  376
#define ID_SERVER_NOPLAYERCLIP               377
#define ID_SERVER_JUMPHEIGHT                 378
#define ID_SERVER_REGENARMOR                 379
#define ID_SERVER_AMMOLIMIT                  380
#define ID_SERVER_QUADFACTOR                 381
#define ID_SERVER_RESPAWNTIME                382
#define ID_SERVER_FORCERESPAWN               383
#define ID_SERVER_VAMPIRE                     384
#define ID_SERVER_VAMPIRE_MAX_HEALTH         385
#define ID_SERVER_REGEN                       386
#define ID_SERVER_MAXWEAPONPICKUP            387
#define ID_SERVER_DROPPEDITEMTIME            388
#define ID_SERVER_AUTOFLAGRETURN              389
#define ID_SERVER_ARMORPROTECT               390
#define ID_SERVER_RESPAWNWAIT                391
#define ID_SERVER_SPEEDFACTOR                 392
#define ID_SERVER_SCOUTSPEEDFACTOR           393
#define ID_SERVER_SCOUTFIRESPEED             394
#define ID_SERVER_SCOUTDAMAGEFACTOR          395
#define ID_SERVER_SCOUTGRAVITYMODIFIER       396
#define ID_SERVER_SCOUT_INFAMMO               397
#define ID_SERVER_SCOUTHEALTHMODIFIER        398
#define ID_SERVER_DOUBLERFIRESPEED          399
#define ID_SERVER_DOUBLERDAMAGEFACTOR       400
#define ID_SERVER_DOUBLERSPEEDFACTOR        401
#define ID_SERVER_DOUBLERGRAVITYMODIFIER    402
#define ID_SERVER_DOUBLER_INFAMMO            403
#define ID_SERVER_DOUBLERHEALTHMODIFIER     404
#define ID_SERVER_GUARDFIRESPEED            405
#define ID_SERVER_GUARDDAMAGEFACTOR         406
#define ID_SERVER_GUARDSPEEDFACTOR           407
#define ID_SERVER_GUARDGRAVITYMODIFIER      408
#define ID_SERVER_GUARD_INFAMMO              409
#define ID_SERVER_GUARDHEALTHMODIFIER       410
#define ID_SERVER_AMMOREGENFIRESPEED        411
#define ID_SERVER_AMMOREGEN_INFAMMO          412
#define ID_SERVER_AMMOREGENDAMAGEFACTOR     413
#define ID_SERVER_AMMOREGENSPEEDFACTOR      414
#define ID_SERVER_AMMOREGENGRAVITYMODIFIER  415
#define ID_SERVER_AMMOREGENHEALTHMODIFIER    416
#define ID_SERVER_TEAMRED_SPEED              417
#define ID_SERVER_TEAMRED_GRAVITYMODIFIER    418
#define ID_SERVER_TEAMRED_FIRESPEED          419
#define ID_SERVER_TEAMRED_DAMAGE              420
#define ID_SERVER_TEAMRED_INFAMMO            421
#define ID_SERVER_TEAMRED_RESPAWNWAIT       422
#define ID_SERVER_TEAMRED_PICKUPITEMS        423
#define ID_SERVER_ELIMINATIONREDRESPAWN      424
#define ID_SERVER_ELIMINATIONRED_STARTHEALTH 425
#define ID_SERVER_ELIMINATIONRED_STARTARMOR   426
#define ID_SERVER_TEAMBLUE_SPEED              427
#define ID_SERVER_TEAMBLUE_GRAVITYMODIFIER    428
#define ID_SERVER_TEAMBLUE_FIRESPEED          429
#define ID_SERVER_TEAMBLUE_DAMAGE              430
#define ID_SERVER_TEAMBLUE_INFAMMO            431
#define ID_SERVER_TEAMBLUE_RESPAWNWAIT       432
#define ID_SERVER_TEAMBLUE_PICKUPITEMS        433
#define ID_SERVER_ELIMINATIONRESPAWN          434
#define ID_SERVER_ELIMINATION_STARTHEALTH     435
#define ID_SERVER_ELIMINATION_STARTARMOR      436
#define ID_SERVER_ELIMINATIONRED_GRAPPLE      437
#define ID_SERVER_ELIMINATIONRED_GAUNTLET     438
#define ID_SERVER_ELIMINATIONRED_MACHINEGUN   439
#define ID_SERVER_ELIMINATIONRED_SHOTGUN      440
#define ID_SERVER_ELIMINATIONRED_GRENADE      441
#define ID_SERVER_ELIMINATIONRED_ROCKET       442
#define ID_SERVER_ELIMINATIONRED_RAILGUN      443
#define ID_SERVER_ELIMINATIONRED_LIGHTNING    444
#define ID_SERVER_ELIMINATIONRED_PLASMAGUN    445
#define ID_SERVER_ELIMINATIONRED_BFG          446
#define ID_SERVER_ELIMINATIONRED_CHAIN        447
#define ID_SERVER_ELIMINATIONRED_MINE         448
#define ID_SERVER_ELIMINATIONRED_NAIL         449
#define ID_SERVER_ELIMINATIONRED_FLAME        450
#define ID_SERVER_ELIMINATIONRED_ANTIMATTER   451
#define ID_SERVER_ELIMINATION_GRAPPLE         452
#define ID_SERVER_ELIMINATION_GAUNTLET       453
#define ID_SERVER_ELIMINATION_MACHINEGUN     454
#define ID_SERVER_ELIMINATION_SHOTGUN        455
#define ID_SERVER_ELIMINATION_GRENADE        456
#define ID_SERVER_ELIMINATION_ROCKET         457
#define ID_SERVER_ELIMINATION_RAILGUN        458
#define ID_SERVER_ELIMINATION_LIGHTNING      459
#define ID_SERVER_ELIMINATION_PLASMAGUN      460
#define ID_SERVER_ELIMINATION_BFG            461
#define ID_SERVER_ELIMINATION_CHAIN          462
#define ID_SERVER_ELIMINATION_MINE           463
#define ID_SERVER_ELIMINATION_NAIL           464
#define ID_SERVER_ELIMINATION_FLAME          465
#define ID_SERVER_ELIMINATION_ANTIMATTER     466
#define ID_SERVER_ELIMINATIONRED_QUAD        467
#define ID_SERVER_ELIMINATIONRED_HASTE       468
#define ID_SERVER_ELIMINATIONRED_BSUIT       469
#define ID_SERVER_ELIMINATIONRED_INVIS       470
#define ID_SERVER_ELIMINATIONRED_REGEN       471
#define ID_SERVER_ELIMINATIONRED_FLIGHT      472
#define ID_SERVER_ELIMINATIONRED_HOLDABLE     473
#define ID_SERVER_ELIMINATION_QUAD           474
#define ID_SERVER_ELIMINATION_HASTE          475
#define ID_SERVER_ELIMINATION_BSUIT          476
#define ID_SERVER_ELIMINATION_INVIS          477
#define ID_SERVER_ELIMINATION_REGEN          478
#define ID_SERVER_ELIMINATION_FLIGHT         479
#define ID_SERVER_ELIMINATION_ITEMS          480
#define ID_SERVER_ELIMINATION_HOLDABLE       481
#define ID_SERVER_MINIGAME                   482
#define ID_SERVER_OVERLAY                    483
#define ID_SERVER_RANDOMITEMS                484
#define ID_SERVER_ALLOWVOTE                  485
#define ID_SERVER_SPAWNPROTECT               486
#define ID_SERVER_ELIMINATION_LOCKSPECTATOR  487
#define ID_SERVER_AWARDPUSHING               488
#define ID_SERVER_RANDOMTELEPORT             489
#define ID_SERVER_FALLDAMAGESMALL            490
#define ID_SERVER_FALLDAMAGEBIG              491
#define ID_SERVER_WATERDAMAGE                492
#define ID_SERVER_LAVADAMAGE                 493
#define ID_SERVER_SLIMEDAMAGE                494
#define ID_SERVER_DROWNDAMAGE                495
#define ID_SERVER_INVULINF                   496
#define ID_SERVER_INVULMOVE                  497
#define ID_SERVER_INVULTIME                  498
#define ID_SERVER_KAMIKAZEINF                499
#define ID_SERVER_PORTALINF                  500
#define ID_SERVER_PORTALTIMEOUT              501
#define ID_SERVER_PORTALHEALTH               502
#define ID_SERVER_TELEPORTERINF              503
#define ID_SERVER_MEDKITINF                  504
#define ID_SERVER_MEDKITLIMIT                505
#define ID_SERVER_MEDKITMODIFIER             506
#define ID_SERVER_FASTHEALTHREGEN            507
#define ID_SERVER_SLOWHEALTHREGEN            508
#define ID_SERVER_HASTEFIRESPEED             509
#define ID_SERVER_QUADTIME                   510
#define ID_SERVER_BSUITTIME                  511
#define ID_SERVER_HASTETIME                  512
#define ID_SERVER_INVISTIME                  513
#define ID_SERVER_REGENTIME                  514
#define ID_SERVER_FLIGHTTIME                 515
#define ID_SERVER_ARMORRESPAWN               516
#define ID_SERVER_HEALTHRESPAWN              517
#define ID_SERVER_AMMORESPAWN                518
#define ID_SERVER_HOLDABLERESPAWN            519
#define ID_SERVER_MEGAHEALTHRESPAWN          520
#define ID_SERVER_POWERUPRESPAWN             521
#define ID_SERVER_WEAPONRESPAWN              522
#define ID_SERVER_SELECTEDMOD                 523
#define ID_SERVER_SLICKMOVE                  524

#define SERVER_SAVE0 "menu/art/save_0"
#define SERVER_SAVE1 "menu/art/save_1"
#define SERVER_LOAD0 "menu/art/load_0"
#define SERVER_LOAD1 "menu/art/load_1"


#define CONTROL_POSX (GAMETYPECOLUMN_X + 6*SMALLCHAR_WIDTH)

#define SERVERCOLUMN_X GAMETYPECOLUMN_X



enum {
	SRVCTRL_SPIN,
	SRVCTRL_RADIO,
	SRVCTRL_TEXTFIELD,
	SRVCTRL_NUMFIELD,
	SRVCTRL_BLANK
};


/*
	These flags define how controls are drawn

	ITEM_ALWAYSON			never modified
	ITEM_GRAYIF_PREVON		grayed if prev spin control is on (dyn)
	ITEM_GRAYIF_PREVOFF		grayed if prev spin control is off (dyn)
	ITEM_TEAM_GAME			control displayed when a team game
	ITEM_NOTEAM_GAME		control displayed when not a team game
	ITEM_HALFGAP			places a gap after this item

	On use of ITEM_GRAYIF_* flags:
	The control with this flag will be enabled/grayed depending on the
	most recent radio control that is enabled. You can use this flag to
	enable/disable groups of controls that include a radio control - without
	that disabled radio control influencing the group.

	Example:

	radio1
	spin1	GRAYIF_PREVON
	radio2	GRAYIF_PREVON
	spin2	GRAYIF_PREVON

	if radio1 is on then all following controls are grayed, radio2 is ignored
	if radio1 is off then spin1 and radio2 are enabled, and spin2 depends on radio2
*/
#define ITEM_ALWAYSON			0x0000
#define ITEM_GRAYIF_PREVON	 	0x0001
#define ITEM_GRAYIF_PREVOFF 	0x0002
#define ITEM_TEAM_GAME			0x0004
#define ITEM_NOTEAM_GAME		0x0008
#define ITEM_HALFGAP			0x0010



// list of created controls on page
typedef struct controlinit_s {
	int type;	// SRVCTRL_*
	int offset;	// index into array storing type
	int id;		// control id ID_SERVER_*
	int flags;	// ITEM_* flags

	char* title;	// text describing control

	// used by RADIO, NUMFIELD, or SPIN
	int* number;

	// used by NUMFIELD
	// min, max ignored if identical
	int min;	// min value of number
	int max;	// max value of number

	// used by TEXTFIELD
	char* array;

	// used by TEXTFIELD and NUMFIELD
	int displaysize;
	int arraysize;

	// used by SPIN
	const char** itemnames;
} controlinit_t;



#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))


// single tabbed page of controls
typedef struct initlist_s {
	int id;
	char* title;
	menutext_s* menutext;
	controlinit_t* list;
	int count;
} initlist_t;





typedef struct radiobuttoncontrol_s {
	menuradiobutton_s control;

	controlinit_t* init;
} radiobuttoncontrol_t;


typedef struct textfieldcontrol_s {
	menufield_s control;

	controlinit_t* init;
} textfieldcontrol_t;


typedef struct spincontrol_s {
	menulist_s control;

	controlinit_t* init;
} spincontrol_t;



#define MAX_SERVER_RADIO_CONTROL 24
#define MAX_SERVER_MFIELD_CONTROL 18
#define MAX_SERVER_SPIN_CONTROL 18



typedef struct servercontrols_s {
	menuframework_s menu;
	commoncontrols_t common;

	menulist_s gameType;
	menubitmap_s gameTypeIcon;
	menufield_s hostname;

	radiobuttoncontrol_t radio[MAX_SERVER_RADIO_CONTROL];
	textfieldcontrol_t field[MAX_SERVER_MFIELD_CONTROL];
	spincontrol_t spin[MAX_SERVER_SPIN_CONTROL];

	menutext_s connect;
	menutext_s admin;
	menutext_s general;
	menutext_s gamemode;
	menutext_s physics;
	menutext_s rules;
	menutext_s rune1;
	menutext_s rune2;
	menutext_s redteam;
	menutext_s blueteam;
	menutext_s redteamweapons;
	menutext_s blueteamweapons;
	menutext_s teamother;
	menutext_s other;
	menutext_s enviroment;
	menutext_s item;
	menutext_s time;

	// currently selected tabbeed page for server
	int activePage;

	// source of data for manipulating list
	initlist_t* pageList;
	int pageListSize;

	// current number of controls on displayed page
	int num_radio;
	int num_field;
	int num_spin;

	menubitmap_s saveScript;
	menubitmap_s loadScript;

	int statusbar_height;
	int savetime;
	char statusbar_message[MAX_STATUSBAR_TEXT];
} servercontrols_t;




static servercontrols_t s_servercontrols;



//
// static data used by controls
//


static const char *dedicated_list[] = {
	"No",	// SRVDED_OFF
	"LAN",	// SRVDED_LAN
	"Internet",	// SRVDED_INTERNET
	0
};

static const char *daytime_list[] = {
	"Dynamic",	// SRVDED_OFF
	"Morning",	// SRVDED_LAN
	"Afternoon",	// SRVDED_INTERNET
	"Evening",	// SRVDED_INTERNET
	"Night",	// SRVDED_INTERNET
	0
};

static const char *lmsMode_list[] = {
	"Round+OT",
	"Round-OT",
	"Kill+OT",
	"Kill-OT",
	NULL
};

static const char *holdable_list[] = {
	"Off",
	"Teleporter",
	"Medkit",
	"Kamikaze",
	"Invulnerability",
	"Portal",
	0
};

static const char *holdableinf_list[] = {
	"Off",
	"Unlimited",
	"Teleporter",
	"Medkit",
	"Kamikaze",
	"Invulnerability",
	"Portal",
	0
};

static const char *overlay_list[] = {
	"Off",
	"All",
	"Red",
	"Blue",
	0
};

static const char *invulmove_list[] = {
	"Off",
	"Fly",
	"Noclip",
	0
};

static const char *slickmove_list[] = {
	"Default",
	"Slick",
	0
};




// connect controls
static controlinit_t srv_connect[] = {
	/*{ SRVCTRL_RADIO, 0, ID_SERVER_PURE, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Pure server:", &s_scriptdata.server.pure, 0, 0, NULL, 0, 0, NULL },*/

	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWMINPING, ITEM_ALWAYSON,
		"Use min ping:", &s_scriptdata.server.allowMinPing, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MINPING, ITEM_GRAYIF_PREVOFF,
		"Min ping (ms):", &s_scriptdata.server.minPing, 0, 999, NULL, 4, 4, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWMAXPING, ITEM_ALWAYSON,
		"Use max ping:", &s_scriptdata.server.allowMaxPing, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MAXPING, ITEM_GRAYIF_PREVOFF,
		"Max ping (ms):", &s_scriptdata.server.maxPing, 0, 999, NULL, 4, 4, NULL },
		
	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWWARMUP, ITEM_ALWAYSON,
		"Allow warmup:", &s_scriptdata.server.allowWarmup, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_WARMUP, ITEM_GRAYIF_PREVOFF|ITEM_HALFGAP,
		"Warmup time:", &s_scriptdata.server.warmupTime, 0, 0, NULL, 4, 4, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_SMOOTHCLIENTS, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Smooth clients:", &s_scriptdata.server.smoothclients, 0, 0, NULL, 0, 0, NULL },
	
	{ SRVCTRL_RADIO, 0, ID_SERVER_FRIENDLY, ITEM_TEAM_GAME,
		"Friendly fire:", &s_scriptdata.server.friendlyFire, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_AUTOJOIN, ITEM_TEAM_GAME,
		"Team auto join:", &s_scriptdata.server.autoJoin, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_TEAMBALANCE, ITEM_TEAM_GAME,
		"Team balance:", &s_scriptdata.server.teamBalance, 0, 0, NULL, 0, 0, NULL }
};

// admin controls
static controlinit_t srv_admin[] = {
	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWMAXRATE, ITEM_ALWAYSON,
		"Server maxrate:", &s_scriptdata.server.allowmaxrate, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MAXRATE, ITEM_GRAYIF_PREVOFF|ITEM_HALFGAP,
		"bytes/s:", &s_scriptdata.server.maxrate, 0, 0, NULL, 6, 6, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWVOTE, ITEM_ALWAYSON,
		"Allow voting:", &s_scriptdata.server.allowvote, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWDOWNLOAD, ITEM_ALWAYSON,
		"Allow download:", &s_scriptdata.server.allowdownload, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_SYNCCLIENTS, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Sync clients:", &s_scriptdata.server.syncClients, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_NETPORT, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Net port:", &s_scriptdata.server.netport, 1024, 65535, NULL, 6, 6, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SVFPS, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Server FPS:", &s_scriptdata.server.sv_fps, 20, 160, NULL, 4, 4, NULL },
		
	{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWPASS, ITEM_ALWAYSON,
		"Private server:", &s_scriptdata.server.allowpass, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_PASSWORD, ITEM_GRAYIF_PREVOFF|ITEM_HALFGAP,
		"Private password:", NULL, 0, 0, s_scriptdata.server.password, 10, MAX_PASSWORD_LENGTH, NULL },

	/*{ SRVCTRL_RADIO, 0, ID_SERVER_ALLOWPRIVATECLIENT, ITEM_ALWAYSON,
		"Reserve clients:", &s_scriptdata.server.allowPrivateClients, 0, 0, NULL, 0, 0, NULL },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_PRIVATECLIENT, ITEM_GRAYIF_PREVOFF,
		"Private clients:", &s_scriptdata.server.privateClients, 0, 32, NULL, 4, 4, NULL },

	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_PRIVATECLIENTPWD, ITEM_GRAYIF_PREVOFF,
		"Client password:", NULL, 0, 0, s_scriptdata.server.privatePassword, 10, MAX_PASSWORD_LENGTH, NULL },*/

	{ SRVCTRL_SPIN, 0, ID_SERVER_DEDICATED, ITEM_ALWAYSON,
		"Dedicated server:", &s_scriptdata.server.dedicatedServer, 0, 0, NULL, 0, 0, dedicated_list },

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_INACTIVITY, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Inactivity timeout:", &s_scriptdata.server.inactivityTime, 0, 999, NULL, 3, 3, NULL },

	{ SRVCTRL_RADIO, 0, ID_SERVER_LANFORCERATE, ITEM_ALWAYSON,
		"LAN force rate:", &s_scriptdata.server.lanForceRate, 0, 0, NULL, 0, 0, NULL },
};

// gameplay controls
static controlinit_t srv_general[] = {
		

	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SELECTEDMOD, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Mod folder name:", NULL, 0, 0, s_scriptdata.server.selectedmod, 20, 20, NULL },
		
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MAXENTITIES, ITEM_ALWAYSON,
		"Max Entities:", &s_scriptdata.server.maxEntities, 512, MAX_GENTITIES, NULL, 9, 9, NULL },
		
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_DRAWDISTANCE, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Entities load distance:", &s_scriptdata.server.viewdistance, 0, 90, NULL, 3, 3, NULL },
		
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SINGLESKILL, ITEM_ALWAYSON,
		"Singleplayer skill:", &s_scriptdata.server.singleskill, 1, 14, NULL, 9, 9, NULL },
		
	{ SRVCTRL_RADIO, 0, ID_SERVER_KILL, ITEM_ALWAYSON,
		"Allow kill command:", &s_scriptdata.server.kill, -9999999, 9999999, NULL, 0, 0, NULL },
		
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_DAMAGEMODIFIER, ITEM_ALWAYSON|ITEM_HALFGAP,
		"Damage modifier(float):", NULL, 0, 0, s_scriptdata.server.damageModifier, 9, 9, NULL },
		
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATION, ITEM_ALWAYSON,
		"Elimination mode:", &s_scriptdata.server.elimination, -9999999, 9999999, NULL, 0, 0, NULL }
};

// gamemode controls
static controlinit_t srv_gamemode[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_OBELISKHEALTH, ITEM_ALWAYSON,
		"Overload HP:", &s_scriptdata.server.obeliskHealth, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_OBELISKREGENPERIOD, ITEM_ALWAYSON,
		"Overload regen time:", &s_scriptdata.server.obeliskRegenPeriod, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_OBELISKREGENAMOUNT, ITEM_ALWAYSON,
		"Overload regen amount:", &s_scriptdata.server.obeliskRegenAmount, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_OBELISKRESPAWNDELAY, ITEM_ALWAYSON,
		"Overload respawn delay:", &s_scriptdata.server.obeliskRespawnDelay, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_CUBETIMEOUT, ITEM_ALWAYSON,
		"Skull timeout:", &s_scriptdata.server.cubeTimeout, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FLAGRESPAWN, ITEM_ALWAYSON,
		"Flag respawn:", &s_scriptdata.server.flagrespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_WEAPONTEAMRESPAWN, ITEM_ALWAYSON,
		"Weapon team respawn:", &s_scriptdata.server.weaponTeamRespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATION_CTF_ONEWAY, ITEM_ALWAYSON,
		"Elimination CTF oneway:", &s_scriptdata.server.elimination_ctf_oneway, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATION_SELFDAMAGE, ITEM_ALWAYSON,
		"Elimination selfdamage:", &s_scriptdata.server.elimination_selfdamage, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_ROUNDTIME, ITEM_ALWAYSON,
		"Elimination roundtime:", &s_scriptdata.server.elimination_roundtime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_WARMUP, ITEM_ALWAYSON,
		"Elimination warmup:", &s_scriptdata.server.elimination_warmup, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_ACTIVEWARMUP, ITEM_ALWAYSON,
		"Elimination active warmup:", &s_scriptdata.server.elimination_activewarmup, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_LMS_LIVES, ITEM_ALWAYSON,
		"Elimination LMS lives:", &s_scriptdata.server.lms_lives, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_SPIN, 0, ID_SERVER_LMS_MODE, ITEM_ALWAYSON,
		"Elimination LMS mode:", &s_scriptdata.server.lms_mode, -999999999, 999999999, NULL, 9, 9, lmsMode_list },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATION_ITEMS, ITEM_ALWAYSON,
		"Elimination items:", &s_scriptdata.server.elimination_items, -999999999, 999999999, NULL, 9, 9, NULL },

};



// physics
static controlinit_t srv_physics[] = {
	{ SRVCTRL_RADIO, 0, ID_SERVER_PMOVEFIXED, ITEM_ALWAYSON,
		"Pmove fixed:", &s_scriptdata.server.pmove_fixed, 0, 0, NULL, 0, 0, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_PMOVEMSEC, ITEM_GRAYIF_PREVOFF|ITEM_HALFGAP,
		"msec:", &s_scriptdata.server.pmove_msec, 1, 128, NULL, 3, 3, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ACCELERATE, ITEM_ALWAYSON,
		"Accelerate:", &s_scriptdata.server.accelerate, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SPECTATORSPEED, ITEM_ALWAYSON,
		"Spectator speed:", &s_scriptdata.server.spectatorspeed, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SPEED, ITEM_ALWAYSON,
		"Player speed:", &s_scriptdata.server.speed, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_GRAVITY, ITEM_ALWAYSON,
		"Gravity:", &s_scriptdata.server.gravity, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_GRAVITYMODIFIER, ITEM_ALWAYSON,
		"Gravity modifier(float):", NULL, 0, 0, s_scriptdata.server.gravityModifier, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_KNOCKBACK, ITEM_ALWAYSON,
		"Knockback:", &s_scriptdata.server.knockback, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_NOPLAYERCLIP, ITEM_ALWAYSON,
		"No player clip:", &s_scriptdata.server.noplayerclip, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_JUMPHEIGHT, ITEM_ALWAYSON,
		"Jump height:", &s_scriptdata.server.jumpheight, -999999999, 999999999, NULL, 9, 9, NULL }

};


// rules controls
static controlinit_t srv_rules[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_REGENARMOR, ITEM_ALWAYSON,
		"Regen armor:", &s_scriptdata.server.regenarmor, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_AMMOLIMIT, ITEM_ALWAYSON,
		"Ammo limit:", &s_scriptdata.server.ammolimit, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_QUADFACTOR, ITEM_ALWAYSON,
		"Quad factor(float):", NULL, 0, 0, s_scriptdata.server.quadfactor, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_RESPAWNTIME, ITEM_ALWAYSON,
		"Respawn time:", &s_scriptdata.server.respawntime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FORCERESPAWN, ITEM_ALWAYSON,
		"Force respawn:", &s_scriptdata.server.forcerespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_VAMPIRE, ITEM_ALWAYSON,
		"Vampire(float):", NULL, 0, 0, s_scriptdata.server.vampire, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_VAMPIRE_MAX_HEALTH, ITEM_ALWAYSON,
		"Vampire max health:", &s_scriptdata.server.vampire_max_health, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_REGEN, ITEM_ALWAYSON,
		"Regen health:", &s_scriptdata.server.regen, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MAXWEAPONPICKUP, ITEM_ALWAYSON,
		"Max weapon pickup:", &s_scriptdata.server.maxweaponpickup, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_DROPPEDITEMTIME, ITEM_ALWAYSON,
		"Dropped item time:", &s_scriptdata.server.droppeditemtime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_AUTOFLAGRETURN, ITEM_ALWAYSON,
		"Auto flag return:", &s_scriptdata.server.autoflagreturn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_ARMORPROTECT, ITEM_ALWAYSON,
		"Armor protect(float):", NULL, 0, 0, s_scriptdata.server.armorprotect, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_RESPAWNWAIT, ITEM_ALWAYSON,
		"Respawn wait:", &s_scriptdata.server.respawnwait, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SPEEDFACTOR, ITEM_ALWAYSON,
		"Speed powerup factor(float):", NULL, 0, 0, s_scriptdata.server.speedfactor, 9, 9, NULL }

};

// rune1 controls
static controlinit_t srv_rune1[] = {

	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SCOUTSPEEDFACTOR, ITEM_ALWAYSON,
		"Scout rune speed factor(float):", NULL, 0, 0, s_scriptdata.server.scoutspeedfactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SCOUTFIRESPEED, ITEM_ALWAYSON,
		"Scout rune fire speed(float):", NULL, 0, 0, s_scriptdata.server.scoutfirespeed, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SCOUTDAMAGEFACTOR, ITEM_ALWAYSON,
		"Scout rune damage(float):", NULL, 0, 0, s_scriptdata.server.scoutdamagefactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SCOUTGRAVITYMODIFIER, ITEM_ALWAYSON,
		"Scout rune gravity(float):", NULL, 0, 0, s_scriptdata.server.scoutgravitymodifier, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_SCOUT_INFAMMO, ITEM_ALWAYSON,
		"Scout rune infinity ammo:", &s_scriptdata.server.scout_infammo, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_SCOUTHEALTHMODIFIER, ITEM_ALWAYSON,
		"Scout rune health(float):", NULL, 0, 0, s_scriptdata.server.scouthealthmodifier, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_DOUBLERFIRESPEED, ITEM_ALWAYSON,
		"Doubler rune fire speed(float):", NULL, 0, 0, s_scriptdata.server.doublerfirespeed, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_DOUBLERDAMAGEFACTOR, ITEM_ALWAYSON,
		"Doubler rune damage(float):", NULL, 0, 0, s_scriptdata.server.doublerdamagefactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_DOUBLERSPEEDFACTOR, ITEM_ALWAYSON,
		"Doubler rune speed factor(float):", NULL, 0, 0, s_scriptdata.server.doublerspeedfactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_DOUBLERSPEEDFACTOR, ITEM_ALWAYSON,
		"Doubler rune gravity(float):", NULL, 0, 0, s_scriptdata.server.doublergravitymodifier, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_DOUBLER_INFAMMO, ITEM_ALWAYSON,
		"Doubler rune infinity ammo:", &s_scriptdata.server.doubler_infammo, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_DOUBLERHEALTHMODIFIER, ITEM_ALWAYSON,
		"Doubler rune fire speed(float):", NULL, 0, 0, s_scriptdata.server.doublerhealthmodifier, 9, 9, NULL }

};

// rune2 controls
static controlinit_t srv_rune2[] = {

	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_GUARDHEALTHMODIFIER, ITEM_ALWAYSON,
		"Guard rune health(float):", NULL, 0, 0, s_scriptdata.server.guardhealthmodifier, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_GUARDFIRESPEED, ITEM_ALWAYSON,
		"Guard rune fire speed(float):", NULL, 0, 0, s_scriptdata.server.guardfirespeed, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_GUARDDAMAGEFACTOR, ITEM_ALWAYSON,
		"Guard rune damage(float):", NULL, 0, 0, s_scriptdata.server.guarddamagefactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_GUARDSPEEDFACTOR, ITEM_ALWAYSON,
		"Guard rune speed factor(float):", NULL, 0, 0, s_scriptdata.server.guardspeedfactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_GUARDGRAVITYMODIFIER, ITEM_ALWAYSON,
		"Guard rune gravity(float):", NULL, 0, 0, s_scriptdata.server.guardgravitymodifier, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_GUARD_INFAMMO, ITEM_ALWAYSON,
		"Guard rune Infinity ammo:", &s_scriptdata.server.guard_infammo, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_AMMOREGENFIRESPEED, ITEM_ALWAYSON,
		"Ammoregen rune fire speed(float):", NULL, 0, 0, s_scriptdata.server.ammoregenfirespeed, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_AMMOREGEN_INFAMMO, ITEM_ALWAYSON,
		"Ammoregen rune infinity ammo:", &s_scriptdata.server.ammoregen_infammo, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_AMMOREGENDAMAGEFACTOR, ITEM_ALWAYSON,
		"Ammoregen rune damage(float):", NULL, 0, 0, s_scriptdata.server.ammoregendamagefactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_AMMOREGENSPEEDFACTOR, ITEM_ALWAYSON,
		"Ammoregen rune speed factor(float):", NULL, 0, 0, s_scriptdata.server.ammoregenspeedfactor, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_AMMOREGENGRAVITYMODIFIER, ITEM_ALWAYSON,
		"Ammoregen rune gravity(float):", NULL, 0, 0, s_scriptdata.server.ammoregengravitymodifier, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_AMMOREGENHEALTHMODIFIER, ITEM_ALWAYSON,
		"Ammoregen rune health(float):", NULL, 0, 0, s_scriptdata.server.ammoregenhealthmodifier, 9, 9, NULL }

};

// redteam controls
static controlinit_t srv_redteam[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_TEAMRED_SPEED, ITEM_ALWAYSON,
		"Team red speed:", &s_scriptdata.server.teamred_speed, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_TEAMRED_GRAVITYMODIFIER, ITEM_ALWAYSON,
		"Team red gravity modifier(float):", NULL, 0, 0, s_scriptdata.server.teamred_gravityModifier, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_TEAMRED_FIRESPEED, ITEM_ALWAYSON,
		"Team red fire speed(float):", NULL, 0, 0, s_scriptdata.server.teamred_firespeed, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_TEAMRED_DAMAGE, ITEM_ALWAYSON,
		"Team red damage(float):", NULL, 0, 0, s_scriptdata.server.teamred_damage, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_TEAMRED_INFAMMO, ITEM_ALWAYSON,
		"Team red unlimited ammo:", &s_scriptdata.server.teamred_infammo, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_TEAMRED_RESPAWNWAIT, ITEM_ALWAYSON,
		"Team red respawn wait:", &s_scriptdata.server.teamred_respawnwait, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_TEAMRED_PICKUPITEMS, ITEM_ALWAYSON,
		"Team red pickup items:", &s_scriptdata.server.teamred_pickupitems, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATIONREDRESPAWN, ITEM_ALWAYSON,
		"Elimination red respawn:", &s_scriptdata.server.eliminationredrespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_STARTHEALTH, ITEM_ALWAYSON,
		"Elimination red start health:", &s_scriptdata.server.eliminationred_startHealth, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_STARTARMOR, ITEM_ALWAYSON,
		"Elimination red start armor:", &s_scriptdata.server.eliminationred_startArmor, -999999999, 999999999, NULL, 9, 9, NULL }

};

// blueteam controls
static controlinit_t srv_blueteam[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_TEAMBLUE_SPEED, ITEM_ALWAYSON,
		"Team blue speed:", &s_scriptdata.server.teamblue_speed, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_TEAMBLUE_GRAVITYMODIFIER, ITEM_ALWAYSON,
		"Team blue gravity modifier(float):", NULL, 0, 0, s_scriptdata.server.teamblue_gravityModifier, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_TEAMBLUE_FIRESPEED, ITEM_ALWAYSON,
		"Team blue fire speed(float):", NULL, 0, 0, s_scriptdata.server.teamblue_firespeed, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_TEAMBLUE_DAMAGE, ITEM_ALWAYSON,
		"Team blue damage(float):", NULL, 0, 0, s_scriptdata.server.teamblue_damage, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_TEAMBLUE_INFAMMO, ITEM_ALWAYSON,
		"Team blue unlimited ammo:", &s_scriptdata.server.teamblue_infammo, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_TEAMBLUE_RESPAWNWAIT, ITEM_ALWAYSON,
		"Team blue respawn wait:", &s_scriptdata.server.teamblue_respawnwait, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_TEAMBLUE_PICKUPITEMS, ITEM_ALWAYSON,
		"Team blue pickup items:", &s_scriptdata.server.teamblue_pickupitems, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATIONRESPAWN, ITEM_ALWAYSON,
		"Elimination respawn:", &s_scriptdata.server.eliminationrespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_STARTHEALTH, ITEM_ALWAYSON,
		"Elimination start health:", &s_scriptdata.server.elimination_startHealth, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_STARTARMOR, ITEM_ALWAYSON,
		"Elimination start armor:", &s_scriptdata.server.elimination_startArmor, -999999999, 999999999, NULL, 9, 9, NULL }

};

// redteamweapons controls
static controlinit_t srv_redteamweapons[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_GRAPPLE, ITEM_ALWAYSON,
		"Elimination red grapple:", &s_scriptdata.server.eliminationred_grapple, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_GAUNTLET, ITEM_ALWAYSON,
		"Elimination red gauntlet:", &s_scriptdata.server.eliminationred_gauntlet, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_MACHINEGUN, ITEM_ALWAYSON,
		"Elimination red machinegun:", &s_scriptdata.server.eliminationred_machinegun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_SHOTGUN, ITEM_ALWAYSON,
		"Elimination red shotgun:", &s_scriptdata.server.eliminationred_shotgun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_GRENADE, ITEM_ALWAYSON,
		"Elimination red grenade:", &s_scriptdata.server.eliminationred_grenade, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_ROCKET, ITEM_ALWAYSON,
		"Elimination red rocket:", &s_scriptdata.server.eliminationred_rocket, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_RAILGUN, ITEM_ALWAYSON,
		"Elimination red railgun:", &s_scriptdata.server.eliminationred_railgun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_LIGHTNING, ITEM_ALWAYSON,
		"Elimination red lightning:", &s_scriptdata.server.eliminationred_lightning, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_PLASMAGUN, ITEM_ALWAYSON,
		"Elimination red plasmagun:", &s_scriptdata.server.eliminationred_plasmagun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_BFG, ITEM_ALWAYSON,
		"Elimination red bfg:", &s_scriptdata.server.eliminationred_bfg, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_CHAIN, ITEM_ALWAYSON,
		"Elimination red chain:", &s_scriptdata.server.eliminationred_chain, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_MINE, ITEM_ALWAYSON,
		"Elimination red mine:", &s_scriptdata.server.eliminationred_mine, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_NAIL, ITEM_ALWAYSON,
		"Elimination red nail:", &s_scriptdata.server.eliminationred_nail, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_FLAME, ITEM_ALWAYSON,
		"Elimination red flame:", &s_scriptdata.server.eliminationred_flame, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_ANTIMATTER, ITEM_ALWAYSON,
		"Elimination red antimatter:", &s_scriptdata.server.eliminationred_antimatter, -999999999, 999999999, NULL, 9, 9, NULL }

};

// blueteamweapons controls
static controlinit_t srv_blueteamweapons[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_GRAPPLE, ITEM_ALWAYSON,
		"Elimination blue grapple:", &s_scriptdata.server.elimination_grapple, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_GAUNTLET, ITEM_ALWAYSON,
		"Elimination blue gauntlet:", &s_scriptdata.server.elimination_gauntlet, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_MACHINEGUN, ITEM_ALWAYSON,
		"Elimination blue machinegun:", &s_scriptdata.server.elimination_machinegun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_SHOTGUN, ITEM_ALWAYSON,
		"Elimination blue shotgun:", &s_scriptdata.server.elimination_shotgun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_GRENADE, ITEM_ALWAYSON,
		"Elimination blue grenade:", &s_scriptdata.server.elimination_grenade, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_ROCKET, ITEM_ALWAYSON,
		"Elimination blue rocket:", &s_scriptdata.server.elimination_rocket, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_RAILGUN, ITEM_ALWAYSON,
		"Elimination blue railgun:", &s_scriptdata.server.elimination_railgun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_LIGHTNING, ITEM_ALWAYSON,
		"Elimination blue lightning:", &s_scriptdata.server.elimination_lightning, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_PLASMAGUN, ITEM_ALWAYSON,
		"Elimination blue plasmagun:", &s_scriptdata.server.elimination_plasmagun, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_BFG, ITEM_ALWAYSON,
		"Elimination blue bfg:", &s_scriptdata.server.elimination_bfg, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_CHAIN, ITEM_ALWAYSON,
		"Elimination blue chain:", &s_scriptdata.server.elimination_chain, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_MINE, ITEM_ALWAYSON,
		"Elimination blue mine:", &s_scriptdata.server.elimination_mine, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_NAIL, ITEM_ALWAYSON,
		"Elimination blue nail:", &s_scriptdata.server.elimination_nail, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_FLAME, ITEM_ALWAYSON,
		"Elimination blue flame:", &s_scriptdata.server.elimination_flame, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_ANTIMATTER, ITEM_ALWAYSON,
		"Elimination blue antimatter:", &s_scriptdata.server.elimination_antimatter, -999999999, 999999999, NULL, 9, 9, NULL }

};

// teamother controls
static controlinit_t srv_teamother[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_QUAD, ITEM_ALWAYSON,
		"Elimination red quad damage:", &s_scriptdata.server.eliminationred_quad, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_HASTE, ITEM_ALWAYSON,
		"Elimination red haste:", &s_scriptdata.server.eliminationred_haste, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_BSUIT, ITEM_ALWAYSON,
		"Elimination red battle suit:", &s_scriptdata.server.eliminationred_bsuit, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_INVIS, ITEM_ALWAYSON,
		"Elimination red invisibility:", &s_scriptdata.server.eliminationred_invis, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_REGEN, ITEM_ALWAYSON,
		"Elimination red regeneration:", &s_scriptdata.server.eliminationred_regen, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATIONRED_FLIGHT, ITEM_ALWAYSON,
		"Elimination red flight:", &s_scriptdata.server.eliminationred_flight, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_SPIN, 0, ID_SERVER_ELIMINATIONRED_HOLDABLE, ITEM_ALWAYSON,
		"Elimination red holdable:", &s_scriptdata.server.eliminationred_holdable, -999999999, 999999999, NULL, 9, 9, holdable_list },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_QUAD, ITEM_ALWAYSON,
		"Elimination blue quad damage:", &s_scriptdata.server.elimination_quad, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_HASTE, ITEM_ALWAYSON,
		"Elimination blue haste:", &s_scriptdata.server.elimination_haste, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_BSUIT, ITEM_ALWAYSON,
		"Elimination blue battle suit:", &s_scriptdata.server.elimination_bsuit, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_INVIS, ITEM_ALWAYSON,
		"Elimination blue invisibility:", &s_scriptdata.server.elimination_invis, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_REGEN, ITEM_ALWAYSON,
		"Elimination blue regeneration:", &s_scriptdata.server.elimination_regen, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_ELIMINATION_FLIGHT, ITEM_ALWAYSON,
		"Elimination blue flight:", &s_scriptdata.server.elimination_flight, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_SPIN, 0, ID_SERVER_ELIMINATION_HOLDABLE, ITEM_ALWAYSON,
		"Elimination blue holdable:", &s_scriptdata.server.elimination_holdable, -999999999, 999999999, NULL, 9, 9, holdable_list }

};

// other controls
static controlinit_t srv_other[] = {
	{ SRVCTRL_RADIO, 0, ID_SERVER_MINIGAME, ITEM_ALWAYSON,
		"Minigame quad:", &s_scriptdata.server.minigame, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_SPIN, 0, ID_SERVER_OVERLAY, ITEM_ALWAYSON,
		"Overlay:", &s_scriptdata.server.overlay, -999999999, 999999999, NULL, 9, 9, overlay_list },
	{ SRVCTRL_RADIO, 0, ID_SERVER_RANDOMITEMS, ITEM_ALWAYSON,
		"Random items:", &s_scriptdata.server.randomItems, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_SPIN, 0, ID_SERVER_SLICKMOVE, ITEM_ALWAYSON,
		"Player move:", &s_scriptdata.server.slickmove, -999999999, 999999999, NULL, 9, 9, slickmove_list },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SPAWNPROTECT, ITEM_ALWAYSON,
		"Spawn protect:", &s_scriptdata.server.spawnprotect, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_ELIMINATION_LOCKSPECTATOR, ITEM_ALWAYSON,
		"Elimination lock spectator:", &s_scriptdata.server.elimination_lockspectator, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_AWARDPUSHING, ITEM_ALWAYSON,
		"Award pushing:", &s_scriptdata.server.awardpushing, -999999999, 999999999, NULL, 9, 9, NULL }

};


// enviroment controls
static controlinit_t srv_enviroment[] = {

	{ SRVCTRL_RADIO, 0, ID_SERVER_RANDOMTELEPORT, ITEM_ALWAYSON,
		"Random teleport:", &s_scriptdata.server.randomteleport, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FALLDAMAGESMALL, ITEM_ALWAYSON,
		"Fall damage small:", &s_scriptdata.server.falldamagesmall, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FALLDAMAGEBIG, ITEM_ALWAYSON,
		"Fall damage big:", &s_scriptdata.server.falldamagebig, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_WATERDAMAGE, ITEM_ALWAYSON,
		"Water damage:", &s_scriptdata.server.waterdamage, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_LAVADAMAGE, ITEM_ALWAYSON,
		"Lava damage:", &s_scriptdata.server.lavadamage, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SLIMEDAMAGE, ITEM_ALWAYSON,
		"Slime damage:", &s_scriptdata.server.slimedamage, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_RADIO, 0, ID_SERVER_DROWNDAMAGE, ITEM_ALWAYSON,
		"Drown damage:", &s_scriptdata.server.drowndamage, -999999999, 999999999, NULL, 9, 9, NULL }

};


// item controls
static controlinit_t srv_item[] = {

	{ SRVCTRL_SPIN, 0, ID_SERVER_TELEPORTERINF, ITEM_ALWAYSON,
		"Teleporter unlimited:", &s_scriptdata.server.teleporterinf, -999999999, 999999999, NULL, 9, 9, holdableinf_list },
	{ SRVCTRL_SPIN, 0, ID_SERVER_MEDKITINF, ITEM_ALWAYSON,
		"Medkit unlimited:", &s_scriptdata.server.medkitinf, -999999999, 999999999, NULL, 9, 9, holdableinf_list },
	{ SRVCTRL_SPIN, 0, ID_SERVER_KAMIKAZEINF, ITEM_ALWAYSON,
		"Kamikaze unlimited:", &s_scriptdata.server.kamikazeinf, -999999999, 999999999, NULL, 9, 9, holdableinf_list },
	{ SRVCTRL_SPIN, 0, ID_SERVER_INVULINF, ITEM_ALWAYSON,
		"Invulnerability unlimited:", &s_scriptdata.server.invulinf, -999999999, 999999999, NULL, 9, 9, holdableinf_list },
	{ SRVCTRL_SPIN, 0, ID_SERVER_PORTALINF, ITEM_ALWAYSON,
		"Portal unlimited:", &s_scriptdata.server.portalinf, -999999999, 999999999, NULL, 9, 9, holdableinf_list },
	{ SRVCTRL_SPIN, 0, ID_SERVER_INVULMOVE, ITEM_ALWAYSON,
		"Invulnerability move:", &s_scriptdata.server.invulmove, -999999999, 999999999, NULL, 9, 9, invulmove_list },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_INVULTIME, ITEM_ALWAYSON,
		"Invulnerability time:", &s_scriptdata.server.invultime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_PORTALTIMEOUT, ITEM_ALWAYSON,
		"Portal timeout:", &s_scriptdata.server.portaltimeout, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_PORTALHEALTH, ITEM_ALWAYSON,
		"Portal health:", &s_scriptdata.server.portalhealth, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MEDKITLIMIT, ITEM_ALWAYSON,
		"Medkit limit:", &s_scriptdata.server.medkitlimit, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MEDKITMODIFIER, ITEM_ALWAYSON,
		"Medkit modifier:", &s_scriptdata.server.medkitmodifier, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FASTHEALTHREGEN, ITEM_ALWAYSON,
		"Fast health regen:", &s_scriptdata.server.fasthealthregen, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_SLOWHEALTHREGEN, ITEM_ALWAYSON,
		"Slow health regen:", &s_scriptdata.server.slowhealthregen, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_HASTEFIRESPEED, ITEM_ALWAYSON,
		"Haste fire speed(float):", NULL, 0, 0, s_scriptdata.server.hastefirespeed, 9, 9, NULL }

};


// time controls
static controlinit_t srv_time[] = {

	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_QUADTIME, ITEM_ALWAYSON,
		"Quad damage powerup time:", &s_scriptdata.server.quadtime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_BSUITTIME, ITEM_ALWAYSON,
		"Battle suit powerup time:", &s_scriptdata.server.bsuittime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_HASTETIME, ITEM_ALWAYSON,
		"Haste powerup time:", &s_scriptdata.server.hastetime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_INVISTIME, ITEM_ALWAYSON,
		"Invisibility powerup time:", &s_scriptdata.server.invistime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_REGENTIME, ITEM_ALWAYSON,
		"Regeneration powerup time:", &s_scriptdata.server.regentime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FLIGHTTIME, ITEM_ALWAYSON,
		"Flight powerup time:", &s_scriptdata.server.flighttime, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_FLIGHTTIME, ITEM_ALWAYSON,
		"Armor respawn:", &s_scriptdata.server.armorrespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_HEALTHRESPAWN, ITEM_ALWAYSON,
		"Health respawn:", &s_scriptdata.server.healthrespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_AMMORESPAWN, ITEM_ALWAYSON,
		"Ammo respawn:", &s_scriptdata.server.ammorespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_HOLDABLERESPAWN, ITEM_ALWAYSON,
		"Holdable respawn:", &s_scriptdata.server.holdablerespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_MEGAHEALTHRESPAWN, ITEM_ALWAYSON,
		"Megahealth respawn:", &s_scriptdata.server.megahealthrespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_POWERUPRESPAWN, ITEM_ALWAYSON,
		"Powerup respawn:", &s_scriptdata.server.poweruprespawn, -999999999, 999999999, NULL, 9, 9, NULL },
	{ SRVCTRL_NUMFIELD, 0, ID_SERVER_WEAPONRESPAWN, ITEM_ALWAYSON,
		"Weapon respawn:", &s_scriptdata.server.weaponrespawn, -999999999, 999999999, NULL, 9, 9, NULL }

};

// list of controls used for a multiplayer/dedicated game
static initlist_t srv_multiplayerserver[] =
{
	{ ID_SERVERTAB_CONNECT, "CONNECT", &s_servercontrols.connect, srv_connect, ARRAY_COUNT(srv_connect) },
	{ ID_SERVERTAB_ADMIN, "ADMIN", &s_servercontrols.admin, srv_admin, ARRAY_COUNT(srv_admin) },
	{ ID_SERVERTAB_GENERAL, "GENERAL", &s_servercontrols.general, srv_general, ARRAY_COUNT(srv_general) },
	{ ID_SERVERTAB_GAMEMODE, "GAMEMODE", &s_servercontrols.gamemode, srv_gamemode, ARRAY_COUNT(srv_gamemode) },
	{ ID_SERVERTAB_PHYSICS, "PHYSICS", &s_servercontrols.physics, srv_physics, ARRAY_COUNT(srv_physics) },
	{ ID_SERVERTAB_RULES, "RULES", &s_servercontrols.rules, srv_rules, ARRAY_COUNT(srv_rules) },
	{ ID_SERVERTAB_RUNE1, "RUNE1", &s_servercontrols.rune1, srv_rune1, ARRAY_COUNT(srv_rune1) },
	{ ID_SERVERTAB_RUNE2, "RUNE2", &s_servercontrols.rune2, srv_rune2, ARRAY_COUNT(srv_rune2) },
	{ ID_SERVERTAB_REDTEAM, "REDTEAM", &s_servercontrols.redteam, srv_redteam, ARRAY_COUNT(srv_redteam) },
	{ ID_SERVERTAB_BLUETEAM, "BLUETEAM", &s_servercontrols.blueteam, srv_blueteam, ARRAY_COUNT(srv_blueteam) },
	{ ID_SERVERTAB_REDTEAMWEAPONS, "REDWEAPONS", &s_servercontrols.redteamweapons, srv_redteamweapons, ARRAY_COUNT(srv_redteamweapons) },
	{ ID_SERVERTAB_BLUETEAMWEAPONS, "BLUEWEAPONS", &s_servercontrols.blueteamweapons, srv_blueteamweapons, ARRAY_COUNT(srv_blueteamweapons) },
	{ ID_SERVERTAB_TEAMOTHER, "TEAMOTHER", &s_servercontrols.teamother, srv_teamother, ARRAY_COUNT(srv_teamother) },
	{ ID_SERVERTAB_OTHER, "OTHER", &s_servercontrols.other, srv_other, ARRAY_COUNT(srv_other) },
	{ ID_SERVERTAB_ENVIROMENT, "ENVIROMENT", &s_servercontrols.enviroment, srv_enviroment, ARRAY_COUNT(srv_enviroment) },
	{ ID_SERVERTAB_ITEM, "ITEM", &s_servercontrols.item, srv_item, ARRAY_COUNT(srv_item) },
	{ ID_SERVERTAB_TIME, "TIME", &s_servercontrols.time, srv_time, ARRAY_COUNT(srv_time) },
};

/*
=================
StartServer_ServerPage_SetControlState
=================
*/
static void StartServer_ServerPage_SetControlState(menucommon_s* c,int type)
{
	// set its appearance
	switch (type) {
	default:
		c->flags &= ~(QMF_HIDDEN|QMF_INACTIVE|QMF_GRAYED);
		break;
	case QMF_HIDDEN:
		c->flags &= ~(QMF_GRAYED);
		c->flags |= (QMF_HIDDEN|QMF_INACTIVE);
		break;
	case QMF_GRAYED:
		c->flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
		c->flags |= (QMF_GRAYED);
	}
}



/*
=================
StartServer_ServerPage_SetControlFromId
=================
*/
static void StartServer_ServerPage_SetControlFromId(int id, int type)
{
	int i;

	// locate control
	for (i = 0; i < s_servercontrols.num_radio; i++) {
		if (s_servercontrols.radio[i].init->id == id)
		{
			StartServer_ServerPage_SetControlState(&s_servercontrols.radio[i].control.generic, type);
			return;
		}
	}

	for (i = 0; i < s_servercontrols.num_field; i++)
	{
		if (s_servercontrols.field[i].init->id == id) {
			StartServer_ServerPage_SetControlState(&s_servercontrols.field[i].control.generic, type);
			return;
		}
	}

	for (i = 0; i < s_servercontrols.num_spin; i++)
	{
		if (s_servercontrols.spin[i].init->id == id) {
			StartServer_ServerPage_SetControlState(&s_servercontrols.spin[i].control.generic, type);
			return;
		}
	}
}



/*
=================
StartServer_ServerPage_UpdateInterface
=================
*/
static void StartServer_ServerPage_UpdateInterface(void)
{
	int i;
	initlist_t* group;
	qboolean lastSet;

	StartServer_SetIconFromGameType(&s_servercontrols.gameTypeIcon, s_scriptdata.gametype, qfalse);

	// clear visible controls
	for (i = 0; i < s_servercontrols.num_radio; i++) {
		StartServer_ServerPage_SetControlState(&s_servercontrols.radio[i].control.generic, 0);
	}

	for (i = 0; i < s_servercontrols.num_field; i++) {
		StartServer_ServerPage_SetControlState(&s_servercontrols.field[i].control.generic, 0);
	}

	for (i = 0; i < s_servercontrols.num_spin; i++) {
		StartServer_ServerPage_SetControlState(&s_servercontrols.spin[i].control.generic, 0);
	}


	// walk item list, setting states as appropriate
	group = &s_servercontrols.pageList[s_servercontrols.activePage];
	lastSet = qfalse;
	for (i = 0; i < group->count; i++) {
		// hide/show control based on previous set value
		// if item is grayed then we avoid changing the lastSet flag by
		// continuing. This prevents a grayed *radio* control from affecting
		// a group of grayed controls
		if ((group->list[i].flags & ITEM_GRAYIF_PREVON) && lastSet) {
			StartServer_ServerPage_SetControlFromId(group->list[i].id, QMF_GRAYED);
			continue;
		}

		if ((group->list[i].flags & ITEM_GRAYIF_PREVOFF) && !lastSet) {
			StartServer_ServerPage_SetControlFromId(group->list[i].id, QMF_GRAYED);
			continue;
		}

		// update status of "most recent" spin control
		if (group->list[i].type == SRVCTRL_RADIO) {
			if (s_servercontrols.radio[ group->list[i].offset ].control.curvalue)
				lastSet = qtrue;
			else
				lastSet = qfalse;
		}
	}

	// enable fight button if possible
	if (!StartServer_CheckFightReady(&s_servercontrols.common)) {
		s_servercontrols.saveScript.generic.flags |= QMF_GRAYED;
	}
	else {
		s_servercontrols.saveScript.generic.flags &= ~QMF_GRAYED;
	}
}



/*
=================
StartServer_ServerPage_Cache
=================
*/
void StartServer_ServerPage_Cache( void )
{
	trap_R_RegisterShaderNoMip( SERVER_SAVE0 );
	trap_R_RegisterShaderNoMip( SERVER_SAVE1 );
}





/*
=================
StartServer_ServerPage_Save
=================
*/
static void StartServer_ServerPage_Save( void )
{
	StartServer_SaveScriptData();
}






/*
=================
StartServer_ServerPage_CheckMinMaxFail
=================
*/
static qboolean StartServer_ServerPage_CheckMinMaxFail( controlinit_t* c )
{
	int value;

	if (c->min == c->max)
		return qfalse;

	value = *(c->number);
	if (value < c->min)
	{
		*(c->number) = c->min;
		return qtrue;
	}
	if (value > c->max)
	{
		*(c->number) = c->max;
		return qtrue;
	}

	return qfalse;
}




/*
=================
StartServer_ServerPage_SpinEvent
=================
*/
static void StartServer_ServerPage_SpinEvent( void* ptr, int event )
{
	int id;
	spincontrol_t* s;

	if (event != QM_ACTIVATED)
		return;

	id = ((menucommon_s*)ptr)->id;

	s = &s_servercontrols.spin[id];

	*(s->init->number) = s_servercontrols.spin[id].control.curvalue;
	StartServer_ServerPage_UpdateInterface();
}




/*
=================
StartServer_ServerPage_RadioEvent
=================
*/
static void StartServer_ServerPage_RadioEvent( void* ptr, int event )
{
	int id;
	radiobuttoncontrol_t* r;

	if (event != QM_ACTIVATED)
		return;

	id = ((menucommon_s*)ptr)->id;

	r = &s_servercontrols.radio[id];

	*(r->init->number) = s_servercontrols.radio[id].control.curvalue;

	StartServer_ServerPage_UpdateInterface();
}



/*
=================
StartServer_ServerPage_FieldEvent
=================
*/
static void StartServer_ServerPage_FieldEvent( void* ptr, int event )
{
	int id;
	textfieldcontrol_t* r;
	controlinit_t* c;

	if (event != QM_LOSTFOCUS)
		return;

	id = ((menucommon_s*)ptr)->id;

	r = &s_servercontrols.field[id];
	c = r->init;

	if (c->type == SRVCTRL_NUMFIELD)
	{
		*(c->number) = atoi(r->control.field.buffer);
		if (StartServer_ServerPage_CheckMinMaxFail(c)) {
			Q_strncpyz(r->control.field.buffer, va("%i", *(c->number)), c->arraysize);
		}
	}
	else if (c->type == SRVCTRL_TEXTFIELD)
	{
		Q_strncpyz(c->array, r->control.field.buffer, c->arraysize);
	}

	StartServer_ServerPage_UpdateInterface();
}




/*
=================
StartServer_ServerPage_SetStatusBar
=================
*/
static void StartServer_ServerPage_SetStatusBar( const char* text )
{
	s_servercontrols.savetime = uis.realtime + STATUSBAR_FADETIME;
	if (text)
		Q_strncpyz(s_servercontrols.statusbar_message, text, MAX_STATUSBAR_TEXT);
}



/*
=================
StartServer_ServerPage_ControlListStatusBar
=================
*/
static void StartServer_ServerPage_ControlListStatusBar(void* ptr)
{
	menucommon_s* m;
	controlinit_t* c;
	int i;

	m = (menucommon_s*)ptr;

	switch (m->type) {
	case MTYPE_RADIOBUTTON:
		c = s_servercontrols.radio[m->id].init;
		break;

	case MTYPE_SPINCONTROL:
		c = s_servercontrols.spin[m->id].init;
		break;

	case MTYPE_FIELD:
		c = s_servercontrols.field[m->id].init;
		break;

	default:
		Com_Printf("Status bar: unsupported control type (%i)\n", m->id);
		return;
	}


	switch (c->id) {
	case ID_SERVER_CONFIGBUG:
		StartServer_ServerPage_SetStatusBar("fixes change of time and frag limits (pre 1.30 only)");
		return;

	case ID_SERVER_PMOVEMSEC:
		StartServer_ServerPage_SetStatusBar("pmove step time, minimum 8 (recommended)");
		return;

	case ID_SERVER_PMOVEFIXED:
		StartServer_ServerPage_SetStatusBar("on = same movement physics for all players");
		return;

	case ID_SERVER_RESPAWN:
		StartServer_ServerPage_SetStatusBar("forces respawn of dead player");
		return;

	case ID_SERVER_ALLOWMAXRATE:
		StartServer_ServerPage_SetStatusBar("limit data sent to a single client per second");
		return;

	case ID_SERVER_MAXRATE:
		StartServer_ServerPage_SetStatusBar("8000 or 10000 are typical, 0 = no limit");
		return;

	case ID_SERVER_DEDICATED:
		StartServer_ServerPage_SetStatusBar("LAN = local game, Internet = announced to master server");
		return;

	case ID_SERVER_SMOOTHCLIENTS:
		StartServer_ServerPage_SetStatusBar("on = server allows a client to predict other player movement");
		return;

	case ID_SERVER_SYNCCLIENTS:
		StartServer_ServerPage_SetStatusBar("on = allows client to record demos, may affect performance");
		return;

	case ID_SERVER_GRAVITY:
		StartServer_ServerPage_SetStatusBar("affects all maps, default = 800");
		return;
		
	case ID_SERVER_JUMPHEIGHT:
		StartServer_ServerPage_SetStatusBar("affects all maps, default = 270");
		return;

	case ID_SERVER_KNOCKBACK:
		StartServer_ServerPage_SetStatusBar("kickback from damage, default = 1000");
		return;

	case ID_SERVER_QUADFACTOR:
		StartServer_ServerPage_SetStatusBar("additional damage caused by quad, default = 3");
		return;

	case ID_SERVER_NETPORT:
		StartServer_ServerPage_SetStatusBar("server listens on this port for client connections");
		return;

	case ID_SERVER_SVFPS:
		StartServer_ServerPage_SetStatusBar("server framerate, default = 20");
		return;

	case ID_SERVER_ALLOWPRIVATECLIENT:
		StartServer_ServerPage_SetStatusBar("reserve some player slots - password required");
		return;

	case ID_SERVER_PRIVATECLIENT:
		StartServer_ServerPage_SetStatusBar("number of reserved slots, reduces max clients");
		return;

	case ID_SERVER_WEAPONRESPAWN:
		StartServer_ServerPage_SetStatusBar("time before weapon respawns, default = 5, TeamDM = 30");
		return;

	case ID_SERVER_ALLOWPASS:
		StartServer_ServerPage_SetStatusBar("all clients must use a password to connect");
		return;

	case ID_SERVER_ALLOWMINPING:
	case ID_SERVER_ALLOWMAXPING:
	case ID_SERVER_MINPING:
	case ID_SERVER_MAXPING:
		StartServer_ServerPage_SetStatusBar("Client ping limit, tested on first connection");
		return;
	}
}






/*
=================
StartServer_ServerPage_InitSpinCtrl
=================
*/
static void StartServer_ServerPage_InitSpinCtrl(controlinit_t* c, int height)
{
	spincontrol_t* s;

	if (s_servercontrols.num_spin == MAX_SERVER_SPIN_CONTROL) {
		Com_Printf("ServerPage: Max spin controls (%i) reached\n", s_servercontrols.num_spin);
		return;
	}

	if (!c->number) {
		Com_Printf("ServerPage: Spin control (id:%i) has no data\n", c->id);
		return;
	}

	if (!c->itemnames) {
		Com_Printf("ServerPage: Spin control (id:%i) has no strings\n", c->id);
		return;
	}

	s = &s_servercontrols.spin[s_servercontrols.num_spin];

	s->init = c;

	s->control.generic.type		= MTYPE_SPINCONTROL;
	s->control.generic.name		= c->title;
	s->control.generic.id		= s_servercontrols.num_spin;	// self reference
	s->control.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_RIGHT_JUSTIFY;
	s->control.generic.callback	= StartServer_ServerPage_SpinEvent;
	s->control.generic.statusbar	= StartServer_ServerPage_ControlListStatusBar;
	s->control.generic.x			= CONTROL_POSX;
	s->control.generic.y			= height;
	s->control.itemnames 			= c->itemnames;

	s->control.curvalue = *(c->number);

	// item is registered but we have to do the init here
	SpinControl_Init(&s->control);

	c->offset = s_servercontrols.num_spin++;
}




/*
=================
StartServer_ServerPage_InitRadioCtrl
=================
*/
static void StartServer_ServerPage_InitRadioCtrl(controlinit_t* c, int height)
{
	radiobuttoncontrol_t* r;

	if (s_servercontrols.num_radio == MAX_SERVER_RADIO_CONTROL) {
		Com_Printf("ServerPage: Max radio controls (%i) reached\n", s_servercontrols.num_radio);
		return;
	}

	if (!c->number) {
		Com_Printf("ServerPage: Radio control (id:%i) has no data\n", c->id);
		return;
	}

	r = &s_servercontrols.radio[s_servercontrols.num_radio];

	r->init = c;

	r->control.generic.type		= MTYPE_RADIOBUTTON;
	r->control.generic.name		= c->title;
	r->control.generic.id		= s_servercontrols.num_radio;	// self reference
	r->control.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_RIGHT_JUSTIFY;
	r->control.generic.callback	= StartServer_ServerPage_RadioEvent;
	r->control.generic.statusbar	= StartServer_ServerPage_ControlListStatusBar;
	r->control.generic.x			= CONTROL_POSX;
	r->control.generic.y			= height;

	r->control.curvalue = *(c->number);

	c->offset = s_servercontrols.num_radio++;
}



/*
=================
StartServer_ServerPage_InitFieldCtrl
=================
*/
static void StartServer_ServerPage_InitFieldCtrl(controlinit_t* c, int height)
{
	textfieldcontrol_t* f;
	int flags;

	if (s_servercontrols.num_field == MAX_SERVER_MFIELD_CONTROL) {
		Com_Printf("ServerPage: Max field controls (%i) reached\n", s_servercontrols.num_field);
		return;
	}

	if (c->type == SRVCTRL_NUMFIELD && !c->number) {
		Com_Printf("ServerPage: NumField control (id:%i) has no data\n", c->id);
		return;
	}

	if (c->type == SRVCTRL_TEXTFIELD && !c->array) {
		Com_Printf("ServerPage: TextField control (id:%i) has no data\n", c->id);
		return;
	}

	f = &s_servercontrols.field[s_servercontrols.num_field];

	f->init = c;

	flags = QMF_SMALLFONT|QMF_PULSEIFFOCUS|QMF_RIGHT_JUSTIFY;
	if (c->type == SRVCTRL_NUMFIELD)
		flags |= QMF_NUMBERSONLY;

	f->control.generic.type       = MTYPE_FIELD;
	f->control.generic.name       = c->title;
	f->control.generic.id		= s_servercontrols.num_field;	// self reference
	f->control.generic.callback	= StartServer_ServerPage_FieldEvent;
	f->control.generic.statusbar	= StartServer_ServerPage_ControlListStatusBar;
	f->control.generic.flags      = flags;
	f->control.generic.x			= CONTROL_POSX;
	f->control.generic.y			= height;

	f->control.field.widthInChars = c->displaysize;
	f->control.field.maxchars     = c->arraysize;

	// setting of field is deferred until after init

	c->offset = s_servercontrols.num_field++;
}




/*
=================
StartServer_ServerPage_InitControlList
=================
*/
static void StartServer_ServerPage_InitControlList(initlist_t* il)
{
	int i;
	int offset;
	controlinit_t* list;
	controlinit_t* c;
	textfieldcontrol_t* f;
	int size;
	qboolean teamGame;

	list = il->list;
	size = il->count;

	teamGame = qfalse;
	if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS) )
		teamGame = qtrue;

	offset = 0;	// relative to top of list
	for (i = 0; i < size; i++)
	{
		if (teamGame && (list[i].flags & ITEM_NOTEAM_GAME))
			continue;
		if (!teamGame && (list[i].flags & ITEM_TEAM_GAME))
			continue;

		switch (list[i].type) {
		case SRVCTRL_SPIN:
			StartServer_ServerPage_InitSpinCtrl(&list[i], offset);
			break;
		case SRVCTRL_RADIO:
			StartServer_ServerPage_InitRadioCtrl(&list[i], offset);
			break;
		case SRVCTRL_TEXTFIELD:
		case SRVCTRL_NUMFIELD:
			StartServer_ServerPage_InitFieldCtrl(&list[i], offset);
			break;
		case SRVCTRL_BLANK:
			break;
		}

		offset += LINE_HEIGHT;

		if (list[i].flags & ITEM_HALFGAP)
			offset += (LINE_HEIGHT/2);
	}

	// move all controls to middle of screen and init them
	offset = TABCONTROLCENTER_Y - offset/2;
	for (i = 0; i < s_servercontrols.num_radio; i++) {
		s_servercontrols.radio[i].control.generic.y += offset;

		// item is registered but we have to do the init here
		RadioButton_Init(&s_servercontrols.radio[i].control);
	}

	for (i = 0; i < s_servercontrols.num_field; i++) {
		c = s_servercontrols.field[i].init;
		f = &s_servercontrols.field[i];

		f->control.generic.y += offset;

		// item is registered but we have to do the init here
		MenuField_Init(&f->control);

		// deferred init because MenuField_Init() clears text
		if (c->type == SRVCTRL_NUMFIELD) {
			Q_strncpyz(f->control.field.buffer, va("%i", *c->number), c->arraysize);
		}
		else if (c->type == SRVCTRL_TEXTFIELD) {
			Q_strncpyz(f->control.field.buffer, c->array, c->arraysize);
		}
	}

	for (i = 0; i < s_servercontrols.num_spin; i++) {
		s_servercontrols.spin[i].control.generic.y += offset;

		// item is registered but we have to do the init here
		SpinControl_Init(&s_servercontrols.spin[i].control);
	}
}




/*
=================
StartServer_ServerPage_SetTab
=================
*/
static void StartServer_ServerPage_SetTab( int tab )
{
	int i, index;
	initlist_t* controlList;

	if (tab == -1) {
		index = s_servercontrols.activePage;
	}
	else {
		index = -1;
		for (i = 0; i < s_servercontrols.pageListSize; i++) {
			if (s_servercontrols.pageList[i].menutext->generic.id == tab) {
				index = i;
				break;
			}
		}
	}

	// walk all controls and mark them hidden
	for (i = 0; i < s_servercontrols.num_radio; i++) {
		StartServer_ServerPage_SetControlState(&s_servercontrols.radio[i].control.generic, QMF_HIDDEN);
	}

	for (i = 0; i < s_servercontrols.num_field; i++) {
		StartServer_ServerPage_SetControlState(&s_servercontrols.field[i].control.generic, QMF_HIDDEN);
	}

	for (i = 0; i < s_servercontrols.num_spin; i++) {
		StartServer_ServerPage_SetControlState(&s_servercontrols.spin[i].control.generic, QMF_HIDDEN);
	}

	if (index == -1) {
		Com_Printf("Server tab: unknown index (%i)\n", tab);
		return;
	}

	// set the controls for the current tab
	s_servercontrols.activePage = index;
	s_servercontrols.num_radio = 0;
	s_servercontrols.num_field = 0;
	s_servercontrols.num_spin = 0;

	StartServer_ServerPage_InitControlList(&s_servercontrols.pageList[index]);

	// brighten active tab control, remove pulse effect
	for (i = 0; i < s_servercontrols.pageListSize; i++) {
		s_servercontrols.pageList[i].menutext->generic.flags &= ~(QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		s_servercontrols.pageList[i].menutext->generic.flags |= QMF_PULSEIFFOCUS;
	}

	s_servercontrols.pageList[index].menutext->generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_servercontrols.pageList[index].menutext->generic.flags &= ~(QMF_PULSEIFFOCUS);
}




/*
=================
StartServer_ServerPage_InitControlsFromScript
=================
*/
static void StartServer_ServerPage_InitControlsFromScript( int tab )
{
	int i;
	radiobuttoncontrol_t* r;
	textfieldcontrol_t* t;
	spincontrol_t* s;

	StartServer_ServerPage_SetTab(tab);

	// set hostname
	Q_strncpyz(s_servercontrols.hostname.field.buffer, s_scriptdata.server.hostname, MAX_HOSTNAME_LENGTH);
}




/*
=================
StartServer_ServerPage_Load
=================
*/
static void StartServer_ServerPage_Load( void )
{
	s_servercontrols.gameType.curvalue = s_scriptdata.gametype;

	StartServer_ServerPage_InitControlsFromScript(-1);
}





/*
=================
StartServer_ServerPage_CommonEvent
=================
*/
static void StartServer_ServerPage_CommonEvent( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}

	StartServer_ServerPage_Save();
	switch( ((menucommon_s*)ptr)->id )
	{
		case ID_SERVERCOMMON_ITEMS:
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_BOTS:
			UI_PopMenu();
			UI_PopMenu();
			break;
			
		case ID_SERVERCOMMON_WEAPON:
			UI_PopMenu();
			StartServer_WeaponPage_MenuInit();
			break;

		case ID_SERVERCOMMON_MAPS:
			UI_PopMenu();
			UI_PopMenu();
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_BACK:
			UI_PopMenu();
			UI_PopMenu();
			UI_PopMenu();
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_FIGHT:
			StartServer_CreateServer(NULL);
			break;
	}
}



/*
=================
StartServer_ServerPage_DrawStatusBarText
=================
*/
static void StartServer_ServerPage_DrawStatusBarText( void )
{
	vec4_t color;
	int fade;
	float fadecol;

	if (uis.realtime > s_servercontrols.savetime)
		return;

	if (!s_servercontrols.statusbar_message[0])
		return;

	fade = s_servercontrols.savetime - uis.realtime;

	fadecol = (float)fade/STATUSBAR_FADETIME;

	color[0] = 1.0;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = fadecol;

	UI_DrawString(320, s_servercontrols.statusbar_height,
		s_servercontrols.statusbar_message, UI_CENTER|UI_SMALLFONT, color);
}



/*
=================
StartServer_ServerPage_MenuKey
=================
*/
static sfxHandle_t StartServer_ServerPage_MenuKey( int key )
{
	switch (key)
	{
		case K_MOUSE2:
		case K_ESCAPE:
			StartServer_ServerPage_Save();
			UI_PopMenu();
			UI_PopMenu();
			UI_PopMenu();
			break;
	}

	return ( Menu_DefaultKey( &s_servercontrols.menu, key ) );
}





/*
=================
StartServer_ServerPage_SaveScript
=================
*/
static qboolean StartServer_ServerPage_SaveScript( const char* filename )
{
	UI_PopMenu();
	if (StartServer_CreateServer( filename )) {
		StartServer_ServerPage_SetStatusBar("Saved!");
		return qtrue;
	}
	else {
		StartServer_ServerPage_SetStatusBar(StartServer_GetLastScriptError());
		return qfalse;
	}
}


/*
=================
StartServer_ServerPage_LoadScript
=================
*/
static qboolean StartServer_ServerPage_LoadScript( const char* filename )
{
	UI_PopMenu();

	if (StartServer_LoadFromConfig(filename))
	{
		// so far we've only loaded the cvars, now we need
		// to refresh the stored data
		StartServer_LoadScriptDataFromType(s_scriptdata.gametype);

		StartServer_ServerPage_Load();
		StartServer_ServerPage_UpdateInterface();

		StartServer_ServerPage_SetStatusBar("Loaded!");
		
		MainMenu_ReloadGame();
	}
	return qtrue;
}


/*
=================
StartServer_ServerPage_Mods
=================
*/
void StartServer_ServerPage_Mods(void)
{
UI_StartServerMenu( qtrue );
StartServer_MapPage_MenuInit();
StartServer_BotPage_MenuInit();
StartServer_ItemPage_MenuInit();
StartServer_ServerPage_MenuInit();
StartServer_ServerPage_Save();
if(cl_language.integer == 0){
UI_ConfigMenu("LOAD SCRIPT", qtrue, StartServer_ServerPage_LoadScript);
}
if(cl_language.integer == 1){
UI_ConfigMenu("ЗАГРУЗИТЬ СКРИПТ", qtrue, StartServer_ServerPage_LoadScript);
}
}


/*
=================
StartServer_ServerPage_Event
=================
*/
static void StartServer_ServerPage_Event( void* ptr, int event )
{
	int value;
	int id;

	id = ((menucommon_s*)ptr)->id;
	switch (id) {
		case ID_SERVER_GAMETYPE:
			if( event != QM_ACTIVATED ) {
				return;
			}

			StartServer_SaveScriptData();
			StartServer_LoadScriptDataFromType(s_servercontrols.gameType.curvalue);

			StartServer_ServerPage_InitControlsFromScript(-1);	// gametype is already accurate
			StartServer_ServerPage_UpdateInterface();
			break;

		case ID_SERVER_HOSTNAME:
			if (event != QM_LOSTFOCUS)
				return;

			// store hostname
			Q_strncpyz(s_scriptdata.server.hostname, s_servercontrols.hostname.field.buffer, MAX_HOSTNAME_LENGTH);
			break;

		case ID_SERVER_SAVE:
			if( event != QM_ACTIVATED ) {
				return;
			}
			StartServer_ServerPage_Save();
			if(cl_language.integer == 0){
			UI_ConfigMenu("SAVE SCRIPT", qfalse, StartServer_ServerPage_SaveScript);
			}
			if(cl_language.integer == 1){
			UI_ConfigMenu("СОХРАНИТЬ СКРИПТ", qfalse, StartServer_ServerPage_SaveScript);
			}
			break;

		case ID_SERVER_LOAD:
			if( event != QM_ACTIVATED ) {
				return;
			}
			StartServer_ServerPage_Save();
			if(cl_language.integer == 0){
			UI_ConfigMenu("LOAD SCRIPT", qtrue, StartServer_ServerPage_LoadScript);
			}
			if(cl_language.integer == 1){
			UI_ConfigMenu("ЗАГРУЗИТЬ СКРИПТ", qtrue, StartServer_ServerPage_LoadScript);
			}
			break;

case ID_SERVERTAB_CONNECT:
case ID_SERVERTAB_ADMIN:
case ID_SERVERTAB_GENERAL:
case ID_SERVERTAB_GAMEMODE:
case ID_SERVERTAB_PHYSICS:
case ID_SERVERTAB_RULES:
case ID_SERVERTAB_RUNE1:
case ID_SERVERTAB_RUNE2:
case ID_SERVERTAB_REDTEAM:
case ID_SERVERTAB_BLUETEAM:
case ID_SERVERTAB_REDTEAMWEAPONS:
case ID_SERVERTAB_BLUETEAMWEAPONS:
case ID_SERVERTAB_TEAMOTHER:
case ID_SERVERTAB_OTHER:
case ID_SERVERTAB_ENVIROMENT:
case ID_SERVERTAB_ITEM:
case ID_SERVERTAB_TIME:
			if( event != QM_ACTIVATED ) {
				return;
			}
			StartServer_ServerPage_SetTab(id);
			StartServer_ServerPage_UpdateInterface();
			break;
	}
}


/*
=================
StartServer_ServerPage_TabEvent
=================
*/
static void StartServer_ServerPage_TabEvent( void* ptr, int event )
{
	int id;

	if( event != QM_ACTIVATED ) {
		return;
	}

	id = ((menucommon_s*)ptr)->id;
	StartServer_ServerPage_SetTab(id);
	StartServer_ServerPage_UpdateInterface();
}





/*
=================
StartServer_ServerPage_MenuDraw
=================
*/
static void StartServer_ServerPage_MenuDraw(void)
{
	if (uis.firstdraw) {
		StartServer_ServerPage_Load();
		StartServer_ServerPage_UpdateInterface();
	}

	StartServer_BackgroundDraw(qfalse);

	// draw the controls
	Menu_Draw(&s_servercontrols.menu);

	// if we have no server controls, say so
	if (s_servercontrols.num_spin == 0 && s_servercontrols.num_radio == 0 &&
		s_servercontrols.num_field == 0)
	{
		UI_DrawString(CONTROL_POSX, TABCONTROLCENTER_Y - LINE_HEIGHT/2,
			"<no controls>", UI_RIGHT|UI_SMALLFONT, text_color_disabled);
	}

	StartServer_ServerPage_DrawStatusBarText();
}




/*
=================
StartServer_ServerPage_StatusBar
=================
*/
static void StartServer_ServerPage_StatusBar(void* ptr)
{
	switch (((menucommon_s*)ptr)->id) {
	case ID_SERVER_SAVE:
		StartServer_ServerPage_SetStatusBar("Create a script for use with \\exec");
		break;

	case ID_SERVER_LOAD:
		StartServer_ServerPage_SetStatusBar("Load a previously saved UIE script");
		break;
	};
}




/*
=================
StartServer_ServerPage_MenuInit
=================
*/
void StartServer_ServerPage_MenuInit(void)
{
	menuframework_s* menuptr;
	int y, y_base;
	int i;
	int style;
	float scale;

	memset(&s_servercontrols, 0, sizeof(servercontrols_t));

	StartServer_ServerPage_Cache();

	menuptr = &s_servercontrols.menu;

	menuptr->key = StartServer_ServerPage_MenuKey;
	menuptr->wrapAround = qtrue;
	menuptr->native 	= qfalse;
	menuptr->fullscreen = qtrue;
	menuptr->draw = StartServer_ServerPage_MenuDraw;

	StartServer_CommonControls_Init(menuptr, &s_servercontrols.common, StartServer_ServerPage_CommonEvent, COMMONCTRL_SERVER);


	//
	// the user controlled server params
	//

	y = GAMETYPEROW_Y;
	s_servercontrols.gameType.generic.type		= MTYPE_SPINCONTROL;
	s_servercontrols.gameType.generic.id		= ID_SERVER_GAMETYPE;
	s_servercontrols.gameType.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_servercontrols.gameType.generic.callback	= StartServer_ServerPage_Event;
	s_servercontrols.gameType.generic.x			= GAMETYPECOLUMN_X;
	s_servercontrols.gameType.generic.y			= y;

	s_servercontrols.gameTypeIcon.generic.type  = MTYPE_BITMAP;
	s_servercontrols.gameTypeIcon.generic.flags = QMF_INACTIVE;
	s_servercontrols.gameTypeIcon.generic.x	 = GAMETYPEICON_X;
	s_servercontrols.gameTypeIcon.generic.y	 = y;
	s_servercontrols.gameTypeIcon.width  	     = 32;
	s_servercontrols.gameTypeIcon.height  	     = 32;

	y += LINE_HEIGHT;
	s_servercontrols.hostname.generic.type       = MTYPE_FIELD;
	s_servercontrols.hostname.generic.flags      = QMF_SMALLFONT|QMF_PULSEIFFOCUS;
	s_servercontrols.hostname.generic.x          = GAMETYPECOLUMN_X;
	s_servercontrols.hostname.generic.y	        = y;
	s_servercontrols.hostname.generic.id	        = ID_SERVER_HOSTNAME;
	s_servercontrols.hostname.generic.callback	= StartServer_ServerPage_Event;
	s_servercontrols.hostname.field.widthInChars = 32;
	s_servercontrols.hostname.field.maxchars     = MAX_HOSTNAME_LENGTH;


	s_servercontrols.num_radio = 0;
	s_servercontrols.num_field = 0;
	s_servercontrols.num_spin = 0;

	y += LINE_HEIGHT;

	s_servercontrols.saveScript.generic.type  = MTYPE_BITMAP;
	s_servercontrols.saveScript.generic.name  = SERVER_SAVE0;
	s_servercontrols.saveScript.generic.flags = QMF_PULSEIFFOCUS;
	s_servercontrols.saveScript.generic.x	   =  320 - 128;
	s_servercontrols.saveScript.generic.y	   = 480 - 64;
	s_servercontrols.saveScript.generic.id	   = ID_SERVER_SAVE;
	s_servercontrols.saveScript.generic.callback = StartServer_ServerPage_Event;
	s_servercontrols.saveScript.generic.statusbar = StartServer_ServerPage_StatusBar;
	s_servercontrols.saveScript.width = 128;
	s_servercontrols.saveScript.height = 64;
	s_servercontrols.saveScript.focuspic = SERVER_SAVE1;

	s_servercontrols.loadScript.generic.type  = MTYPE_BITMAP;
	s_servercontrols.loadScript.generic.name  = SERVER_LOAD0;
	s_servercontrols.loadScript.generic.flags = QMF_PULSEIFFOCUS;
	s_servercontrols.loadScript.generic.x	   =  320;
	s_servercontrols.loadScript.generic.y	   = 480 - 64;
	s_servercontrols.loadScript.generic.id	   = ID_SERVER_LOAD;
	s_servercontrols.loadScript.generic.callback = StartServer_ServerPage_Event;
	s_servercontrols.loadScript.generic.statusbar = StartServer_ServerPage_StatusBar;
	s_servercontrols.loadScript.width = 128;
	s_servercontrols.loadScript.height = 64;
	s_servercontrols.loadScript.focuspic = SERVER_LOAD1;

	s_servercontrols.statusbar_height = 480 - 64 - LINE_HEIGHT;
	
	if(cl_language.integer == 0){
	s_servercontrols.gameType.generic.name		= "Game Type:";
	s_servercontrols.gameType.itemnames			= gametype_items;
	s_servercontrols.hostname.generic.name       = "Hostname:";
	}
	if(cl_language.integer == 1){
	s_servercontrols.gameType.generic.name		= "Режим Игры:";
	s_servercontrols.gameType.itemnames			= gametype_itemsru;
	s_servercontrols.hostname.generic.name       = "Имя сервера:";
	}


	// init the tabbed control list for the page
	s_servercontrols.activePage = 0;
	if (s_scriptdata.multiplayer) {
   		s_servercontrols.pageList = srv_multiplayerserver;
	   	s_servercontrols.pageListSize = ARRAY_COUNT(srv_multiplayerserver);
	}

	style = UI_RIGHT|UI_MEDIUMFONT;
	scale = UI_ProportionalSizeScale(style, 0);
	y_base = TABCONTROLCENTER_Y - s_servercontrols.pageListSize*PROP_HEIGHT*scale/2;

	for (i = 0; i < s_servercontrols.pageListSize; i++)
	{
		s_servercontrols.pageList[i].menutext->generic.type     = MTYPE_PTEXT;
		s_servercontrols.pageList[i].menutext->generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_servercontrols.pageList[i].menutext->generic.id	     = s_servercontrols.pageList[i].id;
		s_servercontrols.pageList[i].menutext->generic.callback = StartServer_ServerPage_TabEvent;
		s_servercontrols.pageList[i].menutext->generic.x	     = 140 - uis.wideoffset/2;
		s_servercontrols.pageList[i].menutext->generic.y	     = y_base + i*PROP_HEIGHT*scale;
		s_servercontrols.pageList[i].menutext->string			= s_servercontrols.pageList[i].title;
		s_servercontrols.pageList[i].menutext->style			= style;
		s_servercontrols.pageList[i].menutext->color			= color_white;

		Menu_AddItem( menuptr, s_servercontrols.pageList[i].menutext);
	}

	// register controls
	Menu_AddItem( menuptr, &s_servercontrols.gameType);
	Menu_AddItem( menuptr, &s_servercontrols.gameTypeIcon);
	Menu_AddItem( menuptr, &s_servercontrols.hostname);

	// we only need the QMF_NODEFAULTINIT flag to get these controls
	// registered. Type is added later
	for (i = 0; i < MAX_SERVER_RADIO_CONTROL; i++) {
		s_servercontrols.radio[i].control.generic.type = MTYPE_RADIOBUTTON;
		s_servercontrols.radio[i].control.generic.flags = QMF_NODEFAULTINIT|QMF_HIDDEN|QMF_INACTIVE;
		Menu_AddItem( menuptr, &s_servercontrols.radio[i].control);
	}

	for (i = 0; i < MAX_SERVER_MFIELD_CONTROL; i++) {
		s_servercontrols.field[i].control.generic.type = MTYPE_FIELD;
		s_servercontrols.field[i].control.generic.flags = QMF_NODEFAULTINIT|QMF_HIDDEN|QMF_INACTIVE;
		Menu_AddItem( menuptr, &s_servercontrols.field[i].control);
	}

	for (i = 0; i < MAX_SERVER_SPIN_CONTROL; i++) {
		s_servercontrols.spin[i].control.generic.type = MTYPE_SPINCONTROL;
		s_servercontrols.spin[i].control.generic.flags = QMF_NODEFAULTINIT|QMF_HIDDEN|QMF_INACTIVE;
		Menu_AddItem( menuptr, &s_servercontrols.spin[i].control);
	}

	Menu_AddItem( menuptr, &s_servercontrols.saveScript);
	Menu_AddItem( menuptr, &s_servercontrols.loadScript);

	// control init is done on uis.firstdraw
	UI_PushMenu( &s_servercontrols.menu );
}

