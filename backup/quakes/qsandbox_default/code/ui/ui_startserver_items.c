/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/


#include "ui_local.h"
#include "ui_startserver_q3.h"


/*
	This menu is slightly unusual. It can be either a part of the setup
	for a map/bot rotation, or it can be called in-game to adjust the
	parameters for the next map.

	*_InGame_* function are only used in the ingame menu
	*_ItemPage_* functions are only used in the skirmish menu
	*_BothItemMenus_* functions... 
*/

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))


#define INGAME_CANCEL0 	"menu/art/back_0"
#define INGAME_CANCEL1 	"menu/art/back_1"
#define INGAME_ACCEPT0 	"menu/art/accept_0"
#define INGAME_ACCEPT1 	"menu/art/accept_1"

#define SERVER_ITEM_RESET0 	"menu/art/reset_0"
#define SERVER_ITEM_RESET1 	"menu/art/reset_1"
//#define SERVER_ITEM_CLEAR0 	"menu/uie_art/clear_0"
//#define SERVER_ITEM_CLEAR1 	"menu/uie_art/clear_1"


#define ID_ITEM_GAMETYPE 	401
#define ID_ITEM_RESET		402
#define ID_ITEM_CLEAR		403

#define ID_ITEMINGAME_CANCEL 	430
#define ID_ITEMINGAME_ACCEPT 	431

#define ITEMCONTROL_X (320 + 4*SMALLCHAR_WIDTH)



//
// controls
//


#define MAX_ITEM_ONPAGE 16


// a single item, and related data/controls
typedef struct iteminfo_s {
	char* name;
	char* runame;
	int ident;	// ITEM_* flag

	itemnode_t* item;
	menuradiobutton_s* control;
} iteminfo_t;




typedef struct itemradionbutton_s {
	menuradiobutton_s control;
	iteminfo_t* item;
	int bg_index;	// bg_items[] index
} itemradiobutton_t;




// controls that appear when used as an in-game menu
typedef struct ingame_controls_t {
	menutext_s		title;
	menubitmap_s	cancel;
	menubitmap_s	accept;
	menutext_s		info;
} ingame_controls_s;



// and finally the page controls themselves
typedef struct itemcontrols_s {
	menuframework_s 	menu;
	commoncontrols_t 	common;
	ingame_controls_s	ingame;

	menulist_s 		gameType;
	menubitmap_s 	gameTypeIcon;

	menulist_s 		groupMaster[ITEMGROUP_COUNT];	// don't use ITEMGROUP_* to index
	menutext_s		tabbedText[ITEMGROUP_COUNT];	// selection text for a page of items

	itemradiobutton_t itemCtrl[MAX_ITEM_ONPAGE]; // don't use ITEM_* to index

	menubitmap_s	reset;

	qboolean ingame_menu;	// menu display format

	int currentPage;
	int numPageItems;

	int base_master_y;	// height of master controls
	int mid_tabbed_y;	// height of middle of tabbed controls

	// state of the controls
	// this abstraction "aliases" or "hides" where the data is stored,
	// and helps us separate data associated with the script from
	// data associated with the in-game version of this menu

	// array size assumed as ITEM_COUNT
	// indexed by ITEM_*
	qboolean* enabled;

	// array size assumed as ITEMGROUP_COUNT
	// indexed by ITEMGROUP_*
	int* grouptype;
} itemcontrols_t;



static itemcontrols_t s_itemcontrols;


//
// data storage used during ingame menu display
// you shouldn't need to read/write these values directly
//
static qboolean ingame_enabled[ITEM_COUNT];
static int ingame_grouptype[ITEMGROUP_COUNT];



// enum used to distinguish how group controls are updated
// arguments to StartServer_BothItemMenus_SetGroupControl()
enum {
	IGROUP_FLUSHCUSTOM,
	IGROUP_PRESERVECUSTOM
};



//
// data lists
//


// WEAPONS   ------------------------------------------

static iteminfo_t weaponitems_list[] =
{
	{"Hook:", "Крюк:", ITEM_GRAPPLING_HOOK, NULL},
	{"Machinegun:", "Автомат:", ITEM_MGUN, NULL},
	{"Shotgun:", "Дробовик:", ITEM_SHOTGUN, NULL},
	{"Grenade:", "Гранатомёт:", ITEM_GRENADE, NULL},
	{"Rocket:", "Ракетница:", ITEM_ROCKET, NULL},
	{"Plasma:", "Плазмаган:", ITEM_PLASMA, NULL},
	{"Lightning:", "Молния:", ITEM_LIGHTNING, NULL},
	{"Railgun:", "Рэйлган:", ITEM_RAIL, NULL},
	{"BFG10k:", "БФГ:", ITEM_BFG, NULL},
	{"Nailgun:", "Гвоздомёт:", ITEM_NAILGUN, NULL},
	{"Prox Launcher:", "Миномёт:", ITEM_PROX_LAUNCHER, NULL},
	{"Chaingun:", "Пулемёт:", ITEM_CHAINGUN, NULL},
	{"Flamethrower:", "Огнемёт:", ITEM_FLAMETHROWER, NULL},
	{"Dark Flare:", "Темная вспышка:", ITEM_ANTIMATTER, NULL}
};
static iteminfo_t weaponammo_list[] =
{
	{"Bullets:", "Пули:", ITEM_MGUN_AMMO, NULL},
	{"Shells:", "Дробь:", ITEM_SHOTGUN_AMMO, NULL},
	{"Grenades:", "Гранаты:", ITEM_GRENADE_AMMO, NULL},
	{"Rockets:", "Ракеты:", ITEM_ROCKET_AMMO, NULL},
	{"Cells:", "Плазма:", ITEM_PLASMA_AMMO, NULL},
	{"Lightning:", "Заряд:", ITEM_LIGHTNING_AMMO, NULL},
	{"Slugs:", "Лучи:", ITEM_RAIL_AMMO, NULL},
	{"BFG Ammo:", "БФГ Заряд:", ITEM_BFG_AMMO, NULL},
	{"Nails:", "Гвозди:", ITEM_NAILGUN_AMMO, NULL},
	{"Proximity Mines:", "Мины:", ITEM_PROX_LAUNCHER_AMMO, NULL},
	{"Chaingun Belt:", "Пулеметные пули:", ITEM_CHAINGUN_AMMO, NULL},
	{"Flame:", "Напалм:", ITEM_FLAMETHROWER_AMMO, NULL}
};

