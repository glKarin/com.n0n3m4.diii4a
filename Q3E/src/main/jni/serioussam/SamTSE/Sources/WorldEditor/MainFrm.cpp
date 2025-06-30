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


// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "MainFrm.h"
#include <Engine/Templates/Stock_CTextureData.h>
#include <process.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern COLOR acol_ColorizePallete[];
FLOAT _fLastNumKeyDownTime=-1;
FLOAT _fLastTimePressureApplied=-1;
#define BRUSH_PRESSURE_DELAY 0.25f
#define BRUSH_PRESSURE_SUB_DELAY 0.5f 

#define GET_COLOR_FROM_INI(iColor, strColorIndex) \
  {char chrColor[ 16];\
  COLOR colResult;\
  sprintf( chrColor, "0x%08x", acol_ColorizePallete[iColor]);\
  strcpy( chrColor, CStringA(theApp.GetProfileString( L"Custom picker colors", L"Color" strColorIndex, CString(chrColor))));\
  sscanf( chrColor, "0x%08x", &colResult);\
  acol_ColorizePallete[iColor] = colResult;}

#define SET_COLOR_TO_INI(iColor, strColorIndex) \
  {char chrColor[ 16];\
  sprintf( chrColor, "0x%08x", acol_ColorizePallete[iColor]);\
  theApp.WriteProfileString( L"Custom picker colors", L"Color" strColorIndex, CString(chrColor));}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_COMMAND_EX(ID_VIEW_PROPERTYCOMBO, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTYCOMBO, OnUpdateControlBarMenu)
	ON_COMMAND_EX(ID_VIEW_BROWSEDIALOGBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BROWSEDIALOGBAR, OnUpdateControlBarMenu)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIRTUAL_TREE, OnVirtualTree)
	ON_WM_CLOSE()
	ON_WM_CANCELMODE()
	ON_WM_INITMENU()
	ON_COMMAND(ID_VIEW_INFOWINDOW, OnViewInfowindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INFOWINDOW, OnUpdateViewInfowindow)
	ON_COMMAND(ID_VIEW_CSGTOOLS, OnViewCsgtools)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CSGTOOLS, OnUpdateViewCsgtools)
	ON_COMMAND(ID_VIEW_PROJECTIONS_BAR, OnViewProjectionsBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROJECTIONS_BAR, OnUpdateViewProjectionsBar)
	ON_COMMAND(ID_VIEW_WORK_BAR, OnViewWorkBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WORK_BAR, OnUpdateViewWorkBar)
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(ID_CREATE_TEXTURE, OnCreateTexture)
	ON_COMMAND(ID_CALL_MODELER, OnCallModeler)
	ON_COMMAND(ID_CALL_TEXMAKER, OnCallTexmaker)
	ON_COMMAND(ID_VIEW_SETTINGS_AND_UTILITY_BAR, OnViewSettingsAndUtilityBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SETTINGS_AND_UTILITY_BAR, OnUpdateViewSettingsAndUtilityBar)
	ON_COMMAND(ID_VIEW_SHADOWS_AND_TEXTURE_BAR, OnViewShadowsAndTextureBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHADOWS_AND_TEXTURE_BAR, OnUpdateViewShadowsAndTextureBar)
	ON_COMMAND(ID_VIEW_SELECT_ENTITY_BAR, OnViewSelectEntityBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SELECT_ENTITY_BAR, OnUpdateViewSelectEntityBar)
	ON_COMMAND(ID_VIEW_VIEW_TOOLS_BAR, OnViewViewToolsBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VIEW_TOOLS_BAR, OnUpdateViewViewToolsBar)
	ON_COMMAND(ID_VIEW_VIEW_TOOLS_BAR2, OnViewViewToolsBar2)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VIEW_TOOLS_BAR2, OnUpdateViewViewToolsBar2)
	ON_COMMAND(ID_GAME_AUDIO, OnGameAudio)
	ON_COMMAND(ID_GAME_VIDEO, OnGameVideo)
	ON_COMMAND(ID_GAME_PLAYER, OnGamePlayer)
	ON_COMMAND(ID_GAME_SELECT_PLAYER, OnGameSelectPlayer)
	ON_COMMAND(ID_SHOW_TREE_SHORTCUTS, OnShowTreeShortcuts)
	ON_COMMAND(ID_MENU_SHORTCUT01, OnMenuShortcut01)
	ON_COMMAND(ID_MENU_SHORTCUT02, OnMenuShortcut02)
	ON_COMMAND(ID_MENU_SHORTCUT03, OnMenuShortcut03)
	ON_COMMAND(ID_MENU_SHORTCUT04, OnMenuShortcut04)
	ON_COMMAND(ID_MENU_SHORTCUT05, OnMenuShortcut05)
	ON_COMMAND(ID_MENU_SHORTCUT06, OnMenuShortcut06)
	ON_COMMAND(ID_MENU_SHORTCUT07, OnMenuShortcut07)
	ON_COMMAND(ID_MENU_SHORTCUT08, OnMenuShortcut08)
	ON_COMMAND(ID_MENU_SHORTCUT09, OnMenuShortcut09)
	ON_COMMAND(ID_MENU_SHORTCUT10, OnMenuShortcut10)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT01, OnStoreMenuShortcut01)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT02, OnStoreMenuShortcut02)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT03, OnStoreMenuShortcut03)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT04, OnStoreMenuShortcut04)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT05, OnStoreMenuShortcut05)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT06, OnStoreMenuShortcut06)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT07, OnStoreMenuShortcut07)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT08, OnStoreMenuShortcut08)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT09, OnStoreMenuShortcut09)
	ON_COMMAND(ID_STORE_MENU_SHORTCUT10, OnStoreMenuShortcut10)
	ON_COMMAND(ID_CONSOLE, OnConsole)
	ON_COMMAND(ID_VIEW_MIP_TOOLS_BAR, OnViewMipToolsBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MIP_TOOLS_BAR, OnUpdateViewMipToolsBar)
	ON_COMMAND(ID_TOOL_RECREATE_TEXTURE, OnToolRecreateTexture)
	ON_COMMAND(ID_RECREATE_CURRENT_TEXTURE, OnRecreateCurrentTexture)
	ON_COMMAND(ID_LIGHT_ANIMATION, OnLightAnimation)
	ON_WM_TIMER()
	ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
	//}}AFX_MSG_MAP
	// Global help commands - modified to use html-help
	//ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	//ON_COMMAND(ID_HELP, OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_SEPARATOR,
	ID_SEPARATOR,
	ID_SEPARATOR,
	ID_SEPARATOR,
};

#define STD_BROWSER_WIDTH  162
#define STD_BROWSER_HEIGHT 400
#define STD_PROPERTYCOMBO_WIDTH  162
#define STD_PROPERTYCOMBO_HEIGHT 144

#define SET_BAR_SIZE( bar, dx, dy)   \
	bar.m_Size.cx = dx;                 \
	bar.m_Size.cy = dy;                 \
  bar.CalcDynamicLayout(0, LM_HORZDOCK)
