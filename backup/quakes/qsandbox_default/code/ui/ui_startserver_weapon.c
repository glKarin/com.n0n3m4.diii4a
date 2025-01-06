/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/



#include "ui_local.h"
#include "ui_startserver_q3.h"




#define ID_SERVER_GAMETYPE 300
#define ID_SERVER_HOSTNAME 301
#define ID_SERVER_SAVE 302
#define ID_SERVER_LOAD 303


// server page tabs
#define ID_SERVERTAB_HOOK 350
#define ID_SERVERTAB_GAUNTLET 351
#define ID_SERVERTAB_MACHINEGUN 352
#define ID_SERVERTAB_SHOTGUN 353
#define ID_SERVERTAB_GRENADE 354
#define ID_SERVERTAB_ROCKET 355
#define ID_SERVERTAB_PLASMA 356
#define ID_SERVERTAB_LIGHTNING 357
#define ID_SERVERTAB_RAILGUN 358
#define ID_SERVERTAB_BFG 359
#define ID_SERVERTAB_NAILGUN 360
#define ID_SERVERTAB_PROX 361
#define ID_SERVERTAB_CHAINGUN 362
#define ID_SERVERTAB_FLAMETHROWER 363
#define ID_SERVERTAB_DARKFLARE 364
#define ID_SERVER_g_ghspeed 							364
#define ID_SERVER_g_ghtimeout						365
#define ID_SERVER_g_gdelay             				366
#define ID_SERVER_g_gdamage							367
#define ID_SERVER_g_grange             				368
#define ID_SERVER_g_gknockback   					369
#define ID_SERVER_g_mgammocount    				370
#define ID_SERVER_g_mgweaponcount      			371
#define ID_SERVER_g_mgdelay							372
#define ID_SERVER_g_mgdamage 						373
#define ID_SERVER_g_mgspread 						374
#define ID_SERVER_g_mgexplode  						375
#define ID_SERVER_g_mgsdamage  					376
#define ID_SERVER_g_mgsradius  						377
#define ID_SERVER_g_mgvampire  						378
#define ID_SERVER_g_mginf            					379
#define ID_SERVER_g_mgknockback    				380
#define ID_SERVER_g_sgammocount    				381
#define ID_SERVER_g_sgweaponcount      			382
#define ID_SERVER_g_sgdelay							383
#define ID_SERVER_g_sgdamage 						384
#define ID_SERVER_g_sgspread 							385
#define ID_SERVER_g_sgexplode  						386
#define ID_SERVER_g_sgsdamage  						387
#define ID_SERVER_g_sgsradius  						388
#define ID_SERVER_g_sgcount							389
#define ID_SERVER_g_sgvampire  						390
#define ID_SERVER_g_sginf            					391
#define ID_SERVER_g_sgknockback    				392
#define ID_SERVER_g_glammocount    				393
#define ID_SERVER_g_glweaponcount      			394
#define ID_SERVER_g_gldelay							395
#define ID_SERVER_g_glspeed							396
#define ID_SERVER_g_glbounce 							397
#define ID_SERVER_g_glgravity  						398
#define ID_SERVER_g_gltimeout  						399
#define ID_SERVER_g_glsradius  						400
#define ID_SERVER_g_glsdamage  						401
#define ID_SERVER_g_gldamage 						402
#define ID_SERVER_g_glvampire  						403
#define ID_SERVER_g_glinf            					404
#define ID_SERVER_g_glbouncemodifier        		405
#define ID_SERVER_g_glknockback    				406
#define ID_SERVER_g_glhoming 							407
#define ID_SERVER_g_glguided 							408
#define ID_SERVER_g_rlammocount    				409
#define ID_SERVER_g_rlweaponcount      			410
#define ID_SERVER_g_rldelay								411
#define ID_SERVER_g_rlspeed							412
#define ID_SERVER_g_rlbounce 							413
#define ID_SERVER_g_rlgravity  							414
#define ID_SERVER_g_rltimeout  						415
#define ID_SERVER_g_rlsradius  						416
#define ID_SERVER_g_rlsdamage  						417
#define ID_SERVER_g_rldamage 						418
#define ID_SERVER_g_rlvampire  						419
#define ID_SERVER_g_rlinf            						420
#define ID_SERVER_g_rlbouncemodifier         		421
#define ID_SERVER_g_rlknockback    					422
#define ID_SERVER_g_rlhoming 							423
#define ID_SERVER_g_rlguided 							424
#define ID_SERVER_g_lgammocount    				425
#define ID_SERVER_g_lgweaponcount      			426
#define ID_SERVER_g_lgrange							427
#define ID_SERVER_g_lgdelay							428
#define ID_SERVER_g_lgdamage 						429
#define ID_SERVER_g_lgvampire  						430
#define ID_SERVER_g_lgexplode  						431
#define ID_SERVER_g_lgsdamage  						432
#define ID_SERVER_g_lgsradius  						433
#define ID_SERVER_g_lginf            					434
#define ID_SERVER_g_lgknockback    				435
#define ID_SERVER_g_rgammocount    				436
#define ID_SERVER_g_rgweaponcount      			437
#define ID_SERVER_g_rgdelay							438
#define ID_SERVER_g_rgdamage 						439
#define ID_SERVER_g_rgvampire  						440
#define ID_SERVER_g_rginf            					441
#define ID_SERVER_g_rgknockback    				442
#define ID_SERVER_g_pgammocount    				443
#define ID_SERVER_g_pgweaponcount      			444
#define ID_SERVER_g_pgdelay							445
#define ID_SERVER_g_pgspeed							446
#define ID_SERVER_g_pgbounce 						447
#define ID_SERVER_g_pggravity  						448
#define ID_SERVER_g_pgtimeout  						449
#define ID_SERVER_g_pgsradius  						450
#define ID_SERVER_g_pgsdamage  						451
#define ID_SERVER_g_pgdamage 						452
#define ID_SERVER_g_pgvampire  						453
#define ID_SERVER_g_pginf            					454
#define ID_SERVER_g_pgbouncemodifier         	455
#define ID_SERVER_g_pgknockback    				456
#define ID_SERVER_g_pghoming 						457
#define ID_SERVER_g_pgguided 							458
#define ID_SERVER_g_bfgammocount     			459
#define ID_SERVER_g_bfgweaponcount       		460
#define ID_SERVER_g_bfgdelay 							461
#define ID_SERVER_g_bfgspeed 							462
#define ID_SERVER_g_bfgbounce  						463
#define ID_SERVER_g_bfggravity   						464
#define ID_SERVER_g_bfgtimeout   					465
#define ID_SERVER_g_bfgsradius   						466
#define ID_SERVER_g_bfgsdamage   					467
#define ID_SERVER_g_bfgdamage  						468
#define ID_SERVER_g_bfgvampire   					469
#define ID_SERVER_g_bfginf             					470
#define ID_SERVER_g_bfgbouncemodifier          	471
#define ID_SERVER_g_bfgknockback     				472
#define ID_SERVER_g_bfghoming  						473
#define ID_SERVER_g_bfgguided  						474
#define ID_SERVER_g_ngammocount    				475
#define ID_SERVER_g_ngweaponcount      			476
#define ID_SERVER_g_ngdelay							477
#define ID_SERVER_g_ngspeed							478
#define ID_SERVER_g_ngbounce 						479
#define ID_SERVER_g_nggravity  						480
#define ID_SERVER_g_ngtimeout  						481
#define ID_SERVER_g_ngcount							482
#define ID_SERVER_g_ngspread 							483
#define ID_SERVER_g_ngdamage 						484
#define ID_SERVER_g_ngrandom 						485
#define ID_SERVER_g_ngvampire  						486
#define ID_SERVER_g_nginf            					487
#define ID_SERVER_g_ngbouncemodifier         	488
#define ID_SERVER_g_ngknockback    				489
#define ID_SERVER_g_nghoming 						490
#define ID_SERVER_g_ngguided 							491
#define ID_SERVER_g_plammocount    				492
#define ID_SERVER_g_plweaponcount      			493
#define ID_SERVER_g_pldelay							494
#define ID_SERVER_g_plspeed							495
#define ID_SERVER_g_plgravity  						496
#define ID_SERVER_g_pltimeout  						497
#define ID_SERVER_g_plsradius  						498
#define ID_SERVER_g_plsdamage  						499
#define ID_SERVER_g_pldamage 						500
#define ID_SERVER_g_plvampire  						501
#define ID_SERVER_g_plinf            					502
#define ID_SERVER_g_plknockback    				503
#define ID_SERVER_g_cgammocount    				504
#define ID_SERVER_g_cgweaponcount      			505
#define ID_SERVER_g_cgdelay							506
#define ID_SERVER_g_cgspread 							507
#define ID_SERVER_g_cgdamage 						508
#define ID_SERVER_g_cgvampire  						509
#define ID_SERVER_g_cginf            					510
#define ID_SERVER_g_cgknockback    				511
#define ID_SERVER_g_ftammocount    				512
#define ID_SERVER_g_ftweaponcount      			513
#define ID_SERVER_g_ftdelay								514
#define ID_SERVER_g_ftspeed							515
#define ID_SERVER_g_ftbounce 							516
#define ID_SERVER_g_ftgravity  						517
#define ID_SERVER_g_fttimeout  						518
#define ID_SERVER_g_ftsradius  						519
#define ID_SERVER_g_ftsdamage  						520
#define ID_SERVER_g_ftdamage 						521
#define ID_SERVER_g_ftvampire  						522
#define ID_SERVER_g_ftinf            					523
#define ID_SERVER_g_ftbouncemodifier         		524
#define ID_SERVER_g_ftknockback    					525
#define ID_SERVER_g_fthoming 							526
#define ID_SERVER_g_ftguided 							527
#define ID_SERVER_g_amweaponcount      			528
#define ID_SERVER_g_amdelay							529
#define ID_SERVER_g_amspeed							530
#define ID_SERVER_g_ambounce 						531
#define ID_SERVER_g_amgravity  						532
#define ID_SERVER_g_amtimeout  						533
#define ID_SERVER_g_amsradius  						534
#define ID_SERVER_g_amsdamage  					535
#define ID_SERVER_g_amdamage 						536
#define ID_SERVER_g_amvampire  						537
#define ID_SERVER_g_aminf            					538
#define ID_SERVER_g_ambouncemodifier         	539
#define ID_SERVER_g_amknockback    				540
#define ID_SERVER_g_amhoming 						541
#define ID_SERVER_g_amguided 						542


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


