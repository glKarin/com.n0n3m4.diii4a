/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/*
=============================================================================

LOAD CONFIG MENU

=============================================================================
*/

#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_FIGHT0			"menu/art/load_0"
#define ART_FIGHT1			"menu/art/load_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_ARROWS			"menu/art/arrows_horz_0"
#define ART_ARROWLEFT		"menu/art/arrows_horz_left"
#define ART_ARROWRIGHT		"menu/art/arrows_horz_right"
#define ART_BACKGROUND		"menu/art/cut_frame"

#define MAX_MAPFILES		4096
#define MAPNAMEBUFSIZE			( MAX_MAPFILES * 16 )

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_LEFT				13
#define ID_RIGHT			14

#define ARROWS_WIDTH		128
#define ARROWS_HEIGHT		48


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		list;

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menubitmap_s	back;
	menubitmap_s	go;
	menubitmap_s	background;

	char			names[524288];
	char*			configlist[65536];
} s_loadMapEd_t;

static s_loadMapEd_t	s_loadMapEd;


/*
===============
loadMapEd_MenuEvent
===============
*/
static void loadMapEd_MenuEvent( void *ptr, int event ) {

	if( event != QM_ACTIVATED ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "loadmap maps/%s.add\n", s_loadMapEd.list.itemnames[s_loadMapEd.list.curvalue] ) );
		UI_PopMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_LEFT:
		ScrollList_Key( &s_loadMapEd.list, K_LEFTARROW );
		break;

	case ID_RIGHT:
		ScrollList_Key( &s_loadMapEd.list, K_RIGHTARROW );
		break;
	}
}


