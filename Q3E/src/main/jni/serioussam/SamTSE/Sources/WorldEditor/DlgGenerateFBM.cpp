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

// DlgGenerateFBM.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgGenerateFBM.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgGenerateFBM dialog

INDEX _iFBMOctaves;
FLOAT _fFBMHighFrequencyStep;
FLOAT _fFBMStepFactor;
FLOAT _fFBMMaxAmplitude;
FLOAT _fFBMfAmplitudeDecreaser;
BOOL _bFBMAddNegativeValues;
BOOL _bFBMRandomOffset;

CDlgGenerateFBM::CDlgGenerateFBM(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgGenerateFBM::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgGenerateFBM)
	m_fMaxAltitude = 0.0f;
	m_fOctaveAmplitudeDecreaser = 0.0f;
	m_fOctaveAmplitudeStep = 0.0f;
	m_fHighFrequencyStep = 0.0f;
	m_ctOctaves = 0;
	m_bAddNegativeValues = FALSE;
	m_bRandomOffset = FALSE;
	//}}AFX_DATA_INIT

  m_bCustomWindowCreated = FALSE;
  m_pdp=NULL;
  m_pvp=NULL;

  _iFBMOctaves=theApp.m_iFBMOctaves;
  _fFBMHighFrequencyStep=theApp.m_fFBMHighFrequencyStep;
  _fFBMStepFactor=theApp.m_fFBMStepFactor;
  _fFBMMaxAmplitude=theApp.m_fFBMMaxAmplitude;
  _fFBMfAmplitudeDecreaser=theApp.m_fFBMfAmplitudeDecreaser;
  _bFBMAddNegativeValues=theApp.m_bFBMAddNegativeValues;
  _bFBMRandomOffset=theApp.m_bFBMRandomOffset;
}


void CDlgGenerateFBM::DoDataExchange(CDataExchange* pDX)
{
  // if dialog is recieving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {
    m_ctOctaves=_iFBMOctaves;
    m_fHighFrequencyStep=_fFBMHighFrequencyStep;
    m_fOctaveAmplitudeStep=_fFBMStepFactor;
    m_fMaxAltitude=_fFBMMaxAmplitude;
    m_fOctaveAmplitudeDecreaser=_fFBMfAmplitudeDecreaser;
    m_bAddNegativeValues=_bFBMAddNegativeValues;
    m_bRandomOffset=_bFBMRandomOffset;
  }
  else if( !::IsWindow(m_ctrlCtOctavesSpin.m_hWnd)) return;

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgGenerateFBM)
	DDX_Control(pDX, IDC_CT_OCTAVES_SPIN, m_ctrlCtOctavesSpin);
	DDX_Text(pDX, IDC_FBM_MAX_ALTITUDE, m_fMaxAltitude);
	DDV_MinMaxFloat(pDX, m_fMaxAltitude, 0.f, 65535.f);
	DDX_Text(pDX, IDC_FBM_OCTAVE_AMPLITUDE_DECREASE, m_fOctaveAmplitudeDecreaser);
	DDV_MinMaxFloat(pDX, m_fOctaveAmplitudeDecreaser, 0.f, 16.f);
	DDX_Text(pDX, IDC_FBM_OCTAVE_STEP, m_fOctaveAmplitudeStep);
	DDV_MinMaxFloat(pDX, m_fOctaveAmplitudeStep, 0.f, 128.f);
	DDX_Text(pDX, IDC_FBM_HIGH_FREQUENCY_STEP, m_fHighFrequencyStep);
	DDV_MinMaxFloat(pDX, m_fHighFrequencyStep, 0.f, 16.f);
	DDX_Text(pDX, IDC_FBM_OCTAVES, m_ctOctaves);
	DDV_MinMaxInt(pDX, m_ctOctaves, 0, 16);
	DDX_Check(pDX, IDC_ADD_NEGATIVE_VALUES, m_bAddNegativeValues);
	DDX_Check(pDX, IDC_RANDOM_OFFSET, m_bRandomOffset);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if(pDX->m_bSaveAndValidate != FALSE)
  {
    _iFBMOctaves=m_ctOctaves;
    _fFBMHighFrequencyStep=m_fHighFrequencyStep;
    _fFBMStepFactor=m_fOctaveAmplitudeStep;
    _fFBMMaxAmplitude=m_fMaxAltitude;
    _fFBMfAmplitudeDecreaser=m_fOctaveAmplitudeDecreaser;
  }
}