static const char *bounce_list[] = {
	"Off",
	"Bouncemodifier",
	"Infinitybounce",
	0
};



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

	menutext_s hook;
	menutext_s gauntlet;
	menutext_s machinegun;
	menutext_s shotgun;
	menutext_s grenade;
	menutext_s rocket;
	menutext_s plasma;
	menutext_s lightning;
	menutext_s railgun;
	menutext_s bfg;
	menutext_s nailgun;
	menutext_s prox;
	menutext_s chaingun;
	menutext_s flamethrower;
	menutext_s darkflare;

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




static servercontrols_t s_weaponcontrols;



//
// static data used by controls
//


static const char *dedicated_list2[] = {
	"No",	// SRVDED_OFF
	"LAN",	// SRVDED_LAN
	"Internet",	// SRVDED_INTERNET
	0
};




// connect controls
static controlinit_t weapv_hook[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ghspeed, ITEM_ALWAYSON, "Grappling hook speed:", &s_scriptdata.server.g_ghspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ghtimeout, ITEM_ALWAYSON, "Grappling hook timeout:", &s_scriptdata.server.g_ghtimeout, -999999999, 999999999, NULL, 9, 9, NULL }

};


// gameplay controls
static controlinit_t weapv_gauntlet[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_gdelay, ITEM_ALWAYSON, "Gauntlet delay:", &s_scriptdata.server.g_gdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_gdamage, ITEM_ALWAYSON, "Gauntlet damage:", &s_scriptdata.server.g_gdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_grange, ITEM_ALWAYSON, "Gauntlet range:", &s_scriptdata.server.g_grange, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_gknockback, ITEM_ALWAYSON, "Gauntlet knockback:", NULL, 0, 0, s_scriptdata.server.g_gknockback, 9, 9, NULL }

};



// physics
static controlinit_t weapv_machinegun[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgammocount, ITEM_ALWAYSON, "Machine gun ammocount:", &s_scriptdata.server.g_mgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgweaponcount, ITEM_ALWAYSON, "Machine gun weaponcount:", &s_scriptdata.server.g_mgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgdelay, ITEM_ALWAYSON, "Machine gun delay:", &s_scriptdata.server.g_mgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgdamage, ITEM_ALWAYSON, "Machine gun damage:", &s_scriptdata.server.g_mgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgspread, ITEM_ALWAYSON, "Machine gun spread:", &s_scriptdata.server.g_mgspread, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_mgexplode, ITEM_ALWAYSON, "Machine gun explode:", &s_scriptdata.server.g_mgexplode, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgsdamage, ITEM_ALWAYSON, "Machine gun sdamage:", &s_scriptdata.server.g_mgsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_mgsradius, ITEM_ALWAYSON, "Machine gun sradius:", &s_scriptdata.server.g_mgsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_mgvampire, ITEM_ALWAYSON, "Machine gun vampire:", &s_scriptdata.server.g_mgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_mginf, ITEM_ALWAYSON, "Machine gun inf:", &s_scriptdata.server.g_mginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_mgknockback, ITEM_ALWAYSON, "Machine gun knockback:", NULL, 0, 0, s_scriptdata.server.g_mgknockback, 9, 9, NULL }

};


// team controls
static controlinit_t weapv_shotgun[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgammocount, ITEM_ALWAYSON, "Shotgun ammocount:", &s_scriptdata.server.g_sgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgweaponcount, ITEM_ALWAYSON, "Shotgun weaponcount:", &s_scriptdata.server.g_sgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgdelay, ITEM_ALWAYSON, "Shotgun delay:", &s_scriptdata.server.g_sgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgdamage, ITEM_ALWAYSON, "Shotgun damage:", &s_scriptdata.server.g_sgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgspread, ITEM_ALWAYSON, "Shotgun spread:", &s_scriptdata.server.g_sgspread, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_sgexplode, ITEM_ALWAYSON, "Shotgun explode:", &s_scriptdata.server.g_sgexplode, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgsdamage, ITEM_ALWAYSON, "Shotgun sdamage:", &s_scriptdata.server.g_sgsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgsradius, ITEM_ALWAYSON, "Shotgun sradius:", &s_scriptdata.server.g_sgsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_sgcount, ITEM_ALWAYSON, "Shotgun count:", &s_scriptdata.server.g_sgcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_sgvampire, ITEM_ALWAYSON, "Shotgun vampire:", &s_scriptdata.server.g_sgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_sginf, ITEM_ALWAYSON, "Shotgun inf:", &s_scriptdata.server.g_sginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_sgknockback, ITEM_ALWAYSON, "Shotgun knockback:", NULL, 0, 0, s_scriptdata.server.g_sgknockback, 9, 9, NULL }

};



// admin controls
static controlinit_t weapv_grenade[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_glammocount, ITEM_ALWAYSON, "Grenade launcher ammocount:", &s_scriptdata.server.g_glammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_glweaponcount, ITEM_ALWAYSON, "Grenade launcher weaponcount:", &s_scriptdata.server.g_glweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_gldelay, ITEM_ALWAYSON, "Grenade launcher delay:", &s_scriptdata.server.g_gldelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_glspeed, ITEM_ALWAYSON, "Grenade launcher speed:", &s_scriptdata.server.g_glspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_glbounce, ITEM_ALWAYSON, "Grenade launcher bounce:", &s_scriptdata.server.g_glbounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_glgravity, ITEM_ALWAYSON, "Grenade launcher gravity:", &s_scriptdata.server.g_glgravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_gltimeout, ITEM_ALWAYSON, "Grenade launcher timeout:", &s_scriptdata.server.g_gltimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_glsradius, ITEM_ALWAYSON, "Grenade launcher sradius:", &s_scriptdata.server.g_glsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_glsdamage, ITEM_ALWAYSON, "Grenade launcher sdamage:", &s_scriptdata.server.g_glsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_gldamage, ITEM_ALWAYSON, "Grenade launcher damage:", &s_scriptdata.server.g_gldamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_glvampire, ITEM_ALWAYSON, "Grenade launcher vampire:", &s_scriptdata.server.g_glvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_glinf, ITEM_ALWAYSON, "Grenade launcher inf:", &s_scriptdata.server.g_glinf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_glbouncemodifier, ITEM_ALWAYSON, "Grenade launcher bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_glbouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_glknockback, ITEM_ALWAYSON, "Grenade launcher knockback:", NULL, 0, 0, s_scriptdata.server.g_glknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_glhoming, ITEM_ALWAYSON, "Grenade launcher homing:", &s_scriptdata.server.g_glhoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_glguided, ITEM_ALWAYSON, "Grenade launcher guided:", &s_scriptdata.server.g_glguided, -999999999, 999999999, NULL, 9, 9, NULL }

};


