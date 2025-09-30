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

/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/


#include "ui_local.h"


// STONELANCEgametype
/*
#define GAMESERVER_BACK0		"menu/art/back_0"
#define GAMESERVER_BACK1		"menu/art/back_1"
#define GAMESERVER_NEXT0		"menu/art/next_0"
#define GAMESERVER_NEXT1		"menu/art/next_1"
#define GAMESERVER_FRAMEL		"menu/art/frame2_l"
#define GAMESERVER_FRAMER		"menu/art/frame1_r"
*/
// END
#define GAMESERVER_SELECT		"menu/art/maps_select"
#define GAMESERVER_SELECTED		"menu/art/maps_selected"
// STONELANCE
/*
#define GAMESERVER_FIGHT0		"menu/art/fight_0"
#define GAMESERVER_FIGHT1		"menu/art/fight_1"
*/
// END
#define GAMESERVER_UNKNOWNMAP	"menu/art/unknownmap"
// STONELANCE
/*
#define GAMESERVER_ARROWS		"menu/art/gs_arrows_0"
#define GAMESERVER_ARROWSL		"menu/art/gs_arrows_l"
#define GAMESERVER_ARROWSR		"menu/art/gs_arrows_r"
*/
// END

// STONELANCE
#define MAX_MAPSPERPAGE		20

#define MAX_STATSPERPAGE	5
#define MAX_MAPSTATS		16
// END

#define	MAX_SERVERSTEXT	8192

#define MAX_SERVERMAPS	64
#define MAX_NAMELENGTH	28

#define ID_GAMETYPE				10
// STONELANCE
#define ID_PICTURE				11
// END
#define ID_PREVPAGE				15
#define ID_NEXTPAGE				16
#define ID_STARTSERVERBACK		17
#define ID_STARTSERVERNEXT		18
#define ID_LIST					19


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menulist_s		gametype;
// STONELANCE
//	menubitmap_s	mappics[MAX_MAPSPERPAGE];
//	menubitmap_s	mapbuttons[MAX_MAPSPERPAGE];

/*
	menubitmap_s	back;
	menubitmap_s	next;
*/
	menutext_s		back;
	menutext_s		next;

	menubitmap_s	mappic;
	menutext_s		mapname;
	menulist_s		list;
	menulist_s		statlist;
// END

	menubitmap_s	item_null;

	qboolean		multiplayer;
	int				currentmap;
	int				nummaps;
// STONE LANCE
//	int				page;
//	int				maxpages;
	int				top;
// END
	char			maplist[MAX_SERVERMAPS][MAX_NAMELENGTH];
	char			mapinfo[MAX_SERVERMAPS][BIG_INFO_STRING];
// STONELANCE
	char			maplistname[MAX_SERVERMAPS][MAX_NAMELENGTH];
	char			mapstats[MAX_MAPSTATS][128];
	char*			items[MAX_SERVERMAPS];
	char*			statitems[MAX_SERVERMAPS];
	int				numstats;
// END
	int				mapGamebits[MAX_SERVERMAPS];
} startserver_t;

static startserver_t s_startserver;

static const char *gametype_items[] = {

	"Racing",
	"Racing Deathmatch",
	"Demolition Derby",
	"Last Car Standing",
	"Deathmatch",
	"Team Deathmatch",
	"Team Racing",
	"Team Racing Deathmatch",
	"Capture the Flag",
    "Domination",
	0
};

// STONELANCE
// gametype_items[gametype_remap2[s_serveroptions.gametype]]
static int gametype_remap[] = {GT_RACING, GT_RACING_DM, GT_DERBY, GT_LCS, GT_DEATHMATCH, GT_TEAM, GT_TEAM_RACING, GT_TEAM_RACING_DM, GT_CTF, GT_DOMINATION};
static int gametype_remap2[] = {0, 1, 0, 2, 3, 4, 5, 6, 7, 8, 9};

int		allowLength[3];
int		reversable;
// END


static void UI_ServerOptionsMenu( qboolean multiplayer );

// STONELANCE
char *UI_GetStatKey(int num){
	switch(num){
	case MS_NUMSTARTS:
		return "starts";

//	case MS_LAPS:
//		return "laps";

	case MS_LAPTIME:
		return "laptime";

	case MS_NUMCHECKPOINTS:
		return "checkpoints";

	case MS_NUMOBSERVERSPOTS:
		return "observerspots";

	case MS_NUMWEAPONS:
		return "weapons";

	case MS_NUMPOWERUPS:
		return "powerups";

	case MS_REVERSABLE:
		return "reversable";

	case MS_TRACKLENGTHS:
		return "tracklengths";
    
    case MS_BOTSUPPORT:
        return "botsupport";
        
    case MS_NUMTEAMS:
        return "teams";

	default:
		return "";
	}
}

char *UI_GetStatName(int num){
	switch(num){
	case MS_NUMSTARTS:
		return "Start positions:";

//	case MS_LAPS:
//		return "Laps:";

	case MS_LAPTIME:
		return "Laptime:";

	case MS_NUMCHECKPOINTS:
		return "Checkpoints:";

	case MS_NUMOBSERVERSPOTS:
		return "Observer spots:";

	case MS_NUMWEAPONS:
		return "Weapons:";

	case MS_NUMPOWERUPS:
		return "Powerups:";

	case MS_REVERSABLE:
		return "Reversable:";

	case MS_TRACKLENGTHS:
		return "Track lengths available:";
    
    case MS_BOTSUPPORT:
        return "Bot Support:";
        
    case MS_NUMTEAMS:
        return "Teams:";

	default:
		return "";
	}
}


char *UI_GetDefaultStatValue( int num ){
	switch(num){
	case MS_NUMSTARTS:
		return "Unknown";

//	case MS_LAPS:
//		return "Unknown";

	case MS_LAPTIME:
		return "Unknown";

	case MS_NUMCHECKPOINTS:
		return "Unknown";

	case MS_NUMOBSERVERSPOTS:
		return "Unknown";

	case MS_NUMWEAPONS:
		return "Unknown";

	case MS_NUMPOWERUPS:
		return "Unknown";

	case MS_REVERSABLE:
		return "0";

	case MS_TRACKLENGTHS:
		return "0 1 2";
        
    case MS_BOTSUPPORT:
        return "Unknown";
        
    case MS_NUMTEAMS:
        return "Unknown";

	default:
		return "";
	}
}


char *UI_GetStatValue( const char *info, int num ){
	char	*s;
	char	*result;

	s = UI_GetStatKey( num );

	result = Info_ValueForKey( info, s );

	if ( !result || !strcmp(result, "")){
		result = UI_GetDefaultStatValue( num );
	}

	// process numbers into valid strings
	switch ( num ){
	case MS_REVERSABLE:
		if ( !strcmp(result, "0") ){
			reversable = 0;
			result = "No";
		}
		else if ( !strcmp(result, "1") ){
			reversable = 1;
			result = "Yes";
		}
		break;

	case MS_TRACKLENGTHS:
		allowLength[0] = allowLength[1] = allowLength[2] = 1;

		if ( !strchr(result, '0') )
			allowLength[0] = 0;
		if ( !strchr(result, '1') )
			allowLength[1] = 0;
		if ( !strchr(result, '2') )
			allowLength[2] = 0;

		if ( allowLength[0] && allowLength[1] && allowLength[2] )
			result = "All";
		else if ( !allowLength[0] && allowLength[1] && allowLength[2] )
			result = "Medium, Long";
		else if ( !allowLength[0] && !allowLength[1] && allowLength[2] )
			result = "Long";
		else if ( allowLength[0] && !allowLength[1] && allowLength[2] )
			result = "Short, Long";
		else if ( allowLength[0] && !allowLength[1] && !allowLength[2] )
			result = "Short";
		else if ( !allowLength[0] && allowLength[1] && !allowLength[2] )
			result = "Medium";
		else if ( allowLength[0] && allowLength[1] && !allowLength[2] )
			result = "Short, Medium";
		else
			result = "All";

		break;
	}

	return result;
}

static void UI_SetupMapStatsForArena( int arena ){
	int				i;
	const char		*info;
	char			*s;

	if (arena < 0 || arena >= s_startserver.nummaps){
		for (i = 0; i < MAX_MAPSTATS; i++){
			Q_strncpyz(s_startserver.mapstats[i], "", 128);
		}
		return;
	}

	//info = UI_GetArenaInfoByNumber( arena );
	info = s_startserver.mapinfo[arena];

	s_startserver.numstats = 0;
	for (i = 0; i < MAX_MAPSTATS; i++){
		s = UI_GetStatValue(info, i);

		if (!s || !strcmp(s, ""))
			continue;

		Com_sprintf(s_startserver.mapstats[s_startserver.numstats], 128, "%30s %s", UI_GetStatName(i), s);
		s_startserver.numstats++;
	}

	for (i = s_startserver.numstats; i < MAX_MAPSTATS; i++){
		Q_strncpyz(s_startserver.mapstats[i], "", 128);
	}

	s_startserver.statlist.numitems = s_startserver.numstats;
	for( i = 0; i < s_startserver.numstats; i++ ) {
		s_startserver.statitems[i] = s_startserver.mapstats[i];
	}
}
// END

/*
=================
GametypeBits
=================
*/
static int GametypeBits( char *string ) {
	int		bits;
	char	*p;
	char	*token;

	bits = 0;
	p = string;
	while( 1 ) {
		token = COM_ParseExt( &p, qfalse );
		if( token[0] == 0 ) {
			break;
		}
// STONELANCE
		if( Q_stricmp( token, "q3r_racing" ) == 0 ) {
			bits |= 1 << GT_RACING;
			continue;
		}

		if( Q_stricmp( token, "q3r_racing_dm" ) == 0 ) {
			bits |= 1 << GT_RACING_DM;
			continue;
		}

		if( Q_stricmp( token, "q3r_derby" ) == 0 ) {
			bits |= 1 << GT_DERBY;
			continue;
		}
		
		if( Q_stricmp( token, "q3r_lcs" ) == 0 ) {
			bits |= 1 << GT_LCS;
			continue;
		}

		if( Q_stricmp( token, "q3r_dm" ) == 0 ) {
			bits |= 1 << GT_DEATHMATCH;
			continue;
		}

		if( Q_stricmp( token, "q3r_single" ) == 0 ) {
			bits |= 1 << GT_SINGLE_PLAYER;
			continue;
		}

		if( Q_stricmp( token, "q3r_team_racing" ) == 0 ) {
			bits |= 1 << GT_TEAM_RACING;
			continue;
		}

		if( Q_stricmp( token, "q3r_team_racing_dm" ) == 0 ) {
			bits |= 1 << GT_TEAM_RACING_DM;
			continue;
		}
// END

		if( Q_stricmp( token, "q3r_team_dm" ) == 0 ) {
			bits |= 1 << GT_TEAM;
			continue;
		}

		if( Q_stricmp( token, "q3r_ctf" ) == 0 ) {
			bits |= 1 << GT_CTF;
			continue;
		}

    if( Q_stricmp( token, "q3r_dom" ) == 0 ) {
			bits |= 1 << GT_DOMINATION;
			continue;
		}
   	}
	return bits;
}