// HEALTH    ------------------------------------------
static iteminfo_t healthitems_list[] =
{
	{"Mega:", "Мега:", ITEM_MEGA, NULL},
	{"Green +5:", "Зеленая:", ITEM_HEALTH_SMALL, NULL},
	{"Yellow +25:", "Жёлтая:", ITEM_HEALTH_MEDIUM, NULL},
	{"Gold +50:", "Золотая:", ITEM_HEALTH_LARGE, NULL}
};



// ARMOUR    ------------------------------------------
static iteminfo_t armouritems_list[] =
{
	{"Purple +200:", "Фиолетовая +200:", ITEM_ARMOUR_PURPLE, NULL},
	{"Red +100:", "Красная +200:", ITEM_ARMOUR_RED, NULL},
	{"Yellow +50:", "Жёлтая +200:", ITEM_ARMOUR_YELLOW, NULL},
	{"Green +25:", "Зеленая +200:", ITEM_ARMOUR_GREEN, NULL},
	{"Shard +5:", "Кусок +200:", ITEM_ARMOUR_SHARD, NULL}
};


// POWERUPS  ------------------------------------------
static iteminfo_t powerupitems_list[] =
{
	{"Quad:", "Квад:", ITEM_QUAD, NULL},
	{"Regen:", "Реген:", ITEM_REGEN, NULL},
	{"Haste:", "Скорость:", ITEM_HASTE, NULL},
	{"Battlesuit:", "Боевой щит:", ITEM_BATTLESUIT, NULL},
	{"Invisible:", "Невидимость:", ITEM_INVISIBLE, NULL},
	{"Flight:", "Полет:", ITEM_FLIGHT, NULL},
	{"Scout:", "Скаут:", ITEM_SCOUT, NULL},
	{"Guard:", "Защитник:", ITEM_GUARD, NULL},
	{"Doubler:", "Удвоитель:", ITEM_DOUBLER, NULL},
	{"AmmoRegen:", "Реген пуль:", ITEM_AMMOREGEN, NULL}
};


// HOLDABLE  ------------------------------------------
static iteminfo_t holdableitems_list[] =
{
	{"Medkit:", "Аптечка:", ITEM_MEDKIT, NULL},
	{"Teleporter:", "Телепортер:", ITEM_TELEPORTER, NULL},
	{"Kamikaze:", "Камикадзе:", ITEM_KAMIKAZE, NULL},
	{"Invulnerability:", "Оболочка:", ITEM_INVULNERABILITY, NULL},
	{"Portal:", "Портал:", ITEM_PORTAL, NULL}
};



// GROUP LAYOUT ------------------------------------------


typedef struct mastercontrol_s {
	int ident;
	char* masterTitle;
	char* tabbedTitle;
	char* masterTitleru;
	char* tabbedTitleru;
	menulist_s* control;
	iteminfo_t* items;
	int item_count;
} mastercontrol_t;



static mastercontrol_t masterControl[ITEMGROUP_COUNT] = {
	{ ITEMGROUP_WEAPON, "Weapon:", "Weapons", "Оружие:", "Оружие", NULL, weaponitems_list, ARRAY_COUNT(weaponitems_list) },
	{ ITEMGROUP_AMMO, "Ammo:", "Ammo", "Пули:", "Пули", NULL, weaponammo_list, ARRAY_COUNT(weaponammo_list) },
	{ ITEMGROUP_HEALTH, "Health:", "Health", "Жизни:", "Жизни", NULL, healthitems_list, ARRAY_COUNT(healthitems_list) },
	{ ITEMGROUP_ARMOUR, "Armour:", "Armour", "Броня:", "Броня", NULL, armouritems_list, ARRAY_COUNT(armouritems_list) },
	{ ITEMGROUP_POWERUPS, "Powerups:", "Powerups", "Бонус:", "Бонусы", NULL, powerupitems_list, ARRAY_COUNT(powerupitems_list) },
	{ ITEMGROUP_HOLDABLE, "Holdables:", "Holdables", "Предмет:", "Предметы", NULL, holdableitems_list, ARRAY_COUNT(holdableitems_list) }
};

static const char* allgroups_items[] = {
	"All",	// ALLGROUPS_ENABLED
	"Custom",	// ALLGROUPS_CUSTOM
	"Hidden",	// ALLGROUPS_HIDDEN
	0
};

