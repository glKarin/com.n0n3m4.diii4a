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

// DlgPgPolygon.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgPolygon.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgPolygon property page

IMPLEMENT_DYNCREATE(CDlgPgPolygon, CPropertyPage)

CDlgPgPolygon::CDlgPgPolygon() : CPropertyPage(CDlgPgPolygon::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgPolygon)
	//}}AFX_DATA_INIT
}

CDlgPgPolygon::~CDlgPgPolygon()
{
}

void CDlgPgPolygon::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

  CPropertyPage::DoDataExchange(pDX);
  // mark that property page has been modified
  SetModified( TRUE);

  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL)  return;
  // polygon mode must be on
  if( pDoc->GetEditingMode() != POLYGON_MODE)  return;
  // get flags of control activity
  BOOL bSelectionExists = pDoc->m_selPolygonSelection.Count() != 0;
  
  // if dialog is recieving data and control window is valid
  if( (pDX->m_bSaveAndValidate == FALSE) && IsWindow( m_IsPortal.m_hWnd) )  
  {
    // initialize combo boxes
    InitComboBoxes();

    // polygon controls exist if polygon selection exists
    m_IsPortal.EnableWindow( bSelectionExists);
    m_IsOldPortal.EnableWindow( bSelectionExists);
    m_IsOccluder.EnableWindow( bSelectionExists);
    m_IsPassable.EnableWindow( bSelectionExists);
    m_bStairs.EnableWindow( bSelectionExists);
    m_bShootThru.EnableWindow( bSelectionExists);
    m_IsInvisible.EnableWindow( bSelectionExists);
    m_IsDoubleSided.EnableWindow( bSelectionExists);
    m_bIsDetail.EnableWindow( bSelectionExists);    
    m_IsTranslucent.EnableWindow( bSelectionExists);
    m_IsTransparent.EnableWindow( bSelectionExists);
	  m_ComboMirror.EnableWindow( bSelectionExists);
	  m_ComboFriction.EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_MIRROR)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_FRICTION)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_PRETENDER_DISTANCE)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_PRETENDER_DISTANCE)->EnableWindow( bSelectionExists);

    // if selection exists, calculate tri-state value of attribute intersection
    if( bSelectionExists)
    {
      // get friction, illumination, mirror and effect properties from first polygon
      UBYTE ubFirstFriction;
      UBYTE ubFirstMirror;
      BOOL bSameFriction = TRUE;
      BOOL bSameMirror = TRUE;
      UWORD uwFirstPretenderDistance;
      m_bPretenderDistance = TRUE;

      ULONG ulFlagsOn = MAX_ULONG;
      ULONG ulFlagsOff = MAX_ULONG;

      INDEX iPolygon = 0;
      // for each of the selected polygons
      FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        ulFlagsOn &= itbpo->bpo_ulFlags;
        ulFlagsOff &= ~itbpo->bpo_ulFlags;

        if( iPolygon == 0)
        {
          ubFirstFriction = itbpo->bpo_bppProperties.bpp_ubSurfaceType;
          ubFirstMirror = itbpo->bpo_bppProperties.bpp_ubMirrorType;
          uwFirstPretenderDistance = itbpo->bpo_bppProperties.bpp_uwPretenderDistance;
        }
        else
        {
          if( itbpo->bpo_bppProperties.bpp_ubSurfaceType != ubFirstFriction) bSameFriction = FALSE;
          if( itbpo->bpo_bppProperties.bpp_ubMirrorType != ubFirstMirror) bSameMirror = FALSE;
          if( itbpo->bpo_bppProperties.bpp_uwPretenderDistance != uwFirstPretenderDistance) m_bPretenderDistance = FALSE;
        }
        iPolygon++;
      }