/*
=================
StartServer_Update
=================
*/
static void StartServer_Update( void ) {
// STONELANCE
//	int				i;
//	int				top;
// END
	static	char	picname[64];

// STONELANCE
	/*
	top = s_startserver.page*MAX_MAPSPERPAGE;

	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		if (top+i >= s_startserver.nummaps)
			break;

		Com_sprintf( picname[i], sizeof(picname[i]), "levelshots/%s", s_startserver.maplist[top+i] );

		s_startserver.mappics[i].generic.flags &= ~QMF_HIGHLIGHT;
		s_startserver.mappics[i].generic.name   = picname[i];
		s_startserver.mappics[i].shader         = 0;

		// reset
		s_startserver.mapbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
		s_startserver.mapbuttons[i].generic.flags &= ~QMF_INACTIVE;
	}

	for (; i<MAX_MAPSPERPAGE; i++)
	{
		s_startserver.mappics[i].generic.flags &= ~QMF_HIGHLIGHT;
		s_startserver.mappics[i].generic.name   = NULL;
		s_startserver.mappics[i].shader         = 0;

		// disable
		s_startserver.mapbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		s_startserver.mapbuttons[i].generic.flags |= QMF_INACTIVE;
	}
*/
// END

	Com_sprintf( picname, sizeof(picname), "levelshots/%s", s_startserver.maplist[s_startserver.list.curvalue] );
	s_startserver.mappic.generic.name   = picname;
	s_startserver.mappic.shader         = 0;

	// no servers to start
	if( !s_startserver.nummaps ) {
// STONELANCE
//		s_startserver.next.generic.flags |= QMF_INACTIVE;
// END

		// set the map name
		strcpy( s_startserver.mapname.string, "NO MAPS FOUND" );

// STONELANCE
		UI_SetupMapStatsForArena(-1);
// END
	}
	else {
		// set the highlight
// STONELANCE
//		s_startserver.next.generic.flags &= ~QMF_INACTIVE;

/*
		i = s_startserver.currentmap - top;
		if ( i >=0 && i < MAX_MAPSPERPAGE ) 
		{
			s_startserver.mappics[i].generic.flags    |= QMF_HIGHLIGHT;
			s_startserver.mapbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		}
*/
		// set the map name
		strcpy( s_startserver.mapname.string, s_startserver.maplist[s_startserver.currentmap] );

		UI_SetupMapStatsForArena(s_startserver.currentmap);
// END
	}

	Q_strupr( s_startserver.mapname.string );
}


/*
=================
StartServer_MapEvent
=================
*/
static void StartServer_MapEvent( void* ptr, int event ) {

	int id = ((menucommon_s*)ptr)->id;

	if( event != QM_ACTIVATED && id != ID_LIST ) {
		return;
	}

	switch( id ) {
	case ID_LIST:
		if( event == QM_GOTFOCUS ) {
			s_startserver.currentmap = s_startserver.list.curvalue;
			StartServer_Update();
		}
		break;
	}

// STONELANCE
//	s_startserver.currentmap = (s_startserver.page*MAX_MAPSPERPAGE) + (((menucommon_s*)ptr)->id - ID_PICTURES);
// END
	StartServer_Update();
}


/*
=================
StartServer_GametypeEvent
=================
*/
static void StartServer_GametypeEvent( void* ptr, int event ) {
	int			i;
	int			count;
	int			gamebits;
	int			matchbits;
	const char	*info;

	if( event != QM_ACTIVATED) {
		return;
	}

	count = UI_GetNumArenas();
	s_startserver.nummaps = 0;
	matchbits = 1 << gametype_remap[s_startserver.gametype.curvalue];
// STONELANCE
/*
	if( gametype_remap[s_startserver.gametype.curvalue] == GT_FFA ) {
		matchbits |= ( 1 << GT_SINGLE_PLAYER );
	}
*/
// END
	for( i = 0; i < count; i++ ) {
		info = UI_GetArenaInfoByNumber( i );

		gamebits = GametypeBits( Info_ValueForKey( info, "type") );
		if( !( gamebits & matchbits ) ) {
			continue;
		}

// STONELANCE
		Q_strncpyz(s_startserver.mapinfo[s_startserver.nummaps], info, sizeof(s_startserver.mapinfo[i]));
// END
		Q_strncpyz( s_startserver.maplist[s_startserver.nummaps], Info_ValueForKey( info, "map"), MAX_NAMELENGTH );
		// ZTM: Disabled so linux without pk3s works
		//Q_strupr( s_startserver.maplist[s_startserver.nummaps] );
// STONELANCE
		Q_strncpyz( s_startserver.maplistname[s_startserver.nummaps], Info_ValueForKey( info, "longname"), MAX_NAMELENGTH );
		if (s_startserver.maplistname[s_startserver.nummaps][0] == 0)
			Q_strncpyz( s_startserver.maplistname[s_startserver.nummaps], s_startserver.maplist[s_startserver.nummaps], MAX_NAMELENGTH );
		else
			Q_strupr( s_startserver.maplistname[s_startserver.nummaps] );
// END
		s_startserver.mapGamebits[s_startserver.nummaps] = gamebits;
		s_startserver.nummaps++;
	}
// STONELANCE
//	s_startserver.maxpages = (s_startserver.nummaps + MAX_MAPSPERPAGE-1)/MAX_MAPSPERPAGE;
//	s_startserver.page = 0;
	s_startserver.currentmap = 0;
	s_startserver.top = 0;
	s_startserver.list.top = s_startserver.top;
	s_startserver.list.curvalue = s_startserver.currentmap;
	s_startserver.list.numitems = s_startserver.nummaps;
// END

	StartServer_Update();
}


/*
=================
StartServer_MenuEvent
=================
*/
static void StartServer_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
// STONELANCE
/*
	case ID_PREVPAGE:
		if( s_startserver.page > 0 ) {
			s_startserver.page--;
			StartServer_Update();
		}
		break;

	case ID_NEXTPAGE:
		if( s_startserver.page < s_startserver.maxpages - 1 ) {
			s_startserver.page++;
			StartServer_Update();
		}
		break;
*/
// END
	case ID_STARTSERVERNEXT:
		trap_Cvar_SetValue( "g_gameType", gametype_remap[s_startserver.gametype.curvalue] );
		UI_ServerOptionsMenu( s_startserver.multiplayer );
		break;

	case ID_STARTSERVERBACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
StartServer_LevelshotDraw
===============
*/
static void StartServer_LevelshotDraw( void *self ) {
	menubitmap_s	*b;

// STONELANCE
	char			*s;
	char			*info;
	char			author[MAX_QPATH];
// END
	int				x;
	int				y;
	int				w;
	int				h;
	
// STONELANCE
//	int				n;
// END

	b = (menubitmap_s *)self;

	if( !b->generic.name ) {
		return;
	}

	if( b->generic.name && !b->shader ) {
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if( !b->shader && b->errorpic ) {
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
		}
	}

// STONELANCE
	w = 256;
	x = b->generic.x - w/2;
	y = b->generic.y;
	UI_FillRect( x, y, w, 140, menu_back_color );

	if (s_startserver.list.curvalue < 0 || s_startserver.list.curvalue >= s_startserver.nummaps)
		return;

	x = b->generic.x - b->width / 2;
// END
	
	x = b->generic.x - b->width / 2;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( b->shader ) {
		UI_DrawHandlePic( x, y, w, h, b->shader );
	}

	x += b->width / 2;
	
// STONELANCE
	y += 96 + 4;
//	n = s_startserver.page * MAX_MAPSPERPAGE + b->generic.id - ID_PICTURES;
	//UI_DrawString( x, y, s_startserver.maplist[n], UI_CENTER|UI_SMALLFONT, color_orange );
	UI_DrawString( x, y, s_startserver.maplistname[s_startserver.list.curvalue], UI_CENTER|UI_SMALLFONT, text_color_normal );

	info = s_startserver.mapinfo[s_startserver.currentmap];
	s = Info_ValueForKey( info, "author");
	if (!s || !strcmp(s, ""))
		Com_sprintf(author, sizeof(author), "Author: Unknown");
	else
		Com_sprintf(author, sizeof(author), "Author: %s", s);

	y += SMALLCHAR_HEIGHT;
	UI_DrawString( x, y, author, UI_CENTER|UI_SMALLFONT, text_color_normal );
// END

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height + 28;
	if( b->generic.flags & QMF_HIGHLIGHT ) {	
		UI_DrawHandlePic( x, y, w, h, b->focusshader );
		
	}
}

