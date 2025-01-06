// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

INGAME MENU

=======================================================================
*/





#include "ui_local.h"
#include "ui_dynamicmenu.h"


#define INGAME_FRAME		"menu/art/addbotframe"
#define INGAME_SCROLL		"menu/uie_art/separator"
//#define INGAME_FRAME					"menu/art/cut_frame"
#define INGAME_MENU_VERTICAL_SPACING	26

#define MAX_INGAME_SCROLLS 		6
#define SCROLL_HEIGHT			16

#define ID_TEAM					10
#define ID_ADDBOTS				11
#define ID_REMOVEBOTS			12
#define ID_SETUP				13
#define ID_SERVERINFO			14
#define ID_LEAVEARENA			15
#define ID_RESTART				16
#define ID_QUIT					17
#define ID_RESUME				18
#define ID_TEAMORDERS			19
#define ID_NEXTMAP				20
#define ID_ENABLEDITEMS			21


typedef struct {
	menuframework_s	menu;

	menubitmap_s	frame;
	menutext_s		team;
	menutext_s		setup;
	menutext_s		server;
	menutext_s		leave;
	menutext_s		restart;
	menutext_s		addbots;
	menutext_s		removebots;
	menutext_s		teamorders;
	menutext_s		quit;
	menutext_s		resume;
	menutext_s		nextmap;
	menutext_s		enableditems;

	int num_scrolls;
	int scroll_y[MAX_INGAME_SCROLLS];
} ingamemenu_t;

static ingamemenu_t	s_ingame;




/*
=================
UI_CurrentPlayerTeam
=================
*/
int UI_CurrentPlayerTeam( void )
{
	static uiClientState_t	cs;
	static char	info[MAX_INFO_STRING];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	return atoi(Info_ValueForKey(info, "t"));
}



/*
=================
InGame_RestartAction
=================
*/
static void InGame_RestartAction( qboolean result ) {
	if( !result ) {
		return;
	}

	UI_ForceMenuOff();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}


/*
=================
InGame_QuitAction
=================
*/
static void InGame_QuitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_ForceMenuOff();
	UI_CreditMenu(0);
}


/*
=================
InGame_NextMap
=================
*/
static void InGame_NextMap( qboolean result )
{
	int gametype;
	char	info[MAX_INFO_STRING];
	int i;
	int numPlayers;

	if( !result ) {
		return;
	}
	UI_ForceMenuOff();

	gametype = DynamicMenu_ServerGametype();
	if (gametype == GT_TOURNAMENT)
	{
		trap_Cmd_ExecuteText( EXEC_INSERT, "set activeAction \"");

		trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
		numPlayers = atoi( Info_ValueForKey( info, "sv_maxclients" ) );

		// try and move all bots to spectator mode
		for( i = 0; i < numPlayers; i++ ) {
			trap_GetConfigString( CS_PLAYERS + i, info, MAX_INFO_STRING );

			if (!atoi( Info_ValueForKey( info, "skill" ) ))
				continue;

			trap_Cmd_ExecuteText( EXEC_INSERT, va("forceteam %i spectator;", i));
		}
		trap_Cmd_ExecuteText( EXEC_INSERT, "\"\n");
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "vstr nextmap\n");
}



/*
=================
InGame_EventHandler

May be used by dynamic menu system also
=================
*/
static void InGame_EventHandler(int id)
{
	switch( id ) {
	case ID_TEAM:
		UI_TeamMainMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_LEAVEARENA:
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		break;

	case ID_RESTART:
		if(cl_language.integer == 0){
		UI_ConfirmMenu( "RESTART ARENA?", 0, InGame_RestartAction );
		}
		if(cl_language.integer == 1){
		UI_ConfirmMenu( "РЕСТАРТ АРЕНЫ?", 0, InGame_RestartAction );
		}
		break;

	case ID_QUIT:
		if(cl_language.integer == 0){
		UI_ConfirmMenu( "EXIT GAME?", 0, InGame_QuitAction );
		}
		if(cl_language.integer == 1){
		UI_ConfirmMenu( "ВЫЙТИ ИЗ ИГРЫ?", 0, InGame_QuitAction );
		}
		break;

	case ID_SERVERINFO:
		UI_ServerInfoMenu();
		break;

	case ID_ADDBOTS:
		UI_AddBotsMenu();
		break;

	case ID_REMOVEBOTS:
		UI_RemoveBotsMenu(RBM_KICKBOT);
		break;

	case ID_TEAMORDERS:
		UI_BotCommandMenu_f();
		break;

	case ID_RESUME:
		UI_ForceMenuOff();
		break;

	case ID_NEXTMAP:
		if(cl_language.integer == 0){
		UI_ConfirmMenu( "NEXT MAP?", 0, InGame_NextMap);
		}
		if(cl_language.integer == 1){
		UI_ConfirmMenu( "СЛЕДУЮШАЯ КАРТА?", 0, InGame_NextMap);
		}
		break;

	case ID_ENABLEDITEMS:
		UIE_InGame_EnabledItems();
		break;
	}
}




/*
=================
InGame_Event
=================
*/
static void InGame_Event( void *ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	InGame_EventHandler(((menucommon_s*)ptr)->id );
}



/*
=================
InGame_MenuDraw
=================
*/
static void InGame_MenuDraw(void)
{
	int i;

	for (i = 0; i < s_ingame.num_scrolls; i++) {
		UI_DrawNamedPic(320 - 64, s_ingame.scroll_y[i],  128, SCROLL_HEIGHT, INGAME_SCROLL);
	}

	// draw the controls
	Menu_Draw(&s_ingame.menu);
}



