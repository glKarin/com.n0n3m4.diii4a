/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/



#include "ui_local.h"
#include "ui_startserver_q3.h"



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
#define BOTSELECT_SMALLSELECTED		"menu/uie_art/opponents_smallselected"
#define BOTSELECT_ARROWS		"menu/art/gs_arrows_0"
#define BOTSELECT_ARROWSL		"menu/art/gs_arrows_l"
#define BOTSELECT_ARROWSR		"menu/art/gs_arrows_r"


#define ID_BOTSELECT_VIEWLIST 	1
#define ID_BOTSELECT_LEFT 		2
#define ID_BOTSELECT_RIGHT      3
#define ID_BOTSELECT_BACK	    4
#define ID_BOTSELECT_ACCEPT     5
#define ID_BOTSELECT_MULTISEL	6


#define BOTNAME_LENGTH 16

#define BOTGRID_COLS			4
#define BOTGRID_ROWS			4
#define MAX_GRIDMODELSPERPAGE		(BOTGRID_ROWS * BOTGRID_COLS)

#define BOTLIST_COLS			3
#define BOTLIST_ROWS			12
#define MAX_LISTMODELSPERPAGE	(BOTLIST_COLS * BOTLIST_ROWS)

#define BOTLIST_ICONSIZE	28

#define MAX_MULTISELECTED   15


#if (MAX_LISTMODELSPERPAGE > MAX_GRIDMODELSPERPAGE)
	#define MAX_MODELSPERPAGE MAX_LISTMODELSPERPAGE
#else
	#define MAX_MODELSPERPAGE MAX_GRIDMODELSPERPAGE
#endif


// holdover viewing method
//static qboolean botselRevisit = qfalse;
//static qboolean lastVisitViewList = qfalse;
//static qboolean useMultiSel = qfalse;

static vec4_t transparent_color = { 1.0, 1.0, 1.0, 0.7 };


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menubitmap_s	picbuttons[MAX_GRIDMODELSPERPAGE];

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;

	menubitmap_s	go;
	menubitmap_s	back;

	menulist_s		botlist; 	// only ever contains a screen full of information
	menuradiobutton_s	viewlist;
	menuradiobutton_s	multisel;

	int 			maxBotsPerPage;

	int 			index;
	int				numBots;
	int				page;
	int				numpages;

	// most recently selected bot
	int				selectedbot;

	int				sortedBotNums[MAX_BOTS];
	char			boticons[MAX_MODELSPERPAGE][MAX_QPATH];
	char			botnames[MAX_MODELSPERPAGE][BOTNAME_LENGTH];

	int				numMultiSel;
	int				multiSel[MAX_MULTISELECTED];

	float*			botcolor[MAX_MODELSPERPAGE];
	const char*		botalias[MAX_MODELSPERPAGE];
} botSelectInfo_t;

static botSelectInfo_t	botSelectInfo;