static const char* allgroups_itemsru[] = {
	"Все",	// ALLGROUPS_ENABLED
	"Настр",	// ALLGROUPS_CUSTOM
	"Скрыть",	// ALLGROUPS_HIDDEN
	0
};




//------------------------------------------------------------------

// Most functions common to both types a page
// The remaining functions can be found at the end of the file

//------------------------------------------------------------------



/*
=================
StartServer_ItemPage_GetItemFromMapObject
=================
*/
static int StartServer_GetItemFromMapObject(const char* ident)
{
	int i;

	for (i = 0; i < bg_numItems; i++)
	{
		if (!Q_stricmp(ident, bg_itemlist[i].classname))
			return i;
	}

	Com_Printf("bg_itemlist, unknown type: %s\n", ident);
	return 0;
}






/*
=================
StartServer_BothItemMenus_Cache
=================
*/
void StartServer_BothItemMenus_Cache( void )
{
	if (s_itemcontrols.ingame_menu)
	{
		trap_R_RegisterShaderNoMip( INGAME_CANCEL0 );
		trap_R_RegisterShaderNoMip( INGAME_CANCEL1 );
		trap_R_RegisterShaderNoMip( INGAME_ACCEPT0 );
		trap_R_RegisterShaderNoMip( INGAME_ACCEPT1 );
	}

	trap_R_RegisterShaderNoMip( SERVER_ITEM_RESET0 );
	trap_R_RegisterShaderNoMip( SERVER_ITEM_RESET1 );
//	trap_R_RegisterShaderNoMip( SERVER_ITEM_CLEAR0 );
//	trap_R_RegisterShaderNoMip( SERVER_ITEM_CLEAR1 );
}



/*
=================
StartServer_BothItemMenus_SetTabbedCtrlValues
=================
*/
static void StartServer_BothItemMenus_SetTabbedCtrlValues( void )
{
	int page;
	qboolean 	enabled;
	int 		grouptype;
	iteminfo_t*	item;
	int i;

	page = s_itemcontrols.currentPage;

	enabled = qtrue;
	grouptype = s_itemcontrols.grouptype[ page ];
	if (grouptype == ALLGROUPS_HIDDEN)
		enabled = qfalse;

	for (i = 0; i < s_itemcontrols.numPageItems; i++)
	{
		item = &masterControl[page].items[i];
		if (grouptype == ALLGROUPS_CUSTOM) {
			enabled = s_itemcontrols.enabled[ item->ident ];
		}

		item->control->curvalue = enabled;
	}
}


/*
=================
StartServer_BothItemMenus_InitControls
=================
*/
static void StartServer_BothItemMenus_InitControls( void )
{
	int 		i;

	// initialize the master controls
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		masterControl[i].control->curvalue = s_itemcontrols.grouptype[ masterControl[i].ident ];
	}

	// initialize current group of items
	StartServer_BothItemMenus_SetTabbedCtrlValues();
}




/*
=================
StartServer_BothItemMenus_ResetAll

Sets all item controls on/off and group settings
become custom
=================
*/
static void StartServer_BothItemMenus_ResetAll( qboolean enabled)
{
	int i;

	for (i = 0; i < ITEM_COUNT; i++)
	{
		s_itemcontrols.enabled[i] = enabled;
	}

	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		s_itemcontrols.grouptype[i] = ALLGROUPS_CUSTOM;
	}

	StartServer_BothItemMenus_InitControls();
}



/*
=================
StartServer_BothItemMenus_FindGroupType
=================
*/
static int StartServer_BothItemMenus_FindGroupType( iteminfo_t* list, int size )
{
	int grouptype;
	qboolean enabled;
	int i;

	if (size == 0)
		return ALLGROUPS_ENABLED;

	grouptype = ALLGROUPS_ENABLED;
	enabled = s_itemcontrols.enabled[ list[0].ident ];
	if (!enabled)
		grouptype = ALLGROUPS_HIDDEN;

	for (i = 0; i < size; i++)
	{
		if (enabled != s_itemcontrols.enabled[ list[i].ident ]) {
			return ALLGROUPS_CUSTOM;
		}
	}

	return grouptype;
}




/*
=================
StartServer_BothItemMenus_UpdateInterface
=================
*/
static void StartServer_BothItemMenus_UpdateInterface(void)
{
	int i;
	int page, type;
	menuradiobutton_s* ctrl;
	iteminfo_t* item;

	// menu type specific interface updates
	if (!s_itemcontrols.ingame_menu)
	{
		StartServer_SetIconFromGameType(&s_itemcontrols.gameTypeIcon, s_scriptdata.gametype, qfalse);
	}

	// set the group controls
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		if (masterControl[i].control)
			masterControl[i].control->curvalue = s_itemcontrols.grouptype[i];
	}

	// set tab controls
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		s_itemcontrols.tabbedText[i].generic.flags |= (QMF_PULSEIFFOCUS);
		s_itemcontrols.tabbedText[i].generic.flags &= ~(QMF_HIGHLIGHT_IF_FOCUS|QMF_HIGHLIGHT);
	}

	page = s_itemcontrols.currentPage;
	s_itemcontrols.tabbedText[page].generic.flags &= ~(QMF_PULSEIFFOCUS);
	s_itemcontrols.tabbedText[page].generic.flags |= (QMF_HIGHLIGHT_IF_FOCUS|QMF_HIGHLIGHT);


	// set controls on the currently tabbed page

	for (i = 0; i < s_itemcontrols.numPageItems; i++)
	{
		type = masterControl[page].control->curvalue;
		item = &masterControl[page].items[i];
		ctrl = item->control;
		ctrl->generic.flags &= ~(QMF_GRAYED|QMF_INACTIVE);
		switch (type) {
		case ALLGROUPS_ENABLED:
			ctrl->curvalue = qtrue;
			break;

		case ALLGROUPS_HIDDEN:
			ctrl->curvalue = qfalse;
			break;

		case ALLGROUPS_CUSTOM:
			ctrl->curvalue = s_itemcontrols.enabled[ item->ident ];
			break;
		}
	}

	// enable fight button if possible
	if (!s_itemcontrols.ingame_menu)
		StartServer_CheckFightReady(&s_itemcontrols.common);
}




