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

// DlgCreateReflectionTexture.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateReflectionTexture dialog

DECLARE_CTFILENAME( fnBCGTexture, "Models\\Editor\\SpecularPreviewBCG.tex");

static TIME timeLastTick=TIME(0);

static BOOL _bTimerEnabled = TRUE;

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

// formulae used for ray-plane intersection ray always from (0,0,0)
// system (parametric equations for plane by planar coordinates on it and 
// parametric equations for ray):
// u*ux+v*vx+ox-l*dx=0
// u*uy+v*vy+oy-l*dy=0
// u*uz+v*vz+oz-l*dz=0
// solution:
// 
// u=-(dx*(oy*vz-oz*vy)+dy*(oz*vx-ox*vz)+dz*(ox*vy-oy*vx))/(dx*(uy*vz-uz*vy)+dy*(uz*vx-ux*vz)+dz*(ux*vy-uy*vx))
// v= (dx*(oy*uz-oz*uy)+dy*(oz*ux-ox*uz)+dz*(ox*uy-oy*ux))/(dx*(uy*vz-uz*vy)+dy*(uz*vx-ux*vz)+dz*(ux*vy-uy*vx))
// l= (ox*(uy*vz-uz*vy)+oy*(uz*vx-ux*vz)+oz*(ux*vy-uy*vx))/(dx*(uy*vz-uz*vy)+dy*(uz*vx-ux*vz)+dz*(ux*vy-uy*vx))

struct PlaneParam {
  FLOAT pl_fOX;
  FLOAT pl_fOY;
  FLOAT pl_fOZ;
  FLOAT pl_fUX;
  FLOAT pl_fUY;
  FLOAT pl_fUZ;
  FLOAT pl_fVX;
  FLOAT pl_fVY;
  FLOAT pl_fVZ;
};
//                          ox   oy   oz   ux uy uz  vx vy vz
struct PlaneParam plN = { -128,+128,-128,   +1, 0, 0,  0,-1, 0 };
struct PlaneParam plS = { +128,+128,+128,   -1, 0, 0,  0,-1, 0 };
struct PlaneParam plE = { +128,+128,-128,    0, 0,+1,  0,-1, 0 };
struct PlaneParam plW = { -128,+128,+128,    0, 0,-1,  0,-1, 0 };
struct PlaneParam plF = { -128,-128,-128,   +1, 0, 0,  0, 0,+1 };
struct PlaneParam plC = { -128,+128,+128,   +1, 0, 0,  0, 0,-1 };

BOOL PlaneRayIntersection(
  FLOAT fRayX, FLOAT fRayY, FLOAT fRayZ, struct PlaneParam &plPlane, FLOAT &fU, FLOAT &fV, FLOAT &fL)
{
  FLOAT dx = fRayX;
  FLOAT dy = fRayY;
  FLOAT dz = fRayZ;

  FLOAT ox = plPlane.pl_fOX;
  FLOAT oy = plPlane.pl_fOY;
  FLOAT oz = plPlane.pl_fOZ;
  FLOAT ux = plPlane.pl_fUX;
  FLOAT uy = plPlane.pl_fUY;
  FLOAT uz = plPlane.pl_fUZ;
  FLOAT vx = plPlane.pl_fVX;
  FLOAT vy = plPlane.pl_fVY;
  FLOAT vz = plPlane.pl_fVZ;

  FLOAT fDivisor = dx*(uy*vz-uz*vy)+dy*(uz*vx-ux*vz)+dz*(ux*vy-uy*vx);
  if (fDivisor<0.0001) {
    return FALSE;
  }
  fU = -(dx*(oy*vz-oz*vy)+dy*(oz*vx-ox*vz)+dz*(ox*vy-oy*vx))/fDivisor;
  fV = +(dx*(oy*uz-oz*uy)+dy*(oz*ux-ox*uz)+dz*(ox*uy-oy*ux))/fDivisor;
  fL = +(ox*(uy*vz-uz*vy)+oy*(uz*vx-ux*vz)+oz*(ux*vy-uy*vx))/fDivisor;
  return TRUE;
}