/*
=================
UI_BotSelect_SortCompare
=================
*/
static int QDECL UI_BotSelect_SortCompare( const void *arg1, const void *arg2 ) {
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
UI_BotSelect_SelectedOnPage
=================
*/
static qboolean UI_BotSelect_SelectedOnPage(void)
{
	int page;

	if (botSelectInfo.selectedbot == -1)
		return qfalse;

	page = botSelectInfo.page * botSelectInfo.maxBotsPerPage;
	if (botSelectInfo.selectedbot < page)
		return qfalse;

	if (botSelectInfo.selectedbot >= page + botSelectInfo.maxBotsPerPage)
		return qfalse;

	return qtrue;
}



/*
=================
UI_BotSelect_SetBotInfoInCaller
=================
*/
static void UI_BotSelect_SetBotInfoInCaller(void)
{
	char* info;
	char* name;
	int index;
	int sel;
	int type;

	if (botSelectInfo.multisel.curvalue)
	{
		index = botSelectInfo.index;
		type = StartServer_SlotTeam(index);
		if (type == SLOTTEAM_INVALID)
			return;

		for (sel = 0; sel < botSelectInfo.numMultiSel; sel++, index++)
		{
			if (StartServer_SlotTeam(index) != type)
				break;

			info = UI_GetBotInfoByNumber(botSelectInfo.sortedBotNums[ botSelectInfo.multiSel[sel] ]);
			name = Info_ValueForKey(info, "name");

			if (sel == 0)
				StartServer_SetNamedBot(index, name);
			else
				StartServer_InsertNamedBot(index, name);
		}
	}
	else {
		if (botSelectInfo.selectedbot == -1)
			return;
			
		info = UI_GetBotInfoByNumber(botSelectInfo.sortedBotNums[ botSelectInfo.selectedbot ]);
		name = Info_ValueForKey(info, "name");

		StartServer_SetNamedBot(botSelectInfo.index, name);
	}
}



/*
=================
UI_BotSelect_SelectBot
=================
*/
static void UI_BotSelect_AddBotSelection(int bot)
{
	int i, j;

	// single selection only
	if (!botSelectInfo.multisel.curvalue) {
		if (botSelectInfo.selectedbot == bot)
			// toggle current selection
			botSelectInfo.selectedbot = -1;
		else
			botSelectInfo.selectedbot = bot;
		return;
	}

	// check for presence in list already, and remove if found
	for (i = 0; i < botSelectInfo.numMultiSel; i++) {
		if (botSelectInfo.multiSel[i] == bot) {
			for (j = i; j < botSelectInfo.numMultiSel - 1; j++)
				botSelectInfo.multiSel[j] = botSelectInfo.multiSel[j + 1];

			botSelectInfo.numMultiSel--;
			return;
		}
	}

	// add to list, if enough space
	if (botSelectInfo.numMultiSel == MAX_MULTISELECTED)
		return;

	botSelectInfo.multiSel[ botSelectInfo.numMultiSel ] = bot;
	botSelectInfo.numMultiSel++;
	botSelectInfo.selectedbot = bot;
}




/*
=================
UI_BotSelect_ToggleMultiSelect
=================
*/
static void UI_BotSelect_ToggleMultiSelect( void )
{
	trap_Cvar_SetValue("uie_bot_multisel", botSelectInfo.multisel.curvalue);

	if (!botSelectInfo.multisel.curvalue) {
		// change to single sel
		if (botSelectInfo.numMultiSel)
			botSelectInfo.selectedbot = botSelectInfo.multiSel[0];
		else
			botSelectInfo.selectedbot = -1;	
		return;
	}

	// change to multiple sel
	if (botSelectInfo.selectedbot == -1) {
		botSelectInfo.numMultiSel = 0;
		return;
	}

	if (botSelectInfo.numMultiSel == 0) {
		UI_BotSelect_AddBotSelection(botSelectInfo.selectedbot);
		return;
	}

	if (botSelectInfo.selectedbot != botSelectInfo.multiSel[0])
	{
		botSelectInfo.multiSel[0] = botSelectInfo.selectedbot;
		botSelectInfo.numMultiSel = 1;
	}
}



/*
=================
UI_BotSelect_SetViewType
=================
*/
static void UI_BotSelect_SetViewType( void )
{
	if (botSelectInfo.viewlist.curvalue) {
		botSelectInfo.maxBotsPerPage = MAX_LISTMODELSPERPAGE;
	}
	else {
		botSelectInfo.maxBotsPerPage = MAX_GRIDMODELSPERPAGE;
	}

	botSelectInfo.numpages = botSelectInfo.numBots / botSelectInfo.maxBotsPerPage;
	if( botSelectInfo.numBots % botSelectInfo.maxBotsPerPage) {
		botSelectInfo.numpages++;
	}

	trap_Cvar_SetValue("uie_bot_list", botSelectInfo.viewlist.curvalue);
}



/*
=================
UI_BotSelect_BuildList
=================
*/
static void UI_BotSelect_BuildList( void ) {
	int		n;

	botSelectInfo.numBots = UI_GetNumBots();

	// initialize the array
	for( n = 0; n < botSelectInfo.numBots; n++ ) {
		botSelectInfo.sortedBotNums[n] = n;
	}

	// now sort it
	qsort( botSelectInfo.sortedBotNums, botSelectInfo.numBots, sizeof(botSelectInfo.sortedBotNums[0]), UI_BotSelect_SortCompare );
}



/*
=================
UI_ServerPlayerIcon
=================
*/
void UI_ServerPlayerIcon( const char *modelAndSkin, char *iconName, int iconNameMaxSize ) {
	char	*skin;
	char	model[MAX_QPATH];

	Q_strncpyz( model, modelAndSkin, sizeof(model));
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = "default";
	}

	Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_%s.tga", model, skin );

	if( !trap_R_RegisterShaderNoMip( iconName ) && Q_stricmp( skin, "default" ) != 0 ) {
		Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_default.tga", model );
	}
}


/*
=================
UI_ServerPlayerIcon
=================
*/
void UI_ServerNpcIcon( const char *modelAndSkin, char *iconName, int iconNameMaxSize ) {
	char	*skin;
	char	model[MAX_QPATH];

	Q_strncpyz( model, modelAndSkin, sizeof(model));
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = "default";
	}

	Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_%s.tga", model, skin );

	if( !trap_R_RegisterShaderNoMip( iconName ) && Q_stricmp( skin, "default" ) != 0 ) {
		Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_default.tga", model );
	}
}


