/*
=======================================================================

INGAME CALLVOTE MAP SELECTION MENU

=======================================================================
*/





#include "ui_local.h"

#include "ui_startserver.h"
#include "ui_dynamicmenu.h"


#define MAPVOTE_FRAME		"menu/art/addbotframe"
#define ART_ARROWS			"menu/art/arrows_vert_0"
#define ART_ARROWUP			"menu/art/arrows_vert_top"
#define ART_ARROWDOWN		"menu/art/arrows_vert_bot"
#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_VOTE0			"menu/uie_art/vote_0"
#define ART_VOTE1			"menu/uie_art/vote_1"


#define LISTMAPS_PERPAGE	12
#define MAX_MAPNAME			44

#define SCROLLBUTTONS_X 	120


#define ID_CALLVOTE 1
#define ID_CANCEL 	2
#define ID_MAPS		3
#define ID_UP		4
#define ID_DOWN		5
#define ID_FILTER	6


typedef struct {
	menuframework_s menu;

	menubitmap_s 	frame;
	menutext_s		banner;

	menubitmap_s 	callvote;
	menubitmap_s 	cancel;
	menulist_s 		maps;
	menulist_s 		filter;

	menubitmap_s 	arrows;
	menubitmap_s 	up;
	menubitmap_s 	down;

	// local data
	int nummaps;
	int page;
	int maxpage;
	int index[MAX_MAPS_LIST];
	char text[LISTMAPS_PERPAGE][MAX_MAPNAME];

	const char* map_alias[LISTMAPS_PERPAGE];
} ingame_mapvote_t;


static ingame_mapvote_t s_mapvote;


static int filter_gametype[] = {
	-1, GT_SANDBOX, GT_FFA, GT_SINGLE, GT_TOURNAMENT, GT_TEAM, GT_CTF, GT_1FCTF, GT_OBELISK, GT_HARVESTER, GT_ELIMINATION, GT_CTF_ELIMINATION, GT_LMS, GT_DOUBLE_D, GT_DOMINATION
};


static int filter_gametype_size = sizeof(filter_gametype)/sizeof(filter_gametype[0]);


static const char* filter_gametype_list[] = {
	"All",
	"Sandbox",
	"Free for All",
	"Single Player",
	"Tournament",
	"Team DM",
	"Capture the Flag",
	"One Flag Capture",
	"Overload",
	"Harvester",
	"Elimination",
	"CTF Elimination",
	"Last Man Standing",
	"Double Domination",
	"Domination",
	0
};
static const char* filter_gametype_listru[] = {
	"Все",
	"Песочница",
	"Одиночная Игра",
	"Все Против Всех",
	"Турнир",
	"Командный Бой",
	"Захват флага",
	"Один Флаг",
	"Атака Базы",
	"Жнец",
	"Устранение",
	"Устранение: Захват флага",
	"Последний оставшийся",
	"Двойное доминирование",
	"Доминирование",
	0
};






/*
=================
MapVote_LoadMapList
=================
*/
static void MapVote_LoadMapList( void )
{
	s_mapvote.nummaps = UI_BuildMapListByType( s_mapvote.index, MAX_MAPS_LIST,
		filter_gametype[s_mapvote.filter.curvalue], 0 );

	s_mapvote.page = 0;
	s_mapvote.maxpage = s_mapvote.nummaps / LISTMAPS_PERPAGE;
	if (s_mapvote.nummaps % LISTMAPS_PERPAGE)
		s_mapvote.maxpage++;
}





/*
=================
MapVote_UpdateView
=================
*/
static void MapVote_UpdateView( void )
{
	const char* info;
	int i;
	int base;
	char* mapname;
	char* longname;
	char* spacebuf;
	int offset, maxoffset;

	if (s_mapvote.nummaps == 0) {
		Q_strncpyz(s_mapvote.text[0], "<<No maps found>>", MAX_MAPNAME);
		s_mapvote.maps.numitems = 1;
		return;
	}

	// spaces:  1234567890123456
	spacebuf = "                ";
	maxoffset = strlen(spacebuf) - 1;

	base = s_mapvote.page * LISTMAPS_PERPAGE;
	s_mapvote.maps.numitems = 0;
	for (i = 0; i < LISTMAPS_PERPAGE; i++)
	{
		if (base + i >= s_mapvote.nummaps)
			break;

		info = UI_GetArenaInfoByNumber(s_mapvote.index[base + i]);

		mapname = Info_ValueForKey(info, "map");
		longname = Info_ValueForKey(info, "longname");

		if (!mapname[0])
			continue;

		offset = strlen(mapname);
		if (offset > maxoffset)
			offset = maxoffset;

		Q_strncpyz( s_mapvote.text[i], va("%s%s - %s", &spacebuf[offset], mapname, longname), MAX_MAPNAME);

		s_mapvote.maps.numitems++;
	}

	if (s_mapvote.maps.curvalue >= s_mapvote.maps.numitems) {
		s_mapvote.maps.curvalue = s_mapvote.maps.numitems - 1;
	}

	// movement arrows
	if (s_mapvote.page == 0 || s_mapvote.maxpage == 1)
		s_mapvote.up.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	else
		s_mapvote.up.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

	if ((s_mapvote.page == s_mapvote.maxpage - 1) || s_mapvote.maxpage == 1)
		s_mapvote.down.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	else
		s_mapvote.down.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

	if (s_mapvote.maxpage == 1)
		s_mapvote.arrows.generic.flags |= QMF_GRAYED;
	else
		s_mapvote.arrows.generic.flags &= ~QMF_GRAYED;
}




