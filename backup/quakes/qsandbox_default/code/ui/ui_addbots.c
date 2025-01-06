// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

ADD BOTS MENU

=======================================================================
*/




#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"	
#define ART_FIGHT0			"menu/art/accept_0"
#define ART_FIGHT1			"menu/art/accept_1"
#define ART_BACKGROUND		"menu/art/addbotframe"
#define ART_ARROWS			"menu/art/arrows_vert_0"
#define ART_ARROWUP			"menu/art/arrows_vert_top"
#define ART_ARROWDOWN		"menu/art/arrows_vert_bot"
#define ART_SELECT			"menu/art/opponents_select"

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_UP				13
#define ID_DOWN				14
#define ID_SKILL			15
#define ID_TEAM				16
#define ID_BOTNAME0			20
#define ID_BOTNAME1			21
#define ID_BOTNAME2			22
#define ID_BOTNAME3			23
#define ID_BOTNAME4			24
#define ID_BOTNAME5			25
#define ID_BOTNAME6			26
#define ID_SKILLSLIDER  	27


typedef struct {
	menuframework_s	menu;
	menubitmap_s	arrows;
	menubitmap_s	up;
	menubitmap_s	down;
	menutext_s		bots[7];
	menulist_s		skill;
   menuslider_s   skill_slider;
	menulist_s		team;
	menubitmap_s	icon;
	menubitmap_s	icon_hilite;

	menubitmap_s	go;
	menubitmap_s	back;

	int				numBots;
	int				delay;
	int				baseBotNum;
	int				selectedBotNum;
	int				sortedBotNums[MAX_BOTS];
	char			botnames[7][32];
	char			boticon[MAX_QPATH];
   int 			gametype;
} addBotsMenuInfo_t;

static addBotsMenuInfo_t	addBotsMenuInfo;




/*
=================
AddBots_SetBotIcon
=================
*/
static void AddBots_SetBotIcon( void)
{
	char* info;

	info = UI_GetBotInfoByNumber( addBotsMenuInfo.sortedBotNums[addBotsMenuInfo.baseBotNum + addBotsMenuInfo.selectedBotNum] );
	UI_ServerPlayerIcon( Info_ValueForKey( info, "model" ), addBotsMenuInfo.boticon, MAX_QPATH );
	addBotsMenuInfo.icon.shader = 0;
}


/*
=================
UI_AddBotsMenu_SkillChangeEvent
=================
*/
static void UI_AddBotsMenu_SkillChangeEvent( void* ptr, int event) {
   menucommon_s* common;
   if (event != QM_ACTIVATED) {
      return;
   }

   // ties the skill controls together
   common = (menucommon_s*) ptr;
   switch (common->id) {
      case ID_SKILL: {
         menulist_s* skill = (menulist_s*)ptr;
         addBotsMenuInfo.skill_slider.curvalue = skill->curvalue;
         break;
      }
      case ID_SKILLSLIDER: {
         menuslider_s* skillslider = (menuslider_s*)ptr;
         addBotsMenuInfo.skill.curvalue = (int)skillslider->curvalue;
         break;
      }   
   }
}

/*
=================
UI_AddBotsMenu_FightEvent
=================
*/
static void UI_AddBotsMenu_FightEvent( void* ptr, int event ) {
	const char	*team;
	float			skill;

	if (event != QM_ACTIVATED) {
		return;
	}

	team = addBotsMenuInfo.team.itemnames[addBotsMenuInfo.team.curvalue];
	skill = addBotsMenuInfo.skill_slider.curvalue + 1;

	trap_Cmd_ExecuteText( EXEC_APPEND, va("addbot %s %4.2f %s %i\n",
		addBotsMenuInfo.botnames[addBotsMenuInfo.selectedBotNum], skill, team, addBotsMenuInfo.delay) );

	addBotsMenuInfo.delay += 1500;
}


/*
=================
UI_AddBotsMenu_BotEvent
=================
*/
static void UI_AddBotsMenu_BotEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	addBotsMenuInfo.bots[addBotsMenuInfo.selectedBotNum].color = color_grey;
	addBotsMenuInfo.selectedBotNum = ((menucommon_s*)ptr)->id - ID_BOTNAME0;
	addBotsMenuInfo.bots[addBotsMenuInfo.selectedBotNum].color = color_white;

	AddBots_SetBotIcon();
}