CDlgCreateReflectionTexture::CDlgCreateReflectionTexture(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateReflectionTexture::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreateReflectionTexture)
	m_bAutoRotate = FALSE;
	//}}AFX_DATA_INIT

  m_bAutoRotate = TRUE;
  m_colorReflection.m_pwndParentDialog = this;
  m_colorLight.m_pwndParentDialog = this;
  m_colorAmbient.m_pwndParentDialog = this;

  m_colorAmbient.SetColor( 0x030303FF);
  for (INDEX iwin=0; iwin<7; iwin++) {
    m_apdp[iwin] = NULL;
    m_apvp[iwin] = NULL;
  }
  
  m_bCustomWindowsCreated = FALSE;

  m_plPlacement.pl_OrientationAngle = ANGLE3D( 30, 0, 0);
  m_moModel.mo_Stretch = FLOAT3D( 1.0f, 1.0f, 1.0f);

  // mark that timer is not yet started
  m_iTimerID = -1;
}


void CDlgCreateReflectionTexture::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  if( !pDX->m_bSaveAndValidate)
  {
  }

	//{{AFX_DATA_MAP(CDlgCreateReflectionTexture)
	DDX_Control(pDX, IDC_SIZE_IN_PIXELS, m_comboSizeInPixels);
	DDX_Control(pDX, IDC_REFLECTION_COLOR, m_colorReflection);
	DDX_Control(pDX, IDC_LIGHT_COLOR, m_colorLight);
	DDX_Control(pDX, IDC_AMBIENT_COLOR, m_colorAmbient);
	DDX_Check(pDX, IDC_AUTO_ROTATE, m_bAutoRotate);
	//}}AFX_DATA_MAP

  if( pDX->m_bSaveAndValidate)
  { 

    _bTimerEnabled = FALSE;
    try {
      CreateReflectionTexture_t( CTString("temp\\ReflectionTemp.tex"));
      CTextureData *pTD = (CTextureData *) m_moModel.mo_toReflection.GetData();
      if( pTD != NULL) pTD->Reload();
    } catch( char *strError) { 
      WarningMessage( strError);
    }
    _bTimerEnabled = TRUE;
    Invalidate( FALSE);
  }
}


BEGIN_MESSAGE_MAP(CDlgCreateReflectionTexture, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateReflectionTexture)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_AUTO_ROTATE, OnAutoRotate)
	ON_CBN_SELCHANGE(IDC_SIZE_IN_PIXELS, OnSelchangeSizeInPixels)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateReflectionTexture message handlers

