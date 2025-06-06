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

// DlgSelectMode.cpp : implementation file
//

#include "EngineGui/StdH.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// internal routines for displaying test screen 

static void DrawGradient( CDrawPort *pDP, COLOR colStart, COLOR colEnd,
                          PIX pixI0, PIX pixJ0, PIX pixI1, PIX pixJ1)
{
  colStart |= CT_OPAQUE;
  colEnd   |= CT_OPAQUE;
  pDP->Fill( pixI0, pixJ0, pixI1-pixI0, pixJ1-pixJ0, colStart, colEnd, colStart, colEnd);
}


static void ShowTestModeScreen( CDrawPort *pDP, CViewPort *pVP)
{
  // try to lock draw port
  if( !pDP->Lock()) return;

  // draw rectangle
  PIX dpWidth  = pDP->GetWidth();
  PIX dpHeight = pDP->GetHeight();
  pDP->Fill( C_WHITE | CT_OPAQUE);
  pDP->Fill( 2,2, dpWidth-4, dpHeight-4, C_BLACK | CT_OPAQUE);

  // draw gradients
  DrawGradient( pDP, C_WHITE, C_RED,   1.0f/8*dpWidth, 1.0f/16*dpHeight, 1.0f/2*dpWidth, 2.0f/16*dpHeight);
  DrawGradient( pDP, C_RED,   C_BLACK, 1.0f/2*dpWidth, 1.0f/16*dpHeight, 7.0f/8*dpWidth, 2.0f/16*dpHeight);
  DrawGradient( pDP, C_BLACK, C_GREEN, 1.0f/8*dpWidth, 2.0f/16*dpHeight, 1.0f/2*dpWidth, 3.0f/16*dpHeight);
  DrawGradient( pDP, C_GREEN, C_WHITE, 1.0f/2*dpWidth, 2.0f/16*dpHeight, 7.0f/8*dpWidth, 3.0f/16*dpHeight);
  DrawGradient( pDP, C_WHITE, C_BLUE,  1.0f/8*dpWidth, 3.0f/16*dpHeight, 1.0f/2*dpWidth, 4.0f/16*dpHeight);
  DrawGradient( pDP, C_BLUE,  C_BLACK, 1.0f/2*dpWidth, 3.0f/16*dpHeight, 7.0f/8*dpWidth, 4.0f/16*dpHeight);

  DrawGradient( pDP, C_BLACK,   C_CYAN,    1.0f/8*dpWidth, 4.5f/16*dpHeight, 1.0f/2*dpWidth, 5.5f/16*dpHeight);
  DrawGradient( pDP, C_CYAN,    C_WHITE,   1.0f/2*dpWidth, 4.5f/16*dpHeight, 7.0f/8*dpWidth, 5.5f/16*dpHeight);
  DrawGradient( pDP, C_WHITE,   C_MAGENTA, 1.0f/8*dpWidth, 5.5f/16*dpHeight, 1.0f/2*dpWidth, 6.5f/16*dpHeight);
  DrawGradient( pDP, C_MAGENTA, C_BLACK,   1.0f/2*dpWidth, 5.5f/16*dpHeight, 7.0f/8*dpWidth, 6.5f/16*dpHeight);
  DrawGradient( pDP, C_BLACK,   C_YELLOW,  1.0f/8*dpWidth, 6.5f/16*dpHeight, 1.0f/2*dpWidth, 7.5f/16*dpHeight);
  DrawGradient( pDP, C_YELLOW,  C_WHITE,   1.0f/2*dpWidth, 6.5f/16*dpHeight, 7.0f/8*dpWidth, 7.5f/16*dpHeight);

  DrawGradient( pDP, C_WHITE,   C_BLACK,   1.0f/8*dpWidth, 8.0f/16*dpHeight, 7.0f/8*dpWidth, 10.0f/16*dpHeight);

  // draw rectangles
  pDP->Fill( 1.5f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dRED     |CT_OPAQUE);
  pDP->Fill( 1.5f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_RED      |CT_OPAQUE);
  pDP->Fill( 1.5f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lRED     |CT_OPAQUE);

  pDP->Fill( 2.0f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dGREEN   |CT_OPAQUE);
  pDP->Fill( 2.0f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_GREEN    |CT_OPAQUE);
  pDP->Fill( 2.0f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lGREEN   |CT_OPAQUE);

  pDP->Fill( 2.5f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dBLUE    |CT_OPAQUE);
  pDP->Fill( 2.5f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_BLUE     |CT_OPAQUE);
  pDP->Fill( 2.5f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lBLUE    |CT_OPAQUE);

  pDP->Fill( 3.0f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dCYAN    |CT_OPAQUE);
  pDP->Fill( 3.0f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_CYAN     |CT_OPAQUE);
  pDP->Fill( 3.0f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lCYAN    |CT_OPAQUE);

  pDP->Fill( 3.5f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dMAGENTA |CT_OPAQUE);
  pDP->Fill( 3.5f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_MAGENTA  |CT_OPAQUE);
  pDP->Fill( 3.5f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lMAGENTA |CT_OPAQUE);

  pDP->Fill( 4.0f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dYELLOW  |CT_OPAQUE);
  pDP->Fill( 4.0f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_YELLOW   |CT_OPAQUE);
  pDP->Fill( 4.0f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lYELLOW  |CT_OPAQUE);

  pDP->Fill( 4.5f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dGRAY    |CT_OPAQUE);
  pDP->Fill( 4.5f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_GRAY     |CT_OPAQUE);
  pDP->Fill( 4.5f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lGRAY    |CT_OPAQUE);

  pDP->Fill( 5.0f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dORANGE  |CT_OPAQUE);
  pDP->Fill( 5.0f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_ORANGE   |CT_OPAQUE);
  pDP->Fill( 5.0f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lORANGE  |CT_OPAQUE);

  pDP->Fill( 5.5f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dBROWN   |CT_OPAQUE);
  pDP->Fill( 5.5f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_BROWN    |CT_OPAQUE);
  pDP->Fill( 5.5f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lBROWN   |CT_OPAQUE);

  pDP->Fill( 6.0f/8*dpWidth, 10.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_dPINK    |CT_OPAQUE);
  pDP->Fill( 6.0f/8*dpWidth, 11.0f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_PINK     |CT_OPAQUE);
  pDP->Fill( 6.0f/8*dpWidth, 11.5f/16*dpHeight, 1.0f/16*dpWidth, 0.5f/16*dpHeight, C_lPINK    |CT_OPAQUE);

  // set font
  pDP->SetFont( _pfdDisplayFont);
  // create text to display
  CTString strTestMessage = "test screen";

  // type messages
  pDP->PutTextC( strTestMessage, 1.0f/4*dpWidth, 12.5f/16*dpHeight, C_dRED  |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 2.0f/4*dpWidth, 12.5f/16*dpHeight, C_RED   |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 3.0f/4*dpWidth, 12.5f/16*dpHeight, C_lRED  |CT_OPAQUE);

  pDP->PutTextC( strTestMessage, 1.0f/4*dpWidth, 13.0f/16*dpHeight, C_dGREEN|CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 2.0f/4*dpWidth, 13.0f/16*dpHeight, C_GREEN |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 3.0f/4*dpWidth, 13.0f/16*dpHeight, C_lGREEN|CT_OPAQUE);

  pDP->PutTextC( strTestMessage, 1.0f/4*dpWidth, 13.5f/16*dpHeight, C_dBLUE |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 2.0f/4*dpWidth, 13.5f/16*dpHeight, C_BLUE  |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 3.0f/4*dpWidth, 13.5f/16*dpHeight, C_lBLUE |CT_OPAQUE);

  pDP->PutTextC( strTestMessage, 1.0f/4*dpWidth, 14.0f/16*dpHeight, C_dGRAY |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 2.0f/4*dpWidth, 14.0f/16*dpHeight, C_GRAY  |CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 3.0f/4*dpWidth, 14.0f/16*dpHeight, C_lGRAY |CT_OPAQUE);

  // type resolution
  CDisplayMode dmCurrent;
  _pGfx->GetCurrentDisplayMode( dmCurrent);
  strTestMessage.PrintF( "%d x %d x %s", dpWidth, dpHeight, dmCurrent.DepthString());
  pDP->PutTextC( strTestMessage, 1.0f/2*dpWidth+2, 1.0f/2*dpHeight+2, C_dGRAY|CT_OPAQUE);
  pDP->PutTextC( strTestMessage, 1.0f/2*dpWidth,   1.0f/2*dpHeight,   C_WHITE|CT_OPAQUE);

  // unlock draw port
  pDP->Unlock();

  // show screen
  pVP->SwapBuffers();
}