/*
=================
UI_AddBotsMenu_TeamEvent
=================
*/
static void UI_AddBotsMenu_TeamEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if (addBotsMenuInfo.gametype < GT_TEAM ||  addBotsMenuInfo.gametype == GT_LMS)
		return;

	if (addBotsMenuInfo.team.curvalue == 1)
		addBotsMenuInfo.icon_hilite.focuscolor			= color_blue;
	else
		addBotsMenuInfo.icon_hilite.focuscolor			= color_red;
}



/*
=================
UI_AddBotsMenu_BackEvent
=================
*/
static void UI_AddBotsMenu_BackEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
}

static void UI_AddBotsMenu_Createdbot( const char *info ) {
	fileHandle_t f;
	
	trap_FS_FOpenFile(va("bots/%s.dbot", info ),&f,FS_WRITE);
	
	trap_FS_Write("null", 4, f);
	
	trap_FS_FCloseFile(f);
}


/*
=================
UI_AddBotsMenu_SetBotNames
=================
*/
static void UI_AddBotsMenu_SetBotNames( void ) {
	int			n;
	const char	*info;

	for ( n = 0; n < 7; n++ ) {
		info = UI_GetBotInfoByNumber( addBotsMenuInfo.sortedBotNums[addBotsMenuInfo.baseBotNum + n] );
		Q_strncpyz( addBotsMenuInfo.botnames[n], Info_ValueForKey( info, "name" ), sizeof(addBotsMenuInfo.botnames[n]) );
		//UI_AddBotsMenu_Createdbot(Info_ValueForKey( info, "name" ));
	}

}


/*
=================
UI_AddBotsMenu_UpEvent
=================
*/
static void UI_AddBotsMenu_UpEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if( addBotsMenuInfo.baseBotNum > 0 ) {
		addBotsMenuInfo.baseBotNum -= 7;
		if (addBotsMenuInfo.baseBotNum < 0)
			addBotsMenuInfo.baseBotNum = 0;
		UI_AddBotsMenu_SetBotNames();
		AddBots_SetBotIcon();
	}
}


/*
=================
UI_AddBotsMenu_DownEvent
=================
*/
static void UI_AddBotsMenu_DownEvent( void* ptr, int event ) {
	int bot;

	if (event != QM_ACTIVATED) {
		return;
	}

	bot = addBotsMenuInfo.baseBotNum;
	if (bot + 7 < addBotsMenuInfo.numBots)
		bot += 7;
	else
		return;	

	if ( bot + 7 >= addBotsMenuInfo.numBots ) {
		bot = addBotsMenuInfo.numBots - 7;
		if (bot < 0)
			bot = 0;
	}

	addBotsMenuInfo.baseBotNum = bot;
	UI_AddBotsMenu_SetBotNames();
	AddBots_SetBotIcon();
}


