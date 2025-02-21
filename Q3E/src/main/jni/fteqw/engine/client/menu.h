/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//There are still menu states, which say which menu gets absolute control.
//Some of the destinations are healthier than others. :)

//m_none	- menu is disabled
//m_complex	- hirachy of item types
//m_help	- old q1 style help menu (fixme: make m_complex)
//m_plugin	- A QVM based or DLL based plugin.


//the m_complex menu state is the most advanced, and drives the bulk of FTE's menus in an event driven way.
//It consists of menus and items. Fairly basic really.
//Each item type has a structure (or shares a structure).
//Each of these structures contain a menucommon_t.
//The backend of this system lives in m_items.c.
//If you're creating your own quake menu, there should be little need to go in there.
//These are the item types:

//mt_childwindow	-
//mt_button			- Executes a console command or callback on enter. Uses conchars.
//mt_buttonbigfont	- Used by hexen2's menus. Uses gfx/menu/bigfont.lmp as it's characters.
//mt_box			- A 2d box. The same one as the quit dialog from q1, but resized.
//mt_colouredbox	- Not used.
//mt_line			- Not used.
//mt_edit			- A one row edit box, either attached to a cvar, or an apply button.
//mt_text			- unselectable. Otherwise like mt_button
//mt_slider			- a horizontal slider, like quake's gamma option, attached to a cvar.
//mt_combo			- multiple specific options. Created with specifically structured info.
//mt_bind			- a key binding option.
//mt_checkbox		- a yes/no toggle, attached to a cvar.
//mt_picture		- Just draws a lmp from it's x/y.
//mt_menudot		- The 24*24 rotating quake menudot. Should be specified as the cursoritem, and should be placed to match the selecteditem's y position.
//mt_custom			- Just an option with callbacks. This is the basis of the top/bottom color seletion, and could be used for all sorts of things.


//Sample menu creation, entirly within / * and * /
//Note that most of FTE's menus are more complicated, as FTE runs on Q1/Q2/H2 data, and it's choice of menu images reflects this.
//Most of the normal menus also have more items too, of course.

//FTE's menu console commands are registered from M_Init_Internal instead of M_Init as implied here. Why?
//FTE's menu.dat support unregisters the menu console commands so the menu.dat can use those commands instead.
//This results in more user friendliness for mods but makes the code a little more confusing.
//If you make the menu name unique enough, then there's no need to follow the standard menu code.
/*
//M_SomeMenuConsoleCommand_f
//Spawns a sample menu.
void M_SomeMenuConsoleCommand_f (void)
{
	menu_t *m = M_CreateMenu(0);
	int y = 32;

	//add the title
	MC_AddCenterPicture(m, 4, 24, "gfx/p_option.lmp");

	//add the blinking > thingie
	//(note NULL instead of a valid string, this should really be a variant of mt_menudot instead)
	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 200, y, NULL, false);

	//Set up so the first item is selected. :)
	m->selecteditem = (menuoption_t*)

	//Add the items.
	MC_AddConsoleCommand(menu, 16, y,	"    Customize controls", "menu_keys\n"); y+=8;
	MC_AddSlider(menu, 16, y,			"           Mouse Speed", &sensitivity,		1,		10); y+=8;
	MC_AddCheckBox(menu, 16, y,			"            Lookstrafe", &lookstrafe,0); y+=8;
}

//eg: M_Init
void M_SomeInitialisationFunctionCalledAtStartup(void)
{
	Cmd_AddCommand("menu_somemenu", M_SomeMenuConsoleCommand_f);
}
*/

void M_DrawTextBox (int x, int y, int width, int lines);