/////////////////////////////////////////////////////////////////////////////
// CDlgSelectMode dialog


CDlgSelectMode::CDlgSelectMode( CDisplayMode &dm, enum GfxAPIType &gfxAPI, 
                                CWnd* pParent /*=NULL*/) : CDialog( CDlgSelectMode::IDD, pParent)
{
  // obtain all available modes
  m_pdmAvailableModes = _pGfx->EnumDisplayModes(m_ctAvailableDisplayModes);

  // remember initial mode reference
  m_pdm = &dm;
  m_pGfxAPI = &gfxAPI;

  //{{AFX_DATA_INIT(CDlgSelectMode)
	m_strCurrentMode = _T("");
	m_strCurrentDriver = _T("");
	m_iColor = -1;
	//}}AFX_DATA_INIT

  // set current mode and driver strings
  CTString str;
  str.PrintF( "%d x %d x %s", dm.dm_pixSizeI, dm.dm_pixSizeJ, dm.DepthString());
  m_strCurrentMode = str;

  switch(gfxAPI) {
  case GAT_OGL:
    m_strCurrentDriver = "OpenGL";
    break;
#ifdef SE1_D3D
  case GAT_D3D:
    m_strCurrentDriver = "Direct3D";
    break;
#endif // SE1_D3D
  default:
    m_strCurrentDriver = "none";
    break;
  }
}

