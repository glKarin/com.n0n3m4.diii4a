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

// DlgBarTreeView.cpp: implementation of the CDlgBarTreeView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SeriousSkaStudio.h"
#include "DlgBarTreeView.h"
#include "resource.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "DlgTemplate.h"
#include <Engine\Base\Stream.h>
#include <Engine\Ska\Skeleton.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// array of depth in recursion to selected item
CStaticStackArray<INDEX> _aSelectItem;
// array of selected mesh surfaces
CStaticStackArray<INDEX> _aiSelectedMeshSurfaces;


CStaticArray<class CTextureControl> _atxcTexControls;
CStaticArray<class CTexCoordControl> _atxccTexCoordControls;
CStaticArray<class CFloatControl> _afcFloatControls;
CStaticArray<class CFlagControl>  _afcFlagControls;
CStaticArray<class CColorControl> _accColors;
extern INDEX iCustomControlID;

BEGIN_MESSAGE_MAP(CDlgBarTreeView, CDlgTemplate)
	//{{AFX_MSG_MAP(CDlgBarTreeView)
	ON_NOTIFY(TCN_SELCHANGE, IDC_MODE_SELECT_TAB, OnSelchangeModeSelectTab)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDlgBarTreeView::CDlgBarTreeView()
{
  pdlgCurrent = NULL;
}
CDlgBarTreeView::~CDlgBarTreeView()
{
}
// returns info of item in tree view
NodeInfo &CDlgBarTreeView::GetNodeInfo(HTREEITEM hItem)
{
  ASSERT(hItem!=NULL);
  INDEX iNodeIndex = m_TreeCtrl.GetItemData(hItem);
  return theApp.aNodeInfo[iNodeIndex];
}

// returns infog of selected item in tree view
NodeInfo &CDlgBarTreeView::GetSelectedNodeInfo()
{
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  ASSERT(hSelected!=NULL);
  // aditional check
  INDEX iNodeIndex = 0;
  if(hSelected!=NULL) {
    iNodeIndex = m_TreeCtrl.GetItemData(hSelected);
  }
  return theApp.aNodeInfo[iNodeIndex];
}


// chose control group to show
void CDlgBarTreeView::ShowControlGroup(INDEX iGroup)
{
  CDialog *pLastVisibleDlg = pdlgCurrent;
  switch(iGroup)
  {
    case GR_PARENT:
      pdlgCurrent = &m_dlgParent;
    break;
    case GR_ANIMSET:
      pdlgCurrent = &m_dlgAnimSet;
    break;
    case GR_COLISION:
      pdlgCurrent = &m_dlgColision;
    break;
    case GR_ALLFRAMES_BBOX:
      pdlgCurrent = &m_dlgAllFrames;
    break;
    case GR_LOD:
      pdlgCurrent = &m_dlgLod;
    break;
    case GR_BONE:
      pdlgCurrent = &m_dlgBone;
    break;
    case GR_TEXTURE:
      pdlgCurrent = &m_dlgTexture;
    break;
    case GR_LISTOPTIONS:
      pdlgCurrent = &m_dlgListOpt;
    break;
    case GR_SHADERS:
      pdlgCurrent = &m_dlgShader;
    break;
    default:
      pdlgCurrent = NULL;
    break;
  }
  if(pLastVisibleDlg!=pdlgCurrent) {
    if(pLastVisibleDlg!=NULL) {
      // hide current dialog
      pLastVisibleDlg->ShowWindow(SW_HIDE);
    }
    if(pdlgCurrent!=NULL) {
      // show new dialog
      pdlgCurrent->ShowWindow(SW_SHOW);
    }
  }
}

// select surface in tree view
void CDlgBarTreeView::SelectMeshSurface(HTREEITEM hItem)
{
  INDEX iNodeIndex = m_TreeCtrl.GetItemData(hItem);
  INDEX iIcon = 12;
  NodeInfo &ni = theApp.aNodeInfo[iNodeIndex];
  ASSERT(ni.ni_iType == NT_MESHSURFACE);

  // count selected mesh surfaces
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surface in array
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    INDEX &iIndex = _aiSelectedMeshSurfaces[isms];
    // check if surface is allready selected
    if(iIndex == iNodeIndex)
    {
      // surface is allready selected, return
      return;
    }
  }
  // add surface to selection array
  INDEX &iIndex = _aiSelectedMeshSurfaces.Push();
  iIndex = iNodeIndex;
  // set new icon
  m_TreeCtrl.SetItemImage(hItem,iIcon,iIcon);
}

// Seselect surface in tree view
void CDlgBarTreeView::DeselectMeshSurface(HTREEITEM hItem)
{
  INDEX iNodeIndex = m_TreeCtrl.GetItemData(hItem);
  INDEX iIcon = 11;
  NodeInfo &ni = theApp.aNodeInfo[iNodeIndex];
  ASSERT(ni.ni_iType == NT_MESHSURFACE);

  // count selected mesh surfaces
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surface in array
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    INDEX &iIndex = _aiSelectedMeshSurfaces[isms];
    // check if this is clicked item
    if(iIndex == iNodeIndex)
    {
      // remove one item
      INDEX iLastIndex = _aiSelectedMeshSurfaces.Pop();
      iIndex = iLastIndex;
      // set new icon
      m_TreeCtrl.SetItemImage(hItem,iIcon,iIcon);
      return;
    }
  }
}

// Togle surface selection in tree view
void CDlgBarTreeView::TogleSurfaceSelection(HTREEITEM hItem)
{
  INDEX iNodeIndex = m_TreeCtrl.GetItemData(hItem);
  NodeInfo &ni = theApp.aNodeInfo[iNodeIndex];
  ASSERT(ni.ni_iType == NT_MESHSURFACE);
  
  // count selected mesh surfaces
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surface in array
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    INDEX &iIndex = _aiSelectedMeshSurfaces[isms];
    // check if this is clicked item
    if(iIndex == iNodeIndex)
    {
      // deselect surface
      DeselectMeshSurface(hItem);
      return;
    }
  }
  // if item was't previously selected, select it 
  SelectMeshSurface(hItem);
}

// Deselect all surfaces
void CDlgBarTreeView::DeSelectAllSurfaces(HTREEITEM hParent/*=NULL*/)
{
  INDEX iIcon=11;
  if(hParent==NULL)
  {
    hParent = m_TreeCtrl.GetRootItem();
    _aiSelectedMeshSurfaces.PopAll();
  }

  HTREEITEM hChild = m_TreeCtrl.GetChildItem(hParent);
  while(hChild!=NULL)
  {
    DeSelectAllSurfaces(hChild);
    NodeInfo &ni = GetNodeInfo(hChild);
    if(ni.ni_iType == NT_MESHSURFACE)
    {
      m_TreeCtrl.SetItemImage(hChild,iIcon,iIcon);
    }
    hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
  }
}

// select all surfaces if item is not selected, otherwise deselect all
void CDlgBarTreeView::SelectAllSurfaces(HTREEITEM hItem)
{
  INDEX iIcon = -1;
  INDEX iNodeIndex = m_TreeCtrl.GetItemData(hItem);
  NodeInfo &ni = theApp.aNodeInfo[iNodeIndex];
  ASSERT(ni.ni_iType == NT_MESHSURFACE);
  MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
  // clicked item is allready selected, deselect all surfaces
  BOOL bSelect = IsSurfaceSelected(*pmsrf);
  // get parent
  HTREEITEM hParent = m_TreeCtrl.GetParentItem(hItem);
  INDEX iParentIndex = m_TreeCtrl.GetItemData(hParent);
  NodeInfo &niParent = theApp.aNodeInfo[iParentIndex];
  ASSERT(niParent.ni_iType == NT_MESHLOD);
  MeshLOD *pmlod = (MeshLOD*)niParent.ni_pPtr;
  // get children of mesh lod
  HTREEITEM hChild = m_TreeCtrl.GetChildItem(hParent);
  while(hChild!=NULL)
  {
    // select surface
    if(!bSelect) SelectMeshSurface(hChild);
    else DeselectMeshSurface(hChild);
    hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
  }
}

// check if surface is selected
BOOL CDlgBarTreeView::IsSurfaceSelected(MeshSurface &msrf)
{
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    // check if this is selected surface
    if(pmsrf == &msrf) return TRUE;
  }
  return FALSE;
}

// change texture for all selected surfaces
void CDlgBarTreeView::ChangeTextureOnSelectedSurfaces(CTString strControlID,CTString strNewTexID)
{
  // get id of texture
  INDEX iTextureID = ska_GetIDFromStringTable(strNewTexID);
  // get selected surfaces count
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    // get pointer to mesh surface
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    if(pmsrf->msrf_pShader==NULL) continue;
    // get surface description and find witch param has same id as control that was modified
    ShaderDesc shDesc;
    pmsrf->msrf_pShader->GetShaderDesc(shDesc);
    INDEX cttx = shDesc.sd_astrTextureNames.Count();
    // for each texture in shader
    for(INDEX itx=0;itx<cttx;itx++)
    {
      // if this surface has shader param that has text same as control that changed value
      if(shDesc.sd_astrTextureNames[itx] == strControlID)
      {
        // change it
        pmsrf->msrf_ShadingParams.sp_aiTextureIDs[itx] = iTextureID;
      }
    }
  }
}

