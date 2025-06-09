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

#ifdef _DIII4A //karin: no SDL
#include "../Q3E/Q3EInput.cpp"
#else
#define __STDC_LIMIT_MACROS 1
#include "SDL.h"

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
      _aiScancodeToKid[SDL_GetScancodeFromKey(iVirt)] = iKID;
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
      SDL_SetRelativeMouseMode(SDL_FALSE);
      inp_bSDLGrabInput = FALSE;
    } // if
    else
    {
      // turn on input grab.
      SDL_SetRelativeMouseMode(SDL_TRUE);
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
      const int retval = SDL_PollEvent(sdlevent);
      if (retval)
          sdl_event_handler(sdlevent);
      return retval;
    }
    return 0;
} // SE_SDL_InputEventPoll


#if 0  // !!! FIXME:  Can we support this?
// --------- 2ND MOUSE HANDLING

#define MOUSECOMBUFFERSIZE 256L
static HANDLE _h2ndMouse = NONE;
static BOOL  _bIgnoreMouse2 = TRUE;
static INDEX _i2ndMouseX, _i2ndMouseY, _i2ndMouseButtons;
static INDEX _iByteNum = 0;
static UBYTE _aubComBytes[4] = {0,0,0,0};
static INDEX _iLastPort = -1;



static void Poll2ndMouse(void)
{
  // reset (mouse reading is relative)
  _i2ndMouseX = 0;
  _i2ndMouseY = 0;
  if( _h2ndMouse==NONE) return;

  // check
  COMSTAT csComStat;
  DWORD dwErrorFlags;
  ClearCommError( _h2ndMouse, &dwErrorFlags, &csComStat);
  DWORD dwLength = Min( MOUSECOMBUFFERSIZE, (INDEX)csComStat.cbInQue);
  if( dwLength<=0) return;

  // readout
  UBYTE aubMouseBuffer[MOUSECOMBUFFERSIZE];
  INDEX iRetries = 999;
  while( iRetries>0 && !ReadFile( _h2ndMouse, aubMouseBuffer, dwLength, &dwLength, NULL)) iRetries--;
  if( iRetries<=0) return; // what, didn't make it?

  // parse the mouse packets
  for( INDEX i=0; i<dwLength; i++)
  {
    // prepare    
    if( aubMouseBuffer[i] & 64) _iByteNum  = 0;
    if( _iByteNum<4) _aubComBytes[_iByteNum] = aubMouseBuffer[i];
    _iByteNum++;

    // buttons ?
    if( _iByteNum==1) {
      _i2ndMouseButtons &= ~3;
      _i2ndMouseButtons |= (_aubComBytes[0] & (32+16)) >>4;
    }
    // axes ?
    else if( _iByteNum==3) {
      char iDX = ((_aubComBytes[0] &  3) <<6) + _aubComBytes[1];
      char iDY = ((_aubComBytes[0] & 12) <<4) + _aubComBytes[2];
      _i2ndMouseX += iDX;
      _i2ndMouseY += iDY;
    }
    // 3rd button?
    else if( _iByteNum==4) {
      _i2ndMouseButtons &= ~4;
      if( aubMouseBuffer[i]&32) _i2ndMouseButtons |= 4;
    }
  }

  // ignore pooling?
  if( _bIgnoreMouse2) {
    if( _i2ndMouseX!=0 || _i2ndMouseY!=0) _bIgnoreMouse2 = FALSE;
    _i2ndMouseX = 0;
    _i2ndMouseY = 0;
    _i2ndMouseButtons = 0;
    return;
  }
}


