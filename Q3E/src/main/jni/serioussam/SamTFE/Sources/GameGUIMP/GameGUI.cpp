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

// GameGUI.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#ifdef FIRST_ENCOUNTER
  #ifdef _DEBUG
    #define GAMEGUI_DLL_NAME "GameGUID.dll"
  #else
    #define GAMEGUI_DLL_NAME "GameGUI.dll"
  #endif
#else
  #ifdef _DEBUG
    #define GAMEGUI_DLL_NAME "GameGUIMPD.dll"
  #else
    #define GAMEGUI_DLL_NAME "GameGUIMP.dll"
  #endif
#endif

extern CGame *_pGame = NULL;
// global game object
CGameGUI _GameGUI;

static struct GameGUI_interface _Interface;

// initialize game and load settings
void Initialize(const CTFileName &fnGameSettings)
{
  try {
#ifdef FIRST_ENCOUNTER
    #ifndef NDEBUG 
      #define GAMEDLL "Bin\\Debug\\GameD.dll"
    #else
      #define GAMEDLL "Bin\\Game.dll"
    #endif
#else
    #ifndef NDEBUG 
      #define GAMEDLL "Bin\\Debug\\GameMPD.dll"
    #else
      #define GAMEDLL "Bin\\GameMP.dll"
    #endif
#endif
    CTFileName fnmExpanded;
    ExpandFilePath(EFP_READ, CTString(GAMEDLL), fnmExpanded);
    HMODULE hGame = LoadLibraryA(fnmExpanded);
    if (hGame==NULL) {
      ThrowF_t("%s", GetWindowsError(GetLastError()));
    }
    CGame* (*GAME_Create)(void) = (CGame* (*)(void))GetProcAddress(hGame, "GAME_Create");
    if (GAME_Create==NULL) {
      ThrowF_t("%s", GetWindowsError(GetLastError()));
    }
    _pGame = GAME_Create();

  } catch (char *strError) {
    FatalError("%s", strError);
  }
  // init game - this will load persistent symbols
  _pGame->Initialize(fnGameSettings);

}
// save settings and cleanup
void End(void)
{
  _pGame->End();
}

// run a quicktest game from within editor
void QuickTest(const CTFileName &fnMapName, CDrawPort *pdpDrawport, CViewPort *pvpViewport)
{
  _pGame->QuickTest(fnMapName, pdpDrawport, pvpViewport);
}

// show console window
void OnInvokeConsole(void)
{
  _GameGUI.OnInvokeConsole();
}

// adjust players and controls
void OnPlayerSettings(void)
{
  _GameGUI.OnPlayerSettings();
}

// adjust audio settings
void OnAudioQuality(void)
{
  _GameGUI.OnAudioQuality();
}

// adjust video settings
void OnVideoQuality(void)
{
  _GameGUI.OnVideoQuality();
}

// select current active player and controls
void OnSelectPlayerAndControls(void)
{
  _GameGUI.OnSelectPlayerAndControls();
}

extern "C" _declspec(dllexport) struct GameGUI_interface *GAMEGUI_Create(void)
{
  _Interface.Initialize                 = ::Initialize                 ;
  _Interface.End                        = ::End                        ;
  _Interface.QuickTest                  = ::QuickTest                  ;
  _Interface.OnInvokeConsole            = ::OnInvokeConsole            ;
  _Interface.OnPlayerSettings           = ::OnPlayerSettings           ;
  _Interface.OnAudioQuality             = ::OnAudioQuality             ;
  _Interface.OnVideoQuality             = ::OnVideoQuality             ;
  _Interface.OnSelectPlayerAndControls  = ::OnSelectPlayerAndControls  ;

  return &_Interface;
}

static int iDialogResult;

#define CALL_DIALOG( class_name, dlg_name)                        \
  try {                                                           \
    _pGame->Load_t();                                             \
  }                                                               \
  catch( char *pError) {                                          \
    (void) pError;                                                \
  }                                                               \
  HANDLE hOldResource = AfxGetResourceHandle();                   \
  class_name dlg_name;                                            \
  AfxSetResourceHandle( GetModuleHandleA(GAMEGUI_DLL_NAME) );     \
  iDialogResult = dlg_name.DoModal();                             \
  AfxSetResourceHandle( (HINSTANCE) hOldResource);                \
  if( iDialogResult == IDOK)                                      \
  try {                                                           \
    _pGame->Save_t();                                             \
  }                                                               \
  catch( char *pError) {                                          \
    AfxMessageBox( CString(pError));                              \
    iDialogResult = IDCANCEL;                                     \
  }

/*
 We cannot use dllmain if using MFC.
 See MSDN article "Regular DLLs Dynamically Linked to MFC" if initialization is needed.

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
*/

/////////////////////////////////////////////////////////////////////////////
// global routines called trough game's application menu

void CGameGUI::OnInvokeConsole(void)
{
  CALL_DIALOG( CDlgConsole, dlgConsole);
}

void CGameGUI::OnPlayerSettings(void)
{
  //CALL_DIALOG( CDlgPlayerSettings, dlgPlayerSettings);
}

void CGameGUI::OnAudioQuality(void)
{
  CALL_DIALOG( CDlgAudioQuality, dlgAudioQuality);
}

void CGameGUI::OnVideoQuality(void)
{
  CALL_DIALOG( CDlgVideoQuality, dlgVideoQuality);
}

void CGameGUI::OnSelectPlayerAndControls(void)
{
  //CALL_DIALOG( CDlgSelectPlayer, dlgSelectPlayerAndControls);
}
