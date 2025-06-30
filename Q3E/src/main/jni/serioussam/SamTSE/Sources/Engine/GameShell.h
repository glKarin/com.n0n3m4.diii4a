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

#ifndef SE_INCL_GAMESHELL_H
#define SE_INCL_GAMESHELL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

struct GameGUI_interface {
  // initialize game and load settings
  void (*Initialize)(const CTFileName &fnGameSettings);
  // save settings and cleanup
  void (*End)(void);
  // run a quicktest game from within editor
  void (*QuickTest)(const CTFileName &fnMapName, CDrawPort *pdpDrawport, CViewPort *pvpViewport);
  // show console window
  void (*OnInvokeConsole)(void);
  // adjust players and controls
  void (*OnPlayerSettings)(void);
  // adjust audio settings
  void (*OnAudioQuality)(void);
  // adjust video settings
  void (*OnVideoQuality)(void);
  // select current active player and controls
  void (*OnSelectPlayerAndControls)(void);
};


#endif  /* include-once check. */

