// Copyright (C) 1999-2000 Id Software, Inc.
//




#include "ui_local.h"

#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_MODEL0			"menu/art/model_0"
#define ART_MODEL1			"menu/art/model_1"
#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_FX_BASE			"menu/art/fx_base"
#define ART_FX_BLUE			"menu/art/fx_blue"
#define ART_FX_CYAN			"menu/art/fx_cyan"
#define ART_FX_GREEN		"menu/art/fx_grn"
#define ART_FX_RED			"menu/art/fx_red"
#define ART_FX_TEAL			"menu/art/fx_teal"
#define ART_FX_WHITE		"menu/art/fx_white"
#define ART_FX_YELLOW		"menu/art/fx_yel"

#define ID_NAME			10
#define ID_EFFECTS		11
#define ID_BACK			12
#define ID_MODEL		13
#define ID_MODELTYPE	14
#define ID_FLASHRED				15
#define ID_FLASHGREEN			16
#define ID_FLASHBLUE			17
#define ID_HEFLASHRED			18
#define ID_HEFLASHGREEN			19
#define ID_HEFLASHBLUE			20
#define ID_TOFLASHRED			21
#define ID_TOFLASHGREEN			22
#define ID_TOFLASHBLUE			23

#define MAX_NAMELENGTH	32

#define MENUTEXT_COLUMN 144

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;

	menufield_s			name;
	menulist_s			effects;

	menuslider_s  		flashred;
	menuslider_s  		flashgreen;
	menuslider_s  		flashblue;
	menuslider_s  		heflashred;
	menuslider_s  		heflashgreen;
	menuslider_s  		heflashblue;
	menuslider_s  		toflashred;
	menuslider_s  		toflashgreen;
	menuslider_s  		toflashblue;
	menubitmap_s		back;
	menubitmap_s		model;
	menubitmap_s		item_null;
	menutext_s			modeltype;

	qhandle_t			fxBasePic;
	qhandle_t			fxPic[7];
	int					current_fx;
	modelAnim_t			player;

} playersettings_t;


static playersettings_t	s_playersettings;

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};

/*
=================
PlayerSettings_SetPlayerModelType
=================
*/
static void PlayerSettings_SetPlayerModelType( void )
{
	if (UIE_PlayerInfo_IsTeamModel())
	{
		if(cl_language.integer == 0){
		s_playersettings.modeltype.string = "Team Model";
		}
		if(cl_language.integer == 1){
		s_playersettings.modeltype.string = "Командная Модель";
		}
	}
	else
	{
		if(cl_language.integer == 0){
		s_playersettings.modeltype.string = "DM Model";
		}
		if(cl_language.integer == 1){
		s_playersettings.modeltype.string = "Обычная Модель";
		}
	}

	PText_Init(&s_playersettings.modeltype);
}



/*
=================
PlayerSettings_ToggleModelType
=================
*/
static void PlayerSettings_ToggleModelType( void )
{
	qboolean type;

	if (UIE_PlayerInfo_IsTeamModel()) {
		type = qfalse;
	}
	else {
		type = qtrue;
	}

	UIE_PlayerInfo_DrawTeamModel(&s_playersettings.player, type);
	PlayerSettings_SetPlayerModelType();
}




/*
=================
PlayerSettings_DrawName
=================
*/
static void PlayerSettings_DrawName( void *self ) {
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			*txt;
	char			c;
	float			*color;
	int				n;
	int				basex, x, y;
	char			name[32];

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = color_highlight;
	}