// change texture coord index for all selected surfaces
void CDlgBarTreeView::ChangeTextureCoordsOnSelectedSurfaces(CTString strControlID,INDEX iNewTexCoordsIndex)
{
  // get selected surfaces count
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    // get pointer to mesh surface
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    if(pmsrf->msrf_pShader==NULL) continue;
    // get surface description and find witch param has same id as control that was modified
    ShaderDesc shDesc;
    pmsrf->msrf_pShader->GetShaderDesc(shDesc);
    INDEX cttxc = shDesc.sd_astrTexCoordNames.Count();
    // for each texture in shader
    for(INDEX itxc=0;itxc<cttxc;itxc++)
    {
      // if this surface has shader param that has text same as control that changed value
      if(shDesc.sd_astrTexCoordNames[itxc] == strControlID)
      {
        // change it
        pmsrf->msrf_ShadingParams.sp_aiTexCoordsIndex[itxc] = iNewTexCoordsIndex;
      }
    }
  }
}

// change color on curently selected model instance
void CDlgBarTreeView::ChangeColorOnSelectedModel(COLOR colNewColor)
{
  ASSERT(pmiSelected!=NULL);
  pmiSelected->mi_colModelColor = colNewColor;
}

// change color on all selected mesh surfaces
void CDlgBarTreeView::ChangeColorOnSelectedSurfaces(CTString strControlID,COLOR colNewColor)
{
  // get selected surfaces count
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++)
  {
    // get pointer to mesh surface
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    if(pmsrf->msrf_pShader==NULL) continue;
    // get surface description and find witch param has same id as control that was modified
    ShaderDesc shDesc;
    pmsrf->msrf_pShader->GetShaderDesc(shDesc);
    INDEX ctcl = shDesc.sd_astrColorNames.Count();
    // for each color in shader
    for(INDEX icl=0;icl<ctcl;icl++)
    {
      // if this surface has shader param that has text same as control that changed value
      if(shDesc.sd_astrColorNames[icl] == strControlID)
      {
        // change it
        pmsrf->msrf_ShadingParams.sp_acolColors[icl] = colNewColor;
      }
    }
  }
}

// change floats on all selected mesh surfaces
void CDlgBarTreeView::ChangeFloatOnSelectedSurfaces(CTString strControlID,FLOAT fNewFloat)
{
  // get selected surfaces count
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++) {
    // get pointer to mesh surface
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    if(pmsrf->msrf_pShader==NULL) continue;
    // get surface description and find witch param has same id as control that was modified
    ShaderDesc shDesc;
    pmsrf->msrf_pShader->GetShaderDesc(shDesc);
    INDEX ctfl = shDesc.sd_astrFloatNames.Count();
    // for each float in shader
    for(INDEX ifl=0;ifl<ctfl;ifl++) {
      // if this surface has shader param that has text same as control that changed value
      if(shDesc.sd_astrFloatNames[ifl] == strControlID) {
        // change it
        pmsrf->msrf_ShadingParams.sp_afFloats[ifl] = fNewFloat;
      }
    }
  }
}

void CDlgBarTreeView::ChangeFlagOnSelectedSurfaces(CTString strControlID, BOOL bChecked, INDEX iFlagIndex)
{
  ULONG ulNewFlag = (1UL<<iFlagIndex);
  // get selected surfaces count
  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++) {
    // get pointer to mesh surface
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    if(pmsrf->msrf_pShader==NULL) continue;
    // get surface description and find witch param has same id as control that was modified
    ShaderDesc shDesc;
    pmsrf->msrf_pShader->GetShaderDesc(shDesc);
    INDEX ctfl = shDesc.sd_astrFlagNames.Count();
    // for each flag in shader
    for(INDEX ifl=0;ifl<ctfl;ifl++) {
      // if this surface has shader param that has text same as control that changed value
      if(shDesc.sd_astrFlagNames[ifl] == strControlID) {
        // if now checked 
        if(bChecked) {
          // add this flag
          pmsrf->msrf_ShadingParams.sp_ulFlags |= ulNewFlag;
        // if unchecked
        } else {
          // remove this flag
          pmsrf->msrf_ShadingParams.sp_ulFlags &= ~ulNewFlag;
        }
      }
    }
  }
}
// change shader for all surfaces that are in selection array
void CDlgBarTreeView::ChangeShaderOnSelectedSurfaces(CTString fnNewShader)
{
  #pragma message(">> Remove: Warning if usage of Double sided shader")
  // TEMP 
  if(fnNewShader.FindSubstr("DS")!=(-1)) {
    theApp.ErrorMessage("Usage of double sided shader");
  }

  INDEX ctsms = _aiSelectedMeshSurfaces.Count();
  // for each selected mesh surfaces
  for(INDEX isms=0;isms<ctsms;isms++) {
    NodeInfo &ni = theApp.aNodeInfo[_aiSelectedMeshSurfaces[isms]];
    ASSERT(ni.ni_iType == NT_MESHSURFACE);
    MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
    ASSERT(pmsrf!=NULL);
    try {
      // try to change surface shader
      ChangeSurfaceShader_t(*pmsrf,fnNewShader);
    } catch(char *strErr) {
      theApp.ErrorMessage(strErr);
    }
  }
}

// notification that item was clicked
void CDlgBarTreeView::OnItemClick(HTREEITEM hItem,HTREEITEM hLastSelected)
{
  BOOL bControl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  ASSERT(hItem != NULL);
  NodeInfo &ni = theApp.aNodeInfo[m_TreeCtrl.GetItemData(hItem)];
  if(ni.ni_iType == NT_MESHSURFACE) {
    if (bShift) {
      if(hLastSelected!=NULL) {
        NodeInfo &niLastSelected = GetNodeInfo(hLastSelected);
        if(niLastSelected.ni_iType == NT_MESHSURFACE) {
          HTREEITEM hParent = m_TreeCtrl.GetParentItem(hItem);
          HTREEITEM hLastParent = m_TreeCtrl.GetParentItem(hLastSelected);
          ASSERT(hParent!=NULL);
          ASSERT(hLastParent!=NULL);
          if(hParent==hLastParent)           {
            // deselect all items
            if(!bControl) DeSelectAllSurfaces();
            // get first child
            HTREEITEM hChild = m_TreeCtrl.GetChildItem(hParent);
            // find first item or last selected item
            while((hChild!=hItem)&&(hChild!=hLastSelected)) {
              hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
              ASSERT(hChild!=NULL);
            }
            // select first item
            SelectMeshSurface(hChild);
            if(hItem!=hLastSelected) hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
            // select all items until item or last selected item
            while((hChild!=hItem)&&(hChild!=hLastSelected)) {
              SelectMeshSurface(hChild);
              hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
              ASSERT(hChild!=NULL);
            }
            SelectMeshSurface(hChild);
          }
          return;
        }
      }
    }
    if(bControl) {
      // select curent surface
      TogleSurfaceSelection(hItem);
      return;
    }
    // deselect all items
    DeSelectAllSurfaces();
    // select curent surface
    SelectMeshSurface(hItem);
  }
}

// notification that item icons was clicked
void CDlgBarTreeView::OnItemIconClick(HTREEITEM hItem)
{
  NodeInfo &ni = GetNodeInfo(hItem);
  if(ni.ni_iType == NT_MESHSURFACE) {
    TogleSurfaceSelection(hItem);
  }
}

