/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/



#include "ui_local.h"
#include "ui_startserver_q3.h"


/*
=============================================================================

MAP SELECT MENU *****

=============================================================================
*/

#define MAPSELECT_HARROWS "menu/art/arrows_horz_0"
#define MAPSELECT_NEXT "menu/art/arrows_horz_right"
#define MAPSELECT_PREVIOUS "menu/art/arrows_horz_left"
#define MAPSELECT_CANCEL0 "menu/art/back_0"
#define MAPSELECT_CANCEL1 "menu/art/back_1"
#define MAPSELECT_ACCEPT0 "menu/art/accept_0"
#define MAPSELECT_ACCEPT1 "menu/art/accept_1"
#define MAPSELECT_MAPFOCUS "menu/uie_art/mapfocus"
#define MAPSELECT_MAPSELECTED "menu/art/maps_selected"

#define MAPNAME_LONGBUFFER 64

#define MAPSELECT_ERRORPIC "menu/art/unknownmap"

#define MAPPIC_WIDTH 138
#define MAPPIC_HEIGHT 92

#define ID_MAPSELECT_CANCEL 10
#define ID_MAPSELECT_NEXT 11
#define ID_MAPSELECT_PREV 12
#define ID_MAPSELECT_OK 13
#define ID_MAPSELECT_ALLMAPS 14
#define ID_MAPSELECT_FILTERMAPS 15
#define ID_MAPSELECT_MAPICONS 16
#define ID_MAPSELECT_MULTISEL 17
#define ID_MAPSELECT_LISTVIEW 18

#define MAPGRID_ROWS 3
#define MAPGRID_COLUMNS 4
#define MAX_GRIDMAPSPERPAGE (MAPGRID_ROWS * MAPGRID_COLUMNS)

#define MAPLIST_ROWS 12
#define MAPLIST_COLUMNS 3
#define MAX_LISTMAPSPERPAGE (MAPLIST_ROWS * MAPLIST_COLUMNS)

#if (MAX_LISTMAPSPERPAGE > MAX_GRIDMAPSPERPAGE)
	#define MAX_MAPSPERPAGE MAX_LISTMAPSPERPAGE
#else
	#define MAX_MAPSPERPAGE MAX_GRIDMAPSPERPAGE
#endif




const char* mapicons_items[] = {
	"All",		// MAPICONS_ALL
	"Custom",	// MAPICONS_CUSTOM
	"None",		// MAPICONS_NONE
	0
};
const char* mapicons_itemsru[] = {
	"Все",		// MAPICONS_ALL
	"Свои",	// MAPICONS_CUSTOM
	"Нет",		// MAPICONS_NONE
	0
};



static int ms_lastgametype = -1;
static int ms_allmaps = 0;
static int ms_filter = MAPFILTER_OFF;

static vec4_t color_nomap = {0.75, 0.0, 0.0, 1.0};


typedef struct mapselect_s {
	menuframework_s menu;

	menutext_s banner;
	menubitmap_s mappics[MAX_GRIDMAPSPERPAGE];

	menubitmap_s icona;
	menubitmap_s iconb;

	menubitmap_s arrows;
	menubitmap_s next;
	menubitmap_s previous;
	menubitmap_s cancel;
	menubitmap_s accept;

	menutext_s maptype;
	menuradiobutton_s allmaps;
	menulist_s filter;
	menulist_s mapicons;

	menuradiobutton_s multisel;
	menuradiobutton_s listview;

	menulist_s maplist;

#ifndef NO_UIE_MINILOGO_SKIRMISH
	menubitmap_s	logo;
#endif

	int gametype;	// GT_* format
	int nummaps;
	int maxpages;
	int page;

	// index into index_maplist[]
	// -1 => no selection
	// otherwise >=0 and <nummaps
	int currentmap;

	int index;	// index of map to change
	qboolean nomaps;

	int maxMapsPerPage;
	mappic_t mapinfo[MAX_MAPSPERPAGE];

	const char* maplist_alias[MAX_LISTMAPSPERPAGE];

	float* maptext_color[MAX_MAPSPERPAGE];
	int index_maplist[MAX_MAPS_LIST];
	char maplongname[MAX_MAPS_LIST][MAPNAME_LONGBUFFER];
	char mapdrawname[MAX_MAPS_LIST][MAPNAME_LONGBUFFER];
	int mapsecondline[MAX_MAPS_LIST];

	int bottomrow_y;

	// index into index_maplist[]
	// valid values always between >= 0 and <nummaps
	int multiselect[MAX_NUMMAPS];
	int num_multisel;
} mapselect_t;

static mapselect_t s_mapselect;





/*
=================
MapSelect_OnMultiList

returns position on list, -1 of not found
=================
*/
static int MapSelect_MultiListPos(int index)
{
	int i;
	for (i = 0; i < s_mapselect.num_multisel; i++)
		if (s_mapselect.multiselect[i] == index)
			return i;

	return -1;		
}



/*
=================
MapSelect_DeleteMultiAtPos
=================
*/
static void MapSelect_DeleteMultiAtPos(int pos)
{
	int i;

	if (pos < 0 || pos >= s_mapselect.num_multisel)
		return;

	for (i = pos; i < s_mapselect.num_multisel - 1; i++)
	{
		s_mapselect.multiselect[i] = s_mapselect.multiselect[i + 1];
	}

	s_mapselect.num_multisel--;
}



/*
=================
MapSelect_AddToMultiSelect

Expects an index into the index_maplist[],
NOT an Arena number
=================
*/
static void MapSelect_AddToMultiSelect(int index)
{
	int pos;

	if (s_mapselect.num_multisel == MAX_NUMMAPS || index < 0)
		return;

	// if already on list, remove it
	pos = MapSelect_MultiListPos(index);
	if (pos >= 0) {
		MapSelect_DeleteMultiAtPos(pos);
		return;
	}

	s_mapselect.multiselect[s_mapselect.num_multisel] = index;
	s_mapselect.num_multisel++;
}



/*
=================
MapSelect_ClearMultiSelect
=================
*/
static void MapSelect_ClearMultiSelect(void)
{
	s_mapselect.num_multisel = 0;
}



/*
=================
MapSelect_ToggleMultiSelect
=================
*/
static void MapSelect_ToggleMultiSelect(void)
{
	trap_Cvar_SetValue("uie_map_multisel", s_mapselect.multisel.curvalue);

	// from single to multiple
	if (s_mapselect.multisel.curvalue) {
		// reset if single selection is different
		if (s_mapselect.num_multisel > 0 && s_mapselect.currentmap != s_mapselect.multiselect[0])
		{
			MapSelect_ClearMultiSelect();
			MapSelect_AddToMultiSelect(s_mapselect.currentmap);
		}
		else if (s_mapselect.num_multisel == 0)
			MapSelect_AddToMultiSelect(s_mapselect.currentmap);

		s_mapselect.currentmap = -1;
		return;
	}

	// from multiple back to single
	if (s_mapselect.num_multisel) {
		s_mapselect.currentmap = s_mapselect.multiselect[0];
	}
	else {
		s_mapselect.currentmap = -1;
	}
}




