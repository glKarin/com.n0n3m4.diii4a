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

// DlgBrowseByClass.cpp : implementation file
//

#include "stdafx.h"
#include "DlgBrowseByClass.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgBrowseByClass dialog

CDynamicContainer<CEntity> dcEntities;

// property description formats
#define PDF_STRING 0
#define PDF_FLOAT  1
#define PDF_INDEX  2
#define PDF_COLOR  3

#define COLUMN_CLASS 0
#define COLUMN_NAME 1
#define COLUMN_DESCRIPTION 2
#define COLUMN_SECTOR_NAME 3
#define COLUMN_INDEX 4
#define COLUMN_SPAWN_FLAGS 5
#define COLUMN_DISTANCE 6
#define COLUMN_X 7
#define COLUMN_Y 8
#define COLUMN_Z 9
#define COLUMN_H 10 
#define COLUMN_P 11
#define COLUMN_B 12
#define COLUMN_PROPERTY_START 13

CEntity *_penForDistanceSort = NULL;
BOOL _bOfSameClass=FALSE;
INDEX _ctProperties=0;
CDynamicContainer<class CEntity> _tempContainer;
BOOL _bTempContainer=FALSE;

BOOL AreAllEntitiesOfTheSameClass(CDynamicContainer<class CEntity> *penContainer)
{
  // obtain this entity's class ptr
  CEntityClass *pdecClass = NULL;
  // add each entity in container
  {FOREACHINDYNAMICCONTAINER(*penContainer, CEntity, iten)
  {
    // obtain this entity's class ptr
    CEntityClass *pdecCurrentClass = iten->GetClass();
    if( pdecClass==NULL)
    {
      pdecClass=pdecCurrentClass;
    }
    if( pdecClass!=pdecCurrentClass)
    {
      return FALSE;
    }
  }}
  return TRUE;
}

CTString GetPropertyValue(CEntity *pen, CEntityProperty *pepProperty, INDEX &iFormat)
{
  CTString strResult="";
  iFormat=PDF_STRING;
  // see type of changing property
  switch( pepProperty->ep_eptType)
  {
  case CEntityProperty::EPT_FLAGS:
  {
    strResult.PrintF("0x%08x", ENTITYPROPERTY( pen, pepProperty->ep_slOffset, ULONG));
    break;
  }
  case CEntityProperty::EPT_ENUM:
  {
    // obtain enum property description object
    CEntityPropertyEnumType *epEnum = pepProperty->ep_pepetEnumType;
    INDEX iEnum = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, INDEX);
    // search for selected enum
    BOOL bEnumFound=FALSE;
    for(INDEX iEnumItem=0; iEnumItem<epEnum->epet_ctValues; iEnumItem++)
    {
      if(iEnum==epEnum->epet_aepevValues[ iEnumItem].epev_iValue)
      {
        strResult=epEnum->epet_aepevValues[ iEnumItem].epev_strName;
        bEnumFound=TRUE;
      }
    }
    if( !bEnumFound)
    {
      strResult="Invalid enum value!!!";
    }
    break;
  }
  case CEntityProperty::EPT_ANIMATION:
  {
    iFormat=PDF_INDEX;
    INDEX iAnim = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, INDEX);
    CAnimData *pAD = pen->GetAnimData( pepProperty->ep_slOffset);
    if( pAD != NULL)
    {
      CAnimInfo aiInfo;
      pAD->GetAnimInfo(iAnim, aiInfo);
      strResult=aiInfo.ai_AnimName;
    }
    break;
  }
  case CEntityProperty::EPT_ENTITYPTR:
  case CEntityProperty::EPT_PARENT:
  {
    CEntity *penPtr = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, CEntityPointer);
    if( penPtr!=NULL)
    {
      strResult="->"+penPtr->GetName();
    }
    else
    {
      strResult="No target";
    }
    break;
  }
  case CEntityProperty::EPT_FLOAT:
  case CEntityProperty::EPT_RANGE:
  case CEntityProperty::EPT_ANGLE:
  {
    iFormat=PDF_FLOAT;
    strResult.PrintF("%g", ENTITYPROPERTY( pen, pepProperty->ep_slOffset, FLOAT));
    break;
  }
  case CEntityProperty::EPT_ILLUMINATIONTYPE:
  {
    INDEX iIllumination = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, INDEX);
    strResult=pen->en_pwoWorld->wo_aitIlluminationTypes[iIllumination].it_strName;
    break;
  }
  case CEntityProperty::EPT_STRING:
  case CEntityProperty::EPT_STRINGTRANS:
  {
    strResult=ENTITYPROPERTY( pen, pepProperty->ep_slOffset, CTString);
    break;
  }
  case CEntityProperty::EPT_FLOATAABBOX3D:
  {
    FLOATaabbox3D box=ENTITYPROPERTY( pen, pepProperty->ep_slOffset, FLOATaabbox3D);
    strResult.PrintF("(%g,%g,%g)-(%g,%g,%g)",
      box.Min()(1),box.Min()(2),box.Min()(3),
      box.Max()(1),box.Max()(2),box.Max()(3));
    break;
  }
  case CEntityProperty::EPT_ANGLE3D:
  {
    ANGLE3D ang=ENTITYPROPERTY( pen, pepProperty->ep_slOffset, ANGLE3D);
    strResult.PrintF("%g,%g,%g",ang(1),ang(2),ang(3));
    break;
  }
  case CEntityProperty::EPT_INDEX:
  {
    iFormat=PDF_INDEX;
    strResult.PrintF("%d", ENTITYPROPERTY( pen, pepProperty->ep_slOffset, INDEX));
    break;
  }
  case CEntityProperty::EPT_BOOL:
  {
    if(ENTITYPROPERTY( pen, pepProperty->ep_slOffset, BOOL))
    {
      strResult="Yes";
    }
    else
    {
      strResult="No";
    }
    break;
  }
  case CEntityProperty::EPT_COLOR:
  {
    iFormat=PDF_COLOR;
    COLOR col=ENTITYPROPERTY( pen, pepProperty->ep_slOffset, COLOR);
    UBYTE ubR, ubG, ubB;
    UBYTE ubH, ubS, ubV;
    ColorToHSV( col, ubH, ubS, ubV);
    ColorToRGB( col, ubR, ubG, ubB);
    UBYTE ubA = (UBYTE) (col&255);
    strResult.PrintF( "RGB=(%d,%d,%d) HSV=(%d,%d,%d) Alpha=%d", ubR, ubG, ubB, ubH, ubS, ubV, ubA);
    break;
  }
  case CEntityProperty::EPT_FILENAME:
  {
    strResult = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, CTFileName);
    break;
  }
  case CEntityProperty::EPT_FILENAMENODEP:
  {
    strResult = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, CTFileNameNoDep);
    break;
  }
  default:
  {
  }
  }
  return strResult;
}