// show shader for surface
void CDlgBarTreeView::ShowSurfaceShader(MeshSurface *pmsrf,MeshLOD *pmlod,MeshInstance *pmshi)
{
  ASSERT(pmsrf!=NULL);
  ASSERT(pmlod!=NULL);
  ASSERT(pmshi!=NULL);

  // remember vertical scroll pos
  INDEX ivScroll = m_dlgShader.GetScrollPos(SB_VERT);
  // set scroll to 0
  m_dlgShader.SetScrollPos(SB_VERT,0);
  VScrollControls(&m_dlgShader);
  // clear shader dlg
  RemoveDialogControls(&m_dlgShader);
  CComboBox *cbShader = ((CComboBox*)m_dlgShader.GetDlgItem(IDC_CB_SHADER));

  _atxcTexControls.Clear();
  _atxccTexCoordControls.Clear();
  _accColors.Clear();
  _afcFloatControls.Clear();
  _afcFlagControls.Clear();
  iCustomControlID=FIRSTSHADEID;

  ShaderDesc shDesc;

  INDEX ctTextures = 0;
  INDEX ctTexCoords = 0;
  INDEX ctColors = 0;
  INDEX ctFloats = 0;
  INDEX ctFlags  = 0;

  // if surface has shader
  if(pmsrf->msrf_pShader!=NULL) {
    pmsrf->msrf_pShader->GetShaderDesc(shDesc);
    ctTextures = shDesc.sd_astrTextureNames.Count();
    ctTexCoords = shDesc.sd_astrTexCoordNames.Count();
    ctColors = shDesc.sd_astrColorNames.Count();
    ctFloats = shDesc.sd_astrFloatNames.Count();
    ctFlags  = shDesc.sd_astrFlagNames.Count();
    // select shader in combo box
    CTFileName fnShader = pmsrf->msrf_pShader->GetName();
    CTString strShader = fnShader.FileName();
    ASSERT(cbShader!=NULL);
    if(cbShader->SelectString(-1,CString(strShader)) == CB_ERR) {
      // error: shader is not found in list of shaders
      return;
    }
    ShaderParams &mspParams = pmsrf->msrf_ShadingParams;

//    ASSERT(pmsrf->msrf_ShadingParams.sp_acolColors.Count() == ctColors);

    _atxcTexControls.New(ctTextures);
    _atxccTexCoordControls.New(ctTexCoords);
    _accColors.New(ctColors);
    _afcFloatControls.New(ctFloats);
    _afcFlagControls.New(ctFlags);

    // add texture controls to shader dialog
    for(INDEX itx=0;itx<ctTextures;itx++) {
      CTextureControl &txc = _atxcTexControls[itx];
      txc.AddControl(shDesc.sd_astrTextureNames[itx],&pmsrf->msrf_ShadingParams.sp_aiTextureIDs[itx]);
      // count texture instances
      INDEX ctti=pmshi->mi_tiTextures.Count();
      // for each texture instances
      for(INDEX iti=0;iti<ctti;iti++) {
        TextureInstance &ti = pmshi->mi_tiTextures[iti];
        CTString strTexName = ska_GetStringFromTable(ti.GetID());
        txc.txc_Combo.AddString(CString(strTexName));
      }
      INDEX iSelectTexID = mspParams.sp_aiTextureIDs[itx];
      CTString strSelectTexID = ska_GetStringFromTable(iSelectTexID);
      int iItemIndex = txc.txc_Combo.FindStringExact(-1,CString(strSelectTexID));
      if(iItemIndex!=CB_ERR) txc.txc_Combo.SetCurSel(iItemIndex);
    }
    // add texcoord controls to shader dialog
    for(INDEX itxc=0;itxc<ctTexCoords;itxc++) {
      CTexCoordControl &txcc = _atxccTexCoordControls[itxc];
      txcc.AddControl(shDesc.sd_astrTexCoordNames[itxc],&pmsrf->msrf_ShadingParams.sp_aiTexCoordsIndex[itxc]);
      // count uvmaps
      INDEX ctuv = pmlod->mlod_aUVMaps.Count();
      // for each uvmap in lod
      for(INDEX iuv=0;iuv<ctuv;iuv++) {
        // add uvmap name in combo box
        MeshUVMap &uvm = pmlod->mlod_aUVMaps[iuv];
        CTString strUVMapName = ska_GetStringFromTable(uvm.muv_iID);
        txcc.txcc_Combo.AddString(CString(strUVMapName));
      }

      INDEX iSelectTexCoordsIndex = mspParams.sp_aiTexCoordsIndex[itxc];
      txcc.txcc_Combo.SetCurSel(iSelectTexCoordsIndex);
    }
    // add color controls to shader dialog
    for(INDEX icc=0;icc<ctColors;icc++) {
      CColorControl &cc = _accColors[icc];
      cc.AddControl(shDesc.sd_astrColorNames[icc],&pmsrf->msrf_ShadingParams.sp_acolColors[icc]);
    }
    // add float controls to shader dialog
    INDEX ifc=0;
    for(;ifc<ctFloats;ifc++) {
      CFloatControl &fc = _afcFloatControls[ifc];
      fc.AddControl(shDesc.sd_astrFloatNames[ifc],&pmsrf->msrf_ShadingParams.sp_afFloats[ifc]);
    }
    // add flag controls to shader dialog
    for(ifc=0;ifc<ctFlags;ifc++) {
      CFlagControl &fc = _afcFlagControls[ifc];
      fc.AddControl(shDesc.sd_astrFlagNames[ifc],ifc,mspParams.sp_ulFlags);
    }
  } else {
    // clear text from combo box
    cbShader->SetCurSel(-1);
  }
  

  CRect rcDlg;
  m_dlgShader.GetWindowRect(rcDlg);

  INDEX iCtrlCount = iCustomControlID-FIRSTSHADEID;
  INDEX iDlgHeight = (rcDlg.bottom-rcDlg.top-15);

  if(iDlgHeight<YPOS) m_dlgShader.SetScrollRange(SB_VERT,1,YPOS-iDlgHeight);
  else m_dlgShader.SetScrollRange(SB_VERT,0,0);

  AddDialogControls(&m_dlgShader);
  // return old vertical scroll pos
  m_dlgShader.SetScrollPos(SB_VERT,ivScroll);
  VScrollControls(&m_dlgShader);

  //m_dlgShader.Invalidate(FALSE);
}

void CDlgBarTreeView::VScrollControls(CDialog *pDlg)
{
  //pDlg->SetRedraw(FALSE);
  int iPosition = pDlg->GetScrollPos(SB_VERT);
  // GetScrollPos returns values from 1 to n
  if(iPosition>0) iPosition--;
  INDEX ctctrl = dlg_aControls.Count();
  // for each control in array of contrls
  for(INDEX ictrl=0;ictrl<ctctrl;ictrl++) {
    Control &ctrl = dlg_aControls[ictrl];
    // check if its dialog is being scrolled
    if(ctrl.ct_pParentDlg == pDlg) {
      // move control
      CRect rcCtrl;
      pDlg->GetDlgItem(ctrl.ct_iID)->GetWindowRect(rcCtrl);
      pDlg->ScreenToClient(rcCtrl);
      rcCtrl.top = ctrl.ct_iTop - iPosition;
      rcCtrl.bottom = ctrl.ct_iBottom - iPosition;
      pDlg->GetDlgItem(ctrl.ct_iID)->MoveWindow(rcCtrl);
    }
  }
//  pDlg->SetRedraw(TRUE);
//  pDlg->Invalidate(TRUE);
}

