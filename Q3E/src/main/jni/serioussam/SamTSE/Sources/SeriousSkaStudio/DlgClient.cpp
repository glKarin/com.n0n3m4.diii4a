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

// DlgClient.cpp : implementation file
//

#include "stdafx.h"
#include "DlgClient.h"
#include "resource.h"
#include "ModelTreeCtrl.h"
#include "SeriousSkaStudio.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CRect rcDummy = CRect(0,0,0,0);
#define LABEL_STYLES        SS_RIGHT|WS_VISIBLE|SS_LEFT|SS_CENTERIMAGE|WS_CHILD
#define COMBO_STYLES        WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL
#define TEXT_STYLES         WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP
#define CHECK_STYLES        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_CHECKBOX|BS_AUTOCHECKBOX
#define RECT_LEFT_SIDE      CRect(10,YPOS,iDialogWidth/2-10,YPOS+CTRLHEIGTH)
#define RECT_RIGHT_SIDE     CRect(iDialogWidth/2-5,YPOS-1,iDialogWidth-15,YPOS+CTRLHEIGTH+1)
#define RECT_RIGHT_SIDE_CB  CRect(iDialogWidth/2-5,YPOS,iDialogWidth-25,YPOS+CBHEIGHT)
#define RECT_FULL_SIZE      CRect(10,YPOS,iDialogWidth-10,YPOS+CTRLHEIGTH)


#define DLGSHADER theApp.m_dlgBarTreeView.m_dlgShader
#define CBHEIGHT    120
INDEX iCustomControlID;

void CTextureControl::AddControl(CTString strLabelText, INDEX *piItemID)
{
  CRect rcParent;
  DLGSHADER.GetWindowRect(rcParent);
  INDEX iDialogWidth = rcParent.right - rcParent.left - 14;
  INDEX iCtrlID = iCustomControlID++;

  if(txc_Label.Create(L"texture",LABEL_STYLES,RECT_LEFT_SIDE,&DLGSHADER,iCtrlID))
  {
    txc_Label.SetFont(DLGSHADER.GetFont(),FALSE);
    txc_Label.SetWindowText(CString(strLabelText+": "));
  }
  if(txc_Combo.Create(COMBO_STYLES,RECT_RIGHT_SIDE_CB,&DLGSHADER,iCtrlID+1))
  {
    txc_Combo.SetFont(DLGSHADER.GetFont(),FALSE);
    txc_Combo.RememberIDs();
    txc_Combo.SetDataPtr(piItemID);
    txc_Combo.m_strID = strLabelText;
    txc_Combo.SetDroppedWidth(200);
  }
  iCustomControlID++;
}

void CTexCoordControl::AddControl(CTString strLabelText, INDEX *piItemID)
{
  CRect rcParent;
  DLGSHADER.GetWindowRect(rcParent);
  INDEX iDialogWidth = rcParent.right - rcParent.left - 14;
  INDEX iCtrlID = iCustomControlID++;

  if(txcc_Label.Create(L"uvmap",LABEL_STYLES,RECT_LEFT_SIDE,&DLGSHADER,iCtrlID))
  {
    txcc_Label.SetFont(DLGSHADER.GetFont(),FALSE);
    txcc_Label.SetWindowText(CString(strLabelText+": "));
  }
  if(txcc_Combo.Create(COMBO_STYLES,RECT_RIGHT_SIDE_CB,&DLGSHADER,iCtrlID+1))
  {
    txcc_Combo.SetFont(DLGSHADER.GetFont(),FALSE);
    txcc_Combo.SetDataPtr(piItemID);
    txcc_Combo.m_strID = strLabelText;
    txcc_Combo.SetDroppedWidth(200);

  }
  iCustomControlID++;
}

void CColorControl::AddControl(CTString strLabelText, COLOR *pcolColor)
{
  CRect rcParent;
  DLGSHADER.GetWindowRect(rcParent);
  INDEX iDialogWidth = rcParent.right - rcParent.left - 14;
  INDEX iCtrlID = iCustomControlID++;

  if(cc_Label.Create(L"color",LABEL_STYLES,RECT_LEFT_SIDE,&DLGSHADER,iCtrlID))
  {
    cc_Label.SetFont(DLGSHADER.GetFont(),FALSE);
    cc_Label.SetWindowText(CString(strLabelText+": "));
  }
  
  if(cc_Button.Create(L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,RECT_RIGHT_SIDE,&DLGSHADER,iCtrlID+1))
  {
    cc_Button.SetColor(*pcolColor);
    cc_Button.m_strID = strLabelText;
  }
  iCustomControlID++;
}