if(cl_language.integer == 0){
	UI_DrawString( basex, y, "Name", style, color );
}
if(cl_language.integer == 1){
	UI_DrawString( basex, y, "Имя", style, color );
}

	// draw the actual name
	basex += 64;
	y += PROP_HEIGHT;
	txt = f->field.buffer;
	color = g_color_table[ColorIndex(COLOR_WHITE)];
	x = basex;
	while ( (c = *txt) != 0 ) {
		if ( !focus && Q_IsColorString( txt ) ) {
			n = ColorIndex( *(txt+1) );
			if( n == 0 ) {
				n = 7;
			}
			color = g_color_table[n];
			txt += 2;
			continue;
		}
		UI_DrawChar( x, y, c, style, color );
		txt++;
		x += SMALLCHAR_WIDTH;
	}

	// draw cursor if we have focus
	if( focus ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar( basex + f->field.cursor * SMALLCHAR_WIDTH, y, c, style, color_white );
	}

	// draw at bottom also using proportional font
	Q_strncpyz( name, f->field.buffer, sizeof(name) );
	Q_CleanStr( name );
	UI_DrawString( 320, 440, name, UI_CENTER|UI_BIGFONT, text_color_normal );
}

/*
=================
PlayerSettings_DrawEffects
=================
*/
static void PlayerSettings_DrawEffects( void *self ) {
	menulist_s		*item;
	qboolean		focus;
	int				style;
	float			*color;
	int				textlen;

	item = (menulist_s *)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = color_highlight;
	}

	if(cl_language.integer == 0){
	if (item->generic.id == ID_EFFECTS) {
		UI_DrawString( item->generic.x, item->generic.y, "Rail core:", style, color );
	} else {
		UI_DrawString( item->generic.x, item->generic.y, "Rail ring:", style, color );
	}
	}
	if(cl_language.integer == 1){
	if (item->generic.id == ID_EFFECTS) {
		UI_DrawString( item->generic.x, item->generic.y, "Рэйл луч:", style, color );
	} else {
		UI_DrawString( item->generic.x, item->generic.y, "Рейл кольца:", style, color );
	}
	}
if(cl_language.integer == 0){
	textlen = UI_ProportionalStringWidth("Rail core:") * UI_ProportionalSizeScale(style, 0) * 1.00;
}
if(cl_language.integer == 1){
	textlen = UI_ProportionalStringWidth("Рэйл луч:") * UI_ProportionalSizeScale(style, 0) * 1.25;
}

	UI_DrawHandlePic( item->generic.x + textlen, item->generic.y + 4, 128, 8, s_playersettings.fxBasePic );
	UI_DrawHandlePic( item->generic.x + textlen + item->curvalue * 16 + 8, item->generic.y + 2, 16, 12, s_playersettings.fxPic[item->curvalue] );
}


/*
=================
PlayerSettings_DrawPlayer
=================
*/
static void PlayerSettings_DrawPlayer( void *self ) {
	UIE_PlayerInfo_AnimateModel(&s_playersettings.player);
}


/*
=================
PlayerSettings_DrawMenu
=================
*/
static void PlayerSettings_MenuDraw(void)
{
	if (uis.firstdraw)
		PlayerSettings_SetPlayerModelType();

	// standard menu drawing
	Menu_Draw( &s_playersettings.menu );
}


/*
=================
PlayerSettings_SaveChanges
=================
*/
static void PlayerSettings_SaveChanges( void ) {
	// name
	trap_Cvar_Set( "name", s_playersettings.name.field.buffer );

	// effects color
	trap_Cvar_SetValue( "color1", uitogamecode[s_playersettings.effects.curvalue] );
}


/*
=================
PlayerSettings_MenuKey
=================
*/
static sfxHandle_t PlayerSettings_MenuKey( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		PlayerSettings_SaveChanges();
	}
	return Menu_DefaultKey( &s_playersettings.menu, key );
}


