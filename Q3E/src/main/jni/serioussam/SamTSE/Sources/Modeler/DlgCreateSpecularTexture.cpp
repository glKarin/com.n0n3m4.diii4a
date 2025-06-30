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

// DlgCreateSpecularTexture.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateSpecularTexture dialog

static TIME timeLastTick=TIME(0);

static ANGLE3D a3dObjectRotation = ANGLE3D( 0, 0, 0);
static ANGLE3D a3dLightRotation = ANGLE3D( 0, 0, 0);
static ANGLE3D a3dObjectAngles = ANGLE3D( 0, 0, 0);
static ANGLE3D a3dLightAngles = ANGLE3D( 0, 0, 0);

static CPoint ptLMBDown;
static CPoint ptRMBDown;

static ANGLE3D GetRotForDelta(CPoint ptDelta, BOOL bInvertY) 
{
  FLOAT fdH = ptDelta.x/15.0f;
  FLOAT fdP = ptDelta.y/15.0f;
  if( bInvertY)
    return ANGLE3D( fdH, -fdP, 0);
  else
    return ANGLE3D( fdH, fdP, 0);
}

#define DEFAULT_EXPONENT_POS 50

CDlgCreateSpecularTexture::CDlgCreateSpecularTexture(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateSpecularTexture::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreateSpecularTexture)
	m_strNumericalExponent = _T("");
	m_bAutoRotate = FALSE;
	//}}AFX_DATA_INIT
  
	m_bAutoRotate = TRUE;
  m_colorSpecular.m_pwndParentDialog = this;
  m_colorLight.m_pwndParentDialog = this;
  m_colorAmbient.m_pwndParentDialog = this;
  m_pPreviewDrawPort = NULL;
  m_pPreviewViewPort = NULL;
  m_pGraphDrawPort = NULL;
  m_pGraphViewPort = NULL;

  m_colorAmbient.SetColor( 0x030303FF);
  
  m_bCustomWindowsCreated = FALSE;

  m_plPlacement.pl_OrientationAngle = ANGLE3D( 30, 0, 0);
  m_moModel.mo_Stretch = FLOAT3D( 1.0f, 1.0f, 1.0f);

  // mark that timer is not yet started
  m_iTimerID = -1;
}


void CDlgCreateSpecularTexture::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  if( !pDX->m_bSaveAndValidate)
  {
    INDEX iExponent = DEFAULT_EXPONENT_POS;
    if( IsWindow( m_sliderSpecularExponent.m_hWnd))
    {
      iExponent = m_sliderSpecularExponent.GetPos();
    }
    CTString strNumericalExponent;
    strNumericalExponent.PrintF( "Value: %.1f", GetFactorForPercentage(iExponent));
    m_strNumericalExponent = strNumericalExponent;
  }
	
  //{{AFX_DATA_MAP(CDlgCreateSpecularTexture)
	DDX_Control(pDX, IDC_SPECULAR_EXPONENT, m_sliderSpecularExponent);
	DDX_Control(pDX, IDC_SPECULAR_COLOR, m_colorSpecular);
	DDX_Control(pDX, IDC_SIZE_IN_PIXELS, m_comboSizeInPixels);
	DDX_Control(pDX, IDC_LIGHT_COLOR, m_colorLight);
	DDX_Control(pDX, IDC_AMBIENT_COLOR, m_colorAmbient);
	DDX_Text(pDX, IDC_NUMERIC_EXPONENT_T, m_strNumericalExponent);
	DDX_Check(pDX, IDC_AUTO_ROTATE, m_bAutoRotate);
	//}}AFX_DATA_MAP
  
  if( (pDX->m_bSaveAndValidate) && IsWindow( m_sliderSpecularExponent.m_hWnd) )
  {                    
    INDEX iSlider = m_sliderSpecularExponent.GetPos();
    CreateTexture( CTString("temp\\SpecularTemp.tex"), GetFactorForPercentage( iSlider));
    CTextureData *pTD = (CTextureData *) m_moModel.mo_toSpecular.GetData();
    if( pTD != NULL) pTD->Reload();
    Invalidate( FALSE);
  }
}


