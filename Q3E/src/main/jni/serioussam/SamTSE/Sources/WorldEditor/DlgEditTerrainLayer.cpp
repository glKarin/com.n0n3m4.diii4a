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

// DlgEditTerrainLayer.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgEditTerrainLayer.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgEditTerrainLayer dialog


CDlgEditTerrainLayer::CDlgEditTerrainLayer(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgEditTerrainLayer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgEditTerrainLayer)
	m_bAutoGenerate = FALSE;
	m_fAltitudeMax = 0.0f;
	m_fAltitudeMaxFade = 0.0f;
	m_fAltitudeMin = 0.0f;
	m_fAltitudeMinFade = 0.0f;
	m_fLayerCoverage = 0.0f;
	m_strLayerName = _T("");
	m_fTextureOffsetX = 0.0f;
	m_fTextureOffsetY = 0.0f;
	m_fTextureRotationU = 0.0f;
	m_fTextureRotationV = 0.0f;
	m_fSlopeMax = 0.0f;
	m_fSlopeMaxFade = 0.0f;
	m_fSlopeMin = 0.0f;
	m_fSlopeMinFade = 0.0f;
	m_fTextureStretchX = 0.0f;
	m_fTextureStretchY = 0.0f;
	m_fCoverageFade = 0.0f;
	m_fAltitudeMaxNoise = 0.0f;
	m_bApplyMaxAltitude = FALSE;
	m_bApplyMaxSlope = FALSE;
	m_bApplyMinAltitude = FALSE;
	m_bApplyMinSlope = FALSE;
	m_fAltitudeMinNoise = 0.0f;
	m_fSlopeMaxNoise = 0.0f;
	m_fSlopeMinNoise = 0.0f;
	//}}AFX_DATA_INIT
}