// notification that selected item has changed
void CDlgBarTreeView::SelItemChanged(HTREEITEM hSelected)
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  BOOL bCtrl  = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bAlt   = (GetKeyState( VK_MENU)&0x8000) != 0;

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // reset selected bone in treeview
  theApp.iSelectedBoneID = -1;
  // if no item selected
  if(hSelected == NULL) {
    pmiSelected = NULL;
    ResetControls();
  }
  // get index in node array of selected item
  INDEX iSelIndex = m_TreeCtrl.GetItemData(hSelected);
  // get node info of selected item
  NodeInfo &ni = theApp.aNodeInfo[iSelIndex];
  // get model instance that holds selected item
  pmiSelected = ni.pmi;
  // if selected item is model instance set it to be selected model instance
  if(ni.ni_iType == NT_MODELINSTANCE) pmiSelected = (CModelInstance*)ni.ni_pPtr;

  if(pmiSelected!=NULL) {
    CTString strStretch = CTString(0,"%g",pmiSelected->mi_vStretch(1));
    pMainFrame->m_ctrlMIStretch.SetWindowText(CString(strStretch));
  }
  // get parent of selected item
  HTREEITEM hParent = m_TreeCtrl.GetParentItem(hSelected);
  INDEX iParent=-1;
  // if parent exists get ist index
  if(hParent != NULL) iParent = m_TreeCtrl.GetItemData(hParent);
  // get parent model isntance of selected model instance
  CModelInstance *pmiParent = pmiSelected->GetParent(pDoc->m_ModelInstance);
  // if parent exisits fill combo box with parent bones
  if(pmiParent != NULL) {
    // fill combo box with parent bones
    if(pmiParent->mi_psklSkeleton != NULL) FillBonesToComboBox(pmiParent->mi_psklSkeleton,0);
    // clear combo box for parent bones
    else ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->ResetContent();
  } else {
    // clear combo box for parent bones
    ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->ResetContent();
  }
  // get name of parent bone
  CTString strBoneName = ska_GetStringFromTable(pmiSelected->mi_iParentBoneID);
  // select parent bone in combo box
  ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->SelectString(-1,CString(strBoneName));

  // select parent model instance in combo box
  CComboBox *cbParentList = ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTMODEL));
  // count items in combo box
  INDEX ctpl = cbParentList->GetCount();
  // for each item in combo box
  for(INDEX ipl=0;ipl<ctpl;ipl++) {
    CModelInstance *pmiParentInList = (CModelInstance*)cbParentList->GetItemDataPtr(ipl);
    // chech if this item is parent in current selected mi
    if(pmiParentInList == pmiParent) {
      cbParentList->SetCurSel(ipl);
      break;
    }
    cbParentList->SetCurSel(-1);
  }
  
  // switch type of selected item
  switch(ni.ni_iType)
  { 
    case NT_MODELINSTANCE:
    {
      ShowControlGroup(GR_PARENT);
      CModelInstance *pmi = (CModelInstance*)ni.ni_pPtr;
      m_tbMiName.SetWindowText(CString(pmi->GetName()));
    }
    break;
    case NT_ANIMSET:
    case NT_MESHLODLIST:
    case NT_SKELETONLODLIST:
      ShowControlGroup(GR_LISTOPTIONS);
    break;
    case NT_TEXINSTANCE:
    {
      TextureInstance *pti = (TextureInstance*)ni.ni_pPtr;
      CTString strTexName = ska_GetStringFromTable(pti->GetID());
      m_tbTextureName.SetWindowText(CString(strTexName));
      m_tvTexView.ChangeTexture(pti->ti_toTexture.GetName());
      ShowControlGroup(GR_TEXTURE);
    }
    break;
    case NT_MESHLOD:
    {
      MeshLOD *pmlod = (MeshLOD*)ni.ni_pPtr;
      CTString strName;
      strName.PrintF("%g",pmlod->mlod_fMaxDistance);
      m_tbDistance.SetWindowText(CString(strName));
      // set this mesh lod as forced lod
      pDoc->fCustomMeshLodDist=pmlod->mlod_fMaxDistance-0.01f;
      ShowControlGroup(GR_LOD);
      SetCustomTabText(L"Mesh lod");
    }
    break;
    case NT_SKELETONLOD:
    {
      SkeletonLOD *pslod = (SkeletonLOD*)ni.ni_pPtr;
      CTString strName = CTString(0,"%g",pslod->slod_fMaxDistance);
      m_tbDistance.SetWindowText(CString(strName));
      // set this skleleton lod as forced lod
      pDoc->fCustomSkeletonLodDist=pslod->slod_fMaxDistance-0.01f;
      ShowControlGroup(GR_LOD);
      SetCustomTabText(L"Skeleton lod");
    }
    break;
    case NT_BONE:
    {
      SkeletonBone *pbone = (SkeletonBone*)ni.ni_pPtr;
      theApp.iSelectedBoneID = pbone->sb_iID;
      ShowControlGroup(GR_BONE);
    }
    break;
    case NT_ANIMATION:
    {
      Animation *pan = (Animation*)ni.ni_pPtr;
      // start playing selected animation
      ULONG ulAnimFlags = 0;// | AN_NORESTART;
      
      // if looping animations
      if(pDoc->bAnimLoop) {
        // if shift is not presed 
        if(!bShift) {
          // loop animations
          ulAnimFlags |= AN_LOOPING;
        }
      // if not looping and shift pressed
      } else if(bShift) {
        // loop anims
        ulAnimFlags |= AN_LOOPING;
      }

      // if control is presed
      if(bCtrl) {
        // Add animation
        pmiSelected->AddAnimation(pan->an_iID,ulAnimFlags|AN_CLONE,1.0f,0);
      // if alt is presed 
      } else if(bAlt) {
        // do new cloned state
        pmiSelected->NewClonedState(0.2f);
        // Add animation
        pmiSelected->RemAnimation(pan->an_iID);
      } else {
        // Add animation
        pmiSelected->AddAnimation(pan->an_iID,ulAnimFlags|AN_CLEAR,1,0);
      }

      // Set timer for current document
      pDoc->SetTimerForDocument();
      CTString strTreshold;
      CTString strSecPerFrame;
      CTString strZSpeed;
      CTString strLoopSec;
      strTreshold.PrintF("%g",pan->an_fTreshold);
      strSecPerFrame.PrintF("%g",pan->an_fSecPerFrame);
      strZSpeed.PrintF("%g",pDoc->m_fSpeedZ);
      strLoopSec.PrintF("%g",pDoc->m_fLoopSecends);
      m_tbTreshold.SetWindowText(CString(strTreshold));
      m_tbAnimSpeed.SetWindowText(CString(strSecPerFrame));
      m_tbWalkSpeed.SetWindowText(CString(strZSpeed));
      m_tbWalkLoopSec.SetWindowText(CString(strLoopSec));
      // check compresion 
      ((CButton*)m_dlgAnimSet.GetDlgItem(IDC_CB_COMPRESION))->SetCheck(pan->an_bCompresed);
      CheckSecPerFrameCtrl(pan->an_bCustomSpeed);
      ShowControlGroup(GR_ANIMSET);
      SetCustomTabText(L"Animation");
    }
    break;
    case NT_MESHSURFACE:
    {
      // get mesh instance
      HTREEITEM hParentsParent = m_TreeCtrl.GetParentItem(hParent);
      NodeInfo &niParent = GetNodeInfo(hParent);
      NodeInfo &niParentsParent = GetNodeInfo(hParentsParent);

      MeshSurface *pmsrf = (MeshSurface*)ni.ni_pPtr;
      MeshLOD *pmlod = (MeshLOD*)niParent.ni_pPtr;
      MeshInstance *pmshi = (MeshInstance*)niParentsParent.ni_pPtr;

      ShowControlGroup(GR_SHADERS);
      ShowSurfaceShader(pmsrf,pmlod,pmshi);
    }
    break;
    case NT_COLISIONBOX:
    {
      ColisionBox *pcb = (ColisionBox*)ni.ni_pPtr;
      INDEX iIndex = -1;
      // get colision box count
      INDEX ctcb = pmiSelected->mi_cbAABox.Count();
      // for each colision box in array
      for(INDEX icb=0;icb<ctcb;icb++) {
        ColisionBox *pcb2 = &pmiSelected->mi_cbAABox[icb];
        if(pcb == pcb2) {
          iIndex = icb;
          break;
        }
      }
      if(iIndex<0) return;
      // set it to be curent colision box
      pmiSelected->mi_iCurentBBox = iIndex;
      ShowControlGroup(GR_COLISION);
      SetCustomTabText(L"Colision");
    }
    break;
    case NT_ALLFRAMESBBOX:
    {
      ShowControlGroup(GR_ALLFRAMES_BBOX);
      SetCustomTabText(L"All frames");
    }
    break;
    default:
      // no custom group
      //iShowCustomGroup = -1;
      ShowControlGroup(-1);
      SetCustomTabText(L"Custom");
    break;
  }
  
  // show name of selected model instance
  GetDlgItem(IDC_SELECTEDMI)->SetWindowText(CString(pmiSelected->GetName()));

  // show offset of model instance 
  FLOATmatrix3D mat;
  FLOAT3D vPos = pmiSelected->mi_qvOffset.vPos;
  ANGLE3D aRot;
  pmiSelected->mi_qvOffset.qRot.ToMatrix(mat);
  DecomposeRotationMatrix(aRot,mat);

  m_tbOffPosX.SetWindowText(CString(CTString(0,"%g",vPos(1))));
  m_tbOffPosY.SetWindowText(CString(CTString(0,"%g",vPos(2))));
  m_tbOffPosZ.SetWindowText(CString(CTString(0,"%g",vPos(3))));
  m_tbOffRotH.SetWindowText(CString(CTString(0,"%g",aRot(1))));
  m_tbOffRotP.SetWindowText(CString(CTString(0,"%g",aRot(2))));
  m_tbOffRotB.SetWindowText(CString(CTString(0,"%g",aRot(3))));

  // show collision box values
  if((pmiSelected->mi_iCurentBBox >= 0) && (pmiSelected->mi_iCurentBBox < pmiSelected->mi_cbAABox.Count()))
  {
    ColisionBox &cb = pmiSelected->mi_cbAABox[pmiSelected->mi_iCurentBBox];
    FLOAT fWidth = (cb.Max()(1)-cb.Min()(1));
    FLOAT fHeight = (cb.Max()(2)-cb.Min()(2));
    FLOAT fLength = (cb.Max()(3)-cb.Min()(3));
    FLOAT fPosX = (cb.Max()(1)+cb.Min()(1)) / 2;
    FLOAT fPosY = (cb.Max()(2)+cb.Min()(2)) / 2;
    FLOAT fPosZ = (cb.Max()(3)+cb.Min()(3)) / 2;
    fPosY -= fHeight/2;
    m_tbColName.SetWindowText(  CString(cb.GetName()));
    m_tbColWidth.SetWindowText( CString(CTString(0,"%g",fWidth)));
    m_tbColHeight.SetWindowText(CString(CTString(0,"%g",fHeight)));
    m_tbColLength.SetWindowText(CString(CTString(0,"%g",fLength)));
    m_tbColPosX.SetWindowText(  CString(CTString(0,"%g",fPosX)));
    m_tbColPosY.SetWindowText(  CString(CTString(0,"%g",fPosY)));
    m_tbColPosZ.SetWindowText(  CString(CTString(0,"%g",fPosZ)));
  }
  // show all frames bbox
  ColisionBox &cb = pmiSelected->mi_cbAllFramesBBox;
  FLOAT fWidth = (cb.Max()(1)-cb.Min()(1));
  FLOAT fHeight = (cb.Max()(2)-cb.Min()(2));
  FLOAT fLength = (cb.Max()(3)-cb.Min()(3));
  FLOAT fPosX = (cb.Max()(1)+cb.Min()(1)) / 2;
  FLOAT fPosY = (cb.Max()(2)+cb.Min()(2)) / 2;
  FLOAT fPosZ = (cb.Max()(3)+cb.Min()(3)) / 2;
  fPosY -= fHeight/2;
  m_tbAFBBWidth.SetWindowText( CString(CTString(0,"%g",fWidth)));
  m_tbAFBBHeight.SetWindowText(CString(CTString(0,"%g",fHeight)));
  m_tbAFBBLength.SetWindowText(CString(CTString(0,"%g",fLength)));
  m_tbAFBBPosX.SetWindowText(  CString(CTString(0,"%g",fPosX)));
  m_tbAFBBPosY.SetWindowText(  CString(CTString(0,"%g",fPosY)));
  m_tbAFBBPosZ.SetWindowText(  CString(CTString(0,"%g",fPosZ)));
  // change custom controls visibility
//  ShowControlGroup(iVisibleGroup);
}

