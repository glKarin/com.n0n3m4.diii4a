/*
===========================================================================
// Project: Quake Sandbox - Noire.Script
// File: ns_interface.c
// Description: Noire.Script (NS) is a lightweight scripting 
//              language designed for Quake Sandbox. It enables 
//              dynamic interaction with game logic, UI, and 
//              server-side functionality, offering flexibility 
//              in modding and gameplay customization.
// Features: - Support for game events and triggers
//           - Integration with game entities and UI
//           - Easy-to-write syntax for creating complex behaviors
//           - Modular structure for server and client-side scripts
===========================================================================
*/

#include "ns_local.h"
extern void *UI_Alloc( int size ); //karin: missing decl

typedef struct {
	menuframework_s	menu;
	menuobject_s	item[MAX_OBJECTS];
	char* lists[MAX_OBJECTS][65536];
	char  listnames[MAX_OBJECTS][65536];
} nsgui_t;

static nsgui_t s_nsgui;

void NSGUI_UpdateValues( void ){
	char buffer[MAX_VAR_CHAR_BUF];
	int i;
	for ( i = 1; i < MAX_OBJECTS-1; i++ ) {
			if(s_nsgui.item[i].type == 4){
				set_variable_value(va("gui%i_value", i), s_nsgui.item[i].field.buffer, TYPE_CHAR);
			}
			if(s_nsgui.item[i].type == 5){
				if(s_nsgui.item[i].mode <= 0){
					set_variable_value(va("gui%i_value", i), s_nsgui.item[i].itemnames[s_nsgui.item[i].curvalue], TYPE_CHAR);
				}
				if(s_nsgui.item[i].mode == 1){
					set_variable_value(va("gui%i_value", i), va("%i", s_nsgui.item[i].curvalue), TYPE_INT);
				}
			}
			if(s_nsgui.item[i].type == 7){
				if(s_nsgui.item[i].mode <= 0){
				set_variable_value(va("gui%i_value", i), va("%i", s_nsgui.item[i].curvalue), TYPE_INT);
				}
				if(s_nsgui.item[i].mode == 1){
				set_variable_value(va("gui%i_value", i), va("%i", !(s_nsgui.item[i].curvalue)), TYPE_INT);
				}
			}
			if(s_nsgui.item[i].type == 8){
				set_variable_value(va("gui%i_value", i), va("%.6f", (float)s_nsgui.item[i].curvalue / (float)s_nsgui.item[i].mode), TYPE_FLOAT);
			}
	}
}

void NSGUI_Event (void* ptr, int event) {
	int i;
	char temp_cmd[MAX_VAR_CHAR_BUF];
	
	if(get_variable_int(va("gui%i_acttype", ((menucommon_s*)ptr)->id)) <= 1){
	if( event != QM_ACTIVATED ) { return; }
	}
	if(get_variable_int(va("gui%i_acttype", ((menucommon_s*)ptr)->id)) == 2){
	if( event != QM_LOSTFOCUS ) { return; }
	}
	if(get_variable_int(va("gui%i_acttype", ((menucommon_s*)ptr)->id)) == 3){
	if( event != QM_GOTFOCUS ) { return; }
	}

	NSGUI_UpdateValues();
		
	for ( i = 1; i < MAX_OBJECTS-1; i++ ) {
		if(i == ((menucommon_s*)ptr)->id){
		NS_ArgumentText(((menucommon_s*)ptr)->cmd, temp_cmd, sizeof(temp_cmd));
		Interpret(temp_cmd);
		}
	}
}

sfxHandle_t NSGUI_Key(int key)
{
	switch ( key )
	{
		case K_ESCAPE:
			UI_PopMenu();
			return menu_out_sound;
	}
	return 0;
}

/*
===============
NSGUI_MenuDraw
===============
*/

refdef_t		nsguirender[MAX_OBJECTS];
refEntity_t		nsguientity[MAX_OBJECTS];

