/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

/* rcg10072001 Moved stuff into this file. */

#define __STDC_LIMIT_MACROS 1

#include <Engine/Base/Timer.h>
#include <Engine/Base/Input.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/KeyNames.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Synchronization.h>
#include <Engine/Base/SDL/SDLEvents.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/ErrorReporting.h>

#include "SDL.h"

static const SDL_Keycode SDL_default_keymap[SDL_NUM_SCANCODES] = {
    /* 0 */ 0,
    /* 1 */ 0,
    /* 2 */ 0,
    /* 3 */ 0,
    /* 4 */ 'a',
    /* 5 */ 'b',
    /* 6 */ 'c',
    /* 7 */ 'd',
    /* 8 */ 'e',
    /* 9 */ 'f',
    /* 10 */ 'g',
    /* 11 */ 'h',
    /* 12 */ 'i',
    /* 13 */ 'j',
    /* 14 */ 'k',
    /* 15 */ 'l',
    /* 16 */ 'm',
    /* 17 */ 'n',
    /* 18 */ 'o',
    /* 19 */ 'p',
    /* 20 */ 'q',
    /* 21 */ 'r',
    /* 22 */ 's',
    /* 23 */ 't',
    /* 24 */ 'u',
    /* 25 */ 'v',
    /* 26 */ 'w',
    /* 27 */ 'x',
    /* 28 */ 'y',
    /* 29 */ 'z',
    /* 30 */ '1',
    /* 31 */ '2',
    /* 32 */ '3',
    /* 33 */ '4',
    /* 34 */ '5',
    /* 35 */ '6',
    /* 36 */ '7',
    /* 37 */ '8',
    /* 38 */ '9',
    /* 39 */ '0',
    /* 40 */ SDLK_RETURN,
    /* 41 */ SDLK_ESCAPE,
    /* 42 */ SDLK_BACKSPACE,
    /* 43 */ SDLK_TAB,
    /* 44 */ SDLK_SPACE,
    /* 45 */ '-',
    /* 46 */ '=',
    /* 47 */ '[',
    /* 48 */ ']',
    /* 49 */ '\\',
    /* 50 */ '#',
    /* 51 */ ';',
    /* 52 */ '\'',
    /* 53 */ '`',
    /* 54 */ ',',
    /* 55 */ '.',
    /* 56 */ '/',
    /* 57 */ SDLK_CAPSLOCK,
    /* 58 */ SDLK_F1,
    /* 59 */ SDLK_F2,
    /* 60 */ SDLK_F3,
    /* 61 */ SDLK_F4,
    /* 62 */ SDLK_F5,
    /* 63 */ SDLK_F6,
    /* 64 */ SDLK_F7,
    /* 65 */ SDLK_F8,
    /* 66 */ SDLK_F9,
    /* 67 */ SDLK_F10,
    /* 68 */ SDLK_F11,
    /* 69 */ SDLK_F12,
    /* 70 */ SDLK_PRINTSCREEN,
    /* 71 */ SDLK_SCROLLLOCK,
    /* 72 */ SDLK_PAUSE,
    /* 73 */ SDLK_INSERT,
    /* 74 */ SDLK_HOME,
    /* 75 */ SDLK_PAGEUP,
    /* 76 */ SDLK_DELETE,
    /* 77 */ SDLK_END,
    /* 78 */ SDLK_PAGEDOWN,
    /* 79 */ SDLK_RIGHT,
    /* 80 */ SDLK_LEFT,
    /* 81 */ SDLK_DOWN,
    /* 82 */ SDLK_UP,
    /* 83 */ SDLK_NUMLOCKCLEAR,
    /* 84 */ SDLK_KP_DIVIDE,
    /* 85 */ SDLK_KP_MULTIPLY,
    /* 86 */ SDLK_KP_MINUS,
    /* 87 */ SDLK_KP_PLUS,
    /* 88 */ SDLK_KP_ENTER,
    /* 89 */ SDLK_KP_1,
    /* 90 */ SDLK_KP_2,
    /* 91 */ SDLK_KP_3,
    /* 92 */ SDLK_KP_4,
    /* 93 */ SDLK_KP_5,
    /* 94 */ SDLK_KP_6,
    /* 95 */ SDLK_KP_7,
    /* 96 */ SDLK_KP_8,
    /* 97 */ SDLK_KP_9,
    /* 98 */ SDLK_KP_0,
    /* 99 */ SDLK_KP_PERIOD,
    /* 100 */ 0,
    /* 101 */ SDLK_APPLICATION,
    /* 102 */ SDLK_POWER,
    /* 103 */ SDLK_KP_EQUALS,
    /* 104 */ SDLK_F13,
    /* 105 */ SDLK_F14,
    /* 106 */ SDLK_F15,
    /* 107 */ SDLK_F16,
    /* 108 */ SDLK_F17,
    /* 109 */ SDLK_F18,
    /* 110 */ SDLK_F19,
    /* 111 */ SDLK_F20,
    /* 112 */ SDLK_F21,
    /* 113 */ SDLK_F22,
    /* 114 */ SDLK_F23,
    /* 115 */ SDLK_F24,
    /* 116 */ SDLK_EXECUTE,
    /* 117 */ SDLK_HELP,
    /* 118 */ SDLK_MENU,
    /* 119 */ SDLK_SELECT,
    /* 120 */ SDLK_STOP,
    /* 121 */ SDLK_AGAIN,
    /* 122 */ SDLK_UNDO,
    /* 123 */ SDLK_CUT,
    /* 124 */ SDLK_COPY,
    /* 125 */ SDLK_PASTE,
    /* 126 */ SDLK_FIND,
    /* 127 */ SDLK_MUTE,
    /* 128 */ SDLK_VOLUMEUP,
    /* 129 */ SDLK_VOLUMEDOWN,
    /* 130 */ 0,
    /* 131 */ 0,
    /* 132 */ 0,
    /* 133 */ SDLK_KP_COMMA,
    /* 134 */ SDLK_KP_EQUALSAS400,
    /* 135 */ 0,
    /* 136 */ 0,
    /* 137 */ 0,
    /* 138 */ 0,
    /* 139 */ 0,
    /* 140 */ 0,
    /* 141 */ 0,
    /* 142 */ 0,
    /* 143 */ 0,
    /* 144 */ 0,
    /* 145 */ 0,
    /* 146 */ 0,
    /* 147 */ 0,
    /* 148 */ 0,
    /* 149 */ 0,
    /* 150 */ 0,
    /* 151 */ 0,
    /* 152 */ 0,
    /* 153 */ SDLK_ALTERASE,
    /* 154 */ SDLK_SYSREQ,
    /* 155 */ SDLK_CANCEL,
    /* 156 */ SDLK_CLEAR,
    /* 157 */ SDLK_PRIOR,
    /* 158 */ SDLK_RETURN2,
    /* 159 */ SDLK_SEPARATOR,
    /* 160 */ SDLK_OUT,
    /* 161 */ SDLK_OPER,
    /* 162 */ SDLK_CLEARAGAIN,
    /* 163 */ SDLK_CRSEL,
    /* 164 */ SDLK_EXSEL,
    /* 165 */ 0,
    /* 166 */ 0,
    /* 167 */ 0,
    /* 168 */ 0,
    /* 169 */ 0,
    /* 170 */ 0,
    /* 171 */ 0,
    /* 172 */ 0,
    /* 173 */ 0,
    /* 174 */ 0,
    /* 175 */ 0,
    /* 176 */ SDLK_KP_00,
    /* 177 */ SDLK_KP_000,
    /* 178 */ SDLK_THOUSANDSSEPARATOR,
    /* 179 */ SDLK_DECIMALSEPARATOR,
    /* 180 */ SDLK_CURRENCYUNIT,
    /* 181 */ SDLK_CURRENCYSUBUNIT,
    /* 182 */ SDLK_KP_LEFTPAREN,
    /* 183 */ SDLK_KP_RIGHTPAREN,
    /* 184 */ SDLK_KP_LEFTBRACE,
    /* 185 */ SDLK_KP_RIGHTBRACE,
    /* 186 */ SDLK_KP_TAB,
    /* 187 */ SDLK_KP_BACKSPACE,
    /* 188 */ SDLK_KP_A,
    /* 189 */ SDLK_KP_B,
    /* 190 */ SDLK_KP_C,
    /* 191 */ SDLK_KP_D,
    /* 192 */ SDLK_KP_E,
    /* 193 */ SDLK_KP_F,
    /* 194 */ SDLK_KP_XOR,
    /* 195 */ SDLK_KP_POWER,
    /* 196 */ SDLK_KP_PERCENT,
    /* 197 */ SDLK_KP_LESS,
    /* 198 */ SDLK_KP_GREATER,
    /* 199 */ SDLK_KP_AMPERSAND,
    /* 200 */ SDLK_KP_DBLAMPERSAND,
    /* 201 */ SDLK_KP_VERTICALBAR,
    /* 202 */ SDLK_KP_DBLVERTICALBAR,
    /* 203 */ SDLK_KP_COLON,
    /* 204 */ SDLK_KP_HASH,
    /* 205 */ SDLK_KP_SPACE,
    /* 206 */ SDLK_KP_AT,
    /* 207 */ SDLK_KP_EXCLAM,
    /* 208 */ SDLK_KP_MEMSTORE,
    /* 209 */ SDLK_KP_MEMRECALL,
    /* 210 */ SDLK_KP_MEMCLEAR,
    /* 211 */ SDLK_KP_MEMADD,
    /* 212 */ SDLK_KP_MEMSUBTRACT,
    /* 213 */ SDLK_KP_MEMMULTIPLY,
    /* 214 */ SDLK_KP_MEMDIVIDE,
    /* 215 */ SDLK_KP_PLUSMINUS,
    /* 216 */ SDLK_KP_CLEAR,
    /* 217 */ SDLK_KP_CLEARENTRY,
    /* 218 */ SDLK_KP_BINARY,
    /* 219 */ SDLK_KP_OCTAL,
    /* 220 */ SDLK_KP_DECIMAL,
    /* 221 */ SDLK_KP_HEXADECIMAL,
    /* 222 */ 0,
    /* 223 */ 0,
    /* 224 */ SDLK_LCTRL,
    /* 225 */ SDLK_LSHIFT,
    /* 226 */ SDLK_LALT,
    /* 227 */ SDLK_LGUI,
    /* 228 */ SDLK_RCTRL,
    /* 229 */ SDLK_RSHIFT,
    /* 230 */ SDLK_RALT,
    /* 231 */ SDLK_RGUI,
    /* 232 */ 0,
    /* 233 */ 0,
    /* 234 */ 0,
    /* 235 */ 0,
    /* 236 */ 0,
    /* 237 */ 0,
    /* 238 */ 0,
    /* 239 */ 0,
    /* 240 */ 0,
    /* 241 */ 0,
    /* 242 */ 0,
    /* 243 */ 0,
    /* 244 */ 0,
    /* 245 */ 0,
    /* 246 */ 0,
    /* 247 */ 0,
    /* 248 */ 0,
    /* 249 */ 0,
    /* 250 */ 0,
    /* 251 */ 0,
    /* 252 */ 0,
    /* 253 */ 0,
    /* 254 */ 0,
    /* 255 */ 0,
    /* 256 */ 0,
    /* 257 */ SDLK_MODE,
    /* 258 */ SDLK_AUDIONEXT,
    /* 259 */ SDLK_AUDIOPREV,
    /* 260 */ SDLK_AUDIOSTOP,
    /* 261 */ SDLK_AUDIOPLAY,
    /* 262 */ SDLK_AUDIOMUTE,
    /* 263 */ SDLK_MEDIASELECT,
    /* 264 */ SDLK_WWW,
    /* 265 */ SDLK_MAIL,
    /* 266 */ SDLK_CALCULATOR,
    /* 267 */ SDLK_COMPUTER,
    /* 268 */ SDLK_AC_SEARCH,
    /* 269 */ SDLK_AC_HOME,
    /* 270 */ SDLK_AC_BACK,
    /* 271 */ SDLK_AC_FORWARD,
    /* 272 */ SDLK_AC_STOP,
    /* 273 */ SDLK_AC_REFRESH,
    /* 274 */ SDLK_AC_BOOKMARKS,
    /* 275 */ SDLK_BRIGHTNESSDOWN,
    /* 276 */ SDLK_BRIGHTNESSUP,
    /* 277 */ SDLK_DISPLAYSWITCH,
    /* 278 */ SDLK_KBDILLUMTOGGLE,
    /* 279 */ SDLK_KBDILLUMDOWN,
    /* 280 */ SDLK_KBDILLUMUP,
    /* 281 */ SDLK_EJECT,
    /* 282 */ SDLK_SLEEP,
#if 0
    /* 283 */ SDLK_APP1,
    /* 284 */ SDLK_APP2,
    /* 285 */ SDLK_AUDIOREWIND,
    /* 286 */ SDLK_AUDIOFASTFORWARD,
    /* 287 */ SDLK_SOFTLEFT,
    /* 288 */ SDLK_SOFTRIGHT,
    /* 289 */ SDLK_CALL,
    /* 290 */ SDLK_ENDCALL,
#endif
};

