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

// PatchPalette.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchPalette dialog


CPatchPalette::CPatchPalette(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchPalette::IDD, pParent)
{
  //{{AFX_DATA_INIT(CPatchPalette)
	m_PatchName = _T("");
	m_fStretch = 0.0f;
	m_strPatchFile = _T("");
	//}}AFX_DATA_INIT
	m_LastViewUpdated = NULL;
}


void CPatchPalette::DoDataExchange(CDataExchange* pDX)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  
  m_PatchName = "";
  m_strPatchFile = "";
  m_fStretch = 1.0f;
  if( pModelerView != NULL)
  {
    CModelerDoc* pDoc = (CModelerDoc *) pModelerView->GetDocument();
    if( !pDX->m_bSaveAndValidate && 
        (pDoc->m_emEditModel.CountPatches() != 0) )
    {
	    GetDlgItem( IDC_EDIT_PATCH_NAME)->EnableWindow( TRUE);
	    GetDlgItem( IDC_EDIT_PATCH_STRETCH)->EnableWindow( TRUE);
	    GetDlgItem( IDC_PATCH_FILE_T)->EnableWindow( TRUE);
	    GetDlgItem( IDC_PATCH_NAME_T)->EnableWindow( TRUE);
	    GetDlgItem( IDC_PATCH_STRETCH_T)->EnableWindow( TRUE);
      CModelPatch &mp = pDoc->m_emEditModel.edm_md.md_mpPatches[ pModelerView->m_iActivePatchBitIndex];
      m_strPatchFile = mp.mp_toTexture.GetName().FileName();      
      m_PatchName = mp.mp_strName;
      m_fStretch = mp.mp_fStretch;
    }
  }
  
  CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatchPalette)
	DDX_Control(pDX, IDC_PATCH_BUTTON9, m_PatchButton9);
	DDX_Control(pDX, IDC_PATCH_BUTTON8, m_PatchButton8);
	DDX_Control(pDX, IDC_PATCH_BUTTON7, m_PatchButton7);
	DDX_Control(pDX, IDC_PATCH_BUTTON6, m_PatchButton6);
	DDX_Control(pDX, IDC_PATCH_BUTTON5, m_PatchButton5);
	DDX_Control(pDX, IDC_PATCH_BUTTON4, m_PatchButton4);
	DDX_Control(pDX, IDC_PATCH_BUTTON32, m_PatchButton32);
	DDX_Control(pDX, IDC_PATCH_BUTTON31, m_PatchButton31);
	DDX_Control(pDX, IDC_PATCH_BUTTON30, m_PatchButton30);
	DDX_Control(pDX, IDC_PATCH_BUTTON3, m_PatchButton3);
	DDX_Control(pDX, IDC_PATCH_BUTTON29, m_PatchButton29);
	DDX_Control(pDX, IDC_PATCH_BUTTON28, m_PatchButton28);
	DDX_Control(pDX, IDC_PATCH_BUTTON27, m_PatchButton27);
	DDX_Control(pDX, IDC_PATCH_BUTTON26, m_PatchButton26);
	DDX_Control(pDX, IDC_PATCH_BUTTON25, m_PatchButton25);
	DDX_Control(pDX, IDC_PATCH_BUTTON24, m_PatchButton24);
	DDX_Control(pDX, IDC_PATCH_BUTTON23, m_PatchButton23);
	DDX_Control(pDX, IDC_PATCH_BUTTON22, m_PatchButton22);
	DDX_Control(pDX, IDC_PATCH_BUTTON21, m_PatchButton21);
	DDX_Control(pDX, IDC_PATCH_BUTTON20, m_PatchButton20);
	DDX_Control(pDX, IDC_PATCH_BUTTON2, m_PatchButton2);
	DDX_Control(pDX, IDC_PATCH_BUTTON19, m_PatchButton19);
	DDX_Control(pDX, IDC_PATCH_BUTTON18, m_PatchButton18);
	DDX_Control(pDX, IDC_PATCH_BUTTON17, m_PatchButton17);
	DDX_Control(pDX, IDC_PATCH_BUTTON16, m_PatchButton16);
	DDX_Control(pDX, IDC_PATCH_BUTTON15, m_PatchButton15);
	DDX_Control(pDX, IDC_PATCH_BUTTON14, m_PatchButton14);
	DDX_Control(pDX, IDC_PATCH_BUTTON13, m_PatchButton13);
	DDX_Control(pDX, IDC_PATCH_BUTTON12, m_PatchButton12);
	DDX_Control(pDX, IDC_PATCH_BUTTON11, m_PatchButton11);
	DDX_Control(pDX, IDC_PATCH_BUTTON10, m_PatchButton10);
	DDX_Control(pDX, IDC_PATCH_BUTTON1, m_PatchButton1);
	DDX_Text(pDX, IDC_EDIT_PATCH_NAME, m_PatchName);
	DDX_SkyFloat(pDX, IDC_EDIT_PATCH_STRETCH, m_fStretch);
	DDX_Text(pDX, IDC_PATCH_FILE_T, m_strPatchFile);
	//}}AFX_DATA_MAP
  if( (pDX->m_bSaveAndValidate) && ( pModelerView != NULL) )
  {
    CModelerDoc* pDoc = (CModelerDoc *) pModelerView->GetDocument();
    if( pDoc->m_emEditModel.CountPatches() != 0)
    {
      CModelPatch &mp = pDoc->m_emEditModel.edm_md.md_mpPatches[ pModelerView->m_iActivePatchBitIndex];
      pDoc->m_emEditModel.SetPatchStretch(pModelerView->m_iActivePatchBitIndex, m_fStretch);
      mp.mp_strName = CStringA(m_PatchName);
      pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
      pDoc->SetModifiedFlag();
    }
  }
}