void CFloatControl::AddControl(CTString strLabelText, FLOAT *pFloat)
{
  CRect rcParent;
  DLGSHADER.GetWindowRect(rcParent);
  INDEX iDialogWidth = rcParent.right - rcParent.left - 14;
  INDEX iCtrlID = iCustomControlID++;

  if(fc_Label.Create(L"float",LABEL_STYLES,RECT_LEFT_SIDE,&DLGSHADER,iCtrlID))
  {
    fc_Label.SetFont(DLGSHADER.GetFont(),FALSE);
    fc_Label.SetWindowText(CString(strLabelText+": "));
  }
  
  if(fc_TextBox.Create(TEXT_STYLES,RECT_RIGHT_SIDE,&DLGSHADER,iCtrlID+1))
  {
    fc_TextBox.SetFont(DLGSHADER.GetFont(),FALSE);
    fc_TextBox.SetDataPtr(pFloat);
    fc_TextBox.m_strID = strLabelText;
  }
  iCustomControlID++;
}

void CFlagControl::AddControl(CTString strLabelText, INDEX iFlagIndex, ULONG ulFlags)
{
  CRect rcParent;
  DLGSHADER.GetWindowRect(rcParent);
  INDEX iDialogWidth = rcParent.right - rcParent.left - 14;
  INDEX iCtrlID = iCustomControlID++;

  if(fc_CheckBox.Create(CString(strLabelText),CHECK_STYLES,RECT_FULL_SIZE,&DLGSHADER,iCtrlID+1))
  {
    fc_CheckBox.SetFont(DLGSHADER.GetFont(),FALSE);
    fc_CheckBox.SetIndex(iFlagIndex,ulFlags);
    fc_CheckBox.m_strID = strLabelText;
  }
  iCustomControlID++;

  /*
  if(fc_Label.Create("",LABEL_STYLES,RECT_LEFT_SIDE,&DLGSHADER,iCtrlID))
  {
    fc_Label.SetFont(DLGSHADER.GetFont(),FALSE);
    fc_Label.SetWindowText((const char*)(strLabelText+": "));
  }
  
  if(fc_CheckBox.Create(CHECK_STYLES,RECT_RIGHT_SIDE,&DLGSHADER,iCtrlID+1))
  {
    fc_CheckBox.SetFont(DLGSHADER.GetFont(),FALSE);
    fc_CheckBox.SetDataPtr(iFlagIndex);
    fc_TextBox.m_strID = strLabelText;
  }
  iCustomControlID++;*/
}

CDlgClient::CDlgClient(CWnd* pParent )
{
}