// password controls
static controlinit_t weapv_rocket[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rlammocount, ITEM_ALWAYSON, "Rocket launcher ammocount:", &s_scriptdata.server.g_rlammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rlweaponcount, ITEM_ALWAYSON, "Rocket launcher weaponcount:", &s_scriptdata.server.g_rlweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rldelay, ITEM_ALWAYSON, "Rocket launcher delay:", &s_scriptdata.server.g_rldelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rlspeed, ITEM_ALWAYSON, "Rocket launcher speed:", &s_scriptdata.server.g_rlspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_rlbounce, ITEM_ALWAYSON, "Rocket launcher bounce:", &s_scriptdata.server.g_rlbounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rlgravity, ITEM_ALWAYSON, "Rocket launcher gravity:", &s_scriptdata.server.g_rlgravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rltimeout, ITEM_ALWAYSON, "Rocket launcher timeout:", &s_scriptdata.server.g_rltimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rlsradius, ITEM_ALWAYSON, "Rocket launcher sradius:", &s_scriptdata.server.g_rlsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rlsdamage, ITEM_ALWAYSON, "Rocket launcher sdamage:", &s_scriptdata.server.g_rlsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rldamage, ITEM_ALWAYSON, "Rocket launcher damage:", &s_scriptdata.server.g_rldamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rlvampire, ITEM_ALWAYSON, "Rocket launcher vampire:", &s_scriptdata.server.g_rlvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rlinf, ITEM_ALWAYSON, "Rocket launcher inf:", &s_scriptdata.server.g_rlinf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_rlbouncemodifier, ITEM_ALWAYSON, "Rocket launcher bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_rlbouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_rlknockback, ITEM_ALWAYSON, "Rocket launcher knockback:", NULL, 0, 0, s_scriptdata.server.g_rlknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rlhoming, ITEM_ALWAYSON, "Rocket launcher homing:", &s_scriptdata.server.g_rlhoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rlguided, ITEM_ALWAYSON, "Rocket launcher guided:", &s_scriptdata.server.g_rlguided, -999999999, 999999999, NULL, 9, 9, NULL }

};


// misc controls
static controlinit_t weapv_plasma[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgammocount, ITEM_ALWAYSON, "Plasmagun ammocount:", &s_scriptdata.server.g_pgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgweaponcount, ITEM_ALWAYSON, "Plasmagun weaponcount:", &s_scriptdata.server.g_pgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgdelay, ITEM_ALWAYSON, "Plasmagun delay:", &s_scriptdata.server.g_pgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgspeed, ITEM_ALWAYSON, "Plasmagun speed:", &s_scriptdata.server.g_pgspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_pgbounce, ITEM_ALWAYSON, "Plasmagun bounce:", &s_scriptdata.server.g_pgbounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_pggravity, ITEM_ALWAYSON, "Plasmagun gravity:", &s_scriptdata.server.g_pggravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgtimeout, ITEM_ALWAYSON, "Plasmagun timeout:", &s_scriptdata.server.g_pgtimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgsradius, ITEM_ALWAYSON, "Plasmagun sradius:", &s_scriptdata.server.g_pgsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgsdamage, ITEM_ALWAYSON, "Plasmagun sdamage:", &s_scriptdata.server.g_pgsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pgdamage, ITEM_ALWAYSON, "Plasmagun damage:", &s_scriptdata.server.g_pgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_pgvampire, ITEM_ALWAYSON, "Plasmagun vampire:", &s_scriptdata.server.g_pgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_pginf, ITEM_ALWAYSON, "Plasmagun inf:", &s_scriptdata.server.g_pginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_pgbouncemodifier, ITEM_ALWAYSON, "Plasmagun bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_pgbouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_pgknockback, ITEM_ALWAYSON, "Plasmagun knockback:", NULL, 0, 0, s_scriptdata.server.g_pgknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_pghoming, ITEM_ALWAYSON, "Plasmagun homing:", &s_scriptdata.server.g_pghoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_pgguided, ITEM_ALWAYSON, "Plasmagun guided:", &s_scriptdata.server.g_pgguided, -999999999, 999999999, NULL, 9, 9, NULL }

};


static controlinit_t weapv_lightning[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgammocount, ITEM_ALWAYSON, "Lightning gun ammocount:", &s_scriptdata.server.g_lgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgweaponcount, ITEM_ALWAYSON, "Lightning gun weaponcount:", &s_scriptdata.server.g_lgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgrange, ITEM_ALWAYSON, "Lightning gun range:", &s_scriptdata.server.g_lgrange, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgdelay, ITEM_ALWAYSON, "Lightning gun delay:", &s_scriptdata.server.g_lgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgdamage, ITEM_ALWAYSON, "Lightning gun damage:", &s_scriptdata.server.g_lgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_lgvampire, ITEM_ALWAYSON, "Lightning gun vampire:", &s_scriptdata.server.g_lgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_lgexplode, ITEM_ALWAYSON, "Lightning gun explode:", &s_scriptdata.server.g_lgexplode, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgsdamage, ITEM_ALWAYSON, "Lightning gun sdamage:", &s_scriptdata.server.g_lgsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_lgsradius, ITEM_ALWAYSON, "Lightning gun sradius:", &s_scriptdata.server.g_lgsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_lginf, ITEM_ALWAYSON, "Lightning gun inf:", &s_scriptdata.server.g_lginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_lgknockback, ITEM_ALWAYSON, "Lightning gun knockback:", NULL, 0, 0, s_scriptdata.server.g_lgknockback, 9, 9, NULL }

};

static controlinit_t weapv_railgun[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rgammocount, ITEM_ALWAYSON, "Railgun ammocount:", &s_scriptdata.server.g_rgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rgweaponcount, ITEM_ALWAYSON, "Railgun weaponcount:", &s_scriptdata.server.g_rgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rgdelay, ITEM_ALWAYSON, "Railgun delay:", &s_scriptdata.server.g_rgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_rgdamage, ITEM_ALWAYSON, "Railgun damage:", &s_scriptdata.server.g_rgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rgvampire, ITEM_ALWAYSON, "Railgun vampire:", &s_scriptdata.server.g_rgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_rginf, ITEM_ALWAYSON, "Railgun inf:", &s_scriptdata.server.g_rginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_rgknockback, ITEM_ALWAYSON, "Railgun knockback:", NULL, 0, 0, s_scriptdata.server.g_rgknockback, 9, 9, NULL }

};

static controlinit_t weapv_bfg[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgammocount, ITEM_ALWAYSON, "BFG ammocount:", &s_scriptdata.server.g_bfgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgweaponcount, ITEM_ALWAYSON, "BFG weaponcount:", &s_scriptdata.server.g_bfgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgdelay, ITEM_ALWAYSON, "BFG delay:", &s_scriptdata.server.g_bfgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgspeed, ITEM_ALWAYSON, "BFG speed:", &s_scriptdata.server.g_bfgspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_bfgbounce, ITEM_ALWAYSON, "BFG bounce:", &s_scriptdata.server.g_bfgbounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_bfggravity, ITEM_ALWAYSON, "BFG gravity:", &s_scriptdata.server.g_bfggravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgtimeout, ITEM_ALWAYSON, "BFG timeout:", &s_scriptdata.server.g_bfgtimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgsradius, ITEM_ALWAYSON, "BFG sradius:", &s_scriptdata.server.g_bfgsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgsdamage, ITEM_ALWAYSON, "BFG sdamage:", &s_scriptdata.server.g_bfgsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_bfgdamage, ITEM_ALWAYSON, "BFG damage:", &s_scriptdata.server.g_bfgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_bfgvampire, ITEM_ALWAYSON, "BFG vampire:", &s_scriptdata.server.g_bfgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_bfginf, ITEM_ALWAYSON, "BFG inf:", &s_scriptdata.server.g_bfginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_bfgbouncemodifier, ITEM_ALWAYSON, "BFG bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_bfgbouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_bfgknockback, ITEM_ALWAYSON, "BFG knockback:", NULL, 0, 0, s_scriptdata.server.g_bfgknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_bfghoming, ITEM_ALWAYSON, "BFG homing:", &s_scriptdata.server.g_bfghoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_bfgguided, ITEM_ALWAYSON, "BFG guided:", &s_scriptdata.server.g_bfgguided, -999999999, 999999999, NULL, 9, 9, NULL }

};

static controlinit_t weapv_nailgun[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngammocount, ITEM_ALWAYSON, "Nailgun ammocount:", &s_scriptdata.server.g_ngammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngweaponcount, ITEM_ALWAYSON, "Nailgun weaponcount:", &s_scriptdata.server.g_ngweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngdelay, ITEM_ALWAYSON, "Nailgun delay:", &s_scriptdata.server.g_ngdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngspeed, ITEM_ALWAYSON, "Nailgun speed:", &s_scriptdata.server.g_ngspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_ngbounce, ITEM_ALWAYSON, "Nailgun bounce:", &s_scriptdata.server.g_ngbounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_nggravity, ITEM_ALWAYSON, "Nailgun gravity:", &s_scriptdata.server.g_nggravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngtimeout, ITEM_ALWAYSON, "Nailgun timeout:", &s_scriptdata.server.g_ngtimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngcount, ITEM_ALWAYSON, "Nailgun count:", &s_scriptdata.server.g_ngcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngspread, ITEM_ALWAYSON, "Nailgun spread:", &s_scriptdata.server.g_ngspread, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngdamage, ITEM_ALWAYSON, "Nailgun damage:", &s_scriptdata.server.g_ngdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ngrandom, ITEM_ALWAYSON, "Nailgun random:", &s_scriptdata.server.g_ngrandom, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_ngvampire, ITEM_ALWAYSON, "Nailgun vampire:", &s_scriptdata.server.g_ngvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_nginf, ITEM_ALWAYSON, "Nailgun inf:", &s_scriptdata.server.g_nginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_ngbouncemodifier, ITEM_ALWAYSON, "Nailgun bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_ngbouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_ngknockback, ITEM_ALWAYSON, "Nailgun knockback:", NULL, 0, 0, s_scriptdata.server.g_ngknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_nghoming, ITEM_ALWAYSON, "Nailgun homing:", &s_scriptdata.server.g_nghoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_ngguided, ITEM_ALWAYSON, "Nailgun guided:", &s_scriptdata.server.g_ngguided, -999999999, 999999999, NULL, 9, 9, NULL }

};

