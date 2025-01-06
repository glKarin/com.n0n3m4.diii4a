//Copyright (C) 1999-2005 Id Software, Inc.
//
#include "ui_local.h"


#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	

#define VERTICAL_SPACING	30

#define ID_BACK			10
#define ID_CIN_IDLOGO	11
#define ID_CIN_INTRO	12
#define ID_CIN_TIER1	13
#define ID_CIN_TIER2	14
#define ID_CIN_TIER3	15
#define ID_CIN_TIER4	16
#define ID_CIN_TIER5	17
#define ID_CIN_TIER6	18


typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		cin_idlogo;
	menutext_s		cin_intro;
	menutext_s		cin_tier1;
	menutext_s		cin_tier2;
	menutext_s		cin_tier3;
	menutext_s		cin_tier4;
	menutext_s		cin_tier5;
	menutext_s		cin_tier6;
	menubitmap_s	back;
} cinematicsMenuInfo_t;

static cinematicsMenuInfo_t	cinematicsMenuInfo;

static char *cinematics[] = {
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8"
};

/*
===============
UI_CinematicsMenu_BackEvent
===============
*/
static void UI_CinematicsMenu_BackEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	UI_PopMenu();
}


/*
===============
UI_CinematicsMenu_Event
===============
*/
static void UI_CinematicsMenu_Event( void *ptr, int event ) {
	int		n;

	if (event != QM_ACTIVATED)
		return;

	n = ((menucommon_s*)ptr)->id - ID_CIN_IDLOGO;
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "savegame %s\n", cinematics[n] ) );
	
	UI_PopMenu();
}

/*
===============
UI_CinematicsMenu_Event2
===============
*/
static void UI_CinematicsMenu_Event2( void *ptr, int event ) {
	int		n;

	if (event != QM_ACTIVATED)
		return;

	n = ((menucommon_s*)ptr)->id - ID_CIN_IDLOGO;
	//trap_Cmd_ExecuteText( EXEC_APPEND, va( "savegame %s\n", cinematics[n] ) );
	
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "exec save0%i.cfg\n", atoi(cinematics[n]) ) );
	if(atoi(cinematics[n]) == 1){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save1_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save1_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save1_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save1_curmap" ) );
	}
	if(atoi(cinematics[n]) == 2){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save2_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save2_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save2_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save2_curmap" ) );
	}
	if(atoi(cinematics[n]) == 3){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save3_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save3_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save3_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save3_curmap" ) );
	}
	if(atoi(cinematics[n]) == 4){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save4_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save4_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save4_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save4_curmap" ) );
	}
	if(atoi(cinematics[n]) == 5){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save5_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save5_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save5_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save5_curmap" ) );
	}
	if(atoi(cinematics[n]) == 6){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save6_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save6_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save6_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save6_curmap" ) );
	}
	if(atoi(cinematics[n]) == 7){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save7_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save7_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save7_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save7_curmap" ) );
	}
	if(atoi(cinematics[n]) == 8){
	trap_Cvar_Set( "session", 	"0" );
	trap_Cvar_Set( "session0", 	UI_Cvar_VariableString( "save8_session0" ) );
	trap_Cvar_Set( "session0_lvl", UI_Cvar_VariableString( "save8_session0_lvl" ) );
	trap_Cvar_Set( "epsession", 	UI_Cvar_VariableString( "save8_epsession" ) );
	trap_Cvar_Set( "save_curmap", 	UI_Cvar_VariableString( "save8_curmap" ) );
	}
	UI_StartServerMenu( qtrue );
	StartServer_LoadServer(NULL);
}