//menus can have all sorts of implementation.
//however, to avoid a cacophony of ugly blends, these are always mutually exclusive - only one type of menu will be at the top and only that one will be drawn.
//if you want windows etc you can implement that inside a single one of these.
//each menu subsystem can implement this and then provide its own widgets.
typedef struct menu_s {
	struct menu_s *prev;

	void *ctx;	//for finding a specific menu
	void (*videoreset)	(struct menu_s *);	//called after a video mode switch / shader reload.
	void (*release)		(struct menu_s *, qboolean forced);	//forced says 'dont load any other menus'.
	qboolean (*keyevent)(struct menu_s *, qboolean isdown, unsigned int devid, int key, int unicode);	//true if key was handled
	qboolean (*mousemove)(struct menu_s *, qboolean abs, unsigned int devid, float x, float y);
	qboolean (*joyaxis)	(struct menu_s *, unsigned int devid, int axis, float val);
	void (*drawmenu)	(struct menu_s *);
	struct key_cursor_s *cursor; //NULL for relative motion
	qboolean lowpriority;	//appears underneath other menus.
	qboolean isopaque;	//guarentees an opaque background
	qboolean persist;	//try really hard to not kill this.
	qboolean showosk;
} menu_t;
extern menu_t *topmenu;		//the currently visible menu.
extern menu_t *promptmenu;	//the currently visible prompt (separate from menus, so they always appear over the top of consoles too, they also always show the menu underneath)
void Menu_KeyEvent(qboolean down, int qdeviceid, int key, int unicode);
int Menu_WantOSK(void);
void Menu_Draw(void);
void Prompts_Draw(void);
void Menu_PopAll(void); //attempts to pop all menus (this is for map starts, some might linger)
void Menu_Unlink(menu_t *menu, qboolean forced);
void Menu_Push(menu_t *menu, qboolean prompt);
menu_t *Menu_FindContext(void *ctx);
qboolean Menu_IsLinked(menu_t *menu);

typedef enum
{
	PROMPT_YES		= 0,
	PROMPT_NO		= 1,
	PROMPT_CANCEL	= -1,
} promptbutton_t;
#ifdef HAVE_CLIENT
void Menu_Prompt (void (*callback)(void *, promptbutton_t), void *ctx, const char *messages, const char *optionyes, const char *optionno, const char *optioncancel, qboolean highpri);
#define Menu_PromptOrPrint(messages,optioncancel,highpri) Menu_Prompt(NULL, NULL, messages, NULL, NULL, optioncancel, highpri)
#else
#define Menu_PromptOrPrint(messages,optioncancel,highpri) Con_Printf("%s", messages)
#endif

void M_Window_ClosePrompt (void);	//called when the window was requested to be closed. displays whatever quit menu is appropriate.
#ifndef NOBUILTINMENUS

//
// menus
//
void M_Init (void);
void M_Reinit(void);
void M_Shutdown(qboolean total);
void M_Menu_Mods_f (void);	//used at startup if the current gamedirs look dodgy.
void M_Menu_Installer (void);	//given an embedded manifest, this displays an install menu for said game.
mpic_t	*M_CachePic (char *path);
void M_Menu_Quit_f (void);
typedef struct emenu_s emenu_t;


typedef enum {
	mt_framestart,
	mt_frameend,
	mt_button,
	mt_qbuttonbigfont,
	mt_hexen2buttonbigfont,
	mt_box,
	mt_colouredbox,
	mt_line,
	mt_edit,
	mt_text,
	mt_slider,
	mt_combo,
	mt_bind,
	mt_checkbox,
	mt_picture,
	mt_picturesel,
	mt_menudot,
	mt_menucursor,
	mt_custom
} menutype_t;

typedef struct {	//must be first of each structure type.
	menutype_t type;
	int posx;
	int posy;
	int width;		//total width
	int height;		//total height
	int extracollide; // dirty hack to stretch collide box left (the real fix is to have separate collide/render rects)
	//unsigned int grav_type;
	//int grav_x;	//
	int	grav_y;		//
	//int grav_size[2];
	const char *tooltip;
	qboolean noselectionsound:1;
	qboolean iszone:1;
	qboolean ishidden:1;
	union menuoption_s *next;
} menucommon_t;


typedef struct {
	menucommon_t common;
	const char *text;
	const char *command;
	qboolean rightalign;
	qboolean (*key) (union menuoption_s *option, struct emenu_s *, int key);
} menubutton_t;