/*
=================
PlayerSettings_SetMenuItems
=================
*/
static void PlayerSettings_SetMenuItems( void ) {
	vec3_t	viewangles;
	int		c;
	int		h;

	// name
	Q_strncpyz( s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"), sizeof(s_playersettings.name.field.buffer) );

	// effects color
	c = trap_Cvar_VariableValue( "color1" ) - 1;
	if( c < 0 || c > 6 ) {
		c = 6;
	}
	s_playersettings.effects.curvalue = gamecodetoui[c];

s_playersettings.heflashred.curvalue  = trap_Cvar_VariableValue( "cg_helightred");
	
	s_playersettings.heflashgreen.curvalue  = trap_Cvar_VariableValue( "cg_helightgreen");
	
	s_playersettings.heflashblue.curvalue  = trap_Cvar_VariableValue( "cg_helightblue");

	s_playersettings.toflashred.curvalue  = trap_Cvar_VariableValue( "cg_tolightred");
	
	s_playersettings.toflashgreen.curvalue  = trap_Cvar_VariableValue( "cg_tolightgreen");
	
	s_playersettings.toflashblue.curvalue  = trap_Cvar_VariableValue( "cg_tolightblue");

	s_playersettings.flashred.curvalue  = trap_Cvar_VariableValue( "cg_plightred");
	
	s_playersettings.flashgreen.curvalue  = trap_Cvar_VariableValue( "cg_plightgreen");
	
	s_playersettings.flashblue.curvalue  = trap_Cvar_VariableValue( "cg_plightblue");

	// model/skin
	UIE_PlayerInfo_InitModel(&s_playersettings.player);
}

/*
=================
PlayerSettings_MenuEvent
=================
*/
static void PlayerSettings_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {

	case ID_MODEL:
		PlayerSettings_SaveChanges();
		UI_PlayerModelMenu();
		break;

	case ID_MODELTYPE:
		PlayerSettings_ToggleModelType();
		break;

	case ID_BACK:
		PlayerSettings_SaveChanges();
		UI_PopMenu();
		break;
	case ID_FLASHRED:
		trap_Cvar_SetValue( "cg_plightred", s_playersettings.flashred.curvalue);
		break;
		
	case ID_FLASHGREEN:
		trap_Cvar_SetValue( "cg_plightgreen", s_playersettings.flashgreen.curvalue);
		break;
		
	case ID_FLASHBLUE:
		trap_Cvar_SetValue( "cg_plightblue", s_playersettings.flashblue.curvalue);
		break;
		
	case ID_TOFLASHRED:
		trap_Cvar_SetValue( "cg_tolightred", s_playersettings.toflashred.curvalue);
		break;
		
	case ID_TOFLASHGREEN:
		trap_Cvar_SetValue( "cg_tolightgreen", s_playersettings.toflashgreen.curvalue);
		break;
		
	case ID_TOFLASHBLUE:
		trap_Cvar_SetValue( "cg_tolightblue", s_playersettings.toflashblue.curvalue);
		break;
		
	case ID_HEFLASHRED:
		trap_Cvar_SetValue( "cg_helightred", s_playersettings.heflashred.curvalue);
		break;
		
	case ID_HEFLASHGREEN:
		trap_Cvar_SetValue( "cg_helightgreen", s_playersettings.heflashgreen.curvalue);
		break;
		
	case ID_HEFLASHBLUE:
		trap_Cvar_SetValue( "cg_helightblue", s_playersettings.heflashblue.curvalue);
		break;
	}
}


