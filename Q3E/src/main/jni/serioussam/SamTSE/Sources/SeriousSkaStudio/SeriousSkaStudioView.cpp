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

// SeriousSkaStudioView.cpp : implementation of the CSeriousSkaStudioView class
//

#include "stdafx.h"
#include "SeriousSkaStudio.h"

#include "SeriousSkaStudioDoc.h"
#include "SeriousSkaStudioView.h"

#include <Engine/Templates/Stock_CMesh.h>
#include <Engine/Templates/Stock_CSkeleton.h>
#include <Engine/Templates/Stock_CAnimSet.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Graphics/GfxLibrary.h>

#include "MainFrm.h"


#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL _bSelectedItemChanged = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioView

IMPLEMENT_DYNCREATE(CSeriousSkaStudioView, CView)

BEGIN_MESSAGE_MAP(CSeriousSkaStudioView, CView)
	//{{AFX_MSG_MAP(CSeriousSkaStudioView)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_RESET_VIEW, OnResetView)
	ON_COMMAND(ID_SHOW_WIREFRAME, OnShowWireframe)
	ON_COMMAND(ID_SHOW_SKELETON, OnShowSkeleton)
	ON_COMMAND(ID_SHOW_TEXTURE, OnShowTexture)
	ON_COMMAND(ID_ADD_MESHLOD, OnAddMeshlod)
	ON_COMMAND(ID_ADD_ANIMATION, OnAddAnimation)
	ON_COMMAND(ID_ADD_SKELETONLOD, OnAddSkeletonlod)
	ON_COMMAND(ID_DELETESELECTED, OnDeleteselected)
	ON_COMMAND(ID_ADD_ANIMSET, OnAddAnimset)
	ON_COMMAND(ID_ADD_MESHLIST, (AFX_PMSG)OnAddMeshlist)
	ON_COMMAND(ID_ADD_SKELETONLIST, OnAddSkeletonlist)
	ON_COMMAND(ID_ADD_TEXTURE, OnAddTexture)
	ON_COMMAND(ID_ADD_CHILD_MODEL_INSTANCE, OnAddChildModelInstance)
	ON_COMMAND(ID_ADD_COLISIONBOX, OnAddColisionbox)
	ON_COMMAND(ID_ANIM_STOP, OnAnimStop)
	ON_COMMAND(ID_ANIM_SYNC, OnAnimSync)
	ON_COMMAND(ID_AUTO_MIPING, OnAutoMiping)
	ON_UPDATE_COMMAND_UI(ID_AUTO_MIPING, OnUpdateAutoMiping)
	ON_COMMAND(ID_SHOW_GROUND, OnShowGround)
	ON_UPDATE_COMMAND_UI(ID_SHOW_GROUND, OnUpdateShowGround)
	ON_UPDATE_COMMAND_UI(ID_SHOW_SKELETON, OnUpdateShowSkeleton)
	ON_UPDATE_COMMAND_UI(ID_SHOW_TEXTURE, OnUpdateShowTexture)
	ON_UPDATE_COMMAND_UI(ID_SHOW_WIREFRAME, OnUpdateShowWireframe)
	ON_COMMAND(ID_SHOW_ANIM_QUEUE, OnShowAnimQueue)
	ON_UPDATE_COMMAND_UI(ID_SHOW_ANIM_QUEUE, OnUpdateShowAnimQueue)
	ON_COMMAND(ID_FILE_SAVEMI, OnFileSaveModel)
	ON_COMMAND(ID_SHOW_NORMALS, OnShowNormals)
	ON_UPDATE_COMMAND_UI(ID_SHOW_NORMALS, OnUpdateShowNormals)
	ON_COMMAND(ID_SHOW_LIGHTS, OnShowLights)
	ON_UPDATE_COMMAND_UI(ID_SHOW_LIGHTS, OnUpdateShowLights)
	ON_COMMAND(ID_CHANGE_AMBIENTCOLOR, OnChangeAmbientcolor)
	ON_COMMAND(ID_CHANGE_LIGHTCOLOR, OnChangeLightcolor)
	ON_COMMAND(ID_ANIM_LOOP, OnAnimLoop)
	ON_UPDATE_COMMAND_UI(ID_ANIM_LOOP, OnUpdateAnimLoop)
	ON_COMMAND(ID_ANIM_PAUSE, OnAnimPause)
	ON_UPDATE_COMMAND_UI(ID_ANIM_PAUSE, OnUpdateAnimPause)
	ON_COMMAND(ID_SHOW_COLISION, OnShowColision)
	ON_UPDATE_COMMAND_UI(ID_SHOW_COLISION, OnUpdateShowColision)
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_COMMAND(ID_FILE_SAVEMI_AS, OnFileSavemiAs)
	ON_COMMAND(ID_FILE_RECREATETEXTURE, OnFileRecreatetexture)
	ON_UPDATE_COMMAND_UI(ID_FILE_RECREATETEXTURE, OnUpdateFileRecreatetexture)
	ON_COMMAND(ID_VK_DOWN, OnVkDown)
	ON_COMMAND(ID_VK_UP, OnVkUp)
	ON_COMMAND(ID_VK_ESCAPE, OnVkEscape)
	ON_COMMAND(ID_CREATE_ADD_TEXTURE, OnCreateAddTexture)
	ON_COMMAND(ID_ADD_TEXTURE_BUMP, OnAddTextureBump)
	ON_COMMAND(ID_ADD_TEXTURE_REFLECTION, OnAddTextureReflection)
	ON_COMMAND(ID_ADD_TEXTURE_SPECULAR, OnAddTextureSpecular)
	ON_COMMAND(ID_SHOW_ACTIVE_SKELETON, OnShowActiveSkeleton)
	ON_UPDATE_COMMAND_UI(ID_SHOW_ACTIVE_SKELETON, OnUpdateShowActiveSkeleton)
	ON_COMMAND(ID_SHOW_ALL_FRAMES_BBOX, OnShowAllFramesBbox)
	ON_UPDATE_COMMAND_UI(ID_SHOW_ALL_FRAMES_BBOX, OnUpdateShowAllFramesBbox)
	ON_COMMAND(ID_MODELINSTANCE_SAVEWITHOFFSET, OnModelinstanceSavewithoffset)
	ON_COMMAND(ID_VK_LEFT, OnVkLeft)
	ON_COMMAND(ID_VK_RIGHT, OnVkRight)
	ON_COMMAND(ID_VK_LEFT_WITH_CTRL, OnVkLeftWithCtrl)
	ON_COMMAND(ID_VK_RIGHT_WITH_CTRL, OnVkRightWithCtrl)
	ON_COMMAND(ID_CONVERT_SELECTED, OnConvertSelected)
	ON_COMMAND(ID_RESET_COLISIONBOX, OnResetColisionbox)
	ON_COMMAND(ID_ALL_FRAMES_RECALC, OnAllFramesRecalc)
	ON_COMMAND(ID_RELOAD_TEXTURE, OnReloadTexture)
	ON_COMMAND(ID_RECREATE_TEXTURE, OnRecreateTexture)
	ON_COMMAND(ID_BROWSE_TEXTURE, OnBrowseTexture)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioView construction/destruction

CSeriousSkaStudioView::CSeriousSkaStudioView()
{
  m_fFOV = 60.0f;
  m_pdpDrawPort = NULL;
  m_pvpViewPort = NULL;
  m_angModelAngle = ANGLE3D(0.0f, 0.0f, 0.0f);
  m_plLightPlacement.pl_PositionVector = FLOAT3D( 0.0f, 0.0f, 0.0f); // center
  m_plLightPlacement.pl_OrientationAngle(1) = AngleDeg( 45.0f);      // heading
  m_plLightPlacement.pl_OrientationAngle(2) = AngleDeg( -45.0f);     // pitch
  m_plLightPlacement.pl_OrientationAngle(3) = AngleDeg( 0.0f);       // banking
  m_fLightDistance = 3.5f;
  OnResetView();
}

CSeriousSkaStudioView::~CSeriousSkaStudioView()
{
}

void CSeriousSkaStudioView::SetProjectionData( CPerspectiveProjection3D &prProjection, CDrawPort *pDP)
{
  prProjection.FOVL() = AngleDeg(m_fFOV);
  prProjection.ScreenBBoxL() = FLOATaabbox2D( FLOAT2D(0.0f,0.0f),
  FLOAT2D((float)pDP->GetWidth(), (float)pDP->GetHeight()));
  prProjection.AspectRatioL() = 1.0f;
  prProjection.FrontClipDistanceL() = 0.05f;

  prProjection.ViewerPlacementL().pl_PositionVector = m_vTarget;
  prProjection.ViewerPlacementL().pl_OrientationAngle = m_angViewerOrientation;
  prProjection.Prepare();
  prProjection.ViewerPlacementL().Translate_OwnSystem(FLOAT3D( 0.0f, 0.0f, m_fTargetDistance));
}

void CSeriousSkaStudioView::OnIdle(void)
{
  Invalidate(FALSE);
}