BEGIN_MESSAGE_MAP(CDlgCreateSpecularTexture, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateSpecularTexture)
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_AUTO_ROTATE, OnAutoRotate)
	ON_CBN_SELCHANGE(IDC_SIZE_IN_PIXELS, OnSelchangeSizeInPixels)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

void CDlgCreateSpecularTexture::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  UpdateData( TRUE);
  UpdateData( FALSE);
}

#define EXP_MIN (0.1)
#define EXP_MAX (4000.0)

#define EXP_FACTOR EXP_MIN
#define EXP_BASE pow(EXP_MAX/EXP_MIN, 1.0/100)

FLOAT CDlgCreateSpecularTexture::GetFactorForPercentage( INDEX iSlider)
{
  return FLOAT(EXP_FACTOR*pow(EXP_BASE, iSlider));
}

void CDlgCreateSpecularTexture::CreateTexture( CTFileName fnTexture, FLOAT fExp)
{
  CImageInfo II;
  CTFileStream fsFile;
  CTextureData TD;

  INDEX iSelectedSize = m_comboSizeInPixels.GetCurSel();
  ASSERT( iSelectedSize != CB_ERR);
  PIX pixSize = 1UL<<iSelectedSize;
  PIX pixSizeI = pixSize;
  PIX pixSizeJ = pixSize;
  UBYTE *pubImage = (UBYTE *)AllocMemory(pixSize*pixSize*3);
  II.Attach(pubImage, pixSize, pixSize, 24);
  for (PIX pixI=0; pixI<pixSizeI; pixI++) {
    for (PIX pixJ=0; pixJ<pixSizeJ; pixJ++) {
      FLOAT fS = pixI*2.0f/pixSizeI-1;
      FLOAT fT = pixJ*2.0f/pixSizeJ-1;
      FLOAT fZ = Sqrt(1-2*fS*fS-2*fT*fT);
      fZ = Clamp(fZ, 0.0f, 1.0f);
      FLOAT fZN = FLOAT(pow(fZ, fExp));
      ASSERT(fZN>=0 && fZN<=1);
      UBYTE ub = UBYTE(fZN*255);
      pubImage[(pixJ*pixSize+pixI)*3+0] = ub;
      pubImage[(pixJ*pixSize+pixI)*3+1] = ub;
      pubImage[(pixJ*pixSize+pixI)*3+2] = ub;
    }
  }

  try
  {
    TD.Create_t( &II, pixSize, 1, FALSE);
    fsFile.Create_t( fnTexture);
    TD.Write_t( &fsFile);
    fsFile.Close();
  }
  // if failed
  catch (char *strError)
  {
    // report error
    AfxMessageBox(CString(strError));
  }
  II.Detach();
  FreeMemory(pubImage);
}

