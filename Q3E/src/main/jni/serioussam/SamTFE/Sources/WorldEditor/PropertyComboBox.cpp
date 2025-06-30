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

// PropertyComboBox.cpp : implementation file
//

#include "stdafx.h"
#include "PropertyComboBox.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropertyComboBox

CPropertyComboBox::CPropertyComboBox()
{
}

CPropertyComboBox::~CPropertyComboBox()
{
  // delete current property list
  FORDELETELIST(CPropertyID, pid_lnNode, m_lhProperties, itDel)
  {
    delete &itDel.Current();
  }
}

void CPropertyComboBox::SetDialogPtr( CPropertyComboBar *pDialog)
{
  m_pDialog = pDialog;
  DisableCombo();
}

BEGIN_MESSAGE_MAP(CPropertyComboBox, CComboBox)
	//{{AFX_MSG_MAP(CPropertyComboBox)
	ON_WM_CONTEXTMENU()
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertyComboBox message handlers

void CPropertyComboBox::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	INDEX i=0;
}

void CPropertyComboBox::JoinProperties( CEntity *penEntity, BOOL bIntersect)
{
  // if we should add all of this entity's properties (if this is first entity)
  if( !bIntersect)
  {
    // obtain entity class ptr
    CDLLEntityClass *pdecDLLClass = penEntity->GetClass()->ec_pdecDLLClass;
    // for all classes in hierarchy of this entity
    for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
        // don't add properties with no name
        if( epProperty.ep_strName != CTString("") )
        {
          CAnimData *pAD = NULL;
          // remember anim data
          if( epProperty.ep_eptType == CEntityProperty::EPT_ANIMATION)
          {
            pAD = penEntity->GetAnimData( epProperty.ep_slOffset);
          }
          // create current CPropertyID
          CPropertyID *pPropertyID = new CPropertyID( epProperty.ep_strName,
            epProperty.ep_eptType, &epProperty, pAD);
          // if we should add all of this entity's properties (if this is first entity)
          // and add it into list
          m_lhProperties.AddTail( pPropertyID->pid_lnNode);
        }
      }
    }
  }
  // in case of intersecting properties we should take one of existing properties in list
  // and see if investigating entity has property with same descriptive name
  // If not, remove it that existing property.
  else
  {
    FORDELETELIST(CPropertyID, pid_lnNode, m_lhProperties, itProp)
    {
      CTString strCurrentName = itProp->pid_strName;
      CEntityProperty::PropertyType eptCurrentType = itProp->pid_eptType;
      // mark that property with same name is not found
      BOOL bSameFound = FALSE;

      // obtain entity class ptr
      CDLLEntityClass *pdecDLLClass = penEntity->GetClass()->ec_pdecDLLClass;
      // for all classes in hierarchy of this entity
      for(; pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
      {
        // for all properties
        for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) 
        {
          CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
          CAnimData *pAD = NULL;
          // remember anim data
          if( epProperty.ep_eptType == CEntityProperty::EPT_ANIMATION)
          {
            pAD = penEntity->GetAnimData( epProperty.ep_slOffset);
          }

          // create current CPropertyID
          CPropertyID PropertyID = CPropertyID( epProperty.ep_strName, epProperty.ep_eptType,
            &epProperty, pAD);

          // is this property same as one we are investigating
          if( (strCurrentName == PropertyID.pid_strName) &&
              (eptCurrentType == PropertyID.pid_eptType) )
          {
            // if propperty is enum, enum ptr must also be the same
            if( itProp->pid_eptType == CEntityProperty::EPT_ENUM)
            {
              // only then,
              if( itProp->pid_penpProperty->ep_pepetEnumType == 
                  PropertyID.pid_penpProperty->ep_pepetEnumType)
              {
                // same property is found
                bSameFound = TRUE;
              }
              else
              {
                bSameFound = FALSE;
              }
              goto pcb_OutLoop_JoinProperties;
            }
            // if propperty is animation, anim data ptr must be the same
            else if( itProp->pid_eptType == CEntityProperty::EPT_ANIMATION)
            {
              if(itProp->pid_padAnimData == PropertyID.pid_padAnimData)
              {
                // same property is found
                bSameFound = TRUE;
              }
              else
              {
                bSameFound = FALSE;
              }
              goto pcb_OutLoop_JoinProperties;
            }
            else
            {
              // same property is found
              bSameFound = TRUE;
              goto pcb_OutLoop_JoinProperties;
            }
          }
        }
      }
pcb_OutLoop_JoinProperties:;
      // if property with same name is not found
      if( !bSameFound)
      {
        // remove our investigating property from list
        itProp->pid_lnNode.Remove();
        // and delete it
        delete &itProp.Current();
      }
    }
  }
}