/*
===============
loadMapEd_MenuInit
===============
*/
static void loadMapEd_MenuInit( void ) {
	int		i;
	int		len;
	char	*configname;

	UI_loadMapEd_Cache();

	memset( &s_loadMapEd, 0 ,sizeof(s_loadMapEd_t) );
	s_loadMapEd.menu.wrapAround = qtrue;
	s_loadMapEd.menu.native 	= qfalse;
	s_loadMapEd.menu.fullscreen = qfalse;

	s_loadMapEd.banner.generic.type	= MTYPE_BTEXT;
	s_loadMapEd.banner.generic.x		= 320;
	s_loadMapEd.banner.generic.y		= 16;
	if(cl_language.integer == 0){
	s_loadMapEd.banner.string			= "Load Map";
	}
	if(cl_language.integer == 1){
	s_loadMapEd.banner.string			= "Загрузить Карту";
	}
	s_loadMapEd.banner.color			= color_white;
	s_loadMapEd.banner.style			= UI_CENTER;
	
	s_loadMapEd.background.generic.type		= MTYPE_BITMAP;
	s_loadMapEd.background.generic.name		= ART_BACKGROUND;
	s_loadMapEd.background.generic.flags		= QMF_INACTIVE;
	s_loadMapEd.background.generic.x			= -10000000;
	s_loadMapEd.background.generic.y			= -1000;
	s_loadMapEd.background.width				= 46600000;
	s_loadMapEd.background.height			= 33200000;

	s_loadMapEd.framel.generic.type	= MTYPE_BITMAP;
	s_loadMapEd.framel.generic.name	= ART_FRAMEL;
	s_loadMapEd.framel.generic.flags	= QMF_INACTIVE;
	s_loadMapEd.framel.generic.x		= 0;
	s_loadMapEd.framel.generic.y		= 78;
	s_loadMapEd.framel.width			= 256;
	s_loadMapEd.framel.height			= 329;

	s_loadMapEd.framer.generic.type	= MTYPE_BITMAP;
	s_loadMapEd.framer.generic.name	= ART_FRAMER;
	s_loadMapEd.framer.generic.flags	= QMF_INACTIVE;
	s_loadMapEd.framer.generic.x		= 376;
	s_loadMapEd.framer.generic.y		= 76;
	s_loadMapEd.framer.width			= 256;
	s_loadMapEd.framer.height			= 334;

	s_loadMapEd.arrows.generic.type	= MTYPE_BITMAP;
	s_loadMapEd.arrows.generic.name	= ART_ARROWS;
	s_loadMapEd.arrows.generic.flags	= QMF_INACTIVE;
	s_loadMapEd.arrows.generic.x		= 320-ARROWS_WIDTH/2;
	s_loadMapEd.arrows.generic.y		= 400;
	s_loadMapEd.arrows.width			= ARROWS_WIDTH;
	s_loadMapEd.arrows.height			= ARROWS_HEIGHT;

	s_loadMapEd.left.generic.type		= MTYPE_BITMAP;
	s_loadMapEd.left.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_loadMapEd.left.generic.x		= 320-ARROWS_WIDTH/2;
	s_loadMapEd.left.generic.y		= 400;
	s_loadMapEd.left.generic.id		= ID_LEFT;
	s_loadMapEd.left.generic.callback	= loadMapEd_MenuEvent;
	s_loadMapEd.left.width			= ARROWS_WIDTH/2;
	s_loadMapEd.left.height			= ARROWS_HEIGHT;
	s_loadMapEd.left.focuspic			= ART_ARROWLEFT;

	s_loadMapEd.right.generic.type	= MTYPE_BITMAP;
	s_loadMapEd.right.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_loadMapEd.right.generic.x		= 320;
	s_loadMapEd.right.generic.y		= 400;
	s_loadMapEd.right.generic.id		= ID_RIGHT;
	s_loadMapEd.right.generic.callback = loadMapEd_MenuEvent;
	s_loadMapEd.right.width			= ARROWS_WIDTH/2;
	s_loadMapEd.right.height			= ARROWS_HEIGHT;
	s_loadMapEd.right.focuspic		= ART_ARROWRIGHT;

	s_loadMapEd.back.generic.type		= MTYPE_BITMAP;
	s_loadMapEd.back.generic.name		= ART_BACK0;
	s_loadMapEd.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_loadMapEd.back.generic.id		= ID_BACK;
	s_loadMapEd.back.generic.callback	= loadMapEd_MenuEvent;
	s_loadMapEd.back.generic.x		= 0 - uis.wideoffset;
	s_loadMapEd.back.generic.y		= 480-64;
	s_loadMapEd.back.width			= 128;
	s_loadMapEd.back.height			= 64;
	s_loadMapEd.back.focuspic			= ART_BACK1;

	s_loadMapEd.go.generic.type		= MTYPE_BITMAP;
	s_loadMapEd.go.generic.name		= ART_FIGHT0;
	s_loadMapEd.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_loadMapEd.go.generic.id			= ID_GO;
	s_loadMapEd.go.generic.callback	= loadMapEd_MenuEvent;
	s_loadMapEd.go.generic.x			= 640 + uis.wideoffset;
	s_loadMapEd.go.generic.y			= 480-64;
	s_loadMapEd.go.width				= 128;
	s_loadMapEd.go.height				= 64;
	s_loadMapEd.go.focuspic			= ART_FIGHT1;

	// scan for configs
	s_loadMapEd.list.generic.type		= MTYPE_SCROLLLIST;
	s_loadMapEd.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_loadMapEd.list.generic.callback	= loadMapEd_MenuEvent;
	s_loadMapEd.list.generic.id		= ID_LIST;
	s_loadMapEd.list.generic.x		= 118;
	s_loadMapEd.list.generic.y		= 130;
	s_loadMapEd.list.width			= 16;
	s_loadMapEd.list.height			= 14;
	s_loadMapEd.list.numitems			= trap_FS_GetFileList( "maps", "add", s_loadMapEd.names, 524288 );
	s_loadMapEd.list.itemnames		= (const char **)s_loadMapEd.configlist;
	s_loadMapEd.list.columns			= 1;

	if (!s_loadMapEd.list.numitems) {
		if(cl_language.integer == 0){
		strcpy(s_loadMapEd.names,"No MapFiles Found.");
		}
		if(cl_language.integer == 1){
		strcpy(s_loadMapEd.names,"Файлы карт не найдены.");
		}
		s_loadMapEd.list.numitems = 1;

		//degenerate case, not selectable
		s_loadMapEd.go.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else if (s_loadMapEd.list.numitems > MAX_MAPFILES)
		s_loadMapEd.list.numitems = MAX_MAPFILES;

	configname = s_loadMapEd.names;
	for ( i = 0; i < s_loadMapEd.list.numitems; i++ ) {
		s_loadMapEd.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 4,".add"))
			configname[len-4] = '\0';

		configname += len + 1;
	}

	Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.background );
	Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.banner );
	//Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.framel );
	//Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.framer );
	Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.list );
	//Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.arrows );
	//Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.left );
	//Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.right );
	Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.back );
	Menu_AddItem( &s_loadMapEd.menu, &s_loadMapEd.go );
}