BEGIN_MESSAGE_MAP(CDlgGenerateFBM, CDialog)
	//{{AFX_MSG_MAP(CDlgGenerateFBM)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_FBM_RANDOMIZE, OnFbmRandomize)
	ON_EN_CHANGE(IDC_FBM_HIGH_FREQUENCY_STEP, OnChangeFbmHighFrequencyStep)
	ON_EN_CHANGE(IDC_FBM_MAX_ALTITUDE, OnChangeFbmMaxAltitude)
	ON_EN_CHANGE(IDC_FBM_OCTAVE_AMPLITUDE_DECREASE, OnChangeFbmOctaveAmplitudeDecrease)
	ON_EN_CHANGE(IDC_FBM_OCTAVE_STEP, OnChangeFbmOctaveStep)
	ON_EN_CHANGE(IDC_FBM_OCTAVES, OnChangeFbmOctaves)
	ON_BN_CLICKED(IDC_ADD_NEGATIVE_VALUES, OnAddNegativeValues)
	ON_BN_CLICKED(IDC_FBM_EXPORT, OnFbmExport)
	ON_BN_CLICKED(IDC_RANDOM_OFFSET, OnRandomOffset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgGenerateFBM message handlers

BOOL CreateFBMTexture(PIX pixW, PIX pixH, CTFileName fnFBMFile)
{
  FLOAT fMin, fMax;
  FLOAT *pafFBM=GenerateTerrain_FBMBuffer( pixW, pixH, _iFBMOctaves,
    _fFBMHighFrequencyStep, _fFBMStepFactor, _fFBMMaxAmplitude,
    _fFBMfAmplitudeDecreaser, _bFBMAddNegativeValues, _bFBMRandomOffset, fMin, fMax);

  CImageInfo ii;
  ii.ii_Width=pixW;
  ii.ii_Height=pixH;
  ii.ii_BitsPerPixel=32;
  ii.ii_Picture=(UBYTE*) AllocMemory(ii.ii_Width*ii.ii_Height*sizeof(COLOR));
  COLOR *pcol=(COLOR *)ii.ii_Picture;

  // convert buffer to equalized color map
  FLOAT fConvertFactor=MAX_UBYTE/(fMax-fMin);
  // pixelate preview area
  for(INDEX y=0; y<pixH; y++)
  {
    for(INDEX x=0; x<pixW; x++)
    {
      INDEX iOffset=y*pixW+x;
      FLOAT fValue=pafFBM[iOffset];
      UBYTE ub=(fValue-fMin)*fConvertFactor;
      COLOR col=RGBToColor(ub,ub,ub)|CT_OPAQUE;
      *pcol=ByteSwap(col);
      pcol++;
    }
  }

  CTextureData tdFBM;
  try
  {
    tdFBM.Create_t( &ii, pixW, 16, TRUE);
    tdFBM.Save_t( fnFBMFile);
  }
  catch (const char *strError)
  {
    (void) strError;
    WarningMessage("Unable to create FBM preview texture!");
    FreeMemory( pafFBM);
    return FALSE;
  }
  FreeMemory( pafFBM);
  return TRUE;
}


void CDlgGenerateFBM::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

  if( !m_bCustomWindowCreated)
  {
    // obtain window position
    CWnd *pwndTexture = GetDlgItem(IDC_FBM_PREVIEW_FRAME);
    ASSERT(pwndTexture!= NULL);
    CRect rect;
    pwndTexture->GetWindowRect(&rect);
    ScreenToClient(&rect);
    m_wndTexture.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rect, this, IDW_FBM_PREVIEW);
    // mark that custom window is created
    m_bCustomWindowCreated = TRUE;
  }

  // ******** Render preview texture
  if (m_pdp==NULL)
  {
    _pGfx->CreateWindowCanvas( m_wndTexture.m_hWnd, &m_pvp, &m_pdp);
  }
  if( (m_pdp!=NULL) && (m_pdp->Lock()) )
  {
    PIX pixW=256;
    PIX pixH=256;
    
    CTFileName fnFBMFile=CTString("Textures\\Editor\\FMPPreview.tex");
    if(CreateFBMTexture(pixW, pixH, fnFBMFile))
    {
      try
      {
        CTextureObject to;
        to.SetData_t(fnFBMFile);
        CTextureData *ptd=(CTextureData *)to.GetData();
        ptd->Reload();
        m_pdp->PutTexture( &to, PIXaabbox2D(PIX2D(0,0),PIX2D(m_pdp->GetWidth(),m_pdp->GetHeight())));
      }
      catch (const char *strError)
      {
        (void) strError;
        WarningMessage("Unable to create FBM preview texture!");
      }
      m_pdp->Unlock();
    }
  }
  if (m_pvp!=NULL)
  {
    m_pvp->SwapBuffers();
  }
}