/*
=================
UI_AddBotsMenu_GetSortedBotNums
=================
*/
static int QDECL UI_AddBotsMenu_SortCompare( const void *arg1, const void *arg2 ) {
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

static void UI_AddBotsMenu_GetSortedBotNums( void ) {
	int		n;

	// initialize the array
	for( n = 0; n < addBotsMenuInfo.numBots; n++ ) {
		addBotsMenuInfo.sortedBotNums[n] = n;
	}

	qsort( addBotsMenuInfo.sortedBotNums, addBotsMenuInfo.numBots, sizeof(addBotsMenuInfo.sortedBotNums[0]), UI_AddBotsMenu_SortCompare );
}

/*
=================
UI_AddBotsMenu_Draw
=================
*/
static void UI_AddBotsMenu_Draw( void ) {
	qhandle_t hpic;
	menubitmap_s* b;
	int x, y, w, h;
	int skill;
   float f_skill;

if(cl_language.integer == 0){
	UI_DrawString( 320, 16, "ADD BOTS", UI_CENTER, color_white );
}
if(cl_language.integer == 1){
	UI_DrawString( 320, 16, "ДОБАВИТЬ БОТОВ", UI_CENTER, color_white );
}
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );

	// standard menu drawing
	Menu_Draw( &addBotsMenuInfo.menu );

	if (addBotsMenuInfo.boticon[0])
	{
		skill = addBotsMenuInfo.skill.curvalue + 1;
		//hpic = trap_R_RegisterShaderNoMip( va("menu/art/skill%i", skill));
		if (1 == 1)
			return;

		// put icon in bottom right corner of pic
		b = &addBotsMenuInfo.icon;
		w = b->width;
		h = b->height;
		x = b->generic.x + w;
		y = b->generic.y + h;

		w /= 3;
		h /= 3;
		x -= w;
		y -= h;

		trap_R_SetColor(color_black);
		UI_DrawHandlePic( x, y, w, h, hpic);
		trap_R_SetColor(NULL);

		UI_DrawHandlePic( x - 2, y - 2, w, h, hpic);

      // write bot skill as float
      x = b->generic.x + b->width - 4 * SMALLCHAR_WIDTH;
      y = b->generic.y + b->height + 2;
      f_skill = addBotsMenuInfo.skill_slider.curvalue + 1;
	   UI_DrawString(x, y, va("%4.2f", f_skill), UI_SMALLFONT, color_grey);
	}
}

/*
=================
UI_AddBotsMenu_Init
=================
*/
static const char *skillNames[] = {
	"Noob",
	"Easy",
	"Medium",
	"Hard",
	"Nightmare!",
	"Infinite ammo",
	"Ultra HP",
	"Ultra Damage",
	"Nextbot",
	0
};

static const char* skillNames_ru[] = {
	"Нуб",
	"Легкий",
	"Средний",
	"Сложный",
	"Кошмар!",
	"Беск патроны",
	"Ультра HP",
	"Ультра урон",
	"Некстбот",
	0
};

static const char *teamNames1[] = {
	"Free",
	0
};

static const char *teamNames2[] = {
	"Red",
	"Blue",
	0
};