/*
=================
MapSelect_MapCellSize
=================
*/
static void MapSelect_MapCellSize(int* colh, int* colw)
{
	// screen height - 2 buttons
	// colh rounded to multiple of 2 to reduce drawing "artifacts"

	*colw = 640 / MAPGRID_COLUMNS;
	*colh = (( 480 - 2 * 64 ) / MAPGRID_ROWS) & 0xFE;
}



/*
=================
MapSelect_SetViewType
=================
*/
static void MapSelect_SetViewType(void)
{
	if (s_mapselect.listview.curvalue) {
		s_mapselect.maxMapsPerPage = MAX_LISTMAPSPERPAGE;
	}
	else {
		s_mapselect.maxMapsPerPage = MAX_GRIDMAPSPERPAGE;
	}

	s_mapselect.maxpages = s_mapselect.nummaps / s_mapselect.maxMapsPerPage;
	if (s_mapselect.nummaps % s_mapselect.maxMapsPerPage)
		s_mapselect.maxpages++;

	s_mapselect.page = 0;
	if (s_mapselect.currentmap >= 0)
	{
		s_mapselect.page = s_mapselect.currentmap / s_mapselect.maxMapsPerPage;
	}
}



/*
=================
MapSelect_MapIndex
=================
*/
static int MapSelect_MapIndex(const char* mapname)
{
	int i;
	const char* info;

	// check for valid mapname
	if (!mapname || *mapname == '\0')
		return -1;

	// find the map
	for (i = 0 ; i < s_mapselect.nummaps; i++)
	{
		info = UI_GetArenaInfoByNumber(s_mapselect.index_maplist[i]);
		if (Q_stricmp(mapname, Info_ValueForKey(info,"map")) == 0)
			return i;
	}

	return -1;
}



/*
=================
MapSelect_MapSupportsGametype
=================
*/
static qboolean MapSelect_MapSupportsGametype(const char* mapname) {
	int i;
	int count, matchbits, gamebits;
	const char	*info;
	char* arena_mapname;

	if (!mapname || !mapname[0])
		return qtrue;

	count = UI_GetNumArenas();
	if (count > MAX_MAPS_LIST)
		count = MAX_MAPS_LIST;

	matchbits = 1 << s_mapselect.gametype;
	for( i = 0; i < count; i++ ) {
		info = UI_GetArenaInfoByNumber( i );

		if (!info) {
			continue;
		}

		arena_mapname = Info_ValueForKey( info, "map");
		if (!arena_mapname || arena_mapname[0] == '\0') {
			continue;
		}

		gamebits = GametypeBits( Info_ValueForKey( info, "type") );
		if( !( gamebits & matchbits )) {
			continue;
		}
		if (Q_stricmp(mapname, arena_mapname) == 0)
			return qtrue;
	}

	return qfalse;
}





/*
=================
MapSelect_FilteredMap
=================
*/
static qboolean MapSelect_FilteredMap(const char* mapname)
{
	qboolean idmap;
	int type;

	if (s_mapselect.filter.curvalue == MAPFILTER_OFF)
		return qtrue;

	// handle request for Id or non-Id map type
	if (s_mapselect.filter.curvalue < MAPFILTER_MAX)
	{
		idmap = StartServer_IsIdMap(mapname);
		if (s_mapselect.filter.curvalue == MAPFILTER_NONID)
		{
			if (idmap)
				return qfalse;
			return qtrue;
		}
		return idmap;
	}

	// check for specific map list
	type = s_mapselect.filter.curvalue - MAPFILTER_MAX;
	return StartServer_IsCustomMapType(mapname, type);
}




/*
=================
MapSelect_SetMapTypeIcons
=================
*/
static void MapSelect_SetMapTypeIcons( void )
{
	int gametype, customtype;
	menubitmap_s* icon_type;
	menubitmap_s* icon_custom;

	icon_type = &s_mapselect.icona;
	icon_custom = &s_mapselect.iconb;

	// using all maps, so don't set gametype icon
	gametype = s_mapselect.gametype;
	if (s_mapselect.allmaps.curvalue)
	{
		gametype = -1;
	}

	// check for custom map icon, bump gametype icon
	// to left if so
	customtype = s_mapselect.filter.curvalue - MAPFILTER_MAX;
	if (customtype >= 0)
	{
		icon_custom = icon_type;
		icon_type = &s_mapselect.iconb;
	}

	StartServer_SetIconFromGameType(icon_type, gametype, qfalse);
	StartServer_SetIconFromGameType(icon_custom, customtype, qtrue);
}




/*
=================
MapSelect_CheckLoadedMap
=================
*/
static qboolean MapSelect_ValidateMapForLoad( const char* info, int matchbits, qboolean cache)
{
	int gamebits;
	const char* arena_mapname;

	// error check the map
	arena_mapname = Info_ValueForKey( info, "map");
	if (!arena_mapname || arena_mapname[0] == '\0') {
		return qfalse;
	}

	gamebits = GametypeBits( Info_ValueForKey( info, "type") );
	if( !( gamebits & matchbits ) && !s_mapselect.allmaps.curvalue) {
		return qfalse;
	}

	if (!MapSelect_FilteredMap(arena_mapname))
		return qfalse;

	// cache map image
	if (cache) {
		trap_R_RegisterShaderNoMip( va("levelshots/%s.tga", arena_mapname) );
	}

	return qtrue;
}



/*
=================
MapSelect_LoadMaps
=================
*/
static void MapSelect_LoadMaps(const char* mapname, qboolean cache) {
	int			i;
	int			count;
	int			matchbits;
	const char	*info;
	int j, nchars, lastspace, secondline, count2;
	char *buf;

	count = UI_GetNumArenas();
	if (count > MAX_MAPS_LIST)
		count = MAX_MAPS_LIST;

	s_mapselect.nummaps = 0;
	matchbits = 1 << s_mapselect.gametype;

	for( i = 0; i < count; i++ ) {
		info = UI_GetArenaInfoByNumber( i );

		if (!info || !MapSelect_ValidateMapForLoad(info, matchbits, cache)) {
			if (!info)
				trap_Print(va("Load Map error (%i)\n", i));
			continue;
		}

		s_mapselect.index_maplist[s_mapselect.nummaps] = i;
		Q_strncpyz( s_mapselect.maplongname[s_mapselect.nummaps], Info_ValueForKey( info, "longname"), MAPNAME_LONGBUFFER);

		// convert the map long name into a name that spans (at most) 2 rows
		Q_strncpyz( s_mapselect.mapdrawname[s_mapselect.nummaps], s_mapselect.maplongname[s_mapselect.nummaps], MAPNAME_LONGBUFFER);
		buf = s_mapselect.mapdrawname[s_mapselect.nummaps];
		buf[MAPNAME_LONGBUFFER - 1] = '\0';
		nchars = (640 / (SMALLCHAR_WIDTH * MAPGRID_COLUMNS)) - 1;
		lastspace = 0;
		count2 = 0;
		secondline = 0;
		for (j = 0; j < MAPNAME_LONGBUFFER; j++)
		{
		   if (buf[j] == '\0')
			   break;
		   if (buf[j] == ' ')
			   lastspace = j;

		   count2++;
		   if ((count2 % nchars) == 0)
		   {
			   if (lastspace)
			   {
				   buf[lastspace] = '\0';
				   count2 = j - lastspace;
				   if (!secondline)
					   secondline = lastspace + 1;
				   lastspace = 0;
				   continue;
			   }
			   // move always preserves buf[MAPNAME_LONGBUFFER - 1]
			   memcpy(&buf[j + 1], &buf[j], MAPNAME_LONGBUFFER - j - 2 );
			   buf[j] = '\0';
			   count2 = 0;
			   if (!secondline)
				   secondline = j + 1;
		   }
		}
		s_mapselect.mapsecondline[s_mapselect.nummaps] = secondline;

		s_mapselect.nummaps++;
	}

	// set up the correct map page
	s_mapselect.currentmap = MapSelect_MapIndex(mapname);
	MapSelect_SetViewType();
}



