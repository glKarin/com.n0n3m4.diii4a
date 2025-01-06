// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

CONTROLS MENU

=======================================================================
*/

#include "ui_local.h"

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"


typedef struct {
	char	*command;
	char	*label;
	int		id;
	int		anim;
	int		defaultbind1;
	int		defaultbind2;
	int		bind1;
	int		bind2;
} bind_t;

typedef struct
{
	char*	name;
	float	defaultvalue;
	float	value;	
} configcvar_t;

#define SAVE_NOOP		0
#define SAVE_YES		1
#define SAVE_NO			2
#define SAVE_CANCEL		3

// control sections
#define C_MOVEMENT		0
#define C_LOOKING		1
#define C_WEAPONS		2
#define C_INPUT			3
#define C_MISC			4
#define C_MAX			5

#define ID_MOVEMENT		100
#define ID_LOOKING		101
#define ID_WEAPONS		102
#define ID_MISC			103
#define ID_DEFAULTS		104
#define ID_BACK			105
#define ID_SAVEANDEXIT	106
#define ID_EXIT			107
#define ID_INPUT		108

// bindable actions
#define ID_SHOWSCORES	0
#define ID_USEITEM		1	
#define ID_SPEED		2	
#define ID_FORWARD		3	
#define ID_BACKPEDAL	4
#define ID_MOVELEFT		5
#define ID_MOVERIGHT	6
#define ID_MOVEUP		7	
#define ID_MOVEDOWN		8
#define ID_LEFT			9	
#define ID_RIGHT		10	
#define ID_STRAFE		11
#define ID_LOOKUP		12	
#define ID_LOOKDOWN		13
#define ID_MOUSELOOK	14
#define ID_CENTERVIEW	15
#define ID_ZOOMVIEW		16
#define ID_WEAPON1		17
#define ID_WEAPON2		18
#define ID_WEAPON3		19
#define ID_WEAPON4		20
#define ID_WEAPON5		21
#define ID_WEAPON6		22
#define ID_WEAPON7		23
#define ID_WEAPON8		24
#define ID_WEAPON9		25
#define ID_WEAPON10		26
#define ID_WEAPON11		27
#define ID_WEAPON12		28
#define ID_WEAPON13		29
#define ID_WEAPON14		30
#define ID_WEAPON15		31
#define ID_ATTACK		32
#define ID_WEAPPREV		33
#define ID_WEAPNEXT		34
#define ID_GESTURE		35
#define ID_CHAT			36
#define ID_CHAT2		37
#define ID_CHAT3		38
#define ID_CHAT4		39
#define ID_BOTMENU		40
#define ID_FLASHLIGHT		41
#define ID_THIRDPERSON		42
#define ID_DWEAPON		43
#define ID_DHOLDABLE		44
#define ID_ACC		45
#define ID_CLRUN		46
#define ID_SANDBOX		47
#define ID_SANDBOXMODE		48
#define ID_NEWSANDBOX		49
#define ID_EXITVEHICLE		50


// all others
#define ID_FREELOOK		51
#define ID_INVERTMOUSE	52
#define ID_ALWAYSRUN	53
#define ID_MOUSESPEED	54
#define ID_JOYENABLE	55
#define ID_JOYTHRESHOLD	56
#define ID_SMOOTHMOUSE	57
#define ID_MOUSESTYLE	58


typedef struct
{
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;
//	menubitmap_s		player;

	menutext_s			movement;
	menutext_s			looking;
	menutext_s			weapons;
	menutext_s			input;
	menutext_s			misc;

	menulist_s			mousestyle;
	menuaction_s		walkforward;
	menuaction_s		backpedal;
	menuaction_s		stepleft;
	menuaction_s		stepright;
	menuaction_s		moveup;
	menuaction_s		movedown;
	menuaction_s		turnleft;
	menuaction_s		turnright;
	menuaction_s		sidestep;
	menuaction_s		run;
	menuaction_s		machinegun;
	menuaction_s		chainsaw;
	menuaction_s		shotgun;
	menuaction_s		grenadelauncher;
	menuaction_s		rocketlauncher;
	menuaction_s		lightning;
	menuaction_s		railgun;
	menuaction_s		plasma;
	menuaction_s		bfg;
	menuaction_s		hook;
	menuaction_s		nailgun;
	menuaction_s		prox;
	menuaction_s		chaingun;
	menuaction_s		flamethrower;
	menuaction_s		darkflare;
	menuaction_s		attack;
	menuaction_s		prevweapon;
	menuaction_s		nextweapon;
	menuaction_s		lookup;
	menuaction_s		lookdown;
	menuaction_s		mouselook;
	menuradiobutton_s	freelook;
	menuaction_s		centerview;
	menuaction_s		zoomview;
	menuaction_s		gesture;
	menuradiobutton_s	invertmouse;
	menuslider_s		sensitivity;
	menuradiobutton_s	smoothmouse;
	menuradiobutton_s	alwaysrun;
	menuaction_s		showscores;
	menuaction_s		useitem;
	playerInfo_t		playerinfo;
	qboolean			changesmade;
	menuaction_s		chat;
	menuaction_s		chat2;
	menuaction_s		chat3;
	menuaction_s		chat4;
	menuaction_s		botmenu;
	menuaction_s		flashlight;
	menuaction_s		thirdperson;
	menuaction_s		dweapon;
	menuaction_s		dholdable;
	menuaction_s		acc;
	menuaction_s		clrun;
	menuaction_s		sandbox;
	menuaction_s		sandboxmode;
	menuaction_s		newsandbox;
	menuaction_s		exitvehicle;
	menuradiobutton_s	joyenable;
	menuslider_s		joythreshold;
	int					section;
	qboolean			waitingforkey;

	modelAnim_t			model;

	menubitmap_s		back;
	menutext_s			name;
} controls_t; 	

static controls_t s_controls;

static vec4_t controls_binding_color  = {1.00f, 0.43f, 0.00f, 1.00f};