static void NSGUI_MenuDraw( void ) {
	vec4_t			color1 = {0.85, 0.9, 1.0, 1};

	Menu_Draw( &s_nsgui.menu );

	if (uis.debug) {
	UI_DrawString( 320, 1, "NS Gui v1.0 by Noire.dev", UI_CENTER|UI_SMALLFONT, color1 );
	}
}

/*
===============
NSGUI
===============
*/
void NSGUI_Clear( void ) {
	int i;
	for ( i = 1; i < MAX_OBJECTS-1; i++ ) {
	UI_Free(s_nsgui.item[i].generic.cmd);
	UI_Free(s_nsgui.item[i].generic.text);
	UI_Free(s_nsgui.item[i].generic.picn);
	UI_Free(s_nsgui.item[i].string);
	}
	memset( &s_nsgui, 0 ,sizeof(nsgui_t) );

	set_variable_value("gui_scroll", "", TYPE_INT);
	for ( i = 1; i < MAX_OBJECTS; i++ ) {
		set_variable_value(va("gui%i_type", i), "", TYPE_INT);
		set_variable_value(va("gui%i_acttype", i), "", TYPE_INT);
		set_variable_value(va("gui%i_x", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_y", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_w", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_h", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_text", i), "", TYPE_CHAR);
		set_variable_value(va("gui%i_cmd", i), "", TYPE_CHAR);
		set_variable_value(va("gui%i_file", i), "", TYPE_CHAR);
		set_variable_value(va("gui%i_value", i), "", TYPE_CHAR);
		set_variable_value(va("gui%i_colorR", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorG", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorB", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorA", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorinnerR", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorinnerG", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorinnerB", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_colorinnerA", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_fontsize", i), "", TYPE_FLOAT);
		set_variable_value(va("gui%i_corner", i), "", TYPE_INT);
		set_variable_value(va("gui%i_col", i), "", TYPE_INT);
		set_variable_value(va("gui%i_mode", i), "", TYPE_INT);
		set_variable_value(va("gui%i_style", i), "", TYPE_INT);
		set_variable_value(va("gui%i_min", i), "", TYPE_INT);
		set_variable_value(va("gui%i_max", i), "", TYPE_INT);
	}
}

void NSGUI_Init( void ) {
	int i;
	if(variable_exists("gui_scroll")){
		return;
	}
	create_variable("gui_scroll", "", TYPE_INT);
	for ( i = 1; i < MAX_OBJECTS; i++ ) {
		create_variable(va("gui%i_type", i), "", TYPE_INT);
		create_variable(va("gui%i_acttype", i), "", TYPE_INT);
		create_variable(va("gui%i_x", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_y", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_w", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_h", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_text", i), "", TYPE_CHAR);
		create_variable(va("gui%i_cmd", i), "", TYPE_CHAR);
		create_variable(va("gui%i_file", i), "", TYPE_CHAR);
		create_variable(va("gui%i_value", i), "", TYPE_CHAR);
		create_variable(va("gui%i_colorR", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorG", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorB", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorA", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorinnerR", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorinnerG", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorinnerB", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_colorinnerA", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_fontsize", i), "", TYPE_FLOAT);
		create_variable(va("gui%i_corner", i), "", TYPE_INT);
		create_variable(va("gui%i_col", i), "", TYPE_INT);
		create_variable(va("gui%i_style", i), "", TYPE_INT);
		create_variable(va("gui%i_min", i), "", TYPE_INT);
		create_variable(va("gui%i_max", i), "", TYPE_INT);
		create_variable(va("gui%i_mode", i), "", TYPE_INT);
	}
}

/*
===============
LoadNSGuiList
===============
*/
static void LoadNSGuiList( int it )
{
	int 	i;
	int		len;
	char	*configname;
	
	if (!s_nsgui.item[it].numitems) {
		strcpy(s_nsgui.listnames[it],"No files");
		s_nsgui.item[it].numitems = 1;
	}
	else if (s_nsgui.item[it].numitems > 65536)
		s_nsgui.item[it].numitems = 65536;

	configname = s_nsgui.listnames[it];
	for ( i = 0; i < s_nsgui.item[it].numitems; i++ ) {
		s_nsgui.item[it].itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - (strlen(s_nsgui.item[it].generic.picn)+1),va(".%s", s_nsgui.item[it].generic.picn)))
			configname[len-(strlen(s_nsgui.item[it].generic.picn)+1)] = '\0';

		configname += len + 1;
	}
}

