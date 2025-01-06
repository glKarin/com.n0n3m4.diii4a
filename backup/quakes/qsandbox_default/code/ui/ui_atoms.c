// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/

#include "../qcommon/ns_local.h"
#include "ui_local.h" //karin: add

uiStatic_t		uis;
qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound

// these are here so the functions in q_shared.c can link
#ifndef UI_HARD_LINKED

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];
	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error( va("%s", text) );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( va("%s", text) );
}

#endif

/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop( void ) {
	trap_Cmd_ExecuteText( EXEC_APPEND, "d1\n" );
}

/*
================
UI_ScreenOffset

Adjusted for resolution and screen aspect ratio
================
*/
void UI_ScreenOffset( void ) {
	float scrx;
	float scry;
	char  svinfo[MAX_INFO_STRING];
	
	trap_GetGlconfig( &uis.glconfig );
	
	scrx = uis.glconfig.vidWidth;
	scry = uis.glconfig.vidHeight;
	
	if((scrx / (scry / 480)-640)/2 >= 0){
	trap_Cvar_SetValue("cl_screenoffset", (scrx / (scry / 480)-640)/2);
	uis.wideoffset = (scrx / (scry / 480)-640)/2;
	} else {
	trap_Cvar_SetValue("cl_screenoffset", 0);	
	uis.wideoffset = 0;
	}
	trap_GetConfigString( CS_SERVERINFO, svinfo, MAX_INFO_STRING );
	if(strlen(svinfo) <= 0){
	uis.onmap = qfalse;
	} else {
	uis.onmap = qtrue;	
	}
	if(uis.glconfig.vidHeight/480.0f > 0){
	trap_Cvar_SetValue("con_scale", 1.85);
	}
	trap_Cvar_Set("cl_conColor", "8 8 8 192");
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu( menuframework_s *menu )
{
	int				i;
	int				number;
	menucommon_s*	item;
	
	number = rand() % 4 + 1;
	
	trap_Cvar_SetValue( "ui_backcolors", number );
	if(uis.onmap){
	trap_Cvar_Set( "r_fx_blur", "1" );			//blur UI postFX
	} else {
	trap_Cvar_Set( "r_fx_blur", "0" );			//blur UI postFX		
	}
	
	// initialize the screen offset
	UI_ScreenOffset();
	
	uis.menuscroll = 0;

	// avoid stacking menus invoked by hotkeys
	for (i=0 ; i<uis.menusp ; i++)
	{
		if (uis.stack[i] == menu)
		{
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp)
	{
		if (uis.menusp >= MAX_MENUDEPTH)
			trap_Error("UI_PushMenu: menu stack overflow");

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	// default cursor position
	menu->cursor      = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Key_SetCatcher( KEYCATCH_UI );

	// force first available item to have focus
	for (i=0; i<menu->nitems; i++)
	{
		item = (menucommon_s *)menu->items[i];
		if (!(item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE)))
		{
			menu->cursor_prev = -1;
			Menu_SetCursor( menu, i );
			break;
		}
	}

	uis.firstdraw = qtrue;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu (void)
{
	trap_S_StartLocalSound( menu_out_sound, CHAN_LOCAL_SOUND );

	uis.menusp--;
	
	uis.menuscroll = 0;

	if (uis.menusp < 0)
		UI_ForceMenuOff ();

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp-1];
		uis.firstdraw = qtrue;
	} else {
		UI_ForceMenuOff ();
	}
}

void UI_ForceMenuOff (void)
{
	uis.menusp     = 0;
	uis.activemenu = NULL;

	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	trap_Key_ClearStates();
	trap_Cvar_Set( "cl_paused", "0" );
	trap_Cvar_Set( "r_fx_blur", "0" );			//blur UI postFX
}

/*
=================
UI_LerpColor
=================
*/
void UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

int UI_ProportionalStringWidth( const char* str ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;

	s = str;
	width = 0;
	while ( *s ) {
        if ( Q_IsColorString( s ) )
		{
			s += 2;
			continue;
		}
		ch = *s & 255;
        charWidth = 16;
		if ( charWidth != -1 ) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	width -= PROP_GAP_WIDTH;
	if(ifstrlenru(str)){
		return width * 0.5;	
	} else {
		return width;
	}
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style, float customsize ) {
if(customsize == 0){
	if(  style & UI_SMALLFONT ) {
		return PROP_SMALL_SIZE_SCALE;
	}

	if (style & UI_MEDIUMFONT ) {
		return PROP_MEDIUM_SIZE_SCALE;
	}
} else {
	return customsize;
}
	return 1.00;
}

/*
=================
UI_DrawString2
=================
*/
static void UI_DrawString2(int x, int y, const char* str, vec4_t color, int charw, int charh, float width)
{
	const char* s;
	char ch;
	int forceColor = qfalse; //APSFIXME;
	int prev_unicode = 0;
	vec4_t tempcolor;
	float ax;
	float ay;
	float aw;
	float ah;
	float frow;
	float fcol;
	float alignstate = 0;
	int char_count = 0;
	int q;

	q = 0;
	if(charh > 16){
	q = 1;	
	}
	if(charh > 32){
	q = 2;	
	}

	// Align states for center and right alignment
	if (charw < 0) {
		charw = -charw;
		alignstate = 0.5; // center_align
	}
	if (charh < 0) {
		charh = -charh;
		alignstate = 1; // right_align
	}

	// Set color for the text
	trap_R_SetColor(color);

	ax = x * uis.scale + uis.bias;
	ay = y * uis.scale;
	ay += uis.menuscroll;
	aw = charw * uis.scale;
	ah = charh * uis.scale;

	s = str;
	while (*s)
	{
		if ((*s == -48) || (*s == -47)) {
			ax = ax + aw * alignstate;
		}
		s++;
	}

	s = str;
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			memcpy(tempcolor, g_color_table[ColorIndex(s[1])], sizeof(tempcolor));
			tempcolor[3] = color[3];
			trap_R_SetColor(tempcolor);
			s += 2;
			continue;
		}

		if (*s != ' ')
		{
			ch = *s & 255;

			// Unicode Russian support
			if (ch < 0) {
				if ((ch == -48) || (ch == -47)) {
					prev_unicode = ch;
					s++;
					continue;
				}
				if (ch >= -112) {
					if ((ch == -111) && (prev_unicode == -47)) {
						ch = ch - 13;
					} else {
						ch = ch + 48;
					}
				} else {
					if ((ch == -127) && (prev_unicode == -48)) {
						// ch = ch +
					} else {
						ch = ch + 112; // +64 offset of damn unicode
					}
				}
			}

			frow = (ch >> 4) * 0.0625;
			fcol = (ch & 15) * 0.0625;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset[q]);
		}

		ax += aw;
		char_count++;

		if (char_count >= width) {
			ax = x * uis.scale + uis.bias;
			ay += ah + 2;
			char_count = 0;
		}

		s++;
	}

	trap_R_SetColor(NULL);
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, const char* str, int style, vec4_t color )
{
	int		len;
	int		charw;
	int		charh;
	vec4_t	newcolor;
	float	*drawcolor;
	vec4_t	dropcolor;

	if( !str ) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	if (style & UI_SMALLFONT)
	{
		charw =	SMALLCHAR_WIDTH;
		charh =	SMALLCHAR_HEIGHT;
	}
	else if (style & UI_TINYFONT)
	{
		charw =	TINYCHAR_WIDTH;
		charh =	TINYCHAR_HEIGHT;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
		charh =	GIANTCHAR_HEIGHT;
	}
	else
	{
		charw =	BIGCHAR_WIDTH;
		charh =	BIGCHAR_HEIGHT;
	}

	if (style & UI_PULSE)
	{
		UI_LerpColor(color,color_highlight,newcolor,0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}	
	else
		drawcolor = color;

	switch (style & UI_FORMATMASK)
	{
		case UI_CENTER:
			// center justify at x
			len = strlen(str);
			x   = x - len*charw/2;
                        charw = -charw; //Mix3r - sly way to transfer align to drawstring2
			break;

		case UI_RIGHT:
			// right justify at x
			len = strlen(str);
			x   = x - len*charw;
                        charw = -charw; //Mix3r - sly way to transfer align to drawstring2
                        charh = -charh;
			break;

		default:
			// left justify at x
			break;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x+1,y+1,str,dropcolor,charw,charh,512);
	}

	UI_DrawString2(x,y,str,drawcolor,charw,charh,512);
}