/*
=================
UI_BotSelect_UpdateGridInterface
=================
*/
static void UI_BotSelect_UpdateGridInterface( void )
{
	int 		i;
	int 		j;
	int			page;
	int 		sel;

	// clear out old values
	j = botSelectInfo.page * botSelectInfo.maxBotsPerPage;
	for( i = 0; i < MAX_GRIDMODELSPERPAGE; i++, j++) {
		botSelectInfo.picbuttons[i].generic.flags &= ~QMF_HIDDEN;

		if( j < botSelectInfo.numBots ) {
			botSelectInfo.picbuttons[i].generic.flags &= ~QMF_INACTIVE;
			botSelectInfo.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
		}
		else
		{
			// dead control
			botSelectInfo.picbuttons[i].generic.flags |= QMF_INACTIVE;
		}

		botSelectInfo.picbuttons[i].generic.flags       &= ~QMF_HIGHLIGHT;
	}

	// set selected model(s), if visible
	if (botSelectInfo.multisel.curvalue)
	{
		for (i = 0; i < botSelectInfo.numMultiSel; i++)
		{
			sel = botSelectInfo.multiSel[i];
			page = sel / botSelectInfo.maxBotsPerPage;
			if (botSelectInfo.page != page)
				continue;

			sel %= botSelectInfo.maxBotsPerPage;
			botSelectInfo.picbuttons[sel].generic.flags |= QMF_HIGHLIGHT;
			botSelectInfo.picbuttons[sel].generic.flags &= ~QMF_PULSEIFFOCUS;
		}
	}
	else {
		if (botSelectInfo.selectedbot == -1)
			return;

		page = botSelectInfo.selectedbot / botSelectInfo.maxBotsPerPage;
		if (botSelectInfo.page == page)
		{
			i = botSelectInfo.selectedbot % botSelectInfo.maxBotsPerPage;
			botSelectInfo.picbuttons[i].generic.flags |= QMF_HIGHLIGHT;
			botSelectInfo.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		}
	}
}




/*
=================
UI_BotSelect_CheckAcceptButton
=================
*/
static void UI_BotSelect_CheckAcceptButton( void )
{
	qboolean enable;

	enable = qfalse;
	if (botSelectInfo.multisel.curvalue) {
		if (botSelectInfo.numMultiSel > 0)
			enable = qtrue;
	}
	else {
		if (botSelectInfo.selectedbot != -1)
			enable = qtrue;
	}

	if (enable) {
		botSelectInfo.go.generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
	}
	else {
		botSelectInfo.go.generic.flags |= (QMF_INACTIVE|QMF_GRAYED);
	}
}