// calculate size of tree view
CSize CDlgBarTreeView::CalcLayout(int nLength, DWORD nMode)
{
  CSize csResult;
  // Return default if it is being docked or floated
  if(nMode & LM_VERTDOCK)
  {
    csResult = m_Size;
    CRect rc;
    // get main frm
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    // get his child
    CMDIClientWnd *pMDIClient = &pMainFrame->m_wndMDIClient;
    pMDIClient->GetWindowRect(rc);
    csResult.cy = rc.bottom - rc.top;
  }
  else if((nMode & LM_VERTDOCK) || (nMode & LM_HORZDOCK))
  {
    if (nMode & LM_STRETCH) // if not docked stretch to fit
    {
      csResult = CSize((nMode & LM_HORZ) ? 32767 : m_Size.cx,
                       (nMode & LM_HORZ) ? m_Size.cy : 32767);
    }
    else
    {
      csResult = m_Size;
    }
  }
  else if (nMode & LM_MRUWIDTH)
  {
    csResult = m_Size;
  }
  // In all other cases, accept the dynamic length
  else
  {
    if (nMode & LM_LENGTHY)
    {
      // Note that we don't change m_Size.cy because we disabled vertical sizing
      csResult = CSize( m_Size.cx, m_Size.cy = nLength);
    }
    else
    {
      csResult = CSize( m_Size.cx = nLength, m_Size.cy);
    }
  }
//  csResult = CSize(300,300);
  return csResult;
}

// resize dialog
CSize CDlgBarTreeView::CalcDynamicLayout(int nLength, DWORD nMode)
{
  // return CDlgTemplate::CalcDynamicLayout(nLength, nMode);
  CSize csResult = CalcLayout(nLength,nMode);
  if(theApp.IsErrorDlgVisible()) {
    csResult.cy+=theApp.GetLogDlgSize().cy;
  }

  #define H_BORDER  20
  #define V_BORDER  10
  #define LINE1     15
  #define LINE2     35
  #define LINE3     55
  #define LINE1U     5
  #define LINE2U    25
  #define LINE3U    45
  #define LINE4U    65

  #define WIT (csResult.cx-2*H_BORDER)/4
  #define YOFFSET csResult.cy - V_BORDER - 140
  // move tab control
  GetDlgItem(IDC_MODE_SELECT_TAB)->MoveWindow(CRect(H_BORDER/2,YOFFSET-30,csResult.cx-H_BORDER/2,YOFFSET + 140));
  // move dialogs
  CRect rcDlg = CRect(2,6,csResult.cx-H_BORDER/2-13,165); // !!! fix
  ResizeDlgWithChildren(&m_dlgParent,rcDlg);
  ResizeDlgWithChildren(&m_dlgAnimSet,rcDlg);
  ResizeDlgWithChildren(&m_dlgColision,rcDlg);
  ResizeDlgWithChildren(&m_dlgAllFrames,rcDlg);
  ResizeDlgWithChildren(&m_dlgLod,rcDlg);
  ResizeDlgWithChildren(&m_dlgBone,rcDlg);
  ResizeDlgWithChildren(&m_dlgTexture,rcDlg);
  ResizeDlgWithChildren(&m_dlgListOpt,rcDlg);
  ResizeDlgWithChildren(&m_dlgShader,rcDlg);

  AdjustSplitter();

  /*
  if(iDockSide == AFX_IDW_DOCKBAR_LEFT)
  {
    GetDlgItem(IDC_SPLITER_FRAME)->MoveWindow(CRect(csResult.cx-SPLITTER_WITDH,0,csResult.cx,csResult.cy));
    GetDlgItem(IDC_SPLITER_FRAME)->ShowWindow(SW_SHOW);
  }
  else if(iDockSide == AFX_IDW_DOCKBAR_RIGHT)
  {
    GetDlgItem(IDC_SPLITER_FRAME)->MoveWindow(CRect(0,0,SPLITTER_WITDH,csResult.cy));
    GetDlgItem(IDC_SPLITER_FRAME)->ShowWindow(SW_SHOW);
  }
  else
  {
    GetDlgItem(IDC_SPLITER_FRAME)->ShowWindow(SW_HIDE);
  }
  */

  // move label that display curent selected mi                 
  GetDlgItem(IDC_SELECTEDMI)->MoveWindow(CRect(H_BORDER         ,3,csResult.cx-H_BORDER,V_BORDER + 15));
  // move tree ctrl
  CRect NewTreePos;
  NewTreePos = CRect(H_BORDER/2, V_BORDER + 15, csResult.cx - H_BORDER/2, csResult.cy - V_BORDER - 180);
  CWnd *pwndTree = GetDlgItem(IDC_TREE1);
  pwndTree->MoveWindow(NewTreePos);
  pwndTree->UpdateWindow();
  UpdateWindow();
  return csResult;
}

void CDlgBarTreeView::ResizeDlgWithChildren(CDialog *pDlg,CRect rcDlg)
{
  INDEX iDlgWidth = rcDlg.right - rcDlg.left;
  INDEX ctctrl = dlg_aControls.Count();
  // for each control in array of contrls
  for(INDEX ictrl=0;ictrl<ctctrl;ictrl++)
  {
    Control &ctrl = dlg_aControls[ictrl];
    // check if its dialog is being resized
    if(ctrl.ct_pParentDlg == pDlg)
    {
      // resize control
      CRect rcCtrl;
      rcCtrl.left = iDlgWidth * ctrl.ct_fLeft;
      rcCtrl.right = iDlgWidth * ctrl.ct_fRight;
      rcCtrl.top = ctrl.ct_iTop;
      rcCtrl.bottom = ctrl.ct_iBottom;
      pDlg->GetDlgItem(ctrl.ct_iID)->MoveWindow(rcCtrl);
    }
  }
  // resize dialog
  pDlg->MoveWindow(rcDlg);
}

// remove control form array 
void CDlgBarTreeView::RemoveControlFromArray(CWnd *pChild, CDialog *pDlg)
{
  // get control ID
  INDEX ictrlID = pChild->GetDlgCtrlID();
  // count controls in array
  INDEX ctctrl = dlg_aControls.Count();
  // for each control in array
  for(INDEX ictrl=0;ictrl<ctctrl;ictrl++) {
    Control &ctrl = dlg_aControls[ictrl];
    // check if this is control to remove
    if((ctrl.ct_iID == ictrlID) && (ctrl.ct_pParentDlg == pDlg)) {
      // get last ctrl
      Control &ctrlLast = dlg_aControls[ctctrl-1];
      // copy last control insted of one that has to be removed
      ctrl = ctrlLast;
      // remove last control from array
      dlg_aControls.Pop();
      return;
    }
  }
}

// add control to array witch stores all controls that need to be dynamicly resized
void CDlgBarTreeView::AddControlToArray(CWnd *pChild, CDialog *pDlg)
{
  CRect rcParent;
  CRect rcChild;

  pDlg->GetWindowRect(rcParent);
  pChild->GetWindowRect(rcChild);

  INDEX iParentWidth = rcParent.right - rcParent.left;
  // convert to parent coords
  pDlg->ScreenToClient(rcChild);
  
  Control &ctrl = dlg_aControls.Push();
  ctrl.ct_iID = pChild->GetDlgCtrlID();
  ctrl.ct_pParentDlg = pDlg;
  ctrl.ct_iTop = rcChild.top;
  ctrl.ct_iBottom = rcChild.bottom;
  ctrl.ct_fLeft = ((FLOAT)rcChild.left/(FLOAT)iParentWidth);
  ctrl.ct_fRight = ((FLOAT)rcChild.right/(FLOAT)iParentWidth);
}

// remove controls from array
void CDlgBarTreeView::RemoveDialogControls(CDialog *pDlg)
{
  CWnd *pChild = pDlg->GetWindow(GW_CHILD);
  while(pChild!=NULL) {
    RemoveControlFromArray(pChild,pDlg);
    pChild = pChild->GetWindow(GW_HWNDNEXT);
  }
}

// add all controls from dialog to array
void CDlgBarTreeView::AddDialogControls(CDialog *pDlg)
{
  CWnd *pChild = pDlg->GetWindow(GW_CHILD);
  while(pChild!=NULL) {
    AddControlToArray(pChild,pDlg);
    pChild = pChild->GetWindow(GW_HWNDNEXT);
  }
}