/*
=================
MapSelect_HighlightIfOnPage

Only used for grid display of maps
=================
*/
static void MapSelect_HighlightIfOnPage( int index )
{
	int i;

	i = index - s_mapselect.page * MAX_GRIDMAPSPERPAGE;
	if (i >=0 && i < MAX_GRIDMAPSPERPAGE)
	{
		s_mapselect.mappics[i].generic.flags |= QMF_HIGHLIGHT;
		s_mapselect.mappics[i].generic.flags &= ~QMF_PULSEIFFOCUS;
	}
}


/*
=================
MapSelect_OnCurrentPage
=================
*/
static qboolean MapSelect_OnCurrentPage(int index)
{
	int base;

	base = s_mapselect.page * s_mapselect.maxMapsPerPage;
	if (index < base)
		return qfalse;

	if (index >= base + s_mapselect.maxMapsPerPage)
		return qfalse;	

	if (index >= s_mapselect.nummaps)
		return qfalse;

	return qtrue;
}



/*
=================
MapSelect_UpdateAcceptInterface
=================
*/
static void MapSelect_UpdateAcceptInterface( void )
{
	// enable/disable accept button
	if (s_mapselect.multisel.curvalue)
	{
		if (s_mapselect.num_multisel == 0)
			s_mapselect.accept.generic.flags |= (QMF_GRAYED | QMF_INACTIVE);
		else
			s_mapselect.accept.generic.flags &= ~(QMF_GRAYED | QMF_INACTIVE);
	}
	else {
		if (s_mapselect.currentmap == -1) {
			s_mapselect.accept.generic.flags |= (QMF_GRAYED | QMF_INACTIVE);
		}
		else {
			s_mapselect.accept.generic.flags &= ~(QMF_GRAYED | QMF_INACTIVE);
		}
	}
}