static void UI_AddBotsMenu_Init( void ) {
	int		n;
	int		y;
	int		count;
	char	info[MAX_INFO_STRING];

	trap_GetConfigString(CS_SERVERINFO, info, MAX_INFO_STRING);

	memset( &addBotsMenuInfo, 0 ,sizeof(addBotsMenuInfo) );
	addBotsMenuInfo.menu.draw = UI_AddBotsMenu_Draw;
	addBotsMenuInfo.menu.fullscreen = qfalse;
	addBotsMenuInfo.menu.wrapAround = qtrue;
	addBotsMenuInfo.menu.native = qfalse;
	addBotsMenuInfo.delay = 1000;

	addBotsMenuInfo.gametype = atoi( Info_ValueForKey( info,"g_gametype" ) );

	UI_AddBots_Cache();

	addBotsMenuInfo.numBots = UI_GetNumBots();
	count = addBotsMenuInfo.numBots < 7 ? addBotsMenuInfo.numBots : 7;

   // set the default bot skill
   //addBotsMenuInfo.f_skill = Com_Clamp( 0, 4, (int)trap_Cvar_VariableValue( "g_spSkill" ) - 1 );

   // prepare the menu items
	addBotsMenuInfo.arrows.generic.type  = MTYPE_BITMAP;
	addBotsMenuInfo.arrows.generic.name  = ART_ARROWS;
	addBotsMenuInfo.arrows.generic.flags = QMF_INACTIVE;
	addBotsMenuInfo.arrows.generic.x	 = 200;
	addBotsMenuInfo.arrows.generic.y	 = 128;
	addBotsMenuInfo.arrows.width  	     = 64;
	addBotsMenuInfo.arrows.height  	     = 128;

	addBotsMenuInfo.up.generic.type	    = MTYPE_BITMAP;
	addBotsMenuInfo.up.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	addBotsMenuInfo.up.generic.x		= 200;
	addBotsMenuInfo.up.generic.y		= 128;
	addBotsMenuInfo.up.generic.id	    = ID_UP;
	addBotsMenuInfo.up.generic.callback = UI_AddBotsMenu_UpEvent;
	addBotsMenuInfo.up.width  		    = 64;
	addBotsMenuInfo.up.height  		    = 64;
	addBotsMenuInfo.up.focuspic         = ART_ARROWUP;

	addBotsMenuInfo.down.generic.type	  = MTYPE_BITMAP;
	addBotsMenuInfo.down.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	addBotsMenuInfo.down.generic.x		  = 200;
	addBotsMenuInfo.down.generic.y		  = 128 +64;
	addBotsMenuInfo.down.generic.id	      = ID_DOWN;
	addBotsMenuInfo.down.generic.callback = UI_AddBotsMenu_DownEvent;
	addBotsMenuInfo.down.width  		  = 64;
	addBotsMenuInfo.down.height  		  = 64;
	addBotsMenuInfo.down.focuspic         = ART_ARROWDOWN;

	for( n = 0, y = 120; n < count; n++, y += 20 ) {
		addBotsMenuInfo.bots[n].generic.type		= MTYPE_PTEXT;
		addBotsMenuInfo.bots[n].generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		addBotsMenuInfo.bots[n].generic.id			= ID_BOTNAME0 + n;
		addBotsMenuInfo.bots[n].generic.x			= 320 - 56;
		addBotsMenuInfo.bots[n].generic.y			= y;
		addBotsMenuInfo.bots[n].generic.callback	= UI_AddBotsMenu_BotEvent;
		addBotsMenuInfo.bots[n].string				= addBotsMenuInfo.botnames[n];
		addBotsMenuInfo.bots[n].color				= color_grey;
		addBotsMenuInfo.bots[n].style				= UI_LEFT|UI_SMALLFONT;
	}

	y += 12;
	addBotsMenuInfo.skill_slider.generic.type		= MTYPE_SLIDER;
	addBotsMenuInfo.skill_slider.generic.x			= 320;
	addBotsMenuInfo.skill_slider.generic.y			= y;
if(cl_language.integer == 0){
	addBotsMenuInfo.skill_slider.generic.name		= "Skill:";
}
if(cl_language.integer == 1){
	addBotsMenuInfo.skill_slider.generic.name		= "Сложность:";
}
	addBotsMenuInfo.skill_slider.generic.callback = UI_AddBotsMenu_SkillChangeEvent;
	addBotsMenuInfo.skill_slider.generic.id		= ID_SKILLSLIDER;
	addBotsMenuInfo.skill_slider.minvalue			= 0.0;
	addBotsMenuInfo.skill_slider.maxvalue			= 8.0;
	addBotsMenuInfo.skill_slider.curvalue			= Com_Clamp( 0, 8, (int)trap_Cvar_VariableValue( "g_spSkill" ) - 1 );

   y += SMALLCHAR_HEIGHT;
	addBotsMenuInfo.skill.generic.type		= MTYPE_SPINCONTROL;
	addBotsMenuInfo.skill.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	addBotsMenuInfo.skill.generic.callback = UI_AddBotsMenu_SkillChangeEvent;
	addBotsMenuInfo.skill.generic.x			= 320;
	addBotsMenuInfo.skill.generic.y			= y;
	addBotsMenuInfo.skill.generic.id		= ID_SKILL;
	if(cl_language.integer == 0){
	addBotsMenuInfo.skill.itemnames			= skillNames;
	}
	if(cl_language.integer == 1){
	addBotsMenuInfo.skill.itemnames			= skillNames_ru;
	}
	addBotsMenuInfo.skill.curvalue			= (int)addBotsMenuInfo.skill_slider.curvalue;

	y += SMALLCHAR_HEIGHT;
	addBotsMenuInfo.team.generic.type		= MTYPE_SPINCONTROL;
	addBotsMenuInfo.team.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	addBotsMenuInfo.team.generic.callback		= UI_AddBotsMenu_TeamEvent;
	addBotsMenuInfo.team.generic.x			= 320;
	addBotsMenuInfo.team.generic.y			= y;
if(cl_language.integer == 0){
	addBotsMenuInfo.team.generic.name		= "Team: ";
}
if(cl_language.integer == 1){
	addBotsMenuInfo.team.generic.name		= "Команда: ";
}
	addBotsMenuInfo.team.generic.id			= ID_TEAM;
	if( addBotsMenuInfo.gametype >= GT_TEAM && !(addBotsMenuInfo.gametype == GT_LMS) ) {
		addBotsMenuInfo.team.itemnames		= teamNames2;
	}
	else {
		addBotsMenuInfo.team.itemnames		= teamNames1;
		addBotsMenuInfo.team.generic.flags	= QMF_GRAYED;
	}

	addBotsMenuInfo.go.generic.type			= MTYPE_BITMAP;
	addBotsMenuInfo.go.generic.name			= ART_FIGHT0;
	addBotsMenuInfo.go.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	addBotsMenuInfo.go.generic.id			= ID_GO;
	addBotsMenuInfo.go.generic.callback		= UI_AddBotsMenu_FightEvent;
	addBotsMenuInfo.go.generic.x			= 320+128-128;
	addBotsMenuInfo.go.generic.y			= 256+128-64;
	addBotsMenuInfo.go.width  				= 128;
	addBotsMenuInfo.go.height  				= 64;
	addBotsMenuInfo.go.focuspic				= ART_FIGHT1;

	addBotsMenuInfo.back.generic.type		= MTYPE_BITMAP;
	addBotsMenuInfo.back.generic.name		= ART_BACK0;
	addBotsMenuInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	addBotsMenuInfo.back.generic.id			= ID_BACK;
	addBotsMenuInfo.back.generic.callback	= UI_AddBotsMenu_BackEvent;
	addBotsMenuInfo.back.generic.x			= 320-128;
	addBotsMenuInfo.back.generic.y			= 256+128-64;
	addBotsMenuInfo.back.width				= 128;
	addBotsMenuInfo.back.height				= 64;
	addBotsMenuInfo.back.focuspic			= ART_BACK1;

	addBotsMenuInfo.icon_hilite.generic.type		= MTYPE_BITMAP;
	addBotsMenuInfo.icon_hilite.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE|QMF_HIGHLIGHT;
	addBotsMenuInfo.icon_hilite.generic.x			= 190 - 64 - 15; //320 + 128 - 15;
	addBotsMenuInfo.icon_hilite.generic.y			= 240 - 32 - 16;
	addBotsMenuInfo.icon_hilite.width				= 128;
	addBotsMenuInfo.icon_hilite.height				= 128;
	addBotsMenuInfo.icon_hilite.focuspic			= ART_SELECT;
	addBotsMenuInfo.icon_hilite.focuscolor			= color_red;

	addBotsMenuInfo.icon.generic.type		= MTYPE_BITMAP;
	addBotsMenuInfo.icon.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	addBotsMenuInfo.icon.generic.name		= addBotsMenuInfo.boticon;
	addBotsMenuInfo.icon.generic.x			= 190 - 64; //320 + 128;
	addBotsMenuInfo.icon.generic.y			= 240 - 32;
	addBotsMenuInfo.icon.width				= 64;
	addBotsMenuInfo.icon.height				= 64;

	addBotsMenuInfo.baseBotNum = 0;
	addBotsMenuInfo.selectedBotNum = 0;
	addBotsMenuInfo.bots[0].color = color_white;

	UI_AddBotsMenu_GetSortedBotNums();
	UI_AddBotsMenu_SetBotNames();
	AddBots_SetBotIcon();

	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.arrows );

	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.up );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.down );
	for( n = 0; n < count; n++ ) {
		Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.bots[n] );
	}
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.skill );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.skill_slider );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.team );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.go );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.back );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.icon_hilite );
	Menu_AddItem( &addBotsMenuInfo.menu, &addBotsMenuInfo.icon );
}


/*
=================
UI_AddBots_Cache
=================
*/
void UI_AddBots_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_BACKGROUND );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWUP );
	trap_R_RegisterShaderNoMip( ART_ARROWDOWN );
	trap_R_RegisterShaderNoMip( ART_SELECT);
}


/*
=================
UI_AddBotsMenu
=================
*/
void UI_AddBotsMenu( void ) {
	UI_AddBotsMenu_Init();
	UI_PushMenu( &addBotsMenuInfo.menu );
}
