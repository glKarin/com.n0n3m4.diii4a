// Copyright (C) 1999-2000 Id Software, Inc.
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
#define ART_SAVE0			"menu/art/save_0"
#define ART_SAVE1			"menu/art/save_1"
#define ART_BACKGROUND		"menu/art/frame1_l"

#define MAX_CONFIGS			2048
#define NAMEBUFSIZE			( MAX_CONFIGS * 16 )

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_LEFT				13
#define ID_RIGHT			14
#define ID_SHOWID			15
#define ID_EXECFORCED		16

#define ARROWS_WIDTH		128
#define ARROWS_HEIGHT		48

#define FILENAME_Y			432

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menuobject_s		list;

	menufield_s		savename;

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menubitmap_s	back;
	menubitmap_s	go;
	menuradiobutton_s showid;
	menuradiobutton_s force_exec;

	char			names[NAMEBUFSIZE];
	char*			configlist[MAX_CONFIGS];
	char			configname[MAX_QPATH];
	qboolean 		load;
	qboolean 		loaderror;
	qboolean		noFilesFound;

	configCallback	callbackAction;

	int statusbar_time;
	char status_text[MAX_STATUSBAR_TEXT];
} configs_t;

static configs_t	s_configs;



// known config files that may cause saving problems
static const char* configfiles[] = {

	"q3config", "default", "autoexec", "uiautoexec", "q3config_server", "screen", "voice", "maps_ctf", "maps_dd", "maps_dom",
	"maps_dm", "maps_elimination", "maps_harvester", "maps_obelisk", "maps_oneflag", "maps_tdm", "maps_tourney", "maps_lms",

	0
};




/*
===============
UI_LoadConfig_SaveEvent
===============
*/
static qboolean LoadConfig_KnownIdConfig(const char* filename)
{
	int i;

	for(i = 0; configfiles[i]; i++)
	{
		if (!Q_stricmp(filename, configfiles[i]))
			return qtrue;
	}

	return qfalse;
}



/*
=============
SortRanks
=============
*/
int QDECL SortName( const void *a, const void *b )
{
	char* first, *second;

	first = *(char**)a;
	second = *(char**)b;
	return Q_stricmp(first,second);
}


/*
=============
LoadConfig_SetStatusText
=============
*/
void LoadConfig_SetStatusText(const char* text)
{
	if (text) {
		Q_strncpyz(s_configs.status_text, text, MAX_STATUSBAR_TEXT);
		s_configs.statusbar_time = uis.realtime + STATUSBAR_FADETIME;
	}
	else {
		s_configs.status_text[0] = '\0';
		s_configs.statusbar_time = 0;
	}
}



/*
===============
LoadConfig_LoadFileNames
===============
*/
static void LoadConfig_LoadFileNames( void )
{
	int i;
	int		len, filelen;
	char	*configname, *next;
	int 	numitems;
	int 	count;
	fileHandle_t	handle;
	qboolean showid;

	showid = s_configs.showid.curvalue;

	count = trap_FS_GetFileList( "", ".cfg", s_configs.names, NAMEBUFSIZE );
	next = s_configs.names;
	numitems = 0;
	s_configs.loaderror = qfalse;
	for ( i = 0; i < count; i++ )
	{
		configname = next;

		len = strlen( configname );
		next = configname + len + 1;

		filelen = trap_FS_FOpenFile(configname, &handle, FS_READ);
		if (filelen <= 0) {
			s_configs.loaderror = qtrue;
			continue;
		}

		trap_FS_FCloseFile(handle);

		// strip extension
		if (!Q_stricmp(configname +  len - 4,".cfg"))
			configname[len-4] = '\0';

		if (!showid && LoadConfig_KnownIdConfig(configname))
		{
			continue;
		}

		s_configs.list.itemnames[numitems] = configname;

		numitems++;
		if (numitems == MAX_CONFIGS)
			break;
	}


	if (!numitems) {
		if(cl_language.integer == 0){
		strcpy(s_configs.names,"No Files Found.");
		}
		if(cl_language.integer == 1){
		strcpy(s_configs.names,"Файлы не найдены.");
		}
		numitems = 1;

        s_configs.noFilesFound = qtrue;

		//degenerate case, not selectable
		if (s_configs.load)
			s_configs.go.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}

	s_configs.list.numitems = numitems;

	// sort the list
	qsort(s_configs.configlist, numitems, sizeof(s_configs.configlist[0]), SortName);
}