static controlinit_t weapv_prox[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_plammocount, ITEM_ALWAYSON, "Prox launcher ammocount:", &s_scriptdata.server.g_plammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_plweaponcount, ITEM_ALWAYSON, "Prox launcher weaponcount:", &s_scriptdata.server.g_plweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pldelay, ITEM_ALWAYSON, "Prox launcher delay:", &s_scriptdata.server.g_pldelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_plspeed, ITEM_ALWAYSON, "Prox launcher speed:", &s_scriptdata.server.g_plspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_plgravity, ITEM_ALWAYSON, "Prox launcher gravity:", &s_scriptdata.server.g_plgravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pltimeout, ITEM_ALWAYSON, "Prox launcher timeout:", &s_scriptdata.server.g_pltimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_plsradius, ITEM_ALWAYSON, "Prox launcher sradius:", &s_scriptdata.server.g_plsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_plsdamage, ITEM_ALWAYSON, "Prox launcher sdamage:", &s_scriptdata.server.g_plsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_pldamage, ITEM_ALWAYSON, "Prox launcher damage:", &s_scriptdata.server.g_pldamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_plvampire, ITEM_ALWAYSON, "Prox launcher vampire:", &s_scriptdata.server.g_plvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_plinf, ITEM_ALWAYSON, "Prox launcher inf:", &s_scriptdata.server.g_plinf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_plknockback, ITEM_ALWAYSON, "Prox launcher knockback:", NULL, 0, 0, s_scriptdata.server.g_plknockback, 9, 9, NULL }

};

static controlinit_t weapv_chaingun[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_cgammocount, ITEM_ALWAYSON, "Chaingun ammocount:", &s_scriptdata.server.g_cgammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_cgweaponcount, ITEM_ALWAYSON, "Chaingun weaponcount:", &s_scriptdata.server.g_cgweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_cgdelay, ITEM_ALWAYSON, "Chaingun delay:", &s_scriptdata.server.g_cgdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_cgspread, ITEM_ALWAYSON, "Chaingun spread:", &s_scriptdata.server.g_cgspread, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_cgdamage, ITEM_ALWAYSON, "Chaingun damage:", &s_scriptdata.server.g_cgdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_cgvampire, ITEM_ALWAYSON, "Chaingun vampire:", &s_scriptdata.server.g_cgvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_cginf, ITEM_ALWAYSON, "Chaingun inf:", &s_scriptdata.server.g_cginf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_cgknockback, ITEM_ALWAYSON, "Chaingun knockback:", NULL, 0, 0, s_scriptdata.server.g_cgknockback, 9, 9, NULL }

};

static controlinit_t weapv_flamethrower[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftammocount, ITEM_ALWAYSON, "Flamethrower ammocount:", &s_scriptdata.server.g_ftammocount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftweaponcount, ITEM_ALWAYSON, "Flamethrower weaponcount:", &s_scriptdata.server.g_ftweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftdelay, ITEM_ALWAYSON, "Flamethrower delay:", &s_scriptdata.server.g_ftdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftspeed, ITEM_ALWAYSON, "Flamethrower speed:", &s_scriptdata.server.g_ftspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_ftbounce, ITEM_ALWAYSON, "Flamethrower bounce:", &s_scriptdata.server.g_ftbounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_ftgravity, ITEM_ALWAYSON, "Flamethrower gravity:", &s_scriptdata.server.g_ftgravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_fttimeout, ITEM_ALWAYSON, "Flamethrower timeout:", &s_scriptdata.server.g_fttimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftsradius, ITEM_ALWAYSON, "Flamethrower sradius:", &s_scriptdata.server.g_ftsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftsdamage, ITEM_ALWAYSON, "Flamethrower sdamage:", &s_scriptdata.server.g_ftsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_ftdamage, ITEM_ALWAYSON, "Flamethrower damage:", &s_scriptdata.server.g_ftdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_ftvampire, ITEM_ALWAYSON, "Flamethrower vampire:", &s_scriptdata.server.g_ftvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_ftinf, ITEM_ALWAYSON, "Flamethrower inf:", &s_scriptdata.server.g_ftinf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_ftbouncemodifier, ITEM_ALWAYSON, "Flamethrower bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_ftbouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_ftknockback, ITEM_ALWAYSON, "Flamethrower knockback:", NULL, 0, 0, s_scriptdata.server.g_ftknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_fthoming, ITEM_ALWAYSON, "Flamethrower homing:", &s_scriptdata.server.g_fthoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_ftguided, ITEM_ALWAYSON, "Flamethrower guided:", &s_scriptdata.server.g_ftguided, -999999999, 999999999, NULL, 9, 9, NULL }

};

static controlinit_t weapv_darkflare[] = {

{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amweaponcount, ITEM_ALWAYSON, "Dark flare weaponcount:", &s_scriptdata.server.g_amweaponcount, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amdelay, ITEM_ALWAYSON, "Dark flare delay:", &s_scriptdata.server.g_amdelay, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amspeed, ITEM_ALWAYSON, "Dark flare speed:", &s_scriptdata.server.g_amspeed, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_SPIN, 0, ID_SERVER_g_ambounce, ITEM_ALWAYSON, "Dark flare bounce:", &s_scriptdata.server.g_ambounce, -999999999, 999999999, NULL, 9, 9, bounce_list },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_amgravity, ITEM_ALWAYSON, "Dark flare gravity:", &s_scriptdata.server.g_amgravity, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amtimeout, ITEM_ALWAYSON, "Dark flare timeout:", &s_scriptdata.server.g_amtimeout, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amsradius, ITEM_ALWAYSON, "Dark flare sradius:", &s_scriptdata.server.g_amsradius, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amsdamage, ITEM_ALWAYSON, "Dark flare sdamage:", &s_scriptdata.server.g_amsdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_NUMFIELD, 0, ID_SERVER_g_amdamage, ITEM_ALWAYSON, "Dark flare damage:", &s_scriptdata.server.g_amdamage, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_amvampire, ITEM_ALWAYSON, "Dark flare vampire:", &s_scriptdata.server.g_amvampire, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_aminf, ITEM_ALWAYSON, "Dark flare inf:", &s_scriptdata.server.g_aminf, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_ambouncemodifier, ITEM_ALWAYSON, "Dark flare bouncemodifier:", NULL, 0, 0, s_scriptdata.server.g_ambouncemodifier, 9, 9, NULL },
{ SRVCTRL_TEXTFIELD, 0, ID_SERVER_g_amknockback, ITEM_ALWAYSON, "Dark flare knockback:", NULL, 0, 0, s_scriptdata.server.g_amknockback, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_amhoming, ITEM_ALWAYSON, "Dark flare homing:", &s_scriptdata.server.g_amhoming, -999999999, 999999999, NULL, 9, 9, NULL },
{ SRVCTRL_RADIO, 0, ID_SERVER_g_amguided, ITEM_ALWAYSON, "Dark flare guided:", &s_scriptdata.server.g_amguided, -999999999, 999999999, NULL, 9, 9, NULL }

};




