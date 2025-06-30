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

#ifndef SE_INCL_SDLEVENTS_H
#define SE_INCL_SDLEVENTS_H

#ifdef PRAGMA_ONCE
#pragma once
#endif

#include <Engine/Base/Types.h>
#include "SDL.h"

typedef struct SSAM_SDL_MSG {
  UINT   message;
  WPARAM wParam; 
  LPARAM lParam; 
  Uint16 unicode;
} MSG, *PMSG;

#define PM_REMOVE         37337   // super l33t.  :)

#define WM_CHAR           (SDL_TEXTINPUT)
#define WM_MOUSEWHEEL     (SDL_MOUSEWHEEL)
#define WM_KEYDOWN        (SDL_KEYDOWN)
#define WM_KEYUP          (SDL_KEYUP)
#define WM_MOUSEMOVE      (SDL_MOUSEMOTION)
#define WM_QUIT           (SDL_QUIT)

extern SDL_EventType WM_SYSKEYDOWN;
extern SDL_EventType WM_LBUTTONDOWN;
extern SDL_EventType WM_LBUTTONUP;
extern SDL_EventType WM_RBUTTONDOWN;
extern SDL_EventType WM_RBUTTONUP;
extern SDL_EventType WM_PAINT;

// !!! FIXME: are these not used? Because we can deal with some of these in SDL2.
#define WM_NULL           (SDL_FIRSTEVENT)
#define WM_CLOSE          (SDL_FIRSTEVENT)
#define WM_COMMAND        (SDL_FIRSTEVENT)
#define WM_ERASEBKGND     (SDL_FIRSTEVENT)
#define WM_KILLFOCUS      (SDL_FIRSTEVENT)
#define WM_MOUSEFIRST     (SDL_FIRSTEVENT)
#define WM_MOUSELAST      (SDL_FIRSTEVENT)
#define WM_LBUTTONDBLCLK  (SDL_FIRSTEVENT)
#define WM_RBUTTONDBLCLK  (SDL_FIRSTEVENT)
#define WM_SYSCOMMAND     (SDL_FIRSTEVENT)
#define WM_SETFOCUS       (SDL_FIRSTEVENT)
#define WM_ACTIVATE       (SDL_FIRSTEVENT)
#define WM_ACTIVATEAPP    (SDL_FIRSTEVENT)
#define WM_CANCELMODE     (SDL_FIRSTEVENT)

BOOL PeekMessage(MSG *msg, void *hwnd, UINT wMsgFilterMin,
                 UINT wMsgFilterMax, UINT wRemoveMsg);
void TranslateMessage(MSG *msg);
void DispatchMessage(MSG *msg);

#define VK_ADD            SDLK_KP_PLUS
#define VK_BACK           SDLK_BACKSPACE
#define VK_CAPITAL        SDLK_CAPSLOCK
#define VK_CONTROL        SDLK_RCTRL
#define VK_DECIMAL        SDLK_KP_PERIOD
#define VK_DELETE         SDLK_DELETE
#define VK_DIVIDE         SDLK_KP_DIVIDE
#define VK_DOWN           SDLK_DOWN
#define VK_END            SDLK_END
#define VK_ESCAPE         SDLK_ESCAPE
#define VK_F1             SDLK_F1
#define VK_F2             SDLK_F2
#define VK_F3             SDLK_F3
#define VK_F4             SDLK_F4
#define VK_F5             SDLK_F5
#define VK_F6             SDLK_F6
#define VK_F7             SDLK_F7
#define VK_F8             SDLK_F8
#define VK_F9             SDLK_F9
#define VK_F10            SDLK_F10
#define VK_F11            SDLK_F11
#define VK_F12            SDLK_F12
#define VK_HOME           SDLK_HOME
#define VK_INSERT         SDLK_INSERT
#define VK_LCONTROL       SDLK_LCTRL
#define VK_LEFT           SDLK_LEFT
#define VK_LMENU          SDLK_LALT
#define VK_LSHIFT         SDLK_LSHIFT
#define VK_MENU           SDLK_LALT
#define VK_MULTIPLY       SDLK_KP_MULTIPLY
#define VK_NEXT           SDLK_PAGEDOWN
#define VK_NUMLOCK        SDLK_NUMLOCK
#define VK_NUMPAD0        SDLK_KP0
#define VK_NUMPAD1        SDLK_KP1
#define VK_NUMPAD2        SDLK_KP2
#define VK_NUMPAD3        SDLK_KP3
#define VK_NUMPAD4        SDLK_KP4
#define VK_NUMPAD5        SDLK_KP5
#define VK_NUMPAD6        SDLK_KP6
#define VK_NUMPAD7        SDLK_KP7
#define VK_NUMPAD8        SDLK_KP8
#define VK_NUMPAD9        SDLK_KP9
#define VK_PAUSE          SDLK_PAUSE
#define VK_PRIOR          SDLK_PAGEUP
#define VK_RCONTROL       SDLK_RCTRL
#define VK_RETURN         SDLK_RETURN
#define VK_RIGHT          SDLK_RIGHT
#define VK_RMENU          SDLK_RALT
#define VK_RSHIFT         SDLK_RSHIFT
#define VK_SCROLL         SDLK_SCROLLOCK
#define VK_SEPARATOR      SDLK_KP_ENTER
#define VK_SHIFT          SDLK_LSHIFT
#define VK_SNAPSHOT       SDLK_PRINT
#define VK_SPACE          SDLK_SPACE
#define VK_SUBTRACT       SDLK_KP_MINUS
#define VK_TAB            SDLK_TAB
#define VK_UP             SDLK_UP

// Pray these never get filled.
#define VK_LBUTTON        1
#define VK_RBUTTON        2
#define VK_MBUTTON        3

BOOL IsIconic(void *hWnd);
SHORT GetKeyState(int vk);
SHORT GetAsyncKeyState(int vk);
BOOL GetCursorPos(LPPOINT lpPoint);
BOOL ScreenToClient(void *hWnd, LPPOINT lpPoint);
int ShowCursor(BOOL yes);

#define LOWORD(x) (x & 0x0000FFFF)
#define HIWORD(x) ((x >> 16) & 0x0000FFFF)

#endif /* include-once blocker. */