/*
=================
StartServer_MenuInit
=================
*/
static void StartServer_MenuInit( void ) {
	int	i;
// STONELANCE
//	int	x;
//	int	y;
// END
	static char mapnamebuffer[64];

	// zero set all our globals
	memset( &s_startserver, 0 ,sizeof(startserver_t) );

	StartServer_Cache();

	s_startserver.menu.wrapAround = qtrue;
	s_startserver.menu.fullscreen = qtrue;

	s_startserver.banner.generic.type  = MTYPE_BTEXT;
	s_startserver.banner.generic.x	   = 320;
	s_startserver.banner.generic.y	   = 16;
	s_startserver.banner.string        = "MAP SELECT";
	s_startserver.banner.color         = text_color_normal;
	s_startserver.banner.style         = UI_CENTER;

	s_startserver.gametype.generic.type		= MTYPE_SPINCONTROL;
	s_startserver.gametype.generic.name		= "Game Type:";
	s_startserver.gametype.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_startserver.gametype.generic.callback	= StartServer_GametypeEvent;
	s_startserver.gametype.generic.id		= ID_GAMETYPE;
	s_startserver.gametype.generic.x		= 320 - 24;
// STONELANCE
//	s_startserver.gametype.generic.y		= 368;
	s_startserver.gametype.generic.y		= 480 - 50;
// END
	s_startserver.gametype.itemnames		= gametype_items;

// STONELANCE
/*
	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		x =	(i % MAX_MAPCOLS) * (128+8) + 188;
		y = (i / MAX_MAPROWS) * (128+8) + 96;

		s_startserver.mappics[i].generic.type   = MTYPE_BITMAP;
		s_startserver.mappics[i].generic.flags  = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
		s_startserver.mappics[i].generic.x	    = x;
		s_startserver.mappics[i].generic.y	    = y;
		s_startserver.mappics[i].generic.id		= ID_PICTURES+i;
		s_startserver.mappics[i].width  		= 128;
		s_startserver.mappics[i].height  	    = 96;
		s_startserver.mappics[i].focuspic       = GAMESERVER_SELECTED;
		s_startserver.mappics[i].errorpic       = GAMESERVER_UNKNOWNMAP;
		s_startserver.mappics[i].generic.ownerdraw = StartServer_LevelshotDraw;

		s_startserver.mapbuttons[i].generic.type     = MTYPE_BITMAP;
		s_startserver.mapbuttons[i].generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_NODEFAULTINIT;
		s_startserver.mapbuttons[i].generic.id       = ID_PICTURES+i;
		s_startserver.mapbuttons[i].generic.callback = StartServer_MapEvent;
		s_startserver.mapbuttons[i].generic.x	     = x - 30;
		s_startserver.mapbuttons[i].generic.y	     = y - 32;
		s_startserver.mapbuttons[i].width  		     = 256;
		s_startserver.mapbuttons[i].height  	     = 248;
		s_startserver.mapbuttons[i].generic.left     = x;
		s_startserver.mapbuttons[i].generic.top  	 = y;
		s_startserver.mapbuttons[i].generic.right    = x + 128;
		s_startserver.mapbuttons[i].generic.bottom   = y + 128;
		s_startserver.mapbuttons[i].focuspic         = GAMESERVER_SELECT;
	}

	s_startserver.arrows.generic.type  = MTYPE_BITMAP;
	s_startserver.arrows.generic.name  = GAMESERVER_ARROWS;
	s_startserver.arrows.generic.flags = QMF_INACTIVE;
	s_startserver.arrows.generic.x	   = 260;
	s_startserver.arrows.generic.y	   = 400;
	s_startserver.arrows.width  	   = 128;
	s_startserver.arrows.height  	   = 32;

	s_startserver.prevpage.generic.type	    = MTYPE_BITMAP;
	s_startserver.prevpage.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.prevpage.generic.callback = StartServer_MenuEvent;
	s_startserver.prevpage.generic.id	    = ID_PREVPAGE;
	s_startserver.prevpage.generic.x		= 260;
	s_startserver.prevpage.generic.y		= 400;
	s_startserver.prevpage.width  		    = 64;
	s_startserver.prevpage.height  		    = 32;
	s_startserver.prevpage.focuspic         = GAMESERVER_ARROWSL;

	s_startserver.nextpage.generic.type	    = MTYPE_BITMAP;
	s_startserver.nextpage.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.nextpage.generic.callback = StartServer_MenuEvent;
	s_startserver.nextpage.generic.id	    = ID_NEXTPAGE;
	s_startserver.nextpage.generic.x		= 321;
	s_startserver.nextpage.generic.y		= 400;
	s_startserver.nextpage.width  		    = 64;
	s_startserver.nextpage.height  		    = 32;
	s_startserver.nextpage.focuspic         = GAMESERVER_ARROWSR;
*/

	s_startserver.list.generic.type			= MTYPE_LISTBOX;
	s_startserver.list.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_startserver.list.generic.id			= ID_LIST;
	s_startserver.list.scrollbarAlignment	= SB_RIGHT;
	s_startserver.list.generic.callback		= StartServer_MapEvent;
	s_startserver.list.generic.x			= 30;
	s_startserver.list.generic.y			= 80;
	s_startserver.list.width				= 40;
	s_startserver.list.height				= MAX_MAPSPERPAGE;
	s_startserver.list.itemnames			= (const char **)s_startserver.items;
	s_startserver.list.numitems				= s_startserver.nummaps;
	for( i = 0; i < s_startserver.nummaps; i++ ) {
		s_startserver.items[i] = s_startserver.maplistname[i];
	}

	s_startserver.statlist.generic.type			= MTYPE_LISTBOX;
	s_startserver.statlist.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS | QMF_SCROLL_ONLY;
	s_startserver.statlist.scrollbarAlignment	= SB_RIGHT | SB_HIDE;
	s_startserver.statlist.generic.x			= 290;
	s_startserver.statlist.generic.y			= 272;
	s_startserver.statlist.width				= 52;
	s_startserver.statlist.height				= 8;
	s_startserver.statlist.itemnames			= (const char **)s_startserver.statitems;
	s_startserver.statlist.numitems				= s_startserver.numstats;
	for( i = 0; i < s_startserver.numstats; i++ ) {
		s_startserver.statitems[i] = s_startserver.mapstats[i];
	}

	s_startserver.mappic.generic.type		= MTYPE_BITMAP;
	s_startserver.mappic.generic.flags		= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_startserver.mappic.generic.x			= 445;
	s_startserver.mappic.generic.y			= 80;
	s_startserver.mappic.generic.id			= ID_PICTURE;
	s_startserver.mappic.width  			= 170;
	s_startserver.mappic.height  			= 96;
	s_startserver.mappic.errorpic			= GAMESERVER_UNKNOWNMAP;
	s_startserver.mappic.generic.ownerdraw	= StartServer_LevelshotDraw;
// END

	s_startserver.mapname.generic.type  = MTYPE_PTEXT;
	s_startserver.mapname.generic.flags = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
// STONELANCE
//	s_startserver.mapname.generic.x	    = 320;
//	s_startserver.mapname.generic.y	    = 440;
	s_startserver.mapname.generic.x	    = 455;
	s_startserver.mapname.generic.y	    = 228;
// END
	s_startserver.mapname.string        = mapnamebuffer;
	s_startserver.mapname.style         = UI_CENTER|UI_BIGFONT;
	s_startserver.mapname.color         = text_color_normal;

// STONELANCE
/*
	s_startserver.back.generic.type	    = MTYPE_BITMAP;
	s_startserver.back.generic.name     = GAMESERVER_BACK0;
	s_startserver.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.back.generic.callback = StartServer_MenuEvent;
	s_startserver.back.generic.id	    = ID_STARTSERVERBACK;
	s_startserver.back.generic.x		= 0;
	s_startserver.back.generic.y		= 480-64;
	s_startserver.back.width  		    = 128;
	s_startserver.back.height  		    = 64;
	s_startserver.back.focuspic         = GAMESERVER_BACK1;

	s_startserver.next.generic.type	    = MTYPE_BITMAP;
	s_startserver.next.generic.name     = GAMESERVER_NEXT0;
	s_startserver.next.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.next.generic.callback = StartServer_MenuEvent;
	s_startserver.next.generic.id	    = ID_STARTSERVERNEXT;
	s_startserver.next.generic.x		= 640;
	s_startserver.next.generic.y		= 480-64;
	s_startserver.next.width  		    = 128;
	s_startserver.next.height  		    = 64;
	s_startserver.next.focuspic         = GAMESERVER_NEXT1;
*/
	s_startserver.back.generic.type			= MTYPE_PTEXT;
	s_startserver.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.back.generic.x			= 20;
	s_startserver.back.generic.y			= 480 - 50;
	s_startserver.back.generic.id			= ID_STARTSERVERBACK;
	s_startserver.back.generic.callback		= StartServer_MenuEvent; 
	s_startserver.back.string				= "< BACK";
	s_startserver.back.color				= text_color_normal;
	s_startserver.back.style				= UI_LEFT | UI_SMALLFONT;

	s_startserver.next.generic.type			= MTYPE_PTEXT;
	s_startserver.next.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.next.generic.x			= 640 - 20;
	s_startserver.next.generic.y			= 480 - 50;
	s_startserver.next.generic.id			= ID_STARTSERVERNEXT;
	s_startserver.next.generic.callback		= StartServer_MenuEvent; 
	s_startserver.next.string				= "NEXT >";
	s_startserver.next.color				= text_color_normal;
	s_startserver.next.style				= UI_RIGHT | UI_SMALLFONT;
// END

	s_startserver.item_null.generic.type	= MTYPE_BITMAP;
	s_startserver.item_null.generic.flags	= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	s_startserver.item_null.generic.x		= 0;
	s_startserver.item_null.generic.y		= 0;
	s_startserver.item_null.width			= 640;
	s_startserver.item_null.height			= 480;

	Menu_AddItem( &s_startserver.menu, &s_startserver.banner );

	Menu_AddItem( &s_startserver.menu, &s_startserver.gametype );
/*
	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		Menu_AddItem( &s_startserver.menu, &s_startserver.mappics[i] );
		Menu_AddItem( &s_startserver.menu, &s_startserver.mapbuttons[i] );
	}

	Menu_AddItem( &s_startserver.menu, &s_startserver.arrows );
	Menu_AddItem( &s_startserver.menu, &s_startserver.prevpage );
	Menu_AddItem( &s_startserver.menu, &s_startserver.nextpage );
*/
	Menu_AddItem( &s_startserver.menu, &s_startserver.list );
	Menu_AddItem( &s_startserver.menu, &s_startserver.statlist );

		Menu_AddItem( &s_startserver.menu, &s_startserver.mappic );
// END

	Menu_AddItem( &s_startserver.menu, &s_startserver.back );
	Menu_AddItem( &s_startserver.menu, &s_startserver.next );
	Menu_AddItem( &s_startserver.menu, &s_startserver.mapname );
	Menu_AddItem( &s_startserver.menu, &s_startserver.item_null );

	StartServer_GametypeEvent( NULL, QM_ACTIVATED );
}


/*
=================
StartServer_Cache
=================
*/
void StartServer_Cache( void )
{
	int				i;
	const char		*info;
	qboolean		precache;
	char			picname[64];

// STONELANCE
/*
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK0 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK1 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_NEXT0 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_NEXT1 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_FRAMEL );	
	trap_R_RegisterShaderNoMip( GAMESERVER_FRAMER );
*/
// END
	trap_R_RegisterShaderNoMip( GAMESERVER_SELECT );	
	trap_R_RegisterShaderNoMip( GAMESERVER_SELECTED );	
	trap_R_RegisterShaderNoMip( GAMESERVER_UNKNOWNMAP );
// STONELANCE
/*
	trap_R_RegisterShaderNoMip( GAMESERVER_ARROWS );
	trap_R_RegisterShaderNoMip( GAMESERVER_ARROWSL );
	trap_R_RegisterShaderNoMip( GAMESERVER_ARROWSR );
*/
// END

	precache = trap_Cvar_VariableValue("com_buildscript");

	s_startserver.nummaps = UI_GetNumArenas();

	for( i = 0; i < s_startserver.nummaps; i++ ) {
		info = UI_GetArenaInfoByNumber( i );

		Q_strncpyz( s_startserver.maplist[i], Info_ValueForKey( info, "map"), MAX_NAMELENGTH );
		// ZTM: Disabled so linux without pk3s works
		//Q_strupr( s_startserver.maplist[i] );

// STONELANCE
		Q_strncpyz( s_startserver.maplistname[i], Info_ValueForKey( info, "longname"), MAX_NAMELENGTH );
		if (s_startserver.maplistname[i][0] == 0)
			Q_strncpyz( s_startserver.maplistname[i], s_startserver.maplist[i], MAX_NAMELENGTH );
		else
			Q_strupr( s_startserver.maplistname[i] );
// END

		s_startserver.mapGamebits[i] = GametypeBits( Info_ValueForKey( info, "type") );

		if( precache ) {
			Com_sprintf( picname, sizeof(picname), "levelshots/%s", s_startserver.maplist[i] );
			trap_R_RegisterShaderNoMip(picname);
			
		}
	}

// STONELANCE
//	s_startserver.maxpages = (s_startserver.nummaps + MAX_MAPSPERPAGE-1)/MAX_MAPSPERPAGE;
// END
}


/*
=================
UI_StartServerMenu
=================
*/
void UI_StartServerMenu( qboolean multiplayer ) {
	StartServer_MenuInit();
	s_startserver.multiplayer = multiplayer;
	UI_PushMenu( &s_startserver.menu );
}



/*
=============================================================================

SERVER OPTIONS MENU *****

=============================================================================
*/

#define ID_PLAYER_TYPE			10
#define ID_MAXCLIENTS			11
#define ID_DEDICATED			12
#define ID_GO					13
#define ID_BACK					14
// STONELANCE
#define ID_TRACK_LENGTH			15
#define ID_TRACK_REVERSED		16
// END
#define PLAYER_SLOTS			12


typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menubitmap_s		mappic;
	

// STONELANCE
//	menubitmap_s		picframe;
// END

	menulist_s			dedicated;
	menufield_s			timelimit;
	menufield_s			fraglimit;
	menufield_s			flaglimit;
	menuradiobutton_s	friendlyfire;
	menufield_s			hostname;
    menulist_s          dominationSpawnStyle;
	menuradiobutton_s   sigillocator;
// STONLANCE
	menulist_s			trackLength;
	menulist_s			reversed;

	menuradiobutton_s	pure;
	menulist_s			botSkill;
// END

	menutext_s			player0;
	menulist_s			playerType[PLAYER_SLOTS];
	menutext_s			playerName[PLAYER_SLOTS];
	menulist_s			playerTeam[PLAYER_SLOTS];

// STONELANCE
/*
	menubitmap_s		go;
	menubitmap_s		back;
*/
	menutext_s			go;
	menutext_s			back;
// END

	qboolean			multiplayer;
	int					gametype;
	char				mapnamebuffer[32];
	char				playerNameBuffers[PLAYER_SLOTS][16];

	qboolean			newBot;
	int					newBotIndex;
	char				newBotName[16];
} serveroptions_t;

static serveroptions_t s_serveroptions;

static const char *dedicated_list[] = {
	"No",
	"LAN",
	"Internet",
	0
};

// STONELANCE
static char *track_length_list[] = {
	"Short",
	"Medium",
	"Long",
	0
};

/*
static const char *track_length_list[] = {
	"Short",
	"Medium",
	"Long",
	0
};
*/