/*
=================
StartServer_BothItemMenus_DrawItemButton
=================
*/
static void StartServer_BothItemMenus_DrawItemButton( void* ptr )
{
	int	x;
	int y;
	float *color;
	int	style;
	qboolean focus;
	menuradiobutton_s *rb;
	char* iconname;
	qhandle_t	icon;
	vec4_t gcolor;

	rb = (menuradiobutton_s*)ptr;
	x = rb->generic.x;
	y = rb->generic.y;

	gcolor[0] = 1.0;
	gcolor[1] = 1.0;
	gcolor[2] = 1.0;
	gcolor[3] = 1.0;

	// load the icon
	iconname = bg_itemlist[ s_itemcontrols.itemCtrl[rb->generic.id].bg_index ].icon;
	icon = trap_R_RegisterShaderNoMip(va("uie_%s", iconname));
	if (!icon)
		icon = trap_R_RegisterShaderNoMip(iconname);

	// setup text colour states
	focus = (rb->generic.parent->cursor == rb->generic.menuPosition);
	if ( rb->generic.flags & QMF_GRAYED )
	{
		gcolor[3] = 0.3;
		color = text_color_disabled;
		style = UI_LEFT|UI_SMALLFONT;
	}
	else if ( focus )
	{
		color = color_highlight;
		style = UI_LEFT|UI_PULSE|UI_SMALLFONT;
	}
	else
	{
		color = text_color_normal;
		style = UI_LEFT|UI_SMALLFONT;
	}

	// draw it!
	if ( rb->generic.name )
		UI_DrawString( x - SMALLCHAR_WIDTH, y, rb->generic.name, UI_RIGHT|UI_SMALLFONT, color );

	if ( !rb->curvalue )
	{
		gcolor[3] = 0.3;
		if(cl_language.integer == 0){
		UI_DrawString( x + 16, y, "off", style, color );
		}
		if(cl_language.integer == 1){
		UI_DrawString( x + 16, y, "откл", style, color );
		}
	}
	else
	{
		if(cl_language.integer == 0){
		UI_DrawString( x + 16, y, "on", style, color );
		}
		if(cl_language.integer == 1){
		UI_DrawString( x + 16, y, "вкл", style, color );
		}
	}

	trap_R_SetColor(gcolor);
	// square image, always
	UI_DrawHandlePic( x - 5, y, SMALLCHAR_HEIGHT, SMALLCHAR_HEIGHT, icon);
	trap_R_SetColor(NULL);
}





/*
=================
StartServer_BothItemMenus_ItemEvent

Changing a single item may force the group
setting to change as well. We must keep these values in sync
=================
*/
static void StartServer_BothItemMenus_ItemEvent( void* ptr, int event )
{
	menuradiobutton_s* rb;
	iteminfo_t* item;
	int i, type, page;
	qboolean enabled;
	mastercontrol_t* master;

	if (event != QM_ACTIVATED)
		return;

	// store the value of the control in server_itemlist[]
	rb = (menuradiobutton_s*)ptr;
	item = s_itemcontrols.itemCtrl[ rb->generic.id ].item;

	if (item) {
		page = s_itemcontrols.currentPage;
		type = s_itemcontrols.grouptype[page];
		master = &masterControl[page];
		if (type == ALLGROUPS_ENABLED || type == ALLGROUPS_HIDDEN) {
			// we're changing an item on a page where all controls are set/unset
			// so we must override the stored custom settings
			enabled = qfalse;
			if (type == ALLGROUPS_ENABLED)
				enabled = qtrue;

			for (i = 0; i < master->item_count; i++) {
				s_itemcontrols.enabled[ master->items[i].ident ] = enabled;
			}
		}

		// set the item flag and recover the new grouptype
		s_itemcontrols.enabled[ item->ident ] = rb->curvalue;
		s_itemcontrols.grouptype[page] = StartServer_BothItemMenus_FindGroupType(master->items, master->item_count);

		StartServer_BothItemMenus_UpdateInterface();
	}
}