/*
=================
MapVote_SetFilter
=================
*/
static void MapVote_SetFilter( int gametype )
{
	int i;

	s_mapvote.filter.curvalue = 0;
	for (i = 0; i < filter_gametype_size; i++)
	{
		if (gametype == filter_gametype[i]) {
			s_mapvote.filter.curvalue = i;
			break;
		}
	}

	MapVote_LoadMapList();
	MapVote_UpdateView();
}





/*
=================
MapVote_Cache
=================
*/
void MapVote_Cache( void )
{
	trap_R_RegisterShaderNoMip( MAPVOTE_FRAME );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWUP );
	trap_R_RegisterShaderNoMip( ART_ARROWDOWN );
}




/*
=================
MapVote_DoVote
=================
*/
static void MapVote_DoVote( void )
{
	const char* info;
	int index;

	index = s_mapvote.page * LISTMAPS_PERPAGE + s_mapvote.maps.curvalue;
	info = UI_GetArenaInfoByNumber(s_mapvote.index[index]);

	UI_ForceMenuOff();
	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote map %s\n", Info_ValueForKey(info, "map")));
}



/*
=================
MapVote_Event
=================
*/
static void MapVote_Event( void *ptr, int notification )
{
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch (((menucommon_s*)ptr)->id) {
	case ID_CALLVOTE:
		MapVote_DoVote();
		break;

	case ID_CANCEL:
		UI_PopMenu();
		break;

	case ID_MAPS:
		break;

	case ID_FILTER:
		MapVote_LoadMapList();
		MapVote_UpdateView();
		break;

	case ID_UP:
		if (s_mapvote.page > 0) {
			s_mapvote.page--;
			MapVote_UpdateView();
		}
		break;

	case ID_DOWN:
		if (s_mapvote.page < s_mapvote.maxpage - 1) {
			s_mapvote.page++;
			MapVote_UpdateView();
		}
		break;
	}

}