CTString GetItemValue(CEntity *pen, INDEX iColumn, INDEX &iFormat)
{
  ASSERT(pen!=NULL);
  if(pen==NULL) return CTString("");
  CEntityClass *pecEntityClass = pen->GetClass();
  CDLLEntityClass *pdllecDllEntityClass = pecEntityClass->ec_pdecDLLClass;
  FLOAT3D vOrigin = pen->GetPlacement().pl_PositionVector;
  ANGLE3D vAngles = pen->GetPlacement().pl_OrientationAngle;
  CTString strResult="";
  iFormat=PDF_STRING;
  
  switch( iColumn)
  {
  case COLUMN_INDEX:
  {
    INDEX iIndex=dcEntities.GetIndex(pen);
    strResult.PrintF("%d", iIndex);
    iFormat=PDF_INDEX;
    break;
  }
  case COLUMN_CLASS:
  {
    strResult=pdllecDllEntityClass->dec_strName;
    break;
  }
  case COLUMN_NAME:
  {
    strResult=pen->GetName();
    break;
  }
  case COLUMN_DESCRIPTION:
  {
    strResult=pen->GetDescription();
    break;
  }
  case COLUMN_SECTOR_NAME:
  {
    CBrushSector *pbsc = pen->GetFirstSectorWithName();
    if( pbsc!=NULL)
    {
      strResult=pbsc->bsc_strName;
    }
    break;
  }
  case COLUMN_X:
  {
    strResult.PrintF("%g", vOrigin(1));
    iFormat=PDF_FLOAT;
    break;
  }
  case COLUMN_Y:
  {
    strResult.PrintF("%g", vOrigin(2));
    iFormat=PDF_FLOAT;
    break;
  }
  case COLUMN_Z:
  {
    strResult.PrintF("%g", vOrigin(3));
    iFormat=PDF_FLOAT;
    break;
  }
  case COLUMN_H:
  {
    strResult.PrintF("%g", vAngles(1));
    iFormat=PDF_FLOAT;
    break;
  }
  case COLUMN_P:
  {
    strResult.PrintF("%g", vAngles(2));
    iFormat=PDF_FLOAT;
    break;
  }
  case COLUMN_B:
  {
    strResult.PrintF("%g", vAngles(3));
    iFormat=PDF_FLOAT;
    break;
  }
  case COLUMN_DISTANCE:
  {
    if( _penForDistanceSort != NULL)
    {
      FLOAT3D vSelectedOrigin = _penForDistanceSort->GetPlacement().pl_PositionVector;
      FLOAT3D fDistance = vOrigin-vSelectedOrigin;
      strResult.PrintF("%g", fDistance.Length());
      iFormat=PDF_FLOAT;
    }
    break;
  }
  case COLUMN_SPAWN_FLAGS:
  {
    strResult.PrintF("0x%08X", pen->GetSpawnFlags());
    break;
  }
  // entity properties
  default:
  {
    CDLLEntityClass *pdecDLLClass = pen->GetClass()->ec_pdecDLLClass;
    // for all classes in hierarchy of this entity
    INDEX iPropertyOrder=0;
    for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
        if( pepProperty->ep_strName!=CTString(""))
        {
          if( iPropertyOrder==iColumn-COLUMN_PROPERTY_START)
          {
            strResult=GetPropertyValue(pen, pepProperty, iFormat);
            return strResult;
          }
          iPropertyOrder++;
        }
      }
    }
  }
  }
  return strResult;
}