void CDlgCreateSpecularTexture::DrawPreview( CDrawPort *pdp, FLOAT fExp)
{
  BOOL bErrorOcured = FALSE;
  
  if( (m_moModel.GetData() == NULL) || (m_moModel.mo_toTexture.GetData() == NULL) ||
      (m_moModel.mo_toSpecular.GetData() == NULL) )
  // obtain components for rendering
  try
  {
    DECLARE_CTFILENAME( fnBCGTexture, "Models\\Editor\\SpecularPreviewBCG.tex");
    m_toBackground.SetData_t( fnBCGTexture);

    DECLARE_CTFILENAME( fnTeapotModel, "Models\\Editor\\Teapot.mdl");
    DECLARE_CTFILENAME( fnTeapotTexture, "Models\\Editor\\Teapot.tex");
    m_moModel.SetData_t( fnTeapotModel);
    m_moModel.mo_toTexture.SetData_t( fnTeapotTexture);
    m_moModel.mo_toSpecular.SetData_t( CTString("temp\\SpecularTemp.tex"));
  }
  catch( char *strError)
  {
    (void) strError;
    bErrorOcured = TRUE;
  }

  if( !bErrorOcured)
  {

    ((CModelData*)m_moModel.GetData())->md_colSpecular = m_colorSpecular.GetColor();
    PIXaabbox2D screenBox = PIXaabbox2D( PIX2D(0,0), PIX2D(pdp->GetWidth(), pdp->GetHeight()) );
    //pdp->PutTexture( &m_moModel.mo_toSpecular, screenBox);
    //return;
    pdp->PutTexture( &m_toBackground, screenBox);
    pdp->FillZBuffer( ZBUF_BACK);

    CRenderModel rmRenderModel;
    CPerspectiveProjection3D prPerspectiveProjection;

    a3dObjectAngles += a3dObjectRotation;
    a3dLightAngles += a3dLightRotation;

    m_plPlacement.pl_OrientationAngle = a3dObjectAngles;
    AnglesToDirectionVector( a3dLightAngles, rmRenderModel.rm_vLightDirection);

    prPerspectiveProjection.FOVL() = AngleDeg(50.0f);
    prPerspectiveProjection.ScreenBBoxL() = FLOATaabbox2D( 
      FLOAT2D(0.0f,0.0f),FLOAT2D((float)pdp->GetWidth(), (float)pdp->GetHeight()));
    prPerspectiveProjection.AspectRatioL() = 1.0f;
    prPerspectiveProjection.FrontClipDistanceL() = 0.05f;

    prPerspectiveProjection.ViewerPlacementL().pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
    prPerspectiveProjection.ViewerPlacementL().pl_OrientationAngle = ANGLE3D( 0, -20, 0);
    prPerspectiveProjection.Prepare();
    CAnyProjection3D apr;
    apr = prPerspectiveProjection;
    BeginModelRenderingView(apr, pdp);

    _mrpModelRenderPrefs.SetRenderType( RT_TEXTURE|RT_SHADING_PHONG);
    m_plPlacement.pl_PositionVector = FLOAT3D( 0.0f, -0.19f, -0.35f);
    rmRenderModel.SetObjectPlacement(m_plPlacement);
    rmRenderModel.rm_colLight = m_colorLight.GetColor();
    rmRenderModel.rm_colAmbient = m_colorAmbient.GetColor();
    m_moModel.SetupModelRendering(rmRenderModel);
    m_moModel.RenderModel(rmRenderModel);
    EndModelRenderingView();
  }


  /*
  pdp->Fill(C_GREEN|CT_OPAQUE);
  PIX pixSizeI = pdp->GetWidth();
  PIX pixSizeJ = pdp->GetHeight();
  for (PIX pixI=0; pixI<pixSizeI; pixI++) {
    for (PIX pixJ=0; pixJ<pixSizeJ; pixJ++) {
      FLOAT fX = pixI*2.0f/pixSizeI-1;
      FLOAT fY = pixJ*2.0f/pixSizeJ-1;
      FLOAT fZ;
      FLOAT fZ2 = 1-fX*fX-fY*fY;
      if (fZ2<0) {
        fZ = 0;
      } else {
        fZ = Sqrt(fZ2);
      }
      FLOAT fZN = FLOAT(pow(fZ, fExp));
      ASSERT(fZN>=0 && fZN<=1);
      UBYTE ub = UBYTE(fZN*255);
      pdp->Fill(pixI, pixJ, 1,1, RGBToColor(ub,ub,ub)|CT_OPAQUE);
    }
  }
  */
}

void CDlgCreateSpecularTexture::DrawGraph( CDrawPort *pdp, FLOAT fExp)
{
  pdp->Fill(C_WHITE|CT_OPAQUE);
  PIX pixSizeI = pdp->GetWidth();
  PIX pixSizeJ = pdp->GetHeight();
  PIX pixLastI = 0;
  PIX pixLastJ = 0;
  for (PIX pixI=0; pixI<pixSizeI; pixI++) {
    FLOAT fAlpha = 90.0f*pixI/pixSizeI;
    FLOAT fCos = FLOAT(pow(Cos(fAlpha), fExp));
    PIX pixJ = PIX(pixSizeJ*(1-fCos));
    pdp->DrawLine(pixLastI, pixLastJ, pixI, pixJ, C_BLACK|CT_OPAQUE);
    pixLastI = pixI;
    pixLastJ = pixJ;
  }
}