/*
=================
MapSelect_UpdateInterface
=================
*/
static void MapSelect_UpdateInterface( void )
{
	int i;
	int top;

	if (s_mapselect.listview.curvalue) {
		for (i = 0; i < MAX_GRIDMAPSPERPAGE; i++) {
			s_mapselect.mappics[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
		}
		s_mapselect.maplist.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	else {
		// set attributes of buttons
		top = s_mapselect.page * MAX_GRIDMAPSPERPAGE;
		for (i = 0; i < MAX_GRIDMAPSPERPAGE; i++)
		{
		  if ((top + i) >= s_mapselect.nummaps)
			  break;

		  s_mapselect.mappics[i].generic.flags &= ~(QMF_HIGHLIGHT|QMF_INACTIVE|QMF_HIDDEN);
		  s_mapselect.mappics[i].generic.flags |= QMF_PULSEIFFOCUS;
		}

		for (; i < MAX_GRIDMAPSPERPAGE; i++)
		{
		  s_mapselect.mappics[i].generic.flags &= ~(QMF_HIGHLIGHT|QMF_PULSEIFFOCUS);
		  s_mapselect.mappics[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
		}

		s_mapselect.maplist.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);

		if (s_mapselect.multisel.curvalue) {
			for (i = 0; i < s_mapselect.num_multisel; i++)
				MapSelect_HighlightIfOnPage(s_mapselect.multiselect[i]);
		}
		else {
			MapSelect_HighlightIfOnPage(s_mapselect.currentmap);
		}
	}

	MapSelect_UpdateAcceptInterface();
}



/*
=================
MapSelect_SetNewMapPics
=================
*/
static void MapSelect_SetNewMapPics( void )
{
	int i, j;
	int top;
	const char* info;

	// set map names and start with clear buttons
	top = s_mapselect.page * s_mapselect.maxMapsPerPage;
	for (i = 0; i < s_mapselect.maxMapsPerPage; i++)
	{
		if ((top + i) >= s_mapselect.nummaps)
			break;

		StartServer_InitMapPictureFromIndex(&s_mapselect.mapinfo[i], s_mapselect.index_maplist[top + i]);

		s_mapselect.maptext_color[i] = color_white;

		// check if map has been used before
		if (s_mapselect.index < 0 || s_mapselect.index >= MAX_NUMMAPS)
			continue;

		for (j = 0; j < s_scriptdata.map.num_maps; j++)
		{
			info = UI_GetArenaInfoByNumber(s_mapselect.index_maplist[top + i]);
			if (Q_stricmp(Info_ValueForKey(info, "map"), s_scriptdata.map.data[j].shortName) == 0) {
				s_mapselect.maptext_color[i] = color_white;
				break;
			}
		}
	}

	// clear any left-over grid buttons
	if (!s_mapselect.listview.curvalue) {
		for (; i < MAX_GRIDMAPSPERPAGE; i++)
		{
			s_mapselect.mappics[i].generic.name = 0;
			s_mapselect.mappics[i].shader = 0;
		}
	}

	// no maps found
	if (!s_mapselect.nummaps)
	{
		s_mapselect.nomaps = qtrue;
		s_mapselect.accept.generic.flags |= QMF_INACTIVE;
	}
	else
	{
		s_mapselect.nomaps = qfalse;
	}
}



/*
=================
MapSelect_SetMapListData
=================
*/
static void MapSelect_SetNewListNames(void)
{
	int i;
	int base;

	base = s_mapselect.page * s_mapselect.maxMapsPerPage;
	s_mapselect.maplist.numitems = 0;
	for (i = 0; i < s_mapselect.maxMapsPerPage; i++) {
		if (base + i == s_mapselect.nummaps)
			break;
		s_mapselect.maplist_alias[i] = s_mapselect.mapinfo[i].mapname;
		s_mapselect.maplist.numitems++;
	}

	if (MapSelect_OnCurrentPage(s_mapselect.currentmap))
		s_mapselect.maplist.curvalue = s_mapselect.currentmap % s_mapselect.maxMapsPerPage;
	else
		s_mapselect.maplist.curvalue = -1;	

    s_mapselect.maplist.oldvalue = s_mapselect.maplist.curvalue;
}



/*
=================
MapSelect_SetNewMapPage
=================
*/
static void MapSelect_SetNewMapPage( void )
{
	MapSelect_SetNewMapPics();
	if (s_mapselect.listview.curvalue) {
		MapSelect_SetNewListNames();
	}

	MapSelect_UpdateInterface();
}






/*
=================
MapSelect_FilterChanged
=================
*/
static void MapSelect_FilterChanged(void)
{
	char mapname[MAPNAME_BUFFER];
	const char* info;
	qboolean found;
	int index;
	int i, j;

	if (s_mapselect.currentmap >= 0)
	{
		info = UI_GetArenaInfoByNumber(s_mapselect.index_maplist[s_mapselect.currentmap]);
		Q_strncpyz(mapname, Info_ValueForKey(info,"map"), MAPNAME_BUFFER);
	}
	else
		mapname[0]='\0';


	// handle muliple selections
	// try and keep as many as possible across filter changes
	// convert to arena index
	for (i = 0; i < s_mapselect.num_multisel; i++)
	{
		index = s_mapselect.multiselect[i];
		s_mapselect.multiselect[i] = s_mapselect.index_maplist[index];
	}

	MapSelect_LoadMaps(mapname, qfalse);
	MapSelect_SetNewMapPage();
	MapSelect_SetMapTypeIcons();

	// restore multiple selection to those maps that are still present
	// check arena index values
	while (i < s_mapselect.num_multisel)
	{
		found = qfalse;
		for (j = 0; j < s_mapselect.nummaps; j++)
		{
			if (s_mapselect.index_maplist[j] == s_mapselect.multiselect[i]) {
				found = qtrue;
				break;
			}
		}

		if (found) {
			s_mapselect.multiselect[i] = j;
			i++;
			continue;
		}

		// delete, then recheck this index again
		MapSelect_DeleteMultiAtPos(i);
	}
}



/*
=================
MapSelect_CommitSelection
=================
*/
static void MapSelect_CommitSelection(void)
{
	const char* info;
	int i;

	if (s_mapselect.multisel.curvalue)
	{
		if (s_mapselect.num_multisel ==0)
			return;

		// replace first map, then insert the rest
		StartServer_StoreMap(s_mapselect.index, s_mapselect.index_maplist[s_mapselect.multiselect[0]]);
		for (i = 1; i < s_mapselect.num_multisel; i ++)
		{
			StartServer_InsertMap(s_mapselect.index + i,s_mapselect.index_maplist[s_mapselect.multiselect[i]]);
		}
	}
	else
	{
		// overwrite existing map
		StartServer_StoreMap(s_mapselect.index, s_mapselect.index_maplist[s_mapselect.currentmap]);
	}
}


/*
=================
MapSelect_MenuEvent
=================
*/
static void MapSelect_MenuEvent( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}
	switch( ((menucommon_s*)ptr)->id ) {
	case ID_MAPSELECT_CANCEL:
		UI_PopMenu();
		break;

	case ID_MAPSELECT_OK:
		ms_allmaps = s_mapselect.allmaps.curvalue;
		ms_filter = s_mapselect.filter.curvalue;
		MapSelect_CommitSelection();
		UI_PopMenu();
		break;

	case ID_MAPSELECT_PREV:
		if (s_mapselect.page > 0)
		{
			s_mapselect.page--;
			MapSelect_SetNewMapPage();
		}
		break;

	case ID_MAPSELECT_NEXT:
		if (s_mapselect.page < s_mapselect.maxpages - 1)
		{
			s_mapselect.page++;
			MapSelect_SetNewMapPage();
		}
		break;

	case ID_MAPSELECT_FILTERMAPS:
	case ID_MAPSELECT_ALLMAPS:
		MapSelect_FilterChanged();
		break;

	case ID_MAPSELECT_MULTISEL:
		MapSelect_ToggleMultiSelect();
		MapSelect_UpdateInterface();
		break;

	case ID_MAPSELECT_LISTVIEW:
		trap_Cvar_SetValue("uie_map_list", s_mapselect.listview.curvalue);
		MapSelect_SetViewType();
		MapSelect_SetNewMapPage();
		MapSelect_UpdateInterface();
		break;

	case ID_MAPSELECT_MAPICONS:
		trap_Cvar_SetValue("uie_mapicons", s_mapselect.mapicons.curvalue);
		break;
	}
}




/*
=================
MapSelect_MapSelectEvent
=================
*/
static void MapSelect_MapSelectEvent( void* ptr, int event ) {
	int index;

	if( event != QM_ACTIVATED ) {
		return;
	}

	index = (s_mapselect.page * MAX_GRIDMAPSPERPAGE) + ((menucommon_s*) ptr)->id;
	if (s_mapselect.multisel.curvalue) {
		MapSelect_AddToMultiSelect(index);
	}
	else {
		s_mapselect.currentmap = index;
	}
	MapSelect_UpdateInterface();
}




/*
=================
MapSelect_DrawMapPic
=================
*/
static void MapSelect_DrawMapPic( void *self ) {
	menubitmap_s	*b;
	int				x;
	int				y;
	int				w;
	int				h;
	int				n;
	int				i;
	int 			multi;
	int id;
	int hasfocus;
	int secondline, offset;
	vec4_t tempcolor;
	float* color;
	vec4_t colormaps;

	colormaps[0] = 0.4;
	colormaps[1] = 0.4;
	colormaps[2] = 0.4;
	colormaps[3] = 1.0;

	b = (menubitmap_s *)self;

	if( b->focuspic && !b->focusshader ) {
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );
	}

	// draw focus/highlight
	if (!(b->generic.flags & QMF_INACTIVE)) {
		x = b->generic.left;
		y = b->generic.top;
		w = b->generic.right - b->generic.left;
		h =	b->generic.bottom - b->generic.top;
		hasfocus = ((b->generic.flags & QMF_PULSE) ||
		 (b->generic.flags & QMF_PULSEIFFOCUS && (Menu_ItemAtCursor( b->generic.parent ) == b)) );
		if (hasfocus)
		{
			if (b->focuscolor)
			{
			  tempcolor[0] = b->focuscolor[0];
			  tempcolor[1] = b->focuscolor[1];
			  tempcolor[2] = b->focuscolor[2];
			  color        = tempcolor;
			}
			else
			  color = pulsecolor;
			color[3] = 0.7+0.3*sin(uis.realtime/PULSE_DIVISOR);

			trap_R_SetColor( color );
			UI_DrawHandlePic( x, y, w, h, b->focusshader );
			trap_R_SetColor( NULL );
		}

		// check for multi-selection
		multi = -1;
		n = (s_mapselect.page * s_mapselect.maxMapsPerPage) + b->generic.id;
		if (s_mapselect.multisel.curvalue)
		{
			for (i = 0; i < s_mapselect.num_multisel; i++)
			{
				if (s_mapselect.multiselect[i] == n) {
					multi = i + 1;
					break;
				}
			}
		}
	}

	// draw image/text
	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	id = b->generic.id;
	StartServer_DrawMapPicture( x, y, w, h, &s_mapselect.mapinfo[id], colormaps);

	if( b->generic.flags & QMF_HIGHLIGHT) {
		UI_DrawNamedPic( x, y, w, h, MAPSELECT_MAPSELECTED);
	}

	x = b->generic.x + b->width/2;
	y = b->generic.y + b->height + 2;

	if (hasfocus)
	{
		tempcolor[0] = s_mapselect.maptext_color[id][0];
		tempcolor[1] = s_mapselect.maptext_color[id][1];
		tempcolor[2] = s_mapselect.maptext_color[id][2];
		color = tempcolor;
		color[3] = 0.7+0.3*sin(uis.realtime/PULSE_DIVISOR);
	}
	else
		color = s_mapselect.maptext_color[id];

	secondline = s_mapselect.mapsecondline[n];

	offset = LINE_HEIGHT;

	if (secondline) {
		offset += LINE_HEIGHT;
		UI_DrawString( x, y - LINE_HEIGHT, &s_mapselect.mapdrawname[n][secondline],
			UI_CENTER|UI_SMALLFONT, color);
	}
	UI_DrawString( x, y - offset, s_mapselect.mapdrawname[n], UI_CENTER|UI_SMALLFONT, color);

	UI_DrawString( x, y , s_mapselect.mapinfo[id].mapname, UI_CENTER|UI_SMALLFONT, color );


	// draw multi-select
	if (multi < 0)
		return;

	x = b->generic.x + b->width/2;
	y = b->generic.y + b->height/2 - GIANTCHAR_HEIGHT/2;

	tempcolor[0] = 1.0;
	tempcolor[1] = 1.0;
	tempcolor[2] = 1.0;
	tempcolor[3] = 0.8;
	color = tempcolor;

	if (hasfocus)
		color[3] = 0.7+0.3*sin(uis.realtime/PULSE_DIVISOR);

	UI_DrawString( x, y , va("%i", multi), UI_CENTER|UI_DROPSHADOW|UI_GIANTFONT, color );
}



