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

// DlgTEOperationSettings.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgTEOperationSettings.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgTEOperationSettings dialog


CDlgTEOperationSettings::CDlgTEOperationSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgTEOperationSettings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgTEOperationSettings)
	m_fClampAltitude = 0.0f;
	m_fNoiseAltitude = 0.0f;
	m_fPaintPower = 0.0f;
	m_fPosterizeStep = 0.0f;
	m_fSmoothPower = 0.0f;
	m_fFilterPower = 0.0f;
	m_strContinousNoiseTexture = _T("");
	m_strDistributionNoiseTexture = _T("");
	//}}AFX_DATA_INIT
}


void CDlgTEOperationSettings::DoDataExchange(CDataExchange* pDX)
{
  // if dialog is recieving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {
    m_strDistributionNoiseTexture=theApp.m_fnDistributionNoiseTexture.FileName();
    m_strContinousNoiseTexture=theApp.m_fnContinousNoiseTexture.FileName();

    CTerrain *ptrTerrain=GetTerrain();
    if(ptrTerrain!=NULL)
    {
      GetDlgItem(IDC_EQUALIZE_VALUE)->EnableWindow( TRUE);
  	  m_fClampAltitude=FLOAT(theApp.m_uwEditAltitude)/65535*ptrTerrain->tr_vTerrainSize(2);
    }
    else
    {
      GetDlgItem(IDC_EQUALIZE_VALUE)->EnableWindow( FALSE);
      m_fClampAltitude=0;
    }
	  m_fPaintPower=theApp.m_fPaintPower*100.0f;
	  m_fSmoothPower=theApp.m_fSmoothPower*100.0f;
	  m_fFilterPower=theApp.m_fFilterPower*100.0f;
	  m_fPosterizeStep=theApp.m_fPosterizeStep;
	  m_fNoiseAltitude=theApp.m_fNoiseAltitude;
  }

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgTEOperationSettings)
	DDX_Control(pDX, IDC_GENERATION_ALGORITHM, m_ctrlGenerationMethod);
	DDX_Control(pDX, IDC_FILTER_COMBO, m_ctrlFilter);
	DDX_Text(pDX, IDC_EQUALIZE_VALUE, m_fClampAltitude);
	DDV_MinMaxFloat(pDX, m_fClampAltitude, 0.f, 65535.f);
	DDX_Text(pDX, IDC_NOISE_STRENGTH, m_fNoiseAltitude);
	DDV_MinMaxFloat(pDX, m_fNoiseAltitude, 0.f, 65535.f);
	DDX_Text(pDX, IDC_PAINT_POWER, m_fPaintPower);
	DDV_MinMaxFloat(pDX, m_fPaintPower, 0.f, 10000.f);
	DDX_Text(pDX, IDC_POSTERIZE_STEP, m_fPosterizeStep);
	DDV_MinMaxFloat(pDX, m_fPosterizeStep, 0.f, 65535.f);
	DDX_Text(pDX, IDC_SMOOTH_POWER, m_fSmoothPower);
	DDV_MinMaxFloat(pDX, m_fSmoothPower, 0.f, 10000.f);
	DDX_Text(pDX, IDC_FILTER_POWER, m_fFilterPower);
	DDX_Text(pDX, IDC_CONTINOUS_NOISE_T, m_strContinousNoiseTexture);
	DDX_Text(pDX, IDC_DISTRIBUTION_NOISE_T, m_strDistributionNoiseTexture);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if(pDX->m_bSaveAndValidate != FALSE)
  {
    CTerrain *ptrTerrain=GetTerrain();
    if(ptrTerrain!=NULL)
    {
	    theApp.m_uwEditAltitude=m_fClampAltitude/ptrTerrain->tr_vTerrainSize(2)*65535;
    }
	  theApp.m_fPaintPower=m_fPaintPower/100.0f;
	  theApp.m_fSmoothPower=m_fSmoothPower/100.0f;
    INDEX iSelectedItem=iSelectedItem=m_ctrlFilter.GetCurSel();
    theApp.m_iFilter=m_ctrlFilter.GetItemData(iSelectedItem);
	  theApp.m_fFilterPower=m_fFilterPower/100.0f;
	  theApp.m_fPosterizeStep=m_fPosterizeStep;
	  theApp.m_fNoiseAltitude=m_fNoiseAltitude;
    theApp.m_iTerrainGenerationMethod=m_ctrlGenerationMethod.GetCurSel();
  }
}