BEGIN_MESSAGE_MAP(CPatchPalette, CDialog)
	//{{AFX_MSG_MAP(CPatchPalette)
	ON_EN_CHANGE(IDC_EDIT_PATCH_NAME, OnChangeEditPatchName)
	ON_EN_CHANGE(IDC_EDIT_PATCH_STRETCH, OnChangeEditPatchStretch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchPalette message handlers

BOOL CPatchPalette::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  m_PatchExistIcon = AfxGetApp()->LoadIcon(IDI_PATCH_EXIST);
  m_PatchActiveIcon = AfxGetApp()->LoadIcon(IDI_PATCH_ACTIVE);
  m_PatchInactiveIcon = AfxGetApp()->LoadIcon(IDI_PATCH_INACTIVE);
  ASSERT( m_PatchExistIcon != NULL);
  ASSERT( m_PatchActiveIcon != NULL);
  ASSERT( m_PatchInactiveIcon != NULL);
  
	return TRUE;
}

BOOL CPatchPalette::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if( (pModelerView != m_LastViewUpdated) || theApp.m_bRefreshPatchPalette)
  {
    theApp.m_bRefreshPatchPalette = FALSE;
    UpdateData( FALSE);
    Invalidate( FALSE);
    m_LastViewUpdated = pModelerView;
    if( pModelerView == NULL)
    {
      m_LastViewUpdated = NULL;
	    GetDlgItem( IDC_EDIT_PATCH_NAME)->EnableWindow( FALSE);
	    GetDlgItem( IDC_EDIT_PATCH_STRETCH)->EnableWindow( FALSE);
	    GetDlgItem( IDC_PATCH_NAME_T)->EnableWindow( FALSE);
	    GetDlgItem( IDC_PATCH_STRETCH_T)->EnableWindow( FALSE);
	    GetDlgItem( IDC_PATCH_FILE_T)->EnableWindow( FALSE);
    }
  }

  return TRUE;
}

void CPatchPalette::OnChangeEditPatchName() 
{
	UpdateData(TRUE);	
}

void CPatchPalette::OnChangeEditPatchStretch() 
{
	UpdateData(TRUE);	
}