/*
=================
InGame_MenuInit
=================
*/
void InGame_MenuInit( void ) {
	int		y;
	int gametype;

	memset( &s_ingame, 0 ,sizeof(ingamemenu_t) );

	InGame_Cache();

	gametype = DynamicMenu_ServerGametype();
	s_ingame.menu.wrapAround = qtrue;
	s_ingame.menu.native = qfalse;
	s_ingame.menu.fullscreen = qfalse;
	s_ingame.menu.draw = InGame_MenuDraw;

	s_ingame.frame.generic.type			= MTYPE_BITMAP;
	s_ingame.frame.generic.flags		= QMF_INACTIVE;
	s_ingame.frame.generic.name			= INGAME_FRAME;
	s_ingame.frame.generic.x			= 320-233;
	s_ingame.frame.generic.y			= 232-196;
	s_ingame.frame.width				= 466;
	s_ingame.frame.height				= 396;

	//y = 96;
	y = 50;
	s_ingame.team.generic.type			= MTYPE_PTEXT;
	s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.team.generic.x				= 320;
	s_ingame.team.generic.y				= y;
	s_ingame.team.generic.id			= ID_TEAM;
	s_ingame.team.generic.callback		= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.team.string				= "START";
	}
	if(cl_language.integer == 1){
	s_ingame.team.string				= "СТАРТ";
	}
	s_ingame.team.color					= color_white;
	s_ingame.team.style					= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.addbots.generic.type		= MTYPE_PTEXT;
	s_ingame.addbots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.addbots.generic.x			= 320;
	s_ingame.addbots.generic.y			= y;
	s_ingame.addbots.generic.id			= ID_ADDBOTS;
	s_ingame.addbots.generic.callback	= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.addbots.string				= "ADD BOTS";
	}
	if(cl_language.integer == 1){
	s_ingame.addbots.string				= "ДОБАВИТЬ БОТОВ";
	}
	s_ingame.addbots.color				= color_white;
	s_ingame.addbots.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" ) ) {
		s_ingame.addbots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.removebots.generic.type		= MTYPE_PTEXT;
	s_ingame.removebots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.removebots.generic.x			= 320;
	s_ingame.removebots.generic.y			= y;
	s_ingame.removebots.generic.id			= ID_REMOVEBOTS;
	s_ingame.removebots.generic.callback	= InGame_Event; 
	if(cl_language.integer == 0){
	s_ingame.removebots.string				= "REMOVE BOTS";
	}
	if(cl_language.integer == 1){
	s_ingame.removebots.string				= "УДАЛИТЬ БОТОВ";
	}
	s_ingame.removebots.color				= color_white;
	s_ingame.removebots.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" ) ) {
		s_ingame.removebots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.teamorders.generic.type		= MTYPE_PTEXT;
	s_ingame.teamorders.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.teamorders.generic.x			= 320;
	s_ingame.teamorders.generic.y			= y;
	s_ingame.teamorders.generic.id			= ID_TEAMORDERS;
	s_ingame.teamorders.generic.callback	= InGame_Event; 
	if(cl_language.integer == 0){
	s_ingame.teamorders.string				= "TEAM ORDERS";
	}
	if(cl_language.integer == 1){
	s_ingame.teamorders.string				= "КОМАНДНЫЕ ПРИКАЗЫ";
	}
	s_ingame.teamorders.color				= color_white;
	s_ingame.teamorders.style				= UI_CENTER|UI_SMALLFONT;
	if( !(gametype >= GT_TEAM) ) {
		s_ingame.teamorders.generic.flags |= QMF_GRAYED;
	}
	else if( UI_CurrentPlayerTeam() == TEAM_SPECTATOR ) {
		s_ingame.teamorders.generic.flags |= QMF_GRAYED;
	}
	
	if(gametype == GT_LMS) {
	s_ingame.teamorders.generic.flags |= QMF_GRAYED;	
	}

	y += INGAME_MENU_VERTICAL_SPACING;
    s_ingame.scroll_y[ s_ingame.num_scrolls++ ] = y;

    y += SCROLL_HEIGHT + 2;
	s_ingame.setup.generic.type			= MTYPE_PTEXT;
	s_ingame.setup.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.setup.generic.x			= 320;
	s_ingame.setup.generic.y			= y;
	s_ingame.setup.generic.id			= ID_SETUP;
	s_ingame.setup.generic.callback		= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.setup.string				= "SETUP";
	}
	if(cl_language.integer == 1){
	s_ingame.setup.string				= "НАСТРОЙКИ";
	}
	s_ingame.setup.color				= color_white;
	s_ingame.setup.style				= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.server.generic.type		= MTYPE_PTEXT;
	s_ingame.server.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.server.generic.x			= 320;
	s_ingame.server.generic.y			= y;
	s_ingame.server.generic.id			= ID_SERVERINFO;
	s_ingame.server.generic.callback	= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.server.string				= "SERVER INFO";
	}
	if(cl_language.integer == 1){
	s_ingame.server.string				= "ИНФОРМАЦИЯ О СЕРВЕРЕ";
	}
	s_ingame.server.color				= color_white;
	s_ingame.server.style				= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
    s_ingame.scroll_y[ s_ingame.num_scrolls++ ] = y;

    y += SCROLL_HEIGHT + 2;
	s_ingame.enableditems.generic.type			= MTYPE_PTEXT;
	s_ingame.enableditems.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.enableditems.generic.x			= 320;
	s_ingame.enableditems.generic.y			= y;
	s_ingame.enableditems.generic.id			= ID_ENABLEDITEMS;
	s_ingame.enableditems.generic.callback		= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.enableditems.string				= "DISABLE ITEMS";
	}
	if(cl_language.integer == 1){
	s_ingame.enableditems.string				= "ОТКЛЮЧЕНИЕ ПРЕДМЕТОВ";
	}
	s_ingame.enableditems.color				= color_white;
	s_ingame.enableditems.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) )
	{
		s_ingame.enableditems.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.restart.generic.type		= MTYPE_PTEXT;
	s_ingame.restart.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.restart.generic.x			= 320;
	s_ingame.restart.generic.y			= y;
	s_ingame.restart.generic.id			= ID_RESTART;
	s_ingame.restart.generic.callback	= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.restart.string				= "RESTART ARENA";
	}
	if(cl_language.integer == 1){
	s_ingame.restart.string				= "РЕСТАРТ АРЕНЫ";
	}
	s_ingame.restart.color				= color_white;
	s_ingame.restart.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) ) {
		s_ingame.restart.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.nextmap.generic.type		= MTYPE_PTEXT;
	s_ingame.nextmap.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.nextmap.generic.x			= 320;
	s_ingame.nextmap.generic.y			= y;
	s_ingame.nextmap.generic.id			= ID_NEXTMAP;
	s_ingame.nextmap.generic.callback	= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.nextmap.string				= "NEXT MAP";
	}
	if(cl_language.integer == 1){
	s_ingame.nextmap.string				= "СЛЕДУЮШАЯ КАРТА";
	}
	s_ingame.nextmap.color				= color_white;
	s_ingame.nextmap.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) ) {
		s_ingame.nextmap.generic.flags |= QMF_GRAYED;
	}
	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.resume.generic.type			= MTYPE_PTEXT;
	s_ingame.resume.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.resume.generic.x				= 320;
	s_ingame.resume.generic.y				= y;
	s_ingame.resume.generic.id				= ID_RESUME;
	s_ingame.resume.generic.callback		= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.resume.string					= "RESUME GAME";
	}
	if(cl_language.integer == 1){
	s_ingame.resume.string					= "ПРОДОЛЖИТЬ ИГРУ";
	}
	s_ingame.resume.color					= color_white;
	s_ingame.resume.style					= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
    s_ingame.scroll_y[ s_ingame.num_scrolls++ ] = y;

    y += SCROLL_HEIGHT + 2;
	s_ingame.leave.generic.type			= MTYPE_PTEXT;
	s_ingame.leave.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.leave.generic.x			= 320;
	s_ingame.leave.generic.y			= y;
	s_ingame.leave.generic.id			= ID_LEAVEARENA;
	s_ingame.leave.generic.callback		= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.leave.string				= "LEAVE ARENA";
	}
	if(cl_language.integer == 1){
	s_ingame.leave.string				= "ПОКИНУТЬ АРЕНУ";
	}
	s_ingame.leave.color				= color_white;
	s_ingame.leave.style				= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.quit.generic.type			= MTYPE_PTEXT;
	s_ingame.quit.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.quit.generic.x				= 320;
	s_ingame.quit.generic.y				= y;
	s_ingame.quit.generic.id			= ID_QUIT;
	s_ingame.quit.generic.callback		= InGame_Event;
	if(cl_language.integer == 0){
	s_ingame.quit.string				= "EXIT GAME";
	}
	if(cl_language.integer == 1){
	s_ingame.quit.string				= "ВЫЙТИ ИЗ ИГРЫ";
	}
	s_ingame.quit.color					= color_white;
	s_ingame.quit.style					= UI_CENTER|UI_SMALLFONT;

	Menu_AddItem( &s_ingame.menu, &s_ingame.frame );
	Menu_AddItem( &s_ingame.menu, &s_ingame.team );
	Menu_AddItem( &s_ingame.menu, &s_ingame.addbots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.removebots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.teamorders );
	Menu_AddItem( &s_ingame.menu, &s_ingame.setup );
	Menu_AddItem( &s_ingame.menu, &s_ingame.server );
	Menu_AddItem( &s_ingame.menu, &s_ingame.enableditems );
	Menu_AddItem( &s_ingame.menu, &s_ingame.restart );
	Menu_AddItem( &s_ingame.menu, &s_ingame.nextmap );
	Menu_AddItem( &s_ingame.menu, &s_ingame.resume );
	Menu_AddItem( &s_ingame.menu, &s_ingame.leave );
	Menu_AddItem( &s_ingame.menu, &s_ingame.quit );
}


/*
=================
InGame_Cache
=================
*/
void InGame_Cache( void ) {
	trap_R_RegisterShaderNoMip( INGAME_FRAME );
}



/*
=======================================================================

INGAME ESCAPE MENU, USING DYNAMIC MENU SYSTEM

=======================================================================
*/


typedef struct {
	int 	gametype;
	char* 	menu;
} gametypeMenu;