/*
=================
MapSelect_Cache
=================
*/
void MapSelect_Cache( void )
{
	trap_R_RegisterShaderNoMip( MAPSELECT_HARROWS );
	trap_R_RegisterShaderNoMip( MAPSELECT_NEXT );
	trap_R_RegisterShaderNoMip( MAPSELECT_PREVIOUS );
	trap_R_RegisterShaderNoMip( MAPSELECT_CANCEL0 );
	trap_R_RegisterShaderNoMip( MAPSELECT_CANCEL1 );
	trap_R_RegisterShaderNoMip( MAPSELECT_ACCEPT0 );
	trap_R_RegisterShaderNoMip( MAPSELECT_ACCEPT1 );
	trap_R_RegisterShaderNoMip( MAPSELECT_ERRORPIC );
	trap_R_RegisterShaderNoMip( MAPSELECT_MAPFOCUS );

	trap_R_RegisterShaderNoMip( MAPSELECT_MAPSELECTED );
}





/*
=================
MapSelect_ScrollCharParams
=================
*/
static void MapSelect_ScrollCharParams( int* height, int* width, int* line)
{
	*height = SMALLCHAR_HEIGHT;
	*width = SMALLCHAR_WIDTH;
	*line = SMALLCHAR_HEIGHT + 2;
}



/*
=================
MapSelect_ScrollListDraw
=================
*/
static void MapSelect_ScrollListDraw( void* ptr )
{
	int			x;
	int			u;
	int			y;
	int			i, j;
	int			base;
	int			column;
	float*		color;
	qboolean	hasfocus;
	int			style;
	menulist_s *l;
	float	scale;
	int 	charwidth;
	int 	charheight;
	int 	lineheight;
	int 	index;
	int		map;
	int 	offset;

	l = (menulist_s*)ptr;
	hasfocus = (l->generic.parent->cursor == l->generic.menuPosition);
	MapSelect_ScrollCharParams( &charheight, &charwidth, &lineheight);

	x =	l->generic.x;
	for( column = 0; column < l->columns; column++ ) {
		y =	l->generic.y;
		base = l->top + column * l->height;
		for( i = base; i < base + l->height; i++) {
			if (i >= l->numitems)
				break;

			style = UI_SMALLFONT;
			color = s_mapselect.maptext_color[i];
			if (i == l->curvalue)
			{
				u = x - 2;
				if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
					u -= (l->width * charwidth) / 2 + 1;
				}

				UI_FillRect(u,y,l->width*charwidth ,lineheight,color_select_bluo);
				if (color != color_white)
					color = color_highlight;

				if (hasfocus)
					style |= (UI_PULSE|UI_LEFT);
				else
					style |= UI_LEFT;
			}
			else
			{
				style |= UI_LEFT|UI_INVERSE;
			}

			if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
				style |= UI_CENTER;
			}

			index = 0;
			map = i + s_mapselect.page * s_mapselect.maxMapsPerPage;
			if (s_mapselect.multisel.curvalue) {
				index = MapSelect_MultiListPos(map) + 1;
			}

			if (index > 0) {
				offset = 0;
				if (index < 10)
					offset += charwidth;

				UI_DrawString(x + offset, y + (lineheight - charheight)/2,
					va("%i", index), style, color_white);
			}

			UI_DrawString(
				x + 3*charwidth,
				y + (lineheight - charheight)/2,
				l->itemnames[i],
				style,
				color);

			y += lineheight;
		}
		x += (l->width + l->seperation) * charwidth;
	}
}



/*
=================
MapSelect_ListIndexFromCursor
=================
*/
static qboolean MapSelect_ListIndexFromCursor(int* pos)
{
	menulist_s* l;
	int	x;
	int	y;
	int	w;
	int	cursorx;
	int	cursory;
	int	column;
	int 	charwidth;
	int 	charheight;
	int 	lineheight;

	l = &s_mapselect.maplist;
	MapSelect_ScrollCharParams( &charheight, &charwidth, &lineheight);

    *pos = -1;

	// check scroll region
	x = l->generic.x;
	y = l->generic.y;
	w = ( (l->width + l->seperation) * l->columns - l->seperation) * charwidth;
	if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
		x -= w / 2;
	}
	if (!UI_CursorInRect( x, y, w, l->height*lineheight - 1))
		return qfalse;

	cursorx = (uis.cursorx - x)/charwidth;
	column = cursorx / (l->width + l->seperation);
	cursory = (uis.cursory - y)/lineheight;
	*pos = column * l->height + cursory;

	return qtrue;
}