/*
===============
LoadConfig_DoSave
===============
*/
static void LoadConfig_DoSave( qboolean dosave )
{
	if (!dosave)
		return;

	if (s_configs.callbackAction) {
		s_configs.callbackAction(s_configs.configname);
		return;
	}

	// otherwise do default action, save system config
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "writeconfig %s\n", s_configs.configname ) );
	LoadConfig_SetStatusText(va("%s saved!", s_configs.configname));
}



/*
===============
LoadConfig_SaveCheck
===============
*/
static void LoadConfig_SaveCheck( const char* configname )
{
	fileHandle_t 	handle;
	int 			len;

	len = trap_FS_FOpenFile(s_configs.configname, &handle, FS_READ);

	if (len < 0)
		LoadConfig_DoSave(qtrue);
	else {
		trap_FS_FCloseFile(handle);
		if(cl_language.integer == 0){
		UI_ConfirmMenu("Overwrite?", 0, LoadConfig_DoSave);
		}
		if(cl_language.integer == 1){
		UI_ConfirmMenu("Перезаписать?", 0, LoadConfig_DoSave);
		}
	}
}




/*
===============
LoadConfig_Go
===============
*/
static void LoadConfig_Go( void )
{
	if( !s_configs.configname[0] ) {
		return;
	}

	if (s_configs.load) {
		if (s_configs.callbackAction && !s_configs.force_exec.curvalue) {
			s_configs.callbackAction(s_configs.configname);
			return;
		}

		// otherwise do the default action
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "exec %s\n", s_configs.configname) );
		LoadConfig_SetStatusText(va("%s executed", s_configs.configname));
	}
	else {
		LoadConfig_SaveCheck(s_configs.configname);
	}
}




/*
===============
LoadConfig_MenuEvent
===============
*/
static void LoadConfig_MenuEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		// confirm overwrite if save
		if (s_configs.load) {
			COM_StripExtensionOld(s_configs.configlist[s_configs.list.curvalue], s_configs.configname);
		}
		else {
			COM_StripExtensionOld(s_configs.savename.field.buffer, s_configs.configname);
		}
		if (s_configs.configname[0])
			strcat(s_configs.configname, ".cfg");

		Q_strlwr(s_configs.configname);	
		LoadConfig_Go();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_LEFT:
		UIObject_Key( &s_configs.list, K_LEFTARROW );
		break;

	case ID_RIGHT:
		UIObject_Key( &s_configs.list, K_RIGHTARROW );
		break;

	case ID_SHOWID:
		trap_Cvar_SetValue("uie_config_showid", s_configs.showid.curvalue);
		LoadConfig_LoadFileNames();
		UIObject_Init(&s_configs.list);
		break;

	case ID_LIST:
		if (s_configs.load) {
			LoadConfig_Go();
		}
		else {
			const char* str;

			str = s_configs.list.itemnames[s_configs.list.curvalue];
			strcpy(s_configs.savename.field.buffer, str);
			s_configs.savename.field.scroll = 0;
			s_configs.savename.field.cursor = strlen(str);
		}
		break;
	}
}