/*
===============
UI_CinematicsMenu_Init
===============
*/
static void UI_CinematicsMenu_Init( int load ) {
	int		y;

	UI_CinematicsMenu_Cache();

	memset( &cinematicsMenuInfo, 0, sizeof(cinematicsMenuInfo) );
	cinematicsMenuInfo.menu.fullscreen = qtrue;

	cinematicsMenuInfo.banner.generic.type		= MTYPE_BTEXT;
	cinematicsMenuInfo.banner.generic.x			= 320;
	cinematicsMenuInfo.banner.generic.y			= 16;
	if(cl_language.integer == 0){
	if(!load){
	cinematicsMenuInfo.banner.string			= "SAVE GAME";
	} else {
	cinematicsMenuInfo.banner.string			= "LOAD GAME";
	}
	}
	if(cl_language.integer == 1){
	if(!load){
	cinematicsMenuInfo.banner.string			= "СОХРАНИТЬ ИГРУ";
	} else {
	cinematicsMenuInfo.banner.string			= "ЗАГРУЗИТЬ ИГРУ";
	}
	}
	cinematicsMenuInfo.banner.color				= color_white;
	cinematicsMenuInfo.banner.style				= UI_CENTER;

	y = 100;
	cinematicsMenuInfo.cin_idlogo.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_idlogo.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_idlogo.generic.x			= 320;
	cinematicsMenuInfo.cin_idlogo.generic.y			= y;
	cinematicsMenuInfo.cin_idlogo.generic.id		= ID_CIN_IDLOGO;
	cinematicsMenuInfo.cin_idlogo.string			= "SLOT 1";
	cinematicsMenuInfo.cin_idlogo.color				= color_white;
	cinematicsMenuInfo.cin_idlogo.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_intro.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_intro.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_intro.generic.x			= 320;
	cinematicsMenuInfo.cin_intro.generic.y			= y;
	cinematicsMenuInfo.cin_intro.generic.id			= ID_CIN_INTRO;
	cinematicsMenuInfo.cin_intro.string				= "SLOT 2";
	cinematicsMenuInfo.cin_intro.color				= color_white;
	cinematicsMenuInfo.cin_intro.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier1.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier1.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier1.generic.x			= 320;
	cinematicsMenuInfo.cin_tier1.generic.y			= y;
	cinematicsMenuInfo.cin_tier1.generic.id			= ID_CIN_TIER1; 
	cinematicsMenuInfo.cin_tier1.string				= "SLOT 3";
	cinematicsMenuInfo.cin_tier1.color				= color_white;
	cinematicsMenuInfo.cin_tier1.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier2.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier2.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier2.generic.x			= 320;
	cinematicsMenuInfo.cin_tier2.generic.y			= y;
	cinematicsMenuInfo.cin_tier2.generic.id			= ID_CIN_TIER2;
	cinematicsMenuInfo.cin_tier2.string				= "SLOT 4";
	cinematicsMenuInfo.cin_tier2.color				= color_white;
	cinematicsMenuInfo.cin_tier2.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier3.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier3.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier3.generic.x			= 320;
	cinematicsMenuInfo.cin_tier3.generic.y			= y;
	cinematicsMenuInfo.cin_tier3.generic.id			= ID_CIN_TIER3;
	cinematicsMenuInfo.cin_tier3.string				= "SLOT 5";
	cinematicsMenuInfo.cin_tier3.color				= color_white;
	cinematicsMenuInfo.cin_tier3.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier4.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier4.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier4.generic.x			= 320;
	cinematicsMenuInfo.cin_tier4.generic.y			= y;
	cinematicsMenuInfo.cin_tier4.generic.id			= ID_CIN_TIER4;
	cinematicsMenuInfo.cin_tier4.string				= "SLOT 6";
	cinematicsMenuInfo.cin_tier4.color				= color_white;
	cinematicsMenuInfo.cin_tier4.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier5.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier5.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier5.generic.x			= 320;
	cinematicsMenuInfo.cin_tier5.generic.y			= y;
	cinematicsMenuInfo.cin_tier5.generic.id			= ID_CIN_TIER5;
	cinematicsMenuInfo.cin_tier5.string				= "SLOT 7";
	cinematicsMenuInfo.cin_tier5.color				= color_white;
	cinematicsMenuInfo.cin_tier5.style				= UI_CENTER;

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier6.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier6.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier6.generic.x			= 320;
	cinematicsMenuInfo.cin_tier6.generic.y			= y;
	cinematicsMenuInfo.cin_tier6.generic.id			= ID_CIN_TIER6;	
	cinematicsMenuInfo.cin_tier6.string				= "SLOT 8";
	cinematicsMenuInfo.cin_tier6.color				= color_white;
	cinematicsMenuInfo.cin_tier6.style				= UI_CENTER;
	
	
	if(!load){
	cinematicsMenuInfo.cin_tier6.generic.callback	= UI_CinematicsMenu_Event;
	cinematicsMenuInfo.cin_tier5.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier4.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier3.generic.callback	= UI_CinematicsMenu_Event;
	cinematicsMenuInfo.cin_tier2.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier1.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_intro.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_idlogo.generic.callback	= UI_CinematicsMenu_Event;	
	} else {
	cinematicsMenuInfo.cin_tier6.generic.callback	= UI_CinematicsMenu_Event2;
	cinematicsMenuInfo.cin_tier5.generic.callback	= UI_CinematicsMenu_Event2; 
	cinematicsMenuInfo.cin_tier4.generic.callback	= UI_CinematicsMenu_Event2; 
	cinematicsMenuInfo.cin_tier3.generic.callback	= UI_CinematicsMenu_Event2;
	cinematicsMenuInfo.cin_tier2.generic.callback	= UI_CinematicsMenu_Event2; 
	cinematicsMenuInfo.cin_tier1.generic.callback	= UI_CinematicsMenu_Event2; 
	cinematicsMenuInfo.cin_intro.generic.callback	= UI_CinematicsMenu_Event2; 
	cinematicsMenuInfo.cin_idlogo.generic.callback	= UI_CinematicsMenu_Event2;	
	}
	

	cinematicsMenuInfo.back.generic.type		= MTYPE_BITMAP;
	cinematicsMenuInfo.back.generic.name		= ART_BACK0;
	cinematicsMenuInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.back.generic.id			= ID_BACK;
	cinematicsMenuInfo.back.generic.callback	= UI_CinematicsMenu_BackEvent;
	cinematicsMenuInfo.back.generic.x			= 0 - uis.wideoffset;
	cinematicsMenuInfo.back.generic.y			= 480-64;
	cinematicsMenuInfo.back.width				= 128;
	cinematicsMenuInfo.back.height				= 64;
	cinematicsMenuInfo.back.focuspic			= ART_BACK1;

	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.banner );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_idlogo );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_intro );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier1 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier2 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier3 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier4 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier5 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier6 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.back );
}


/*
=================
UI_CinematicsMenu_Cache
=================
*/
void UI_CinematicsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_CinematicsMenu
===============
*/
void UI_CinematicsMenu( int load ) {
	if(!load){
	UI_CinematicsMenu_Init(0);
	} else {
	UI_CinematicsMenu_Init(1);
	}
	UI_PushMenu( &cinematicsMenuInfo.menu );
}


/*
===============
UI_CinematicsMenu_f
===============
*/
void UI_CinematicsMenu_f( int load ) {
	if(!load){
	UI_CinematicsMenu(0);
	} else {
	UI_CinematicsMenu(1);
	}
}
