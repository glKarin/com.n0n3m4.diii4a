/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

/* rcg10072001 Moved stuff into this file. */

#include "Engine/StdH.h"
#include <Engine/Base/Timer.h>
#include <Engine/Base/Input.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/KeyNames.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/Viewport.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Synchronization.h>

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

extern INDEX inp_ctJoysticksAllowed;

extern INDEX inp_iMButton4Dn = 0x20040;
extern INDEX inp_iMButton4Up = 0x20000;
extern INDEX inp_iMButton5Dn = 0x10020;
extern INDEX inp_iMButton5Up = 0x10000;
extern INDEX inp_bMsgDebugger = FALSE;
extern INDEX inp_bForceJoystickPolling = 0;
extern INDEX inp_bAutoDisableJoysticks = 0;

/*

NOTE: Three different types of key codes are used here:
  1) kid - engine internal type - defined in KeyNames.h
  2) scancode - raw PC keyboard scancodes as returned in keydown/keyup messages
  3) virtkey - virtual key codes used by windows

*/

// name that is not translated (international)
#define INTNAME(str) str, ""
// name that is translated
#define TRANAME(str) str, "ETRS" str

// basic key conversion table
static struct KeyConversion {
  INDEX kc_iKID;
  INDEX kc_iVirtKey;
  INDEX kc_iScanCode;
  char *kc_strName;
  char *kc_strNameTrans;
} _akcKeys[] = {

  // reserved for 'no-key-pressed'
  { KID_NONE, -1, -1, TRANAME("None")},
                            
// numbers row              
  { KID_1               , '1',   2, INTNAME("1")},
  { KID_2               , '2',   3, INTNAME("2")},
  { KID_3               , '3',   4, INTNAME("3")},
  { KID_4               , '4',   5, INTNAME("4")},
  { KID_5               , '5',   6, INTNAME("5")},
  { KID_6               , '6',   7, INTNAME("6")},
  { KID_7               , '7',   8, INTNAME("7")},
  { KID_8               , '8',   9, INTNAME("8")},
  { KID_9               , '9',  10, INTNAME("9")},
  { KID_0               , '0',  11, INTNAME("0")},
  { KID_MINUS           , 189,  12, INTNAME("-")},
  { KID_EQUALS          , 187,  13, INTNAME("=")},
                            
// 1st alpha row            
  { KID_Q               , 'Q',  16, INTNAME("Q")},
  { KID_W               , 'W',  17, INTNAME("W")},
  { KID_E               , 'E',  18, INTNAME("E")},
  { KID_R               , 'R',  19, INTNAME("R")},
  { KID_T               , 'T',  20, INTNAME("T")},
  { KID_Y               , 'Y',  21, INTNAME("Y")},
  { KID_U               , 'U',  22, INTNAME("U")},
  { KID_I               , 'I',  23, INTNAME("I")},
  { KID_O               , 'O',  24, INTNAME("O")},
  { KID_P               , 'P',  25, INTNAME("P")},
  { KID_LBRACKET        , 219,  26, INTNAME("[")},
  { KID_RBRACKET        , 221,  27, INTNAME("]")},
  { KID_BACKSLASH       , 220,  43, INTNAME("\\")},
                            
// 2nd alpha row            
  { KID_A               , 'A',  30, INTNAME("A")},
  { KID_S               , 'S',  31, INTNAME("S")},
  { KID_D               , 'D',  32, INTNAME("D")},
  { KID_F               , 'F',  33, INTNAME("F")},
  { KID_G               , 'G',  34, INTNAME("G")},
  { KID_H               , 'H',  35, INTNAME("H")},
  { KID_J               , 'J',  36, INTNAME("J")},
  { KID_K               , 'K',  37, INTNAME("K")},
  { KID_L               , 'L',  38, INTNAME("L")},
  { KID_SEMICOLON       , 186,  39, INTNAME(";")},
  { KID_APOSTROPHE      , 222,  40, INTNAME("'")},
// 3rd alpha row
  { KID_Z               , 'Z',  44, INTNAME("Z")},
  { KID_X               , 'X',  45, INTNAME("X")},
  { KID_C               , 'C',  46, INTNAME("C")},
  { KID_V               , 'V',  47, INTNAME("V")},
  { KID_B               , 'B',  48, INTNAME("B")},
  { KID_N               , 'N',  49, INTNAME("N")},
  { KID_M               , 'M',  50, INTNAME("M")},
  { KID_COMMA           , 190,  51, INTNAME(",")},
  { KID_PERIOD          , 188,  52, INTNAME(".")},
  { KID_SLASH           , 191,  53, INTNAME("/")},
                                       
// row with F-keys                     
  { KID_F1              ,  VK_F1,  59, INTNAME("F1")},
  { KID_F2              ,  VK_F2,  60, INTNAME("F2")},
  { KID_F3              ,  VK_F3,  61, INTNAME("F3")},
  { KID_F4              ,  VK_F4,  62, INTNAME("F4")},
  { KID_F5              ,  VK_F5,  63, INTNAME("F5")},
  { KID_F6              ,  VK_F6,  64, INTNAME("F6")},
  { KID_F7              ,  VK_F7,  65, INTNAME("F7")},
  { KID_F8              ,  VK_F8,  66, INTNAME("F8")},
  { KID_F9              ,  VK_F9,  67, INTNAME("F9")},
  { KID_F10             , VK_F10,  68, INTNAME("F10")},
  { KID_F11             , VK_F11,  87, INTNAME("F11")},
  { KID_F12             , VK_F12,  88, INTNAME("F12")},
                            
// extra keys               
  { KID_ESCAPE          , VK_ESCAPE,     1, TRANAME("Escape")},
  { KID_TILDE           , -1,           41, TRANAME("Tilde")},
  { KID_BACKSPACE       , VK_BACK,      14, TRANAME("Backspace")},
  { KID_TAB             , VK_TAB,       15, TRANAME("Tab")},
  { KID_CAPSLOCK        , VK_CAPITAL,   58, TRANAME("Caps Lock")},
  { KID_ENTER           , VK_RETURN,    28, TRANAME("Enter")},
  { KID_SPACE           , VK_SPACE,     57, TRANAME("Space")},
                                            
// modifier keys                            
  { KID_LSHIFT          , VK_LSHIFT  , 42, TRANAME("Left Shift")},
  { KID_RSHIFT          , VK_RSHIFT  , 54, TRANAME("Right Shift")},
  { KID_LCONTROL        , VK_LCONTROL, 29, TRANAME("Left Control")},
  { KID_RCONTROL        , VK_RCONTROL, 256+29, TRANAME("Right Control")},
  { KID_LALT            , VK_LMENU   , 56, TRANAME("Left Alt")},
  { KID_RALT            , VK_RMENU   , 256+56, TRANAME("Right Alt")},
                            
// navigation keys          
  { KID_ARROWUP         , VK_UP,        256+72, TRANAME("Arrow Up")},
  { KID_ARROWDOWN       , VK_DOWN,      256+80, TRANAME("Arrow Down")},
  { KID_ARROWLEFT       , VK_LEFT,      256+75, TRANAME("Arrow Left")},
  { KID_ARROWRIGHT      , VK_RIGHT,     256+77, TRANAME("Arrow Right")},
  { KID_INSERT          , VK_INSERT,    256+82, TRANAME("Insert")},
  { KID_DELETE          , VK_DELETE,    256+83, TRANAME("Delete")},
  { KID_HOME            , VK_HOME,      256+71, TRANAME("Home")},
  { KID_END             , VK_END,       256+79, TRANAME("End")},
  { KID_PAGEUP          , VK_PRIOR,     256+73, TRANAME("Page Up")},
  { KID_PAGEDOWN        , VK_NEXT,      256+81, TRANAME("Page Down")},
  { KID_PRINTSCR        , VK_SNAPSHOT,  256+55, TRANAME("Print Screen")},
  { KID_SCROLLLOCK      , VK_SCROLL,    70, TRANAME("Scroll Lock")},
  { KID_PAUSE           , VK_PAUSE,     69, TRANAME("Pause")},
                            
// numpad numbers           
  { KID_NUM0            , VK_NUMPAD0, 82, INTNAME("Num 0")},
  { KID_NUM1            , VK_NUMPAD1, 79, INTNAME("Num 1")},
  { KID_NUM2            , VK_NUMPAD2, 80, INTNAME("Num 2")},
  { KID_NUM3            , VK_NUMPAD3, 81, INTNAME("Num 3")},
  { KID_NUM4            , VK_NUMPAD4, 75, INTNAME("Num 4")},
  { KID_NUM5            , VK_NUMPAD5, 76, INTNAME("Num 5")},
  { KID_NUM6            , VK_NUMPAD6, 77, INTNAME("Num 6")},
  { KID_NUM7            , VK_NUMPAD7, 71, INTNAME("Num 7")},
  { KID_NUM8            , VK_NUMPAD8, 72, INTNAME("Num 8")},
  { KID_NUM9            , VK_NUMPAD9, 73, INTNAME("Num 9")},
  { KID_NUMDECIMAL      , VK_DECIMAL, 83, INTNAME("Num .")},
                            
// numpad gray keys         
  { KID_NUMLOCK         , VK_NUMLOCK,   256+69, INTNAME("Num Lock")},
  { KID_NUMSLASH        , VK_DIVIDE,    256+53, INTNAME("Num /")},
  { KID_NUMMULTIPLY     , VK_MULTIPLY,  55, INTNAME("Num *")},
  { KID_NUMMINUS        , VK_SUBTRACT,  74, INTNAME("Num -")},
  { KID_NUMPLUS         , VK_ADD,       78, INTNAME("Num +")},
  { KID_NUMENTER        , VK_SEPARATOR, 256+28, TRANAME("Num Enter")},

// mouse buttons
  { KID_MOUSE1          , VK_LBUTTON, -1, TRANAME("Mouse Button 1")},
  { KID_MOUSE2          , VK_RBUTTON, -1, TRANAME("Mouse Button 2")},
  { KID_MOUSE3          , VK_MBUTTON, -1, TRANAME("Mouse Button 3")},
  { KID_MOUSE4          , -1, -1, TRANAME("Mouse Button 4")},
  { KID_MOUSE5          , -1, -1, TRANAME("Mouse Button 5")},
  { KID_MOUSEWHEELUP    , -1, -1, TRANAME("Mouse Wheel Up")},
  { KID_MOUSEWHEELDOWN  , -1, -1, TRANAME("Mouse Wheel Down")},

// 2nd mouse buttons
  { KID_2MOUSE1         , -1, -1, TRANAME("2nd Mouse Button 1")},
  { KID_2MOUSE2         , -1, -1, TRANAME("2nd Mouse Button 2")},
  { KID_2MOUSE3         , -1, -1, TRANAME("2nd Mouse Button 3")},
};