static bind_t g_bindings[] = 
{
	{"+scores",			"show scores",		ID_SHOWSCORES,	ANIM_IDLE,		K_TAB,			-1,		-1, -1},
	{"+button2",		"use item",			ID_USEITEM,		ANIM_IDLE,		K_ENTER,		-1,		-1, -1},
	{"+speed", 			"run / walk",		ID_SPEED,		ANIM_RUN,		K_SHIFT,		-1,		-1,	-1},
	{"+forward", 		"walk forward / accel",		ID_FORWARD,		ANIM_WALK,		K_UPARROW,		-1,		-1, -1},
	{"+back", 			"walk back / backpedal",		ID_BACKPEDAL,	ANIM_BACK,		K_DOWNARROW,	-1,		-1, -1},
	{"+moveleft", 		"step left",		ID_MOVELEFT,	ANIM_STEPLEFT,	',',			-1,		-1, -1},
	{"+moveright", 		"step right",		ID_MOVERIGHT,	ANIM_STEPRIGHT,	'.',			-1,		-1, -1},
	{"+moveup",			"up / jump / handbrake",		ID_MOVEUP,		ANIM_JUMP,		K_SPACE,		-1,		-1, -1},
	{"+movedown",		"down / crouch / exit",	ID_MOVEDOWN,	ANIM_CROUCH,	'c',			-1,		-1, -1},
	{"+left", 			"turn left",		ID_LEFT,		ANIM_TURNLEFT,	K_LEFTARROW,	-1,		-1, -1},
	{"+right", 			"turn right",		ID_RIGHT,		ANIM_TURNRIGHT,	K_RIGHTARROW,	-1,		-1, -1},
	{"+strafe", 		"sidestep / turn",	ID_STRAFE,		ANIM_IDLE,		K_ALT,			-1,		-1, -1},
	{"+lookup", 		"look up",			ID_LOOKUP,		ANIM_LOOKUP,	K_PGDN,			-1,		-1, -1},
	{"+lookdown", 		"look down",		ID_LOOKDOWN,	ANIM_LOOKDOWN,	K_DEL,			-1,		-1, -1},
	{"+mlook", 			"mouse look",		ID_MOUSELOOK,	ANIM_IDLE,		'/',			-1,		-1, -1},
	{"centerview", 		"center view",		ID_CENTERVIEW,	ANIM_IDLE,		K_END,			-1,		-1, -1},
	{"+zoom", 			"zoom view",		ID_ZOOMVIEW,	ANIM_IDLE,		-1,				-1,		-1, -1},
	{"weapon 1",		"gauntlet",			ID_WEAPON1,		ANIM_WEAPON1,	'1',			-1,		-1, -1},
	{"weapon 2",		"machinegun",		ID_WEAPON2,		ANIM_WEAPON2,	'2',			-1,		-1, -1},
	{"weapon 3",		"shotgun",			ID_WEAPON3,		ANIM_WEAPON3,	'3',			-1,		-1, -1},
	{"weapon 4",		"grenade launcher",	ID_WEAPON4,		ANIM_WEAPON4,	'4',			-1,		-1, -1},
	{"weapon 5",		"rocket launcher",	ID_WEAPON5,		ANIM_WEAPON5,	'5',			-1,		-1, -1},
	{"weapon 6",		"lightning",		ID_WEAPON6,		ANIM_WEAPON6,	'6',			-1,		-1, -1},
	{"weapon 7",		"railgun",			ID_WEAPON7,		ANIM_WEAPON7,	'7',			-1,		-1, -1},
	{"weapon 8",		"plasma gun",		ID_WEAPON8,		ANIM_WEAPON8,	'8',			-1,		-1, -1},
	{"weapon 9",		"BFG",				ID_WEAPON9,		ANIM_WEAPON9,	'9',			-1,		-1, -1},
	{"weapon 10",		"grappling hook",				ID_WEAPON10,		ANIM_WEAPON10,	'h',			-1,		-1, -1},
	{"weapon 11",		"nailgun",				ID_WEAPON11,		ANIM_WEAPON11,	'n',			-1,		-1, -1},
	{"weapon 12",		"prox Launcher",				ID_WEAPON12,		ANIM_WEAPON12,	'p',			-1,		-1, -1},
	{"weapon 13",		"chaingun",				ID_WEAPON13,		ANIM_WEAPON13,	'i',			-1,		-1, -1},
	{"weapon 14",		"flamethrower",				ID_WEAPON14,		ANIM_WEAPON14,	'l',			-1,		-1, -1},
	{"weapon 15",		"darkflare",				ID_WEAPON15,		ANIM_WEAPON15,	'k',			-1,		-1, -1},
	{"+attack", 		"attack",			ID_ATTACK,		ANIM_ATTACK,	K_CTRL,			-1,		-1, -1},
	{"weapprev",		"prev weapon",		ID_WEAPPREV,	ANIM_IDLE,		'[',			-1,		-1, -1},
	{"weapnext", 		"next weapon",		ID_WEAPNEXT,	ANIM_IDLE,		']',			-1,		-1, -1},
	{"+button3", 		"gesture / horn",			ID_GESTURE,		ANIM_GESTURE,	'g',		-1,		-1, -1},
	{"messagemode", 	"chat",				ID_CHAT,		ANIM_CHAT,		't',			-1,		-1, -1},
	{"messagemode2", 	"chat - team",		ID_CHAT2,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"messagemode3", 	"chat - target",	ID_CHAT3,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"messagemode4", 	"chat - attacker",	ID_CHAT4,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"ui_teamorders", 	"bot command menu",	ID_BOTMENU,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"flashlight", 	"flashlight",	ID_FLASHLIGHT,		ANIM_CHAT,		'f',				-1,		-1, -1},
	{"toggle cg_thirdperson", 	"third person toggle",	ID_THIRDPERSON,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"dropweapon", 	"drop weapon",	ID_DWEAPON,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"dropholdable", 	"drop holdable",	ID_DHOLDABLE,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"+acc", 	"accuracy",	ID_ACC,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"toggle cl_run", 	"run/walk toggle",	ID_CLRUN,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"ui_sandbox", 	"sandbox menu",	ID_SANDBOX,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"vstr uitoolmode", 	"sandbox tool mode",	ID_SANDBOXMODE,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"vstr lastui", 	"addon menu",	ID_NEWSANDBOX,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"exitvehicle", 	"exit vehicle",	ID_EXITVEHICLE,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{(char*)NULL,		(char*)NULL,		0,				0,				-1,				-1,		-1,	-1},
};

static bind_t g_bindingsrus[] = 
{
	{"+scores",			"показать результаты",		ID_SHOWSCORES,	ANIM_IDLE,		K_TAB,			-1,		-1, -1},
	{"+button2",		"использовать предмет",			ID_USEITEM,		ANIM_IDLE,		K_ENTER,		-1,		-1, -1},
	{"+speed", 			"бег / ходьба",		ID_SPEED,		ANIM_RUN,		K_SHIFT,		-1,		-1,	-1},
	{"+forward", 		"идти вперед / газ",		ID_FORWARD,		ANIM_WALK,		K_UPARROW,		-1,		-1, -1},
	{"+back", 			"идти назад / ехать назад",		ID_BACKPEDAL,	ANIM_BACK,		K_DOWNARROW,	-1,		-1, -1},
	{"+moveleft", 		"идти влево",		ID_MOVELEFT,	ANIM_STEPLEFT,	',',			-1,		-1, -1},
	{"+moveright", 		"идти вправо",		ID_MOVERIGHT,	ANIM_STEPRIGHT,	'.',			-1,		-1, -1},
	{"+moveup",			"прыжок / ручной тормоз",		ID_MOVEUP,		ANIM_JUMP,		K_SPACE,		-1,		-1, -1},
	{"+movedown",		"присесть / выход",	ID_MOVEDOWN,	ANIM_CROUCH,	'c',			-1,		-1, -1},
	{"+left", 			"посмотреть влево",		ID_LEFT,		ANIM_TURNLEFT,	K_LEFTARROW,	-1,		-1, -1},
	{"+right", 			"посмотреть вправо",		ID_RIGHT,		ANIM_TURNRIGHT,	K_RIGHTARROW,	-1,		-1, -1},
	{"+strafe", 		"ходьба мышкой",	ID_STRAFE,		ANIM_IDLE,		K_ALT,			-1,		-1, -1},
	{"+lookup", 		"посмотреть вверх",			ID_LOOKUP,		ANIM_LOOKUP,	K_PGDN,			-1,		-1, -1},
	{"+lookdown", 		"посмотреть вниз",		ID_LOOKDOWN,	ANIM_LOOKDOWN,	K_DEL,			-1,		-1, -1},
	{"+mlook", 			"обзор мышкой",		ID_MOUSELOOK,	ANIM_IDLE,		'/',			-1,		-1, -1},
	{"centerview", 		"центр вид",		ID_CENTERVIEW,	ANIM_IDLE,		K_END,			-1,		-1, -1},
	{"+zoom", 			"приближение",		ID_ZOOMVIEW,	ANIM_IDLE,		-1,				-1,		-1, -1},
	{"weapon 1",		"кулак",			ID_WEAPON1,		ANIM_WEAPON1,	'1',			-1,		-1, -1},
	{"weapon 2",		"автомат",		ID_WEAPON2,		ANIM_WEAPON2,	'2',			-1,		-1, -1},
	{"weapon 3",		"дробовик",			ID_WEAPON3,		ANIM_WEAPON3,	'3',			-1,		-1, -1},
	{"weapon 4",		"гранатомет",	ID_WEAPON4,		ANIM_WEAPON4,	'4',			-1,		-1, -1},
	{"weapon 5",		"ракетница",	ID_WEAPON5,		ANIM_WEAPON5,	'5',			-1,		-1, -1},
	{"weapon 6",		"молния",		ID_WEAPON6,		ANIM_WEAPON6,	'6',			-1,		-1, -1},
	{"weapon 7",		"рэйлган",			ID_WEAPON7,		ANIM_WEAPON7,	'7',			-1,		-1, -1},
	{"weapon 8",		"плазмаган",		ID_WEAPON8,		ANIM_WEAPON8,	'8',			-1,		-1, -1},
	{"weapon 9",		"БФГ",				ID_WEAPON9,		ANIM_WEAPON9,	'9',			-1,		-1, -1},
	{"weapon 10",		"крюк",				ID_WEAPON10,		ANIM_WEAPON10,	'h',			-1,		-1, -1},
	{"weapon 11",		"гвоздомет",				ID_WEAPON11,		ANIM_WEAPON11,	'n',			-1,		-1, -1},
	{"weapon 12",		"мины",				ID_WEAPON12,		ANIM_WEAPON12,	'p',			-1,		-1, -1},
	{"weapon 13",		"пулемет",				ID_WEAPON13,		ANIM_WEAPON13,	'i',			-1,		-1, -1},
	{"weapon 14",		"огнемет",				ID_WEAPON14,		ANIM_WEAPON14,	'l',			-1,		-1, -1},
	{"weapon 15",		"черная вспышка",				ID_WEAPON15,		ANIM_WEAPON15,	'k',			-1,		-1, -1},
	{"+attack", 		"атака",			ID_ATTACK,		ANIM_ATTACK,	K_CTRL,			-1,		-1, -1},
	{"weapprev",		"пред оружие",		ID_WEAPPREV,	ANIM_IDLE,		'[',			-1,		-1, -1},
	{"weapnext", 		"след оружие",		ID_WEAPNEXT,	ANIM_IDLE,		']',			-1,		-1, -1},
	{"+button3", 		"жест / гудок",			ID_GESTURE,		ANIM_GESTURE,	'g',		-1,		-1, -1},
	{"messagemode", 	"чат",				ID_CHAT,		ANIM_CHAT,		't',			-1,		-1, -1},
	{"messagemode2", 	"чат - команда",		ID_CHAT2,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"messagemode3", 	"чат - цель",	ID_CHAT3,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"messagemode4", 	"чат - аттакующий",	ID_CHAT4,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"ui_teamorders", 	"меню командных приказов",	ID_BOTMENU,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"flashlight", 	"фонарик / фары",	ID_FLASHLIGHT,		ANIM_CHAT,		'f',				-1,		-1, -1},
	{"toggle cg_thirdperson", 	"переключение третьего лица",	ID_THIRDPERSON,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"dropweapon", 	"выбросить оружие",	ID_DWEAPON,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"dropholdable", 	"выбросить предмет",	ID_DHOLDABLE,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"+acc", 	"точность",	ID_ACC,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"toggle cl_run", 	"бег/ходьба переключение",	ID_CLRUN,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"ui_sandbox", 	"спавн меню",	ID_SANDBOX,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"vstr uitoolmode", 	"песочница режим инструмента",	ID_SANDBOXMODE,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"vstr lastui", 	"меню аддонов",	ID_NEWSANDBOX,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{"exitvehicle", 	"выйти из транспорта",	ID_EXITVEHICLE,		ANIM_CHAT,		-1,				-1,		-1, -1},
	{(char*)NULL,		(char*)NULL,		0,				0,				-1,				-1,		-1,	-1},
};

static configcvar_t g_configcvars[] =
{
	{"cl_run",			0,					0},
	{"m_pitch",			0,					0},
	{"sensitivity",		0,					0},
	{"in_joystick",		0,					0},
	{"joy_threshold",	0,					0},
	{"m_filter",		0,					0},
	{"cl_freelook",		0,					0},
//	{"in_mouse",		-1,					0},	// latched cvar... broken by init procedure
	{NULL,				0,					0}
};


static const char* mousestyle_description[] = {
	"Win32",
	"Raw",
	NULL
};


static menucommon_s *g_movement_controls[] =
{
	(menucommon_s *)&s_controls.alwaysrun,     
	(menucommon_s *)&s_controls.run,            
	(menucommon_s *)&s_controls.walkforward,
	(menucommon_s *)&s_controls.backpedal,
	(menucommon_s *)&s_controls.stepleft,      
	(menucommon_s *)&s_controls.stepright,     
	(menucommon_s *)&s_controls.moveup,        
	(menucommon_s *)&s_controls.movedown,      
	(menucommon_s *)&s_controls.turnleft,      
	(menucommon_s *)&s_controls.turnright,     
	(menucommon_s *)&s_controls.sidestep,
	NULL
};

static menucommon_s *g_weapons_controls[] = {
	(menucommon_s *)&s_controls.attack,           
	(menucommon_s *)&s_controls.nextweapon,
	(menucommon_s *)&s_controls.prevweapon, 
	(menucommon_s *)&s_controls.chainsaw,         
	(menucommon_s *)&s_controls.machinegun,
	(menucommon_s *)&s_controls.shotgun,          
	(menucommon_s *)&s_controls.grenadelauncher,
	(menucommon_s *)&s_controls.rocketlauncher,   
	(menucommon_s *)&s_controls.lightning,   
	(menucommon_s *)&s_controls.railgun,          
	(menucommon_s *)&s_controls.plasma,           
	(menucommon_s *)&s_controls.bfg,              
	(menucommon_s *)&s_controls.hook,  
	(menucommon_s *)&s_controls.nailgun,  
	(menucommon_s *)&s_controls.prox,  
	(menucommon_s *)&s_controls.chaingun,  
	(menucommon_s *)&s_controls.flamethrower,  
	(menucommon_s *)&s_controls.darkflare,  
	NULL,
};

static menucommon_s *g_looking_controls[] = {
	(menucommon_s *)&s_controls.lookup,
	(menucommon_s *)&s_controls.lookdown,
	(menucommon_s *)&s_controls.mouselook,
	(menucommon_s *)&s_controls.freelook,
	(menucommon_s *)&s_controls.centerview,
	(menucommon_s *)&s_controls.zoomview,
	NULL,
};

static menucommon_s *g_misc_controls[] = {
	(menucommon_s *)&s_controls.showscores,
	(menucommon_s *)&s_controls.useitem,
	(menucommon_s *)&s_controls.gesture,
	(menucommon_s *)&s_controls.chat,
	(menucommon_s *)&s_controls.chat2,
	(menucommon_s *)&s_controls.chat3,
	(menucommon_s *)&s_controls.chat4,
	(menucommon_s *)&s_controls.botmenu,
	(menucommon_s *)&s_controls.flashlight,
	(menucommon_s *)&s_controls.thirdperson,
	(menucommon_s *)&s_controls.dweapon,
	(menucommon_s *)&s_controls.dholdable,
	(menucommon_s *)&s_controls.acc,
	(menucommon_s *)&s_controls.clrun,
	(menucommon_s *)&s_controls.sandbox,
	(menucommon_s *)&s_controls.sandboxmode,
	(menucommon_s *)&s_controls.newsandbox,
	(menucommon_s *)&s_controls.exitvehicle,
	NULL,
};

static menucommon_s *g_input_controls[] = {
	(menucommon_s *)&s_controls.sensitivity,
	(menucommon_s *)&s_controls.mousestyle,
	(menucommon_s *)&s_controls.smoothmouse,
	(menucommon_s *)&s_controls.invertmouse,
	(menucommon_s *)&s_controls.joyenable,
	(menucommon_s *)&s_controls.joythreshold,
	NULL
};


static menucommon_s **g_controls[] = {
	g_movement_controls,
	g_looking_controls,
	g_weapons_controls,
	g_input_controls,
	g_misc_controls,
};

/*
=================
Controls_InitCvars
=================
*/
static void Controls_InitCvars( void )
{
	int				i;
	configcvar_t*	cvarptr;

	cvarptr = g_configcvars;
	for (i=0; ;i++,cvarptr++)
	{
		if (!cvarptr->name)
			break;

		// get current value
		cvarptr->value = trap_Cvar_VariableValue( cvarptr->name );

		// get default value
		trap_Cvar_Reset( cvarptr->name );
		cvarptr->defaultvalue = trap_Cvar_VariableValue( cvarptr->name );

		// restore current value
		trap_Cvar_SetValue( cvarptr->name, cvarptr->value );
	}
}

/*
=================
Controls_GetCvarDefault
=================
*/
static float Controls_GetCvarDefault( char* name )
{
	configcvar_t*	cvarptr;
	int				i;

	cvarptr = g_configcvars;
	for (i=0; ;i++,cvarptr++)
	{
		if (!cvarptr->name)
			return (0);

		if (!strcmp(cvarptr->name,name))
			break;
	}

	return (cvarptr->defaultvalue);
}

/*
=================
Controls_GetCvarValue
=================
*/
static float Controls_GetCvarValue( char* name )
{
	configcvar_t*	cvarptr;
	int				i;

	cvarptr = g_configcvars;
	for (i=0; ;i++,cvarptr++)
	{
		if (!cvarptr->name)
			return (0);

		if (!strcmp(cvarptr->name,name))
			break;
	}

	return (cvarptr->value);
}


/*
=================
Controls_UpdateModel
=================
*/
static void Controls_UpdateModel( int anim ) {
	UIE_PlayerInfo_ChangeAnimation(&s_controls.model, anim);
}


/*
=================
Controls_Update
=================
*/
static void Controls_Update( void ) {
	int		i;
	int		j;
	int		y;
	menucommon_s	**controls;
	menucommon_s	*control;

	// disable all controls in all groups
	for( i = 0; i < C_MAX; i++ ) {
		controls = g_controls[i];
		// bk001204 - parentheses
		for( j = 0;	(control = controls[j]); j++ ) {
			control->flags |= (QMF_HIDDEN|QMF_INACTIVE);
		}
	}

	controls = g_controls[s_controls.section];

	// enable controls in active group (and count number of items for vertical centering)
	// bk001204 - parentheses
	for( j = 0;	(control = controls[j]); j++ ) {
		control->flags &= ~(QMF_GRAYED|QMF_HIDDEN|QMF_INACTIVE);
	}

	// position controls
	y = ( SCREEN_HEIGHT - j * SMALLCHAR_HEIGHT ) / 2;
	// bk001204 - parentheses
	for( j = 0;	(control = controls[j]); j++, y += SMALLCHAR_HEIGHT ) {
		control->x      = 320;
		control->y      = y-30;
		control->left   = 320 - 19*SMALLCHAR_WIDTH;
		control->right  = 320 + 21*SMALLCHAR_WIDTH;
		control->top    = y-30;
		control->bottom = y-30 + SMALLCHAR_HEIGHT;
	}

	if( s_controls.waitingforkey ) {
		// disable everybody
		for( i = 0; i < s_controls.menu.nitems; i++ ) {
			((menucommon_s*)(s_controls.menu.items[i]))->flags |= QMF_GRAYED;
		}

		// enable action item
		((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->flags &= ~QMF_GRAYED;

		// don't gray out player's name
		s_controls.name.generic.flags &= ~QMF_GRAYED;

		return;
	}

	// enable everybody
	for( i = 0; i < s_controls.menu.nitems; i++ ) {
		((menucommon_s*)(s_controls.menu.items[i]))->flags &= ~QMF_GRAYED;
	}

	// makes sure flags are right on the group selection controls
	s_controls.looking.generic.flags  &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.movement.generic.flags &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.weapons.generic.flags  &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.misc.generic.flags     &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);

	s_controls.looking.generic.flags  |= QMF_PULSEIFFOCUS;
	s_controls.movement.generic.flags |= QMF_PULSEIFFOCUS;
	s_controls.weapons.generic.flags  |= QMF_PULSEIFFOCUS;
	s_controls.misc.generic.flags     |= QMF_PULSEIFFOCUS;

	// set buttons
	switch( s_controls.section ) {
	case C_MOVEMENT:
		s_controls.movement.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.movement.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	
	case C_LOOKING:
		s_controls.looking.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.looking.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	
	case C_WEAPONS:
		s_controls.weapons.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.weapons.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;		

	case C_MISC:
		s_controls.misc.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.misc.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	}
}


/*
=================
Controls_DrawKeyBinding
=================
*/
static void Controls_DrawKeyBinding( void *self )
{
	menuaction_s*	a;
	int				x;
	int				y;
	int				b1;
	int				b2;
	qboolean		c;
	char			name[32];
	char			name2[32];

	a = (menuaction_s*) self;

	x =	a->generic.x;
	y = a->generic.y;

	c = (Menu_ItemAtCursor( a->generic.parent ) == a);

	b1 = g_bindings[a->generic.id].bind1;
	if (b1 == -1)
		strcpy(name,"???");
	else
	{
		trap_Key_KeynumToStringBuf( b1, name, 32 );
		Q_strupr(name);

		b2 = g_bindings[a->generic.id].bind2;
		if (b2 != -1)
		{
			trap_Key_KeynumToStringBuf( b2, name2, 32 );
			Q_strupr(name2);

			strcat( name, " or " );
			strcat( name, name2 );
		}
	}

	if (c)
	{
		UI_FillRect( a->generic.left, a->generic.top, a->generic.right-a->generic.left+1, a->generic.bottom-a->generic.top+1, color_select_bluo ); 
		if(cl_language.integer == 0){
		UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, color_highlight );
		}
		if(cl_language.integer == 1){
		UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindingsrus[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, color_highlight );
		}
		UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_SMALLFONT|UI_PULSE, color_highlight );

		if (s_controls.waitingforkey)
		{
			UI_DrawChar( x, y, '=', UI_CENTER|UI_BLINK|UI_SMALLFONT, color_highlight);
			if(cl_language.integer == 0){
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Waiting for new key ... ESCAPE to cancel", UI_SMALLFONT|UI_CENTER|UI_PULSE, colorWhite );
			}
			if(cl_language.integer == 1){
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Ожидание новой кнопки ... ESCAPE для отмены", UI_SMALLFONT|UI_CENTER|UI_PULSE, colorWhite );
			}
		}
		else
		{
			UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color_highlight);
			if(cl_language.integer == 0){
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.78, "Press ENTER or CLICK to change", UI_SMALLFONT|UI_CENTER, colorWhite );
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.82, "Press BACKSPACE to clear", UI_SMALLFONT|UI_CENTER, colorWhite );
			}
			if(cl_language.integer == 1){
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.78, "Нажмите ENTER или ЛКМ для изменения", UI_SMALLFONT|UI_CENTER, colorWhite );
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.82, "Нажмите BACKSPACE для сброса", UI_SMALLFONT|UI_CENTER, colorWhite );
			}
		}
	}
	else
	{
		if (a->generic.flags & QMF_GRAYED)
		{
			if(cl_language.integer == 0){
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, text_color_disabled );
			}
			if(cl_language.integer == 1){
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindingsrus[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, text_color_disabled );
			}
			UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_SMALLFONT, text_color_disabled );
		}
		else
		{
			if(cl_language.integer == 0){
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, color_white );
			}
			if(cl_language.integer == 1){
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindingsrus[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, color_white );
			}
			UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_SMALLFONT, color_white );
		}
	}
}

