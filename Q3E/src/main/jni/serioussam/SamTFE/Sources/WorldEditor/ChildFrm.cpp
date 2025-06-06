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

// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "ChildFrm.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern INDEX wed_bSaveTestGameFirstTime = TRUE;
ENGINE_API extern INDEX snd_iFormat;

extern ENGINE_API FLOAT _fPlayerFOVAdjuster;
extern ENGINE_API FLOAT _fWeaponFOVAdjuster;
extern ENGINE_API FLOAT _fArmorHeightAdjuster;
extern ENGINE_API FLOAT _fFragScorerHeightAdjuster;

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_COMMAND(ID_GRID_ON_OFF, OnGridOnOff)
	ON_UPDATE_COMMAND_UI(ID_GRID_ON_OFF, OnUpdateGridOnOff)
	ON_COMMAND(ID_TEST_GAME, OnTestGameWindowed)
	ON_COMMAND(ID_TEST_GAME_FULLSCREEN, OnTestGameFullScreen)
	ON_UPDATE_COMMAND_UI(ID_TEST_GAME, OnUpdateTestGame)
	ON_COMMAND(ID_RENDER_TARGETS, OnRenderTargets)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TARGETS, OnUpdateRenderTargets)
	ON_COMMAND(ID_MOVE_ANCHORED, OnMoveAnchored)
	ON_UPDATE_COMMAND_UI(ID_MOVE_ANCHORED, OnUpdateMoveAnchored)
	ON_WM_TIMER()
	ON_COMMAND(ID_SCENE_RENDERING_TIME, OnSceneRenderingTime)
	ON_UPDATE_COMMAND_UI(ID_SCENE_RENDERING_TIME, OnUpdateSceneRenderingTime)
	ON_COMMAND(ID_AUTO_MIP_LEVELING, OnAutoMipLeveling)
	ON_UPDATE_COMMAND_UI(ID_AUTO_MIP_LEVELING, OnUpdateAutoMipLeveling)
	ON_COMMAND(ID_WINDOW_CLOSE, OnWindowClose)
	ON_COMMAND(ID_VIEW_SELECTION, OnViewSelection)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SELECTION, OnUpdateViewSelection)
	ON_COMMAND(ID_MAXIMIZE_VIEW, OnMaximizeView)
	ON_COMMAND(ID_TOGGLE_VIEW_PICTURES, OnToggleViewPictures)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_VIEW_PICTURES, OnUpdateToggleViewPictures)
	ON_COMMAND(ID_VIEW_FROM_ENTITY, OnViewFromEntity)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FROM_ENTITY, OnUpdateViewFromEntity)
	ON_COMMAND(ID_VIEW_SHADOWS_ONOFF, OnViewShadowsOnoff)
	ON_COMMAND(ID_CALCULATE_SHADOWS_ONOFF, OnCalculateShadowsOnoff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHADOWS_ONOFF, OnUpdateViewShadowsOnoff)
	ON_UPDATE_COMMAND_UI(ID_CALCULATE_SHADOWS_ONOFF, OnUpdateCalculateShadowsOnoff)
	ON_COMMAND(ID_STORE_POSITION01, OnStorePosition01)
	ON_COMMAND(ID_STORE_POSITION02, OnStorePosition02)
	ON_COMMAND(ID_RESTORE_POSITION01, OnRestorePosition01)
	ON_COMMAND(ID_RESTORE_POSITION02, OnRestorePosition02)
	ON_COMMAND(ID_TOGGLE_ENTITY_NAMES, OnToggleEntityNames)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_ENTITY_NAMES, OnUpdateToggleEntityNames)
	ON_COMMAND(ID_TOGGLE_VISIBILITY_TWEAKS, OnToggleVisibilityTweaks)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_VISIBILITY_TWEAKS, OnUpdateToggleVisibilityTweaks)
	ON_COMMAND(ID_ENABLE_VISIBILITY_TWEAKS, OnEnableVisibilityTweaks)
	ON_UPDATE_COMMAND_UI(ID_ENABLE_VISIBILITY_TWEAKS, OnUpdateEnableVisibilityTweaks)
	ON_COMMAND(ID_RESTORE_POSITION03, OnRestorePosition03)
	ON_COMMAND(ID_RESTORE_POSITION04, OnRestorePosition04)
	ON_COMMAND(ID_STORE_POSITION03, OnStorePosition03)
	ON_COMMAND(ID_STORE_POSITION04, OnStorePosition04)
	ON_COMMAND(ID_KEY_B, OnKeyB)
	ON_UPDATE_COMMAND_UI(ID_KEY_B, OnUpdateKeyB)
	ON_COMMAND(ID_KEY_G, OnKeyG)
	ON_UPDATE_COMMAND_UI(ID_KEY_G, OnUpdateKeyG)
	ON_COMMAND(ID_KEY_Y, OnKeyY)
	ON_UPDATE_COMMAND_UI(ID_KEY_Y, OnUpdateKeyY)
	ON_UPDATE_COMMAND_UI(ID_TEST_GAME_FULLSCREEN, OnUpdateTestGame)
	ON_COMMAND(ID_KEY_CTRL_G, OnKeyCtrlG)
	ON_UPDATE_COMMAND_UI(ID_KEY_CTRL_G, OnUpdateKeyCtrlG)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
  m_bShowVisibilityTweaks=FALSE;
  m_bDisableVisibilityTweaks=FALSE;
  m_bTestGameOn = FALSE;
  m_bShowTargets = FALSE;
  m_bShowEntityNames = FALSE;
  m_fManualMipBrushingFactor = 1.0f;
  m_bLastAutoMipBrushingOn = FALSE;
  m_bAutoMipBrushingOn = FALSE;
  m_iSelectedConfiguration = 0;
  m_bSceneRenderingTime = FALSE;
  m_bRenderViewPictures = FALSE;
  // don't allow moving of anchored entities
  m_bAncoredMovingAllowed = FALSE;
  if( theApp.m_Preferences.ap_bHideShadowsOnOpen)
  {
    m_stShadowType = CWorldRenderPrefs::SHT_NONE;
    m_bShadowsVisible = FALSE;
    m_bShadowsCalculate = TRUE;
  }
  else
  {
    m_stShadowType = CWorldRenderPrefs::SHT_FULL;
    m_bShadowsVisible = TRUE;
    m_bShadowsCalculate = TRUE;
  }

  m_iAnchoredResetTimerID = -1;
  m_bInfoVisible = 0;
  m_bSelectionVisible = TRUE;
  m_bViewFromEntity = FALSE;

  wo_plStored01 = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  wo_plStored02 = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  wo_plStored03 = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  wo_plStored04 = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  wo_fStored01 = 10.0f;
  wo_fStored02 = 10.0f;
  wo_fStored03 = 10.0f;
  wo_fStored04 = 10.0f;
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	return m_wndSplitter.Create( this,
		2, 2,                 // TODO: adjust the number of rows, columns
		CSize( 10, 10 ),      // TODO: adjust the minimum pane size
		pContext, WS_CHILD | WS_VISIBLE | SPLS_DYNAMIC_SPLIT);
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

  BOOL bMaximized = FALSE;
  CMDIFrameWnd *pFrame = (CMDIFrameWnd *)AfxGetApp()->m_pMainWnd;
  if( ((pFrame->MDIGetActive( &bMaximized) == NULL) || (bMaximized)) && 
      (theApp.m_Preferences.ap_AutoMaximizeWindow) )
  {
    cs.style |= WS_VISIBLE | WS_MAXIMIZE;
  }

	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