#include <unordered_map>
static std::unordered_map<SDL_Keycode, SDL_Scancode> keycode_to_scancode;
SDL_Scancode Q3E_GetScancodeFromKeycode(SDL_Keycode key)
{
	auto itor = keycode_to_scancode.find(key);
	return itor != keycode_to_scancode.end() ? itor->second : SDL_SCANCODE_UNKNOWN;
}

SDL_Scancode Q3E_GetScancodeFromKey(SDL_Keycode key)
{
    int scancode;

    for (scancode = SDL_SCANCODE_UNKNOWN; scancode < SDL_NUM_SCANCODES;
         ++scancode) {
        if (SDL_default_keymap[scancode] == key) {
            return (SDL_Scancode)scancode;
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}
extern const uint8_t * Q3E_GetKeyboardState(int *numkeys);
extern void Q3E_GetRelativeMouseState(int *x, int *y);


extern INDEX inp_iKeyboardReadingMethod;
extern FLOAT inp_fMouseSensitivity;
extern INDEX inp_bAllowMouseAcceleration;
extern INDEX inp_bMousePrecision;
extern FLOAT inp_fMousePrecisionFactor;
extern FLOAT inp_fMousePrecisionThreshold;
extern FLOAT inp_fMousePrecisionTimeout;
extern FLOAT inp_bInvertMouse;
extern INDEX inp_bFilterMouse;
extern INDEX inp_bAllowPrescan;

extern INDEX inp_i2ndMousePort;
extern FLOAT inp_f2ndMouseSensitivity;
extern INDEX inp_b2ndMousePrecision;
extern FLOAT inp_f2ndMousePrecisionThreshold;
extern FLOAT inp_f2ndMousePrecisionTimeout;
extern FLOAT inp_f2ndMousePrecisionFactor;
extern INDEX inp_bFilter2ndMouse;
extern INDEX inp_bInvert2ndMouse;

static BOOL inp_bSDLPermitCtrlG = TRUE;
static BOOL inp_bSDLGrabInput = TRUE;
static Sint16 mouse_relative_x = 0;
static Sint16 mouse_relative_y = 0;

INDEX inp_iMButton4Dn = 0x20040;
INDEX inp_iMButton4Up = 0x20000;
INDEX inp_iMButton5Dn = 0x10020;
INDEX inp_iMButton5Up = 0x10000;
INDEX inp_bMsgDebugger = FALSE;
INDEX inp_bForceJoystickPolling = 0;
INDEX inp_bAutoDisableJoysticks = 0;

static CTCriticalSection sl_csInput;

extern INDEX inp_ctJoysticksAllowed;

extern void Q3E_SetRelativeMouseMode(BOOL on);
extern void Q3E_GetWindowSize(void *, int *winw, int *winh);
extern void Q3E_GetMouseState(int *x, int *y);
extern BOOL Q3E_PollEvent(SDL_Event *ev);
/*

NOTE: Two different types of key codes are used here:
  1) kid - engine internal type - defined in KeyNames.h
  2) virtkey - virtual key codes used by SDL.

*/

// name that is not translated (international)
#define INTNAME(str) str, ""
// name that is translated
#define TRANAME(str) str, "ETRS" str

// basic key conversion table
struct KeyConversion {
  INDEX kc_iKID;
  SDL_Keycode kc_iVirtKey;
  const char *kc_strName;
  const char *kc_strNameTrans;
};

static const KeyConversion _akcKeys[] = {
// reserved for 'no-key-pressed'
  { KID_NONE, -1, TRANAME("None")},

// numbers row
  { KID_1               , SDLK_1,      INTNAME("1")},
  { KID_2               , SDLK_2,      INTNAME("2")},
  { KID_3               , SDLK_3,      INTNAME("3")},
  { KID_4               , SDLK_4,      INTNAME("4")},
  { KID_5               , SDLK_5,      INTNAME("5")},
  { KID_6               , SDLK_6,      INTNAME("6")},
  { KID_7               , SDLK_7,      INTNAME("7")},
  { KID_8               , SDLK_8,      INTNAME("8")},
  { KID_9               , SDLK_9,      INTNAME("9")},
  { KID_0               , SDLK_0,      INTNAME("0")},
  { KID_MINUS           , SDLK_MINUS,  INTNAME("-")},
  { KID_EQUALS          , SDLK_EQUALS, INTNAME("=")},
                            
// 1st alpha row            
  { KID_Q               , SDLK_q, INTNAME("Q")},
  { KID_W               , SDLK_w, INTNAME("W")},
  { KID_E               , SDLK_e, INTNAME("E")},
  { KID_R               , SDLK_r, INTNAME("R")},
  { KID_T               , SDLK_t, INTNAME("T")},
  { KID_Y               , SDLK_y, INTNAME("Y")},
  { KID_U               , SDLK_u, INTNAME("U")},
  { KID_I               , SDLK_i, INTNAME("I")},
  { KID_O               , SDLK_o, INTNAME("O")},
  { KID_P               , SDLK_p, INTNAME("P")},
  { KID_LBRACKET        , SDLK_RIGHTPAREN, INTNAME("[")},
  { KID_RBRACKET        , SDLK_RIGHTPAREN, INTNAME("]")},
  { KID_BACKSLASH       , SDLK_BACKSLASH,  INTNAME("\\")},

// 2nd alpha row            
  { KID_A               , SDLK_a,   INTNAME("A")},
  { KID_S               , SDLK_s,   INTNAME("S")},
  { KID_D               , SDLK_d,   INTNAME("D")},
  { KID_F               , SDLK_f,   INTNAME("F")},
  { KID_G               , SDLK_g,   INTNAME("G")},
  { KID_H               , SDLK_h,   INTNAME("H")},
  { KID_J               , SDLK_j,   INTNAME("J")},
  { KID_K               , SDLK_k,   INTNAME("K")},
  { KID_L               , SDLK_l,   INTNAME("L")},
  { KID_SEMICOLON       , SDLK_SEMICOLON,   INTNAME(";")},
  { KID_APOSTROPHE      , SDLK_QUOTE,       INTNAME("'")},

// 3rd alpha row
  { KID_Z               , SDLK_z,   INTNAME("Z")},
  { KID_X               , SDLK_x,   INTNAME("X")},
  { KID_C               , SDLK_c,   INTNAME("C")},
  { KID_V               , SDLK_v,   INTNAME("V")},
  { KID_B               , SDLK_b,   INTNAME("B")},
  { KID_N               , SDLK_n,   INTNAME("N")},
  { KID_M               , SDLK_m,   INTNAME("M")},
  { KID_COMMA           , SDLK_COMMA,   INTNAME(",")},
  { KID_PERIOD          , SDLK_PERIOD,   INTNAME(".")},
  { KID_SLASH           , SDLK_SLASH,   INTNAME("/")},

// row with F-keys                     
  { KID_F1              , SDLK_F1,   INTNAME("F1")},
  { KID_F2              , SDLK_F2,   INTNAME("F2")},
  { KID_F3              , SDLK_F3,   INTNAME("F3")},
  { KID_F4              , SDLK_F4,   INTNAME("F4")},
  { KID_F5              , SDLK_F5,   INTNAME("F5")},
  { KID_F6              , SDLK_F6,   INTNAME("F6")},
  { KID_F7              , SDLK_F7,   INTNAME("F7")},
  { KID_F8              , SDLK_F8,   INTNAME("F8")},
  { KID_F9              , SDLK_F9,   INTNAME("F9")},
  { KID_F10             , SDLK_F10,  INTNAME("F10")},
  { KID_F11             , SDLK_F11,  INTNAME("F11")},
  { KID_F12             , SDLK_F12,  INTNAME("F12")},
                            
// extra keys               
  { KID_ESCAPE          , SDLK_ESCAPE,   TRANAME("Escape")},
  { KID_TILDE           , -1,            TRANAME("Tilde")},
  { KID_BACKSPACE       , SDLK_BACKSPACE,     TRANAME("Backspace")},
  { KID_TAB             , SDLK_TAB,      TRANAME("Tab")},
  { KID_CAPSLOCK        , SDLK_CAPSLOCK,  TRANAME("Caps Lock")},
  { KID_ENTER           , SDLK_RETURN,   TRANAME("Enter")},
  { KID_SPACE           , SDLK_SPACE,    TRANAME("Space")},
                                            
// modifier keys                            
  { KID_LSHIFT          , SDLK_LSHIFT  , TRANAME("Left Shift")},
  { KID_RSHIFT          , SDLK_RSHIFT  , TRANAME("Right Shift")},
  { KID_LCONTROL        , SDLK_LCTRL, TRANAME("Left Control")},
  { KID_RCONTROL        , SDLK_RCTRL, TRANAME("Right Control")},
  { KID_LALT            , SDLK_LALT   , TRANAME("Left Alt")},
  { KID_RALT            , SDLK_RALT   , TRANAME("Right Alt")},
                            
// navigation keys          
  { KID_ARROWUP         , SDLK_UP,       TRANAME("Arrow Up")},
  { KID_ARROWDOWN       , SDLK_DOWN,     TRANAME("Arrow Down")},
  { KID_ARROWLEFT       , SDLK_LEFT,     TRANAME("Arrow Left")},
  { KID_ARROWRIGHT      , SDLK_RIGHT,    TRANAME("Arrow Right")},
  { KID_INSERT          , SDLK_INSERT,   TRANAME("Insert")},
  { KID_DELETE          , SDLK_DELETE,   TRANAME("Delete")},
  { KID_HOME            , SDLK_HOME,     TRANAME("Home")},
  { KID_END             , SDLK_END,      TRANAME("End")},
  { KID_PAGEUP          , SDLK_PAGEUP,    TRANAME("Page Up")},
  { KID_PAGEDOWN        , SDLK_PAGEDOWN,     TRANAME("Page Down")},
  { KID_PRINTSCR        , SDLK_PRINTSCREEN, TRANAME("Print Screen")},
  { KID_SCROLLLOCK      , SDLK_SCROLLLOCK,   TRANAME("Scroll Lock")},
  { KID_PAUSE           , SDLK_PAUSE,    TRANAME("Pause")},
                            
// numpad numbers           
  { KID_NUM0            , SDLK_KP_0, INTNAME("Num 0")},
  { KID_NUM1            , SDLK_KP_1, INTNAME("Num 1")},
  { KID_NUM2            , SDLK_KP_2, INTNAME("Num 2")},
  { KID_NUM3            , SDLK_KP_3, INTNAME("Num 3")},
  { KID_NUM4            , SDLK_KP_4, INTNAME("Num 4")},
  { KID_NUM5            , SDLK_KP_5, INTNAME("Num 5")},
  { KID_NUM6            , SDLK_KP_6, INTNAME("Num 6")},
  { KID_NUM7            , SDLK_KP_7, INTNAME("Num 7")},
  { KID_NUM8            , SDLK_KP_8, INTNAME("Num 8")},
  { KID_NUM9            , SDLK_KP_9, INTNAME("Num 9")},
  { KID_NUMDECIMAL      , SDLK_KP_PERIOD, INTNAME("Num .")},
                            
// numpad gray keys         
  { KID_NUMLOCK         , SDLK_NUMLOCKCLEAR,     INTNAME("Num Lock")},
  { KID_NUMSLASH        , SDLK_KP_DIVIDE,   INTNAME("Num /")},
  { KID_NUMMULTIPLY     , SDLK_KP_MULTIPLY, INTNAME("Num *")},
  { KID_NUMMINUS        , SDLK_KP_MINUS,    INTNAME("Num -")},
  { KID_NUMPLUS         , SDLK_KP_PLUS,     INTNAME("Num +")},
  { KID_NUMENTER        , SDLK_KP_ENTER,    TRANAME("Num Enter")},

// mouse buttons
  { KID_MOUSE1          , -1, TRANAME("Mouse Button 1")},
  { KID_MOUSE2          , -1, TRANAME("Mouse Button 2")},
  { KID_MOUSE3          , -1, TRANAME("Mouse Button 3")},
  { KID_MOUSE4          , -1, TRANAME("Mouse Button 4")},
  { KID_MOUSE5          , -1, TRANAME("Mouse Button 5")},
  { KID_MOUSEWHEELUP    , -1, TRANAME("Mouse Wheel Up")},
  { KID_MOUSEWHEELDOWN  , -1, TRANAME("Mouse Wheel Down")},

// 2nd mouse buttons
  { KID_2MOUSE1         , -1, TRANAME("2nd Mouse Button 1")},
  { KID_2MOUSE2         , -1, TRANAME("2nd Mouse Button 2")},
  { KID_2MOUSE3         , -1, TRANAME("2nd Mouse Button 3")},

};


// autogenerated fast conversion tables
static INDEX _aiScancodeToKid[SDL_NUM_SCANCODES];

// make fast conversion tables from the general table
static void MakeConversionTables(void)
{
  // clear conversion tables
  for (int i = 0; i < static_cast<int>(ARRAYCOUNT(_aiScancodeToKid)); i++) {
    _aiScancodeToKid[i] = -1;
  }

  // for each Key
  for (INDEX iKey=0; iKey<static_cast<INDEX>(ARRAYCOUNT(_akcKeys)); iKey++) {
    const KeyConversion &kc = _akcKeys[iKey];

    // get codes
    const INDEX iKID  = kc.kc_iKID;
    //INDEX iScan = kc.kc_iScanCode;
    const SDL_Keycode iVirt = kc.kc_iVirtKey;

    if (iVirt>=0) {
		auto scancode = Q3E_GetScancodeFromKey(iVirt);
      _aiScancodeToKid[scancode] = iKID;

	  keycode_to_scancode.insert({iVirt, scancode});
    }
  }

  _aiScancodeToKid[SDL_SCANCODE_UNKNOWN] = -1;  // in case several items set this.
}

// variables for message interception
//static HHOOK _hGetMsgHook = NULL;
//static HHOOK _hSendMsgHook = NULL;
static int _iMouseZ = 0;
static BOOL _bWheelUp = FALSE;
static BOOL _bWheelDn = FALSE;

// emulate Win32: A single mouse wheel rotation
// is +120 (upwards) or -120 (downwards)
#define MOUSE_SCROLL_INTERVAL 120

CTCriticalSection csInput;

// which keys are pressed, as recorded by message interception (by KIDs)
static UBYTE _abKeysPressed[256];

// set a key according to a keydown/keyup message
static void SetKeyFromEvent(const SDL_Event *event, const BOOL bDown)
{
  ASSERT((event->type == SDL_KEYUP) || (event->type == SDL_KEYDOWN));

  if ( (event->key.keysym.mod & KMOD_CTRL) &&
       (event->key.keysym.sym == SDLK_g) &&
       (event->type == SDL_KEYDOWN) &&
       (inp_bSDLPermitCtrlG) )
  {
    if (inp_bSDLGrabInput)
    {
      // turn off input grab.
		Q3E_SetRelativeMouseMode(false);
      inp_bSDLGrabInput = FALSE;
    } // if
    else
    {
      // turn on input grab.
		Q3E_SetRelativeMouseMode(true);
      inp_bSDLGrabInput = TRUE;
    } // else

    mouse_relative_x = mouse_relative_y = 0;
    return;
  } // if

  #if defined(PLATFORM_PANDORA) || defined(PLATFORM_PYRA)
  if(event->key.keysym.sym==SDLK_RCTRL) {
    _abKeysPressed[KID_MOUSE1] = bDown;
    return;
  } else if(event->key.keysym.sym==SDLK_RSHIFT) {
    _abKeysPressed[KID_MOUSE2] = bDown;
    return;
  }
  #endif

    // convert virtualkey to kid
  const INDEX iKID = _aiScancodeToKid[event->key.keysym.scancode];

  if (iKID>=0 && iKID<static_cast<INDEX>(ARRAYCOUNT(_abKeysPressed))) {
    //CPrintF("%s: %d\n", _pInput->inp_strButtonNames[iKID], bDown);
    _abKeysPressed[iKID] = bDown;
  }
}


static void sdl_event_handler(const SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_MOUSEMOTION:
            mouse_relative_x += event->motion.xrel;
            mouse_relative_y += event->motion.yrel;
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (event->button.button <= 5) {
              int button = KID_MOUSE1;
              switch(event->button.button) {
                case SDL_BUTTON_RIGHT: button = KID_MOUSE2; break;
                case SDL_BUTTON_MIDDLE: button = KID_MOUSE3; break;
                case 4: button = KID_MOUSE4; break;
                case 5: button = KID_MOUSE5; break;
              }
              _abKeysPressed[button] = (event->button.state == SDL_PRESSED) ? TRUE : FALSE;
            }
            break;

        case SDL_MOUSEWHEEL:
            _iMouseZ += event->wheel.y * MOUSE_SCROLL_INTERVAL;
            break;

        case SDL_KEYDOWN:
            SetKeyFromEvent(event, TRUE);
            break;

        case SDL_KEYUP:
            SetKeyFromEvent(event, FALSE);
            break;

        default: break;
    } // switch
} // sdl_event_handler


// This keeps the input subsystem in sync with everything else, by
//  making sure all SDL events tunnel through one function.
//   DO NOT DIRECTLY MESS WITH THE SDL EVENT QUEUE THROUGH ANY OTHER FUNCTION.
//   Parameters/retval are same as SDL_PollEvent().
int SE_SDL_InputEventPoll(SDL_Event *sdlevent)
{
    ASSERT(sdlevent != NULL);
    CTSingleLock slInput(&sl_csInput, FALSE);
    if(slInput.TryToLock()) {
      const int retval = Q3E_PollEvent(sdlevent);
      if (retval)
          sdl_event_handler(sdlevent);
      return retval;
    }
    return 0;
} // SE_SDL_InputEventPoll


BOOL CInput::PlatformInit(void)
{
  _pShell->DeclareSymbol("persistent user INDEX inp_bSDLPermitCtrlG;", (void*)&inp_bSDLPermitCtrlG);
  _pShell->DeclareSymbol("persistent user INDEX inp_bSDLGrabInput;", (void*)&inp_bSDLGrabInput);
  MakeConversionTables();

  sl_csInput.cs_iIndex = 3000;

  return(TRUE);
}


// destructor
CInput::~CInput()
{
}


BOOL CInput::PlatformSetKeyNames(void)
{
  // for each Key
  for (INDEX iKey=0; iKey<static_cast<int>(ARRAYCOUNT(_akcKeys)); iKey++) {
    const KeyConversion &kc = _akcKeys[iKey];
    // set the name
    if (kc.kc_strName!=NULL) {
      inp_strButtonNames[kc.kc_iKID] = kc.kc_strName;
      if (strlen(kc.kc_strNameTrans)==0) {
        inp_strButtonNamesTra[kc.kc_iKID] = kc.kc_strName;
      } else {
        inp_strButtonNamesTra[kc.kc_iKID] = TranslateConst(kc.kc_strNameTrans, 4);
      }
    }
  }

  return(TRUE);
}


LONG CInput::PlatformGetJoystickCount(void)
{
  LONG retval = (LONG) 0;
  return(retval);
}


// check if a joystick exists
BOOL CInput::CheckJoystick(INDEX iJoy)
{
  return FALSE;
}


void CInput::EnableInput(HWND hwnd)
{
  // skip if already enabled
  if( inp_bInputEnabled) return;

  // determine screen center position
  int winw, winh;
  Q3E_GetWindowSize(hwnd, &winw, &winh);
  inp_slScreenCenterX = winw / 2;
  inp_slScreenCenterY = winh / 2;

  // remember mouse pos
  int mousex, mousey;
  Q3E_GetMouseState(&mousex, &mousey);  // !!! FIXME: this isn't necessarily (hwnd)
  inp_ptOldMousePos.x = mousex;
  inp_ptOldMousePos.y = mousey;

  Q3E_SetRelativeMouseMode(inp_bSDLGrabInput);

  // save system mouse settings
  memset(&inp_mscMouseSettings, '\0', sizeof (MouseSpeedControl));


  // clear button's buffer
  memset( _abKeysPressed, 0, sizeof( _abKeysPressed));

  // remember current status
  inp_bInputEnabled = TRUE;
  inp_bPollJoysticks = FALSE;
}


/*
 * Disable direct input
 */
void CInput::DisableInput( void)
{
  // skip if allready disabled
  if( !inp_bInputEnabled) return;

  // show mouse on screen
  Q3E_SetRelativeMouseMode(false);

  // eventually disable 2nd mouse

  // remember current status
  inp_bInputEnabled = FALSE;
  inp_bPollJoysticks = FALSE;
}

#define USE_MOUSEWARP 1 //karin: TODO
// Define this to use GetMouse instead of using Message to read mouse coordinates

// blank any queued mousemove events...SDLInput.cpp needs this when
//  returning from the menus/console to game or the viewport will jump...
void CInput::ClearRelativeMouseMotion(void)
{
    mouse_relative_x = mouse_relative_y = 0;
}


/*
 * Scan states of all available input sources
 */
void CInput::GetInput(BOOL bPreScan)
{
//  CTSingleLock sl(&csInput, TRUE);

  if (!inp_bInputEnabled) {
    return;
  }

  if (bPreScan && !inp_bAllowPrescan) {
    return;
  }

  // if not pre-scanning
  if (!bPreScan) {
    // clear button's buffer
    memset( inp_ubButtonsBuffer, 0, sizeof( inp_ubButtonsBuffer));

    const Uint8 *keystate = Q3E_GetKeyboardState(NULL);
    // for each Key
    for (INDEX iKey=0; iKey<static_cast<INDEX>(ARRAYCOUNT(_akcKeys)); iKey++) {
      const KeyConversion &kc = _akcKeys[iKey];
      // get codes
      INDEX iKID  = kc.kc_iKID;
      //INDEX iScan = kc.kc_iScanCode;
      INDEX iVirt = kc.kc_iVirtKey;
      // if reading async keystate
      if (0/*inp_iKeyboardReadingMethod==0*/) {
        // if there is a valid virtkey
        if (iVirt>=0) {
          // is state is pressed
          if (keystate[Q3E_GetScancodeFromKey((SDL_Keycode)iVirt)]) {
            // mark it as pressed
            inp_ubButtonsBuffer[iKID] = 0xFF;
          }
        }
      }
      else
      {
        // if snooped that key is pressed
        if (_abKeysPressed[iKID]) {
          // mark it as pressed
          inp_ubButtonsBuffer[iKID] = 0xFF;
        }
      }
    }
  }

  // read mouse position
  #ifdef USE_MOUSEWARP
  int iMx, iMy;
  Q3E_GetRelativeMouseState(&iMx, &iMy);
  mouse_relative_x = iMx;
  mouse_relative_y = iMy;
  #else
  if ((mouse_relative_x != 0) || (mouse_relative_y != 0)) 
  #endif
  {
    FLOAT fDX = FLOAT( mouse_relative_x );
    FLOAT fDY = FLOAT( mouse_relative_y );

    mouse_relative_x = mouse_relative_y = 0;

    FLOAT fSensitivity = inp_fMouseSensitivity;
    if( inp_bAllowMouseAcceleration) fSensitivity *= 0.25f;

    FLOAT fD = Sqrt(fDX*fDX+fDY*fDY);
    if (inp_bMousePrecision) {
      static FLOAT _tmTime = 0.0f;
      if( fD<inp_fMousePrecisionThreshold) _tmTime += 0.05f;
      else _tmTime = 0.0f;
      if( _tmTime>inp_fMousePrecisionTimeout) fSensitivity /= inp_fMousePrecisionFactor;
    }

    static FLOAT fDXOld;
    static FLOAT fDYOld;
    static TIME tmOldDelta;
    static CTimerValue tvBefore;
    CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
    TIME tmNowDelta = (tvNow-tvBefore).GetSeconds();
    if (tmNowDelta<0.001f) {
      tmNowDelta = 0.001f;
    }
    tvBefore = tvNow;

    FLOAT fDXSmooth = (fDXOld*tmOldDelta+fDX*tmNowDelta)/(tmOldDelta+tmNowDelta);
    FLOAT fDYSmooth = (fDYOld*tmOldDelta+fDY*tmNowDelta)/(tmOldDelta+tmNowDelta);
    fDXOld = fDX;
    fDYOld = fDY;
    tmOldDelta = tmNowDelta;
    if (inp_bFilterMouse) {
      fDX = fDXSmooth;
      fDY = fDYSmooth;
    }

    // get final mouse values
    FLOAT fMouseRelX = +fDX*fSensitivity;
    FLOAT fMouseRelY = -fDY*fSensitivity;
    if (inp_bInvertMouse) {
      fMouseRelY = -fMouseRelY;
    }
    FLOAT fMouseRelZ = _iMouseZ;

    // just interpret values as normal
    inp_caiAllAxisInfo[1].cai_fReading = fMouseRelX;
    inp_caiAllAxisInfo[2].cai_fReading = fMouseRelY;
    inp_caiAllAxisInfo[3].cai_fReading = fMouseRelZ;
  } 
  #ifndef USE_MOUSEWARP
  else {
    inp_caiAllAxisInfo[1].cai_fReading = 0.0;
    inp_caiAllAxisInfo[2].cai_fReading = 0.0;
    inp_caiAllAxisInfo[3].cai_fReading = 0.0;
  }
  #endif

  // if not pre-scanning
  if (!bPreScan) {
    // detect wheel up/down movement

    if (_iMouseZ>0) {
      if (_bWheelUp) {
        inp_ubButtonsBuffer[KID_MOUSEWHEELUP] = 0x00;
      } else {
        inp_ubButtonsBuffer[KID_MOUSEWHEELUP] = 0xFF;
        _iMouseZ = ClampDn(_iMouseZ-MOUSE_SCROLL_INTERVAL, 0);
      }
    }
    _bWheelUp = inp_ubButtonsBuffer[KID_MOUSEWHEELUP];
    if (_iMouseZ<0) {
      if (_bWheelDn) {
        inp_ubButtonsBuffer[KID_MOUSEWHEELDOWN] = 0x00;
      } else {
        inp_ubButtonsBuffer[KID_MOUSEWHEELDOWN] = 0xFF;
        _iMouseZ = ClampUp(_iMouseZ+MOUSE_SCROLL_INTERVAL, 0);
      }
    }
    _bWheelDn = inp_ubButtonsBuffer[KID_MOUSEWHEELDOWN];
  }

  inp_bLastPrescan = bPreScan;
}


/*
 * Scans axis and buttons for given joystick
 */
BOOL CInput::ScanJoystick(INDEX iJoy, BOOL bPreScan)
{
  return TRUE;
}

// end of SDLInput.cpp ...


