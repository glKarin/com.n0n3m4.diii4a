// Copyright (C) 2007 Id Software, Inc.
//

#ifndef _KEYNUM_H_
#define _KEYNUM_H_

// these are the key numbers that are used by the key system
// normal keys should be passed as lowercased ascii
// Some high ascii (> 127) characters that are mapped directly to keys on
// western european keyboards are inserted in this table so that those keys
// are bindable (otherwise they get bound as one of the special keys in this
// table)
#if !defined(_SPLASHDAMAGE)
typedef enum keyNum_e {
    K_INVALID = 0x00,

    K_BACKSPACE = 0x08,
    K_TAB = 0x09,
    K_ENTER = 0x0D,
    K_ESCAPE = 0x1B,
    K_SPACE = 0x20,

    K_EXCLAMATION = 0x21,
    K_HASH = 0x23,
    K_DOLLAR,
    K_AMPERSAND = 0x26,
    K_APOSTROPHE,
    K_LEFTPARENTHESIS,
    K_RIGHTPARENTHESIS,
    K_ASTERISK,
    K_PLUS,
    K_COMMA,
    K_MINUS,
    K_PERIOD,
    K_SLASH,

    K_0 = 0x30,
    K_1,
    K_2,
    K_3,
    K_4,
    K_5,
    K_6,
    K_7,
    K_8,
    K_9,

    K_SEMICOLON = 0x3B,
    //K_LESS = 0x3C,
    K_EQUALS = 0x3D,

    K_LEFTBRACKET = 0x5B,
    K_BACKSLASH,
    K_RIGHTBRACKET,

    K_BACKQUOTE = 0x60,

    K_A = 0x61,
    K_B,
    K_C,
    K_D,
    K_E,
    K_F,
    K_G,
    K_H,
    K_I,
    K_J,
    K_K,
    K_L,
    K_M,
    K_N,
    K_O,
    K_P,
    K_Q,
    K_R,
    K_S,
    K_T,
    K_U,
    K_V,
    K_W,
    K_X,
    K_Y,
    K_Z,

    // end of ASCII mapped range

    K_COMMAND = 0x80,
    K_CAPSLOCK,
    K_SCROLL,
    K_PAUSE,

    K_UPARROW = 0x85,
    K_DOWNARROW,
    K_LEFTARROW,
    K_RIGHTARROW,

    // The 3 windows keys
    K_LWIN = 0x89,
    K_RWIN,
    K_MENU,

    K_ALT = 0x8C,
    K_CTRL,
    K_SHIFT,
    K_INS,
    K_DEL,
    K_PGDN,
    K_PGUP,
    K_HOME,
    K_END,

    K_F1 = 0x95,
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
    K_F16 =0xA4,

    K_KP_HOME = 0xA5,
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

    K_GRAVE_A = 0xE0,	// lowercase a with grave accent
    K_OEM_102,			// "<>" or "\|" on RT 102-key kbd.

    K_AUX1 = 0xE6,
    K_CEDILLA_C = 0xE7,	// lowercase c with Cedilla
    K_GRAVE_E = 0xE8,	// lowercase e with grave accent
    K_AUX2,
    K_AUX3,
    K_AUX4,
    K_GRAVE_I = 0xEC,	// lowercase i with grave accent
    K_AUX5,
    K_AUX6,
    K_AUX7,
    K_AUX8,
    K_TILDE_N = 0xF1,	// lowercase n with tilde
    K_GRAVE_O = 0xF2,	// lowercase o with grave accent
    K_AUX9,
    K_AUX10,
    K_GRAVE_U = 0xF9,	// lowercase u with grave accent

    K_PRINT_SCR	= 0xFA,	// SysRq / PrintScr
    K_RIGHT_ALT = 0xFB,	// used by some languages as "Alt-Gr"
    K_RIGHT_SHIFT = 0xFC,
    K_RIGHT_CTRL = 0xFD,
    K_CONSOLE = 0xFE,	// maps to scan code 0x29

    K_NUM_KEYS = 0xFF
} keyNum_t;
#else
typedef enum {
    K_INVALID = 0x00,

    K_BACKSPACE = 0x08,
    K_TAB = 0x09,
    K_ENTER = 0x0D,
    K_ESCAPE = 0x1B,
    K_SPACE = 0x20,

	//karin: add < 127
	K_EXCLAMATION = 0x21,
	K_HASH = 0x23,
	K_DOLLAR,
	K_AMPERSAND = 0x26,
	K_APOSTROPHE,
	K_LEFTPARENTHESIS,
	K_RIGHTPARENTHESIS,
	K_ASTERISK,
	K_PLUS,
	K_COMMA,
	K_MINUS,
	K_PERIOD,
	K_SLASH,

	K_0 = 0x30,
	K_1,
	K_2,
	K_3,
	K_4,
	K_5,
	K_6,
	K_7,
	K_8,
	K_9,

	K_SEMICOLON = 0x3B,
	//K_LESS = 0x3C,
	K_EQUALS = 0x3D,

	K_LEFTBRACKET = 0x5B,
	K_BACKSLASH,
	K_RIGHTBRACKET,

	K_BACKQUOTE = 0x60,

	K_A = 0x61,
	K_B,
	K_C,
	K_D,
	K_E,
	K_F,
	K_G,
	K_H,
	K_I,
	K_J,
	K_K,
	K_L,
	K_M,
	K_N,
	K_O,
	K_P,
	K_Q,
	K_R,
	K_S,
	K_T,
	K_U,
	K_V,
	K_W,
	K_X,
	K_Y,
	K_Z,

    // end of ASCII mapped range

    K_COMMAND = 0x80,
    K_CAPSLOCK,
    K_SCROLL,
    K_PAUSE,

	K_POWER,

    K_UPARROW = 0x85,
    K_DOWNARROW,
    K_LEFTARROW,
    K_RIGHTARROW,

    // The 3 windows keys
    K_LWIN = 0x89,
    K_RWIN,
    K_MENU,

    K_ALT = 0x8C,
    K_CTRL,
    K_SHIFT,
    K_INS,
    K_DEL,
    K_PGDN,
    K_PGUP,
    K_HOME,
    K_END,

    K_F1 = 0x95,
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
    K_F16 =0xA4,

    K_KP_HOME = 0xA5,
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

	K_MASCULINE_ORDINATOR = 186,
	// K_MOUSE enums must be contiguous (no char codes in the middle)
	K_MOUSE1 = 187,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,

	K_MWHEELDOWN = 195,
	K_MWHEELUP,

	K_JOY1 = 197,
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
	K_GRAVE_A = 0xE0,	// lowercase a with grave accent
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,
    K_OEM_102,			// "<>" or "\|" on RT 102-key kbd.

    K_AUX1 = 0xE6,
    K_CEDILLA_C = 0xE7,	// lowercase c with Cedilla
    K_GRAVE_E = 0xE8,	// lowercase e with grave accent
    K_AUX2,
    K_AUX3,
    K_AUX4,
    K_GRAVE_I = 0xEC,	// lowercase i with grave accent
    K_AUX5,
    K_AUX6,
    K_AUX7,
    K_AUX8,
    K_TILDE_N = 0xF1,	// lowercase n with tilde
    K_GRAVE_O = 0xF2,	// lowercase o with grave accent
    K_AUX9,
    K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
    K_GRAVE_U = 0xF9,	// lowercase u with grave accent
	K_AUX15,
	K_AUX16,

    K_PRINT_SCR	= 0xFA,	// SysRq / PrintScr
    K_RIGHT_ALT = 0xFB,	// used by some languages as "Alt-Gr"
    K_RIGHT_SHIFT = 0xFC,
    K_RIGHT_CTRL = 0xFD,
    K_CONSOLE = 0xFE,	// maps to scan code 0x29

	K_LAST_KEY  = 254	// this better be < 256!
} keyNum_t;
#define K_RIGHT_SHIFT K_SHIFT
#define K_RIGHT_CTRL K_CTRL
typedef keyNum_t keyNum_e; // original in sys/keynum.h
#endif

#endif