void CDlgEditTerrainLayer::DoDataExchange(CDataExchange* pDX)
{
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    CTerrainLayer *ptlLayer=GetLayer();
    if(ptlLayer!=NULL)
    {
	    m_bAutoGenerate=ptlLayer->tl_bAutoRegenerated;
	    m_fAltitudeMax=ptlLayer->tl_fMaxAltitude*100.0f;
	    m_fAltitudeMaxFade=ptlLayer->tl_fMaxAltitudeFade*100.0f;
	    m_fAltitudeMin=ptlLayer->tl_fMinAltitude*100.0f;
	    m_fAltitudeMinFade=ptlLayer->tl_fMinAltitudeFade*100.0f;
	    m_fLayerCoverage=ptlLayer->tl_fCoverage*100.0f;
	    m_strLayerName=ptlLayer->tl_strName;
	    m_fTextureOffsetX=ptlLayer->tl_fOffsetX;
	    m_fTextureOffsetY=ptlLayer->tl_fOffsetY;
	    m_fTextureRotationU=ptlLayer->tl_fRotateX;
	    m_fTextureRotationV=ptlLayer->tl_fRotateY;
	    m_fSlopeMax=ptlLayer->tl_fMaxSlope*100.0f;
	    m_fSlopeMaxFade=ptlLayer->tl_fMaxSlopeFade*100.0f;
	    m_fSlopeMin=ptlLayer->tl_fMinSlope*100.0f;
	    m_fSlopeMinFade=ptlLayer->tl_fMinSlopeFade*100.0f;
	    m_fTextureStretchX=ptlLayer->tl_fStretchX;
	    m_fTextureStretchY=ptlLayer->tl_fStretchY;
      m_fCoverageFade=ptlLayer->tl_fCoverageNoise*100.0f;
	    m_fAltitudeMaxNoise=ptlLayer->tl_fMaxAltitudeNoise*100.0f;
	    m_bApplyMaxAltitude=ptlLayer->tl_bApplyMaxAltitude;
	    m_bApplyMaxSlope=ptlLayer->tl_bApplyMaxSlope;
	    m_bApplyMinAltitude=ptlLayer->tl_bApplyMinAltitude;
	    m_bApplyMinSlope=ptlLayer->tl_bApplyMinSlope;
	    m_fAltitudeMinNoise=ptlLayer->tl_fMinAltitudeNoise*100.0f;
	    m_fSlopeMaxNoise=ptlLayer->tl_fMaxSlopeNoise*100.0f;
	    m_fSlopeMinNoise=ptlLayer->tl_fMinSlopeNoise*100.0f;
	  }
  }

	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CDlgEditTerrainLayer)
	DDX_Check(pDX, IDC_AUTO_GENERATE_LAYER_DISTRIBUTION, m_bAutoGenerate);
	DDX_Text(pDX, IDC_TRL_ALTITUDE_MAX, m_fAltitudeMax);
	DDV_MinMaxFloat(pDX, m_fAltitudeMax, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_ALTITUDE_MAX_FADE, m_fAltitudeMaxFade);
	DDV_MinMaxFloat(pDX, m_fAltitudeMaxFade, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_ALTITUDE_MIN, m_fAltitudeMin);
	DDV_MinMaxFloat(pDX, m_fAltitudeMin, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_ALTITUDE_MIN_FADE, m_fAltitudeMinFade);
	DDV_MinMaxFloat(pDX, m_fAltitudeMinFade, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_LAYER_COVERAGE, m_fLayerCoverage);
	DDV_MinMaxFloat(pDX, m_fLayerCoverage, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_LAYER_NAME, m_strLayerName);
	DDX_Text(pDX, IDC_TRL_OFFSET_X, m_fTextureOffsetX);
	DDX_Text(pDX, IDC_TRL_OFFSET_Y, m_fTextureOffsetY);
	DDX_Text(pDX, IDC_TRL_ROTATE_U, m_fTextureRotationU);
	DDX_Text(pDX, IDC_TRL_ROTATE_V, m_fTextureRotationV);
	DDX_Text(pDX, IDC_TRL_SLOPE_MAX, m_fSlopeMax);
	DDV_MinMaxFloat(pDX, m_fSlopeMax, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_SLOPE_MAX_FADE, m_fSlopeMaxFade);
	DDV_MinMaxFloat(pDX, m_fSlopeMaxFade, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_SLOPE_MIN, m_fSlopeMin);
	DDV_MinMaxFloat(pDX, m_fSlopeMin, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_SLOPE_MIN_FADE, m_fSlopeMinFade);
	DDV_MinMaxFloat(pDX, m_fSlopeMinFade, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_STRETCH_X, m_fTextureStretchX);
	DDX_Text(pDX, IDC_TRL_STRETCH_Y, m_fTextureStretchY);
	DDX_Text(pDX, IDC_TRL_COVERAGE_FADE, m_fCoverageFade);
	DDV_MinMaxFloat(pDX, m_fCoverageFade, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_ALTITUDE_MAX_NOISE, m_fAltitudeMaxNoise);
	DDV_MinMaxFloat(pDX, m_fAltitudeMaxNoise, 0.f, 100.f);
	DDX_Check(pDX, IDC_APPLY_MAX_ALTITUDE_DISTRIBUTION, m_bApplyMaxAltitude);
	DDX_Check(pDX, IDC_APPLY_MAX_SLOPE_DISTRIBUTION, m_bApplyMaxSlope);
	DDX_Check(pDX, IDC_APPLY_MIN_ALTITUDE_DISTRIBUTION, m_bApplyMinAltitude);
	DDX_Check(pDX, IDC_APPLY_MIN_SLOPE_DISTRIBUTION, m_bApplyMinSlope);
	DDX_Text(pDX, IDC_TRL_ALTITUDE_MIN_NOISE, m_fAltitudeMinNoise);
	DDV_MinMaxFloat(pDX, m_fAltitudeMinNoise, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_SLOPE_MAX_NOISE, m_fSlopeMaxNoise);
	DDV_MinMaxFloat(pDX, m_fSlopeMaxNoise, 0.f, 100.f);
	DDX_Text(pDX, IDC_TRL_SLOPE_MIN_NOISE, m_fSlopeMinNoise);
	DDV_MinMaxFloat(pDX, m_fSlopeMinNoise, 0.f, 100.f);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    CTerrainLayer *ptlLayer=GetLayer();
    if(ptlLayer!=NULL)
    {
	    ptlLayer->tl_bAutoRegenerated=m_bAutoGenerate;
	    ptlLayer->tl_fMaxAltitude=m_fAltitudeMax/100.0f;
	    ptlLayer->tl_fMaxAltitudeFade=m_fAltitudeMaxFade/100.0f;
	    ptlLayer->tl_fMinAltitude=m_fAltitudeMin/100.0f;
	    ptlLayer->tl_fMinAltitudeFade=m_fAltitudeMinFade/100.0f;
	    ptlLayer->tl_fCoverage=m_fLayerCoverage/100.0f;
	    ptlLayer->tl_strName=CStringA(m_strLayerName);
	    ptlLayer->tl_fOffsetX=m_fTextureOffsetX;
	    ptlLayer->tl_fOffsetY=m_fTextureOffsetY;
	    ptlLayer->tl_fRotateX=m_fTextureRotationU;
	    ptlLayer->tl_fRotateY=m_fTextureRotationV;
	    ptlLayer->tl_fMaxSlope=m_fSlopeMax/100.0f;
	    ptlLayer->tl_fMaxSlopeFade=m_fSlopeMaxFade/100.0f;
	    ptlLayer->tl_fMinSlope=m_fSlopeMin/100.0f;
	    ptlLayer->tl_fMinSlopeFade=m_fSlopeMinFade/100.0f;
	    ptlLayer->tl_fStretchX=m_fTextureStretchX;
	    ptlLayer->tl_fStretchY=m_fTextureStretchY;
      ptlLayer->tl_fCoverageNoise=m_fCoverageFade/100.0f;
	    ptlLayer->tl_fMaxAltitudeNoise=m_fAltitudeMaxNoise/100.0f;
	    ptlLayer->tl_bApplyMaxAltitude=m_bApplyMaxAltitude;
	    ptlLayer->tl_bApplyMaxSlope=m_bApplyMaxSlope;
	    ptlLayer->tl_bApplyMinAltitude=m_bApplyMinAltitude;
	    ptlLayer->tl_bApplyMinSlope=m_bApplyMinSlope;
	    ptlLayer->tl_fMinAltitudeNoise=m_fAltitudeMinNoise/100.0f;
	    ptlLayer->tl_fMaxSlopeNoise=m_fSlopeMaxNoise/100.0f;
	    ptlLayer->tl_fMinSlopeNoise=m_fSlopeMinNoise/100.0f;
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgEditTerrainLayer, CDialog)
	//{{AFX_MSG_MAP(CDlgEditTerrainLayer)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgEditTerrainLayer message handlers