/*
=================
StartServer_BothItemMenus_InitTabbedControls
=================
*/
static void StartServer_BothItemMenus_InitTabbedControls(int page)
{
	int y;
	int ident;
	iteminfo_t* items;
	itemnode_t* node;
	int i;

	// set all controls to invisible
	for (i = 0; i < MAX_ITEM_ONPAGE; i++) {
		s_itemcontrols.itemCtrl[i].control.generic.type = MTYPE_RADIOBUTTON;
		s_itemcontrols.itemCtrl[i].control.generic.flags = QMF_HIDDEN|QMF_INACTIVE;
	}

	if (page < 0 || page >= ITEMGROUP_COUNT)
		return;

	// set control count
	s_itemcontrols.currentPage = page;
	s_itemcontrols.numPageItems = masterControl[page].item_count;
	if (s_itemcontrols.numPageItems > MAX_ITEM_ONPAGE)
		s_itemcontrols.numPageItems = MAX_ITEM_ONPAGE;

	// setup controls
	items = masterControl[page].items;
	if (!items)
		return;

	y = s_itemcontrols.mid_tabbed_y - (s_itemcontrols.numPageItems * LINE_HEIGHT)/2;
	for (i = 0; i < s_itemcontrols.numPageItems; i++)
	{
		ident = items[i].ident;
		node = &server_itemlist[ ident ];

		s_itemcontrols.itemCtrl[i].control.generic.type	= MTYPE_RADIOBUTTON;
		s_itemcontrols.itemCtrl[i].control.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NODEFAULTINIT;
		if(cl_language.integer == 0){
		s_itemcontrols.itemCtrl[i].control.generic.name 	= items[i].name;
		}
		if(cl_language.integer == 1){
		s_itemcontrols.itemCtrl[i].control.generic.name 	= items[i].runame;
		}
		s_itemcontrols.itemCtrl[i].control.generic.callback	= StartServer_BothItemMenus_ItemEvent;
		s_itemcontrols.itemCtrl[i].control.generic.ownerdraw = StartServer_BothItemMenus_DrawItemButton;
		s_itemcontrols.itemCtrl[i].control.generic.id		= i;	// self index
		s_itemcontrols.itemCtrl[i].control.generic.x		= ITEMCONTROL_X;
		s_itemcontrols.itemCtrl[i].control.generic.y		= y;

		s_itemcontrols.itemCtrl[i].item = &items[i];
		s_itemcontrols.itemCtrl[i].bg_index = StartServer_GetItemFromMapObject(server_itemlist[ident].mapitem);

		items[i].control = &s_itemcontrols.itemCtrl[i].control;

		RadioButton_Init(&s_itemcontrols.itemCtrl[i].control);

		y += LINE_HEIGHT;
	}
}


/*
=================
StartServer_BothItemMenus_SetTabbedControlsPage

Prepare the new page of controls - setup their screen position
and then set their current values
=================
*/
static void StartServer_BothItemMenus_SetTabbedControlsPage(int page)
{
	StartServer_BothItemMenus_InitTabbedControls(page);
	StartServer_BothItemMenus_SetTabbedCtrlValues();
}



/*
=================
StartServer_BothItemMenus_GroupEvent

Syncs array with data control value
=================
*/
static void StartServer_BothItemMenus_GroupEvent( void* ptr, int event )
{
	menulist_s* l;

	if (event != QM_ACTIVATED)
		return;

	// store the value of the control
	l = (menulist_s*)ptr;
	s_itemcontrols.grouptype[l->generic.id] = l->curvalue;

	if (l->generic.id != s_itemcontrols.currentPage)
		StartServer_BothItemMenus_SetTabbedControlsPage(l->generic.id);

//	Com_Printf(va("Control selected: %i, %i\n", l->generic.id, item->ident));
	StartServer_BothItemMenus_UpdateInterface();
}




/*
=================
StartServer_BothItemMenus_Event
=================
*/
static void StartServer_BothItemMenus_Event( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s*)ptr)->id) {
	case ID_ITEM_RESET:
		StartServer_BothItemMenus_ResetAll(qtrue);
		StartServer_BothItemMenus_UpdateInterface();
		break;

	case ID_ITEM_CLEAR:
		StartServer_BothItemMenus_ResetAll(qfalse);
		StartServer_BothItemMenus_UpdateInterface();
		break;
	}
}





/*
=================
StartServer_BothItemMenus_TabbedEvent
=================
*/
static void StartServer_BothItemMenus_TabbedEvent( void* ptr, int event )
{
	menutext_s* m;

	if (event != QM_ACTIVATED)
		return;

	m = (menutext_s*)ptr;

	StartServer_BothItemMenus_SetTabbedControlsPage(m->generic.id);
	StartServer_BothItemMenus_UpdateInterface();
}






