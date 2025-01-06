// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

SETUP MENU

=======================================================================
*/





#include "ui_local.h"


#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_CUSTOMIZEPLAYER		10
#define ID_CUSTOMIZECONTROLS	11
#define ID_SYSTEMCONFIG			12
#define ID_GAME					13
#define ID_ADVANCED				14
#define ID_LANGUAGE				15
#define ID_LOAD					16
#define ID_SAVE					17
#define ID_DEFAULTS				18
#define ID_BACK					19
#define ID_CUSTOMIZEMODEL		20


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		setupplayer;
	menutext_s		setupmodel;
	menutext_s		setupcontrols;
	menutext_s		setupsystem;
	menutext_s		game;
	menutext_s		advanced;
	menutext_s		language;
	menutext_s		load;
	menutext_s		save;
	menutext_s		defaults;
	menubitmap_s	back;
} setupMenuInfo_t;

static setupMenuInfo_t	setupMenuInfo;


/*
=================
Setup_ResetDefaults_Action
=================
*/
static void Setup_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}


/*
=================
Setup_ResetDefaults_Draw
=================
*/
static void Setup_ResetDefaults_Draw( void ) {
	if(cl_language.integer == 0){
	UI_DrawString( SCREEN_WIDTH/2, 300 + PROP_HEIGHT * 0, "WARNING: This will reset *ALL*", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawString( SCREEN_WIDTH/2, 300 + PROP_HEIGHT * 1, "options to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
	}
	if(cl_language.integer == 1){
	UI_DrawString( SCREEN_WIDTH/2, 300 + PROP_HEIGHT * 0, "ВНИМАНИЕ: Вы действительно хотите сбросить", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawString( SCREEN_WIDTH/2, 300 + PROP_HEIGHT * 1, "*ВСЕ* настройки до стандартных.", UI_CENTER|UI_SMALLFONT, color_yellow );
	}
}


/*
===============
UI_SetupMenu_Event
===============
*/
static void UI_SetupMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_CUSTOMIZEPLAYER:
		UI_PlayerSettingsMenu();
		break;

	case ID_CUSTOMIZEMODEL:
		UI_PlayerModelMenu();
		break;

	case ID_CUSTOMIZECONTROLS:
		UI_ControlsMenu();
		break;

	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;

	case ID_GAME:
		UI_PreferencesMenu();
		break;
		
	case ID_ADVANCED:
		UI_AdvancedMenu();
		break;

	case ID_LANGUAGE:
		if(cl_language.integer == 0){
		trap_Cvar_SetValue("cl_language", 1);
		UI_PopMenu();
		UI_PopMenu();
		UI_PopMenu();
		return;
		}
		if(cl_language.integer == 1){
		trap_Cvar_SetValue("cl_language", 0);
		UI_PopMenu();
		UI_PopMenu();
		UI_PopMenu();
		return;
		}
		break;

	case ID_LOAD:
		UI_LoadConfigMenu();
		break;

	case ID_SAVE:
		UI_SaveConfigMenu();
		break;

	case ID_DEFAULTS:
	if(cl_language.integer == 0){
		UI_ConfirmMenu( "SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
	}
	if(cl_language.integer == 1){
		UI_ConfirmMenu( "СБРОСИТЬ ДО СТАНДАРТНЫХ?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
	}
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_SetupMenu_Init
===============
*/
static void UI_SetupMenu_Init( void ) {
	int				y;
	int 			style;

	UI_SetupMenu_Cache();

	memset( &setupMenuInfo, 0, sizeof(setupMenuInfo) );
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.native 	   = qfalse;
	setupMenuInfo.menu.fullscreen = qtrue;

	setupMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	setupMenuInfo.banner.generic.x					= 320;
	setupMenuInfo.banner.generic.y					= 16;
	setupMenuInfo.banner.color						= color_white;
	setupMenuInfo.banner.style						= UI_CENTER;

	setupMenuInfo.framel.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framel.generic.name				= ART_FRAMEL;
	setupMenuInfo.framel.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framel.generic.x					= 0;  
	setupMenuInfo.framel.generic.y					= 78;
	setupMenuInfo.framel.width  					= 256;
	setupMenuInfo.framel.height  					= 329;

	setupMenuInfo.framer.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framer.generic.name				= ART_FRAMER;
	setupMenuInfo.framer.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framer.generic.x					= 376;
	setupMenuInfo.framer.generic.y					= 76;
	setupMenuInfo.framer.width  					= 256;
	setupMenuInfo.framer.height  					= 334;

	y = 128 - SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupplayer.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupplayer.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupplayer.generic.x				= 320;
	setupMenuInfo.setupplayer.generic.y				= y;
	setupMenuInfo.setupplayer.generic.id			= ID_CUSTOMIZEPLAYER;
	setupMenuInfo.setupplayer.generic.callback		= UI_SetupMenu_Event;
	setupMenuInfo.setupplayer.color					= color_white;
	setupMenuInfo.setupplayer.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupmodel.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupmodel.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupmodel.generic.x			= 320;
	setupMenuInfo.setupmodel.generic.y			= y;
	setupMenuInfo.setupmodel.generic.id			= ID_CUSTOMIZEMODEL;
	setupMenuInfo.setupmodel.generic.callback	= UI_SetupMenu_Event;
	setupMenuInfo.setupmodel.color				= color_white;
	setupMenuInfo.setupmodel.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupcontrols.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupcontrols.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupcontrols.generic.x			= 320;
	setupMenuInfo.setupcontrols.generic.y			= y;
	setupMenuInfo.setupcontrols.generic.id			= ID_CUSTOMIZECONTROLS;
	setupMenuInfo.setupcontrols.generic.callback	= UI_SetupMenu_Event;
	setupMenuInfo.setupcontrols.color				= color_white;
	setupMenuInfo.setupcontrols.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupsystem.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupsystem.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsystem.generic.x				= 320;
	setupMenuInfo.setupsystem.generic.y				= y;
	setupMenuInfo.setupsystem.generic.id			= ID_SYSTEMCONFIG;
	setupMenuInfo.setupsystem.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupsystem.color					= color_white;
	setupMenuInfo.setupsystem.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.game.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.game.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.game.generic.x					= 320;
	setupMenuInfo.game.generic.y					= y;
	setupMenuInfo.game.generic.id					= ID_GAME;
	setupMenuInfo.game.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.game.color						= color_white;
	setupMenuInfo.game.style						= UI_CENTER;
	
	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.advanced.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.advanced.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.advanced.generic.x					= 320;
	setupMenuInfo.advanced.generic.y					= y;
	setupMenuInfo.advanced.generic.id					= ID_ADVANCED;
	setupMenuInfo.advanced.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.advanced.color						= color_white;
	setupMenuInfo.advanced.style						= UI_CENTER;

	style = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	if( trap_Cvar_VariableValue( "cl_paused" ) ) {
		style |= QMF_GRAYED;
	}

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.load.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.load.generic.flags				= style;
	setupMenuInfo.load.generic.x					= 320;
	setupMenuInfo.load.generic.y					= y;
	setupMenuInfo.load.generic.id					= ID_LOAD;
	setupMenuInfo.load.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.load.color						= color_white;
	setupMenuInfo.load.style						= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.save.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.save.generic.flags				= style;
	setupMenuInfo.save.generic.x					= 320;
	setupMenuInfo.save.generic.y					= y;
	setupMenuInfo.save.generic.id					= ID_SAVE;
	setupMenuInfo.save.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.save.color						= color_white;
	setupMenuInfo.save.style						= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
	setupMenuInfo.defaults.generic.flags			= style;
	setupMenuInfo.defaults.generic.x				= 320;
	setupMenuInfo.defaults.generic.y				= y;
	setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
	setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event;
	setupMenuInfo.defaults.color					= color_white;
	setupMenuInfo.defaults.style					= UI_CENTER;
	
	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.language.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.language.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.language.generic.x					= 320;
	setupMenuInfo.language.generic.y					= y;
	setupMenuInfo.language.generic.id					= ID_LANGUAGE;
	setupMenuInfo.language.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.language.color						= color_white;
	setupMenuInfo.language.style						= UI_CENTER;

	setupMenuInfo.back.generic.type					= MTYPE_BITMAP;
	setupMenuInfo.back.generic.name					= ART_BACK0;
	setupMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.id					= ID_BACK;
	setupMenuInfo.back.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.back.generic.x					= 0 - uis.wideoffset;
	setupMenuInfo.back.generic.y					= 480-64;
	setupMenuInfo.back.width						= 128;
	setupMenuInfo.back.height						= 64;
	setupMenuInfo.back.focuspic						= ART_BACK1;
	
	if(cl_language.integer == 0){
	setupMenuInfo.banner.string						= "SETUP";
	setupMenuInfo.setupplayer.string				= "PLAYER";
	setupMenuInfo.setupmodel.string					= "MODEL";
	setupMenuInfo.setupcontrols.string				= "CONTROLS";
	setupMenuInfo.setupsystem.string				= "SYSTEM";
	setupMenuInfo.game.string						= "GAME OPTIONS";
	setupMenuInfo.advanced.string					= "ADVANCED";
	setupMenuInfo.load.string						= "LOAD/EXEC CONFIG";
	setupMenuInfo.save.string						= "SAVE CONFIG";
	setupMenuInfo.defaults.string					= "DEFAULTS";
	setupMenuInfo.language.string					= "РУССКИЙ";
	}
	if(cl_language.integer == 1){
	setupMenuInfo.banner.string						= "НАСТРОЙКИ";
	setupMenuInfo.setupplayer.string				= "ИГРОК";
	setupMenuInfo.setupmodel.string					= "МОДЕЛЬ";
	setupMenuInfo.setupcontrols.string				= "УПРАВЛЕНИЕ";
	setupMenuInfo.setupsystem.string				= "СИСТЕМА";
	setupMenuInfo.game.string						= "ИГРОВЫЕ ОПЦИИ";
	setupMenuInfo.advanced.string					= "РАСШИРЕННЫЕ ОПЦИИ";
	setupMenuInfo.load.string						= "ЗАГРУЗКА КОНФИГОВ";
	setupMenuInfo.save.string						= "СОХРАНЕНИЕ КОНФИГОВ";
	setupMenuInfo.defaults.string					= "СБРОС";
	setupMenuInfo.language.string					= "ENGLISH";
	}

	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.banner );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framel );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framer );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupplayer );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupmodel );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupcontrols );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupsystem );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.game );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.advanced );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.language );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.load );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.save );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back );
}


/*
=================
UI_SetupMenu_Cache
=================
*/
void UI_SetupMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_SetupMenu
===============
*/
void UI_SetupMenu( void ) {
	UI_SetupMenu_Init();
	UI_PushMenu( &setupMenuInfo.menu );
}