static gametypeMenu gametypeMenu_data[] = {
	{ GT_SANDBOX, "SandBox"},
	{ GT_FFA, "DeathMatch"},
	{ GT_SINGLE, "Single"},
	{ GT_TOURNAMENT, "Tournament"},
	{ GT_TEAM, "Team DM"},
	{ GT_CTF, "CTF"},
	{ GT_1FCTF, "One Flag"},
	{ GT_OBELISK, "Overload"},
	{ GT_HARVESTER, "Harvester"},
	{ GT_ELIMINATION, "Elimination"},
	{ GT_CTF_ELIMINATION, "CTF Elimination"},
	{ GT_LMS, "Last Man Standing"},
	{ GT_DOUBLE_D, "2 Domination"},
	{ GT_DOMINATION, "Domination"},
};
static gametypeMenu gametypeMenu_dataru[] = {
	{ GT_SANDBOX, "Песочница"},
	{ GT_FFA, "Все против всех"},
	{ GT_SINGLE, "Одиночная игра"},
	{ GT_TOURNAMENT, "Турнир"},
	{ GT_TEAM, "Командный бой"},
	{ GT_CTF, "Захват флага"},
	{ GT_1FCTF, "Один флаг"},
	{ GT_OBELISK, "Атака базы"},
	{ GT_HARVESTER, "Жнец"},
	{ GT_ELIMINATION, "Устранение"},
	{ GT_CTF_ELIMINATION, "CTF Устранение"},
	{ GT_LMS, "Последний оставшийся"},
	{ GT_DOUBLE_D, "2 доминирование"},
	{ GT_DOMINATION, "Доминирование"},
};


static int gametypeMenu_size = sizeof(gametypeMenu_data)/sizeof(gametypeMenu_data[0]);



// main dynamic in game menu
enum {
	IGM_CLOSE,
	IGM_ACTIONS,
	IGM_SAVE,
	IGM_START,
	IGM_BOTS,
	IGM_TEAMORDERS,
	IGM_SETUP,
	IGM_MAP,
	IGM_VOTE,
	IGM_CALLVOTE,
	IGM_EXIT
};



// callvote misc options
enum {
	CVM_NEXTMAP,
	CVM_MAPRESTART
};


// callvote options
enum {
	IGCV_KICK,
	IGCV_MAP,
	IGCV_LEADER,
	IGCV_GAMETYPE,
	IGCV_MISC
};


// vote options
enum {
	IGV_YES,
	IGV_NO,
	IGV_TEAMYES,
	IGV_TEAMNO
};


// setup options
enum {
	IGS_PLAYER,
	IGS_MODEL,
	IGS_CONTROLS,
	IGS_OPTIONS,
	IGS_GRAPHICS,
	IGS_DISPLAY,
	IGS_SOUND,
	IGS_NETWORK,
	IGS_ADVANCED
};

// actions options
enum {
	IGS_RECORD,
	IGS_STOPRECORD,
	IGS_KILL,
	IGS_THIRDPERSON,
	IGS_FLASHLIGHT,
	IGS_EXITVEHICLE,
	IGS_DROPWEAPON,
	IGS_DROPHOLDABLE,
	IGS_HUD
};

// save options
enum {
	IGS_SAVE1,
	IGS_SAVE2,
	IGS_SAVE3,
	IGS_SAVE4,
	IGS_SAVE5,
	IGS_SAVE6,
	IGS_SAVE7,
	IGS_SAVE8
};


// join team options
enum {
	DM_START_SPECTATOR,
	DM_START_GAME,
	DM_START_RED,
	DM_START_BLUE,
	DM_START_AUTO,
	DM_START_FOLLOW1,
	DM_START_FOLLOW2,

	DM_START_MAX
};

static char* jointeam_cmd[DM_START_MAX] = {
	"spectator",	// DM_START_SPECTATOR
	"free",	// DM_START_GAME
	"red",	// DM_START_RED
	"blue",	// DM_START_BLUE
	"auto",	// DM_START_AUTO
	"follow1",	// DM_START_FOLLOW1
	"follow2"	// DM_START_FOLLOW2
};





/*
=================
InGameDynamic_Close
=================
*/
static void InGameDynamic_Close( void )
{
//	UI_PopMenu();
}





/*
=================
IG_FragLimit_Event
=================
*/
static void IG_FragLimit_Event( int index )
{
	int id;

	id = DynamicMenu_IdAtIndex(index);
	UI_ForceMenuOff();

	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote fraglimit %i\n",id));
}


/*
=================
IG_TimeLimit_Event
=================
*/
static void IG_TimeLimit_Event( int index )
{
	int id;

	id = DynamicMenu_IdAtIndex(index);
	UI_ForceMenuOff();

	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote timelimit %i\n",id));
}



/*
=================
IG_DoWarmup_Event
=================
*/
static void IG_DoWarmup_Event( int index )
{
	int id;

	id = DynamicMenu_IdAtIndex(index);
	UI_ForceMenuOff();

	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote g_doWarmup %i\n",id));
}



/*
=================
IG_UseOldInGame_Event
=================
*/
static void IG_UseOldInGame_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	InGameDynamic_Close();

	switch (id) {
	case ID_ADDBOTS:
	case ID_REMOVEBOTS:
	case ID_RESTART:
	case ID_LEAVEARENA:
	case ID_QUIT:
	case ID_ENABLEDITEMS:
	case ID_NEXTMAP:
		break;
	default:
		Com_Printf("IG_UseOldInGame_Event: unknown id (%i)", id);
		return;
	}

	InGame_EventHandler(id);
}




/*
=================
IG_Setup_Event
=================
*/
static void IG_Setup_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	InGameDynamic_Close();

	switch (id) {
	case IGS_PLAYER:
		UI_PlayerSettingsMenu();
		break;
	case IGS_MODEL:
		UI_PlayerModelMenu();
		break;
	case IGS_CONTROLS:
		UI_ControlsMenu();
		break;
	case IGS_OPTIONS:
		UI_PreferencesMenu();
		break;
	case IGS_GRAPHICS:
		UI_GraphicsOptionsMenu();
		break;
	case IGS_DISPLAY:
		UI_DisplayOptionsMenu();
		break;
	case IGS_SOUND:
		UI_SoundOptionsMenu();
		break;
	case IGS_NETWORK:
		UI_NetworkOptionsMenu();
		break;
	case IGS_ADVANCED:
		UI_AdvancedMenu();
		break;
	default:
		Com_Printf("IG_Setup_Event: unknown id (%i)", id);
		return;
	}
}

/*
=================
IG_Actions_Event
=================
*/
static void IG_Actions_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	InGameDynamic_Close();

	switch (id) {
	case IGS_RECORD:
		trap_Cmd_ExecuteText( EXEC_APPEND, "record \n");
		break;
	case IGS_STOPRECORD:
		trap_Cmd_ExecuteText( EXEC_APPEND, "stoprecord \n");
		break;
	case IGS_KILL:
		trap_Cmd_ExecuteText( EXEC_APPEND, "kill \n");
		break;
	case IGS_THIRDPERSON:
		trap_Cmd_ExecuteText( EXEC_APPEND, "toggle cg_thirdperson \n");
		break;
	case IGS_FLASHLIGHT:
		trap_Cmd_ExecuteText( EXEC_APPEND, "flashlight \n");
		break;
	case IGS_EXITVEHICLE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "exitvehicle \n");
		break;
	case IGS_DROPWEAPON:
		trap_Cmd_ExecuteText( EXEC_APPEND, "dropweapon \n");
		break;
	case IGS_DROPHOLDABLE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "dropholdable \n");
		break;
	case IGS_HUD:
		trap_Cmd_ExecuteText( EXEC_APPEND, "toggle cg_draw2D \n");
		break;

	default:
		Com_Printf("IG_Actions_Event: unknown id (%i)", id);
		return;
	}
}

/*
=================
IG_Save_Event
=================
*/
static void IG_Save_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	InGameDynamic_Close();

	switch (id) {
	case IGS_SAVE1:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 1 \n");
		break;
	case IGS_SAVE2:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 2 \n");
		break;
	case IGS_SAVE3:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 3 \n");
		break;
	case IGS_SAVE4:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 4 \n");
		break;
	case IGS_SAVE5:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 5 \n");
		break;
	case IGS_SAVE6:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 6 \n");
		break;
	case IGS_SAVE7:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 7 \n");
		break;
	case IGS_SAVE8:
		trap_Cmd_ExecuteText( EXEC_APPEND, "savegame 8 \n");
		break;
	default:
		Com_Printf("IG_Save_Event: unknown id (%i)", id);
		return;
	}
}