void CDlgCreateReflectionTexture::CreateReflectionTexture_t( CTFileName fnTexture) // throw char *
{
  INDEX iSelectedSize = m_comboSizeInPixels.GetCurSel();
  ASSERT( iSelectedSize != CB_ERR);
  PIX pixSize = 1UL<<(iSelectedSize+1);
  PIX pixSizeU = pixSize;
  PIX pixSizeV = pixSize;
  UBYTE *pubImage = (UBYTE *)AllocMemory(pixSize*pixSize*3);

  CTFileName fnN = m_strBase+"N"+m_strExt;
  CTFileName fnS = m_strBase+"S"+m_strExt;
  CTFileName fnE = m_strBase+"E"+m_strExt;
  CTFileName fnW = m_strBase+"W"+m_strExt;
  CTFileName fnF = m_strBase+"F"+m_strExt;
  CTFileName fnC = m_strBase+"C"+m_strExt;
  CTFileName fnEnv= fnTexture;

  CImageInfo iiN;
  CImageInfo iiS;
  CImageInfo iiW;
  CImageInfo iiE;
  CImageInfo iiF;
  CImageInfo iiC;
  CImageInfo iiEnv;

  UBYTE (*paubN)[256][256][3];
  UBYTE (*paubS)[256][256][3];
  UBYTE (*paubW)[256][256][3];
  UBYTE (*paubE)[256][256][3];
  UBYTE (*paubF)[256][256][3];
  UBYTE (*paubC)[256][256][3];

  BOOL bSucesseful = TRUE;
  try
  {
    iiN.LoadAnyGfxFormat_t(fnN);
    iiS.LoadAnyGfxFormat_t(fnS);
    iiE.LoadAnyGfxFormat_t(fnE);
    iiW.LoadAnyGfxFormat_t(fnW);
    iiF.LoadAnyGfxFormat_t(fnF);
    iiC.LoadAnyGfxFormat_t(fnC);
  }
  catch( char *strError)
  {
    (void) strError;
    bSucesseful = FALSE;
  }

  if( bSucesseful)
  {
    if (iiN.ii_Width!=256 || iiN.ii_Height!=256 || iiN.ii_BitsPerPixel!=24
      ||iiS.ii_Width!=256 || iiS.ii_Height!=256 || iiS.ii_BitsPerPixel!=24
      ||iiE.ii_Width!=256 || iiE.ii_Height!=256 || iiE.ii_BitsPerPixel!=24
      ||iiW.ii_Width!=256 || iiW.ii_Height!=256 || iiW.ii_BitsPerPixel!=24
      ||iiF.ii_Width!=256 || iiF.ii_Height!=256 || iiF.ii_BitsPerPixel!=24
      ||iiC.ii_Width!=256 || iiC.ii_Height!=256 || iiC.ii_BitsPerPixel!=24) {
      throw "All pictures must be 256x256 pixels in 24 bpp!";
    }

    paubN=(UBYTE(*)[256][256][3])iiN.ii_Picture;
    paubS=(UBYTE(*)[256][256][3])iiS.ii_Picture;
    paubE=(UBYTE(*)[256][256][3])iiE.ii_Picture;
    paubW=(UBYTE(*)[256][256][3])iiW.ii_Picture;
    paubF=(UBYTE(*)[256][256][3])iiF.ii_Picture;
    paubC=(UBYTE(*)[256][256][3])iiC.ii_Picture;

    for (PIX pixEnvU=0; pixEnvU<pixSizeU; pixEnvU++) {
      for (PIX pixEnvV=0; pixEnvV<pixSizeV; pixEnvV++) {
        FLOAT fS = pixEnvU*2.0f/pixSizeU-1;
        FLOAT fT = pixEnvV*2.0f/pixSizeV-1;
        FLOAT fZ = 1-2*fS*fS-2*fT*fT;
        FLOAT fM = Sqrt(2+2*fZ);
        FLOAT fX = fS*fM;
        FLOAT fY = fT*fM;
        FLOAT fLen = sqrt(fX*fX+fY*fY+fZ*fZ);
        fX/=fLen;
        fY/=fLen;
        fZ/=fLen;
        Swap(fZ, fY);

        UBYTE (*paub)[256][256][3] = NULL;
        FLOAT fU, fV, fL, fBestU, fBestV, fBestL;
        fBestU = 0.0f;
        fBestV = 0.0f;
        fBestL = 10000.0f;
        if (PlaneRayIntersection(fX, fY, fZ, plN, fU, fV, fL) && fL>0 && fL<fBestL) {
          paub = paubN;
          fBestU = fU;
          fBestV = fV;
          fBestL = fL;
        }
        if (PlaneRayIntersection(fX, fY, fZ, plS, fU, fV, fL) && fL>0 && fL<fBestL) {
          paub = paubS;
          fBestU = fU;
          fBestV = fV;
          fBestL = fL;
        }
        if (PlaneRayIntersection(fX, fY, fZ, plE, fU, fV, fL) && fL>0 && fL<fBestL) {
          paub = paubE;
          fBestU = fU;
          fBestV = fV;
          fBestL = fL;
        }
        if (PlaneRayIntersection(fX, fY, fZ, plW, fU, fV, fL) && fL>0 && fL<fBestL) {
          paub = paubW;
          fBestU = fU;
          fBestV = fV;
          fBestL = fL;
        }
        if (PlaneRayIntersection(fX, fY, fZ, plF, fU, fV, fL) && fL>0 && fL<fBestL){
          paub = paubF;
          fBestU = fU;
          fBestV = fV;
          fBestL = fL;
        }
        if (PlaneRayIntersection(fX, fY, fZ, plC, fU, fV, fL) && fL>0 && fL<fBestL) {
          paub = paubC;
          fBestU = fU;
          fBestV = fV;
          fBestL = fL;
        }
        ASSERT(fBestL>0.1);
        PIX pixU = PIX(fBestU);
        PIX pixV = PIX(fBestV);
        if (pixU<0) {
          ASSERT(pixU>-10);
          pixU = 0;
        }
        if (pixU>255) {
          ASSERT(pixU<260);
          pixU = 255;
        }
        if (pixV<0) {
          ASSERT(pixV>-10);
          pixV = 0;
        }
        if (pixV>255) {
          ASSERT(pixV<260);
          pixV = 255;
        }
        pubImage[(pixEnvV*pixSizeU+pixEnvU)*3+0] = (*paub)[pixV][pixU][0];
        pubImage[(pixEnvV*pixSizeU+pixEnvU)*3+1] = (*paub)[pixV][pixU][1];
        pubImage[(pixEnvV*pixSizeU+pixEnvU)*3+2] = (*paub)[pixV][pixU][2];
      }
    }
  }
  // create black reflection map
  else
  {
    for (PIX pixU=0; pixU<pixSizeU; pixU++)
    {
      for (PIX pixV=0; pixV<pixSizeV; pixV++)
      {
        pubImage[(pixV*pixSizeU+pixU)*3+0] = 0;
        pubImage[(pixV*pixSizeU+pixU)*3+1] = 0;
        pubImage[(pixV*pixSizeU+pixU)*3+2] = 0;
      }
    }
  }

  iiEnv.Attach( pubImage, pixSizeU, pixSizeV, 24);
  try
  {
    CTFileStream fsFile;
    CTextureData td;
    iiEnv.SaveTGA_t( fnEnv.NoExt()+".tga");
    td.Create_t( &iiEnv, pixSize, 1, FALSE);
    fsFile.Create_t(fnEnv);
    td.Write_t( &fsFile);
    fsFile.Close();
  }
  catch(char *strError)
  {
    iiEnv.Detach();
    throw strError;
  }
  iiEnv.Detach();
  
  FreeMemory(pubImage);
}