/*
=================
MapSelect_DrawListMapPic

Draws the picture under cursor in the map selection listbox
=================
*/
static void MapSelect_DrawListMapPic(void)
{
	static int oldindex = 0;
	static int maptime = 0;

	int x,y;
	int colw, colh;
	int index;
	int base;
	int delta;
	vec4_t colour;

	colour[0] = 1.0;
	colour[1] = 1.0;
	colour[2] = 1.0;
	colour[3] = 1.0;

	base = s_mapselect.page * s_mapselect.maxMapsPerPage;

	// cursor is outside list, fade map
	if (MapSelect_ListIndexFromCursor(&index) && MapSelect_OnCurrentPage(base + index)) {
		maptime = uis.realtime;
		oldindex = index;
	}
	else {
		index = oldindex;
		delta = uis.realtime - maptime;
		if (delta >= MAP_FADETIME)
			return;

		colour[3] = 1.0 - (float)(delta) / MAP_FADETIME;
	}
	fading_red[3] = colour[3];

	x = 320 - MAPPIC_WIDTH/2;
	y = s_mapselect.bottomrow_y;

    MapSelect_MapCellSize(&colh, &colw);

	trap_R_SetColor( fading_red );
	UI_DrawNamedPic(x + uis.wideoffset - ( colw - MAPPIC_WIDTH )/2, y - 8, colw, colh, MAPSELECT_SELECT);
	trap_R_SetColor( NULL );

	StartServer_DrawMapPicture( x + uis.wideoffset, y,
		MAPPIC_WIDTH, MAPPIC_HEIGHT, &s_mapselect.mapinfo[index], colour);

	UI_DrawString( 320 + uis.wideoffset, y + MAPPIC_HEIGHT + 8, s_mapselect.maplongname[base + index],
		UI_CENTER|UI_SMALLFONT, colour);
}




/*
=================
MapSelect_MenuDraw
=================
*/
static void MapSelect_MenuDraw(void)
{
	StartServer_BackgroundDraw(qfalse);

	// draw the controls
	Menu_Draw(&s_mapselect.menu);

	if (s_mapselect.nomaps)
	{
		if(cl_language.integer == 0){
		UI_DrawString(320, 240 - 32, "NO MAPS FOUND", UI_CENTER, color_nomap);
		}
		if(cl_language.integer == 1){
		UI_DrawString(320, 240 - 32, "КАРТЫ НЕ НАЙДЕНЫ", UI_CENTER, color_nomap);
		}
		return;
	}

	if (s_mapselect.listview.curvalue) {
		MapSelect_DrawListMapPic();
	}
}






/*
=================
MapSelect_HandleListKey

Returns true if the list box accepts that key input
=================
*/
static qboolean MapSelect_HandleListKey( int key, sfxHandle_t* psfx)
{
	menulist_s* l;
	int	index;
	int	sel;

	l = &s_mapselect.maplist;

	switch (key) {
	case K_MOUSE1:
		if (l->generic.flags & QMF_HASMOUSEFOCUS)
		{
			// absorbed, silent sound effect
			*psfx = (menu_null_sound);
			if (!MapSelect_ListIndexFromCursor(&index))
				return qtrue;

			if (l->top + index < l->numitems)
			{
				l->oldvalue = l->curvalue;
				l->curvalue = l->top + index;

				if (l->oldvalue != l->curvalue && l->generic.callback)
				{
					l->generic.callback( l, QM_GOTFOCUS );
				}
				sel = s_mapselect.page * s_mapselect.maxMapsPerPage;
				sel += l->curvalue;
				s_mapselect.currentmap = sel;
				if (s_mapselect.multisel.curvalue)
					MapSelect_AddToMultiSelect(sel);

				MapSelect_UpdateAcceptInterface();
				*psfx = (menu_move_sound);
			}
		}
		return qtrue;
	// keys that have the default action
	case K_ESCAPE:
		*psfx = Menu_DefaultKey( &s_mapselect.menu, key );
		return qtrue;
	}
	return qfalse;
}




/*
=================
MapSelect_Key
=================
*/
static sfxHandle_t MapSelect_Key( int key )
{
	menulist_s	*l;
	sfxHandle_t sfx;

	l = (menulist_s	*)Menu_ItemAtCursor( &s_mapselect.menu );

	sfx = menu_null_sound;
	if( l == &s_mapselect.maplist) {
		if (!MapSelect_HandleListKey(key, &sfx))
			return menu_buzz_sound;
	}
	else {
		sfx = Menu_DefaultKey( &s_mapselect.menu, key );
	}

	return sfx;
}




/*
=================
MapSelect_ScrollList_Init
=================
*/
static void MapSelect_ScrollListInit( menulist_s *l )
{
	int		w;
	int 	charwidth;
	int 	charheight;
	int 	lineheight;

	l->oldvalue = 0;
	l->curvalue = 0;
	l->top      = 0;

	MapSelect_ScrollCharParams(&charheight, &charwidth, &lineheight);

	if( !l->columns ) {
		l->columns = 1;
		l->seperation = 0;
	}
	else if( !l->seperation ) {
		l->seperation = 3;
	}

	w = ( (l->width + l->seperation) * l->columns - l->seperation) * charwidth;

	l->generic.left   =	l->generic.x;
	l->generic.top    = l->generic.y;
	l->generic.right  =	l->generic.x + w;
	l->generic.bottom =	l->generic.y + l->height * lineheight;

	if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
		l->generic.left -= w / 2;
		l->generic.right -= w / 2;
	}
}