int CALLBACK SortEntities(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  int iResult = 0;
  CEntity *pen1 = (CEntity *) lParam1;
  CEntity *pen2 = (CEntity *) lParam2;

  INDEX iFormat;
  CTString strEn1=GetItemValue(pen1, lParamSort, iFormat);
  CTString strEn2=GetItemValue(pen2, lParamSort, iFormat);

  if( iFormat==PDF_STRING)
  {
    iResult = stricmp( strEn1, strEn2);
  }
  else if( iFormat==PDF_COLOR)
  {
    INDEX ubR, ubG, ubB;
    INDEX ubH, ubS, ubV;
    INDEX ubA;
    strEn1.ScanF( "RGB=(%d,%d,%d) HSV=(%d,%d,%d) Alpha=%d", &ubR, &ubG, &ubB, &ubH, &ubS, &ubV, &ubA);
    INDEX iVal1=ubV;
    strEn2.ScanF( "RGB=(%d,%d,%d) HSV=(%d,%d,%d) Alpha=%d", &ubR, &ubG, &ubB, &ubH, &ubS, &ubV, &ubA);
    INDEX iVal2=ubV;
    iResult=iVal1>iVal2;
  }
  else
  {
    if( iFormat==PDF_FLOAT)
    {
      FLOAT fEn1=0;
      FLOAT fEn2=0;
      strEn1.ScanF("%g", &fEn1);
      strEn2.ScanF("%g", &fEn2);
      if( fEn1>fEn2) iResult=1; else iResult=-1;
    }
    else if( iFormat==PDF_INDEX)
    {
      INDEX iEn1=0;
      INDEX iEn2=0;
      strEn1.ScanF("%d", &iEn1);
      strEn2.ScanF("%d", &iEn2);
      if( iEn1>iEn2) iResult=1; else iResult=-1;
    }
  }

  if( theApp.m_bInvertClassSort) return -iResult; else return iResult;
}

CDlgBrowseByClass::CDlgBrowseByClass(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgBrowseByClass::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgBrowseByClass)
	m_strEntitiesInVolume = _T("");
	m_bShowVolume = FALSE;
	m_bShowImportants = FALSE;
	//}}AFX_DATA_INIT

  m_bCenterSelected = FALSE;
}

CDlgBrowseByClass::~CDlgBrowseByClass()
{
  dcEntities.Clear();
}

void CDlgBrowseByClass::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  CWorldEditorDoc *pDoc = theApp.GetDocument();

  // if dialog is recieving data
  if( (pDX->m_bSaveAndValidate == FALSE) && (::IsWindow(m_listEntities.m_hWnd)) )
  {
    INDEX ctEntities = m_listEntities.GetItemCount();
    INDEX ctSelectedEntities = m_listEntities.GetSelectedCount();
    char achrEntitiesCount[ 64];
    sprintf( achrEntitiesCount, "Selected entities:     %d            Total entities:     %d", ctSelectedEntities, ctEntities);
    m_strEntitiesInVolume = achrEntitiesCount;
    
    INDEX iSelectedItem = m_listEntities.GetNextItem( -1, LVNI_ALL|LVNI_SELECTED);
    if( iSelectedItem != -1) m_listEntities.EnsureVisible( iSelectedItem, FALSE);
  }

	//{{AFX_DATA_MAP(CDlgBrowseByClass)
	DDX_Control(pDX, IDC_PLUGGINS, m_ctrlPluggins);
	DDX_Control(pDX, IDC_ENTITY_LIST, m_listEntities);
	DDX_Text(pDX, IDC_ENTITIES_IN_VOLUME_T, m_strEntitiesInVolume);
	DDX_Check(pDX, IDC_DISPLAY_VOLUME, m_bShowVolume);
	DDX_Check(pDX, IDC_DISPLAY_IMPORTANTS, m_bShowImportants);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    // clear document selection
    pDoc->m_selEntitySelection.Clear();
    // mark all selected entities in list as selected in document's entity selection
    INDEX iSelectedItem = -1;
    FOREVER
    {
      iSelectedItem = m_listEntities.GetNextItem( iSelectedItem, LVNI_ALL|LVNI_SELECTED);
      if( iSelectedItem == -1)
      {
        break;
      }
      // get selected entity
      CEntity *penEntity = (CEntity *) m_listEntities.GetItemData(iSelectedItem);
      // add entity into normal selection
      pDoc->m_selEntitySelection.Select( *penEntity);
    }
    if( pDoc->m_bBrowseEntitiesMode)
    {
      // clear volume container
      pDoc->m_cenEntitiesSelectedByVolume.Clear();
      // go out of browse by volume mode
      pDoc->OnBrowseEntitiesMode();
    }
    // mark that selections have been changed
    pDoc->m_chSelections.MarkChanged();
  }
}


