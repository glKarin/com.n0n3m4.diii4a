/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/*
=======================================================================

DEMOS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"	
#define ART_GO0				"menu/art/play_0"
#define ART_GO1				"menu/art/play_1"
#define ART_LEFT0       "menu/art/arrow_l0"
#define ART_LEFT1       "menu/art/arrow_l1"
#define ART_RIGHT0      "menu/art/arrow_r0"
#define ART_RIGHT1      "menu/art/arrow_r1"

#define MAX_DEMOS			1024
#define NAMEBUFSIZE			(MAX_DEMOS * 32)

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_RIGHT			13
#define ID_LEFT				14



typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menulist_s		list;

	menubitmap_s	left;
	menubitmap_s	right;
	menutext_s	back;
	menutext_s	go;

	int				numDemos;
	char			names[NAMEBUFSIZE];
	
	char			*demolist[MAX_DEMOS];
} demos_t;

static demos_t	s_demos;


/*
===============
Demos_MenuEvent
===============
*/
static void Demos_MenuEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		UI_ForceMenuOff ();
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo %s\n",
								s_demos.list.itemnames[s_demos.list.curvalue]) );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_LEFT:
		ScrollList_Key( &s_demos.list, K_LEFTARROW );
		break;

	case ID_RIGHT:
		ScrollList_Key( &s_demos.list, K_RIGHTARROW );
		break;
	}
}


/*
===============
Demos_MenuInit
===============
*/
static void Demos_MenuInit( void ) {
	int		i, j;
	int		len;
	char	*demoname, extension[32];
	int	protocol, protocolLegacy;

	memset( &s_demos, 0 ,sizeof(demos_t) );

	Demos_Cache();

	s_demos.menu.fullscreen = qtrue;
	s_demos.menu.wrapAround = qtrue;

	s_demos.banner.generic.type		= MTYPE_BTEXT;
	s_demos.banner.generic.x		= 320;
	s_demos.banner.generic.y		= 16;
	s_demos.banner.string			= "DEMOS";
	s_demos.banner.color			= color_white;
	s_demos.banner.style			= UI_CENTER;

	s_demos.left.generic.type		= MTYPE_BITMAP;
	s_demos.left.generic.name		= ART_LEFT0;
	s_demos.left.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_demos.left.generic.x			= 260;
	s_demos.left.generic.y			= 480-70;
	s_demos.left.generic.id			= ID_LEFT;
	s_demos.left.generic.callback	= Demos_MenuEvent;
	s_demos.left.width				= 20;
	s_demos.left.height				= 20;
	s_demos.left.focuspic			= ART_LEFT1;

	s_demos.right.generic.type		= MTYPE_BITMAP;
	s_demos.right.generic.name		= ART_RIGHT0;
	s_demos.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_demos.right.generic.x			= 400;
	s_demos.right.generic.y			= 480-70;
	s_demos.right.generic.id		= ID_RIGHT;
	s_demos.right.generic.callback	= Demos_MenuEvent;
	s_demos.right.width				= 20;
	s_demos.right.height			= 20;
	s_demos.right.focuspic			= ART_RIGHT1;

	s_demos.back.generic.type		= MTYPE_PTEXT;
	s_demos.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_demos.back.generic.id			= ID_BACK;
	s_demos.back.generic.callback	= Demos_MenuEvent;
	s_demos.back.generic.x			= 20;
	s_demos.back.generic.y			= 480-64;
	s_demos.back.string				= "< BACK";
	s_demos.back.color				= text_color_normal;
	s_demos.back.style				= UI_LEFT | UI_SMALLFONT;

	s_demos.go.generic.type			= MTYPE_PTEXT;
	s_demos.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_demos.go.generic.id			= ID_GO;
	s_demos.go.generic.callback		= Demos_MenuEvent;
	s_demos.go.generic.x			= 640 - 20;
	s_demos.go.generic.y			= 480-64;
	s_demos.go.string				= "GO >";
	s_demos.go.color				= text_color_normal;
	s_demos.go.style				= UI_RIGHT | UI_SMALLFONT;

	s_demos.list.generic.type		= MTYPE_SCROLLLIST;
	s_demos.list.generic.flags		= QMF_PULSEIFFOCUS;
	s_demos.list.generic.callback	= Demos_MenuEvent;
	s_demos.list.generic.id			= ID_LIST;
	s_demos.list.generic.x			= 118;
	s_demos.list.generic.y			= 130;
	s_demos.list.width				= 16;
	s_demos.list.height				= 14;
	s_demos.list.itemnames			= (const char **)s_demos.demolist;
	s_demos.list.columns			= 3;

	protocolLegacy = trap_Cvar_VariableValue("com_legacyprotocol");
	protocol = trap_Cvar_VariableValue("com_protocol");

	if(!protocol)
		protocol = trap_Cvar_VariableValue("protocol");
	if(protocolLegacy == protocol)
		protocolLegacy = 0;

	Com_sprintf(extension, sizeof(extension), ".%s%d", DEMOEXT, protocol);
	s_demos.numDemos = trap_FS_GetFileList("demos", extension, s_demos.names, ARRAY_LEN(s_demos.names));

	demoname = s_demos.names;
	i = 0;

	for(j = 0; j < 2; j++)
	{
		if(s_demos.numDemos > MAX_DEMOS)
			s_demos.numDemos = MAX_DEMOS;

		for(; i < s_demos.numDemos; i++)
		{
			s_demos.list.itemnames[i] = demoname;
		
			len = strlen(demoname);

			demoname += len + 1;
		}

		if(!j)
		{
			if(protocolLegacy > 0 && s_demos.numDemos < MAX_DEMOS)
			{
				Com_sprintf(extension, sizeof(extension), ".%s%d", DEMOEXT, protocolLegacy);
				s_demos.numDemos += trap_FS_GetFileList("demos", extension, demoname,
									ARRAY_LEN(s_demos.names) - (demoname - s_demos.names));
			}
			else
				break;
		}
	}

	s_demos.list.numitems = s_demos.numDemos;

	if(!s_demos.numDemos)
	{
		s_demos.list.itemnames[0] = "No Demos Found.";
		s_demos.list.numitems = 1;

		//degenerate case, not selectable
		s_demos.go.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}

	Menu_AddItem( &s_demos.menu, &s_demos.banner );
	Menu_AddItem( &s_demos.menu, &s_demos.list );
	Menu_AddItem( &s_demos.menu, &s_demos.left );
	Menu_AddItem( &s_demos.menu, &s_demos.right );
	Menu_AddItem( &s_demos.menu, &s_demos.back );
	Menu_AddItem( &s_demos.menu, &s_demos.go );
}

/*
=================
Demos_Cache
=================
*/
void Demos_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_GO0 );
	trap_R_RegisterShaderNoMip( ART_GO1 );
	trap_R_RegisterShaderNoMip( ART_LEFT0 );
	trap_R_RegisterShaderNoMip( ART_LEFT1 );
	trap_R_RegisterShaderNoMip( ART_RIGHT0 );
	trap_R_RegisterShaderNoMip( ART_RIGHT1 );
}

/*
===============
UI_DemosMenu
===============
*/
void UI_DemosMenu( void ) {
	Demos_MenuInit();
	UI_PushMenu( &s_demos.menu );
}