BOOL CSeriousSkaStudioView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioView drawing
static INDEX ctChildLevel=0;
#define MAKESPACE(x) (x>0?"%*c":""),x,' '
void CreateCurrentAnimationList(CModelInstance *pmi,CTString &strAnimations)
{
  CTString strText;
  CTString strTemp;

  // return if no animsets
  INDEX ctas = pmi->mi_aAnimSet.Count();
  if(ctas == 0) {
    strText.PrintF(MAKESPACE(ctChildLevel));
    strText += CTString(0,"[%s]\n",(const char*)pmi->GetName());
    strAnimations += strText;
    return;
  }

  // count animlists
  INDEX ctal = pmi->mi_aqAnims.aq_Lists.Count();
  // find newes animlist that has fully faded in
  INDEX iFirstAnimList = 0;
  // loop from newer to older
  INDEX ial=ctal-1;
  for(;ial>=0;ial--) {
    AnimList &alList = pmi->mi_aqAnims.aq_Lists[ial];
    // calculate fade factor
    FLOAT fFadeFactor = CalculateFadeFactor(alList);
    if(fFadeFactor >= 1.0f) {
      iFirstAnimList = ial;
      break;
    }
  }

  strText.PrintF(MAKESPACE(ctChildLevel));
  strText += CTString(0,"[%s]\n",(const char*)pmi->GetName());
  strAnimations += strText;

  // for each anim list after iFirstAnimList
  for(ial=iFirstAnimList;ial<ctal;ial++) {
    AnimList &alList = pmi->mi_aqAnims.aq_Lists[ial];
    AnimList *palListNext=NULL;
    if(ial+1<ctal) palListNext = &pmi->mi_aqAnims.aq_Lists[ial+1];
    
    INDEX ctpa = alList.al_PlayedAnims.Count();
    // for each played anim in played anim list
    for(int ipa=0;ipa<ctpa;ipa++) {
      FLOAT fTime = _pTimer->GetLerpedCurrentTick();
      PlayedAnim &pa = alList.al_PlayedAnims[ipa];

      strText.PrintF(MAKESPACE(ctChildLevel+1));
      strAnimations += strText;

      BOOL bAnimLooping = pa.pa_ulFlags & AN_LOOPING;

      INDEX iAnimSetIndex;
      INDEX iAnimIndex;
      // find anim by ID in all anim sets within this model
      if(pmi->FindAnimationByID(pa.pa_iAnimID,&iAnimSetIndex,&iAnimIndex)) {
        // if found, animate bones
        Animation &an = pmi->mi_aAnimSet[iAnimSetIndex].as_Anims[iAnimIndex];
        
        // calculate end time for this animation list
        FLOAT fEndTime = alList.al_fStartTime + alList.al_fFadeTime;
        // calculate curent and next frame in animation
        if(palListNext!=NULL) {
          if(fTime > palListNext->al_fStartTime) fTime = palListNext->al_fStartTime;
        }

        if(fTime < fEndTime) fTime = fEndTime;
        FLOAT f = (fTime - fEndTime) / (TIME)an.an_fSecPerFrame;

        INDEX iCurentFrame;
        INDEX iAnimFrame,iNextAnimFrame;
        
        if(bAnimLooping) {
          f = fmod(f,an.an_iFrames);
          iCurentFrame = INDEX(f);
          iAnimFrame = iCurentFrame % an.an_iFrames;
          iNextAnimFrame = (iCurentFrame+1) % an.an_iFrames;
        } else {
          if(f>an.an_iFrames) f = an.an_iFrames-1;
          iCurentFrame = INDEX(f);
          iAnimFrame = ClampUp(iCurentFrame,an.an_iFrames-1L);
          iNextAnimFrame = ClampUp(iCurentFrame+1L,an.an_iFrames-1L);
        }

        FLOAT fAnimLength = an.an_iFrames * an.an_fSecPerFrame;
        FLOAT fFadeFactor = CalculateFadeFactor(alList);
        strText.PrintF("%s %g - %g",ska_GetStringFromTable(pa.pa_iAnimID),f,fFadeFactor);
        INDEX iLength = strText.Length();
        if(iLength<30) {
          strTemp.PrintF(MAKESPACE(30-iLength));
          strText+=strTemp;
        }
        strTemp.PrintF("[%g / %g]\n",f*an.an_fSecPerFrame,fAnimLength);
        strAnimations += strText+strTemp;
      }
    }
    strText.PrintF(MAKESPACE(ctChildLevel+1));
    strAnimations += strText+"-----------------------\n";
  }
        
  INDEX ctcmi = pmi->mi_cmiChildren.Count();
  ctChildLevel+=2;
  // for each child model instance
  for(INDEX icmi=0;icmi<ctcmi;icmi++)   {
    CModelInstance *pcmi = &pmi->mi_cmiChildren[icmi];
    // create list of animations for child
    CreateCurrentAnimationList(pcmi,strAnimations);
  }
  ctChildLevel-=2;
}

INDEX CSeriousSkaStudioView::TestRayCastHit(CPoint &pt) 
{
  INDEX iHitBone = -1;
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  ASSERT(pDoc!=NULL);
  if(pDoc!=NULL) {
    CModelInstance *pmi = pDoc->m_ModelInstance;
    ASSERT(pmi!=NULL);
    if(pmi!=NULL) {
    
      INDEX iBoneID = -1;
      FLOATmatrix3D mat;
      mat.Diagonal(1);
      // get viewer's direction vector

      // set projection data
      CPerspectiveProjection3D prPerspectiveProjection;
      SetProjectionData( prPerspectiveProjection, m_pdpDrawPort);

      // prepare render model structure
      CAnyProjection3D apr;
      apr = prPerspectiveProjection;
      apr->Prepare();


      CPlacement3D plRay;
      apr->RayThroughPoint(FLOAT3D((float)pt.x,
        m_pdpDrawPort->GetHeight()-(float)pt.y, 0.0f), plRay);

      FLOAT3D vDirection;
      AnglesToDirectionVector( plRay.pl_OrientationAngle, vDirection);
      FLOAT3D vResult;
      vResult = plRay.pl_PositionVector + vDirection;

      RM_TestRayCastHit(*pmi,mat,FLOAT3D(0,0,0),plRay.pl_PositionVector,vResult,300000,&iBoneID);
      if(iBoneID>=0) {
        iHitBone = iBoneID;
      }
    }
  }
  return iHitBone;
}

void CSeriousSkaStudioView::RenderLightModels(CDrawPort *pdp,CPlacement3D &pl)
{

  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  // show light objects
  if(pDoc!=NULL && pDoc->bShowLights && theApp.pmiLight!=NULL)
  {
    CPlacement3D plLightPlacement;
    plLightPlacement.pl_OrientationAngle = m_plLightPlacement.pl_OrientationAngle;
    plLightPlacement.pl_PositionVector = FLOAT3D(0,0,0);
    plLightPlacement.Translate_OwnSystem( FLOAT3D( 0.0f, 0.0f, m_fLightDistance));
    if(pDoc->m_ModelInstance != NULL) {
      plLightPlacement.Translate_AbsoluteSystem( pDoc->m_ModelInstance->GetOffsetPos());
    }


    // back up current model flags
    ULONG ulModelFlags = RM_GetFlags();
    // change render flags
    RM_SetFlags(RMF_SHOWTEXTURE);
    
    RM_SetObjectPlacement(plLightPlacement);
    // render light model 
    RM_RenderSKA(*theApp.pmiLight);
    
    // restore model flags
    RM_SetFlags(ulModelFlags);
  }
}

void AdjustBonesCallback(void *pData)
{
  // aditionaly adjust bones 
}

void CSeriousSkaStudioView::RenderView(CDrawPort *pdp)
{
  if(theApp.GetDisableRenderRequests()>0) return;

  CSeriousSkaStudioDoc* pDoc = GetDocument();

  pDoc->SetTimerForDocument();

  pdp->Fill(C_lGRAY | CT_OPAQUE);
  pdp->FillZBuffer(ZBUF_BACK);
  pdp->SetTextLineSpacing( 64);
  pdp->SetFont( _pfdDisplayFont);
  pdp->SetTextAspect( 1.0f);
      
  CTString strTest;
  strTest.PrintF("Serious SKA Studio");
  
  FLOAT fX = pdp->dp_Width - 10 - pdp->GetTextWidth(strTest);
  FLOAT fY = pdp->dp_Height - 8 - pdp->dp_FontData->fd_pixCharHeight;
  
  // pdp->PutText( strTest, fX, fY, 0x0055ff00|CT_OPAQUE);

  CPerspectiveProjection3D prPerspectiveProjection;

  CPlacement3D pl;
  pl.pl_OrientationAngle = m_angModelAngle;
  pl.pl_PositionVector   = FLOAT3D(0.0f, 0.0f, 0.0f);

  if(pmiSelected!=NULL) {
    // Adjust custom model instance stretch
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CString strStretch;
    pMainFrame->m_ctrlMIStretch.GetWindowText(strStretch);
    FLOAT fStretch = atof(CStringA(strStretch));
    pmiSelected->mi_vStretch = FLOAT3D(fStretch,fStretch,fStretch);
  }


  if(pDoc->m_ModelInstance != NULL)
  {
    // set projection data
    SetProjectionData( prPerspectiveProjection, pdp);

    // prepare render model structure
    CAnyProjection3D apr;
    apr = prPerspectiveProjection;

    RM_BeginRenderingView(apr, m_pdpDrawPort);
    // set placement of model
    CPlacement3D pl;
    pl.pl_OrientationAngle = m_angModelAngle;
    FLOAT fZPos = pDoc->m_fSpeedZ*fmod(_pTimer->GetLerpedCurrentTick(),pDoc->m_fLoopSecends);
    pl.pl_PositionVector   = FLOAT3D(0.0f, 0.0f, -fZPos);
    RM_SetObjectPlacement(pl);

    // Set color of shading light
    RM_SetLightColor(pDoc->m_colAmbient, pDoc->m_colLight);
    // Set light direction
    FLOAT3D vLightDir;
    AnglesToDirectionVector(pDoc->m_vLightDir,vLightDir);

    CPlacement3D plLightPlacement;
    plLightPlacement.pl_OrientationAngle = m_plLightPlacement.pl_OrientationAngle;
    plLightPlacement.pl_PositionVector = FLOAT3D(0,0,0);
    plLightPlacement.Translate_OwnSystem( FLOAT3D( 0.0f, 0.0f, m_fLightDistance));
    if(pDoc->m_ModelInstance != NULL) {
      plLightPlacement.Translate_AbsoluteSystem( pDoc->m_ModelInstance->GetOffsetPos());
    }

    RM_SetLightDirection(-plLightPlacement.pl_PositionVector);
    //

    // if ground is visible
    if(pDoc->bShowGround) {
      RM_RenderGround(theApp.toGroundTexture);
    }

    // Fix this
    if(pDoc->bAutoMiping) {
      // auto mipping
      RM_SetCustomMeshLodDistance(-1);
      RM_SetCustomSkeletonLodDistance(-1);
    } else {
      // custom mipping
      RM_SetCustomMeshLodDistance(pDoc->fCustomMeshLodDist);
      RM_SetCustomSkeletonLodDistance(pDoc->fCustomSkeletonLodDist);
    }
    
    /*
    // Render shadow
    FLOATplane3D plShadowPlane = FLOATplane3D(FLOAT3D(0,1,0),0);
    pDoc->m_ModelInstance->AddSimpleShadow(1.0f,plShadowPlane);
    */

    RM_SetBoneAdjustCallback(&AdjustBonesCallback,this);

    // render model
    RM_RenderSKA(*pDoc->m_ModelInstance);
    // render selected bone
    if(theApp.iSelectedBoneID >= 0) {
      RM_RenderBone(*pDoc->m_ModelInstance,theApp.iSelectedBoneID);
    }


    CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
    HTREEITEM hSelectedItem = m_TreeCtrl.GetSelectedItem();
    NodeInfo *pni = NULL;
    if(hSelectedItem!=NULL) {
      pni = &theApp.m_dlgBarTreeView.GetNodeInfo(hSelectedItem);
    }
    
    BOOL bShowColisionBoxes = pDoc->bShowColisionBox;
    if(pni!=NULL && pni->ni_iType == NT_COLISIONBOX) {
      bShowColisionBoxes = TRUE;
    }
    BOOL bShowAllFramesBBox = pDoc->bShowAllFramesBBox;
    if(pni!=NULL && pni->ni_iType == NT_ALLFRAMESBBOX) {
      bShowAllFramesBBox = TRUE;
    }

    // show colision box
    if(pmiSelected->mi_cbAABox.Count()>0 &&pmiSelected->mi_iCurentBBox>=0) {
      if(bShowColisionBoxes) {
        ColisionBox &cb = pmiSelected->GetColisionBox(pmiSelected->mi_iCurentBBox);
        RM_RenderColisionBox(*pmiSelected,cb,C_mlGREEN);
      }
    }
    if(bShowAllFramesBBox) {
      ColisionBox &cbAllFrames = pmiSelected->mi_cbAllFramesBBox;
      RM_RenderColisionBox(*pmiSelected,cbAllFrames,C_ORANGE);
    }

    RenderLightModels(pdp,pl);

    RM_EndRenderingView();
    // show paused indicator
    if(pDoc->m_bViewPaused) {
      TIME tmNow = _pTimer->GetHighPrecisionTimer().GetSeconds();
      ULONG ulAlpha = sin(tmNow*16)*96 +128;
      pdp->SetFont( _pfdConsoleFont);
      pdp->PutText( "Paused", m_pdpDrawPort->dp_Width-50, ClampDn<PIX>(m_pdpDrawPort->dp_Height-15,0), 0xffff0000|ulAlpha);
    }

    // Show anim queue
    if(theApp.bShowAnimQueue) {
      pdp->Fill(0,0,200,m_pdpDrawPort->dp_Height,0x0000007f,0x00000000,0x0000007f,0x00000000);
      // show curent played animations
      pdp->SetFont( _pfdConsoleFont);

      CTString strAnimations;
      CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
      // strAnimations.PrintF("Test %g\n",tvNow.GetMilliseconds());
      CreateCurrentAnimationList(pDoc->m_ModelInstance,strAnimations);
      pdp->PutText( strAnimations, 0, 0, 0xffffff00|CT_OPAQUE);
    }

    /*
    if(theApp.iSelectedBoneID>=0) {
      FLOAT3D vStartPoint;
      FLOAT3D vEndPoint;
      if(RM_GetBoneAbsPosition(*pDoc->m_ModelInstance,theApp.iSelectedBoneID,vStartPoint,vEndPoint)) {
        CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
        if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgBone) {
          FLOAT3D vDirection = vEndPoint - vStartPoint;
          ANGLE3D angAngle;
          DirectionVectorToAngles(vDirection,angAngle);
          
          pDlgCurrent->GetDlgItem(IDC_LB_POSX)->SetWindowText((const char*)CTString(0,"%g",vStartPoint(1)));
          pDlgCurrent->GetDlgItem(IDC_LB_POSY)->SetWindowText((const char*)CTString(0,"%g",vStartPoint(2)));
          pDlgCurrent->GetDlgItem(IDC_LB_POSZ)->SetWindowText((const char*)CTString(0,"%g",vStartPoint(3)));
          pDlgCurrent->GetDlgItem(IDC_LB_HEADING)->SetWindowText((const char*)CTString(0,"%g",angAngle(1)));
          pDlgCurrent->GetDlgItem(IDC_LB_PITCH)->SetWindowText((const char*)CTString(0,"%g",angAngle(2)));
          pDlgCurrent->GetDlgItem(IDC_LB_BANKING)->SetWindowText((const char*)CTString(0,"%g",angAngle(3)));
        } else {
          ASSERT(FALSE); // Current client dialog must be texture dialog
        }
      }
    }*/
  }
}