void CDlgCreateReflectionTexture::DrawPreview( CDrawPort *pdp)
{
  BOOL bErrorOcured = FALSE;
  
  if( (m_moModel.GetData() == NULL) || (m_moModel.mo_toTexture.GetData() == NULL) ||
      (m_moModel.mo_toReflection.GetData() == NULL) )
  // obtain components for rendering
  try
  {
    m_toBackground.SetData_t( fnBCGTexture);
    DECLARE_CTFILENAME( fnTeapotModel, "Models\\Editor\\Teapot.mdl");
    DECLARE_CTFILENAME( fnTeapotTexture, "Models\\Editor\\Teapot.tex");
    m_moModel.SetData_t( fnTeapotModel);
    m_moModel.mo_toTexture.SetData_t( fnTeapotTexture);
    m_moModel.mo_toReflection.SetData_t( CTString("temp\\ReflectionTemp.tex"));
    m_moModel.mo_toSpecular.SetData_t( CTString("temp\\SpecularTemp.tex"));
  }
  catch( char *strError)
  {
    (void) strError;
    bErrorOcured = TRUE;
  }

  if( !bErrorOcured)
  {
    ((CModelData*)m_moModel.GetData())->md_colReflections = m_colorReflection.GetColor();
    PIXaabbox2D screenBox = PIXaabbox2D( PIX2D(0,0), PIX2D(pdp->GetWidth(), pdp->GetHeight()) );
    //pdp->PutTexture( &m_moModel.mo_toReflection, screenBox);
    //return;
    if( m_toBackground.GetData() != NULL) {
      pdp->PutTexture( &m_toBackground, screenBox);
    } else {
      pdp->Fill( C_BLACK|CT_OPAQUE);
    }
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
    m_moModel.SetupModelRendering( rmRenderModel);
    m_moModel.RenderModel( rmRenderModel);
    EndModelRenderingView();
  }
}

void CDlgCreateReflectionTexture::PutPicture(CWnd &wnd, CTextureObject &to, INDEX iwin)
{
  CDrawPort *&pdp = m_apdp[iwin];
  CViewPort *&pvp = m_apvp[iwin];
  if (pvp==NULL) {
    _pGfx->CreateWindowCanvas( wnd.m_hWnd, &pvp, &pdp);
  }
  if( (pdp != NULL) && (pdp->Lock()) ) {
    if( to.GetData()!= NULL) {
      PIXaabbox2D screenBox = PIXaabbox2D( PIX2D(0,0), PIX2D(pdp->GetWidth(), pdp->GetHeight()) );
      pdp->PutTexture( &to, screenBox);
    } else {
      pdp->Fill( C_BLACK|CT_OPAQUE);
    }
    pdp->Unlock();
  }
  if (pvp!=NULL)    pvp->SwapBuffers();
}

