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

// DlgSnapVertex.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgSnapVertex.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgSnapVertex dialog


CDlgSnapVertex::CDlgSnapVertex(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSnapVertex::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSnapVertex)
	m_fX = 0.0f;
	m_fY = 0.0f;
	m_fZ = 0.0f;
	//}}AFX_DATA_INIT
}


void CDlgSnapVertex::DoDataExchange(CDataExchange* pDX)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
	CDialog::DoDataExchange(pDX);

  if( pDX->m_bSaveAndValidate == FALSE)
  {
    CBrushVertex *pvtx=pDoc->m_selVertexSelection.GetFirstInSelection();
    FLOAT3D vFirst=pvtx->bvx_vAbsolute;
    m_fX=vFirst(1);
    m_fY=vFirst(2);
    m_fZ=vFirst(3);
  }

	//{{AFX_DATA_MAP(CDlgSnapVertex)
	DDX_SkyFloat(pDX, IDC_VTX_SNAP_X, m_fX);
	DDX_SkyFloat(pDX, IDC_VTX_SNAP_Y, m_fY);
	DDX_SkyFloat(pDX, IDC_VTX_SNAP_Z, m_fZ);
	//}}AFX_DATA_MAP

  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    CBrushVertex *pvtx=pDoc->m_selVertexSelection.GetFirstInSelection();
    FLOAT3D vFirst=pvtx->bvx_vAbsolute;
    BOOL bValidX, bValidY, bValidZ;
    bValidX=bValidY=bValidZ=TRUE;
    // for each of the dynamic container
    {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itvtx)
    {
      if( itvtx->bvx_vAbsolute(1)!=vFirst(1)) bValidX=FALSE;
      if( itvtx->bvx_vAbsolute(2)!=vFirst(2)) bValidY=FALSE;
      if( itvtx->bvx_vAbsolute(3)!=vFirst(3)) bValidZ=FALSE;
    }}
    if( !bValidX) GetDlgItem(IDC_VTX_SNAP_X)->SetWindowText(L"");
    if( !bValidY) GetDlgItem(IDC_VTX_SNAP_Y)->SetWindowText(L"");
    if( !bValidZ) GetDlgItem(IDC_VTX_SNAP_Z)->SetWindowText(L"");
  }

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  { 
    CString strX, strY, strZ;
    BOOL bApplyX, bApplyY, bApplyZ;
    bApplyX=bApplyY=bApplyZ=FALSE;
    GetDlgItem(IDC_VTX_SNAP_X)->GetWindowText(strX);
    GetDlgItem(IDC_VTX_SNAP_Y)->GetWindowText(strY);
    GetDlgItem(IDC_VTX_SNAP_Z)->GetWindowText(strZ);
    if( strX!="") bApplyX=TRUE;
    if( strY!="") bApplyY=TRUE;
    if( strZ!="") bApplyZ=TRUE;

    if( bApplyX|bApplyY|bApplyZ)
    {
      pDoc->RememberUndo();
      pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);
      // for each of the dynamic container
      {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itvtx)
      {
        DOUBLE3D vNew=FLOATtoDOUBLE(itvtx->bvx_vAbsolute);
        if( bApplyX) vNew(1)=m_fX;
        if( bApplyY) vNew(2)=m_fY;
        if( bApplyZ) vNew(3)=m_fZ;
        itvtx->SetAbsolutePosition(vNew);
      }}
      pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
      pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
      pDoc->UpdateAllViews( NULL);
      pDoc->SetModifiedFlag();
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgSnapVertex, CDialog)
	//{{AFX_MSG_MAP(CDlgSnapVertex)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgSnapVertex message handlers

BOOL CDlgSnapVertex::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN)
  {
    if( pMsg->wParam==VK_ESCAPE)
    {
      EndDialog( IDCANCEL);
      return TRUE;
    }
  }
	
	return CDialog::PreTranslateMessage(pMsg);
}