// apply flags to controls
#define SET_TRI_STATE_TO_CTRL( ctrl, flag)\
  if((ulFlagsOn & flag) && !(ulFlagsOff & flag)) ctrl.SetCheck( 1);\
  else if(!(ulFlagsOn & flag) && (ulFlagsOff & flag)) ctrl.SetCheck( 0);\
  else ctrl.SetCheck( 2);

      SET_TRI_STATE_TO_CTRL( m_IsPortal,      BPOF_PORTAL);
      SET_TRI_STATE_TO_CTRL( m_IsOldPortal,   OPOF_PORTAL);
      SET_TRI_STATE_TO_CTRL( m_IsOccluder,    BPOF_OCCLUDER);
      SET_TRI_STATE_TO_CTRL( m_IsPassable,    BPOF_PASSABLE);
      SET_TRI_STATE_TO_CTRL( m_bStairs,       BPOF_STAIRS);
      SET_TRI_STATE_TO_CTRL( m_bShootThru,    BPOF_SHOOTTHRU);
      SET_TRI_STATE_TO_CTRL( m_IsInvisible,   BPOF_INVISIBLE);
      SET_TRI_STATE_TO_CTRL( m_IsDoubleSided, BPOF_DOUBLESIDED);
      SET_TRI_STATE_TO_CTRL( m_bIsDetail,     BPOF_DETAILPOLYGON);      
      SET_TRI_STATE_TO_CTRL( m_IsTranslucent, BPOF_TRANSLUCENT);
      SET_TRI_STATE_TO_CTRL( m_IsTransparent, BPOF_TRANSPARENT);

      if( bSameFriction) m_ComboFriction.SetCurSel( ubFirstFriction);
      else m_ComboFriction.SetCurSel(-1);
      if( bSameMirror) m_ComboMirror.SetCurSel( ubFirstMirror);
      else m_ComboMirror.SetCurSel(-1);
      if( m_bPretenderDistance) m_fPretenderDistance = uwFirstPretenderDistance;
    }
    // mark that page is updated
    m_udPolygonSelection.MarkUpdated();
  }

  //{{AFX_DATA_MAP(CDlgPgPolygon)
	DDX_Control(pDX, IDC_DOUBLESIDED, m_IsDoubleSided);
	DDX_Control(pDX, IDC_SHOOTTROUGH, m_bShootThru);
	DDX_Control(pDX, IDC_IS_TRANSPARENT, m_IsTransparent);
	DDX_Control(pDX, IDC_STAIRS, m_bStairs);
	DDX_Control(pDX, IDC_IS_OCCLUDER, m_IsOccluder);
	DDX_Control(pDX, IDC_MIRROR_COMBO, m_ComboMirror);
	DDX_Control(pDX, IDC_IS_OLD_PORTAL, m_IsOldPortal);
	DDX_Control(pDX, IDC_IS_DETAIL, m_bIsDetail);
	DDX_Control(pDX, IDC_INVISIBLE, m_IsInvisible);
	DDX_Control(pDX, IDC_IS_TRANSLUSCENT, m_IsTranslucent);
	DDX_Control(pDX, IDC_IS_PASSABLE, m_IsPassable);
	DDX_Control(pDX, IDC_IS_PORTAL, m_IsPortal);
	DDX_Control(pDX, IDC_FRICTION_COMBO, m_ComboFriction);
	//}}AFX_DATA_MAP

  DDX_SkyFloat(pDX, IDC_PRETENDER_DISTANCE, m_fPretenderDistance, m_bPretenderDistance);

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    BOOL bFindShadowLayers = FALSE;
    // calculate bounding box for all polygons
    FLOATaabbox3D boxBoundingBoxPolygonSelection;
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      boxBoundingBoxPolygonSelection |= itbpo->bpo_boxBoundingBox;

      if( m_ComboFriction.GetCurSel()!=-1) itbpo->bpo_bppProperties.bpp_ubSurfaceType = m_ComboFriction.GetCurSel();
      INDEX iItemMirror = m_ComboMirror.GetCurSel();
      if( iItemMirror!=CB_ERR)
      {
        if( itbpo->bpo_bppProperties.bpp_ubMirrorType != iItemMirror)
        {
          itbpo->bpo_bppProperties.bpp_ubMirrorType = (UBYTE) iItemMirror;
        }
      }

      // if all polygons have same pretender distance
      if( m_bPretenderDistance)
      {
        itbpo->bpo_bppProperties.bpp_uwPretenderDistance = UWORD (m_fPretenderDistance);
      }