BEGIN_MESSAGE_MAP(CDlgClient, CDialog)
	//{{AFX_MSG_MAP(CDlgClient)
	ON_BN_CLICKED(IDC_CB_COMPRESION, OnCbCompresion)
	ON_BN_CLICKED(IDC_CB_SECPERFRAME, OnCbSecperframe)
	ON_CBN_SELENDOK(IDC_CB_PARENTBONE, OnSelendokCbParentbone)
	ON_CBN_SELENDOK(IDC_CB_PARENTMODEL, OnSelendokCbParentmodel)
	ON_WM_SHOWWINDOW()
	ON_WM_VSCROLL()
	ON_CBN_SELENDOK(IDC_CB_SHADER, OnSelendokCbShader)
	ON_BN_CLICKED(IDC_BT_CONVERT, OnBtConvert)
	ON_BN_CLICKED(IDC_BT_RELOAD_TEXTURE, OnBtReloadTexture)
	ON_BN_CLICKED(IDC_BT_RECREATE_TEXTURE, OnBtRecreateTexture)
	ON_BN_CLICKED(IDC_BT_BROWSE_TEXTURE, OnBtBrowseTexture)
	ON_BN_CLICKED(IDC_BT_RESET_COLISION, OnBtResetColision)
	ON_BN_CLICKED(IDC_BT_RESET_OFFSET, OnBtResetOffset)
	ON_BN_CLICKED(IDC_BT_CALC_ALLFRAMES_BBOX, OnBtCalcAllframesBbox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgClient message handlers
CModelTreeCtrl &m_Tree = theApp.m_dlgBarTreeView.m_TreeCtrl;
// turn compression on or off
void CDlgClient::OnCbCompresion() 
{
  //CModelTreeCtrl &m_Tree = theApp.m_dlgBarTreeView.m_TreeCtrl;
  INDEX iCompresion = ((CButton*)GetDlgItem(IDC_CB_COMPRESION))->GetCheck();
  // get selected item and his parent
  HTREEITEM hSelected = m_Tree.GetSelectedItem();
  if(hSelected == NULL) return;
  INDEX iSelected = m_Tree.GetItemData(hSelected);
  NodeInfo &niSelected = theApp.aNodeInfo[iSelected];
  HTREEITEM hParent = m_Tree.GetParentItem(hSelected);
  if(hParent == NULL) return;
  INDEX iParent = m_Tree.GetItemData(hParent);
  NodeInfo &niParent = theApp.aNodeInfo[iParent];
  // get type of selected item
  ASSERT(niSelected.ni_iType==NT_ANIMATION);

  CAnimSet *pas = (CAnimSet*)niParent.ni_pPtr;
  Animation *pan = (Animation*)niSelected.ni_pPtr;
  BOOL bAnimCompresion = pan->an_bCompresed;
  // set animation compression
  pan->an_bCompresed = iCompresion;
  // convert only one animation in animset
  if(!theApp.ConvertAnimationInAnimSet(pas,pan)) {
    pan->an_bCompresed = bAnimCompresion;
  }

  // update model instance
  theApp.UpdateRootModelInstance();
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->MarkAsChanged();
}

// enable custom speed
void CDlgClient::OnCbSecperframe() 
{
  INDEX iCustomSpeed = ((CButton*)GetDlgItem(IDC_CB_SECPERFRAME))->GetCheck();
  GetDlgItem(IDC_EB_SECPERFRAME)->EnableWindow(iCustomSpeed);
  // get selected item and his parent
  HTREEITEM hSelected = m_Tree.GetSelectedItem();
  if(hSelected == NULL) return;
  INDEX iSelected = m_Tree.GetItemData(hSelected);
  NodeInfo &niSelected = theApp.aNodeInfo[iSelected];
  HTREEITEM hParent = m_Tree.GetParentItem(hSelected);
  if(hParent == NULL) return;
  INDEX iParent = m_Tree.GetItemData(hParent);
  NodeInfo &niParent = theApp.aNodeInfo[iParent];
  // get type of selected item
  ASSERT(niSelected.ni_iType==NT_ANIMATION);

  CAnimSet *pas = (CAnimSet*)niParent.ni_pPtr;
  Animation *pan = (Animation*)niSelected.ni_pPtr;
  BOOL bCustomSpeed = iCustomSpeed;
  // set animation compression
  pan->an_bCustomSpeed = iCustomSpeed;
  // convert only one animation in animset
  if(!theApp.ConvertAnimationInAnimSet(pas,pan)) {
    pan->an_bCustomSpeed = bCustomSpeed;
  }

  // update model instance
  theApp.UpdateRootModelInstance();
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->MarkAsChanged();
}

// change parent bone of model instance
void CDlgClient::OnSelendokCbParentbone() 
{
  CComboBox *cbParentBone = ((CComboBox*)GetDlgItem(IDC_CB_PARENTBONE));
  if(pmiSelected==NULL) return;

  wchar_t strParentName[256];
  cbParentBone->GetLBText(cbParentBone->GetCurSel(),&strParentName[0]);
  INDEX iNewBoneParent = ska_GetIDFromStringTable(CTString(CStringA(strParentName)));
  if(iNewBoneParent != pmiSelected->mi_iParentBoneID)
  {
    pmiSelected->SetParentBone(iNewBoneParent);
    // pmiSelected->mi_iParentBoneID = iNewBoneParent;
    // save parent model instance file
    //theApp.SaveSmcParent(*pmiSelected,TRUE);
    // update model instance
    theApp.UpdateRootModelInstance();
    CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
    pDoc->MarkAsChanged();
  }
}
// change parent of model instance
void CDlgClient::OnSelendokCbParentmodel() 
{
  CComboBox *cbParentModel = ((CComboBox*)GetDlgItem(IDC_CB_PARENTMODEL));

  wchar_t strParentName[256];
  INDEX iCurSelected = cbParentModel->GetCurSel();
  // no selection
  if(iCurSelected < 0)
    return;
  // get curent selected in combo box
  cbParentModel->GetLBText(iCurSelected,&strParentName[0]);
  // curent selected in combo box has pointer of model instance that is selected
  CModelInstance *pmiNewParent = (CModelInstance*)cbParentModel->GetItemDataPtr(iCurSelected);
  // get curent parent of this model instance
  CModelInstance *pmiOldParent = pmiSelected->GetParent(theApp.GetDocument()->m_ModelInstance);
  // if no parent
  if(pmiOldParent == NULL)
  {
    theApp.ErrorMessage("Can't move root object!");
    return;
  }
  // old parent is same as new parent
  if(pmiNewParent == pmiOldParent)
  {
    theApp.ErrorMessage("New parent is same as old one");
    return;
  }
  // new parent is same as selected model instance
  if(pmiNewParent == pmiSelected)
  {
    theApp.ErrorMessage("Can't attach object to himself!");
    return;
  }

  // check if pmiNewParent if child of pmiSelected
  if(pmiNewParent->GetParent(pmiSelected) != NULL)
  {
    theApp.ErrorMessage("Can't attach object to his child!");
    return;
  }
  CSkeleton *psklParent = pmiNewParent->mi_psklSkeleton;
  SkeletonLOD *psklodParent = NULL;
  INDEX iParentBoneID = 0;
  if(psklParent != NULL)
  {
    // if parent has skeleton lods
    INDEX ctslod = psklParent->skl_aSkeletonLODs.Count();
    if(ctslod > 0)
    {
      psklodParent = &psklParent->skl_aSkeletonLODs[0];
      INDEX ctsb = psklodParent->slod_aBones.Count();
      // if parent has any bones
      if(ctsb > 0)
      {
        SkeletonBone &sb = psklodParent->slod_aBones[0];
        iParentBoneID = sb.sb_iID;
      }
    }
  }
  // change parent of model instance (pmiSelected)
  pmiSelected->ChangeParent(pmiOldParent,pmiNewParent);
  pmiSelected->mi_iParentBoneID = iParentBoneID;
  // if pmiSelected wasn't added as include of parent
  if(pmiSelected->mi_fnSourceFile == pmiOldParent->mi_fnSourceFile)
  {
    pmiSelected->mi_fnSourceFile = pmiNewParent->mi_fnSourceFile;
  }
  // update model instance
  theApp.UpdateRootModelInstance();
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->MarkAsChanged();
}
// change shader for current surface
void CDlgClient::OnSelendokCbShader() 
{
  CComboBox *cbShader = ((CComboBox*)DLGSHADER.GetDlgItem(IDC_CB_SHADER));
  ASSERT(cbShader!=NULL);
  // get selected item and his parent
  HTREEITEM hSelected = m_Tree.GetSelectedItem();
  if(hSelected == NULL) return;
  INDEX iSelected = m_Tree.GetItemData(hSelected);
  NodeInfo &niSelected = theApp.aNodeInfo[iSelected];
  ASSERT(niSelected.ni_iType==NT_MESHSURFACE);
  MeshSurface *pmsrf = (MeshSurface*)niSelected.ni_pPtr;

  // get text of current seleceted item in combo
  wchar_t strName[MAX_PATH];
  cbShader->GetLBText(cbShader->GetCurSel(),strName);
  CTString strNewShaderName = CTString("Shaders\\") + CTString(CStringA(strName)) + ".sha";
  // change shader for all selected mesh surfaces
  theApp.m_dlgBarTreeView.ChangeShaderOnSelectedSurfaces(strNewShaderName);
  // show new controls for new shader
  theApp.m_dlgBarTreeView.SelItemChanged(hSelected);
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->MarkAsChanged();
}

void CDlgClient::OnShowWindow(BOOL bShow, UINT nStatus) 
{
  CComboBox *cbShader = ((CComboBox*)GetDlgItem(IDC_CB_SHADER));
  if((bShow==TRUE) && (cbShader!=NULL))
  {
    // read all shaders files
    CDynamicStackArray<CTFileName> afnShaders;
    MakeDirList( afnShaders, CTString("Shaders\\"), CTString("*.sha"), 0);
    cbShader->ResetContent();
    for(INDEX ifn=0; ifn<afnShaders.Count(); ifn++)
    {
      CTFileName fnShader = afnShaders[ifn];
      cbShader->AddString(CString(fnShader.FileName()));
    }
  }
	CDialog::OnShowWindow(bShow, nStatus);
}

void CDlgClient::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  int iPosition = GetScrollPos(SB_VERT);
  int iMin,iMax;
  GetScrollRange(SB_VERT,&iMin,&iMax);
  if(iMax<=iMin) return;

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	switch( nSBCode )
  {
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
      SetScrollPos(SB_VERT,nPos);
      break;
    }
    case SB_LINEDOWN:
    {
      SetScrollPos(SB_VERT,iPosition+1);
      break;
    }
    case SB_LINEUP:
    {
      SetScrollPos(SB_VERT,iPosition-1);
      break;
    }
    case SB_PAGEDOWN:
    {
      SetScrollPos(SB_VERT,iPosition+(iMax-iMin)/3);
      break;
    }
    case SB_PAGEUP:
    {
      SetScrollPos(SB_VERT,iPosition-(iMax-iMin)/3);
      break;
    }
  }
  theApp.m_dlgBarTreeView.VScrollControls(this);
}

