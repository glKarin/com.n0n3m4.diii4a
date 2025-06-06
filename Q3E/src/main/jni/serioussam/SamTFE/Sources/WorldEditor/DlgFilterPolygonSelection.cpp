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

// DlgFilterPolygonSelection.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgFilterPolygonSelection.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgFilterPolygonSelection dialog


// set grayed flags
#define SET_GRAYED_CONTROL( ctrl)\
  ((CButton *)GetDlgItem(ctrl))->SetCheck(2);

#define ADD_TO_FLAG_MASK( varmask, varvalue, ctrl, flag)\
  if(((CButton *)GetDlgItem(ctrl))->GetCheck()!=2) {\
    varmask|=flag;\
  if(((CButton *)GetDlgItem(ctrl))->GetCheck()==1) varvalue|=flag;}

CDlgFilterPolygonSelection::CDlgFilterPolygonSelection(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFilterPolygonSelection::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgFilterPolygonSelection)
	m_fMinH = 0.0f;
	m_fMinP = 0.0f;
	m_fMaxH = 0.0f;
	m_fMaxP = 0.0f;
	m_fMinX = 0.0f;
	m_fMinY = 0.0f;
	m_fMinZ = 0.0f;
	m_fMaxX = 0.0f;
	m_fMaxY = 0.0f;
	m_fMaxZ = 0.0f;
	//}}AFX_DATA_INIT

  m_fMinH =-100000.0f;
	m_fMinP =-100000.0f;
	m_fMaxH = 100000.0f;
	m_fMaxP = 100000.0f;
	m_fMinX =-100000.0f;
	m_fMinY =-100000.0f;
	m_fMinZ =-100000.0f;
	m_fMaxX = 100000.0f;
	m_fMaxY = 100000.0f;
	m_fMaxZ = 100000.0f;
  m_ctrlMultiplyColor.SetPickerType(  CColoredButton::PT_MFC);
}