BEGIN_MESSAGE_MAP(CDlgBrowseByClass, CDialog)
	//{{AFX_MSG_MAP(CDlgBrowseByClass)
	ON_NOTIFY(NM_DBLCLK, IDC_ENTITY_LIST, OnDblclkEntityList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_ENTITY_LIST, OnColumnclickEntityList)
	ON_NOTIFY(NM_RCLICK, IDC_ENTITY_LIST, OnRclickEntityList)
	ON_NOTIFY(NM_CLICK, IDC_ENTITY_LIST, OnClickEntityList)
	ON_BN_CLICKED(ID_REMOVE, OnRemove)
	ON_BN_CLICKED(ID_LEAVE, OnLeave)
	ON_BN_CLICKED(ID_SELECT_ALL, OnSelectAll)
	ON_BN_CLICKED(ID_FEED_VOLUME, OnFeedVolume)
	ON_BN_CLICKED(ID_REVERT, OnRevert)
	ON_BN_CLICKED(ID_SELECT_SECTORS, OnSelectSectors)
	ON_BN_CLICKED(ID_DELETE_BROWSE_BY_CLASS, OnDeleteBrowseByClass)
	ON_BN_CLICKED(IDC_DISPLAY_VOLUME, OnDisplayVolume)
	ON_CBN_SELENDOK(IDC_PLUGGINS, OnSelendokPluggins)
	ON_BN_CLICKED(IDC_DISPLAY_IMPORTANTS, OnDisplayImportants)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgBrowseByClass message handlers

#define INDEX_PERCENTAGE 4
#define CLASS_PERCENTAGE 12
#define NAME_PERCENTAGE 16
#define DESCRIPTION_PERCENTAGE 16
#define SECTOR_NAME_PERCENTAGE 6
#define FLAGS_PERCENTAGE 8
#define DISTANCE_PERCENTAGE 6

void CDlgBrowseByClass::AddEntity( CEntity *pen)
{
  dcEntities.Add( pen);
  // one item to serve for all entities
  LV_ITEM itItem;
  memset(&itItem, 0, sizeof(itItem));
  // all items will be of text type
  itItem.mask = LVIF_TEXT;

  // add entity
  wchar_t achrTemp[256];
  INDEX iEntity = m_listEntities.GetItemCount();
  itItem.iSubItem = COLUMN_CLASS;
  itItem.pszText = achrTemp;
  itItem.iItem = m_listEntities.InsertItem( &itItem);
  m_listEntities.SetItemData( itItem.iItem, (DWORD_PTR) pen);

  for( INDEX iColumn=COLUMN_CLASS; iColumn<COLUMN_PROPERTY_START+_ctProperties; iColumn++)
  {
    itItem.iSubItem = iColumn;
    INDEX iFormat;
    CTString strValue=::GetItemValue(pen, iColumn, iFormat);
    swprintf( achrTemp, L"%s", CString(strValue));
    itItem.pszText = achrTemp;
    m_listEntities.SetItem( &itItem);
  }
}

CDynamicContainer<class CEntity> *CDlgBrowseByClass::GetCurrentContainer(void)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  // if should create and use temp container
  if( _bTempContainer)
  {
    _tempContainer.Clear();
    INDEX ctItems = m_listEntities.GetItemCount();
    for( INDEX iItem=0; iItem<ctItems; iItem++)
    {
      CEntity *pen = (CEntity *) m_listEntities.GetItemData( iItem);
      _tempContainer.Add(pen);
    }
    return &_tempContainer;
  }
  else if( m_bShowVolume)
  {
    return &pDoc->m_cenEntitiesSelectedByVolume;
  }
  else if( pDoc->m_selEntitySelection.Count() > 1)
  {
    return &pDoc->m_selEntitySelection;
  }
  return &pDoc->m_woWorld.wo_cenEntities;
}