void CSeriousSkaStudioView::OnDraw(CDC* pDC)
{
	CSeriousSkaStudioDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
  // if there is a valid drawport, and the drawport can be locked
  if( m_pdpDrawPort!=NULL && m_pdpDrawPort->Lock())
  {
    // render view
    RenderView( m_pdpDrawPort);

    // unlock the drawport
    m_pdpDrawPort->Unlock();

    // swap if there is a valid viewport
    if( m_pvpViewPort!=NULL)
    {
      m_pvpViewPort->SwapBuffers();
    }
  }
}

void CSeriousSkaStudioView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
  // get mainfrm
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get mdi client
  CMDIClientWnd *pMDIClient = &pMainFrame->m_wndMDIClient;
  // get mdi client rect
  CRect rc;
  pMDIClient->GetWindowRect(&rc);
  INDEX iWidth = rc.right - rc.left - 4;
  INDEX iHeight = rc.bottom - rc.top - 4;
  // check if view is maximized
  if((iWidth == cx) && (iHeight == cy))
  {
  	m_iViewSize = SIZE_MAXIMIZED;
    theApp.bChildrenMaximized = TRUE;
  }
  else 
  {
    m_iViewSize = 0;
    theApp.bChildrenMaximized = FALSE;
  }


  // if we are not in game mode and changing of display mode is not on
  if( m_pvpViewPort!=NULL)
  { // resize it
    m_pvpViewPort->Resize();
  }
}

void CSeriousSkaStudioView::OnDestroy() 
{
	CView::OnDestroy();
	
	// destroy canvas that is currently used
  _pGfx->DestroyWindowCanvas( m_pvpViewPort);
  m_pvpViewPort = NULL;

  CView::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioView diagnostics

#ifdef _DEBUG
void CSeriousSkaStudioView::AssertValid() const
{
	CView::AssertValid();
}

void CSeriousSkaStudioView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSeriousSkaStudioDoc* CSeriousSkaStudioView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSeriousSkaStudioDoc)));
	return (CSeriousSkaStudioDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioView message handlers

void CSeriousSkaStudioView::OnInitialUpdate() 
{
  CView::OnInitialUpdate();
  
  CSeriousSkaStudioDoc* pDoc = GetDocument();
  // at this time, m_hWnd is valid, so we do canvas initialization here
 	_pGfx->CreateWindowCanvas(m_hWnd, &m_pvpViewPort, &m_pdpDrawPort);
}

void CSeriousSkaStudioView::OnMouseMove(UINT nFlags, CPoint point) 
{
  BOOL bSpace = (GetKeyState(' ') & 128) != 0;  
  BOOL bLMB  = nFlags & MK_LBUTTON;
  BOOL bRMB  = nFlags & MK_RBUTTON;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bShift = nFlags & MK_SHIFT;


  CPoint pntDelta = point - m_pntLastMouse;  
  m_pntLastMouse = point;
  FLOAT fDistance = -m_fTargetDistance;

  CPlacement3D plViewer;
  plViewer.pl_PositionVector = m_vTarget;
  plViewer.pl_OrientationAngle = m_angViewerOrientation;
  // moving offsets need small amounts
  FLOAT dx = 0.001f * pntDelta.x * fDistance;
	FLOAT dy = 0.001f * pntDelta.y * fDistance;
	FLOAT dz =  0.01f * pntDelta.y * fDistance;
  // angles need lot for rotation
  ANGLE dAngleX = AngleDeg( -0.5f * pntDelta.x);
  ANGLE dAngleY = AngleDeg( -0.5f * pntDelta.y);
 
  if (bSpace && bCtrl) {
  // if only space
  } else if (bSpace)
  {
    if (bRMB && bLMB)
    {
      m_angViewerOrientation(1)+=dAngleX;
      m_angViewerOrientation(2)+=dAngleY;
      // if only left button
    } else if (bLMB) {
      CPlacement3D plTarget;
      plTarget.pl_PositionVector = m_vTarget;
      plTarget.pl_OrientationAngle = m_angViewerOrientation;
      
      // project the placement to the viewer's system
      plTarget.AbsoluteToRelative( plViewer);
      // translate it
      plTarget.Translate_AbsoluteSystem( FLOAT3D( dx, -dy, 0.0f));
      // project the placement back from viewer's system
      plTarget.RelativeToAbsolute( plViewer);
      m_vTarget = plTarget.pl_PositionVector;
      // if only right button
    } else if (bRMB) {
      // move target away
      m_fTargetDistance += dz;

      // now apply left/right movement
      CPlacement3D plTarget;
      plTarget.pl_PositionVector = m_vTarget;
      plTarget.pl_OrientationAngle = m_angViewerOrientation;
      
      // project the placement to the viewer's system
      plTarget.AbsoluteToRelative( plViewer);
      // translate it
      plTarget.Translate_AbsoluteSystem( FLOAT3D( dx, 0.0f, 0.0f));
      // project the placement back from viewer's system
      plTarget.RelativeToAbsolute( plViewer);
      m_vTarget = plTarget.pl_PositionVector;
      // if no buttons are held
    } else {
      
    }
  }
  // if only ctrl
  else if (bCtrl && (bRMB || bLMB))
  {
    CSeriousSkaStudioDoc* pDoc = GetDocument();
    CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
    HTREEITEM hSelectedItem = m_TreeCtrl.GetSelectedItem();
    if(hSelectedItem==NULL) {
      return;
    }
    NodeInfo &niSelected = theApp.m_dlgBarTreeView.GetNodeInfo(hSelectedItem);
    CModelInstance *pmi = NULL;
    // is selected item model instance
    if(niSelected.ni_iType == NT_MODELINSTANCE){
      // get pointer to model instance
      pmi = (CModelInstance*)niSelected.ni_pPtr;

      // set default bone placement
      QVect qvBonePlacement;
      memset(&qvBonePlacement,0,sizeof(QVect));
      qvBonePlacement.qRot.q_w = 1;
      // find parent bone of selected model instance
      RenBone rb;
      if(RM_GetRenBoneAbs(*pDoc->m_ModelInstance,pmi->mi_iParentBoneID,rb)) {
        // get bone qvect
        Matrix12ToQVect(qvBonePlacement,rb.rb_mBonePlacement);
      }
      CPlacement3D plBone,plOffset;
      FLOATmatrix3D matBone,matOffset;

      // set bone placement
      qvBonePlacement.qRot.ToMatrix(matBone);
      plBone.pl_PositionVector = qvBonePlacement.vPos; 
      DecomposeRotationMatrix(plBone.pl_OrientationAngle,matBone);
    
      // set offset
      pmi->mi_qvOffset.qRot.ToMatrix(matOffset);
      plOffset.pl_PositionVector = pmi->mi_qvOffset.vPos;
      DecomposeRotationMatrix(plOffset.pl_OrientationAngle,matOffset);

      plOffset.RelativeToRelative(plBone,plViewer);

      if(bLMB && bRMB) {
        plOffset.Rotate_TrackBall( ANGLE3D( -dAngleX/2, -dAngleY/2, 0));
      } else if(bLMB) {
        plOffset.pl_PositionVector +=  FLOAT3D( -dx, dy, 0.0f);
      } else if(bRMB) {
        plOffset.pl_PositionVector +=  FLOAT3D( 0.0f, 0.0f, -dy);
      }
      plOffset.RelativeToRelative(plViewer,plBone);

      pmi->mi_qvOffset.vPos=plOffset.pl_PositionVector;
      pmi->mi_qvOffset.qRot.FromEuler(plOffset.pl_OrientationAngle);

      _bSelectedItemChanged = TRUE;
      pDoc->MarkAsChanged();
    // is selected item colision box
    } else if (niSelected.ni_iType == NT_COLISIONBOX) {
      // get pointer to parent model instance
      pmi = (CModelInstance*)niSelected.pmi;

      ColisionBox &cb = pmi->GetColisionBox(pmi->mi_iCurentBBox);

      CPlacement3D plCBMin;
      CPlacement3D plCBMax;
      FLOATmatrix3D matOffset;

      // set min and max of colision box
      plCBMin.pl_PositionVector = cb.Min();
      plCBMin.pl_OrientationAngle = ANGLE3D(0,0,0);
      plCBMin.AbsoluteToRelative(plViewer);

      plCBMax.pl_PositionVector = cb.Max();
      plCBMax.pl_OrientationAngle = ANGLE3D(0,0,0);
      plCBMax.AbsoluteToRelative(plViewer);
      // if shift is pressed
      if(bShift) {
        if(bLMB) {
          plCBMin.pl_PositionVector +=  FLOAT3D( -dx, dy, 0.0f);
        } else if(bRMB) {
          plCBMax.pl_PositionVector +=  FLOAT3D( -dx, dy, 0.0f);
        }
      // if not
      } else {
        if(bLMB) {
          plCBMin.pl_PositionVector +=  FLOAT3D( -dx, dy, 0.0f);
          plCBMax.pl_PositionVector +=  FLOAT3D( -dx, dy, 0.0f);
        } else if(bRMB) {
          plCBMin.pl_PositionVector +=  FLOAT3D( 0.0f, 0.0f, -dy);
        }
      }

      plCBMin.RelativeToAbsolute(plViewer);
      plCBMax.RelativeToAbsolute(plViewer);
      cb.SetMin(plCBMin.pl_PositionVector);
      cb.SetMax(plCBMax.pl_PositionVector);

      _bSelectedItemChanged = TRUE;
      pDoc->MarkAsChanged();
    }
  // if only shift is presed
  } else if(bShift) {
    if(bLMB) {
      // project the placement to the viewer's system
      m_plLightPlacement.AbsoluteToRelative( plViewer);
      // rotate it
      m_plLightPlacement.Rotate_TrackBall( ANGLE3D( -dAngleX/2, -dAngleY/2, 0));
      // project the placement back from viewer's system
      m_plLightPlacement.RelativeToAbsolute( plViewer);
    } else if(bRMB) {
      FLOAT fNewDistance = m_fLightDistance - dz;
      if( fNewDistance > 0.2f) {
        m_fLightDistance = fNewDistance;
      }
    }
  }


  // SetActiveWindow();
  SetFocus();
  /*
  GetParentFrame()->SetActiveView( this);
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->MDIActivate(GetParentFrame());
  */
/*
  HWND hwndUnderMouse = ::WindowFromPoint( point);
  HWND hwndInfo = NULL;
  if( pMainFrame->m_pInfoFrame != NULL)
    hwndInfo = pMainFrame->m_pInfoFrame->m_hWnd;

  if( (m_hWnd != ::GetActiveWindow()) && ( hwndInfo != hwndUnderMouse) )
  {
    SetActiveWindow();
    SetFocus();
    GetParentFrame()->SetActiveView( this);
    pMainFrame->MDIActivate(GetParentFrame());
  }
*/  
CView::OnMouseMove(nFlags, point);
}


void CSeriousSkaStudioView::FastZoomIn()
{
  if( m_fTargetDistance > 1.0f) {
    m_fTargetDistance /= 2.0f;
  }
}

void CSeriousSkaStudioView::FastZoomOut()
{
  if( m_fTargetDistance < 65535.0f) {
    m_fTargetDistance *= 2.0f;
  }
}

void CSeriousSkaStudioView::OnLButtonDown(UINT nFlags, CPoint point)
{
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bShift = nFlags & MK_SHIFT;
  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace) {
    FastZoomIn();
    return;
  }

  if(bCtrl||bSpace||bShift) {}
  else {
    INDEX iHitBone = TestRayCastHit(point);
    if(iHitBone!=(-1) && iHitBone!=theApp.iSelectedBoneID) {

    }
    theApp.iSelectedBoneID = iHitBone;
  }

  m_pntLastMouse = point;
  CView::OnLButtonDown(nFlags, point);
}