#define MAX_EDIT_LENGTH 256
typedef struct {
	menucommon_t common;
	int captionwidth;
	const char *caption;
	cvar_t *cvar;
	char text[MAX_EDIT_LENGTH];
	int cursorpos;
	qboolean modified;
	qboolean slim;
} menuedit_t;
typedef struct {
	menucommon_t common;
	float min;
	float max;
	float current;
	float smallchange;
	float largechange;
	float vx;
	cvar_t *var;
	int textwidth;
	const char *text;
} menuslider_t;

typedef enum {CHK_CHECKED, CHK_TOGGLE} chk_set_t;
typedef struct menucheck_s {
	menucommon_t common;
	const char *text;
	int textwidth;
	cvar_t *var;
	int bits;
	float value;
	qboolean (*func) (struct menucheck_s *option, struct emenu_s *menu, chk_set_t set);
} menucheck_t;

typedef struct {
	menucommon_t common;
	const char *text;
	int rightalign;
	qboolean isred;
} menutext_t;

typedef struct menucustom_s {
	menucommon_t common;
	void *dptr;
	void *dptr2;
	int dint;
	void (*draw) (int x, int y, struct menucustom_s *, struct emenu_s *);
	qboolean (*key) (struct menucustom_s *, struct emenu_s *, int key, unsigned int unicode);
} menucustom_t;

typedef struct {
	menucommon_t common;
	char *picturename;
} menupicture_t;

typedef struct {
	menucommon_t common;
	int width;
	int height;
} menubox_t;

typedef struct {
	menucommon_t common;

	int captionwidth;
	char const*caption;
	char const*const*options;
	char const*const*values;
	cvar_t *cvar;
	int numoptions;
	int selectedoption;
} menucombo_t;

typedef struct {
	menucommon_t common;
	int captionwidth;
	char *caption;
	char *command;
} menubind_t;

typedef struct {
	menucommon_t common;
	qboolean mousedown;
	float frac;
	union menuoption_s *suboptions;
} menuframe_t;

typedef union menuoption_s {
	menucommon_t	common;
	menubutton_t	button;
	menuedit_t		edit;
	menucombo_t		combo;
	menuslider_t	slider;
	menutext_t		text;
	menucustom_t	custom;
	menupicture_t	picture;
	menubox_t		box;
	menucheck_t		check;
	menubind_t		bind;
	menuframe_t		frame;
} menuoption_t;

typedef struct menutooltip_s {
	conchar_t *end;
	conchar_t text[1];
} menutooltip_t;

typedef struct menuresel_s	//THIS STRUCT MUST BE STATICALLY ALLOCATED.
{
	int x, y;
} menuresel_t;

struct emenu_s {
	menu_t menu;

	int xpos;
	int ypos;
	int width;
	int height;
	qboolean dontexpand;		//false=recenter xpos
	int numoptions;
	menuresel_t *reselection;	//stores some info to restore selection properly.

	qboolean	iszone;
	qboolean	nobacktint;

	void *data;	//typecast

	void (*reset)		(struct emenu_s *);	//called after a video mode switch / shader reload.
	void (*remove)		(struct emenu_s *);
	qboolean (*key)		(struct emenu_s *, int key, unsigned int unicode);	//true if key was handled
	void (*predraw)		(struct emenu_s *);
	void (*postdraw)	(struct emenu_s *);
	menuoption_t *options;

	menuoption_t *selecteditem;
	menuoption_t *mouseitem;

	menutooltip_t *tooltip;
	double tooltiptime;

	int cursorpos;
	menuoption_t *cursoritem;
};