// create dialog 
BOOL CDlgBarTreeView::Create( CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID)
{
  CRect rectDummy(0,0,0,0);
  if(!CDlgTemplate::Create(pParentWnd,nIDTemplate,nStyle,nID)) return FALSE;
  m_Size = m_sizeDefault;  
  SetSplitterControlID(IDC_SPLITTER_TREE);
  // spliter
  // m_wndSpliterFrame.SubclassDlgItem(IDC_SPLITTER_TREE,this);
  // m_wndSpliterFrame.SetDockingSide(AFX_IDW_DOCKBAR_FLOAT);

  // tree ctrl
  m_IconsImageList.Create( IDB_BITMAP1, 16, 32, CLR_NONE);
  m_TreeCtrl.SubclassDlgItem(IDC_TREE1, this);
  m_TreeCtrl.SetImageList( &m_IconsImageList, TVSIL_NORMAL);
  // add tab control buttons
  CTabCtrl *pTabCtrl = (CTabCtrl*)GetDlgItem(IDC_MODE_SELECT_TAB);

  // create dialogs
  m_dlgParent.Create(IDD_PARENT,pTabCtrl);
  m_dlgAnimSet.Create(IDD_ANIMSET,pTabCtrl);
  m_dlgColision.Create(IDD_COLISION,pTabCtrl);
  m_dlgAllFrames.Create(IDD_ALL_FRAMES_BBOX,pTabCtrl);
  m_dlgLod.Create(IDD_LOD,pTabCtrl);
  m_dlgBone.Create(IDD_BONE,pTabCtrl);
  m_dlgTexture.Create(IDD_TEXTURE,pTabCtrl);
  m_dlgListOpt.Create(IDD_LIST_OPTIONS,pTabCtrl);
  m_dlgShader.Create(IDD_SHADER,pTabCtrl);


  CRect rcDummy = CRect(0,0,100,20);

  AddDialogControls(&m_dlgTexture);
  AddDialogControls(&m_dlgListOpt);
  AddDialogControls(&m_dlgParent);
  AddDialogControls(&m_dlgAnimSet);
  AddDialogControls(&m_dlgColision);
  AddDialogControls(&m_dlgAllFrames);
  AddDialogControls(&m_dlgLod);
  AddDialogControls(&m_dlgBone);
  AddDialogControls(&m_dlgShader);

  // subclass controls in parent dialog
  m_tbOffPosX.SubclassDlgItem(IDC_TB_OFFSET_POSX, &m_dlgParent);
  m_tbOffPosY.SubclassDlgItem(IDC_TB_OFFSET_POSY, &m_dlgParent);
  m_tbOffPosZ.SubclassDlgItem(IDC_TB_OFFSET_POSZ, &m_dlgParent);
  m_tbOffRotH.SubclassDlgItem(IDC_TB_OFFSET_ROTH, &m_dlgParent);
  m_tbOffRotP.SubclassDlgItem(IDC_TB_OFFSET_ROTP, &m_dlgParent);
  m_tbOffRotB.SubclassDlgItem(IDC_TB_OFFSET_ROTB, &m_dlgParent);
  m_tbMiName.SubclassDlgItem(IDC_TB_MI_NAME, &m_dlgParent); 

  // subclass controls in animset dialog
  m_tbTreshold.SubclassDlgItem(IDC_EB_TRESHOLD    ,&m_dlgAnimSet);
  m_tbAnimSpeed.SubclassDlgItem(IDC_EB_SECPERFRAME,&m_dlgAnimSet);
  m_tbWalkSpeed.SubclassDlgItem(IDC_TB_ZTRANSSPEED,&m_dlgAnimSet);
  m_tbWalkLoopSec.SubclassDlgItem(IDC_TB_ZTRANSLOOP,&m_dlgAnimSet);
  // subclass controls in colision dialog
  m_tbColName.SubclassDlgItem(IDC_TB_COLNAME,   &m_dlgColision);
  m_tbColWidth.SubclassDlgItem(IDC_TB_COLWIDTH, &m_dlgColision);
  m_tbColHeight.SubclassDlgItem(IDC_TB_COLHEIGHT,&m_dlgColision);
  m_tbColLength.SubclassDlgItem(IDC_TB_COLLENGTH,&m_dlgColision);
  m_tbColPosX.SubclassDlgItem(IDC_TB_COLPOSX,   &m_dlgColision);
  m_tbColPosY.SubclassDlgItem(IDC_TB_COLPOSY,   &m_dlgColision);
  m_tbColPosZ.SubclassDlgItem(IDC_TB_COLPOSZ,   &m_dlgColision);

  // subclass controls in colision dialog
  m_tbAFBBWidth.SubclassDlgItem(IDC_TB_COLWIDTH, &m_dlgAllFrames);
  m_tbAFBBHeight.SubclassDlgItem(IDC_TB_COLHEIGHT,&m_dlgAllFrames);
  m_tbAFBBLength.SubclassDlgItem(IDC_TB_COLLENGTH,&m_dlgAllFrames);
  m_tbAFBBPosX.SubclassDlgItem(IDC_TB_COLPOSX,   &m_dlgAllFrames);
  m_tbAFBBPosY.SubclassDlgItem(IDC_TB_COLPOSY,   &m_dlgAllFrames);
  m_tbAFBBPosZ.SubclassDlgItem(IDC_TB_COLPOSZ,   &m_dlgAllFrames);

  m_tbDistance.SubclassDlgItem(IDC_EB_DISTANCE, &m_dlgLod);

  m_tbTextureName.SubclassDlgItem(IDC_EB_TEXTURENAME, &m_dlgTexture); 

  CWnd *pTexViewFrame = m_dlgTexture.GetDlgItem(IDC_TEXTURE_VIEW);
  // CRect rc;
  // pTexViewFrame->GetClientRect(&rc);
  CRect rc = CRect(0,0,200,100);
  m_tvTexView.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rc, pTexViewFrame,IDC_TEXTURE_VIEW);
  m_tvTexView.SubclassDlgItem(IDC_TEXTURE_VIEW, &m_dlgTexture);

  // set width of shader combo box
  ((CComboBox*)m_dlgShader.GetDlgItem(IDC_CB_SHADER))->SetDroppedWidth(200);

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_ctrlMIStretch.SetFont(m_dlgShader.GetFont(),FALSE);
  return TRUE;
}

// add node from tree view to nodeinfo array
INDEX AddNode(INDEX iType,void *ni_pPtr,CModelInstance *pmi)
{
  INDEX ctni = theApp.aNodeInfo.Count();
  NodeInfo &ni = theApp.aNodeInfo.Push();
  ni.ni_iType = iType;
  ni.pmi = pmi;
  ni.ni_pPtr = ni_pPtr;
  ni.ni_bSelected = FALSE;
  return ctni;
}

// add model instance to tree view
HTREEITEM CDlgBarTreeView::AddModelInst(CModelInstance &mi, CModelInstance *pmiParent, HTREEITEM hParent)
{
  // insert model instance item
  HTREEITEM hItem;
  // expand only root model
  if(hParent == TVI_ROOT) {
    // add parent model instance
    hItem = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE , L"", 0, 0, TVIS_EXPANDED, TVIS_EXPANDED, 0, hParent, 0 );
    m_TreeCtrl.SetItemText(hItem,CString(mi.GetName()));
  } else {
    int iIcon = 0;
    if(mi.mi_fnSourceFile != pmiParent->mi_fnSourceFile) iIcon = 8;

    // add child model instance
    hItem = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE , L"", iIcon, iIcon, TVIS_EXPANDED, TVIS_EXPANDED, 0, hParent, 0 );
    // get parent bone name
    CTString strParentBoneName = ska_GetStringFromTable(mi.mi_iParentBoneID);
    CTString strText = mi.GetName() + " [" + strParentBoneName + "]";
    m_TreeCtrl.SetItemText(hItem,CString(strText));
  }
  m_TreeCtrl.SetItemData(hItem,AddNode(NT_MODELINSTANCE,&mi,pmiParent));

  // add its mesh instances
  AddMeshInstances(mi,hItem);
  // if skeleton exists 
  if(mi.mi_psklSkeleton != NULL) {
    // add skeleton
    AddSkeleton(mi,hItem);
  }
  // add animsets  
  AddAnimSet(mi,hItem);
  // add colision boxes
  AddColisionBoxes(mi,hItem);
  // add all frames colision box
  AddAllFramesBBox(mi,hItem);
  // add model instance children
  INDEX ctmi = mi.mi_cmiChildren.Count();
  // for each child in model isntance
  for(INDEX imi=0;imi<ctmi;imi++) {
    // add child 
    AddModelInst(mi.mi_cmiChildren[imi],&mi,hItem);
  }
  return hItem;
}

// add skeleton to tree view
void CDlgBarTreeView::AddSkeleton(CModelInstance &mi, HTREEITEM hParent)
{
  if(mi.mi_psklSkeleton == NULL) return;
  CSkeleton &sk = *mi.mi_psklSkeleton;
  // insert skeleton
  HTREEITEM hItem = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE  , L"", 5, 5, TVIS_SELECTED, TVIF_STATE, 0, hParent, 0 );
  CTString strSkeletonName;
  strSkeletonName.PrintF("%s [%d]",(const char*)sk.GetName().FileName(),sk.skl_aSkeletonLODs.Count());

  m_TreeCtrl.SetItemText(hItem,CString(strSkeletonName));
  m_TreeCtrl.SetItemData(hItem,AddNode(NT_SKELETONLODLIST,&sk,&mi));

  INDEX ctslod = sk.skl_aSkeletonLODs.Count();
  for(INDEX islod=0;islod<ctslod;islod++) {
    SkeletonLOD &slod = sk.skl_aSkeletonLODs[islod];
    // insert skeleton lod
    HTREEITEM hSlod = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"", 5, 5, TVIS_SELECTED, TVIF_STATE, 0, hItem, 0 );
    // count bones
    INDEX ctb = slod.slod_aBones.Count();

    CTString strText;
    CTFileName fnSlodSource = sk.skl_aSkeletonLODs[islod].slod_fnSourceFile;
    fnSlodSource = fnSlodSource.FileName();
    strText.PrintF("%s [%g]-[%d]",(const char*)fnSlodSource.NoExt(),sk.skl_aSkeletonLODs[islod].slod_fMaxDistance,ctb);
    m_TreeCtrl.SetItemText(hSlod,CString(strText));
    m_TreeCtrl.SetItemData(hSlod,AddNode(NT_SKELETONLOD,&slod,&mi));

    for(INDEX ib=0;ib<ctb;ib++) {
      SkeletonBone &sb = sk.skl_aSkeletonLODs[islod].slod_aBones[ib];
      // insert bone
      HTREEITEM hBone = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"", 7, 7, TVIS_SELECTED, TVIF_STATE, 0, hSlod , 0 );
      m_TreeCtrl.SetItemText(hBone,CString(ska_GetStringFromTable(sb.sb_iID)));
      m_TreeCtrl.SetItemData(hBone,AddNode(NT_BONE,&sb,&mi));
    }
  }
}