/*
=================
UI_DrawStringCustom
=================
*/
void UI_DrawStringCustom( int x, int y, const char* str, int style, vec4_t color, float csize, float width )
{
	int		len;
	int		charw;
	int		charh;
	vec4_t	newcolor;
	float	*drawcolor;
	vec4_t	dropcolor;

	if( !str ) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

if(csize == 0){
	if (style & UI_SMALLFONT)
	{
		charw =	SMALLCHAR_WIDTH;
		charh =	SMALLCHAR_HEIGHT;
	}
	else if (style & UI_TINYFONT)
	{
		charw =	TINYCHAR_WIDTH;
		charh =	TINYCHAR_HEIGHT;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
		charh =	GIANTCHAR_HEIGHT;
	}
	else
	{
		charw =	BIGCHAR_WIDTH;
		charh =	BIGCHAR_HEIGHT;
	}
} else {
		charw =	BIGCHAR_WIDTH*csize;
		charh =	BIGCHAR_HEIGHT*csize;	
}

	if (style & UI_PULSE)
	{
		UI_LerpColor(color,color_highlight,newcolor,0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}	
	else
		drawcolor = color;

	switch (style & UI_FORMATMASK)
	{
		case UI_CENTER:
			// center justify at x
			len = strlen(str);
			x   = x - len*charw/2;
                        charw = -charw; //Mix3r - sly way to transfer align to drawstring2
			break;

		case UI_RIGHT:
			// right justify at x
			len = strlen(str);
			x   = x - len*charw;
                        charw = -charw; //Mix3r - sly way to transfer align to drawstring2
                        charh = -charh;
			break;

		default:
			// left justify at x
			break;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x+1,y+1,str,dropcolor,charw,charh,width);
	}

	UI_DrawString2(x,y,str,drawcolor,charw,charh,width);
}