menutext_t *MC_AddBufferedText(emenu_t *menu, int lhs, int rhs, int y, const char *text, int rightalign, qboolean red);
menutext_t *MC_AddRedText(emenu_t *menu, int lhs, int rhs, int y, const char *text, int rightalign);
menutext_t *MC_AddWhiteText(emenu_t *menu, int lhs, int rhs, int y, const char *text, int rightalign);
menubind_t *MC_AddBind(emenu_t *menu, int cx, int bx, int y, const char *caption, char *command, const char *tooltip);
menubox_t *MC_AddBox(emenu_t *menu, int x, int y, int width, int height);
menupicture_t *MC_AddPicture(emenu_t *menu, int x, int y, int width, int height, const char *picname);
menupicture_t *MC_AddSelectablePicture(emenu_t *menu, int x, int y, int height, const char *picname);
menupicture_t *MC_AddCenterPicture(emenu_t *menu, int y, int height, const char *picname);
menupicture_t *MC_AddCursor(emenu_t *menu, menuresel_t *resel, int x, int y);
menuoption_t *MC_AddCursorSmall(emenu_t *menu, menuresel_t *reselection, int x);
menuslider_t *MC_AddSlider(emenu_t *menu, int tx, int sx, int y, const char *text, cvar_t *var, float min, float max, float delta);
menucheck_t *MC_AddCheckBox(emenu_t *menu, int tx, int cx, int y, const char *text, cvar_t *var, int cvarbitmask);
menucheck_t *MC_AddCheckBoxFunc(emenu_t *menu, int tx, int cx, int y, const char *text, qboolean (*func) (menucheck_t *option, emenu_t *menu, chk_set_t set), int bits);
menubutton_t *MC_AddConsoleCommand(emenu_t *menu, int lhs, int rhs, int y, const char *text, const char *command);
menubutton_t *MC_AddConsoleCommandQBigFont(emenu_t *menu, int x, int y, const char *text, const char *command);
void *QBigFontWorks(void);	//treat as a boolean.
menubutton_t *MC_AddConsoleCommandHexen2BigFont(emenu_t *menu, int x, int y, const char *text, const char *command);
menubutton_t *VARGS MC_AddConsoleCommandf(emenu_t *menu, int lhs, int rhs, int y, int rightalign, const char *text, char *command, ...);
menubutton_t *MC_AddCommand(emenu_t *menu, int lhs, int rhs, int y, const char *text, qboolean (*command) (union menuoption_s *,struct emenu_s *,int));
menucombo_t *MC_AddCombo(emenu_t *menu, int tx, int cx, int y, const char *caption, const char **ops, int initialvalue);
menucombo_t *MC_AddCvarCombo(emenu_t *menu, int tx, int cx, int y, const char *caption, cvar_t *cvar, const char **ops, const char **values);
menuedit_t *MC_AddEdit(emenu_t *menu, int cx, int ex, int y, const char *text, const char *def);
menuedit_t *MC_AddEditCvar(emenu_t *menu, int cx, int ex, int y, const char *text, const char *name, qboolean slim);
menucustom_t *MC_AddCustom(emenu_t *menu, int x, int y, void *dptr, int dint, const char *tooltip);
menuframe_t *MC_AddFrameStart(emenu_t *menu, int y);	//call before items are added
menuframe_t *MC_AddFrameEnd(emenu_t *menu, int y);	//and call AFTER that stuff with the same y.

typedef struct menubulk_s {
	menutype_t type;
	int variant;
	const char *text;
	const char *tooltip;
	char *consolecmd; // console command
	cvar_t *cvar; // check box, slider
	int flags; // check box
	qboolean (*func) (struct menucheck_s *option, struct emenu_s *menu, chk_set_t set); // check box
	float min; // slider
	float max; // slider
	float delta; // slider
	qboolean rightalign; // text
	qboolean (*command) (union menuoption_s *, struct emenu_s *, int); // command
	char *cvarname; // edit cvar
	const char **options; // combo
	const char **values; // cvar combo
	int selectedoption; // other combo
	union menuoption_s **ret; // other combo
	int spacing; // spacing
} menubulk_t;