/*
=================
Controls_StatusBar
=================
*/
static void Controls_StatusBar( void *self )
{
	if(cl_language.integer == 0){
	UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Use Arrow Keys or CLICK to change", UI_SMALLFONT|UI_CENTER, colorWhite );
	}
	if(cl_language.integer == 1){
	UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Используйте стрелочки или ЛКМ для изменения", UI_SMALLFONT|UI_CENTER, colorWhite );
	}
}


/*
=================
Controls_DrawPlayer
=================
*/
static void Controls_DrawPlayer( void *self ) {
	UIE_PlayerInfo_AnimateModel(&s_controls.model);
}


/*
=================
Controls_GetKeyAssignment
=================
*/
static void Controls_GetKeyAssignment (char *command, int *twokeys)
{
	int		count;
	int		j;
	char	b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j = 0; j < 256; j++ )
	{
		trap_Key_GetBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_stricmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

/*
=================
Controls_GetConfig
=================
*/
static void Controls_GetConfig( void )
{
	int		i;
	int		twokeys[2];
	bind_t*	bindptr;

	// put the bindings into a local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)
			break;

		Controls_GetKeyAssignment(bindptr->command, twokeys);

		bindptr->bind1 = twokeys[0];
		bindptr->bind2 = twokeys[1];
	}

	s_controls.invertmouse.curvalue  = Controls_GetCvarValue( "m_pitch" ) < 0;
	s_controls.smoothmouse.curvalue  = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "m_filter" ) );
	s_controls.alwaysrun.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_run" ) );
	s_controls.sensitivity.curvalue  = UI_ClampCvar( 2, 30, Controls_GetCvarValue( "sensitivity" ) );
	s_controls.joyenable.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "in_joystick" ) );
	s_controls.joythreshold.curvalue = UI_ClampCvar( 0.05f, 0.75f, Controls_GetCvarValue( "joy_threshold" ) );
	s_controls.freelook.curvalue     = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_freelook" ) );
	s_controls.mousestyle.curvalue   = (trap_Cvar_VariableValue( "in_mouse" ) > 0) ? 1:0;
}