CDlgSelectMode::~CDlgSelectMode()
{

}

void CDlgSelectMode::ApplySettings( CDisplayMode *pdm, enum GfxAPIType *m_pGfxAPI)
{
  // pass driver type var
  *m_pGfxAPI = (GfxAPIType)m_ctrlDriverCombo.GetCurSel();
  // determine color mode
  DisplayDepth ddDepth;
  switch( m_iColor) {
  case 0: ddDepth = DD_DEFAULT;  break;
  case 1: ddDepth = DD_16BIT;    break;
  case 2: ddDepth = DD_32BIT;    break;
  default: ASSERT(FALSE); ddDepth = DD_DEFAULT;  break;
  }
  // get resolution
  const ULONG ulRes = (ULONG)m_ctrlResCombo.GetItemData( m_ctrlResCombo.GetCurSel());
  const PIX pixSizeI = ulRes>>16;
  const PIX pixSizeJ = ulRes&0xFFFF;

  // find potentional corresponding modes
  for( INDEX iMode=0; iMode<m_ctAvailableDisplayModes; iMode++)
  { // if found mode that matches in resolution
    if( pixSizeI==m_pdmAvailableModes[iMode].dm_pixSizeI
     && pixSizeJ==m_pdmAvailableModes[iMode].dm_pixSizeJ) {
      // get it and set wanted depth
      pdm->dm_pixSizeI = pixSizeI;
      pdm->dm_pixSizeJ = pixSizeJ;
      pdm->dm_ddDepth  = ddDepth;
    }
  }
}


void CDlgSelectMode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // prepare radio buttons
  if( !pDX->m_bSaveAndValidate)
  {
    // set current color radios
    switch (m_pdm->dm_ddDepth) {
    case DD_DEFAULT: m_iColor = 0; break;
    case DD_16BIT  : m_iColor = 1; break;
    case DD_32BIT  : m_iColor = 2; break;
    default: ASSERT(FALSE); m_iColor=0;  break;
    }
  }

	//{{AFX_DATA_MAP(CDlgSelectMode)
	DDX_Control(pDX, IDC_RESOLUTIONS, m_ctrlResCombo);
	DDX_Control(pDX, IDC_API, m_ctrlDriverCombo);
	DDX_Text(pDX, IDC_CURRENT_MODE, m_strCurrentMode);
	DDX_Text(pDX, IDC_CURRENT_DRIVER, m_strCurrentDriver);
	DDX_Radio(pDX, IDC_COLOR_DEFAULT, m_iColor);
	//}}AFX_DATA_MAP

  // if dialog is recieving data
  if( !pDX->m_bSaveAndValidate)
  { 
    INDEX i, iSelect=0;

    // clear combo boxes
    m_ctrlDriverCombo.ResetContent();
    m_ctrlResCombo.ResetContent();

    // init driver combo
    i = m_ctrlDriverCombo.AddString( L"OpenGL");
    m_ctrlDriverCombo.SetItemData( i, (INDEX)GAT_OGL);
    if( *m_pGfxAPI==GAT_OGL) iSelect = i;
#ifdef SE1_D3D
    i = m_ctrlDriverCombo.AddString( L"Direct3D");
    m_ctrlDriverCombo.SetItemData( i, (INDEX)GAT_D3D);
    if( *m_pGfxAPI==GAT_D3D) iSelect = i;
#endif // SE1_D3D
    // set old driver to be default
    m_ctrlDriverCombo.SetCurSel( iSelect);
  
    // init resolutions combo
    iSelect=0;
    for( INDEX iMode=0; iMode<m_ctAvailableDisplayModes; iMode++)
    { // prepare resolution string
      CTString strRes;
      PIX pixSizeI = m_pdmAvailableModes[iMode].dm_pixSizeI;
      PIX pixSizeJ = m_pdmAvailableModes[iMode].dm_pixSizeJ;
      strRes.PrintF( "%d x %d", pixSizeI, pixSizeJ);
      // if not yet added
      if( m_ctrlResCombo.FindStringExact( 0, CString(strRes)) == CB_ERR) {
        // add it to combo box list
        i = m_ctrlResCombo.AddString(CString(strRes));
        // set item data to match the resolutions (I in upper word, J in lower)
        m_ctrlResCombo.SetItemData( i, (pixSizeI<<16)|pixSizeJ);
        // if found old full screen mode
        if( pixSizeI==m_pdm->dm_pixSizeI && pixSizeJ==m_pdm->dm_pixSizeJ) {
          // mark it to be selected by default
          iSelect = i;
        }
      }
    } // set current res combo default mode
    m_ctrlResCombo.SetCurSel( iSelect);
  }