BEGIN_MESSAGE_MAP(CDlgTEOperationSettings, CDialog)
	//{{AFX_MSG_MAP(CDlgTEOperationSettings)
	ON_BN_CLICKED(IDC_VIEW_NOISE_TEXTURE, OnViewNoiseTexture)
	ON_BN_CLICKED(IDC_BROWSE_CONTINOUS_NOISE, OnBrowseContinousNoise)
	ON_BN_CLICKED(IDC_BROWSE_DISTRIBUTION_NOISE, OnBrowseDistributionNoise)
	ON_BN_CLICKED(IDC_VIEW_DISTRIBUTION_NOISE_TEXTURE, OnViewDistributionNoiseTexture)
	ON_BN_CLICKED(IDC_GENERATION_SETTINGS, OnGenerationSettings)
	ON_CBN_DROPDOWN(IDC_GENERATION_ALGORITHM, OnDropdownGenerationAlgorithm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgTEOperationSettings message handlers

CTString GetFilterName(INDEX iFilter)
{
  if(iFilter==FLT_SHARPEN) return "Sharpen";
  if(iFilter==FLT_FINEBLUR) return "Fine blur";
  if(iFilter==FLT_EMBOSS) return "Emboss";
  if(iFilter==FLT_EDGEDETECT) return "Edge detect";
  return "Unknown";
}

BOOL CDlgTEOperationSettings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  if( IsWindow( m_ctrlFilter.m_hWnd))
  {
    INDEX iItem;
    iItem=m_ctrlFilter.AddString(CString(GetFilterName(FLT_SHARPEN)));
    m_ctrlFilter.SetItemData(iItem,FLT_SHARPEN);
    if(theApp.m_iFilter==FLT_SHARPEN) m_ctrlFilter.SetCurSel(iItem);
    iItem=m_ctrlFilter.AddString(CString(GetFilterName(FLT_FINEBLUR)));
    m_ctrlFilter.SetItemData(iItem,FLT_FINEBLUR);
    if(theApp.m_iFilter==FLT_FINEBLUR) m_ctrlFilter.SetCurSel(iItem);
    iItem=m_ctrlFilter.AddString(CString(GetFilterName(FLT_EMBOSS)));
    m_ctrlFilter.SetItemData(iItem,FLT_EMBOSS);
    if(theApp.m_iFilter==FLT_EMBOSS) m_ctrlFilter.SetCurSel(iItem);
    iItem=m_ctrlFilter.AddString(CString(GetFilterName(FLT_EDGEDETECT)));
    m_ctrlFilter.SetItemData(iItem,FLT_EDGEDETECT);
    if(theApp.m_iFilter==FLT_EDGEDETECT) m_ctrlFilter.SetCurSel(iItem);

    m_ctrlGenerationMethod.AddString(L"Subdivide and displace");
    m_ctrlGenerationMethod.AddString(L"Fractal Brownian motion (FBM)");
    m_ctrlGenerationMethod.SetCurSel(theApp.m_iTerrainGenerationMethod);
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgTEOperationSettings::OnViewNoiseTexture() 
{
  if(!SetupContinousNoiseTexture()) return;

  POINT pt;
  GetCursorPos(&pt);
  CTString strText1=_ptdContinousRandomNoise->GetName();
  CTString strText2=_ptdContinousRandomNoise->GetDescription();

  CWndDisplayTexture *pDisplay=new CWndDisplayTexture;
  pDisplay->Initialize(pt.x, pt.y, _ptdContinousRandomNoise, strText1, strText2);

  FreeContinousNoiseTexture();
}

void CDlgTEOperationSettings::OnViewDistributionNoiseTexture() 
{
  if(!SetupDistributionNoiseTexture()) return;

  POINT pt;
  GetCursorPos(&pt);
  CTString strText1=_ptdDistributionRandomNoise->GetName();
  CTString strText2=_ptdDistributionRandomNoise->GetDescription();

  CWndDisplayTexture *pDisplay=new CWndDisplayTexture;
  pDisplay->Initialize(pt.x, pt.y, _ptdDistributionRandomNoise, strText1, strText2);

  FreeDistributionNoiseTexture();
}

void CDlgTEOperationSettings::OnBrowseContinousNoise() 
{
  CTFileName fnNoise=_EngineGUI.FileRequester(
    "Noise texture", FILTER_TEX FILTER_ALL FILTER_END,
    "Noise texture directory", "Textures\\");
  if( fnNoise=="") return;

  if(!SetupContinousNoiseTexture()) return;
  theApp.m_fnContinousNoiseTexture=fnNoise;
  FreeContinousNoiseTexture();
  UpdateData(FALSE);
}

void CDlgTEOperationSettings::OnBrowseDistributionNoise() 
{
  CTFileName fnNoise=_EngineGUI.FileRequester(
    "Noise texture", FILTER_TEX FILTER_ALL FILTER_END,
    "Noise texture directory", "Textures\\");
  if( fnNoise=="") return;

  if(!SetupDistributionNoiseTexture()) return;
  theApp.m_fnDistributionNoiseTexture=fnNoise;
  FreeDistributionNoiseTexture();
  UpdateData(FALSE);
}

void CDlgTEOperationSettings::OnGenerationSettings() 
{
  INDEX iItem=m_ctrlGenerationMethod.GetCurSel();
  switch(iItem)
  {
  case 0:
  {
	  CDlgEditFloat dlg(this);
    dlg.m_fEditFloat=theApp.m_iRNDSubdivideAndDisplaceItterations;
	  dlg.m_strVarName = "Random itterations";
    dlg.m_strTitle = "Subdivide and displace";
    if(dlg.DoModal()!=IDOK)
    {
      return;
    }
    theApp.m_iRNDSubdivideAndDisplaceItterations=dlg.m_fEditFloat;
    break;
  }
  case 1:
  {
    CDlgGenerateFBM dlg(this);
    dlg.DoModal();
    break;
  }
  }
}

void CDlgTEOperationSettings::OnDropdownGenerationAlgorithm() 
{
  m_ctrlGenerationMethod.SetDroppedWidth( 256);
}