void UI_NSGUI( void ) {
	int i;
	int type;
	char text[256];
	char command[256];
	char pic[256];
	char initvalue[256];
	vec4_t color_nsgui[MAX_OBJECTS]	    = {1.00f, 0.00f, 1.00f, 1.00f};
	vec4_t color_nsgui2[MAX_OBJECTS]	    = {1.00f, 1.00f, 1.00f, 1.00f};

	for ( i = 1; i < MAX_OBJECTS-1; i++ ) {
	UI_Free(s_nsgui.item[i].generic.cmd);
	UI_Free(s_nsgui.item[i].generic.text);
	UI_Free(s_nsgui.item[i].generic.picn);
	UI_Free(s_nsgui.item[i].string);
	}
	
	memset( &s_nsgui, 0 ,sizeof(nsgui_t) );
	
	s_nsgui.menu.draw = NSGUI_MenuDraw;
	if(!uis.onmap){
	s_nsgui.menu.fullscreen = qtrue;
	} else {
	s_nsgui.menu.fullscreen = qfalse;	
	}
	s_nsgui.menu.wrapAround = qtrue;
	s_nsgui.menu.native = qfalse;
	s_nsgui.menu.downlimitscroll = get_variable_float("gui_scroll");
	//s_nsgui.menu.key = NSGUI_Key;

	for ( i = 1; i < MAX_OBJECTS-1; i++ ) {
	type = get_variable_int(va("gui%i_type", i));
	if(type >= 1 && type <= 8){
	strncpy(text, get_variable_char(va("gui%i_text", i)), sizeof(text));
	strncpy(command, get_variable_char(va("gui%i_cmd", i)), sizeof(command));
	strncpy(pic, get_variable_char(va("gui%i_file", i)), sizeof(pic));
	color_nsgui[i][0] = get_variable_float(va("gui%i_colorR", i));
	color_nsgui[i][1] = get_variable_float(va("gui%i_colorG", i));
	color_nsgui[i][2] = get_variable_float(va("gui%i_colorB", i));
	color_nsgui[i][3] = get_variable_float(va("gui%i_colorA", i));
	color_nsgui2[i][0] = get_variable_float(va("gui%i_colorinnerR", i));
	color_nsgui2[i][1] = get_variable_float(va("gui%i_colorinnerG", i));
	color_nsgui2[i][2] = get_variable_float(va("gui%i_colorinnerB", i));
	color_nsgui2[i][3] = get_variable_float(va("gui%i_colorinnerA", i));
	s_nsgui.item[i].generic.type			= MTYPE_UIOBJECT;
	if(strlen(command) >= 1){
	s_nsgui.item[i].generic.flags		= QMF_CENTER_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	} else {
	s_nsgui.item[i].generic.flags		= QMF_CENTER_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS|QMF_INACTIVE;
	}
	s_nsgui.item[i].generic.id				= i;
    s_nsgui.item[i].generic.cmd 			= (char *)UI_Alloc(256);
    s_nsgui.item[i].generic.picn 			= (char *)UI_Alloc(256);
    s_nsgui.item[i].string 					= (char *)UI_Alloc(256);
	s_nsgui.item[i].generic.callback		= NSGUI_Event;
	s_nsgui.item[i].type					= type;
	s_nsgui.item[i].generic.x				= get_variable_float(va("gui%i_x", i));
	s_nsgui.item[i].generic.y				= get_variable_float(va("gui%i_y", i));
	s_nsgui.item[i].width					= get_variable_float(va("gui%i_w", i));
	s_nsgui.item[i].height					= get_variable_float(va("gui%i_h", i));
	s_nsgui.item[i].color					= color_nsgui[i];
	s_nsgui.item[i].color2					= color_nsgui2[i];
	s_nsgui.item[i].corner					= get_variable_int(va("gui%i_corner", i));
	s_nsgui.item[i].mode					= get_variable_int(va("gui%i_mode", i));
	s_nsgui.item[i].fontsize				= get_variable_float(va("gui%i_fontsize", i));
	s_nsgui.item[i].styles		    		= get_variable_float(va("gui%i_style", i));
	s_nsgui.item[i].style		    		= UI_SMALLFONT;
	strcpy(s_nsgui.item[i].string, text);
	strcpy(s_nsgui.item[i].generic.cmd, command);
	strcpy(s_nsgui.item[i].generic.picn, pic);
	if(type == 4){
	s_nsgui.item[i].field.widthInChars	= get_variable_float(va("gui%i_w", i));
	s_nsgui.item[i].field.maxchars		= get_variable_float(va("gui%i_w", i));
	s_nsgui.item[i].generic.text = (char *)UI_Alloc(256);
	strcpy(s_nsgui.item[i].generic.text, text);
	}
	if(type == 5){
	if(strlen(command) >= 1){
	s_nsgui.item[i].generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	} else {
	s_nsgui.item[i].generic.flags		= QMF_HIGHLIGHT_IF_FOCUS|QMF_INACTIVE;
	}
	s_nsgui.item[i].numitems			= trap_FS_GetFileList( text, pic, s_nsgui.listnames[i], 65536 );
	s_nsgui.item[i].itemnames			= (const char **)s_nsgui.lists[i];
	s_nsgui.item[i].columns				= get_variable_int(va("gui%i_col", i));
	s_nsgui.item[i].seperation			= 0;
	}
	if(type == 7){
	s_nsgui.item[i].generic.text = (char *)UI_Alloc(256);
	strcpy(s_nsgui.item[i].generic.text, text);
	}
	if(type == 8){
	s_nsgui.item[i].generic.text = (char *)UI_Alloc(256);
	strcpy(s_nsgui.item[i].generic.text, text);
	s_nsgui.item[i].minvalue = get_variable_int(va("gui%i_min", i));
	s_nsgui.item[i].maxvalue = get_variable_int(va("gui%i_max", i));
	}
	}
	}

	for ( i = 1; i < MAX_OBJECTS-1; i++ ) {
		if(s_nsgui.item[i].type >= 1 && s_nsgui.item[i].type <= 8){
			if(s_nsgui.item[i].type == 5){
				LoadNSGuiList(i);
			}
			Menu_AddItem( &s_nsgui.menu, &s_nsgui.item[i] );
		}
		if(strlen(get_variable_char(va("gui%i_value", i))) != 0){
			if(s_nsgui.item[i].type == 4){
				Q_strncpyz( s_nsgui.item[i].field.buffer, get_variable_char(va("gui%i_value", i)), sizeof(s_nsgui.item[i].field.buffer) );
			}
			if(s_nsgui.item[i].type == 5){
				if(s_nsgui.item[i].mode == 1){
					s_nsgui.item[i].curvalue = get_variable_int(va("gui%i_value", i));
				}
			}
			if(s_nsgui.item[i].type == 7){
				if(s_nsgui.item[i].mode <= 0){
				s_nsgui.item[i].curvalue = get_variable_int(va("gui%i_value", i));
				}
				if(s_nsgui.item[i].mode == 1){
				s_nsgui.item[i].curvalue = !(get_variable_int(va("gui%i_value", i)));
				}
			}
			if(s_nsgui.item[i].type == 8){
				s_nsgui.item[i].curvalue = get_variable_float(va("gui%i_value", i)) * (float)s_nsgui.item[i].mode;
			}
		}
	}
}

void NSGUI_Load( void ) {
	UI_NSGUI();
	UI_PushMenu( &s_nsgui.menu );
}
