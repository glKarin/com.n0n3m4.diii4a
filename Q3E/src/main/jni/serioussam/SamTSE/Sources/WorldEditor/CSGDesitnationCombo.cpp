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

// CSGDesitnationCombo.cpp : implementation file
//

#include "stdafx.h"
#include "CSGDesitnationCombo.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCSGDesitnationCombo

CCSGDesitnationCombo::CCSGDesitnationCombo()
{
  // set dummy document ptr
  m_pLastDoc = (CWorldEditorDoc *) 0x12345678;
}

CCSGDesitnationCombo::~CCSGDesitnationCombo()
{
}


BEGIN_MESSAGE_MAP(CCSGDesitnationCombo, CComboBox)
	//{{AFX_MSG_MAP(CCSGDesitnationCombo)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCSGDesitnationCombo message handlers

BOOL CCSGDesitnationCombo::OnIdle(LONG lCount)
{
  // get document ptr
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();

  // if document ptr has changed
  // or if document was closed (pDoc == NULL and pLastDoc != NULL)
  // or if document was changed from last OnIdle, refresh CSG destination combo box
  if( (m_pLastDoc != pDoc) ||
      ((pDoc == NULL) && (m_pLastDoc != NULL)) ||
      ((pDoc != NULL) && !pDoc->m_chDocument.IsUpToDate( m_udComboEntries)) )
  {
    CEntity *penPreviouslySelectedEntity = NULL;
    INDEX iCurrentSelection = GetCurSel();
    if( iCurrentSelection != CB_ERR)
    {
      penPreviouslySelectedEntity = (CEntity *) GetItemData( iCurrentSelection);
    }
    // remove all combo entries
    ResetContent();
    // select entry one by default
    INDEX iSelectedEntryByName = -1;
    INDEX iSelectedEntryByPtr = -1;
    // if document doesn't exist
    if( pDoc == NULL)
    {
      AddString( L"None Available");
    }
    // if document exists
    else
    {
      // for all of the world's entities
      FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
      {
        // if the entity is brush
        CEntity::RenderType rt = iten->GetRenderType();
        if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
        {
          CTString strEntityName = iten->GetName();
          // and it has name property defined
          if( strEntityName != "")
          {
            // add it to CSG destination combo
            INDEX iComboEntry = AddString( CString(strEntityName));
            // set item's data as ptr to current entity
            SetItemData( iComboEntry, (DWORD_PTR)(&*iten));
            // try to select previously selected combo entry
            if( strEntityName == m_strLastSelectedName)
            {
              iSelectedEntryByName = iComboEntry;
            }
            if( &iten.Current() == penPreviouslySelectedEntity)
            {
              iSelectedEntryByPtr = iComboEntry;
            }
          }
        }
      }
      ASSERT( (GetCount() != CB_ERR) && (GetCount() != 0) );
    }
    if( iSelectedEntryByPtr != -1) SetCurSel( iSelectedEntryByPtr);
    else if( iSelectedEntryByName != -1) SetCurSel( iSelectedEntryByName);
    else SetCurSel( 0);
    m_udComboEntries.MarkUpdated();
    m_pLastDoc = pDoc;
  }
  return TRUE;
}

void CCSGDesitnationCombo::SelectBrushEntity( CEntity *penBrush)
{
  // loop all entries in combo box
  for( INDEX i=0; i<GetCount(); i++)
  {
    // if this is searched brush entity
    if( ((CEntity *) GetItemData( i)) == penBrush)
    {
      // select it
      SetCurSel( i);
      m_strLastSelectedName = penBrush->GetName();
      break;
    }
  }
}

CEntity *CCSGDesitnationCombo::GetSelectedBrushEntity(void)
{
  // assure valid brush entity ptr
  OnIdle( 0);

  INDEX iCurrentSelection = GetCurSel();
  if( iCurrentSelection == CB_ERR)
  {
    return NULL;
  }
  return (CEntity *) GetItemData( iCurrentSelection);
}

void CCSGDesitnationCombo::OnSelchange() 
{
  CEntity *penSelected = GetSelectedBrushEntity();
  if( penSelected != NULL)
  {
    m_strLastSelectedName = penSelected->GetName();
  }
} 

void CCSGDesitnationCombo::OnDropdown() 
{
  INDEX ctItems = GetCount();
  if( ctItems == CB_ERR) return;
  
  CRect rectCombo;
  GetWindowRect( &rectCombo);
  
  PIX pixScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
  PIX pixMaxHeight = pixScreenHeight - rectCombo.top;

  CWnd *pwndParent = GetParent();
  if( pwndParent == NULL) return;
  pwndParent->ScreenToClient( &rectCombo);
  PIX pixNewHeight = GetItemHeight(0)*(ctItems+2);
  rectCombo.bottom = rectCombo.top + ClampUp( pixNewHeight, pixMaxHeight);
  MoveWindow( rectCombo);
}
