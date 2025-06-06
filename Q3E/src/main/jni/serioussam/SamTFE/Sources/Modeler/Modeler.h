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

// Modeler.h : main header file for the MODELER application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
// CModelerApp:
// See Modeler.cpp for the implementation of this class
//

// Class used for linking bcg textures into global list
class CBcgTexture
{
public:
  CBcgTexture();
  ~CBcgTexture();
  CListNode wt_ListNode;
  CTextureObject wt_toTexture;
  CTextureData *wt_TextureData;
  CTFileName wt_FileName;
};

// Class used for linking patches into global list
class CWorkingPatch
{
public:
  CListNode wp_ListNode;
  CTextureData *wp_TextureData;
  CTFileName wp_FileName;
};

// Class used for holding global modeler's preferences
class CAppPrefs
{
public:
	~CAppPrefs();
  BOOL ap_CopyExistingWindowPrefs;
  BOOL ap_bIsBcgVisibleByDefault;
  BOOL ap_bIsFloorVisibleByDefault;
  BOOL ap_SetDefaultColors;
  BOOL ap_AutoMaximizeWindow;
  BOOL ap_AutoWindowFit;
  BOOL ap_AllwaysSeeLamp;
  BOOL ap_bAllowSoundLock;

  COLOR ap_DefaultInkColor;
  COLOR ap_DefaultPaperColor;
  COLOR ap_MappingActiveSurfaceColor;
  COLOR ap_MappingInactiveSurfaceColor;
  COLOR ap_MappingPaperColor;
  COLOR ap_MappingWinBcgColor;

  COLOR ap_colDefaultAmbientColor;

  FLOAT ap_fDefaultHeading;
  FLOAT ap_fDefaultPitch;
  FLOAT ap_fDefaultBanking;
  FLOAT ap_fDefaultFOW;

  CTFileName ap_DefaultWinBcgTexture;

  void ReadFromIniFile();
  void WriteToIniFile();
};

class CModelerApp : public CWinApp
{
private:
public:
	BOOL SubInitInstance(void);
  BOOL m_bRefreshPatchPalette;
  BOOL m_bFirstTimeStarted;
  BOOL m_OnIdlePaused;
  void CreateNewDocument( CTFileName fnRequestedFile);
  BOOL AddModelerWorkingTexture( CTFileName fnTexName);
  BOOL AddModelerWorkingPatch( CTFileName fnPatchName);
  const CTextureObject *GetValidBcgTexture( CTFileName fnTexName);
  const CTFileName NextPrevBcgTexture( CTFileName fnTexName, INDEX iNextPrev);
  class CModelerView* GetActiveView(void);
  class CModelerDoc* GetDocument(void);

  INDEX m_iApi;
  BOOL m_bChangeDisplayModeInProgress;
	// for lamp model
  CModelData *m_pLampModelData;
	CModelObject *m_LampModelObject;
  CTextureData *m_ptdLamp;
  // for collision box
  CTextureData *m_ptdCollisionBoxTexture;
	CModelData *m_pCollisionBoxModelData;
	CModelObject *m_pCollisionBoxModelObject;
  // for floor
  CTextureData *m_ptdFloorTexture;
	CModelData *m_pFloorModelData;
	CModelObject *m_pFloorModelObject;
  CDocTemplate *m_pdtModelDocTemplate;
  CDocTemplate *m_pdtScriptTemplate;
  // List head for holding working textures
  CListHead m_WorkingTextures;
  // List head for holding working patches
  CListHead m_WorkingPatches;
  // Only instance of CAppPrefs holding preferences data for modeler application
  class CAppPrefs m_Preferences;
  // Application's Croteam font data
  CFontData *m_pfntFont;

  // ptrs to property pages
	class CDlgInfoPgNone *m_pPgInfoNone;
  class CDlgInfoPgRendering *m_pPgInfoRendering;
	class CDlgInfoPgGlobal *m_pPgInfoGlobal;
	class CDlgInfoPgMip *m_pPgInfoMip;
	class CDlgInfoPgPos *m_pPgInfoPos;
	class CDlgInfoPgAnim *m_pPgInfoAnim;
	class CDlgPgCollision *m_pPgInfoCollision;
	class CDlgPgInfoAttachingPlacement *m_pPgAttachingPlacement;
	class CDlgInfoPgSurf *m_pPgInfoSurf;
	class CDlgInfoPgColorizingSurface *m_pPgInfoColorizingSurface;

  // variables for display modes for different modes
	CChangeable m_chPlacement;
	CChangeable m_chGlobal;

  CModelerApp();
  ~CModelerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModelerApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual int ExitInstance();
	virtual int Run();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CModelerApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFilePreferences();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CModelerApp theApp;

BOOL GetFlagFromProfile( CTString strVarName, BOOL bDefault);
void SetFlagToProfile( CTString strVarName, BOOL bValue);
INDEX GetIndexFromProfile( CTString strVarName, INDEX iDefault);
void SetIndexToProfile( CTString strVarName, INDEX iValue);
COLOR GetColorFromProfile( CTString strVarName, COLOR colDefault);
void SetColorToProfile( CTString strVarName, COLOR colValue);

//--------------------------------------------------------------------------------------------
#define CLRF_CLR(clr) ( ((clr & 0xff000000) >> 24) | \
                        ((clr & 0x00ff0000) >>  8) | \
                        ((clr & 0x0000ff00) <<  8))

#define CLR_CLRF(clrref) ( ((clrref & 0x000000ff) << 24) | \
                           ((clrref & 0x0000ff00) <<  8) | \
                           ((clrref & 0x00ff0000) >>  8))

#define TOOLS_INIT_TOP 100

#include "ModelerDoc.h"
#include "ScriptDoc.h"
#include "ModelerView.h"
#include "ScriptView.h"
#include "ChildFrm.h"

#include "CtrlEditBoolean.h"
#include "LinkedSurfaceList.h"
#include "AnimComboBox.h"
#include "TextureComboBox.h"
#include "StainsComboBox.h"
#include "ChoosedColorButton.h"
#include "ColoredButton.h"
#include "PaletteButton.h"
#include "PaletteDialog.h"
#include "PatchPaletteButton.h"
#include "PatchPalette.h"
#include "DlgNewProgress.h"
#include "DlgPleaseWait.h"
#include "DlgPreferences.h"
#include "DlgMultiplyMapping.h"
#include "DlgMarkLinkedSurfaces.h"
#include "DlgAutoMipModeling.h"

#include "DlgInfoPgNone.h"
#include "DlgInfoPgGlobal.h"
#include "DlgInfoPgMip.h"
#include "DlgInfoPgPos.h"
#include "DlgInfoPgAnim.h"
#include "DlgInfoPgRendering.h"
#include "DlgPgCollision.h"
#include "DlgPgInfoAttachingPlacement.h"
#include "DlgPgInfoAttachingSound.h"
#include "DlgInfoSheet.h"
#include "DlgInfoFrame.h"
#include "DlgNumericAlpha.h"
#include "DlgCreateSpecularTexture.h"
#include "DlgCreateReflectionTexture.h"
#include "DlgExportForSkinning.h"

#include "MainFrm.h"

#include "DlgChooseAnim.h"
#include "SkyFloat.h"

/////////////////////////////////////////////////////////////////////////////