static const char *reversed_list[] = {
	"No",
	"Yes",
	0
};
// END

static const char *playerType_list[] = {
	"Open",
	"Bot",
	"----",
	0
};

static const char *playerTeam_list[] = {
	"Blue",
	"Red",
	"Green",
	"Yellow",
	0
};

static const char *botSkill_list[] = {
	"I Can Win",
	"Bring It On",
	"Hurt Me Plenty",
	"Hardcore",
	"Nightmare!",
	0
};

// for dominationSpawnStyle
static const char *dtfspawn_list[] = {
  "DM Spawns",
  "CTF Team Spawns",
  0
};

/*
=================
BotAlreadySelected
=================
*/
static qboolean BotAlreadySelected( const char *checkName ) {
	int		n;

	for( n = 1; n < PLAYER_SLOTS; n++ ) {
		if( s_serveroptions.playerType[n].curvalue != 1 ) {
			continue;
		}
		if( (s_serveroptions.gametype >= GT_TEAM) &&
			(s_serveroptions.playerTeam[n].curvalue != s_serveroptions.playerTeam[s_serveroptions.newBotIndex].curvalue ) ) {
			continue;
		}
		if( Q_stricmp( checkName, s_serveroptions.playerNameBuffers[n] ) == 0 ) {
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
ServerOptions_Start
=================
*/
static void ServerOptions_Start( void ) {
	int		timelimit;
	int		fraglimit;
    int     dominationSpawnStyle;
	int     sigillocator;
	int		maxclients;
	int		dedicated;
	int		friendlyfire;
	int		flaglimit;
	int		pure;
// STONELANCE
	int		trackLength;
	int		reversed;
// END
	int		skill;
	int		n;
	char	buf[64];

	timelimit	 = atoi( s_serveroptions.timelimit.field.buffer );
	fraglimit	 = atoi( s_serveroptions.fraglimit.field.buffer );
	flaglimit	 = atoi( s_serveroptions.flaglimit.field.buffer );
	dedicated	 = s_serveroptions.dedicated.curvalue;
	friendlyfire = s_serveroptions.friendlyfire.curvalue;
	pure		 = s_serveroptions.pure.curvalue;
// STONELANCE
	skill		 = s_serveroptions.botSkill.curvalue + 1;
    dominationSpawnStyle = s_serveroptions.dominationSpawnStyle.curvalue; // dtf
    sigillocator = s_serveroptions.sigillocator.curvalue; // dtf
	trackLength  = s_serveroptions.trackLength.curvalue;
	reversed     = s_serveroptions.reversed.curvalue;
// END

	//set maxclients
	for( n = 0, maxclients = 0; n < PLAYER_SLOTS; n++ ) {
		if( s_serveroptions.playerType[n].curvalue == 2 ) {
			continue;
		}
		if( (s_serveroptions.playerType[n].curvalue == 1) && (s_serveroptions.playerNameBuffers[n][0] == 0) ) {
			continue;
		}
		maxclients++;
	}

	switch( s_serveroptions.gametype ) {
// STONELANCE
	case GT_RACING:
	case GT_RACING_DM:
	default:
		trap_Cvar_SetValue( "ui_racing_laplimit", fraglimit );
		trap_Cvar_SetValue( "ui_racing_timelimit", timelimit );
		break;

	case GT_TEAM_RACING:
	case GT_TEAM_RACING_DM:
		trap_Cvar_SetValue( "ui_team_racing_laplimit", fraglimit );
		trap_Cvar_SetValue( "ui_team_racing_timelimit", timelimit );
		trap_Cvar_SetValue( "ui_team_racing_friendly", friendlyfire );
		break;

	case GT_DERBY:
		trap_Cvar_SetValue( "ui_derby_timelimit", timelimit );
		break;
		
	case GT_LCS:
		trap_Cvar_SetValue( "ui_lcs_timelimit", timelimit );
		break;

	case GT_DEATHMATCH:
		trap_Cvar_SetValue( "ui_dm_fraglimit", fraglimit );
		trap_Cvar_SetValue( "ui_dm_timelimit", timelimit );
		break;
// END
	case GT_TEAM:
		trap_Cvar_SetValue( "ui_team_fraglimit", fraglimit );
		trap_Cvar_SetValue( "ui_team_timelimit", timelimit );
		trap_Cvar_SetValue( "ui_team_friendly", friendlyfire );
		break;

	case GT_CTF:
		trap_Cvar_SetValue( "ui_ctf_capturelimit", flaglimit );
		trap_Cvar_SetValue( "ui_ctf_timelimit", timelimit );
		trap_Cvar_SetValue( "ui_ctf_friendly", friendlyfire );
		break;
		
// Q3Rally Code Start
    case GT_DOMINATION:
		trap_Cvar_SetValue( "g_dominationSpawnStyle", Com_Clamp( 0, 1, dominationSpawnStyle ) );
		trap_Cvar_SetValue( "cg_sigilLocator", Com_Clamp( 0, 1, sigillocator) );
		trap_Cvar_SetValue( "ui_dom_capturelimit", flaglimit );
		trap_Cvar_SetValue( "ui_dom_timelimit", timelimit );
		trap_Cvar_SetValue( "ui_dom_friendly", friendlyfire );
		break;
// Q3Rally Code END
	}

	trap_Cvar_SetValue( "sv_maxclients", Com_Clamp( 0, 12, maxclients ) );
	trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, dedicated ) );
	trap_Cvar_SetValue ("timelimit", Com_Clamp( 0, timelimit, timelimit ) );
	trap_Cvar_SetValue ("fraglimit", Com_Clamp( 0, fraglimit, fraglimit ) );
// STONELANCE
	trap_Cvar_SetValue ("laplimit", Com_Clamp( 0, fraglimit, fraglimit ) );
// END
	trap_Cvar_SetValue ("capturelimit", Com_Clamp( 0, flaglimit, flaglimit ) );
	trap_Cvar_SetValue( "g_friendlyfire", friendlyfire );
	trap_Cvar_SetValue( "sv_pure", pure );
// STONELANCE
	trap_Cvar_SetValue( "g_trackLength", Com_Clamp( 0, trackLength, 2 ) );
	trap_Cvar_SetValue( "g_trackReversed", Com_Clamp( 0, reversed, 1 ) );

	trap_Cvar_SetValue( "ui_racing_tracklength", Com_Clamp( 0, trackLength, 2 ) );
	trap_Cvar_SetValue( "ui_racing_trackreversed", Com_Clamp( 0, reversed, 1 ) );
// END
	trap_Cvar_Set("sv_hostname", s_serveroptions.hostname.field.buffer );

	// the wait commands will allow the dedicated to take effect
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", s_startserver.maplist[s_startserver.currentmap] ) );

	// add bots
	trap_Cmd_ExecuteText( EXEC_APPEND, "wait 3\n" );
	for( n = 1; n < PLAYER_SLOTS; n++ ) {
		if( s_serveroptions.playerType[n].curvalue != 1 ) {
			continue;
		}
		if( s_serveroptions.playerNameBuffers[n][0] == 0 ) {
			continue;
		}
		if( s_serveroptions.playerNameBuffers[n][0] == '-' ) {
			continue;
		}
		if( s_serveroptions.gametype >= GT_TEAM ) {
			Com_sprintf( buf, sizeof(buf), "addbot %s %i %s\n", s_serveroptions.playerNameBuffers[n], skill,
				playerTeam_list[s_serveroptions.playerTeam[n].curvalue] );
		}
		else {
			Com_sprintf( buf, sizeof(buf), "addbot %s %i\n", s_serveroptions.playerNameBuffers[n], skill );
		}
		trap_Cmd_ExecuteText( EXEC_APPEND, buf );
	}

	// set player's team
	if( dedicated == 0 && s_serveroptions.gametype >= GT_TEAM ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait 5; team %s\n", playerTeam_list[s_serveroptions.playerTeam[0].curvalue] ) );
	}
}


/*
=================
ServerOptions_InitPlayerItems
=================
*/
static void ServerOptions_InitPlayerItems( void ) {
	int		n;
	int		v;

	// init types
	if( s_serveroptions.multiplayer ) {
		v = 0;	// open
	}
	else {
		v = 1;	// bot
	}
	
	for( n = 0; n < PLAYER_SLOTS; n++ ) {
		s_serveroptions.playerType[n].curvalue = v;
	}

	if( s_serveroptions.multiplayer && (s_serveroptions.gametype < GT_TEAM) ) {
		for( n = 8; n < PLAYER_SLOTS; n++ ) {
			s_serveroptions.playerType[n].curvalue = 2;
		}
	}

	// if not a dedicated server, first slot is reserved for the human on the server
	if( s_serveroptions.dedicated.curvalue == 0 ) {
		// human
		s_serveroptions.playerType[0].generic.flags |= QMF_INACTIVE;
		s_serveroptions.playerType[0].curvalue = 0;
		trap_Cvar_VariableStringBuffer( "name", s_serveroptions.playerNameBuffers[0], sizeof(s_serveroptions.playerNameBuffers[0]) );
		Q_CleanStr( s_serveroptions.playerNameBuffers[0] );
	}

	// init teams
	if( s_serveroptions.gametype >= GT_TEAM ) {
		for( n = 0; n < (PLAYER_SLOTS / 3); n++ ) {
			s_serveroptions.playerTeam[n].curvalue = 0;
		}
		for( ; n < PLAYER_SLOTS; n++ ) {
			s_serveroptions.playerTeam[n].curvalue = 1;
		}
	}
	else {
		for( n = 0; n < PLAYER_SLOTS; n++ ) {
			s_serveroptions.playerTeam[n].generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		}
	}
}


/*
=================
ServerOptions_SetPlayerItems
=================
*/
static void ServerOptions_SetPlayerItems( void ) {
	int		start;
	int		n;

	// types
//	for( n = 0; n < PLAYER_SLOTS; n++ ) {
//		if( (!s_serveroptions.multiplayer) && (n > 0) && (s_serveroptions.playerType[n].curvalue == 0) ) {
//			s_serveroptions.playerType[n].curvalue = 1;
//		}
//	}

	// names
	if( s_serveroptions.dedicated.curvalue == 0 ) {
		s_serveroptions.player0.string = "Human";
		s_serveroptions.playerName[0].generic.flags &= ~QMF_HIDDEN;

		start = 1;
	}
	else {
		s_serveroptions.player0.string = "Open";
		start = 0;
	}

	for( n = start; n < PLAYER_SLOTS; n++ ) {
		if( s_serveroptions.playerType[n].curvalue == 1 ) {
			s_serveroptions.playerName[n].generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
		}
		else {
			s_serveroptions.playerName[n].generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		}
	}

	// teams
	if( s_serveroptions.gametype < GT_TEAM ) {
		return;
	}
	for( n = start; n < PLAYER_SLOTS; n++ ) {
		if( s_serveroptions.playerType[n].curvalue == 2 ) {
			s_serveroptions.playerTeam[n].generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		}
		else {
			s_serveroptions.playerTeam[n].generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
		}
	}
}


/*
=================
ServerOptions_Event
=================
*/
static void ServerOptions_Event( void* ptr, int event ) {
	switch( ((menucommon_s*)ptr)->id ) {
	case ID_PLAYER_TYPE:
		if( event != QM_ACTIVATED ) {
			break;
		}
		ServerOptions_SetPlayerItems();
		break;

	case ID_MAXCLIENTS:
	case ID_DEDICATED:
		ServerOptions_SetPlayerItems();
		break;

	case ID_GO:
		if( event != QM_ACTIVATED ) {
			break;
		}
		ServerOptions_Start();
		break;

	case ID_BACK:
		if( event != QM_ACTIVATED ) {
			break;
		}
		UI_PopMenu();
		break;
	}
}


static void ServerOptions_PlayerNameEvent( void* ptr, int event ) {
	int		n;

	if( event != QM_ACTIVATED ) {
		return;
	}
	n = ((menutext_s*)ptr)->generic.id;
	s_serveroptions.newBotIndex = n;
	UI_BotSelectMenu( s_serveroptions.playerNameBuffers[n] );
}


/*
=================
ServerOptions_StatusBar
=================
*/
static void ServerOptions_StatusBar( void* ptr ) {
	UI_DrawString( 320, 440, "0 = NO LIMIT", UI_CENTER|UI_SMALLFONT, colorWhite );
}


// STONELANCE
/*
===============
ServerOptions_LevelshotDraw
===============
*/
static void ServerOptions_LevelshotDraw( void *self ) {
	menubitmap_s	*b;
	char			*s;
	char			*info;
	char			author[MAX_QPATH];
	int				x;
	int				y;
	int				w;
	int				h;
//	int				n;

	// strange place for this, but it works
	if( s_serveroptions.newBot ) {
		Q_strncpyz( s_serveroptions.playerNameBuffers[s_serveroptions.newBotIndex], s_serveroptions.newBotName, 16 );
		s_serveroptions.newBot = qfalse;
	}

	b = (menubitmap_s *)self;

	if( !b->generic.name ) {
		return;
	}

	if( b->generic.name && !b->shader ) {
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if( !b->shader && b->errorpic ) {
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
		}
	}

	if( b->focuspic && !b->focusshader ) {
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );
	}

	// STONELANCE
//	w = 256;
//	x = b->generic.x - 100;
//	y = b->generic.y;
	UI_FillRect( 325, 80, 256, 160, menu_back_color );

	if (s_startserver.list.curvalue < 0 || s_startserver.list.curvalue >= s_startserver.nummaps)
		return;

	x = b->generic.x - b->width / 2;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( b->shader ) {
		UI_DrawHandlePic( x, y, w, h, b->shader );
	}

	x += b->width / 2;
	y += 96 + 4;
//	n = s_startserver.page * MAX_MAPSPERPAGE + b->generic.id - ID_PICTURES;
	//UI_DrawString( x, y, s_startserver.maplist[n], UI_CENTER|UI_SMALLFONT, color_orange );
	UI_DrawString( x, y, s_startserver.maplistname[s_startserver.list.curvalue], UI_CENTER|UI_SMALLFONT, text_color_normal );

	info = s_startserver.mapinfo[s_startserver.currentmap];
	s = Info_ValueForKey( info, "author");
	if (!s || !strcmp(s, ""))
		Com_sprintf(author, sizeof(author), "Author: Unknown");
	else
		Com_sprintf(author, sizeof(author), "Author: %s", s);

	y += SMALLCHAR_HEIGHT;
	UI_DrawString( x, y, author, UI_CENTER|UI_SMALLFONT, text_color_normal );

	y += SMALLCHAR_HEIGHT + 2;
	UI_DrawString( x, y, gametype_items[gametype_remap2[s_serveroptions.gametype]], UI_CENTER|UI_SMALLFONT, text_color_normal );

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height + 28;
	if( b->generic.flags & QMF_HIGHLIGHT ) {	
		UI_DrawHandlePic( x, y, w, h, b->focusshader );
	}
}

