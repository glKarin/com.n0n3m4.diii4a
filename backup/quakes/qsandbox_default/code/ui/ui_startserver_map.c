/*
=============================================================================

START SERVER MAP SELECT MENU *****

=============================================================================
*/


#include "ui_local.h"
#include "ui_startserver_q3.h"



#define ID_MAP_GAMETYPE 200
#define ID_MAP_TIMELIMIT 201
#define ID_MAP_FRAGLIMIT 202
#define ID_MAP_MAPSOURCE 203
#define ID_MAP_UP 204
#define ID_MAP_DOWN 205
#define ID_MAP_DEL 206
#define ID_MAP_SHORTNAME 207
#define ID_MAP_MAPREPEAT 208
#define ID_MAP_SOURCECOUNT 209
#define ID_MAP_FRAGCOUNT 210
#define ID_MAP_TIMECOUNT 211
#define ID_MAP_PAGENUM 212
#define ID_MAP_CAPTURELIMIT 213
#define ID_MAP_CAPTURECOUNT 214
#define ID_MAP_ACTION 215
#define ID_MAP_SOURCETYPE 216
#define ID_MAP_ACTIONTYPE 217

// screen positions
#define MAPCOLUMN_LEFTX 	COLUMN_LEFT - uis.wideoffset
#define MAPCOLUMN_RIGHTX 	COLUMN_RIGHT + uis.wideoffset
#define MAPBUTTONS_X 		(SMALLCHAR_WIDTH ) - uis.wideoffset
#define MAPARRAYCOLUMN_X 	(SMALLCHAR_WIDTH * 13) - uis.wideoffset
#define MAPLONGNAME_DX 		(SMALLCHAR_WIDTH * (SHORTMAP_BUFFER + 1))
#define MAPFRAGS_DX 		(MAPLONGNAME_DX + SMALLCHAR_WIDTH * (LONGMAP_BUFFER + 4))
#define MAPTIME_DX 			(MAPFRAGS_DX + SMALLCHAR_WIDTH * 8)
#define MAPACTIVATE_X 		392

#define GAMESERVER_ARROWWIDTH 64
#define GAMESERVER_ARROWHEIGHT 64

#define MAP_NUMBER_FIELD 3

#define ACTIONTYPE_DELETE "Delete all"
#define ACTIONTYPE_COPY " Copy from"
#define ACTIONTYPE_DELETERU "Удалить все"
#define ACTIONTYPE_COPYRU " Скопировать из"



//
// controls 
//

typedef struct mapcontrols_s {
	menuframework_s menu;
	commoncontrols_t common;

	menulist_s gameType;
	menubitmap_s gameTypeIcon;

	menubitmap_s displaySelected[NUMMAPS_PERPAGE];
	menutext_s displayMapName[NUMMAPS_PERPAGE];

	menufield_s displayFragLimit[NUMMAPS_PERPAGE];
	menufield_s displayTimeLimit[NUMMAPS_PERPAGE];

	menulist_s timeLimitType;
	menulist_s fragLimitType;
	menufield_s timeLimit;
	menufield_s fragLimit;
	menulist_s mapSource;
	menufield_s mapSourceCount;
	menulist_s mapSourceType;
	menuradiobutton_s mapRepeat;
	menutext_s mapPage;

	menutext_s mapText;
	menutext_s nameText;
	menutext_s fragsText;
	menutext_s timeText;

	menubitmap_s arrows;
	menubitmap_s up;
	menubitmap_s down;
	menubitmap_s del;

	menulist_s actionSrc;
	menulist_s actionDest;
	menubitmap_s actionActivate;

#ifndef NO_UIE_MINILOGO_SKIRMISH
	menubitmap_s logo;
#endif

	// local data implementing interface
	int statusbar_height;
	char statusbar_text[MAX_STATUSBAR_TEXT];
	mappic_t mappic;
	int fadetime;

	int map_page;
	int map_selected;	// -1 = none, or index to current page

	char mappage_text[MAPPAGE_TEXT];
} mapcontrols_t;


static mapcontrols_t s_mapcontrols;


//
// static data used by controls
//


static const char *map_comsprintf[] = {
	"%i: %s",
	" %i: %s"
};



static const char* fragcontrol_text[] = {
	"Frag Limit:", "Frags",
	"Capture Limit:", "Caps"
};


static const char* timelimittype_items[MAP_LT_COUNT + 1] = {
	"Default",	// MAP_LT_DEFAULT
	"None",	// MAP_LT_NONE
	"Custom",	// MAP_LT_CUSTOM
	0
};


static const char* fraglimittype_items[MAP_LT_COUNT + 1] = {
	"Default",	// MAP_LT_DEFAULT
	"None",	// MAP_LT_NONE
	"Custom",	// MAP_LT_CUSTOM
	0
};



static const char* mapSource_items[MAP_MS_MAX + 1]={
	"List, in order",	// MAP_MS_ORDER
	"List, random",	// MAP_MS_RANDOMLIST
	"Randomly chosen",	// MAP_MS_RANDOM
	"Random, not list",	// MAP_MS_RANDOMEXCLUDE
	0
};


static const char* copyFrom_items[MAP_CF_COUNT + 1] = {
	"arena frag limit",	// MAP_CF_ARENASCRIPT
	"time limit",	// MAP_CF_TIME
	"frag limit",	// MAP_CF_FRAG
	"frag and time limit",	// MAP_CF_BOTH
	"maps from list",	// MAP_CF_CLEARALL
	"maps on this page",	// MAP_CF_CLEARPAGE
	0
};


static const char* copyFrom_ctf_items[MAP_CF_COUNT + 1] = {
	"<<never shown>>",	// MAP_CF_ARENASCRIPT
	"time limit",	// MAP_CF_TIME
	"caps limit",	// MAP_CF_FRAG
	"frag and caps limits",	// MAP_CF_BOTH
	"maps from list",	// MAP_CF_CLEARALL
	"maps on this page",	// MAP_CF_CLEARPAGE
	0
};



static const char* copyTo_items[MAP_CT_COUNT + 1] = {
	"selected map",	// MAP_CT_SELECTED
	"all maps on page",	// MAP_CT_PAGE
	"all maps",	// MAP_CT_ALL
	0
};


static const char* fragcontrol_textru[] = {
	"Лимит Фрагов:", "Фраги",
	"Лимит Захватов:", "Захваты"
};


static const char* timelimittype_itemsru[MAP_LT_COUNT + 1] = {
	"Стандарт",	// MAP_LT_DEFAULT
	"Нет",	// MAP_LT_NONE
	"Настр",	// MAP_LT_CUSTOM
	0
};


static const char* fraglimittype_itemsru[MAP_LT_COUNT + 1] = {
	"Стандарт",	// MAP_LT_DEFAULT
	"Нет",	// MAP_LT_NONE
	"Настр",	// MAP_LT_CUSTOM
	0
};



static const char* mapSource_itemsru[MAP_MS_MAX + 1]={
	"Список, по порядку",	// MAP_MS_ORDER
	"Список, рандом",	// MAP_MS_RANDOMLIST
	"Рандом",	// MAP_MS_RANDOM
	"Рандом, не из списка",	// MAP_MS_RANDOMEXCLUDE
	0
};


static const char* copyFrom_itemsru[MAP_CF_COUNT + 1] = {
	"лимит фрагов из скрипта",	// MAP_CF_ARENASCRIPT
	"лимит времени",	// MAP_CF_TIME
	"лимит фрагов",	// MAP_CF_FRAG
	"лимит времени и фрагов",	// MAP_CF_BOTH
	"карты из списка",	// MAP_CF_CLEARALL
	"карты на этой странице",	// MAP_CF_CLEARPAGE
	0
};


static const char* copyFrom_ctf_itemsru[MAP_CF_COUNT + 1] = {
	"<<не показывать>>",	// MAP_CF_ARENASCRIPT
	"лимит времени",	// MAP_CF_TIME
	"лимит захватов",	// MAP_CF_FRAG
	"лимит времени и захватов",	// MAP_CF_BOTH
	"карты из списка",	// MAP_CF_CLEARALL
	"карты на этой странице",	// MAP_CF_CLEARPAGE
	0
};