// autogenerated fast conversion tables
static INDEX _aiScanToKid[512];
static INDEX _aiVirtToKid[256];

// make fast conversion tables from the general table
static void MakeConversionTables(void)
{
  // clear conversion tables
  memset(_aiScanToKid, -1, sizeof(_aiScanToKid));
  memset(_aiVirtToKid, -1, sizeof(_aiVirtToKid));

  // for each Key
  for (INDEX iKey=0; iKey<ARRAYCOUNT(_akcKeys); iKey++) {
    struct KeyConversion &kc = _akcKeys[iKey];

    // get codes
    INDEX iKID  = kc.kc_iKID;
    INDEX iScan = kc.kc_iScanCode;
    INDEX iVirt = kc.kc_iVirtKey;

    // update the tables
    if (iScan>=0) {
      _aiScanToKid[iScan] = iKID;
    }
    if (iVirt>=0) {
      _aiVirtToKid[iVirt] = iKID;
    }
  }
}

// variables for message interception
static HHOOK _hGetMsgHook = NULL;
static HHOOK _hSendMsgHook = NULL;
static int _iMouseZ = 0;
static BOOL _bWheelUp = FALSE;
static BOOL _bWheelDn = FALSE;

CTCriticalSection csInput;

// which keys are pressed, as recorded by message interception (by KIDs)
static UBYTE _abKeysPressed[256];

