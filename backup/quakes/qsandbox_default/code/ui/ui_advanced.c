//Copyright (C) 1999-2005 Id Software, Inc.
//last update: 2023 Dec 11
#include "ui_local.h"

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"	
#define ART_FIGHT0			"menu/art/accept_0"
#define ART_FIGHT1			"menu/art/accept_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"

#define MAX_WSITEMS			256
#define NAMEBUFSIZE			( MAX_WSITEMS * 48 )
#define GAMEBUFSIZE			( MAX_WSITEMS * 16 )

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_SEARCH			13
#define ID_VALUE			14


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		list;
	menufield_s		filter;
	menufield_s		value;

	menubitmap_s	back;
	menubitmap_s	go;

	char			description[NAMEBUFSIZE];
	char			fs_game[GAMEBUFSIZE];

	char			*descriptionPtr;
	char			*fs_gamePtr;

	char			*descriptionList[MAX_WSITEMS];
	char			*fs_gameList[MAX_WSITEMS];
	char*			advanced_itemslist[524288];
} advanced_t;

static advanced_t	s_advanced;


/*
===============
UI_Advanced_MenuEvent
===============
*/
static void UI_Advanced_MenuEvent( void *ptr, int event ) {

	if( event != QM_ACTIVATED ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		trap_Cmd_ExecuteText( EXEC_APPEND, va("set %s \"%s\"\n", s_advanced.list.itemnames[s_advanced.list.curvalue], s_advanced.value.field.buffer) );
		break;
		
	case ID_LIST:
		//trap_Cmd_ExecuteText( EXEC_APPEND, va("unset %s\n", s_advanced.list.itemnames[s_advanced.list.curvalue]) );
		Q_strncpyz( s_advanced.value.field.buffer, UI_Cvar_VariableString(s_advanced.list.itemnames[s_advanced.list.curvalue]), sizeof(s_advanced.value.field.buffer) );
		break;

	case ID_BACK:
		UI_PopMenu();
		UI_LoadArenas();
		//trap_Cmd_ExecuteText( EXEC_APPEND, "game_restart");
		break;
	}
}
static void UI_Advanced_MenuEvent2( void *ptr, int event ) {

	if( event != QM_LOSTFOCUS ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_SEARCH:

		break;
	case ID_VALUE:

		break;
	}
}


char* 			advanced_items[] = {
"cg_leiChibi",
"cl_propsmallsizescale",
"cl_propheight",
"cl_propgapwidth",
"cl_smallcharwidth",
"cl_smallcharheight",
"cl_bigcharwidth",
"cl_bigcharheight",
"cl_giantcharwidth",
"cl_giantcharheight",
"cg_brassTime",
"cg_gibtime",
"cg_gibvelocity",
"cg_gibjump",
"cg_gibmodifier",
"cg_draw2D",
"cg_drawIcons",
"cg_drawRewards",
"cg_crosshairX",
"cg_crosshairY",
"cg_railTrailTime",
"cg_gunX",
"cg_gunY",
"cg_gunZ",
"cg_drawsubtitles",
"cg_runpitch",
"cg_runroll",
"cg_bobup",
"cg_bobpitch",
"cg_bobroll",
"cg_swingSpeed",
"cg_scorePlums",
"cg_alwaysWeaponBar",
"cg_chatTime",
"cg_consoleTime",
"cg_teamChatTime",
"cg_toolguninfo",
"cg_teamChatY",
"cg_chatY",
"cg_newConsole",
"cg_consoleSizeX",
"cg_consoleSizeY",
"cg_chatSizeX",
"cg_chatSizeY",
"cg_teamChatSizeX",
"cg_teamChatSizeY",
"cg_consoleLines",
"cg_commonConsoleLines",
"cg_chatLines",
"cg_teamChatLines",
"cg_noProjectileTrail",
"cg_leiEnhancement",
"cg_leiGoreNoise",
"cg_leiBrassNoise",
"cg_crosshairPulse",
"cg_chatBeep",
"cg_teamChatBeep",
"cg_zoomtime",
"cg_itemscaletime",
"cg_weaponselecttime",
"r_picmip",
"r_overBrightBits",
"r_mapOverBrightBits",
"r_defaultImage",
"r_noborder",
"r_mode",
"r_modeFullscreen",
"r_customPixelAspect",
"r_customWidth",
"r_customHeight",
"cl_mapAutoDownload",
"s_doppler",
"com_yieldCPU",
"handicap"
};