void CSeriousSkaStudioView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bCtrl = nFlags & MK_CONTROL;
  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace) {
    FastZoomIn();
  }
	CView::OnLButtonDblClk(nFlags, point);
}

void CSeriousSkaStudioView::OnRButtonDown(UINT nFlags, CPoint point) {
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bCtrl = nFlags & MK_CONTROL;
  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace) {
    FastZoomOut();
    return;
  }
  CView::OnRButtonDown(nFlags, point);
}
void CSeriousSkaStudioView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bCtrl = nFlags & MK_CONTROL;
  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace) {
    FastZoomOut();
  }
	CView::OnRButtonDblClk(nFlags, point);
}


void CSeriousSkaStudioView::OnLButtonUp(UINT nFlags, CPoint point)
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelectedItem = m_TreeCtrl.GetSelectedItem();
  if(hSelectedItem!=NULL && _bSelectedItemChanged) {
    theApp.m_dlgBarTreeView.SelItemChanged(hSelectedItem);
    _bSelectedItemChanged = FALSE;
  }
  CView::OnLButtonUp(nFlags, point);
}
void CSeriousSkaStudioView::OnRButtonUp(UINT nFlags, CPoint point)
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelectedItem = m_TreeCtrl.GetSelectedItem();
  if(hSelectedItem!=NULL && _bSelectedItemChanged) {
    theApp.m_dlgBarTreeView.SelItemChanged(hSelectedItem);
    _bSelectedItemChanged = FALSE;
  }
  CView::OnRButtonUp(nFlags, point);
}

void CSeriousSkaStudioView::OnResetView() 
{
  m_angViewerOrientation = ANGLE3D(180.0f, -20.0f, 0.0f);
  m_vTarget = FLOAT3D(0.0f, 1.0f, 0.0f);
  m_fTargetDistance = 5.0f;
}

// add CMesh to selected model instance
BOOL CSeriousSkaStudioView::OnAddMeshlist() 
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  CTFileName fnSim;

  if(pmiSelected == NULL)
  {
    theApp.ErrorMessage("Model instance is not selected!");
    return FALSE;
  }

  CTFileName fnSelected = pmiSelected->mi_fnSourceFile;
  fnSelected = fnSelected.FileName();
  // get file name  
  fnSim = _EngineGUI.FileRequester( "Type name for new Mesh list or select existing",
    "ASCII model files (*.aml)\0*.aml\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\",fnSelected);
  if (fnSim=="") return FALSE;
  // check if file allready exist
  if(FileExists(fnSim))
  {
    CTString strText = CTString(0,"File '%s' exist\n\nPress OK to load it, Cancel to overwrite it",fnSim);
    int iRet = AfxMessageBox(CString(strText),MB_OKCANCEL);
    if(iRet == IDOK)
    {
      // convert and load animset
      if(theApp.ConvertMesh(fnSim))
      {
        fnSim = fnSim.NoExt() + ".bm";
        try
        {
          pmiSelected->AddMesh_t(fnSim);
          theApp.UpdateRootModelInstance();
          pDoc->MarkAsChanged();
          return TRUE;
        }
        catch(char *strError)
        {
          theApp.ErrorMessage("%s",strError);
          return FALSE;
        }
      }
      return FALSE;
    }
    // else owerwrite it bellow
  }
  // if file does not exist create new one
  CTFileStream ostrFile;
  try
  {
    // create new file
    ostrFile.Create_t(fnSim,CTStream::CM_TEXT);
    // write empty header in file
    ostrFile.FPrintF_t("MESHLODLIST\n{\n}\n");
    // close file
    ostrFile.Close();
    // convert it to binary (just to exist)
    if(theApp.ConvertMesh(fnSim))
    {
      fnSim = fnSim.NoExt() + ".bm";
      // add it to selected model instance
      try
      {
        pmiSelected->AddMesh_t(fnSim);
        theApp.UpdateRootModelInstance();
      }
      catch(char *strError)
      {
        theApp.ErrorMessage("%s",strError);
        return FALSE;
      }
    }
  }
  catch(char *strError)
  {
    theApp.ErrorMessage("%s",strError);
    return FALSE;
  }
  pDoc->MarkAsChanged();
  return TRUE;
}

// add CSkeleton to selected model instance
void CSeriousSkaStudioView::OnAddSkeletonlist() 
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  CTFileName fnSim;

  if(pmiSelected == NULL)
  {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }

  CSkeleton *skl = pmiSelected->mi_psklSkeleton;
  if(skl != NULL)
  {
    theApp.ErrorMessage("Skeleton for this object exists. Only one skeleton per model is allowed!");
    return;
  }

  CTFileName fnSelected = pmiSelected->mi_fnSourceFile;
  fnSelected = fnSelected.FileName();
  // get file name  
  fnSim = _EngineGUI.FileRequester( "Type name for new Skeleton list or select existing",
    "ASCII model files (*.asl)\0*.asl\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\", fnSelected);
  if (fnSim=="") return;
  // check if file allready exist
  if(FileExists(fnSim))
  {
    CTString strText = CTString(0,"File '%s' exist\n\nPress OK to load it, Cancel to overwrite it",fnSim);
    int iRet = AfxMessageBox(CString(strText),MB_OKCANCEL);
    if(iRet == IDOK)
    {
      // convert and load animset
      if(theApp.ConvertSkeleton(fnSim))
      {
        fnSim = fnSim.NoExt() + ".bs";
        try
        {
          pmiSelected->AddSkeleton_t(fnSim);
          theApp.UpdateRootModelInstance();
          pDoc->MarkAsChanged();
        }
        catch(char *strError)
        {
          theApp.ErrorMessage("%s",strError);
          return;
        }
      }
      return;
    }
    // else owerwrite it bellow
  }
  // if file does not exist create new one
  CTFileStream ostrFile;
  try
  {
    // create new file
    ostrFile.Create_t(fnSim,CTStream::CM_TEXT);
    // write empty header in file
    ostrFile.FPrintF_t("SKELETONLODLIST\n{\n}\n");
    // close file
    ostrFile.Close();
    // convert it to binary (just to exist)
    if(theApp.ConvertSkeleton(fnSim))
    {
      fnSim = fnSim.NoExt() + ".bs";
      // add it to selected model instance
      pmiSelected->AddSkeleton_t(fnSim);
      // save smc
      theApp.UpdateRootModelInstance();
    }
  }
  catch(char *strError)
  {
    theApp.ErrorMessage("%s",strError);
  }
  pDoc->MarkAsChanged();
  return;
}