// set a key according to a keydown/keyup message
static void SetKeyFromMsg(MSG *pMsg, BOOL bDown)
{
  INDEX iKID = -1;
  // if capturing scan codes
  if (inp_iKeyboardReadingMethod==2) {
    // get scan code
    INDEX iScan = (pMsg->lParam>>16)&0x1FF; // (we use the extended bit too!)
    // convert scan code to kid
    iKID = _aiScanToKid[iScan];
  // if capturing virtual key codes
  } else if (inp_iKeyboardReadingMethod==1) {
    // get virtualkey
    INDEX iVirt = (pMsg->wParam)&0xFF;

    if (iVirt == VK_SHIFT) {
      iVirt = VK_LSHIFT;
    }
    if (iVirt == VK_CONTROL) {
      iVirt = VK_LCONTROL;
    }
    if (iVirt == VK_MENU) {
      iVirt = VK_LMENU;
    }
    // convert virtualkey to kid
    iKID = _aiVirtToKid[iVirt];
  // if not capturing
  } else {
    // do nothing
    return;
  }
  if (iKID>=0 && iKID<ARRAYCOUNT(_abKeysPressed)) {
//    CPrintF("%s: %d\n", _pInput->inp_strButtonNames[iKID], bDown);
    _abKeysPressed[iKID] = bDown;
  }
}