// add mesh surfaces to tree view
void CDlgBarTreeView::AddSurfaces(CModelInstance &mi,MeshLOD &mlod,HTREEITEM hParent)
{
  INDEX ctsrf = mlod.mlod_aSurfaces.Count();
  for(INDEX isrf=0;isrf<ctsrf;isrf++) {
    MeshSurface &msrf = mlod.mlod_aSurfaces[isrf];
    CShader *pShader =  msrf.msrf_pShader;
    CTString strShaderName;
    if(pShader!=NULL) strShaderName = pShader->GetName().FileName();
    HTREEITEM hItem = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  11, 11, TVIS_SELECTED, TVIF_STATE, 0, hParent, 0);
    CTString strSurfName;
    strSurfName.PrintF("%s [%d]-[%d]",(const char*)ska_GetStringFromTable(msrf.msrf_iSurfaceID),
      msrf.msrf_ctVertices,msrf.msrf_aTriangles.Count());
    m_TreeCtrl.SetItemText(hItem,CString(strSurfName));
    m_TreeCtrl.SetItemData(hItem,AddNode(NT_MESHSURFACE,&msrf,&mi));
  }
}

// add mesh instance to tree view
void CDlgBarTreeView::AddMeshInstances(CModelInstance &mi,HTREEITEM hParent)
{
  INDEX ctmsh = mi.mi_aMeshInst.Count();
  for(INDEX imsh=0;imsh<ctmsh;imsh++)
  {
    MeshInstance &mshi = mi.mi_aMeshInst[imsh];
    HTREEITEM hItem = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  1, 1, TVIS_SELECTED, TVIF_STATE, 0, hParent, 0);
    CTString strMeshName;
    strMeshName.PrintF("%s [%d]",(const char*)mshi.mi_pMesh->GetName().FileName(),mshi.mi_pMesh->msh_aMeshLODs.Count());
    m_TreeCtrl.SetItemText(hItem,CString(strMeshName));
    m_TreeCtrl.SetItemData(hItem,AddNode(NT_MESHLODLIST,&mshi,&mi));

    // add mesh lods
    INDEX ctmlod = mshi.mi_pMesh->msh_aMeshLODs.Count();
    for(INDEX imlod=0;imlod<ctmlod;imlod++)
    {
      MeshLOD &mlod = mshi.mi_pMesh->msh_aMeshLODs[imlod];
      HTREEITEM hMlod = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  3, 3, TVIS_SELECTED, TVIF_STATE, 0, hItem, 0);

      CTString strMeshLod;
      CTFileName fnMlodSource = mlod.mlod_fnSourceFile;
      fnMlodSource = fnMlodSource.FileName();

      strMeshLod.PrintF("%s [%g]-[%d]",(const char*)fnMlodSource.NoExt(),mlod.mlod_fMaxDistance,mlod.mlod_aVertices.Count());
      m_TreeCtrl.SetItemText(hMlod,CString(strMeshLod));
      m_TreeCtrl.SetItemData(hMlod,AddNode(NT_MESHLOD,&mlod,&mi));
      AddSurfaces(mi,mlod,hMlod);
    }
    // add textures for this mesh
    INDEX cttex = mshi.mi_tiTextures.Count();
    for(INDEX itex=0;itex<cttex;itex++)
    {
      TextureInstance &ti = mshi.mi_tiTextures[itex];
      CTString strTextName = ska_GetStringFromTable(ti.GetID());
      HTREEITEM hTexture = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  4, 4, TVIS_SELECTED, TVIF_STATE, 0, hItem, 0 );
      m_TreeCtrl.SetItemText(hTexture,CString(strTextName));
      m_TreeCtrl.SetItemData(hTexture,AddNode(NT_TEXINSTANCE,&ti,&mi));
    }
  }
}
// add collision boxes to tree view
void CDlgBarTreeView::AddColisionBoxes(CModelInstance &mi,HTREEITEM hParent)
{
  INDEX ctcb = mi.mi_cbAABox.Count();
  // for each collision box
  for(INDEX icb=0;icb<ctcb;icb++)
  {
    // add collision box
    ColisionBox &cb = mi.mi_cbAABox[icb];
    HTREEITEM hColisionBox = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  2, 2, TVIS_SELECTED, TVIF_STATE, 0, hParent, 0);
    m_TreeCtrl.SetItemText(hColisionBox,CString(cb.GetName()));
    m_TreeCtrl.SetItemData(hColisionBox,AddNode(NT_COLISIONBOX,&cb,&mi));
  }
}

void CDlgBarTreeView::AddAllFramesBBox(CModelInstance &mi,HTREEITEM hParent)
{
#pragma message(">> Remove AddAllFramesBBox")
  // add all frames bounding box
  ColisionBox &cb = mi.mi_cbAllFramesBBox;
  HTREEITEM hAllFramesBBox = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  6, 6, TVIS_SELECTED, TVIF_STATE, 0, hParent, 0);
  m_TreeCtrl.SetItemText(hAllFramesBBox,L"All frames BBox");
  m_TreeCtrl.SetItemData(hAllFramesBBox,AddNode(NT_ALLFRAMESBBOX,&cb,&mi));
}