/*
=================
IG_CallVoteGameType_Event
=================
*/
static void IG_CallVoteGameType_Event( int index )
{
	int id;

	id = DynamicMenu_IdAtIndex(index);
	UI_ForceMenuOff();

	switch (id) {
	case GT_SANDBOX:
	case GT_FFA:
	case GT_SINGLE:
	case GT_TOURNAMENT:
	case GT_TEAM:
	case GT_CTF:
	case GT_1FCTF:
	case GT_OBELISK:
	case GT_HARVESTER:
	case GT_ELIMINATION:
	case GT_CTF_ELIMINATION:
	case GT_LMS:
	case GT_DOUBLE_D:
	case GT_DOMINATION:
		break;

	default:
		Com_Printf("IG_CallVoteGameType_Event: unknown game-id (%i)", id);
		return;
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote g_gametype %i\n",id));
}



/*
=================
IG_CallVoteMisc_Event
=================
*/
static void IG_CallVoteMisc_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	UI_ForceMenuOff();

	switch (id) {
	case CVM_NEXTMAP:
		s = "nextmap";
		break;
	case CVM_MAPRESTART:
		s = "map_restart";
		break;
	default:
		Com_Printf("IG_CallVote_Event: unknown id (%i)", id);
		return;
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote %s\n",s));
}




/*
=================
IG_CallVote_Event
=================
*/
static void IG_CallVote_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	InGameDynamic_Close();

	switch (id) {
	case IGCV_KICK:
		UI_RemoveBotsMenu(RBM_CALLVOTEKICK);
		break;
	case IGCV_MAP:
		UI_MapCallVote();
		break;
	case IGCV_LEADER:
		UI_RemoveBotsMenu(RBM_CALLVOTELEADER);
		break;
	default:
		Com_Printf("IG_CallVote_Event: unknown id (%i)", id);
		return;
	}
}




/*
=================
IG_Vote_Event
=================
*/
static void IG_Vote_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	InGameDynamic_Close();

	switch (id) {
	case IGV_YES:
		s = "vote yes\n";
		break;
	case IGV_NO:
		s = "vote no\n";
		break;
	case IGV_TEAMYES:
		s = "teamvote yes\n";
		break;
	case IGV_TEAMNO:
		s = "teamvote yes\n";
		break;
	default:
		Com_Printf("IG_Vote_Event: unknown id (%i)", id);
		return;
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, s);
}



/*
=================
IG_TeamOrders_Event
=================
*/
static void IG_TeamOrders_Event( int index )
{
	UI_PopMenu();
	UI_BotCommandMenu_f();
}


/*
=================
IG_Start_Event
=================
*/
static void IG_Start_Event( int index )
{
	int id;
	char* s;

	id = DynamicMenu_IdAtIndex(index);
	UI_ForceMenuOff();

	if (id < 0 || id >= DM_START_MAX) {
		Com_Printf("IG_Start_Event: unknown id (%i)", id);
		return;
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, va("team %s\n", jointeam_cmd[id]));
}