#define MB_CONSOLECMD(text, cmd, tip) 								{mt_button,		0, localtext(text), localtext(tip), cmd}
#define MB_CONSOLECMDRETURN(text, cmd, tip, ret)					{mt_button,		0, localtext(text), localtext(tip), cmd, NULL, 0, NULL, 0, 0, 0, false, NULL, NULL, NULL, NULL, 0, (union menuoption_s **)&ret}
#define MB_CHECKBOXCVAR(text, cvar, cvarflag) 						{mt_checkbox,	0, localtext(text), NULL, NULL, &cvar, cvarflag}
#define MB_CHECKBOXCVARTIP(text, cvar, cvarflag, tip) 				{mt_checkbox,	0, localtext(text), localtext(tip), NULL, &cvar, cvarflag}
#define MB_CHECKBOXCVARRETURN(text, cvar, cvarflag, ret) 			{mt_checkbox,	0, localtext(text), NULL, NULL, &cvar, cvarflag, NULL, 0, 0, 0, false, NULL, NULL, NULL, NULL, 0, (union menuoption_s **)&ret}
#define MB_CHECKBOXFUNC(text, func, flags, tip) 					{mt_checkbox,	0, localtext(text), localtext(tip), NULL, NULL, flags, func}
#define MB_SLIDER(text, cvar, min, max, delta, tip) 				{mt_slider,		0, localtext(text), localtext(tip), NULL, &cvar, 0, NULL, min, max, delta}
#define MB_TEXT(text, align) 										{mt_text,		0, localtext(text), NULL, NULL, NULL, 0, NULL, 0, 0, 0, align}
#define MB_REDTEXT(text, align) 									{mt_text,		1, localtext(text), NULL, NULL, NULL, 0, NULL, 0, 0, 0, align}
#define MB_CMD(text, cmdfunc, tip) 									{mt_button,		1, localtext(text), localtext(tip), NULL, NULL, 0, NULL, 0, 0, 0, false, cmdfunc}
#define MB_EDITCVARTIP(text, cvarname, tip) 						{mt_edit,		0, localtext(text), localtext(tip), NULL, NULL, 0, NULL, 0, 0, 0, false, NULL, cvarname}
#define MB_EDITCVAR(text, cvarname) 								{mt_edit,		0, localtext(text), NULL, NULL, NULL, 0, NULL, 0, 0, 0, false, NULL, cvarname}
#define MB_EDITCVARSLIM(text, cvarname, tip) 						{mt_edit,		1, localtext(text), localtext(tip), NULL, NULL, 0, NULL, 0, 0, 0, false, NULL, cvarname}
#define MB_EDITCVARSLIMRETURN(text, cvarname, ret) 					{mt_edit,		1, localtext(text), NULL, NULL, NULL, 0, NULL, 0, 0, 0, false, NULL, cvarname, NULL, NULL, 0, (union menuoption_s **)&ret}
#define MB_COMBOCVAR(text, cvar, options, values, tip) 				{mt_combo,		0, localtext(text), localtext(tip), NULL, &cvar, 0, NULL, 0, 0, 0, false, NULL, NULL, options, values}
#define MB_COMBORETURN(text, options, selected, ret, tip) 			{mt_combo,		1, localtext(text), localtext(tip), NULL, NULL, 0, NULL, 0, 0, 0, false, NULL, NULL, options, NULL, selected, (union menuoption_s **)&ret}
#define MB_COMBOCVARRETURN(text, cvar, options, values, ret, tip) 	{mt_combo,		0, localtext(text), localtext(tip), NULL, &cvar, 0, NULL, 0, 0, 0, false, NULL, NULL, options, values, 0, (union menuoption_s **)&ret}
#define MB_SPACING(space) 											{mt_text,		2, NULL, NULL, NULL, NULL, 0, NULL, 0, 0, 0, false, NULL, NULL, NULL, NULL, 0, NULL, space}
#define MB_END() 													{mt_text, -1}

int MC_AddBulk(emenu_t *menu, menuresel_t *resel, menubulk_t *bulk, int xstart, int xtextend, int y);



emenu_t *M_Options_Title(int *y, int infosize);	/*Create a menu with the default options titlebar*/
emenu_t *M_CreateMenu (int extrasize);
void M_RemoveMenu (emenu_t *menu);
void M_RemoveAllMenus (qboolean leaveprompts);
void M_ReloadMenus(void);
menubutton_t *M_FindButton(emenu_t *menu, const char *command);