static void CheckMessage(MSG *pMsg)
{
  if ( pMsg->message == WM_LBUTTONUP) {
    _abKeysPressed[KID_MOUSE1] = FALSE;
  } else if ( pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONDBLCLK) {
    _abKeysPressed[KID_MOUSE1] = TRUE;
  } else if ( pMsg->message == WM_RBUTTONUP) {
    _abKeysPressed[KID_MOUSE2] = FALSE;
  } else if ( pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_RBUTTONDBLCLK) {
    _abKeysPressed[KID_MOUSE2] = TRUE;
  } else if ( pMsg->message == WM_MBUTTONUP) {
    _abKeysPressed[KID_MOUSE3] = FALSE;
  } else if ( pMsg->message == WM_MBUTTONDOWN || pMsg->message == WM_MBUTTONDBLCLK) {
    _abKeysPressed[KID_MOUSE3] = TRUE;

  } else if ( pMsg->message == inp_iMButton4Dn) {
    _abKeysPressed[KID_MOUSE4] = TRUE;
  } else if ( pMsg->message == inp_iMButton4Up) {
    _abKeysPressed[KID_MOUSE4] = FALSE;

  } else if ( pMsg->message == inp_iMButton5Dn) {
    _abKeysPressed[KID_MOUSE5] = TRUE;
  } else if ( pMsg->message == inp_iMButton5Up) {
    _abKeysPressed[KID_MOUSE5] = FALSE;

  } else if (pMsg->message==WM_KEYUP || pMsg->message==WM_SYSKEYUP) {
    SetKeyFromMsg(pMsg, FALSE);
  } else if (pMsg->message==WM_KEYDOWN || pMsg->message==WM_SYSKEYDOWN) {
    SetKeyFromMsg(pMsg, TRUE);
  } else if (inp_bMsgDebugger && pMsg->message >= 0x10000) {
    CPrintF("%08x(%d)\n", pMsg->message, pMsg->message);
  }
}