static const char* copyTo_itemsru[MAP_CT_COUNT + 1] = {
	"выбранная карта",	// MAP_CT_SELECTED
	"все карты на странице",	// MAP_CT_PAGE
	"все карты",	// MAP_CT_ALL
	0
};

/*
=================
StartServer_MapPage_SetPageText
=================
*/
static void StartServer_MapPage_SetPageText(void)
{
	int pagecount;
	int pagenum;
	char* s;
if(cl_language.integer == 0){
	s = "[Page %i of %i]";
}
if(cl_language.integer == 1){
	s = "[Страница %i из %i]";
}
	pagecount = StartServer_MapPageCount();

	if (s_mapcontrols.map_page > pagecount - 1)
		s_mapcontrols.map_page = pagecount - 1;

	if (pagecount == 1) {
		s_mapcontrols.mapPage.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		pagenum = 1;
	}
	else
	{
		// page text is all the same length (all digits less than 10)
		// so leave generic.left etc. alone
		s_mapcontrols.mapPage.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
		pagenum = s_mapcontrols.map_page + 1;
	}

	Q_strncpyz(s_mapcontrols.mappage_text, va(s, pagenum, pagecount), MAPPAGE_TEXT);
}




/*
=================
StartServer_MapPage_CopyCustomLimitsToControls
=================
*/
void StartServer_MapPage_CopyCustomLimitsToControls(void)
{
	int i, base;

	base = s_mapcontrols.map_page * NUMMAPS_PERPAGE;
	for (i = 0; i < NUMMAPS_PERPAGE; i++)
	{
		memcpy(s_mapcontrols.displayFragLimit[i].field.buffer,
			s_scriptdata.map.data[base + i].fragLimit, MAX_LIMIT_BUF);
	}
	for (i = 0; i < NUMMAPS_PERPAGE; i++)
	{
		memcpy(s_mapcontrols.displayTimeLimit[i].field.buffer,
			s_scriptdata.map.data[base + i].timeLimit, MAX_LIMIT_BUF);
	}
}