void CDlgCreateSpecularTexture::RenderGraph(void) 
{
  // ******** Render graph window
  if (m_pGraphViewPort==NULL) {
    _pGfx->CreateWindowCanvas( m_wndGraph.m_hWnd, &m_pGraphViewPort, &m_pGraphDrawPort);
  }
  if( (m_pGraphDrawPort != NULL) && (m_pGraphDrawPort->Lock()) )
  {
    INDEX iSlider = m_sliderSpecularExponent.GetPos();
    DrawGraph( m_pGraphDrawPort, GetFactorForPercentage( iSlider));
    m_pGraphDrawPort->Unlock();
  }
  if (m_pGraphViewPort!=NULL)    m_pGraphViewPort->SwapBuffers();
  if( m_pGraphViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pGraphViewPort);
    m_pGraphViewPort = NULL;
  }
}

void CDlgCreateSpecularTexture::RenderPreview(void) 
{
  // ******** Render preview window
  if (m_pPreviewDrawPort==NULL) {
    _pGfx->CreateWindowCanvas( m_wndPreview.m_hWnd, &m_pPreviewViewPort, &m_pPreviewDrawPort);
  }
  if( (m_pPreviewDrawPort != NULL) && (m_pPreviewDrawPort->Lock()) )
  {
    INDEX iSlider = m_sliderSpecularExponent.GetPos();
    DrawPreview( m_pPreviewDrawPort, GetFactorForPercentage( iSlider));
    m_pPreviewDrawPort->Unlock();
  }
  if (m_pPreviewViewPort!=NULL)    m_pPreviewViewPort->SwapBuffers();
}

void CDlgCreateSpecularTexture::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

  if( m_iTimerID == -1)
  {
    m_iTimerID = (int) SetTimer( 1, 24, NULL);
  }

  if( !m_bCustomWindowsCreated)
  {
    // ---------------- Create window for graph
    // obtain frames area window
    CWnd *pwndGraph = GetDlgItem(IDC_GRAPH_FRAME);
    ASSERT(pwndGraph != NULL);
    CRect rectGraphWindow;
    pwndGraph->GetWindowRect(&rectGraphWindow);
    ScreenToClient(&rectGraphWindow);
    m_wndGraph.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rectGraphWindow,
                                 this, IDW_SPECULAR_GRAPH);
    // ---------------- Create window for preview
    CWnd *pWndPreview = GetDlgItem(IDC_PREVIEW_FRAME);
    ASSERT(pWndPreview != NULL);
    CRect rectPreview;
    pWndPreview->GetWindowRect(&rectPreview);
    ScreenToClient(&rectPreview);
    // create window for for animation testing
    m_wndPreview.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rectPreview,
                                   this, IDW_SPECULAR_PREVIEW);
    // mark that custom windows are created
    m_bCustomWindowsCreated = TRUE;
  }

  RenderGraph();
  RenderPreview();
}

BOOL CDlgCreateSpecularTexture::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  if(::IsWindow( m_comboSizeInPixels.m_hWnd))
  {
    m_comboSizeInPixels.SetCurSel( 6);
  }

  if( IsWindow( m_sliderSpecularExponent.m_hWnd))
  {
    m_sliderSpecularExponent.SetRange(0, 100, TRUE);
    m_sliderSpecularExponent.SetPos(DEFAULT_EXPONENT_POS);
  }

  UpdateData( TRUE);

	return TRUE;
}