static void Startup2ndMouse(INDEX iPort)
{
  // skip if disabled
  ASSERT( iPort>=0 && iPort<=4);
  if( iPort==0) return; 
  // determine port string
  CTString str2ndMousePort( 0, "COM%d", iPort);
    
  // create COM handle if needed
  if( _h2ndMouse==NONE) {
    _h2ndMouse = CreateFile( str2ndMousePort, GENERIC_READ|GENERIC_WRITE, 0, NULL,           
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( _h2ndMouse==INVALID_HANDLE_VALUE) {
      // failed! :(
      INDEX iError = GetLastError();
      if( iError==5) CPrintF( "Cannot open %s (access denied).\n"
                              "The port is probably already used by another device (mouse, modem...)\n",
                              str2ndMousePort);
      else CPrintF( "Cannot open %s (error %d).\n", str2ndMousePort, iError);
      _h2ndMouse = NONE;
      return;
    }
  }
  // setup and purge buffers
  SetupComm( _h2ndMouse, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);
  PurgeComm( _h2ndMouse, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  // setup port to 1200 7N1
  DCB dcb;
  dcb.DCBlength = sizeof(DCB);
  GetCommState( _h2ndMouse, &dcb);
  dcb.BaudRate = CBR_1200;
  dcb.ByteSize = 7;
  dcb.Parity   = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;
  dcb.fBinary = TRUE;
  dcb.fParity = TRUE;
  SetCommState( _h2ndMouse, &dcb);

  // reset
  _iByteNum = 0;
  _aubComBytes[0] = _aubComBytes[1] = _aubComBytes[2] = _aubComBytes[3] = 0;
  _bIgnoreMouse2 = TRUE; // ignore mouse polling until 1 after non-0 readout 
  _iLastPort = iPort;
  //CPrintF( "STARTUP M2!\n");
}


static void Shutdown2ndMouse(void)
{
  // skip if already disabled
  if( _h2ndMouse==NONE) return;

  // disable!
  SetCommMask( _h2ndMouse, 0);
  EscapeCommFunction( _h2ndMouse, CLRDTR);
  EscapeCommFunction( _h2ndMouse, CLRRTS);
  PurgeComm( _h2ndMouse, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
  // close port if changed
  if( _iLastPort != inp_i2ndMousePort) {
    CloseHandle( _h2ndMouse);
    _h2ndMouse = NONE;
  } // over and out
  _bIgnoreMouse2 = TRUE;
}
#endif

static SDL_Joystick **sticks = NULL;
static int ctJoysticks = 0;

BOOL CInput::PlatformInit(void)
{
#if 0
  _h2ndMouse = NONE;
#endif

  ASSERT(sticks == NULL);
  ASSERT(ctJoysticks == 0);

  _pShell->DeclareSymbol("persistent user INDEX inp_bSDLPermitCtrlG;", (void*)&inp_bSDLPermitCtrlG);
  _pShell->DeclareSymbol("persistent user INDEX inp_bSDLGrabInput;", (void*)&inp_bSDLGrabInput);
  MakeConversionTables();

  sl_csInput.cs_iIndex = 3000;

  return(TRUE);
}


// destructor
CInput::~CInput()
{
  if (sticks != NULL) {
    int max = ctJoysticks;
    for (int i = 0; i < max; i++) {
      if (sticks[i] != NULL) {
        SDL_JoystickClose(sticks[i]);
      }
    }
    delete[] sticks;
    sticks = NULL;
  }

  ctJoysticks = 0;

#if 0
  if (_h2ndMouse != NONE)
    CloseHandle(_h2ndMouse);
  _h2ndMouse = NONE;
#endif
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
  LONG retval = (LONG) SDL_NumJoysticks();
  if (retval > 0) {
    sticks = new SDL_Joystick *[retval];
    ctJoysticks = (int) retval;
    memset(sticks, '\0', sizeof (SDL_Joystick *) * retval);
  }

  return(retval);
}


// check if a joystick exists
BOOL CInput::CheckJoystick(INDEX iJoy)
{
  CPrintF(TRANSV("  joy %d:"), iJoy + 1);

  ASSERT(ctJoysticks > iJoy);
  CPrintF(" '%s'\n", SDL_JoystickNameForIndex(iJoy));

  SDL_Joystick *stick = SDL_JoystickOpen(iJoy);
  if (stick == NULL) {
    CPrintF("   ...can't open joystick.\n   reason: %s\n", SDL_GetError());
    return FALSE;
  }

  ASSERT(sticks != NULL);
  ASSERT(sticks[iJoy] == NULL);
  sticks[iJoy] = stick;

  int ctAxes = SDL_JoystickNumAxes(stick);
  CPrintF(TRANSV("    %d axes\n"), ctAxes);
  CPrintF(TRANSV("    %d buttons\n"), SDL_JoystickNumButtons(stick));
  if (SDL_JoystickNumHats(stick) > 0) {
    CPrintF(TRANSV("    POV hat present\n"));
  }

  // for each axis
  for(INDEX iAxis=0; iAxis<MAX_AXES_PER_JOYSTICK; iAxis++) {
    ControlAxisInfo &cai= inp_caiAllAxisInfo[ FIRST_JOYAXIS+iJoy*MAX_AXES_PER_JOYSTICK+iAxis];
    // remember min/max info
    cai.cai_slMin = -32768; cai.cai_slMax = 32767;
    cai.cai_bExisting = (iAxis < ctAxes);
  }

  return TRUE;
}


void CInput::EnableInput(HWND hwnd)
{
  // skip if already enabled
  if( inp_bInputEnabled) return;

  SDL_JoystickEventState(SDL_ENABLE);

  // determine screen center position
  int winw, winh;
  SDL_GetWindowSize((SDL_Window *) hwnd, &winw, &winh);
  inp_slScreenCenterX = winw / 2;
  inp_slScreenCenterY = winh / 2;

  // remember mouse pos
  int mousex, mousey;
  SDL_GetMouseState(&mousex, &mousey);  // !!! FIXME: this isn't necessarily (hwnd)
  inp_ptOldMousePos.x = mousex;
  inp_ptOldMousePos.y = mousey;

  SDL_SetRelativeMouseMode(inp_bSDLGrabInput ? SDL_TRUE : SDL_FALSE);

  // save system mouse settings
  memset(&inp_mscMouseSettings, '\0', sizeof (MouseSpeedControl));

#if 0
  // if required, try to enable 2nd mouse
  Shutdown2ndMouse();
  inp_i2ndMousePort = Clamp( inp_i2ndMousePort, 0L, 4L);
  Startup2ndMouse(inp_i2ndMousePort);
#endif

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

  SDL_JoystickEventState(SDL_DISABLE);

  // show mouse on screen
  SDL_SetRelativeMouseMode(SDL_FALSE);

  // eventually disable 2nd mouse
#if 0
  Shutdown2ndMouse();
#endif

  // remember current status
  inp_bInputEnabled = FALSE;
  inp_bPollJoysticks = FALSE;
}

#define USE_MOUSEWARP 1
// Define this to use GetMouse instead of using Message to read mouse coordinates

// blank any queued mousemove events...SDLInput.cpp needs this when
//  returning from the menus/console to game or the viewport will jump...
void CInput::ClearRelativeMouseMotion(void)
{
    #if USE_MOUSEWARP
    SDL_GetRelativeMouseState(NULL, NULL);
    #endif
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

#if 0
  // should not be usefull
  SDL_Event event;
  while (SE_SDL_InputEventPoll(&event)) { /* do nothing... */ }
#endif

  // if not pre-scanning
  if (!bPreScan) {
    // clear button's buffer
    memset( inp_ubButtonsBuffer, 0, sizeof( inp_ubButtonsBuffer));

    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
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
          if (keystate[SDL_GetScancodeFromKey((SDL_Keycode)iVirt)]) {
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
  SDL_GetRelativeMouseState(&iMx, &iMy);
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

// !!! FIXME
#if 0
  // readout 2nd mouse if enabled
  if( _h2ndMouse!=NONE)
  {
    Poll2ndMouse();
    //CPrintF( "m2X: %4d, m2Y: %4d, m2B: 0x%02X\n", _i2ndMouseX, _i2ndMouseY, _i2ndMouseButtons);

    // handle 2nd mouse buttons
    if( _i2ndMouseButtons & 2) inp_ubButtonsBuffer[KID_2MOUSE1] = 0xFF;
    if( _i2ndMouseButtons & 1) inp_ubButtonsBuffer[KID_2MOUSE2] = 0xFF;
    if( _i2ndMouseButtons & 4) inp_ubButtonsBuffer[KID_2MOUSE3] = 0xFF;

    // handle 2nd mouse movement
    FLOAT fDX = _i2ndMouseX;
    FLOAT fDY = _i2ndMouseY;
    FLOAT fSensitivity = inp_f2ndMouseSensitivity;

    FLOAT fD = Sqrt(fDX*fDX+fDY*fDY);
    if( inp_b2ndMousePrecision) {
      static FLOAT _tm2Time = 0.0f;
      if( fD<inp_f2ndMousePrecisionThreshold) _tm2Time += 0.05f;
      else _tm2Time = 0.0f;
      if( _tm2Time>inp_f2ndMousePrecisionTimeout) fSensitivity /= inp_f2ndMousePrecisionFactor;
    }

    static FLOAT f2DXOld;
    static FLOAT f2DYOld;
    static TIME tm2OldDelta;
    static CTimerValue tv2Before;
    CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
    TIME tmNowDelta = (tvNow-tv2Before).GetSeconds();
    if( tmNowDelta<0.001f) tmNowDelta = 0.001f;
    tv2Before = tvNow;

    FLOAT fDXSmooth = (f2DXOld*tm2OldDelta+fDX*tmNowDelta) / (tm2OldDelta+tmNowDelta);
    FLOAT fDYSmooth = (f2DYOld*tm2OldDelta+fDY*tmNowDelta) / (tm2OldDelta+tmNowDelta);
    f2DXOld = fDX;
    f2DYOld = fDY;
    tm2OldDelta = tmNowDelta;
    if( inp_bFilter2ndMouse) {
      fDX = fDXSmooth;
      fDY = fDYSmooth;
    }

    // get final mouse values
    FLOAT fMouseRelX = +fDX*fSensitivity;
    FLOAT fMouseRelY = -fDY*fSensitivity;
    if( inp_bInvert2ndMouse) fMouseRelY = -fMouseRelY;

    // just interpret values as normal
    inp_caiAllAxisInfo[4].cai_fReading = fMouseRelX;
    inp_caiAllAxisInfo[5].cai_fReading = fMouseRelY;
  }
#endif

  // if joystick polling is enabled
  if (inp_bPollJoysticks || inp_bForceJoystickPolling) {
    // scan all available joysticks
    for( INDEX iJoy=0; iJoy<MAX_JOYSTICKS; iJoy++) {
      if (inp_abJoystickOn[iJoy] && iJoy<inp_ctJoysticksAllowed) {
        // scan joy state
        BOOL bSucceeded = ScanJoystick(iJoy, bPreScan);
        // if joystick reading failed
        if (!bSucceeded && inp_bAutoDisableJoysticks) {
          // kill it, so it doesn't slow down CPU
          CPrintF(TRANSV("Joystick %d failed, disabling it!\n"), iJoy+1);
          inp_abJoystickOn[iJoy] = FALSE;
        }
      }
    }
  }
}


/*
 * Scans axis and buttons for given joystick
 */
BOOL CInput::ScanJoystick(INDEX iJoy, BOOL bPreScan)
{
  ASSERT(ctJoysticks > iJoy);
  ASSERT(sticks != NULL);
  ASSERT(sticks[iJoy] != NULL);

  SDL_Joystick *stick = sticks[iJoy];

  // for each available axis
  for( INDEX iAxis=0; iAxis<MAX_AXES_PER_JOYSTICK; iAxis++) {
    ControlAxisInfo &cai = inp_caiAllAxisInfo[ FIRST_JOYAXIS+iJoy*MAX_AXES_PER_JOYSTICK+iAxis];
    // if the axis is not present
    if (!cai.cai_bExisting) {
      // read as zero
      cai.cai_fReading = 0.0f;
      // skip to next axis
      continue;
    }
    // read its state
    SLONG slAxisReading = SDL_JoystickGetAxis(stick, iAxis);

    // convert from min..max to -1..+1
    FLOAT fAxisReading = FLOAT(slAxisReading-cai.cai_slMin)/(cai.cai_slMax-cai.cai_slMin)*2.0f-1.0f;

    // set current axis value
    cai.cai_fReading = fAxisReading;
  }

  // if not pre-scanning
  if (!bPreScan) {
    INDEX iButtonTotal = FIRST_JOYBUTTON+iJoy*MAX_BUTTONS_PER_JOYSTICK;
    // for each available button
    for( INDEX iButton=0; iButton<32; iButton++) {
      // test if the button is pressed
      if (SDL_JoystickGetButton(stick, iButton)) {
        inp_ubButtonsBuffer[ iButtonTotal++] = 128;
      } else {
        inp_ubButtonsBuffer[ iButtonTotal++] = 0;
      }
    }

/*
  !!! FIXME: Support hats as extra buttons...
    // POV hat initially not pressed
  //  CPrintF("%d\n", ji.dwPOV);
    INDEX iStartPOV = iButtonTotal;
    inp_ubButtonsBuffer[ iStartPOV+0] = 0;
    inp_ubButtonsBuffer[ iStartPOV+1] = 0;
    inp_ubButtonsBuffer[ iStartPOV+2] = 0;
    inp_ubButtonsBuffer[ iStartPOV+3] = 0;
    // check the four pov directions
    if (ji.dwPOV==JOY_POVFORWARD) {
      inp_ubButtonsBuffer[ iStartPOV+0] = 128;
    } else if (ji.dwPOV==JOY_POVRIGHT) {
      inp_ubButtonsBuffer[ iStartPOV+1] = 128;
    } else if (ji.dwPOV==JOY_POVBACKWARD) {
      inp_ubButtonsBuffer[ iStartPOV+2] = 128;
    } else if (ji.dwPOV==JOY_POVLEFT) {
      inp_ubButtonsBuffer[ iStartPOV+3] = 128;
    // and four mid-positions
    } else if (ji.dwPOV==JOY_POVFORWARD+4500) {
      inp_ubButtonsBuffer[ iStartPOV+0] = 128;
      inp_ubButtonsBuffer[ iStartPOV+1] = 128;
    } else if (ji.dwPOV==JOY_POVRIGHT+4500) {
      inp_ubButtonsBuffer[ iStartPOV+1] = 128;
      inp_ubButtonsBuffer[ iStartPOV+2] = 128;
    } else if (ji.dwPOV==JOY_POVBACKWARD+4500) {
      inp_ubButtonsBuffer[ iStartPOV+2] = 128;
      inp_ubButtonsBuffer[ iStartPOV+3] = 128;
    } else if (ji.dwPOV==JOY_POVLEFT+4500) {
      inp_ubButtonsBuffer[ iStartPOV+3] = 128;
      inp_ubButtonsBuffer[ iStartPOV+0] = 128;
    }
*/

  }

  // successful
  return TRUE;
}

// end of SDLInput.cpp ...


#endif