/*
===============
UI_SaveConfigMenu_SavenameDraw
===============
*/
static void UI_SaveConfigMenu_SavenameDraw( void *self ) {
	menufield_s		*f;
	int				style;
	float			*color;
	vec4_t			fade;

	f = (menufield_s *)self;

	if( f == Menu_ItemAtCursor( &s_configs.menu ) ) {
		style = UI_LEFT|UI_PULSE|UI_SMALLFONT;
		color = color_highlight;
	}
	else {
		style = UI_LEFT|UI_SMALLFONT;
		color = colorRed;
	}

	fade[0] = 1.0;
	fade[1] = 1.0;
	fade[2] = 1.0;
	fade[3] = 0.8;

	trap_R_SetColor(fade);
	UI_DrawNamedPic(320 - 120, FILENAME_Y - 12, 80, SMALLCHAR_HEIGHT + 24, ART_FRAMEL);
	UI_DrawNamedPic(320 + 40, FILENAME_Y - 12, 80, SMALLCHAR_HEIGHT + 24, ART_FRAMER);
	trap_R_SetColor(NULL);

	if(cl_language.integer == 0){
	UI_DrawString( 320, FILENAME_Y - 36, "Enter filename:", UI_CENTER|UI_SMALLFONT, color_grey );
	}
	if(cl_language.integer == 1){
	UI_DrawString( 320, FILENAME_Y - 36, "Введите имя файла:", UI_CENTER|UI_SMALLFONT, color_grey );
	}
	UI_FillRect( f->generic.x, f->generic.y, f->field.widthInChars*SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, colorBlack );
	MField_Draw( &f->field, f->generic.x, f->generic.y, style, color );
}




/*
=================
LoadConfig_MenuDraw
=================
*/
static void LoadConfig_MenuDraw(void)
{
	float f;
	vec4_t fadecol;

	if (uis.realtime < s_configs.statusbar_time)
	{
		f = (s_configs.statusbar_time - uis.realtime);
		f /= STATUSBAR_FADETIME;

		fadecol[0] = f;
		fadecol[1] = f;
		fadecol[2] = f;
		fadecol[3] = 1.0;

		UI_DrawString(320, 64, s_configs.status_text, UI_CENTER|UI_SMALLFONT, fadecol);
	}

	// draw the controls
	Menu_Draw(&s_configs.menu);
}