/*
=================
Controls_SetConfig
=================
*/
static void Controls_SetConfig( void )
{
	int		i;
	bind_t*	bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)
			break;

		if (bindptr->bind1 != -1)
		{	
			trap_Key_SetBinding( bindptr->bind1, bindptr->command );

			if (bindptr->bind2 != -1)
				trap_Key_SetBinding( bindptr->bind2, bindptr->command );
		}
	}

	if ( s_controls.invertmouse.curvalue )
		trap_Cvar_SetValue( "m_pitch", -fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );
	else
		trap_Cvar_SetValue( "m_pitch", fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );

	trap_Cvar_SetValue( "m_filter", s_controls.smoothmouse.curvalue );
	trap_Cvar_SetValue( "cl_run", s_controls.alwaysrun.curvalue );
	trap_Cvar_SetValue( "sensitivity", s_controls.sensitivity.curvalue );
	trap_Cvar_SetValue( "in_joystick", s_controls.joyenable.curvalue );
	trap_Cvar_SetValue( "joy_threshold", s_controls.joythreshold.curvalue );
	trap_Cvar_SetValue( "cl_freelook", s_controls.freelook.curvalue );
	trap_Cvar_SetValue( "in_mouse", (s_controls.mousestyle.curvalue == 1) ? 1:-1);
	trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
static void Controls_SetDefaults( void )
{
	int	i;
	bind_t*	bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, set its default binding
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)
			break;

		bindptr->bind1 = bindptr->defaultbind1;
		bindptr->bind2 = bindptr->defaultbind2;
	}

	s_controls.invertmouse.curvalue  = Controls_GetCvarDefault( "m_pitch" ) < 0;
	s_controls.smoothmouse.curvalue  = Controls_GetCvarDefault( "m_filter" );
	s_controls.alwaysrun.curvalue    = Controls_GetCvarDefault( "cl_run" );
	s_controls.sensitivity.curvalue  = Controls_GetCvarDefault( "sensitivity" );
	s_controls.joyenable.curvalue    = Controls_GetCvarDefault( "in_joystick" );
	s_controls.joythreshold.curvalue = Controls_GetCvarDefault( "joy_threshold" );
	s_controls.freelook.curvalue     = Controls_GetCvarDefault( "cl_freelook" );
	s_controls.mousestyle.curvalue     = (Controls_GetCvarDefault( "in_mouse" ) == 1) ? 1:0;
}