void CDlgBrowseByClass::FillListWithEntities(void)
{
  // initialize list columns
  InitializeListColumns();

  CWorldEditorDoc *pDoc = theApp.GetDocument();
  
  CDynamicContainer<class CEntity> *penContainer=GetCurrentContainer();
  _bOfSameClass=AreAllEntitiesOfTheSameClass(penContainer);  

  // empty entities list
  m_listEntities.DeleteAllItems();
  dcEntities.Clear();
  m_listEntities.SetRedraw(FALSE);
  
  // add each entity in container
  {FOREACHINDYNAMICCONTAINER(*penContainer, CEntity, iten)
  {
    // for all non-hidden entities that are not classified into hidden sectors, filter importants if requested
    CBrushSector *pbscSector = iten->GetFirstSector();
    if(!(iten->en_ulFlags&ENF_HIDDEN) &&
        ((pbscSector == NULL) || !(pbscSector->bsc_ulFlags & BSCF_HIDDEN)) &&
        (!m_bShowImportants || iten->IsImportant()))
    {
      AddEntity( &iten.Current());
    }
  }}

  // sort items
  m_listEntities.SortItems( SortEntities, theApp.m_iLastClassSortAplied);

  // select one that was selected before calling dialog
  if( pDoc->m_selEntitySelection.Count() == 1)
  {
    pDoc->m_selEntitySelection.Lock();
    CEntity *penOnly = pDoc->m_selEntitySelection.Pointer(0);
    pDoc->m_selEntitySelection.Unlock();

    INDEX ctItems = m_listEntities.GetItemCount();
    for( INDEX iItem=0; iItem<ctItems; iItem++)
    {
      CEntity *pen = (CEntity *) m_listEntities.GetItemData( iItem);
      if( pen == penOnly)
      {
        m_listEntities.SetItemState( iItem, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        m_listEntities.EnsureVisible( iItem, FALSE);
        break;
      }
    }
  }
  // select first in the list
  else
  {
    m_listEntities.SetItemState( 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
    m_listEntities.EnsureVisible( 0, FALSE);
  }
  m_listEntities.SetRedraw(TRUE);
  m_listEntities.Invalidate(TRUE);
  m_listEntities.UpdateWindow();

  _bTempContainer=FALSE;
  UpdateData( FALSE);
}


void CDlgBrowseByClass::InitializeListColumns(void)
{
  while( m_listEntities.DeleteColumn(0)) {};
  // initialize list headers
  CRect rectListControl;
  m_listEntities.GetClientRect( rectListControl);
  PIX pixEntityClassName  = rectListControl.Width()*CLASS_PERCENTAGE/100;
  PIX pixInstanceName     = rectListControl.Width()*NAME_PERCENTAGE/100;
  PIX pixDescription      = rectListControl.Width()*DESCRIPTION_PERCENTAGE/100;
  PIX pixSectorName       = rectListControl.Width()*SECTOR_NAME_PERCENTAGE/100;
  PIX pixEntityIndex      = rectListControl.Width()*INDEX_PERCENTAGE/100;
  PIX pixSpawnFlags       = rectListControl.Width()*FLAGS_PERCENTAGE/100;
  PIX pixDistance         = rectListControl.Width()*DISTANCE_PERCENTAGE/100;
  m_listEntities.InsertColumn( COLUMN_CLASS, L"Class", LVCFMT_LEFT, pixEntityClassName);
  m_listEntities.InsertColumn( COLUMN_NAME, L"Name", LVCFMT_LEFT, pixInstanceName);
  m_listEntities.InsertColumn( COLUMN_DESCRIPTION, L"Description", LVCFMT_LEFT, pixDescription);
  m_listEntities.InsertColumn( COLUMN_SECTOR_NAME, L"Sector name", LVCFMT_LEFT, pixSectorName);
  m_listEntities.InsertColumn( COLUMN_INDEX, L"No", LVCFMT_LEFT, pixEntityIndex);
  m_listEntities.InsertColumn( COLUMN_SPAWN_FLAGS, L"Spawn Flags", LVCFMT_LEFT, pixSpawnFlags);
  m_listEntities.InsertColumn( COLUMN_DISTANCE, L"Distance", LVCFMT_LEFT, pixDistance);
  PIX pixPlacement = (rectListControl.Width()-
    (pixEntityIndex+pixEntityClassName+pixInstanceName+
    pixDescription+pixSectorName+pixSpawnFlags+pixDistance))/6;
  m_listEntities.InsertColumn( COLUMN_X, L"X", LVCFMT_LEFT, pixPlacement);
  m_listEntities.InsertColumn( COLUMN_Y, L"Y", LVCFMT_LEFT, pixPlacement);
  m_listEntities.InsertColumn( COLUMN_Z, L"Z", LVCFMT_LEFT, pixPlacement);
  m_listEntities.InsertColumn( COLUMN_H, L"H", LVCFMT_LEFT, pixPlacement);
  m_listEntities.InsertColumn( COLUMN_P, L"P", LVCFMT_LEFT, pixPlacement);
  m_listEntities.InsertColumn( COLUMN_B, L"B", LVCFMT_LEFT, pixPlacement);
  
  // add properties if of the same class and not empty selection
  CDynamicContainer<class CEntity> *penContainer=GetCurrentContainer();
  _bOfSameClass=AreAllEntitiesOfTheSameClass(penContainer);
  if( _bOfSameClass && penContainer->Count()!=0)
  {
    _ctProperties=0;
    CEntity *pen = &penContainer->GetFirst();
    CDLLEntityClass *pdecDLLClass = pen->GetClass()->ec_pdecDLLClass;
    // for all classes in hierarchy of this entity
    for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
        if( pepProperty->ep_strName!=CTString(""))
        {
          CSize szText=GetDC()->GetTextExtent(pepProperty->ep_strName);
          PIX pixProperty=szText.cx+8;
          m_listEntities.InsertColumn( COLUMN_PROPERTY_START+_ctProperties, CString(pepProperty->ep_strName), LVCFMT_LEFT, pixProperty);
          _ctProperties++;
        }
      }
    }
  }
}