/*
 * Set new child's configuration
 */
void CChildFrame::RememberChildConfiguration( INDEX iViewConfiguration)
{
  INDEX i, j;
  // get client size
  CRect rectClient;
  GetClientRect( &rectClient);
  PIX pixClientWidth, pixClientHeight;
  pixClientWidth = rectClient.Width();
  pixClientHeight = rectClient.Height();

  // create reference to choosed view configuration
  CChildConfiguration &CC = theApp.m_ccChildConfigurations[ iViewConfiguration];
  // look for numbers of vertical splitter windows
  CC.m_iVerticalSplitters = m_wndSplitter.GetColumnCount();
  ASSERT( CC.m_iVerticalSplitters <= 2);
  // look for numbers of horizontal splitter windows
  CC.m_iHorizontalSplitters = m_wndSplitter.GetRowCount();
  ASSERT( CC.m_iHorizontalSplitters <= 2);
  // here we will receive splitter's sizes
  int iCurWidth, iCurHeight;
  // get width and height of splitter windows
  CWnd *pWnd = m_wndSplitter.GetPane( 0, 0);
  CRect rectLU;
  pWnd->GetClientRect( &rectLU);
  iCurWidth = rectLU.Width();
  iCurHeight = rectLU.Height();
  // if window is splitted vericaly
  if( CC.m_iVerticalSplitters != 1)
  {
    // calculate percentages of left window
    CC.m_fPercentageLeft = ((FLOAT)iCurWidth)/pixClientWidth;
  }
  // if window is splitted horizontaly
  if( CC.m_iHorizontalSplitters != 1)
  {
    // calculate percentages of top window
    CC.m_fPercentageTop = ((FLOAT)iCurHeight)/pixClientHeight;
  }
  // remember grid on/off flag
  CC.m_bGridOn = m_bGridOn;

  // remember rendering preferences of all views
  for( j=0; j<CC.m_iVerticalSplitters; j++)
  {
    for( i=0; i<CC.m_iHorizontalSplitters; i++)
    {
      CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( j, i);
      ASSERT( pWEDView != NULL);
      CC.m_vpViewPrefs[ j*2 + i] = pWEDView->m_vpViewPrefs;
      CC.m_ptProjectionType[ j*2 + i] = pWEDView->m_ptProjectionType;
    }
  }
}

void CChildFrame::ApplySettingsFromPerspectiveView( CWorldEditorView *pwedView, INDEX iViewConfiguration)
{
  CChildConfiguration &cc = theApp.m_ccChildConfigurations[ iViewConfiguration];
  // type of projection
  for( INDEX iView=0; iView<cc.m_iHorizontalSplitters*cc.m_iHorizontalSplitters; iView++)
  {
    if( cc.m_ptProjectionType[ iView] == CSlaveViewer::PT_PERSPECTIVE)
    {
      pwedView->m_vpViewPrefs = cc.m_vpViewPrefs[ iView];
    }
  }
}

/*
 * Set new child's configuration
 */