// procedure called when message is retreived
LRESULT CALLBACK GetMsgProc(
  int nCode,      // hook code
  WPARAM wParam,  // message identifier
  LPARAM lParam)  // mouse coordinates
{
  MSG *pMsg = (MSG*)lParam;
  CheckMessage(pMsg);

  LRESULT retValue = 0;
  retValue = CallNextHookEx( _hGetMsgHook, nCode, wParam, lParam );

#ifndef WM_MOUSEWHEEL
 #define WM_MOUSEWHEEL 0x020A
#endif

  if (wParam == PM_NOREMOVE) {
    return retValue;
  }

  if ( pMsg->message == WM_MOUSEWHEEL) {
    _iMouseZ += SWORD(UWORD(HIWORD(pMsg->wParam)));
  }

  return retValue;
}


// procedure called when message is sent
LRESULT CALLBACK SendMsgProc(
  int nCode,      // hook code
  WPARAM wParam,  // message identifier
  LPARAM lParam)  // mouse coordinates
{
  MSG *pMsg = (MSG*)lParam;
  CheckMessage(pMsg);

  LRESULT retValue = 0;
  retValue = CallNextHookEx( _hSendMsgHook, nCode, wParam, lParam );
  
  return retValue;
}



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
  DWORD dwLength = Min((INDEX)MOUSECOMBUFFERSIZE, (INDEX)csComStat.cbInQue);
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
    _h2ndMouse = CreateFileA( str2ndMousePort, GENERIC_READ|GENERIC_WRITE, 0, NULL,           
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( _h2ndMouse==INVALID_HANDLE_VALUE) {
      // failed! :(
      INDEX iError = GetLastError();
/*
      if( iError==5) CPrintF( "Cannot open %s (access denied).\n"
                              "The port is probably already used by another device (mouse, modem...)\n",
                              str2ndMousePort);
      else CPrintF( "Cannot open %s (error %d).\n", str2ndMousePort, iError);
      */
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


BOOL CInput::PlatformInit(void)
{
  _h2ndMouse = NONE;
  MakeConversionTables();
  return TRUE;
}


// destructor
CInput::~CInput()
{
  if( _h2ndMouse!=NONE) CloseHandle( _h2ndMouse);
  _h2ndMouse = NONE;
}


BOOL CInput::PlatformSetKeyNames(void)
{
  // for each Key
  for (INDEX iKey=0; iKey<ARRAYCOUNT(_akcKeys); iKey++) {
    struct KeyConversion &kc = _akcKeys[iKey];
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

  return TRUE;
}


void CInput::EnableInput(HWND hwnd)
{
  // skip if already enabled
  if( inp_bInputEnabled) return;

  // get window rectangle
  RECT rectClient;
  GetClientRect(hwnd, &rectClient);
  POINT pt;
  pt.x=0;
  pt.y=0;
  ClientToScreen(hwnd, &pt);
  OffsetRect(&rectClient, pt.x, pt.y);

  // remember mouse pos
  GetCursorPos( &inp_ptOldMousePos);
  // set mouse clip region
  ClipCursor(&rectClient);
  // determine screen center position
  inp_slScreenCenterX = (rectClient.left + rectClient.right) / 2;
  inp_slScreenCenterY = (rectClient.top + rectClient.bottom) / 2;

  // clear mouse from screen
  while (ShowCursor(FALSE) >= 0);
  // save system mouse settings
  SystemParametersInfo(SPI_GETMOUSE, 0, &inp_mscMouseSettings, 0);
  // set new mouse speed
  if (!inp_bAllowMouseAcceleration) {
    MouseSpeedControl mscNewSetting = { 0, 0, 0};
    SystemParametersInfo(SPI_SETMOUSE, 0, &mscNewSetting, 0);
  }
  // set cursor position to screen center
  SetCursorPos(inp_slScreenCenterX, inp_slScreenCenterY);

  _hGetMsgHook  = SetWindowsHookEx(WH_GETMESSAGE, &GetMsgProc, NULL, GetCurrentThreadId());
  _hSendMsgHook = SetWindowsHookEx(WH_CALLWNDPROC, &SendMsgProc, NULL, GetCurrentThreadId());

  // if required, try to enable 2nd mouse
  Shutdown2ndMouse();
  inp_i2ndMousePort = Clamp( inp_i2ndMousePort, (INDEX)0, (INDEX)4);
  Startup2ndMouse(inp_i2ndMousePort);

  // clear button's buffer
  memset( _abKeysPressed, 0, sizeof( _abKeysPressed));

  // This can be enabled to pre-read the state of currently pressed keys
  // for snooping methods, since they only detect transitions.
  // That has effect of detecting key presses for keys that were held down before
  // enabling.
  // the entire thing is disabled because it caused last menu key to re-apply in game.
#if 0
  // for each Key
  {for (INDEX iKey=0; iKey<ARRAYCOUNT(_akcKeys); iKey++) {
    struct KeyConversion &kc = _akcKeys[iKey];
    // get codes
    INDEX iKID  = kc.kc_iKID;
    INDEX iScan = kc.kc_iScanCode;
    INDEX iVirt = kc.kc_iVirtKey;

    // if there is a valid virtkey
    if (iVirt>=0) {
      // transcribe if modifier
      if (iVirt == VK_LSHIFT) {
        iVirt = VK_SHIFT;
      }
      if (iVirt == VK_LCONTROL) {
        iVirt = VK_CONTROL;
      }
      if (iVirt == VK_LMENU) {
        iVirt = VK_MENU;
      }
      // is state is pressed
      if (GetAsyncKeyState(iVirt)&0x8000) {
        // mark it as pressed
        _abKeysPressed[iKID] = 0xFF;
      }
    }
  }}
#endif
  
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
  
  UnhookWindowsHookEx(_hGetMsgHook);
  UnhookWindowsHookEx(_hSendMsgHook);

  // set mouse clip region to entire screen
  ClipCursor(NULL);
  // restore mouse pos
  SetCursorPos( inp_ptOldMousePos.x, inp_ptOldMousePos.y);

  // show mouse on screen
  while (ShowCursor(TRUE) < 0);
  // set system mouse settings
  SystemParametersInfo(SPI_SETMOUSE, 0, &inp_mscMouseSettings, 0);

  // eventually disable 2nd mouse
  Shutdown2ndMouse();

  // remember current status
  inp_bInputEnabled = FALSE;
  inp_bPollJoysticks = FALSE;
}


// blank any queued mousemove events...SDLInput.cpp needs this when
//  returning from the menus/console to game or the viewport will jump...  --ryan.
void CInput::ClearRelativeMouseMotion(void)
{
  // no-op on win32.  --ryan.
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

    // for each Key
    {for (INDEX iKey=0; iKey<ARRAYCOUNT(_akcKeys); iKey++) {
      struct KeyConversion &kc = _akcKeys[iKey];
      // get codes
      INDEX iKID  = kc.kc_iKID;
      INDEX iScan = kc.kc_iScanCode;
      INDEX iVirt = kc.kc_iVirtKey;

      // if reading async keystate
      if (inp_iKeyboardReadingMethod==0) {
        // if there is a valid virtkey
        if (iVirt>=0) {
          // transcribe if modifier
          if (iVirt == VK_LSHIFT) {
            iVirt = VK_SHIFT;
          }
          if (iVirt == VK_LCONTROL) {
            iVirt = VK_CONTROL;
          }
          if (iVirt == VK_LMENU) {
            iVirt = VK_MENU;
          }
          // is state is pressed
          if (GetAsyncKeyState(iVirt)&0x8000) {
            // mark it as pressed
            inp_ubButtonsBuffer[iKID] = 0xFF;
          }
        }
    
      // if snooping messages
      } else {
        // if snooped that key is pressed
        if (_abKeysPressed[iKID]) {
          // mark it as pressed
          inp_ubButtonsBuffer[iKID] = 0xFF;
        }
      }
    }}
  }

  // read mouse position
  POINT pntMouse;
  if( GetCursorPos( &pntMouse))
  {
    FLOAT fDX = FLOAT( SLONG(pntMouse.x) - inp_slScreenCenterX);
    FLOAT fDY = FLOAT( SLONG(pntMouse.y) - inp_slScreenCenterY);

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

    // if not pre-scanning
    if (!bPreScan) {
      // detect wheel up/down movement
      _bWheelDn = FALSE;
      if (_iMouseZ>0) {
        if (_bWheelUp) {
          inp_ubButtonsBuffer[KID_MOUSEWHEELUP] = 0x00;
        } else {
          inp_ubButtonsBuffer[KID_MOUSEWHEELUP] = 0xFF;
          _iMouseZ = ClampDn(_iMouseZ-120, 0);
        }
      }
      _bWheelUp = inp_ubButtonsBuffer[KID_MOUSEWHEELUP];
      if (_iMouseZ<0) {
        if (_bWheelDn) {
          inp_ubButtonsBuffer[KID_MOUSEWHEELDOWN] = 0x00;
        } else {
          inp_ubButtonsBuffer[KID_MOUSEWHEELDOWN] = 0xFF;
          _iMouseZ = ClampUp(_iMouseZ+120, 0);
        }
      }
      _bWheelDn = inp_ubButtonsBuffer[KID_MOUSEWHEELDOWN];
    }
  }
  inp_bLastPrescan = bPreScan;

  // set cursor position to screen center
  if (pntMouse.x!=inp_slScreenCenterX || pntMouse.y!=inp_slScreenCenterY) {
    SetCursorPos(inp_slScreenCenterX, inp_slScreenCenterY);
  }

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
  // read joystick state
  JOYINFOEX ji;
  ji.dwFlags = JOY_RETURNBUTTONS|JOY_RETURNCENTERED|JOY_RETURNPOV|JOY_RETURNR|
    JOY_RETURNX|JOY_RETURNY|JOY_RETURNZ|JOY_RETURNU|JOY_RETURNV;
  ji.dwSize = sizeof( JOYINFOEX);
  MMRESULT mmResult = joyGetPosEx( JOYSTICKID1+iJoy, &ji);

  // if some error
  if( mmResult != JOYERR_NOERROR) {
    // fail
    return FALSE;
  }

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
    SLONG slAxisReading;
    switch( iAxis) {
    case 0: slAxisReading = ji.dwXpos; break;
    case 1: slAxisReading = ji.dwYpos; break;
    case 2: slAxisReading = ji.dwZpos; break;
    case 3: slAxisReading = ji.dwRpos; break;
    case 4: slAxisReading = ji.dwUpos; break;
    case 5: slAxisReading = ji.dwVpos; break;
    }
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
      if(ji.dwButtons & (1L<<iButton)) {
        inp_ubButtonsBuffer[ iButtonTotal++] = 128;
      } else {
        inp_ubButtonsBuffer[ iButtonTotal++] = 0;
      }
    }

    // POV hat initially not pressed
  //  CPrintF("%d\n", ji.dwPOV);
    INDEX iStartPOV = iButtonTotal;
    inp_ubButtonsBuffer[ iStartPOV+0] = 0;
    inp_ubButtonsBuffer[ iStartPOV+1] = 0;
    inp_ubButtonsBuffer[ iStartPOV+2] = 0;
    inp_ubButtonsBuffer[ iStartPOV+3] = 0;

    // if we have POV
    if (inp_abJoystickHasPOV[iJoy]) {
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
    }
  }
  // successful
  return TRUE;
}

LONG CInput::PlatformGetJoystickCount(void)
{
  return((LONG) joyGetNumDevs());
}

// end of Win32Input.cpp ...