BOOL CDlgClient::PreTranslateMessage(MSG* pMsg) 
{
  if(pMsg->message==WM_KEYDOWN)
  {
    if(((int)pMsg->wParam==VK_ESCAPE) || ((int)pMsg->wParam==VK_RETURN))
    {
      // don't let dialog to get enter or esc key
      return TRUE;
    }
  }
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgClient::OnBtConvert() 
{
  HTREEITEM hSelected = m_Tree.GetSelectedItem();
  // just in case that no item is seleceted
  if(hSelected==NULL) {
    return;
  }

  NodeInfo &niSelected = theApp.m_dlgBarTreeView.GetNodeInfo(hSelected);
  // if selected type is mesh list
  if(niSelected.ni_iType == NT_MESHLODLIST) {

    MeshInstance *pmshi = (MeshInstance*)niSelected.ni_pPtr;
    CTString fnMesh = pmshi->mi_pMesh->GetName();
    // save and convert mesh instance
    if(theApp.SaveMeshListFile(*pmshi,TRUE)) {
      theApp.NotificationMessage("Mesh list file '%s' converted",(const char*)fnMesh);
      theApp.UpdateRootModelInstance();
    } else {
      theApp.ErrorMessage("Cannot convert mesh file '%s'",(const char*)fnMesh);
    }
  // if selected type is skeleton list
  } else if(niSelected.ni_iType == NT_SKELETONLODLIST) {
    // Get pointer to skeleton
    CSkeleton *pskl = (CSkeleton*)niSelected.ni_pPtr;
    CTString fnSkeleton = pskl->GetName();
    // save and convert mesh instance
    if(theApp.SaveSkeletonListFile(*pskl,TRUE)) {
      theApp.NotificationMessage("Skeleton list file '%s' converted",(const char*)fnSkeleton);
      theApp.UpdateRootModelInstance();
    } else {
      theApp.ErrorMessage("Cannot convert skeleton list file '%s'",(const char*)fnSkeleton);
    }
  // if selected type is animset
  } else if(niSelected.ni_iType == NT_ANIMSET) {
    // Get pointer to animset
    CAnimSet *pas = (CAnimSet*)niSelected.ni_pPtr;
    CTString fnAnimSet = pas->GetName();
    // save and convert mesh instance
    if(theApp.SaveAnimSetFile(*pas,TRUE)) {
      theApp.NotificationMessage("AnimSet file '%s' converted",(const char*)fnAnimSet);
      theApp.UpdateRootModelInstance();
    } else {
      theApp.ErrorMessage("Cannot convert animset file '%s'",(const char*)fnAnimSet);
    }
  }
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->MarkAsChanged();
}

void CDlgClient::OnBtReloadTexture() 
{
  HTREEITEM hSelected = m_Tree.GetSelectedItem();
  // just in case that no item is seleceted
  if(hSelected==NULL) {
    return;
  }
  
  NodeInfo &niSelected = theApp.m_dlgBarTreeView.GetNodeInfo(hSelected);
  // is selected item texture instance
  if(niSelected.ni_iType == NT_TEXINSTANCE) {
    TextureInstance *pti = (TextureInstance*)niSelected.ni_pPtr;
    CTextureData *pTexData = (CTextureData*)pti->ti_toTexture.GetData();
    pTexData->Reload();
    theApp.m_dlgBarTreeView.SelItemChanged(hSelected);
  }
}

void CDlgClient::OnBtRecreateTexture() 
{
  HTREEITEM hSelected = m_Tree.GetSelectedItem();
  // just in case that no item is seleceted
  if(hSelected==NULL) {
    return;
  }
  NodeInfo &niSelected = theApp.m_dlgBarTreeView.GetNodeInfo(hSelected);
  // is selected item texture instance
  if(niSelected.ni_iType == NT_TEXINSTANCE) {
    TextureInstance *pti = (TextureInstance*)niSelected.ni_pPtr;
    CTextureData *pTexData = (CTextureData*)pti->ti_toTexture.GetData();
    CTFileName fnTextureName = pTexData->GetName();
    // call (re)create texture dialog
    _EngineGUI.CreateTexture(fnTextureName);
  }
  OnBtReloadTexture();
}

void CDlgClient::OnBtBrowseTexture() 
{
	CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc==NULL) {
    ASSERT(FALSE);
    return;
  }

  HTREEITEM hSelected = m_Tree.GetSelectedItem();;
  if(hSelected==NULL) {
    ASSERT(FALSE);
    return;
  }
  HTREEITEM hParent = m_Tree.GetParentItem(hSelected);
  if(hParent==NULL) {
    ASSERT(FALSE);
    return;
  }

  
  NodeInfo &ni = theApp.m_dlgBarTreeView.GetNodeInfo(hSelected);
  NodeInfo &niParent = theApp.m_dlgBarTreeView.GetNodeInfo(hParent);
  // is selected item texture instance
  if(ni.ni_iType == NT_TEXINSTANCE) {
    // get pointers to texture and mesh instances
    TextureInstance *pti = (TextureInstance*)ni.ni_pPtr;
    MeshInstance *pmshi = (MeshInstance*)niParent.ni_pPtr;
    CModelInstance *pmi = (CModelInstance*)ni.pmi;
    ASSERT(pmi!=NULL);
    ASSERT(pti!=NULL);
    ASSERT(pmshi!=NULL);

    // browse texture
    //CDynamicArray<CTFileName> afnTexture;
    CTFileName fnTexture = _EngineGUI.FileRequester( "Open texture files", 
      FILTER_TEXTURE, "Open directory", "Models\\", "", NULL);
    if(fnTexture=="") {
      return;
    }

    INDEX iTexID = pti->GetID();
    CTextureObject *pto = &pti->ti_toTexture;
    CTString strTexID = ska_GetStringFromTable(iTexID);
    pmi->RemoveTexture(pti,pmshi);
    pmi->AddTexture_t(fnTexture,strTexID,pmshi);

    // update root model instance
    theApp.UpdateRootModelInstance();
    pDoc->MarkAsChanged();
  }
}