/*
===============
UI_Advanced_ParseInfos
===============
*/
	int advanced_i = 0;
	int advanced_j = 0;
void UI_Advanced_ParseInfos( void ) {
	for (advanced_i = 0; advanced_i < 74; advanced_i++) {
	if(Q_stricmp (s_advanced.filter.field.buffer, "")){
	if ( !Q_stristr( advanced_items[advanced_i], s_advanced.filter.field.buffer ) ) {
		continue;
	}
	}
	s_advanced.list.itemnames[advanced_j] = advanced_items[advanced_i];
	advanced_j += 1;
	}
    s_advanced.list.numitems = advanced_j;
}


/*
===============
UI_Advanced_LoadItemsFromFile
===============
*/
void UI_Advanced_LoadItemsFromFile() {
advanced_i = 0;
advanced_j = 0;
s_advanced.list.curvalue = 0;
s_advanced.list.top      = 0;
UI_Advanced_ParseInfos();
}


/*
=================
Advanced_MenuKey
=================
*/
static sfxHandle_t Advanced_MenuKey( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE ) {

	}
	if( key == K_F5 ) {
	UI_Advanced_LoadItemsFromFile();	
	}
	return Menu_DefaultKey( &s_advanced.menu, key );
}


static void UI_Advanced_Draw( void ) {
	int i;
	float			x, y, w, h;
	vec4_t			color1 = {0.85, 0.9, 1.0, 1};

	Menu_Draw( &s_advanced.menu );
	
	
	if(cl_language.integer == 0){
	UI_DrawString( 320, 3, "Press F5 to search", UI_CENTER|UI_SMALLFONT, color1 );
	}
	if(cl_language.integer == 1){
	UI_DrawString( 320, 3, "Нажмите F5 для поиска", UI_CENTER|UI_SMALLFONT, color1 );
	}
}