void CDlgFilterPolygonSelection::DoDataExchange(CDataExchange* pDX)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    SET_GRAYED_CONTROL( IDC_IS_PORTAL2);
    SET_GRAYED_CONTROL( IDC_IS_OCCLUDER2);
    SET_GRAYED_CONTROL( IDC_IS_OLD_PORTAL2);
    SET_GRAYED_CONTROL( IDC_IS_PASSABLE2);
    SET_GRAYED_CONTROL( IDC_INVISIBLE2);
    SET_GRAYED_CONTROL( IDC_IS_DETAIL2);      
    SET_GRAYED_CONTROL( IDC_IS_TRANSLUSCENT2);
    
    SET_GRAYED_CONTROL( IDC_HAS_DIRECTIONAL_SHADOWS2);
    SET_GRAYED_CONTROL( IDC_HAS_PRECISE_SHADOWS2);      
    SET_GRAYED_CONTROL( IDC_HAS_DIRECTIONAL_AMBIENT2);      
    SET_GRAYED_CONTROL( IDC_NO_PLANE_DIFFUSION2);
    SET_GRAYED_CONTROL( IDC_DARK_CORNERS2);
    SET_GRAYED_CONTROL( IDC_DYNAMIC_LIGHTS_ONLY2);
    SET_GRAYED_CONTROL( IDC_IS_LIGHT_BEAM_PASSABLLE2);
    SET_GRAYED_CONTROL( IDC_IS_RECEIVING_SHADOWS2);
    SET_GRAYED_CONTROL( IDC_NO_DYNAMIC_LIGHTS2);
    SET_GRAYED_CONTROL( IDC_NO_SHADOW2);
    
    SET_GRAYED_CONTROL( IDC_CLAMP_U2);
    SET_GRAYED_CONTROL( IDC_CLAMP_V2);
    SET_GRAYED_CONTROL( IDC_REFLECTIVE2);
    SET_GRAYED_CONTROL( IDC_AFTER_SHADOW2);
    m_ctrlMultiplyColor.SetMixedColor();
  }

  CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgFilterPolygonSelection)
	DDX_Control(pDX, ID_COMBINE_COLOR, m_ctrlMultiplyColor);
	DDX_Control(pDX, IDC_FILTER_POLYGON_MIRROR, m_ctrlFilterPolygonMirror);
	DDX_Control(pDX, IDC_FILTER_POLYGON_SURFACE, m_ctrFilterPolygonSurface);
	DDX_Control(pDX, IDC_FILTER_CLUSTER_MEMORY, m_ctrlClusterMemoryCombo);
	DDX_Control(pDX, IDC_FILTER_CLUSTER_SIZE, m_ctrlClusterSizeCombo);
	DDX_Text(pDX, IDC_MIN_H, m_fMinH);
	DDX_Text(pDX, IDC_MIN_P, m_fMinP);
	DDX_Text(pDX, IDC_MAX_H, m_fMaxH);
	DDX_Text(pDX, IDC_MAX_P, m_fMaxP);
	DDX_Text(pDX, IDC_MIN_X, m_fMinX);
	DDX_Text(pDX, IDC_MIN_Y, m_fMinY);
	DDX_Text(pDX, IDC_MIN_Z, m_fMinZ);
	DDX_Text(pDX, IDC_MAX_X, m_fMaxX);
	DDX_Text(pDX, IDC_MAX_Y, m_fMaxY);
	DDX_Text(pDX, IDC_MAX_Z, m_fMaxZ);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    
    ULONG ulMask = 0;
    ULONG ulValue = 0;
    ULONG ulTexMask = 0;
    ULONG ulTexValue = 0;

    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_PORTAL2, BPOF_PORTAL);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_OLD_PORTAL2, OPOF_PORTAL);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_OCCLUDER2, BPOF_OCCLUDER);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_PASSABLE2, BPOF_PASSABLE);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_INVISIBLE2, BPOF_INVISIBLE);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_DETAIL2, BPOF_DETAILPOLYGON);      
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_TRANSLUSCENT2, BPOF_TRANSLUCENT);
    
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_HAS_DIRECTIONAL_SHADOWS2, BPOF_HASDIRECTIONALLIGHT);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_HAS_PRECISE_SHADOWS2, BPOF_ACCURATESHADOWS);      
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_HAS_DIRECTIONAL_AMBIENT2, BPOF_HASDIRECTIONALAMBIENT);      
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_NO_PLANE_DIFFUSION2, BPOF_NOPLANEDIFFUSION);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_DARK_CORNERS2, BPOF_DARKCORNERS);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_DYNAMIC_LIGHTS_ONLY2, BPOF_DYNAMICLIGHTSONLY);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_LIGHT_BEAM_PASSABLLE2, BPOF_DOESNOTCASTSHADOW);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_IS_RECEIVING_SHADOWS2, BPOF_DOESNOTRECEIVESHADOW);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_NO_DYNAMIC_LIGHTS2, BPOF_NODYNAMICLIGHTS);
    ADD_TO_FLAG_MASK( ulMask, ulValue, IDC_NO_SHADOW2, BPOF_FULLBRIGHT);
    
    ADD_TO_FLAG_MASK( ulTexMask, ulTexValue, IDC_CLAMP_U2, BPTF_CLAMPU);
    ADD_TO_FLAG_MASK( ulTexMask, ulTexValue, IDC_CLAMP_V2, BPTF_CLAMPV);
    ADD_TO_FLAG_MASK( ulTexMask, ulTexValue, IDC_REFLECTIVE2,   BPTF_REFLECTION);
    ADD_TO_FLAG_MASK( ulTexMask, ulTexValue, IDC_AFTER_SHADOW2, BPTF_AFTERSHADOW);

    CDynamicContainer<CBrushPolygon> dcPolygons;
    {FOREACHINDYNAMICCONTAINER( pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      dcPolygons.Add( itbpo);
    }}

    INDEX iClusterItem = m_ctrlClusterSizeCombo.GetCurSel();
    INDEX iMemoryItem = m_ctrlClusterMemoryCombo.GetCurSel();
    INDEX iSurface = m_ctrFilterPolygonSurface.GetCurSel();
    // for each of the dynamic container
    {FOREACHINDYNAMICCONTAINER( dcPolygons, CBrushPolygon, itbpo)
    {
      CBrushPolygon &bpo= *itbpo;
      BOOL bDeselect = ((bpo.bpo_ulFlags & ulMask) != ulValue) ||
          ((bpo.bpo_abptTextures[pDoc->m_iTexture].s.bpt_ubFlags & ulTexMask) != ulTexValue) ||
          ((iClusterItem != CB_ERR) && (iClusterItem-4 != bpo.bpo_bppProperties.bpp_sbShadowClusterSize)) ||
          ((iSurface != CB_ERR) && (iSurface!=bpo.bpo_bppProperties.bpp_ubSurfaceType)) ||
          ((iMemoryItem != CB_ERR) && ((1<<(16-iMemoryItem))*BYTES_PER_TEXEL != bpo.bpo_smShadowMap.GetShadowSize())) ||
          (m_ctrlMultiplyColor.IsColorValid() &&
          (m_ctrlMultiplyColor.GetColor()!=bpo.bpo_abptTextures[pDoc->m_iTexture].s.bpt_colColor));
      if( !bDeselect)
      {
        FLOATplane3D &pl=bpo.bpo_pbplPlane->bpl_plAbsolute;
        ANGLE3D ang;
        DirectionVectorToAngles(pl, ang);
        if(ang(1)<m_fMinH || ang(1)>m_fMaxH) bDeselect=TRUE;
        if(ang(2)<m_fMinP || ang(2)>m_fMaxP) bDeselect=TRUE;

        FLOAT3D vMin = bpo.bpo_boxBoundingBox.Min();
        FLOAT3D vMax = bpo.bpo_boxBoundingBox.Max();
        if(vMin(1)>m_fMaxX || vMax(1)<m_fMinX) bDeselect=TRUE;
        if(vMin(2)>m_fMaxY || vMax(2)<m_fMinY) bDeselect=TRUE;
        if(vMin(3)>m_fMaxZ || vMax(3)<m_fMinZ) bDeselect=TRUE;
      }
      
      // deselect if is not acording to filter
      if( bDeselect)
      {
        pDoc->m_selPolygonSelection.Deselect( *itbpo);
      }
    }}


    // refresh selection
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgFilterPolygonSelection, CDialog)
	//{{AFX_MSG_MAP(CDlgFilterPolygonSelection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgFilterPolygonSelection message handlers

