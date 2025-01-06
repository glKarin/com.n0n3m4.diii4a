// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=============================================================================

START SERVER MENU HEADER (Q3 UI INTERFACE SPECIFIC) *****

=============================================================================
*/


/*
	This file should only contain info and data that is used
	to draw/update or interact with the Q3 user interface.

	Put UI INDEPENDENT stuff into ui_startserver.h
*/



#include "ui_startserver.h"



#define GAMESERVER_BACK0 "menu/art/back_0"
#define GAMESERVER_BACK1 "menu/art/back_1"
#define GAMESERVER_FIGHT0 "menu/art/fight_0"
#define GAMESERVER_FIGHT1 "menu/art/fight_1"
#define GAMESERVER_SERVER0 "menu/uie_art/server_0"
#define GAMESERVER_SERVER1 "menu/uie_art/server_1"
#define GAMESERVER_WEAPONS0 "menu/uie_art/weapons_0"
#define GAMESERVER_WEAPONS1 "menu/uie_art/weapons_1"
#define GAMESERVER_ITEMS0 "menu/uie_art/items_0"
#define GAMESERVER_ITEMS1 "menu/uie_art/items_1"
#define GAMESERVER_BOTS0 "menu/uie_art/bots_0"
#define GAMESERVER_BOTS1 "menu/uie_art/bots_1"
#define GAMESERVER_MAPS0 "menu/uie_art/maps_0"
#define GAMESERVER_MAPS1 "menu/uie_art/maps_1"
#define GAMESERVER_VARROWS "menu/art/arrows_vert_0"
#define GAMESERVER_UP "menu/art/arrows_vert_top"
#define GAMESERVER_DOWN "menu/art/arrows_vert_bot"
#define GAMESERVER_DEL0 "menu/uie_art/del_0"
#define GAMESERVER_DEL1 "menu/uie_art/del_1"
#define GAMESERVER_SELECTED0 "menu/art/switch_off"
#define GAMESERVER_SELECTED1 "menu/art/switch_on"
#define GAMESERVER_ACTION0 "menu/uie_art/action0"
#define GAMESERVER_ACTION1 "menu/uie_art/action1"



#define FRAME_LEFT "menu/art/frame2_l"
#define FRAME_RIGHT "menu/art/frame1_r"
#define FRAME_SPLASH "menu/art/cut_frame"
#define FRAME_EXCLUDED "menu/uie_art/excluded"


// control id's are grouped for ease of allocation
// < 100 are common to all pages
#define ID_SERVERCOMMON_BACK 	10
#define ID_SERVERCOMMON_FIGHT 	11
#define ID_SERVERCOMMON_SERVER 	12
#define ID_SERVERCOMMON_ITEMS 	13
#define ID_SERVERCOMMON_BOTS 	14
#define ID_SERVERCOMMON_MAPS 	15
#define ID_SERVERCOMMON_WEAPON 	16


#define COLUMN_LEFT (SMALLCHAR_WIDTH * 18)
#define COLUMN_RIGHT (320 + (SMALLCHAR_WIDTH * 20))

#define LINE_HEIGHT (SMALLCHAR_HEIGHT + 2)

#define GAMETYPECOLUMN_X 400
#define GAMETYPEICON_X (GAMETYPECOLUMN_X - (20 * SMALLCHAR_WIDTH))
#define GAMETYPEROW_Y (64 + (LINE_HEIGHT/2))


#define MAPPAGE_TEXT 40


#define MAPSELECT_SELECT	"menu/uie_art/mapfocus"
#define MAP_FADETIME 1000


#define TABCONTROLCENTER_Y (240 + LINE_HEIGHT) + 20



typedef void (*CtrlCallback_t)(void * self, int event);


// all the controls common to menus
typedef struct commoncontrols_s {
	menubitmap_s back;
	menubitmap_s fight;
	menubitmap_s server;
	menubitmap_s weapon;
	menubitmap_s items;
	menubitmap_s maps;
	menubitmap_s bots;

	menutext_s servertext;
	menutext_s itemtext;
	menutext_s maptext;
	menutext_s bottext;
	menutext_s weapontext;

} commoncontrols_t;



enum { COMMONCTRL_BOTS, COMMONCTRL_MAPS, COMMONCTRL_SERVER, COMMONCTRL_ITEMS, COMMONCTRL_WEAPON };



// global data

extern vec4_t pulsecolor;
extern vec4_t fading_red;





//
// declarations
//

// ui_startserver_botsel.c
void UI_BotSelect_Index( char *bot , int index);

// ui_startserver_mapsel.c
void UI_StartMapMenu( int gametype, int index, const char* mapname);


// ui_startserver_common.c
qboolean StartServer_CheckFightReady(commoncontrols_t* c);
void StartServer_CommonControls_Init(menuframework_s* ,commoncontrols_t* ,CtrlCallback_t ,int );
void StartServer_BackgroundDraw(qboolean excluded);
void StartServer_SelectionDraw(void* self );



// ui_startserver_map.c
void StartServer_MapPage_MenuInit(void);
void StartServer_MapPage_Cache( void );
void StartServer_MapPage_CopyCustomLimitsToControls(void);

// ui_startserver_bot.c
void StartServer_BotPage_MenuInit(void);
void StartServer_BotPage_Cache( void );

// ui_startserver_server.c
void StartServer_ServerPage_MenuInit(void);
void StartServer_ServerPage_Mods(void);
void StartServer_ServerPage_Cache( void );

// ui_startserver_item.c
void StartServer_BothItemMenus_Cache( void );
void StartServer_ItemPage_MenuInit(void);