void CChildFrame::SetChildConfiguration( INDEX iViewConfiguration)
{
  INDEX i, j;
  // get client size
  CRect rectClient;
  GetClientRect( &rectClient);
  PIX pixClientWidth, pixClientHeight;
  pixClientWidth = rectClient.Width();
  pixClientHeight = rectClient.Height();

  CChildConfiguration &CC = theApp.m_ccChildConfigurations[ iViewConfiguration];
  m_iSelectedConfiguration = iViewConfiguration;

  // delete all possible columns
  for( i=1; i<m_wndSplitter.GetColumnCount(); i++)
  {
    m_wndSplitter.DeleteColumn( 0);
  }
  // delete all possible rows
  for( i=1; i<m_wndSplitter.GetRowCount(); i++)
  {
    m_wndSplitter.DeleteRow( 0);
  }

  // restore grid on/off flag
  m_bGridOn = CC.m_bGridOn;

  // here we will prepare splitter's size
  int iCurWidth, iCurHeight;
  // if window is splitted vericaly
  if( CC.m_iVerticalSplitters != 1)
  {
    // add new splitter with position calculated using remembered percentage
    iCurWidth = (int) (pixClientWidth * CC.m_fPercentageLeft) + 2;
    m_wndSplitter.SplitColumn( iCurWidth);
  }
  // if window is splitted horizontaly
  if( CC.m_iHorizontalSplitters != 1)
  {
    // add new splitter with position calculated using remembered percentage
    iCurHeight = (int) (pixClientHeight * CC.m_fPercentageTop) + 3;
    m_wndSplitter.SplitRow( iCurHeight);
  }
  // restore rendering preferences of all views
  for( j=0; j<CC.m_iVerticalSplitters; j++)
  {
    for( i=0; i<CC.m_iHorizontalSplitters; i++)
    {
      CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( j, i);
      ASSERT( pWEDView != NULL);
      pWEDView->m_vpViewPrefs = CC.m_vpViewPrefs[ j*2 + i];
      pWEDView->m_ptProjectionType = CC.m_ptProjectionType[ j*2 + i];
			// resize it
      pWEDView->m_pvpViewPort->Resize();
    }
  }
  
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  // redraw all views, child configuration changed
  pWEDView->GetDocument()->UpdateAllViews( NULL);
  RecalcLayout();
}

void CChildFrame::KeyPressed(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  // first set invalid view configuration buffer number
  INDEX iViewConfiguration = -1;

  // if control pressed
  BOOL bCtrl = (GetKeyState( VK_CONTROL) & 128) != 0;
  // look for wanted keys on numeric part of keyboard and extract buffer number
  if( nFlags == 0x52)  iViewConfiguration = 0;
  if( nFlags == 0x4f)  iViewConfiguration = 1;
  if( nFlags == 0x50)  iViewConfiguration = 2;
  if( nFlags == 0x51)  iViewConfiguration = 3;
  if( nFlags == 0x4b)  iViewConfiguration = 4;
  if( nFlags == 0x4c)  iViewConfiguration = 5;
  if( nFlags == 0x4d)  iViewConfiguration = 6;
  if( nFlags == 0x47)  iViewConfiguration = 7;
  if( nFlags == 0x48)  iViewConfiguration = 8;
  if( nFlags == 0x49)  iViewConfiguration = 9;
  // if any of view configuration buffers requested
  if( iViewConfiguration != -1)
  {
    if( bCtrl)
    {
      // edit child's configuration
      RememberChildConfiguration( iViewConfiguration);
    }
    else
    {
      // set new child's configuration
      SetChildConfiguration( iViewConfiguration);
    }
  }
}

void CChildFrame::ActivateFrame(int nCmdShow) 
{
	CMDIChildWnd::ActivateFrame(nCmdShow);

  // set default child's configuration
  SetChildConfiguration( theApp.m_Preferences.ap_iStartupWindowSetup);
}

void CChildFrame::OnGridOnOff() 
{
  m_bGridOn = !m_bGridOn;
  Invalidate( FALSE);
}

void CChildFrame::OnUpdateGridOnOff(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bGridOn);
}


CWorldEditorView *CChildFrame::GetPerspectiveView(void)
{
  CWorldEditorView *pPerspectiveView = NULL;
  // create reference to choosed view configuration
  CChildConfiguration &CC = theApp.m_ccChildConfigurations[ m_iSelectedConfiguration];
  // find perspective view
  for( INDEX iVerticalSpliter=0; iVerticalSpliter<GetVSplitters(); iVerticalSpliter++)
  {
    for( INDEX iHorizontalSpliter=0; iHorizontalSpliter<GetHSplitters(); iHorizontalSpliter++)
    {
      CWorldEditorView *pWEDView = (CWorldEditorView *) 
        m_wndSplitter.GetPane( iVerticalSpliter, iHorizontalSpliter);
      ASSERT( pWEDView != NULL);
      if( pWEDView->m_ptProjectionType == CSlaveViewer::PT_PERSPECTIVE)
      {
        pPerspectiveView = pWEDView;
        break;
      }
    }
  }
  return pPerspectiveView;
}



// test game routines

#define APPLICATION_NAME "TestGame FullScreen"