// add anim set to tree view
HTREEITEM CDlgBarTreeView::AddAnimSet(CModelInstance &mi,HTREEITEM hParent)
{
  INDEX ctas = mi.mi_aAnimSet.Count();
  // for each animset
  for(INDEX ias=0;ias<ctas;ias++)
  {
    CAnimSet &as = mi.mi_aAnimSet[ias];
    INDEX ctan = as.as_Anims.Count();
    CTString strAnimSetName;
    strAnimSetName.PrintF("%s [%d]",(const char*)(as.GetName()).FileName(),ctan);
    HTREEITEM hAnimSet = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  9, 9, TVIS_SELECTED, TVIF_STATE, 0, hParent, 0);
    m_TreeCtrl.SetItemText(hAnimSet,CString(strAnimSetName));
    m_TreeCtrl.SetItemData(hAnimSet,AddNode(NT_ANIMSET,&as,&mi));
    // for each anim
    for(INDEX ian=0;ian<ctan;ian++)
    {
      Animation &an = as.as_Anims[ian];
      CTString strAnimName;
      //strAnimName.PrintF("%s [%d]",(const char*)ska_GetStringFromTable(an.an_iID),an.an_iFrames);
      strAnimName.PrintF("%s [%d]",(const char*)ska_GetStringFromTable(an.an_iID),an.an_iFrames);
      HTREEITEM hAnim = m_TreeCtrl.InsertItem( TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  9, 9, TVIS_SELECTED, TVIF_STATE, 0, hAnimSet, 0);
      m_TreeCtrl.SetItemText(hAnim,CString(strAnimName));
      m_TreeCtrl.SetItemData(hAnim,AddNode(NT_ANIMATION,&an,&mi));
      INDEX ctbe=an.an_abeBones.Count();
      for(INDEX ibe=0;ibe<ctbe;ibe++)
      {
        BoneEnvelope &be = an.an_abeBones[ibe];
        CTString strBoneEnvName;
        CTString strBoneName = ska_GetStringFromTable(be.be_iBoneID);

        INDEX ctr = be.be_arRot.Count();
        if(an.an_bCompresed) ctr = be.be_arRotOpt.Count();

        strBoneEnvName.PrintF("%s [%d]-[%d]",(const char*)strBoneName,ctr,be.be_apPos.Count());
        HTREEITEM hBoneEnv = m_TreeCtrl.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE , L"",  9, 9, TVIS_SELECTED, TVIF_STATE, 0, hAnim, 0);
        m_TreeCtrl.SetItemText(hBoneEnv,CString(strBoneEnvName));
        m_TreeCtrl.SetItemData(hBoneEnv,AddNode(NT_ANIM_BONEENV,&be,&mi));
      }
    }
  }
  return 0;
}
// add all model instances to combo box
void CDlgBarTreeView::FillParentDropDown(CModelInstance *pmi)
{
  if(pmi == NULL) return;
  CComboBox *cbParentList = ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTMODEL));
  INDEX iItem = cbParentList->AddString(CString(pmi->GetName()));
  cbParentList->SetItemDataPtr(iItem,pmi);
  // add all children to combo box
  INDEX ctmi = pmi->mi_cmiChildren.Count();
  for(INDEX imi=0;imi<ctmi;imi++)
  {
    FillParentDropDown(&pmi->mi_cmiChildren[imi]);
  }
}
// find out how many children from root to selected item
BOOL CDlgBarTreeView::RememberSelectedItem(HTREEITEM hParent,HTREEITEM hSelected)
{
  if(hParent==NULL) return FALSE;
  HTREEITEM hChild = m_TreeCtrl.GetChildItem(hParent);
  INDEX ctLoops = 0;
  while(hChild != NULL)
  {
    ctLoops++;
    if(m_TreeCtrl.ItemHasChildren(hChild))
    {
      if(RememberSelectedItem(hChild,hSelected))
      {
        INDEX &iCur = _aSelectItem.Push();
        iCur = ctLoops;
        return TRUE;
      }
    }
    if(hChild == hSelected)
    {
      INDEX &iCur = _aSelectItem.Push();
      iCur = ctLoops;
      return TRUE;
    }
    hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
  }
  return FALSE;
}
// use previously filed array of child depth to reach selected item
BOOL CDlgBarTreeView::ReselectItem(HTREEITEM hParent)
{
  INDEX ctrec=_aSelectItem.Count();
  // from last to first
  HTREEITEM hChild = hParent;
  // if child is NULL select his parent and return
  // loop filled array of recursion depth for selected item
  for(INDEX irec=ctrec-1;irec>=0;irec--)
  {
    HTREEITEM hRet = m_TreeCtrl.GetChildItem(hChild);
    if(hRet == NULL)
    {
        m_TreeCtrl.SelectItem(hChild);
        return FALSE;
    }
    else hChild = hRet;

    INDEX cti=_aSelectItem[irec];
    for(INDEX i=0;i<cti-1;i++)
    {
      HTREEITEM hRet = m_TreeCtrl.GetNextSiblingItem(hChild);
      if(hRet == NULL)
      {
        m_TreeCtrl.SelectItem(hChild);
        return FALSE;
      }
      else hChild = hRet;
    }
  }
  if(hChild != NULL)
  {
    m_TreeCtrl.SelectItem(hChild);
  }
  return TRUE;
}
// update tree view containing whole hierarchy of model instance
void CDlgBarTreeView::UpdateModelInstInfo(CModelInstance *pmi)
{
  m_TreeCtrl.SetRedraw(FALSE);
//  ShowControlGroup(-1);
  HTREEITEM htSelectedItem = m_TreeCtrl.GetSelectedItem();
  HTREEITEM hRoot = m_TreeCtrl.GetRootItem();

  CTString strRoot;
  // if root item exists
  if(hRoot!=NULL) {
    // remember its name
    strRoot = CStringA(m_TreeCtrl.GetItemText(hRoot));
  }

  // clear array for selected item
  _aSelectItem.PopAll();
  _aiSelectedMeshSurfaces.PopAll();
  // get current selected item and fill array of depths how to reach it
  RememberSelectedItem(hRoot,htSelectedItem);
  theApp.iSelectedBoneID = -1;

  INDEX iSelIndex=0;
  NodeInfo niSelected;
  if(htSelectedItem != NULL)
  {
    iSelIndex = m_TreeCtrl.GetItemData(htSelectedItem);
    niSelected = theApp.aNodeInfo[iSelIndex];
  }

  m_TreeCtrl.DeleteAllItems();
  theApp.aNodeInfo.PopAll();

  // reset combo box
  ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTMODEL))->ResetContent();

  if(pmi == NULL) {
    m_TreeCtrl.SetRedraw(TRUE);
    return;
  }
  
  // fill combo with all parents
  FillParentDropDown(pmi);
  // fill tree ctrl with all hierarchy
  HTREEITEM hParent = AddModelInst(*pmi,NULL,TVI_ROOT);

  // get name of root item
  CTString strNewRoot = CStringA(m_TreeCtrl.GetItemText(hParent));
  // if root item name is different then old root item name clear selection
  if(strRoot != strNewRoot && strRoot.Length() > 0) _aSelectItem.PopAll();
  if(_aSelectItem.Count() > 0)
  {
    // try to select item that was selected before reloading
    ReselectItem(hParent);
  }
  else
  {
    // select hParent
    m_TreeCtrl.SelectItem(hParent);
  }
  m_TreeCtrl.SetRedraw(TRUE);
}
// check custom seconds per frame check box
void CDlgBarTreeView::CheckSecPerFrameCtrl(BOOL bCheck)
{
  ((CButton*)m_dlgAnimSet.GetDlgItem(IDC_CB_SECPERFRAME))->SetCheck(bCheck);
  m_dlgAnimSet.GetDlgItem(IDC_EB_SECPERFRAME)->EnableWindow(bCheck);
}
// change tab in tab control
void CDlgBarTreeView::OnSelchangeModeSelectTab(NMHDR* pNMHDR, LRESULT* pResult) 
{
//  CTabCtrl* pTab = (CTabCtrl*)GetDlgItem(IDC_MODE_SELECT_TAB);
//  ShowControlGroup(pTab->GetCurSel());
	*pResult = 0;
}
// expand all model instances in tree view
void CDlgBarTreeView::ExpandAllModelInstances(HTREEITEM hItem)
{
  INDEX iIndex = m_TreeCtrl.GetItemData(hItem);
  NodeInfo &ni = theApp.aNodeInfo[iIndex];
  // it this item is model instance
  if(ni.ni_iType == NT_MODELINSTANCE)
  {
    // expand it
    m_TreeCtrl.Expand(hItem,TVE_EXPAND);
  }

  // check if current item has children
  if(m_TreeCtrl.ItemHasChildren(hItem))
  {
    HTREEITEM hChild = m_TreeCtrl.GetChildItem(hItem);
    while(TRUE)
    {
      // expand all model instance in this child
      ExpandAllModelInstances(hChild);
      // get next item
      hChild = m_TreeCtrl.GetNextSiblingItem(hChild);
      if(hChild==NULL)
      {
        break;
      }
    }
  }
}

// put all bones of selected skeleton in combo box
void CDlgBarTreeView::FillBonesToComboBox(CSkeleton *pskl,INDEX iSelectedIndex)
{
  // delete all bones from combo box
  ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->ResetContent();
  // if skeleton does not exist return
  if(pskl == NULL) return;
  // count skeleton lods
  INDEX ctslod = pskl->skl_aSkeletonLODs.Count();
  if(ctslod<1) return;

  SkeletonLOD *pslod = &pskl->skl_aSkeletonLODs[0];
  // if lod doesnt exist return;
  if(pslod == NULL) return;
  // count bones in skeleton lod
  INDEX ctsb = pslod->slod_aBones.Count();
  // for each bone in skeleton lod
  for(INDEX isb=0;isb<ctsb;isb++)
  {
    SkeletonBone &sb = pslod->slod_aBones[isb];
    // add bone to combo box
    ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->AddString(CString(ska_GetStringFromTable(sb.sb_iID)));
  }
  ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->SetCurSel(iSelectedIndex);
}

// set text for 'custom' tab in tab control
void CDlgBarTreeView::SetCustomTabText(wchar_t *strText)
{
  // fill tab control item
  TCITEM tcitem;
  memset(&tcitem,0,sizeof(tcitem));
  tcitem.mask = TCIF_TEXT;
  tcitem.cchTextMax = 256;
  tcitem.pszText = strText;
  ((CTabCtrl*)GetDlgItem(IDC_MODE_SELECT_TAB))->SetItem(2,&tcitem);
}
// reset all controls on dialog
void CDlgBarTreeView::ResetControls()
{
  m_TreeCtrl.DeleteAllItems();
  m_TreeCtrl.hLastSelected = NULL;
  GetDlgItem(IDC_SELECTEDMI)->SetWindowText(L"(none)");
  ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTBONE))->ResetContent();
  ((CComboBox*)m_dlgParent.GetDlgItem(IDC_CB_PARENTMODEL))->ResetContent();
  ((CButton*)m_dlgAnimSet.GetDlgItem(IDC_CB_COMPRESION))->SetCheck(FALSE);
  CheckSecPerFrameCtrl(FALSE);
  ShowControlGroup(-1);

  
  m_tbOffPosX.SetWindowText(L"");
  m_tbOffPosY.SetWindowText(L"");
  m_tbOffPosZ.SetWindowText(L"");
  m_tbOffRotH.SetWindowText(L"");
  m_tbOffRotP.SetWindowText(L"");
  m_tbOffRotB.SetWindowText(L"");

  m_tbTreshold.SetWindowText(L"");
  m_tbAnimSpeed.SetWindowText(L"");

  m_tbColName.SetWindowText(L"");
  m_tbColWidth.SetWindowText(L"");
  m_tbColHeight.SetWindowText(L"");
  m_tbColLength.SetWindowText(L"");
  m_tbColPosX.SetWindowText(L"");
  m_tbColPosY.SetWindowText(L"");
  m_tbColPosZ.SetWindowText(L"");

  m_tbDistance.SetWindowText(L"");
  //GET_CTRL(IDC_CB_TEXNAME)->SetWindowText("");
}

void CDlgBarTreeView::OnSize(UINT nType, int cx, int cy) 
{
  /*
  // if app has initialized
  if(theApp.bAppInitialized) {
    // adjust splitter
    AdjustSplitter();
  }
*/
	CDlgTemplate::OnSize(nType, cx, cy);
}