BOOL CDlgBrowseByClass::OnInitDialog() 
{
  CDialog::OnInitDialog();

  int iScreenX = ::GetSystemMetrics(SM_CXSCREEN);	// screen size
	int iScreenY = ::GetSystemMetrics(SM_CYSCREEN) - 32;
  
  PIX pixSX = 8;
  PIX pixSY = 16;
  
  PIX pixButtonsY = iScreenY-64;
  PIX pixButtonSpacing = 16;
  PIX pixButtonOccupies = (iScreenX-pixSX*2)/9;
  PIX pixButtonHeight = 24;

  MoveWindow( 0, 0, iScreenX, iScreenY);

  // obtain placement of selected entities text ctrl' window
  WINDOWPLACEMENT wpl;
  GetDlgItem( IDC_ENTITIES_IN_VOLUME_T)->GetWindowPlacement( &wpl);
  CRect rect=wpl.rcNormalPosition;

  PIX pixx=rect.left;
  PIX pixw=(iScreenX-pixx-pixSX*2)/4;
  GetDlgItem( IDC_ENTITIES_IN_VOLUME_T)->MoveWindow(pixx, rect.top, pixw, rect.bottom);
  GetDlgItem( IDC_DISPLAY_VOLUME)->MoveWindow(pixx+pixw*1, rect.top, pixw, rect.bottom);
  GetDlgItem( IDC_DISPLAY_IMPORTANTS)->MoveWindow(pixx+pixw*2, rect.top, pixw, rect.bottom);
  GetDlgItem( IDC_PLUGGINS_T)->MoveWindow(pixx+pixw*3, rect.top, pixw/4, rect.bottom);
  GetDlgItem( IDC_PLUGGINS)->MoveWindow(pixx+pixw*3+pixw/4, rect.top-4, pixw*3/4, rect.bottom-4);
  
  pixSY+=24;
  
  m_listEntities.MoveWindow(pixSX, pixSY, iScreenX-pixSX*2, (pixButtonsY-16)-pixSY);

#define MOVE_BUTTON( id, button_collumn)\
  GetDlgItem( id)->MoveWindow( \
  iScreenX-pixButtonOccupies*button_collumn, pixButtonsY,\
  pixButtonOccupies-pixButtonSpacing, pixButtonHeight);
  
  MOVE_BUTTON( ID_DELETE_BROWSE_BY_CLASS, 9);
  MOVE_BUTTON( ID_REMOVE, 8);
  MOVE_BUTTON( ID_LEAVE, 7);
  MOVE_BUTTON( ID_SELECT_ALL, 6);
  MOVE_BUTTON( ID_FEED_VOLUME, 5);
  MOVE_BUTTON( ID_REVERT, 4);
  MOVE_BUTTON( ID_SELECT_SECTORS, 3);
  MOVE_BUTTON( IDOK, 2);
  MOVE_BUTTON( IDCANCEL, 1);

  FillListWithEntities();
  InitializePluggins();
	return TRUE;
}

void CDlgBrowseByClass::OnDblclkEntityList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  CRect rcItem;
  POINT ptMouse;
  GetCursorPos( &ptMouse); 
  m_listEntities.ScreenToClient( &ptMouse);
  
  LVHITTESTINFO htInfo;
  htInfo.pt = ptMouse;
  m_listEntities.SubItemHitTest( &htInfo);
  if( htInfo.iItem != -1)
  {
    m_listEntities.SetItemState( htInfo.iItem, LVIS_SELECTED, LVIS_SELECTED);
  }

  m_bCenterSelected = FALSE;
  *pResult = 0;
  CDialog::OnOK();
}

void CDlgBrowseByClass::OnOK() 
{
  m_bCenterSelected = FALSE;
  CDialog::OnOK();
}

void CDlgBrowseByClass::OnColumnclickEntityList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
  if( theApp.m_iLastClassSortAplied == pNMListView->iSubItem) theApp.m_bInvertClassSort = !theApp.m_bInvertClassSort;
  else                                                        theApp.m_bInvertClassSort = FALSE;

  // if sorting by distance
  if( pNMListView->iSubItem == COLUMN_DISTANCE)
  {
    INDEX iSelectedItem = m_listEntities.GetNextItem( -1, LVNI_ALL|LVNI_SELECTED);
    if( iSelectedItem != -1)
    {
      _penForDistanceSort = (CEntity *) m_listEntities.GetItemData( iSelectedItem);
      FLOAT3D vSelectedOrigin = _penForDistanceSort->GetPlacement().pl_PositionVector;
      for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
      {
        CEntity *penEntity = (CEntity *) m_listEntities.GetItemData(iItem);
        FLOAT3D vCurrentOrigin = penEntity->GetPlacement().pl_PositionVector;
        FLOAT3D fDistance = vSelectedOrigin-vCurrentOrigin;
        
        char achrNumber[16];
        itoa( (int)fDistance.Length(), achrNumber, 10);
        m_listEntities.SetItemText( iItem, COLUMN_DISTANCE, CString(achrNumber));
      }
    }
    else
    {
      _penForDistanceSort = NULL;
    }
  }

  m_listEntities.SortItems( SortEntities, pNMListView->iSubItem);
  theApp.m_iLastClassSortAplied = pNMListView->iSubItem;

  *pResult = 0;
  UpdateData( FALSE);
}