void CDlgClient::OnBtResetColision() 
{
	CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  NodeInfo &ni = theApp.m_dlgBarTreeView.GetSelectedNodeInfo();
  if(ni.ni_iType == NT_COLISIONBOX) {
    ColisionBox &cb = *(ColisionBox*)ni.ni_pPtr;
    cb.SetMax(FLOAT3D(0.5,2,0.5));
    cb.SetMin(FLOAT3D(-0.5,0,-0.5));
  } else {
    ASSERT(FALSE);
  }

  theApp.ReselectCurrentItem();
  pDoc->MarkAsChanged();
}

void CDlgClient::OnBtResetOffset() 
{
	CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  NodeInfo &ni = theApp.m_dlgBarTreeView.GetSelectedNodeInfo();
  if(ni.ni_iType == NT_MODELINSTANCE) {
    CModelInstance &mi = *(CModelInstance*)ni.ni_pPtr;
    FLOAT fOffset[6] = {0,0,0,0,0,0};
    mi.SetOffset(fOffset);
  } else {
    ASSERT(FALSE);
  }

  theApp.ReselectCurrentItem();
  pDoc->MarkAsChanged();
}

static FLOATaabbox3D AddAllVerticesToBBox(CModelInstance &mi)
{
  FLOATmatrix3D mat;
  FLOAT3D vPos = FLOAT3D(0,0,0);
  mat.Diagonal(1);
  CStaticStackArray<FLOAT3D> avVertices;
  mi.GetModelVertices(avVertices,mat,vPos,0,0);

  INDEX ctvtx = avVertices.Count();
  // if at least one vertex exists
  FLOATaabbox3D bbox;
  if(ctvtx>0) {
    bbox = FLOATaabbox3D(avVertices[0]);
    // for each vertex after first one
    for(INDEX ivx=1;ivx<ctvtx;ivx++) {
      // add this vertex position to all frames bbox
      bbox |= FLOATaabbox3D(avVertices[ivx]);
    }
  }
  return bbox;
}