/*
===============
ServerOptions_InitBotNames
===============
*/
static void ServerOptions_InitBotNames( void ) {
	int			count;
	int			n;
	const char	*arenaInfo;
	const char	*botInfo;
	char		*p;
	char		*bot;
	char		bots[MAX_INFO_STRING];

// changed to declare bots in arena files
//	if( s_serveroptions.gametype >= GT_TEAM ) {
    if( s_serveroptions.gametype > GT_DOMINATION ) {
		Q_strncpyz( s_serveroptions.playerNameBuffers[1], "Bobby", 16 );
		Q_strncpyz( s_serveroptions.playerNameBuffers[2], "Carla", 16 );
// changed to declare bots in arena files
//	if( s_serveroptions.gametype == GT_TEAM ) {
    if( s_serveroptions.gametype > GT_DOMINATION ) {
		Q_strncpyz( s_serveroptions.playerNameBuffers[3], "Paul", 16 );
		}    
		else {
			s_serveroptions.playerType[3].curvalue = 2;
		}
		s_serveroptions.playerType[4].curvalue = 2;
		s_serveroptions.playerType[5].curvalue = 2;

		Q_strncpyz( s_serveroptions.playerNameBuffers[6], "Alexandra", 16 );
		Q_strncpyz( s_serveroptions.playerNameBuffers[7], "Sam", 16 );
		Q_strncpyz( s_serveroptions.playerNameBuffers[8], "Dean", 16 );
// changed to declare bots in arena files
//	if( s_serveroptions.gametype == GT_TEAM ) {
    if( s_serveroptions.gametype > GT_DOMINATION ) {
		Q_strncpyz( s_serveroptions.playerNameBuffers[9], "Janine", 16 );
		}
		else {
			s_serveroptions.playerType[9].curvalue = 2;
		}
		s_serveroptions.playerType[10].curvalue = 2;
		s_serveroptions.playerType[11].curvalue = 2;

		return;
	}

	count = 1;	// skip the first slot, reserved for a human

	// get info for this map
	arenaInfo = UI_GetArenaInfoByMap( s_serveroptions.mapnamebuffer );

	// get the bot info - we'll seed with them if any are listed
	Q_strncpyz( bots, Info_ValueForKey( arenaInfo, "bots" ), sizeof(bots) );
	p = &bots[0];
	while( *p && count < PLAYER_SLOTS ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		botInfo = UI_GetBotInfoByName( bot );
		bot = Info_ValueForKey( botInfo, "name" );

		Q_strncpyz( s_serveroptions.playerNameBuffers[count], bot, sizeof(s_serveroptions.playerNameBuffers[count]) );
		count++;
	}

	// set the rest of the bot slots to "---"
	for( n = count; n < PLAYER_SLOTS; n++ ) {
		strcpy( s_serveroptions.playerNameBuffers[n], "--------" );
	}

	// pad up to #8 as open slots
	for( ;count < 8; count++ ) {
		s_serveroptions.playerType[count].curvalue = 0;
	}

	// close off the rest by default
	for( ;count < PLAYER_SLOTS; count++ ) {
		if( s_serveroptions.playerType[count].curvalue == 1 ) {
			s_serveroptions.playerType[count].curvalue = 2;
		}
	}
}