void CDlgBrowseByClass::OnRclickEntityList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;

	*pResult = 0;
  
  CRect rcItem;
  POINT ptMouse;
  GetCursorPos( &ptMouse); 
  m_listEntities.ScreenToClient( &ptMouse);
  
  LVHITTESTINFO htInfo;
  htInfo.pt = ptMouse;
  m_listEntities.SubItemHitTest( &htInfo);
  if( htInfo.iItem != -1)
  {
    CString strText = m_listEntities.GetItemText( htInfo.iItem, htInfo.iSubItem);
    
    m_listEntities.SetRedraw(FALSE);
    INDEX ctItems = m_listEntities.GetItemCount();
    for( INDEX iItem = 0; iItem<ctItems; iItem++) 
    {
      if( ((m_listEntities.GetItemText( iItem, htInfo.iSubItem) != strText) && !bShift) ||
          ((m_listEntities.GetItemText( iItem, htInfo.iSubItem) == strText) && bShift) )
      {
        m_listEntities.DeleteItem( iItem);
        iItem--;
        ctItems--;
      }
    }
    m_listEntities.SetRedraw(TRUE);
  }
  _bTempContainer=TRUE;
  FillListWithEntities();

  m_listEntities.Invalidate(TRUE);
  m_listEntities.UpdateWindow();
  UpdateData( FALSE);
}

void CDlgBrowseByClass::OnClickEntityList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  CRect rcItem;
  POINT ptMouse;
  GetCursorPos( &ptMouse); 
  m_listEntities.ScreenToClient( &ptMouse);
  
  LVHITTESTINFO htInfo;
  htInfo.pt = ptMouse;
  m_listEntities.SubItemHitTest( &htInfo);
  if( htInfo.iItem != -1)
  {
    m_listEntities.SetItemState( htInfo.iItem, LVIS_SELECTED, LVIS_SELECTED);
  }
  UpdateData( FALSE);
	*pResult = 0;
}

void CDlgBrowseByClass::OnRemove() 
{
  m_listEntities.SetRedraw(FALSE);
  INDEX iSelectedItem = -1;
  FOREVER
  {
    iSelectedItem = m_listEntities.GetNextItem( -1, LVNI_ALL|LVNI_SELECTED);
    if( iSelectedItem == -1)
    {
      break;
    }
    m_listEntities.DeleteItem( iSelectedItem);
  }
  _bTempContainer=TRUE;
  FillListWithEntities();

  m_listEntities.SetRedraw(TRUE);
  m_listEntities.Invalidate(TRUE);
  m_listEntities.UpdateWindow();
  UpdateData( FALSE);
}

void CDlgBrowseByClass::OnLeave() 
{
  m_listEntities.SetRedraw(FALSE);
  INDEX ctItems = m_listEntities.GetItemCount();
  INDEX iItem = 0;
  for( ; iItem<ctItems; iItem++) 
  {
    if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) != LVIS_SELECTED)
    {
      m_listEntities.DeleteItem( iItem);
      iItem--;
      ctItems--;
    }
  }
  
  for( iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
  {
    m_listEntities.SetItemState( iItem, 0, LVIS_SELECTED);
  }
  
  _bTempContainer=TRUE;
  FillListWithEntities();

  m_listEntities.SetRedraw(TRUE);
  m_listEntities.Invalidate(TRUE);
  m_listEntities.UpdateWindow();
  m_listEntities.SetFocus();
  UpdateData( FALSE);
}

void CDlgBrowseByClass::OnSelectAll() 
{
  for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
  {
    m_listEntities.SetItemState( iItem, LVIS_SELECTED, LVIS_SELECTED);
  }
  m_listEntities.SetFocus();
}

void CDlgBrowseByClass::OnFeedVolume() 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();

  if( pDoc->m_bBrowseEntitiesMode)
  {
    // go out of browse by volume mode
    pDoc->OnBrowseEntitiesMode();
    pDoc->m_bBrowseEntitiesMode = FALSE;
  }
  pDoc->m_cenEntitiesSelectedByVolume.Clear();
  for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
  {
    if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) == LVIS_SELECTED)
    {
      // get selected entity
      CEntity *penEntity = (CEntity *) m_listEntities.GetItemData(iItem);
      // add entity into volume container
      pDoc->m_cenEntitiesSelectedByVolume.Add( penEntity);
    }
  }
}

void CDlgBrowseByClass::OnRevert() 
{
  FillListWithEntities();
}

void CDlgBrowseByClass::OnSelectSectors() 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
  {
    if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) == LVIS_SELECTED)
    {
      // get selected entity
      CEntity *penEntity = (CEntity *) m_listEntities.GetItemData(iItem);
      CBrushSector *pbscSector = penEntity->GetFirstSectorWithName();
      if( pbscSector != NULL)
      {
        if( !pbscSector->IsSelected( BSCF_SELECTED))
        {
          pDoc->m_selSectorSelection.Select( *pbscSector);
        }
      }
    }
  }
  pDoc->SetEditingMode( SECTOR_MODE);
  EndDialog( 0);
}