void CDlgCreateSpecularTexture::OnTimer(UINT_PTR nIDEvent) 
{
	// on our timer discard preview window
  if( nIDEvent == 1)
  {
    TIME timeCurrentTick = _pTimer->GetRealTimeTick();
    if( timeCurrentTick > timeLastTick )
    {
      _pTimer->SetCurrentTick( timeCurrentTick);
      timeLastTick = timeCurrentTick;
    }
    RenderPreview();	
  }

	CDialog::OnTimer(nIDEvent);
}

void CDlgCreateSpecularTexture::OnDestroy() 
{
  if( m_pPreviewViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pPreviewViewPort);
    m_pPreviewViewPort = NULL;
  }

  KillTimer( m_iTimerID);
  _pTimer->SetCurrentTick( 0.0f);
	CDialog::OnDestroy();
}

void CDlgCreateSpecularTexture::OnAutoRotate() 
{
  a3dObjectAngles = ANGLE3D( 0, 0, 0);
  a3dLightAngles = ANGLE3D( 0, 0, 0);
  if( m_bAutoRotate)
  {
    a3dObjectRotation = ANGLE3D( 0, 0, 0);
    a3dLightRotation = ANGLE3D( 0, 0, 0);
  }
  else
  {
    a3dObjectRotation = ANGLE3D( -2.5f, 0, 0);
    a3dLightRotation = ANGLE3D( 0, 0, 0);
  }
  m_bAutoRotate = !m_bAutoRotate;
}

void CDlgCreateSpecularTexture::OnSelchangeSizeInPixels() 
{
  UpdateData( TRUE);
}

static BOOL bWeStartedMouseDown = FALSE;

BOOL CDlgCreateSpecularTexture::PreTranslateMessage(MSG* pMsg) 
{
  ULONG fwKeys = pMsg->wParam;
  PIX xPos = LOWORD(pMsg->lParam);
  PIX yPos = HIWORD(pMsg->lParam);
  CPoint point = CPoint(xPos, yPos);
  CPoint pointScreen;
  GetCursorPos( &pointScreen);

  CWnd *pWndPreview = GetDlgItem(IDC_PREVIEW_FRAME);
  CRect rectPreview;
  pWndPreview->GetClientRect( &rectPreview);
  pWndPreview->ClientToScreen( &rectPreview);

  
  if( rectPreview.PtInRect( pointScreen))
  {
    if( (pMsg->message == WM_MOUSEMOVE) && (bWeStartedMouseDown) )
    {
      if( fwKeys&MK_LBUTTON)      a3dObjectRotation = GetRotForDelta( point-ptLMBDown, FALSE);
      else if( fwKeys&MK_RBUTTON) a3dLightRotation = GetRotForDelta( point-ptRMBDown, FALSE);
    }
    else if( (pMsg->message == WM_LBUTTONDOWN) || ( pMsg->message == WM_RBUTTONDOWN) )
    {
      ptLMBDown = point;
      ptRMBDown = point;
      bWeStartedMouseDown = TRUE;
    }
    else if( (pMsg->message == WM_LBUTTONUP) && (bWeStartedMouseDown) )
    {
      a3dObjectRotation = GetRotForDelta( point-ptLMBDown, FALSE);
      bWeStartedMouseDown = FALSE;
    }
    else if( (pMsg->message == WM_RBUTTONUP) && (bWeStartedMouseDown) )
    {
      a3dLightRotation = GetRotForDelta( point-ptRMBDown, FALSE);
      bWeStartedMouseDown = FALSE;
    }
  }

  return CDialog::PreTranslateMessage(pMsg);
}

void CDlgCreateSpecularTexture::OnOK() 
{
  CTFileName fnTemp = _fnmApplicationPath+CTString("temp\\SpecularTemp.tex");
  CTFileName fnFinal = _EngineGUI.FileRequester( "Save texture as ...",
                                                FILTER_TEX FILTER_ALL FILTER_END,
                                                "Specular map textures directory",
                                                "Textures\\");
  if( fnFinal != "")
  {
    CopyFileA( fnTemp, _fnmApplicationPath+fnFinal, FALSE);
  }

	CDialog::OnOK();
}