// list of controls used for a multiplayer/dedicated game
static initlist_t weapv_multiplayerserver[] =
{
	{ ID_SERVERTAB_HOOK, "HOOK", &s_weaponcontrols.hook, weapv_hook, ARRAY_COUNT(weapv_hook) },
	{ ID_SERVERTAB_GAUNTLET, "GAUNTLET", &s_weaponcontrols.gauntlet, weapv_gauntlet, ARRAY_COUNT(weapv_gauntlet) },
	{ ID_SERVERTAB_MACHINEGUN, "MACHINEGUN", &s_weaponcontrols.machinegun, weapv_machinegun, ARRAY_COUNT(weapv_machinegun) },
	{ ID_SERVERTAB_SHOTGUN, "SHOTGUN", &s_weaponcontrols.shotgun, weapv_shotgun, ARRAY_COUNT(weapv_shotgun) },
	{ ID_SERVERTAB_GRENADE, "GRENADE", &s_weaponcontrols.grenade, weapv_grenade, ARRAY_COUNT(weapv_grenade) },
	{ ID_SERVERTAB_ROCKET, "ROCKET", &s_weaponcontrols.rocket, weapv_rocket, ARRAY_COUNT(weapv_rocket) },
	{ ID_SERVERTAB_PLASMA, "PLASMA", &s_weaponcontrols.plasma, weapv_plasma, ARRAY_COUNT(weapv_plasma) },
	{ ID_SERVERTAB_LIGHTNING, "LIGHTNING", &s_weaponcontrols.lightning, weapv_lightning, ARRAY_COUNT(weapv_lightning) },
	{ ID_SERVERTAB_RAILGUN, "RAILGUN", &s_weaponcontrols.railgun, weapv_railgun, ARRAY_COUNT(weapv_railgun) },
	{ ID_SERVERTAB_BFG, "BFG", &s_weaponcontrols.bfg, weapv_bfg, ARRAY_COUNT(weapv_bfg) },
	{ ID_SERVERTAB_NAILGUN, "NAILGUN", &s_weaponcontrols.nailgun, weapv_nailgun, ARRAY_COUNT(weapv_nailgun) },
	{ ID_SERVERTAB_PROX, "PROX", &s_weaponcontrols.prox, weapv_prox, ARRAY_COUNT(weapv_prox) },
	{ ID_SERVERTAB_CHAINGUN, "CHAINGUN", &s_weaponcontrols.chaingun, weapv_chaingun, ARRAY_COUNT(weapv_chaingun) },
	{ ID_SERVERTAB_FLAMETHROWER, "FLAMETHROWER", &s_weaponcontrols.flamethrower, weapv_flamethrower, ARRAY_COUNT(weapv_flamethrower) },
	{ ID_SERVERTAB_DARKFLARE, "DARKFLARE", &s_weaponcontrols.darkflare, weapv_darkflare, ARRAY_COUNT(weapv_darkflare) }
};





/*
=================
StartServer_WeaponPage_SetControlState
=================
*/
static void StartServer_WeaponPage_SetControlState(menucommon_s* c,int type)
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
StartServer_WeaponPage_SetControlFromId
=================
*/
static void StartServer_WeaponPage_SetControlFromId(int id, int type)
{
	int i;

	// locate control
	for (i = 0; i < s_weaponcontrols.num_radio; i++) {
		if (s_weaponcontrols.radio[i].init->id == id)
		{
			StartServer_WeaponPage_SetControlState(&s_weaponcontrols.radio[i].control.generic, type);
			return;
		}
	}

	for (i = 0; i < s_weaponcontrols.num_field; i++)
	{
		if (s_weaponcontrols.field[i].init->id == id) {
			StartServer_WeaponPage_SetControlState(&s_weaponcontrols.field[i].control.generic, type);
			return;
		}
	}

	for (i = 0; i < s_weaponcontrols.num_spin; i++)
	{
		if (s_weaponcontrols.spin[i].init->id == id) {
			StartServer_WeaponPage_SetControlState(&s_weaponcontrols.spin[i].control.generic, type);
			return;
		}
	}
}



/*
=================
StartServer_WeaponPage_UpdateInterface
=================
*/
static void StartServer_WeaponPage_UpdateInterface(void)
{
	int i;
	initlist_t* group;
	qboolean lastSet;

	StartServer_SetIconFromGameType(&s_weaponcontrols.gameTypeIcon, s_scriptdata.gametype, qfalse);

	// clear visible controls
	for (i = 0; i < s_weaponcontrols.num_radio; i++) {
		StartServer_WeaponPage_SetControlState(&s_weaponcontrols.radio[i].control.generic, 0);
	}

	for (i = 0; i < s_weaponcontrols.num_field; i++) {
		StartServer_WeaponPage_SetControlState(&s_weaponcontrols.field[i].control.generic, 0);
	}

	for (i = 0; i < s_weaponcontrols.num_spin; i++) {
		StartServer_WeaponPage_SetControlState(&s_weaponcontrols.spin[i].control.generic, 0);
	}


	// walk item list, setting states as appropriate
	group = &s_weaponcontrols.pageList[s_weaponcontrols.activePage];
	lastSet = qfalse;
	for (i = 0; i < group->count; i++) {
		// hide/show control based on previous set value
		// if item is grayed then we avoid changing the lastSet flag by
		// continuing. This prevents a grayed *radio* control from affecting
		// a group of grayed controls
		if ((group->list[i].flags & ITEM_GRAYIF_PREVON) && lastSet) {
			StartServer_WeaponPage_SetControlFromId(group->list[i].id, QMF_GRAYED);
			continue;
		}

		if ((group->list[i].flags & ITEM_GRAYIF_PREVOFF) && !lastSet) {
			StartServer_WeaponPage_SetControlFromId(group->list[i].id, QMF_GRAYED);
			continue;
		}

		// update status of "most recent" spin control
		if (group->list[i].type == SRVCTRL_RADIO) {
			if (s_weaponcontrols.radio[ group->list[i].offset ].control.curvalue)
				lastSet = qtrue;
			else
				lastSet = qfalse;
		}
	}

	// enable fight button if possible
	if (!StartServer_CheckFightReady(&s_weaponcontrols.common)) {
		s_weaponcontrols.saveScript.generic.flags |= QMF_GRAYED;
	}
	else {
		s_weaponcontrols.saveScript.generic.flags &= ~QMF_GRAYED;
	}
}



/*
=================
StartServer_WeaponPage_Cache
=================
*/
void StartServer_WeaponPage_Cache( void )
{
	trap_R_RegisterShaderNoMip( SERVER_SAVE0 );
	trap_R_RegisterShaderNoMip( SERVER_SAVE1 );
}





/*
=================
StartServer_WeaponPage_Save
=================
*/
static void StartServer_WeaponPage_Save( void )
{
	StartServer_SaveScriptData();
}






/*
=================
StartServer_WeaponPage_CheckMinMaxFail
=================
*/
static qboolean StartServer_WeaponPage_CheckMinMaxFail( controlinit_t* c )
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
StartServer_WeaponPage_SpinEvent
=================
*/
static void StartServer_WeaponPage_SpinEvent( void* ptr, int event )
{
	int id;
	spincontrol_t* s;

	if (event != QM_ACTIVATED)
		return;

	id = ((menucommon_s*)ptr)->id;

	s = &s_weaponcontrols.spin[id];

	*(s->init->number) = s_weaponcontrols.spin[id].control.curvalue;
	StartServer_WeaponPage_UpdateInterface();
}




/*
=================
StartServer_WeaponPage_RadioEvent
=================
*/
static void StartServer_WeaponPage_RadioEvent( void* ptr, int event )
{
	int id;
	radiobuttoncontrol_t* r;

	if (event != QM_ACTIVATED)
		return;

	id = ((menucommon_s*)ptr)->id;

	r = &s_weaponcontrols.radio[id];

	*(r->init->number) = s_weaponcontrols.radio[id].control.curvalue;

	StartServer_WeaponPage_UpdateInterface();
}



/*
=================
StartServer_WeaponPage_FieldEvent
=================
*/
static void StartServer_WeaponPage_FieldEvent( void* ptr, int event )
{
	int id;
	textfieldcontrol_t* r;
	controlinit_t* c;

	if (event != QM_LOSTFOCUS)
		return;

	id = ((menucommon_s*)ptr)->id;

	r = &s_weaponcontrols.field[id];
	c = r->init;

	if (c->type == SRVCTRL_NUMFIELD)
	{
		*(c->number) = atoi(r->control.field.buffer);
		if (StartServer_WeaponPage_CheckMinMaxFail(c)) {
			Q_strncpyz(r->control.field.buffer, va("%i", *(c->number)), c->arraysize);
		}
	}
	else if (c->type == SRVCTRL_TEXTFIELD)
	{
		Q_strncpyz(c->array, r->control.field.buffer, c->arraysize);
	}

	StartServer_WeaponPage_UpdateInterface();
}




/*
=================
StartServer_WeaponPage_SetStatusBar
=================
*/
static void StartServer_WeaponPage_SetStatusBar( const char* text )
{
	s_weaponcontrols.savetime = uis.realtime + STATUSBAR_FADETIME;
	if (text)
		Q_strncpyz(s_weaponcontrols.statusbar_message, text, MAX_STATUSBAR_TEXT);
}



