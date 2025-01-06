// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

SOUND OPTIONS MENU

=======================================================================
*/




#include "ui_local.h"


#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"

#define ID_GRAPHICS			10
#define ID_DISPLAY			11
#define ID_SOUND			12
#define ID_NETWORK			13
#define ID_EFFECTSVOLUME	14
#define ID_MUSICVOLUME		15
#define ID_QUALITY			16
#define ID_SDRIVER			17
#define ID_BACK				18
#define ID_ANIMSFX			19


static const char *quality_items[] = {
	"Low", "Medium", "High", "Ultra", 0
};

static const char *quality_itemsru[] = {
	"Низкое", "Среднее", "Высокое", "Ультра", 0
};

static const char *sdriver_items[] = {
	"DirectSound", "WASAPI", 0
};

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;

	menutext_s			graphics;
	menutext_s			display;
	menutext_s			sound;
	menutext_s			network;

	menuslider_s		sfxvolume;
	menuslider_s		musicvolume;
	menulist_s			quality;
	menulist_s			sdriver;
	menuradiobutton_s	animsfx;

	menubitmap_s		back;
} soundOptionsInfo_t;

static soundOptionsInfo_t	soundOptionsInfo;


/*
=================
UI_SoundOptionsMenu_Event
=================
*/
static void UI_SoundOptionsMenu_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GRAPHICS:
		UI_PopMenu();
		UI_GraphicsOptionsMenu();
		break;

	case ID_DISPLAY:
		UI_PopMenu();
		UI_DisplayOptionsMenu();
		break;

	case ID_SOUND:
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;

	case ID_EFFECTSVOLUME:
		trap_Cvar_SetValue( "s_volume", soundOptionsInfo.sfxvolume.curvalue / 10 );
		break;

	case ID_MUSICVOLUME:
		trap_Cvar_SetValue( "s_musicvolume", soundOptionsInfo.musicvolume.curvalue / 10 );
		break;

	case ID_QUALITY:
		if( soundOptionsInfo.quality.curvalue == 0 ) {
			trap_Cvar_SetValue( "s_khz", 11 );
		}
		if( soundOptionsInfo.quality.curvalue == 1 ) {
			trap_Cvar_SetValue( "s_khz", 22 );
		}
		if( soundOptionsInfo.quality.curvalue == 2 ) {
			trap_Cvar_SetValue( "s_khz", 44 );
		}
		if( soundOptionsInfo.quality.curvalue == 3 ) {
			trap_Cvar_SetValue( "s_khz", 48 );
		}
		UI_ForceMenuOff();
		trap_Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		break;

	case ID_SDRIVER:
		if( soundOptionsInfo.sdriver.curvalue == 0 ) {
			trap_Cvar_Set( "s_driver", "dsound" );
		}
		if( soundOptionsInfo.sdriver.curvalue == 1 ) {
			trap_Cvar_Set( "s_driver", "wasapi" );
		}
		UI_ForceMenuOff();
		trap_Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		break;


	case ID_ANIMSFX:
		trap_Cvar_SetValue("uie_s_animsfx", soundOptionsInfo.animsfx.curvalue);
		break;
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_SoundOptionsMenu_Init
===============
*/
static void UI_SoundOptionsMenu_Init( void ) {
	int				y;
	char 			drivername[MAX_QPATH];

	memset( &soundOptionsInfo, 0, sizeof(soundOptionsInfo) );

	UI_SoundOptionsMenu_Cache();
	soundOptionsInfo.menu.wrapAround = qtrue;
	soundOptionsInfo.menu.native 	   = qfalse;
	soundOptionsInfo.menu.fullscreen = qtrue;

	soundOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	soundOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	soundOptionsInfo.banner.generic.x			= 320;
	soundOptionsInfo.banner.generic.y			= 16;
	soundOptionsInfo.banner.color				= color_white;
	soundOptionsInfo.banner.style				= UI_CENTER;

	soundOptionsInfo.framel.generic.type		= MTYPE_BITMAP;
	soundOptionsInfo.framel.generic.name		= ART_FRAMEL;
	soundOptionsInfo.framel.generic.flags		= QMF_INACTIVE;
	soundOptionsInfo.framel.generic.x			= 0;  
	soundOptionsInfo.framel.generic.y			= 78;
	soundOptionsInfo.framel.width				= 256;
	soundOptionsInfo.framel.height				= 329;

	soundOptionsInfo.framer.generic.type		= MTYPE_BITMAP;
	soundOptionsInfo.framer.generic.name		= ART_FRAMER;
	soundOptionsInfo.framer.generic.flags		= QMF_INACTIVE;
	soundOptionsInfo.framer.generic.x			= 376;
	soundOptionsInfo.framer.generic.y			= 76;
	soundOptionsInfo.framer.width				= 256;
	soundOptionsInfo.framer.height				= 334;

	soundOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	soundOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.graphics.generic.id		= ID_GRAPHICS;
	soundOptionsInfo.graphics.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.graphics.generic.x			= 140 - uis.wideoffset;
	soundOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	soundOptionsInfo.graphics.style				= UI_RIGHT;
	soundOptionsInfo.graphics.color				= color_white;

	soundOptionsInfo.display.generic.type		= MTYPE_PTEXT;
	soundOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.display.generic.id			= ID_DISPLAY;
	soundOptionsInfo.display.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.display.generic.x			= 140 - uis.wideoffset;
	soundOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	soundOptionsInfo.display.style				= UI_RIGHT;
	soundOptionsInfo.display.color				= color_white;

	soundOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	soundOptionsInfo.sound.generic.flags		= QMF_RIGHT_JUSTIFY;
	soundOptionsInfo.sound.generic.id			= ID_SOUND;
	soundOptionsInfo.sound.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.sound.generic.x			= 140 - uis.wideoffset;
	soundOptionsInfo.sound.generic.y			= 240;
	soundOptionsInfo.sound.style				= UI_RIGHT;
	soundOptionsInfo.sound.color				= color_grey;

	soundOptionsInfo.network.generic.type		= MTYPE_PTEXT;
	soundOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.network.generic.id			= ID_NETWORK;
	soundOptionsInfo.network.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.network.generic.x			= 140 - uis.wideoffset;
	soundOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	soundOptionsInfo.network.style				= UI_RIGHT;
	soundOptionsInfo.network.color				= color_white;

	y = 240 - 2.0 * (BIGCHAR_HEIGHT + 2);
	soundOptionsInfo.sfxvolume.generic.type		= MTYPE_SLIDER;
	soundOptionsInfo.sfxvolume.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.sfxvolume.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.sfxvolume.generic.id		= ID_EFFECTSVOLUME;
	soundOptionsInfo.sfxvolume.generic.x		= 400;
	soundOptionsInfo.sfxvolume.generic.y		= y;
	soundOptionsInfo.sfxvolume.minvalue			= 0;
	soundOptionsInfo.sfxvolume.maxvalue			= 10;

	y += BIGCHAR_HEIGHT+2;
	soundOptionsInfo.musicvolume.generic.type		= MTYPE_SLIDER;
	soundOptionsInfo.musicvolume.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.musicvolume.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.musicvolume.generic.id			= ID_MUSICVOLUME;
	soundOptionsInfo.musicvolume.generic.x			= 400;
	soundOptionsInfo.musicvolume.generic.y			= y;
	soundOptionsInfo.musicvolume.minvalue			= 0;
	soundOptionsInfo.musicvolume.maxvalue			= 10;

	y += BIGCHAR_HEIGHT+2;
	soundOptionsInfo.quality.generic.type		= MTYPE_SPINCONTROL;
	soundOptionsInfo.quality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.quality.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.quality.generic.id			= ID_QUALITY;
	soundOptionsInfo.quality.generic.x			= 400;
	soundOptionsInfo.quality.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	soundOptionsInfo.sdriver.generic.type			= MTYPE_SPINCONTROL;
	soundOptionsInfo.sdriver.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.sdriver.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.sdriver.generic.id				= ID_SDRIVER;
	soundOptionsInfo.sdriver.generic.x				= 400;
	soundOptionsInfo.sdriver.generic.y				= y;

	y += BIGCHAR_HEIGHT+2;
	soundOptionsInfo.animsfx.generic.type			= MTYPE_RADIOBUTTON;
	soundOptionsInfo.animsfx.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.animsfx.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.animsfx.generic.id				= ID_ANIMSFX;
	soundOptionsInfo.animsfx.generic.x				= 400;
	soundOptionsInfo.animsfx.generic.y				= y;

	soundOptionsInfo.back.generic.type			= MTYPE_BITMAP;
	soundOptionsInfo.back.generic.name			= ART_BACK0;
	soundOptionsInfo.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.back.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.back.generic.id			= ID_BACK;
	soundOptionsInfo.back.generic.x				= 0 - uis.wideoffset;
	soundOptionsInfo.back.generic.y				= 480-64;
	soundOptionsInfo.back.width					= 128;
	soundOptionsInfo.back.height				= 64;
	soundOptionsInfo.back.focuspic				= ART_BACK1;
	
	if(cl_language.integer == 0){
	soundOptionsInfo.banner.string				= "SYSTEM SETUP";
	soundOptionsInfo.graphics.string			= "GRAPHICS";
	soundOptionsInfo.display.string				= "DISPLAY";
	soundOptionsInfo.sound.string				= "SOUND";
	soundOptionsInfo.network.string				= "NETWORK";
	soundOptionsInfo.sfxvolume.generic.name		= "Effects Volume:";
	soundOptionsInfo.musicvolume.generic.name		= "Music Volume:";
	soundOptionsInfo.quality.generic.name		= "Sound Quality:";
	soundOptionsInfo.quality.itemnames			= quality_items;
	soundOptionsInfo.sdriver.generic.name		= "Sound Driver:";
	soundOptionsInfo.sdriver.itemnames			= sdriver_items;
	soundOptionsInfo.animsfx.generic.name			= "UI Animation sfx:";
	}
	
	if(cl_language.integer == 1){
	soundOptionsInfo.banner.string				= "СИСТЕМНЫЕ НАСТРОЙКИ";
	soundOptionsInfo.graphics.string			= "ГРАФИКА";
	soundOptionsInfo.display.string				= "ЭКРАН";
	soundOptionsInfo.sound.string				= "ЗВУК";
	soundOptionsInfo.network.string				= "СЕТЬ";
	soundOptionsInfo.sfxvolume.generic.name		= "Громкость Эффектов:";
	soundOptionsInfo.musicvolume.generic.name		= "Громкость Музыки:";
	soundOptionsInfo.quality.generic.name		= "Качество Звука:";
	soundOptionsInfo.quality.itemnames			= quality_itemsru;
	soundOptionsInfo.sdriver.generic.name		= "Драйвер звука:";
	soundOptionsInfo.sdriver.itemnames			= sdriver_items;
	soundOptionsInfo.animsfx.generic.name			= "UI Звуки Анимации:";
	}

	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.banner );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.framel );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.framer );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.graphics );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.display );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.sound );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.network );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.sfxvolume );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.musicvolume );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.quality );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.sdriver );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.animsfx );
	Menu_AddItem( &soundOptionsInfo.menu, ( void * ) &soundOptionsInfo.back );

	soundOptionsInfo.sfxvolume.curvalue = trap_Cvar_VariableValue( "s_volume" ) * 10;
	soundOptionsInfo.musicvolume.curvalue = trap_Cvar_VariableValue( "s_musicvolume" ) * 10;
	soundOptionsInfo.animsfx.curvalue = (int)trap_Cvar_VariableValue( "uie_s_animsfx" );
	
	if( trap_Cvar_VariableValue( "s_khz" ) == 11 ) {
	soundOptionsInfo.quality.curvalue = 0;
	}
	if( trap_Cvar_VariableValue( "s_khz" ) == 22 ) {
	soundOptionsInfo.quality.curvalue = 1;
	}
	if( trap_Cvar_VariableValue( "s_khz" ) == 44 ) {
	soundOptionsInfo.quality.curvalue = 2;
	}
	if( trap_Cvar_VariableValue( "s_khz" ) == 48 ) {
	soundOptionsInfo.quality.curvalue = 3;
	}
	
	trap_Cvar_VariableStringBuffer( "s_driver", drivername, MAX_QPATH );
	
	if( Q_stricmp (drivername, "dsound") == 0 ){
	soundOptionsInfo.sdriver.curvalue = 0;
	}
	if( Q_stricmp (drivername, "wasapi") == 0 ){
	soundOptionsInfo.sdriver.curvalue = 1;
	}
}


/*
===============
UI_SoundOptionsMenu_Cache
===============
*/
void UI_SoundOptionsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_SoundOptionsMenu
===============
*/
void UI_SoundOptionsMenu( void ) {
	UI_SoundOptionsMenu_Init();
	UI_PushMenu( &soundOptionsInfo.menu );
	Menu_SetCursorToItem( &soundOptionsInfo.menu, &soundOptionsInfo.sound );
}