// add anim set to selected model instance
void CSeriousSkaStudioView::OnAddAnimset() 
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  CTFileName fnSim;

  if(pmiSelected == NULL)
  {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }

  CTFileName fnSelected = pmiSelected->mi_fnSourceFile;
  fnSelected = fnSelected.FileName();
  // get file name  
  fnSim = _EngineGUI.FileRequester( "Type name for new Animset or select existing",
    "ASCII model files (*.aal)\0*.aal\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\", fnSelected);
  if (fnSim=="") return;
  // check if file allready exist
  if(FileExists(fnSim))
  {
    CTString strText = CTString(0,"File '%s' exist\n\nPress OK to load it, Cancel to overwrite it",fnSim);
    int iRet = AfxMessageBox(CString(strText),MB_OKCANCEL);
    if(iRet == IDOK)
    {
      // convert and load animset
      if(theApp.ConvertAnimSet(fnSim))
      {
        fnSim = fnSim.NoExt() + ".ba";
        try
        {
          pmiSelected->AddAnimSet_t(fnSim);
          theApp.UpdateRootModelInstance();
          pDoc->MarkAsChanged();
        }
        catch(char *strError)
        {
          theApp.ErrorMessage("%s",strError);
          return;
        }
      }
      return;
    }
    // else owerwrite it bellow
  }
  // if file does not exist create new one
  CTFileStream ostrFile;
  try
  {
    // create new file
    ostrFile.Create_t(fnSim,CTStream::CM_TEXT);
    // write empty header in file
    ostrFile.FPrintF_t("ANIMSETLIST\n{\n}\n");
    // close file
    ostrFile.Close();
    // convert it to binary (just to exist)
    if(theApp.ConvertAnimSet(fnSim))
    {
      fnSim = fnSim.NoExt() + ".ba";
      // add it to selected model instance
      pmiSelected->AddAnimSet_t(fnSim);
    }
  }
  catch(char *strError)
  {
    theApp.ErrorMessage("%s",strError);
  }

  theApp.UpdateRootModelInstance();
  pDoc->MarkAsChanged();
  return;
}

// add new child model instance to selected model instance
void CSeriousSkaStudioView::OnAddChildModelInstance() 
{
  if(pmiSelected == NULL)
  {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }
  CModelInstance::EnableSrcRememberFN(TRUE);
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  if(pDoc->m_ModelInstance == NULL)
  {
    theApp.ErrorMessage("There is no root model instance");
    return;
  }
  
  CSkeleton *psklParent = pmiSelected->mi_psklSkeleton;
  SkeletonLOD *psklodParent = NULL;
  INDEX iParentBoneID = 0;
  // if parent has skeleton
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
 
  CTFileName fnSim;
  fnSim = _EngineGUI.FileRequester( "Open ASCII intermediate files",
    "ASCII model files (*.smc)\0*.smc\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\", "");
  if (fnSim=="") return;

  CTFileName fnFull;
  fnFull = _fnmApplicationPath + fnSim;
  CModelInstance *pcmi=NULL;
  try
  {
    pcmi = ParseSmcFile_t(fnFull);
  }
  catch(char *strError)
  {
    // error in parsing occured
    theApp.ErrorMessage("%s",strError);
    if(pcmi != NULL) pcmi->Clear();
    return;
  }

  CTString strText = CTString(0,"Add this model as reference or use own copy");
  int iRet = AfxMessageBox(CString(strText),MB_YESNO);
  if(iRet == IDNO)
  {
    // assign same source file name as parent model instance so it will be saved in same file
    pcmi->mi_fnSourceFile = pmiSelected->mi_fnSourceFile;
  }
  pcmi->mi_iParentBoneID = iParentBoneID;

  // reset offset
  memset(&pcmi->mi_qvOffset,0,sizeof(QVect));
  pcmi->mi_qvOffset.qRot.q_w = 1;
  // if there is selected model instance ,add as child
  pmiSelected->mi_cmiChildren.Add(pcmi);
  theApp.UpdateRootModelInstance();
  pDoc->MarkAsChanged();
}
// add mesh lod to existing CMesh in selected model instance
void CSeriousSkaStudioView::OnAddMeshlod() 
{
	CSeriousSkaStudioDoc *pDoc = GetDocument();

  if(pmiSelected == NULL)
  {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }

  // if mesh list does not exist
  INDEX ctmshi = pmiSelected->mi_aMeshInst.Count();
  if(ctmshi < 1)
  {
    theApp.ErrorMessage("Model instance must have mesh lod list to add mesh lod to it");
    return;
  }
  // get pointer to selected item
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  INDEX iSelected = m_TreeCtrl.GetItemData(hSelected);
  NodeInfo &niSelected = theApp.aNodeInfo[iSelected];
  MeshInstance *pmshi;

  // get selected mesh instance
  if(niSelected.ni_iType == NT_MESHLODLIST)
  {
    // selected item is mesh instance so use its pointer as current mesh instance
    pmshi=(MeshInstance*)niSelected.ni_pPtr;
  }
  else if(niSelected.ni_iType == NT_MESHLOD)
  {
    // selected item is mesh lod, so get his parent and use his pointer as current mesh instance
    HTREEITEM hParent = m_TreeCtrl.GetParentItem(hSelected);
    INDEX iParent = m_TreeCtrl.GetItemData(hParent);
    NodeInfo &niParent = theApp.aNodeInfo[iParent];
    pmshi=(MeshInstance*)niParent.ni_pPtr;
  }
  else
  {
    // other item is selected, curent mesh instance is first mesh instance in model instance
    pmshi=&pmiSelected->mi_aMeshInst[0];
  }
  
  CMesh *pMesh = pmshi->mi_pMesh;

  CDynamicArray<CTFileName> afnMesh;
  _EngineGUI.FileRequester( "Open ASCII intermediate files",
    FILTER_MESH,
    "Open directory", "Models\\", "", &afnMesh);

  // return if no files selected
  if(afnMesh.Count()<=0) return;
  FLOAT fMaxDistance = 10;
  INDEX ctmlod = pMesh->msh_aMeshLODs.Count();
  // for each lod in skeleton
  INDEX imlod=0;
  for(;imlod<ctmlod;imlod++)
  {
    // find current max distance in skeleton
    if((fMaxDistance-5)<pMesh->msh_aMeshLODs[imlod].mlod_fMaxDistance) 
      fMaxDistance = pMesh->msh_aMeshLODs[imlod].mlod_fMaxDistance;
  }
  // create new skeleton list that holds only new skeleton lods
  CTString strMeshList;
  strMeshList = "MESHLODLIST\n{\n";

  FOREACHINDYNAMICARRAY( afnMesh, CTFileName, itMesh)
  {
    fMaxDistance+=5;
    CTString strLine;
    strLine.PrintF("  MAX_DISTANCE %g;\n  #INCLUDE \"%s\"\n",fMaxDistance,(const char*)itMesh.Current());
    // add this file to mesh list file
    strMeshList += strLine;
  }
  strMeshList += "}\n";

  CTFileName fnMeshList = (CTString)"Temp/mesh.aml";
  CTFileName fnMeshBin = (CTString)"Temp/mesh.bm";

  CMesh mshTemp;
  try
  {
    // save list file
    strMeshList.Save_t(fnMeshList);
    // convert animset list file
    if(!theApp.ConvertMesh(fnMeshList))
    {
      RemoveFile(fnMeshList);
      return;
    }
    // load it
    mshTemp.Load_t(fnMeshBin);
  }
  catch(char *strErr)
  {
    theApp.ErrorMessage(strErr);
    mshTemp.Clear();
    RemoveFile(fnMeshBin);
    return;
  }
  RemoveFile(fnMeshList);
  RemoveFile(fnMeshBin);

  // count new skleton lods in temp skeleton list
  INDEX ctNewMlods = mshTemp.msh_aMeshLODs.Count();
  // for each new skeleton in temp skeleton lod list
  for(imlod=0;imlod<ctNewMlods;imlod++)
  {
    // add skeleton lod to selected skeleton
    pMesh->AddMeshLod(mshTemp.msh_aMeshLODs[imlod]);
  }
  // clear temp mesh
  mshTemp.Clear();
  // update model instance
  theApp.UpdateRootModelInstance();
  pDoc->MarkAsChanged();
}
// add skeleton lod to existing CSkeleton in selected model instance
void CSeriousSkaStudioView::OnAddSkeletonlod() 
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();

  if(pmiSelected == NULL)
  {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }

  CSkeleton *pSkeleton = pmiSelected->mi_psklSkeleton;
  if(pSkeleton == NULL)
  {
    theApp.ErrorMessage("Model instance must have Skeleton list to add skeleton lod to it");
    return;
  }

  CDynamicArray<CTFileName> afnSkeleton;
  _EngineGUI.FileRequester( "Open ASCII intermediate files",
    FILTER_SKELETON,
    "Open directory", "Models\\", "", &afnSkeleton);

  // return if no files selected
  if(afnSkeleton.Count()<=0) return;

  FLOAT fMaxDistance=10;
  INDEX ctslod = pSkeleton->skl_aSkeletonLODs.Count();
  // for each lod in skeleton
  for(INDEX ilod=0;ilod<ctslod;ilod++)
  {
    // find current max distance in skeleton
    if((fMaxDistance-5)<pSkeleton->skl_aSkeletonLODs[ilod].slod_fMaxDistance) fMaxDistance = pSkeleton->skl_aSkeletonLODs[ilod].slod_fMaxDistance;
  }

  // create new skeleton list that holds only new skeleton lods
  CTString strSkeletonList;
  strSkeletonList = "SKELETONLODLIST\n{\n";

  FOREACHINDYNAMICARRAY( afnSkeleton, CTFileName, itSkeleton)
  {
    fMaxDistance+=5;
    CTString strLine;
    strLine.PrintF("  MAX_DISTANCE %g;\n  #INCLUDE \"%s\"\n",fMaxDistance,(const char*)itSkeleton.Current());
    // add this file to skeleton list file
    strSkeletonList += strLine;
  }
  strSkeletonList += "}\n";

  CTFileName fnSklList = (CTString)"Temp/skeleton.asl";
  CTFileName fnSklBin = (CTString)"Temp/skeleton.bs";
  CSkeleton slTemp;

  try
  {
    // save list file
    strSkeletonList.Save_t(fnSklList);
    // convert animset list file
    if(!theApp.ConvertSkeleton(fnSklList))
    {
      slTemp.Clear();
      RemoveFile(fnSklList);
      return;
    }
    // load it
    slTemp.Load_t(fnSklBin);
  }
  catch(char *strErr)
  {
    theApp.ErrorMessage(strErr);
    slTemp.Clear();
    RemoveFile(fnSklBin);
    return;
  }
  RemoveFile(fnSklList);
  RemoveFile(fnSklBin);

  // count new skleton lods in temp skeleton list
  INDEX ctNewSlods = slTemp.skl_aSkeletonLODs.Count();
  // for each new skeleton in temp skeleton lod list
  for(INDEX isl=0;isl<ctNewSlods;isl++)
  {
    // add skeleton lod to selected skeleton
    pSkeleton->AddSkletonLod(slTemp.skl_aSkeletonLODs[isl]);
  }
  // clear temp skeleton
  slTemp.Clear();
  // update model instance
  theApp.UpdateRootModelInstance();
  pDoc->MarkAsChanged();
}
// add animation to existing Anim set in selected model instance
void CSeriousSkaStudioView::OnAddAnimation() 
{
  // check for selected model instance
  if(pmiSelected == NULL) {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }
  // get animset count
  INDEX ctas = pmiSelected->mi_aAnimSet.Count();
  if(ctas < 1) {
    theApp.ErrorMessage("Model instance must have Anim set to add animations to it");
    return;
  }
  // get document
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  CDynamicArray<CTFileName> afnAnimation;
  _EngineGUI.FileRequester( "Open ASCII intermediate files",
    FILTER_ANIMATION,
    "Open directory", "Models\\", "", &afnAnimation);

  // return if no files selected
  if(afnAnimation.Count()<=0) return;

  // get pointer to seleceted animset
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  INDEX iSelected = m_TreeCtrl.GetItemData(hSelected);
  NodeInfo &niSelected = theApp.aNodeInfo[iSelected];
  CAnimSet *pas=NULL;
  if(niSelected.ni_iType == NT_ANIMSET) {
    // selected item is animset
    pas = (CAnimSet*)niSelected.ni_pPtr;
  } else if(niSelected.ni_iType == NT_ANIMATION) {
    // selected item is animation, get its animset
    HTREEITEM hParent = m_TreeCtrl.GetParentItem(hSelected);
    INDEX iParent = m_TreeCtrl.GetItemData(hParent);
    NodeInfo &niParent = theApp.aNodeInfo[iParent];
    pas = (CAnimSet*)niParent.ni_pPtr;
  } else {
    // something else is selected, use first animset in model instance
    pas = &pmiSelected->mi_aAnimSet[0];
  }
  
  // create new animset that holds only new animations
  CTString strAnimSet;
  strAnimSet = "ANIMSETLIST\n{\n";
  // for each selected file
  FOREACHINDYNAMICARRAY( afnAnimation, CTFileName, itAnimation)
  {
    // add this file to animset list file
    CTString strLine;
    strLine.PrintF("  TRESHOLD 0;\n  COMPRESION FALSE;\n  #INCLUDE \"%s\"\n",(const char*)itAnimation.Current());
    strAnimSet += strLine;
  }
  strAnimSet += "}\n";

  CTFileName fnAnimSetList = (CTString)"Temp/animset.aal";
  CTFileName fnAnimSetBin = (CTString)"Temp/animset.ba";
  CAnimSet asTemp;

  try
  {
    // save list file
    strAnimSet.Save_t(fnAnimSetList);
    // convert animset list file
    if(!theApp.ConvertAnimSet(fnAnimSetList))
    {
      RemoveFile(fnAnimSetList);
      return;
    }
    // load it
    asTemp.Load_t(fnAnimSetBin);
  }
  catch(char *strErr)
  {
    theApp.ErrorMessage(strErr);
    asTemp.Clear();
    RemoveFile(fnAnimSetBin);
    return;
  }
  RemoveFile(fnAnimSetList);
  RemoveFile(fnAnimSetBin);

  // count new animations in temp animset
  INDEX ctan = asTemp.as_Anims.Count();
  // for each new animation in temp animset
  for(INDEX ian=0;ian<ctan;ian++)
  {
    Animation &anTemp = asTemp.as_Anims[ian];
    // add animation to selected animset
    pas->AddAnimation(&anTemp);
  }
  // clear temp animset
  asTemp.Clear();
  // update model instance
  theApp.UpdateRootModelInstance();
  pDoc->MarkAsChanged();
}