void CChildFrame::SetAdjusters(float ratio)
{
	//if (pdp == NULL) return;
	//float ratio = (float)pdp->GetWidth() / (float)pdp->GetHeight();
 	if (ratio >= 1.30f && ratio < 1.43f) 		//4:3
 	{
		_fWeaponFOVAdjuster			= 1.0f;		//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.0f;		//Field of View for player
		_fArmorHeightAdjuster		= 0.7f;
		_fFragScorerHeightAdjuster	= 0.75f;
	}else if (ratio >= 1.2f && ratio < 1.30f) 	//5:4
 	{
		_fWeaponFOVAdjuster			= 1.0f;		//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.0f;		//Field of View for player
		_fArmorHeightAdjuster		= 0.7f;
		_fFragScorerHeightAdjuster	= 0.75f;
 	}else if (ratio >= 1.73f && ratio < 1.8f)	//16:9
 	{
		_fWeaponFOVAdjuster			= 1.25f;	//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.15f;	//Field of View for player
		_fArmorHeightAdjuster		= 0.835f;
		_fFragScorerHeightAdjuster	= 1.5f;
 	}else if (ratio >= 1.43f && ratio < 1.73f) 	//16:10
 	{
		_fWeaponFOVAdjuster			= 1.15f;	//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.10f;	//Field of View for player
		_fArmorHeightAdjuster		= 0.78f;
		_fFragScorerHeightAdjuster	= 1.17f;
 	}else if (ratio >= 1.8f && ratio <= 4.2f) 	//21:9
 	{
		_fWeaponFOVAdjuster			= 1.55f;	//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.35f;	//Field of View for player
		_fArmorHeightAdjuster	    = 1.0f;
		_fFragScorerHeightAdjuster	= 2.35f;
	}
	_pShell->SetFLOAT("_fWeaponFOVAdjuster", _fWeaponFOVAdjuster);
	_pShell->SetFLOAT("_fPlayerFOVAdjuster", _fPlayerFOVAdjuster);
	_pShell->SetFLOAT("_fArmorHeightAdjuster", _fArmorHeightAdjuster);
	_pShell->SetFLOAT("_fFragScorerHeightAdjuster", _fFragScorerHeightAdjuster);
	/* CPrintF("[WorlEditor] _fWeaponFOVAdjuster: %f  _fPlayerFOVAdjuster: %f  _fArmorHeightAdjuster: %f  _fFragScorerHeightAdjuster: %f\n",
	  _fWeaponFOVAdjuster,_fPlayerFOVAdjuster, _fArmorHeightAdjuster,_fFragScorerHeightAdjuster ); */ // For Debug
}