void M_Complex_Key(emenu_t *currentmenu, int key, int unicode);
void M_Script_Init(void);
void M_Serverlist_Init(void);

void M_Menu_BasedirPrompt(ftemanifest_t *man);

const char *M_ChooseAutoSave(void);
void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_Net_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Video_f (void);
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);
	void M_Menu_Preset_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

/*
void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
		void M_Net_Draw (void);
	void M_Options_Draw (void);
		void M_Video_Draw (void);
	void M_Help_Draw (void);
	void M_Quit_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
	void M_MultiPlayer_Key (int key);
		void M_Setup_Key (int key);
		void M_Net_Key (int key);
	void M_Options_Key (int key);
		void M_Video_Key (int key);
	void M_Help_Key (int key);
	void M_Quit_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);
*/

//drawing funcs
void M_BuildTranslationTable(unsigned int pc, unsigned int top, unsigned int bottom, unsigned int *translationTable);
void M_DrawCharacter (int cx, int line, unsigned int num);
void M_Print (int cx, int cy, qbyte *str);
void M_PrintWhite (int cx, int cy, qbyte *str);
void M_DrawScalePic (int x, int y, int w, int h, mpic_t *pic);


void M_UnbindCommand (const char *command);

#else
//no builtin menu code.
//stubs
//#define M_Shutdown(t) MP_Shutdown()

void M_Init (void);
void M_Reinit(void);
void M_Shutdown(qboolean total);
#endif
int M_FindKeysForCommand (int bindmap, int pnum, const char *command, int *keylist, int *keymods, int keycount);
int QDECL M_FindKeysForBind (int bindmap, const char *command, int *keylist, int *keymods, int keycount);
void M_ToggleMenu_f (void);

#ifdef MENU_DAT
void MP_CvarChanged(cvar_t *var);
qboolean MP_Init (void);
void MP_Shutdown (void);
qboolean MP_Toggle(int mode);
void MP_RendererRestarted(void);
void MP_Draw(void);
qboolean MP_UsingGamecodeLoadingScreen(void);
void MP_RegisterCvarsAndCmds(void);
int MP_BuiltinValid(const char *name, int num);
qboolean MP_ConsoleCommand(const char *cmdtext);
int MP_GetServerCategory(int index);
#else
#define MP_UsingGamecodeLoadingScreen() false
#endif

#ifdef MENU_NATIVECODE
#ifdef HAVE_CLIENT
#include "api_menu.h"
extern menu_export_t *mn_entry;
void MN_Shutdown(void);
qboolean MN_Init(void);
#endif
#endif

#define MGT_BAD    ~0
#define MGT_QUAKE1 0
#define MGT_HEXEN2 1
#define MGT_QUAKE2 2
int M_GameType(void);





//plugin functions
#ifdef PLUGINS
qboolean	Plug_CenterPrintMessage(const char *buffer, int clientnum);
qboolean	Plug_ChatMessage(char *buffer, int talkernum, int tpflags);
void		Plug_Command_f(void);
int			Plug_ConnectionlessClientPacket(char *buffer, int size);
qboolean	Plug_ConsoleLink(char *text, char *info, const char *consolename);
qboolean	Plug_ConsoleLinkMouseOver(float x, float y, char *text, char *info);
void		Plug_DrawReloadImages(void);
void		Plug_Initialise(qboolean fromgamedir);
void		Plug_Shutdown(qboolean preliminary);
qboolean	Plug_Menu_Event(int eventtype, int keyparam, int unicodeparam);
void		Plug_ResChanged(qboolean restarted);
void		Plug_SBar(playerview_t *pv);
qboolean	Plug_ServerMessage(char *buffer, int messagelevel);
void		Plug_Tick(void);
qboolean	Plugin_ExecuteString(void);
void		Plug_FreeAllImages(void);

#ifdef ANDROID
#define PLUGINPREFIX "libplug_" //android is kinda annoying and only extracts specific files.
#else
#define PLUGINPREFIX "fteplug_" //this string defines what consitutes a plugin, as opposed to some other dll
#endif
#endif