// add texture to existing mesh instance
void CSeriousSkaStudioView::AddTexture(CTFileName &fnFull)
{
  if(pmiSelected == NULL) {
    theApp.ErrorMessage("Model instance is not selected!");
    return;
  }

  INDEX ctmshi = pmiSelected->mi_aMeshInst.Count();
  if(ctmshi < 1) {
    theApp.ErrorMessage("Model instance must have mesh lod list to add texture to it");
    return;
  }

  // get selected item in tree view
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  INDEX iSelected = m_TreeCtrl.GetItemData(hSelected);
  NodeInfo &niSelected = theApp.aNodeInfo[iSelected];
  MeshInstance *pmshi;
  // get selected mesh instance
  if(niSelected.ni_iType == NT_MESHLODLIST) {
    // selected item is mesh instance so use its pointer as current mesh instance
    pmshi=(MeshInstance*)niSelected.ni_pPtr;
  } else if(niSelected.ni_iType == NT_MESHLOD) {
    // selected item is mesh lod, so get his parent and use his pointer as current mesh instance
    HTREEITEM hParent = m_TreeCtrl.GetParentItem(hSelected);
    INDEX iParent = m_TreeCtrl.GetItemData(hParent);
    NodeInfo &niParent = theApp.aNodeInfo[iParent];
    pmshi=(MeshInstance*)niParent.ni_pPtr;
  } else {
    // other item is selected, curent mesh instance is first mesh instance in model instance
    pmshi=&pmiSelected->mi_aMeshInst[0];
  }
  /*
  // check if file exists
  if(!FileExists(fnFull)) {
    theApp.ErrorMessage("Texture file '%s' does not exists",fnFull);
    continue;
  }
  */
  // try adding texture to model instance
  try {
    pmiSelected->AddTexture_t(fnFull,fnFull.FileName(),pmshi);
  } catch(char *strError) {
    theApp.ErrorMessage(strError);
    //continue;
  }
}

void CSeriousSkaStudioView::BrowseTexture(CTString strTextureDir)
{
  CString strRegKeyName = "";
  if(strTextureDir=="Models\\") {
    strTextureDir = "Open directory";
  }
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  CDynamicArray<CTFileName> afnTexture;
  _EngineGUI.FileRequester( "Open texture files",
    FILTER_TEXTURE,
    (char*)(const char*)strTextureDir, strTextureDir, "", &afnTexture);

  // return if no files selected
  if(afnTexture.Count()<=0) return;
  // for each selected filename
  FOREACHINDYNAMICARRAY( afnTexture, CTFileName, itTexture) {
    CTFileName fnFull;
    fnFull = itTexture.Current();
    AddTexture(fnFull);
  }
  pDoc->MarkAsChanged();
  theApp.UpdateRootModelInstance();
}

// add texture to existing mesh instance
void CSeriousSkaStudioView::OnAddTexture() 
{
  BrowseTexture("Models\\");
}
void CSeriousSkaStudioView::OnAddTextureBump() 
{
  BrowseTexture("Models\\BumpTextures\\");
}

void CSeriousSkaStudioView::OnAddTextureReflection() 
{
  BrowseTexture("Models\\ReflectionTextures\\");
}

void CSeriousSkaStudioView::OnAddTextureSpecular() 
{
  BrowseTexture("Models\\SpecularTextures\\");
}

void CSeriousSkaStudioView::OnCreateAddTexture() 
{
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTFileName fn = (CTString)pMainFrame->CreateTexture();
  if(fn!="") {
    AddTexture(fn);
    pDoc->MarkAsChanged();
    theApp.UpdateRootModelInstance();
  }
}

void CSeriousSkaStudioView::OnAddColisionbox() 
{
  if(pmiSelected == NULL) return; 
	CSeriousSkaStudioDoc *pDoc = GetDocument();

  INDEX ctcb = pmiSelected->mi_cbAABox.Count();
  FLOAT3D vMin = FLOAT3D(-.5f,0,-.5f);
  FLOAT3D vMax = FLOAT3D(.5f,1,.5f);
  CTString strName = CTString(0,"Default %d",ctcb);
  pmiSelected->AddColisionBox(strName,vMin,vMax);
  theApp.UpdateRootModelInstance();
  pDoc->MarkAsChanged();
}

// delete selected item in tree view control
void CSeriousSkaStudioView::OnDeleteselected() 
{
  // get tree ctrl
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  // get current document
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  // get selected item and its parent
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  if(hSelected == NULL) return;
  INDEX iSelIndex = m_TreeCtrl.GetItemData(hSelected);
  INDEX iParentIndex = -1;
  NodeInfo *pniParent = NULL;
  NodeInfo *pni = &theApp.aNodeInfo[iSelIndex];
  HTREEITEM hParent = m_TreeCtrl.GetParentItem(hSelected);
  if(hParent != NULL)
  {
     iParentIndex = m_TreeCtrl.GetItemData(hParent);
     pniParent = &theApp.aNodeInfo[iParentIndex];
  }
  // switch type of selected item
  switch(pni->ni_iType)
  {
    case NT_MODELINSTANCE:
    {
      // find parent of selected model instance
      CModelInstance *pmiParent = pmiSelected->GetParent(pDoc->m_ModelInstance);
      if(pmiParent != NULL) {
        // remove selected model instance form parent 
        pmiParent->RemoveChild(pmiSelected);
        // update root model instance
        theApp.UpdateRootModelInstance();
      } else {
        // root item is selected
        theApp.ErrorMessage("Can't delete root model instance");
        return;
      }
    }
    break;
    case NT_MESHLODLIST:
    {
      // get pointer to mesh instance
      MeshInstance *pmshi = (MeshInstance*)pni->ni_pPtr;
      // get pointer to mesh
      CMesh *pMesh = pmshi->mi_pMesh;
      // release mesh
      _pMeshStock->Release(pMesh);
      // count textures
      INDEX ctti=pmshi->mi_tiTextures.Count();
      // for each texture in selected mesh instance
      for(INDEX iti=0;iti<ctti;iti++) {
        // release texture from stock
        TextureInstance &ti = pmshi->mi_tiTextures[iti];
        ti.ti_toTexture.SetData(NULL);
        //CTextureData &td = *ti.ti_tdTexture;
        //_pTextureStock->Release(&td);
      }
      // count mesh instances
      INDEX ctmshi=pmiSelected->mi_aMeshInst.Count();
      INDEX itmpmshi=0;
      CStaticArray<struct MeshInstance> atmpMeshInst;
      // create array for mesh instances 
      atmpMeshInst.New(ctmshi-1);
      // for each mesh instance in selected model instance 
      for(INDEX imshi=0;imshi<ctmshi;imshi++) {
        MeshInstance *ptmpmshi = &pmiSelected->mi_aMeshInst[imshi];
        // if mesh instance is diferent then selected mesh instance
        if(ptmpmshi != pmshi) {
          // copy it to temp array
          atmpMeshInst[itmpmshi] = *ptmpmshi;
          itmpmshi++;
        }
      }
      // clear mesh instance
      pmiSelected->mi_aMeshInst.Clear();
      pmiSelected->mi_aMeshInst.CopyArray(atmpMeshInst);
      // update root model instance
      theApp.UpdateRootModelInstance();
    }
    break;
    case NT_MESHLOD:
    {
      ASSERT(pniParent!=NULL);
      MeshLOD *pmlodSelected = (MeshLOD*)pni->ni_pPtr;
      MeshInstance *pmshi = (MeshInstance*)pniParent->ni_pPtr;
      CMesh *pmesh = pmshi->mi_pMesh;
      pmesh->RemoveMeshLod(pmlodSelected);
      theApp.UpdateRootModelInstance();
    }
    break;
    case NT_TEXINSTANCE:
    {
      // get pointers to texture and mesh instances
      TextureInstance *pti = (TextureInstance*)pni->ni_pPtr;
      MeshInstance *pmshi = (MeshInstance*)pniParent->ni_pPtr;
      ASSERT(pti!=NULL);
      ASSERT(pmshi!=NULL);

      pmiSelected->RemoveTexture(pti,pmshi);
      // update root model instance
      theApp.UpdateRootModelInstance();
    }
    break;
    case NT_SKELETONLODLIST:
    {
      CSkeleton *skl = pmiSelected->mi_psklSkeleton;
      _pSkeletonStock->Release(skl);
      pmiSelected->mi_psklSkeleton = NULL;
      // update root model instance
      theApp.UpdateRootModelInstance();//!!!
    }
    break;
    case NT_SKELETONLOD:
    {
      // get pointers to skeleton and skeleton lod
      SkeletonLOD *pslodSelected = (SkeletonLOD*)pni->ni_pPtr;
      CSkeleton *pskl = (CSkeleton*)pniParent->ni_pPtr;
      pskl->RemoveSkeletonLod(pslodSelected);
      // update root model instance
      theApp.UpdateRootModelInstance();
    }
    break;
    case NT_BONE:
    {
     return;
    }
    break;
    case NT_ANIM_BONEENV:
    {
      return;
    }
    break;
    case NT_MESHSURFACE:
    {
      return;
    }
    break;
    case NT_ANIMSET:
    {
      CAnimSet *pas = (CAnimSet*)pni->ni_pPtr;
      pmiSelected->mi_aAnimSet.Remove(pas);
      _pAnimSetStock->Release(pas);
      // update root model instance
      theApp.UpdateRootModelInstance();
    }
    break;
    case NT_ANIMATION:
    {
      // take pointer to anim set
      Animation *panSelected = (Animation*)pni->ni_pPtr;
      CAnimSet *pas = (CAnimSet*)pniParent->ni_pPtr;
      pas->RemoveAnimation(panSelected);
      // update model instance
      theApp.UpdateRootModelInstance();
    }
    break;
    case NT_COLISIONBOX:
    {
      ColisionBox *pcbSelected = (ColisionBox*)pni->ni_pPtr;
      // find colision box
      INDEX iIndex = -1;
      // count colision boxes
      INDEX ctcb = pmiSelected->mi_cbAABox.Count();
      // for each colision box in model instance
      for(INDEX icb=0;icb<ctcb;icb++) {
        ColisionBox *pcb2 = &pmiSelected->mi_cbAABox[icb];
        if(pcbSelected == pcb2) {
          // remember index of selected colision box
          iIndex = icb;
          break;
        }
      }
      // return if no index
      if(iIndex<0) return;
      // remove bounding box from mi
      pmiSelected->RemoveColisionBox(iIndex);
      // update root model instance
      theApp.UpdateRootModelInstance();
    }
    break;
  }
  pDoc->MarkAsChanged();
}