/*
=================
StartServer_MapPage_UpdateActionControls
=================
*/
static void StartServer_MapPage_UpdateActionControls( void )
{
	int curvalue;
	qboolean del;

	curvalue = s_mapcontrols.actionSrc.curvalue;

	// prevent TEAM games from seeing arena script setting
	if (s_scriptdata.gametype >= GT_TEAM && curvalue == MAP_CF_ARENASCRIPT) {
		curvalue = MAP_CF_TIME;
		s_mapcontrols.actionSrc.curvalue = curvalue;
	}

	if (curvalue == MAP_CF_CLEARALL || curvalue == MAP_CF_CLEARPAGE)
		del = qtrue;
	else
		del = qfalse;

	// prevent irrelevant options appearing in random map generation
	if (StartServer_IsRandomGeneratedMapList(s_scriptdata.map.listSource) && !del)
	{
		curvalue = MAP_CF_CLEARALL;
		s_mapcontrols.actionSrc.curvalue = curvalue;
		del = qtrue;
	}

if(cl_language.integer == 0){
	if (del)
	{
		s_mapcontrols.actionSrc.generic.name = ACTIONTYPE_DELETE;
		s_mapcontrols.actionDest.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else
	{
		s_mapcontrols.actionSrc.generic.name = ACTIONTYPE_COPY;
		s_mapcontrols.actionDest.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
	}
}
if(cl_language.integer == 1){
	if (del)
	{
		s_mapcontrols.actionSrc.generic.name = ACTIONTYPE_DELETERU;
		s_mapcontrols.actionDest.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else
	{
		s_mapcontrols.actionSrc.generic.name = ACTIONTYPE_COPYRU;
		s_mapcontrols.actionDest.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
	}
}
}




/*
=================
StartServer_MapPage_SetSelectedControl
=================
*/
static void StartServer_MapPage_SetSelectedControl( int index )
{
	if (s_mapcontrols.map_selected != -1) {
		s_mapcontrols.displaySelected[ s_mapcontrols.map_selected ].generic.flags &= ~(QMF_HIGHLIGHT);
	}

	if (index == -1 || index == s_mapcontrols.map_selected ) {
		// none selected
		s_mapcontrols.up.generic.flags |= (QMF_INACTIVE|QMF_GRAYED);
		s_mapcontrols.down.generic.flags |= (QMF_INACTIVE|QMF_GRAYED);
		s_mapcontrols.arrows.generic.flags |= QMF_GRAYED;
		s_mapcontrols.del.generic.flags |= (QMF_INACTIVE|QMF_GRAYED);
		index = -1;
	} else {
		s_mapcontrols.displaySelected[ index ].generic.flags |= QMF_HIGHLIGHT;
		s_mapcontrols.up.generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		s_mapcontrols.down.generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		s_mapcontrols.arrows.generic.flags &= ~QMF_GRAYED;
		s_mapcontrols.del.generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
	}

	s_mapcontrols.map_selected = index;
}




/*
=================
StartServer_MapPage_UpdateInterface
=================
*/
static void StartServer_MapPage_UpdateInterface(void)
{
	int i, value, mapcount;
	int adjust, mappages;
	int remap;
	qboolean hide;

	// handle appearance/disappearance of controls
	// based on page values

	//
	// mapPage
	//

	// update icon
	StartServer_SetIconFromGameType(&s_mapcontrols.gameTypeIcon, s_scriptdata.gametype, qfalse);

	// updates map_page to accurate value
	StartServer_MapPage_SetPageText();


	// offset by one so we display an empty map field if needed
	// an empty field can have no custom time and frag fields, so
	// we'll then adjust back by one
	mapcount = 1 + s_scriptdata.map.num_maps - (s_mapcontrols.map_page * NUMMAPS_PERPAGE);
	adjust = 1;
	if (mapcount > NUMMAPS_PERPAGE)
	{
		mapcount = NUMMAPS_PERPAGE;
		adjust = 0;
	}

	//
	// map short and long name
	//
	for (i = 0; i < mapcount; i++)
		s_mapcontrols.displayMapName[i].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

	for ( ; i < NUMMAPS_PERPAGE; i++)
		s_mapcontrols.displayMapName[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);

	mapcount -= adjust;

	//
	// map time limit
	//
	value = s_mapcontrols.timeLimitType.curvalue;
	if (value == MAP_LT_NONE || value == MAP_LT_CUSTOM) {
		s_mapcontrols.timeLimit.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	}
	else {
		s_mapcontrols.timeLimit.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	i = 0;
	hide = qfalse;
	if (value == MAP_LT_NONE || value == MAP_LT_DEFAULT)
		hide = qtrue;

	if (StartServer_IsRandomGeneratedMapList(s_scriptdata.map.listSource))
		hide = qtrue;

	if (hide)
	{
		// hide custom column
		s_mapcontrols.timeText.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
		for ( ; i < mapcount; i++)
			s_mapcontrols.displayTimeLimit[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	}
	else
	{
		// show custom column
		s_mapcontrols.timeText.generic.flags &= ~QMF_HIDDEN;
		for ( ; i < mapcount; i++)
			s_mapcontrols.displayTimeLimit[i].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	// hide the rest
	for ( ; i < NUMMAPS_PERPAGE; i++)
		s_mapcontrols.displayTimeLimit[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);

	//
	// map frag limit
	//
	value = s_mapcontrols.fragLimitType.curvalue;
	if (value == MAP_LT_NONE || value == MAP_LT_CUSTOM) {
		s_mapcontrols.fragLimit.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	}
	else {
		s_mapcontrols.fragLimit.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	i = 0;
	hide = qfalse;
	if (value == MAP_LT_NONE || value == MAP_LT_DEFAULT)
		hide = qtrue;

	if (StartServer_IsRandomGeneratedMapList(s_scriptdata.map.listSource))
		hide = qtrue;

	if (hide) {
		// hide custom column
		s_mapcontrols.fragsText.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
		for ( ; i < mapcount; i++)
			s_mapcontrols.displayFragLimit[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	}
	else {
		// show custom column
		s_mapcontrols.fragsText.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
		for ( ; i < mapcount; i++)
			s_mapcontrols.displayFragLimit[i].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	// hide the rest
	for ( ; i < NUMMAPS_PERPAGE; i++)
		s_mapcontrols.displayFragLimit[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);

	//
	// displaySelected
	//
	for ( i = 0; i < mapcount; i++)
		s_mapcontrols.displaySelected[i].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

	for ( ; i < NUMMAPS_PERPAGE; i++)
		s_mapcontrols.displaySelected[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);


	//
	// mapSource and map manipulation controls
	//
	if (s_scriptdata.map.listSource == MAP_MS_RANDOM)
	{
		for (i = 0; i < NUMMAPS_PERPAGE; i++) {
			s_mapcontrols.displayMapName[i].generic.flags |= QMF_GRAYED;
			s_mapcontrols.displaySelected[i].generic.flags |= QMF_GRAYED;
		}
		s_mapcontrols.mapPage.generic.flags |= QMF_GRAYED;
	}
	else
	{
		for (i = 0; i < NUMMAPS_PERPAGE; i++) {
			s_mapcontrols.displayMapName[i].generic.flags &= ~QMF_GRAYED;
			s_mapcontrols.displaySelected[i].generic.flags &= ~QMF_GRAYED;
		}

		s_mapcontrols.mapPage.generic.flags &= ~QMF_GRAYED;
	}

	StartServer_MapPage_UpdateActionControls();

	if (StartServer_IsRandomGeneratedMapList(s_scriptdata.map.listSource))
	{
		s_mapcontrols.mapSourceCount.generic.flags &= ~QMF_GRAYED;
		s_mapcontrols.mapSourceType.generic.flags &= ~QMF_GRAYED;

		// clear highlight
		StartServer_MapPage_SetSelectedControl(-1);
	}
	else {
		s_mapcontrols.mapSourceCount.generic.flags |= QMF_GRAYED;
		s_mapcontrols.mapSourceType.generic.flags |= QMF_GRAYED;
	}

	if (s_scriptdata.map.listSource == MAP_MS_RANDOM)
	{
		s_mapcontrols.actionActivate.generic.flags |= QMF_GRAYED;
		s_mapcontrols.actionSrc.generic.flags |= QMF_GRAYED;
		s_mapcontrols.actionDest.generic.flags |= QMF_GRAYED;
	}
	else
	{
		s_mapcontrols.actionActivate.generic.flags &= ~QMF_GRAYED;
		s_mapcontrols.actionSrc.generic.flags &= ~QMF_GRAYED;
		s_mapcontrols.actionDest.generic.flags &= ~QMF_GRAYED;
	}

	// enable fight button if possible
	StartServer_CheckFightReady(&s_mapcontrols.common);
}








/*
=================
StartServer_MapPage_Cache
=================
*/
void StartServer_MapPage_Cache( void )
{
	trap_R_RegisterShaderNoMip( GAMESERVER_VARROWS );
	trap_R_RegisterShaderNoMip( GAMESERVER_UP );
	trap_R_RegisterShaderNoMip( GAMESERVER_DOWN );
	trap_R_RegisterShaderNoMip( GAMESERVER_DEL0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_DEL1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_SELECTED0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_SELECTED1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_ACTION0);
	trap_R_RegisterShaderNoMip( GAMESERVER_ACTION1);
}





/*
=================
StartServer_MapPage_DrawMapName
=================
*/
static void StartServer_MapPage_DrawMapName( void *item )
{
	menutext_s	*s;
	float		*color;
	int			x, y;
	int			style;
	qboolean	focus;
	int index;
	char* string;

	s = (menutext_s *)item;

	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, color_select_bluo );
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	// draw short name
	string = 0;
	index = s->generic.id + ( s_mapcontrols.map_page * NUMMAPS_PERPAGE );
	if (index < s_scriptdata.map.num_maps)
		string = s_scriptdata.map.data[index].shortName;
	else if (index == s_scriptdata.map.num_maps){
		
		if(cl_language.integer == 0){
			string = "<-- add map -->";
		}
		if(cl_language.integer == 1){
			string = "<-- добавить карту -->";
		}
	}
	

	if (string)
		UI_DrawString( x + SMALLCHAR_WIDTH, y, string, style|UI_LEFT, color );

	// draw long name
	string = 0;
	if (index < s_scriptdata.map.num_maps)
		string = s_scriptdata.map.data[index].longName;
	if (string)
		UI_DrawString( x + SMALLCHAR_WIDTH + MAPLONGNAME_DX, y, string, style|UI_LEFT, color );
}





/*
=================
StartServer_MapPage_DoAction
=================
*/
static void StartServer_MapPage_DoAction( void )
{
	int dest, src;

	src = s_mapcontrols.actionSrc.curvalue;
	dest = s_mapcontrols.actionDest.curvalue;

	StartServer_MapDoAction(src, dest, s_mapcontrols.map_page, s_mapcontrols.map_selected);

	StartServer_MapPage_CopyCustomLimitsToControls();
	StartServer_MapPage_UpdateInterface();
}





/*
=================
StartServer_MapPage_InitControlsFromScript
=================
*/
static void StartServer_MapPage_InitControlsFromScript(void)
{
	char buf[64];
	menucommon_s *c;
	int i, index, num, max;

	Com_sprintf( s_mapcontrols.fragLimit.field.buffer, 4, "%i", s_scriptdata.map.fragLimit );
	Com_sprintf( s_mapcontrols.timeLimit.field.buffer, 4, "%i", s_scriptdata.map.timeLimit );

	index = 0;
	if (s_scriptdata.gametype >= GT_CTF && !(s_scriptdata.gametype == GT_LMS))
		index = 2;

	// set "frag" or "capture" control text
	// and fix the mouse hit box for control
	c = &s_mapcontrols.fragLimitType.generic;
	if(cl_language.integer == 0){
	c->name = fragcontrol_text[index];
	s_mapcontrols.fragsText.generic.name = fragcontrol_text[index + 1];
	}
	if(cl_language.integer == 1){
	c->name = fragcontrol_textru[index];
	s_mapcontrols.fragsText.generic.name = fragcontrol_textru[index + 1];
	}
	c->left = c->x - (strlen(c->name) + 1) * SMALLCHAR_WIDTH;

	// load type of frag/time value used to start game (none, default, custom)
	s_mapcontrols.fragLimitType.curvalue = s_scriptdata.map.fragLimitType;
	s_mapcontrols.timeLimitType.curvalue = s_scriptdata.map.timeLimitType;

	// load map source and repeat info
	s_mapcontrols.mapRepeat.curvalue = s_scriptdata.map.Repeat;
	s_mapcontrols.mapSource.curvalue = s_scriptdata.map.listSource;

	Q_strncpyz(s_mapcontrols.mapSourceCount.field.buffer, va("%i", s_scriptdata.map.SourceCount), 3);

	s_mapcontrols.mapSourceType.curvalue = s_scriptdata.map.SourceType;

if(cl_language.integer == 0){
	// get the right text string in the multi-function control
	if (s_scriptdata.gametype == GT_CTF)
		s_mapcontrols.actionSrc.itemnames = copyFrom_ctf_items;
	else
		s_mapcontrols.actionSrc.itemnames = copyFrom_items;
	
}
if(cl_language.integer == 1){
	// get the right text string in the multi-function control
	if (s_scriptdata.gametype == GT_CTF)
		s_mapcontrols.actionSrc.itemnames = copyFrom_ctf_itemsru;
	else
		s_mapcontrols.actionSrc.itemnames = copyFrom_itemsru;
	
}

	StartServer_MapPage_SetSelectedControl(-1);	
	StartServer_MapPage_CopyCustomLimitsToControls();
}





/*
=================
StartServer_MapPage_CheckLimitType
=================
*/
static void StartServer_MapPage_CheckLimitType(void)
{
	if (!StartServer_IsRandomGeneratedMapList(s_mapcontrols.mapSource.curvalue))
		return;

	// random maps can't have custom frag limits
	if (s_mapcontrols.fragLimitType.curvalue == MAP_LT_CUSTOM)
	{
		s_mapcontrols.fragLimitType.curvalue++;
		if (s_mapcontrols.fragLimitType.curvalue >= s_mapcontrols.fragLimitType.numitems )
			s_mapcontrols.fragLimitType.curvalue = 0;
	}

	// random maps can't have custom time limits
	if (s_mapcontrols.timeLimitType.curvalue == MAP_LT_CUSTOM)
	{
		s_mapcontrols.timeLimitType.curvalue++;
		if (s_mapcontrols.timeLimitType.curvalue >= s_mapcontrols.timeLimitType.numitems )
			s_mapcontrols.timeLimitType.curvalue = 0;
	}
}




/*
=================
StartServer_MapPage_Load
=================
*/
static void StartServer_MapPage_Load(void)
{
	s_mapcontrols.gameType.curvalue = s_scriptdata.gametype;

	StartServer_MapPage_InitControlsFromScript();
	StartServer_MapPage_CheckLimitType();
}


/*
=================
StartServer_MapPage_Save
=================
*/
static void StartServer_MapPage_Save(void)
{
	StartServer_SaveScriptData();
}




/*
=================
StartServer_MapPage_SelectionEvent
=================
*/
static void StartServer_MapPage_SelectionEvent( void* ptr, int event )
{
	if (event == QM_ACTIVATED) {
		StartServer_MapPage_SetSelectedControl( ((menucommon_s*)ptr)->id );
	}
}



/*
=================
StartServer_MapPage_SetStatusBarText
=================
*/
static void StartServer_MapPage_SetStatusBarText(const char* text)
{
	if (!text || !text[0])
	{
		s_mapcontrols.statusbar_text[0] = 0;
		return;
	}

	Q_strncpyz(s_mapcontrols.statusbar_text, text, MAX_STATUSBAR_TEXT);
}




/*
=================
StartServer_MapPage_MapCount_callback
=================
*/
static qboolean StartServer_MapPage_MapCount_callback(const char* info)
{
	int subtype;
	const char* mapname;

	subtype = s_scriptdata.map.SourceType;
	if (subtype < MAP_RND_MAX)
		return qfalse;

	subtype -= MAP_RND_MAX;
	mapname = Info_ValueForKey(info, "map");
	return StartServer_IsCustomMapType(mapname, subtype);
}







/*
=================
StartServer_MapPage_SetSourceTypeText
=================
*/
static void StartServer_MapPage_SetSourceTypeText(void)
{
	char *s, *t;
	int id, nonid, total;
	int count;
	int remap;
	int subtype;

	remap = s_scriptdata.gametype;
	id = s_scriptdata.map.TypeCount[remap][MAP_GROUP_ID];
	nonid = s_scriptdata.map.TypeCount[remap][MAP_GROUP_NONID];
	total = id + nonid;

	if(cl_language.integer == 0){
	if (total != 0)
	{
		if ( id == 0)
		{
			s = "all non-QS";
		}
		else
		if ( nonid == 0)
		{
			s = "all by QS";
		}
		else
		{
			s = va("%i by QS", id);
		}

		subtype = s_mapcontrols.mapSourceType.curvalue - MAP_RND_MAX;
		if (subtype >= 0)
		{
			count = UI_BuildMapListByType(NULL, 0, s_scriptdata.gametype, StartServer_MapPage_MapCount_callback);

			s = va("%i of this type", count);
		}

		t = va("There are %i maps total, %s", total, s);
	}
	else
	{
		t = "No maps";
	}
	}
	if(cl_language.integer == 1){
	if (total != 0)
	{
		if ( id == 0)
		{
			s = "все не-QS";
		}
		else
		if ( nonid == 0)
		{
			s = "все от QS";
		}
		else
		{
			s = va("%i от QS", id);
		}

		subtype = s_mapcontrols.mapSourceType.curvalue - MAP_RND_MAX;
		if (subtype >= 0)
		{
			count = UI_BuildMapListByType(NULL, 0, s_scriptdata.gametype, StartServer_MapPage_MapCount_callback);

			s = va("%i этого типа", count);
		}

		t = va("Здесь %i карт, %s всего", total, s);
	}
	else
	{
		t = "Нет карт";
	}
	}

	StartServer_MapPage_SetStatusBarText(t);
}



/*
=================
StartServer_MapPage_LimitStatusbar
=================
*/
static void StartServer_MapPage_LimitStatusbar(void* self)
{
	if(cl_language.integer == 0){
	UI_DrawString(320, s_mapcontrols.statusbar_height, "0 = Unlimited", UI_CENTER|UI_SMALLFONT, color_white);
	}
	if(cl_language.integer == 1){
	UI_DrawString(320, s_mapcontrols.statusbar_height, "0 = Бесконечно", UI_CENTER|UI_SMALLFONT, color_white);
	}
}




/*
=================
StartServer_MapPage_TimeLimitEvent
=================
*/
static void StartServer_MapPage_TimeLimitEvent( void* ptr, int event )
{
	int index;
	int base;

	base = s_mapcontrols.map_page * NUMMAPS_PERPAGE;
	index = ((menucommon_s*)ptr)->id;

	if (event != QM_LOSTFOCUS)
		return;

	Q_strncpyz(s_scriptdata.map.data[index + base].timeLimit, s_mapcontrols.displayTimeLimit[index].field.buffer, MAX_LIMIT_BUF);
}



/*
=================
StartServer_MapPage_FragLimitEvent
=================
*/
static void StartServer_MapPage_FragLimitEvent( void* ptr, int event )
{
	int index;
	int base;

	base = s_mapcontrols.map_page * NUMMAPS_PERPAGE;
	index = ((menucommon_s*)ptr)->id;

	if (event != QM_LOSTFOCUS)
		return;

	Q_strncpyz(s_scriptdata.map.data[index + base].fragLimit, s_mapcontrols.displayFragLimit[index].field.buffer, MAX_LIMIT_BUF);
}


/*
=================
StartServer_MapPage_MenuEvent
=================
*/
static void StartServer_MapPage_MenuEvent( void* ptr, int event )
{
	static char* buf;
	static int num, index;
	static char* s;

	switch( ((menucommon_s*)ptr)->id )
	{
		// controls that update script data

		case ID_MAP_GAMETYPE:
			if (event != QM_ACTIVATED)
				break;

			// make all changes before updating control page
			StartServer_SaveScriptData();

			StartServer_LoadScriptDataFromType( s_mapcontrols.gameType.curvalue );
			StartServer_MapPage_InitControlsFromScript();

			StartServer_MapPage_UpdateInterface();
			break;

		case ID_MAP_MAPSOURCE:
			if (event != QM_ACTIVATED)
				break;

			StartServer_SaveMapList();

			s_scriptdata.map.listSource = s_mapcontrols.mapSource.curvalue;

			StartServer_LoadMapList();
			StartServer_RefreshMapNames();

			StartServer_MapPage_CheckLimitType();
			StartServer_MapPage_UpdateInterface();
			break;

		case ID_MAP_SOURCECOUNT:
			if (event == QM_LOSTFOCUS) {
				buf = s_mapcontrols.mapSourceCount.field.buffer;
				num = (int)Com_Clamp(2, 99 , atoi(buf));
				Q_strncpyz( buf, va("%i",num), 3);
				s_mapcontrols.mapSourceCount.field.cursor = 0;

				s_scriptdata.map.SourceCount = atoi(s_mapcontrols.mapSourceCount.field.buffer);
			}
			break;

		case ID_MAP_MAPREPEAT:
			if (event == QM_ACTIVATED)
			{
				s_scriptdata.map.Repeat = s_mapcontrols.mapRepeat.curvalue;
			}
			break;

		case ID_MAP_TIMELIMIT:
		case ID_MAP_FRAGLIMIT:
			if (event != QM_ACTIVATED)
				break;

			StartServer_MapPage_CheckLimitType();
			s_scriptdata.map.fragLimitType = s_mapcontrols.fragLimitType.curvalue;
			s_scriptdata.map.timeLimitType = s_mapcontrols.timeLimitType.curvalue;

			StartServer_MapPage_UpdateInterface();
			break;

		case ID_MAP_TIMECOUNT:
			if (event == QM_LOSTFOCUS) {
				buf = s_mapcontrols.timeLimit.field.buffer;
				if ( atoi(buf) < 0 ) {
					Q_strncpyz( buf, "0", 3);
					s_mapcontrols.timeLimit.field.cursor = 0;
				}
				s_scriptdata.map.timeLimit = (int)Com_Clamp( 0, 999, atoi(s_mapcontrols.timeLimit.field.buffer) );
			}

			break;

		case ID_MAP_FRAGCOUNT:
			if (event == QM_LOSTFOCUS) {
				buf = s_mapcontrols.fragLimit.field.buffer;
				if ( atoi(buf) < 0 ) {
					Q_strncpyz( buf, "0", 3);
					s_mapcontrols.fragLimit.field.cursor = 0;
				}
				s_scriptdata.map.fragLimit = (int)Com_Clamp( 0, 999, atoi(s_mapcontrols.fragLimit.field.buffer) );
			}
			break;

		case ID_MAP_DEL:
			if (event != QM_ACTIVATED)
				break;

			index = s_mapcontrols.map_selected;
			if (index == -1)
				break;

			num = s_mapcontrols.map_page * NUMMAPS_PERPAGE;
			StartServer_DeleteMap( num + index );

			// end of list deleted, move up one place
			if (num + index == s_scriptdata.map.num_maps) {
				if ( index == 0 ) {	// move to previous page
					if (s_mapcontrols.map_page > 0) {
						s_mapcontrols.map_page--;
						index = NUMMAPS_PERPAGE - 1;
					} else {
						index = -1;
					}
				} else {	// move up one line
					index--;
				}
				// index is guaranteed to have changed
				StartServer_MapPage_SetSelectedControl( index );
			}

			StartServer_MapPage_CopyCustomLimitsToControls();
			StartServer_MapPage_UpdateInterface();
			break;

		case ID_MAP_UP:
			if (event != QM_ACTIVATED)
				break;

			index = s_mapcontrols.map_selected;
			if (index == -1)
				break;

			num = s_mapcontrols.map_page * NUMMAPS_PERPAGE;
			if (StartServer_SwapMaps(num + index, num + index - 1)) {
				if (index > 0) {
					index--;
				} else if (s_mapcontrols.map_page > 0) {
					// map at top of page, move to previous
					s_mapcontrols.map_page--;
					index = NUMMAPS_PERPAGE - 1;
				}
			}

			StartServer_MapPage_CopyCustomLimitsToControls();
			StartServer_MapPage_UpdateInterface();

			if (index != s_mapcontrols.map_selected)
				StartServer_MapPage_SetSelectedControl( index );
			break;

		case ID_MAP_DOWN:
			if (event != QM_ACTIVATED)
				break;

			index = s_mapcontrols.map_selected;
			if (index == -1)
				break;

			num = s_mapcontrols.map_page * NUMMAPS_PERPAGE;
			if (StartServer_SwapMaps(num + index, num + index + 1)) {
				if (index < NUMMAPS_PERPAGE - 1) {
					index++;
				} else if (num + index + 1 < s_scriptdata.map.num_maps) {
					// map at bottom of page, move to next if it has a map
					s_mapcontrols.map_page++;
					index = 0;
				}

			}
			StartServer_MapPage_CopyCustomLimitsToControls();
			StartServer_MapPage_UpdateInterface();

			if (index != s_mapcontrols.map_selected)
				StartServer_MapPage_SetSelectedControl( index );
			break;

		// controls that update the interface only

		case ID_MAP_PAGENUM:
			if (event != QM_ACTIVATED)
				break;

			s_mapcontrols.map_page++;
			if (s_mapcontrols.map_page == StartServer_MapPageCount())
				s_mapcontrols.map_page = 0;

			StartServer_MapPage_SetSelectedControl(-1);

			StartServer_MapPage_CopyCustomLimitsToControls();
			StartServer_MapPage_UpdateInterface();
			break;

		case ID_MAP_ACTIONTYPE:
			if (event != QM_ACTIVATED)
				break;

			StartServer_MapPage_UpdateActionControls();
			break;

		case ID_MAP_ACTION:
			if (event != QM_ACTIVATED)
				break;

			StartServer_MapPage_DoAction();
			break;

		case ID_MAP_SOURCETYPE:
			if (event != QM_ACTIVATED)
				break;

			StartServer_MapPage_SetSourceTypeText();
			s_scriptdata.map.SourceType = s_mapcontrols.mapSourceType.curvalue;
			break;
	}
}




/*
=================
StartServer_MapPage_CommonEvent
=================
*/
static void StartServer_MapPage_CommonEvent( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}

	StartServer_MapPage_Save();
	switch( ((menucommon_s*)ptr)->id )
	{
		case ID_SERVERCOMMON_SERVER:
			StartServer_BotPage_MenuInit();
			StartServer_ItemPage_MenuInit();
			StartServer_ServerPage_MenuInit();
			break;
			
		case ID_SERVERCOMMON_WEAPON:
			StartServer_BotPage_MenuInit();
			StartServer_ItemPage_MenuInit();
			StartServer_WeaponPage_MenuInit();
			break;

		case ID_SERVERCOMMON_BOTS:
			StartServer_BotPage_MenuInit();
			break;

		case ID_SERVERCOMMON_ITEMS:
			StartServer_BotPage_MenuInit();
			StartServer_ItemPage_MenuInit();
			break;

		case ID_SERVERCOMMON_BACK:
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_FIGHT:
			StartServer_CreateServer(NULL);
			break;
	}
}





/*
=================
StartServer_MapPage_ChangeMapEvent
=================
*/
static void StartServer_MapPage_ChangeMapEvent( void* ptr, int event ) {
	int index;

	index = ((menucommon_s*)ptr)->id + (NUMMAPS_PERPAGE * s_mapcontrols.map_page);

	switch (event) {
	case QM_ACTIVATED:
		UI_StartMapMenu(s_scriptdata.gametype, index, s_scriptdata.map.data[index].shortName);
		break;

	case QM_GOTFOCUS:
		if (index < s_scriptdata.map.num_maps)
		{
			StartServer_InitMapPicture(&s_mapcontrols.mappic, s_scriptdata.map.data[index].shortName);
		}
		break;
	}
}




/*
=================
StartServer_MapPage_InitPageText
=================
*/
static void StartServer_MapPage_InitPageText( menutext_s *a )
{
	int	len;
	int cw, ch;

	// calculate bounds
	if (a->generic.name)
		len = strlen(a->generic.name);
	else
		len = 0;

	if (a->generic.flags & QMF_SMALLFONT) {
		ch = SMALLCHAR_HEIGHT;
		cw = SMALLCHAR_WIDTH;
	}
	else {
		ch = BIGCHAR_HEIGHT;
		cw = BIGCHAR_WIDTH;
	}

	// left justify text
	a->generic.left   = a->generic.x;
	a->generic.right  = a->generic.x + len*cw;
	a->generic.top    = a->generic.y;
	a->generic.bottom = a->generic.y + ch;
}



/*
=================
StartServer_MapPage_DrawPageText
=================
*/
static void StartServer_MapPage_DrawPageText( void* b )
{
	float *color;
	int	x,y;
	int	style;
	qboolean focus;
	menutext_s* s;

	s = (menutext_s*)b;
	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, color_select_bluo );
	}

	UI_DrawString( x, y, s->generic.name, style, color );
}




/*
=================
StartServer_MapPage_DrawStatusBarText
=================
*/
static void StartServer_MapPage_DrawStatusBarText(void* self)
{
	UI_DrawString(320, s_mapcontrols.statusbar_height, s_mapcontrols.statusbar_text, UI_CENTER|UI_SMALLFONT, color_white);
}



/*
=================
StartServer_MapPage_MenuDraw
=================
*/
static void StartServer_MapPage_MenuDraw(void)
{
	int i;
	float f;
	qboolean excluded;
	menucommon_s* g;
	int maxmaps;
	qhandle_t picshader;

	if (uis.firstdraw) {
		StartServer_MapPage_Load();
		StartServer_MapPage_UpdateInterface();
	}

	excluded = qfalse;
	if (s_scriptdata.map.listSource == MAP_MS_RANDOMEXCLUDE)
		excluded = qtrue;

	StartServer_BackgroundDraw(excluded);

	// draw map picture
	maxmaps = s_scriptdata.map.num_maps - s_mapcontrols.map_page * NUMMAPS_PERPAGE;
	if (maxmaps > NUMMAPS_PERPAGE)
		maxmaps = NUMMAPS_PERPAGE;

	for (i = 0; i < maxmaps; i++)
	{
		g = &s_mapcontrols.displayMapName[i].generic;
		if (g->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN))
			continue;

		if (UI_CursorInRect(g->left, g->top, g->right - g->left, g->bottom - g->top))
		{
			s_mapcontrols.fadetime = uis.realtime + MAP_FADETIME;
			break;
		}
	}

	if (uis.realtime < s_mapcontrols.fadetime )
	{
		f = (float)(s_mapcontrols.fadetime - uis.realtime) / MAP_FADETIME;

		pulsecolor[3] = f;
		fading_red[3] = f;

		trap_R_SetColor( fading_red );
		UI_DrawNamedPic(640 -12 - 134 + uis.wideoffset, 24 - 7,  144, 106, MAPSELECT_SELECT);
		trap_R_SetColor( NULL );

		StartServer_DrawMapPicture( 640 -12 -124 + uis.wideoffset, 24, 124, 85, &s_mapcontrols.mappic, pulsecolor);
	}

	// draw the controls
	Menu_Draw(&s_mapcontrols.menu);
}


/*
=================
StartServer_MapPage_MenuInit
=================
*/
void StartServer_MapPage_MenuInit(void)
{
	menuframework_s* menuptr;
	int y, n;

	memset(&s_mapcontrols, 0, sizeof(mapcontrols_t));

	StartServer_MapPage_Cache();

	menuptr = &s_mapcontrols.menu;

	menuptr->wrapAround = qtrue;
	menuptr->native 	= qfalse;
	menuptr->fullscreen = qtrue;
	menuptr->draw = StartServer_MapPage_MenuDraw;

	StartServer_CommonControls_Init(menuptr, &s_mapcontrols.common, StartServer_MapPage_CommonEvent, COMMONCTRL_MAPS);

    s_mapcontrols.map_selected = -1;

	//
	// the user selected maps
	//
	y = GAMETYPEROW_Y;
	s_mapcontrols.gameType.generic.type  = MTYPE_SPINCONTROL;
	s_mapcontrols.gameType.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.gameType.generic.id	   = ID_MAP_GAMETYPE;
	s_mapcontrols.gameType.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.gameType.generic.x	   = GAMETYPECOLUMN_X;
	s_mapcontrols.gameType.generic.y	   = y;
	

	s_mapcontrols.gameTypeIcon.generic.type  = MTYPE_BITMAP;
	s_mapcontrols.gameTypeIcon.generic.flags = QMF_INACTIVE;
	s_mapcontrols.gameTypeIcon.generic.x	 = GAMETYPEICON_X;
	s_mapcontrols.gameTypeIcon.generic.y	 = y;
	s_mapcontrols.gameTypeIcon.width  	     = 32;
	s_mapcontrols.gameTypeIcon.height  	     = 32;

	y += LINE_HEIGHT;
	s_mapcontrols.mapSource.generic.type  = MTYPE_SPINCONTROL;
	s_mapcontrols.mapSource.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.mapSource.generic.id	   = ID_MAP_MAPSOURCE;
	s_mapcontrols.mapSource.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.mapSource.generic.x	   = GAMETYPECOLUMN_X;
	s_mapcontrols.mapSource.generic.y	   = y;
	

	y += 2*LINE_HEIGHT;
	s_mapcontrols.timeLimitType.generic.type  = MTYPE_SPINCONTROL;
	s_mapcontrols.timeLimitType.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.timeLimitType.generic.id	   = ID_MAP_TIMELIMIT;
	s_mapcontrols.timeLimitType.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.timeLimitType.generic.x	   = MAPCOLUMN_LEFTX;
	s_mapcontrols.timeLimitType.generic.y	   = y;

	s_mapcontrols.timeLimit.generic.type       = MTYPE_FIELD;
	s_mapcontrols.timeLimit.generic.name       = 0;
	s_mapcontrols.timeLimit.generic.flags      = QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_mapcontrols.timeLimit.generic.x          = MAPCOLUMN_LEFTX + 9 * SMALLCHAR_WIDTH;
	s_mapcontrols.timeLimit.generic.y	        = y;
	s_mapcontrols.timeLimit.generic.id	        = ID_MAP_TIMECOUNT;
	s_mapcontrols.timeLimit.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.timeLimit.generic.statusbar  = StartServer_MapPage_LimitStatusbar;
	s_mapcontrols.timeLimit.field.widthInChars = 3;
	s_mapcontrols.timeLimit.field.maxchars     = 3;

	s_mapcontrols.mapSourceCount.generic.type       = MTYPE_FIELD;
	s_mapcontrols.mapSourceCount.generic.flags      = QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_mapcontrols.mapSourceCount.generic.x          = MAPCOLUMN_RIGHTX;
	s_mapcontrols.mapSourceCount.generic.y	        = y;
	s_mapcontrols.mapSourceCount.generic.id	        = ID_MAP_SOURCECOUNT;
	s_mapcontrols.mapSourceCount.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.mapSourceCount.field.widthInChars = 3;
	s_mapcontrols.mapSourceCount.field.maxchars     = 2;

	s_mapcontrols.mapSourceType.generic.type       = MTYPE_SPINCONTROL;
	s_mapcontrols.mapSourceType.generic.name       = NULL;
	s_mapcontrols.mapSourceType.generic.flags      = QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_mapcontrols.mapSourceType.generic.x          = MAPCOLUMN_RIGHTX + (4*SMALLCHAR_WIDTH);
	s_mapcontrols.mapSourceType.generic.y	        = y;
	s_mapcontrols.mapSourceType.generic.id	        = ID_MAP_SOURCETYPE;
	s_mapcontrols.mapSourceType.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.mapSourceType.generic.statusbar  = StartServer_MapPage_DrawStatusBarText;
	

	y += LINE_HEIGHT;
	s_mapcontrols.fragLimitType.generic.type  = MTYPE_SPINCONTROL;
	s_mapcontrols.fragLimitType.generic.name = 0;
	s_mapcontrols.fragLimitType.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.fragLimitType.generic.id	   = ID_MAP_FRAGLIMIT;
	s_mapcontrols.fragLimitType.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.fragLimitType.generic.x	   = MAPCOLUMN_LEFTX;
	s_mapcontrols.fragLimitType.generic.y	   = y;
	

	s_mapcontrols.fragLimit.generic.type       = MTYPE_FIELD;
	s_mapcontrols.fragLimit.generic.name       = 0;
	s_mapcontrols.fragLimit.generic.flags      = QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_mapcontrols.fragLimit.generic.x          = MAPCOLUMN_LEFTX + 9 * SMALLCHAR_WIDTH;
	s_mapcontrols.fragLimit.generic.y	        = y;
	s_mapcontrols.fragLimit.generic.id	        = ID_MAP_FRAGCOUNT;
	s_mapcontrols.fragLimit.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.fragLimit.generic.statusbar  = StartServer_MapPage_LimitStatusbar;
	s_mapcontrols.fragLimit.field.widthInChars = 3;
	s_mapcontrols.fragLimit.field.maxchars     = 3;

	s_mapcontrols.mapRepeat.generic.type  = MTYPE_RADIOBUTTON;
	s_mapcontrols.mapRepeat.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.mapRepeat.generic.id	   = ID_MAP_MAPREPEAT;
	s_mapcontrols.mapRepeat.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.mapRepeat.generic.x	   = MAPCOLUMN_RIGHTX;
	s_mapcontrols.mapRepeat.generic.y	   = y;

	y += 2 * LINE_HEIGHT;
	s_mapcontrols.mapText.generic.type  = MTYPE_TEXT;
	s_mapcontrols.mapText.generic.x	   = MAPARRAYCOLUMN_X;
	s_mapcontrols.mapText.generic.y	   = y;
	s_mapcontrols.mapText.style = UI_SMALLFONT;
	s_mapcontrols.mapText.color = color_white;

	s_mapcontrols.nameText.generic.type  = MTYPE_TEXT;
	s_mapcontrols.nameText.generic.x	   = MAPARRAYCOLUMN_X + MAPLONGNAME_DX;
	s_mapcontrols.nameText.generic.y	   = y;
	s_mapcontrols.nameText.style = UI_SMALLFONT;
	s_mapcontrols.nameText.color = color_white;

	s_mapcontrols.fragsText.generic.type  = MTYPE_TEXT;
	s_mapcontrols.fragsText.generic.x	   = MAPARRAYCOLUMN_X + MAPFRAGS_DX + 2 * SMALLCHAR_WIDTH;
	s_mapcontrols.fragsText.generic.y	   = y;
	s_mapcontrols.fragsText.style = UI_SMALLFONT|UI_CENTER;
	s_mapcontrols.fragsText.string = 0;
	s_mapcontrols.fragsText.color = color_white;

	s_mapcontrols.timeText.generic.type  = MTYPE_TEXT;
	s_mapcontrols.timeText.generic.x	   = MAPARRAYCOLUMN_X + MAPTIME_DX + 2*SMALLCHAR_WIDTH;
	s_mapcontrols.timeText.generic.y	   = y;
	s_mapcontrols.timeText.style = UI_SMALLFONT|UI_CENTER;
	s_mapcontrols.timeText.color = color_white;

	s_mapcontrols.arrows.generic.type  = MTYPE_BITMAP;
	s_mapcontrols.arrows.generic.name  = GAMESERVER_VARROWS;
	s_mapcontrols.arrows.generic.flags = QMF_INACTIVE;
	s_mapcontrols.arrows.generic.x	 = MAPBUTTONS_X;
	s_mapcontrols.arrows.generic.y	 = y;
	s_mapcontrols.arrows.width  	     = GAMESERVER_ARROWWIDTH;
	s_mapcontrols.arrows.height  	     = 2 * GAMESERVER_ARROWHEIGHT;

	s_mapcontrols.up.generic.type     = MTYPE_BITMAP;
	s_mapcontrols.up.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapcontrols.up.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.up.generic.id	    = ID_MAP_UP;
	s_mapcontrols.up.generic.x		= MAPBUTTONS_X;
	s_mapcontrols.up.generic.y		= y;
	s_mapcontrols.up.width  		    = GAMESERVER_ARROWWIDTH;
	s_mapcontrols.up.height  		    = GAMESERVER_ARROWHEIGHT;
	s_mapcontrols.up.focuspic     = GAMESERVER_UP;

	s_mapcontrols.down.generic.type     = MTYPE_BITMAP;
	s_mapcontrols.down.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapcontrols.down.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.down.generic.id	    = ID_MAP_DOWN;
	s_mapcontrols.down.generic.x		= MAPBUTTONS_X;
	s_mapcontrols.down.generic.y		= y + GAMESERVER_ARROWHEIGHT;
	s_mapcontrols.down.width  		    = GAMESERVER_ARROWWIDTH;
	s_mapcontrols.down.height  		    = GAMESERVER_ARROWHEIGHT;
	s_mapcontrols.down.focuspic = GAMESERVER_DOWN;

	s_mapcontrols.del.generic.type     = MTYPE_BITMAP;
	s_mapcontrols.del.generic.name     = GAMESERVER_DEL0;
	s_mapcontrols.del.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapcontrols.del.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.del.generic.id	    = ID_MAP_DEL;
	s_mapcontrols.del.generic.x		= MAPBUTTONS_X;
	s_mapcontrols.del.generic.y		= y + 2* GAMESERVER_ARROWHEIGHT;
	s_mapcontrols.del.width  		    = 48;
	s_mapcontrols.del.height  		    = 96;
	s_mapcontrols.del.focuspic         = GAMESERVER_DEL1;

	for (n = 0; n < NUMMAPS_PERPAGE; n++)
	{
		y += (5 * LINE_HEIGHT)/4;

		s_mapcontrols.displaySelected[n].generic.type  = MTYPE_BITMAP;
		s_mapcontrols.displaySelected[n].generic.name  = GAMESERVER_SELECTED0;
		s_mapcontrols.displaySelected[n].generic.flags = QMF_PULSEIFFOCUS;
		s_mapcontrols.displaySelected[n].generic.x	   = MAPARRAYCOLUMN_X - 20;
		s_mapcontrols.displaySelected[n].generic.y	   = y;
		s_mapcontrols.displaySelected[n].generic.id	   = n;
		s_mapcontrols.displaySelected[n].generic.callback = StartServer_MapPage_SelectionEvent;
		s_mapcontrols.displaySelected[n].generic.ownerdraw = StartServer_SelectionDraw;
		s_mapcontrols.displaySelected[n].width = 16;
		s_mapcontrols.displaySelected[n].height = 16;
		s_mapcontrols.displaySelected[n].focuspic = GAMESERVER_SELECTED1;

		s_mapcontrols.displayMapName[n].generic.type  = MTYPE_TEXT;
		s_mapcontrols.displayMapName[n].generic.flags = QMF_SMALLFONT | QMF_NODEFAULTINIT;
		s_mapcontrols.displayMapName[n].generic.x	   = MAPARRAYCOLUMN_X;
		s_mapcontrols.displayMapName[n].generic.y	   = y;
		s_mapcontrols.displayMapName[n].generic.id	   = n;
		s_mapcontrols.displayMapName[n].generic.callback = StartServer_MapPage_ChangeMapEvent;
		s_mapcontrols.displayMapName[n].generic.ownerdraw = StartServer_MapPage_DrawMapName;
		s_mapcontrols.displayMapName[n].generic.top	   = y;
		s_mapcontrols.displayMapName[n].generic.bottom	   = y + SMALLCHAR_HEIGHT;
		s_mapcontrols.displayMapName[n].generic.left	   = MAPARRAYCOLUMN_X - SMALLCHAR_WIDTH/2;
		s_mapcontrols.displayMapName[n].generic.right	   =
			MAPARRAYCOLUMN_X + MAPFRAGS_DX - SMALLCHAR_WIDTH/2;
		s_mapcontrols.displayMapName[n].style = UI_SMALLFONT;
		s_mapcontrols.displayMapName[n].color = color_white;

		s_mapcontrols.displayFragLimit[n].generic.type       = MTYPE_FIELD;
		s_mapcontrols.displayFragLimit[n].generic.name       = 0;
		s_mapcontrols.displayFragLimit[n].generic.flags      = QMF_SMALLFONT|QMF_NUMBERSONLY|QMF_PULSEIFFOCUS;
		s_mapcontrols.displayFragLimit[n].generic.x          = MAPARRAYCOLUMN_X + MAPFRAGS_DX;
		s_mapcontrols.displayFragLimit[n].generic.y	        = y;
		s_mapcontrols.displayFragLimit[n].generic.id	        = n;
		s_mapcontrols.displayFragLimit[n].generic.callback	= StartServer_MapPage_FragLimitEvent;
		s_mapcontrols.displayFragLimit[n].generic.statusbar  = StartServer_MapPage_LimitStatusbar;
		s_mapcontrols.displayFragLimit[n].field.widthInChars = 3;
		s_mapcontrols.displayFragLimit[n].field.maxchars     = 3;

		s_mapcontrols.displayTimeLimit[n].generic.type       = MTYPE_FIELD;
		s_mapcontrols.displayTimeLimit[n].generic.name       = 0;
		s_mapcontrols.displayTimeLimit[n].generic.flags      = QMF_SMALLFONT|QMF_NUMBERSONLY|QMF_PULSEIFFOCUS;
		s_mapcontrols.displayTimeLimit[n].generic.x          = MAPARRAYCOLUMN_X + MAPTIME_DX;
		s_mapcontrols.displayTimeLimit[n].generic.y	        = y;
		s_mapcontrols.displayTimeLimit[n].generic.id		= n;
		s_mapcontrols.displayTimeLimit[n].generic.callback	= StartServer_MapPage_TimeLimitEvent;
		s_mapcontrols.displayTimeLimit[n].generic.statusbar  = StartServer_MapPage_LimitStatusbar;
		s_mapcontrols.displayTimeLimit[n].field.widthInChars = 3;
		s_mapcontrols.displayTimeLimit[n].field.maxchars     = MAP_NUMBER_FIELD;
	}

	// custom drawn and initialized control
	y += 2* LINE_HEIGHT;
	s_mapcontrols.mapPage.generic.type  = MTYPE_TEXT;
	s_mapcontrols.mapPage.generic.flags  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.mapPage.generic.callback  = StartServer_MapPage_MenuEvent;
	s_mapcontrols.mapPage.generic.id = ID_MAP_PAGENUM;
	s_mapcontrols.mapPage.generic.x	   = MAPARRAYCOLUMN_X;
	s_mapcontrols.mapPage.generic.y	   = y;
	s_mapcontrols.mapPage.generic.ownerdraw = StartServer_MapPage_DrawPageText;
	s_mapcontrols.mapPage.generic.name = s_mapcontrols.mappage_text;
	s_mapcontrols.mapPage.color 		= text_color_normal;
	StartServer_MapPage_SetPageText();

	s_mapcontrols.actionSrc.generic.type  = MTYPE_SPINCONTROL;
	s_mapcontrols.actionSrc.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.actionSrc.generic.id = ID_MAP_ACTIONTYPE;
	s_mapcontrols.actionSrc.generic.name = ACTIONTYPE_DELETE;
	s_mapcontrols.actionSrc.generic.callback  = StartServer_MapPage_MenuEvent;
	s_mapcontrols.actionSrc.generic.x	   = MAPACTIVATE_X+40;
	s_mapcontrols.actionSrc.generic.y	   = y + 100;


	s_mapcontrols.actionDest.generic.type  = MTYPE_SPINCONTROL;
	s_mapcontrols.actionDest.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_mapcontrols.actionDest.generic.x	   = MAPACTIVATE_X+40;
	s_mapcontrols.actionDest.generic.y	   = y + LINE_HEIGHT + 100;
	

	s_mapcontrols.actionActivate.generic.type     = MTYPE_BITMAP;
	s_mapcontrols.actionActivate.generic.name     = GAMESERVER_ACTION0;
	s_mapcontrols.actionActivate.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mapcontrols.actionActivate.generic.callback = StartServer_MapPage_MenuEvent;
	s_mapcontrols.actionActivate.generic.id	    = ID_MAP_ACTION;
	s_mapcontrols.actionActivate.generic.x		= MAPACTIVATE_X - 64 - 11*SMALLCHAR_WIDTH;
	s_mapcontrols.actionActivate.generic.y		= y + 100;
	s_mapcontrols.actionActivate.width  		    = 64;
	s_mapcontrols.actionActivate.height  		    = 32;
	s_mapcontrols.actionActivate.focuspic         = GAMESERVER_ACTION1;
	
	if(cl_language.integer == 0){
	s_mapcontrols.fragLimitType.itemnames	   = fraglimittype_items;
	s_mapcontrols.mapSourceType.itemnames = randommaptype_items;
	s_mapcontrols.gameType.itemnames	   = gametype_items;
	s_mapcontrols.mapSource.itemnames	   = mapSource_items;
	s_mapcontrols.timeLimitType.itemnames	   = timelimittype_items;
	s_mapcontrols.actionDest.itemnames	   = copyTo_items;
	s_mapcontrols.actionSrc.itemnames	   = copyFrom_items;
	s_mapcontrols.timeText.string = "Time";
	s_mapcontrols.nameText.string = "Name";
	s_mapcontrols.mapText.string = "Map";
	s_mapcontrols.mapRepeat.generic.name = "Repeat forever:";
	s_mapcontrols.mapSourceCount.generic.name       = "Randomly choose:";
	s_mapcontrols.timeLimitType.generic.name = "Time Limit:";
	s_mapcontrols.mapSource.generic.name = "Map Source:";
	s_mapcontrols.gameType.generic.name = "Game Type:";
	s_mapcontrols.actionDest.generic.name = "to";
	}
	if(cl_language.integer == 1){
	s_mapcontrols.fragLimitType.itemnames	   = fraglimittype_itemsru;
	s_mapcontrols.mapSourceType.itemnames = randommaptype_itemsru;
	s_mapcontrols.gameType.itemnames	   = gametype_itemsru;
	s_mapcontrols.mapSource.itemnames	   = mapSource_itemsru;
	s_mapcontrols.timeLimitType.itemnames	   = timelimittype_itemsru;
	s_mapcontrols.actionDest.itemnames	   = copyTo_itemsru;
	s_mapcontrols.actionSrc.itemnames	   = copyFrom_itemsru;
	s_mapcontrols.timeText.string = "Время";
	s_mapcontrols.nameText.string = "Имя";
	s_mapcontrols.mapText.string = "Карта";
	s_mapcontrols.mapRepeat.generic.name = "Повторять всегда:";
	s_mapcontrols.mapSourceCount.generic.name       = "Выбрать случайно:";
	s_mapcontrols.timeLimitType.generic.name = "Лимит Времени:";
	s_mapcontrols.mapSource.generic.name = "Источник Карт:";
	s_mapcontrols.gameType.generic.name = "Режим Игры:";
	s_mapcontrols.actionDest.generic.name = "в";
	}

#ifndef NO_UIE_MINILOGO_SKIRMISH
	s_mapcontrols.logo.generic.type			= MTYPE_BITMAP;
	s_mapcontrols.logo.generic.flags		= QMF_INACTIVE|QMF_HIGHLIGHT;
	s_mapcontrols.logo.generic.x			= UIE_LOGO_X;
	s_mapcontrols.logo.generic.y			= UIE_LOGO_Y;
	s_mapcontrols.logo.width				= 64;
	s_mapcontrols.logo.height				= 16;
	s_mapcontrols.logo.focuspic 			= UIE_LOGO_NAME;
	s_mapcontrols.logo.focuscolor 			= color_translucent;
#endif

	y += 32 + LINE_HEIGHT;
	s_mapcontrols.statusbar_height = y;

	// load map info here, so mapSourceType is correctly initialized
	UI_LoadMapTypeInfo();

	//
	// register page controls
	//

	Menu_AddItem( menuptr, &s_mapcontrols.gameType);
	Menu_AddItem( menuptr, &s_mapcontrols.gameTypeIcon);

	Menu_AddItem( menuptr, &s_mapcontrols.timeLimitType);
	Menu_AddItem( menuptr, &s_mapcontrols.timeLimit);
	Menu_AddItem( menuptr, &s_mapcontrols.mapRepeat);
	Menu_AddItem( menuptr, &s_mapcontrols.fragLimitType);
	Menu_AddItem( menuptr, &s_mapcontrols.fragLimit);
	Menu_AddItem( menuptr, &s_mapcontrols.mapSource);
	Menu_AddItem( menuptr, &s_mapcontrols.mapSourceCount);
	Menu_AddItem( menuptr, &s_mapcontrols.mapSourceType);

	Menu_AddItem( menuptr, &s_mapcontrols.mapText);
	Menu_AddItem( menuptr, &s_mapcontrols.nameText);
	Menu_AddItem( menuptr, &s_mapcontrols.fragsText);
	Menu_AddItem( menuptr, &s_mapcontrols.timeText);

	StartServer_MapPage_InitPageText(&s_mapcontrols.mapPage);
	Menu_AddItem( menuptr, &s_mapcontrols.mapPage);

	for (n = 0; n < NUMMAPS_PERPAGE; n++)
	{
		Menu_AddItem( menuptr, &s_mapcontrols.displaySelected[n]);
		Menu_AddItem( menuptr, &s_mapcontrols.displayMapName[n]);
		Menu_AddItem( menuptr, &s_mapcontrols.displayFragLimit[n]);
		Menu_AddItem( menuptr, &s_mapcontrols.displayTimeLimit[n]);
	}

	Menu_AddItem( menuptr, &s_mapcontrols.arrows);
	Menu_AddItem( menuptr, &s_mapcontrols.up);
	Menu_AddItem( menuptr, &s_mapcontrols.down);
	Menu_AddItem( menuptr, &s_mapcontrols.del);

	Menu_AddItem( menuptr, &s_mapcontrols.actionSrc);
	Menu_AddItem( menuptr, &s_mapcontrols.actionDest);
	Menu_AddItem( menuptr, &s_mapcontrols.actionActivate);

#ifndef NO_UIE_MINILOGO_SKIRMISH
	if (random() < 0.1)
		Menu_AddItem( menuptr,	&s_mapcontrols.logo);
#endif

	UI_PushMenu( &s_mapcontrols.menu);
}



/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/





/*
=================
UI_StartServerMenu
=================
*/
void UI_StartServerMenu( qboolean multi)
{
	StartServer_Cache();
	StartServer_InitScriptData(multi);
	StartServer_MapPage_MenuInit();
}