/*
=================
StartServer_WeaponPage_ControlListStatusBar
=================
*/
static void StartServer_WeaponPage_ControlListStatusBar(void* ptr)
{
	menucommon_s* m;
	controlinit_t* c;
	int i;

	m = (menucommon_s*)ptr;

	switch (m->type) {
	case MTYPE_RADIOBUTTON:
		c = s_weaponcontrols.radio[m->id].init;
		break;

	case MTYPE_SPINCONTROL:
		c = s_weaponcontrols.spin[m->id].init;
		break;

	case MTYPE_FIELD:
		c = s_weaponcontrols.field[m->id].init;
		break;

	default:
		Com_Printf("Status bar: unsupported control type (%i)\n", m->id);
		return;
	}


	/*switch (c->id) {
	case ID_SERVER_CONFIGBUG:
		StartServer_WeaponPage_SetStatusBar("fixes change of time and frag limits (pre 1.30 only)");
		return;

	case ID_SERVER_PMOVEMSEC:
		StartServer_WeaponPage_SetStatusBar("pmove step time, minimum 8 (recommended)");
		return;

	case ID_SERVER_PMOVEFIXED:
		StartServer_WeaponPage_SetStatusBar("on = same movement physics for all players");
		return;

	case ID_SERVER_RESPAWN:
		StartServer_WeaponPage_SetStatusBar("forces respawn of dead player");
		return;

	case ID_SERVER_ALLOWMAXRATE:
		StartServer_WeaponPage_SetStatusBar("limit data sent to a single client per second");
		return;

	case ID_SERVER_MAXRATE:
		StartServer_WeaponPage_SetStatusBar("8000 or 10000 are typical, 0 = no limit");
		return;

	case ID_SERVER_DEDICATED:
		StartServer_WeaponPage_SetStatusBar("LAN = local game, Internet = announced to master server");
		return;

	case ID_SERVER_SMOOTHCLIENTS:
		StartServer_WeaponPage_SetStatusBar("on = server allows a client to predict other player movement");
		return;

	case ID_SERVER_SYNCCLIENTS:
		StartServer_WeaponPage_SetStatusBar("on = allows client to record demos, may affect performance");
		return;

	case ID_SERVER_GRAVITY:
		StartServer_WeaponPage_SetStatusBar("affects all maps, default = 800");
		return;
		
	case ID_SERVER_JUMPHEIGHT:
		StartServer_WeaponPage_SetStatusBar("affects all maps, default = 270");
		return;

	case ID_SERVER_KNOCKBACK:
		StartServer_WeaponPage_SetStatusBar("kickback from damage, default = 1000");
		return;

	case ID_SERVER_QUADFACTOR:
		StartServer_WeaponPage_SetStatusBar("additional damage caused by quad, default = 3");
		return;

	case ID_SERVER_NETPORT:
		StartServer_WeaponPage_SetStatusBar("server listens on this port for client connections");
		return;

	case ID_SERVER_SVFPS:
		StartServer_WeaponPage_SetStatusBar("server framerate, default = 20");
		return;

	case ID_SERVER_ALLOWPRIVATECLIENT:
		StartServer_WeaponPage_SetStatusBar("reserve some player slots - password required");
		return;

	case ID_SERVER_PRIVATECLIENT:
		StartServer_WeaponPage_SetStatusBar("number of reserved slots, reduces max clients");
		return;

	case ID_SERVER_WEAPONRESPAWN:
		StartServer_WeaponPage_SetStatusBar("time before weapon respawns, default = 5, TeamDM = 30");
		return;

	case ID_SERVER_ALLOWPASS:
		StartServer_WeaponPage_SetStatusBar("all clients must use a password to connect");
		return;

	case ID_SERVER_ALLOWMINPING:
	case ID_SERVER_ALLOWMAXPING:
	case ID_SERVER_MINPING:
	case ID_SERVER_MAXPING:
		StartServer_WeaponPage_SetStatusBar("Client ping limit, tested on first connection");
		return;
	}*/
}