void CDlgCreateReflectionTexture::RenderPreview(void) 
{
  // ******** Render preview window
  CDrawPort *&pdp = m_apdp[0];
  CViewPort *&pvp = m_apvp[0];
  if (pvp==NULL) _pGfx->CreateWindowCanvas( m_wndPreview.m_hWnd, &pvp, &pdp);
  if( (pdp != NULL) && (pdp->Lock()) )
  {
    DrawPreview( pdp);
    pdp->Unlock();
  }
  if (pvp!=NULL) pvp->SwapBuffers();

  PutPicture(m_wndN, m_toN, 1);
  PutPicture(m_wndE, m_toE, 2);
  PutPicture(m_wndS, m_toS, 3);
  PutPicture(m_wndW, m_toW, 4);
  PutPicture(m_wndC, m_toC, 5);
  PutPicture(m_wndF, m_toF, 6);
}

#define CREATE_WND( DLGID, wnd, IDW) \
  pWnd = GetDlgItem(DLGID);\
  pWnd->GetWindowRect(&rectWnd);\
  ScreenToClient(&rectWnd);\
  wnd.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rectWnd, this, IDW);
void CDlgCreateReflectionTexture::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

  if( m_iTimerID == -1)
  {
    m_iTimerID = (int) SetTimer( 1, 24, NULL);
  }

  if( !m_bCustomWindowsCreated)
  {
    // ---------------- Create custom windows

    CWnd *pWnd;
    CRect rectWnd;
    CREATE_WND( IDC_PREVIEW_FRAME, m_wndPreview, IDW_REFLECTION_PREVIEW);
    CREATE_WND( IDC_FRAME_FRONT, m_wndN, IDW_FRONT);
    CREATE_WND( IDC_FRAME_RIGHT, m_wndE, IDW_RIGHT);
    CREATE_WND( IDC_FRAME_BACK,  m_wndS, IDW_BACK );
    CREATE_WND( IDC_FRAME_LEFT,  m_wndW, IDW_LEFT );
    CREATE_WND( IDC_FRAME_UP,    m_wndC, IDW_UP   );
    CREATE_WND( IDC_FRAME_DOWN,  m_wndF, IDW_DOWN );

    // mark that custom windows are created
    m_bCustomWindowsCreated = TRUE;
  }
  RenderPreview();
}

BOOL CDlgCreateReflectionTexture::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  if(::IsWindow( m_comboSizeInPixels.m_hWnd))
  {
    m_comboSizeInPixels.SetCurSel( 6);
  }
  UpdateData( TRUE);
	return TRUE;
}