void CDlgGenerateFBM::OnFbmRandomize() 
{
  RandomizeWhiteNoise();
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnChangeFbmHighFrequencyStep() 
{
  UpdateData(TRUE);
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnChangeFbmMaxAltitude() 
{
  UpdateData(TRUE);
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnChangeFbmOctaveAmplitudeDecrease() 
{
  UpdateData(TRUE);
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnChangeFbmOctaveStep() 
{
  UpdateData(TRUE);
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnChangeFbmOctaves() 
{
  UpdateData(TRUE);
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnOK() 
{
  theApp.m_iFBMOctaves=_iFBMOctaves;
  theApp.m_fFBMHighFrequencyStep=_fFBMHighFrequencyStep;
  theApp.m_fFBMStepFactor=_fFBMStepFactor;
  theApp.m_fFBMMaxAmplitude=_fFBMMaxAmplitude;
  theApp.m_fFBMfAmplitudeDecreaser=_fFBMfAmplitudeDecreaser;
  theApp.m_bFBMAddNegativeValues=_bFBMAddNegativeValues;
  theApp.m_bFBMRandomOffset=_bFBMRandomOffset;

	CDialog::OnOK();
}

BOOL CDlgGenerateFBM::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ctrlCtOctavesSpin.SetRange(0,16);
  m_ctrlCtOctavesSpin.SetPos(_iFBMOctaves);

  return TRUE;
}

void CDlgGenerateFBM::OnAddNegativeValues() 
{
  _bFBMAddNegativeValues=!_bFBMAddNegativeValues;
  UpdateData(TRUE);
  Invalidate(FALSE);
}

void CDlgGenerateFBM::OnRandomOffset() 
{
  _bFBMRandomOffset=!_bFBMRandomOffset;
  UpdateData(TRUE);
  Invalidate(FALSE);
}


void CDlgGenerateFBM::OnFbmExport() 
{
  CTFileName fnFBM=_EngineGUI.FileRequester(
    "Export FBM texture", FILTER_TEX FILTER_PCX FILTER_ALL FILTER_END,
    "Layer mask directory", "Textures\\");
  if( fnFBM=="") return;

	CDlgEditFloat dlg;
  dlg.m_fEditFloat=256.0f;
	dlg.m_strVarName = "Width (pixels)";
  dlg.m_strTitle = "Texture size";
  if(dlg.DoModal()!=IDOK) return;

  PIX pixW=dlg.m_fEditFloat;
  
  if(pixW!=1<<((INDEX)Log2( (FLOAT)pixW)))
  {
    WarningMessage("Size must be power of 2!");
  }
  else
  {
    CreateFBMTexture(pixW, pixW, fnFBM);
  }
}
