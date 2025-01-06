//Copyright (C) 1999-2005 Id Software, Inc.
//
/*
=======================================================================

DISPLAY OPTIONS MENU

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
#define ID_BRIGHTNESS		14
#define ID_SCREENSIZE		15
#define ID_BACK				16
#define ID_CROSSSIZE		17
#define ID_THIRDPERSON			18
#define ID_THIRDPERSONRANGE		19
#define ID_THIRDPERSONOFFSET	20
#define ID_ICONS			21
#define ID_STATUS			22
#define ID_GUN				23
#define ID_SPEED			24
#define ID_FRIEND			25
#define ID_ISTYLE			26
#define ID_REALVIEW			27
#define ID_REALVIEWF		28
#define ID_REALVIEWU		29

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menuslider_s	brightness;
	menuslider_s	screensize;
	menuslider_s	crosssize;
    menuradiobutton_s  thirdperson;
	menuslider_s	thirdpersonrange;
	menuslider_s	thirdpersonoffset;
	menuradiobutton_s  icons;
	menuradiobutton_s  status;
	menuslider_s  gun;
	menuslider_s  istyle;
	menuradiobutton_s  rview;
	menuslider_s  rviewf;
	menuslider_s  rviewu;
	menuradiobutton_s  speed;
	menuradiobutton_s  friend;

	menubitmap_s	back;
} displayOptionsInfo_t;

static displayOptionsInfo_t	displayOptionsInfo;


/*
=================
UI_DisplayOptionsMenu_Event
=================
*/
static void UI_DisplayOptionsMenu_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GRAPHICS:
		UI_PopMenu();
		UI_GraphicsOptionsMenu();
		break;

	case ID_DISPLAY:
		break;

	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;

	case ID_BRIGHTNESS:
		trap_Cvar_SetValue( "r_gamma", displayOptionsInfo.brightness.curvalue / 10.0f );
		break;
	
	case ID_SCREENSIZE:
		trap_Cvar_SetValue( "cg_viewsize", displayOptionsInfo.screensize.curvalue * 10 );
		break;

	case ID_CROSSSIZE:
		trap_Cvar_SetValue( "cg_crosshairScale", displayOptionsInfo.crosssize.curvalue);
		break;

	case ID_THIRDPERSON:
		trap_Cvar_SetValue( "cg_thirdperson", displayOptionsInfo.thirdperson.curvalue);
		break;

	case ID_THIRDPERSONRANGE:
		trap_Cvar_SetValue( "cg_thirdpersonrange", displayOptionsInfo.thirdpersonrange.curvalue);
		break;

	case ID_THIRDPERSONOFFSET:
		trap_Cvar_SetValue( "cg_thirdpersonoffset", displayOptionsInfo.thirdpersonoffset.curvalue);
		break;

	case ID_ICONS:
		trap_Cvar_SetValue( "cg_draw3dIcons", displayOptionsInfo.icons.curvalue);
		break;

	case ID_STATUS:
		trap_Cvar_SetValue( "cg_drawstatus", displayOptionsInfo.status.curvalue);
		break;

	case ID_GUN:
		if(displayOptionsInfo.gun.curvalue == 0){
		trap_Cvar_SetValue( "cg_drawGun", 0);
		}
		if(displayOptionsInfo.gun.curvalue == 1){
		trap_Cvar_SetValue( "cg_drawGun", 2);
		}
		if(displayOptionsInfo.gun.curvalue == 2){
		trap_Cvar_SetValue( "cg_drawGun", 3);
		}
		if(displayOptionsInfo.gun.curvalue == 3){
		trap_Cvar_SetValue( "cg_drawGun", 1);
		}
		break;

	case ID_REALVIEW:
		if(displayOptionsInfo.rview.curvalue){
		trap_Cvar_SetValue( "cg_cameraEyes", 1);
		} else {
		trap_Cvar_SetValue( "cg_cameraEyes", 0);
		}
		break;

	case ID_REALVIEWF:
		trap_Cvar_SetValue( "cg_cameraEyes_Fwd", displayOptionsInfo.rviewf.curvalue);
		break;

	case ID_REALVIEWU:
		trap_Cvar_SetValue( "cg_cameraEyes_Up", displayOptionsInfo.rviewu.curvalue);
		break;

	case ID_SPEED:
		trap_Cvar_SetValue( "cg_drawspeed", displayOptionsInfo.speed.curvalue);
		break;

	case ID_FRIEND:
		trap_Cvar_SetValue( "cg_drawfriend", displayOptionsInfo.friend.curvalue);
		break;

	case ID_ISTYLE:
		trap_Cvar_SetValue( "cg_itemstyle", displayOptionsInfo.istyle.curvalue);
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_DisplayOptionsMenu_Init
===============
*/
static void UI_DisplayOptionsMenu_Init( void ) {
	int		y;

	memset( &displayOptionsInfo, 0, sizeof(displayOptionsInfo) );

	UI_DisplayOptionsMenu_Cache();
	displayOptionsInfo.menu.wrapAround = qtrue;
	displayOptionsInfo.menu.native 	   = qfalse;
	displayOptionsInfo.menu.fullscreen = qtrue;

	displayOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	displayOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	displayOptionsInfo.banner.generic.x			= 320;
	displayOptionsInfo.banner.generic.y			= 16;
	if(cl_language.integer == 0){
	displayOptionsInfo.banner.string			= "SYSTEM SETUP";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.banner.string			= "СИСТЕМНЫЕ НАСТРОЙКИ";
	}
	displayOptionsInfo.banner.color				= color_white;
	displayOptionsInfo.banner.style				= UI_CENTER;

	displayOptionsInfo.framel.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.framel.generic.name		= ART_FRAMEL;
	displayOptionsInfo.framel.generic.flags		= QMF_INACTIVE;
	displayOptionsInfo.framel.generic.x			= 0;  
	displayOptionsInfo.framel.generic.y			= 78;
	displayOptionsInfo.framel.width				= 256;
	displayOptionsInfo.framel.height			= 329;

	displayOptionsInfo.framer.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.framer.generic.name		= ART_FRAMER;
	displayOptionsInfo.framer.generic.flags		= QMF_INACTIVE;
	displayOptionsInfo.framer.generic.x			= 376;
	displayOptionsInfo.framer.generic.y			= 76;
	displayOptionsInfo.framer.width				= 256;
	displayOptionsInfo.framer.height			= 334;

	displayOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	displayOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.graphics.generic.id			= ID_GRAPHICS;
	displayOptionsInfo.graphics.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.graphics.generic.x			= 140- uis.wideoffset;
	displayOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	if(cl_language.integer == 0){
	displayOptionsInfo.graphics.string				= "GRAPHICS";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.graphics.string				= "ГРАФИКА";
	}
	displayOptionsInfo.graphics.style				= UI_RIGHT;
	displayOptionsInfo.graphics.color				= color_white;

	displayOptionsInfo.display.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY;
	displayOptionsInfo.display.generic.id			= ID_DISPLAY;
	displayOptionsInfo.display.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.display.generic.x			= 140- uis.wideoffset;
	displayOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	if(cl_language.integer == 0){
	displayOptionsInfo.display.string				= "DISPLAY";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.display.string				= "ЭКРАН";
	}
	displayOptionsInfo.display.style				= UI_RIGHT;
	displayOptionsInfo.display.color				= color_grey;

	displayOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.sound.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.sound.generic.id				= ID_SOUND;
	displayOptionsInfo.sound.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.sound.generic.x				= 140- uis.wideoffset;
	displayOptionsInfo.sound.generic.y				= 240;
	if(cl_language.integer == 0){
	displayOptionsInfo.sound.string					= "SOUND";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.sound.string					= "ЗВУК";
	}
	displayOptionsInfo.sound.style					= UI_RIGHT;
	displayOptionsInfo.sound.color					= color_white;

	displayOptionsInfo.network.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.network.generic.id			= ID_NETWORK;
	displayOptionsInfo.network.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.network.generic.x			= 140- uis.wideoffset;
	displayOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	if(cl_language.integer == 0){
	displayOptionsInfo.network.string				= "NETWORK";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.network.string				= "СЕТЬ";
	}
	displayOptionsInfo.network.style				= UI_RIGHT;
	displayOptionsInfo.network.color				= color_white;

	y = 120 - 1 * (BIGCHAR_HEIGHT+2);
	displayOptionsInfo.brightness.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	displayOptionsInfo.brightness.generic.name		= "Brightness:";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.brightness.generic.name		= "Яркость:";
	}
	displayOptionsInfo.brightness.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.brightness.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.brightness.generic.id		= ID_BRIGHTNESS;
	displayOptionsInfo.brightness.generic.x			= 400;
	displayOptionsInfo.brightness.generic.y			= y;
	displayOptionsInfo.brightness.minvalue			= 5;
	displayOptionsInfo.brightness.maxvalue			= 20;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.screensize.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	displayOptionsInfo.screensize.generic.name		= "Screen Size:";
	}
	if(cl_language.integer == 1){
	displayOptionsInfo.screensize.generic.name		= "Размер экрана:";
	}
	displayOptionsInfo.screensize.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.screensize.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.screensize.generic.id		= ID_SCREENSIZE;
	displayOptionsInfo.screensize.generic.x			= 400;
	displayOptionsInfo.screensize.generic.y			= y;
	displayOptionsInfo.screensize.minvalue			= 3;
    displayOptionsInfo.screensize.maxvalue			= 10;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.crosssize.generic.type			= MTYPE_SLIDER;
	displayOptionsInfo.crosssize.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.crosssize.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.crosssize.generic.id		= ID_CROSSSIZE;
	displayOptionsInfo.crosssize.generic.x			= 400;
	displayOptionsInfo.crosssize.generic.y			= y;
	displayOptionsInfo.crosssize.minvalue				= 1;
    displayOptionsInfo.crosssize.maxvalue				= 40;

	y += BIGCHAR_HEIGHT+4;
	displayOptionsInfo.thirdperson.generic.type     	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.thirdperson.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.thirdperson.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.thirdperson.generic.id       	= ID_THIRDPERSON;
	displayOptionsInfo.thirdperson.generic.x	       	= 400;
	displayOptionsInfo.thirdperson.generic.y	        = y;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.thirdpersonrange.generic.type			= MTYPE_SLIDER;
	displayOptionsInfo.thirdpersonrange.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.thirdpersonrange.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.thirdpersonrange.generic.id		= ID_THIRDPERSONRANGE;
	displayOptionsInfo.thirdpersonrange.generic.x			= 400;
	displayOptionsInfo.thirdpersonrange.generic.y			= y;
	displayOptionsInfo.thirdpersonrange.minvalue				= 10;
    displayOptionsInfo.thirdpersonrange.maxvalue				= 250;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.thirdpersonoffset.generic.type			= MTYPE_SLIDER;
	displayOptionsInfo.thirdpersonoffset.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.thirdpersonoffset.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.thirdpersonoffset.generic.id		= ID_THIRDPERSONOFFSET;
	displayOptionsInfo.thirdpersonoffset.generic.x			= 400;
	displayOptionsInfo.thirdpersonoffset.generic.y			= y;
	displayOptionsInfo.thirdpersonoffset.minvalue				= -50;
    displayOptionsInfo.thirdpersonoffset.maxvalue				= 50;

	y += BIGCHAR_HEIGHT+4;
	displayOptionsInfo.icons.generic.type     	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.icons.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.icons.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.icons.generic.id       	= ID_ICONS;
	displayOptionsInfo.icons.generic.x	       	= 400;
	displayOptionsInfo.icons.generic.y	        = y;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.status.generic.type     	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.status.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.status.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.status.generic.id       	= ID_STATUS;
	displayOptionsInfo.status.generic.x	       	= 400;
	displayOptionsInfo.status.generic.y	        = y;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.gun.generic.type     	= MTYPE_SLIDER;
	displayOptionsInfo.gun.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.gun.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.gun.generic.id       	= ID_GUN;
	displayOptionsInfo.gun.generic.x	       	= 400;
	displayOptionsInfo.gun.generic.y	        = y;
	displayOptionsInfo.gun.minvalue				= 0;
    displayOptionsInfo.gun.maxvalue				= 3;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.rview.generic.type     	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.rview.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.rview.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.rview.generic.id       	= ID_REALVIEW;
	displayOptionsInfo.rview.generic.x	       	= 400;
	displayOptionsInfo.rview.generic.y	        = y;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.rviewf.generic.type     	= MTYPE_SLIDER;
	displayOptionsInfo.rviewf.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.rviewf.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.rviewf.generic.id       	= ID_REALVIEWF;
	displayOptionsInfo.rviewf.generic.x	       	= 400;
	displayOptionsInfo.rviewf.generic.y	        = y;
	displayOptionsInfo.rviewf.minvalue			= -10;
    displayOptionsInfo.rviewf.maxvalue			= 15;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.rviewu.generic.type     	= MTYPE_SLIDER;
	displayOptionsInfo.rviewu.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.rviewu.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.rviewu.generic.id       	= ID_REALVIEWU;
	displayOptionsInfo.rviewu.generic.x	       	= 400;
	displayOptionsInfo.rviewu.generic.y	        = y;
	displayOptionsInfo.rviewu.minvalue			= -10;
    displayOptionsInfo.rviewu.maxvalue			= 15;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.speed.generic.type     	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.speed.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.speed.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.speed.generic.id       	= ID_SPEED;
	displayOptionsInfo.speed.generic.x	       	= 400;
	displayOptionsInfo.speed.generic.y	        = y;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.friend.generic.type     	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.friend.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.friend.generic.callback 	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.friend.generic.id       	= ID_FRIEND;
	displayOptionsInfo.friend.generic.x	       	= 400;
	displayOptionsInfo.friend.generic.y	        = y;

	y += BIGCHAR_HEIGHT+2;
	displayOptionsInfo.istyle.generic.type			= MTYPE_SLIDER;
	displayOptionsInfo.istyle.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.istyle.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.istyle.generic.id		= ID_ISTYLE;
	displayOptionsInfo.istyle.generic.x			= 400;
	displayOptionsInfo.istyle.generic.y			= y;
	displayOptionsInfo.istyle.minvalue				= 1;
    displayOptionsInfo.istyle.maxvalue				= 3;

	displayOptionsInfo.back.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.back.generic.name		= ART_BACK0;
	displayOptionsInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.back.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.back.generic.id			= ID_BACK;
	displayOptionsInfo.back.generic.x			= 0 - uis.wideoffset;
	displayOptionsInfo.back.generic.y			= 480-64;
	displayOptionsInfo.back.width				= 128;
	displayOptionsInfo.back.height				= 64;
	displayOptionsInfo.back.focuspic			= ART_BACK1;

