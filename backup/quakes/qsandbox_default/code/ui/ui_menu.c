// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/





#include "ui_local.h"


#define MODLOADER				"menu/gamemode_default"



#define ID_SINGLEPLAYER			10
#define ID_MULTIPLAYER			11
#define ID_SETUP				12
#define ID_DEMOS				13
#define ID_CINEMATICS			14
#define ID_TEAMARENA			15
#define ID_MODS					16
#define ID_EXIT					17
#define ID_SKIRMISH             18
#define ID_PLAYERNAME			19
#define ID_MODELTYPE			20
#define ID_GAMEMODEP			21
#define ID_MODDB				22
#define ID_MODLOADER			23
#define ID_BUTTON1				24
#define ID_BUTTON2				25
#define ID_BUTTON3				26
#define ID_MODLIST					27

#define MODDB				"menu/moddb"
#define M_BUTTON1			"menu/button1"
#define M_BUTTON2			"menu/button2"
#define M_BUTTON3			"menu/button3"
#define MAIN_MENU_VERTICAL_SPACING		34

#define MAIN_MENU_CENTER 200


vec4_t color_translucent	= {1.0f, 1.0f, 1.0f, 0.2f};



typedef struct {
	menuframework_s	menu;

	menutext_s		gamemodep;
    menubitmap_s	modloader;
	menubitmap_s	moddb;
	menubitmap_s	button1;
	menubitmap_s	button2;
	menubitmap_s	button3;
	menutext_s		singleplayer;
	menutext_s      skirmish;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		cinematics;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		exit;
	menutext_s		name;
	menutext_s		modeltype;
	menuobject_s	modlist;
	
	char			names[524288];
	char*			configlist[524288];

#ifndef NO_UIE_MINILOGO
	menubitmap_s	logo;
#endif

	modelAnim_t 	model;
	char 			playername[MAX_NAME_LENGTH];

	qhandle_t		bannerModel;
} mainmenu_t;


static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;	
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu(0);
}

/*
=================
MainMenu_ReloadAction
=================
*/
static void MainMenu_ReloadAction( qboolean result ) {
	if( !result ) {
		return;
	}
trap_Cmd_ExecuteText( EXEC_APPEND, "game_restart;" );
}

/*
=================
MainMenu_LoadScript
=================
*/
static qboolean MainMenu_LoadScript( const char* filename )
{
	UI_PopMenu();
	MainMenu_ReloadGame();
	return qtrue;
}

void MainMenu_ReloadGame( void )
{
	if(cl_language.integer == 0){
	UI_ConfirmMenu( "RELOAD GAME?", 0, MainMenu_ReloadAction );
	}
	if(cl_language.integer == 1){
	UI_ConfirmMenu( "ПЕРЕЗАГРУЗИТЬ ИГРУ?", 0, MainMenu_ReloadAction );
	}
}

/*
=================
Main_SetPlayerModelType
=================
*/
static void Main_SetPlayerModelType( void )
{
	if (UIE_PlayerInfo_IsTeamModel())
	{
		if(cl_language.integer == 0){
		s_main.modeltype.string = "Team Model";
		}
		if(cl_language.integer == 1){
		s_main.modeltype.string = "Командная Модель";
		}
	}
	else
	{
		if(cl_language.integer == 0){
		s_main.modeltype.string = "DM Model";
		}
		if(cl_language.integer == 1){
		s_main.modeltype.string = "Обычная Модель";
		}
	}

	PText_Init(&s_main.modeltype);
}