/*
=================
UI_DrawChar
=================
*/
void UI_DrawChar( int x, int y, int ch, int style, vec4_t color )
{
	char	buff[2];

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawString( x, y, buff, style, color );
}

/*
=================
UI_DrawCharCustom
=================
*/
void UI_DrawCharCustom( int x, int y, int ch, int style, vec4_t color, float csize )
{
	char	buff[2];

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawStringCustom( x, y, buff, style, color, csize, 512 );
}

qboolean UI_IsFullscreen( void ) {
	if ( uis.activemenu && ( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		if(!uis.onmap){
		return uis.activemenu->fullscreen;
		} else {
		return 0;
		}
	}

	return qfalse;
}

void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	Menu_Cache();

	switch ( menu ) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_ScreenOffset();
		if(strlen(ui_3dmap.string) <= 1){
		UI_MainMenu();
		UI_CreditMenu(1);
		}
		if(strlen(ui_3dmap.string)){
		trap_Cmd_ExecuteText( EXEC_APPEND, va("set sv_maxclients 1; map %s\n", ui_3dmap.string) );
		}
		return;
	case UIMENU_NEED_CD:
		return;
	case UIMENU_BAD_CD_KEY:
		return;
	case UIMENU_INGAME:
		trap_Cvar_Set( "cl_paused", "1" );
		UI_InGameMenu();
		return;

	// bk001204
	case UIMENU_TEAM:
	case UIMENU_POSTGAME:
	default:
#ifndef NDEBUG
	  Com_Printf("UI_SetActiveMenu: bad enum %d\n", menu );
#endif
	break;
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int down ) {
	sfxHandle_t		s;

	if (!uis.activemenu) {
		return;
	}

	if (!down) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key( key );
	else
		s = Menu_DefaultKey( uis.activemenu, key );

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	int				i;
	float scrx;
	float scry;
	menucommon_s*	m;

	if (!uis.activemenu)
		return;
	
	trap_GetGlconfig( &uis.glconfig );
	
	scrx = uis.glconfig.vidWidth;
	scry = uis.glconfig.vidHeight;

	// update mouse screen position
	if(uis.activemenu->native > 0){
	uis.cursorx += dx * sensitivitymenu.value;
	if (uis.cursorx < 0)
		uis.cursorx = 0;
	else if (uis.cursorx > uis.glconfig.vidWidth)
		uis.cursorx = uis.glconfig.vidWidth;

	uis.cursory += dy * sensitivitymenu.value;
	if (uis.cursory < 0+uis.activemenu->uplimitscroll)
		uis.cursory = 0+uis.activemenu->uplimitscroll;
	else if (uis.cursory > uis.glconfig.vidHeight+uis.activemenu->downlimitscroll)
		uis.cursory = uis.glconfig.vidHeight+uis.activemenu->downlimitscroll;
	} else {
		uis.cursorx += dx * sensitivitymenu.value;
	if (uis.cursorx < 0-uis.wideoffset)
		uis.cursorx = 0-uis.wideoffset;
	else if (uis.cursorx > 640+uis.wideoffset)
		uis.cursorx = 640+uis.wideoffset;

	uis.cursory += dy * sensitivitymenu.value;
	if (uis.cursory < 0+uis.activemenu->uplimitscroll)
		uis.cursory = 0+uis.activemenu->uplimitscroll;
	else if (uis.cursory > 480+uis.activemenu->downlimitscroll)
		uis.cursory = 480+uis.activemenu->downlimitscroll;	
	}

	// region test the active menu items
	for (i=0; i<uis.activemenu->nitems; i++)
	{
		m = (menucommon_s*)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED|QMF_INACTIVE))
			continue;

		if ((uis.cursorx < m->left) ||
			(uis.cursorx > m->right) ||
			(uis.cursory < m->top) ||
			(uis.cursory > m->bottom))
		{
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i)
		{
			Menu_SetCursor( uis.activemenu, i );
			((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if ( !(((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT ) ) {
				trap_S_StartLocalSound( menu_move_sound, CHAN_LOCAL_SOUND );
			}
		}

		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}  

	if (uis.activemenu->nitems > 0) {
		// out of any region
		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

/*
=================
UI_Cache
=================
*/
void UI_Cache_f( void ) {
	MainMenu_Cache();
	InGame_Cache();
	ConfirmMenu_Cache();
	PlayerModel_Cache();
	PlayerSettings_Cache();
	Controls_Cache();
	Demos_Cache();
	UI_CinematicsMenu_Cache();
	Preferences_Cache();
	ServerInfo_Cache();
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();
	DriverInfo_Cache();
	GraphicsOptions_Cache();
	UI_DisplayOptionsMenu_Cache();
	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
	UI_SetupMenu_Cache();
	UI_BotSelect_Cache();
	UI_ModsMenu_Cache();
}


/*
==================
UI_ConcatArgs
==================
*/
char	*UI_ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	int    i;
	char	*cmd;
	int number01;
	int number02;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	Menu_Cache();

UI_ScreenOffset();

if( Q_stricmp (UI_Argv(0), "ui_addbots") == 0 ){
UI_AddBotsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_advanced") == 0 ){
UI_AdvancedMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_savegame") == 0 ){
UI_CinematicsMenu(0);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_loadgame") == 0 ){
UI_CinematicsMenu(1);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_controls2") == 0 ){
UI_ControlsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_credits0") == 0 ){
UI_CreditMenu(0);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_credits1") == 0 ){
UI_CreditMenu(1);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_demo2") == 0 ){
UI_DemosMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_display") == 0 ){
UI_DisplayOptionsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_ingame_bots") == 0 ){
UI_BotCommandMenu_f();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_ingame") == 0 ){
UI_InGameMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_ingame_mapvote") == 0 ){
UI_MapCallVote();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_menu") == 0 ){
UI_MainMenu();
return qtrue;
}	
if( Q_stricmp (UI_Argv(0), "ui_mods") == 0 ){
UI_ModsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_network") == 0 ){
UI_NetworkOptionsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_options") == 0 ){
UI_SystemConfigMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_playermodel") == 0 ){
UI_PlayerModelMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_playersettings") == 0 ){
UI_PlayerSettingsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_preferences") == 0 ){
UI_PreferencesMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_removebots0") == 0 ){
UI_RemoveBotsMenu(0);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_removebots1") == 0 ){
UI_RemoveBotsMenu(1);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_removebots2") == 0 ){
UI_RemoveBotsMenu(2);
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_sandbox") == 0 ){
UI_SandboxMainMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_saveconfig") == 0 ){
UI_SaveConfigMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_saveconfiged") == 0 ){
UI_saveMapEdMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_serverinfo") == 0 ){
UI_ServerInfoMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_servers2") == 0 ){
UI_ArenaServersMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_setup") == 0 ){
UI_SetupMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_sound") == 0 ){
UI_SoundOptionsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_specifyserver") == 0 ){
UI_SpecifyServerMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_team") == 0 ){
UI_TeamMainMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_teamorders") == 0 ){
UI_BotCommandMenu_f();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_video") == 0 ){
UI_GraphicsOptionsMenu();
return qtrue;
}
if( Q_stricmp (UI_Argv(0), "ui_workshop") == 0 ){
UI_WorkshopMenu();
return qtrue;
}

	if ( Q_stricmp (UI_Argv(0), "remapShader") == 0 ) {
		char shader1[MAX_QPATH];
		char shader2[MAX_QPATH];
		char shader3[MAX_QPATH];

		Q_strncpyz(shader1, UI_Argv(1), sizeof(shader1));
		Q_strncpyz(shader2, UI_Argv(2), sizeof(shader2));
		Q_strncpyz(shader3, UI_Argv(3), sizeof(shader3));

		trap_R_RemapShader(shader1, shader2, shader3);

		return qtrue;
	}
	
	if ( Q_stricmp (UI_Argv(0), "concat") == 0 ) {
		
		trap_Cmd_ExecuteText( EXEC_INSERT, va("%s \"%s\"", UI_Argv(1), UI_ConcatArgs(2)));

		return qtrue;
	}

	if ( Q_stricmp (UI_Argv(0), "nsgui") == 0 ) {
		trap_Cmd_ExecuteText( EXEC_INSERT, va("ns_openscript_ui \"nsgui/%s\"", UI_ConcatArgs(1)));
		return qtrue;
	}

	if ( Q_stricmp (UI_Argv(0), "ns_openscript_ui") == 0 ) {
		NS_OpenScript(UI_Argv(1), NULL, 0);
		return qtrue;
	}

	if ( Q_stricmp (UI_Argv(0), "ns_interpret_ui") == 0 ) {
		Interpret(UI_ConcatArgs(1));
		return qtrue;
	}

	if ( Q_stricmp (UI_Argv(0), "ns_variablelist_ui") == 0 ) {
		print_variables();
		return qtrue;
	}

	if ( Q_stricmp (UI_Argv(0), "ns_threadlist_ui") == 0 ) {
		print_threads();
		return qtrue;
	}

	if ( Q_stricmp (UI_Argv(0), "ns_sendvariable_ui") == 0 ) {
  		if(!variable_exists(UI_Argv(1))){
			create_variable(UI_Argv(1), UI_Argv(2), atoi(UI_Argv(3)));
		}

		set_variable_value(UI_Argv(1), UI_Argv(2), atoi(UI_Argv(3)));
	}

	if ( Q_stricmp (cmd, "workshop") == 0 ) {
		UI_WorkshopMenu();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "reloadgame") == 0 ) {
		MainMenu_ReloadGame();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "menuback") == 0 ) {
		UI_PopMenu();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "save_menu") == 0 ) {
		UI_CinematicsMenu_f(0);
		return qtrue;
	}
	
	if ( Q_stricmp (cmd, "load_menu") == 0 ) {
		UI_CinematicsMenu_f(1);
		return qtrue;
	}

	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}