void CChildFrame::TestGame( BOOL bFullScreen) 
{
  // turn off info window
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->HideInfoWindow();

  CWorldEditorView *pPerspectiveView = GetPerspectiveView();
  ASSERT( pPerspectiveView != NULL);
  SetActiveView( pPerspectiveView, FALSE);
	CWorldEditorDoc* pDoc = pPerspectiveView->GetDocument();

  CTFileName fnmWorldToPlay;
  CTFileName fnmTempWorld = CTString("Temp\\TestGame.wld");

  // if the world was never saved or if it is modified
  if( (!pDoc->m_bWasEverSaved && wed_bSaveTestGameFirstTime) || pDoc->IsModified())
  { // save world under temporary name
    ASSERT_VALID(pDoc);
    try {  
      pDoc->m_woWorld.Save_t(fnmTempWorld);
      fnmWorldToPlay = fnmTempWorld;
    } catch (const char *strError) {
      AfxMessageBox( CString(strError));
      pPerspectiveView->EnableToolTips(TRUE);
      return;
    }
  // if the world is not modified (it is saved on disk)
  } else
  { // use the saved world
    fnmWorldToPlay = CTString(CStringA(pDoc->GetPathName()));
    try {
      fnmWorldToPlay.RemoveApplicationPath_t();
    } catch (const char *strError) {
      AfxMessageBox( CString(strError));
      pPerspectiveView->EnableToolTips(TRUE);
      return;
    }
  }

  // set rendering preferences
  _wrpWorldRenderPrefs = pPerspectiveView->m_vpViewPrefs.m_wrpWorldRenderPrefs;
  _mrpModelRenderPrefs = pPerspectiveView->m_vpViewPrefs.m_mrpModelRenderPrefs;
  _wrpWorldRenderPrefs.SetShadowsType( pPerspectiveView->GetChildFrame()->m_stShadowType);
  _wrpWorldRenderPrefs.SetSelectedEntityModel( theApp.m_pEntityMarkerModelObject);
  _wrpWorldRenderPrefs.SetSelectedPortalModel( theApp.m_pPortalMarkerModelObject);
  _wrpWorldRenderPrefs.SetEmptyBrushModel( theApp.m_pEmptyBrushModelObject);
  _wrpWorldRenderPrefs.SetTextureLayerOn( theApp.m_bTexture1, 0);
  _wrpWorldRenderPrefs.SetTextureLayerOn( theApp.m_bTexture2, 1);
  _wrpWorldRenderPrefs.SetTextureLayerOn( theApp.m_bTexture3, 2);
  _wrpWorldRenderPrefs.DisableVisTweaks(FALSE);

  _wrpWorldRenderPrefs.SetSelectedEntityModel( theApp.m_pEntityMarkerModelObject);
  _wrpWorldRenderPrefs.SetSelectedPortalModel( theApp.m_pPortalMarkerModelObject);
  _wrpWorldRenderPrefs.SetEmptyBrushModel( theApp.m_pEmptyBrushModelObject);

  // prepare test game view/draw ports
  CViewPort *pvp = pPerspectiveView->m_pvpViewPort;
  CDrawPort *pdp = pPerspectiveView->m_pdpDrawPort;
  pdp->SetOverlappedRendering(FALSE); // we are not rendering scene over already rendered scene (used for CSG layer)

  // if full screen mode is required
  HWND hWndFullScreen=NULL;
  HINSTANCE hInstanceFullScreen;
  WNDCLASSEX wcFullScreen;
  char achWindowTitle[256]; // current window title

  // Set Adjusters for HUD and FOV
  FLOAT ratio;
  if (bFullScreen) {
	ratio = (FLOAT)theApp.m_dmFullScreen.dm_pixSizeI / (FLOAT)theApp.m_dmFullScreen.dm_pixSizeJ;
  } else {
	ratio = (FLOAT)pdp->GetWidth() / (FLOAT)pdp->GetHeight();
  }
  /*
  CPrintF("[WorlEditor] pdp->GetWidth: %d pdp->GetHeight: %d  Ratio: %f\n",pdp->GetWidth(), pdp->GetHeight(), ratio);
  */ // For Debug
  SetAdjusters(ratio);

  if( bFullScreen) 
  {
    // get full screen display mode info
    const PIX pixSizeI = theApp.m_dmFullScreen.dm_pixSizeI;
    const PIX pixSizeJ = theApp.m_dmFullScreen.dm_pixSizeJ;
    const DisplayDepth dd = theApp.m_dmFullScreen.dm_ddDepth;
    const GfxAPIType gat  = theApp.m_gatFullScreen;
    // set OpenGL fullscreen (before window)
    if( gat==GAT_OGL) {
      const BOOL bRes = _pGfx->SetDisplayMode( gat, 0, pixSizeI, pixSizeJ, dd);
      if( !bRes) {
        WarningMessage( "Unable to setup full screen display.");
        return;
      }
    } // register the window class
    hInstanceFullScreen = AfxGetInstanceHandle();
    ASSERT( hInstanceFullScreen!=NULL);
    wcFullScreen.cbSize = sizeof(wcFullScreen);
    wcFullScreen.style = CS_HREDRAW | CS_VREDRAW;
    wcFullScreen.lpfnWndProc = ::DefWindowProc;
    wcFullScreen.cbClsExtra = 0;
    wcFullScreen.cbWndExtra = 0;
    wcFullScreen.hInstance = hInstanceFullScreen;
    wcFullScreen.hIcon = LoadIcon( hInstanceFullScreen, (LPCTSTR)IDR_MAINFRAME );
    wcFullScreen.hCursor = NULL;
    wcFullScreen.hbrBackground = NULL;
    wcFullScreen.lpszMenuName  = CString(APPLICATION_NAME);
    wcFullScreen.lpszClassName = CString(APPLICATION_NAME);
    wcFullScreen.hIconSm = NULL;
    RegisterClassEx(&wcFullScreen);

    // create a window, invisible initially
    hWndFullScreen = CreateWindowExA(
      WS_EX_TOPMOST,
      APPLICATION_NAME,
      "Serious Editor - Full Screen Test Game",   // title
      WS_POPUP,
      0,0,
      pixSizeI, pixSizeJ,  // window size
      NULL,
      NULL,
      hInstanceFullScreen,
      NULL);
    // didn't make it?
    ASSERT( hWndFullScreen!=NULL);
    if( hWndFullScreen==NULL) {
      if( gat==GAT_OGL) _pGfx->ResetDisplayMode( (enum GfxAPIType)theApp.m_iApi);
      WarningMessage( "Unable to setup window for full screen display.");
      return;
    }
    // set windows for engine
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    SE_UpdateWindowHandle( hWndFullScreen);

    // set window title and show it
    sprintf( achWindowTitle, "Serious Editor - Test Game (FullScreen %dx%d)", pixSizeI, pixSizeJ);
    ::SetWindowTextA( hWndFullScreen, achWindowTitle);
    ::ShowWindow(    hWndFullScreen, SW_SHOWNORMAL);

    // set Direct3D full screen (after window)
#ifdef SE1_D3D
    if( gat==GAT_D3D) {
      const BOOL bRes = _pGfx->SetDisplayMode( gat, 0, pixSizeI, pixSizeJ, dd);
      if( !bRes) {
        WarningMessage( "Unable to setup full screen display.");
        ::DestroyWindow( hWndFullScreen);
        SE_UpdateWindowHandle( pMainFrame->m_hWnd);
        return;
      }
    }
#endif // SE1_D3D
    // create canvas
    _pGfx->CreateWindowCanvas( hWndFullScreen, &pvp, &pdp);
    // initial screen fill and swap, just to get context running
    BOOL bSuccess = FALSE;
    if( pdp!=NULL && pdp->Lock()) {
      pdp->Fill(C_dGREEN|CT_OPAQUE);
      pdp->Unlock();
      pvp->SwapBuffers();
      bSuccess = TRUE;
    }
    // must succeed!
    ASSERT( bSuccess);
    if( !bSuccess) {
      _pGfx->ResetDisplayMode( (enum GfxAPIType)theApp.m_iApi);
      WarningMessage( "Unable to setup canvas for full screen display.");
      return;
    }
  }

  // enable sound
  snd_iFormat = Clamp( snd_iFormat, (INDEX)CSoundLibrary::SF_NONE, (INDEX)CSoundLibrary::SF_44100_16);
  _pSound->SetFormat( (enum CSoundLibrary::SoundFormat)snd_iFormat, TRUE);

  // run quick test game
  extern BOOL _bInOnDraw; 
  _bInOnDraw = TRUE;
  _pGameGUI->QuickTest( fnmWorldToPlay, pdp, pvp);
  _bInOnDraw = FALSE;

  // disable sound
  _pSound->SetFormat( CSoundLibrary::SF_NONE);

  // restore default display mode and close test full screen window
  if( hWndFullScreen!=NULL) {
    _pGfx->ResetDisplayMode( (enum GfxAPIType)theApp.m_iApi);
    ::DestroyWindow( hWndFullScreen);
    SE_UpdateWindowHandle( pMainFrame->m_hWnd);
  }
  // redraw all views
  pDoc->UpdateAllViews( NULL);
  pPerspectiveView->EnableToolTips( TRUE);
}


