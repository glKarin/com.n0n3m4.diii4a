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

// DlgLinkTree.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgLinkTree.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgLinkTree dialog


CDlgLinkTree::CDlgLinkTree(CEntity *pen, CPoint pt, BOOL bWhoTargets, BOOL bPropertyNames,
                           CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLinkTree::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgLinkTree)
	m_bClass = FALSE;
	m_bName = FALSE;
	m_bProperty = FALSE;
	m_bWho = FALSE;
	//}}AFX_DATA_INIT
  m_pt=pt;
  m_pen=pen;
  m_bWho=bWhoTargets;
	m_bName=TRUE;
  m_bProperty=bPropertyNames;
  m_bClass=bPropertyNames;
  m_HitItem=NULL;
}


void CDlgLinkTree::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgLinkTree)
	DDX_Control(pDX, IDC_LINK_TREE, m_ctrTree);
	DDX_Check(pDX, IDC_LT_CLASS, m_bClass);
	DDX_Check(pDX, IDC_LT_NAME, m_bName);
	DDX_Check(pDX, IDC_LT_PROPERTY, m_bProperty);
	DDX_Check(pDX, IDC_LT_WHO, m_bWho);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgLinkTree, CDialog)
	//{{AFX_MSG_MAP(CDlgLinkTree)
	ON_NOTIFY(NM_DBLCLK, IDC_LINK_TREE, OnDblclkLinkTree)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_LT_CONTRACT_ALL, OnLtContractAll)
	ON_COMMAND(ID_LT_CONTRACT_BRANCH, OnLtContractBranch)
	ON_COMMAND(ID_LT_EXPAND_ALL, OnLtExpandAll)
	ON_COMMAND(ID_LT_EXPAND_BRANCH, OnLtExpandBranch)
	ON_COMMAND(ID_LT_LEAVE_BRANCH, OnLtLeaveBranch)
	ON_COMMAND(ID_LT_LAST_LEVEL, OnLtLastLevel)
	ON_BN_CLICKED(IDC_LT_CLASS, OnLtClass)
	ON_BN_CLICKED(IDC_LT_NAME, OnLtName)
	ON_BN_CLICKED(IDC_LT_PROPERTY, OnLtProperty)
	ON_BN_CLICKED(IDC_LT_WHO, OnLtWho)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgLinkTree message handlers

CDynamicContainer<CEntity> _penAdded;
BOOL CDlgLinkTree::OnInitDialog() 
{
  InitializeTree();
	return TRUE;
}

void CDlgLinkTree::InitializeTree(void)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
	CDialog::OnInitDialog();
  
  _penAdded.Clear();
  m_ctrTree.DeleteAllItems();
  if( m_pen==NULL || m_pen->IsSelected( ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      CEntity &en=*iten;
      AddEntityPtrsRecursiv( &en, 0, "");
    }
  }
  else
  {
    AddEntityPtrsRecursiv( m_pen, 0, "");
  }

  // get dlg wnd - tree ctrl wnd before resizing
  CRect rectdlg, recttree;
  GetClientRect(recttree);
  GetWindowRect(rectdlg);
  PIX dW=rectdlg.Width()-recttree.Width();
  PIX dH=rectdlg.Height()-recttree.Height();
#define PIX_FLAG_LINE PIX(18)
  // get screen size
  int iScrW = ::GetSystemMetrics(SM_CXSCREEN);	// screen size
	int iScrH = ::GetSystemMetrics(SM_CYSCREEN) - 32;

  // expand all nodes
  HTREEITEM pRootItem = m_ctrTree.GetRootItem();
  ExpandTree(pRootItem, TRUE);
  CRect result=CRect(0,0,0,0);
  CalculateOccupiedSpace(pRootItem, result);
  PIX pixIndent=m_ctrTree.GetIndent();
  PIX pixOffset=pixIndent+1;

  // calculate tree wnd size
  result.left-=pixOffset;
  result.bottom+=pixOffset;
  result.right+=pixOffset;
  PIX pixTreeW=Clamp(PIX(result.Width()), PIX(4*32), iScrW-dW);
  PIX pixTreeH=ClampUp(PIX(result.Height()), iScrH-dH-PIX_FLAG_LINE);

  // now move resulting window so LU-corner would be where mouse was clicked
  CRect rectPopup=CRect(m_pt.x, m_pt.y, m_pt.x+pixTreeW+dW, m_pt.y+pixTreeH+dH+PIX_FLAG_LINE);
  if( rectPopup.right>iScrW)
  {
    rectPopup.left=ClampDn(iScrW-rectPopup.Width(), 0);
    rectPopup.right=iScrW;
  }
  if( rectPopup.bottom>iScrH)
  {
    rectPopup.top=ClampDn( iScrH-rectPopup.Height(), 0);
    rectPopup.bottom=iScrH;
  }
  MoveWindow(rectPopup);
  
  CRect newTreePos;
  newTreePos.left=0;
  newTreePos.top=0;
  newTreePos.right=rectPopup.Width();
  newTreePos.bottom=rectPopup.Height()-dH-PIX_FLAG_LINE;
  m_ctrTree.MoveWindow(newTreePos);