/*
=================
ServerOptions_SetMenuItems
=================
*/
static void ServerOptions_SetMenuItems( void ) {
	static char picname[64];

	switch( s_serveroptions.gametype ) {
// STONELANCE
	case GT_RACING:
		
	case GT_RACING_DM:
	default:
		Com_sprintf( s_serveroptions.fraglimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_racing_laplimit" ) ) );
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_racing_timelimit" ) ) );
		break;

	case GT_TEAM_RACING:
	case GT_TEAM_RACING_DM:
		Com_sprintf( s_serveroptions.fraglimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_team_racing_laplimit" ) ) );
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_team_racing_timelimit" ) ) );
		s_serveroptions.friendlyfire.curvalue = (int)Com_Clamp( 0, 1, trap_Cvar_VariableValue( "ui_team_racing_friendly" ) );
		break;

	case GT_DERBY:
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_derby_timelimit" ) ) );
		break;
		
	case GT_LCS:
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_lcs_timelimit" ) ) );
		break;

	case GT_DEATHMATCH:
		Com_sprintf( s_serveroptions.fraglimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_dm_fraglimit" ) ) );
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_dm_timelimit" ) ) );
		break;
// END
	case GT_TEAM:
		Com_sprintf( s_serveroptions.fraglimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_team_fraglimit" ) ) );
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_team_timelimit" ) ) );
		s_serveroptions.friendlyfire.curvalue = (int)Com_Clamp( 0, 1, trap_Cvar_VariableValue( "ui_team_friendly" ) );
		break;

	case GT_CTF:
		Com_sprintf( s_serveroptions.flaglimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 100, trap_Cvar_VariableValue( "ui_ctf_capturelimit" ) ) );
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_ctf_timelimit" ) ) );
		s_serveroptions.friendlyfire.curvalue = (int)Com_Clamp( 0, 1, trap_Cvar_VariableValue( "ui_ctf_friendly" ) );
		break;

// Q3Rally Code Start
	case GT_DOMINATION:
		Com_sprintf( s_serveroptions.flaglimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 100, trap_Cvar_VariableValue( "ui_dom_capturelimit" ) ) );
		Com_sprintf( s_serveroptions.timelimit.field.buffer, 4, "%i", (int)Com_Clamp( 0, 999, trap_Cvar_VariableValue( "ui_dom_timelimit" ) ) );
		s_serveroptions.friendlyfire.curvalue = (int)Com_Clamp( 0, 1, trap_Cvar_VariableValue( "ui_dom_friendly" ) );
		s_serveroptions.sigillocator.curvalue = (int)Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_sigilLocator" ) );
		break;
// Q3Rally Code END
	}

	Q_strncpyz( s_serveroptions.hostname.field.buffer, UI_Cvar_VariableString( "sv_hostname" ), sizeof( s_serveroptions.hostname.field.buffer ) );
	s_serveroptions.pure.curvalue = Com_Clamp( 0, 1, trap_Cvar_VariableValue( "sv_pure" ) );
// STONELANCE
	s_serveroptions.trackLength.curvalue = (int)Com_Clamp( 0, 2, trap_Cvar_VariableValue( "ui_racing_tracklength" ) );
	s_serveroptions.reversed.curvalue = (int)Com_Clamp( 0, 1, trap_Cvar_VariableValue( "ui_racing_trackreversed" ) );
// END

	// set the map pic
	Com_sprintf( picname, 64, "levelshots/%s", s_startserver.maplist[s_startserver.currentmap] );
	s_serveroptions.mappic.generic.name = picname;


	// set the map name
	strcpy( s_serveroptions.mapnamebuffer, s_startserver.mapname.string );
	Q_strupr( s_serveroptions.mapnamebuffer );

	// get the player selections initialized
	ServerOptions_InitPlayerItems();
	ServerOptions_SetPlayerItems();

	// seed bot names
	ServerOptions_InitBotNames();
	ServerOptions_SetPlayerItems();
}

/*
=================
PlayerName_Draw
=================
*/
static void PlayerName_Draw( void *item ) {
	menutext_s	*s;
	float		*color;
	int			x, y;
	int			style;
	qboolean	focus;

	s = (menutext_s *)item;

	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	UI_DrawString( x + SMALLCHAR_WIDTH, y, s->string, style|UI_LEFT, color );
}


/*
=================
ServerOptions_MenuInit
=================
*/
#define OPTIONS_X	456

static void ServerOptions_MenuInit( qboolean multiplayer ) {
	int		y;
	int		n;
//	static char cirname[64];
	

	memset( &s_serveroptions, 0 ,sizeof(serveroptions_t) );
	s_serveroptions.multiplayer = multiplayer;
	s_serveroptions.gametype = (int) Com_Clamp(0, ARRAY_LEN(gametype_remap2) - 1,
						trap_Cvar_VariableValue("g_gametype"));

	ServerOptions_Cache();

	s_serveroptions.menu.wrapAround = qtrue;
	s_serveroptions.menu.fullscreen = qtrue;

	s_serveroptions.banner.generic.type			= MTYPE_BTEXT;
	s_serveroptions.banner.generic.x			= 320;
	s_serveroptions.banner.generic.y			= 16;
	s_serveroptions.banner.string  				= "GAME SERVER";
// STONELANCE
	s_serveroptions.banner.color  				= text_color_normal;
// END
	s_serveroptions.banner.style  				= UI_CENTER;

	s_serveroptions.mappic.generic.type			= MTYPE_BITMAP;
	s_serveroptions.mappic.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
// STONELANCE
//	s_serveroptions.mappic.generic.x			= 352;
	s_serveroptions.mappic.generic.x			= 455;
	s_serveroptions.mappic.generic.y			= 80;
	s_serveroptions.mappic.width				= 170;
	s_serveroptions.mappic.height				= 96;
// END
	s_serveroptions.mappic.errorpic				= GAMESERVER_UNKNOWNMAP;
	s_serveroptions.mappic.generic.ownerdraw	= ServerOptions_LevelshotDraw;

// STONELANCE
/*
	s_serveroptions.picframe.generic.type		= MTYPE_BITMAP;
	s_serveroptions.picframe.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE|QMF_HIGHLIGHT;
	s_serveroptions.picframe.generic.x			= 352 - 38;
	s_serveroptions.picframe.generic.y			= 80 - 40;
	s_serveroptions.picframe.width  			= 320;
	s_serveroptions.picframe.height  			= 320;
	s_serveroptions.picframe.focuspic			= GAMESERVER_SELECT;
*/
// END

	y = 272;
// STONELANCE
	if( s_serveroptions.gametype == GT_RACING ||
		s_serveroptions.gametype == GT_RACING_DM ||
		s_serveroptions.gametype == GT_TEAM_RACING ||
		s_serveroptions.gametype == GT_TEAM_RACING_DM) {

		s_serveroptions.fraglimit.generic.type       = MTYPE_FIELD;
		s_serveroptions.fraglimit.generic.name       = "Laps:";
		s_serveroptions.fraglimit.generic.flags      = QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_serveroptions.fraglimit.generic.x	         = OPTIONS_X;
		s_serveroptions.fraglimit.generic.y	         = y;
		s_serveroptions.fraglimit.generic.statusbar  = ServerOptions_StatusBar;
		s_serveroptions.fraglimit.field.widthInChars = 3;
		s_serveroptions.fraglimit.field.maxchars     = 3;
	}
		else if( s_serveroptions.gametype != GT_CTF && s_serveroptions.gametype != GT_DOMINATION ) {
//	if( s_serveroptions.gametype != GT_CTF ) {
// END
		s_serveroptions.fraglimit.generic.type       = MTYPE_FIELD;
		s_serveroptions.fraglimit.generic.name       = "Frag Limit:";
		s_serveroptions.fraglimit.generic.flags      = QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_serveroptions.fraglimit.generic.x	         = OPTIONS_X;
		s_serveroptions.fraglimit.generic.y	         = y;
		s_serveroptions.fraglimit.generic.statusbar  = ServerOptions_StatusBar;
		s_serveroptions.fraglimit.field.widthInChars = 3;
		s_serveroptions.fraglimit.field.maxchars     = 3;
	}
	else {
		s_serveroptions.flaglimit.generic.type       = MTYPE_FIELD;
		s_serveroptions.flaglimit.generic.name       = "Capture Limit:";
		s_serveroptions.flaglimit.generic.flags      = QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_serveroptions.flaglimit.generic.x	         = OPTIONS_X;
		s_serveroptions.flaglimit.generic.y	         = y;
		s_serveroptions.flaglimit.generic.statusbar  = ServerOptions_StatusBar;
		s_serveroptions.flaglimit.field.widthInChars = 3;
		s_serveroptions.flaglimit.field.maxchars     = 3;
	}

	y += BIGCHAR_HEIGHT+2;
	s_serveroptions.timelimit.generic.type       = MTYPE_FIELD;
	s_serveroptions.timelimit.generic.name       = "Time Limit:";
	s_serveroptions.timelimit.generic.flags      = QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_serveroptions.timelimit.generic.x	         = OPTIONS_X;
	s_serveroptions.timelimit.generic.y	         = y;
	s_serveroptions.timelimit.generic.statusbar  = ServerOptions_StatusBar;
	s_serveroptions.timelimit.field.widthInChars = 3;
	s_serveroptions.timelimit.field.maxchars     = 3;

	if( s_serveroptions.gametype >= GT_TEAM ) {
		y += BIGCHAR_HEIGHT+2;
		s_serveroptions.friendlyfire.generic.type     = MTYPE_RADIOBUTTON;
		s_serveroptions.friendlyfire.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_serveroptions.friendlyfire.generic.x	      = OPTIONS_X;
		s_serveroptions.friendlyfire.generic.y	      = y;
		s_serveroptions.friendlyfire.generic.name	  = "Friendly Fire:";
	}

	y += BIGCHAR_HEIGHT+2;
	s_serveroptions.pure.generic.type			= MTYPE_RADIOBUTTON;
	s_serveroptions.pure.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_serveroptions.pure.generic.x				= OPTIONS_X;
	s_serveroptions.pure.generic.y				= y;
	s_serveroptions.pure.generic.name			= "Pure Server:";

	if ( trap_Cvar_VariableValue( "fs_unpure" ) ) {
		// ZTM: Don't let users think they can modify sv_pure, it won't work.
		s_serveroptions.pure.generic.flags |= QMF_GRAYED;
	}

// STONELANCE
	n = 0;
	if ( allowLength[0] ){
		track_length_list[n] = "Short";
		n++;
	}
	if ( allowLength[1] ){
		track_length_list[n] = "Medium";
		n++;
	}
	if ( allowLength[2] ){
		track_length_list[n] = "Long";
		n++;
	}
	track_length_list[n] = 0;

	y += BIGCHAR_HEIGHT+2;
	s_serveroptions.trackLength.generic.type		= MTYPE_SPINCONTROL;
	s_serveroptions.trackLength.generic.id			= ID_TRACK_LENGTH;
	s_serveroptions.trackLength.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
//	s_serveroptions.trackLength.generic.callback	= ServerOptions_Event;
	s_serveroptions.trackLength.generic.x			= OPTIONS_X;
	s_serveroptions.trackLength.generic.y			= y;
	s_serveroptions.trackLength.generic.name		= "Track Length:";
	s_serveroptions.trackLength.itemnames			= (const char **)track_length_list;

	y += BIGCHAR_HEIGHT+2;
	s_serveroptions.reversed.generic.type			= MTYPE_SPINCONTROL;
	s_serveroptions.reversed.generic.id				= ID_TRACK_REVERSED;
	s_serveroptions.reversed.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
//	s_serveroptions.reversed.generic.callback		= ServerOptions_Event;
	s_serveroptions.reversed.generic.x				= OPTIONS_X;
	s_serveroptions.reversed.generic.y				= y;
	s_serveroptions.reversed.generic.name			= "Track Reversed:";
	s_serveroptions.reversed.itemnames				= reversed_list;
// END
	if( s_serveroptions.multiplayer ) {
		y += BIGCHAR_HEIGHT+2;
		s_serveroptions.dedicated.generic.type		= MTYPE_SPINCONTROL;
		s_serveroptions.dedicated.generic.id		= ID_DEDICATED;
		s_serveroptions.dedicated.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_serveroptions.dedicated.generic.callback	= ServerOptions_Event;
		s_serveroptions.dedicated.generic.x			= OPTIONS_X;
		s_serveroptions.dedicated.generic.y			= y;
		s_serveroptions.dedicated.generic.name		= "Dedicated:";
		s_serveroptions.dedicated.itemnames			= dedicated_list;
	}

	if( s_serveroptions.multiplayer ) {
		y += BIGCHAR_HEIGHT+2;
		s_serveroptions.hostname.generic.type       = MTYPE_FIELD;
		s_serveroptions.hostname.generic.name       = "Hostname:";
		s_serveroptions.hostname.generic.flags      = QMF_SMALLFONT;
		s_serveroptions.hostname.generic.x          = OPTIONS_X;
		s_serveroptions.hostname.generic.y	        = y;
		s_serveroptions.hostname.field.widthInChars = 18;
		s_serveroptions.hostname.field.maxchars     = 64;
	}

if (s_serveroptions.gametype == GT_DOMINATION) {
    y += BIGCHAR_HEIGHT+2;
    s_serveroptions.dominationSpawnStyle.generic.type  = MTYPE_SPINCONTROL;
    s_serveroptions.dominationSpawnStyle.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_serveroptions.dominationSpawnStyle.generic.x     = OPTIONS_X;
    s_serveroptions.dominationSpawnStyle.generic.y     = y;
    s_serveroptions.dominationSpawnStyle.generic.name  = "Spawn Style:";
    s_serveroptions.dominationSpawnStyle.itemnames     = dtfspawn_list;

    y += BIGCHAR_HEIGHT+2;
    s_serveroptions.sigillocator.generic.type   = MTYPE_RADIOBUTTON;
    s_serveroptions.sigillocator.generic.flags  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_serveroptions.sigillocator.generic.x      = OPTIONS_X;
    s_serveroptions.sigillocator.generic.y      = y;
    s_serveroptions.sigillocator.generic.name   = "Flag Locator:";
  }

	y = 80;
	s_serveroptions.botSkill.generic.type			= MTYPE_SPINCONTROL;
	s_serveroptions.botSkill.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_serveroptions.botSkill.generic.name			= "Bot Skill:  ";
	s_serveroptions.botSkill.generic.x				= 32 + (strlen(s_serveroptions.botSkill.generic.name) + 2 ) * SMALLCHAR_WIDTH;
	s_serveroptions.botSkill.generic.y				= y;
	s_serveroptions.botSkill.itemnames				= botSkill_list;
	s_serveroptions.botSkill.curvalue				= 1;

	y += ( 2 * SMALLCHAR_HEIGHT );
	s_serveroptions.player0.generic.type			= MTYPE_TEXT;
	s_serveroptions.player0.generic.flags			= QMF_SMALLFONT;
	s_serveroptions.player0.generic.x				= 32 + SMALLCHAR_WIDTH;
	s_serveroptions.player0.generic.y				= y;
// STONELANCE
	//s_serveroptions.player0.color					= color_orange;
	s_serveroptions.player0.color					= text_color_normal;
// END
	s_serveroptions.player0.style					= UI_LEFT|UI_SMALLFONT;

	for( n = 0; n < PLAYER_SLOTS; n++ ) {
		s_serveroptions.playerType[n].generic.type		= MTYPE_SPINCONTROL;
		s_serveroptions.playerType[n].generic.flags		= QMF_SMALLFONT;
		s_serveroptions.playerType[n].generic.id		= ID_PLAYER_TYPE;
		s_serveroptions.playerType[n].generic.callback	= ServerOptions_Event;
		s_serveroptions.playerType[n].generic.x			= 32;
		s_serveroptions.playerType[n].generic.y			= y;
		s_serveroptions.playerType[n].itemnames			= playerType_list;

		s_serveroptions.playerName[n].generic.type		= MTYPE_TEXT;
		s_serveroptions.playerName[n].generic.flags		= QMF_SMALLFONT;
		s_serveroptions.playerName[n].generic.x			= 96;
		s_serveroptions.playerName[n].generic.y			= y;
		s_serveroptions.playerName[n].generic.callback	= ServerOptions_PlayerNameEvent;
		s_serveroptions.playerName[n].generic.id		= n;
		s_serveroptions.playerName[n].generic.ownerdraw	= PlayerName_Draw;
// STONELANCE
		//s_serveroptions.playerName[n].color				= color_orange;
		s_serveroptions.playerName[n].color				= text_color_normal;
// END
		s_serveroptions.playerName[n].style				= UI_SMALLFONT;
		s_serveroptions.playerName[n].string			= s_serveroptions.playerNameBuffers[n];
		s_serveroptions.playerName[n].generic.top		= s_serveroptions.playerName[n].generic.y;
		s_serveroptions.playerName[n].generic.bottom	= s_serveroptions.playerName[n].generic.y + SMALLCHAR_HEIGHT;
		s_serveroptions.playerName[n].generic.left		= s_serveroptions.playerName[n].generic.x - SMALLCHAR_HEIGHT/ 2;
		s_serveroptions.playerName[n].generic.right		= s_serveroptions.playerName[n].generic.x + 16 * SMALLCHAR_WIDTH;

		s_serveroptions.playerTeam[n].generic.type		= MTYPE_SPINCONTROL;
		s_serveroptions.playerTeam[n].generic.flags		= QMF_SMALLFONT;
		s_serveroptions.playerTeam[n].generic.x			= 240;
		s_serveroptions.playerTeam[n].generic.y			= y;
		s_serveroptions.playerTeam[n].itemnames			= playerTeam_list;

		y += ( SMALLCHAR_HEIGHT + 4 );
	}

// STONELANCE
/*
	s_serveroptions.back.generic.type	  = MTYPE_BITMAP;
	s_serveroptions.back.generic.name     = GAMESERVER_BACK0;
	s_serveroptions.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serveroptions.back.generic.callback = ServerOptions_Event;
	s_serveroptions.back.generic.id	      = ID_BACK;
	s_serveroptions.back.generic.x		  = 0;
	s_serveroptions.back.generic.y		  = 480-64;
	s_serveroptions.back.width  		  = 128;
	s_serveroptions.back.height  		  = 64;
	s_serveroptions.back.focuspic         = GAMESERVER_BACK1;

	s_serveroptions.go.generic.type	    = MTYPE_BITMAP;
	s_serveroptions.go.generic.name     = GAMESERVER_FIGHT0;
	s_serveroptions.go.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serveroptions.go.generic.callback = ServerOptions_Event;
	s_serveroptions.go.generic.id	    = ID_GO;
	s_serveroptions.go.generic.x		= 640;
	s_serveroptions.go.generic.y		= 480-64;
	s_serveroptions.go.width  		    = 128;
	s_serveroptions.go.height  		    = 64;
	s_serveroptions.go.focuspic         = GAMESERVER_FIGHT1;
*/

	s_serveroptions.back.generic.type		= MTYPE_PTEXT;
	s_serveroptions.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serveroptions.back.generic.x			= 20;
	s_serveroptions.back.generic.y			= 480 - 50;
	s_serveroptions.back.generic.id			= ID_BACK;
	s_serveroptions.back.generic.callback	= ServerOptions_Event; 
	s_serveroptions.back.string				= "< BACK";
	s_serveroptions.back.color				= text_color_normal;
	s_serveroptions.back.style				= UI_LEFT | UI_SMALLFONT;

	s_serveroptions.go.generic.type			= MTYPE_PTEXT;
	s_serveroptions.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serveroptions.go.generic.x			= 640 - 20;
	s_serveroptions.go.generic.y			= 480 - 50;
	s_serveroptions.go.generic.id			= ID_GO;
	s_serveroptions.go.generic.callback		= ServerOptions_Event; 
	s_serveroptions.go.string				= "RACE!";
	s_serveroptions.go.color				= text_color_normal;
	s_serveroptions.go.style				= UI_RIGHT | UI_SMALLFONT;
// END

	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.banner );

	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.mappic );

// STONELANCE
//	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.picframe );
// END
	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.botSkill );

	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.player0 );
	for( n = 0; n < PLAYER_SLOTS; n++ ) {
		if( n != 0 ) {
			Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.playerType[n] );
		}
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.playerName[n] );
		if( s_serveroptions.gametype >= GT_TEAM ) {
			Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.playerTeam[n] );
		}
	}

	// STONELANCE
	if( s_serveroptions.gametype == GT_CTF || s_serveroptions.gametype == GT_DOMINATION ) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.flaglimit );
	}
	else if( s_serveroptions.gametype != GT_DERBY && s_serveroptions.gametype != GT_LCS ) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.fraglimit );
	}
	// END

	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.timelimit );

	if( s_serveroptions.gametype >= GT_TEAM ) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.friendlyfire );
	}

	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.pure );

	// STONELANCE
	if( s_serveroptions.gametype == GT_RACING || s_serveroptions.gametype == GT_RACING_DM
		|| s_serveroptions.gametype == GT_TEAM_RACING || s_serveroptions.gametype == GT_TEAM_RACING_DM) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.trackLength );

		if ( reversable )
			Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.reversed );
	}
	// END

	if( s_serveroptions.multiplayer ) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.dedicated );
	}

	if( s_serveroptions.multiplayer ) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.hostname );
	}

	if (s_serveroptions.gametype == GT_DOMINATION) {
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.dominationSpawnStyle );
		Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.sigillocator );
	}

	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.back );
	Menu_AddItem( &s_serveroptions.menu, &s_serveroptions.go );

	ServerOptions_SetMenuItems();
}