BOOL CPropertyComboBox::OnIdle(LONG lCount)
{
  // get active document 
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();

  // if document ptr has changed
  // or if document was closed (pDoc == NULL and pLastDoc != NULL)
  // or if mode was changed
  // or if selection was changed from last OnIdle, refresh properties
  if( (m_pLastDoc != pDoc) ||
      ((pDoc == NULL) && (m_pLastDoc != NULL)) ||
      ((pDoc != NULL) && (m_iLastMode != pDoc->m_iMode)) ||
      ((pDoc != NULL) && !pDoc->m_chSelections.IsUpToDate( m_udComboEntries)) )
  {
    // refresh selected members message
    if( (pDoc != NULL))
    {
      pDoc->SetStatusLineModeInfoMessage();
    }

    // remove all combo entries
    ResetContent();
    // if document exists and mode is entities
    if( (pDoc != NULL) && (pDoc->m_iMode == ENTITY_MODE) )
    {
      // delete current property list
      FORDELETELIST(CPropertyID, pid_lnNode, m_lhProperties, itDel)
      {
        delete &itDel.Current();
      }

      // lock selection's dynamic container
      pDoc->m_selEntitySelection.Lock();

      // for each of the selected entities
      FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        // if this is first entity in dynamic container
        if( pDoc->m_selEntitySelection.Pointer(0) == iten)
        {
          // add all of its properties into joint list but don't intersect with existing ones
          JoinProperties( iten, FALSE);
        }
        else
        {
          // intersect entity's properties with existing ones
          JoinProperties( iten, TRUE);
        }
      }
      // unlock selection's dynamic container
      pDoc->m_selEntitySelection.Unlock();

      if( pDoc->m_selEntitySelection.Count() != 0)
      {
        // -----------------  Add spawn flags property 
        CPropertyID *ppidSpawnFlags = new CPropertyID( "Spawn flags (Alt+Shift+S)",
          CEntityProperty::EPT_SPAWNFLAGS, NULL, NULL);
        m_lhProperties.AddTail( ppidSpawnFlags->pid_lnNode);
        // -----------------  Add parent entity property 
        CPropertyID *ppidParent = new CPropertyID( "Parent (Alt+Shift+A)",
          CEntityProperty::EPT_PARENT, NULL, NULL);
        m_lhProperties.AddTail( ppidParent->pid_lnNode);
      }

      // if there are some intersecting properties
      if( !m_lhProperties.IsEmpty())
      {
        // add intersecting properties of selected entities into combo box
        FOREACHINLIST(CPropertyID, pid_lnNode, m_lhProperties, itProp)
        {
          char achrShortcutKey[ 64] = "";
          if( itProp->pid_chrShortcutKey != 0)
          {
            sprintf( achrShortcutKey, " (%c)", itProp->pid_chrShortcutKey);
          }
          // add property name and shortcut key
          INDEX iAddedAs = AddString( CString(itProp->pid_strName + achrShortcutKey));
          // set ptr to property ID object
          SetItemData( iAddedAs, (DWORD_PTR) &*itProp);
          // enable combo
          EnableWindow();
        }
      }
      // if there are no properties to choose from
      else
      {
        DisableCombo();
      }
    }
    // if there are no document or application is not in edit entitiy mode
    else
    {
      DisableCombo();
    }

    // index of property that is selected (trying to keep the same property active)
    INDEX iSelectedProperty = 0;
    // for all members in combo box
    for( INDEX iMembers = 0; iMembers<GetCount(); iMembers++)
    {
      CPropertyID *ppidPropertyID = (CPropertyID *) GetItemData( iMembers);
      // if this is valid property
      if( ppidPropertyID != NULL)
      {
        // if name of this property is same as last selected property name
        if( ppidPropertyID->pid_strName == m_strLastPropertyName)
        {
          // mark this property as selected by default
          iSelectedProperty = iMembers;
          break;
        }
      }
    }
    
    // select entity 0 by default
    SetCurSel( iSelectedProperty);
    // mark possible document change
    m_pLastDoc = pDoc;
    // set new mode
    if( pDoc != NULL)
    {
      m_iLastMode = pDoc->m_iMode;
    }
    m_pDialog->ArrangeControls();
    // if edit color property is active
    if( m_pDialog->m_EditColorCtrl.IsWindowVisible())
    {
      // invalidate it so it will be refreshed properly
      m_pDialog->m_EditColorCtrl.Invalidate( FALSE);
    }
    m_udComboEntries.MarkUpdated();
  }

  return TRUE;
}

void CPropertyComboBox::OnSelchange() 
{
  SelectProperty();
}

void CPropertyComboBox::SelectProperty(void)
{
  CPropertyID *ppidPropertyID = (CPropertyID *) GetItemData( GetCurSel());
  // must be valid property
  ASSERT( ppidPropertyID != NULL);
  // remember name of this property as last selected property name
  m_strLastPropertyName = ppidPropertyID->pid_strName;
  // show/hide appropriate editing propterty controls
  m_pDialog->ArrangeControls();
}

void CPropertyComboBox::DisableCombo()
{
  // remove all combo entries
  ResetContent();
  // set default message
  INDEX iAddedAs = AddString( L"None Available");
  // set invalid ptr
  SetItemData( iAddedAs, NULL);
  // disable combo
  EnableWindow( FALSE);
}

void CPropertyComboBox::OnDropdown() 
{
  INDEX ctItems = GetCount();
  if( ctItems == CB_ERR) return;
  
  CRect rectCombo;
  GetWindowRect( &rectCombo);
  
  PIX pixScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
  PIX pixMaxHeight = pixScreenHeight - rectCombo.top;

  m_pDialog->ScreenToClient( &rectCombo);
  PIX pixNewHeight = GetItemHeight(0)*(ctItems+2);
  rectCombo.bottom = rectCombo.top + ClampUp( pixNewHeight, pixMaxHeight);
  MoveWindow( rectCombo);
}