/*
=================
PlayerSettings_MenuInit
=================
*/
static void PlayerSettings_MenuInit( void ) {
	int		y;
	float 	sizeScale;

	memset(&s_playersettings,0,sizeof(playersettings_t));

	s_playersettings.menu.fullscreen = qtrue;
	s_playersettings.menu.wrapAround = qtrue;
	s_playersettings.menu.native 	   = qfalse;
	s_playersettings.menu.draw = PlayerSettings_MenuDraw;

	PlayerSettings_Cache();

	s_playersettings.menu.key        = PlayerSettings_MenuKey;

	sizeScale = UI_ProportionalSizeScale( UI_SMALLFONT, 0 );

	s_playersettings.banner.generic.type  = MTYPE_BTEXT;
	s_playersettings.banner.generic.x     = 320;
	s_playersettings.banner.generic.y     = 16;
	if(cl_language.integer == 0){
	s_playersettings.banner.string        = "PLAYER SETTINGS";
	}
	if(cl_language.integer == 1){
	s_playersettings.banner.string        = "НАСТРОЙКИ ИГРОКА";
	}
	s_playersettings.banner.color         = color_white;
	s_playersettings.banner.style         = UI_CENTER;

	s_playersettings.framel.generic.type  = MTYPE_BITMAP;
	s_playersettings.framel.generic.name  = ART_FRAMEL;
	s_playersettings.framel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_playersettings.framel.generic.x     = 0;
	s_playersettings.framel.generic.y     = 78;
	s_playersettings.framel.width         = 256;
	s_playersettings.framel.height        = 329;

	s_playersettings.framer.generic.type  = MTYPE_BITMAP;
	s_playersettings.framer.generic.name  = ART_FRAMER;
	s_playersettings.framer.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_playersettings.framer.generic.x     = 376;
	s_playersettings.framer.generic.y     = 76;
	s_playersettings.framer.width         = 256;
	s_playersettings.framer.height        = 334;

	y = 50;
	s_playersettings.name.generic.type			= MTYPE_FIELD;
	s_playersettings.name.generic.flags			= QMF_NODEFAULTINIT;
	s_playersettings.name.generic.ownerdraw		= PlayerSettings_DrawName;
	s_playersettings.name.field.widthInChars	= MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars		= MAX_NAMELENGTH;
	s_playersettings.name.generic.x				= MENUTEXT_COLUMN;
	s_playersettings.name.generic.y				= y;
	s_playersettings.name.generic.left			= MENUTEXT_COLUMN - 8;
	s_playersettings.name.generic.top			= y - 8;
	s_playersettings.name.generic.right			= MENUTEXT_COLUMN + 250;
	s_playersettings.name.generic.bottom		= y + 2 * PROP_HEIGHT;

	y += 5 * PROP_HEIGHT / 2;
	s_playersettings.effects.generic.type		= MTYPE_SPINCONTROL;
	s_playersettings.effects.generic.flags		= QMF_NODEFAULTINIT;
	s_playersettings.effects.generic.id			= ID_EFFECTS;
	s_playersettings.effects.generic.ownerdraw	= PlayerSettings_DrawEffects;
	s_playersettings.effects.generic.x			= MENUTEXT_COLUMN;
	s_playersettings.effects.generic.y			= y;
	s_playersettings.effects.generic.left		= MENUTEXT_COLUMN - 8;
	s_playersettings.effects.generic.top		= y - 4;
	s_playersettings.effects.generic.right		= MENUTEXT_COLUMN + 250;
	s_playersettings.effects.generic.bottom		= y + PROP_HEIGHT;
	s_playersettings.effects.numitems			= 7;

	s_playersettings.model.generic.type			= MTYPE_BITMAP;
	s_playersettings.model.generic.name			= ART_MODEL0;
	s_playersettings.model.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.model.generic.id			= ID_MODEL;
	s_playersettings.model.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.model.generic.x			= 640 + uis.wideoffset;
	s_playersettings.model.generic.y			= 480-64;
	s_playersettings.model.width				= 128;
	s_playersettings.model.height				= 64;
	s_playersettings.model.focuspic				= ART_MODEL1;

	s_playersettings.player.bitmap.generic.type		= MTYPE_BITMAP;
	s_playersettings.player.bitmap.generic.flags		= QMF_INACTIVE;
	s_playersettings.player.bitmap.generic.ownerdraw	= PlayerSettings_DrawPlayer;
	s_playersettings.player.bitmap.generic.x			= PLAYERMODEL_X;
	s_playersettings.player.bitmap.generic.y			= PLAYERMODEL_Y;
	s_playersettings.player.bitmap.width				= PLAYERMODEL_WIDTH;
	s_playersettings.player.bitmap.height				= PLAYERMODEL_HEIGHT;

y = 170;
//    y += BIGCHAR_HEIGHT+2;
    s_playersettings.heflashred.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.heflashred.generic.name		= "^1Head:";
	}
	if(cl_language.integer == 1){
	s_playersettings.heflashred.generic.name		= "^1Голова:";
	}
	s_playersettings.heflashred.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.heflashred.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.heflashred.generic.id		= ID_HEFLASHRED;
	s_playersettings.heflashred.generic.x			= 220;
	s_playersettings.heflashred.generic.y			= y;
	s_playersettings.heflashred.minvalue			= 0.0f;
	s_playersettings.heflashred.maxvalue			= 255;

        y += BIGCHAR_HEIGHT+5;
    s_playersettings.heflashgreen.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.heflashgreen.generic.name		= "^2Head:";
	}
	if(cl_language.integer == 1){
	s_playersettings.heflashgreen.generic.name		= "^2Голова:";
	}
	s_playersettings.heflashgreen.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.heflashgreen.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.heflashgreen.generic.id			= ID_HEFLASHGREEN;
	s_playersettings.heflashgreen.generic.x			= 220;
	s_playersettings.heflashgreen.generic.y			= y;
	s_playersettings.heflashgreen.minvalue			= 0.0f;
	s_playersettings.heflashgreen.maxvalue			= 255;

        y += BIGCHAR_HEIGHT+5;
    s_playersettings.heflashblue.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.heflashblue.generic.name		= "^4Head:";
	}
	if(cl_language.integer == 1){
	s_playersettings.heflashblue.generic.name		= "^4Голова:";
	}
	s_playersettings.heflashblue.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.heflashblue.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.heflashblue.generic.id		= ID_HEFLASHBLUE;
	s_playersettings.heflashblue.generic.x			= 220;
	s_playersettings.heflashblue.generic.y			= y;
	s_playersettings.heflashblue.minvalue			= 0;
	s_playersettings.heflashblue.maxvalue			= 255;
	
    y += BIGCHAR_HEIGHT+50;
    s_playersettings.toflashred.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.toflashred.generic.name		= "^1Torso:";
	}
	if(cl_language.integer == 1){
	s_playersettings.toflashred.generic.name		= "^1Торс:";
	}
	s_playersettings.toflashred.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.toflashred.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.toflashred.generic.id		= ID_TOFLASHRED;
	s_playersettings.toflashred.generic.x			= 220;
	s_playersettings.toflashred.generic.y			= y;
	s_playersettings.toflashred.minvalue			= 0.0f;
	s_playersettings.toflashred.maxvalue			= 255;

        y += BIGCHAR_HEIGHT+5;
    s_playersettings.toflashgreen.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.toflashgreen.generic.name		= "^2Torso:";
	}
	if(cl_language.integer == 1){
	s_playersettings.toflashgreen.generic.name		= "^2Торс:";
	}
	s_playersettings.toflashgreen.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.toflashgreen.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.toflashgreen.generic.id			= ID_TOFLASHGREEN;
	s_playersettings.toflashgreen.generic.x			= 220;
	s_playersettings.toflashgreen.generic.y			= y;
	s_playersettings.toflashgreen.minvalue			= 0.0f;
	s_playersettings.toflashgreen.maxvalue			= 255;

        y += BIGCHAR_HEIGHT+5;
    s_playersettings.toflashblue.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.toflashblue.generic.name		= "^4Torso:";
	}
	if(cl_language.integer == 1){
	s_playersettings.toflashblue.generic.name		= "^4Торс:";
	}
	s_playersettings.toflashblue.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.toflashblue.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.toflashblue.generic.id		= ID_TOFLASHBLUE;
	s_playersettings.toflashblue.generic.x			= 220;
	s_playersettings.toflashblue.generic.y			= y;
	s_playersettings.toflashblue.minvalue			= 0;
	s_playersettings.toflashblue.maxvalue			= 255;
	
    y += BIGCHAR_HEIGHT+50;
    s_playersettings.flashred.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.flashred.generic.name		= "^1Legs:";
	}
	if(cl_language.integer == 1){
	s_playersettings.flashred.generic.name		= "^1Ноги:";
	}
	s_playersettings.flashred.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.flashred.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.flashred.generic.id		= ID_FLASHRED;
	s_playersettings.flashred.generic.x			= 220;
	s_playersettings.flashred.generic.y			= y;
	s_playersettings.flashred.minvalue			= 0.0f;
	s_playersettings.flashred.maxvalue			= 255;

        y += BIGCHAR_HEIGHT+5;
    s_playersettings.flashgreen.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.flashgreen.generic.name		= "^2Legs:";
	}
	if(cl_language.integer == 1){
	s_playersettings.flashgreen.generic.name		= "^2Ноги:";
	}
	s_playersettings.flashgreen.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.flashgreen.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.flashgreen.generic.id			= ID_FLASHGREEN;
	s_playersettings.flashgreen.generic.x			= 220;
	s_playersettings.flashgreen.generic.y			= y;
	s_playersettings.flashgreen.minvalue			= 0.0f;
	s_playersettings.flashgreen.maxvalue			= 255;

        y += BIGCHAR_HEIGHT+5;
    s_playersettings.flashblue.generic.type		= MTYPE_SLIDER;
	if(cl_language.integer == 0){
	s_playersettings.flashblue.generic.name		= "^4Legs:";
	}
	if(cl_language.integer == 1){
	s_playersettings.flashblue.generic.name		= "^4Ноги:";
	}
	s_playersettings.flashblue.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.flashblue.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.flashblue.generic.id		= ID_FLASHBLUE;
	s_playersettings.flashblue.generic.x			= 220;
	s_playersettings.flashblue.generic.y			= y;
	s_playersettings.flashblue.minvalue			= 0;
	s_playersettings.flashblue.maxvalue			= 255;

	s_playersettings.back.generic.type			= MTYPE_BITMAP;
	s_playersettings.back.generic.name			= ART_BACK0;
	s_playersettings.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.back.generic.id			= ID_BACK;
	s_playersettings.back.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.back.generic.x				= 0 - uis.wideoffset;
	s_playersettings.back.generic.y				= 480-64;
	s_playersettings.back.width					= 128;
	s_playersettings.back.height				= 64;
	s_playersettings.back.focuspic				= ART_BACK1;

	s_playersettings.item_null.generic.type		= MTYPE_BITMAP;
	s_playersettings.item_null.generic.flags	= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.item_null.generic.x		= 0;
	s_playersettings.item_null.generic.y		= 0;
	s_playersettings.item_null.width			= 640;
	s_playersettings.item_null.height			= 480;

	s_playersettings.modeltype.generic.type			= MTYPE_PTEXT;
	s_playersettings.modeltype.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.modeltype.generic.x				= 480;
	s_playersettings.modeltype.generic.y				= PLAYERMODEL_TEXTHEIGHT + PROP_HEIGHT*sizeScale;
	s_playersettings.modeltype.generic.id				= ID_MODELTYPE;
	s_playersettings.modeltype.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.modeltype.string					= "";
	s_playersettings.modeltype.color					= text_color_normal;
	s_playersettings.modeltype.style					= UI_CENTER|UI_DROPSHADOW|UI_SMALLFONT;

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.banner );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framer );

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.name );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.model );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.back );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.flashred );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.flashgreen );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.flashblue );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.heflashred );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.heflashgreen );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.heflashblue );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.toflashred );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.toflashgreen );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.toflashblue );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.modeltype );

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.player.bitmap );

	// kills selected item when cursor moves off it, no item
	// registered after this will activate
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.item_null );

	PlayerSettings_SetMenuItems();
}


/*
=================
PlayerSettings_Cache
=================
*/
void PlayerSettings_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_MODEL0 );
	trap_R_RegisterShaderNoMip( ART_MODEL1 );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );

	s_playersettings.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	s_playersettings.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	s_playersettings.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	s_playersettings.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	s_playersettings.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	s_playersettings.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	s_playersettings.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	s_playersettings.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
}


/*
=================
UI_PlayerSettingsMenu
=================
*/
void UI_PlayerSettingsMenu( void ) {
	PlayerSettings_MenuInit();
	UI_PushMenu( &s_playersettings.menu );
}
