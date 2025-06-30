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

#include <GameMP/Game.h>

// This class is exported from the GameGUI.dll
class CGameGUI {
public:
  // functions called from World Editor
  __declspec(dllexport) static void OnInvokeConsole(void);
  __declspec(dllexport) static void OnPlayerSettings(void);
  __declspec(dllexport) static void OnAudioQuality(void);
  __declspec(dllexport) static void OnVideoQuality(void);
  __declspec(dllexport) static void OnSelectPlayerAndControls(void);
};

// global game gui object
extern CGameGUI _GameGUI;