#define MOVE_FLAG(id, no)\
  rectFlag.top=newTreePos.bottom;\
  rectFlag.bottom=newTreePos.bottom+PIX_FLAG_LINE;\
  rectFlag.left=no*32;\
  rectFlag.right=(no+1)*32;\
  GetDlgItem(id)->MoveWindow(rectFlag);
  
  CRect rectFlag;
  MOVE_FLAG(IDC_LT_CLASS, 0);
  MOVE_FLAG(IDC_LT_PROPERTY, 1);
  MOVE_FLAG(IDC_LT_NAME, 2);
  MOVE_FLAG(IDC_LT_WHO, 3);
}

void CDlgLinkTree::AddEntityPtrsRecursiv(CEntity *pen, HTREEITEM hParent, CTString strPropertyName)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( _penAdded.IsMember( pen)) return;
  // Insert entity's ptrs into directory tree
  HTREEITEM InsertedEntity;
  InsertedEntity = m_ctrTree.InsertItem( 0, L"", 0, 0,
    TVIS_SELECTED, TVIF_STATE, 0, hParent, 0);
  m_ctrTree.SetItemData( InsertedEntity, (DWORD_PTR)(pen));
  CTString strText="";
  if( m_bClass)
  {
    CTString strPrev="{";
    CTString strPost="}";
    if( !m_bName && !m_bProperty)
    {
      strPrev="";
      strPost="";
    }
    strText=strPrev+pen->GetClass()->GetName().FileName()+strPost+"    ";
  }
  if( m_bProperty && strPropertyName!="")
  {
    CTString strPrev="<";
    CTString strPost=">";
    if( !m_bName && !m_bClass)
    {
      strPrev="";
      strPost="";
    }
    strText=strText+strPrev+strPropertyName+strPost+"    ";
  }
  if( m_bName)
  {
    strText=strText+pen->GetName();
  }
  m_ctrTree.SetItemText( InsertedEntity, CString(strText));
  _penAdded.Add(pen);
  if(_penAdded.Count()>16) return;

  if( m_bWho)
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      // ---- Add entities that target
      // obtain entity class ptr
      CDLLEntityClass *pdecDLLClass = iten->GetClass()->ec_pdecDLLClass;
      // for all classes in hierarchy of this entity
      for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
      {
        // for all properties
        for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
        {
          CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ENTITYPTR)
          {
            // obtain property ptr
            CEntity *penPtr = ENTITYPROPERTY( &*iten, pepProperty->ep_slOffset, CEntityPointer);
            if( penPtr == pen)
            {
              AddEntityPtrsRecursiv( &*iten, InsertedEntity, pepProperty->ep_strName);
            }
          }
        }
      }
    }
  }
  else
  {
    // ---- Add this entity's non-NULL ptrs recurively
    // obtain entity class ptr
    CDLLEntityClass *pdecDLLClass = pen->GetClass()->ec_pdecDLLClass;
    // for all classes in hierarchy of this entity
    for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
        if( pepProperty->ep_eptType == CEntityProperty::EPT_ENTITYPTR)
        {
          // obtain property ptr
          CEntity *penPtr = ENTITYPROPERTY( pen, pepProperty->ep_slOffset, CEntityPointer);
          if( penPtr != NULL)
          {
            AddEntityPtrsRecursiv( penPtr, InsertedEntity, pepProperty->ep_strName);
          }
        }
      }
    }
  }
}
void CDlgLinkTree::CalculateOccupiedSpace(HTREEITEM hItem, CRect &rect)
{
  CRect rectThis;
  m_ctrTree.GetItemRect( hItem, &rectThis, TRUE);
  rect|=rectThis;
  HTREEITEM pNext=m_ctrTree.GetNextItem( hItem, TVGN_NEXTVISIBLE);
  if( pNext!=NULL)
  {
    CalculateOccupiedSpace(pNext, rect);
  }
}

INDEX _iMaxLevel=-1;
void CDlgLinkTree::ExpandTree(HTREEITEM pItem, BOOL bExpand, INDEX iMaxLevel/*=-1*/, BOOL bNoNextSibling/*=FALSE*/)
{
  _iMaxLevel=iMaxLevel;
  ExpandRecursivly(pItem, bExpand, bNoNextSibling);
}