// --------------------------

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate)
  { // apply new display mode settings
    ApplySettings( m_pdm, m_pGfxAPI);
  }
}


BEGIN_MESSAGE_MAP(CDlgSelectMode, CDialog)
	//{{AFX_MSG_MAP(CDlgSelectMode)
	ON_BN_CLICKED(ID_TEST_BUTTON, OnTestButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgSelectMode message handlers

void CDlgSelectMode::OnTestButton()
{
  CWnd wndTestWindowedMode;

  UpdateData( TRUE);

  // apply wanted display mode settings
  CDisplayMode dm;
  enum GfxAPIType gfxAPI;
  ApplySettings( &dm, &gfxAPI);

  // try to set wanted display mode
  PIX pixSizeI    = dm.dm_pixSizeI;
  PIX pixSizeJ    = dm.dm_pixSizeJ;

  BOOL bDisplayModeSet = _pGfx->SetDisplayMode( GAT_OGL, 0, pixSizeI, pixSizeJ, dm.dm_ddDepth);
  if( !bDisplayModeSet) {
    AfxMessageBox( L"Unable to setup full screen display. Test mode failed.");
    return;
  }

  //--------------------------- Open window for testing windowed display mode

  // draw ports and viewports needed for printing message
  CDrawPort *pDrawPort;
  CViewPort *pViewPort;

  // get the windows dimensions for this display
	int iScreenX = ::GetSystemMetrics(SM_CXSCREEN);	// screen size
	int iScreenY = ::GetSystemMetrics(SM_CYSCREEN);

  // open window of display mode size
  const wchar_t *strWindowClass = AfxRegisterWndClass( CS_OWNDC|CS_NOCLOSE);
  wndTestWindowedMode.CreateEx( WS_EX_TOPMOST, strWindowClass, L"Test mode",
                                WS_POPUP|WS_VISIBLE, 0,0, iScreenX,iScreenY, m_hWnd, 0);
  // create window canvas
  _pGfx->CreateWindowCanvas( wndTestWindowedMode.m_hWnd, &pViewPort, &pDrawPort);

  // if screen or window opening was not successful
  if( pViewPort == NULL) {
    AfxMessageBox( L"Unable to setup full screen display. Test mode failed.");
    return;
  }

  // show test mode screen
  ShowTestModeScreen( pDrawPort, pViewPort);
    
  // get starting time
  CTimerValue tvStart = _pTimer->GetHighPrecisionTimer();
  // loop forever
  FOREVER {
    // get current time
    CTimerValue tvCurrent = _pTimer->GetHighPrecisionTimer();
    // get time difference in seconds
    CTimerValue tvElapsed = tvCurrent - tvStart;
    // three seconds passed?
    if( tvElapsed.GetSeconds() > 5.0f) break;
  }
  
	// destroy windowed canvas
  _pGfx->DestroyWindowCanvas( pViewPort);
  pViewPort = NULL;
  // destroy window
  wndTestWindowedMode.DestroyWindow();

  // restore old mode
  _pGfx->ResetDisplayMode();

  if( AfxMessageBox( L"Did You see displayed message correctly?", MB_YESNO) == IDYES) {
    GetDlgItem( IDOK)->SetFocus(); // set focus to apply button
  } else {
    AfxMessageBox( L"Mode is not valid and it is rejected. Choose another one.");
  }

  Invalidate( FALSE);
}