/*
=================
UI_loadMapEd_Cache
=================
*/
void UI_loadMapEd_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWLEFT );
	trap_R_RegisterShaderNoMip( ART_ARROWRIGHT );
}


/*
===============
UI_loadMapEdMenu
===============
*/
void UI_loadMapEdMenu( void ) {
	loadMapEd_MenuInit();
	UI_PushMenu( &s_loadMapEd.menu );
}



















/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/*
=============================================================================

LOAD CONFIG MENU

=============================================================================
*/

#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_FIGHT0			"menu/art/load_0"
#define ART_FIGHT1			"menu/art/load_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_ARROWS			"menu/art/arrows_horz_0"
#define ART_ARROWLEFT		"menu/art/arrows_horz_left"
#define ART_ARROWRIGHT		"menu/art/arrows_horz_right"

/*#define MAX_MAPFILES		512
#define MAPNAMEBUFSIZE			( MAX_MAPFILES * 16 )*/

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_LEFT				13
#define ID_RIGHT			14

#define ARROWS_WIDTH		128
#define ARROWS_HEIGHT		48


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		list;

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menubitmap_s	back;
	menubitmap_s	go;

	char			names[MAPNAMEBUFSIZE];
	char*			configlist[MAX_MAPFILES];
} s_selecttoolEd_t;

static s_selecttoolEd_t	s_selecttoolEd;


/*
===============
selecttoolEd_MenuEvent
===============
*/
static void selecttoolEd_MenuEvent( void *ptr, int event ) {

	if( event != QM_ACTIVATED ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		trap_Cvar_SetValue( "toolgun_tool", s_selecttoolEd.list.curvalue);
		UI_PopMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_LEFT:
		ScrollList_Key( &s_selecttoolEd.list, K_LEFTARROW );
		break;

	case ID_RIGHT:
		ScrollList_Key( &s_selecttoolEd.list, K_RIGHTARROW );
		break;
	}
}

/*
=================
UI_selecttoolEd_Cache
=================
*/
void UI_selecttoolEd_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWLEFT );
	trap_R_RegisterShaderNoMip( ART_ARROWRIGHT );
}