static void ClearAnimQueue(CModelInstance &mi)
{
  INDEX ctal = mi.mi_aqAnims.aq_Lists.Count();
  for(INDEX ial=0;ial<ctal;ial++) {
    AnimList &al = mi.mi_aqAnims.aq_Lists[ial];
    al.al_PlayedAnims.Clear();
  }
  mi.mi_aqAnims.aq_Lists.Clear();
}
// Recalculate all frames bounding box for current model
void CDlgClient::OnBtCalcAllframesBbox() 
{
	CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  CModelInstance *pmi = pmiSelected;
  if(pmi == NULL) return;
  FLOATaabbox3D bbox;
  // AnimQueue aqOld = pmi->mi_aqAnims;

  ClearAnimQueue(*pmi);
  pmi->NewClearState(0.0f);
  bbox = AddAllVerticesToBBox(*pmi);
  // for each animset in model instance
  INDEX ctas = pmi->mi_aAnimSet.Count();
  for(INDEX ias=0;ias<ctas;ias++) {
    CAnimSet &as = pmi->mi_aAnimSet[ias];
    // for each animation in animset
    INDEX ctan = as.as_Anims.Count();
    for(INDEX ian=0;ian<ctan;ian++) {
      Animation &an = as.as_Anims[ian];
      AnimQueue &aq = pmi->mi_aqAnims;
      FLOAT fSecPerFrame = an.an_fSecPerFrame;
      INDEX ctFrames = an.an_iFrames;
      ClearAnimQueue(*pmi);
      pmi->NewClearState(0.0f);
      pmi->AddAnimation(an.an_iID,AN_NOGROUP_SORT,1,0);
      ASSERT(aq.aq_Lists.Count()==1);
      FLOAT fNow = aq.aq_Lists[0].al_fStartTime - fSecPerFrame*ctFrames;

      // for each frame in animation
      for(INDEX ifr=0;ifr<ctFrames;ifr++) {
        AnimList &an = aq.aq_Lists[0];
        ASSERT(an.al_PlayedAnims.Count()==1);
        PlayedAnim &pa = an.al_PlayedAnims[0];
        an.al_fStartTime=fNow;
        pa.pa_fStartTime=fNow;
        fNow+=fSecPerFrame;
        bbox |= AddAllVerticesToBBox(*pmi);
      }
    }
  }
  
  pmi->NewClearState(0.0f);
  ClearAnimQueue(*pmi);
  // pmi->mi_aqAnims = aqOld;

  
  // Set bbox in model instance
  FLOAT3D vMin = bbox.Min();
  FLOAT3D vMax = bbox.Max();
  pmi->mi_cbAllFramesBBox.SetMin(vMin);
  pmi->mi_cbAllFramesBBox.SetMax(vMax);

  theApp.ReselectCurrentItem();
  pDoc->MarkAsChanged();
  theApp.NotificationMessage("All frames bounding box recalculated.");
}