void CDlgLinkTree::ExpandRecursivly(HTREEITEM pItem, BOOL bExpand, BOOL bNoNextSibling)
{
  BOOL bForceContract=FALSE;
  
  if(_iMaxLevel!=-1 && GetItemLevel(pItem)>=_iMaxLevel) bForceContract=TRUE;
  
  // obtain next child item in branch
  HTREEITEM pCurrent=pItem;
  do
  {
    if( m_ctrTree.ItemHasChildren( pCurrent))
    {
      if( bExpand&&!bForceContract)
      {
        m_ctrTree.Expand( pCurrent, TVE_EXPAND);
      }
      else
      {
        m_ctrTree.Expand( pCurrent, TVE_COLLAPSE);
      }
      // get its child
      HTREEITEM pChild=m_ctrTree.GetNextItem( pCurrent, TVGN_CHILD);
      ExpandRecursivly(pChild, bExpand, FALSE);
    }
    // get next in dir
    if( bNoNextSibling) break;
    pCurrent=m_ctrTree.GetNextItem( pCurrent, TVGN_NEXT);
  }
  while( pCurrent!=NULL);
}

void CDlgLinkTree::OnDblclkLinkTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
  HTREEITEM item = m_ctrTree.GetSelectedItem();
  if( item!=NULL)
  {
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    pDoc->m_selEntitySelection.Clear();
    CEntity *pen=(CEntity *) m_ctrTree.GetItemData(item);
    pDoc->m_selEntitySelection.Select( *pen);
    pDoc->m_chSelections.MarkChanged();
    EndDialog( IDOK);
  }
	*pResult = 0;
}

BOOL CDlgLinkTree::PreTranslateMessage(MSG* pMsg) 
{
  if( (pMsg->message==WM_KEYDOWN) && ((int)pMsg->wParam==192))
  {
    EndDialog( IDOK);
  }
  /*
  if( pMsg->message==WM_MOUSEMOVE)
  {
    UINT nKeys = pMsg->wParam;
    PIX xPos = LOWORD(pMsg->lParam);  // horizontal position of cursor 
    PIX yPos = HIWORD(pMsg->lParam);  // vertical position of cursor 
    CPoint point=CPoint(xPos, yPos);
    OnMouseMove(nKeys, point);
    return TRUE;
  }
  */
  if( pMsg->message==WM_LBUTTONDOWN)
  {
    PIX xPos = LOWORD(pMsg->lParam);  // horizontal position of cursor 
    PIX yPos = HIWORD(pMsg->lParam);  // vertical position of cursor 
    CPoint pt=CPoint(xPos, yPos);
    
    GetWindowRect(m_rectWndOnMouseDown);
    m_ptMouseDown=pt;

    UINT nFlags=(int)pMsg->wParam;
    BOOL bShift = nFlags & MK_SHIFT;
    BOOL bCtrl = nFlags & MK_CONTROL;
    if( bShift||bCtrl)
    {
      OnLButtonDown((int)pMsg->wParam, pt);
      return TRUE;
    }
  }
  if( pMsg->message==WM_RBUTTONDOWN)
  {
    PIX xPos = LOWORD(pMsg->lParam);  // horizontal position of cursor 
    PIX yPos = HIWORD(pMsg->lParam);  // vertical position of cursor 
    CPoint pt=CPoint(xPos, yPos);
    OnRButtonDown((int)pMsg->wParam, pt);
    return TRUE;
  }
  
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgLinkTree::OnLButtonDown(UINT nFlags, CPoint point) 
{
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bCtrl = nFlags & MK_CONTROL;
	
  TVHITTESTINFO testinfo;
  testinfo.pt=point;
  HTREEITEM item=m_ctrTree.HitTest( &testinfo);
  // allow only string clicks
  if( !(testinfo.flags&TVHT_ONITEMLABEL))
  {
    item=NULL;
  }
  m_HitItem=item;

  if(bCtrl&&bShift)
  {
    OnLtContractAll();
  }
  if( m_HitItem!=NULL)
  {
    if(bCtrl)
    {
      OnLtContractBranch();
    }
    else if(bShift)
    {
      OnLtLeaveBranch();
    }
  }

	CDialog::OnLButtonDown(nFlags, point);
}

void CDlgLinkTree::OnRButtonDown(UINT nFlags, CPoint point) 
{
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bCtrl = nFlags & MK_CONTROL;

  TVHITTESTINFO testinfo;
  testinfo.pt=point;
  HTREEITEM item=m_ctrTree.HitTest( &testinfo);
  // allow only string clicks
  if( !(testinfo.flags&TVHT_ONITEMLABEL))
  {
    item=NULL;
  }
  m_HitItem=item;

  if(bCtrl&&bShift)
  {
    OnLtExpandAll();
  }
  
  if( m_HitItem!=NULL)
  {
    if(bCtrl&&!bShift)
    {
      OnLtExpandBranch();
    }
    else if(bShift&&!bCtrl)
    {
      OnLtLastLevel();
    }
  }
  if(!bShift&&!bCtrl)
  {
    CMenu menu;
    if( menu.LoadMenu(IDR_LINK_TREE_POPUP))
    {
      CMenu* pPopup = menu.GetSubMenu(0);
      if( item==NULL)
      {
        pPopup->EnableMenuItem(ID_LT_CONTRACT_BRANCH, MF_DISABLED|MF_GRAYED);
        pPopup->EnableMenuItem(ID_LT_EXPAND_BRANCH, MF_DISABLED|MF_GRAYED);
        pPopup->EnableMenuItem(ID_LT_LAST_LEVEL, MF_DISABLED|MF_GRAYED);
        pPopup->EnableMenuItem(ID_LT_LEAVE_BRANCH, MF_DISABLED|MF_GRAYED);
      }
      ClientToScreen(&point);
      pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								   point.x, point.y, this);
    }
  }
	CDialog::OnRButtonDown(nFlags, point);
}

