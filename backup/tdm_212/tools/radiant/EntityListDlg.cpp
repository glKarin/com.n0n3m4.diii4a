/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "qe3.h"
#include "Radiant.h"
#include "EntityListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEntityListDlg g_EntityListDlg;
/////////////////////////////////////////////////////////////////////////////
// CEntityListDlg dialog

void CEntityListDlg::ShowDialog() {
	if (g_EntityListDlg.GetSafeHwnd() == NULL) {
		g_EntityListDlg.Create(IDD_DLG_ENTITYLIST);
	} 
	g_EntityListDlg.UpdateList();
	g_EntityListDlg.ShowWindow(SW_SHOW);

}

CEntityListDlg::CEntityListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEntityListDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEntityListDlg)
	//}}AFX_DATA_INIT
}


void CEntityListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEntityListDlg)
	DDX_Control(pDX, IDC_LIST_ENTITY, m_lstEntity);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LIST_ENTITIES, listEntities);
}

BEGIN_MESSAGE_MAP(CEntityListDlg, CDialog)
	//{{AFX_MSG_MAP(CEntityListDlg)
	ON_BN_CLICKED(IDC_SELECT, OnSelect)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_LBN_SELCHANGE(IDC_LIST_ENTITIES, OnLbnSelchangeListEntities)
	ON_LBN_DBLCLK(IDC_LIST_ENTITIES, OnLbnDblclkListEntities)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntityListDlg message handlers

void CEntityListDlg::OnSelect() 
{
	int index = listEntities.GetCurSel();
	if (index != LB_ERR) {
		entity_t *ent = reinterpret_cast<entity_t*>(listEntities.GetItemDataPtr(index));
		if (ent) {
			Select_Deselect();
			Select_Brush (ent->brushes.onext);
		}
	}
  Sys_UpdateWindows(W_ALL);
}

void CEntityListDlg::UpdateList() {
	listEntities.ResetContent();
	for (entity_t* pEntity=entities.next ; pEntity != &entities ; pEntity=pEntity->next) {
		int index = listEntities.AddString(pEntity->epairs.GetString("name"));
		if (index != LB_ERR) {
			listEntities.SetItemDataPtr(index, (void*)pEntity);
		}
	}
}

void CEntityListDlg::OnSysCommand(UINT nID,  LPARAM lParam) {
	if (nID == SC_CLOSE) {
		DestroyWindow();
	}
}

void CEntityListDlg::OnCancel() {
	DestroyWindow();
}

BOOL CEntityListDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UpdateList();

	CRect rct;
	m_lstEntity.GetClientRect(rct);
	m_lstEntity.InsertColumn(0, "Key", LVCFMT_LEFT, rct.Width() / 2);
	m_lstEntity.InsertColumn(1, "Value", LVCFMT_LEFT, rct.Width() / 2);
	m_lstEntity.DeleteColumn(2);
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEntityListDlg::OnClose() {
	DestroyWindow();
}

void CEntityListDlg::OnLbnSelchangeListEntities()
{
	int index = listEntities.GetCurSel();
	if (index != LB_ERR) {
		m_lstEntity.DeleteAllItems();
		entity_t* pEntity = reinterpret_cast<entity_t*>(listEntities.GetItemDataPtr(index));
	    if (pEntity) {
			int count = pEntity->epairs.GetNumKeyVals();
			for (int i = 0; i < count; i++) {
				int nParent = m_lstEntity.InsertItem(0, pEntity->epairs.GetKeyVal(i)->GetKey());
				m_lstEntity.SetItem(nParent, 1, LVIF_TEXT, pEntity->epairs.GetKeyVal(i)->GetValue(), 0, 0, 0, reinterpret_cast<LPARAM>(pEntity));
			}
		}
	}
}

void CEntityListDlg::OnLbnDblclkListEntities()
{
  OnSelect();
}
