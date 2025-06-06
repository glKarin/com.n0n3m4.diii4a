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

// DlgAllignVertices.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgAllignVertices.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgAllignVertices dialog


CDlgAllignVertices::CDlgAllignVertices(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAllignVertices::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAllignVertices)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgAllignVertices::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAllignVertices)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgAllignVertices, CDialog)
	//{{AFX_MSG_MAP(CDlgAllignVertices)
	ON_BN_CLICKED(IDC_ALLIGN_X, OnAllignX)
	ON_BN_CLICKED(IDC_ALLIGN_Y, OnAllignY)
	ON_BN_CLICKED(IDC_ALLIGN_Z, OnAllignZ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgAllignVertices message handlers

DOUBLE3D CDlgAllignVertices::GetLastSelectedVertex(void)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  DOUBLE3D vNew=DOUBLE3D(0,0,0);
  FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itvtx)
  {
    vNew=FLOATtoDOUBLE(itvtx->bvx_vAbsolute);
  }
  return vNew;
}

void CDlgAllignVertices::OnAllignX() 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  pDoc->RememberUndo();
  pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);

  DOUBLE3D vVtx=GetLastSelectedVertex();
  FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itvtx)
  {
    DOUBLE3D vNew=FLOATtoDOUBLE(itvtx->bvx_vAbsolute);
    vNew(1)=vVtx(1);
    itvtx->SetAbsolutePosition(vNew);
  }
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
  pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
  pDoc->SetModifiedFlag();
  EndDialog(IDOK);
}

void CDlgAllignVertices::OnAllignY() 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  pDoc->RememberUndo();
  pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);

  DOUBLE3D vVtx=GetLastSelectedVertex();
  FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itvtx)
  {
    DOUBLE3D vNew=FLOATtoDOUBLE(itvtx->bvx_vAbsolute);
    vNew(2)=vVtx(2);
    itvtx->SetAbsolutePosition(vNew);
  }
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
  pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
  pDoc->SetModifiedFlag();
  EndDialog(IDOK);
}

void CDlgAllignVertices::OnAllignZ() 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  pDoc->RememberUndo();
  pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);

  DOUBLE3D vVtx=GetLastSelectedVertex();
  FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itvtx)
  {
    DOUBLE3D vNew=FLOATtoDOUBLE(itvtx->bvx_vAbsolute);
    vNew(3)=vVtx(3);
    itvtx->SetAbsolutePosition(vNew);
  }
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
  pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
  pDoc->SetModifiedFlag();
  EndDialog(IDOK);
}

BOOL CDlgAllignVertices::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN)
  {
    if( pMsg->wParam=='X') OnAllignX();
    if( pMsg->wParam=='Y') OnAllignY();
    if( pMsg->wParam=='Z') OnAllignZ();
  }
	return CDialog::PreTranslateMessage(pMsg);
}