/*
=================
Controls_MenuKey
=================
*/
static sfxHandle_t Controls_MenuKey( int key )
{
	int			id;
	int			i;
	qboolean	found;
	bind_t*		bindptr;
	found = qfalse;

	if (!s_controls.waitingforkey)
	{
		switch (key)
		{
			case K_BACKSPACE:
			case K_DEL:
			case K_KP_DEL:
				key = -1;
				break;
		
			case K_MOUSE2:
			case K_ESCAPE:
				if (s_controls.changesmade)
					Controls_SetConfig();
				goto ignorekey;	

			default:
				goto ignorekey;
		}
	}
	else
	{
		if (key & K_CHAR_FLAG)
			goto ignorekey;

		switch (key)
		{
			case K_ESCAPE:
				s_controls.waitingforkey = qfalse;
				Controls_Update();
				return (menu_out_sound);
	
			case '`':
				goto ignorekey;
		}
	}

	s_controls.changesmade = qtrue;
	
	if (key != -1)
	{
		// remove from any other bind
		bindptr = g_bindings;
		for (i=0; ;i++,bindptr++)
		{
			if (!bindptr->label)	
				break;

			if (bindptr->bind2 == key)
				bindptr->bind2 = -1;

			if (bindptr->bind1 == key)
			{
				bindptr->bind1 = bindptr->bind2;	
				bindptr->bind2 = -1;
			}
		}
	}

	// assign key to local store
	id      = ((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->id;
	bindptr = g_bindings;
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)	
			break;
		
		if (bindptr->id == id)
		{
			found = qtrue;
			if (key == -1)
			{
				if( bindptr->bind1 != -1 ) {
					trap_Key_SetBinding( bindptr->bind1, "" );
					bindptr->bind1 = -1;
				}
				if( bindptr->bind2 != -1 ) {
					trap_Key_SetBinding( bindptr->bind2, "" );
					bindptr->bind2 = -1;
				}
			}
			else if (bindptr->bind1 == -1) {
				bindptr->bind1 = key;
			}
			else if (bindptr->bind1 != key && bindptr->bind2 == -1) {
				bindptr->bind2 = key;
			}
			else
			{
				trap_Key_SetBinding( bindptr->bind1, "" );
				trap_Key_SetBinding( bindptr->bind2, "" );
				bindptr->bind1 = key;
				bindptr->bind2 = -1;
			}						
			break;
		}
	}				
		
	s_controls.waitingforkey = qfalse;

	if (found)
	{	
		Controls_Update();
		return (menu_out_sound);
	}

ignorekey:
	return Menu_DefaultKey( &s_controls.menu, key );
}

/*
=================
Controls_ResetDefaults_Action
=================
*/
static void Controls_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}

	s_controls.changesmade = qtrue;
	Controls_SetDefaults();
	Controls_Update();
}

/*
=================
Controls_ResetDefaults_Draw
=================
*/
static void Controls_ResetDefaults_Draw( void ) {
	if(cl_language.integer == 0){
	UI_DrawString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset all", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "controls to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
	}
	if(cl_language.integer == 1){
	UI_DrawString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "ВНИМАНИЕ: Вы действительно хотите сбросить", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "настройки управления до стандартных.", UI_CENTER|UI_SMALLFONT, color_yellow );
	}
}

/*
=================
Controls_MenuEvent
=================
*/
static void Controls_MenuEvent( void* ptr, int event )
{
	switch (((menucommon_s*)ptr)->id)
	{
		case ID_MOVEMENT:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_MOVEMENT; 
				Controls_Update();
			}
			break;

		case ID_LOOKING:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_LOOKING; 
				Controls_Update();
			}
			break;

		case ID_WEAPONS:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_WEAPONS; 
				Controls_Update();
			}
			break;

		case ID_INPUT:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_INPUT;
				Controls_Update();
			}
			break;

		case ID_MISC:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_MISC; 
				Controls_Update();
			}
			break;

		case ID_DEFAULTS:
			if (event == QM_ACTIVATED)
			{
				if(cl_language.integer == 0){
				UI_ConfirmMenu( "SET TO DEFAULTS?", Controls_ResetDefaults_Draw, Controls_ResetDefaults_Action );
				}
				if(cl_language.integer == 1){
				UI_ConfirmMenu( "СБРОСИТЬ ДО СТАНДАРТНЫХ?", Controls_ResetDefaults_Draw, Controls_ResetDefaults_Action );
				}
			}
			break;

		case ID_BACK:
			if (event == QM_ACTIVATED)
			{
				if (s_controls.changesmade)
					Controls_SetConfig();
				UI_PopMenu();
			}
			break;

		case ID_SAVEANDEXIT:
			if (event == QM_ACTIVATED)
			{
				Controls_SetConfig();
				UI_PopMenu();
			}
			break;

		case ID_EXIT:
			if (event == QM_ACTIVATED)
			{
				UI_PopMenu();
			}
			break;

		case ID_MOUSESTYLE:	
		case ID_FREELOOK:
		case ID_MOUSESPEED:
		case ID_INVERTMOUSE:
		case ID_SMOOTHMOUSE:
		case ID_ALWAYSRUN:
		case ID_JOYENABLE:
		case ID_JOYTHRESHOLD:
			if (event == QM_ACTIVATED)
			{
				s_controls.changesmade = qtrue;
			}
			break;		
	}
}

/*
=================
Controls_ActionEvent
=================
*/
static void Controls_ActionEvent( void* ptr, int event )
{
	if (event == QM_LOSTFOCUS)
	{
		Controls_UpdateModel( ANIM_IDLE );
	}
	else if (event == QM_GOTFOCUS)
	{
		Controls_UpdateModel( g_bindings[((menucommon_s*)ptr)->id].anim );
	}
	else if ((event == QM_ACTIVATED) && !s_controls.waitingforkey)
	{
		s_controls.waitingforkey = 1;
		Controls_Update();
	}
}

/*
=================
Controls_InitModel
=================
*/
static void Controls_InitModel( void )
{
	UIE_PlayerInfo_InitModel(&s_controls.model);
	s_controls.model.bNoIdleAnim = qtrue;
}

/*
=================
Controls_InitWeapons
=================
*/
static void Controls_InitWeapons( void ) {
	gitem_t *	item;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType != IT_WEAPON ) {
			continue;
		}
		trap_R_RegisterModel( item->world_model[0] );
	}
}

