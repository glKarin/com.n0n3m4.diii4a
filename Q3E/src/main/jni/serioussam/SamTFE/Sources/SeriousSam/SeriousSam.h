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

#define APPLICATION_NAME "SeriousSam"

extern HINSTANCE _hInstance;
extern BOOL _bRunning, _bQuitScreen;

extern INDEX sam_bFullScreenActive;
extern INDEX sam_bBorderLessActive;
extern INDEX sam_iAspectSizeI;
extern INDEX sam_iAspectSizeJ;
extern INDEX sam_iScreenSizeI;
extern INDEX sam_iScreenSizeJ;
extern INDEX sam_iDisplayDepth;
extern INDEX sam_iDisplayAdapter;
extern INDEX sam_iGfxAPI;
extern INDEX sam_iVideoSetup;  // 0==speed, 1==normal, 2==quality, 3==custom
extern BOOL  sam_bAutoAdjustAudio;

extern void StartNewMode( enum GfxAPIType eAPI, INDEX iAdapter, PIX pixSizeI, PIX pixSizeJ, PIX aspSizeI, PIX aspSizeJ, enum DisplayDepth eColorDepth, BOOL bFullScreenMode, BOOL bBorderLessMode);
struct KeyData {
  char *kd_strASCII;
};

extern CGame *_pGame;