/*
=================
StartServer_WeaponPage_InitSpinCtrl
=================
*/
static void StartServer_WeaponPage_InitSpinCtrl(controlinit_t* c, int height)
{
	spincontrol_t* s;

	if (s_weaponcontrols.num_spin == MAX_SERVER_SPIN_CONTROL) {
		Com_Printf("ServerPage: Max spin controls (%i) reached\n", s_weaponcontrols.num_spin);
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

	s = &s_weaponcontrols.spin[s_weaponcontrols.num_spin];

	s->init = c;

	s->control.generic.type		= MTYPE_SPINCONTROL;
	s->control.generic.name		= c->title;
	s->control.generic.id		= s_weaponcontrols.num_spin;	// self reference
	s->control.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_RIGHT_JUSTIFY;
	s->control.generic.callback	= StartServer_WeaponPage_SpinEvent;
	s->control.generic.statusbar	= StartServer_WeaponPage_ControlListStatusBar;
	s->control.generic.x			= CONTROL_POSX;
	s->control.generic.y			= height;
	s->control.itemnames 			= c->itemnames;

	s->control.curvalue = *(c->number);

	// item is registered but we have to do the init here
	SpinControl_Init(&s->control);

	c->offset = s_weaponcontrols.num_spin++;
}




/*
=================
StartServer_WeaponPage_InitRadioCtrl
=================
*/
static void StartServer_WeaponPage_InitRadioCtrl(controlinit_t* c, int height)
{
	radiobuttoncontrol_t* r;

	if (s_weaponcontrols.num_radio == MAX_SERVER_RADIO_CONTROL) {
		Com_Printf("ServerPage: Max radio controls (%i) reached\n", s_weaponcontrols.num_radio);
		return;
	}

	if (!c->number) {
		Com_Printf("ServerPage: Radio control (id:%i) has no data\n", c->id);
		return;
	}

	r = &s_weaponcontrols.radio[s_weaponcontrols.num_radio];

	r->init = c;

	r->control.generic.type		= MTYPE_RADIOBUTTON;
	r->control.generic.name		= c->title;
	r->control.generic.id		= s_weaponcontrols.num_radio;	// self reference
	r->control.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_RIGHT_JUSTIFY;
	r->control.generic.callback	= StartServer_WeaponPage_RadioEvent;
	r->control.generic.statusbar	= StartServer_WeaponPage_ControlListStatusBar;
	r->control.generic.x			= CONTROL_POSX;
	r->control.generic.y			= height;

	r->control.curvalue = *(c->number);

	c->offset = s_weaponcontrols.num_radio++;
}



/*
=================
StartServer_WeaponPage_InitFieldCtrl
=================
*/
static void StartServer_WeaponPage_InitFieldCtrl(controlinit_t* c, int height)
{
	textfieldcontrol_t* f;
	int flags;

	if (s_weaponcontrols.num_field == MAX_SERVER_MFIELD_CONTROL) {
		Com_Printf("ServerPage: Max field controls (%i) reached\n", s_weaponcontrols.num_field);
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

	f = &s_weaponcontrols.field[s_weaponcontrols.num_field];

	f->init = c;

	flags = QMF_SMALLFONT|QMF_PULSEIFFOCUS|QMF_RIGHT_JUSTIFY;
	if (c->type == SRVCTRL_NUMFIELD)
		flags |= QMF_NUMBERSONLY;

	f->control.generic.type       = MTYPE_FIELD;
	f->control.generic.name       = c->title;
	f->control.generic.id		= s_weaponcontrols.num_field;	// self reference
	f->control.generic.callback	= StartServer_WeaponPage_FieldEvent;
	f->control.generic.statusbar	= StartServer_WeaponPage_ControlListStatusBar;
	f->control.generic.flags      = flags;
	f->control.generic.x			= CONTROL_POSX;
	f->control.generic.y			= height;

	f->control.field.widthInChars = c->displaysize;
	f->control.field.maxchars     = c->arraysize;

	// setting of field is deferred until after init

	c->offset = s_weaponcontrols.num_field++;
}




/*
=================
StartServer_WeaponPage_InitControlList
=================
*/
static void StartServer_WeaponPage_InitControlList(initlist_t* il)
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
			StartServer_WeaponPage_InitSpinCtrl(&list[i], offset);
			break;
		case SRVCTRL_RADIO:
			StartServer_WeaponPage_InitRadioCtrl(&list[i], offset);
			break;
		case SRVCTRL_TEXTFIELD:
		case SRVCTRL_NUMFIELD:
			StartServer_WeaponPage_InitFieldCtrl(&list[i], offset);
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
	for (i = 0; i < s_weaponcontrols.num_radio; i++) {
		s_weaponcontrols.radio[i].control.generic.y += offset;

		// item is registered but we have to do the init here
		RadioButton_Init(&s_weaponcontrols.radio[i].control);
	}

	for (i = 0; i < s_weaponcontrols.num_field; i++) {
		c = s_weaponcontrols.field[i].init;
		f = &s_weaponcontrols.field[i];

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

	for (i = 0; i < s_weaponcontrols.num_spin; i++) {
		s_weaponcontrols.spin[i].control.generic.y += offset;

		// item is registered but we have to do the init here
		SpinControl_Init(&s_weaponcontrols.spin[i].control);
	}
}




/*
=================
StartServer_WeaponPage_SetTab
=================
*/
static void StartServer_WeaponPage_SetTab( int tab )
{
	int i, index;
	initlist_t* controlList;

	if (tab == -1) {
		index = s_weaponcontrols.activePage;
	}
	else {
		index = -1;
		for (i = 0; i < s_weaponcontrols.pageListSize; i++) {
			if (s_weaponcontrols.pageList[i].menutext->generic.id == tab) {
				index = i;
				break;
			}
		}
	}

	// walk all controls and mark them hidden
	for (i = 0; i < s_weaponcontrols.num_radio; i++) {
		StartServer_WeaponPage_SetControlState(&s_weaponcontrols.radio[i].control.generic, QMF_HIDDEN);
	}

	for (i = 0; i < s_weaponcontrols.num_field; i++) {
		StartServer_WeaponPage_SetControlState(&s_weaponcontrols.field[i].control.generic, QMF_HIDDEN);
	}

	for (i = 0; i < s_weaponcontrols.num_spin; i++) {
		StartServer_WeaponPage_SetControlState(&s_weaponcontrols.spin[i].control.generic, QMF_HIDDEN);
	}

	if (index == -1) {
		Com_Printf("Server tab: unknown index (%i)\n", tab);
		return;
	}

	// set the controls for the current tab
	s_weaponcontrols.activePage = index;
	s_weaponcontrols.num_radio = 0;
	s_weaponcontrols.num_field = 0;
	s_weaponcontrols.num_spin = 0;

	StartServer_WeaponPage_InitControlList(&s_weaponcontrols.pageList[index]);

	// brighten active tab control, remove pulse effect
	for (i = 0; i < s_weaponcontrols.pageListSize; i++) {
		s_weaponcontrols.pageList[i].menutext->generic.flags &= ~(QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		s_weaponcontrols.pageList[i].menutext->generic.flags |= QMF_PULSEIFFOCUS;
	}

	s_weaponcontrols.pageList[index].menutext->generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_weaponcontrols.pageList[index].menutext->generic.flags &= ~(QMF_PULSEIFFOCUS);
}




/*
=================
StartServer_WeaponPage_InitControlsFromScript
=================
*/
static void StartServer_WeaponPage_InitControlsFromScript( int tab )
{
	int i;
	radiobuttoncontrol_t* r;
	textfieldcontrol_t* t;
	spincontrol_t* s;

	StartServer_WeaponPage_SetTab(tab);

	// set hostname
	Q_strncpyz(s_weaponcontrols.hostname.field.buffer, s_scriptdata.server.hostname, MAX_HOSTNAME_LENGTH);
}




/*
=================
StartServer_WeaponPage_Load
=================
*/
static void StartServer_WeaponPage_Load( void )
{
	s_weaponcontrols.gameType.curvalue = s_scriptdata.gametype;

	StartServer_WeaponPage_InitControlsFromScript(-1);
}





/*
=================
StartServer_WeaponPage_CommonEvent
=================
*/
static void StartServer_WeaponPage_CommonEvent( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}

	StartServer_WeaponPage_Save();
	switch( ((menucommon_s*)ptr)->id )
	{
		case ID_SERVERCOMMON_ITEMS:
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_SERVER:
			UI_PopMenu();
			StartServer_ServerPage_MenuInit();
			break;

		case ID_SERVERCOMMON_BOTS:
			UI_PopMenu();
			UI_PopMenu();
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
StartServer_WeaponPage_DrawStatusBarText
=================
*/
static void StartServer_WeaponPage_DrawStatusBarText( void )
{
	vec4_t color;
	int fade;
	float fadecol;

	if (uis.realtime > s_weaponcontrols.savetime)
		return;

	if (!s_weaponcontrols.statusbar_message[0])
		return;

	fade = s_weaponcontrols.savetime - uis.realtime;

	fadecol = (float)fade/STATUSBAR_FADETIME;

	color[0] = 1.0;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = fadecol;

	UI_DrawString(320, s_weaponcontrols.statusbar_height,
		s_weaponcontrols.statusbar_message, UI_CENTER|UI_SMALLFONT, color);
}



/*
=================
StartServer_WeaponPage_MenuKey
=================
*/
static sfxHandle_t StartServer_WeaponPage_MenuKey( int key )
{
	switch (key)
	{
		case K_MOUSE2:
		case K_ESCAPE:
			StartServer_WeaponPage_Save();
			UI_PopMenu();
			UI_PopMenu();
			UI_PopMenu();
			break;
	}

	return ( Menu_DefaultKey( &s_weaponcontrols.menu, key ) );
}





/*
=================
StartServer_WeaponPage_SaveScript
=================
*/
static qboolean StartServer_WeaponPage_SaveScript( const char* filename )
{
	UI_PopMenu();
	if (StartServer_CreateServer( filename )) {
		StartServer_WeaponPage_SetStatusBar("Saved!");
		return qtrue;
	}
	else {
		StartServer_WeaponPage_SetStatusBar(StartServer_GetLastScriptError());
		return qfalse;
	}
}


/*
=================
StartServer_WeaponPage_LoadScript
=================
*/
static qboolean StartServer_WeaponPage_LoadScript( const char* filename )
{
	UI_PopMenu();

	if (StartServer_LoadFromConfig(filename))
	{
		// so far we've only loaded the cvars, now we need
		// to refresh the stored data
		StartServer_LoadScriptDataFromType(s_scriptdata.gametype);

		StartServer_WeaponPage_Load();
		StartServer_WeaponPage_UpdateInterface();

		StartServer_WeaponPage_SetStatusBar("Loaded!");
		
		MainMenu_ReloadGame();
	}
	return qtrue;
}





/*
=================
StartServer_WeaponPage_Event
=================
*/
static void StartServer_WeaponPage_Event( void* ptr, int event )
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
			StartServer_LoadScriptDataFromType(s_weaponcontrols.gameType.curvalue);

			StartServer_WeaponPage_InitControlsFromScript(-1);	// gametype is already accurate
			StartServer_WeaponPage_UpdateInterface();
			break;

		case ID_SERVER_HOSTNAME:
			if (event != QM_LOSTFOCUS)
				return;

			// store hostname
			Q_strncpyz(s_scriptdata.server.hostname, s_weaponcontrols.hostname.field.buffer, MAX_HOSTNAME_LENGTH);
			break;

		case ID_SERVER_SAVE:
			if( event != QM_ACTIVATED ) {
				return;
			}
			StartServer_WeaponPage_Save();
			if(cl_language.integer == 0){
			UI_ConfigMenu("SAVE SCRIPT", qfalse, StartServer_WeaponPage_SaveScript);
			}
			if(cl_language.integer == 1){
			UI_ConfigMenu("СОХРАНИТЬ СКРИПТ", qfalse, StartServer_WeaponPage_SaveScript);
			}
			break;

		case ID_SERVER_LOAD:
			if( event != QM_ACTIVATED ) {
				return;
			}
			StartServer_WeaponPage_Save();
			if(cl_language.integer == 0){
			UI_ConfigMenu("LOAD SCRIPT", qtrue, StartServer_WeaponPage_LoadScript);
			}
			if(cl_language.integer == 1){
			UI_ConfigMenu("ЗАГРУЗИТЬ СКРИПТ", qtrue, StartServer_WeaponPage_LoadScript);
			}
			break;

		case ID_SERVERTAB_HOOK:
		case ID_SERVERTAB_GAUNTLET:
		case ID_SERVERTAB_MACHINEGUN:
		case ID_SERVERTAB_SHOTGUN:
		case ID_SERVERTAB_GRENADE:
		case ID_SERVERTAB_ROCKET:
			if( event != QM_ACTIVATED ) {
				return;
			}
			StartServer_WeaponPage_SetTab(id);
			StartServer_WeaponPage_UpdateInterface();
			break;
	}
}


/*
=================
StartServer_WeaponPage_TabEvent
=================
*/
static void StartServer_WeaponPage_TabEvent( void* ptr, int event )
{
	int id;

	if( event != QM_ACTIVATED ) {
		return;
	}

	id = ((menucommon_s*)ptr)->id;
	StartServer_WeaponPage_SetTab(id);
	StartServer_WeaponPage_UpdateInterface();
}





/*
=================
StartServer_WeaponPage_MenuDraw
=================
*/
static void StartServer_WeaponPage_MenuDraw(void)
{
	if (uis.firstdraw) {
		StartServer_WeaponPage_Load();
		StartServer_WeaponPage_UpdateInterface();
	}

	StartServer_BackgroundDraw(qfalse);

	// draw the controls
	Menu_Draw(&s_weaponcontrols.menu);

	// if we have no server controls, say so
	if (s_weaponcontrols.num_spin == 0 && s_weaponcontrols.num_radio == 0 &&
		s_weaponcontrols.num_field == 0)
	{
		UI_DrawString(CONTROL_POSX, TABCONTROLCENTER_Y - LINE_HEIGHT/2,
			"<no controls>", UI_RIGHT|UI_SMALLFONT, text_color_disabled);
	}

	StartServer_WeaponPage_DrawStatusBarText();
}




/*
=================
StartServer_WeaponPage_StatusBar
=================
*/
static void StartServer_WeaponPage_StatusBar(void* ptr)
{
	switch (((menucommon_s*)ptr)->id) {
	case ID_SERVER_SAVE:
		StartServer_WeaponPage_SetStatusBar("Create a script for use with \\exec");
		break;

	case ID_SERVER_LOAD:
		StartServer_WeaponPage_SetStatusBar("Load a previously saved UIE script");
		break;
	};
}




/*
=================
StartServer_WeaponPage_MenuInit
=================
*/
void StartServer_WeaponPage_MenuInit(void)
{
	menuframework_s* menuptr;
	int y, y_base;
	int i;
	int style;
	float scale;

	memset(&s_weaponcontrols, 0, sizeof(servercontrols_t));

	StartServer_WeaponPage_Cache();

	menuptr = &s_weaponcontrols.menu;

	menuptr->key = StartServer_WeaponPage_MenuKey;
	menuptr->wrapAround = qtrue;
	menuptr->native 	= qfalse;
	menuptr->fullscreen = qtrue;
	menuptr->draw = StartServer_WeaponPage_MenuDraw;

	StartServer_CommonControls_Init(menuptr, &s_weaponcontrols.common, StartServer_WeaponPage_CommonEvent, COMMONCTRL_WEAPON);


	//
	// the user controlled server params
	//

	y = GAMETYPEROW_Y;
	s_weaponcontrols.gameType.generic.type		= MTYPE_SPINCONTROL;
	s_weaponcontrols.gameType.generic.id		= ID_SERVER_GAMETYPE;
	s_weaponcontrols.gameType.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_weaponcontrols.gameType.generic.callback	= StartServer_WeaponPage_Event;
	s_weaponcontrols.gameType.generic.x			= GAMETYPECOLUMN_X;
	s_weaponcontrols.gameType.generic.y			= y;

	s_weaponcontrols.gameTypeIcon.generic.type  = MTYPE_BITMAP;
	s_weaponcontrols.gameTypeIcon.generic.flags = QMF_INACTIVE;
	s_weaponcontrols.gameTypeIcon.generic.x	 = GAMETYPEICON_X;
	s_weaponcontrols.gameTypeIcon.generic.y	 = y;
	s_weaponcontrols.gameTypeIcon.width  	     = 32;
	s_weaponcontrols.gameTypeIcon.height  	     = 32;

	y += LINE_HEIGHT;
	s_weaponcontrols.hostname.generic.type       = MTYPE_FIELD;
	s_weaponcontrols.hostname.generic.flags      = QMF_SMALLFONT|QMF_PULSEIFFOCUS;
	s_weaponcontrols.hostname.generic.x          = GAMETYPECOLUMN_X;
	s_weaponcontrols.hostname.generic.y	        = y;
	s_weaponcontrols.hostname.generic.id	        = ID_SERVER_HOSTNAME;
	s_weaponcontrols.hostname.generic.callback	= StartServer_WeaponPage_Event;
	s_weaponcontrols.hostname.field.widthInChars = 32;
	s_weaponcontrols.hostname.field.maxchars     = MAX_HOSTNAME_LENGTH;


	s_weaponcontrols.num_radio = 0;
	s_weaponcontrols.num_field = 0;
	s_weaponcontrols.num_spin = 0;

	y += LINE_HEIGHT;

	s_weaponcontrols.saveScript.generic.type  = MTYPE_BITMAP;
	s_weaponcontrols.saveScript.generic.name  = SERVER_SAVE0;
	s_weaponcontrols.saveScript.generic.flags = QMF_PULSEIFFOCUS;
	s_weaponcontrols.saveScript.generic.x	   =  320 - 128;
	s_weaponcontrols.saveScript.generic.y	   = 480 - 64;
	s_weaponcontrols.saveScript.generic.id	   = ID_SERVER_SAVE;
	s_weaponcontrols.saveScript.generic.callback = StartServer_WeaponPage_Event;
	s_weaponcontrols.saveScript.generic.statusbar = StartServer_WeaponPage_StatusBar;
	s_weaponcontrols.saveScript.width = 128;
	s_weaponcontrols.saveScript.height = 64;
	s_weaponcontrols.saveScript.focuspic = SERVER_SAVE1;

	s_weaponcontrols.loadScript.generic.type  = MTYPE_BITMAP;
	s_weaponcontrols.loadScript.generic.name  = SERVER_LOAD0;
	s_weaponcontrols.loadScript.generic.flags = QMF_PULSEIFFOCUS;
	s_weaponcontrols.loadScript.generic.x	   =  320;
	s_weaponcontrols.loadScript.generic.y	   = 480 - 64;
	s_weaponcontrols.loadScript.generic.id	   = ID_SERVER_LOAD;
	s_weaponcontrols.loadScript.generic.callback = StartServer_WeaponPage_Event;
	s_weaponcontrols.loadScript.generic.statusbar = StartServer_WeaponPage_StatusBar;
	s_weaponcontrols.loadScript.width = 128;
	s_weaponcontrols.loadScript.height = 64;
	s_weaponcontrols.loadScript.focuspic = SERVER_LOAD1;

	s_weaponcontrols.statusbar_height = 480 - 64 - LINE_HEIGHT;
	
	if(cl_language.integer == 0){
	s_weaponcontrols.gameType.generic.name		= "Game Type:";
	s_weaponcontrols.gameType.itemnames			= gametype_items;
	s_weaponcontrols.hostname.generic.name       = "Hostname:";
	}
	if(cl_language.integer == 1){
	s_weaponcontrols.gameType.generic.name		= "Режим Игры:";
	s_weaponcontrols.gameType.itemnames			= gametype_itemsru;
	s_weaponcontrols.hostname.generic.name       = "Имя сервера:";
	}

	// init the tabbed control list for the page
	s_weaponcontrols.activePage = 0;
    s_weaponcontrols.pageList = weapv_multiplayerserver;
	s_weaponcontrols.pageListSize = ARRAY_COUNT(weapv_multiplayerserver);

	style = UI_RIGHT|UI_MEDIUMFONT;
	scale = UI_ProportionalSizeScale(style, 0);
	y_base = TABCONTROLCENTER_Y - s_weaponcontrols.pageListSize*PROP_HEIGHT*scale/2;

	for (i = 0; i < s_weaponcontrols.pageListSize; i++)
	{
		s_weaponcontrols.pageList[i].menutext->generic.type     = MTYPE_PTEXT;
		s_weaponcontrols.pageList[i].menutext->generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_weaponcontrols.pageList[i].menutext->generic.id	     = s_weaponcontrols.pageList[i].id;
		s_weaponcontrols.pageList[i].menutext->generic.callback = StartServer_WeaponPage_TabEvent;
		s_weaponcontrols.pageList[i].menutext->generic.x	     = 140 - uis.wideoffset/2;
		s_weaponcontrols.pageList[i].menutext->generic.y	     = y_base + i*PROP_HEIGHT*scale;
		s_weaponcontrols.pageList[i].menutext->string			= s_weaponcontrols.pageList[i].title;
		s_weaponcontrols.pageList[i].menutext->style			= style;
		s_weaponcontrols.pageList[i].menutext->color			= color_white;

		Menu_AddItem( menuptr, s_weaponcontrols.pageList[i].menutext);
	}

	// register controls
	Menu_AddItem( menuptr, &s_weaponcontrols.gameType);
	Menu_AddItem( menuptr, &s_weaponcontrols.gameTypeIcon);
	Menu_AddItem( menuptr, &s_weaponcontrols.hostname);

	// we only need the QMF_NODEFAULTINIT flag to get these controls
	// registered. Type is added later
	for (i = 0; i < MAX_SERVER_RADIO_CONTROL; i++) {
		s_weaponcontrols.radio[i].control.generic.type = MTYPE_RADIOBUTTON;
		s_weaponcontrols.radio[i].control.generic.flags = QMF_NODEFAULTINIT|QMF_HIDDEN|QMF_INACTIVE;
		Menu_AddItem( menuptr, &s_weaponcontrols.radio[i].control);
	}

	for (i = 0; i < MAX_SERVER_MFIELD_CONTROL; i++) {
		s_weaponcontrols.field[i].control.generic.type = MTYPE_FIELD;
		s_weaponcontrols.field[i].control.generic.flags = QMF_NODEFAULTINIT|QMF_HIDDEN|QMF_INACTIVE;
		Menu_AddItem( menuptr, &s_weaponcontrols.field[i].control);
	}

	for (i = 0; i < MAX_SERVER_SPIN_CONTROL; i++) {
		s_weaponcontrols.spin[i].control.generic.type = MTYPE_SPINCONTROL;
		s_weaponcontrols.spin[i].control.generic.flags = QMF_NODEFAULTINIT|QMF_HIDDEN|QMF_INACTIVE;
		Menu_AddItem( menuptr, &s_weaponcontrols.spin[i].control);
	}

	Menu_AddItem( menuptr, &s_weaponcontrols.saveScript);
	Menu_AddItem( menuptr, &s_weaponcontrols.loadScript);

	// control init is done on uis.firstdraw
	UI_PushMenu( &s_weaponcontrols.menu );
}