#define LOAD_BAR_STATE( WName, HName, bar, dx, dy)                                      \
	bar.m_Size.cx = (AfxGetApp()->GetProfileInt(_T("General"),_T(WName),dx));             \
	bar.m_Size.cy = (AfxGetApp()->GetProfileInt(_T("General"),_T(HName),dy));             \
  bar.CalcDynamicLayout(0, LM_HORZDOCK)
#define SAVE_BAR_STATE( WName, HName, bar)                                              \
  AfxGetApp()->WriteProfileInt( _T("General"),_T(WName), bar.m_Size.cx);                \
	AfxGetApp()->WriteProfileInt( _T("General"),_T(HName), bar.m_Size.cy)

// test buffer keys, return pressed buffer number
extern INDEX TestKeyBuffers(void)
{
  INDEX iResult = -1;
  if( (GetKeyState( '1') & 128) != 0) iResult = 0;
  if( (GetKeyState( '2') & 128) != 0) iResult = 1;
  if( (GetKeyState( '3') & 128) != 0) iResult = 2;
  if( (GetKeyState( '4') & 128) != 0) iResult = 3;
  if( (GetKeyState( '5') & 128) != 0) iResult = 4;
  if( (GetKeyState( '6') & 128) != 0) iResult = 5;
  if( (GetKeyState( '7') & 128) != 0) iResult = 6;
  if( (GetKeyState( '8') & 128) != 0) iResult = 7;
  if( (GetKeyState( '9') & 128) != 0) iResult = 8;
  if( (GetKeyState( '0') & 128) != 0) iResult = 9;
  return iResult;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
  m_pInfoFrame = NULL;
  m_pColorPalette = NULL;
  m_pwndToolTip = NULL;
}