/*
===============
UI_Advanced_MenuInit
===============
*/
static void UI_Advanced_MenuInit( void ) {
	UI_AdvancedMenu_Cache();

	memset( &s_advanced, 0 ,sizeof(advanced_t) );
	s_advanced.menu.wrapAround = qtrue;
	s_advanced.menu.native 	   = qfalse;
	s_advanced.menu.fullscreen = qtrue;
	s_advanced.menu.key        = Advanced_MenuKey;
	s_advanced.menu.draw 	   = UI_Advanced_Draw;

	s_advanced.banner.generic.type		= MTYPE_BTEXT;
	s_advanced.banner.generic.x			= 320;
	s_advanced.banner.generic.y			= 16;
	if(cl_language.integer == 0){
	s_advanced.banner.string			= "ADVANCED SETTINGS";
	}
	if(cl_language.integer == 1){
	s_advanced.banner.string			= "РАСШИРЕННЫЕ НАСТРОЙКИ";
	}
	s_advanced.banner.color				= color_white;
	s_advanced.banner.style				= UI_CENTER;

	s_advanced.framel.generic.type		= MTYPE_BITMAP;
	s_advanced.framel.generic.name		= ART_FRAMEL;
	s_advanced.framel.generic.flags		= QMF_INACTIVE;
	s_advanced.framel.generic.x			= 0;  
	s_advanced.framel.generic.y			= 78;
	s_advanced.framel.width				= 256;
	s_advanced.framel.height			= 329;

	s_advanced.framer.generic.type		= MTYPE_BITMAP;
	s_advanced.framer.generic.name		= ART_FRAMER;
	s_advanced.framer.generic.flags		= QMF_INACTIVE;
	s_advanced.framer.generic.x			= 376;
	s_advanced.framer.generic.y			= 76;
	s_advanced.framer.width				= 256;
	s_advanced.framer.height			= 334;

	s_advanced.back.generic.type		= MTYPE_BITMAP;
	s_advanced.back.generic.name		= ART_BACK0;
	s_advanced.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_advanced.back.generic.id			= ID_BACK;
	s_advanced.back.generic.callback	= UI_Advanced_MenuEvent;
	s_advanced.back.generic.x			= 0 - uis.wideoffset;
	s_advanced.back.generic.y			= 480-64;
	s_advanced.back.width				= 128;
	s_advanced.back.height				= 64;
	s_advanced.back.focuspic			= ART_BACK1;

	s_advanced.go.generic.type			= MTYPE_BITMAP;
	s_advanced.go.generic.name			= ART_FIGHT0;
	s_advanced.go.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_advanced.go.generic.id			= ID_GO;
	s_advanced.go.generic.callback		= UI_Advanced_MenuEvent;
	s_advanced.go.generic.x				= 640 + uis.wideoffset;
	s_advanced.go.generic.y				= 480-64;
	s_advanced.go.width					= 128;
	s_advanced.go.height				= 64;
	s_advanced.go.focuspic				= ART_FIGHT1;
	
	s_advanced.filter.generic.type			= MTYPE_FIELD;
	s_advanced.filter.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT||QMF_CENTER_JUSTIFY;
	s_advanced.filter.generic.callback		= UI_Advanced_MenuEvent2;
	s_advanced.filter.generic.id			= ID_SEARCH;
	s_advanced.filter.field.widthInChars	= 16;
	s_advanced.filter.field.maxchars		= 16;
	if(cl_language.integer == 0){
	s_advanced.filter.generic.name			= "Search:";
	}
	if(cl_language.integer == 1){
	s_advanced.filter.generic.name			= "Поиск:";
	}
	s_advanced.filter.generic.x				= 240;
	s_advanced.filter.generic.y				= 430;
	
	s_advanced.value.generic.type			= MTYPE_FIELD;
	s_advanced.value.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT||QMF_CENTER_JUSTIFY;
	s_advanced.value.generic.callback		= UI_Advanced_MenuEvent2;
	s_advanced.value.generic.id				= ID_VALUE;
	s_advanced.value.field.widthInChars		= 20;
	s_advanced.value.field.maxchars			= 128;
	if(cl_language.integer == 0){
	s_advanced.value.generic.name			= "Value:";
	}
	if(cl_language.integer == 1){
	s_advanced.value.generic.name			= "Значение:";
	}
	s_advanced.value.generic.x				= 240;
	s_advanced.value.generic.y				= 450;

	// scan for items
	s_advanced.list.generic.type		= MTYPE_SCROLLLIST;
	s_advanced.list.generic.flags		= QMF_PULSEIFFOCUS|QMF_CENTER_JUSTIFY;
	s_advanced.list.generic.callback	= UI_Advanced_MenuEvent;
	s_advanced.list.generic.id			= ID_LIST;
	s_advanced.list.generic.x			= 320;
	s_advanced.list.generic.y			= 40;
	s_advanced.list.width				= 48;
	s_advanced.list.height				= 32;
	s_advanced.list.itemnames			= (const char **)s_advanced.advanced_itemslist;

	UI_Advanced_LoadItemsFromFile();

	Menu_AddItem( &s_advanced.menu, &s_advanced.banner );
	Menu_AddItem( &s_advanced.menu, &s_advanced.framel );
	Menu_AddItem( &s_advanced.menu, &s_advanced.framer );
	Menu_AddItem( &s_advanced.menu, &s_advanced.list );
	Menu_AddItem( &s_advanced.menu, &s_advanced.back );
	Menu_AddItem( &s_advanced.menu, &s_advanced.go );
	Menu_AddItem( &s_advanced.menu, &s_advanced.filter );
	Menu_AddItem( &s_advanced.menu, &s_advanced.value );
}

/*
=================
UI_Advanced_Cache
=================
*/
void UI_AdvancedMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_AdvancedMenu
===============
*/
void UI_AdvancedMenu( void ) {
	UI_Advanced_MenuInit();
	UI_PushMenu( &s_advanced.menu );
}