void CChildFrame::OnTestGameWindowed() 
{
  m_bTestGameOn = TRUE;
  // run test game in perspective window
  TestGame( FALSE);
  m_bTestGameOn = FALSE;
}

void CChildFrame::OnTestGameFullScreen() 
{
  m_bTestGameOn = TRUE;
  // run test game in full screen
  TestGame( TRUE);
  m_bTestGameOn = FALSE;
}


void CChildFrame::OnUpdateTestGame(CCmdUI* pCmdUI) 
{
  CWorldEditorView *pPerspectiveView = GetPerspectiveView();
  pCmdUI->Enable( pPerspectiveView != NULL);
}


void CChildFrame::OnKeyG() 
{
  OnRenderTargets();
}

void CChildFrame::OnUpdateKeyG(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( TRUE);
}

void CChildFrame::OnRenderTargets() 
{
  m_bShowTargets = !m_bShowTargets;
  Invalidate( FALSE);
}


void CChildFrame::OnUpdateRenderTargets(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bShowTargets);
}


void CChildFrame::DeleteViewsExcept( CWnd *pwndViewToLeave)
{
  // delete all rows
  INDEX iRowToDelete = 0;
  while( iRowToDelete<GetVSplitters())
  {
    BOOL bCanDeleteRow = TRUE;
    // for all collumns
    for( INDEX iColumn=0; iColumn<GetHSplitters(); iColumn++)
    {
      if( m_wndSplitter.GetPane( iRowToDelete, iColumn) == pwndViewToLeave)
      {
        bCanDeleteRow = FALSE;
        break;
      }
    }
    if( bCanDeleteRow)
    {
      m_wndSplitter.DeleteRow( iRowToDelete);
    }
    else
    {
      iRowToDelete++;
    }
  }

  // delete all columns
  INDEX iColumnToDelete = 0;
  while( iColumnToDelete<GetHSplitters())
  {
    BOOL bCanDeleteColumn = TRUE;
    // for all collumns
    for( INDEX iRow=0; iRow<GetVSplitters(); iRow++)
    {
      if( m_wndSplitter.GetPane( iRow, iColumnToDelete) == pwndViewToLeave)
      {
        bCanDeleteColumn = FALSE;
        break;
      }
    }
    if( bCanDeleteColumn)
    {
      m_wndSplitter.DeleteColumn( iColumnToDelete);
    }
    else
    {
      iColumnToDelete++;
    }
  }

  // activate view that we want to leave
  SetActiveView( (CView*)pwndViewToLeave, FALSE);
}


void CChildFrame::OnSceneRenderingTime() 
{
  m_bSceneRenderingTime = !m_bSceneRenderingTime;
  Invalidate( FALSE);
}

void CChildFrame::OnUpdateSceneRenderingTime(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bSceneRenderingTime);
}

void CChildFrame::OnMoveAnchored() 
{
  m_bAncoredMovingAllowed	= !m_bAncoredMovingAllowed;
  m_iAnchoredResetTimerID = SetTimer( 1, 60000*5, NULL);
}

void CChildFrame::OnUpdateMoveAnchored(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bAncoredMovingAllowed);
}

void CChildFrame::OnTimer(UINT_PTR nIDEvent) 
{
  // if anchored reset happend
  if( nIDEvent == m_iAnchoredResetTimerID)
  {
    m_bAncoredMovingAllowed = FALSE;
    // refresh "enable anchored moving" tool button
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->RecalcLayout();
    KillTimer( m_iAnchoredResetTimerID);
  }
    
	CMDIChildWnd::OnTimer(nIDEvent);
}



void CChildFrame::OnAutoMipLeveling() 
{
  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();
  // get view
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  m_bAutoMipBrushingOn = !m_bAutoMipBrushingOn;
  pWEDView->SetEditingDataPaneInfo( TRUE);
  Invalidate( FALSE);
}