CMainFrame::~CMainFrame()
{
  // info frame window will be destroyed trough auto destroy object mechanism

  CWorldEditorApp *pApp = (CWorldEditorApp *)AfxGetApp();
  pApp->WriteProfileString(L"World editor", L"Last virtual tree", CString(m_fnLastVirtualTree));

  // destroy color palette
  if( m_pColorPalette != NULL)
  {
    delete m_pColorPalette;
    m_pColorPalette = NULL;
  }

  // destroy tool tip
  if( m_pwndToolTip != NULL)
  {
    delete m_pwndToolTip;
    m_pwndToolTip = NULL;
  }
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  CWorldEditorApp *pApp = (CWorldEditorApp *)AfxGetApp();
  if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

  // set same styles for use with all toolbars
  DWORD dwToolBarStyles = WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC |
			CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_GRIPPER;
  CRect rectDummy(0,0,0,0);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_MAIN) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndWorkTools.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_WORK) ||
		!m_wndWorkTools.LoadToolBar(IDR_WORK_TOOLS))
	{
		TRACE0("Failed to create work toolbar\n");
		return -1;      // fail to create
	}

  if (!m_wndStatusBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM, IDW_STATUSBAR) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

  // create pane for grid size
  UINT nID;
  UINT nStyle;
  int cxWidth;
  m_wndStatusBar.GetPaneInfo( GRID_PANE, nID, nStyle, cxWidth);
  cxWidth = 70;
  m_wndStatusBar.SetPaneInfo( GRID_PANE, nID, nStyle, cxWidth);

  // create pane for mouse coordinate
  m_wndStatusBar.GetPaneInfo( POSITION_PANE, nID, nStyle, cxWidth);
  cxWidth = 180;
  m_wndStatusBar.SetPaneInfo( POSITION_PANE, nID, nStyle, cxWidth);

  // create pane for icon telling editing mode
  m_wndStatusBar.GetPaneInfo( EDITING_MODE_ICON_PANE, nID, nStyle, cxWidth);
  cxWidth = 32;
  m_wndStatusBar.SetPaneInfo( EDITING_MODE_ICON_PANE, nID, nStyle, cxWidth);

  // create pane for editing mode
  m_wndStatusBar.GetPaneInfo( EDITING_MODE_PANE, nID, nStyle, cxWidth);
  cxWidth = 90;
  m_wndStatusBar.SetPaneInfo( EDITING_MODE_PANE, nID, nStyle, cxWidth);

	if (!m_wndCSGTools.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_CSG) ||
		!m_wndCSGTools.LoadToolBar(IDR_CSG_TOOLS))
	{
		TRACE0("Failed to create CSG tools toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndMipTools.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_MIP) ||
		!m_wndMipTools.LoadToolBar(IDR_MIP_TOOLS))
	{
		TRACE0("Failed to create mip tools toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndProjections.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_PROJECTIONS) ||
		!m_wndProjections.LoadToolBar(IDR_PROJECTIONS))
	{
		TRACE0("Failed to create projections toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndSettingsAndUtility.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_SETTINGS_AND_UTILITY)
     || !m_wndSettingsAndUtility.LoadToolBar(IDR_SETTINGS_AND_UTILITY) )
	{
		TRACE0("Failed to create settings and utility toolbar\n");
		return -1;      // fail to create
	}

  if (!m_wndShadowsAndTexture.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_SHADOWS_AND_TEXTURE)
     || !m_wndShadowsAndTexture.LoadToolBar(IDR_SHADOWS_AND_TEXTURE) )
	{
		TRACE0("Failed to create shadow and texture toolbar\n");
		return -1;
	}
  static UINT aidShadowsAndTextureToolBar[5] =
  {ID_TEXTURE_1, ID_TEXTURE_2, ID_TEXTURE_3, ID_VIEW_SHADOWS_ONOFF, ID_CALCULATE_SHADOWS_ONOFF};
  m_wndShadowsAndTexture.SetButtons( aidShadowsAndTextureToolBar, 5);

  if (!m_wndSelectEntity.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_SELECT_ENTITY)
     || !m_wndSelectEntity.LoadToolBar(IDR_SELECT_ENTITY) )
	{
		TRACE0("Failed to create select entity toolbar\n");
		return -1;
	}

	if (!m_wndViewTools.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_VIEW_TOOLS)
     || !m_wndViewTools.LoadToolBar(IDR_VIEW_TOOLS) )
	{
		TRACE0("Failed to create view tools toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndViewTools2.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_VIEW_TOOLS2)
     || !m_wndViewTools2.LoadToolBar(IDR_VIEW_TOOLS2) )
	{
		TRACE0("Failed to create view tools 2 toolbar\n");
		return -1;      // fail to create
	}

  // set horizontal size to item that will carry CSG destination combo box
  m_wndCSGTools.SetButtonInfo(0, ID_CSG_DESTINATION, TBBS_SEPARATOR, 128);
  CRect rect;
  // get dimensions of item that will carry combo
	m_wndCSGTools.GetItemRect(0, &rect);
  rect.top = 2;
	// set combo's drop down height
  rect.bottom = rect.top + 100;
  if (!m_CSGDesitnationCombo.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL,
			rect, &m_wndCSGTools, ID_CSG_DESTINATION))
	{
		TRACE0("Failed to create CSG destination combo-box\n");
		return FALSE;
	}
  m_CSGDesitnationCombo.SetDroppedWidth( 256);

  // set horizontal size to item that will carry triangularisation type combo box
  m_wndCSGTools.SetButtonInfo(11, ID_TRIANGULARIZE, TBBS_SEPARATOR, 100);
  CRect rectCombo2;
  // get dimensions of item that will carry combo
	m_wndCSGTools.GetItemRect(11, &rectCombo2);
  rectCombo2.top = 2;
	// set combo's drop down height
  rectCombo2.bottom = rectCombo2.top + 100;
  if (!m_TriangularisationCombo.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL,
			rectCombo2, &m_wndCSGTools, ID_TRIANGULARIZE))
	{
		TRACE0("Failed to create triangularization type combo-box\n");
		return FALSE;
	}

  // set horizontal size to item that will hold mip switch edit ctrl
  m_wndMipTools.SetButtonInfo(2, ID_EDIT_MIP_SWITCH_DISTANCE, TBBS_SEPARATOR, 64);
  CRect rectEdit1;
  // get dimensions of item that will carry edit ctrl
	m_wndMipTools.GetItemRect(2, &rectEdit1);
  rectEdit1.top = 2;
  rectEdit1.bottom = rectEdit1.top + 18;
  
  if (!m_ctrlEditMipSwitchDistance.Create( WS_VISIBLE|WS_BORDER,
    rectEdit1, &m_wndMipTools, ID_EDIT_MIP_SWITCH_DISTANCE) )
	{
		TRACE0("Failed to create mip switch distance edit control\n");
		return FALSE;
	}


	// Initialize dialog bar m_Browser
	if (!m_Browser.Create(this, CG_IDD_BROWSEDIALOGBAR,
		CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE | CBRS_SIZE_DYNAMIC,
		ID_VIEW_BROWSEDIALOGBAR))
	{
		TRACE0("Failed to create dialog bar m_Browser\n");
		return -1;		// fail to create
	}

	// Try to load virtual tree to browser
  m_fnLastVirtualTree = CTString( CStringA(pApp->GetProfileString(L"World editor",
    L"Last virtual tree", L"VirtualTrees\\BasicVirtualTree.vrt")));
  if( m_fnLastVirtualTree != "")
  {
    try
    {
      m_Browser.LoadVirtualTree_t( m_fnLastVirtualTree, NULL);
    }
    catch (const char *strError)
    {
      (void) strError;
      CTString strMessage;
      strMessage.PrintF("Error reading virtual tree file:\n%s.\n\nSwitching to empty virtual tree.", m_fnLastVirtualTree);
      AfxMessageBox( CString(strMessage));
      m_Browser.m_VirtualTree.MakeRoot();
      m_Browser.OnUpdateVirtualTreeControl();
    }
  }


	// Initialize dialog bar m_PropertyComboBar
  if (!m_PropertyComboBar.Create(this, CG_IDD_PROPERTYCOMBO,
		CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE | CBRS_SIZE_DYNAMIC,
		ID_VIEW_PROPERTYCOMBO))
	{
		TRACE0("Failed to create dialog bar m_PropertyComboBar\n");
		return -1;		// fail to create
	}

	// Initialize windows classic tool bar
  m_wndToolBar.SetWindowText(L"File tools");
  m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize work tool bar
  m_wndWorkTools.SetWindowText(L"Work tools");
  m_wndWorkTools.SetBarStyle(m_wndWorkTools.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndWorkTools.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize CSG tools tool bar
  m_wndCSGTools.SetWindowText(L"CSG tools");
  m_wndCSGTools.SetBarStyle(m_wndCSGTools.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndCSGTools.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize mip tools tool bar
  m_wndMipTools.SetWindowText(L"Mip tools");
  m_wndMipTools.SetBarStyle(m_wndMipTools.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndMipTools.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize projections tools tool bar
  m_wndProjections.SetWindowText(L"Projections");
  m_wndProjections.SetBarStyle(m_wndProjections.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndProjections.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize setting and utility tools tool bar
  m_wndSettingsAndUtility.SetWindowText(L"Settings and utility");
  m_wndSettingsAndUtility.SetBarStyle(m_wndSettingsAndUtility.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSettingsAndUtility.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize shadows and texture tool bar
  m_wndShadowsAndTexture.SetWindowText(L"Shadows and texture");
  m_wndShadowsAndTexture.SetBarStyle(m_wndShadowsAndTexture.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndShadowsAndTexture.EnableDocking(CBRS_ALIGN_ANY);
	// Initialize select entity tool bar
  m_wndSelectEntity.SetWindowText(L"Select entity");
  m_wndSelectEntity.SetBarStyle(m_wndSelectEntity.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSelectEntity.EnableDocking(CBRS_ALIGN_ANY);
  // Initialize view tools tool bar
  m_wndViewTools.SetWindowText(L"View tools");
  m_wndViewTools.SetBarStyle(m_wndViewTools.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndViewTools.EnableDocking(CBRS_ALIGN_ANY);
  // Initialize view tools tool bar
  m_wndViewTools2.SetWindowText(L"View tools 2");
  m_wndViewTools2.SetBarStyle(m_wndViewTools2.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndViewTools2.EnableDocking(CBRS_ALIGN_ANY);
  // Initialize browser dialog bar
  m_Browser.SetWindowText(L"Browser");
	m_Browser.EnableDocking(CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);
  // Initialize property dialog bar
  m_PropertyComboBar.SetWindowText(L"Entity properties");
  m_PropertyComboBar.EnableDocking(CBRS_ALIGN_ANY);

	EnableDocking(CBRS_ALIGN_ANY);

  // We will set default width and height of browser and property dialog bars
	SET_BAR_SIZE(m_Browser, STD_BROWSER_WIDTH, STD_BROWSER_HEIGHT);
	SET_BAR_SIZE(m_PropertyComboBar, STD_PROPERTYCOMBO_WIDTH, STD_PROPERTYCOMBO_HEIGHT);

  DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndWorkTools);
  DockControlBar(&m_wndProjections);
  DockControlBar(&m_wndSettingsAndUtility);
  DockControlBar(&m_wndShadowsAndTexture);
  DockControlBar(&m_wndSelectEntity);
  DockControlBar(&m_wndViewTools);
  DockControlBar(&m_wndViewTools2);
	DockControlBar(&m_wndCSGTools);
  DockControlBar(&m_wndMipTools);

  // dock browser and properties dialog
  DockControlBar(&m_Browser);
	DockControlBar(&m_PropertyComboBar);
	//DockControlBarRelativeTo(&m_PropertyComboBar, &m_Browser, DOCK_UP);

  // We will try to load tool docked and floated positions of all ctrl bars from INI file
	LOAD_BAR_STATE("Browser width", "Browser height", m_Browser,
    STD_BROWSER_WIDTH, STD_BROWSER_HEIGHT);
	LOAD_BAR_STATE("Property width", "Property height", m_PropertyComboBar,
    STD_PROPERTYCOMBO_WIDTH, STD_PROPERTYCOMBO_HEIGHT);
  // set font for combo and edit boxes
  m_CSGDesitnationCombo.SetFont(&theApp.m_Font);
  m_TriangularisationCombo.SetFont(&theApp.m_Font);
  m_ctrlEditMipSwitchDistance.SetFont(&theApp.m_Font);

  LoadBarState(_T("General"));

  // load custom picker colors from registry
  GET_COLOR_FROM_INI( 0, L"00");
  GET_COLOR_FROM_INI( 1, L"01");
  GET_COLOR_FROM_INI( 2, L"02");
  GET_COLOR_FROM_INI( 3, L"03");
  GET_COLOR_FROM_INI( 4, L"04");
  GET_COLOR_FROM_INI( 5, L"05");
  GET_COLOR_FROM_INI( 6, L"06");
  GET_COLOR_FROM_INI( 7, L"07");
  GET_COLOR_FROM_INI( 8, L"08");
  GET_COLOR_FROM_INI( 9, L"09");
  GET_COLOR_FROM_INI(10, L"10");
  GET_COLOR_FROM_INI(11, L"11");
  GET_COLOR_FROM_INI(12, L"12");
  GET_COLOR_FROM_INI(13, L"13");
  GET_COLOR_FROM_INI(14, L"14");
  GET_COLOR_FROM_INI(15, L"15");
  GET_COLOR_FROM_INI(16, L"16");
  GET_COLOR_FROM_INI(17, L"17");
  GET_COLOR_FROM_INI(18, L"18");
  GET_COLOR_FROM_INI(19, L"19");
  GET_COLOR_FROM_INI(20, L"20");
  GET_COLOR_FROM_INI(21, L"21");
  GET_COLOR_FROM_INI(22, L"22");
  GET_COLOR_FROM_INI(23, L"23");
  GET_COLOR_FROM_INI(24, L"24");
  GET_COLOR_FROM_INI(25, L"25");
  GET_COLOR_FROM_INI(26, L"26");
  GET_COLOR_FROM_INI(27, L"27");
  GET_COLOR_FROM_INI(28, L"28");
  GET_COLOR_FROM_INI(29, L"29");
  GET_COLOR_FROM_INI(30, L"30");
  GET_COLOR_FROM_INI(31, L"31");

  // set distance for brush vertex selecting
  PIX pixResolutionWidth = GetSystemMetrics(SM_CXSCREEN);
  _pixDeltaAroundVertex = pixResolutionWidth/128;

  return 0;
}

void CMainFrame::DockControlBarRelativeTo(CControlBar* pbarToDock,
                                          CControlBar* pbarRelativeTo,
                                          ULONG ulDockDirection /*= DOCK_RIGHT*/)
{
	CRect rectToDock;
	CRect rectRelativeTo;
	CRect rectResult;
	DWORD dw;
	UINT n;

	// get MFC to adjust the dimensions of all docked ToolBars
	// so that GetWindowRect will be accurate
//	RecalcLayout();
	pbarRelativeTo->GetWindowRect( &rectRelativeTo);
  pbarToDock->GetWindowRect( &rectToDock);

  PIX pixOffsetX = rectRelativeTo.Width();
  PIX pixOffsetY = rectRelativeTo.Height();

	rectResult = CRect( rectRelativeTo.left,
                      0/*rectRelativeTo.top*/,
                      rectRelativeTo.left+rectToDock.Width(),
                      /*rectRelativeTo.top+*/rectToDock.Height() );
	switch( ulDockDirection)
  {
  case DOCK_LEFT:
    {
      rectResult.OffsetRect( -pixOffsetX, 0);
      break;
    }
  case DOCK_RIGHT:
    {
      rectResult.OffsetRect( pixOffsetX+20, 0);
      break;
    }
  case DOCK_UP:
    {
      rectResult.OffsetRect( 0, -pixOffsetY);
      break;
    }
  case DOCK_DOWN:
    {
      rectResult.OffsetRect( 0, pixOffsetY);
      break;
    }
  }

  dw=pbarRelativeTo->GetBarStyle();
	n = 0;
	n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
	n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
	n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;

	// When we take the default parameters on rect, DockControlBar will dock
	// each Toolbar on a seperate line.  By calculating a rectangle, we in effect
	// are simulating a Toolbar being dragged to that location and docked.
	DockControlBar( pbarToDock, n, &rectResult);
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;

	return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnVirtualTree()
{
  if( m_Browser.m_TreeCtrl.m_bIsOpen)
  {
    m_Browser.m_TreeCtrl.CloseTreeCtrl();
  }
  else
  {
    m_Browser.m_TreeCtrl.OpenTreeCtrl();
  }
  m_Browser.m_TreeCtrl.SetFocus();
}

BOOL CMainFrame::DestroyWindow()
{
  m_Browser.CloseSelectedDirectory();

	return CMDIFrameWnd::DestroyWindow();
}

void CMainFrame::OnClose()
{
	SaveBarState(_T("General"));
	SAVE_BAR_STATE("Browser width", "Browser height", m_Browser);
	SAVE_BAR_STATE("Property width", "Property height", m_PropertyComboBar);

  // save custom picker colors to registry
  SET_COLOR_TO_INI( 0, L"00");
  SET_COLOR_TO_INI( 1, L"01");
  SET_COLOR_TO_INI( 2, L"02");
  SET_COLOR_TO_INI( 3, L"03");
  SET_COLOR_TO_INI( 4, L"04");
  SET_COLOR_TO_INI( 5, L"05");
  SET_COLOR_TO_INI( 6, L"06");
  SET_COLOR_TO_INI( 7, L"07");
  SET_COLOR_TO_INI( 8, L"08");
  SET_COLOR_TO_INI( 9, L"09");
  SET_COLOR_TO_INI(10, L"10");
  SET_COLOR_TO_INI(11, L"11");
  SET_COLOR_TO_INI(12, L"12");
  SET_COLOR_TO_INI(13, L"13");
  SET_COLOR_TO_INI(14, L"14");
  SET_COLOR_TO_INI(15, L"15");
  SET_COLOR_TO_INI(16, L"16");
  SET_COLOR_TO_INI(17, L"17");
  SET_COLOR_TO_INI(18, L"18");
  SET_COLOR_TO_INI(19, L"19");
  SET_COLOR_TO_INI(20, L"20");
  SET_COLOR_TO_INI(21, L"21");
  SET_COLOR_TO_INI(22, L"22");
  SET_COLOR_TO_INI(23, L"23");
  SET_COLOR_TO_INI(24, L"24");
  SET_COLOR_TO_INI(25, L"25");
  SET_COLOR_TO_INI(26, L"26");
  SET_COLOR_TO_INI(27, L"27");
  SET_COLOR_TO_INI(28, L"28");
  SET_COLOR_TO_INI(29, L"29");
  SET_COLOR_TO_INI(30, L"30");
  SET_COLOR_TO_INI(31, L"31");

  CMDIFrameWnd::OnClose();
}

void CMainFrame::OnCancelMode()
{
	// switches out of eventual direct screen mode
  CWorldEditorView *pwndView = (CWorldEditorView *)GetActiveView();
  if (pwndView != NULL) {
	  // get the MDIChildFrame of active window
	  CChildFrame *pfrChild = (CChildFrame *)pwndView->GetParentFrame();
    ASSERT(pfrChild!=NULL);
  }
  CMDIFrameWnd::OnCancelMode();
}


void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	// switches out of eventual direct screen mode
  CWorldEditorView *pwndView = (CWorldEditorView *)GetActiveView();
  if (pwndView != NULL) {
	  // get the MDIChildFrame of active window
	  CChildFrame *pfrChild = (CChildFrame *)pwndView->GetParentFrame();
    ASSERT(pfrChild!=NULL);
  }
  CMDIFrameWnd::OnInitMenu(pMenu);
}


void CMainFrame::CustomColorPicker( PIX pixX, PIX pixY)
{
  // calculate palette window's rectangle
  CRect rectWindow;
  rectWindow.left = pixX;
  rectWindow.top = pixY;
  rectWindow.right = rectWindow.left + 100;
  rectWindow.bottom = rectWindow.top + 200;

  COLOR colIntersectingColor;
  // obtain document
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  // must not be null
  if( pDoc == NULL) return;
  // if polygon mode
  if( pDoc->m_iMode == POLYGON_MODE)
  {
    // polygon selection must contain selected polygons
    ASSERT(pDoc->m_selPolygonSelection.Count() != 0);
    // obtain intersecting color
    // for each of the selected polygons
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      // if this is first polygon in dynamic container
      if( pDoc->m_selPolygonSelection.Pointer(0) == itbpo)
      {
        // it is, get color as one that others will compare with
        colIntersectingColor = itbpo->bpo_colColor;
      }
      else
      {
        // if selected polygon's color is not same as testing color
        if( colIntersectingColor != itbpo->bpo_colColor)
        {
          // set invalid color
          colIntersectingColor = MAX_ULONG;
          break;
        }
      }
    }
  }
  else if( pDoc->m_iMode == SECTOR_MODE)
  {
    // we must be in sector mode
    // sector selection must contain selected sectors
    ASSERT(pDoc->m_selSectorSelection.Count() != 0);
    // obtain intersecting color
    // for each of the selected sectors
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      // if this is first sector in dynamic container
      if( pDoc->m_selSectorSelection.Pointer(0) == itbsc)
      {
        // it is, get color as one that others will compare with
        colIntersectingColor = itbsc->bsc_colColor;
      }
      else
      {
        // if selected sector's color is not same as testing color
        if( colIntersectingColor != itbsc->bsc_colColor)
        {
          // set invalid color
          colIntersectingColor = MAX_ULONG;
          break;
        }
      }
    }
  }
  else
  {
    return;
  }

  INDEX iSelectedColor = -1;
  for( INDEX iColTab = 0; iColTab < 32; iColTab++)
  {
    if( colIntersectingColor == acol_ColorizePallete[ iColTab])
    {
      iSelectedColor = iColTab;
      break;
    }
  }

  _pcolColorToSet = NULL;
  if( m_pColorPalette == NULL)
  {
    // instantiate new choose color palette window
    m_pColorPalette = new CColorPaletteWnd;
    // create window
    BOOL bResult = m_pColorPalette->CreateEx( WS_EX_TOOLWINDOW,
      NULL, L"Palette", WS_CHILD|WS_POPUP|WS_VISIBLE,
      rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
      m_hWnd, NULL, NULL);
    if( !bResult)
    {
      AfxMessageBox( L"Error: Failed to create color palette");
      return;
    }
    // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( m_pColorPalette->m_hWnd, &m_pColorPalette->m_pViewPort,
                               &m_pColorPalette->m_pDrawPort);
  }
  else
  {
    m_pColorPalette->ShowWindow(SW_SHOW);
  }
  m_pColorPalette->m_iSelectedColor = iSelectedColor;
}

BOOL CMainFrame::OnIdle(LONG lCount)
{
  // Call OnIdle() for info frame's property sheet
  if( m_pInfoFrame != NULL)
  {
    m_pInfoFrame->m_pInfoSheet->OnIdle( lCount);
  }

  POSITION pos = theApp.m_pDocTemplate->GetFirstDocPosition();
  while (pos!=NULL)
  {
    CWorldEditorDoc *pDoc = (CWorldEditorDoc *)theApp.m_pDocTemplate->GetNextDoc(pos);
    if(pDoc!=NULL)
    {
      pDoc->OnIdle();
    }
  }


  // call on idle for combo boxes
  m_CSGDesitnationCombo.OnIdle( lCount);
  m_TriangularisationCombo.OnIdle( lCount);
  m_ctrlEditMipSwitchDistance.OnIdle( lCount);

  // call on idle for property combo bar
  m_PropertyComboBar.OnIdle( lCount);

  return TRUE;
}

/*
 * toggles info window
 */
void CMainFrame::ToggleInfoWindow(void)
{
  // toggle info state
  OnViewInfowindow();
}

/*
 * shows info window
 */
void CMainFrame::ShowInfoWindow()
{
  // if it doesn't exist or is not visible
  if( (m_pInfoFrame == NULL) ||
      (!m_pInfoFrame->IsWindowVisible()) )
  {
    // create it or toggle info state (to visible)
    OnViewInfowindow();
  }
}

/*
 * reset info window pos
 */
void CMainFrame::ResetInfoWindowPos()
{
  // if it exists and is visible
  if( (m_pInfoFrame != NULL) && (m_pInfoFrame->IsWindowVisible()) )
  {
    PIX pixScrH = ::GetSystemMetrics(SM_CYSCREEN);
    // obtain placement of selected entities text ctrl' window
    WINDOWPLACEMENT wpl;
    m_pInfoFrame->GetWindowPlacement( &wpl);
    CRect rect=wpl.rcNormalPosition;
    PIX pixw=rect.right-rect.left;
    PIX pixh=rect.bottom-rect.top;
    rect.left=0;
    rect.top=pixScrH-pixh;
    rect.right=rect.left+pixw;
    rect.bottom=rect.top+pixh;
    m_pInfoFrame->MoveWindow( &rect, TRUE);
  }
}

/*
 * hides info window
 */
void CMainFrame::HideInfoWindow()
{
  // if it exist and is visible
  if( (m_pInfoFrame != NULL) &&
      (m_pInfoFrame->IsWindowVisible()) )
  {
    // toggle info state (to hidden)
    OnViewInfowindow();
  }
}

void CMainFrame::OnViewInfowindow()
{
  // if info doesn't yet exist, create it
  if( m_pInfoFrame == NULL)
  {
    // create frame window for holding sheet object
    m_pInfoFrame = new CInfoFrame;
    // set initial size of rect window
    CRect rectInfoWindow(0, 0, 0, 0);
    if( !m_pInfoFrame->Create( NULL, L"Tools info",
        MFS_SYNCACTIVE|WS_POPUP|WS_CAPTION|WS_SYSMENU, rectInfoWindow, this))
	  {
		  AfxMessageBox(L"Failed to create info frame window m_pInfoFrame");
      return;
	  }
    //m_pInfoFrame->DragAcceptFiles();
  }

  if( m_pInfoFrame->IsWindowVisible() )
  {
    m_pInfoFrame->ShowWindow(SW_HIDE);
  }
  else
  {
    m_pInfoFrame->ShowWindow(SW_SHOW);
    m_pInfoFrame->m_pInfoSheet->SetFocus();
  }
}

void CMainFrame::OnUpdateViewInfowindow(CCmdUI* pCmdUI)
{
  BOOL bInfoVisible = FALSE;
  if( m_pInfoFrame != NULL)
  {
    bInfoVisible = m_pInfoFrame->IsWindowVisible();
  }
  pCmdUI->SetCheck( bInfoVisible);
}

void CMainFrame::ApplyTreeShortcut( INDEX iVDirBuffer, BOOL bCtrl)
{
  // if control key pressed
  if( bCtrl)
  {
    // remember current virtual directory into buffer
    INDEX iSubDirsCt;
    iSubDirsCt = m_Browser.GetSelectedDirectory( m_Browser.m_astrVTreeBuffer[iVDirBuffer]);
    m_Browser.m_aiSubDirectoriesCt[ iVDirBuffer] = iSubDirsCt;
    // mark that virtual tree has changed
    m_Browser.m_bVirtualTreeChanged = TRUE;
  }
  else
  {
    // get current directory
    CVirtualTreeNode *pVTN = m_Browser.GetSelectedDirectory();
    m_Browser.m_BrowseWindow.CloseDirectory( pVTN);
    // try to select directory
    INDEX iSubDirsCt;
    iSubDirsCt = m_Browser.m_aiSubDirectoriesCt[ iVDirBuffer];
    m_Browser.SelectVirtualDirectory( m_Browser.m_astrVTreeBuffer[iVDirBuffer], iSubDirsCt);
    // obtain newly selected directory
    pVTN = m_Browser.GetSelectedDirectory();
    // and open it
    m_Browser.m_BrowseWindow.OpenDirectory( pVTN);
  }
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
  BOOL bAltPressed = (GetKeyState( VK_MENU)&0x8000) != 0;
  // alt is pressed
  BOOL bAlt = FALSE;

  if(pMsg->message==_uiMessengerMsg)
  {
    // if one application allready started
    HWND hwndMessenger = ::FindWindow(NULL, L"Croteam Messenger");
    if(hwndMessenger != NULL)
    {
      // force messenger to popup
      ::PostMessage( hwndMessenger, _uiMessengerForcePopup, 0, 0);
    }
  }    

  if( pMsg->message==WM_LBUTTONDOWN)
  {
    BOOL bHasDocument = FALSE;
    POSITION pos = theApp.m_pDocTemplate->GetFirstDocPosition();
    while (pos!=NULL) {
      CWorldEditorDoc *pdocCurrent = (CWorldEditorDoc *)theApp.m_pDocTemplate->GetNextDoc(pos);
      bHasDocument = pdocCurrent!=NULL;
    }

    BOOL bMainFrameHasFocus = (this == CWnd::GetForegroundWindow());
    if( !bHasDocument && bMainFrameHasFocus)
    {
      static CTimerValue tvLast;
      static CPoint ptLast;
      CPoint ptNow;
      GetCursorPos( &ptNow);
      CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
      FLOAT tmDelta = (tvNow-tvLast).GetSeconds();
      if( tmDelta<0.5f && abs(ptNow.x-ptLast.x)<5 && abs(ptNow.y-ptLast.y)<5)
      {
        theApp.OnFileOpen();
      }
      tvLast=tvNow;
      ptLast = ptNow;
    }
  }

  // if we caught alt key message
  if( pMsg->message==WM_SYSKEYDOWN)
  {
    // get key data
    int lKeyData = pMsg->lParam;
    // test if it is ghost Alt-F4 situation
    if( lKeyData & (1L<<29))
    {
      // Alt key was really pressed
      bAlt = TRUE;
    }
  }

	// if we caught key down message or alt key is pressed
  if( (pMsg->message==WM_KEYDOWN) || bAlt)
  {
    int iVirtKey = (int) pMsg->wParam;
    int lKeyData = pMsg->lParam;
    // get scan code
  	UWORD uwScanCode = (HIWORD( lKeyData)) & 255;
    // if ctrl pressed
    BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
    // if left shift pressed
    BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
    // if delete pressed
    BOOL bDelete = uwScanCode == 0x53;
    // if insert pressed
    BOOL bInsert = uwScanCode == 0x52;

    // if alt+shift+S pressed, we want to engage "Spawn flags" entity property
    if( bShift && bAltPressed && (iVirtKey=='S') )
    {
      CPropertyComboBox *pPropertyCombo = &m_PropertyComboBar.m_PropertyComboBox;
      // for all members in properties combo box
      for( INDEX iMember = 0; iMember<pPropertyCombo->GetCount(); iMember++)
      {
        CPropertyID *ppidPropertyID = (CPropertyID *) pPropertyCombo->GetItemData( iMember);
        // if this is valid property
        if( (ppidPropertyID != NULL) && (ppidPropertyID->pid_strName == "Spawn flags (Alt+Shift+S)") )
        {
          // select spawn flags
          pPropertyCombo->SetCurSel( iMember);
          // update property controls (show/hide) depending upon property type
          pPropertyCombo->SelectProperty();
        }
      }
    }

    // if alt+shift+A pressed, we want to engage "Parent" entity property
    if( bShift && bAltPressed && (iVirtKey=='A') )
    {
      CPropertyComboBox *pPropertyCombo = &m_PropertyComboBar.m_PropertyComboBox;
      // for all members in properties combo box
      for( INDEX iMember = 0; iMember<pPropertyCombo->GetCount(); iMember++)
      {
        CPropertyID *ppidPropertyID = (CPropertyID *) pPropertyCombo->GetItemData( iMember);
        // if this is valid property
        if( (ppidPropertyID != NULL) && (ppidPropertyID->pid_strName == "Parent (Alt+Shift+A)") )
        {
          // select spawn flags
          pPropertyCombo->SetCurSel( iMember);
          // update property controls (show/hide) depending upon property type
          pPropertyCombo->SelectProperty();
        }
      }
    }

    // if shift pressed, we want to engage entity property shortcut
    if( bShift && !bAltPressed)
    {
      CPropertyComboBox *pPropertyCombo = &m_PropertyComboBar.m_PropertyComboBox;
      // for all members in properties combo box
      for( INDEX iMember = 0; iMember<pPropertyCombo->GetCount(); iMember++)
      {
        CPropertyID *ppidPropertyID = (CPropertyID *) pPropertyCombo->GetItemData( iMember);
        // if this is valid property
        if( ppidPropertyID != NULL)
        {
          // if virtual key-code is same as shortcut for observing property
          if( iVirtKey == ppidPropertyID->pid_chrShortcutKey)
          {
            // select observing entity property
            pPropertyCombo->SetCurSel( iMember);
            // update property controls (show/hide) depending upon property type
            pPropertyCombo->SelectProperty();
          }
        }
      }
    }
    else if( (iVirtKey == 'Q') && !bAltPressed)
    {
      ToggleInfoWindow();
    }
    else
    {
      // remap key ID to number 0-9
      INDEX iNum=-1;
      if( iVirtKey == '0') iNum = 9;
      else                 iNum = iVirtKey-'1';
      if( (iNum>=0) && (iNum<=9) && !bAlt)
      {
        CWorldEditorDoc *pDoc = theApp.GetDocument();
        if( pDoc != NULL && pDoc->GetEditingMode()==TERRAIN_MODE)
        {
          FLOAT fCurrentTime = _pTimer->GetRealTimeTick();
          if(_fLastNumKeyDownTime==-1)
          {
            _fLastNumKeyDownTime = fCurrentTime;
            return TRUE;
          }
          else if( fCurrentTime-_fLastNumKeyDownTime>BRUSH_PRESSURE_DELAY)
          {
            _fLastNumKeyDownTime = -2;
            ApplyTreeShortcut( iNum, bCtrl);
            return TRUE;
          }
        }
        else
        {
          _fLastNumKeyDownTime = -2;
          ApplyTreeShortcut( iNum, bCtrl);
          return TRUE;
        }
      }
    }
  }
  if( pMsg->message==WM_KEYUP)
  {
    // remap key ID to number 0-9
    INDEX iNum=-1;
    int iVirtKey = (int) pMsg->wParam;
    if( iVirtKey == '0') iNum = 9;
    else                 iNum = iVirtKey-'1';
    if( (iNum>=0) && (iNum<=9) && !bAlt)
    {
      CWorldEditorDoc *pDoc = theApp.GetDocument();
      if( pDoc != NULL && pDoc->GetEditingMode()==TERRAIN_MODE)
      {
        FLOAT fCurrentTime = _pTimer->GetRealTimeTick();
        if( fCurrentTime-_fLastNumKeyDownTime<BRUSH_PRESSURE_DELAY)
        {
          if( fCurrentTime-_fLastTimePressureApplied<BRUSH_PRESSURE_SUB_DELAY)
          {
            INDEX iTens=floor((theApp.m_fTerrainBrushPressure-1.0f)/1024.0f*10.0f+0.5f);
            if(iNum==9) iNum=-1;
            INDEX iResult=(iTens*10+iNum+1)%100;
            theApp.m_fTerrainBrushPressure=(iResult)/100.0f*1024.0f+1;
            _fLastTimePressureApplied=-1.0f;
          }
          else
          {
            theApp.m_fTerrainBrushPressure=(iNum+1)*10/100.0f*1024.0f+1;
            _fLastTimePressureApplied=fCurrentTime;
          }
          theApp.m_ctTerrainPageCanvas.MarkChanged();
        }
      }
    }
    _fLastNumKeyDownTime = -1;
  }

  return CMDIFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnViewCsgtools()
{
	BOOL bVisible = ((m_wndCSGTools.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndCSGTools, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewCsgtools(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndCSGTools.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewProjectionsBar()
{
	BOOL bVisible = ((m_wndProjections.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndProjections, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewProjectionsBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndProjections.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewWorkBar()
{
	BOOL bVisible = ((m_wndWorkTools.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndWorkTools, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewWorkBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndWorkTools.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewMipToolsBar()
{
	BOOL bVisible = ((m_wndMipTools.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndMipTools, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewMipToolsBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndMipTools.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask)
{
	CMDIFrameWnd::OnActivateApp(bActive, hTask);

  // if application is activated right now
  if( bActive)
  {
    // show mouse
    while (ShowCursor(TRUE)<0);

    // if browser is valid
    if( ::IsWindow( m_Browser.m_BrowseWindow.m_hWnd))
    {
      // refresh it
      m_Browser.Invalidate( FALSE);
    }
    // and all of the application's documents
    theApp.RefreshAllDocuments();
  }
}

void CMainFrame::OnCreateTexture()
{
  // call create texture dialog
  _EngineGUI.CreateTexture();
}

void CMainFrame::StartApplication( CTString strApplicationToRun)
{
	// setup necessary data for new process
  STARTUPINFOA siStartupInfo;
  siStartupInfo.cb = sizeof( STARTUPINFOA);
  siStartupInfo.lpReserved = NULL;
  siStartupInfo.lpDesktop = NULL;
  siStartupInfo.lpTitle = NULL;
  siStartupInfo.dwFlags = 0;
  siStartupInfo.cbReserved2 = 0;
  siStartupInfo.lpReserved2 = NULL;
  // here we will receive result of process creation
  PROCESS_INFORMATION piProcessInformation;

  // create application name to run
  CTFileName fnApplicationToRun = _fnmApplicationPath + strApplicationToRun;
  // create process for modeler
  BOOL bSuccess = CreateProcessA(
    fnApplicationToRun,
    NULL,
    NULL,
    NULL,
    FALSE,
    CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
    NULL,
    NULL,
    &siStartupInfo,
    &piProcessInformation);
  // if process creation was not successful
  if( !bSuccess)
  {
    WarningMessage( "WorldEditor was unable to run \"%s\"", (CTString&)fnApplicationToRun);
  }
}

void CMainFrame::OnCallModeler()
{
  StartApplication( "Modeler.exe");
}


void CMainFrame::OnCallTexmaker()
{
  StartApplication( "TexMaker.exe");
}


void CMainFrame::OnViewSettingsAndUtilityBar()
{
	BOOL bVisible = ((m_wndSettingsAndUtility.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndSettingsAndUtility, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewSettingsAndUtilityBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndSettingsAndUtility.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewShadowsAndTextureBar()
{
	BOOL bVisible = ((m_wndShadowsAndTexture.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndShadowsAndTexture, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewShadowsAndTextureBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndShadowsAndTexture.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewSelectEntityBar()
{
	BOOL bVisible = ((m_wndSelectEntity.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndSelectEntity, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewSelectEntityBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndSelectEntity.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewViewToolsBar()
{
	BOOL bVisible = ((m_wndViewTools.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndViewTools, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewViewToolsBar(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndViewTools.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewViewToolsBar2()
{
	BOOL bVisible = ((m_wndViewTools2.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndViewTools2, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewViewToolsBar2(CCmdUI* pCmdUI)
{
	BOOL bVisible = ((m_wndViewTools2.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnGameAudio()
{
  _pGameGUI->OnAudioQuality();
}

void CMainFrame::OnGameVideo()
{
  _pGameGUI->OnVideoQuality();
}

void CMainFrame::OnGamePlayer()
{
  _pGameGUI->OnPlayerSettings();
}

void CMainFrame::OnGameSelectPlayer()
{
  _pGameGUI->OnSelectPlayerAndControls();
}


void CMainFrame::OnShowTreeShortcuts()
{
  CDlgTreeShortcuts dlgTreeShortcuts;
  dlgTreeShortcuts.DoModal();

  _fLastNumKeyDownTime = -1;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  if( dlgTreeShortcuts.m_iPressedShortcut != -1)
  {
    ApplyTreeShortcut( dlgTreeShortcuts.m_iPressedShortcut, bCtrl);
  }
}

#define ON_MENU_SHORTCUT( function, index) \
  void CMainFrame::function() { /*ApplyTreeShortcut( index, FALSE);*/ }
#define ON_STORE_MENU_SHORTCUT( function, index) \
  void CMainFrame::function() { /*ApplyTreeShortcut( index, TRUE);*/ }

ON_MENU_SHORTCUT( OnMenuShortcut01, 0);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut01, 0);
ON_MENU_SHORTCUT( OnMenuShortcut02, 1);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut02, 1);
ON_MENU_SHORTCUT( OnMenuShortcut03, 2);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut03, 2);
ON_MENU_SHORTCUT( OnMenuShortcut04, 3);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut04, 3);
ON_MENU_SHORTCUT( OnMenuShortcut05, 4);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut05, 4);
ON_MENU_SHORTCUT( OnMenuShortcut06, 5);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut06, 5);
ON_MENU_SHORTCUT( OnMenuShortcut07, 6);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut07, 6);
ON_MENU_SHORTCUT( OnMenuShortcut08, 7);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut08, 7);
ON_MENU_SHORTCUT( OnMenuShortcut09, 8);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut09, 8);
ON_MENU_SHORTCUT( OnMenuShortcut10, 9);
ON_STORE_MENU_SHORTCUT( OnStoreMenuShortcut10, 9);

void CMainFrame::OnConsole()
{
  _pGameGUI->OnInvokeConsole();
}


void CMainFrame::OnToolRecreateTexture()
{
  CTFileName fnTextureToRecreate = _EngineGUI.BrowseTexture(
    CTString(""), KEY_NAME_CREATE_TEXTURE_DIR, "Browse texture to recreate");
  if( fnTextureToRecreate != "")
  {
    _EngineGUI.CreateTexture( fnTextureToRecreate);
  }
}

void CMainFrame::OnRecreateCurrentTexture()
{
  // there must be valid texture
  if( theApp.m_ptdActiveTexture == NULL) return;
  CTextureData *pTD = theApp.m_ptdActiveTexture;
  CTFileName fnTextureName = pTD->GetName();
  // call recreate texture dialog
  _EngineGUI.CreateTexture( fnTextureName);
  // try to
  CTextureData *ptdTextureToReload;
  try {
    // obtain texture
    ptdTextureToReload = _pTextureStock->Obtain_t( fnTextureName);
  }
  catch ( char *err_str) {
    AfxMessageBox( CString(err_str));
    return;
  }
  // reload the texture
  ptdTextureToReload->Reload();
  // release the texture
  _pTextureStock->Release( ptdTextureToReload);
  // if browser is valid
  if( ::IsWindow( m_Browser.m_BrowseWindow.m_hWnd))
  {
    // refresh it
    m_Browser.m_BrowseWindow.Refresh();
  }
  // obtain document
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc != NULL)
  {
    // and refresh all views
    pDoc->UpdateAllViews( NULL);
  }
}

void CMainFrame::OnLightAnimation()
{
  CDlgLightAnimationEditor dlgEditLightAnimation;
  dlgEditLightAnimation.DoModal();
}

// character matrix
static char achrToolTip[ 256*82+1];

void CMainFrame::ManualToolTipOn( PIX pixManualX, PIX pixManualY)
{
  CCustomToolTip &ctt = theApp.m_cttToolTips;
  ctt.cct_pCallback( ctt.cct_pThis, achrToolTip);
  //ASSERT( CTString(achrToolTip) != "");
  if( CTString(achrToolTip) == "") return;

  m_pwndToolTip = new CToolTipWnd;
  m_pwndToolTip->m_strText = achrToolTip;
  m_pwndToolTip->m_bManualControl = TRUE;
  m_pwndToolTip->m_pixManualX = pixManualX;
  m_pwndToolTip->m_pixManualY = pixManualY;

  const wchar_t *strWindowClass = AfxRegisterWndClass( CS_OWNDC|CS_NOCLOSE);
  if( !m_pwndToolTip->CreateEx( WS_EX_TOPMOST, strWindowClass, L"Tool tip",
      WS_BORDER|WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, m_hWnd, 0))
  {
    // program must never reach this point
    ASSERTALWAYS( "World Editor was unable to open tool tip window");
  }
}

void CMainFrame::ManualToolTipUpdate( void)
{
  CCustomToolTip &ctt = theApp.m_cttToolTips;
  ctt.cct_pCallback( ctt.cct_pThis, achrToolTip);
  ASSERT( CTString(achrToolTip) != "");
  if( CTString(achrToolTip) == "") return;
  
  if( m_pwndToolTip == NULL) return;

  m_pwndToolTip->m_strText = achrToolTip;
  m_pwndToolTip->ManualUpdate();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
  POINT ptMouse;
  GetCursorPos( &ptMouse);
  HWND hwndUnderMouse = ::WindowFromPoint( ptMouse);
  HWND hwndParent = ::GetParent( hwndUnderMouse);
  CCustomToolTip &ctt = theApp.m_cttToolTips;

  // if tool tip happened
  if( (nIDEvent == 0) && (m_pwndToolTip == NULL) )
  {
    if( hwndParent == ctt.cct_hwndCaller)
    {
      // if game is on, disable tool tips
      if( _pInput->IsInputEnabled()) return;

      ctt.cct_pCallback( ctt.cct_pThis, achrToolTip);

      if( CTString(achrToolTip) == "")
      {
        KillTimer( 0);
        return;
      }

      m_pwndToolTip = new CToolTipWnd;
      m_pwndToolTip->m_strText = achrToolTip;
      m_pwndToolTip->m_bManualControl = FALSE;

      const wchar_t *strWindowClass = AfxRegisterWndClass( CS_OWNDC|CS_NOCLOSE);
      if( !m_pwndToolTip->CreateEx( WS_EX_TOPMOST, strWindowClass, L"Tool tip",
          WS_BORDER|WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, m_hWnd, 0))
      {
        // program must never reach this point
        ASSERTALWAYS( "World Editor was unable to open tool tip window");
      }
    }
    KillTimer( 0);
  }

	CMDIFrameWnd::OnTimer(nIDEvent);
}

LRESULT CMainFrame::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  if( message==WM_SYSCOMMAND)
  {
    switch( wParam & ~0x0F)
    {
    case SC_SCREENSAVE:
    case SC_MONITORPOWER:
      return 0;
    }
  }

	return CMDIFrameWnd::DefWindowProc(message, wParam, lParam);
}

void CMainFrame::OnHelpFinder() 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  // must not be null
  if( pDoc != NULL) 
  {
    // if entity mode
    if( pDoc->m_iMode == ENTITY_MODE)
    {
      // if only one entity selected
      if( pDoc->m_selEntitySelection.Count() == 1)
      {
        CEntity *pen = pDoc->m_selEntitySelection.GetFirstInSelection();  
        CTFileName fnecl = pen->GetClass()->GetName();
        theApp.DisplayHelp(fnecl, HH_DISPLAY_TOPIC, NULL);
        return;
      }
    }
  }
  theApp.DisplayHelp(CTFILENAME("Help\\SeriousEditorDefault.hlk"), HH_DISPLAY_TOPIC, NULL);
}

void CMainFrame::SetStatusBarMessage( CTString strMessage, INDEX iPane, FLOAT fTime)
{
  // obtain stop time
  m_wndStatusBar.SetPaneText( iPane, CString(strMessage), TRUE);
  FLOAT tmNow = _pTimer->GetHighPrecisionTimer().GetSeconds();
  theApp.m_tmStartStatusLineInfo=tmNow + fTime;
}