/*
===============
LoadConfig_MenuInit
===============
*/
static void LoadConfig_MenuInit( qboolean load, const char* title,  configCallback callback) {
	static char titlebuf[20];
	int		i;

	UI_LoadConfig_Cache();

	memset( &s_configs, 0 ,sizeof(configs_t) );
	s_configs.menu.wrapAround = qtrue;
	s_configs.menu.native 	  = qfalse;
	s_configs.menu.fullscreen = qtrue;
	s_configs.menu.draw = LoadConfig_MenuDraw;

	s_configs.load = load;
	s_configs.callbackAction = callback;

	LoadConfig_SetStatusText(NULL);

	if (!title) {
		if (load) {
			if(cl_language.integer == 0){
			title = "LOAD CONFIG";
			}
			if(cl_language.integer == 1){
			title = "ЗАГРУЗИТЬ ФАЙЛ";
			}
		}
		else {
			if(cl_language.integer == 0){
			title = "SAVE CONFIG";
			}
			if(cl_language.integer == 1){
			title = "СОХРАНИТЬ ФАЙЛ";
			}
		}
	}
	Q_strncpyz(titlebuf, title, 20);

	s_configs.banner.generic.type	= MTYPE_BTEXT;
	s_configs.banner.generic.x		= 320;
	s_configs.banner.generic.y		= 16;
	s_configs.banner.string 		= titlebuf;
	s_configs.banner.color			= color_white;
	s_configs.banner.style			= UI_CENTER;

	s_configs.framel.generic.type	= MTYPE_BITMAP;
	s_configs.framel.generic.name	= ART_FRAMEL;
	s_configs.framel.generic.flags	= QMF_INACTIVE;
	s_configs.framel.generic.x		= 0;
	s_configs.framel.generic.y		= 52;
	s_configs.framel.width			= 256;
	s_configs.framel.height			= 334;

	s_configs.framer.generic.type	= MTYPE_BITMAP;
	s_configs.framer.generic.name	= ART_FRAMER;
	s_configs.framer.generic.flags	= QMF_INACTIVE;
	s_configs.framer.generic.x		= 376;
	s_configs.framer.generic.y		= 52;
	s_configs.framer.width			= 256;
	s_configs.framer.height			= 334;

	s_configs.arrows.generic.type	= MTYPE_BITMAP;
	s_configs.arrows.generic.name	= ART_ARROWS;
	s_configs.arrows.generic.flags	= QMF_INACTIVE;
	s_configs.arrows.generic.x		= 320-ARROWS_WIDTH/2;
	s_configs.arrows.generic.y		= 400 - 48;
	s_configs.arrows.width			= ARROWS_WIDTH;
	s_configs.arrows.height			= ARROWS_HEIGHT;

	s_configs.left.generic.type		= MTYPE_BITMAP;
	s_configs.left.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_configs.left.generic.x		= 320-ARROWS_WIDTH/2;
	s_configs.left.generic.y		= 400 - 48;
	s_configs.left.generic.id		= ID_LEFT;
	s_configs.left.generic.callback	= LoadConfig_MenuEvent;
	s_configs.left.width			= ARROWS_WIDTH/2;
	s_configs.left.height			= ARROWS_HEIGHT;
	s_configs.left.focuspic			= ART_ARROWLEFT;

	s_configs.right.generic.type	= MTYPE_BITMAP;
	s_configs.right.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_configs.right.generic.x		= 320;
	s_configs.right.generic.y		= 400 - 48;
	s_configs.right.generic.id		= ID_RIGHT;
	s_configs.right.generic.callback = LoadConfig_MenuEvent;
	s_configs.right.width			= ARROWS_WIDTH/2;
	s_configs.right.height			= ARROWS_HEIGHT;
	s_configs.right.focuspic		= ART_ARROWRIGHT;

	s_configs.back.generic.type		= MTYPE_BITMAP;
	s_configs.back.generic.name		= ART_BACK0;
	s_configs.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_configs.back.generic.id		= ID_BACK;
	s_configs.back.generic.callback	= LoadConfig_MenuEvent;
	s_configs.back.generic.x		= 0 - uis.wideoffset;
	s_configs.back.generic.y		= 480-64;
	s_configs.back.width			= 128;
	s_configs.back.height			= 64;
	s_configs.back.focuspic			= ART_BACK1;

	s_configs.go.generic.type		= MTYPE_BITMAP;
	s_configs.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_configs.go.generic.id			= ID_GO;
	s_configs.go.generic.callback	= LoadConfig_MenuEvent;
	s_configs.go.generic.x			= 640 + uis.wideoffset;
	s_configs.go.generic.y			= 480-64;
	s_configs.go.width				= 128;
	s_configs.go.height				= 64;

	if (load) {
		s_configs.go.generic.name		= ART_FIGHT0;
		s_configs.go.focuspic			= ART_FIGHT1;
	}
	else {
		s_configs.go.generic.name		= ART_SAVE0;
		s_configs.go.focuspic			= ART_SAVE1;
	}

	s_configs.savename.generic.type		= MTYPE_FIELD;
	s_configs.savename.generic.flags		= QMF_NODEFAULTINIT|QMF_UPPERCASE;
	s_configs.savename.generic.ownerdraw	= UI_SaveConfigMenu_SavenameDraw;
	s_configs.savename.field.widthInChars	= 20;
	s_configs.savename.field.maxchars		= 20;
	s_configs.savename.generic.x			= 240;
	s_configs.savename.generic.y			= FILENAME_Y;
	s_configs.savename.generic.left		= 240;
	s_configs.savename.generic.top			= FILENAME_Y;
	s_configs.savename.generic.right		= 233 + 20*SMALLCHAR_WIDTH;
	s_configs.savename.generic.bottom		= FILENAME_Y + SMALLCHAR_HEIGHT+2;

	s_configs.showid.generic.type		= MTYPE_RADIOBUTTON;
	s_configs.showid.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_configs.showid.generic.id			= ID_SHOWID;
	if(cl_language.integer == 0){
	s_configs.showid.generic.name		= "QS configs:";
	}
	if(cl_language.integer == 1){
	s_configs.showid.generic.name		= "QS файлы:";
	}
	s_configs.showid.generic.callback	= LoadConfig_MenuEvent;
	s_configs.showid.generic.x			= 640 - 8 * SMALLCHAR_WIDTH;
	s_configs.showid.generic.y			= 480-64 - 32;
	s_configs.showid.curvalue 			= trap_Cvar_VariableValue("uie_config_showid");

	s_configs.force_exec.generic.type		= MTYPE_RADIOBUTTON;
	s_configs.force_exec.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_configs.force_exec.generic.id			= ID_EXECFORCED;
	if(cl_language.integer == 0){
	s_configs.force_exec.generic.name		= "Force exec:";
	}
	if(cl_language.integer == 1){
	s_configs.force_exec.generic.name		= "Принудительный запуск:";
	}
	s_configs.force_exec.generic.callback	= LoadConfig_MenuEvent;
	s_configs.force_exec.generic.x			= 640 - 8 * SMALLCHAR_WIDTH;
	s_configs.force_exec.generic.y			= 480-64 - 32 + SMALLCHAR_HEIGHT + 2;
	s_configs.force_exec.curvalue 			= qfalse;

	// scan for configs
	s_configs.list.generic.type		= MTYPE_UIOBJECT;
	s_configs.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_configs.list.generic.callback	= LoadConfig_MenuEvent;
	s_configs.list.generic.id		= ID_LIST;
	s_configs.list.generic.x		= 118;
	s_configs.list.generic.y		= 106;
	s_configs.list.string			= "";
	s_configs.list.width			= 48;
	s_configs.list.height			= 20;
	s_configs.list.itemnames		= (const char **)s_configs.configlist;
	s_configs.list.columns			= 1;
	s_configs.list.fontsize			= 1;
	s_configs.list.type				= 5;
	s_configs.list.styles			= 1;
	

	LoadConfig_LoadFileNames();
	if (s_configs.loaderror)
		s_configs.showid.generic.flags |= (QMF_GRAYED|QMF_INACTIVE);

	Menu_AddItem( &s_configs.menu, &s_configs.banner );
	Menu_AddItem( &s_configs.menu, &s_configs.framel );
	Menu_AddItem( &s_configs.menu, &s_configs.framer );
	Menu_AddItem( &s_configs.menu, &s_configs.list );
	Menu_AddItem( &s_configs.menu, &s_configs.showid );
	Menu_AddItem( &s_configs.menu, &s_configs.arrows );
	Menu_AddItem( &s_configs.menu, &s_configs.left );
	Menu_AddItem( &s_configs.menu, &s_configs.right );
	Menu_AddItem( &s_configs.menu, &s_configs.back );
	Menu_AddItem( &s_configs.menu, &s_configs.go );

	if (load && callback)
		Menu_AddItem( &s_configs.menu, &s_configs.force_exec );

	if (!load)
		Menu_AddItem( &s_configs.menu, &s_configs.savename );
}

/*
=================
UI_LoadConfig_Cache
=================
*/
void UI_LoadConfig_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWLEFT );
	trap_R_RegisterShaderNoMip( ART_ARROWRIGHT );
	trap_R_RegisterShaderNoMip( ART_SAVE0 );
	trap_R_RegisterShaderNoMip( ART_SAVE1 );
	trap_R_RegisterShaderNoMip( ART_BACKGROUND );
}


/*
===============
UI_LoadConfigMenu
===============
*/
void UI_LoadConfigMenu( void )
{
	LoadConfig_MenuInit(qtrue, 0, 0);
	UI_PushMenu( &s_configs.menu );
}


/*
===============
UI_SaveConfigMenu
===============
*/
void UI_SaveConfigMenu( void )
{
	LoadConfig_MenuInit(qfalse, 0, 0);
	UI_PushMenu( &s_configs.menu );
}



/*
===============
UI_ConfigMenu
===============
*/
void UI_ConfigMenu( const char* title, qboolean load, configCallback callback)
{
	LoadConfig_MenuInit(load, title, callback);
	UI_PushMenu( &s_configs.menu );
}