/*
=================
MapVote_MenuInit
=================
*/
static void MapVote_MenuInit( void ) {
	int		y;
	int		i;
	int		gametype;

	memset( &s_mapvote, 0 ,sizeof(ingame_mapvote_t) );

	MapVote_Cache();

	s_mapvote.menu.wrapAround = qtrue;
	s_mapvote.menu.native 	  = qfalse;
	s_mapvote.menu.fullscreen = qfalse;
//	s_mapvote.menu.draw = InGame_MenuDraw;

	s_mapvote.frame.generic.type			= MTYPE_BITMAP;
	s_mapvote.frame.generic.flags		= QMF_INACTIVE;
	s_mapvote.frame.generic.name			= MAPVOTE_FRAME;
	s_mapvote.frame.generic.x			= 320-280;
	s_mapvote.frame.generic.y			= 240-180;
	s_mapvote.frame.width				= 560;
	s_mapvote.frame.height				= 360;

	s_mapvote.banner.generic.type		= MTYPE_BTEXT;
	s_mapvote.banner.generic.x			= 320;
	s_mapvote.banner.generic.y			= 16;
	s_mapvote.banner.color				= color_white;
	s_mapvote.banner.style				= UI_CENTER;
	if(cl_language.integer == 0){
	s_mapvote.banner.string				= "CALLVOTE MAP";
	}
	if(cl_language.integer == 1){
	s_mapvote.banner.string				= "ВЫЗОВ ГОЛОСОВАНИЯ КАРТЫ";
	}


	for (i = 0; i < LISTMAPS_PERPAGE; i++) {
		s_mapvote.map_alias[i] = s_mapvote.text[i];
	}

	s_mapvote.maps.generic.type		= MTYPE_SCROLLLIST;
	s_mapvote.maps.generic.flags		= QMF_PULSEIFFOCUS;
	s_mapvote.maps.generic.callback	= MapVote_Event;
	s_mapvote.maps.generic.id			= ID_MAPS;
	s_mapvote.maps.generic.x			= 176;
	s_mapvote.maps.generic.y			= 98;
	s_mapvote.maps.width				= MAX_MAPNAME;
	s_mapvote.maps.height				= LISTMAPS_PERPAGE;
	s_mapvote.maps.numitems			= 0;
	s_mapvote.maps.itemnames			= s_mapvote.map_alias;
	s_mapvote.maps.columns			= 1;

	y = 180;
	s_mapvote.arrows.generic.type  = MTYPE_BITMAP;
	s_mapvote.arrows.generic.name  = ART_ARROWS;
	s_mapvote.arrows.generic.flags = QMF_INACTIVE;
	s_mapvote.arrows.generic.x	 = SCROLLBUTTONS_X;
	s_mapvote.arrows.generic.y	 = y;
	s_mapvote.arrows.width  	     = 64;
	s_mapvote.arrows.height  	     = 128;

	s_mapvote.up.generic.type     = MTYPE_BITMAP;
	s_mapvote.up.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapvote.up.generic.callback = MapVote_Event;
	s_mapvote.up.generic.id	    = ID_UP;
	s_mapvote.up.generic.x		= SCROLLBUTTONS_X;
	s_mapvote.up.generic.y		= y;
	s_mapvote.up.width  		    = 64;
	s_mapvote.up.height  		    = 64;
	s_mapvote.up.focuspic     = ART_ARROWUP;

	s_mapvote.down.generic.type     = MTYPE_BITMAP;
	s_mapvote.down.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapvote.down.generic.callback = MapVote_Event;
	s_mapvote.down.generic.id	    = ID_DOWN;
	s_mapvote.down.generic.x		= SCROLLBUTTONS_X;
	s_mapvote.down.generic.y		=  y + 64;
	s_mapvote.down.width  		    = 64;
	s_mapvote.down.height  		    = 64;
	s_mapvote.down.focuspic = ART_ARROWDOWN;

	s_mapvote.callvote.generic.type     = MTYPE_BITMAP;
	s_mapvote.callvote.generic.name 	= ART_VOTE0;
	s_mapvote.callvote.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapvote.callvote.generic.callback = MapVote_Event;
	s_mapvote.callvote.generic.id	    = ID_CALLVOTE;
	s_mapvote.callvote.generic.x		= 320;
	s_mapvote.callvote.generic.y		=  256 + 64;
	s_mapvote.callvote.width  		    = 128;
	s_mapvote.callvote.height  		    = 64;
	s_mapvote.callvote.focuspic 		= ART_VOTE1;

	s_mapvote.cancel.generic.type     = MTYPE_BITMAP;
	s_mapvote.cancel.generic.name 	= ART_BACK0;
	s_mapvote.cancel.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapvote.cancel.generic.callback = MapVote_Event;
	s_mapvote.cancel.generic.id	    = ID_CANCEL;
	s_mapvote.cancel.generic.x		= 320 - 128;
	s_mapvote.cancel.generic.y		=  256 + 64;
	s_mapvote.cancel.width  		    = 128;
	s_mapvote.cancel.height  		    = 64;
	s_mapvote.cancel.focuspic 		= ART_BACK1;

	s_mapvote.filter.generic.type		= MTYPE_SPINCONTROL;
	s_mapvote.filter.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapvote.filter.generic.callback	= MapVote_Event;
	s_mapvote.filter.generic.id			= ID_FILTER;
	s_mapvote.filter.generic.x			= 320;
	s_mapvote.filter.generic.y			= 256 + 64 - SMALLCHAR_HEIGHT;
	if(cl_language.integer == 0){
	s_mapvote.filter.generic.name		= "Gametype:";
	s_mapvote.filter.itemnames			= filter_gametype_list;
	}
	if(cl_language.integer == 1){
	s_mapvote.filter.generic.name		= "Режим игры:";
	s_mapvote.filter.itemnames			= filter_gametype_listru;
	}

	Menu_AddItem( &s_mapvote.menu, &s_mapvote.frame );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.banner );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.maps );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.arrows );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.up );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.down );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.callvote );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.cancel );
	Menu_AddItem( &s_mapvote.menu, &s_mapvote.filter);

	gametype = DynamicMenu_ServerGametype();
	MapVote_SetFilter(gametype);
}



/*
=================
UI_MapCallVote
=================
*/
void UI_MapCallVote( void )
{
	MapVote_MenuInit();

	UI_PushMenu(&s_mapvote.menu);
}