/*
=================
UI_Init
=================
*/
void UI_Init( void ) {
	
	UI_RegisterCvars();

	UI_InitGameinfo();

	// cache redundant calulations
	trap_GetGlconfig( &uis.glconfig );
	

	// for native screen
	uis.sw = uis.glconfig.vidWidth;
	uis.sh = uis.glconfig.vidHeight;
	//uis.scale = uis.glconfig.vidHeight * (1.0/uis.glconfig.vidHeight);
	//uis.bias = 0;
	// for 640x480 virtualized screen
	uis.scale = (uis.glconfig.vidWidth * (1.0 / 640.0) < uis.glconfig.vidHeight * (1.0 / 480.0)) ? uis.glconfig.vidWidth * (1.0 / 640.0) : uis.glconfig.vidHeight * (1.0 / 480.0);
	
	if ( uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640 ) {
		// wide screen
		uis.bias = 0.5 * ( uis.glconfig.vidWidth - ( uis.glconfig.vidHeight * (640.0 / 480.0) ) );
	} else if ( uis.glconfig.vidWidth * 480 < uis.glconfig.vidHeight * 640 ) {
		// 5:4 screen
		uis.bias = 0;
	} else {
		// no wide screen
		uis.bias = 0;
	}

	// initialize the menu system
	Menu_Cache();

	uis.activemenu = NULL;
	uis.menusp     = 0;

	NS_OpenScript("nscript/ui/init.ns", NULL, 0);		//Noire.Script Init in ui.qvm
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// expect valid pointers
	*x = *x * uis.scale + uis.bias;
	*y *= uis.scale;
	*y += uis.menuscroll;
	*w *= uis.scale;
	*h *= uis.scale;
}