/*
=================
IG_TimeLimit_SubMenu
=================
*/
static void IG_TimeLimit_SubMenu( void )
{
	int depth;

	DynamicMenu_SubMenuInit();
	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Unlimited", 0, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("5", 5, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("10", 10, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("15", 15, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("20", 20, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("25", 25, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("30", 30, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("45", 45, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("60", 60, 0, IG_TimeLimit_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Бесконечно", 0, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("5", 5, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("10", 10, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("15", 15, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("20", 20, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("25", 25, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("30", 30, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("45", 45, 0, IG_TimeLimit_Event);
	DynamicMenu_AddItem("60", 60, 0, IG_TimeLimit_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_FragLimit_SubMenu
=================
*/
static void IG_FragLimit_SubMenu( void )
{
	int depth;

	DynamicMenu_SubMenuInit();
	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Unlimited", 0, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("10", 10, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("15", 15, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("20", 20, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("30", 30, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("40", 40, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("50", 50, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("75", 75, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("100", 100, 0, IG_FragLimit_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Бесконечно", 0, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("10", 10, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("15", 15, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("20", 20, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("30", 30, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("40", 40, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("50", 50, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("75", 75, 0, IG_FragLimit_Event);
	DynamicMenu_AddItem("100", 100, 0, IG_FragLimit_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}


/*
=================
IG_DoWarmup_SubMenu
=================
*/
static void IG_DoWarmup_SubMenu( void )
{
	int depth;

	DynamicMenu_SubMenuInit();
	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Enable", 1, 0, IG_DoWarmup_Event);
	DynamicMenu_AddItem("Disable", 0, 0, IG_DoWarmup_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Включить", 1, 0, IG_DoWarmup_Event);
	DynamicMenu_AddItem("Отключить", 0, 0, IG_DoWarmup_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_Map_SubMenu
=================
*/
static void IG_Map_SubMenu( void )
{
	int depth;

	DynamicMenu_SubMenuInit();
	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Restart map...", ID_RESTART, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Disable items...", ID_ENABLEDITEMS, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Next map...", ID_NEXTMAP, 0, IG_UseOldInGame_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Рестарт карты...", ID_RESTART, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Отключение предметов...", ID_ENABLEDITEMS, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Следущая карта...", ID_NEXTMAP, 0, IG_UseOldInGame_Event);
	}

	depth = DynamicMenu_Depth();

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_CallVoteMisc_SubMenu
=================
*/
static void IG_CallVoteMisc_SubMenu( void )
{
	DynamicMenu_SubMenuInit();
	
	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Next map", CVM_NEXTMAP, 0, IG_CallVoteMisc_Event);
	DynamicMenu_AddItem("Map restart", CVM_MAPRESTART, 0, IG_CallVoteMisc_Event);
	DynamicMenu_AddItem("Warmup", 0, IG_DoWarmup_SubMenu, 0);
	DynamicMenu_AddItem("Timelimit", 0, IG_TimeLimit_SubMenu, 0);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Следующая карта", CVM_NEXTMAP, 0, IG_CallVoteMisc_Event);
	DynamicMenu_AddItem("Рестарт карты", CVM_MAPRESTART, 0, IG_CallVoteMisc_Event);
	DynamicMenu_AddItem("Разминка", 0, IG_DoWarmup_SubMenu, 0);
	DynamicMenu_AddItem("Лимит времени", 0, IG_TimeLimit_SubMenu, 0);
	}

	if (DynamicMenu_ServerGametype() == GT_CTF) {
		// No CTF caplimit, callvote doesn't support it
	}
	else {
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("FragLimit", 0, IG_FragLimit_SubMenu, 0);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Лимит фрагов", 0, IG_FragLimit_SubMenu, 0);
		}
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_CallVoteMisc_SubMenu
=================
*/
static void IG_CallVoteGameType_SubMenu( void )
{
	int gametype;
	int i;
	int depth;
	const char* icon;

	gametype = DynamicMenu_ServerGametype();

	DynamicMenu_SubMenuInit();

	depth = DynamicMenu_Depth();
	for (i = 0; i < gametypeMenu_size; i++) {
		icon = UIE_DefaultIconFromGameType(gametypeMenu_data[i].gametype);

if(cl_language.integer == 0){
		DynamicMenu_AddIconItem(gametypeMenu_data[i].menu, gametypeMenu_data[i].gametype,
			icon, 0, IG_CallVoteGameType_Event);
}
if(cl_language.integer == 1){
		DynamicMenu_AddIconItem(gametypeMenu_dataru[i].menu, gametypeMenu_data[i].gametype,
			icon, 0, IG_CallVoteGameType_Event);
}

		if (gametypeMenu_data[i].gametype == gametype)
			DynamicMenu_SetFlags(depth, gametype, QMF_GRAYED);
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_CallVote_SubMenu
=================
*/
static void IG_CallVote_SubMenu( void )
{
	int team;
	int depth;

	team = UI_CurrentPlayerTeam();

	DynamicMenu_SubMenuInit();

	depth = DynamicMenu_Depth();
	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Kick...", IGCV_KICK, 0, IG_CallVote_Event);
	DynamicMenu_AddItem("Map...", IGCV_MAP, 0, IG_CallVote_Event);
	DynamicMenu_AddItem("Leader...", IGCV_LEADER, 0, IG_CallVote_Event);
	DynamicMenu_AddItem("Gametype", IGCV_GAMETYPE, IG_CallVoteGameType_SubMenu, 0);
	DynamicMenu_AddItem("Misc", 0, IG_CallVoteMisc_SubMenu, 0);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Кик...", IGCV_KICK, 0, IG_CallVote_Event);
	DynamicMenu_AddItem("Карта...", IGCV_MAP, 0, IG_CallVote_Event);
	DynamicMenu_AddItem("Лидер...", IGCV_LEADER, 0, IG_CallVote_Event);
	DynamicMenu_AddItem("Режим игры", IGCV_GAMETYPE, IG_CallVoteGameType_SubMenu, 0);
	DynamicMenu_AddItem("Другое", 0, IG_CallVoteMisc_SubMenu, 0);
	}

	if (team == TEAM_SPECTATOR || team == TEAM_FREE) {
		DynamicMenu_SetFlags(depth, IGCV_LEADER, QMF_GRAYED);
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_Vote_SubMenu
=================
*/
static void IG_Vote_SubMenu( void )
{
	char buf[MAX_INFO_STRING];

	DynamicMenu_SubMenuInit();

	trap_GetConfigString(CS_VOTE_TIME, buf, MAX_INFO_STRING);
	if (atoi(buf) != 0) {
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Yes", IGV_YES, 0, IG_Vote_Event);
		DynamicMenu_AddItem("No", IGV_NO, 0, IG_Vote_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Да", IGV_YES, 0, IG_Vote_Event);
		DynamicMenu_AddItem("Нет", IGV_NO, 0, IG_Vote_Event);
		}
	}

	trap_GetConfigString(CS_TEAMVOTE_TIME, buf, MAX_INFO_STRING);
	if (atoi(buf) != 0) {
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Team Yes", IGV_TEAMYES, 0, IG_Vote_Event);
		DynamicMenu_AddItem("Team No", IGV_TEAMNO, 0, IG_Vote_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Команда Да", IGV_TEAMYES, 0, IG_Vote_Event);
		DynamicMenu_AddItem("Команда Нет", IGV_TEAMNO, 0, IG_Vote_Event);
		}
	}


	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_Setup_SubMenu
=================
*/
static void IG_Setup_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Player...", IGS_PLAYER, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Model...", IGS_MODEL, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Controls...", IGS_CONTROLS, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Preferences...", IGS_OPTIONS, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Graphics...", IGS_GRAPHICS, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Display...", IGS_DISPLAY, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Sound...", IGS_SOUND, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Network...", IGS_NETWORK, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Advanced...", IGS_ADVANCED, 0, IG_Setup_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Игрок...", IGS_PLAYER, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Модель...", IGS_MODEL, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Управление...", IGS_CONTROLS, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Параметры...", IGS_OPTIONS, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Графика...", IGS_GRAPHICS, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Экран...", IGS_DISPLAY, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Звук...", IGS_SOUND, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Сеть...", IGS_NETWORK, 0, IG_Setup_Event);
	DynamicMenu_AddItem("Расширеные...", IGS_ADVANCED, 0, IG_Setup_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}

/*
=================
IG_Actions_SubMenu
=================
*/
static void IG_Actions_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Demo record", IGS_RECORD, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Stop record", IGS_STOPRECORD, 0, IG_Actions_Event);
	DynamicMenu_AddItem("I am stuck", IGS_KILL, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Third person", IGS_THIRDPERSON, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Flashlight", IGS_FLASHLIGHT, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Exit vehicle", IGS_EXITVEHICLE, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Drop weapon", IGS_DROPWEAPON, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Drop holdable", IGS_DROPHOLDABLE, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Toggle HUD", IGS_HUD, 0, IG_Actions_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Начать запись", IGS_RECORD, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Остановка записи", IGS_STOPRECORD, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Я застрял", IGS_KILL, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Третье лицо", IGS_THIRDPERSON, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Фонарик", IGS_FLASHLIGHT, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Выход из машины", IGS_EXITVEHICLE, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Кинуть оружие", IGS_DROPWEAPON, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Кинуть предмет", IGS_DROPHOLDABLE, 0, IG_Actions_Event);
	DynamicMenu_AddItem("Скрыть HUD", IGS_HUD, 0, IG_Actions_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}

/*
=================
IG_Save_SubMenu
=================
*/
static void IG_Save_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Save: Slot 1", IGS_SAVE1, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 2", IGS_SAVE2, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 3", IGS_SAVE3, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 4", IGS_SAVE4, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 5", IGS_SAVE5, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 6", IGS_SAVE6, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 7", IGS_SAVE7, 0, IG_Save_Event);
	DynamicMenu_AddItem("Save: Slot 8", IGS_SAVE8, 0, IG_Save_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Сохранить: Слот 1", IGS_SAVE1, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 2", IGS_SAVE2, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 3", IGS_SAVE3, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 4", IGS_SAVE4, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 5", IGS_SAVE5, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 6", IGS_SAVE6, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 7", IGS_SAVE7, 0, IG_Save_Event);
	DynamicMenu_AddItem("Сохранить: Слот 8", IGS_SAVE8, 0, IG_Save_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
IG_AddBot_SubMenu
=================
*/
static void IG_AddBot_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Add bot...", ID_ADDBOTS, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Remove bot...", ID_REMOVEBOTS, 0, IG_UseOldInGame_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Добавить бота...", ID_ADDBOTS, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Удалить бота...", ID_REMOVEBOTS, 0, IG_UseOldInGame_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}





/*
=================
IG_Exit_SubMenu
=================
*/
static void IG_Exit_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Quit...", ID_QUIT, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Main Menu", ID_LEAVEARENA, 0, IG_UseOldInGame_Event);
	}

	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Выход...", ID_QUIT, 0, IG_UseOldInGame_Event);
	DynamicMenu_AddItem("Главное Меню", ID_LEAVEARENA, 0, IG_UseOldInGame_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}




/*
=================
IG_Start_SubMenu
=================
*/
static void IG_Start_SubMenu( void )
{
	int depth;
	int gametype;

	gametype = DynamicMenu_ServerGametype();

	DynamicMenu_SubMenuInit();
	depth = DynamicMenu_Depth();

	if (gametype < GT_TEAM || gametype == GT_LMS) {
		if(cl_language.integer == 0){
		DynamicMenu_AddIconItem("Join Game", DM_START_GAME, "menu/medals/medal_gauntlet", 0, IG_Start_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddIconItem("Присоединиться к Игре", DM_START_GAME, "menu/medals/medal_gauntlet", 0, IG_Start_Event);
		}
	}
	else {
		if(cl_language.integer == 0){
		DynamicMenu_AddIconItem("Auto Join", DM_START_AUTO, "menu/medals/medal_capture", 0, IG_Start_Event);
		DynamicMenu_AddIconItem("Join Red", DM_START_RED, "uie_icons/iconf_red", 0, IG_Start_Event);
		DynamicMenu_AddIconItem("Join Blue", DM_START_BLUE, "uie_icons/iconf_blu", 0, IG_Start_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddIconItem("Авто Присоединение", DM_START_AUTO, "menu/medals/medal_capture", 0, IG_Start_Event);
		DynamicMenu_AddIconItem("Красная команда", DM_START_RED, "uie_icons/iconf_red", 0, IG_Start_Event);
		DynamicMenu_AddIconItem("Синяя команда", DM_START_BLUE, "uie_icons/iconf_blu", 0, IG_Start_Event);
		}
	}

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Spectate", DM_START_SPECTATOR, 0, IG_Start_Event);
	DynamicMenu_AddItem("Follow #1", DM_START_FOLLOW1, 0, IG_Start_Event);
	DynamicMenu_AddItem("Follow #2", DM_START_FOLLOW2, 0, IG_Start_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Наблюдать", DM_START_SPECTATOR, 0, IG_Start_Event);
	DynamicMenu_AddItem("Следить #1", DM_START_FOLLOW1, 0, IG_Start_Event);
	DynamicMenu_AddItem("Следить #2", DM_START_FOLLOW2, 0, IG_Start_Event);
	}

	if (UI_CurrentPlayerTeam() == TEAM_SPECTATOR) {
		DynamicMenu_SetFlags(depth, DM_START_SPECTATOR, QMF_GRAYED);
	}

	DynamicMenu_FinishSubMenuInit();
}




/*
=================
IG_Close_Event
=================
*/
static void IG_Close_Event( int index )
{
	UI_ForceMenuOff();
}



/*
=================
InGameDynamic_InitPrimaryMenu
=================
*/
static void InGameDynamic_InitPrimaryMenu( void )
{
	int depth;
	int gametype;
	int team;
	qboolean localserver;
	qboolean voting;
	char buf[MAX_INFO_STRING];

	team = UI_CurrentPlayerTeam();
	gametype = DynamicMenu_ServerGametype();

	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Close!", IGM_CLOSE, 0, IG_Close_Event);
	if(!ui_singlemode.integer){
	DynamicMenu_AddItem("Actions", IGM_ACTIONS, IG_Actions_SubMenu, 0);
	}
	if(ui_singlemode.integer){
	DynamicMenu_AddItem("Save", IGM_SAVE, IG_Save_SubMenu, 0);	
	}
	DynamicMenu_AddItem("Start", IGM_START, IG_Start_SubMenu, 0);
	DynamicMenu_AddItem("Bots", IGM_BOTS, IG_AddBot_SubMenu, 0);
	DynamicMenu_AddItem("Team Orders...", IGM_TEAMORDERS, 0, IG_TeamOrders_Event);
	DynamicMenu_AddItem("Setup", IGM_SETUP, IG_Setup_SubMenu, 0);
	DynamicMenu_AddItem("Map", IGM_MAP, IG_Map_SubMenu, 0);
	DynamicMenu_AddItem("Vote", IGM_VOTE, IG_Vote_SubMenu, 0);
	DynamicMenu_AddItem("Call Vote", IGM_CALLVOTE, IG_CallVote_SubMenu, 0);
	DynamicMenu_AddItem("Exit!", IGM_EXIT, IG_Exit_SubMenu, 0);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Закрыть!", IGM_CLOSE, 0, IG_Close_Event);
	if(!ui_singlemode.integer){
	DynamicMenu_AddItem("Действия", IGM_ACTIONS, IG_Actions_SubMenu, 0);
	}
	if(ui_singlemode.integer){
	DynamicMenu_AddItem("Сохранить", IGM_SAVE, IG_Save_SubMenu, 0);
	}
	DynamicMenu_AddItem("Старт", IGM_START, IG_Start_SubMenu, 0);
	DynamicMenu_AddItem("Боты", IGM_BOTS, IG_AddBot_SubMenu, 0);
	DynamicMenu_AddItem("Командные Приказы...", IGM_TEAMORDERS, 0, IG_TeamOrders_Event);
	DynamicMenu_AddItem("Настройки", IGM_SETUP, IG_Setup_SubMenu, 0);
	DynamicMenu_AddItem("Карта", IGM_MAP, IG_Map_SubMenu, 0);
	DynamicMenu_AddItem("Голосование", IGM_VOTE, IG_Vote_SubMenu, 0);
	DynamicMenu_AddItem("Создать Голосование", IGM_CALLVOTE, IG_CallVote_SubMenu, 0);
	DynamicMenu_AddItem("Выйти!", IGM_EXIT, IG_Exit_SubMenu, 0);
	}

	depth = DynamicMenu_Depth();
	gametype = trap_Cvar_VariableValue("g_gametype");
	//if (gametype < GT_TEAM || team == TEAM_SPECTATOR || gametype == GT_LMS) {
	//	DynamicMenu_SetFlags(depth, IGM_TEAMORDERS, QMF_GRAYED);
	//}

	// disable map commands if non-local server
	localserver = trap_Cvar_VariableValue( "sv_running" );
	if( !localserver) {
		DynamicMenu_SetFlags(depth, IGM_MAP, QMF_GRAYED);
	}

	// singleplayer/spec protects voting menu (otherwise it could be used to cheat)
	if (team == TEAM_SPECTATOR) {
		DynamicMenu_SetFlags(depth, IGM_CALLVOTE, QMF_GRAYED);
		DynamicMenu_SetFlags(depth, IGM_VOTE, QMF_GRAYED);
	}

	// bot manipulation
	if (!localserver || !trap_Cvar_VariableValue( "bot_enable" )) {
		DynamicMenu_SetFlags(depth, IGM_BOTS, QMF_GRAYED);
	}

	// disable vote menu if no voting
	voting = qfalse;
	trap_GetConfigString(CS_VOTE_TIME, buf, MAX_INFO_STRING);
	if (atoi(buf) != 0) {
		voting = qtrue;
	}
	trap_GetConfigString(CS_TEAMVOTE_TIME, buf, MAX_INFO_STRING);
	if (atoi(buf) != 0) {
		voting = qtrue;
	}

	if (!voting) {
		DynamicMenu_SetFlags(depth, IGM_VOTE, QMF_GRAYED);
	}

	//DynamicMenu_AddBackground(INGAME_FRAME);

	DynamicMenu_FinishSubMenuInit();
}




/*
=================
UI_InGameDynamic
=================
*/
void UI_InGameDynamic( void )
{
	DynamicMenu_MenuInit(qfalse, qtrue);
	InGameDynamic_InitPrimaryMenu();
}




/*
=================
UI_InGameMenu
=================
*/
void UI_InGameMenu( void )
{
	if (uie_ingame_dynamicmenu.integer) {
		UI_InGameDynamic();
	}
	else {
		// force as top level menu
		uis.menusp = 0;

		// set menu cursor to a nice location
		uis.cursorx = 319;
		uis.cursory = 80;

		InGame_MenuInit();
		UI_PushMenu( &s_ingame.menu );
	}
}






/*
=======================================================================

INGAME DYNAMIC BOT COMMAND MENU

=======================================================================
*/


// stores current gametype for fast access by menus
static int botcommandmenu_gametype = 0;



enum {
	COM_WHOLEADER,
	COM_IAMLEADER,
	COM_QUITLEADER,
	COM_MYTASK
} commandId;


static char* commandString[] = {
	"Who is the leader", // COM_WHOLEADER
	"I am the leader",	// COM_IAMLEADER
	"I quit being the leader",	// COM_QUITLEADER
	"What is my job",	// COM_MYTASK
	0
};


enum {
	BC_NULL,
	BC_FOLLOW,
	BC_HELP,
	BC_GET,
	BC_PATROL,
	BC_CAMP,
	BC_HUNT,
	BC_DISMISS,
	BC_REPORT,
	BC_POINT,
	BC_GETFLAG,
	BC_DEFENDBASE,
	BC_ATTACKBASE,
	BC_COLLECTSKULLS,
	BC_DOMINATEA,
	BC_DOMINATEB
} botCommandId;


static char* botCommandStrings[] = {
	"", // BC_NULL
	"%s follow %s", // BC_FOLLOW
	"%s help %s", // BC_HELP
	"%s grab the %s", // BC_GET
	"%s patrol from %s to %s", // BC_PATROL
	"%s camp %s", // BC_CAMP
	"%s kill %s", // BC_HUNT
	"%s dismissed", // BC_DISMISS
	"%s report", // BC_REPORT
	"%s lead the way", // BC_POINT
	"%s capture flag",	// BC_GETFLAG
	"%s defend the base",	// BC_DEFENDBASE
	"%s attack the enemy",	// BC_ATTACKBASE
	"%s collect skulls",	// BC_COLLECTSKULLS
	"%s dominate point A",	// BC_DOMINATEA
	"%s dominate point B",	// BC_DOMINATEB
	0
};







/*
=================
BotCommand_MenuClose
=================
*/
void BotCommand_MenuClose( void )
{
	if (uie_autoclosebotmenu.integer)
		UI_PopMenu();
}





/*
=================
DM_BotPlayerTarget_Event

Issues a command to a bot that needs a target
Assumes index is the object, parent is the command,
and parent of parent is the bot
=================
*/
static void DM_BotPlayerTarget_Event( int index)
{
	int depth;
	int bot, cmd;
	const char* s;

	depth = DynamicMenu_DepthOfIndex(index);

	if (depth != DynamicMenu_Depth() || depth < 3)
	{
		Com_Printf("BotPlayerTarget_Event: index %i at wrong depth (%i)\n", index, depth);
		BotCommand_MenuClose();
		return;
	}

	// validate command
	cmd = DynamicMenu_ActiveIdAtDepth(depth - 1);
	switch (cmd) {
	case BC_FOLLOW:
	case BC_HELP:
	case BC_HUNT:
		break;
	default:
		Com_Printf("BotPlayerTarget_Event: unknown command id %i\n", cmd);
		BotCommand_MenuClose();
		return;
	};

	// get the parent bot, insert it and item into command string
	bot = DynamicMenu_ActiveIndexAtDepth(depth - 2);
	s = va(botCommandStrings[cmd], DynamicMenu_StringAtIndex(bot),
		DynamicMenu_StringAtIndex(index));

	// issue the command
	BotCommand_MenuClose();
	trap_Cmd_ExecuteText( EXEC_APPEND, va("say_team \"%s\"\n", s));
}





/*
=================
DM_BotItemItemTarget_Event

Issues a command to a bot that needs two targets
Assumes index and parent are the objects, grandparent
is the command, and great-grandparent is the bot
=================
*/
static void DM_BotItemItemTarget_Event( int index)
{
	int depth;
	int bot, cmd, item, item2;
	const char* s;

	depth = DynamicMenu_DepthOfIndex(index);

	if (depth != DynamicMenu_Depth() || depth < 4)
	{
		Com_Printf("BotItemItemTarget_Event: index %i at wrong depth (%i)\n", index, depth);
		BotCommand_MenuClose();
		return;
	}

	// validate command
	cmd = DynamicMenu_ActiveIdAtDepth(depth - 2);
	switch (cmd) {
	case BC_PATROL:
		break;
	default:
		Com_Printf("BotItemItemTarget_Event: unknown command id %i\n", cmd);
		BotCommand_MenuClose();
		return;
	};

	// get the parent bot, insert it and item into command string
	bot = DynamicMenu_ActiveIndexAtDepth( depth - 3 );
	item = DynamicMenu_ActiveIdAtDepth(depth - 1);
	item2 = DynamicMenu_IdAtIndex(index);

	s = va(botCommandStrings[cmd], DynamicMenu_StringAtIndex(bot),
		DynamicMenu_ItemShortname(item), DynamicMenu_ItemShortname(item2));

	// issue the command
	BotCommand_MenuClose();
	trap_Cmd_ExecuteText( EXEC_APPEND, va("say_team \"%s\"\n", s));
}




/*
=================
DM_BotItemTarget_Event

Issues a command to a bot that needs a target
Assumes index is the object, parent is the command,
and parent of parent is the bot
=================
*/
static void DM_BotItemTarget_Event( int index)
{
	int depth;
	int bot, cmd, item;
	const char* s;
	const char* item_str;

	depth = DynamicMenu_DepthOfIndex(index);

	if (depth != DynamicMenu_Depth() || depth < 3)
	{
		Com_Printf("BotItemTarget_Event: index %i at wrong depth (%i)\n", index, depth);
		BotCommand_MenuClose();
		return;
	}

	// validate command
	cmd = DynamicMenu_ActiveIdAtDepth(depth - 1);
	switch (cmd) {
	case BC_GET:
	case BC_CAMP:
		break;
	default:
		Com_Printf("BotItemTarget_Event: unknown command id %i\n", cmd);
		BotCommand_MenuClose();
		return;
	};

	// get the parent bot, insert it and item into command string
	bot = DynamicMenu_ActiveIndexAtDepth(depth - 2);
	item = DynamicMenu_IdAtIndex(index);
	if (item < 0)
		item_str = DynamicMenu_StringAtIndex(index);
	else
		item_str = DynamicMenu_ItemShortname(item);

	s = va(botCommandStrings[cmd], DynamicMenu_StringAtIndex(bot), item_str);

	// issue the command
	BotCommand_MenuClose();
	trap_Cmd_ExecuteText( EXEC_APPEND, va("say_team \"%s\"\n", s));
}



/*
=================
DM_BotCommand_Event

Issues a command to a bot
=================
*/
static void DM_BotCommand_Event( int index )
{
	int depth;
	int bot, cmd;
	const char* s;

	depth = DynamicMenu_DepthOfIndex(index);

	if (depth != DynamicMenu_Depth() || depth < 2)
	{
		Com_Printf("BotCommand_Event: index %i at wrong depth (%i)\n", index, depth);
		BotCommand_MenuClose();
		return;
	}

	// validate command
	cmd = DynamicMenu_IdAtIndex(index);
	switch (cmd) {
	case BC_DISMISS:
	case BC_REPORT:
	case BC_POINT:
	case BC_GETFLAG:
	case BC_DEFENDBASE:
	case BC_ATTACKBASE:
	case BC_COLLECTSKULLS:
	case BC_DOMINATEA:
	case BC_DOMINATEB:
		break;
	default:
		Com_Printf("BotCommand_Event: unknown command (%i)\n", cmd);
		BotCommand_MenuClose();
		return;
	};

	// get the parent bot name, insert into command string
	bot = DynamicMenu_ActiveIndexAtDepth(depth - 1);
	s = va(botCommandStrings[cmd], DynamicMenu_StringAtIndex(bot));

	// issue the command
	BotCommand_MenuClose();
	trap_Cmd_ExecuteText( EXEC_APPEND, va("say_team \"%s\"\n", s));
}





/*
=================
DM_Command_Event

Issues a command without target
=================
*/
static void DM_Command_Event( int index )
{
	int depth;
	int cmd;
	const char* s;

	depth = DynamicMenu_DepthOfIndex(index);

	if (depth != DynamicMenu_Depth())
	{
		Com_Printf("Command_Event: index %i at wrong depth (%i)\n", index, depth);
		BotCommand_MenuClose();
		return;
	}

	// validate command
	cmd = DynamicMenu_IdAtIndex(index);
	switch (cmd) {
	case COM_WHOLEADER:
	case COM_IAMLEADER:
	case COM_QUITLEADER:
	case COM_MYTASK:
		break;
	default:
		Com_Printf("Command_Event: unknown command (%i)\n", cmd);
		BotCommand_MenuClose();
		return;
	};

	// issue the command
	BotCommand_MenuClose();
	trap_Cmd_ExecuteText( EXEC_APPEND, va("say_team \"%s\"\n", commandString[cmd]));
}



/*
=================
DM_Close_Event
=================
*/
static void DM_Close_Event( int index )
{
	UI_PopMenu();
}





/*
=================
DM_TeamList_SubMenu
=================
*/
static void DM_TeamList_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	DynamicMenu_AddItem("me", 0, 0, DM_BotPlayerTarget_Event);
	if ( DynamicMenu_ServerGametype() == GT_FFA || DynamicMenu_ServerGametype() == GT_LMS || DynamicMenu_ServerGametype() == GT_SANDBOX ){
	DynamicMenu_AddListOfPlayers(PT_ALL, 0, DM_BotPlayerTarget_Event);
	} else {
	DynamicMenu_AddListOfPlayers(PT_FRIENDLY|PT_EXCLUDEGRANDPARENT, 0, DM_BotPlayerTarget_Event);	
	}

	DynamicMenu_FinishSubMenuInit();
}




/*
=================
DM_ItemPatrol2_SubMenu
=================
*/
static void DM_ItemPatrol2_SubMenu( void )
{
	int exclude;
	int index;
	int depth;

	DynamicMenu_SubMenuInit();

	depth = DynamicMenu_Depth() - 1;
	exclude = DynamicMenu_ActiveIdAtDepth(depth);
//	index = s_dynamic.active[depth - 1];	// previous menu level
//	exclude = s_dynamic.data[index].id;
	DynamicMenu_AddListOfItems(exclude, 0, DM_BotItemItemTarget_Event);

	DynamicMenu_FinishSubMenuInit();
}





/*
=================
DM_ItemPatrol_SubMenu
=================
*/
static void DM_ItemPatrol_SubMenu( void )
{
	DynamicMenu_SubMenuInit();
	DynamicMenu_AddListOfItems(-1, DM_ItemPatrol2_SubMenu, 0);
	DynamicMenu_FinishSubMenuInit();
}





/*
=================
DM_CampItemList_SubMenu
=================
*/
static void DM_CampItemList_SubMenu( void )
{
	DynamicMenu_SubMenuInit();
	DynamicMenu_AddItem("here", -1, 0, DM_BotItemTarget_Event);
	DynamicMenu_AddItem("there", -1, 0, DM_BotItemTarget_Event);
	DynamicMenu_AddListOfItems(-1, 0, DM_BotItemTarget_Event);
	DynamicMenu_FinishSubMenuInit();
}







/*
=================
DM_ItemList_SubMenu
=================
*/
void DM_ItemList_SubMenu( void )
{
	DynamicMenu_SubMenuInit();
	DynamicMenu_AddListOfItems(-1, 0, DM_BotItemTarget_Event);
	DynamicMenu_FinishSubMenuInit();
}





/*
=================
DM_EnemyList_SubMenu
=================
*/
static void DM_EnemyList_SubMenu( void )
{
	DynamicMenu_SubMenuInit();
	if ( DynamicMenu_ServerGametype() == GT_FFA || DynamicMenu_ServerGametype() == GT_LMS || DynamicMenu_ServerGametype() == GT_SANDBOX ){
	DynamicMenu_AddListOfPlayers(PT_ALL, 0, DM_BotPlayerTarget_Event);
	} else {
	DynamicMenu_AddListOfPlayers(PT_ENEMY, 0, DM_BotPlayerTarget_Event);	
	}
	DynamicMenu_FinishSubMenuInit();
}



/*
=================
DM_CommandList_SubMenu
=================
*/
static void DM_CommandList_SubMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Report", BC_REPORT, (createHandler)0, DM_BotCommand_Event);
	DynamicMenu_AddItem("Help", BC_HELP, DM_TeamList_SubMenu, 0);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Репорт", BC_REPORT, (createHandler)0, DM_BotCommand_Event);
	DynamicMenu_AddItem("Помощь", BC_HELP, DM_TeamList_SubMenu, 0);
	}

	if (botcommandmenu_gametype == GT_CTF)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Capture Flag", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Defend Base", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Захват Флага", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Защита Базы", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
	}
	if (botcommandmenu_gametype == GT_CTF_ELIMINATION)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Capture Flag", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Defend Base", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Захват Флага", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Защита Базы", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
	}
	if (botcommandmenu_gametype == GT_1FCTF)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Capture Flag", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Defend Base", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Захват Флага", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Защита Базы", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
	}
	
	if (botcommandmenu_gametype == GT_OBELISK)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Attack Enemy", BC_ATTACKBASE, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Defend Base", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Атакуй Врага", BC_GETFLAG, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Защита Базы", BC_DEFENDBASE, (createHandler)0, DM_BotCommand_Event);
		}
	}
	
	if (botcommandmenu_gametype == GT_HARVESTER)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Collect skulls", BC_COLLECTSKULLS, (createHandler)0, DM_BotCommand_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Собирай черепа", BC_COLLECTSKULLS, (createHandler)0, DM_BotCommand_Event);
		}
	}
	
	if (botcommandmenu_gametype == GT_DOUBLE_D)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("Take A point", BC_DOMINATEA, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Take B point", BC_DOMINATEB, (createHandler)0, DM_BotCommand_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Возьми A точку", BC_DOMINATEA, (createHandler)0, DM_BotCommand_Event);
		DynamicMenu_AddItem("Возьми B точку", BC_DOMINATEB, (createHandler)0, DM_BotCommand_Event);
		}
	}

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Follow", BC_FOLLOW, DM_TeamList_SubMenu, 0);
	DynamicMenu_AddItem("Get", BC_GET, DM_ItemList_SubMenu, 0);
	DynamicMenu_AddItem("Patrol", BC_PATROL, DM_ItemPatrol_SubMenu, 0);
	DynamicMenu_AddItem("Camp", BC_CAMP, DM_CampItemList_SubMenu, 0);
	DynamicMenu_AddItem("Hunt", BC_HUNT, DM_EnemyList_SubMenu, 0);
	DynamicMenu_AddItem("Point+", BC_POINT, (createHandler)0, DM_BotCommand_Event);
	DynamicMenu_AddItem("Dismiss", BC_DISMISS, (createHandler)0, DM_BotCommand_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Следуй", BC_FOLLOW, DM_TeamList_SubMenu, 0);
	DynamicMenu_AddItem("Возьми", BC_GET, DM_ItemList_SubMenu, 0);
	DynamicMenu_AddItem("Патрулируй", BC_PATROL, DM_ItemPatrol_SubMenu, 0);
	DynamicMenu_AddItem("Сиди", BC_CAMP, DM_CampItemList_SubMenu, 0);
	DynamicMenu_AddItem("Охоться", BC_HUNT, DM_EnemyList_SubMenu, 0);
	DynamicMenu_AddItem("Точка+", BC_POINT, (createHandler)0, DM_BotCommand_Event);
	DynamicMenu_AddItem("Отбой", BC_DISMISS, (createHandler)0, DM_BotCommand_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}



/*
=================
BotCommand_InitPrimaryMenu
=================
*/
static void BotCommand_InitPrimaryMenu( void )
{
	DynamicMenu_SubMenuInit();

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Close!", 0, 0, DM_Close_Event);
	DynamicMenu_AddItem("Everyone", 0, DM_CommandList_SubMenu, 0);
	if ( DynamicMenu_ServerGametype() == GT_FFA || DynamicMenu_ServerGametype() == GT_LMS || DynamicMenu_ServerGametype() == GT_SANDBOX ){
	DynamicMenu_AddListOfPlayers(PT_ALL, DM_CommandList_SubMenu, 0);
	} else {
	DynamicMenu_AddListOfPlayers(PT_FRIENDLY|PT_BOTONLY, DM_CommandList_SubMenu, 0);		
	}
	DynamicMenu_AddItem("Leader?", COM_WHOLEADER, 0, DM_Command_Event);
	}
	
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Закрыть!", 0, 0, DM_Close_Event);
	DynamicMenu_AddItem("Everyone", 0, DM_CommandList_SubMenu, 0);
	if ( DynamicMenu_ServerGametype() == GT_FFA || DynamicMenu_ServerGametype() == GT_LMS || DynamicMenu_ServerGametype() == GT_SANDBOX ){
	DynamicMenu_AddListOfPlayers(PT_ALL, DM_CommandList_SubMenu, 0);
	} else {
	DynamicMenu_AddListOfPlayers(PT_FRIENDLY|PT_BOTONLY, DM_CommandList_SubMenu, 0);		
	}
	DynamicMenu_AddItem("Лидер?", COM_WHOLEADER, 0, DM_Command_Event);
	}

	if (botcommandmenu_gametype == GT_CTF)
	{
		if(cl_language.integer == 0){
		DynamicMenu_AddItem("My task?", COM_MYTASK, 0, DM_Command_Event);
		}
		if(cl_language.integer == 1){
		DynamicMenu_AddItem("Моя задача?", COM_MYTASK, 0, DM_Command_Event);
		}
	}

	if(cl_language.integer == 0){
	DynamicMenu_AddItem("Lead", COM_IAMLEADER, (createHandler)0, DM_Command_Event);
	DynamicMenu_AddItem("Resign", COM_QUITLEADER, (createHandler)0, DM_Command_Event);
	}
	if(cl_language.integer == 1){
	DynamicMenu_AddItem("Я лидер", COM_IAMLEADER, (createHandler)0, DM_Command_Event);
	DynamicMenu_AddItem("Я не лидер", COM_QUITLEADER, (createHandler)0, DM_Command_Event);
	}

	DynamicMenu_FinishSubMenuInit();
}







/*
=================
UI_BotCommand_Cache
=================
*/
void UI_BotCommand_Cache( void )
{
}




/*
=================
UI_BotCommandMenu
=================
*/
void UI_BotCommandMenu( void )
{
	if (UI_CurrentPlayerTeam() == TEAM_SPECTATOR)
		return;

	botcommandmenu_gametype = DynamicMenu_ServerGametype();
	//if ( botcommandmenu_gametype< GT_TEAM || botcommandmenu_gametype == GT_LMS)
		//return;

	DynamicMenu_MenuInit(qfalse, qtrue);
	BotCommand_InitPrimaryMenu();
}



/*
=================
UI_BotCommandMenu_f
=================
*/
void UI_BotCommandMenu_f( void )
{
	UI_BotCommandMenu();
}