// fill tree view with new selected document
void CSeriousSkaStudioView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
  if(bActivate) {
    // refresh tree view for curent view
    CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
    CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
    HTREEITEM hRoot = m_TreeCtrl.GetRootItem();

    if(pDoc == NULL) {
      theApp.m_dlgBarTreeView.UpdateModelInstInfo(NULL);
    	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
      return;
    }
    if(hRoot == NULL) {
      theApp.UpdateRootModelInstance();
    	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
      return;
    }
    
    INDEX iRoot = m_TreeCtrl.GetItemData(hRoot);
    NodeInfo &ni = theApp.aNodeInfo[iRoot];
    // update only ih root model changed
    if(ni.ni_pPtr != pDoc->m_ModelInstance) {
      theApp.UpdateRootModelInstance();
    }
  }
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void SyncModelInstance(CModelInstance *pmi,FLOAT fTime)
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();

  AnimQueue &anq = pmi->mi_aqAnims;
  INDEX ctal = anq.aq_Lists.Count();
  // for each anim list
  for(INDEX ial=0;ial<ctal;ial++)
  {
    AnimList &al = anq.aq_Lists[ial];
    al.al_fStartTime = fTime;
    
    // for each played anim
    INDEX ctpa=al.al_PlayedAnims.Count();
    for(INDEX ipa=0;ipa<ctpa;ipa++)
    {
      PlayedAnim &pa = al.al_PlayedAnims[ipa];
      pa.pa_fStartTime = fTime;

      if(pDoc->bAnimLoop) {
        pa.pa_ulFlags |= AN_LOOPING;
      } else {
        pa.pa_ulFlags &= ~AN_LOOPING;
      }
    }
  }

  INDEX ctmi = pmi->mi_cmiChildren.Count();
  // for each child
  for(INDEX imi=0;imi<ctmi;imi++)
  {
    CModelInstance &cmi = pmi->mi_cmiChildren[imi];
    SyncModelInstance(&cmi,fTime);
  }
}

void CSeriousSkaStudioView::OnAnimSync() 
{
  SyncModelInstance(theApp.GetDocument()->m_ModelInstance,_pTimer->CurrentTick());
}

void CSeriousSkaStudioView::OnAnimLoop() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->bAnimLoop=!pDoc->bAnimLoop;
  SyncModelInstance(theApp.GetDocument()->m_ModelInstance,_pTimer->CurrentTick());
}

void CSeriousSkaStudioView::OnUpdateAnimLoop(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) pCmdUI->SetCheck(pDoc->bAnimLoop);
}

void CSeriousSkaStudioView::OnAnimPause() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->m_bViewPaused = !pDoc->m_bViewPaused;
  // if now unpaused
  if(!pDoc->m_bViewPaused) {
    // calculate time in pause
    pDoc->m_tvPauseTime += _pTimer->GetHighPrecisionTimer() - pDoc->m_tvPauseStart;
  }
  pDoc->m_tvPauseStart = _pTimer->GetHighPrecisionTimer();
}

void CSeriousSkaStudioView::OnUpdateAnimPause(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pCmdUI->SetCheck(pDoc->m_bViewPaused);
}

void CSeriousSkaStudioView::OnVkEscape() 
{
  // if error list is visible
  if(theApp.IsErrorDlgVisible()) {
    // hide it
    theApp.ShowErrorDlg(FALSE);
  // else
  } else {
    // stop all animations
    OnAnimStop();
  }
}

void CSeriousSkaStudioView::OnAnimStop() 
{
  CModelInstance *pmi = theApp.GetDocument()->m_ModelInstance;
  pmi->StopAllAnimations(0);
}

void CSeriousSkaStudioView::OnAutoMiping() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->bAutoMiping=!pDoc->bAutoMiping;
}
void CSeriousSkaStudioView::OnUpdateAutoMiping(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) pCmdUI->SetCheck(pDoc->bAutoMiping);
}

void CSeriousSkaStudioView::OnShowGround() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->bShowGround=!pDoc->bShowGround;
}
void CSeriousSkaStudioView::OnUpdateShowGround(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) pCmdUI->SetCheck(pDoc->bShowGround);
}

void CSeriousSkaStudioView::OnShowSkeleton() 
{
  if(RM_GetFlags() & RMF_SHOWSKELETON) RM_RemoveFlag(RMF_SHOWSKELETON);
  else RM_AddFlag(RMF_SHOWSKELETON);
}
void CSeriousSkaStudioView::OnUpdateShowSkeleton(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck((RM_GetFlags() & RMF_SHOWSKELETON) ? 1:0);
}

void CSeriousSkaStudioView::OnShowActiveSkeleton() 
{
  if(RM_GetFlags() & RMF_SHOWACTIVEBONES) RM_RemoveFlag(RMF_SHOWACTIVEBONES);
  else RM_AddFlag(RMF_SHOWACTIVEBONES);
}

void CSeriousSkaStudioView::OnUpdateShowActiveSkeleton(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck((RM_GetFlags() & RMF_SHOWACTIVEBONES) ? 1:0);
}

void CSeriousSkaStudioView::OnShowTexture() 
{
  if(RM_GetFlags() & RMF_SHOWTEXTURE) RM_RemoveFlag(RMF_SHOWTEXTURE);
  else RM_AddFlag(RMF_SHOWTEXTURE);
}

void CSeriousSkaStudioView::OnShowNormals() 
{
  if(RM_GetFlags() & RMF_SHOWNORMALS) RM_RemoveFlag(RMF_SHOWNORMALS);
  else RM_AddFlag(RMF_SHOWNORMALS);
}

void CSeriousSkaStudioView::OnUpdateShowNormals(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck((RM_GetFlags() & RMF_SHOWNORMALS) ? 1:0);
}

void CSeriousSkaStudioView::OnUpdateShowTexture(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck((RM_GetFlags() & RMF_SHOWTEXTURE) ? 1:0);
}

void CSeriousSkaStudioView::OnShowWireframe() 
{
  if(RM_GetFlags() & RMF_WIREFRAME) RM_RemoveFlag(RMF_WIREFRAME);
  else RM_AddFlag(RMF_WIREFRAME);
}

void CSeriousSkaStudioView::OnUpdateShowWireframe(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck((RM_GetFlags() & RMF_WIREFRAME) ? 1:0);
}


void CSeriousSkaStudioView::OnShowColision() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) {
    pDoc->bShowColisionBox = !pDoc->bShowColisionBox;
  }
}
void CSeriousSkaStudioView::OnUpdateShowColision(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) pCmdUI->SetCheck(pDoc->bShowColisionBox);
}

void CSeriousSkaStudioView::OnShowAllFramesBbox() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) {
    pDoc->bShowAllFramesBBox = !pDoc->bShowAllFramesBBox;
  }
}
void CSeriousSkaStudioView::OnUpdateShowAllFramesBbox(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) pCmdUI->SetCheck(pDoc->bShowAllFramesBBox);
}

void CSeriousSkaStudioView::OnShowAnimQueue() 
{
  theApp.bShowAnimQueue=!theApp.bShowAnimQueue;
}
void CSeriousSkaStudioView::OnUpdateShowAnimQueue(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.bShowAnimQueue);
}

void CSeriousSkaStudioView::OnFileSaveModel() 
{
  theApp.SaveRootModel();
}

void CSeriousSkaStudioView::OnFileSavemiAs() 
{
  theApp.SaveRootModelAs();
}

void CSeriousSkaStudioView::OnShowLights() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->bShowLights=!pDoc->bShowLights;
}

void CSeriousSkaStudioView::OnUpdateShowLights(CCmdUI* pCmdUI) 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc!=NULL) pCmdUI->SetCheck(pDoc->bShowLights);
}