void CChildFrame::OnUpdateAutoMipLeveling(CCmdUI* pCmdUI) 
{
  if( theApp.GetDocument()->GetEditingMode() == CSG_MODE)
  {
    pCmdUI->Enable( FALSE);
  }
  pCmdUI->SetCheck( m_bAutoMipBrushingOn);
}

void CChildFrame::OnWindowClose() 
{
  OnClose();
}

void CChildFrame::OnViewSelection() 
{
  // get document
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
	m_bSelectionVisible = !m_bSelectionVisible;
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnUpdateViewSelection(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bSelectionVisible);
}

void CChildFrame::OnToggleVisibilityTweaks() 
{
  // get document
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
	m_bShowVisibilityTweaks = !m_bShowVisibilityTweaks;
	// auto turn off disabling of visibility tweaks
  m_bDisableVisibilityTweaks = FALSE;
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnUpdateToggleVisibilityTweaks(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bShowVisibilityTweaks);
}

void CChildFrame::OnMaximizeView() 
{
  // look for numbers of horizontal splitter windows
  if( (m_wndSplitter.GetRowCount()+m_wndSplitter.GetColumnCount() ) == 4)
  {
    // remember child configuration into temporary buffer
    RememberChildConfiguration( CHILD_CONFIGURATIONS_CT+1);

    // get active view
    CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
    // delete all other views
    DeleteViewsExcept( pWorldEditorView);
  }
  else
  {
    // restore child configuration
    SetChildConfiguration( CHILD_CONFIGURATIONS_CT+1);
  }
}

void CChildFrame::OnToggleViewPictures() 
{
  m_bRenderViewPictures = !m_bRenderViewPictures;
  Invalidate( FALSE);
}

void CChildFrame::OnUpdateToggleViewPictures(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bRenderViewPictures);
}

void CChildFrame::OnViewFromEntity() 
{
  m_bViewFromEntity = !m_bViewFromEntity;
  Invalidate( FALSE);
}

void CChildFrame::OnUpdateViewFromEntity(CCmdUI* pCmdUI) 
{
  /*
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  pCmdUI->Enable( 
    (pDoc != NULL) &&
    (pDoc->GetEditingMode() == ENTITY_MODE) && 
    (pDoc->m_selEntitySelection.Count() == 1) &&
    GetPerspectiveView() != NULL);
    */
  pCmdUI->SetCheck( m_bViewFromEntity);
}

#define APPLY_SHADOW_TYPE( stNewShadowType) \
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);\
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();\
	ASSERT_VALID(pDoc);\
  m_stShadowType = stNewShadowType;\
  pDoc->UpdateAllViews( NULL);\

void CChildFrame::OnViewShadowsOnoff() 
{
  CWorldRenderPrefs::ShadowsType stNew;
  m_bShadowsVisible = !m_bShadowsVisible;
  // if shadows are now visible
  if( m_bShadowsVisible)
  {
    // if shadows are calculating
    if( m_bShadowsCalculate)
    {
      stNew = CWorldRenderPrefs::SHT_FULL;
    }
    // if shadows are not calculating
    else
    {
      stNew = CWorldRenderPrefs::SHT_NOAUTOCALCULATE;
    }
  }
  // if shadows should become visible
  else
  {
    stNew = CWorldRenderPrefs::SHT_NONE;
  }
  APPLY_SHADOW_TYPE( stNew);
}

void CChildFrame::OnUpdateViewShadowsOnoff(CCmdUI* pCmdUI) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  UINT nIDView, nStyleView;
  int iViewImage;
  pMainFrame->m_wndShadowsAndTexture.GetButtonInfo( 3, nIDView, nStyleView, iViewImage);
  // if shadows are visible
  if( m_bShadowsVisible)
  {
    pMainFrame->m_wndShadowsAndTexture.SetButtonInfo( 3, nIDView, nStyleView, 3);
    pCmdUI->SetCheck( TRUE);
  }
  // if shadows are not visible
  else
  {
    pMainFrame->m_wndShadowsAndTexture.SetButtonInfo( 3, nIDView, nStyleView, 5);
    pCmdUI->SetCheck( FALSE);
  }
}

void CChildFrame::OnCalculateShadowsOnoff() 
{
  CWorldRenderPrefs::ShadowsType stNew;
  m_bShadowsCalculate = !m_bShadowsCalculate;
  // if shadows should now calculate
  if( m_bShadowsCalculate)
  {
    // if shadows are visible
    if( m_bShadowsVisible)
    {
      stNew = CWorldRenderPrefs::SHT_FULL;
    }
    // if shadows are not visible
    else
    {
      stNew = CWorldRenderPrefs::SHT_NONE;
    }
  }
  // if shadows should stop calculating
  else
  {
    // if shadows are visible
    if( m_bShadowsVisible)
    {
      stNew = CWorldRenderPrefs::SHT_NOAUTOCALCULATE;
    }
    // if shadows are not visible
    else
    {
      stNew = CWorldRenderPrefs::SHT_NONE;
    }
  }
  APPLY_SHADOW_TYPE( stNew);
}

