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

#ifndef __CLIENT_KEYS_H__
#define __CLIENT_KEYS_H__

enum
{	//fte's assumed gamepad axis
	GPAXIS_LT_RIGHT	= 0,
	GPAXIS_LT_DOWN	= 1,
	GPAXIS_LT_AUX	= 2,

	GPAXIS_RT_RIGHT	= 3,
	GPAXIS_RT_DOWN	= 4,
	GPAXIS_RT_AUX	= 5,

//gah
#define GPAXIS_LT_TRIGGER GPAXIS_LT_AUX
#define GPAXIS_RT_TRIGGER GPAXIS_RT_AUX
};

//
// these are the key numbers that should be passed to Key_Event
//
typedef enum {
	K_TAB		= 9,
	K_ENTER		= 13,
	K_ESCAPE	= 27,
	K_SPACE		= 32,

	// normal keys should be passed as lowercased ascii
	K_BACKSPACE	= 127,


	K_SCRLCK,
	K_CAPSLOCK,
	K_POWER,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_LALT,
	K_LCTRL,
	K_LSHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_F13,
	K_F14,
	K_F15,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	K_MOUSE1,	//aka left
	K_MOUSE2,	//aka right
	K_MOUSE3,	//aka middle
	K_MOUSE4,	//aka back
	K_MOUSE5,	//aka forward

	K_MWHEELDOWN,
	K_MWHEELUP,

	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,

	/* if you change the above order, you _will_ break Trinity!
	 * only make modifcations below, unless you want to start 
	 * remapping keys for that too */

	/* Section dedicated to SDL controller definitions */
	K_GP_DIAMOND_DOWN,
	K_GP_DIAMOND_RIGHT,
	K_GP_DIAMOND_LEFT,
	K_GP_DIAMOND_UP,
//for their behaviours in the menus... we may want to put a conditional in here for japanese-style right-for-confirm, but for now I'm lazy and am sticking with western/xbox/steam mappings.
#define K_GP_DIAMOND_CONFIRM	K_GP_DIAMOND_DOWN	//roughly equivelent to k_return for menu behaviours
#define K_GP_DIAMOND_CANCEL		K_GP_DIAMOND_RIGHT	//roughly like escape, at least in menus
#define K_GP_DIAMOND_ALTCONFIRM	K_GP_DIAMOND_UP		//for more negative confirmations.
	K_GP_VIEW, //aka back (near left stick)
	K_GP_GUIDE,
	K_GP_MENU,	//aka options/start (near right stick)
	K_GP_LEFT_STICK,
	K_GP_RIGHT_STICK,
	K_GP_LEFT_SHOULDER,
	K_GP_RIGHT_SHOULDER,
	K_GP_DPAD_UP,
	K_GP_DPAD_DOWN,
	K_GP_DPAD_LEFT,
	K_GP_DPAD_RIGHT,
	K_GP_MISC1,    /* share/mic-mute button */
	K_GP_PADDLE1,
	K_GP_PADDLE2,
	K_GP_PADDLE3,
	K_GP_PADDLE4, 
	K_GP_TOUCHPAD, /* when pressed */

	/* emulated, we'll trigger these 'buttons' when we reach 50% pressed */
	K_GP_LEFT_TRIGGER, 
	K_GP_RIGHT_TRIGGER,
	K_GP_LEFT_THUMB_UP,
	K_GP_LEFT_THUMB_DOWN,
	K_GP_LEFT_THUMB_LEFT,
	K_GP_LEFT_THUMB_RIGHT,
	K_GP_RIGHT_THUMB_UP,
	K_GP_RIGHT_THUMB_DOWN,
	K_GP_RIGHT_THUMB_LEFT,
	K_GP_RIGHT_THUMB_RIGHT,
	K_GP_UNKNOWN,

	/* extra dinput mouse buttons */
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,
	K_MOUSE9,
	K_MOUSE10,

	/*FIXME*/
#define K_MWHEELLEFT	K_MOUSE9
#define K_MWHEELRIGHT	K_MOUSE10

	/* spare joystick button presses */
	K_JOY_UP,
	K_JOY_DOWN,
	K_JOY_LEFT,
	K_JOY_RIGHT,

	/* extra keys */
	K_LWIN,
	K_RWIN,
	K_APP,
	K_SEARCH,
	K_VOLUP,
	K_VOLDOWN,
	K_RALT,
	K_RCTRL,
	K_RSHIFT,
	K_PRINTSCREEN,

	/* multimedia keyboard */
	K_MM_BROWSER_BACK,
	K_MM_BROWSER_FAVORITES,
	K_MM_BROWSER_FORWARD,
	K_MM_BROWSER_HOME,
	K_MM_BROWSER_REFRESH,
	K_MM_BROWSER_STOP,
	K_MM_VOLUME_MUTE,
	K_MM_TRACK_NEXT,
	K_MM_TRACK_PREV,
	K_MM_TRACK_STOP,
	K_MM_TRACK_PLAYPAUSE,

	//touchscreen stuff.
	K_TOUCH,		//initial touch
	//will be paired with one of...
	K_TOUCHSLIDE,	//existing touch became a slide
	K_TOUCHTAP,		//touched briefly without sliding (treat like a left-click, though fired on final release)
	K_TOUCHLONG,	//touch lasted a while and without moving (treat like a right-click)

	K_MAX,

	//360 buttons
	K_GP_A = K_GP_DIAMOND_DOWN,
	K_GP_B = K_GP_DIAMOND_RIGHT,
	K_GP_X = K_GP_DIAMOND_LEFT,
	K_GP_Y = K_GP_DIAMOND_UP,
	K_GP_BACK = K_GP_VIEW,
	K_GP_START = K_GP_MENU,

	//ps buttons
	K_GP_PS_CROSS		= K_GP_DIAMOND_DOWN,
	K_GP_PS_CIRCLE		= K_GP_DIAMOND_RIGHT,
	K_GP_PS_SQUARE		= K_GP_DIAMOND_LEFT,
	K_GP_PS_TRIANGLE	= K_GP_DIAMOND_UP,
} keynum_t;