/*
=================
StartServer_BothItemMenus_SetupItemControls
=================
*/
static void StartServer_BothItemMenus_SetupItemControls(int y)
{
	int i;
	int max;
	int y_base;
	float scale;
	int style;

	//
	// count size of longest list of items
	//
	max = 0;
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		if (masterControl[i].item_count > max)
			max = masterControl[i].item_count;
	}

	if (max > MAX_ITEM_ONPAGE) {
		Com_Printf("ItemPage: Too many items on a tabbed page (%i)\n", max);
	}


	//
	// setup tabbed selection controls
	//
	style = UI_RIGHT|UI_MEDIUMFONT;
	scale = UI_ProportionalSizeScale(style, 0);

	y_base = TABCONTROLCENTER_Y;	// approx middle of free area
	if (s_itemcontrols.ingame_menu)
		y_base -= PROP_HEIGHT*scale;	// fudged
	s_itemcontrols.mid_tabbed_y = y_base;
	y_base -= (ITEMGROUP_COUNT * PROP_HEIGHT * scale)/2;	// offset for symmetry

	for (i = 0; i < ITEMGROUP_COUNT; i++) {
		s_itemcontrols.tabbedText[i].generic.type     = MTYPE_PTEXT;
		s_itemcontrols.tabbedText[i].generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_itemcontrols.tabbedText[i].generic.id	     = i;	// array index
		s_itemcontrols.tabbedText[i].generic.callback = StartServer_BothItemMenus_TabbedEvent;
		s_itemcontrols.tabbedText[i].generic.x	     = 140 - uis.wideoffset/2;
		s_itemcontrols.tabbedText[i].generic.y	     = y_base + i*PROP_HEIGHT * scale;
		if(cl_language.integer == 0){
		s_itemcontrols.tabbedText[i].string			= masterControl[i].tabbedTitle;
		}
		if(cl_language.integer == 1){
		s_itemcontrols.tabbedText[i].string			= masterControl[i].tabbedTitleru;
		}
		s_itemcontrols.tabbedText[i].style			= style;
		s_itemcontrols.tabbedText[i].color			= color_white;
	}

	// setup controls for the current tab
	StartServer_BothItemMenus_InitTabbedControls(0);
}



/*
=================
StartServer_BothItemMenus_AddMasterControls
=================
*/
static void StartServer_BothItemMenus_AddMasterControls(int y)
{
	int x, dy;
	int i;

	for (i = 0 ; i < ITEMGROUP_COUNT; i++)
	{
		x = GAMETYPECOLUMN_X + ((i / 2) - 1) * SMALLCHAR_WIDTH * 22;
		dy = (i % 2) * LINE_HEIGHT;

		s_itemcontrols.groupMaster[i].generic.type		= MTYPE_SPINCONTROL;
		s_itemcontrols.groupMaster[i].generic.id		= i;
		s_itemcontrols.groupMaster[i].generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_itemcontrols.groupMaster[i].generic.callback	= StartServer_BothItemMenus_GroupEvent;
		s_itemcontrols.groupMaster[i].generic.x		= x;
		s_itemcontrols.groupMaster[i].generic.y		= y + dy +30;
		if(cl_language.integer == 0){
		s_itemcontrols.groupMaster[i].generic.name		= masterControl[i].masterTitle;
		s_itemcontrols.groupMaster[i].itemnames		= allgroups_items;
		}
		if(cl_language.integer == 1){
		s_itemcontrols.groupMaster[i].generic.name		= masterControl[i].masterTitleru;
		s_itemcontrols.groupMaster[i].itemnames		= allgroups_itemsru;
		}

		masterControl[i].control = &s_itemcontrols.groupMaster[i];
	}
}




//------------------------------------------------------------------

// Functions used only by the ingame menu page

//------------------------------------------------------------------




/*
=================
StartServer_InGame_Init
=================
*/
static void StartServer_InGame_Init( void )
{
	int 		i, j;
	qboolean 	disabled;
	qboolean 	init;
	int 		type, t;

	s_itemcontrols.enabled = ingame_enabled;
	s_itemcontrols.grouptype = ingame_grouptype;

	// get the cvars currently set
	for (i = 0; i < ITEM_COUNT; i++)
	{
		disabled = (int)Com_Clamp(0, 1, trap_Cvar_VariableValue(va("disable_%s", server_itemlist[i].mapitem)));
		if (disabled)
			s_itemcontrols.enabled[i] = qfalse;
		else
			s_itemcontrols.enabled[i] = qtrue;
	}

	// all groups are custom by default, just in case a group
	// doesn't have a master control (we want the control visible and editable)
	// we then scan through the items to see how the master should be setup
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		s_itemcontrols.grouptype[i] = ALLGROUPS_CUSTOM;
	}

	StartServer_BothItemMenus_InitControls();
}



/*
=================
StartServer_InGame_SaveChanges
=================
*/
static void StartServer_InGame_SaveChanges( void )
{
	int 		i, j;
	int 		type;
	int			ident;
	qboolean	disable;

	// write out the disabled items list
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		disable = qfalse;
		type = s_itemcontrols.grouptype[ masterControl[i].ident ];
		if (type == ALLGROUPS_HIDDEN)
			disable = qtrue;
		for (j = 0; j < masterControl[i].item_count; j++)
		{
			ident = masterControl[i].items[j].ident;
			if (type == ALLGROUPS_CUSTOM)
			{
				disable = qtrue;
				if (s_itemcontrols.enabled[ ident ])
					disable = qfalse;
			}
			trap_Cvar_Set(va("disable_%s", server_itemlist[ident].mapitem), va("%i",disable));
		}
	}
}



/*
=================
StartServer_InGame_Event
=================
*/
static void StartServer_InGame_Event( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s*)ptr)->id)
	{
	case ID_ITEMINGAME_CANCEL:
		UI_PopMenu();
		break;

	case ID_ITEMINGAME_ACCEPT:
		StartServer_InGame_SaveChanges();
		UI_PopMenu();
		break;
	}
}


//------------------------------------------------------------------

// Functions used only by the skirmish menu page

//------------------------------------------------------------------




/*
=================
StartServer_ItemPage_Save
=================
*/
static void StartServer_ItemPage_Save( void )
{
	StartServer_SaveScriptData();
}