/*
=================
ServerOptions_Cache
=================
*/
void ServerOptions_Cache( void ) {
// STONELANCE
/*
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_FIGHT0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_FIGHT1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_SELECT );
*/
// END
	trap_R_RegisterShaderNoMip( GAMESERVER_UNKNOWNMAP );
}


/*
=================
UI_ServerOptionsMenu
=================
*/
static void UI_ServerOptionsMenu( qboolean multiplayer ) {
	ServerOptions_MenuInit( multiplayer );
	UI_PushMenu( &s_serveroptions.menu );
}



/*
=============================================================================

BOT SELECT MENU *****

=============================================================================
*/


#define BOTSELECT_BACK0			"menu/art/back_0"
#define BOTSELECT_BACK1			"menu/art/back_1"
#define BOTSELECT_ACCEPT0		"menu/art/accept_0"
#define BOTSELECT_ACCEPT1		"menu/art/accept_1"
#define BOTSELECT_SELECT		"menu/art/opponents_select"
#define BOTSELECT_SELECTED		"menu/art/opponents_selected"
#define BOTSELECT_ARROWS		"menu/art/gs_arrows_0"
#define BOTSELECT_ARROWSL		"menu/art/gs_arrows_l"
#define BOTSELECT_ARROWSR		"menu/art/gs_arrows_r"

#define PLAYERGRID_COLS			4
#define PLAYERGRID_ROWS			4
#define MAX_MODELSPERPAGE		(PLAYERGRID_ROWS * PLAYERGRID_COLS)


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menubitmap_s	pics[MAX_MODELSPERPAGE];
	menubitmap_s	picbuttons[MAX_MODELSPERPAGE];
	menutext_s		picnames[MAX_MODELSPERPAGE];

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;

// STONELANCE
//	menubitmap_s	go;
//	menubitmap_s	back;
	menutext_s		go;
	menutext_s		back;
// END

	int				numBots;
	int				modelpage;
	int				numpages;
	int				selectedmodel;
	int				sortedBotNums[MAX_BOTS];
	char			boticons[MAX_MODELSPERPAGE][MAX_QPATH];
	char			botnames[MAX_MODELSPERPAGE][16];
} botSelectInfo_t;

static botSelectInfo_t	botSelectInfo;


/*
=================
UI_BotSelectMenu_SortCompare
=================
*/
static int QDECL UI_BotSelectMenu_SortCompare( const void *arg1, const void *arg2 ) {
	int			num1, num2;
	const char	*info1, *info2;
	const char	*name1, *name2;

	num1 = *(int *)arg1;
	num2 = *(int *)arg2;

	info1 = UI_GetBotInfoByNumber( num1 );
	info2 = UI_GetBotInfoByNumber( num2 );

	name1 = Info_ValueForKey( info1, "name" );
	name2 = Info_ValueForKey( info2, "name" );

	return Q_stricmp( name1, name2 );
}


/*
=================
UI_BotSelectMenu_BuildList
=================
*/
static void UI_BotSelectMenu_BuildList( void ) {
	int		n;

	botSelectInfo.modelpage = 0;
	botSelectInfo.numBots = UI_GetNumBots();
	botSelectInfo.numpages = botSelectInfo.numBots / MAX_MODELSPERPAGE;
	if( botSelectInfo.numBots % MAX_MODELSPERPAGE ) {
		botSelectInfo.numpages++;
	}

	// initialize the array
	for( n = 0; n < botSelectInfo.numBots; n++ ) {
		botSelectInfo.sortedBotNums[n] = n;
	}

	// now sort it
	qsort( botSelectInfo.sortedBotNums, botSelectInfo.numBots, sizeof(botSelectInfo.sortedBotNums[0]), UI_BotSelectMenu_SortCompare );
}


/*
=================
ServerPlayerIcon
=================
*/
static void ServerPlayerIcon( const char *modelAndSkin, char *iconName, int iconNameMaxSize ) {
	char	*skin;
	char	model[MAX_QPATH];

	Q_strncpyz( model, modelAndSkin, sizeof(model));
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
// STONELANCE
//		skin = "default";
		skin = "red";
// END
	}

	Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_%s.tga", model, skin );

// STONELANCE
/*
	if( !trap_R_RegisterShaderNoMip( iconName ) && Q_stricmp( skin, "default" ) != 0 ) {
		Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_default.tga", model );
	}
*/
	if( !trap_R_RegisterShaderNoMip( iconName ) ) {
		Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_red.tga", model );
	}
// END
}


/*
=================
UI_BotSelectMenu_UpdateGrid
=================
*/
static void UI_BotSelectMenu_UpdateGrid( void ) {
	const char	*info;
	int			i;
    int			j;

	j = botSelectInfo.modelpage * MAX_MODELSPERPAGE;
	for( i = 0; i < (PLAYERGRID_ROWS * PLAYERGRID_COLS); i++, j++) {
		if( j < botSelectInfo.numBots ) { 
			info = UI_GetBotInfoByNumber( botSelectInfo.sortedBotNums[j] );
			ServerPlayerIcon( Info_ValueForKey( info, "model" ), botSelectInfo.boticons[i], MAX_QPATH );
			Q_strncpyz( botSelectInfo.botnames[i], Info_ValueForKey( info, "name" ), 16 );
			Q_CleanStr( botSelectInfo.botnames[i] );
 			botSelectInfo.pics[i].generic.name = botSelectInfo.boticons[i];
			if( BotAlreadySelected( botSelectInfo.botnames[i] ) ) {
				botSelectInfo.picnames[i].color = color_red;
			}
			else {
// STONELANCE
				//botSelectInfo.picnames[i].color = color_orange;
				botSelectInfo.picnames[i].color = text_color_normal;
// END
			}
			botSelectInfo.picbuttons[i].generic.flags &= ~QMF_INACTIVE;
		}
		else {
			// dead slot
 			botSelectInfo.pics[i].generic.name         = NULL;
			botSelectInfo.picbuttons[i].generic.flags |= QMF_INACTIVE;
			botSelectInfo.botnames[i][0] = 0;
		}

 		botSelectInfo.pics[i].generic.flags       &= ~QMF_HIGHLIGHT;
 		botSelectInfo.pics[i].shader               = 0;
 		botSelectInfo.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	// set selected model
	i = botSelectInfo.selectedmodel % MAX_MODELSPERPAGE;
	botSelectInfo.pics[i].generic.flags |= QMF_HIGHLIGHT;
	botSelectInfo.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;

	if( botSelectInfo.numpages > 1 ) {
		if( botSelectInfo.modelpage > 0 ) {
			botSelectInfo.left.generic.flags &= ~QMF_INACTIVE;
		}
		else {
			botSelectInfo.left.generic.flags |= QMF_INACTIVE;
		}

		if( botSelectInfo.modelpage < (botSelectInfo.numpages - 1) ) {
			botSelectInfo.right.generic.flags &= ~QMF_INACTIVE;
		}
		else {
			botSelectInfo.right.generic.flags |= QMF_INACTIVE;
		}
	}
	else {
		// hide left/right markers
		botSelectInfo.left.generic.flags |= QMF_INACTIVE;
		botSelectInfo.right.generic.flags |= QMF_INACTIVE;
	}
}


/*
=================
UI_BotSelectMenu_Default
=================
*/
static void UI_BotSelectMenu_Default( char *bot ) {
	const char	*botInfo;
	const char	*test;
	int			n;
	int			i;

	for( n = 0; n < botSelectInfo.numBots; n++ ) {
		botInfo = UI_GetBotInfoByNumber( n );
		test = Info_ValueForKey( botInfo, "name" );
		if( Q_stricmp( bot, test ) == 0 ) {
			break;
		}
	}
	if( n == botSelectInfo.numBots ) {
		botSelectInfo.selectedmodel = 0;
		return;
	}

	for( i = 0; i < botSelectInfo.numBots; i++ ) {
		if( botSelectInfo.sortedBotNums[i] == n ) {
			break;
		}
	}
	if( i == botSelectInfo.numBots ) {
		botSelectInfo.selectedmodel = 0;
		return;
	}

	botSelectInfo.selectedmodel = i;
}


/*
=================
UI_BotSelectMenu_LeftEvent
=================
*/
static void UI_BotSelectMenu_LeftEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	if( botSelectInfo.modelpage > 0 ) {
		botSelectInfo.modelpage--;
		botSelectInfo.selectedmodel = botSelectInfo.modelpage * MAX_MODELSPERPAGE;
		UI_BotSelectMenu_UpdateGrid();
	}
}