/*
=================
UI_BotSelect_UpdateView

The display acts as a "window" on the list of all bots, and
only ever contains one screens worth of information
=================
*/
static void UI_BotSelect_UpdateView( void )
{
	const char	*info;
	int			i;
	int			j;
	int			pageBotCount;

	j = botSelectInfo.page * botSelectInfo.maxBotsPerPage;
	for( i = 0; i < botSelectInfo.maxBotsPerPage; i++, j++) {
		if( j < botSelectInfo.numBots ) {
			info = UI_GetBotInfoByNumber( botSelectInfo.sortedBotNums[j] );
			UI_ServerPlayerIcon( Info_ValueForKey( info, "model" ), botSelectInfo.boticons[i], MAX_QPATH );
			Q_strncpyz( botSelectInfo.botnames[i], Info_ValueForKey( info, "name" ), BOTNAME_LENGTH );
			Q_CleanStr( botSelectInfo.botnames[i] );

			if( botSelectInfo.index != -1 && StartServer_BotOnSelectionList( botSelectInfo.botnames[i] ) ) {
				botSelectInfo.botcolor[i] = color_white;
			}
			else {
				botSelectInfo.botcolor[i] = color_grey;
			}
		}
		else {
			// dead slot
			botSelectInfo.botnames[i][0] = 0;
		}
	}

	// update display details based on the view type
	pageBotCount = botSelectInfo.numBots - botSelectInfo.page * botSelectInfo.maxBotsPerPage;;
	if (pageBotCount > botSelectInfo.maxBotsPerPage)
		pageBotCount = botSelectInfo.maxBotsPerPage;

	if (!botSelectInfo.viewlist.curvalue) {
		// grid display
		UI_BotSelect_UpdateGridInterface();
    	botSelectInfo.botlist.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else {
		// list display

		for (i = 0; i < MAX_GRIDMODELSPERPAGE; i++)
		{
			botSelectInfo.picbuttons[i].generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
		}

		botSelectInfo.botlist.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
		if (UI_BotSelect_SelectedOnPage())
			botSelectInfo.botlist.curvalue = botSelectInfo.selectedbot % botSelectInfo.maxBotsPerPage;
		else
			botSelectInfo.botlist.curvalue = -1;
		botSelectInfo.botlist.numitems = pageBotCount;
	}

	// left/right controls
	if( botSelectInfo.numpages > 1 ) {
		if( botSelectInfo.page > 0 ) {
			botSelectInfo.left.generic.flags &= ~QMF_INACTIVE;
		}
		else {
			botSelectInfo.left.generic.flags |= QMF_INACTIVE;
		}

		if( botSelectInfo.page < (botSelectInfo.numpages - 1) ) {
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
UI_BotSelect_SetPageFromSelected
=================
*/
static void UI_BotSelect_SetPageFromSelected( void )
{
	if (botSelectInfo.selectedbot == -1 ) {
		botSelectInfo.page = 0;
		return;
	}
	botSelectInfo.page = botSelectInfo.selectedbot / botSelectInfo.maxBotsPerPage;
}



/*
=================
UI_BotSelect_Default
=================
*/
static void UI_BotSelect_Default( char *bot ) {
	const char	*botInfo;
	const char	*test;
	int			n;
	int			i;

	botSelectInfo.selectedbot = -1;
	botSelectInfo.page = 0;
	botSelectInfo.numMultiSel = 0;

	for( n = 0; n < botSelectInfo.numBots; n++ ) {
		botInfo = UI_GetBotInfoByNumber( n );
		test = Info_ValueForKey( botInfo, "name" );
		if( Q_stricmp( bot, test ) == 0 ) {
			break;
		}
	}

	// bot name not in list
	if( n == botSelectInfo.numBots ) {
		return;
	}

	// find in sorted list
	for( i = 0; i < botSelectInfo.numBots; i++ ) {
		if( botSelectInfo.sortedBotNums[i] == n ) {
			break;
		}
	}

	// not in sorted list
	if( i == botSelectInfo.numBots ) {
		return;
	}

	// found it!
	UI_BotSelect_AddBotSelection(i);
	UI_BotSelect_SetPageFromSelected();
}





/*
=================
UI_BotSelect_BotEvent
=================
*/
static void UI_BotSelect_BotEvent( void* ptr, int event ) {
	int		i;

	if( event != QM_ACTIVATED ) {
		return;
	}

	i = ((menucommon_s*)ptr)->id;
	i += botSelectInfo.page * botSelectInfo.maxBotsPerPage;

	UI_BotSelect_AddBotSelection(i);
	UI_BotSelect_CheckAcceptButton();
	UI_BotSelect_UpdateGridInterface();
}





/*
=================
UI_BotSelect_Event
=================
*/
static void UI_BotSelect_Event( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch (((menucommon_s*)ptr)->id) {
	case ID_BOTSELECT_VIEWLIST:
		UI_BotSelect_SetViewType();
		UI_BotSelect_SetPageFromSelected();
		UI_BotSelect_UpdateView();
		break;

	case ID_BOTSELECT_MULTISEL:
		UI_BotSelect_ToggleMultiSelect();
		UI_BotSelect_UpdateView();
		break;

	case ID_BOTSELECT_LEFT:
		if( botSelectInfo.page > 0 ) {
			botSelectInfo.page--;
			UI_BotSelect_UpdateView();
		}
		break;

	case ID_BOTSELECT_RIGHT:
		if( botSelectInfo.page < botSelectInfo.numpages - 1 ) {
			botSelectInfo.page++;
			UI_BotSelect_UpdateView();
		}
		break;

	case ID_BOTSELECT_ACCEPT:
		UI_PopMenu();

		if (botSelectInfo.index != -1)
			UI_BotSelect_SetBotInfoInCaller();
		break;

	case ID_BOTSELECT_BACK:
		UI_PopMenu();
		break;
	}
}



/*
=================
UI_BotSelect_Cache
=================
*/
void UI_BotSelect_Cache( void ) {
	trap_R_RegisterShaderNoMip( BOTSELECT_BACK0 );
	trap_R_RegisterShaderNoMip( BOTSELECT_BACK1 );
	trap_R_RegisterShaderNoMip( BOTSELECT_ACCEPT0 );
	trap_R_RegisterShaderNoMip( BOTSELECT_ACCEPT1 );
	trap_R_RegisterShaderNoMip( BOTSELECT_SELECT );
	trap_R_RegisterShaderNoMip( BOTSELECT_SELECTED );
	trap_R_RegisterShaderNoMip( BOTSELECT_ARROWS );
	trap_R_RegisterShaderNoMip( BOTSELECT_ARROWSL );
	trap_R_RegisterShaderNoMip( BOTSELECT_ARROWSR );
	trap_R_RegisterShaderNoMip( BOTSELECT_SMALLSELECTED);
}





/*
=================
UI_BotSelect_ScrollList_LineSize
=================
*/
static void UI_BotSelect_ScrollList_LineSize( int* charheight, int* charwidth, int* lineheight )
{
	float	scale;

	scale = UI_ProportionalSizeScale(UI_SMALLFONT, 0);

	*charwidth = scale * UI_ProportionalStringWidth("X");
	*charheight = scale * PROP_HEIGHT;

	if (*charheight > BOTLIST_ICONSIZE)
		*lineheight = *charheight + 2;
	else
		*lineheight = BOTLIST_ICONSIZE + 2;
}



/*
=================
UI_BotSelect_HandleListKey

Returns true if the list box accepts that key input
Implements the list box input, but with larger proportional fonts 
=================
*/
static qboolean UI_BotSelect_HandleListKey( int key, sfxHandle_t* psfx)
{
	menulist_s* l;
	int	x;
	int	y;
	int	w;
	int	cursorx;
	int	cursory;
	int	column;
	int	index;
	int charwidth;
	int charheight;
	int lineheight;
	int	sel;

	UI_BotSelect_ScrollList_LineSize( &charheight, &charwidth, &lineheight );

	l = &botSelectInfo.botlist;

	switch (key) {
	case K_MOUSE1:
		if (l->generic.flags & QMF_HASMOUSEFOCUS)
		{
			// check scroll region
			x = l->generic.x;
			y = l->generic.y;
			w = ( (l->width + l->seperation) * l->columns - l->seperation) * charwidth;
			if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
				x -= w / 2;
			}
			if (UI_CursorInRect( x, y, w, l->height*lineheight ))
			{
				cursorx = (uis.cursorx - x)/charwidth;
				column = cursorx / (l->width + l->seperation);
				cursory = (uis.cursory - y)/lineheight;
				index = column * l->height + cursory;
				if (l->top + index < l->numitems)
				{
					l->oldvalue = l->curvalue;
					l->curvalue = l->top + index;

					if (l->oldvalue != l->curvalue)
					{
						if (l->generic.callback) {
							l->generic.callback( l, QM_GOTFOCUS );
						}
					}
					sel = botSelectInfo.page * botSelectInfo.maxBotsPerPage;
					sel += l->curvalue;
					UI_BotSelect_AddBotSelection(sel);
					UI_BotSelect_CheckAcceptButton();
					*psfx = (menu_move_sound);
				}
			}
			else {
				// absorbed, silent sound effect
				*psfx = (menu_null_sound);
			}
		}
		return qtrue;

	// keys that have the default action	
	case K_ESCAPE:
		*psfx = Menu_DefaultKey( &botSelectInfo.menu, key );
		return qtrue;	
	}
	return qfalse;
}


/*
=================
UI_BotSelect_Key
=================
*/
static sfxHandle_t UI_BotSelect_Key( int key )
{
	menulist_s	*l;
	sfxHandle_t sfx;

	l = (menulist_s	*)Menu_ItemAtCursor( &botSelectInfo.menu );

	sfx = menu_null_sound;
	if( l == &botSelectInfo.botlist) {
		if (!UI_BotSelect_HandleListKey(key, &sfx))
			return menu_buzz_sound;
	}
	else {
		sfx = Menu_DefaultKey( &botSelectInfo.menu, key );
	}

	return sfx;
}




/*
=================
UI_BotSelect_ScrollList_Init
=================
*/
static void UI_BotSelect_ScrollList_Init( menulist_s *l )
{
	int		w;
	int 	charwidth;
	int 	charheight;
	int 	lineheight;

	l->oldvalue = 0;
	l->curvalue = 0;
	l->top      = 0;

	UI_BotSelect_ScrollList_LineSize(&charheight, &charwidth, &lineheight);

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
UI_BotSelect_ScrollList_Draw
=================
*/
static void UI_BotSelect_ScrollListDraw( void* ptr )
{
	int			x;
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
	int		bot;
	qboolean selected;

	UI_BotSelect_ScrollList_LineSize(&charheight, &charwidth, &lineheight);

	l = (menulist_s*)ptr;
	hasfocus = (l->generic.parent->cursor == l->generic.menuPosition);

	x =	l->generic.x;
	for( column = 0; column < l->columns; column++ ) {
		y =	l->generic.y;
		base = l->top + column * l->height;
		for( i = base; i < base + l->height; i++) {
			if (i >= l->numitems)
				break;

			style = UI_SMALLFONT;
			color = botSelectInfo.botcolor[i];
			if (i == l->curvalue)
			{
				UI_FillRect(x,y + (lineheight - BOTLIST_ICONSIZE)/2,
					l->width*charwidth ,BOTLIST_ICONSIZE, color_select_bluo);
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

			index = -1;
			selected = qfalse;
			bot = i + botSelectInfo.page * botSelectInfo.maxBotsPerPage;
			if (botSelectInfo.multisel.curvalue) {
				for (j = 0; j < botSelectInfo.numMultiSel; j++) {
					if (botSelectInfo.multiSel[j] == bot) {
						index = j + 1;
						selected = qtrue;
						break;
					}
				}
			}
			else {
				if (botSelectInfo.selectedbot == bot)
					selected = qtrue;
			}

			trap_R_SetColor(transparent_color);
			UI_DrawNamedPic(x, y + (lineheight - BOTLIST_ICONSIZE)/2,
				BOTLIST_ICONSIZE, BOTLIST_ICONSIZE, botSelectInfo.boticons[i]);
			trap_R_SetColor(NULL);

			if (selected) {
				trap_R_SetColor( colorRed );
				UI_DrawNamedPic(x, y + (lineheight - BOTLIST_ICONSIZE)/2,
					BOTLIST_ICONSIZE , BOTLIST_ICONSIZE , BOTSELECT_SMALLSELECTED);
				trap_R_SetColor( NULL );
			}

			if (index != -1) {
				UI_DrawString( x + BOTLIST_ICONSIZE/2, y + (lineheight - BIGCHAR_HEIGHT)/2,
					va("%i", index), UI_CENTER|UI_DROPSHADOW, color_white);
			}

			UI_DrawString( x + BOTLIST_ICONSIZE + 2, y + (lineheight - charheight)/2, l->itemnames[i], style, color);

			y += lineheight;
		}
		x += (l->width + l->seperation) * charwidth;
	}
}




/*
=================
UI_BotSelect_BotGridDraw
=================
*/
static void UI_BotSelect_BotGridDraw( void* ptr )
{
	float	x;
	float	y;
	float	w;
	float	h;
	vec4_t	tempcolor;
	float*	color;
	int 	index;
	menubitmap_s* b;
	int 	i;
	int 	bot;

	b = (menubitmap_s*)ptr;

	//
	// draw bot icon
	//
	index = b->generic.id;
	x = b->generic.left;
	y = b->generic.top;
	w = b->generic.right - x;
	h = b->generic.bottom - y;

	if (botSelectInfo.botnames[index][0]) {
		UI_DrawNamedPic(x, y, w, h, botSelectInfo.boticons[index]);
		if (b->generic.flags & QMF_HIGHLIGHT) {
			trap_R_SetColor( color_white );
			UI_DrawNamedPic(x, y, w, h, BOTSELECT_SELECTED);
			trap_R_SetColor( NULL );
		}
	}

	//
	// draw bot position in multi
	//

	if (botSelectInfo.multisel.curvalue) {
		bot = index + botSelectInfo.page * botSelectInfo.maxBotsPerPage;
		for (i = 0; i < botSelectInfo.numMultiSel; i++) {
			if (botSelectInfo.multiSel[i] != bot)
				continue;

			UI_DrawString( x + w/2, y + (h - GIANTCHAR_HEIGHT)/2, va("%i", i + 1),
				UI_CENTER|UI_GIANTFONT|UI_DROPSHADOW, color_white);
			break;
		}
	}

	//
	// draw bot name text
	//

	if (botSelectInfo.botnames[index][0])
		UI_DrawString( x + 32, y + 64, botSelectInfo.botnames[index], UI_CENTER|UI_SMALLFONT, botSelectInfo.botcolor[index]);

	//
	// draws pulse shader showing mouse over
	//

	// no pulse shader required
	if (b->generic.flags & QMF_HIGHLIGHT)
		return;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;

	if (b->generic.flags & QMF_RIGHT_JUSTIFY)
	{
		x = x - w;
	}
	else if (b->generic.flags & QMF_CENTER_JUSTIFY)
	{
		x = x - w/2;
	}

	// used to refresh shader
	if (b->generic.name && !b->shader)
	{
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if (!b->shader && b->errorpic)
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
	}

	if (b->focuspic && !b->focusshader)
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );

	if (b->generic.flags & QMF_GRAYED)
	{
		if (b->shader)
		{
			trap_R_SetColor( colorMdGrey );
			UI_DrawHandlePic( x, y, w, h, b->shader );
			trap_R_SetColor( NULL );
		}
	}
	else
	{
		if (b->shader)
			UI_DrawHandlePic( x, y, w, h, b->shader );

		if ((b->generic.flags & QMF_PULSE) || (b->generic.flags & QMF_PULSEIFFOCUS) && (Menu_ItemAtCursor( b->generic.parent ) == b))
		{
			if (b->focuscolor)
			{
				tempcolor[0] = b->focuscolor[0];
				tempcolor[1] = b->focuscolor[1];
				tempcolor[2] = b->focuscolor[2];
				color        = tempcolor;
			}
			else
				color = pulse_color;
			color[3] = 0.5+0.5*sin(uis.realtime/PULSE_DIVISOR);

			trap_R_SetColor( color );
			UI_DrawHandlePic( x, y, w, h, b->focusshader );
			trap_R_SetColor( NULL );
		}
		else if ((b->generic.flags & QMF_HIGHLIGHT) || ((b->generic.flags & QMF_HIGHLIGHT_IF_FOCUS) && (Menu_ItemAtCursor( b->generic.parent ) == b)))
		{
			if (b->focuscolor)
			{
				trap_R_SetColor( b->focuscolor );
				UI_DrawHandlePic( x, y, w, h, b->focusshader );
				trap_R_SetColor( NULL );
			}
			else
				UI_DrawHandlePic( x, y, w, h, b->focusshader );
		}
	}
}




/*
=================
UI_BotSelect_MenuDraw
=================
*/
static void UI_BotSelect_MenuDraw(void)
{
//	UI_DrawString(0,0,va("%i",botSelectInfo.selectedbot), UI_SMALLFONT, color_white);

	// draw the controls
	Menu_Draw(&botSelectInfo.menu);
}




/*
=================
UI_BotSelect_Init
=================
*/
static void UI_BotSelect_Init( char *bot , int index) {
	int		i, j, k;
	int		x, y;

	memset( &botSelectInfo, 0 ,sizeof(botSelectInfo) );
	botSelectInfo.menu.key = UI_BotSelect_Key;
	botSelectInfo.menu.wrapAround = qtrue;
	botSelectInfo.menu.native 	= qfalse;
	botSelectInfo.menu.fullscreen = qtrue;
	botSelectInfo.menu.draw = UI_BotSelect_MenuDraw;

	UI_BotSelect_Cache();

	botSelectInfo.index = index;
	botSelectInfo.numMultiSel = 0;

	botSelectInfo.banner.generic.type	= MTYPE_BTEXT;
	botSelectInfo.banner.generic.x		= 320;
	botSelectInfo.banner.generic.y		= 16;
	botSelectInfo.banner.color			= color_white;
	botSelectInfo.banner.style			= UI_CENTER;

	botSelectInfo.viewlist.generic.type = MTYPE_RADIOBUTTON;
	botSelectInfo.viewlist.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	botSelectInfo.viewlist.generic.id = ID_BOTSELECT_VIEWLIST;
	botSelectInfo.viewlist.generic.callback = UI_BotSelect_Event;
	botSelectInfo.viewlist.generic.x = 290 - 13* SMALLCHAR_WIDTH;
	botSelectInfo.viewlist.generic.y = 480 - 2*LINE_HEIGHT;

	botSelectInfo.multisel.generic.type = MTYPE_RADIOBUTTON;
	botSelectInfo.multisel.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	botSelectInfo.multisel.generic.id = ID_BOTSELECT_MULTISEL;
	botSelectInfo.multisel.generic.callback = UI_BotSelect_Event;
	botSelectInfo.multisel.generic.x = 290 - 13* SMALLCHAR_WIDTH;
	botSelectInfo.multisel.generic.y = 480 - 3*LINE_HEIGHT;

	// init based on previous value
	botSelectInfo.viewlist.curvalue = (int)Com_Clamp(0, 1, trap_Cvar_VariableValue("uie_bot_list"));
	botSelectInfo.multisel.curvalue = (int)Com_Clamp(0, 1, trap_Cvar_VariableValue("uie_bot_multisel"));

	for (i = 0; i < MAX_MODELSPERPAGE; i++)
		botSelectInfo.botalias[i] = botSelectInfo.botnames[i];

	botSelectInfo.botlist.generic.type = MTYPE_SCROLLLIST;
	botSelectInfo.botlist.generic.flags = QMF_PULSEIFFOCUS|QMF_NODEFAULTINIT;
	botSelectInfo.botlist.generic.x = 21;
	botSelectInfo.botlist.generic.y = 60;
	botSelectInfo.botlist.generic.ownerdraw = UI_BotSelect_ScrollListDraw;
	botSelectInfo.botlist.columns = BOTLIST_COLS;
	botSelectInfo.botlist.seperation = 2;
	botSelectInfo.botlist.height = BOTLIST_ROWS;
	botSelectInfo.botlist.width = 14;
	botSelectInfo.botlist.itemnames = botSelectInfo.botalias;

	UI_BotSelect_ScrollList_Init(&botSelectInfo.botlist);

	y =	80;
	for( i = 0, k = 0; i < BOTGRID_ROWS; i++) {
		x =	180;
		for( j = 0; j < BOTGRID_COLS; j++, k++ ) {
			botSelectInfo.picbuttons[k].generic.type		= MTYPE_BITMAP;
			botSelectInfo.picbuttons[k].generic.flags		= QMF_LEFT_JUSTIFY|QMF_NODEFAULTINIT|QMF_PULSEIFFOCUS;
			botSelectInfo.picbuttons[k].generic.callback	= UI_BotSelect_BotEvent;
			botSelectInfo.picbuttons[k].generic.ownerdraw	= UI_BotSelect_BotGridDraw;
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
	botSelectInfo.left.generic.callback		= UI_BotSelect_Event;
	botSelectInfo.left.generic.id = ID_BOTSELECT_LEFT;
	botSelectInfo.left.generic.x			= 260;
	botSelectInfo.left.generic.y			= 440;
	botSelectInfo.left.width  				= 64;
	botSelectInfo.left.height  				= 32;
	botSelectInfo.left.focuspic				= BOTSELECT_ARROWSL;

	botSelectInfo.right.generic.type	    = MTYPE_BITMAP;
	botSelectInfo.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.right.generic.callback	= UI_BotSelect_Event;
	botSelectInfo.right.generic.id = ID_BOTSELECT_RIGHT;
	botSelectInfo.right.generic.x			= 324;
	botSelectInfo.right.generic.y			= 440;
	botSelectInfo.right.width  				= 64;
	botSelectInfo.right.height  		    = 32;
	botSelectInfo.right.focuspic			= BOTSELECT_ARROWSR;

	botSelectInfo.back.generic.type		= MTYPE_BITMAP;
	botSelectInfo.back.generic.name		= BOTSELECT_BACK0;
	botSelectInfo.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.back.generic.callback	= UI_BotSelect_Event;
	botSelectInfo.back.generic.id = ID_BOTSELECT_BACK;
	botSelectInfo.back.generic.x		= 0 - uis.wideoffset;
	botSelectInfo.back.generic.y		= 480-64;
	botSelectInfo.back.width			= 128;
	botSelectInfo.back.height			= 64;
	botSelectInfo.back.focuspic			= BOTSELECT_BACK1;

	botSelectInfo.go.generic.type		= MTYPE_BITMAP;
	botSelectInfo.go.generic.name		= BOTSELECT_ACCEPT0;
	botSelectInfo.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	botSelectInfo.go.generic.callback	= UI_BotSelect_Event;
	botSelectInfo.go.generic.id = ID_BOTSELECT_ACCEPT;
	botSelectInfo.go.generic.x			= 640 + uis.wideoffset;
	botSelectInfo.go.generic.y			= 480-64;
	botSelectInfo.go.width				= 128;
	botSelectInfo.go.height				= 64;
	botSelectInfo.go.focuspic			= BOTSELECT_ACCEPT1;
	
	if(cl_language.integer == 0){
	botSelectInfo.banner.string			= "SELECT BOT";
	botSelectInfo.viewlist.generic.name = "View list:";
	botSelectInfo.multisel.generic.name = "Multi-select:";
	}
	if(cl_language.integer == 1){
	botSelectInfo.banner.string			= "ВЫБОР БОТА";
	botSelectInfo.viewlist.generic.name = "Список:";
	botSelectInfo.multisel.generic.name = "Мульти-выбор:";
	}

	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.banner );
	for( i = 0; i < MAX_GRIDMODELSPERPAGE; i++ ) {
		Menu_AddItem( &botSelectInfo.menu,	&botSelectInfo.picbuttons[i] );
	}

	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.arrows );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.left );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.right );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.back );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.go );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.viewlist );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.multisel );
	Menu_AddItem( &botSelectInfo.menu, &botSelectInfo.botlist );

	UI_BotSelect_BuildList();
	UI_BotSelect_SetViewType();

	UI_BotSelect_Default( bot );
	UI_BotSelect_UpdateView();
	UI_BotSelect_CheckAcceptButton();
}



/*
=================
UI_BotSelectMenu
=================
*/
void UI_BotSelect_Index( char *bot , int index) {
	UI_BotSelect_Init( bot, index );
	UI_PushMenu( &botSelectInfo.menu );
}

/*
=================
UI_BotSelectMenu
=================
*/
void UI_BotSelectMenu( char *bot ) {
	UI_BotSelect_Init( bot, -1 );
	UI_PushMenu( &botSelectInfo.menu );
}