void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap_R_RegisterShaderNoMip( picname );
	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

void UI_DrawHandlePicFile( float x, float y, float w, float h, const char* file ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;
	int 	file_len;
	qhandle_t hShader;
	
	file_len = strlen(file);
	
	hShader = trap_R_RegisterShaderNoMip("nsgui_e");
	if (Q_stricmp(file + file_len - 4, ".ogg") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_m");
	if (Q_stricmp(file + file_len - 4, ".wav") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_m");
	if (Q_stricmp(file + file_len - 5, ".opus") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_m");
	if (Q_stricmp(file + file_len - 7, ".shader") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_s");
	if (Q_stricmp(file + file_len - 8, ".shaderx") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_s");
	if (Q_stricmp(file + file_len - 3, ".ns") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_s");
	if (Q_stricmp(file + file_len - 4, ".cfg") == 0)
	hShader = trap_R_RegisterShaderNoMip("nsgui_s");

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

void UI_DrawHandleModel( float x, float y, float w, float h, const char* model, int scale ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	
	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );

	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.time = uis.realtime;

	origin[0] = scale;
	origin[1] = 0;
	origin[2] = 0;

	trap_R_ClearScene();

	// add the model

	memset( &ent, 0, sizeof(ent) );

	VectorSet( angles, 0, 180 - 15, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = trap_R_RegisterModel( model );
	ent.shaderRGBA[0] = 128;
	ent.shaderRGBA[1] = 128;
	ent.shaderRGBA[2] = 128;
	ent.shaderRGBA[3] = 255;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );
	ent.customSkin = trap_R_RegisterSkin(va("ptex/%s/%i.skin", model, 0));

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );
}

void UI_DrawBackgroundPic( qhandle_t hShader ) {
	trap_R_DrawStretchPic( 0.0, 0.0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, hShader );
}

/*
================
UI_FillRect2

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect2( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	//UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

void UI_DrawRoundedRect(float x, float y, float width, float height, float radius, const float *color) {
    
	UI_AdjustFrom640( &x, &y, &width, &height );
	
	if(radius*2 > height){ radius=height*0.5; }
	if(radius*2 > width){ radius=width*0.5; }
	
	// Рисование углов
	UI_SetColor( color );
    trap_R_DrawStretchPic(x, y, radius, radius, 1, 0, 0, 1, uis.corner); // Левый верхний угол
    trap_R_DrawStretchPic(x + width - radius, y, radius, radius, 0, 0, 1, 1, uis.corner); // Правый верхний угол
    trap_R_DrawStretchPic(x, y + height - radius, radius, radius, 1, 1, 0, 0, uis.corner); // Левый нижний угол
    trap_R_DrawStretchPic(x + width - radius, y + height - radius, radius, radius, 0, 1, 1, 0, uis.corner); // Правый нижний угол

    // Рисование боковых сторон
    UI_FillRect2(x, y + radius, radius, height - (radius * 2), color); // Левая сторона
    UI_FillRect2(x + width - radius, y + radius, radius, height - (radius * 2), color); // Правая сторона
    UI_FillRect2(x + radius, y, width - (radius * 2), height, color); // Верхняя сторона
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );

	trap_R_DrawStretchPic( x, y, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y, 1, height, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y + height - 1, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x + width - 1, y, 1, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	trap_R_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	trap_UpdateScreen();
}

/*
###############
Noire.Script API - Threads
###############
*/

char uiThreadBuffer[MAX_CYCLE_SIZE];

// Load threads
void RunScriptThreads(int time) {
    int i;

    for (i = 0; i < threadsCount; i++) {
        ScriptLoop* script = &threadsLoops[i];
        if (time - script->lastRunTime >= script->interval) {
            // Обновляем время последнего запуска
            script->lastRunTime = time;

            // Используем временный буфер для выполнения скрипта
            strncpy(uiThreadBuffer, script->code, MAX_CYCLE_SIZE - 1);
            uiThreadBuffer[MAX_CYCLE_SIZE - 1] = '\0'; // Убедимся, что буфер терминальный

            Interpret(uiThreadBuffer); // Запускаем скрипт из временного буфера
        }
    }
}

/*
=================
UI_Refresh
=================
*/
void UI_Refresh( int realtime )
{
	int x;
	uis.frametime = realtime - uis.realtime;
	uis.realtime  = realtime;

	if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	UI_UpdateCvars();

	RunScriptThreads(uis.realtime);		//Noire.Script - run threads

	if ( uis.activemenu )
	{
		if (uis.activemenu->fullscreen)
		{
			if(!uis.onmap){
			// draw the background
			trap_R_DrawStretchPic( 0.0, 0.0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, uis.menuWallpapers );
			}
			trap_R_DrawStretchPic( 0.0, 0.0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 0.5, 1, trap_R_RegisterShaderNoMip( "menu/art/blacktrans" ) );
		}

		if (uis.activemenu->draw)
			uis.activemenu->draw();
		else
			Menu_Draw( uis.activemenu );

		if( uis.firstdraw ) {
			UI_MouseEvent( 0, 0 );
			uis.firstdraw = qfalse;
		}
	}
	
	trap_GetGlconfig( &uis.glconfig );

	if(uis.activemenu->native > 0){
		uis.scale = uis.glconfig.vidHeight * (1.0/uis.glconfig.vidHeight);
		uis.bias = 0;
	} else {
		if ( uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640 ) {
		// wide screen
		uis.bias = 0.5 * ( uis.glconfig.vidWidth - ( uis.glconfig.vidHeight * (640.0/480.0) ) );
		uis.scale = uis.glconfig.vidHeight * (1.0/480.0);
		}
		else {
		// no wide screen
		uis.scale = (uis.glconfig.vidWidth * (1.0 / 640.0) < uis.glconfig.vidHeight * (1.0 / 480.0)) ? uis.glconfig.vidWidth * (1.0 / 640.0) : uis.glconfig.vidHeight * (1.0 / 480.0);
		uis.bias = 0;
		}
	}

	// draw cursor
	if (!uis.hideCursor) {
		UI_SetColor( NULL );
		if(uis.activemenu->native > 0){
		UI_DrawHandlePic( uis.cursorx-16*(uis.glconfig.vidWidth/640), uis.cursory-16*(uis.glconfig.vidWidth/640), 32*(uis.glconfig.vidWidth/640), 32*(uis.glconfig.vidWidth/640), uis.cursor);
		} else {
		UI_DrawHandlePic( uis.cursorx-16, uis.cursory-16, 32, 32, uis.cursor);
		}
	}

#ifndef NDEBUG
	if (uis.debug)
	{
		// cursor coordinates
		if(uis.activemenu->native){
			x = 0;
		} else {
			x = 0-uis.wideoffset;
		}
		UI_DrawString( x, 0, va("cursor xy: (%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
		UI_DrawString( x, 10, va("native: %i",uis.activemenu->native), UI_LEFT|UI_SMALLFONT, colorRed );
		UI_DrawString( x, 20, va("screen: %ix%i",uis.glconfig.vidWidth, uis.glconfig.vidHeight), UI_LEFT|UI_SMALLFONT, colorRed );
		UI_DrawString( x, 30, va("map running: %i",uis.onmap), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		trap_S_StartLocalSound( menu_in_sound, CHAN_LOCAL_SOUND );
		m_entersound = qfalse;
	}
	
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uis.cursorx < x ||
		uis.cursory < y ||
		uis.cursorx > x+width ||
		uis.cursory > y+height)
		return qfalse;

	return qtrue;
}