void CDlgCreateReflectionTexture::OnTimer(UINT_PTR nIDEvent) 
{
	// on our timer discard preview window
  if( nIDEvent == 1 && _bTimerEnabled)
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

void CDlgCreateReflectionTexture::OnDestroy() 
{
  for (INDEX iwin=0; iwin<7; iwin++) {
    if( m_apvp[iwin]!=NULL) _pGfx->DestroyWindowCanvas( m_apvp[iwin]);
  }

  KillTimer( m_iTimerID);
  _pTimer->SetCurrentTick( 0.0f);
	CDialog::OnDestroy();
}

void CDlgCreateReflectionTexture::OnAutoRotate() 
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

void CDlgCreateReflectionTexture::OnSelchangeSizeInPixels() 
{
  UpdateData( TRUE);
}

void CDlgCreateReflectionTexture::AutoSetTextures( CTFileName fnFile)
{
  char achrBase[ PATH_MAX];
  strcpy( achrBase, fnFile.FileDir()+fnFile.FileName() );
  achrBase[ strlen(achrBase)-1] = 0;
  m_strBase = achrBase;
  m_strExt = fnFile.FileExt();

  try
  {
    CreateTexture_t( m_strBase+"N"+m_strExt, CTString("Temp\\TempN.tex"), 256, 15, FALSE);
    CreateTexture_t( m_strBase+"E"+m_strExt, CTString("Temp\\TempE.tex"), 256, 15, FALSE);
    CreateTexture_t( m_strBase+"S"+m_strExt, CTString("Temp\\TempS.tex"), 256, 15, FALSE);
    CreateTexture_t( m_strBase+"W"+m_strExt, CTString("Temp\\TempW.tex"), 256, 15, FALSE);
    CreateTexture_t( m_strBase+"C"+m_strExt, CTString("Temp\\TempC.tex"), 256, 15, FALSE);
    CreateTexture_t( m_strBase+"F"+m_strExt, CTString("Temp\\TempF.tex"), 256, 15, FALSE);

    m_toN.SetData_t( CTString("Temp\\TempN.tex"));
    m_toE.SetData_t( CTString("Temp\\TempE.tex"));
    m_toS.SetData_t( CTString("Temp\\TempS.tex"));
    m_toW.SetData_t( CTString("Temp\\TempW.tex"));
    m_toC.SetData_t( CTString("Temp\\TempC.tex"));
    m_toF.SetData_t( CTString("Temp\\TempF.tex"));
  }
  catch( char *strError)
  {
    (void) strError;
  }
}

static BOOL bWeStartedMouseDown = FALSE;

BOOL CDlgCreateReflectionTexture::PreTranslateMessage(MSG* pMsg) 
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
      bWeStartedMouseDown = TRUE;
      ptLMBDown = point;
      ptRMBDown = point;
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
  
#define BROWSE_TEXTURE( ID)\
  pWnd = GetDlgItem(ID);\
  pWnd->GetClientRect( &rectWnd);\
  pWnd->ClientToScreen( &rectWnd);\
  if( rectWnd.PtInRect( pointScreen)) {\
  fnChoosedFile = _EngineGUI.FileRequester( "Select picture", \
    FILTER_TGA FILTER_PCX FILTER_ALL FILTER_END, "Reflection map picures directory", "Textures\\");\
  if( fnChoosedFile != "") {\
    AutoSetTextures(fnChoosedFile);}\
  UpdateData( TRUE);};

#define SET_BCG_TEXTURE( ID, to)\
  pWnd = GetDlgItem(ID);\
  pWnd->GetClientRect( &rectWnd);\
  pWnd->ClientToScreen( &rectWnd);\
  if( rectWnd.PtInRect( pointScreen)) {\
  try {\
    if( to.GetName() == m_toBackground.GetName()){\
      m_toBackground.SetData_t( fnBCGTexture);\
    } else { m_toBackground.SetData_t( to.GetName());};\
  } catch( char *strError) { (void) strError;};};

  if( pMsg->message == WM_LBUTTONUP)
  {
    CTFileName fnChoosedFile;
    CWnd *pWnd;
    CRect rectWnd;

    BROWSE_TEXTURE( IDC_FRAME_FRONT);
    BROWSE_TEXTURE( IDC_FRAME_RIGHT);
    BROWSE_TEXTURE( IDC_FRAME_BACK);
    BROWSE_TEXTURE( IDC_FRAME_LEFT);
    BROWSE_TEXTURE( IDC_FRAME_UP);
    BROWSE_TEXTURE( IDC_FRAME_DOWN);
  }
  
  if( pMsg->message == WM_RBUTTONUP)
  {
    CWnd *pWnd;
    CRect rectWnd;
    SET_BCG_TEXTURE( IDC_FRAME_FRONT, m_toN);
    SET_BCG_TEXTURE( IDC_FRAME_RIGHT, m_toE);
    SET_BCG_TEXTURE( IDC_FRAME_BACK,  m_toS);
    SET_BCG_TEXTURE( IDC_FRAME_LEFT,  m_toW);
    SET_BCG_TEXTURE( IDC_FRAME_UP,    m_toC);
    SET_BCG_TEXTURE( IDC_FRAME_DOWN,  m_toF);
  }

  return CDialog::PreTranslateMessage(pMsg);
}

void CDlgCreateReflectionTexture::OnOK() 
{
  CTFileName fnTemp = _fnmApplicationPath+CTString("temp\\ReflectionTemp.tex");
  CTFileName fnFinal = _EngineGUI.FileRequester( "Save texture as ...",
                                                FILTER_TEX FILTER_ALL FILTER_END,
                                                "Reflection map textures directory",
                                                "Textures\\");
  if( fnFinal != "")
  {
    CopyFileA( fnTemp, _fnmApplicationPath+fnFinal, FALSE);
  }

	CDialog::OnOK();
}