//TEXT FOR TRANSLATES
if(cl_language.integer == 0){
displayOptionsInfo.banner.string			= "SYSTEM SETUP";
displayOptionsInfo.brightness.generic.name		= "Brightness:";
displayOptionsInfo.screensize.generic.name		= "Screen Size:";
displayOptionsInfo.crosssize.generic.name			= "Croshair size:";
displayOptionsInfo.thirdperson.generic.name	   		= "Third person:";
displayOptionsInfo.thirdpersonrange.generic.name			= "Third person range:";
displayOptionsInfo.thirdpersonoffset.generic.name			= "Third person offset:";
displayOptionsInfo.icons.generic.name	   	= "Draw 3D Icons:";
displayOptionsInfo.status.generic.name	   	= "Draw Status:";
displayOptionsInfo.gun.generic.name	   		= "Draw Gun:";
displayOptionsInfo.rview.generic.name	   	= "Eyes view:";
displayOptionsInfo.rviewf.generic.name	   	= "Eyes view forward:";
displayOptionsInfo.rviewu.generic.name	   	= "Eyes view:";
displayOptionsInfo.speed.generic.name	   	= "Draw Speed:";
displayOptionsInfo.friend.generic.name	   	= "Draw Friend:";
displayOptionsInfo.istyle.generic.name	   	= "Item Style:";
}
if(cl_language.integer == 1){
displayOptionsInfo.banner.string			= "СИСТЕМНЫЕ НАСТРОЙКИ";
displayOptionsInfo.brightness.generic.name		= "Яркость:";
displayOptionsInfo.screensize.generic.name		= "Размер экрана:";
displayOptionsInfo.crosssize.generic.name			= "Размер прицела:";
displayOptionsInfo.thirdperson.generic.name	   		= "Вид от третьего лица:";
displayOptionsInfo.thirdpersonrange.generic.name			= "Вид от третье лица-растояние:";
displayOptionsInfo.thirdpersonoffset.generic.name			= "Вид от третье лица-смещение:";
displayOptionsInfo.icons.generic.name	   	= "Отобразить 3D Значки:";
displayOptionsInfo.status.generic.name	   	= "Отобразить статус:";
displayOptionsInfo.gun.generic.name	   		= "Отобразить оружие:";
displayOptionsInfo.rview.generic.name	   	= "Вид глазами:";
displayOptionsInfo.rviewf.generic.name	   	= "Вид глазами вперёд:";
displayOptionsInfo.rviewu.generic.name	   	= "Вид глазами вверх:";
displayOptionsInfo.speed.generic.name	   	= "Отобразить скорость:";
displayOptionsInfo.friend.generic.name	   	= "Отобразить друга:";
displayOptionsInfo.istyle.generic.name	   	= "Стиль предметов:";
}

	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.banner );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.framel );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.framer );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.graphics );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.display );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.sound );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.network );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.brightness );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.screensize );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.crosssize );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.thirdperson );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.thirdpersonrange );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.thirdpersonoffset );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.icons );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.status );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.gun );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.rview );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.rviewf );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.rviewu );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.speed );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.friend );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.istyle );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.back );

	displayOptionsInfo.brightness.curvalue  = trap_Cvar_VariableValue("r_gamma") * 10;
	displayOptionsInfo.screensize.curvalue  = trap_Cvar_VariableValue( "cg_viewsize")/10;
	displayOptionsInfo.crosssize.curvalue  = trap_Cvar_VariableValue( "cg_crosshairScale");
	displayOptionsInfo.thirdperson.curvalue  = trap_Cvar_VariableValue( "cg_thirdperson");
	displayOptionsInfo.icons.curvalue  = trap_Cvar_VariableValue( "cg_draw3dIcons");
	displayOptionsInfo.status.curvalue  = trap_Cvar_VariableValue( "cg_drawStatus");
	displayOptionsInfo.speed.curvalue  = trap_Cvar_VariableValue( "cg_drawSpeed");
	displayOptionsInfo.friend.curvalue  = trap_Cvar_VariableValue( "cg_drawFriend");
	displayOptionsInfo.thirdpersonrange.curvalue  = trap_Cvar_VariableValue( "cg_thirdpersonrange");
	displayOptionsInfo.thirdpersonoffset.curvalue  = trap_Cvar_VariableValue( "cg_thirdpersonoffset");
	displayOptionsInfo.istyle.curvalue  = trap_Cvar_VariableValue( "cg_itemstyle");
	displayOptionsInfo.rview.curvalue  = trap_Cvar_VariableValue( "cg_cameraEyes");
	displayOptionsInfo.rviewf.curvalue  = trap_Cvar_VariableValue( "cg_cameraEyes_Fwd");
	displayOptionsInfo.rviewu.curvalue  = trap_Cvar_VariableValue( "cg_cameraEyes_Up");

	if(trap_Cvar_VariableValue( "cg_drawGun") == 0){
	displayOptionsInfo.gun.curvalue = 0;
	}
	if(trap_Cvar_VariableValue( "cg_drawGun") == 1){
	displayOptionsInfo.gun.curvalue = 3;
	}
	if(trap_Cvar_VariableValue( "cg_drawGun") == 2){
	displayOptionsInfo.gun.curvalue = 1;
	}
	if(trap_Cvar_VariableValue( "cg_drawGun") == 3){
	displayOptionsInfo.gun.curvalue = 2;
	}
}


/*
===============
UI_DisplayOptionsMenu_Cache
===============
*/
void UI_DisplayOptionsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_DisplayOptionsMenu
===============
*/
void UI_DisplayOptionsMenu( void ) {
	UI_DisplayOptionsMenu_Init();
	UI_PushMenu( &displayOptionsInfo.menu );
	Menu_SetCursorToItem( &displayOptionsInfo.menu, &displayOptionsInfo.display );
}