/*
=================
StartServer_ItemPage_Load
=================
*/
static void StartServer_ItemPage_Load( void )
{
	s_itemcontrols.enabled = s_scriptdata.item.enabled;
	s_itemcontrols.grouptype = s_scriptdata.item.groupstate;

	s_itemcontrols.gameType.curvalue = s_scriptdata.gametype;

	StartServer_BothItemMenus_InitControls();
}



/*
=================
StartServer_ItemPage_CommonEvent
=================
*/
static void StartServer_ItemPage_CommonEvent( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}

	StartServer_ItemPage_Save();
	switch( ((menucommon_s*)ptr)->id )
	{
		case ID_SERVERCOMMON_SERVER:
			StartServer_ServerPage_MenuInit();
			break;
			
		case ID_SERVERCOMMON_WEAPON:
			StartServer_WeaponPage_MenuInit();
			break;

		case ID_SERVERCOMMON_BOTS:
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_MAPS:
			UI_PopMenu();
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_BACK:
			UI_PopMenu();
			UI_PopMenu();
			UI_PopMenu();
			break;

		case ID_SERVERCOMMON_FIGHT:
			StartServer_CreateServer(NULL);
			break;
	}
}




/*
=================
StartServer_ItemPage_Event
=================
*/
static void StartServer_ItemPage_Event( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s*)ptr)->id) {
	case ID_ITEM_GAMETYPE:
		StartServer_SaveScriptData();

		StartServer_LoadScriptDataFromType(s_itemcontrols.gameType.curvalue);

		StartServer_BothItemMenus_InitControls();
		StartServer_BothItemMenus_UpdateInterface();
		break;
	}
}




//-----------------------------------------------------------

// Remaining functions used by both types of item menu

//-----------------------------------------------------------


/*
=================
StartServer_BothItemMenus_MenuDraw
=================
*/
static void StartServer_BothItemMenus_MenuDraw(void)
{
	int i;
	int style;

	if (uis.firstdraw) {
		// put all the data in place
		if (s_itemcontrols.ingame_menu) {
			StartServer_InGame_Init();
		}
		else {
			StartServer_ItemPage_Load();
		}

		StartServer_BothItemMenus_UpdateInterface();
	}

	StartServer_BackgroundDraw(qfalse);

	// draw the controls
	Menu_Draw(&s_itemcontrols.menu);
}




/*
=================
StartServer_BothItemMenus_MenuKey
=================
*/
static sfxHandle_t StartServer_BothItemMenus_MenuKey( int key )
{
	switch (key)
	{
		case K_MOUSE2:
		case K_ESCAPE:
			if (!s_itemcontrols.ingame_menu) {
				StartServer_ItemPage_Save();
				UI_PopMenu();
				UI_PopMenu();
			}
			break;
	}

	return ( Menu_DefaultKey( &s_itemcontrols.menu, key ) );
}