#define KEY_MODIFIER_SHIFT		(1<<0)
#define KEY_MODIFIER_ALT		(1<<1)
#define KEY_MODIFIER_CTRL		(1<<2)
//#define KEY_MODIFIER_META		(1<<?) do we want?
#define KEY_MODIFIER_ALTBINDMAP	(1<<3)
#define	KEY_MODIFIERSTATES		(1<<4)

//legacy aliases, lest we ever forget!
#define K_SHIFT K_LSHIFT
#define K_CTRL K_LCTRL
#define K_ALT K_LALT
#define K_WIN K_LWIN

#ifdef __QUAKEDEF_H__
typedef enum	//highest has priority
{
	kdm_game		= 1u<<0,	//should always be set
	kdm_centerprint	= 1u<<1,	//enabled when there's a centerprint menu with clickable things.
	kdm_message		= 1u<<2,
	kdm_menu		= 1u<<3,	//layered menus (engine menus, qc menus, or plugins/etc)
	kdm_console		= 1u<<4,
	kdm_cwindows	= 1u<<5,
	kdm_prompt		= 1u<<6,	//highest priority - popups that require user interaction (eg: confirmation from untrusted console commands)
} keydestmask_t;

//unsigned int Key_Dest_Get(void);	//returns highest priority destination
#define Key_Dest_Add(kdm) (key_dest_mask |= (kdm))
#define Key_Dest_Remove(kdm) (key_dest_mask &= ~(kdm))
#define Key_Dest_Has(kdm) (key_dest_mask & (kdm))
#define Key_Dest_Has_Higher(kdm) (key_dest_mask & (~0&~((kdm)|((kdm)-1))))	//must be a single bit
#define Key_Dest_Toggle(kdm) do {if (key_dest_mask & kdm) Key_Dest_Remove(kdm); else Key_Dest_Add(kdm);}while(0)

extern unsigned int key_dest_absolutemouse;	//if the active key dest bit is set, the mouse is absolute.
extern unsigned int key_dest_mask;
extern char *keybindings[K_MAX][KEY_MODIFIERSTATES];

extern unsigned int keydown[K_MAX];	//bitmask of devices.

enum
{
	kc_game,		//csprogs.dat
	kc_menuqc,		//
	kc_nativemenu,	//
	kc_plugin,		//for plugins
	kc_console,		//generic engine-defined cursor
	kc_max
};
extern struct key_cursor_s
{
	char name[MAX_QPATH];
	float hotspot[2];
	float scale;
	qboolean dirty;
	void *handle;
} key_customcursor[kc_max];

extern unsigned char *chat_buffer;
extern	int chat_bufferpos;
extern	qboolean	chat_team;

void Key_Event (unsigned int devid, int key, unsigned int unicode, qboolean down);
void Key_Init (void);
void IN_WriteButtons(vfsfile_t *f, qboolean all);
void Key_WriteBindings (struct vfsfile_s *f);
void Key_SetBinding (int keynum, int modifier, const char *binding, int cmdlevel);
void Key_ClearStates (void);
qboolean Key_Centerprint(int key, int unicode, unsigned int devid);
void Key_Unbindall_f (void);	//aka: Key_Shutdown
void Key_ConsoleReplace(const char *instext);
void Key_DefaultLinkClicked(console_t *con, char *text, char *info);
void Key_HandleConsoleLink(console_t *con, char *buffer);

qboolean Key_Console (console_t *con, int key, unsigned int unicode);
void Key_ConsoleRelease(console_t *con, int key, unsigned int unicode);

struct console_s;
qboolean Key_GetConsoleSelectionBox(struct console_s *con, int *sx, int *sy, int *ex, int *ey);
qboolean Key_MouseShouldBeFree(void);

const char *Key_Demoji(char *buffer, size_t buffersize, const char *in);	//strips out :smile: stuff.
void Key_EmojiCompletion_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);
#endif
#endif