/*
===============
selecttoolEd_MenuInit
===============
*/
static void selecttoolEd_MenuInit( void ) {
	int		i;
	int		len;
	char	*configname;

	UI_selecttoolEd_Cache();

	memset( &s_selecttoolEd, 0 ,sizeof(s_selecttoolEd_t) );
	s_selecttoolEd.menu.wrapAround = qtrue;
	s_selecttoolEd.menu.native 	   = qfalse;
	s_selecttoolEd.menu.fullscreen = qtrue;

	s_selecttoolEd.banner.generic.type	= MTYPE_BTEXT;
	s_selecttoolEd.banner.generic.x		= 320;
	s_selecttoolEd.banner.generic.y		= 16;
	s_selecttoolEd.banner.string			= "Select tool";
	s_selecttoolEd.banner.color			= color_white;
	s_selecttoolEd.banner.style			= UI_CENTER;

	s_selecttoolEd.framel.generic.type	= MTYPE_BITMAP;
	s_selecttoolEd.framel.generic.name	= ART_FRAMEL;
	s_selecttoolEd.framel.generic.flags	= QMF_INACTIVE;
	s_selecttoolEd.framel.generic.x		= 0;
	s_selecttoolEd.framel.generic.y		= 78;
	s_selecttoolEd.framel.width			= 256;
	s_selecttoolEd.framel.height			= 329;

	s_selecttoolEd.framer.generic.type	= MTYPE_BITMAP;
	s_selecttoolEd.framer.generic.name	= ART_FRAMER;
	s_selecttoolEd.framer.generic.flags	= QMF_INACTIVE;
	s_selecttoolEd.framer.generic.x		= 376;
	s_selecttoolEd.framer.generic.y		= 76;
	s_selecttoolEd.framer.width			= 256;
	s_selecttoolEd.framer.height			= 334;

	s_selecttoolEd.arrows.generic.type	= MTYPE_BITMAP;
	s_selecttoolEd.arrows.generic.name	= ART_ARROWS;
	s_selecttoolEd.arrows.generic.flags	= QMF_INACTIVE;
	s_selecttoolEd.arrows.generic.x		= 320-ARROWS_WIDTH/2;
	s_selecttoolEd.arrows.generic.y		= 400;
	s_selecttoolEd.arrows.width			= ARROWS_WIDTH;
	s_selecttoolEd.arrows.height			= ARROWS_HEIGHT;

	s_selecttoolEd.left.generic.type		= MTYPE_BITMAP;
	s_selecttoolEd.left.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_selecttoolEd.left.generic.x		= 320-ARROWS_WIDTH/2;
	s_selecttoolEd.left.generic.y		= 400;
	s_selecttoolEd.left.generic.id		= ID_LEFT;
	s_selecttoolEd.left.generic.callback	= selecttoolEd_MenuEvent;
	s_selecttoolEd.left.width			= ARROWS_WIDTH/2;
	s_selecttoolEd.left.height			= ARROWS_HEIGHT;
	s_selecttoolEd.left.focuspic			= ART_ARROWLEFT;

	s_selecttoolEd.right.generic.type	= MTYPE_BITMAP;
	s_selecttoolEd.right.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_selecttoolEd.right.generic.x		= 320;
	s_selecttoolEd.right.generic.y		= 400;
	s_selecttoolEd.right.generic.id		= ID_RIGHT;
	s_selecttoolEd.right.generic.callback = selecttoolEd_MenuEvent;
	s_selecttoolEd.right.width			= ARROWS_WIDTH/2;
	s_selecttoolEd.right.height			= ARROWS_HEIGHT;
	s_selecttoolEd.right.focuspic		= ART_ARROWRIGHT;

	s_selecttoolEd.back.generic.type		= MTYPE_BITMAP;
	s_selecttoolEd.back.generic.name		= ART_BACK0;
	s_selecttoolEd.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_selecttoolEd.back.generic.id		= ID_BACK;
	s_selecttoolEd.back.generic.callback	= selecttoolEd_MenuEvent;
	s_selecttoolEd.back.generic.x		= 0 - uis.wideoffset;
	s_selecttoolEd.back.generic.y		= 480-64;
	s_selecttoolEd.back.width			= 128;
	s_selecttoolEd.back.height			= 64;
	s_selecttoolEd.back.focuspic			= ART_BACK1;

	s_selecttoolEd.go.generic.type		= MTYPE_BITMAP;
	s_selecttoolEd.go.generic.name		= ART_FIGHT0;
	s_selecttoolEd.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_selecttoolEd.go.generic.id			= ID_GO;
	s_selecttoolEd.go.generic.callback	= selecttoolEd_MenuEvent;
	s_selecttoolEd.go.generic.x			= 640 + uis.wideoffset;
	s_selecttoolEd.go.generic.y			= 480-64;
	s_selecttoolEd.go.width				= 128;
	s_selecttoolEd.go.height				= 64;
	s_selecttoolEd.go.focuspic			= ART_FIGHT1;

	// scan for configs
	s_selecttoolEd.list.generic.type		= MTYPE_SCROLLLIST;
	s_selecttoolEd.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_selecttoolEd.list.generic.callback	= selecttoolEd_MenuEvent;
	s_selecttoolEd.list.generic.id		= ID_LIST;
	s_selecttoolEd.list.generic.x		= 118;
	s_selecttoolEd.list.generic.y		= 50;
	s_selecttoolEd.list.width			= 36;
	s_selecttoolEd.list.height			= 28;
	s_selecttoolEd.list.numitems		= 143;
	s_selecttoolEd.list.itemnames		= (const char **)s_selecttoolEd.configlist;
	s_selecttoolEd.list.columns			= 1;

	if (!s_selecttoolEd.list.numitems) {
		strcpy(s_selecttoolEd.names,"No Tools Found.");
		s_selecttoolEd.list.numitems = 1;

		//degenerate case, not selectable
		s_selecttoolEd.go.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else if (s_selecttoolEd.list.numitems > MAX_MAPFILES)
		s_selecttoolEd.list.numitems = MAX_MAPFILES;

	configname = s_selecttoolEd.names;
s_selecttoolEd.list.itemnames[0] = "Build Tool";
s_selecttoolEd.list.itemnames[1] = "Delete Tool";
s_selecttoolEd.list.itemnames[2] = "Gravity Tool";
s_selecttoolEd.list.itemnames[3] = "Freeze Tool";
s_selecttoolEd.list.itemnames[4] = "Linear Tool";
s_selecttoolEd.list.itemnames[5] = "Wait Tool";
s_selecttoolEd.list.itemnames[6] = "Model Tool (1 to 500+)";
s_selecttoolEd.list.itemnames[7] = "Spawnflags Tool";
s_selecttoolEd.list.itemnames[8] = "Speed Tool";
s_selecttoolEd.list.itemnames[9] = "Count Tool";
s_selecttoolEd.list.itemnames[10] = "Health Tool";
s_selecttoolEd.list.itemnames[11] = "Damage Tool";
s_selecttoolEd.list.itemnames[12] = "Target Tool (1 to 100)";
s_selecttoolEd.list.itemnames[13] = "Targetname Tool (1 to 100)";
s_selecttoolEd.list.itemnames[14] = "Door Tool";
s_selecttoolEd.list.itemnames[15] = "Button Tool";
s_selecttoolEd.list.itemnames[16] = "Pendulum Tool";
s_selecttoolEd.list.itemnames[17] = "Bobbing Tool";
s_selecttoolEd.list.itemnames[18] = "Rotating Tool";
s_selecttoolEd.list.itemnames[19] = "Static Tool";
s_selecttoolEd.list.itemnames[20] = "Plat Tool";
s_selecttoolEd.list.itemnames[21] = "Height Tool";
s_selecttoolEd.list.itemnames[22] = "Random Tool";
s_selecttoolEd.list.itemnames[23] = "Set Speed Tool";
s_selecttoolEd.list.itemnames[24] = "Bounce Reset Tool";
s_selecttoolEd.list.itemnames[25] = "Half Bounce Tool";
s_selecttoolEd.list.itemnames[26] = "Full Bounce Tool";
s_selecttoolEd.list.itemnames[27] = "Rocket Tool";
s_selecttoolEd.list.itemnames[28] = "Grenade Tool";
s_selecttoolEd.list.itemnames[29] = "Plasma Tool";
s_selecttoolEd.list.itemnames[30] = "Remove Powerups Tool";
s_selecttoolEd.list.itemnames[31] = "Score Tool";
s_selecttoolEd.list.itemnames[32] = "Sound Tool";
s_selecttoolEd.list.itemnames[33] = "Music Tool";
s_selecttoolEd.list.itemnames[34] = "Cmd Tool";
s_selecttoolEd.list.itemnames[35] = "Model Tool";
s_selecttoolEd.list.itemnames[36] = "Legs Tool";
s_selecttoolEd.list.itemnames[37] = "Head Tool";
s_selecttoolEd.list.itemnames[38] = "Teleporter Tool";
s_selecttoolEd.list.itemnames[39] = "Kill Tool";
s_selecttoolEd.list.itemnames[40] = "Push Tool";
s_selecttoolEd.list.itemnames[41] = "Give Tool";
s_selecttoolEd.list.itemnames[42] = "Print Tool";
s_selecttoolEd.list.itemnames[43] = "Teleporter Dest Tool";
s_selecttoolEd.list.itemnames[44] = "FFA Spawn Tool";
s_selecttoolEd.list.itemnames[45] = "Red Spawn Tool";
s_selecttoolEd.list.itemnames[46] = "Blue Spawn Tool";
s_selecttoolEd.list.itemnames[47] = "Red Base Tool";
s_selecttoolEd.list.itemnames[48] = "Blue Base Tool";
s_selecttoolEd.list.itemnames[49] = "Neutral Base Tool";
s_selecttoolEd.list.itemnames[50] = "Domination Point Tool";
s_selecttoolEd.list.itemnames[51] = "Message Tool";
s_selecttoolEd.list.itemnames[52] = "Create Collision Tool";
s_selecttoolEd.list.itemnames[53] = "Delete Collision Tool";
s_selecttoolEd.list.itemnames[54] = "Respawn Tool";
s_selecttoolEd.list.itemnames[55] = "Move X Tool";
s_selecttoolEd.list.itemnames[56] = "Move Y Tool";
s_selecttoolEd.list.itemnames[57] = "Move Z Tool";
s_selecttoolEd.list.itemnames[58] = "Replace Tool";
s_selecttoolEd.list.itemnames[59] = "Get Classname Tool";
s_selecttoolEd.list.itemnames[60] = "Get Target Tool";
s_selecttoolEd.list.itemnames[61] = "Get Targetname Tool";
s_selecttoolEd.list.itemnames[62] = "Position Tool";
s_selecttoolEd.list.itemnames[63] = "T Multiple Tool (1 to 9)";
s_selecttoolEd.list.itemnames[64] = "T Push Tool (1 to 9)";
s_selecttoolEd.list.itemnames[65] = "T Teleport Tool (1 to 9)";
s_selecttoolEd.list.itemnames[66] = "T Hurt Tool (1 to 9)";
s_selecttoolEd.list.itemnames[67] = "Size Collision Tool(1 to 11)";
s_selecttoolEd.list.itemnames[68] = "Targetname Tool Player (1 to 100)";
s_selecttoolEd.list.itemnames[69] = "Noclip Tool";
s_selecttoolEd.list.itemnames[70] = "Material Tool (1 to 169)";
s_selecttoolEd.list.itemnames[71] = "Armor Shard";
s_selecttoolEd.list.itemnames[72] = "Armor";
s_selecttoolEd.list.itemnames[73] = "Heavy Armor";
s_selecttoolEd.list.itemnames[74] = "5 Health";
s_selecttoolEd.list.itemnames[75] = "25 Health";
s_selecttoolEd.list.itemnames[76] = "50 Health";
s_selecttoolEd.list.itemnames[77] = "Mega Health";
s_selecttoolEd.list.itemnames[78] = "Shotgun";
s_selecttoolEd.list.itemnames[79] = "Machinegun";
s_selecttoolEd.list.itemnames[80] = "Grenade Launcher";
s_selecttoolEd.list.itemnames[81] = "Rocket Launcher";
s_selecttoolEd.list.itemnames[82] = "Lightning Gun";
s_selecttoolEd.list.itemnames[83] = "Railgun";
s_selecttoolEd.list.itemnames[84] = "Plasma Gun";
s_selecttoolEd.list.itemnames[85] = "BFG10K";
s_selecttoolEd.list.itemnames[86] = "Grappling Hook";
s_selecttoolEd.list.itemnames[87] = "Shells";
s_selecttoolEd.list.itemnames[88] = "Bullets";
s_selecttoolEd.list.itemnames[89] = "Grenades";
s_selecttoolEd.list.itemnames[90] = "Cells";
s_selecttoolEd.list.itemnames[91] = "Lightning";
s_selecttoolEd.list.itemnames[92] = "Rockets";
s_selecttoolEd.list.itemnames[93] = "Slugs";
s_selecttoolEd.list.itemnames[94] = "Bfg Ammo";
s_selecttoolEd.list.itemnames[95] = "Personal Teleporter";
s_selecttoolEd.list.itemnames[96] = "Medkit";
s_selecttoolEd.list.itemnames[97] = "Quad Damage";
s_selecttoolEd.list.itemnames[98] = "Battle Suit";
s_selecttoolEd.list.itemnames[99] = "Speed";
s_selecttoolEd.list.itemnames[100] = "Invisibility";
s_selecttoolEd.list.itemnames[101] = "Regeneration";
s_selecttoolEd.list.itemnames[102] = "Flight";
s_selecttoolEd.list.itemnames[103] = "Red Flag";
s_selecttoolEd.list.itemnames[104] = "Blue Flag";
s_selecttoolEd.list.itemnames[105] = "Kamikaze";
s_selecttoolEd.list.itemnames[106] = "Portal";
s_selecttoolEd.list.itemnames[107] = "Invulnerability";
s_selecttoolEd.list.itemnames[108] = "Nails";
s_selecttoolEd.list.itemnames[109] = "Proximity Mines";
s_selecttoolEd.list.itemnames[110] = "Chaingun Belt";
s_selecttoolEd.list.itemnames[111] = "Scout";
s_selecttoolEd.list.itemnames[112] = "Guard";
s_selecttoolEd.list.itemnames[113] = "Doubler";
s_selecttoolEd.list.itemnames[114] = "Ammo Regen";
s_selecttoolEd.list.itemnames[115] = "Neutral Flag";
s_selecttoolEd.list.itemnames[116] = "Nailgun";
s_selecttoolEd.list.itemnames[117] = "Prox Launcher";
s_selecttoolEd.list.itemnames[118] = "Chaingun";
s_selecttoolEd.list.itemnames[119] = "Flamethrower";
s_selecttoolEd.list.itemnames[120] = "Flame";
s_selecttoolEd.list.itemnames[121] = "Dark Flare";
s_selecttoolEd.list.itemnames[122] = "Point A";
s_selecttoolEd.list.itemnames[123] = "Point B";
s_selecttoolEd.list.itemnames[124] = "Neutral domination point";
s_selecttoolEd.list.itemnames[125] = "Noise tool";
s_selecttoolEd.list.itemnames[126] = "Restore Model tool";
s_selecttoolEd.list.itemnames[127] = "Object permissions tool";
s_selecttoolEd.list.itemnames[128] = "Get object permissions";
s_selecttoolEd.list.itemnames[129] = "Prop locker tool";
s_selecttoolEd.list.itemnames[130] = "Set Point 1";
s_selecttoolEd.list.itemnames[131] = "Set Point 2";
s_selecttoolEd.list.itemnames[132] = "Set Point 3";
s_selecttoolEd.list.itemnames[133] = "Teleport Point 1";
s_selecttoolEd.list.itemnames[134] = "Teleport Point 2";
s_selecttoolEd.list.itemnames[135] = "Teleport Point 3";
s_selecttoolEd.list.itemnames[136] = "Teleport Me Point 1";
s_selecttoolEd.list.itemnames[137] = "Teleport Me Point 2";
s_selecttoolEd.list.itemnames[138] = "Teleport Me Point 3";
s_selecttoolEd.list.itemnames[139] = "Grab Item";
s_selecttoolEd.list.itemnames[140] = "Grab Items and weapons";
s_selecttoolEd.list.itemnames[141] = "Targetname Tool Me (1 to 20)";

	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.banner );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.framel );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.framer );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.list );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.arrows );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.left );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.right );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.back );
	Menu_AddItem( &s_selecttoolEd.menu, &s_selecttoolEd.go );
}


/*
===============
UI_selecttoolEdMenu
===============
*/
void UI_selecttoolEdMenu( void ) {
	selecttoolEd_MenuInit();
	UI_PushMenu( &s_selecttoolEd.menu );
}