BOOL CDlgFilterPolygonSelection::OnInitDialog() 
{
	CDialog::OnInitDialog();

  m_ctrlClusterSizeCombo.AddString( L"3.125 cm");
  m_ctrlClusterSizeCombo.AddString( L"6.25 cm");
  m_ctrlClusterSizeCombo.AddString( L"12.5 cm");
  m_ctrlClusterSizeCombo.AddString( L"25 cm");
  m_ctrlClusterSizeCombo.AddString( L"0.5 m");
  m_ctrlClusterSizeCombo.AddString( L"1 m");
  m_ctrlClusterSizeCombo.AddString( L"2 m");
  m_ctrlClusterSizeCombo.AddString( L"4 m");
  m_ctrlClusterSizeCombo.AddString( L"8 m");
  m_ctrlClusterSizeCombo.AddString( L"16 m");
  m_ctrlClusterSizeCombo.AddString( L"32 m");
  m_ctrlClusterSizeCombo.AddString( L"64 m");
  m_ctrlClusterSizeCombo.AddString( L"128 m");
  m_ctrlClusterSizeCombo.AddString( L"256 m");
  m_ctrlClusterSizeCombo.AddString( L"512 m");
  m_ctrlClusterSizeCombo.AddString( L"1024 m");
  m_ctrlClusterSizeCombo.SetCurSel( -1);

  m_ctrlClusterMemoryCombo.AddString( L"256 kb");
  m_ctrlClusterMemoryCombo.AddString( L"128 kb");
  m_ctrlClusterMemoryCombo.AddString( L"32 kb");
  m_ctrlClusterMemoryCombo.AddString( L"16 kb");
  m_ctrlClusterMemoryCombo.AddString( L"8 kb");
  m_ctrlClusterMemoryCombo.AddString( L"4 kb");
  m_ctrlClusterMemoryCombo.AddString( L"2 kb");
  m_ctrlClusterMemoryCombo.AddString( L"1 kb");
  m_ctrlClusterMemoryCombo.AddString( L"512 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"256 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"128 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"64 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"32 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"16 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"8 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"4 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"2 bytes");
  m_ctrlClusterMemoryCombo.AddString( L"1 byte");
  m_ctrlClusterMemoryCombo.SetCurSel( -1);

  CWorldEditorDoc* pDoc = theApp.GetDocument();
  CTString strFrictionName;
  m_ctrFilterPolygonSurface.ResetContent();
  // add all available surfaces
  for(INDEX iSurface=0; iSurface<MAX_UBYTE; iSurface++)
  {
    strFrictionName = pDoc->m_woWorld.wo_astSurfaceTypes[iSurface].st_strName;
    if( strFrictionName == "") break;
    INDEX iAddedAs = m_ctrFilterPolygonSurface.AddString( CString(strFrictionName));
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