void CChildFrame::OnUpdateCalculateShadowsOnoff(CCmdUI* pCmdUI) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  UINT nIDCalculate, nStyleCalculate;
  int iCalculateImage;
  pMainFrame->m_wndShadowsAndTexture.GetButtonInfo( 4, nIDCalculate, nStyleCalculate, iCalculateImage);
  // if shadows are calculating
  if( m_bShadowsCalculate)
  {
    // if shadows are visible
    if( m_bShadowsVisible)
    {
      pMainFrame->m_wndShadowsAndTexture.SetButtonInfo( 4, nIDCalculate, nStyleCalculate, 4);
    }
    // if shadows are not visible
    else
    {
      pMainFrame->m_wndShadowsAndTexture.SetButtonInfo( 4, nIDCalculate, nStyleCalculate, 7);
    }
    pCmdUI->SetCheck( TRUE);
  }
  // if shadows are not visible
  else
  {
    // if shadows are visible
    if( m_bShadowsVisible)
    {
      pMainFrame->m_wndShadowsAndTexture.SetButtonInfo( 4, nIDCalculate, nStyleCalculate, 5);
    }
    // if shadows are not visible
    else
    {
      pMainFrame->m_wndShadowsAndTexture.SetButtonInfo( 4, nIDCalculate, nStyleCalculate, 8);
    }
    pCmdUI->SetCheck( FALSE);
  }
}

void CChildFrame::OnStorePosition01() 
{
  wo_plStored01 = m_mvViewer.mv_plViewer;
  wo_fStored01 = m_mvViewer.mv_fTargetDistance;
}

void CChildFrame::OnStorePosition02() 
{
  wo_plStored02 = m_mvViewer.mv_plViewer;
  wo_fStored02 = m_mvViewer.mv_fTargetDistance;
}

void CChildFrame::OnStorePosition03() 
{
  wo_plStored03 = m_mvViewer.mv_plViewer;
  wo_fStored03 = m_mvViewer.mv_fTargetDistance;
}

void CChildFrame::OnStorePosition04() 
{
  wo_plStored04 = m_mvViewer.mv_plViewer;
  wo_fStored04 = m_mvViewer.mv_fTargetDistance;
}

void CChildFrame::OnRestorePosition01() 
{
  m_mvViewer.mv_plViewer = wo_plStored01;
  m_mvViewer.mv_fTargetDistance = wo_fStored01;

  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnRestorePosition02() 
{
  m_mvViewer.mv_plViewer = wo_plStored02;
  m_mvViewer.mv_fTargetDistance = wo_fStored02;
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);

  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnRestorePosition03() 
{
  m_mvViewer.mv_plViewer = wo_plStored03;
  m_mvViewer.mv_fTargetDistance = wo_fStored03;

  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnRestorePosition04() 
{
  m_mvViewer.mv_plViewer = wo_plStored04;
  m_mvViewer.mv_fTargetDistance = wo_fStored04;

  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnKeyCtrlG() 
{
  if( theApp.GetDocument()->GetEditingMode() == TERRAIN_MODE)
  {
    GenerateLayerDistribution(-1);
  }
  else
  {
    OnToggleEntityNames();
  }
}

void CChildFrame::OnUpdateKeyCtrlG(CCmdUI* pCmdUI) 
{
  if( theApp.GetDocument()->GetEditingMode() == TERRAIN_MODE)
  {
    pCmdUI->SetCheck(TRUE);
  }
  else
  {
    OnUpdateToggleEntityNames(pCmdUI);
  }
}

void CChildFrame::OnToggleEntityNames() 
{
  m_bShowEntityNames = !m_bShowEntityNames;
  Invalidate( FALSE);
}

void CChildFrame::OnUpdateToggleEntityNames(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bShowEntityNames);
}

void CChildFrame::OnEnableVisibilityTweaks() 
{
  // get document
  CWorldEditorView *pWEDView = (CWorldEditorView *) m_wndSplitter.GetPane( 0, 0);
  ASSERT( pWEDView != NULL);
	CWorldEditorDoc* pDoc = pWEDView->GetDocument();
	ASSERT_VALID(pDoc);
	m_bDisableVisibilityTweaks = !m_bDisableVisibilityTweaks;
  pDoc->UpdateAllViews( NULL);
}

void CChildFrame::OnUpdateEnableVisibilityTweaks(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bDisableVisibilityTweaks);
}


void CChildFrame::OnKeyB() 
{
  if( theApp.GetDocument()->GetEditingMode() == TERRAIN_MODE)
  {
    INDEX iNewMode=(INDEX(theApp.m_iTerrainBrushMode)+1)%CT_BRUSH_MODES;
    theApp.m_iTerrainBrushMode=iNewMode;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    theApp.GetDocument()->SetStatusLineModeInfoMessage();
  }
  else
  {
    OnAutoMipLeveling();
  }
}

void CChildFrame::OnUpdateKeyB(CCmdUI* pCmdUI) 
{
  if( theApp.GetDocument()->GetEditingMode() == TERRAIN_MODE)
  {
    pCmdUI->SetCheck(TRUE);
  }
  else
  {
    OnUpdateAutoMipLeveling(pCmdUI);
  }
}

void CChildFrame::OnKeyY() 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_CONTINOUS_NOISE;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
    return;
  }
  else
  {
    OnViewFromEntity();
  }
}

void CChildFrame::OnUpdateKeyY(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    pCmdUI->SetCheck(TRUE);
  }
  else
  {
    OnUpdateViewFromEntity(pCmdUI);
  }
}