/*
=================
Main_ToggleModelType
=================
*/
static void Main_ToggleModelType( void )
{
	qboolean type;

	if (UIE_PlayerInfo_IsTeamModel()) {
		type = qfalse;
	}
	else {
		type = qtrue;
	}

	UIE_PlayerInfo_DrawTeamModel(&s_main.model, type);
	Main_SetPlayerModelType();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SINGLEPLAYER:
        if(ui_singlemode.integer){
            trap_Cmd_ExecuteText( EXEC_APPEND, "ns_openscript_ui new_game.ns;" );
        }
		UI_StartServerMenu( qtrue );
		break;

    case ID_SKIRMISH:
        UI_StartServerMenu(qtrue);
        break;

	case ID_MULTIPLAYER:
		if(!ui_singlemode.integer){
		UI_ArenaServersMenu();
		} else {
		UI_CinematicsMenu(1);	
		}
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_CINEMATICS:
		//UI_CinematicsMenu();
		break;

	case ID_MODS:
		UI_ModsMenu();
		break;

	case ID_TEAMARENA:
		trap_Cvar_Set( "fs_game", "missionpack");
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;

	case ID_PLAYERNAME:
		UI_PlayerSettingsMenu();
		break;	

	case ID_MODELTYPE:
		Main_ToggleModelType();
		break;
		
	case ID_GAMEMODEP:
		UI_WorkshopMenu();
		break;
		
	case ID_MODLOADER:
		StartServer_ServerPage_Mods();
		break;
		
	case ID_MODDB:
	if(trap_Cvar_VariableValue("os_windows")){
		trap_System("start https://www.moddb.com/games/qs");
	}
	if(trap_Cvar_VariableValue("os_linux")){
		trap_System("xdg-open https://www.moddb.com/games/qs");
	}
	if(trap_Cvar_VariableValue("os_macos")){
		trap_System("open https://www.moddb.com/games/qs");
	}
		break;

	case ID_EXIT:
		UI_CreditMenu(0);
		break;
	case ID_BUTTON1:
		trap_Cmd_ExecuteText( EXEC_APPEND, "toggle r_fullscreen;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "set vid_xpos 0;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "set vid_ypos 0;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "minimize;" );
		break;
	case ID_BUTTON2:
		trap_Cmd_ExecuteText( EXEC_APPEND, "toggle r_fullscreen;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "set vid_xpos 0;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "set vid_ypos 0;" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;
	case ID_BUTTON3:
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit;" );
		break;
	case ID_MODLIST:
		trap_Cmd_ExecuteText( EXEC_INSERT, va("nsgui %s.ns\n", s_main.modlist.itemnames[s_main.modlist.curvalue]) );
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( MODLOADER );
	trap_R_RegisterShaderNoMip( MODDB );
	trap_R_RegisterShaderNoMip( M_BUTTON1 );
	trap_R_RegisterShaderNoMip( M_BUTTON2 );
	trap_R_RegisterShaderNoMip( M_BUTTON3 );
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {
	vec4_t			color = {0.85, 0.9, 1.0, 1};
	char 			buffer[MAX_NAME_LENGTH];
	vec4_t			modlistcolor = {0.00f, 0.00f, 0.00f, 0.40f};

	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawString( 320, 192, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else
	{
      // change the players name if different to control
   
	   trap_Cvar_VariableStringBuffer( "name", buffer, MAX_NAME_LENGTH);
	   Q_CleanStr(buffer);
	   if (strcmp(s_main.playername, buffer)) {
		   strcpy(s_main.playername, buffer);
		   PText_Init(&s_main.name);
	   }
   
	   // update type of model displayed
	   if (uis.firstdraw)
		   Main_SetPlayerModelType();
	   
	   UI_DrawRoundedRect(317+uis.wideoffset, 30, 1000000, 20*SMALLCHAR_HEIGHT*1.25, 10, modlistcolor);
	   // standard menu drawing
	   Menu_Draw( &s_main.menu );
   }
	UI_DrawString( 600+uis.wideoffset, 450, "Quake Sandbox v8.1", UI_RIGHT|UI_SMALLFONT, color );
	UI_DrawString( 600+uis.wideoffset, 465, "by Noire.dev", UI_RIGHT|UI_SMALLFONT, color );
}


/*
=================
Main_DrawPlayer
=================
*/
static void Main_DrawPlayer( void *self )
{
	UIE_PlayerInfo_AnimateModel(&s_main.model);
}




/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0) {
			return qtrue;
		}
	dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style;
	float	sizeScale;
	int		i;
	int		len;
	char	*configname;

	//trap_Cvar_Set( "sv_killserver", "1" );

	memset( &s_main, 0 ,sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0 ,sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if (strlen(s_errorMessage.errorMessage))
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;		

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		
		return;
	}

	sizeScale = UI_ProportionalSizeScale( UI_SMALLFONT, 0 );
	style = UI_CENTER | UI_DROPSHADOW;

	MainMenu_Cache();

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;
	s_main.menu.native = qfalse;

	s_main.modloader.generic.type		= MTYPE_BITMAP;
	s_main.modloader.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.modloader.generic.id			= ID_MODLOADER;
	s_main.modloader.generic.callback	= Main_MenuEvent;
	s_main.modloader.generic.x			= 440 + uis.wideoffset;
	s_main.modloader.generic.y			= 365;
	s_main.modloader.width				= 256*0.90;
	s_main.modloader.height				= 38*0.80;
	
	s_main.moddb.generic.type		= MTYPE_BITMAP;
	s_main.moddb.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.moddb.generic.id			= ID_MODDB;
	s_main.moddb.generic.callback	= Main_MenuEvent;
	s_main.moddb.generic.x			= 460 + uis.wideoffset;
	s_main.moddb.generic.y			= 365 + 38*0.80 + 15;
	s_main.moddb.width				= 256*0.90;
	s_main.moddb.height				= 38*0.80;
	
	s_main.button1.generic.type			= MTYPE_BITMAP;
	s_main.button1.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.button1.generic.id			= ID_BUTTON1;
	s_main.button1.generic.callback		= Main_MenuEvent;
	s_main.button1.generic.x			= 577 + uis.wideoffset;
	s_main.button1.generic.y			= 5;
	s_main.button1.width				= 24;
	s_main.button1.height				= 12;
	
	s_main.button2.generic.type			= MTYPE_BITMAP;
	s_main.button2.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.button2.generic.id			= ID_BUTTON2;
	s_main.button2.generic.callback		= Main_MenuEvent;
	s_main.button2.generic.x			= 601 + uis.wideoffset;
	s_main.button2.generic.y			= 5;
	s_main.button2.width				= 24;
	s_main.button2.height				= 12;
	
	s_main.button3.generic.type			= MTYPE_BITMAP;
	s_main.button3.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.button3.generic.id			= ID_BUTTON3;
	s_main.button3.generic.callback		= Main_MenuEvent;
	s_main.button3.generic.x			= 625 + uis.wideoffset;
	s_main.button3.generic.y			= 5;
	s_main.button3.width				= 24;
	s_main.button3.height				= 12;

	y = 143;
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent;
	s_main.singleplayer.generic.x			= 64 - uis.wideoffset;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.color				= color_white;
	s_main.singleplayer.style		    	= UI_LEFT|UI_SMALLFONT;
	
	y += 18;
	s_main.multiplayer.generic.type		= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback	= Main_MenuEvent;
	s_main.multiplayer.generic.x			= 64 - uis.wideoffset;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.color				= color_white;
	s_main.multiplayer.style		    	= UI_LEFT|UI_SMALLFONT;
	
	y += 35;
	s_main.gamemodep.generic.type		= MTYPE_PTEXT;
	s_main.gamemodep.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.gamemodep.generic.id			= ID_GAMEMODEP;
	s_main.gamemodep.generic.callback	= Main_MenuEvent;
	s_main.gamemodep.generic.x			= 64 - uis.wideoffset;
	s_main.gamemodep.generic.y			= y;
	s_main.gamemodep.color				= color_white;
	s_main.gamemodep.style		    	= UI_LEFT|UI_SMALLFONT;

	y += 19;
	s_main.name.generic.type			= MTYPE_PTEXT;
	s_main.name.generic.flags			= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.name.generic.x				= 64 - uis.wideoffset;
	s_main.name.generic.y				= y;
	s_main.name.generic.id				= ID_PLAYERNAME;
	s_main.name.generic.callback		= Main_MenuEvent;
	s_main.name.color					= color_white;
	s_main.name.style					= UI_LEFT|UI_SMALLFONT;
	
	y += 19;
	s_main.mods.generic.type		= MTYPE_PTEXT;
	s_main.mods.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.mods.generic.id			= ID_MODS;
	s_main.mods.generic.callback	= Main_MenuEvent;
	s_main.mods.generic.x			= 64 - uis.wideoffset;
	s_main.mods.generic.y			= y;
	s_main.mods.color				= color_white;
	s_main.mods.style		    	= UI_LEFT|UI_SMALLFONT;
	
	y += 19;
	s_main.demos.generic.type		= MTYPE_PTEXT;
	s_main.demos.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.demos.generic.id			= ID_DEMOS;
	s_main.demos.generic.callback	= Main_MenuEvent;
	s_main.demos.generic.x			= 64 - uis.wideoffset;
	s_main.demos.generic.y			= y;
	s_main.demos.color				= color_white;
	s_main.demos.style		    	= UI_LEFT|UI_SMALLFONT;
	
	y += 36;
	s_main.setup.generic.type		= MTYPE_PTEXT;
	s_main.setup.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.setup.generic.id			= ID_SETUP;
	s_main.setup.generic.callback	= Main_MenuEvent;
	s_main.setup.generic.x			= 64 - uis.wideoffset;
	s_main.setup.generic.y			= y;
	s_main.setup.color				= color_white;
	s_main.setup.style		    	= UI_LEFT|UI_SMALLFONT;
	
	y += 36;
	s_main.exit.generic.type		= MTYPE_PTEXT;
	s_main.exit.generic.flags		= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.exit.generic.id			= ID_EXIT;
	s_main.exit.generic.callback	= Main_MenuEvent;
	s_main.exit.generic.x			= 64 - uis.wideoffset;
	s_main.exit.generic.y			= y;
	s_main.exit.color				= color_white;
	s_main.exit.style		    	= UI_LEFT|UI_SMALLFONT;

	y += MAIN_MENU_VERTICAL_SPACING;

	s_main.modeltype.generic.type			= MTYPE_PTEXT;
	s_main.modeltype.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.modeltype.generic.x				= 480;
	s_main.modeltype.generic.y				= PLAYERMODEL_TEXTHEIGHT + (PROP_HEIGHT*sizeScale);
	s_main.modeltype.generic.id				= ID_MODELTYPE;
	s_main.modeltype.generic.callback		= Main_MenuEvent;
	s_main.modeltype.string					= "";
	s_main.modeltype.color					= text_color_normal;
	s_main.modeltype.style					= style|UI_SMALLFONT;

	s_main.model.bitmap.generic.type      = MTYPE_BITMAP;
	s_main.model.bitmap.generic.flags     = QMF_INACTIVE;
	s_main.model.bitmap.generic.ownerdraw = Main_DrawPlayer;
	s_main.model.bitmap.generic.x	        = PLAYERMODEL_X;
	s_main.model.bitmap.generic.y	        = PLAYERMODEL_Y;
	s_main.model.bitmap.width	            = PLAYERMODEL_WIDTH;
	s_main.model.bitmap.height            = PLAYERMODEL_HEIGHT;
	
	if(cl_language.integer == 0){
    s_main.singleplayer.string			= "Start New Game";
	if(!ui_singlemode.integer){
    s_main.multiplayer.string			= "Find Multiplayer Game";
	} else {
	s_main.multiplayer.string			= "Load Singleplayer Game";
	}
    s_main.gamemodep.string			    = "Workshop";
	s_main.name.string					= "Profile";
    s_main.mods.string			   	 	= "Mods";
    s_main.demos.string			    	= "Demos";
    s_main.setup.string			    	= "Options";
    s_main.exit.string			    	= "Quit";
	}
	if(cl_language.integer == 1){
    s_main.singleplayer.string			= "Начать новую игру";
	if(!ui_singlemode.integer){
    s_main.multiplayer.string			= "Найти сетевую игру";
	} else {
	s_main.multiplayer.string			= "Загрузить сохр игру";
	}
    s_main.gamemodep.string			    = "Дополнения";
	s_main.name.string					= "Профиль";
    s_main.mods.string			    	= "Моды";
    s_main.demos.string			    	= "Повторы";
    s_main.setup.string			    	= "Настройки";
    s_main.exit.string			    	= "Выйти";
	}

#ifndef NO_UIE_MINILOGO
	s_main.logo.generic.type			= MTYPE_BITMAP;
	s_main.logo.generic.flags		= QMF_INACTIVE|QMF_HIGHLIGHT;
	s_main.logo.generic.x			= 54 - uis.wideoffset;
	s_main.logo.generic.y			= 37;
	s_main.logo.width				= 200;
	s_main.logo.height				= 100;
	s_main.logo.focuspic 			= UIE_LOGO_NAME;

	Menu_AddItem( &s_main.menu,	&s_main.logo);
#endif

	s_main.modloader.generic.name		= MODLOADER;
	s_main.modloader.focuspic			= MODLOADER;
	s_main.moddb.generic.name		= MODDB;
	s_main.moddb.focuspic			= MODDB;
	s_main.button1.generic.name		= M_BUTTON1;
	s_main.button1.focuspic			= M_BUTTON1;
	s_main.button2.generic.name		= M_BUTTON2;
	s_main.button2.focuspic			= M_BUTTON2;
	s_main.button3.generic.name		= M_BUTTON3;
	s_main.button3.focuspic			= M_BUTTON3;

	s_main.modlist.generic.type			= MTYPE_UIOBJECT;
	s_main.modlist.type					= 5;
	s_main.modlist.styles				= 1;
	s_main.modlist.fontsize				= 1.25;
	s_main.modlist.string				= "nsgui/icons";
	s_main.modlist.generic.flags		= QMF_PULSEIFFOCUS;
	s_main.modlist.generic.callback		= Main_MenuEvent;
	s_main.modlist.generic.id			= ID_MODLIST;
	s_main.modlist.generic.x			= 319 + uis.wideoffset;
	s_main.modlist.generic.y			= 30;
	s_main.modlist.width				= 32;
	s_main.modlist.height				= 20;
	s_main.modlist.numitems				= trap_FS_GetFileList( "nsgui", "ns", s_main.names, 524288 );
	s_main.modlist.itemnames			= (const char **)s_main.configlist;
	s_main.modlist.columns				= 1;
	s_main.modlist.color				= color_white;
	
	if (!s_main.modlist.numitems) {
		strcpy(s_main.names,"No addons");
		s_main.modlist.numitems = 1;
	}
	else if (s_main.modlist.numitems > 65536)
		s_main.modlist.numitems = 65536;

	configname = s_main.names;
	for ( i = 0; i < s_main.modlist.numitems; i++ ) {
		s_main.modlist.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 3,".ns"))
			configname[len-3] = '\0';

		configname += len + 1;
	}

	Menu_AddItem( &s_main.menu,	&s_main.gamemodep );
	Menu_AddItem( &s_main.menu,	&s_main.modloader );
	Menu_AddItem( &s_main.menu,	&s_main.moddb );
	Menu_AddItem( &s_main.menu,	&s_main.button1 );
	Menu_AddItem( &s_main.menu,	&s_main.button2 );
	Menu_AddItem( &s_main.menu,	&s_main.button3 );
	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.mods );
	Menu_AddItem( &s_main.menu,	&s_main.exit );
	Menu_AddItem( &s_main.menu,	&s_main.name );
	Menu_AddItem( &s_main.menu, &s_main.modlist );
	//Menu_AddItem( &s_main.menu,	&s_main.modeltype );
	//Menu_AddItem( &s_main.menu,	&s_main.model.bitmap);

	// prepare the player model
	//UIE_PlayerInfo_InitModel(&s_main.model);

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
}