/*
=================
UI_BotSelectMenu_RightEvent
=================
*/
static void UI_BotSelectMenu_RightEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	if( botSelectInfo.modelpage < botSelectInfo.numpages - 1 ) {
		botSelectInfo.modelpage++;
		botSelectInfo.selectedmodel = botSelectInfo.modelpage * MAX_MODELSPERPAGE;
		UI_BotSelectMenu_UpdateGrid();
	}
}


/*
=================
UI_BotSelectMenu_BotEvent
=================
*/
static void UI_BotSelectMenu_BotEvent( void* ptr, int event ) {
	int		i;

	if( event != QM_ACTIVATED ) {
		return;
	}

	for( i = 0; i < (PLAYERGRID_ROWS * PLAYERGRID_COLS); i++ ) {
 		botSelectInfo.pics[i].generic.flags &= ~QMF_HIGHLIGHT;
 		botSelectInfo.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	// set selected
	i = ((menucommon_s*)ptr)->id;
	botSelectInfo.pics[i].generic.flags |= QMF_HIGHLIGHT;
	botSelectInfo.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
	botSelectInfo.selectedmodel = botSelectInfo.modelpage * MAX_MODELSPERPAGE + i;
}


/*
=================
UI_BotSelectMenu_BackEvent
=================
*/
static void UI_BotSelectMenu_BackEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	UI_PopMenu();
}


/*
=================
UI_BotSelectMenu_SelectEvent
=================
*/
static void UI_BotSelectMenu_SelectEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	UI_PopMenu();

	s_serveroptions.newBot = qtrue;
	Q_strncpyz( s_serveroptions.newBotName, botSelectInfo.botnames[botSelectInfo.selectedmodel % MAX_MODELSPERPAGE], 16 );
}


/*
=================
UI_BotSelectMenu_Cache
=================
*/
void UI_BotSelectMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( BOTSELECT_BACK0 );
	trap_R_RegisterShaderNoMip( BOTSELECT_BACK1 );
	trap_R_RegisterShaderNoMip( BOTSELECT_ACCEPT0 );
	trap_R_RegisterShaderNoMip( BOTSELECT_ACCEPT1 );
	trap_R_RegisterShaderNoMip( BOTSELECT_SELECT );
	trap_R_RegisterShaderNoMip( BOTSELECT_SELECTED );
	trap_R_RegisterShaderNoMip( BOTSELECT_ARROWS );
	trap_R_RegisterShaderNoMip( BOTSELECT_ARROWSL );
	trap_R_RegisterShaderNoMip( BOTSELECT_ARROWSR );
}


static void UI_BotSelectMenu_Init( char *bot ) {
	int		i, j, k;
	int		x, y;

	memset( &botSelectInfo, 0 ,sizeof(botSelectInfo) );
	botSelectInfo.menu.wrapAround = qtrue;
	botSelectInfo.menu.fullscreen = qtrue;

	UI_BotSelectMenu_Cache();

	botSelectInfo.banner.generic.type	= MTYPE_BTEXT;
	botSelectInfo.banner.generic.x		= 320;
	botSelectInfo.banner.generic.y		= 16;
	botSelectInfo.banner.string			= "SELECT BOT";
	botSelectInfo.banner.color			= color_white;
	botSelectInfo.banner.style			= UI_CENTER;

	y =	80;
	for( i = 0, k = 0; i < PLAYERGRID_ROWS; i++) {
		x =	180;
		for( j = 0; j < PLAYERGRID_COLS; j++, k++ ) {
			botSelectInfo.pics[k].generic.type				= MTYPE_BITMAP;
			botSelectInfo.pics[k].generic.flags				= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
			botSelectInfo.pics[k].generic.x					= x;
			botSelectInfo.pics[k].generic.y					= y;
 			botSelectInfo.pics[k].generic.name				= botSelectInfo.boticons[k];
			botSelectInfo.pics[k].width						= 64;
			botSelectInfo.pics[k].height					= 64;
			botSelectInfo.pics[k].focuspic					= BOTSELECT_SELECTED;
			botSelectInfo.pics[k].focuscolor				= colorRed;

			botSelectInfo.picbuttons[k].generic.type		= MTYPE_BITMAP;
			botSelectInfo.picbuttons[k].generic.flags		= QMF_LEFT_JUSTIFY|QMF_NODEFAULTINIT|QMF_PULSEIFFOCUS;
			botSelectInfo.picbuttons[k].generic.callback	= UI_BotSelectMenu_BotEvent;
			botSelectInfo.picbuttons[k].generic.id			= k;
			botSelectInfo.picbuttons[k].generic.x			= x - 16;
			botSelectInfo.picbuttons[k].generic.y			= y - 16;
			botSelectInfo.picbuttons[k].generic.left		= x;
			botSelectInfo.picbuttons[k].generic.top			= y;
			botSelectInfo.picbuttons[k].generic.right		= x + 64;
			botSelectInfo.picbuttons[k].generic.bottom		= y + 64;
			botSelectInfo.picbuttons[k].width				= 128;
			botSelectInfo.picbuttons[k].height				= 128;
			botSelectInfo.picbuttons[k].focuspic			= BOTSELECT_SELECT;
			botSelectInfo.picbuttons[k].focuscolor			= colorRed;

			botSelectInfo.picnames[k].generic.type			= MTYPE_TEXT;
			botSelectInfo.picnames[k].generic.flags			= QMF_SMALLFONT;
			botSelectInfo.picnames[k].generic.x				= x + 32;
			botSelectInfo.picnames[k].generic.y				= y + 64;
			botSelectInfo.picnames[k].string				= botSelectInfo.botnames[k];
// STONELANCE
			//botSelectInfo.picnames[k].color					= color_orange;
			botSelectInfo.picnames[k].color					= text_color_normal;
// END
			botSelectInfo.picnames[k].style					= UI_CENTER|UI_SMALLFONT;

			x += (64 + 6);
		}
		y += (64 + SMALLCHAR_HEIGHT + 6);
	}

	botSelectInfo.arrows.generic.type		= MTYPE_BITMAP;
	botSelectInfo.arrows.generic.name		= BOTSELECT_ARROWS;
	botSelectInfo.arrows.generic.flags		= QMF_INACTIVE;
	botSelectInfo.arrows.generic.x			= 260;
	botSelectInfo.arrows.generic.y			= 440;
	botSelectInfo.arrows.width				= 128;
	botSelectInfo.arrows.height				= 32;

	botSelectInfo.left.generic.type			= MTYPE_BITMAP;
	botSelectInfo.left.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.left.generic.callback		= UI_BotSelectMenu_LeftEvent;
	botSelectInfo.left.generic.x			= 260;
	botSelectInfo.left.generic.y			= 440;
	botSelectInfo.left.width  				= 64;
	botSelectInfo.left.height  				= 32;
	botSelectInfo.left.focuspic				= BOTSELECT_ARROWSL;

	botSelectInfo.right.generic.type	    = MTYPE_BITMAP;
	botSelectInfo.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.right.generic.callback	= UI_BotSelectMenu_RightEvent;
	botSelectInfo.right.generic.x			= 321;
	botSelectInfo.right.generic.y			= 440;
	botSelectInfo.right.width  				= 64;
	botSelectInfo.right.height  		    = 32;
	botSelectInfo.right.focuspic			= BOTSELECT_ARROWSR;

// STONELANCE
/*
	botSelectInfo.back.generic.type		= MTYPE_BITMAP;
	botSelectInfo.back.generic.name		= BOTSELECT_BACK0;
	botSelectInfo.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.back.generic.callback	= UI_BotSelectMenu_BackEvent;
	botSelectInfo.back.generic.x		= 0;
	botSelectInfo.back.generic.y		= 480-64;
	botSelectInfo.back.width			= 128;
	botSelectInfo.back.height			= 64;
	botSelectInfo.back.focuspic			= BOTSELECT_BACK1;

	botSelectInfo.go.generic.type		= MTYPE_BITMAP;
	botSelectInfo.go.generic.name		= BOTSELECT_ACCEPT0;
	botSelectInfo.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.go.generic.callback	= UI_BotSelectMenu_SelectEvent;
	botSelectInfo.go.generic.x			= 640;
	botSelectInfo.go.generic.y			= 480-64;
	botSelectInfo.go.width				= 128;
	botSelectInfo.go.height				= 64;
	botSelectInfo.go.focuspic			= BOTSELECT_ACCEPT1;
*/

	botSelectInfo.back.generic.type		= MTYPE_PTEXT;
	botSelectInfo.back.generic.name		= BOTSELECT_BACK0;
	botSelectInfo.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.back.generic.x		= 20;
	botSelectInfo.back.generic.y		= 480 - 50;
	botSelectInfo.back.generic.callback	= UI_BotSelectMenu_BackEvent; 
	botSelectInfo.back.string			= "< BACK";
	botSelectInfo.back.color			= text_color_normal;
	botSelectInfo.back.style			= UI_LEFT | UI_SMALLFONT;

	botSelectInfo.go.generic.type		= MTYPE_PTEXT;
	botSelectInfo.go.generic.name		= BOTSELECT_ACCEPT0;
	botSelectInfo.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.go.generic.x			= 640 - 20;
	botSelectInfo.go.generic.y			= 480 - 50;
	botSelectInfo.go.generic.callback	= UI_BotSelectMenu_SelectEvent; 
	botSelectInfo.go.string				= "ACCEPT";
	botSelectInfo.go.color				= text_color_normal;
	botSelectInfo.go.style				= UI_RIGHT | UI_SMALLFONT;
// END

	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.banner );
	for( i = 0; i < MAX_MODELSPERPAGE; i++ ) {
		Menu_AddItem( &botSelectInfo.menu,	&botSelectInfo.pics[i] );
		Menu_AddItem( &botSelectInfo.menu,	&botSelectInfo.picbuttons[i] );
		Menu_AddItem( &botSelectInfo.menu,	&botSelectInfo.picnames[i] );
	}
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.arrows );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.left );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.right );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.back );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.go );

	UI_BotSelectMenu_BuildList();
	UI_BotSelectMenu_Default( bot );
	botSelectInfo.modelpage = botSelectInfo.selectedmodel / MAX_MODELSPERPAGE;
	UI_BotSelectMenu_UpdateGrid();
}


/*
=================
UI_BotSelectMenu
=================
*/
void UI_BotSelectMenu( char *bot ) {
	UI_BotSelectMenu_Init( bot );
	UI_PushMenu( &botSelectInfo.menu );
}