// set polygon's flags acording witg given tri-state ctrl
#define TRI_STATE_CTRL_TO_FLAGS( ctrl, flag, bDiscardShd, bFindShdLayers)\
  if( (ctrl.GetCheck() == 1) && !(itbpo->bpo_ulFlags & flag) ) {\
    itbpo->bpo_ulFlags |= flag;\
    if( bDiscardShd) itbpo->DiscardShadows();\
    if( bFindShdLayers) bFindShadowLayers = TRUE;\
  } else if( (ctrl.GetCheck() == 0) && (itbpo->bpo_ulFlags & flag) ) {\
    itbpo->bpo_ulFlags &= ~flag;\
    if( bDiscardShd) itbpo->DiscardShadows();\
    if( bFindShdLayers) bFindShadowLayers = TRUE;\
  }

      ULONG ulFlagsBefore = itbpo->bpo_ulFlags;
      TRI_STATE_CTRL_TO_FLAGS( m_IsPortal, BPOF_PORTAL, TRUE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsOccluder, BPOF_OCCLUDER, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsOldPortal, OPOF_PORTAL, TRUE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsPassable, BPOF_PASSABLE, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_bStairs, BPOF_STAIRS, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_bShootThru, BPOF_SHOOTTHRU, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsInvisible, BPOF_INVISIBLE, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsDoubleSided, BPOF_DOUBLESIDED, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_bIsDetail, BPOF_DETAILPOLYGON, FALSE, FALSE);      
      TRI_STATE_CTRL_TO_FLAGS( m_IsTranslucent, BPOF_TRANSLUCENT, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsTransparent, BPOF_TRANSPARENT, FALSE, FALSE);
      ULONG ulFlagsAfter = itbpo->bpo_ulFlags;

      // occluder and detail flags can't be on at the same time    
      BOOL bOccluderSet = ((ulFlagsBefore&BPOF_OCCLUDER)!=(ulFlagsAfter&BPOF_OCCLUDER))&&(ulFlagsAfter&BPOF_OCCLUDER);
      BOOL bDetailSet = ((ulFlagsBefore&BPOF_DETAILPOLYGON)!=(ulFlagsAfter&BPOF_DETAILPOLYGON))&&(ulFlagsAfter&BPOF_DETAILPOLYGON);
      BOOL bDoubleSidedSet = ((ulFlagsBefore&BPOF_DOUBLESIDED)!=(ulFlagsAfter&BPOF_DOUBLESIDED))&&(ulFlagsAfter&BPOF_DOUBLESIDED);
    
      // if occluder set
      if( bOccluderSet)
      {
        // turn off detail
        itbpo->bpo_ulFlags &= ~BPOF_DETAILPOLYGON;
        itbpo->bpo_ulFlags |= BPOF_DOESNOTCASTSHADOW;
      }
      // if doublesided set
      if(bDoubleSidedSet)
      {
        // turn on transparent and detail
        itbpo->bpo_ulFlags |= BPOF_TRANSPARENT;
        itbpo->bpo_ulFlags |= BPOF_DETAILPOLYGON;
      }
      // if detail set
      if( bDetailSet)
      {
        // turn off occluder
        itbpo->bpo_ulFlags &= ~BPOF_OCCLUDER;
      }
    }

    // if we should find shadow layers
    if( bFindShadowLayers)
    {
      pDoc->m_woWorld.FindShadowLayers( boxBoundingBoxPolygonSelection);
    }
    // mark that document is changed
    theApp.GetDocument()->SetModifiedFlag( TRUE);
    // redraw to show changes
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgPgPolygon, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgPolygon)
	ON_CBN_SELCHANGE(IDC_FRICTION_COMBO, OnSelchangeFrictionCombo)
	ON_CBN_DROPDOWN(IDC_FRICTION_COMBO, OnDropdownFrictionCombo)
	ON_CBN_SELCHANGE(IDC_MIRROR_COMBO, OnSelchangeMirrorCombo)
	ON_CBN_DROPDOWN(IDC_MIRROR_COMBO, OnDropdownMirrorCombo)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgPolygon message handlers