/*
=================
StartServer_BothItemMenus_MenuInit
=================
*/
static void StartServer_BothItemMenus_MenuInit(qboolean ingame)
{
	menuframework_s* menuptr;
	int i;
	int x, y, dy, left_y, right_y;
	int count, index;
	char* text;

	memset(&s_itemcontrols, 0, sizeof(itemcontrols_t));

	StartServer_BothItemMenus_Cache();

	menuptr = &s_itemcontrols.menu;

	menuptr->key = StartServer_BothItemMenus_MenuKey;
	menuptr->wrapAround = qtrue;
	menuptr->native 	= qfalse;
	menuptr->fullscreen = qtrue;
	menuptr->draw = StartServer_BothItemMenus_MenuDraw;

	y = GAMETYPEROW_Y;
	s_itemcontrols.ingame_menu = ingame;

	// menu type specific initialization
	if (ingame) {
		s_itemcontrols.ingame.title.generic.type     = MTYPE_BTEXT;
		s_itemcontrols.ingame.title.generic.x		= 320;
		s_itemcontrols.ingame.title.generic.y		= 4;
		s_itemcontrols.ingame.title.color			= color_white;
		s_itemcontrols.ingame.title.style			= UI_CENTER;

		s_itemcontrols.ingame.cancel.generic.type     = MTYPE_BITMAP;
		s_itemcontrols.ingame.cancel.generic.name     = INGAME_CANCEL0;
		s_itemcontrols.ingame.cancel.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_itemcontrols.ingame.cancel.generic.callback = StartServer_InGame_Event;
		s_itemcontrols.ingame.cancel.generic.id	    = ID_ITEMINGAME_CANCEL;
		s_itemcontrols.ingame.cancel.generic.x		= 0-uis.wideoffset;
		s_itemcontrols.ingame.cancel.generic.y		= 480-64;
		s_itemcontrols.ingame.cancel.width  		    = 128;
		s_itemcontrols.ingame.cancel.height  		    = 64;
		s_itemcontrols.ingame.cancel.focuspic         = INGAME_CANCEL1;

		s_itemcontrols.ingame.accept.generic.type     = MTYPE_BITMAP;
		s_itemcontrols.ingame.accept.generic.name     = INGAME_ACCEPT0;
		s_itemcontrols.ingame.accept.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_itemcontrols.ingame.accept.generic.callback = StartServer_InGame_Event;
		s_itemcontrols.ingame.accept.generic.id	    = ID_ITEMINGAME_ACCEPT;
		s_itemcontrols.ingame.accept.generic.x		= 640 - 128+uis.wideoffset;
		s_itemcontrols.ingame.accept.generic.y		= 480-64;
		s_itemcontrols.ingame.accept.width  		    = 128;
		s_itemcontrols.ingame.accept.height  		    = 64;
		s_itemcontrols.ingame.accept.focuspic         = INGAME_ACCEPT1;

		s_itemcontrols.ingame.info.generic.type     = MTYPE_PTEXT;
		s_itemcontrols.ingame.info.generic.flags	= QMF_INACTIVE;
		s_itemcontrols.ingame.info.generic.x		= 320;
		s_itemcontrols.ingame.info.generic.y		= 480 - 64 - 36;
		s_itemcontrols.ingame.info.color			= color_white;
		s_itemcontrols.ingame.info.style			= UI_CENTER|UI_SMALLFONT;
		if(cl_language.integer == 0){
		s_itemcontrols.ingame.info.string			= "Requires RESTART or NEXT MAP";
		s_itemcontrols.ingame.title.string			= "DISABLE ITEMS";
		}
		if(cl_language.integer == 1){
		s_itemcontrols.ingame.info.string			= "Требуется РЕСТАРТ или СЛЕД КАРТА";
		s_itemcontrols.ingame.title.string			= "ОТКЛЮЧИТЬ ПРЕДМЕТЫ";
		}

		Menu_AddItem( menuptr, &s_itemcontrols.ingame.title);
		Menu_AddItem( menuptr, &s_itemcontrols.ingame.cancel);
		Menu_AddItem( menuptr, &s_itemcontrols.ingame.accept);
		Menu_AddItem( menuptr, &s_itemcontrols.ingame.info);

		y -= LINE_HEIGHT;
	}
	else {
		StartServer_CommonControls_Init(menuptr, &s_itemcontrols.common, StartServer_ItemPage_CommonEvent, COMMONCTRL_ITEMS);

		s_itemcontrols.gameType.generic.type		= MTYPE_SPINCONTROL;
		s_itemcontrols.gameType.generic.id			= ID_ITEM_GAMETYPE;
		s_itemcontrols.gameType.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_itemcontrols.gameType.generic.callback	= StartServer_ItemPage_Event;
		s_itemcontrols.gameType.generic.x			= GAMETYPECOLUMN_X;
		s_itemcontrols.gameType.generic.y			= y;
		if(cl_language.integer == 0){
		s_itemcontrols.gameType.generic.name		= "Game Type:";
		s_itemcontrols.gameType.itemnames			= gametype_items;
		}
		if(cl_language.integer == 1){
		s_itemcontrols.gameType.generic.name		= "Режим Игры:";
		s_itemcontrols.gameType.itemnames			= gametype_itemsru;
		}

		s_itemcontrols.gameTypeIcon.generic.type  = MTYPE_BITMAP;
		s_itemcontrols.gameTypeIcon.generic.flags = QMF_INACTIVE;
		s_itemcontrols.gameTypeIcon.generic.x	 = GAMETYPEICON_X;
		s_itemcontrols.gameTypeIcon.generic.y	 = y;
		s_itemcontrols.gameTypeIcon.width  	     = 32;
		s_itemcontrols.gameTypeIcon.height 	     = 32;

		Menu_AddItem( menuptr, &s_itemcontrols.gameType);
		Menu_AddItem( menuptr, &s_itemcontrols.gameTypeIcon);

		y += 2*LINE_HEIGHT;
	}


	// controls common to both pages
	s_itemcontrols.reset.generic.type     = MTYPE_BITMAP;
	s_itemcontrols.reset.generic.name     = SERVER_ITEM_RESET0;
	s_itemcontrols.reset.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_itemcontrols.reset.generic.callback = StartServer_BothItemMenus_Event;
	s_itemcontrols.reset.generic.id	    = ID_ITEM_RESET;
	s_itemcontrols.reset.generic.x		= 320 - 64;
	s_itemcontrols.reset.generic.y		= 480-64;
	s_itemcontrols.reset.width  		    = 128;
	s_itemcontrols.reset.height  		    = 64;
	s_itemcontrols.reset.focuspic         = SERVER_ITEM_RESET1;

	s_itemcontrols.numPageItems = 0;
	s_itemcontrols.currentPage = 0;

	StartServer_BothItemMenus_AddMasterControls(y);

	y += 4*LINE_HEIGHT;

	// get all the items set up
	StartServer_BothItemMenus_SetupItemControls(y);

	// add the controls
	Menu_AddItem( menuptr, &s_itemcontrols.reset);

	// only setup controls that we've fully initialized
	for (i = 0; i < ITEMGROUP_COUNT; i++)
		Menu_AddItem( menuptr, &s_itemcontrols.groupMaster[i]);

	for (i = 0; i < ITEMGROUP_COUNT; i++)
		Menu_AddItem( menuptr, &s_itemcontrols.tabbedText[i]);

	for (i = 0; i < MAX_ITEM_ONPAGE; i++)
		Menu_AddItem( menuptr, &s_itemcontrols.itemCtrl[i].control);

	UI_PushMenu( &s_itemcontrols.menu );
}




/*
=================
StartServer_ItemPage_MenuInit
=================
*/
void StartServer_ItemPage_MenuInit(void)
{
	StartServer_BothItemMenus_MenuInit(qfalse);
}




/*
=================
UIE_InGame_EnabledItems
=================
*/
void UIE_InGame_EnabledItems(void)
{
	StartServer_BothItemMenus_MenuInit(qtrue);
}