/*
=================
Controls_MenuInit
=================
*/
static void Controls_MenuInit( void )
{
	static char playername[32];

	// zero set all our globals
	memset( &s_controls, 0 ,sizeof(controls_t) );

	Controls_Cache();

	s_controls.menu.key        = Controls_MenuKey;
	s_controls.menu.wrapAround = qtrue;
	s_controls.menu.fullscreen = qtrue;
	s_controls.menu.native 	   = qfalse;

	s_controls.banner.generic.type	= MTYPE_BTEXT;
	s_controls.banner.generic.flags	= QMF_CENTER_JUSTIFY;
	s_controls.banner.generic.x		= 320;
	s_controls.banner.generic.y		= 16;
	if(cl_language.integer == 0){
	s_controls.banner.string		= "CONTROLS";
	}
	if(cl_language.integer == 1){
	s_controls.banner.string		= "УПРАВЛЕНИЕ";
	}
	s_controls.banner.color			= color_white;
	s_controls.banner.style			= UI_CENTER;

	s_controls.framel.generic.type  = MTYPE_BITMAP;
	s_controls.framel.generic.name  = ART_FRAMEL;
	s_controls.framel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_controls.framel.generic.x     = 0;
	s_controls.framel.generic.y     = 78;
	s_controls.framel.width  	    = 256;
	s_controls.framel.height  	    = 329;

	s_controls.framer.generic.type  = MTYPE_BITMAP;
	s_controls.framer.generic.name  = ART_FRAMER;
	s_controls.framer.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_controls.framer.generic.x     = 376;
	s_controls.framer.generic.y     = 76;
	s_controls.framer.width  	    = 256;
	s_controls.framer.height  	    = 334;

	s_controls.looking.generic.type     = MTYPE_PTEXT;
	s_controls.looking.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.looking.generic.id	    = ID_LOOKING;
	s_controls.looking.generic.callback	= Controls_MenuEvent;
	s_controls.looking.generic.x	    = 152 - uis.wideoffset;
	s_controls.looking.generic.y	    = 240 - 2.5 * PROP_HEIGHT;
	if(cl_language.integer == 0){
	s_controls.looking.string			= "LOOK";
	}
	if(cl_language.integer == 1){
	s_controls.looking.string			= "ОБЗОР";
	}
	s_controls.looking.style			= UI_RIGHT;
	s_controls.looking.color			= color_white;

	s_controls.movement.generic.type     = MTYPE_PTEXT;
	s_controls.movement.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.movement.generic.id	     = ID_MOVEMENT;
	s_controls.movement.generic.callback = Controls_MenuEvent;
	s_controls.movement.generic.x	     = 152 - uis.wideoffset;
	s_controls.movement.generic.y	     = 240 - 1.5*PROP_HEIGHT;
	if(cl_language.integer == 0){
	s_controls.movement.string			= "MOVE";
	}
	if(cl_language.integer == 1){
	s_controls.movement.string			= "ДВИЖЕНИЕ";
	}
	s_controls.movement.style			= UI_RIGHT;
	s_controls.movement.color			= color_white;

	s_controls.weapons.generic.type	    = MTYPE_PTEXT;
	s_controls.weapons.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.weapons.generic.id	    = ID_WEAPONS;
	s_controls.weapons.generic.callback	= Controls_MenuEvent;
	s_controls.weapons.generic.x	    = 152 - uis.wideoffset;
	s_controls.weapons.generic.y	    = 240 - 0.5*PROP_HEIGHT;
	if(cl_language.integer == 0){
	s_controls.weapons.string			= "SHOOT";
	}	
	if(cl_language.integer == 1){
	s_controls.weapons.string			= "СТРЕЛЬБА";
	}
	s_controls.weapons.style			= UI_RIGHT;
	s_controls.weapons.color			= color_white;

	s_controls.input.generic.type	 = MTYPE_PTEXT;
	s_controls.input.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.input.generic.id	     = ID_INPUT;
	s_controls.input.generic.callback = Controls_MenuEvent;
	s_controls.input.generic.x		 = 152 - uis.wideoffset;
	s_controls.input.generic.y		 = 240 + 0.5*PROP_HEIGHT;
	if(cl_language.integer == 0){
	s_controls.input.string			= "INPUT";
	}
	if(cl_language.integer == 1){
	s_controls.input.string			= "ВВОД";
	}
	s_controls.input.style			= UI_RIGHT;
	s_controls.input.color			= color_white;

	s_controls.misc.generic.type	 = MTYPE_PTEXT;
	s_controls.misc.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.misc.generic.id	     = ID_MISC;
	s_controls.misc.generic.callback = Controls_MenuEvent;
	s_controls.misc.generic.x		 = 152 - uis.wideoffset;
	s_controls.misc.generic.y		 = 240 + 1.5*PROP_HEIGHT;
	if(cl_language.integer == 0){
	s_controls.misc.string			= "MISC";
	}
	if(cl_language.integer == 1){
	s_controls.misc.string			= "ДРУГОЕ";
	}
	s_controls.misc.style			= UI_RIGHT;
	s_controls.misc.color			= color_white;

	s_controls.back.generic.type	 = MTYPE_BITMAP;
	s_controls.back.generic.name     = ART_BACK0;
	s_controls.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.back.generic.x		 = 0 - uis.wideoffset;
	s_controls.back.generic.y		 = 480-64;
	s_controls.back.generic.id	     = ID_BACK;
	s_controls.back.generic.callback = Controls_MenuEvent;
	s_controls.back.width  		     = 128;
	s_controls.back.height  		 = 64;
	s_controls.back.focuspic         = ART_BACK1;

	s_controls.model.bitmap.generic.type      = MTYPE_BITMAP;
	s_controls.model.bitmap.generic.flags     = QMF_INACTIVE;
	s_controls.model.bitmap.generic.ownerdraw = Controls_DrawPlayer;
	s_controls.model.bitmap.generic.x	        = PLAYERMODEL_X;
	s_controls.model.bitmap.generic.y	        = PLAYERMODEL_Y;
	s_controls.model.bitmap.width	            = PLAYERMODEL_WIDTH;
	s_controls.model.bitmap.height            = PLAYERMODEL_HEIGHT;

	s_controls.walkforward.generic.type	     = MTYPE_ACTION;
	s_controls.walkforward.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.walkforward.generic.callback  = Controls_ActionEvent;
	s_controls.walkforward.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.walkforward.generic.id 	     = ID_FORWARD;

	s_controls.backpedal.generic.type	   = MTYPE_ACTION;
	s_controls.backpedal.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.backpedal.generic.callback  = Controls_ActionEvent;
	s_controls.backpedal.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.backpedal.generic.id 	   = ID_BACKPEDAL;

	s_controls.stepleft.generic.type	  = MTYPE_ACTION;
	s_controls.stepleft.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepleft.generic.callback  = Controls_ActionEvent;
	s_controls.stepleft.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.stepleft.generic.id 		  = ID_MOVELEFT;

	s_controls.stepright.generic.type	   = MTYPE_ACTION;
	s_controls.stepright.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepright.generic.callback  = Controls_ActionEvent;
	s_controls.stepright.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.stepright.generic.id        = ID_MOVERIGHT;

	s_controls.moveup.generic.type	    = MTYPE_ACTION;
	s_controls.moveup.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.moveup.generic.callback  = Controls_ActionEvent;
	s_controls.moveup.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.moveup.generic.id        = ID_MOVEUP;

	s_controls.movedown.generic.type	  = MTYPE_ACTION;
	s_controls.movedown.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.movedown.generic.callback  = Controls_ActionEvent;
	s_controls.movedown.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.movedown.generic.id        = ID_MOVEDOWN;

	s_controls.turnleft.generic.type	  = MTYPE_ACTION;
	s_controls.turnleft.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.turnleft.generic.callback  = Controls_ActionEvent;
	s_controls.turnleft.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.turnleft.generic.id        = ID_LEFT;

	s_controls.turnright.generic.type	   = MTYPE_ACTION;
	s_controls.turnright.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.turnright.generic.callback  = Controls_ActionEvent;
	s_controls.turnright.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.turnright.generic.id        = ID_RIGHT;

	s_controls.sidestep.generic.type	  = MTYPE_ACTION;
	s_controls.sidestep.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.sidestep.generic.callback  = Controls_ActionEvent;
	s_controls.sidestep.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.sidestep.generic.id        = ID_STRAFE;

	s_controls.run.generic.type	     = MTYPE_ACTION;
	s_controls.run.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.run.generic.callback  = Controls_ActionEvent;
	s_controls.run.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.run.generic.id        = ID_SPEED;

	s_controls.chainsaw.generic.type	  = MTYPE_ACTION;
	s_controls.chainsaw.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chainsaw.generic.callback  = Controls_ActionEvent;
	s_controls.chainsaw.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chainsaw.generic.id        = ID_WEAPON1;

	s_controls.machinegun.generic.type	    = MTYPE_ACTION;
	s_controls.machinegun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.machinegun.generic.callback  = Controls_ActionEvent;
	s_controls.machinegun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.machinegun.generic.id        = ID_WEAPON2;

	s_controls.shotgun.generic.type	     = MTYPE_ACTION;
	s_controls.shotgun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.shotgun.generic.callback  = Controls_ActionEvent;
	s_controls.shotgun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.shotgun.generic.id        = ID_WEAPON3;

	s_controls.grenadelauncher.generic.type	     = MTYPE_ACTION;
	s_controls.grenadelauncher.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.grenadelauncher.generic.callback  = Controls_ActionEvent;
	s_controls.grenadelauncher.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.grenadelauncher.generic.id        = ID_WEAPON4;

	s_controls.rocketlauncher.generic.type	    = MTYPE_ACTION;
	s_controls.rocketlauncher.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.rocketlauncher.generic.callback  = Controls_ActionEvent;
	s_controls.rocketlauncher.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.rocketlauncher.generic.id        = ID_WEAPON5;

	s_controls.lightning.generic.type	   = MTYPE_ACTION;
	s_controls.lightning.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lightning.generic.callback  = Controls_ActionEvent;
	s_controls.lightning.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.lightning.generic.id        = ID_WEAPON6;

	s_controls.railgun.generic.type	     = MTYPE_ACTION;
	s_controls.railgun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.railgun.generic.callback  = Controls_ActionEvent;
	s_controls.railgun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.railgun.generic.id        = ID_WEAPON7;

	s_controls.plasma.generic.type	    = MTYPE_ACTION;
	s_controls.plasma.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.plasma.generic.callback  = Controls_ActionEvent;
	s_controls.plasma.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.plasma.generic.id        = ID_WEAPON8;

	s_controls.bfg.generic.type	     = MTYPE_ACTION;
	s_controls.bfg.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.bfg.generic.callback  = Controls_ActionEvent;
	s_controls.bfg.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.bfg.generic.id        = ID_WEAPON9;
	
	s_controls.hook.generic.type	     = MTYPE_ACTION;
	s_controls.hook.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.hook.generic.callback  = Controls_ActionEvent;
	s_controls.hook.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.hook.generic.id        = ID_WEAPON10;
	
	s_controls.nailgun.generic.type	     = MTYPE_ACTION;
	s_controls.nailgun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.nailgun.generic.callback  = Controls_ActionEvent;
	s_controls.nailgun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.nailgun.generic.id        = ID_WEAPON11;
	
	s_controls.prox.generic.type	     = MTYPE_ACTION;
	s_controls.prox.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.prox.generic.callback  = Controls_ActionEvent;
	s_controls.prox.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.prox.generic.id        = ID_WEAPON12;
	
	s_controls.chaingun.generic.type	     = MTYPE_ACTION;
	s_controls.chaingun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chaingun.generic.callback  = Controls_ActionEvent;
	s_controls.chaingun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chaingun.generic.id        = ID_WEAPON13;
	
	s_controls.flamethrower.generic.type	     = MTYPE_ACTION;
	s_controls.flamethrower.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.flamethrower.generic.callback  = Controls_ActionEvent;
	s_controls.flamethrower.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.flamethrower.generic.id        = ID_WEAPON14;
	
	s_controls.darkflare.generic.type	     = MTYPE_ACTION;
	s_controls.darkflare.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.darkflare.generic.callback  = Controls_ActionEvent;
	s_controls.darkflare.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.darkflare.generic.id        = ID_WEAPON15;
	

	s_controls.attack.generic.type	    = MTYPE_ACTION;
	s_controls.attack.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.attack.generic.callback  = Controls_ActionEvent;
	s_controls.attack.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.attack.generic.id        = ID_ATTACK;

	s_controls.prevweapon.generic.type	    = MTYPE_ACTION;
	s_controls.prevweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.prevweapon.generic.callback  = Controls_ActionEvent;
	s_controls.prevweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.prevweapon.generic.id        = ID_WEAPPREV;

	s_controls.nextweapon.generic.type	    = MTYPE_ACTION;
	s_controls.nextweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.nextweapon.generic.callback  = Controls_ActionEvent;
	s_controls.nextweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.nextweapon.generic.id        = ID_WEAPNEXT;

	s_controls.lookup.generic.type	    = MTYPE_ACTION;
	s_controls.lookup.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lookup.generic.callback  = Controls_ActionEvent;
	s_controls.lookup.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.lookup.generic.id        = ID_LOOKUP;

	s_controls.lookdown.generic.type	  = MTYPE_ACTION;
	s_controls.lookdown.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lookdown.generic.callback  = Controls_ActionEvent;
	s_controls.lookdown.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.lookdown.generic.id        = ID_LOOKDOWN;

	s_controls.mouselook.generic.type	   = MTYPE_ACTION;
	s_controls.mouselook.generic.flags     = QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.mouselook.generic.callback  = Controls_ActionEvent;
	s_controls.mouselook.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.mouselook.generic.id        = ID_MOUSELOOK;

	s_controls.freelook.generic.type		= MTYPE_RADIOBUTTON;
	s_controls.freelook.generic.flags		= QMF_SMALLFONT;
	s_controls.freelook.generic.x			= SCREEN_WIDTH/2;
	if(cl_language.integer == 0){
	s_controls.freelook.generic.name		= "free look";
	}
	if(cl_language.integer == 1){
	s_controls.freelook.generic.name		= "свободный обзор";	
	}
	s_controls.freelook.generic.id			= ID_FREELOOK;
	s_controls.freelook.generic.callback	= Controls_MenuEvent;
	s_controls.freelook.generic.statusbar	= Controls_StatusBar;

	s_controls.centerview.generic.type	    = MTYPE_ACTION;
	s_controls.centerview.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.centerview.generic.callback  = Controls_ActionEvent;
	s_controls.centerview.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.centerview.generic.id        = ID_CENTERVIEW;

	s_controls.zoomview.generic.type	  = MTYPE_ACTION;
	s_controls.zoomview.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.zoomview.generic.callback  = Controls_ActionEvent;
	s_controls.zoomview.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.zoomview.generic.id        = ID_ZOOMVIEW;

	s_controls.useitem.generic.type	     = MTYPE_ACTION;
	s_controls.useitem.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.useitem.generic.callback  = Controls_ActionEvent;
	s_controls.useitem.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.useitem.generic.id        = ID_USEITEM;

	s_controls.showscores.generic.type	    = MTYPE_ACTION;
	s_controls.showscores.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.showscores.generic.callback  = Controls_ActionEvent;
	s_controls.showscores.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.showscores.generic.id        = ID_SHOWSCORES;

	s_controls.invertmouse.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.invertmouse.generic.flags	 = QMF_SMALLFONT;
	s_controls.invertmouse.generic.x	     = SCREEN_WIDTH/2;
	if(cl_language.integer == 0){
	s_controls.invertmouse.generic.name	     = "invert mouse";
	}
	if(cl_language.integer == 1){
	s_controls.invertmouse.generic.name	     = "инвертировать мышь";
	}
	s_controls.invertmouse.generic.id        = ID_INVERTMOUSE;
	s_controls.invertmouse.generic.callback  = Controls_MenuEvent;
	s_controls.invertmouse.generic.statusbar = Controls_StatusBar;

	s_controls.mousestyle.generic.type      = MTYPE_SPINCONTROL;
	s_controls.mousestyle.generic.flags	 = QMF_SMALLFONT;
	s_controls.mousestyle.generic.x	     = SCREEN_WIDTH/2;
	if(cl_language.integer == 0){
	s_controls.mousestyle.generic.name	     = "input method";
	}
	if(cl_language.integer == 1){
	s_controls.mousestyle.generic.name	     = "метод ввода";
	}
	s_controls.mousestyle.generic.id        = ID_MOUSESTYLE;
	s_controls.mousestyle.generic.callback  = Controls_MenuEvent;
	s_controls.mousestyle.generic.statusbar = Controls_StatusBar;
	s_controls.mousestyle.itemnames = mousestyle_description;

	s_controls.smoothmouse.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.smoothmouse.generic.flags	 = QMF_SMALLFONT;
	s_controls.smoothmouse.generic.x	     = SCREEN_WIDTH/2;
	if(cl_language.integer == 0){
	s_controls.smoothmouse.generic.name	     = "smooth mouse";
	}
	if(cl_language.integer == 1){
	s_controls.smoothmouse.generic.name	     = "плавная мышь";
	}
	s_controls.smoothmouse.generic.id        = ID_SMOOTHMOUSE;
	s_controls.smoothmouse.generic.callback  = Controls_MenuEvent;
	s_controls.smoothmouse.generic.statusbar = Controls_StatusBar;

	s_controls.alwaysrun.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.alwaysrun.generic.flags	   = QMF_SMALLFONT;
	s_controls.alwaysrun.generic.x	       = SCREEN_WIDTH/2;
	if(cl_language.integer == 0){
	s_controls.alwaysrun.generic.name	   = "always run";
	}
	if(cl_language.integer == 1){
	s_controls.alwaysrun.generic.name	   = "всегда бег";
	}
	s_controls.alwaysrun.generic.id        = ID_ALWAYSRUN;
	s_controls.alwaysrun.generic.callback  = Controls_MenuEvent;
	s_controls.alwaysrun.generic.statusbar = Controls_StatusBar;

	s_controls.sensitivity.generic.type	     = MTYPE_SLIDER;
	s_controls.sensitivity.generic.x		 = SCREEN_WIDTH/2;
	s_controls.sensitivity.generic.flags	 = QMF_SMALLFONT;
	if(cl_language.integer == 0){
	s_controls.sensitivity.generic.name	     = "mouse speed";
	}
	if(cl_language.integer == 1){
	s_controls.sensitivity.generic.name	     = "чувствительность";
	}
	s_controls.sensitivity.generic.id 	     = ID_MOUSESPEED;
	s_controls.sensitivity.generic.callback  = Controls_MenuEvent;
	s_controls.sensitivity.minvalue		     = 2;
	s_controls.sensitivity.maxvalue		     = 30;
	s_controls.sensitivity.generic.statusbar = Controls_StatusBar;

	s_controls.gesture.generic.type	     = MTYPE_ACTION;
	s_controls.gesture.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.gesture.generic.callback  = Controls_ActionEvent;
	s_controls.gesture.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.gesture.generic.id        = ID_GESTURE;

	s_controls.chat.generic.type	  = MTYPE_ACTION;
	s_controls.chat.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat.generic.callback  = Controls_ActionEvent;
	s_controls.chat.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat.generic.id        = ID_CHAT;

	s_controls.chat2.generic.type	   = MTYPE_ACTION;
	s_controls.chat2.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat2.generic.callback  = Controls_ActionEvent;
	s_controls.chat2.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat2.generic.id        = ID_CHAT2;

	s_controls.chat3.generic.type	   = MTYPE_ACTION;
	s_controls.chat3.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat3.generic.callback  = Controls_ActionEvent;
	s_controls.chat3.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat3.generic.id        = ID_CHAT3;

	s_controls.chat4.generic.type	   = MTYPE_ACTION;
	s_controls.chat4.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat4.generic.callback  = Controls_ActionEvent;
	s_controls.chat4.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat4.generic.id        = ID_CHAT4;

	s_controls.botmenu.generic.type	   = MTYPE_ACTION;
	s_controls.botmenu.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.botmenu.generic.callback  = Controls_ActionEvent;
	s_controls.botmenu.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.botmenu.generic.id        = ID_BOTMENU;
	
	s_controls.flashlight.generic.type	   = MTYPE_ACTION;
	s_controls.flashlight.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.flashlight.generic.callback  = Controls_ActionEvent;
	s_controls.flashlight.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.flashlight.generic.id        = ID_FLASHLIGHT;
	
	s_controls.thirdperson.generic.type	   = MTYPE_ACTION;
	s_controls.thirdperson.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.thirdperson.generic.callback  = Controls_ActionEvent;
	s_controls.thirdperson.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.thirdperson.generic.id        = ID_THIRDPERSON;
	
	s_controls.dweapon.generic.type	   = MTYPE_ACTION;
	s_controls.dweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.dweapon.generic.callback  = Controls_ActionEvent;
	s_controls.dweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.dweapon.generic.id        = ID_DWEAPON;
	
	s_controls.dholdable.generic.type	   = MTYPE_ACTION;
	s_controls.dholdable.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.dholdable.generic.callback  = Controls_ActionEvent;
	s_controls.dholdable.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.dholdable.generic.id        = ID_DHOLDABLE;
	
	s_controls.acc.generic.type	   = MTYPE_ACTION;
	s_controls.acc.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.acc.generic.callback  = Controls_ActionEvent;
	s_controls.acc.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.acc.generic.id        = ID_ACC;
	
	s_controls.clrun.generic.type	   = MTYPE_ACTION;
	s_controls.clrun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.clrun.generic.callback  = Controls_ActionEvent;
	s_controls.clrun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.clrun.generic.id        = ID_CLRUN;
	
	s_controls.sandbox.generic.type	   = MTYPE_ACTION;
	s_controls.sandbox.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.sandbox.generic.callback  = Controls_ActionEvent;
	s_controls.sandbox.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.sandbox.generic.id        = ID_SANDBOX;
	
	s_controls.sandboxmode.generic.type	   = MTYPE_ACTION;
	s_controls.sandboxmode.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.sandboxmode.generic.callback  = Controls_ActionEvent;
	s_controls.sandboxmode.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.sandboxmode.generic.id        = ID_SANDBOXMODE;
	
	s_controls.newsandbox.generic.type	   = MTYPE_ACTION;
	s_controls.newsandbox.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.newsandbox.generic.callback  = Controls_ActionEvent;
	s_controls.newsandbox.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.newsandbox.generic.id        = ID_NEWSANDBOX;
	
	s_controls.exitvehicle.generic.type	   = MTYPE_ACTION;
	s_controls.exitvehicle.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.exitvehicle.generic.callback  = Controls_ActionEvent;
	s_controls.exitvehicle.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.exitvehicle.generic.id        = ID_EXITVEHICLE;

	s_controls.joyenable.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.joyenable.generic.flags	   = QMF_SMALLFONT;
	s_controls.joyenable.generic.x	       = SCREEN_WIDTH/2;
	if(cl_language.integer == 0){
	s_controls.joyenable.generic.name	   = "joystick";
	}
	if(cl_language.integer == 1){
	s_controls.joyenable.generic.name	   = "геймпад";
	}
	s_controls.joyenable.generic.id        = ID_JOYENABLE;
	s_controls.joyenable.generic.callback  = Controls_MenuEvent;
	s_controls.joyenable.generic.statusbar = Controls_StatusBar;

	s_controls.joythreshold.generic.type	  = MTYPE_SLIDER;
	s_controls.joythreshold.generic.x		  = SCREEN_WIDTH/2;
	s_controls.joythreshold.generic.flags	  = QMF_SMALLFONT;
	if(cl_language.integer == 0){
	s_controls.joythreshold.generic.name	  = "joystick threshold";
	}
	if(cl_language.integer == 1){
	s_controls.joythreshold.generic.name	  = "порог геймпада";
	}
	s_controls.joythreshold.generic.id 	      = ID_JOYTHRESHOLD;
	s_controls.joythreshold.generic.callback  = Controls_MenuEvent;
	s_controls.joythreshold.minvalue		  = 0.05f;
	s_controls.joythreshold.maxvalue		  = 0.75f;
	s_controls.joythreshold.generic.statusbar = Controls_StatusBar;

	s_controls.name.generic.type	= MTYPE_PTEXT;
	s_controls.name.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_controls.name.generic.x		= 320;
	s_controls.name.generic.y		= 440;
	s_controls.name.string			= playername;
	s_controls.name.style			= UI_CENTER;
	s_controls.name.color			= text_color_normal;

	Menu_AddItem( &s_controls.menu, &s_controls.banner );
	Menu_AddItem( &s_controls.menu, &s_controls.framel );
	Menu_AddItem( &s_controls.menu, &s_controls.framer );
	Menu_AddItem( &s_controls.menu, &s_controls.model.bitmap );
	Menu_AddItem( &s_controls.menu, &s_controls.name );

	Menu_AddItem( &s_controls.menu, &s_controls.looking );
	Menu_AddItem( &s_controls.menu, &s_controls.movement );
	Menu_AddItem( &s_controls.menu, &s_controls.weapons );
	Menu_AddItem( &s_controls.menu, &s_controls.input );
	Menu_AddItem( &s_controls.menu, &s_controls.misc );

	Menu_AddItem( &s_controls.menu, &s_controls.sensitivity );
	Menu_AddItem( &s_controls.menu, &s_controls.mousestyle );
	Menu_AddItem( &s_controls.menu, &s_controls.smoothmouse );
	Menu_AddItem( &s_controls.menu, &s_controls.invertmouse );
	Menu_AddItem( &s_controls.menu, &s_controls.joyenable );
	Menu_AddItem( &s_controls.menu, &s_controls.joythreshold );

	Menu_AddItem( &s_controls.menu, &s_controls.lookup );
	Menu_AddItem( &s_controls.menu, &s_controls.lookdown );
	Menu_AddItem( &s_controls.menu, &s_controls.mouselook );
	Menu_AddItem( &s_controls.menu, &s_controls.freelook );
	Menu_AddItem( &s_controls.menu, &s_controls.centerview );
	Menu_AddItem( &s_controls.menu, &s_controls.zoomview );

	Menu_AddItem( &s_controls.menu, &s_controls.alwaysrun );
	Menu_AddItem( &s_controls.menu, &s_controls.run );

	Menu_AddItem( &s_controls.menu, &s_controls.walkforward );
	Menu_AddItem( &s_controls.menu, &s_controls.backpedal );
	Menu_AddItem( &s_controls.menu, &s_controls.stepleft );
	Menu_AddItem( &s_controls.menu, &s_controls.stepright );
	Menu_AddItem( &s_controls.menu, &s_controls.moveup );
	Menu_AddItem( &s_controls.menu, &s_controls.movedown );
	Menu_AddItem( &s_controls.menu, &s_controls.turnleft );
	Menu_AddItem( &s_controls.menu, &s_controls.turnright );
	Menu_AddItem( &s_controls.menu, &s_controls.sidestep );

	Menu_AddItem( &s_controls.menu, &s_controls.attack );
	Menu_AddItem( &s_controls.menu, &s_controls.nextweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.prevweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.chainsaw );
	Menu_AddItem( &s_controls.menu, &s_controls.machinegun );
	Menu_AddItem( &s_controls.menu, &s_controls.shotgun );
	Menu_AddItem( &s_controls.menu, &s_controls.grenadelauncher );
	Menu_AddItem( &s_controls.menu, &s_controls.rocketlauncher );
	Menu_AddItem( &s_controls.menu, &s_controls.lightning );
	Menu_AddItem( &s_controls.menu, &s_controls.railgun );
	Menu_AddItem( &s_controls.menu, &s_controls.plasma );
	Menu_AddItem( &s_controls.menu, &s_controls.bfg );
	Menu_AddItem( &s_controls.menu, &s_controls.hook );
	Menu_AddItem( &s_controls.menu, &s_controls.nailgun );
	Menu_AddItem( &s_controls.menu, &s_controls.prox );
	Menu_AddItem( &s_controls.menu, &s_controls.chaingun );
	Menu_AddItem( &s_controls.menu, &s_controls.flamethrower );
	Menu_AddItem( &s_controls.menu, &s_controls.darkflare );

	Menu_AddItem( &s_controls.menu, &s_controls.showscores );
	Menu_AddItem( &s_controls.menu, &s_controls.useitem );
	Menu_AddItem( &s_controls.menu, &s_controls.gesture );
	Menu_AddItem( &s_controls.menu, &s_controls.chat );
	Menu_AddItem( &s_controls.menu, &s_controls.chat2 );
	Menu_AddItem( &s_controls.menu, &s_controls.chat3 );
	Menu_AddItem( &s_controls.menu, &s_controls.chat4 );
	Menu_AddItem( &s_controls.menu, &s_controls.botmenu );
	Menu_AddItem( &s_controls.menu, &s_controls.flashlight );
	Menu_AddItem( &s_controls.menu, &s_controls.thirdperson );
	Menu_AddItem( &s_controls.menu, &s_controls.dweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.dholdable );
	Menu_AddItem( &s_controls.menu, &s_controls.acc );
	Menu_AddItem( &s_controls.menu, &s_controls.clrun );
	Menu_AddItem( &s_controls.menu, &s_controls.sandbox );
	Menu_AddItem( &s_controls.menu, &s_controls.sandboxmode );
	Menu_AddItem( &s_controls.menu, &s_controls.newsandbox );
	Menu_AddItem( &s_controls.menu, &s_controls.exitvehicle );

	Menu_AddItem( &s_controls.menu, &s_controls.back );

	trap_Cvar_VariableStringBuffer( "name", s_controls.name.string, 32 );
	Q_CleanStr( s_controls.name.string );

	// initialize the configurable cvars
	Controls_InitCvars();

	// initialize the current config
	Controls_GetConfig();

	// intialize the model
	Controls_InitModel();

	// intialize the weapons
	Controls_InitWeapons ();

	// initial default section
	s_controls.section = C_LOOKING;

	// update the ui
	Controls_Update();
}


/*
=================
Controls_Cache
=================
*/
void Controls_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
=================
UI_ControlsMenu
=================
*/
void UI_ControlsMenu( void ) {
	Controls_MenuInit();
	UI_PushMenu( &s_controls.menu );
}