void CSeriousSkaStudioView::OnChangeAmbientcolor() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();

  COLORREF acrCustClr[16];
  CHOOSECOLOR cc;
  acrCustClr[ 0] = CLRF_CLR(C_BLACK);
  acrCustClr[ 1] = CLRF_CLR(C_WHITE);
  acrCustClr[ 2] = CLRF_CLR(C_dGRAY);
  acrCustClr[ 3] = CLRF_CLR(C_GRAY);
  acrCustClr[ 4] = CLRF_CLR(C_lGRAY);
  acrCustClr[ 5] = CLRF_CLR(C_dRED); 
  acrCustClr[ 6] = CLRF_CLR(C_dGREEN);
  acrCustClr[ 7] = CLRF_CLR(C_dBLUE);
  acrCustClr[ 8] = CLRF_CLR(C_dCYAN);
  acrCustClr[ 9] = CLRF_CLR(C_dMAGENTA);
  acrCustClr[10] = CLRF_CLR(C_dYELLOW);
  acrCustClr[11] = CLRF_CLR(C_dORANGE);
  acrCustClr[12] = CLRF_CLR(C_dBROWN);
  acrCustClr[13] = CLRF_CLR(C_dPINK);
  acrCustClr[14] = CLRF_CLR(C_lORANGE);
  acrCustClr[15] = CLRF_CLR(C_lBROWN);

  pDoc->m_colAmbient &= 0xFFFFFF00;

  memset(&cc, 0, sizeof(CHOOSECOLOR));
  cc.lStructSize = sizeof(CHOOSECOLOR);
  cc.Flags = CC_FULLOPEN | CC_RGBINIT;
  cc.rgbResult = ByteSwap(pDoc->m_colAmbient);
  cc.hwndOwner = GetSafeHwnd();
  cc.lpCustColors = (LPDWORD) acrCustClr;
  if(ChooseColor(&cc))
  {
    COLOR colAmbient = ByteSwap(cc.rgbResult);
    colAmbient |= 0xFF;
    pDoc->m_colAmbient  = colAmbient;
  }
}

void CSeriousSkaStudioView::OnChangeLightcolor() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();

  COLORREF acrCustClr[16];
  CHOOSECOLOR cc;
  acrCustClr[ 0] = CLRF_CLR(C_BLACK);
  acrCustClr[ 1] = CLRF_CLR(C_WHITE);
  acrCustClr[ 2] = CLRF_CLR(C_dGRAY);
  acrCustClr[ 3] = CLRF_CLR(C_GRAY);
  acrCustClr[ 4] = CLRF_CLR(C_lGRAY);
  acrCustClr[ 5] = CLRF_CLR(C_dRED); 
  acrCustClr[ 6] = CLRF_CLR(C_dGREEN);
  acrCustClr[ 7] = CLRF_CLR(C_dBLUE);
  acrCustClr[ 8] = CLRF_CLR(C_dCYAN);
  acrCustClr[ 9] = CLRF_CLR(C_dMAGENTA);
  acrCustClr[10] = CLRF_CLR(C_dYELLOW);
  acrCustClr[11] = CLRF_CLR(C_dORANGE);
  acrCustClr[12] = CLRF_CLR(C_dBROWN);
  acrCustClr[13] = CLRF_CLR(C_dPINK);
  acrCustClr[14] = CLRF_CLR(C_lORANGE);
  acrCustClr[15] = CLRF_CLR(C_lBROWN);

  pDoc->m_colLight &= 0xFFFFFF00;
  memset(&cc, 0, sizeof(CHOOSECOLOR));
  cc.lStructSize = sizeof(CHOOSECOLOR);
  cc.Flags = CC_FULLOPEN | CC_RGBINIT;
  cc.rgbResult = ByteSwap(pDoc->m_colLight);
  cc.hwndOwner = GetSafeHwnd();
  cc.lpCustColors = (LPDWORD) acrCustClr;
  if(ChooseColor(&cc))
  {
    COLOR colDiffuse = ByteSwap(cc.rgbResult);
    colDiffuse |= 0xFF;
    pDoc->m_colLight = colDiffuse;
  }
}

void CSeriousSkaStudioView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
/*
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
    if(nChar==VK_LEFT) {
      if(!pDoc->m_bViewPaused) {
        OnAnimPause();
      } else {
        pDoc->m_tvPauseStart.tv_llValue -= 10000000;
      }
      return;
    }
    if(nChar==VK_RIGHT) {
      if(!pDoc->m_bViewPaused) {
        OnAnimPause();
      } else {
        pDoc->m_tvPauseStart.tv_llValue += 10000000;
      }
      return;
    }
*/
  CView::OnKeyDown(nChar, nRepCnt, nFlags);
}



void CSeriousSkaStudioView::OnFileRecreatetexture() 
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  // if selected item exists
  if(hSelected!=NULL) {
    NodeInfo &niSelected = theApp.m_dlgBarTreeView.GetNodeInfo(hSelected);
    // is selected item texture instance
    if(niSelected.ni_iType == NT_TEXINSTANCE) {
      // recreate texture
      theApp.m_dlgBarTreeView.m_dlgTexture.OnBtRecreateTexture();
    }
  }
}

void CSeriousSkaStudioView::OnUpdateFileRecreatetexture(CCmdUI* pCmdUI) 
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  // if selected item exists
  if(hSelected!=NULL) {
    NodeInfo &niSelected = theApp.m_dlgBarTreeView.GetNodeInfo(hSelected);
    // is selected item texture instance
    if(niSelected.ni_iType == NT_TEXINSTANCE) {
      // enable recreate texture menu item
      pCmdUI->Enable(TRUE);
    } else {
      // disable recreate texture menu item
      pCmdUI->Enable(FALSE);
    }
  }
}

void CSeriousSkaStudioView::OnVkDown() 
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  if(hSelected!=NULL) {
    HTREEITEM hNext = m_TreeCtrl.GetNextItem(hSelected,TVGN_NEXTVISIBLE);
    if(hNext!=NULL) {
      m_TreeCtrl.SelectItem(hNext);
    }
  }
}

void CSeriousSkaStudioView::OnVkUp() 
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  if(hSelected!=NULL) {
    HTREEITEM hNext = m_TreeCtrl.GetNextItem(hSelected,TVGN_PREVIOUSVISIBLE);
    if(hNext!=NULL) {
      m_TreeCtrl.SelectItem(hNext);
    }
  }
}

static void SelectLeftItemInTree()
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  if(hSelected!=NULL) {
    UINT uiMask = TVIF_STATE;
    UINT uiState = m_TreeCtrl.GetItemState(hSelected,uiMask);
    if(uiState&TVIS_EXPANDED) {
      m_TreeCtrl.Expand(hSelected,TVE_COLLAPSE);
    } else {
      HTREEITEM hParent = m_TreeCtrl.GetNextItem(hSelected,TVGN_PARENT);
      if(hParent!=NULL) {
        m_TreeCtrl.SelectItem(hParent);
      }
    }
  }
}
static void SelectRightItemInTree()
{
  CModelTreeCtrl &m_TreeCtrl = theApp.m_dlgBarTreeView.m_TreeCtrl;
  HTREEITEM hSelected = m_TreeCtrl.GetSelectedItem();
  if(hSelected!=NULL) {
    UINT uiMask = TVIF_STATE;
    UINT uiState = m_TreeCtrl.GetItemState(hSelected,uiMask);
    if(uiState&TVIS_EXPANDED) {
      HTREEITEM hClild = m_TreeCtrl.GetNextItem(hSelected,TVGN_CHILD);
      if(hClild!=NULL) {
        m_TreeCtrl.SelectItem(hClild);
      }
    } else {
      m_TreeCtrl.Expand(hSelected,TVE_EXPAND);
    }
  }
}

static void NextFrame()
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->m_tvPauseStart.tv_llValue += 10000000;
}
static void PrevFrame()
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  pDoc->m_tvPauseStart.tv_llValue -= 10000000;
}

void CSeriousSkaStudioView::OnVkLeft() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc->m_bViewPaused) {
    PrevFrame();
  } else {
    SelectLeftItemInTree();
  }
}

void CSeriousSkaStudioView::OnVkRight() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc->m_bViewPaused) {
    NextFrame();
  } else {
    SelectRightItemInTree();
  }
}

void CSeriousSkaStudioView::OnVkLeftWithCtrl() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc->m_bViewPaused) {
    SelectLeftItemInTree();
  } else {
    OnAnimPause();
    PrevFrame();
  }
}

void CSeriousSkaStudioView::OnVkRightWithCtrl() 
{
  CSeriousSkaStudioDoc *pDoc = theApp.GetDocument();
  if(pDoc->m_bViewPaused) {
    SelectRightItemInTree();
  } else {
    OnAnimPause();
    NextFrame();
  }
}

/*
 *  Menus 
 */ 

void CSeriousSkaStudioView::OnModelinstanceSavewithoffset() 
{
  NodeInfo &ni = theApp.m_dlgBarTreeView.GetSelectedNodeInfo();
  if(ni.ni_iType == NT_MODELINSTANCE) {
    CModelInstance *pmi = (CModelInstance*)ni.ni_pPtr;
    CModelInstance *pmiParent = pmi->GetParent(theApp.GetDocument()->m_ModelInstance);
  
    CString fnOldSmcFile;
    // if parent of model instance exists
    if(pmiParent!=NULL) {
      // remmeber current fn of model instance
      fnOldSmcFile = pmi->mi_fnSourceFile;
      // temporary remove it to save offset of model instance in same file
      pmiParent->RemoveChild(pmi);
    }

    // save model instance
    theApp.SaveModelAs(pmi);

    // if there was a parent for model instance
    if(pmiParent!=NULL) {
      // return model instance to his parent
      pmiParent->AddChild(pmi);
      // restore old filename
      pmi->mi_fnSourceFile = CTString(CStringA(fnOldSmcFile));
    }
  } else {
    ASSERT(FALSE); // This must be model instance
  }
}

void CSeriousSkaStudioView::OnConvertSelected() 
{
  CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
  if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgListOpt) {
    pDlgCurrent->OnBtConvert();
  } else {
    ASSERT(FALSE); // Current client dialog must be listopt dialog
  }
}

void CSeriousSkaStudioView::OnResetColisionbox() 
{
  CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
  if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgColision) {
    pDlgCurrent->OnBtResetColision();
  } else {
    ASSERT(FALSE); // Current client dialog must be colision dialog
  }
}

void CSeriousSkaStudioView::OnAllFramesRecalc() 
{
  CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
  if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgAllFrames) {
    pDlgCurrent->OnBtCalcAllframesBbox();
  } else {
    ASSERT(FALSE); // Current client dialog must be allframes dialog
  }
}

void CSeriousSkaStudioView::OnReloadTexture() 
{
  CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
  if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgTexture) {
    pDlgCurrent->OnBtReloadTexture();
  } else {
    ASSERT(FALSE); // Current client dialog must be texture dialog
  }
}

void CSeriousSkaStudioView::OnRecreateTexture() 
{
  CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
  if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgTexture) {
    pDlgCurrent->OnBtRecreateTexture();
  } else {
    ASSERT(FALSE); // Current client dialog must be texture dialog
  }
}

void CSeriousSkaStudioView::OnBrowseTexture() 
{
  CDlgClient *pDlgCurrent = (CDlgClient*)theApp.m_dlgBarTreeView.pdlgCurrent;
  if(pDlgCurrent == &theApp.m_dlgBarTreeView.m_dlgTexture) {
    pDlgCurrent->OnBtBrowseTexture();
  } else {
    ASSERT(FALSE); // Current client dialog must be texture dialog
  }
}