BOOL CDlgBrowseByClass::PreTranslateMessage(MSG* pMsg) 
{
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  if( ((pMsg->message==WM_KEYDOWN) || (pMsg->message==WM_SYSKEYDOWN)) && 
      ((int)pMsg->wParam=='A') && bCtrl)
  {
    OnSelectAll();
  }
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgBrowseByClass::OnDeleteBrowseByClass() 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();

  pDoc->RememberUndo();
  pDoc->ClearSelections();

  CDynamicContainer<CEntity> dcEntitiesToDelete;

  m_listEntities.SetRedraw(FALSE);
  INDEX iSelectedItem = -1;
  FOREVER
  {
    iSelectedItem = m_listEntities.GetNextItem( -1, LVNI_ALL|LVNI_SELECTED);
    if( iSelectedItem == -1)
    {
      break;
    }
    CEntity *penEntity = (CEntity *) m_listEntities.GetItemData(iSelectedItem);
    dcEntitiesToDelete.Add( penEntity);
    m_listEntities.DeleteItem( iSelectedItem);
  }

  // check for deleting terrain
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    CEntity &en=*iten;
    // if it is terrain
    if(en.GetRenderType()==CEntity::RT_TERRAIN)
    {
      // if it is selected terrain
      if(pDoc->m_ptrSelectedTerrain==en.GetTerrain())
      {
        pDoc->m_ptrSelectedTerrain=NULL;
        theApp.m_ctTerrainPage.MarkChanged();
        break;
      }
    }
  }}

  {FOREACHINDYNAMICCONTAINER(dcEntitiesToDelete, CEntity, itenToDelete)
  {
    dcEntities.Remove( itenToDelete);
    // select it
    pDoc->m_selEntitySelection.Select( *itenToDelete);
  }}
  
  // delete all selected entities
  pDoc->m_woWorld.DestroyEntities( pDoc->m_selEntitySelection);
  pDoc->m_selEntitySelection.Clear();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->m_chDocument.MarkChanged();

  m_listEntities.SetRedraw(TRUE);
  m_listEntities.Invalidate(TRUE);
  m_listEntities.UpdateWindow();
  
  // fill again list with non-deleted entities
  FillListWithEntities();

  UpdateData( FALSE);
}

void CDlgBrowseByClass::OnDisplayVolume() 
{
  m_bShowVolume = !m_bShowVolume;
  m_bShowImportants = FALSE;
  UpdateData( FALSE);
  FillListWithEntities();
}

void CDlgBrowseByClass::InitializePluggins(void)
{
  m_ctrlPluggins.AddString(L"None");
  m_ctrlPluggins.AddString(L"Random select");
  m_ctrlPluggins.AddString(L"Random deselect");
  m_ctrlPluggins.AddString(L"Random heading");
  m_ctrlPluggins.AddString(L"Random pitch");
  m_ctrlPluggins.AddString(L"Random banking");
  m_ctrlPluggins.SetCurSel(0);
}

void CDlgBrowseByClass::OnSelendokPluggins() 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  INDEX isel=m_ctrlPluggins.GetCurSel();
  switch(isel)
  {
    // random select
    case 1:
    {
      for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
      {
        if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) != LVIS_SELECTED)
        {
          if( rand()>RAND_MAX/2)
          {
            m_listEntities.SetItemState( iItem, LVIS_SELECTED, LVIS_SELECTED);
          }
        }
      }
      break;
    }
    // random deselect
    case 2:
    {
      for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
      {
        if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) == LVIS_SELECTED)
        {
          if( rand()>RAND_MAX/2)
          {
            m_listEntities.SetItemState( iItem, 0, LVIS_SELECTED);
          }
        }
      }
      break;
    }
    // random heading
    case 3:
    {
      pDoc->RememberUndo();
      for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
      {
        // if is selected
        if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) == LVIS_SELECTED)
        {
          CEntity *pen = (CEntity *) m_listEntities.GetItemData( iItem);
          // if it can be rotated
          if( (pen->GetFlags() & ENF_ANCHORED) == 0)
          {
            CPlacement3D pl=pen->GetPlacement();
            pl.pl_OrientationAngle(1)=((FLOAT)rand())/(float)(RAND_MAX)*360.0f;
            pen->SetPlacement(pl);
          }
        }
      }
      break;
    }
    // random pitch
    case 4:
    {
      pDoc->RememberUndo();
      for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
      {
        // if is selected
        if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) == LVIS_SELECTED)
        {
          CEntity *pen = (CEntity *) m_listEntities.GetItemData( iItem);
          // if it can be rotated
          if( (pen->GetFlags() & ENF_ANCHORED) == 0)
          {
            CPlacement3D pl=pen->GetPlacement();
            pl.pl_OrientationAngle(2)=((FLOAT)rand())/(float)(RAND_MAX)*360.0f;
            pen->SetPlacement(pl);
          }
        }
      }
      break;
    }
    // random banking
    case 5:
    {
      pDoc->RememberUndo();
      for( INDEX iItem = 0; iItem<m_listEntities.GetItemCount(); iItem++) 
      {
        // if is selected
        if( m_listEntities.GetItemState( iItem, LVIS_SELECTED) == LVIS_SELECTED)
        {
          CEntity *pen = (CEntity *) m_listEntities.GetItemData( iItem);
          // if it can be rotated
          if( (pen->GetFlags() & ENF_ANCHORED) == 0)
          {
            CPlacement3D pl=pen->GetPlacement();
            pl.pl_OrientationAngle(3)=((FLOAT)rand())/(float)(RAND_MAX)*360.0f;
            pen->SetPlacement(pl);
          }
        }
      }
      break;
    }
  }
  Invalidate(FALSE);
}

void CDlgBrowseByClass::OnDisplayImportants() 
{
  m_bShowImportants = !m_bShowImportants;
  m_bShowVolume = FALSE;
  UpdateData( FALSE);
  FillListWithEntities();
}