void CDlgPgPolygon::InitComboBoxes(void)
{
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  CTString strFrictionName;
  CTString strMirrorName;

  m_ComboFriction.ResetContent();
	m_ComboMirror.ResetContent();
  // add all available frictions
  for(INDEX iFriction=0; iFriction<MAX_UBYTE; iFriction++)
  {
    strFrictionName = pDoc->m_woWorld.wo_astSurfaceTypes[iFriction].st_strName;
    if( strFrictionName == "") break;
    INDEX iAddedAs = m_ComboFriction.AddString( CString(strFrictionName));
  }

  // none must exist
  m_ComboMirror.AddString( L"None");

  // if there is polygon selection
  if( pDoc->m_selPolygonSelection.Count() != 0)
  {
    // obtain first polygon's brush
    CBrush3D *pbrBrush = NULL;
    pDoc->m_selPolygonSelection.Lock();
    if( !pDoc->m_selPolygonSelection.IsMember( pDoc->m_pbpoLastCentered))
    {
      pbrBrush = pDoc->m_selPolygonSelection[0].bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush;
    }
    pDoc->m_selPolygonSelection.Unlock();

    BOOL bEnableMirror = TRUE;
    // for each of the selected polygons
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      // disable mirror combo box if all polygons are not from same brush
      if( pbrBrush != itbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush)
      {
        bEnableMirror = FALSE;
        break;
      }
    }
  
    // if mirror combo is enabled
    if( bEnableMirror)
    {
      // add mirrors
      for(INDEX iMirror=1; iMirror<MAX_UBYTE; iMirror++)
      {
        CTString strMirrorName = pbrBrush->br_penEntity->GetMirrorName( iMirror);
        if( strMirrorName == "") break;
        m_ComboMirror.AddString( CString(strMirrorName));
      }
    }
  }
}

BOOL CDlgPgPolygon::OnIdle(LONG lCount)
{
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( (pDoc == NULL) || !IsWindow(m_hWnd) )
  {
    return TRUE;
  }
  // if selections have been changed (they are not up to date)
  if( !pDoc->m_chSelections.IsUpToDate( m_udPolygonSelection) )
  {
    // update dialog data
    UpdateData( FALSE);
  }
  return TRUE;
}

BOOL CDlgPgPolygon::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
  {
    // move data from page to polygon
    UpdateData( TRUE);
    // the message is handled
    return TRUE;
  }
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CDlgPgPolygon::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
  if( IsWindow( m_ComboFriction.m_hWnd))
  {
    InitComboBoxes();
  }
	m_bIsDetail.SetDialogPtr( this);
	m_IsInvisible.SetDialogPtr( this);
	m_IsDoubleSided.SetDialogPtr( this);
	m_IsTranslucent.SetDialogPtr( this);
	m_IsTransparent.SetDialogPtr( this);
	m_IsPassable.SetDialogPtr( this);
	m_bStairs.SetDialogPtr( this);
  m_bShootThru.SetDialogPtr( this);
	m_IsPortal.SetDialogPtr( this);
	m_IsOldPortal.SetDialogPtr( this);
	m_IsOccluder.SetDialogPtr( this);
  return TRUE;
}

void CDlgPgPolygon::OnSelchangeFrictionCombo() 
{
	UpdateData( TRUE);
}

void CDlgPgPolygon::OnDropdownFrictionCombo() 
{
  m_ComboFriction.SetDroppedWidth( 256);
}

void CDlgPgPolygon::OnSelchangeMirrorCombo() 
{
	UpdateData( TRUE);
}

void CDlgPgPolygon::OnDropdownMirrorCombo() 
{
  m_ComboMirror.SetDroppedWidth( 256);
}


void CDlgPgPolygon::OnContextMenu(CWnd* pWnd, CPoint point) 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  
  CMenu menu;
  if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    if( menu.LoadMenu(IDR_INFO_POLYGON_POPUP))
    {
		  CMenu* pPopup = menu.GetSubMenu(0);
      if( pDoc->m_selPolygonSelection.Count() != 1)
      {
        menu.EnableMenuItem(ID_SET_AS_DEFAULT, MF_DISABLED|MF_GRAYED);
      }

      pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								   point.x, point.y, this);
    }
  }
}