void CDlgLinkTree::OnLtExpandAll() 
{
  // expand all nodes
  HTREEITEM pRootItem = m_ctrTree.GetRootItem();
  ExpandTree(pRootItem, TRUE);
}

void CDlgLinkTree::OnLtContractAll() 
{
  // contract all nodes
  HTREEITEM pRootItem = m_ctrTree.GetRootItem();
  ExpandTree(pRootItem, FALSE);
}

void CDlgLinkTree::OnLtExpandBranch() 
{
  ExpandTree(m_HitItem, TRUE, 10000, TRUE);	
}

void CDlgLinkTree::OnLtContractBranch() 
{
  ExpandTree(m_HitItem, FALSE, 10000, TRUE);	
}

void CDlgLinkTree::OnLtLastLevel() 
{
  OnLtContractAll();
  INDEX iLevel=GetItemLevel(m_HitItem);
  HTREEITEM pRootItem = m_ctrTree.GetRootItem();
  ExpandTree(pRootItem, TRUE, iLevel);
}

void CDlgLinkTree::OnLtLeaveBranch() 
{
  OnLtContractAll();
  OnLtExpandBranch();	
  
  INDEX iLevel=-1;
  HTREEITEM item = m_HitItem;
  while( item!=NULL)
  {
    item=m_ctrTree.GetParentItem( item);
    if( item!=NULL)
    {
      m_ctrTree.Expand( item, TVE_EXPAND);
    }
  }
}

INDEX CDlgLinkTree::GetItemLevel(HTREEITEM item)
{
  INDEX iLevel=-1;
  while( item!=NULL)
  {
    item=m_ctrTree.GetParentItem( item);
    iLevel++;
  }
  return iLevel;
}

void CDlgLinkTree::SetNewWindowOrigin(void)
{
  CRect rectWnd;
  GetWindowRect(rectWnd);
  m_pt.x=rectWnd.left;
  m_pt.y=rectWnd.top;
}

void CDlgLinkTree::OnLtClass() 
{
  m_bClass=!m_bClass;
  UpdateData(FALSE);
  SetNewWindowOrigin();
  InitializeTree();
}

void CDlgLinkTree::OnLtName() 
{
  m_bName=!m_bName;
  UpdateData(FALSE);
  SetNewWindowOrigin();
  InitializeTree();
}

void CDlgLinkTree::OnLtProperty() 
{
  m_bProperty=!m_bProperty;
  UpdateData(FALSE);
  SetNewWindowOrigin();
  InitializeTree();
}

void CDlgLinkTree::OnLtWho() 
{
  m_bWho=!m_bWho;
  UpdateData(FALSE);
  SetNewWindowOrigin();
  InitializeTree();
}


void CDlgLinkTree::OnMouseMove(UINT nFlags, CPoint point) 
{
/*
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;	
  BOOL bLMB = nFlags & MK_LBUTTON;

  PIX dx=point.x-m_ptMouseDown.x;
  PIX dy=point.y-m_ptMouseDown.y;

  if( bSpace && bLMB)
  {
    CRect rectWnd=m_rectWndOnMouseDown;
    rectWnd.left+=dx;
    rectWnd.right+=dx;
    rectWnd.top+=dy;
    rectWnd.bottom+=dy;
    MoveWindow( rectWnd);
  }

  m_ptLastMouse = point;
*/
	CDialog::OnMouseMove(nFlags, point);
}