/*
=================
MapSelect_MenuInit
=================
*/
static void MapSelect_MenuInit(int gametype, int index, const char* mapname)
{
	int i, j, x, y, top;
	int colw, colh;
	int lastpage;

	lastpage = -1;
	if (ms_lastgametype == gametype)
		lastpage = s_mapselect.page;
	else
		ms_lastgametype = gametype;

	memset(&s_mapselect,0,sizeof(s_mapselect));
	s_mapselect.gametype = gametype;
	s_mapselect.menu.key = MapSelect_Key;

	MapSelect_Cache();

	if (gametype != GT_CTF && MapSelect_MapSupportsGametype(mapname)) {
		s_mapselect.allmaps.curvalue = ms_allmaps;
	}
	else {
		s_mapselect.allmaps.curvalue = 0;
	}

	// change map filter if needed
	if (mapname && mapname[0])
	{
		if (ms_filter < MAPFILTER_MAX)
		{
			if (StartServer_IsIdMap(mapname))
			{
				if (ms_filter == MAPFILTER_NONID)
					ms_filter = MAPFILTER_OFF;
			}
			else
			{
				if (ms_filter == MAPFILTER_ID)
					ms_filter = MAPFILTER_OFF;
			}
		}
		else if (!StartServer_IsCustomMapType(mapname, ms_filter - MAPFILTER_MAX))
		{
			ms_filter = MAPFILTER_OFF;
		}
	}

	s_mapselect.filter.curvalue = ms_filter;

	// remember previous map page
	if (s_mapselect.currentmap == -1 && lastpage != -1)
		s_mapselect.page = lastpage;

	s_mapselect.index = index;

	s_mapselect.menu.wrapAround = qtrue;
	s_mapselect.menu.native 	= qfalse;
	s_mapselect.menu.fullscreen = qtrue;
	s_mapselect.menu.draw = MapSelect_MenuDraw;

	s_mapselect.banner.generic.type  = MTYPE_BTEXT;
	s_mapselect.banner.generic.x	   = 160;
	s_mapselect.banner.generic.y	   = 4;
	s_mapselect.banner.color         = color_white;
	s_mapselect.banner.style         = UI_CENTER | UI_GIANTFONT;

	s_mapselect.maptype.generic.type  = MTYPE_TEXT;
	s_mapselect.maptype.generic.x	   = 160;
	s_mapselect.maptype.generic.y	   = 4 + 36;
	s_mapselect.maptype.color         = color_white;
	s_mapselect.maptype.style         = UI_CENTER;

	s_mapselect.icona.generic.type  = MTYPE_BITMAP;
	s_mapselect.icona.generic.flags = QMF_INACTIVE;
//	s_mapselect.icona.generic.name = "menu/medals/medal_excellent";
	s_mapselect.icona.generic.x	 = 420  - 32 - SMALLCHAR_WIDTH;
	s_mapselect.icona.generic.y	 = 8 + (SMALLCHAR_HEIGHT - 32)/2;
	s_mapselect.icona.width  	     = 32;
	s_mapselect.icona.height  	     = 32;

	s_mapselect.iconb.generic.type  = MTYPE_BITMAP;
	s_mapselect.iconb.generic.flags = QMF_INACTIVE;
//	s_mapselect.iconb.generic.name = "menu/medals/medal_victory";
	s_mapselect.iconb.generic.x	 = 420 - 64 - SMALLCHAR_WIDTH;
	s_mapselect.iconb.generic.y	 = 8 + (SMALLCHAR_HEIGHT - 32)/2;
	s_mapselect.iconb.width  	     = 32;
	s_mapselect.iconb.height  	     = 32;

	s_mapselect.filter.generic.type = MTYPE_SPINCONTROL;
	s_mapselect.filter.generic.x = 420 + 8*SMALLCHAR_WIDTH;
	s_mapselect.filter.generic.y = 8;
	s_mapselect.filter.generic.id  = ID_MAPSELECT_FILTERMAPS;
	s_mapselect.filter.generic.callback  = MapSelect_MenuEvent;

	s_mapselect.allmaps.generic.type = MTYPE_RADIOBUTTON;
	s_mapselect.allmaps.generic.x = 480 - 8*SMALLCHAR_WIDTH;
	s_mapselect.allmaps.generic.y = 8 + LINE_HEIGHT + 8;
	s_mapselect.allmaps.generic.id  = ID_MAPSELECT_ALLMAPS;
	s_mapselect.allmaps.generic.callback  = MapSelect_MenuEvent;

	s_mapselect.mapicons.generic.type = MTYPE_SPINCONTROL;
	s_mapselect.mapicons.generic.x = 480 + 8*SMALLCHAR_WIDTH;
	s_mapselect.mapicons.generic.y = 8 + LINE_HEIGHT + 8;
	s_mapselect.mapicons.generic.id  = ID_MAPSELECT_MAPICONS;
	s_mapselect.mapicons.generic.callback  = MapSelect_MenuEvent;
	s_mapselect.mapicons.curvalue = (int)Com_Clamp(0, MAPICONS_MAX - 1, uie_mapicons.integer);

	s_mapselect.arrows.generic.type  = MTYPE_BITMAP;
	s_mapselect.arrows.generic.name  = MAPSELECT_HARROWS;
	s_mapselect.arrows.generic.flags = QMF_INACTIVE;
	s_mapselect.arrows.generic.x	 = 320;
	s_mapselect.arrows.generic.y	 = 480 - 64;
	s_mapselect.arrows.width  	     = 192;
	s_mapselect.arrows.height  	     = 64;

	s_mapselect.previous.generic.type  = MTYPE_BITMAP;
	s_mapselect.previous.generic.flags  = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapselect.previous.generic.id  = ID_MAPSELECT_PREV;
	s_mapselect.previous.generic.callback  = MapSelect_MenuEvent;
	s_mapselect.previous.generic.x	 = 320;
	s_mapselect.previous.generic.y	 = 480 - 64;
	s_mapselect.previous.width  	     = 96;
	s_mapselect.previous.height  	     = 64;
	s_mapselect.previous.focuspic = MAPSELECT_PREVIOUS;

	s_mapselect.next.generic.type  = MTYPE_BITMAP;
	s_mapselect.next.generic.flags  = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapselect.next.generic.id  = ID_MAPSELECT_NEXT;
	s_mapselect.next.generic.callback  = MapSelect_MenuEvent;
	s_mapselect.next.generic.x	 = 320 + 96 ;
	s_mapselect.next.generic.y	 = 480 - 64;
	s_mapselect.next.width  	     = 96;
	s_mapselect.next.height  	     = 64;
	s_mapselect.next.focuspic = MAPSELECT_NEXT;

	s_mapselect.cancel.generic.type  = MTYPE_BITMAP;
	s_mapselect.cancel.generic.name  = MAPSELECT_CANCEL0;
	s_mapselect.cancel.generic.flags  = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapselect.cancel.generic.id  = ID_MAPSELECT_CANCEL;
	s_mapselect.cancel.generic.callback  = MapSelect_MenuEvent;
	s_mapselect.cancel.generic.x	   = 0 - uis.wideoffset;
	s_mapselect.cancel.generic.y	   = 480 - 64;
	s_mapselect.cancel.width  	   = 128;
	s_mapselect.cancel.height  	   = 64;
	s_mapselect.cancel.focuspic     = MAPSELECT_CANCEL1;

	s_mapselect.accept.generic.type  = MTYPE_BITMAP;
	s_mapselect.accept.generic.name  = MAPSELECT_ACCEPT0;
	s_mapselect.accept.generic.flags  = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapselect.accept.generic.id = ID_MAPSELECT_OK;
	s_mapselect.accept.generic.callback  = MapSelect_MenuEvent;
	s_mapselect.accept.generic.x	   = 640 - 128 + uis.wideoffset;
	s_mapselect.accept.generic.y	   = 480 - 64;
	s_mapselect.accept.width  	   = 128;
	s_mapselect.accept.height  	   = 64;
	s_mapselect.accept.focuspic     = MAPSELECT_ACCEPT1;

	s_mapselect.maplist.generic.type = MTYPE_SCROLLLIST;
	s_mapselect.maplist.generic.flags = QMF_PULSEIFFOCUS|QMF_NODEFAULTINIT;
	s_mapselect.maplist.generic.x = 5 * SMALLCHAR_WIDTH;
	s_mapselect.maplist.generic.y = 64;
	s_mapselect.maplist.generic.ownerdraw = MapSelect_ScrollListDraw;
	s_mapselect.maplist.columns = MAPLIST_COLUMNS;
	s_mapselect.maplist.seperation = 2;
	s_mapselect.maplist.height = MAPLIST_ROWS;
	s_mapselect.maplist.width = 22;
    MapSelect_ScrollListInit(&s_mapselect.maplist);


	top = s_mapselect.page * s_mapselect.maxMapsPerPage;

	MapSelect_MapCellSize(&colh, &colw);

	s_mapselect.bottomrow_y = 64 + 2*colh;

	for ( i = 0; i < MAX_GRIDMAPSPERPAGE; i++)
	{

		x = colw * (i % MAPGRID_COLUMNS) + (colw - MAPPIC_WIDTH)/2;
		y = 64 + (i / MAPGRID_COLUMNS) * colh; // offset by one button

		s_mapselect.mappics[i].generic.type  = MTYPE_BITMAP;
		s_mapselect.mappics[i].generic.name  = 0;
		s_mapselect.mappics[i].generic.flags  = QMF_NODEFAULTINIT;
		s_mapselect.mappics[i].generic.ownerdraw = MapSelect_DrawMapPic;
		s_mapselect.mappics[i].generic.callback = MapSelect_MapSelectEvent;
		s_mapselect.mappics[i].generic.id	   = i;
		s_mapselect.mappics[i].generic.x	   = x;
		s_mapselect.mappics[i].generic.y	   = y;
		s_mapselect.mappics[i].width  	   = MAPPIC_WIDTH;
		s_mapselect.mappics[i].height  	   = MAPPIC_HEIGHT;
		s_mapselect.mappics[i].focuspic     = MAPSELECT_MAPSELECTED;
		s_mapselect.mappics[i].errorpic     = MAPSELECT_ERRORPIC;
		s_mapselect.mappics[i].generic.left	   = x - ( colw - MAPPIC_WIDTH )/2;
		s_mapselect.mappics[i].generic.top	   = y - 8;
		s_mapselect.mappics[i].generic.right	= x + ( colw + MAPPIC_WIDTH )/2;
		s_mapselect.mappics[i].generic.bottom	= y + colh - 8;
		s_mapselect.mappics[i].focuspic  = MAPSELECT_MAPFOCUS;
	}

	s_mapselect.multisel.generic.type = MTYPE_RADIOBUTTON;
	s_mapselect.multisel.generic.x = 128 + 15*SMALLCHAR_WIDTH;
	s_mapselect.multisel.generic.y = 480 - 3*SMALLCHAR_HEIGHT;
	s_mapselect.multisel.generic.id  = ID_MAPSELECT_MULTISEL;
	s_mapselect.multisel.generic.callback  = MapSelect_MenuEvent;

	s_mapselect.listview.generic.type = MTYPE_RADIOBUTTON;
	s_mapselect.listview.generic.x = 128 + 15*SMALLCHAR_WIDTH;
	s_mapselect.listview.generic.y = 480 - 2*SMALLCHAR_HEIGHT;
	s_mapselect.listview.generic.id  = ID_MAPSELECT_LISTVIEW;
	s_mapselect.listview.generic.callback  = MapSelect_MenuEvent;

	s_mapselect.multisel.curvalue = (int)Com_Clamp(0,1, trap_Cvar_VariableValue("uie_map_multisel"));
	s_mapselect.listview.curvalue = (int)Com_Clamp(0,1, trap_Cvar_VariableValue("uie_map_list"));

#ifndef NO_UIE_MINILOGO_SKIRMISH
	s_mapselect.logo.generic.type			= MTYPE_BITMAP;
	s_mapselect.logo.generic.flags		= QMF_INACTIVE|QMF_HIGHLIGHT;
	s_mapselect.logo.generic.x			= UIE_LOGO_X;
	s_mapselect.logo.generic.y			= UIE_LOGO_Y;
	s_mapselect.logo.width				= 64;
	s_mapselect.logo.height				= 16;
	s_mapselect.logo.focuspic 			= UIE_LOGO_NAME;
	s_mapselect.logo.focuscolor 			= color_translucent;
#endif

	if(cl_language.integer == 0){
	s_mapselect.banner.string        = "MAP SELECT";
	s_mapselect.filter.generic.name = "Filter:";
	s_mapselect.filter.itemnames  = mapfilter_items;
	s_mapselect.allmaps.generic.name = "All maps:";
	s_mapselect.mapicons.generic.name = "Icons:";
	s_mapselect.maplist.itemnames = s_mapselect.maplist_alias;
	s_mapselect.multisel.generic.name = "Multi-select:";
	s_mapselect.listview.generic.name = "List view:";
	s_mapselect.mapicons.itemnames  = mapicons_items;
	s_mapselect.maptype.string        = (char*)gametype_items[gametype];
	}
	if(cl_language.integer == 1){
	s_mapselect.banner.string        = "ВЫБОР КАРТЫ";
	s_mapselect.filter.generic.name = "Фильтр:";
	s_mapselect.filter.itemnames  = mapfilter_itemsru;
	s_mapselect.allmaps.generic.name = "Все карты:";
	s_mapselect.mapicons.generic.name = "Значки:";
	s_mapselect.maplist.itemnames = s_mapselect.maplist_alias;
	s_mapselect.multisel.generic.name = "Мульти-выбор:";
	s_mapselect.listview.generic.name = "Список:";
	s_mapselect.mapicons.itemnames  = mapicons_itemsru;
	s_mapselect.maptype.string        = (char*)gametype_itemsru[gametype];
	}

	// register for display
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.banner);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.arrows);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.previous);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.next);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.cancel);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.accept);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.maptype);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.filter);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.mapicons);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.icona);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.iconb);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.multisel);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.listview);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.maplist);
	Menu_AddItem( &s_mapselect.menu, &s_mapselect.allmaps);

	for (i = 0 ; i < MAX_GRIDMAPSPERPAGE; i++)
	{
		Menu_AddItem( &s_mapselect.menu, &s_mapselect.mappics[i]);
	}

	MapSelect_LoadMaps(mapname, qfalse);

	MapSelect_ClearMultiSelect();
	MapSelect_AddToMultiSelect(s_mapselect.currentmap);

	MapSelect_SetNewMapPage();
	MapSelect_SetMapTypeIcons();

#ifndef NO_UIE_MINILOGO_SKIRMISH
	if (random() < 0.1)
		Menu_AddItem( &s_mapselect.menu, &s_mapselect.logo);
#endif

	UI_PushMenu( &s_mapselect.menu );
}




/*
=================
UI_StartMapMenu
=================
*/
void UI_StartMapMenu( int gametype, int index, const char* mapname)
{
	MapSelect_MenuInit( gametype, index, mapname);
}








