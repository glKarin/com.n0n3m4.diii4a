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

// DlgSelectByName.cpp : implementation file
//

#include "stdafx.h"
#include "DlgSelectByName.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectByName dialog

#define ENTITYPROPERTY(thisptr, offset, type) (*((type *)(((UBYTE *)thisptr)+offset)))     

CDlgSelectByName::CDlgSelectByName( CWorldEditorDoc *pDoc, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSelectByName::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSelectByName)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

  ASSERT( pDoc != NULL);
  m_pDoc = pDoc;
}


void CDlgSelectByName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSelectByName)
	DDX_Control(pDX, IDC_ENTITY_LIST, m_ListBox);
	//}}AFX_DATA_MAP
  
  // if dialog gives data
  if( pDX->m_bSaveAndValidate)
  {
    // loop all list box's entries
    for( INDEX i=0; i<m_ListBox.GetCount(); i++)
    {
      // obtain entity ptr
      CEntity &penEntity = *((CEntity *) m_ListBox.GetItemData( i));
      // if entity was selected
      if( m_pDoc->m_selEntitySelection.IsSelected( penEntity))
      {
        // and check box is now not checked
        if( m_ListBox.GetCheck( i) == 0)
        {
          // deselect entity
          m_pDoc->m_selEntitySelection.Deselect( penEntity);
        }
      }
      else
      // else if entity was not selected
      {
        // and check box is now checked
        if( m_ListBox.GetCheck( i) == 1)
        {
          // select entity
          m_pDoc->m_selEntitySelection.Select( penEntity);
        }
      }
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgSelectByName, CDialog)
	//{{AFX_MSG_MAP(CDlgSelectByName)
	ON_BN_CLICKED(ID_DESELECT_ALL, OnDeselectAll)
	ON_BN_CLICKED(ID_SELECT_ALL, OnSelectAll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectByName message handlers

BOOL CDlgSelectByName::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  ASSERT( m_pDoc != NULL);
  // for all entities in world
  FOREACHINDYNAMICCONTAINER(m_pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    CTString strEntityName = iten->GetDescription();
    // and it has name property defined
    if( strEntityName != "")
    {
      // add it to list box
      INDEX iListEntry = m_ListBox.AddString( CString(strEntityName));
      // set item's data as ptr to current entity
      m_ListBox.SetItemData( iListEntry, (DWORD_PTR)(&*iten));
      // if current entity is selected
      if( iten->IsSelected( ENF_SELECTED))
      {
        // set check to on
        m_ListBox.SetCheck( iListEntry, 1);
      }
      // if entity is not selected
      else
      {
        // set check to off
        m_ListBox.SetCheck( iListEntry, 0);
      }
    }
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgSelectByName::OnDeselectAll() 
{
  // loop all list box's entries
  for( INDEX i=0; i<m_ListBox.GetCount(); i++)
  {
    // slect check box 
    m_ListBox.SetCheck( i, 0);
  }
  m_ListBox.Invalidate();
}

void CDlgSelectByName::OnSelectAll() 
{
  // loop all list box's entries
  for( INDEX i=0; i<m_ListBox.GetCount(); i++)
  {
    // slect check box 
    m_ListBox.SetCheck( i, 1);
  }
  m_ListBox.Invalidate();
}
