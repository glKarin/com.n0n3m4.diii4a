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

// ModelerView.cpp : implementation of the CModelerView class 
//

#include "stdafx.h"
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;   
#endif

/////////////////////////////////////////////////////////////////////////////
// CModelerView

extern UINT APIENTRY ModelerFileRequesterHook( HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

static TIME tmSwapBuffers = 0;
static FLOAT _bSoundPlayed = FALSE;

#define MIN_MODEL_DISTANCE 3
#define MAX_MODEL_DISTANCE -200

IMPLEMENT_DYNCREATE(CModelerView, CView)

BEGIN_MESSAGE_MAP(CModelerView, CView)
	//{{AFX_MSG_MAP(CModelerView)
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_ANIM_PLAY, OnAnimPlay)  
	ON_COMMAND(ID_ANIM_PREVANIM, OnAnimPrevAnim)
	ON_COMMAND(ID_ANIM_NEXTANIM, OnAnimNextAnim)
	ON_COMMAND(ID_ANIM_NEXTFRAME, OnAnimNextFrame)
	ON_COMMAND(ID_ANIM_PREVFRAME, OnAnimPrevFrame)
	ON_WM_RBUTTONDOWN()
	ON_UPDATE_COMMAND_UI(ID_ANIM_PLAY, OnUpdateAnimPlay)
	ON_UPDATE_COMMAND_UI(ID_ANIM_NEXTFRAME, OnUpdateAnimNextframe)
	ON_UPDATE_COMMAND_UI(ID_ANIM_PREVFRAME, OnUpdateAnimPrevframe)
	ON_COMMAND(ID_ANIM_CHOOSE, OnAnimChoose)
	ON_COMMAND(ID_ANIM_MIP_PRECIZE, OnAnimMipPrecize)
	ON_COMMAND(ID_ANIM_MIP_ROUGH, OnAnimMipRough)
	ON_UPDATE_COMMAND_UI(ID_ANIM_MIP_PRECIZE, OnUpdateAnimMipPrecize)
	ON_UPDATE_COMMAND_UI(ID_ANIM_MIP_ROUGH, OnUpdateAnimMipRough)
	ON_COMMAND(ID_OPT_AUTO_MIP_MODELING, OnOptAutoMipModeling)
	ON_UPDATE_COMMAND_UI(ID_OPT_AUTO_MIP_MODELING, OnUpdateOptAutoMipModeling)
	ON_COMMAND(ID_ANIM_ROTATION, OnAnimRotation)
	ON_UPDATE_COMMAND_UI(ID_ANIM_ROTATION, OnUpdateAnimRotation)
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_PREFS_CHANGE_INK, OnPrefsChangeInk)
	ON_COMMAND(ID_PREFS_CHANGE_PAPER, OnPrefsChangePaper)
	ON_COMMAND(ID_FILE_REMOVE_TEXTURE, OnFileRemoveTexture)
	ON_COMMAND(ID_SCRIPT_OPEN, OnScriptOpen)
	ON_COMMAND(ID_SCRIPT_UPDATE_ANIMATIONS, OnScriptUpdateAnimations)
	ON_COMMAND(ID_SCRIPT_UPDATE_MIPMODELS, OnScriptUpdateMipmodels)
	ON_COMMAND(ID_LIGHT_ON, OnLightOn)
	ON_COMMAND(ID_LIGHT_COLOR, OnLightColor)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_ON, OnUpdateLightOn)
	ON_COMMAND(ID_REND_BBOX_ALL, OnRendBboxAll)
	ON_COMMAND(ID_REND_BBOX_FRAME, OnRendBboxFrame)
	ON_COMMAND(ID_REND_WIRE_ONOFF, OnRendWireOnoff)
	ON_COMMAND(ID_REND_HIDDEN_LINES, OnRendHiddenLines)
	ON_COMMAND(ID_REND_NO_TEXTURE, OnRendNoTexture)
	ON_COMMAND(ID_REND_ON_COLORS, OnRendOnColors)
	ON_COMMAND(ID_REND_OFF_COLORS, OnRendOffColors)
	ON_COMMAND(ID_REND_SURFACE_COLORS, OnRendSurfaceColors)
	ON_COMMAND(ID_REND_WHITE_TEXTURE, OnRendWhiteTexture)
	ON_COMMAND(ID_REND_USE_TEXTURE, OnRendUseTexture)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_COLOR, OnUpdateLightColor)
	ON_UPDATE_COMMAND_UI(ID_MAPPING_ON, OnUpdateMappingOn)
	ON_UPDATE_COMMAND_UI(ID_SCRIPT_OPEN, OnUpdateScriptOpen)
	ON_UPDATE_COMMAND_UI(ID_SCRIPT_UPDATE_ANIMATIONS, OnUpdateScriptUpdateAnimations)
	ON_UPDATE_COMMAND_UI(ID_SCRIPT_UPDATE_MIPMODELS, OnUpdateScriptUpdateMipmodels)
	ON_UPDATE_COMMAND_UI(ID_ANIM_NEXTANIM, OnUpdateAnimNextanim)
	ON_UPDATE_COMMAND_UI(ID_ANIM_PREVANIM, OnUpdateAnimPrevanim)
	ON_UPDATE_COMMAND_UI(ID_ANIM_CHOOSE, OnUpdateAnimChoose)
	ON_UPDATE_COMMAND_UI(ID_FILE_REMOVE_TEXTURE, OnUpdateFileRemoveTexture)
	ON_COMMAND(ID_MAGNIFY_LESS, OnMagnifyLess)
	ON_COMMAND(ID_MAGNIFY_MORE, OnMagnifyMore)
	ON_COMMAND(ID_WINDOW_FIT, OnWindowFit)
	ON_COMMAND(ID_WINDOW_CENTER, OnWindowCenter)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_COMMAND(ID_BACKGROUND_TEXTURE, OnBackgroundTexture)
	ON_UPDATE_COMMAND_UI(ID_REND_HIDDEN_LINES, OnUpdateRendHiddenLines)
	ON_UPDATE_COMMAND_UI(ID_REND_WIRE_ONOFF, OnUpdateRendWireOnoff)
	ON_UPDATE_COMMAND_UI(ID_REND_NO_TEXTURE, OnUpdateRendNoTexture)
	ON_UPDATE_COMMAND_UI(ID_REND_WHITE_TEXTURE, OnUpdateRendWhiteTexture)
	ON_UPDATE_COMMAND_UI(ID_REND_SURFACE_COLORS, OnUpdateRendSurfaceColors)
	ON_UPDATE_COMMAND_UI(ID_REND_ON_COLORS, OnUpdateRendOnColors)
	ON_UPDATE_COMMAND_UI(ID_REND_OFF_COLORS, OnUpdateRendOffColors)
	ON_UPDATE_COMMAND_UI(ID_REND_USE_TEXTURE, OnUpdateRendUseTexture)
	ON_UPDATE_COMMAND_UI(ID_REND_BBOX_ALL, OnUpdateRendBboxAll)
	ON_UPDATE_COMMAND_UI(ID_REND_BBOX_FRAME, OnUpdateRendBboxFrame)
	ON_COMMAND(ID_TAKE_SCREEN_SHOOT, OnTakeScreenShoot)
	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_BACKG_PICTURE, OnBackgPicture)
	ON_COMMAND(ID_BACKG_COLOR, OnBackgColor)
	ON_COMMAND(ID_REND_FLOOR, OnRendFloor)
	ON_UPDATE_COMMAND_UI(ID_REND_FLOOR, OnUpdateRendFloor)
	ON_COMMAND(ID_STAINS_INSERT, OnStainsInsert)
	ON_COMMAND(ID_STAINS_DELETE, OnStainsDelete)
	ON_UPDATE_COMMAND_UI(ID_STAINS_DELETE, OnUpdateStainsDelete)
	ON_COMMAND(ID_STAINS_PREVIOUS_STAIN, OnStainsPreviousStain)
	ON_COMMAND(ID_STAINS_NEXT_STAIN, OnStainsNextStain)
	ON_UPDATE_COMMAND_UI(ID_STAINS_INSERT, OnUpdateStainsInsert)
	ON_UPDATE_COMMAND_UI(ID_STAINS_NEXT_STAIN, OnUpdateStainsNextStain)
	ON_UPDATE_COMMAND_UI(ID_STAINS_PREVIOUS_STAIN, OnUpdateStainsPreviousStain)
	ON_COMMAND(ID_SHADOW_WORSE, OnShadowWorse)
	ON_COMMAND(ID_SHADOW_BETTER, OnShadowBetter)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_SAVE_THUMBNAIL, OnSaveThumbnail)
	ON_UPDATE_COMMAND_UI(ID_SAVE_THUMBNAIL, OnUpdateSaveThumbnail)
	ON_COMMAND(ID_RESTART_ANIMATIONS, OnRestartAnimations)
	ON_COMMAND(ID_FRAME_RATE, OnFrameRate)
	ON_UPDATE_COMMAND_UI(ID_FRAME_RATE, OnUpdateFrameRate)
	ON_COMMAND(ID_HEADING, OnHeading)
	ON_COMMAND(ID_PITCH, OnPitch)
	ON_COMMAND(ID_BANKING, OnBanking)
	ON_UPDATE_COMMAND_UI(ID_COLLISION_BOX, OnUpdateCollisionBox)
	ON_COMMAND(ID_COLLISION_BOX, OnCollisionBox)
	ON_COMMAND(ID_RESET_VIEWER, OnResetViewer)
	ON_UPDATE_COMMAND_UI(ID_RESET_VIEWER, OnUpdateResetViewer)
	ON_COMMAND(ID_DOLLY_VIEWER, OnDollyViewer)
	ON_UPDATE_COMMAND_UI(ID_DOLLY_VIEWER, OnUpdateDollyViewer)
	ON_COMMAND(ID_DOLLY_LIGHT, OnDollyLight)
	ON_UPDATE_COMMAND_UI(ID_DOLLY_LIGHT, OnUpdateDollyLight)
	ON_COMMAND(ID_DOLLY_LIGHT_COLOR, OnDollyLightColor)
	ON_UPDATE_COMMAND_UI(ID_DOLLY_LIGHT_COLOR, OnUpdateDollyLightColor)
	ON_COMMAND(ID_NEXT_TEXTURE, OnNextTexture)
	ON_UPDATE_COMMAND_UI(ID_NEXT_TEXTURE, OnUpdateNextTexture)
	ON_COMMAND(ID_PREVIOUS_TEXTURE, OnPreviousTexture)
	ON_UPDATE_COMMAND_UI(ID_PREVIOUS_TEXTURE, OnUpdatePreviousTexture)
	ON_COMMAND(ID_RECREATE_TEXTURE, OnRecreateTexture)
	ON_UPDATE_COMMAND_UI(ID_RECREATE_TEXTURE, OnUpdateRecreateTexture)
	ON_COMMAND(ID_CREATE_MIP_MODELS, OnCreateMipModels)
	ON_COMMAND(ID_PICK_VERTEX, OnPickVertex)
	ON_UPDATE_COMMAND_UI(ID_PICK_VERTEX, OnUpdatePickVertex)
	ON_COMMAND(ID_DOLLY_MIP_MODELING, OnDollyMipModeling)
	ON_UPDATE_COMMAND_UI(ID_DOLLY_MIP_MODELING, OnUpdateDollyMipModeling)
	ON_COMMAND(ID_ANIM_PLAY_ONCE, OnAnimPlayOnce)
	ON_UPDATE_COMMAND_UI(ID_ANIM_PLAY_ONCE, OnUpdateAnimPlayOnce)
	ON_COMMAND(ID_TILE_TEXTURE, OnTileTexture)
	ON_UPDATE_COMMAND_UI(ID_TILE_TEXTURE, OnUpdateTileTexture)
	ON_COMMAND(ID_ADD_REFLECTION_TEXTURE, OnAddReflectionTexture)
	ON_COMMAND(ID_ADD_SPECULAR, OnAddSpecular)
	ON_COMMAND(ID_REMOVE_REFLECTION, OnRemoveReflection)
	ON_UPDATE_COMMAND_UI(ID_REMOVE_REFLECTION, OnUpdateRemoveReflection)
	ON_COMMAND(ID_REMOVE_SPECULAR, OnRemoveSpecular)
	ON_UPDATE_COMMAND_UI(ID_REMOVE_SPECULAR, OnUpdateRemoveSpecular)
	ON_COMMAND(ID_ADD_BUMP_TEXTURE, OnAddBumpTexture)
	ON_COMMAND(ID_REMOVE_BUMP_MAP, OnRemoveBumpMap)
	ON_UPDATE_COMMAND_UI(ID_REMOVE_BUMP_MAP, OnUpdateRemoveBumpMap)
	ON_COMMAND(ID_MAPPING_ON, OnMappingOn)
	ON_COMMAND(ID_SKIN_TEXTURE, OnSkinTexture)
	ON_UPDATE_COMMAND_UI(ID_SKIN_TEXTURE, OnUpdateSkinTexture)
	ON_WM_RBUTTONDBLCLK()
	ON_COMMAND(ID_SURFACE_NUMBERS, OnSurfaceNumbers)
	ON_UPDATE_COMMAND_UI(ID_SURFACE_NUMBERS, OnUpdateSurfaceNumbers)
	ON_COMMAND(ID_EXPORT_SURFACES, OnExportSurfaces)
	ON_COMMAND(ID_PREVIOUS_BCG_TEXTURE, OnPreviousBcgTexture)
	ON_COMMAND(ID_NEXT_BCG_TEXTURE, OnNextBcgTexture)
	ON_UPDATE_COMMAND_UI(ID_PREVIOUS_BCG_TEXTURE, OnUpdatePreviousBcgTexture)
	ON_UPDATE_COMMAND_UI(ID_NEXT_BCG_TEXTURE, OnUpdateNextBcgTexture)
	ON_COMMAND(ID_WINDOW_TOGGLEMAX, OnWindowTogglemax)
	ON_COMMAND(ID_EXPORT_FOR_SKINING, OnExportForSkining)
	ON_COMMAND(ID_RENDER_SURFACES_IN_COLORS, OnRenderSurfacesInColors)
	ON_UPDATE_COMMAND_UI(ID_RENDER_SURFACES_IN_COLORS, OnUpdateRenderSurfacesInColors)
	ON_COMMAND(ID_VIEW_AXIS, OnViewAxis)
	ON_UPDATE_COMMAND_UI(ID_VIEW_AXIS, OnUpdateViewAxis)
	ON_COMMAND(ID_VIEW_INFO, OnViewInfo)
	ON_COMMAND(ID_LIST_ANIMATIONS, OnListAnimations)
	ON_UPDATE_COMMAND_UI(ID_LIST_ANIMATIONS, OnUpdateListAnimations)
	ON_COMMAND(ID_CHANGE_AMBIENT, OnChangeAmbient)
	ON_UPDATE_COMMAND_UI(ID_CHANGE_AMBIENT, OnUpdateChangeAmbient)
	ON_COMMAND(ID_TOGGLE_ALL_SURFACES, OnToggleAllSurfaces)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_ALL_SURFACES, OnUpdateToggleAllSurfaces)
	ON_COMMAND(ID_KEY_A, OnKeyA)
	ON_COMMAND(ID_KEY_T, OnKeyT)
	ON_COMMAND(ID_ANIM_FIRST, OnAnimFirst)
	ON_UPDATE_COMMAND_UI(ID_ANIM_FIRST, OnUpdateAnimFirst)
	ON_COMMAND(ID_ANIM_LAST, OnAnimLast)
	ON_UPDATE_COMMAND_UI(ID_ANIM_LAST, OnUpdateAnimLast)
	ON_COMMAND(ID_TOGGLE_MEASURE_VTX, OnToggleMeasureVtx)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_MEASURE_VTX, OnUpdateToggleMeasureVtx)
	ON_COMMAND(ID_FIRST_FRAME, OnFirstFrame)
	ON_COMMAND(ID_LAST_FRAME, OnLastFrame)
	//}}AFX_MSG_MAP
	// Standard printing commands
	//ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelerView construction/destruction

CModelerView::CModelerView()
{
  m_bViewMeasureVertex = FALSE;
  m_vViewMeasureVertex = FLOAT3D(0,0,0);
  m_atAxisType = AT_NONE;
  m_pmtvClosestVertex = NULL;
  m_bTileMappingBCG = FALSE;
  m_bAnyKeyPressed = FALSE;
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  m_InputAction = IA_NONE;
  m_fTargetDistance = 2.0f;
  m_fDollySpeedMipModeling = 2.0f;
  
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CModelerView *pModelerView = DYNAMIC_DOWNCAST(CModelerView, 
             pMainFrame->GetActiveFrame()->GetActiveView());
  // If view allready exists, copy preferences
  if( (pModelerView != NULL) && (theApp.m_Preferences.ap_CopyExistingWindowPrefs) )
  {
    m_bPrintSurfaceNumbers = pModelerView->m_bPrintSurfaceNumbers;
    m_bRenderMappingInSurfaceColors = pModelerView->m_bRenderMappingInSurfaceColors;
    m_bFrameRate = pModelerView->m_bFrameRate;
    m_AutoRotating = pModelerView->m_AutoRotating;
    m_iChoosedColor = pModelerView->m_iChoosedColor;
    m_bOnColorMode = pModelerView->m_bOnColorMode;
    m_IsWinBcgTexture = pModelerView->m_IsWinBcgTexture;
    m_FloorOn = pModelerView->m_FloorOn;
    m_bDollyViewer = pModelerView->m_bDollyViewer;
    m_bDollyMipModeling = pModelerView->m_bDollyMipModeling;
    m_bDollyLight = pModelerView->m_bDollyLight;
    m_bDollyLightColor = pModelerView->m_bDollyLightColor;
    m_fnBcgTexture = pModelerView->m_fnBcgTexture;
    m_PaperColor = pModelerView->m_PaperColor;
    m_InkColor = pModelerView->m_InkColor;
    m_ptdiTextureDataInfo = pModelerView->m_ptdiTextureDataInfo;
    m_LightModeOn = pModelerView->m_LightModeOn;
    m_bMappingMode = pModelerView->m_bMappingMode;
    m_bCollisionMode = pModelerView->m_bCollisionMode;
    m_IsMappingBcgTexture = pModelerView->m_IsMappingBcgTexture;
    m_fCurrentMipFactor = pModelerView->m_fCurrentMipFactor;
    m_iCurrentFrame = pModelerView->m_iCurrentFrame;
    m_LightColor = pModelerView->m_LightColor;
    m_colAmbientColor = pModelerView->m_colAmbientColor;
    m_plLightPlacement = pModelerView->m_plLightPlacement;
    m_plModelPlacement = pModelerView->m_plModelPlacement;
    m_vTarget = pModelerView->m_vTarget;
    m_fTargetDistance = pModelerView->m_fTargetDistance;
    m_angViewerOrientation = pModelerView->m_angViewerOrientation;
    m_RenderPrefs.SetRenderType( pModelerView->m_RenderPrefs.GetRenderType());
    m_iActivePatchBitIndex = pModelerView->m_iActivePatchBitIndex;
    m_offx = pModelerView->m_offx;
    m_offy = pModelerView->m_offy;
    m_MagnifyFactor = pModelerView->m_MagnifyFactor;
    m_LightDistance = pModelerView->m_LightDistance;
    m_ShowAllSurfaces = pModelerView->m_ShowAllSurfaces;
    m_fFOW = pModelerView->m_fFOW;
  }
  else
  {
    m_bPrintSurfaceNumbers = FALSE;
    m_bRenderMappingInSurfaceColors = FALSE;
    m_bFrameRate = FALSE;
    m_AutoRotating = FALSE;
    m_iChoosedColor = 0;
    m_bOnColorMode = TRUE;
    m_IsWinBcgTexture = theApp.m_Preferences.ap_bIsBcgVisibleByDefault;
    m_FloorOn = theApp.m_Preferences.ap_bIsFloorVisibleByDefault;
    m_bDollyViewer = FALSE;
    m_bDollyMipModeling = FALSE;
    m_bDollyLight = FALSE;
    m_bDollyLightColor = FALSE;
    m_fnBcgTexture = theApp.m_Preferences.ap_DefaultWinBcgTexture;
    m_PaperColor = 0x98c0a0;
    m_InkColor = 0;
    m_ptdiTextureDataInfo = (CTextureDataInfo *) -1;
    m_LightModeOn = FALSE;
    m_bMappingMode = FALSE;
    m_bCollisionMode = FALSE;
    m_IsMappingBcgTexture = TRUE;
    m_fCurrentMipFactor = 0.0f;
    m_iCurrentFrame = 0;
    m_LightColor = 0xBFBFBF00;
    m_plLightPlacement.pl_PositionVector = FLOAT3D( 0.0f, 0.0f, 0.0f); // center
    m_plLightPlacement.pl_OrientationAngle(1) = AngleDeg( 45.0f);      // heading
    m_plLightPlacement.pl_OrientationAngle(2) = AngleDeg( -45.0f);     // pitch
    m_plLightPlacement.pl_OrientationAngle(3) = AngleDeg( 0.0f);       // banking
    m_RenderPrefs.SetRenderType( RT_TEXTURE);
    m_RenderPrefs.SetShadowQuality( 0);
    m_offx = 0;
    m_offy = 0;
    m_MagnifyFactor = 0.25f;
    m_LightDistance = 3.5f;
    
    m_colAmbientColor = theApp.m_Preferences.ap_colDefaultAmbientColor;
    FLOAT fHeading = theApp.m_Preferences.ap_fDefaultHeading;
    FLOAT fPitch = theApp.m_Preferences.ap_fDefaultPitch;
    FLOAT fBanking = theApp.m_Preferences.ap_fDefaultBanking;
    m_fFOW = theApp.m_Preferences.ap_fDefaultFOW;

    m_plModelPlacement.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
    m_plModelPlacement.pl_OrientationAngle = 
      ANGLE3D(AngleDeg(fHeading), AngleDeg(fPitch),AngleDeg(fBanking));
    m_ShowAllSurfaces = TRUE;
  }

  if( theApp.m_Preferences.ap_SetDefaultColors)
  {
    m_PaperColor = CLRF_CLR( theApp.m_Preferences.ap_DefaultPaperColor);
    m_InkColor = CLRF_CLR( theApp.m_Preferences.ap_DefaultInkColor);
  }
}

CModelerView::~CModelerView()
{
	// destroy canvas that is currently used
	if (m_pViewPort!=NULL) {
    _pGfx->DestroyWindowCanvas(m_pViewPort);
	}
}

void CModelerView::ResetViewerPosition(void)
{
  CModelerDoc* pDoc = GetDocument();
  // only now we can obtain model data and set initial viewer position
  // get model data
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;
  // obtain bounding box of all frames
  FLOATaabbox3D MaxBB;
  pMD->GetAllFramesBBox( MaxBB);
  // initial distance is same as of all frames's bbox diagonal vector
  FLOAT fModelInitialDistance = MaxBB.Size().Length();
  // set initial target
  m_vTarget = MaxBB.Center();
  // set initial target distance
  m_fTargetDistance = fModelInitialDistance;
  // set default orientation
  m_angViewerOrientation = ANGLE3D(AngleDeg(-10.0f),AngleDeg(-30.0f),0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine tries to find 
BOOL CModelerView::AssureValidTDI()
{
  if(m_ptdiTextureDataInfo == NULL)
    return FALSE;
  
  CModelerDoc* pDoc = GetDocument();
  BOOL bValidTex = FALSE;
  FOREACHINLIST( CTextureDataInfo, tdi_ListNode, pDoc->m_emEditModel.edm_WorkingSkins, it)
  {
    if( &it.Current() == m_ptdiTextureDataInfo)
    {
      bValidTex = TRUE;
      break;
    }
  }
  if( (bValidTex == FALSE) && (!pDoc->m_emEditModel.edm_WorkingSkins.IsEmpty()) )
  {
  	m_ptdiTextureDataInfo = LIST_HEAD( pDoc->m_emEditModel.edm_WorkingSkins,
                                    CTextureDataInfo, tdi_ListNode);
    bValidTex = TRUE;
  }
  
  if( !bValidTex)
  {
    m_ptdiTextureDataInfo = NULL;
  }
  return bValidTex;
}
/////////////////////////////////////////////////////////////////////////////
// CModelerView drawing


void CModelerView::SetProjectionData( CPerspectiveProjection3D &prProjection, CDrawPort *pDP)
{
  prProjection.FOVL() = AngleDeg(m_fFOW);
  prProjection.ScreenBBoxL() = FLOATaabbox2D( FLOAT2D(0.0f,0.0f),
        FLOAT2D((float)pDP->GetWidth(), (float)pDP->GetHeight()));
  prProjection.AspectRatioL() = 1.0f;
  prProjection.FrontClipDistanceL() = 0.05f;

  prProjection.ViewerPlacementL().pl_PositionVector = m_vTarget;
  prProjection.ViewerPlacementL().pl_OrientationAngle = m_angViewerOrientation;
  prProjection.Prepare();
  prProjection.ViewerPlacementL().Translate_OwnSystem(FLOAT3D( 0.0f, 0.0f, m_fTargetDistance));
}

FLOAT CModelerView::GetModelToViewerDistance(void)
{
  CPlacement3D plViewer = CPlacement3D( m_vTarget, m_angViewerOrientation);
  plViewer.Translate_OwnSystem(FLOAT3D( 0.0f, 0.0f, m_fTargetDistance));
  FLOAT3D vDistance = plViewer.pl_PositionVector-m_plModelPlacement.pl_PositionVector;
  return vDistance.Length();
}

void CModelerView::ClearBcg( COLOR color, CDrawPort *pDrawPort)
{
  // delete bcg or fill it with texture
  CTextureObject *ptoValid = 
    (CTextureObject *) theApp.GetValidBcgTexture( m_fnBcgTexture);
  if( (m_IsWinBcgTexture) && ( ptoValid != NULL) )
  {
    PIXaabbox2D screenBox;
    screenBox = PIXaabbox2D( PIX2D(0,0),
                             PIX2D(pDrawPort->GetWidth(), pDrawPort->GetHeight()) );
    pDrawPort->PutTexture( ptoValid, screenBox);
  }
  else
  {
 	  pDrawPort->Fill(color | CT_OPAQUE);
  }
}

static BOOL TimeIsBefore(TIME tmTime, TIME tmMark, TIME tmWrap)
{        
  BOOL bBefore = FALSE;
  TIME tmDelta = tmTime-tmMark;
  if (Abs(tmDelta)<tmWrap/2) {
    bBefore = TRUE;
  }

  if (tmDelta>0) {
    bBefore = !bBefore;
  }
  return bBefore;
}

/*
 * Draw a line for arrow drawing.
 */
static inline void DrawLine(CDrawPort &dp, const FLOAT2D &vPoint0,
                                 const FLOAT2D &vPoint1, COLOR color, ULONG ulLineType)
{
  PIX x0 = (PIX)vPoint0(1);
  PIX x1 = (PIX)vPoint1(1);
  PIX y0 = (PIX)vPoint0(2);
  PIX y1 = (PIX)vPoint1(2);

  dp.DrawLine(x0, y0, x1, y1, color, ulLineType);
}

/*
 * Draw an arrow for debugging edge directions.
 */
static inline void DrawArrow(CDrawPort &dp, PIX i0, PIX j0, PIX i1, PIX j1, COLOR color,
                             ULONG ulLineType)
{
  FLOAT2D vPoint0 = FLOAT2D((FLOAT)i0, (FLOAT)j0);
  FLOAT2D vPoint1 = FLOAT2D((FLOAT)i1, (FLOAT)j1);
  FLOAT2D vDelta      = vPoint1-vPoint0;
  FLOAT fDelta = vDelta.Length();
  FLOAT2D vArrowLen, vArrowWidth;
  if (fDelta>0.01) {
    vArrowLen   = vDelta/fDelta*FLOAT(10.0);
    vArrowWidth = vDelta/fDelta*FLOAT(2.0);
  } else {
    vArrowWidth = vArrowLen = FLOAT2D(0.0f, 0.0f);
  }
  Swap(vArrowWidth(1), vArrowWidth(2));

  DrawLine(dp, vPoint0, vPoint1, color, ulLineType);
  DrawLine(dp, vPoint1-vArrowLen+vArrowWidth, vPoint1, color, ulLineType);
  DrawLine(dp, vPoint1-vArrowLen-vArrowWidth, vPoint1, color, ulLineType);
}

INDEX _iTextLine = 0;
void CModelerView::DrawArrowAndTypeText(CPerspectiveProjection3D &prProjection,
                                        const FLOAT3D &v0, const FLOAT3D &v1, COLOR colColor, CTString strText)
{
  m_pDrawPort->SetFont( theApp.m_pfntFont);

  // get transformed end vertices
  FLOAT3D tv0, tv1;
  prProjection.PreClip(v0, tv0);
  prProjection.PreClip(v1, tv1);

  // clip the edge line
  FLOAT3D vClipped0 = tv0;
  FLOAT3D vClipped1 = tv1;
  ULONG ulClipFlags = prProjection.ClipLine(vClipped0, vClipped1);

  // if the edge remains after clipping to front plane
  if (ulClipFlags != LCF_EDGEREMOVED)
  {
    // project the vertices
    FLOAT3D v3d0, v3d1;
    prProjection.PostClip(vClipped0, v3d0);
    prProjection.PostClip(vClipped1, v3d1);

    // make 2d vertices
    FLOAT2D v2d0, v2d1;
    v2d0(1) = v3d0(1); v2d0(2) = v3d0(2);
    v2d1(1) = v3d1(1); v2d1(2) = v3d1(2);

    if( (Abs(v2d1(1)-v2d0(1)) > 8) || (Abs(v2d1(2)-v2d0(2)) > 8) )
    {
      // draw arrow-headed line between vertices
      DrawArrow( *m_pDrawPort, (PIX)v2d0(1), (PIX)v2d0(2), (PIX)v2d1(1), (PIX)v2d1(2), colColor, _FULL_);
      // type text over line
      if( strText != "")
      {
        m_pDrawPort->SetFont( theApp.m_pfntFont);
        m_pDrawPort->PutTextCXY( strText, v2d1(1)-8, v2d1(2)-16);
      }
    }
  }
}

void CModelerView::RenderAxis( CPerspectiveProjection3D &prProjection, CPlacement3D &pl, FLOAT fSize)
{
  // get rotation matrix
  FLOATmatrix3D mRot;
  MakeRotationMatrixFast( mRot, pl.pl_OrientationAngle);
  // get axis defining vertices
  FLOAT3D vCenter = pl.pl_PositionVector;
  FLOAT3D vX = FLOAT3D( fSize, 0.0f, 0.0f);
  FLOAT3D vY = FLOAT3D( 0.0f, fSize, 0.0f);
  FLOAT3D vZ = FLOAT3D( 0.0f, 0.0f, fSize);
  FLOAT3D vRotX = vCenter+vX*mRot;
  FLOAT3D vRotY = vCenter+vY*mRot;
  FLOAT3D vRotZ = vCenter-vZ*mRot;
  // project vertices and render axis arrows
  DrawArrowAndTypeText( prProjection, vCenter, vRotX, C_RED|CT_OPAQUE, "X");
  DrawArrowAndTypeText( prProjection, vCenter, vRotY, C_RED|CT_OPAQUE, "Y");
  DrawArrowAndTypeText( prProjection, vCenter, vRotZ, C_RED|CT_OPAQUE, "-Z");
}

CStaticStackArray<CRenderModel> _armRenderModels;

void CModelerView::RenderAxisOfAllAttachments(CPerspectiveProjection3D &prProjection,
                                              CPlacement3D &plParent, CModelObject &mo)
{
  // create render model structure of parent
  CRenderModel *prmParent = &_armRenderModels.Push();
  CAnyProjection3D apr;
  apr = prProjection;
  BeginModelRenderingView(apr, m_pDrawPort);
  prmParent->SetObjectPlacement( plParent);
  mo.SetupModelRendering(*prmParent);
  EndModelRenderingView();

  // for each attachment on this model object
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo.mo_lhAttachments, itamo)
  {
    // create new render model structure
    itamo->amo_prm = &_armRenderModels.Push();
    // obtain attachment's data
    mo.CreateAttachment(*prmParent, *itamo);
    // create placement of attachment (child)
    ANGLE3D a3dAnglesChild;
    DecomposeRotationMatrix(a3dAnglesChild, itamo->amo_prm->rm_mObjectRotation);
    CPlacement3D plChild = CPlacement3D( itamo->amo_prm->rm_vObjectPosition, a3dAnglesChild);
    // recurse
    RenderAxisOfAllAttachments( prProjection, plChild, itamo->amo_moModelObject);
    // don't render non-initialized attachments
    CModelData *pmd = (CModelData *) itamo->amo_moModelObject.GetData();
    if( pmd == NULL) continue;
    // obtain bounding box of attachment
    FLOATaabbox3D box;
    pmd->GetAllFramesBBox( box);
    FLOAT fSize = Clamp(box.Size().Length()/6.0f, 0.125f, 10.0f);
    RenderAxis( prProjection, plChild, fSize);
  }
  // all done
  _armRenderModels.PopAll();
}


void CModelerView::RenderView( CDrawPort *pDrawPort)
{
  CMainFrame* pmf = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  FLOATplane3D plFloorPlane;
  CModelerDoc* pDoc = GetDocument();
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;
  CTextureData *pTD, *pOldTD;
	INDEX i;
  
  // set effect textures (if they exist in edit model)
  try {
    if( m_ModelObject.mo_toReflection.GetName() != pDoc->m_emEditModel.edm_fnReflectionTexture)
      m_ModelObject.mo_toReflection.SetData_t( pDoc->m_emEditModel.edm_fnReflectionTexture);
  } catch (char *strError) { (void) strError;}
  try {
    if( m_ModelObject.mo_toSpecular.GetName() != pDoc->m_emEditModel.edm_fnSpecularTexture)
      m_ModelObject.mo_toSpecular.SetData_t( pDoc->m_emEditModel.edm_fnSpecularTexture);
  } catch (char *strError) { (void) strError;}
  try {
    if( m_ModelObject.mo_toBump.GetName() != pDoc->m_emEditModel.edm_fnBumpTexture)
      m_ModelObject.mo_toBump.SetData_t( pDoc->m_emEditModel.edm_fnBumpTexture);
  } catch (char *strError) { (void) strError;}

  // Calculate light position in absolute sytem 
  CModelInfo MI;
  FLOATaabbox3D MaxBB;
  if( m_ModelObject.GetData() == NULL)
    return;
  m_ModelObject.GetModelInfo( MI);  // info describing our model (not light)
  for( i=0; i<MI.mi_FramesCt; i++) { // here we find max bbox, union of all frames bboxes
    MaxBB |= m_ModelObject.GetFrameBBox( i);
  }

  CPlacement3D plLightPlacement;
  plLightPlacement.pl_OrientationAngle = m_plLightPlacement.pl_OrientationAngle;
  plLightPlacement.pl_PositionVector = MaxBB.Center();
  plLightPlacement.Translate_OwnSystem( FLOAT3D( 0.0f, 0.0f, m_LightDistance));
  plLightPlacement.Translate_AbsoluteSystem( m_plModelPlacement.pl_PositionVector);

  pOldTD = (CTextureData *) m_ModelObject.mo_toTexture.GetData();
  if( AssureValidTDI()) {
    pTD = m_ptdiTextureDataInfo->tdi_TextureData;
    if( pTD != m_ModelObject.mo_toTexture.GetData()) {
      m_ModelObject.mo_toTexture.SetData( pTD);
    }
  } else {
    m_ModelObject.mo_toTexture.SetData( NULL);
    ULONG CurrentTextureType = m_RenderPrefs.GetRenderType();
    // no need to revert to white-texture mode, renderer with handle NULL texture just fine
    // if( CurrentTextureType&RT_TEXTURE) 
    //   m_RenderPrefs.SetTextureType( RT_WHITE_TEXTURE);
    // }
  }

  MEX mexWidth, mexHeight;
  mexWidth = m_ModelObject.GetWidth();
  mexHeight = m_ModelObject.GetHeight();

  FLOAT3D FloorVtx1 = FLOAT3D( -6.0f, 0.0f, -6.0f);
  FLOAT3D FloorVtx2 = FLOAT3D( -6.0f, 0.0f,  6.0f);
  FLOAT3D FloorVtx3 = FLOAT3D(  6.0f, 0.0f,  6.0f);
  FLOAT3D FloorVtx4 = FLOAT3D(  6.0f, 0.0f, -6.0f);

  // make floor plane from three of floor's vertices
  plFloorPlane = FLOATplane3D( FloorVtx1, FloorVtx2, FloorVtx3);

  pDrawPort->FillZBuffer( ZBUF_BACK);
  ClearBcg( CLR_CLRF( m_PaperColor), pDrawPort);

  if( !m_bMappingMode)
  {
    // get current frame for info window
    m_iCurrentFrame = m_ModelObject.GetFrame();

    INDEX iAnim = m_ModelObject.GetAnim();
    CAttachedSound &asSound = pDoc->m_emEditModel.edm_aasAttachedSounds[iAnim];
    TIME tmAnimLen = m_ModelObject.GetAnimLength(iAnim);
    TIME tmPassed = m_ModelObject.GetPassedTime();
    
    BOOL bNowIsBeforeDelay = TimeIsBefore(tmPassed, asSound.as_fDelay, tmAnimLen);
    if(bNowIsBeforeDelay) _bSoundPlayed = FALSE;

    if( (asSound.as_fnAttachedSound != "") && (asSound.as_bPlaying) )
    {
      if( !_bSoundPlayed && !bNowIsBeforeDelay )
      {
        try
        {
          pDoc->m_soSoundObject.Stop();
          pDoc->m_soSoundObject.SetVolume( SL_VOLUME_MAX, SL_VOLUME_MAX);
          if( asSound.as_bLooping)
            pDoc->m_soSoundObject.Play_t( asSound.as_fnAttachedSound, SOF_LOOP);
          else
            pDoc->m_soSoundObject.Play_t( asSound.as_fnAttachedSound, 0);
          _bSoundPlayed = TRUE;
        }
        catch( char *strError)
        {
          (void) strError;
          //WarningMessage( strError);
        }
      }
    }

    // make perspective projection for lamp rendering
    CPerspectiveProjection3D prPerspectiveProjection;

    if( m_FloorOn)
    {
      // floor model and its texture must be valid
      ASSERT( theApp.m_pFloorModelObject != NULL);
      ASSERT( theApp.m_ptdFloorTexture != NULL);

      // initialize projection
      SetProjectionData( prPerspectiveProjection, pDrawPort);
      // set floor box's stretch factor
      theApp.m_pFloorModelObject->mo_Stretch = FLOAT3D( 1.0f, 1.0f, 1.0f);
      // set texture rendering mode and phong shading
      _mrpModelRenderPrefs.SetRenderType( RT_TEXTURE|RT_SHADING_PHONG);
      // render floor model
      // prepare render model structure
      CRenderModel rmRenderModel;
      CAnyProjection3D apr;
      apr = prPerspectiveProjection;
      BeginModelRenderingView(apr, pDrawPort);
      // set light placement
      rmRenderModel.rm_vLightDirection = -plLightPlacement.pl_PositionVector;
      // set color of light
      rmRenderModel.rm_colLight = m_LightColor;
      rmRenderModel.rm_colAmbient = m_colAmbientColor;

      // set floor's placement and orientation
      rmRenderModel.SetObjectPlacement(
        CPlacement3D(ANGLE3D( 0, 0, 0), FLOAT3D( 0.0f, 0.0f, 0.0f)));
      // render floor
      theApp.m_pFloorModelObject->SetupModelRendering(rmRenderModel);
      theApp.m_pFloorModelObject->RenderModel(rmRenderModel);
      EndModelRenderingView();
    }

    // set current color rendering preferences 
    m_RenderPrefs.SetInkColor( CLR_CLRF( m_InkColor));
    m_RenderPrefs.SetPaperColor( CLR_CLRF( m_PaperColor));
    // copy local (view's) rendering preferences over global model rendering preferences
    _mrpModelRenderPrefs = m_RenderPrefs;

    // set projection data
    SetProjectionData( prPerspectiveProjection, pDrawPort);

    // prepare render model structure
    CRenderModel rmRenderModel;
    CAnyProjection3D apr;
    apr = prPerspectiveProjection;
    BeginModelRenderingView(apr, pDrawPort);
    // set light placement
    rmRenderModel.rm_vLightDirection = m_plModelPlacement.pl_PositionVector
                                     - plLightPlacement.pl_PositionVector;
    // set color of light
    rmRenderModel.rm_colLight   = m_LightColor;
    rmRenderModel.rm_colAmbient = m_colAmbientColor;
    
    // obtain translation speed value
    CString csSpeed;
    pmf->m_ctrlZSpeed.GetWindowText( csSpeed);
    CTString strSpeed = CStringA(csSpeed);
    FLOAT fSpeed;
    BOOL bSpeedValid = strSpeed.ScanF( "%g", &fSpeed);
    
    // obtain loop value
    CString csLoop;
    pmf->m_ctrlZLoop.GetWindowText( csLoop);
    CTString strLoop = CStringA(csLoop);
    INDEX iLoop;
    BOOL bLoopValid = strLoop.ScanF( "%d", &iLoop);
    
    // simulate translation along z-axis
    CPlacement3D plTranslated = m_plModelPlacement;
    if( bLoopValid && bSpeedValid && fSpeed!=0 && iLoop>0 && !m_ModelObject.IsPaused()) {
      TIME tmPassed = _pTimer->CurrentTick() - m_ModelObject.ao_tmAnimStart;
      TIME tmDuration = m_ModelObject.GetCurrentAnimLength();
      if( tmPassed>tmDuration*iLoop) {
        tmPassed = 0;
        m_ModelObject.ResetAnim();
      }
      plTranslated.pl_PositionVector(3) += -fSpeed*(tmPassed-tmDuration);
    }
    // set position of document's model
    rmRenderModel.SetObjectPlacement(plTranslated);

    // initialize rendering of document's model
    m_ModelObject.SetupModelRendering( rmRenderModel);
    // get current mip factor
    m_fCurrentMipFactor = rmRenderModel.rm_fMipFactor;

    // if model is visible
    if( m_ModelObject.IsModelVisible( m_fCurrentMipFactor) || (m_InputAction == IA_MIP_RANGING)) {   
      // if floor is on
      if( m_FloorOn) {
        // render shadow of document's model
         m_ModelObject.RenderShadow( rmRenderModel, plLightPlacement, 20.0f, 0.0f, 1.0f, plFloorPlane);
      }
      // render document's model
      m_ModelObject.RenderModel( rmRenderModel);
    }
    // get current mip model
    pDoc->m_iCurrentMip = m_ModelObject.mo_iLastRenderMipLevel;
    EndModelRenderingView();

    // if lamp mode is on or allways see lamp flag is set on and if lamp model exist
    if( (m_LightModeOn || theApp.m_Preferences.ap_AllwaysSeeLamp)
     && (theApp.m_pLampModelData != NULL))
    {
      // set projection data
      SetProjectionData( prPerspectiveProjection, pDrawPort);
      // set light beam's surface color same as current light color
      theApp.m_LampModelObject->SetSurfaceColor( 0, 2, m_LightColor | CT_OPAQUE);
      // set these modele rendering preferences: surface colors, no shadows, no bounding boxes
      _mrpModelRenderPrefs.SetRenderType( RT_TEXTURE|RT_SHADING_PHONG);
      _mrpModelRenderPrefs.SetShadowQuality( 0);
      _mrpModelRenderPrefs.BBoxFrameShow(FALSE);
      _mrpModelRenderPrefs.BBoxAllShow(FALSE);
      _mrpModelRenderPrefs.SetInkColor( 0x55000000);

      // prepare render model structure
      CRenderModel rmRenderModel;
      CAnyProjection3D apr;
      apr = prPerspectiveProjection;
      BeginModelRenderingView(apr, pDrawPort);
      // set color of shading light
      rmRenderModel.rm_colLight   = C_WHITE|CT_OPAQUE;
      rmRenderModel.rm_colAmbient = C_dGRAY|CT_OPAQUE;
      // set shading light placement
      rmRenderModel.rm_vLightDirection = FLOAT3D( 1.0f, 1.0f, 1.0f);
      // set placement of lamp model
      rmRenderModel.SetObjectPlacement(plLightPlacement);
      // render lamp rendering
      theApp.m_LampModelObject->SetupModelRendering(rmRenderModel);
      theApp.m_LampModelObject->RenderModel(rmRenderModel);
      EndModelRenderingView();
    }
  
    // test if we are in collision mode
    if( m_bCollisionMode)
    {
      // collision box model and its texture must be valid
      ASSERT( theApp.m_pCollisionBoxModelObject != NULL);
      ASSERT( theApp.m_ptdCollisionBoxTexture != NULL);

      // initialize projection
      SetProjectionData( prPerspectiveProjection, pDrawPort);
      FLOAT3D vMin = pDoc->m_emEditModel.GetCollisionBoxMin();
      FLOAT3D vMax = pDoc->m_emEditModel.GetCollisionBoxMax();
      // get collision bounding box
      FLOATaabbox3D bbCollision( vMin,vMax);
      // set collision box's stretch factor
      theApp.m_pCollisionBoxModelObject->mo_Stretch = bbCollision.Size();
      // set wire and hiden lines along with texture rendering mode and shiny shading
      _mrpModelRenderPrefs.SetRenderType( 
        RT_WIRE_ON|RT_HIDDEN_LINES|RT_TEXTURE|RT_SHADING_PHONG);
      // render collision box

      // obtain collision box offset vector
      CPlacement3D plRotatedCollisionBoxOffset = 
        CPlacement3D( bbCollision.Center(), ANGLE3D( 0,0,0));
      // convert collision box center into model coordinate system
      plRotatedCollisionBoxOffset.RelativeToAbsolute( m_plModelPlacement);

      // prepare render model structure
      CRenderModel rmRenderModel;
      // set converted collision box's placement
      rmRenderModel.SetObjectPlacement(plRotatedCollisionBoxOffset);
      CAnyProjection3D apr;
      apr = prPerspectiveProjection;
      BeginModelRenderingView(apr, pDrawPort);
      // set placement of shading light
      rmRenderModel.rm_vLightDirection = 
          m_plModelPlacement.pl_PositionVector-plLightPlacement.pl_PositionVector;
      // set color of shading light
      rmRenderModel.rm_colLight = m_LightColor;
      rmRenderModel.rm_colAmbient = m_colAmbientColor;
      // render collision box model
      theApp.m_pCollisionBoxModelObject->SetupModelRendering(rmRenderModel);
      theApp.m_pCollisionBoxModelObject->RenderModel(rmRenderModel);
      EndModelRenderingView();
    }

    if( m_bViewMeasureVertex)
    {
      // prepare the projection
      CPerspectiveProjection3D prProjection;
      SetProjectionData( prProjection, pDrawPort);
      prProjection.ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
      prProjection.Prepare();

      FLOAT3D v3D;
      prProjection.ProjectCoordinate(m_vViewMeasureVertex, v3D);
      // convert y coordinate from mathemathical representation into screen one
      v3D(2) = pDrawPort->GetHeight()- v3D(2);
      FLOAT2D v2D;
      v2D(1)=v3D(1);
      v2D(2)=v3D(2);

      DrawLine(*pDrawPort, v2D-FLOAT2D(5,0), v2D+FLOAT2D(5,0), C_RED|CT_OPAQUE, _FULL_);
      DrawLine(*pDrawPort, v2D-FLOAT2D(0,5), v2D+FLOAT2D(0,5), C_RED|CT_OPAQUE, _FULL_);
    }

    // if we should render axis
    if( m_atAxisType != AT_NONE)
    {
      // prepare the projection
      CPerspectiveProjection3D prProjection;
      SetProjectionData( prProjection, pDrawPort);
      prProjection.ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
      prProjection.Prepare();

      // see if we should render only axis of main model
      {
        FLOATaabbox3D box;
        pMD->GetAllFramesBBox( box);
        CPlacement3D pl = plTranslated;
        FLOAT fSize = Clamp(box.Size().Length()/6.0f, 0.5f, 10.0f);
        RenderAxis( prProjection, pl, fSize);
      }
      if( m_atAxisType == AT_ALL) {
        // render recursivly all attachments and their attachments,...
        RenderAxisOfAllAttachments( prProjection, plTranslated, m_ModelObject);
        _armRenderModels.PopAll();
      }
      _iTextLine = 0;
    }
  }
  // mapping mode
  else
  {
 	  MEX mexWidth, mexHeight;
    // pick up model's texture dimensions
    mexWidth = m_ModelObject.GetWidth();
    mexHeight = m_ModelObject.GetHeight();
    PIX pixRight = (PIX) (mexWidth * m_MagnifyFactor);
    PIX pixDown = (PIX) (mexHeight * m_MagnifyFactor);

    ClearBcg( theApp.m_Preferences.ap_MappingWinBcgColor, pDrawPort);
  
    // set position of texture on screen
    PIXaabbox2D boxScreen( PIX2D(-m_offx, -m_offy),PIX2D(-m_offx + pixRight, -m_offy + pixDown));

    // set no valid texture mode
    CTextureData *pTD = NULL;
    CTextureData *pOldTD;
    pOldTD = (CTextureData *) m_ModelObject.mo_toTexture.GetData();
    // try to get model's exsiting texture
    if( AssureValidTDI())
    {
      pTD = m_ptdiTextureDataInfo->tdi_TextureData;
      if(pTD != m_ModelObject.mo_toTexture.GetData())
      {
        m_ModelObject.mo_toTexture.SetData( pTD);
      }
    }
    // if there are no textures in this model
    else
    {
      m_ModelObject.mo_toTexture.SetData( NULL);
    }

    if( (m_IsMappingBcgTexture) && (pTD != NULL) ) {
      if( !m_bTileMappingBCG)
      {
        MEXaabbox2D boxTexture(MEX2D(0,0), MEX2D(mexWidth-1, mexHeight-1));
        pDrawPort->PutTexture(&m_ModelObject.mo_toTexture, boxScreen, boxTexture);
      }
      else
      {
        PIX pixSizeI = pDrawPort->GetWidth();
        PIX pixSizeJ = pDrawPort->GetHeight();
        MEXaabbox2D boxTexture(MEX2D(0,0), MEX2D(mexWidth-1, mexHeight-1));
        PIX pixDI = boxScreen.Size()(1);
        MEX mexDU = boxTexture.Size()(1);
        FLOAT fMexOverPix = mexDU/FLOAT(pixDI);
        MEX mexU0 = MEX(-boxScreen.Min()(1)*fMexOverPix);
        MEX mexV0 = MEX(-boxScreen.Min()(2)*fMexOverPix);
        MEX mexU1 = MEX(mexU0+pixSizeI*fMexOverPix);
        MEX mexV1 = MEX(mexV0+pixSizeJ*fMexOverPix);

        pDrawPort->PutTexture( &m_ModelObject.mo_toTexture,
                               PIXaabbox2D(PIX2D(0,0), PIX2D( pixSizeI, pixSizeJ)),
                               MEXaabbox2D(MEX2D( mexU0, mexV0), MEX2D( mexU1, mexV1)));
      }
    } else {
      pDrawPort->Fill( boxScreen.Min() (1), boxScreen.Min() (2), 
                       boxScreen.Size()(1), boxScreen.Size()(2),
                       theApp.m_Preferences.ap_MappingPaperColor | CT_OPAQUE);
    }
    
    // render patches
    CModelData *pMD = &pDoc->m_emEditModel.edm_md;
    ModelMipInfo *pMMI = &pMD->md_MipInfos[ pDoc->m_iCurrentMip];

    // if pathes are not hidden for this mip model
    if( pMMI->mmpi_ulFlags & MM_PATCHES_VISIBLE)
    {
      INDEX iExistingPatch=0;
      // for each possible patch
      for( INDEX iMaskBit=0; iMaskBit<MAX_TEXTUREPATCHES; iMaskBit++)
      {
        CTextureData *ptdPatch = (CTextureData *) pMD->md_mpPatches[iMaskBit].mp_toTexture.GetData();
        // if current patch exists and is turned on
        if( (ptdPatch != NULL) &&
            (m_ModelObject.GetPatchesMask() & ((1UL) << iMaskBit)) )
        {
          MEX mexPatchU = pMD->md_mpPatches[iMaskBit].mp_mexPosition(1);
          MEX mexPatchV = pMD->md_mpPatches[iMaskBit].mp_mexPosition(2);
          PIX pixPatchUMin = (PIX) (mexPatchU * m_MagnifyFactor - m_offx);
          PIX pixPatchVMin = (PIX) (mexPatchV * m_MagnifyFactor - m_offy);
          PIX pixPatchUMax = (PIX) ((mexPatchU+ptdPatch->GetWidth()*pMD->md_mpPatches[iMaskBit].mp_fStretch) * m_MagnifyFactor - m_offx);
          PIX pixPatchVMax = (PIX) ((mexPatchV+ptdPatch->GetHeight()*pMD->md_mpPatches[iMaskBit].mp_fStretch) * m_MagnifyFactor - m_offy);
          PIXaabbox2D screenBox =  
            PIXaabbox2D( PIX2D(pixPatchUMin, pixPatchVMin),PIX2D(pixPatchUMax, pixPatchVMax));
          pDrawPort->PutTexture( &pMD->md_mpPatches[iMaskBit].mp_toTexture, screenBox);
        }
      }
    }

    // draw unselected surfaces
    ModelMipInfo &mmi = pDoc->m_emEditModel.edm_md.md_MipInfos[ pDoc->m_iCurrentMip];

    if( m_bRenderMappingInSurfaceColors)
    {
      for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
      {
        MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
        pDoc->m_emEditModel.DrawFilledSurface( pDrawPort, pDoc->m_iCurrentMip, iSurface, m_MagnifyFactor,
          m_offx, m_offy, ms.ms_colColor, ms.ms_colColor);
      }
    }                                                            
    
    if( m_ShowAllSurfaces)
    {
      for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
      {
        MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
        if( !(ms.ms_ulRenderingFlags&SRF_SELECTED))
          pDoc->m_emEditModel.DrawWireSurface( pDrawPort, pDoc->m_iCurrentMip,
            iSurface, m_MagnifyFactor, m_offx, m_offy,
            theApp.m_Preferences.ap_MappingInactiveSurfaceColor,
            theApp.m_Preferences.ap_MappingInactiveSurfaceColor);
      }
    }
    // draw selected surfaces
    for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
    {
      MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
      if( ms.ms_ulRenderingFlags&SRF_SELECTED)
        pDoc->m_emEditModel.DrawWireSurface( pDrawPort, pDoc->m_iCurrentMip, iSurface,
              m_MagnifyFactor, m_offx, m_offy,
              theApp.m_Preferences.ap_MappingActiveSurfaceColor, 
              C_RED);
    }
    // if we should print surface numbers
    if( m_bPrintSurfaceNumbers)
    {
      pDoc->m_emEditModel.PrintSurfaceNumbers( pDrawPort, theApp.m_pfntFont,
        pDoc->m_iCurrentMip, m_MagnifyFactor, m_offx, m_offy, C_BLACK);
    }
  }
}

void CModelerView::OnDraw(CDC* pDC)
{
  CModelerDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  ASSERT_VALID(pDoc);
  
  // ARGH!! I hate these two lines, very, very bad coding but can You do it better?
  if( !pDoc->m_bDocLoadedOk) return;

  // render view if drawport is valid
  if( m_pDrawPort!=NULL && m_pDrawPort->Lock()) {
    CTimerValue tvStart = _pTimer->GetHighPrecisionTimer();
    RenderView( m_pDrawPort);
    m_udViewPicture.MarkUpdated();
    CTimerValue tvStop = _pTimer->GetHighPrecisionTimer();
    TIME tmDelta = (tvStop-tvStart).GetSeconds() +tmSwapBuffers;
    // should we print frame rate?
    if( m_bFrameRate) {
      // prepare string about things that impact to currently rendered picture
      CTString strFPS, strReport;
      STAT_Report( strReport);
      STAT_Reset();
      // adjust and set font
      m_pDrawPort->SetFont( _pfdConsoleFont);
      m_pDrawPort->SetTextScaling( 1.0f);
      // put filter
      PIX pixDPHeight = m_pDrawPort->GetHeight();
      m_pDrawPort->Fill( 0,0, 150,pixDPHeight, C_BLACK|128, C_BLACK|0, C_BLACK|192, C_BLACK|0);
      // printout statistics
      strFPS.PrintF( " %3.0f FPS (%2.0f ms)\n----------------\n", 1.0f/tmDelta, tmDelta*1000.0f);
      m_pDrawPort->PutText( strFPS,    0,  5, C_lCYAN|CT_OPAQUE);
      m_pDrawPort->PutText( strReport, 4, 30, C_GREEN|CT_OPAQUE);
    }
    m_pDrawPort->Unlock();
    // swap if there is a valid viewport
    if( m_pViewPort!=NULL) {
      tvStart = _pTimer->GetHighPrecisionTimer();
      m_pViewPort->SwapBuffers();
      tvStop = _pTimer->GetHighPrecisionTimer();
      tmSwapBuffers = (tvStop-tvStart).GetSeconds();
    }
  }
  // no draw port ?
  else {
    // just fill window
    CRect rectFillArea;
    pDC->GetWindow()->GetClientRect(rectFillArea);
    pDC->FillSolidRect( rectFillArea, GetSysColor( COLOR_APPWORKSPACE));
  }

  // get active view 
  CModelerView *pActiveView = DYNAMIC_DOWNCAST(CModelerView, pMainFrame->GetActiveFrame()->GetActiveView());
  if( pActiveView == this) {
    INDEX iSurface = pDoc->GetOnlySelectedSurface();
    if( iSurface != -1) {
      // line of text
      char achrLine[ 256];
      achrLine[ 0] = 0;
      // prepare pane text line
      sprintf( achrLine, "%s", pDoc->m_emEditModel.GetSurfaceName( pDoc->m_iCurrentMip, iSurface));
      // print active surface
      pMainFrame->m_wndStatusBar.SetPaneText( ACTIVE_SURFACE_PANE, CString(achrLine));
    }
  }
  _pSound->UpdateSounds();
}

/////////////////////////////////////////////////////////////////////////////
// CModelerView printing

BOOL CModelerView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CModelerView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CModelerView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CModelerView diagnostics

#ifdef _DEBUG
void CModelerView::AssertValid() const
{
	CView::AssertValid();
}

void CModelerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CModelerDoc* CModelerView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CModelerDoc)));
	return (CModelerDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CModelerView message handlers

void CModelerView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	// at this time, m_hWnd is valid, so we do canvas initialization here
 	_pGfx->CreateWindowCanvas(m_hWnd, &m_pViewPort, &m_pDrawPort);
  
  CModelerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

  m_ModelObject.SetData( &pDoc->m_emEditModel.edm_md);
  m_ModelObject.SetAnim( 0);

  m_iActivePatchBitIndex = 0;
  
  pDoc->m_emEditModel.GetFirstValidPatchIndex( m_iActivePatchBitIndex);

  if( !pDoc->m_emEditModel.edm_WorkingSkins.IsEmpty())
  {
  	m_ptdiTextureDataInfo = LIST_HEAD( pDoc->m_emEditModel.edm_WorkingSkins,
                                    CTextureDataInfo, tdi_ListNode);
  }
  
  // set default viewer position
  ResetViewerPosition();

  pDoc->ClearAttachments();
  pDoc->SetupAttachments();
}

void CModelerView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	// if window canvas is valid, resize it
  if( m_pViewPort!=NULL) m_pViewPort->Resize();
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnDestroy() 
{
  CView::OnDestroy();
}
//--------------------------------------------------------------------------------------------
void CModelerView::OnMouseMove(UINT nFlags, CPoint point) 
{
  // line of text
  char achrLine[ 256];
  achrLine[ 0] = 0;

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->EnableSound();

  m_MousePosition = point;
  CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  CPoint offset = point - m_MouseDownLocation;
  FLOAT fDistance = -m_fTargetDistance;

  CPlacement3D plViewer;
  plViewer.pl_PositionVector = m_vTarget;
  plViewer.pl_OrientationAngle = m_angViewerOrientation;
  // moving offsets need small amounts
  FLOAT dx = 0.001f * offset.x * fDistance;
	FLOAT dy = 0.001f * offset.y * fDistance;
	FLOAT dz =  0.01f * offset.y * fDistance;
  // angles need lot for rotation
  ANGLE dAngleX = AngleDeg( -0.5f * offset.x);
  ANGLE dAngleY = AngleDeg( -0.5f * offset.y);
  /*
  ANGLE dAngleX = AngleDeg( 0.5f * offset.x * fDistance);
  ANGLE dAngleY = AngleDeg( 0.5f * offset.y * fDistance);
  */

  switch( m_InputAction)
  {
    case IA_MOVING_MEASURE_VERTEX:
    {
      CPlacement3D plMeasureVtx = CPlacement3D(m_vViewMeasureVertex, ANGLE3D(0,0,0));
      plMeasureVtx.AbsoluteToRelative( plViewer);
      plMeasureVtx.Translate_AbsoluteSystem( FLOAT3D( -dx, dy, 0.0f));
      plMeasureVtx.RelativeToAbsolute( plViewer);
      m_vViewMeasureVertex = plMeasureVtx.pl_PositionVector;
      //m_vTarget = m_vViewMeasureVertex;
      break;
    }
    case IA_MOVING_MODEL:
    {
      // project the placement to the viewer's system
      m_plModelPlacement.AbsoluteToRelative( plViewer);
      // translate it
      m_plModelPlacement.Translate_AbsoluteSystem( FLOAT3D( -dx, dy, 0.0f));
      // project the placement back from viewer's system
      m_plModelPlacement.RelativeToAbsolute( plViewer);
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_MOVING_VIEWER:
    {
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

      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_ZOOMING_MODEL:
    {
      CPlacement3D plNew = m_plModelPlacement;
      // project the placement to the viewer's system
      plNew.AbsoluteToRelative( plViewer);
      // translate it
      plNew.Translate_AbsoluteSystem( FLOAT3D( -dx, 0.0f, -dz));
      // project the placement back from viewer's system
      plNew.RelativeToAbsolute( plViewer);

      m_plModelPlacement = plNew;
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_ZOOMING_MEASURE_VERTEX:
    {
      CPlacement3D plMeasureVtx = CPlacement3D(m_vViewMeasureVertex, ANGLE3D(0,0,0));
      plMeasureVtx.AbsoluteToRelative( plViewer);
      plMeasureVtx.Translate_AbsoluteSystem( FLOAT3D( -dx, 0.0f, -dz));
      plMeasureVtx.RelativeToAbsolute( plViewer);
      m_vViewMeasureVertex = plMeasureVtx.pl_PositionVector;
      //m_vTarget = m_vViewMeasureVertex;
      break;
    }
    case IA_ZOOMING_VIEWER:
    {
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

      // update view
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_MIP_RANGING:
    {
      // can't come closer, only traveling away is allowed
      if( offset.y > 0)
      {
        break;
      }
      m_fTargetDistance += dz;
      // update view
      theApp.m_chGlobal.MarkChanged();
      break;
    }
    case IA_ZOOMING_LIGHT:
    {
      float fNewDistance = m_LightDistance - dz;
      if( fNewDistance > 0.2f)
      {
        m_LightDistance = fNewDistance;
      }
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_ROTATING_MODEL:
    {
      // project the placement to the viewer's system
      m_plModelPlacement.AbsoluteToRelative( plViewer);
      // rotate it
      m_plModelPlacement.Rotate_TrackBall( ANGLE3D( -dAngleX, -dAngleY, 0));
      // project the placement back from viewer's system
      m_plModelPlacement.RelativeToAbsolute( plViewer);
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_ROTATING_VIEWER:
    {
      // initialize viewer's orientation
      CPlacement3D plOrientation;
      plOrientation.pl_PositionVector = FLOAT3D( 0.0f, 0.0f, 0.0f);
      plOrientation.pl_OrientationAngle = m_angViewerOrientation;
      // project the placement to the viewer's system
      plOrientation.AbsoluteToRelative( plViewer);
      // rotate it
      plOrientation.Rotate_TrackBall( ANGLE3D( dAngleX, dAngleY, 0));
      // project the placement back from viewer's system
      plOrientation.RelativeToAbsolute( plViewer);
      // copy it back to viewer
      m_angViewerOrientation = plOrientation.pl_OrientationAngle;
      
      // update view
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_ROTATING_LIGHT:
    {
      // project the placement to the viewer's system
      m_plLightPlacement.AbsoluteToRelative( plViewer);
      // rotate it
      m_plLightPlacement.Rotate_TrackBall( ANGLE3D( -dAngleX/2, -dAngleY/2, 0));
      // project the placement back from viewer's system
      m_plLightPlacement.RelativeToAbsolute( plViewer);
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_MOVING_MAPPING:
    {
      PIX new_offx = m_offx - offset.x;
      PIX new_offy = m_offy - offset.y;
      m_offx = new_offx;
      m_offy = new_offy;
      theApp.m_chPlacement.MarkChanged();
      break;
    }
    case IA_MOVING_PATCH:
    {
      pDoc->m_emEditModel.MovePatchRelative( m_iActivePatchBitIndex,
        MEX2D( (MEX)(offset.x/m_MagnifyFactor), (MEX)(offset.y/m_MagnifyFactor)));
      pDoc->UpdateAllViews( NULL);
      pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
      pDoc->SetModifiedFlag();
      break;
    }
    case IA_ZOOMING_PATCH:
    {
      CModelPatch &mp = pDoc->m_emEditModel.edm_md.md_mpPatches[ m_iActivePatchBitIndex];
      FLOAT fNewStretch = mp.mp_fStretch + offset.y * m_MagnifyFactor/150.0f;
      pDoc->m_emEditModel.SetPatchStretch(m_iActivePatchBitIndex, fNewStretch);
      pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
      theApp.m_bRefreshPatchPalette = TRUE;
      pDoc->UpdateAllViews( NULL);
      pDoc->SetModifiedFlag();
      break;
    }
    case IA_ZOOMING_MAPPING:
    {
      float NewMagnifyFactor = m_MagnifyFactor + offset.y * m_MagnifyFactor/150.0f;
      if( (NewMagnifyFactor > 1/32768.0f) && (NewMagnifyFactor < 8.0f) )
      {
 	      MEX mexWidth, mexHeight;
        mexWidth = m_ModelObject.GetWidth();
        mexHeight = m_ModelObject.GetHeight();
        PIX pixOldWidth = (PIX) (mexWidth * m_MagnifyFactor);
        PIX pixOldHeight = (PIX) (mexHeight * m_MagnifyFactor);
        PIX pixNewWidth = (PIX) (mexWidth * NewMagnifyFactor);
        PIX pixNewHeight = (PIX) (mexHeight * NewMagnifyFactor);
        
        PIX pixDPWidth = m_pDrawPort->GetWidth();
        PIX pixDPHeight = m_pDrawPort->GetHeight();
        
        FLOAT fMagnifyDelta = NewMagnifyFactor/m_MagnifyFactor;
        PIX pixDCX = pixDPWidth/2 + m_offx;
        PIX pixDCY = pixDPHeight/2 + m_offy;
        m_offx = -(pixDPWidth/2 - pixDCX*fMagnifyDelta);
        m_offy = -(pixDPHeight/2 - pixDCY*fMagnifyDelta);
        m_MagnifyFactor = NewMagnifyFactor;
        theApp.m_chPlacement.MarkChanged();
      }
      break;
    }
    default:
    {
    }
  }

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

  m_MouseDownLocation = point;
}

void CModelerView::MagnifyMapping(CPoint point, FLOAT fMagnification)
{
  PIX pixDPWidth = m_pDrawPort->GetWidth();
  PIX pixDPHeight = m_pDrawPort->GetHeight();
  if( fMagnification > 1.0f)
  {
    // center clicked point
    m_offx += point.x - pixDPWidth/2;
    m_offy += point.y - pixDPHeight/2;
  }
    
  // correct offset for zoom
  PIX pixDCX = pixDPWidth/2 + m_offx;
  PIX pixDCY = pixDPHeight/2 + m_offy;
  m_offx = -(pixDPWidth/2 - pixDCX*fMagnification);
  m_offy = -(pixDPHeight/2 - pixDCY*fMagnification);

  m_MagnifyFactor *= fMagnification;
  theApp.m_chPlacement.MarkChanged();
  Invalidate( FALSE);

  if( fMagnification > 1.0f)
  {
    // center cursor
    CPoint ptCenter;
    ptCenter.x = pixDPWidth/2;
    ptCenter.y = pixDPHeight/2;
    ClientToScreen( &ptCenter);
    SetCursorPos(ptCenter.x, ptCenter.y);
  }
}

void CModelerView::FastZoomIn( CPoint point)
{
  if( m_bMappingMode)
  {
    MagnifyMapping(point, 2.0f);
  }
  else
  {
    OnMagnifyMore();
  }
}

void CModelerView::FastZoomOut()
{
  if( m_bMappingMode)
  {
    MagnifyMapping( CPoint(0,0), 0.5f);
  }
  else
  {
    OnMagnifyLess();
  }
}

//--------------------------------------------------------------------------------------------
void CModelerView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bLMB = nFlags & MK_LBUTTON;
  BOOL bRMB = nFlags & MK_RBUTTON;
  
  m_InputAction = IA_NONE;
  
  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace)
  {
    FastZoomIn( point);
    return;
  }

  // in mapping mode
  if( m_bMappingMode)
  {
    // LMB+space is used for mapping moving
    if( bSpace)
    {
      m_InputAction = IA_MOVING_MAPPING;
    }
    // Ctrl is used for moving patch
    else if( bCtrl)
    {
      m_InputAction = IA_MOVING_PATCH;
    }
  }
  // model view mode
  else
  {
    // shift operations control light
    if( bShift && bRMB)
    {
      m_InputAction = IA_ROTATING_LIGHT;
    }
    // alt colorizes surface
    else if( bAlt)
    {
      CPerspectiveProjection3D prProjection;
      SetProjectionData( prProjection, m_pDrawPort);
      // set position of document's model
      prProjection.ObjectPlacementL() = m_plModelPlacement;
      m_ModelObject.ColorizePolygon( m_pDrawPort, &prProjection, point.x, point.y,
                                     m_iChoosedColor, m_bOnColorMode);
      pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
      pDoc->SetModifiedFlag();
      Invalidate( FALSE);
    }
    // control operations are used for changing model placement
    else if( bCtrl)
    {
      if(m_bViewMeasureVertex)
      {
        m_InputAction = IA_MOVING_MEASURE_VERTEX;
      }
      else
      {
        m_InputAction = IA_MOVING_MODEL;
      }
    }
    // space operations are used for changing viewer placement
    else if( bSpace && bRMB )
    {
      m_InputAction = IA_ROTATING_VIEWER;
    }
    else if( bSpace)
    {
      m_InputAction = IA_MOVING_VIEWER;
    }
  }
  
  m_MouseDownLocation = point;
	CView::OnLButtonDown(nFlags, point);
}
//--------------------------------------------------------------------------------------------
void CModelerView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bLMB = nFlags & MK_LBUTTON;

  m_InputAction = IA_NONE;
  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace) {
    FastZoomOut();
    return;
  }

  // if mapping mode
  if( m_bMappingMode)
  { // RMB + space is used for zooming mapping
    if( bSpace) {
      m_InputAction = IA_ZOOMING_MAPPING;
    } else if( bCtrl) { // Ctrl is used for zooming patch
      m_InputAction = IA_ZOOMING_PATCH;
    } else {
      m_InputAction = IA_CONTEXT_MENU;
    }
  }
  // model view mode
  else
  { // ctrl+shift start mip ranging
    if( bCtrl && bShift)  {
      m_InputAction = IA_MIP_RANGING;
      // We will turn auto-mip modeling off but before that we have to set current auto mip 
      // model as activ one (because of possible mip-modeling off situation)
      m_ModelObject.AutoMipModelingOn();
      pDoc->SelectMipModel( m_ModelObject.GetMipModel( m_fCurrentMipFactor));
      m_ModelObject.AutoMipModelingOff();
      m_ModelObject.SetManualMipLevel( pDoc->m_iCurrentMip);

      // find center of all frames bbox
      FLOATaabbox3D MaxBB;
      m_ModelObject.GetAllFramesBBox( MaxBB);
      // look at the origin of model
      m_vTarget = m_plModelPlacement.pl_PositionVector;
      // move view up for half of model's max bbox
      m_vTarget(2) += MaxBB.Center()(2);
      // prepare projection to get viewer's placement
      CPerspectiveProjection3D prProjection;
      SetProjectionData( prProjection, m_pDrawPort);
      // update view
      Invalidate( FALSE);
    }
    // shift operations control light
    else if( bShift && bLMB) {
      m_InputAction = IA_ROTATING_LIGHT;
    }
    else if( bShift) {
      m_InputAction = IA_ZOOMING_LIGHT;
    }
    // space operaties with viewer position
    else if( bSpace && bLMB) {
      m_InputAction = IA_ROTATING_VIEWER;
    }
    else if( bSpace) {
      m_InputAction = IA_ZOOMING_VIEWER;
    }
    // ctrl operaties with model position
    else if( bCtrl && bLMB) {
      m_InputAction = IA_ROTATING_MODEL;
    }
    else if( bCtrl) {
      if(m_bViewMeasureVertex)
      {
        m_InputAction = IA_ZOOMING_MEASURE_VERTEX;
      }
      else
      {
        m_InputAction = IA_ZOOMING_MODEL;
      }
    }
    else {
      m_InputAction = IA_CONTEXT_MENU;
    }
  }

  m_MouseDownLocation = point;
	CView::OnRButtonDown(nFlags, point);
}
//--------------------------------------------------------------------------------------------
void CModelerView::OnLButtonUp(UINT nFlags, CPoint point) 
{
  CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  m_pmtvClosestVertex = NULL;
  m_InputAction = IA_NONE;
  CView::OnLButtonUp(nFlags, point);
}
//--------------------------------------------------------------------------------------------
void CModelerView::OnRButtonUp(UINT nFlags, CPoint point) 
{
  CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  
  if( m_InputAction == IA_MIP_RANGING)
  {
    pDoc->SetModifiedFlag();

    // prepare the projection
    CPerspectiveProjection3D prProjection;
    SetProjectionData( prProjection, m_pDrawPort);
    prProjection.Prepare();
    // set current mip factor as switch factor for current mip model
    m_ModelObject.SetMipSwitchFactor( pDoc->m_iCurrentMip, m_fCurrentMipFactor);
    CModelData *pMD = (CModelData *) m_ModelObject.GetData();
    // spread switch factors of rougher mip models proportionally up to maximum
    // default factor
    pMD->SpreadMipSwitchFactors( pDoc->m_iCurrentMip + 1, m_fCurrentMipFactor);
    m_ModelObject.AutoMipModelingOn();
    theApp.m_chGlobal.MarkChanged();
    // update info page
    CDlgInfoFrame *pInfoFrame = ((CMainFrame *)( theApp.m_pMainWnd))->m_pInfoFrame;
    if( pInfoFrame == NULL) return;
    CDlgInfoPgMip *pDlgMip = &pInfoFrame->m_pInfoSheet->m_PgInfoMip;
    if( !::IsWindow(pDlgMip->m_hWnd)) return; 
    pDlgMip->UpdateData( FALSE);
  }
	
	CView::OnRButtonUp(nFlags, point);
}
//--------------------------------------------------------------------------------------------
// restart animation for all attachments recursivly
void PlayAnimForAllAttachments( CModelObject &mo, ULONG ulFlags)
{
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo.mo_lhAttachments, itamo)
  {
    PlayAnimForAllAttachments( itamo->amo_moModelObject, ulFlags);
    mo.ResetAnim();
    mo.PlayAnim( mo.GetAnim(), ulFlags);
  }
}
void PauseAnimForAllAttachments( CModelObject &mo)
{
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo.mo_lhAttachments, itamo)
  {
    PauseAnimForAllAttachments( itamo->amo_moModelObject);
    mo.PauseAnim();
  }
}

void CModelerView::OnAnimNextAnim() 
{
	m_ModelObject.NextAnim();
  if( m_ModelObject.IsPaused())
  {
	  m_ModelObject.ao_tmAnimStart = 0;
  }
}

void CModelerView::OnAnimPrevAnim() 
{
	m_ModelObject.PrevAnim();
  if( m_ModelObject.IsPaused())
  {
	  m_ModelObject.ao_tmAnimStart = 0;
  }
}

void CModelerView::OnAnimPlay() 
{
  if( m_ModelObject.IsPaused())
  {
    m_ModelObject.PlayAnim( m_ModelObject.GetAnim(), AOF_LOOPING|AOF_NORESTART);  
    PlayAnimForAllAttachments( m_ModelObject, AOF_LOOPING|AOF_NORESTART);    
  }
  else
  {
    m_ModelObject.PauseAnim();
    PauseAnimForAllAttachments( m_ModelObject);    
  }
}

void CModelerView::OnFirstFrame() 
{
  if( !m_ModelObject.IsPaused())
  {
    m_ModelObject.PauseAnim();
  }
  m_ModelObject.FirstFrame();
}

void CModelerView::OnLastFrame() 
{
  if( !m_ModelObject.IsPaused())
  {
    m_ModelObject.PauseAnim();
  }
  m_ModelObject.LastFrame();
}

void CModelerView::OnAnimNextFrame() 
{
  if( !m_ModelObject.IsPaused())
  {
    m_ModelObject.PauseAnim();
  }
	m_ModelObject.NextFrame();	
}

void CModelerView::OnAnimPrevFrame() 
{
  if( !m_ModelObject.IsPaused())
  {
    m_ModelObject.PauseAnim();
  }
	m_ModelObject.PrevFrame();	
}

void CModelerView::OnUpdateAnimPlay(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode)
    pCmdUI->Enable( FALSE);
  else if (m_ModelObject.IsPaused()) {
    pCmdUI->SetCheck(0);
  } else {
    pCmdUI->SetCheck(1);
  }
}

void CModelerView::OnRestartAnimations() 
{
  // get current animation index
  INDEX iMdlCurrentAnim = m_ModelObject.GetAnim();
  // restart model's animation
  m_ModelObject.StartAnim( iMdlCurrentAnim);
  // retrieve current model's texture 
  CTextureData *pTD = (CTextureData *) m_ModelObject.mo_toTexture.GetData();
	// if model has texture
  if( pTD != NULL)
  {
    // get model's texture animation index
    INDEX iTexCurrentAnim = m_ModelObject.mo_toTexture.GetAnim();
    // restart model's texture's animation
    m_ModelObject.mo_toTexture.StartAnim( iTexCurrentAnim);
  }
}

void CModelerView::OnUpdateAnimNextframe(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode)
    pCmdUI->Enable( FALSE);
  else 
    pCmdUI->Enable(TRUE);
}

void CModelerView::OnUpdateAnimPrevframe(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode)
    pCmdUI->Enable( FALSE);
  else
    pCmdUI->Enable(TRUE);
}

void CModelerView::OnIdle(void)
{
  if( m_AutoRotating)
  {
    TIME timeNow = _pTimer->GetRealTimeTick();
    TIME tmDelta = timeNow-m_timeLastTick;
    m_plModelPlacement.pl_OrientationAngle( 1) -= AngleDeg(160.0f*tmDelta);
    theApp.m_chPlacement.MarkChanged();
    m_timeLastTick = timeNow;
  }

  FLOAT fTimeVar1 = ((FLOAT)_pTimer->GetRealTimeTick()) / 1.5f;
  FLOAT fTimeVar2 = ((FLOAT)_pTimer->GetRealTimeTick()) * 0.8f;
  FLOAT fTimeVar3 = ((FLOAT)_pTimer->GetRealTimeTick()) * 1.8f;

  if( m_bDollyViewer)
  {
    m_fTargetDistance += (FLOAT)sin( fTimeVar1)/10;
    m_angViewerOrientation(1) += AngleDeg( 10.0f);
    m_angViewerOrientation(2) = AngleDeg( 
      DegAngle(m_angViewerOrientation(2)) + (FLOAT)sin(fTimeVar1)/2.0f);
    theApp.m_chPlacement.MarkChanged();
  }

  if( m_bDollyMipModeling)
  {
    if( m_fTargetDistance >= 100.0f)
    {
      m_fDollySpeedMipModeling = -2.0f;
    }
    else if( m_fTargetDistance <= 3.0f)
    {
      m_fDollySpeedMipModeling = 2.0f;
    }
    FLOAT fCorrector = 0.0f;
    if( m_fTargetDistance<20.0f)
    {
      FLOAT fAbsDistance = Abs(m_fTargetDistance);
      fCorrector = (22.5f-fAbsDistance)/20.0f;
    }
    m_fTargetDistance += m_fDollySpeedMipModeling-m_fDollySpeedMipModeling*fCorrector;
    theApp.m_chPlacement.MarkChanged();
  }
  
  if( m_bDollyLight)
  {
    m_plLightPlacement.pl_OrientationAngle(1) += AngleDeg( 6.0f);
    m_plLightPlacement.pl_OrientationAngle(2) = 
      AngleDeg( DegAngle(m_plLightPlacement.pl_OrientationAngle(2)) + (FLOAT)sin(fTimeVar3)*4.0f);
    theApp.m_chPlacement.MarkChanged();
  }

  if( m_bDollyLightColor)
  {
    UBYTE ubRed = 128+((BYTE)(sin( fTimeVar1)*127));
    UBYTE ubGreen = 128+((BYTE)(sin( fTimeVar2)*127));
    UBYTE ubBlue = 128+((BYTE)(sin( fTimeVar3)*127));
    m_LightColor = ((ULONG)ubRed)<<24 | ((ULONG)ubGreen)<<16 | ((ULONG)ubBlue)<<8;
    theApp.m_chPlacement.MarkChanged();
  }

  BOOL bUpdate = !( m_ModelObject.IsPaused() &&
                    m_ModelObject.IsUpToDate(m_udViewPicture) &&
                    theApp.m_chGlobal.IsUpToDate(m_udViewPicture) &&
                    theApp.m_chPlacement.IsUpToDate(m_udViewPicture) &&
                    (m_ModelObject.GetPatchesMask() == 0) );
  CTextureData *pTD = (CTextureData *) m_ModelObject.mo_toTexture.GetData();
  if( (pTD != NULL) && (pTD->td_ctFrames > 1) ) bUpdate = TRUE;
  
  if( bUpdate)
  {
    Invalidate( FALSE);
  }
}

void CModelerView::OnAnimChoose()
{                        
 	CDChooseAnim dlg( &m_ModelObject);
  dlg.DoModal();
  Invalidate( FALSE);
}

void CModelerView::OnAnimMipPrecize() 
{
  CModelerDoc* pDoc = GetDocument();
  m_ModelObject.PrevManualMipLevel();
  
  pDoc->SelectMipModel( m_ModelObject.GetMipModel( m_fCurrentMipFactor));
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnAnimMipRough() 
{
  CModelerDoc* pDoc = GetDocument();
	m_ModelObject.NextManualMipLevel();
  pDoc->SelectMipModel( m_ModelObject.GetMipModel( m_fCurrentMipFactor));
  //pDoc->SelectMipModel( pDoc->m_iCurrentMip);
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnUpdateAnimMipPrecize(CCmdUI* pCmdUI) 
{
  if( m_ModelObject.IsAutoMipModeling())
    pCmdUI->Enable(FALSE);
  else
    pCmdUI->Enable(TRUE);
}

void CModelerView::OnUpdateAnimMipRough(CCmdUI* pCmdUI) 
{
  if( m_ModelObject.IsAutoMipModeling())
    pCmdUI->Enable(FALSE);
  else
    pCmdUI->Enable(TRUE);
}

void CModelerView::OnOptAutoMipModeling() 
{
  if( m_ModelObject.IsAutoMipModeling())
    m_ModelObject.AutoMipModelingOff();
  else
    m_ModelObject.AutoMipModelingOn();
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnUpdateOptAutoMipModeling(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode)    pCmdUI->Enable( FALSE);
  else                pCmdUI->Enable( TRUE);
  pCmdUI->SetCheck( m_ModelObject.IsAutoMipModeling());
}

void CModelerView::OnAnimRotation() 
{
  m_timeLastTick = _pTimer->GetRealTimeTick();
  m_AutoRotating = !m_AutoRotating;
}

void CModelerView::OnUpdateAnimRotation(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode)
    pCmdUI->Enable( FALSE);
	pCmdUI->SetCheck( m_AutoRotating);
}

CModelerView *CModelerView::GetActiveView(void)
{
  CModelerView *res;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  res = DYNAMIC_DOWNCAST(CModelerView, pMainFrame->GetActiveFrame()->GetActiveView());
  if( (res != NULL) && (res->m_bMappingMode) )
    return NULL;
  return res;
}

CModelerView *CModelerView::GetActiveMappingView(void)
{
  CModelerView *res;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  res = DYNAMIC_DOWNCAST(CModelerView, pMainFrame->GetActiveFrame()->GetActiveView());
  if( (res != NULL) && (res->m_bMappingMode) )
    return res;
  return NULL;
}

CModelerView *CModelerView::GetActiveMappingNormalView(void)
{
  CModelerView *pModelerView = GetActiveView();
  if( pModelerView == NULL)
    return GetActiveMappingView();
  return pModelerView;
}

void CModelerView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
  CMenu menu;
	
  if( m_bMappingMode)
  {
    if( (m_InputAction == IA_CONTEXT_MENU) && (menu.LoadMenu(IDR_MAPPING_VIEW_POPUP)))
    {
		  CMenu* pPopup = menu.GetSubMenu(0);
		  ASSERT(pPopup != NULL);
      pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								   point.x, point.y, this);
    }
  }
  else
  {
    if( (m_InputAction == IA_CONTEXT_MENU) && (menu.LoadMenu(IDR_MODEL_VIEW_POPUP)))
  	{
		  CMenu* pPopup = menu.GetSubMenu(0);
		  ASSERT(pPopup != NULL);
		  if( m_RenderPrefs.BBoxFrameVisible())
        pPopup->CheckMenuItem(ID_REND_BBOX_FRAME, MF_CHECKED);
		  if( m_RenderPrefs.BBoxAllVisible())
        pPopup->CheckMenuItem(ID_REND_BBOX_ALL, MF_CHECKED);

      if( m_RenderPrefs.WireOn()) pPopup->CheckMenuItem(ID_REND_WIRE_ONOFF, MF_CHECKED);
      if( m_RenderPrefs.HiddenLines()) pPopup->CheckMenuItem(ID_REND_HIDDEN_LINES, MF_CHECKED);
    
      ULONG rtRenderType = m_RenderPrefs.GetRenderType();
      if( (rtRenderType & RT_NO_POLYGON_FILL) != 0)
        pPopup->CheckMenuItem( ID_REND_NO_TEXTURE, MF_CHECKED);
      if( (rtRenderType & RT_WHITE_TEXTURE) != 0)
        pPopup->CheckMenuItem( ID_REND_WHITE_TEXTURE, MF_CHECKED);
      if( (rtRenderType & RT_SURFACE_COLORS) != 0)
        pPopup->CheckMenuItem( ID_REND_SURFACE_COLORS, MF_CHECKED);
      if( (rtRenderType & RT_ON_COLORS) != 0)
        pPopup->CheckMenuItem( ID_REND_ON_COLORS, MF_CHECKED);
      if( (rtRenderType & RT_OFF_COLORS) != 0)
        pPopup->CheckMenuItem( ID_REND_OFF_COLORS, MF_CHECKED);
      if( (rtRenderType & RT_TEXTURE) != 0)
        pPopup->CheckMenuItem( ID_REND_USE_TEXTURE, MF_CHECKED);
    
      if( (rtRenderType & RT_SHADING_NONE) != 0)
        pPopup->CheckMenuItem( ID_REND_SHADING_NONE, MF_CHECKED);
      if( (rtRenderType & RT_SHADING_LAMBERT) != 0)
        pPopup->CheckMenuItem( ID_REND_SHADING_LAMBERT, MF_CHECKED);
      if( (rtRenderType & RT_SHADING_PHONG) != 0)
        pPopup->CheckMenuItem( ID_REND_PHONG, MF_CHECKED);

      pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								   point.x, point.y, this);
	  }
  }
  m_InputAction = IA_NONE;	
}

BOOL MyChooseColor( COLORREF &clrNewColor, CWnd &wndOwner)
{
	COLORREF MyCustColors[ 16];
  CHOOSECOLOR ccInit;

  ASSERT( &wndOwner != NULL);
  MyCustColors[ 0] = CLRF_CLR(C_BLACK);
  MyCustColors[ 1] = CLRF_CLR(C_WHITE);
  MyCustColors[ 2] = CLRF_CLR(C_dGRAY);
  MyCustColors[ 3] = CLRF_CLR(C_GRAY);
  MyCustColors[ 4] = CLRF_CLR(C_lGRAY);
  MyCustColors[ 5] = CLRF_CLR(C_dRED); 
  MyCustColors[ 6] = CLRF_CLR(C_dGREEN);
  MyCustColors[ 7] = CLRF_CLR(C_dBLUE);
  MyCustColors[ 8] = CLRF_CLR(C_dCYAN);
  MyCustColors[ 9] = CLRF_CLR(C_dMAGENTA);
  MyCustColors[10] = CLRF_CLR(C_dYELLOW);
  MyCustColors[11] = CLRF_CLR(C_dORANGE);
  MyCustColors[12] = CLRF_CLR(C_dBROWN);
  MyCustColors[13] = CLRF_CLR(C_dPINK);
  MyCustColors[14] = CLRF_CLR(C_lORANGE);
  MyCustColors[15] = CLRF_CLR(C_lBROWN);

  memset(&ccInit, 0, sizeof(CHOOSECOLOR));
  
  ccInit.lStructSize = sizeof(CHOOSECOLOR);
  ccInit.Flags = CC_RGBINIT | CC_FULLOPEN;
  ccInit.rgbResult = clrNewColor;
  ccInit.lpCustColors = &MyCustColors[ 0];
  ccInit.hwndOwner = wndOwner.m_hWnd;
  
  if( !ChooseColor( &ccInit))
    return FALSE;
  
  clrNewColor = ccInit.rgbResult;
  return TRUE;
}

void CModelerView::OnPrefsChangeInk() 
{
  if( MyChooseColor( m_InkColor, *GetParent()))
  {
    Invalidate( FALSE);
  }
}

void CModelerView::OnPrefsChangePaper() 
{
  if( MyChooseColor( m_PaperColor, *GetParent()))
  {
    Invalidate( FALSE);
  }
}

void CModelerView::OnFileRemoveTexture() 
{
  // remove curently selected skin texture
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  ASSERT( pDoc->m_emEditModel.edm_WorkingSkins.Count() != 0);
  CTextureDataInfo *ptdiSelected = NULL;
  INDEX iCurSel = pMainFrame->m_SkinComboBox.GetCurSel();
  INDEX iIter = 0;
  FOREACHINLIST( CTextureDataInfo, tdi_ListNode, pDoc->m_emEditModel.edm_WorkingSkins, it)
  {
    if( iCurSel == iIter)
    {
      ptdiSelected = &it.Current();
      break;
    }
    iIter++;
  }
  _pTextureStock->Release( ptdiSelected->tdi_TextureData);
  ptdiSelected->tdi_ListNode.Remove();
  delete ptdiSelected;
  Invalidate( FALSE);
}

void CModelerView::OnScriptOpen() 
{
	CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  CTFileName fnDocName = CTString(CStringA(pDoc->GetPathName()));
  AfxGetApp()->OpenDocumentFile( CString(fnDocName.FileDir() + fnDocName.FileName() + ".scr"));
}

void CModelerView::OnScriptUpdateAnimations() 
{
  UpdateAnimations();
}

BOOL CModelerView::UpdateAnimations(void) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  CTFileName fnModelName = CTString(CStringA(pDoc->GetPathName()));
  CTFileName fnScriptName = fnModelName.FileDir() + fnModelName.FileName() + ".scr";
	
  pDoc->OnSaveDocument( pDoc->GetPathName());

  try
  {
    fnScriptName.RemoveApplicationPath_t();
    pMainFrame->m_NewProgress.Create( IDD_NEW_PROGRESS, pMainFrame);
    pMainFrame->m_NewProgress.CenterWindow();
    pMainFrame->m_NewProgress.ShowWindow(SW_SHOW);
    pDoc->m_emEditModel.UpdateAnimations_t( fnScriptName);
  }
  catch( char *str_err)
  {
    pMainFrame->m_NewProgress.DestroyWindow();
    pDoc->OnCloseDocument();       // explicit delete on error
    AfxMessageBox( CString(str_err));
    return FALSE;
  }
  pMainFrame->m_NewProgress.DestroyWindow();
  pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
  pDoc->SetModifiedFlag();
  pMainFrame->m_AnimComboBox.m_pvLastUpdatedView = NULL;
  theApp.m_chGlobal.MarkChanged();
  return TRUE;
}

void CModelerView::OnScriptUpdateMipmodels() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  CTFileName fnModelName = CTString(CStringA(pDoc->GetPathName()));
  CTFileName fnScriptName = fnModelName.FileDir() + fnModelName.FileName() + ".scr";
	
  if( ::MessageBoxA( this->m_hWnd, "Updating mip-models will discard current mip-model mapping "
                                  "and colorizing data. Are you sure that you want that?",
             "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1|
             MB_SYSTEMMODAL | MB_TOPMOST) == IDYES)
  {
    try
    {
      fnScriptName.RemoveApplicationPath_t();
      pMainFrame->m_NewProgress.Create( IDD_NEW_PROGRESS, pMainFrame);
      pMainFrame->m_NewProgress.CenterWindow();
      pMainFrame->m_NewProgress.ShowWindow(SW_SHOW);
      pDoc->m_emEditModel.UpdateMipModels_t( fnScriptName);
    }
    catch( char *str_err)
    {
      AfxMessageBox( CString(str_err));
    }
  }
  pMainFrame->m_NewProgress.DestroyWindow();
  pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
  pDoc->SetModifiedFlag();
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnLightOn() 
{
	if( !m_bMappingMode)
  {
    m_LightModeOn = !m_LightModeOn;
    Invalidate( FALSE);
  }
}

void CModelerView::OnLightColor() 
{
  COLORREF TmpColor = CLRF_CLR( m_LightColor);
  if( MyChooseColor( TmpColor, *GetParent()))
  {
    m_LightColor = CLR_CLRF( TmpColor);
    Invalidate( FALSE);
  }
}

void CModelerView::OnUpdateLightOn(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( m_LightModeOn);
}

void CModelerView::OnUpdateLightColor(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode)
    pCmdUI->Enable( FALSE);
}

void CModelerView::OnRendBboxAll() 
{
	m_RenderPrefs.BBoxAllShow( !m_RenderPrefs.BBoxAllVisible());
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendBboxFrame() 
{
	m_RenderPrefs.BBoxFrameShow( !m_RenderPrefs.BBoxFrameVisible());
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendWireOnoff() 
{
  m_RenderPrefs.SetWire( !m_RenderPrefs.WireOn());
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendHiddenLines() 
{
  m_RenderPrefs.SetHiddenLines( !m_RenderPrefs.HiddenLines());
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendNoTexture() 
{
  m_RenderPrefs.SetTextureType( RT_NO_POLYGON_FILL);
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendOnColors() 
{
	m_RenderPrefs.SetTextureType( RT_ON_COLORS);
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendOffColors() 
{
	m_RenderPrefs.SetTextureType( RT_OFF_COLORS);
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendSurfaceColors() 
{
	m_RenderPrefs.SetTextureType( RT_SURFACE_COLORS);
  GetDocument()->UpdateAllViews( NULL);
}

void CModelerView::OnRendWhiteTexture() 
{
	m_RenderPrefs.SetTextureType( RT_WHITE_TEXTURE);
  GetDocument()->UpdateAllViews( NULL);	
}

void CModelerView::OnRendUseTexture() 
{
  m_RenderPrefs.SetTextureType( RT_TEXTURE);
  GetDocument()->UpdateAllViews( NULL);	
}

void CModelerView::OnMappingOn() 
{
  // if we are going into mapping mode
  if( !m_bMappingMode)
  {
    // remember auto mip modeling flag
    m_bAutoMipModelingBeforeMapping = m_ModelObject.IsAutoMipModeling();
    // turn off auto mip modeling
    m_ModelObject.AutoMipModelingOff();
  }
  else
  {
    // restore auto mip modeling flag
    if( m_bAutoMipModelingBeforeMapping)    m_ModelObject.AutoMipModelingOn();
    else                                    m_ModelObject.AutoMipModelingOff();
  }

	m_bMappingMode = !m_bMappingMode;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateMappingOn(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bMappingMode);
}

void CModelerView::OnUpdateScriptOpen(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateScriptUpdateAnimations(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateScriptUpdateMipmodels(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateAnimNextanim(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);	
}

void CModelerView::OnUpdateAnimPrevanim(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateAnimChoose(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateFileRemoveTexture(CCmdUI* pCmdUI)
{
	CModelerDoc *pDoc = (CModelerDoc *) GetDocument();
  INDEX ctSkins = pDoc->m_emEditModel.edm_WorkingSkins.Count();
  pCmdUI->Enable( !m_bMappingMode && (ctSkins!=0) );
}

void CModelerView::OnMagnifyLess() 
{
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
	if( m_bMappingMode)
  {
    if( m_MagnifyFactor > 1.0/16384.0)
    {
      m_MagnifyFactor /= 2.0f;
      if( bAlt)
      {
        OnWindowFit();
      }
    }
  }	
  else if( m_fTargetDistance < 65535.0f)
  {
    m_fTargetDistance *= 2.0f;
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnMagnifyMore() 
{
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  if( m_bMappingMode)
  {
    if(m_MagnifyFactor < 4)
    {
      m_MagnifyFactor *= 2.0f;
      if( bAlt)
      {
        OnWindowFit();
      }
    }
  }
  else if( m_fTargetDistance > 1.0f)
  {
    m_fTargetDistance /= 2.0f;
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnWindowFit() 
{
  if( m_bMappingMode && !GetParent()->IsZoomed())
  {
 	  MEX mexWidth, mexHeight;
    mexWidth = m_ModelObject.GetWidth();
    mexHeight = m_ModelObject.GetHeight();
    PIX pixLeft = -m_offx;
    PIX pixUp = -m_offy;
    PIX pixWidth = (PIX) (mexWidth * m_MagnifyFactor);
    PIX pixHeight = (PIX) (mexHeight * m_MagnifyFactor);
  
    CRect rectClient;
    CRect rectNewPos;
  
    CWnd *pWnd = GetParent();                       // view's window
    pWnd->GetParent()->GetClientRect( rectClient);  // size of whole working area window
    pWnd->GetWindowRect( rectNewPos);               // size of view's window
    pWnd->GetParent()->ScreenToClient( rectNewPos); // view's window size relative to working area
  
    rectNewPos.left += pixLeft;
    if( rectNewPos.left < 0)
      rectNewPos.left = 0;
    m_offx = 0;
    rectNewPos.top += pixUp;
    if( rectNewPos.top < 0)
      rectNewPos.top = 0;
    m_offy = 0;
    rectNewPos.right = rectNewPos.left + pixWidth + 12;
    if( rectNewPos.right > rectClient.right)
      rectNewPos.right = rectClient.right;
    rectNewPos.bottom = rectNewPos.top + pixHeight + 31;
    if( rectNewPos.bottom > rectClient.bottom)
      rectNewPos.bottom = rectClient.bottom;
    pWnd->MoveWindow( rectNewPos, TRUE);
    theApp.m_chGlobal.MarkChanged();
  }
}

void CModelerView::OnWindowCenter() 
{
  if( m_bMappingMode)
  {
 	  MEX mexWidth, mexHeight;
    mexWidth = m_ModelObject.GetWidth();
    mexHeight = m_ModelObject.GetHeight();
    PIX pixRight = (PIX) (mexWidth * m_MagnifyFactor);
    PIX pixDown = (PIX) (mexHeight * m_MagnifyFactor);
	  
    PIX WinW, WinH;
    WinW = m_pDrawPort->GetWidth();
    WinH = m_pDrawPort->GetHeight();

    m_offx = -( WinW - pixRight)/2;
    m_offy = -( WinH - pixDown)/2;

    theApp.m_chGlobal.MarkChanged();
  }
  else
  {
    OnCollisionBox();
  }
}

void CModelerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CModelerView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  if( m_InputAction != IA_MIP_RANGING)
  {
    m_InputAction = IA_NONE;
  }
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CModelerView::OnListAnimations() 
{
 	CDChooseAnim dlg( &m_ModelObject);
  dlg.DoModal();
  Invalidate(FALSE);
}

void CModelerView::OnUpdateListAnimations(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateSkinTexture(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_bMappingMode);
}

void CModelerView::OnSkinTexture() 
{
  m_IsMappingBcgTexture = !m_IsMappingBcgTexture;
}

void CModelerView::OnBackgroundTexture() 
{
  m_IsWinBcgTexture = !m_IsWinBcgTexture;
  Invalidate(FALSE);
}

void CModelerView::OnUpdateRendHiddenLines(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( m_RenderPrefs.HiddenLines());
}

void CModelerView::OnUpdateRendWireOnoff(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( m_RenderPrefs.WireOn());
}

void CModelerView::OnUpdateRendNoTexture(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( (m_RenderPrefs.GetRenderType() & RT_NO_POLYGON_FILL) != 0);
}

void CModelerView::OnUpdateRendWhiteTexture(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( (m_RenderPrefs.GetRenderType() & RT_WHITE_TEXTURE) != 0);
}

void CModelerView::OnUpdateRendSurfaceColors(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( (m_RenderPrefs.GetRenderType() & RT_SURFACE_COLORS) != 0);
}

void CModelerView::OnUpdateRendOnColors(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( (m_RenderPrefs.GetRenderType() & RT_ON_COLORS) != 0);
}

void CModelerView::OnUpdateRendOffColors(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( (m_RenderPrefs.GetRenderType() & RT_OFF_COLORS) != 0);
}

void CModelerView::OnUpdateRendUseTexture(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( (m_RenderPrefs.GetRenderType() & RT_TEXTURE) != 0);
}

void CModelerView::OnUpdateRendBboxAll(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( m_RenderPrefs.BBoxAllVisible() );
}

void CModelerView::OnUpdateRendBboxFrame(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( m_RenderPrefs.BBoxFrameVisible() );
}

void CModelerView::OnTakeScreenShoot() 
{
  // redraw view
  if( m_pDrawPort->Lock())
  {
    RenderView( m_pDrawPort);
    m_pDrawPort->Unlock();
  }
  m_pViewPort->SwapBuffers();

  // grab screen creating image info
  CImageInfo iiImageInfo;
  m_pDrawPort->GrabScreen( iiImageInfo, 1);

  CTFileName fnSSFileName = _EngineGUI.FileRequester( "Select name for screen shot",
                            FILTER_TGA FILTER_END, "Take screen shoots directory",
                            "ScreenShots\\", "", NULL, FALSE);
  if( fnSSFileName == "") return;

  CWaitCursor StartWaitCursor;
  CDlgPleaseWait dlg( "Please wait while saving screen shoot:", fnSSFileName);
  dlg.Create( IDD_PLEASE_WAIT);
  dlg.CenterWindow();
  dlg.RedrawWindow();
  // try to
  try {
    // save image info into file
    iiImageInfo.SaveTGA_t( fnSSFileName);
  }
  catch(char *strError) {
    AfxMessageBox(CString(strError));
  }
  dlg.DestroyWindow();
}

/*
 * Get the pointer to the main frame object of this application
 */
CMainFrame *CModelerView::GetMainFrame()
{
	// get the MDIChildFrame of this window
	CChildFrame *pfrChild = (CChildFrame *)this->GetParentFrame();
  ASSERT(pfrChild!=NULL);
  // get the MDIFrameWnd
  CMainFrame *pfrMain = (CMainFrame *)pfrChild->GetParentFrame();
  ASSERT(pfrMain!=NULL);

	return pfrMain;
}

void CModelerView::OnKillFocus(CWnd* pNewWnd) 
{
	CView::OnKillFocus(pNewWnd);
}

void CModelerView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
  theApp.m_chGlobal.MarkChanged();
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CModelerView::OnBackgPicture() 
{
	m_IsWinBcgTexture = TRUE;	
  Invalidate( FALSE);
}

void CModelerView::OnBackgColor() 
{
	m_IsWinBcgTexture = FALSE;	
  Invalidate( FALSE);
}

void CModelerView::OnRendFloor() 
{
  m_FloorOn = !m_FloorOn;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateRendFloor(CCmdUI* pCmdUI) 
{
  if( !m_bMappingMode)
  {
    if( theApp.m_pFloorModelObject != NULL)
    {
      // set floor mode check (press in button)
      pCmdUI->SetCheck( m_FloorOn);
      // enable tool bar's button
      pCmdUI->Enable( TRUE);
      // don't disable it again
      return;
    }
  }
  // gray collision mode command
  pCmdUI->Enable( FALSE);
}

void CModelerView::OnStainsInsert() 
{
  CModelerDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  
  CWorkingPatch *pWP = NULL;
  int iSelected = pMainFrame->m_StainsComboBox.GetCurSel();

  if( iSelected != CB_ERR)
  {
    INDEX iCt = 0;
    FOREACHINLIST( CWorkingPatch, wp_ListNode, theApp.m_WorkingPatches, it)
    {
      if( iCt == iSelected)
      {
        pWP = &it.Current();
      }
      iCt ++;
    }
  }
  ASSERT(pWP != NULL);

  CTextureData *pTD = pWP->wp_TextureData;
  CModelData *pMD = (CModelData *) m_ModelObject.GetData();
  MEX mexWidth, mexHeight;
  pMD->GetTextureDimensions( mexWidth, mexHeight);
  INDEX iMaskBit;
  // if patch adding is not succesefull
  if( !pDoc->m_emEditModel.EditAddPatch( pWP->wp_FileName,
                      MEX2D( mexWidth/2-pTD->GetWidth()/2, mexHeight/2-pTD->GetHeight()/2),
                      iMaskBit) )
  {
    WarningMessage("Unable to add patch");
    return;
  }
  m_iActivePatchBitIndex = iMaskBit;
  m_ModelObject.ShowPatch( iMaskBit);
  pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
  pDoc->SetModifiedFlag();
  Invalidate( FALSE);

  if( pMainFrame->m_dlgPatchesPalette != NULL)
  {
    pMainFrame->m_dlgPatchesPalette->UpdateData( FALSE);
    pMainFrame->m_dlgPatchesPalette->Invalidate(FALSE);
  }
}

void CModelerView::OnStainsDelete() 
{
  CModelerDoc* pDoc = GetDocument();
  
  POSITION pos = pDoc->GetFirstViewPosition();
  while (pos != NULL)
  {
    CView *pView = pDoc->GetNextView( pos);
    if( DYNAMIC_DOWNCAST(CModelerView, pView) != NULL)
    {
      CModelerView* pModelerView = (CModelerView *) pView;
      pModelerView->m_ModelObject.HidePatch( m_iActivePatchBitIndex);
    }
  }   
  pDoc->m_emEditModel.EditRemovePatch( m_iActivePatchBitIndex);
  INDEX iRemovedPatch = m_iActivePatchBitIndex;
  pDoc->m_emEditModel.GetFirstValidPatchIndex( m_iActivePatchBitIndex);
  pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
  pDoc->SetModifiedFlag();

  // for all views in this document,
  // select first patch that exists if deleted patch has been selected
  pos = pDoc->GetFirstViewPosition();
  while (pos != NULL)
  {
    CView *pView = pDoc->GetNextView( pos);
    if( DYNAMIC_DOWNCAST(CModelerView, pView) != NULL)
    {
      CModelerView* pModelerView = (CModelerView *) pView;
      if( pModelerView->m_iActivePatchBitIndex == iRemovedPatch)
      {
        pModelerView->m_iActivePatchBitIndex = m_iActivePatchBitIndex;
      }
    }
  }
  
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( pMainFrame->m_dlgPatchesPalette != NULL)
  {
    pMainFrame->m_dlgPatchesPalette->UpdateData( FALSE);
    pMainFrame->m_dlgPatchesPalette->Invalidate(FALSE);
  }
}

void CModelerView::OnUpdateStainsInsert(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !theApp.m_WorkingPatches.IsEmpty());
}

void CModelerView::OnUpdateStainsDelete(CCmdUI* pCmdUI) 
{
  CModelerDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->m_emEditModel.CountPatches() != 0);
}

void CModelerView::OnStainsPreviousStain() 
{
  CModelerDoc* pDoc = GetDocument();
  pDoc->m_emEditModel.GetPreviousValidPatchIndex( m_iActivePatchBitIndex);

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( pMainFrame->m_dlgPatchesPalette != NULL)
  {
    pMainFrame->m_dlgPatchesPalette->UpdateData( FALSE);
    pMainFrame->m_dlgPatchesPalette->Invalidate(FALSE);
  }
}

void CModelerView::OnStainsNextStain() 
{
  CModelerDoc* pDoc = GetDocument();
  pDoc->m_emEditModel.GetNextValidPatchIndex( m_iActivePatchBitIndex);

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( pMainFrame->m_dlgPatchesPalette != NULL)
  {
    pMainFrame->m_dlgPatchesPalette->UpdateData( FALSE);
    pMainFrame->m_dlgPatchesPalette->Invalidate(FALSE);
  }
}

void CModelerView::OnUpdateStainsNextStain(CCmdUI* pCmdUI) 
{
  CModelerDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->m_emEditModel.CountPatches() > 1);
}

void CModelerView::OnUpdateStainsPreviousStain(CCmdUI* pCmdUI) 
{
  CModelerDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->m_emEditModel.CountPatches() > 1);
}

void CModelerView::OnShadowWorse() 
{
  m_RenderPrefs.DesreaseShadowQuality();
  Invalidate( FALSE);
}

void CModelerView::OnShadowBetter() 
{
  m_RenderPrefs.IncreaseShadowQuality();
  Invalidate( FALSE);
}

void CModelerView::OnFallDown() 
{
  if( m_bMappingMode)
  {
    m_offx = 0;
    m_offy = 0;
    //m_MagnifyFactor = 1.0f;
  }
  else
  {
    m_vViewMeasureVertex = FLOAT3D(0,0,0);
    m_plModelPlacement.pl_PositionVector(1) = 0.0f;
    m_plModelPlacement.pl_PositionVector(2) = 0.0f;
    m_plModelPlacement.pl_PositionVector(3) = 0.0f;
    FLOAT fHeading = theApp.m_Preferences.ap_fDefaultHeading;
    FLOAT fPitch = theApp.m_Preferences.ap_fDefaultPitch;
    FLOAT fBanking = theApp.m_Preferences.ap_fDefaultBanking;
    m_plModelPlacement.pl_OrientationAngle = 
      ANGLE3D(AngleDeg(fHeading), AngleDeg(fPitch),AngleDeg(fBanking));
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bCtrl = nFlags & MK_CONTROL;

  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace)
  {
    FastZoomIn( point);
  }

  if( m_bMappingMode)
  {
    PIX pixDPWidth = m_pDrawPort->GetWidth();
    PIX pixDPHeight = m_pDrawPort->GetHeight();
    m_offx += point.x - pixDPWidth/2;
    m_offy += point.y - pixDPHeight/2;
  }

  Invalidate( FALSE);

	CView::OnLButtonDblClk(nFlags, point);
}

void CModelerView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;  
  BOOL bCtrl = nFlags & MK_CONTROL;

  // ctrl+space+XMB is used for 2x zooming
  if( bCtrl && bSpace) {
    FastZoomOut();
  }
	
	CView::OnRButtonDblClk(nFlags, point);
}

void CModelerView::OnKeyA()
{
	if( m_bMappingMode)
  {
    OnToggleAllSurfaces();
  }
  else
  {
    OnChangeAmbient();
  }
}

void CModelerView::OnUpdateSaveThumbnail(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnSaveThumbnail() 
{
  CModelerDoc* pDoc = GetDocument();
  FillThumbnailSettings( pDoc->m_emEditModel.edm_tsThumbnailSettings);
  // mark that thumbnail settings have been set
  pDoc->m_emEditModel.edm_tsThumbnailSettings.ts_bSet = TRUE;
  SaveThumbnail();
  STATUS_LINE_MESSAGE( L"Thumbnail saved."); 
}

void CModelerView::SaveThumbnail() 
{
  CDrawPort *pDrawPort;
  CImageInfo iiImageInfo;
  CTextureData TD;
  CAnimData AD;
  CModelerDoc* pDoc = GetDocument();

  // store current thumbnail settings
  CThumbnailSettings tsCurrent;
  FillThumbnailSettings( tsCurrent);
  // if we have precalculated thumbnail settings
  if( pDoc->m_emEditModel.edm_tsThumbnailSettings.ts_bSet)
  {
    // apply them
    ApplyThumbnailSettings( pDoc->m_emEditModel.edm_tsThumbnailSettings);
  }

  _pGfx->CreateWorkCanvas( 128, 128, &pDrawPort);
  if( pDrawPort != NULL)
  {
    INDEX iCurrentMip = m_ModelObject.GetManualMipLevel();
    m_ModelObject.SetManualMipLevel(0);
    BOOL bAutoMipModeling = m_ModelObject.IsAutoMipModeling();
    m_ModelObject.AutoMipModelingOff();

    if( pDrawPort->Lock())
    {
      BOOL bCollisionModeBefore = m_bCollisionMode;
      BOOL bMappingOnBefore = m_bMappingMode;
      m_bCollisionMode = FALSE;
      m_bMappingMode = FALSE;

      RenderView( pDrawPort);

      m_bCollisionMode = bCollisionModeBefore;
      m_bMappingMode = bMappingOnBefore;

      pDrawPort->Unlock();
    }
    
    CTFileName fnDocName = CTString(CStringA(GetDocument()->GetPathName()));
    CTFileName fnThumbnail = fnDocName.FileDir() + fnDocName.FileName() + ".tbn";

    pDrawPort->GrabScreen( iiImageInfo);
    // try to
    try {
      fnThumbnail.RemoveApplicationPath_t();
      // create texture
      TD.Create_t( &iiImageInfo, 128, MAX_MEX_LOG2, FALSE);
      // save the thumbnail
      CTFileStream File;
      File.Create_t( fnThumbnail);
      TD.Write_t( &File);
      File.Close();
    }
    // if failed
    catch (char *strError) {
      // report error
      AfxMessageBox(CString(strError));
    }

    m_ModelObject.SetManualMipLevel( iCurrentMip);
    if( bAutoMipModeling)
    {
      m_ModelObject.AutoMipModelingOn();
    }
    _pGfx->DestroyWorkCanvas( pDrawPort);
  }
  // restore current view settings
  ApplyThumbnailSettings( tsCurrent);
}


void CModelerView::OnFrameRate() 
{
  m_bFrameRate = !m_bFrameRate;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateFrameRate(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bFrameRate);
}


void CModelerView::OnHeading() 
{
  m_plModelPlacement.pl_OrientationAngle( 1) += AngleDeg( 90.0f);
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnPitch() 
{
  m_plModelPlacement.pl_OrientationAngle( 2) += AngleDeg( 90.0f);
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnBanking() 
{
  m_plModelPlacement.pl_OrientationAngle( 3) += AngleDeg( 90.0f);
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnUpdateCollisionBox(CCmdUI* pCmdUI) 
{
  if( !m_bMappingMode)
  {
    if( theApp.m_pCollisionBoxModelObject != NULL)
    {
      // set collision mode check (press in button)
      pCmdUI->SetCheck( m_bCollisionMode);
      // enable tool bar's button
      pCmdUI->Enable( TRUE);
      // don't disable it again
      return;
    }
  }
  // gray collision mode command
  pCmdUI->Enable( FALSE);
}

void CModelerView::OnCollisionBox() 
{
  if( theApp.m_pCollisionBoxModelObject != NULL)
  {
    m_bCollisionMode = !m_bCollisionMode;
    Invalidate( FALSE);
  }
}

void CModelerView::OnResetViewer() 
{
  if( m_bMappingMode)
  {
    m_offx = 0;
    m_offy = 0;
    m_MagnifyFactor = 1.0f;
  }
  else
  {
	  ResetViewerPosition();
    OnFallDown();
  }
  theApp.m_chGlobal.MarkChanged();
  Invalidate( FALSE);
}

void CModelerView::OnUpdateResetViewer(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( TRUE);
}

void CModelerView::OnDollyViewer() 
{
	m_bDollyViewer = !m_bDollyViewer;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateDollyViewer(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bDollyViewer);
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
  // just invalidate the whole window area
	Invalidate(FALSE);	
}

void CModelerView::OnDollyLight() 
{
	m_bDollyLight = !m_bDollyLight;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateDollyLight(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bDollyLight);
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnDollyLightColor() 
{
	m_bDollyLightColor = !m_bDollyLightColor;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateDollyLightColor(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bDollyLightColor);
  pCmdUI->Enable( !m_bMappingMode);
}

BOOL CModelerView::PreTranslateMessage(MSG* pMsg) 
{
	// translate message first
  BOOL bResult = CView::PreTranslateMessage(pMsg);

  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if(pModelerView != this) return bResult;
  CModelerDoc* pDoc = GetDocument();

  // get key statuses 
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bControl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  // get mouse button statuses
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;
  BOOL bRMB = (GetKeyState( VK_RBUTTON)&0x8000) != 0;

  if( (pMsg->message==WM_KEYDOWN) || bShift || bControl || bSpace || bLMB || bRMB )
  {
    m_bAnyKeyPressed = TRUE;
  }
  else if(pMsg->message==WM_KEYUP)
  {
    m_bAnyKeyPressed = FALSE;
  }

  CDlgPgInfoAttachingPlacement *pAttPg = theApp.m_pPgAttachingPlacement;
  BOOL bAttachmentExists = (pAttPg!=NULL) && (pAttPg->GetCurrentAttachingPlacement() != -1);
  BOOL bViewHasFocus = (this == CWnd::GetFocus());
  // see if we are picking vertices to define axis
  if( bViewHasFocus && bAttachmentExists && (pMsg->message==WM_KEYDOWN) &&
      (( pMsg->wParam=='1') || ( pMsg->wParam=='2') || ( pMsg->wParam=='3')) )
  {
    FLOAT3D vClosestVertex;
    INDEX iClosestVertex = GetClosestVertex( vClosestVertex);
    if( iClosestVertex != -1)
    {
      if( pMsg->wParam=='1') pAttPg->SetPlacementReferenceVertex(iClosestVertex, -1, -1);
      if( pMsg->wParam=='2') pAttPg->SetPlacementReferenceVertex(-1, iClosestVertex, -1);
      if( pMsg->wParam=='3') pAttPg->SetPlacementReferenceVertex(-1, -1, iClosestVertex);
    }
  }

  if( m_bViewMeasureVertex)
  {
    CTString strStatusLine;
    CPlacement3D plMeasureVtx = CPlacement3D(m_vViewMeasureVertex, ANGLE3D(0,0,0));
    plMeasureVtx.AbsoluteToRelative( m_plModelPlacement);
    FLOAT3D vDelta = plMeasureVtx.pl_PositionVector;
    strStatusLine.PrintF("Measure vertex offset: %g, %g, %g", vDelta(1), vDelta(2), vDelta(3));
    STATUS_LINE_MESSAGE( CString(strStatusLine));
  }
  // if we caught key or button down message
  else if(
      (pMsg->message==WM_SYSKEYDOWN) ||
      (pMsg->message==WM_KEYDOWN) ||
      (pMsg->message==WM_KEYUP) ||
      (pMsg->message==WM_LBUTTONDOWN) ||
      (pMsg->message==WM_RBUTTONDOWN) ||
      (pMsg->message==WM_LBUTTONUP) ||
      (pMsg->message==WM_RBUTTONUP) ||
      (bShift || bAlt || bControl || bSpace || bLMB || bRMB) )
  {
    // in mapping mode
    if( m_bMappingMode)
    {
      // nothing
      if( !bShift && !bAlt && !bControl && !bSpace && !bLMB && !bRMB)
      {
        STATUS_LINE_MESSAGE( L"Try: Space, Ctrl+Space"); 
      }
      // space
      else if( !bShift && !bAlt && !bControl && bSpace && !bLMB && !bRMB)
      {
        STATUS_LINE_MESSAGE( L"Move with LMB, zoom with RMB, center with LMBx2. Try: Ctrl+Space");
      }
      // ctrl+space
      else if( !bShift && !bAlt && bControl && bSpace && !bLMB && !bRMB)
      {
        STATUS_LINE_MESSAGE( L"LMB zooms in, RMB zooms out");
      }
    }
    // model view mode
    else
    {
      // nothing
      if( !bShift && !bAlt && !bControl && !bSpace && !bLMB && !bRMB)
      {
        // nothing pressed
        STATUS_LINE_MESSAGE( L"Try: Space, Ctrl+Space, Ctrl, Shift"); 
      }
      // space
      else if( !bShift && !bAlt && !bControl && bSpace && !bLMB && !bRMB && !m_bMappingMode)
      {
        STATUS_LINE_MESSAGE( L"LMB moves, RMB zoomes, LMB+RMB rotates viewer. Try: Ctrl+Space");
      }
      // ctrl+space
      else if( !bShift && !bAlt && bControl && bSpace && !bLMB && !bRMB && !m_bMappingMode)
      {
        STATUS_LINE_MESSAGE( L"LMB zooms in, RMB zoomes out");
      }
      // ctrl
      else if( !bShift && !bAlt && bControl && !bSpace && !bLMB && !bRMB && !m_bMappingMode)
      {
        STATUS_LINE_MESSAGE( L"LMB moves, RMB zoomes, LMB+RMB rotates model");
      }
      // shift
      else if( bShift && !bAlt && !bControl && !bSpace && !bLMB && !bRMB && !m_bMappingMode)
      {
        STATUS_LINE_MESSAGE( L"RMB zoomes, LMB+RMB rotates light");
      }
    }
  }
	return bResult;	
}

void CModelerView::OnNextTexture() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get curently selected combo member
  int iCurrentlySelected = pMainFrame->m_SkinComboBox.GetCurSel();
  int iComboMembersCt = pMainFrame->m_SkinComboBox.GetCount();
  
  // if next combo member can't be selected
  if( (iCurrentlySelected+1) >= iComboMembersCt)
  {
    return;
  }

  // select next combo member
  pMainFrame->m_SkinComboBox.SetCurSel( iCurrentlySelected+1);
  // set new texture ptr for active view
  m_ptdiTextureDataInfo = (CTextureDataInfo *) 
    pMainFrame->m_SkinComboBox.GetItemDataPtr( iCurrentlySelected+1);
  // redraw view
  Invalidate( FALSE);
}

void CModelerView::OnUpdateNextTexture(CCmdUI* pCmdUI) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get curently selected combo member
  int iCurrentlySelected = pMainFrame->m_SkinComboBox.GetCurSel();
  int iComboMembersCt = pMainFrame->m_SkinComboBox.GetCount();
  
  // if next combo member can't be selected
  pCmdUI->Enable( (iCurrentlySelected+1) < iComboMembersCt);
}

void CModelerView::OnPreviousTexture() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get curently selected combo member
  int iCurrentlySelected = pMainFrame->m_SkinComboBox.GetCurSel();
  
  // if next combo member can't be selected
  if( (iCurrentlySelected == 0) || (iCurrentlySelected==CB_ERR) )
  {
    return;
  }

  // select next combo member
  pMainFrame->m_SkinComboBox.SetCurSel( iCurrentlySelected-1);
  // set new texture ptr for active view
  m_ptdiTextureDataInfo = (CTextureDataInfo *) 
    pMainFrame->m_SkinComboBox.GetItemDataPtr( iCurrentlySelected-1);
  // redraw view
  Invalidate( FALSE);
}

void CModelerView::OnUpdatePreviousTexture(CCmdUI* pCmdUI) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get curently selected combo member
  int iCurrentlySelected = pMainFrame->m_SkinComboBox.GetCurSel();
  // if previous combo member can't be selected
  pCmdUI->Enable( (iCurrentlySelected != 0) && (iCurrentlySelected!=CB_ERR) );
}

void CModelerView::OnRecreateTexture() 
{
  // there must be valid texture
  ASSERT( AssureValidTDI());
  CTextureData *pTD = m_ptdiTextureDataInfo->tdi_TextureData;
  CTFileName fnTextureName = pTD->GetName();
  // call (re)create texture dialog
  _EngineGUI.CreateTexture( fnTextureName);
  // try to 
  CTextureData *ptdTextureToReload;
  try {
    // obtain texture
    ptdTextureToReload = _pTextureStock->Obtain_t( fnTextureName);
  }
  catch ( char *err_str) {
    AfxMessageBox( CString(err_str));
    return;
  }    
  // reload the texture
  ptdTextureToReload->Reload();
  // release the texture
  _pTextureStock->Release( ptdTextureToReload);
  CModelerDoc* pDoc = GetDocument();
  pDoc->UpdateAllViews( NULL);
}

void CModelerView::OnUpdateRecreateTexture(CCmdUI* pCmdUI) 
{
  if( AssureValidTDI())
  {
    pCmdUI->Enable( TRUE);
  }
  else
  {
    pCmdUI->Enable( FALSE);
  }
}

#define EQUAL_SUB_STR( str) (strnicmp( achrLine, str, strlen(str)) == 0)
void CModelerView::OnCreateMipModels() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	CObject3D objRestFrame, objMipSourceFrame;
	CListHead FrameNamesList;
  CTFileName fnFrameFileName;
	char achrLine[ 128];
  char achrBasePath[ PATH_MAX] = "";
	char achrRestFrame[ PATH_MAX] = "";
	char achrRestFrameFullPath[ PATH_MAX] = "";

  CModelerDoc* pDoc = GetDocument();
  CTFileName fnModelName = CTString(CStringA(pDoc->GetPathName()));
  CTFileName fnScriptName = fnModelName.FileDir() + fnModelName.FileName() + ".scr";
  try
  {
    fnScriptName.RemoveApplicationPath_t();
  	// open script file
    CTFileStream File;
    File.Open_t( fnScriptName);				
    
    FLOATmatrix3D mStretch;
    mStretch.Diagonal(1.0f);
    try
    {
		  FOREVER
      {
        do
        {
          File.GetLine_t(achrLine, 128);
        }
		    while( (strlen( achrLine)== 0) || (achrLine[0]==';'));

		    if( EQUAL_SUB_STR( "DIRECTORY"))
		    {
			    _strupr( achrLine);
          sscanf( achrLine, "DIRECTORY %s", achrBasePath);
			    if( achrBasePath[ strlen( achrBasePath) - 1] != '\\')
				    strcat( achrBasePath,"\\");
		    }
		    else if( EQUAL_SUB_STR( "MIP_MODELS"))
        {
          File.GetLine_t(achrLine, 128);
  			  _strupr( achrLine);
			    sscanf( achrLine, "%s", achrRestFrame);
			    sprintf( achrRestFrameFullPath, "%s%s", achrBasePath, achrRestFrame);
        }
		    else if( EQUAL_SUB_STR( "SIZE"))
        {
  	      _strupr( achrLine);
          FLOAT fStretch = 1.0f;
		      sscanf( achrLine, "SIZE %g", &fStretch);
          mStretch *= fStretch;
		    }
		    else if( EQUAL_SUB_STR( "TRANSFORM")) 
        {
  	      _strupr( achrLine);
          FLOATmatrix3D mTran;
          mTran.Diagonal(1.0f);
		      sscanf( achrLine, "TRANSFORM %g %g %g %g %g %g %g %g %g", 
            &mTran(1,1), &mTran(1,2), &mTran(1,3),
            &mTran(2,1), &mTran(2,2), &mTran(2,3),
            &mTran(3,1), &mTran(3,2), &mTran(3,3));
          mStretch *= mTran;
        }
		    else if( EQUAL_SUB_STR( "ANIM_START"))
        {
  	      pDoc->m_emEditModel.edm_md.LoadFromScript_t( &File, &FrameNamesList);
          // extract file name of last rendered frame
          INDEX iFrame = 0;
          FOREACHINLIST( CFileNameNode, cfnn_Node, FrameNamesList, itFrameName)
          {
            if( m_iCurrentFrame == iFrame)
            {
              fnFrameFileName = CTString(itFrameName->cfnn_FileName);
              break;
            }
            iFrame++;
          }
          // clear list of frames
          FORDELETELIST( CFileNameNode, cfnn_Node, FrameNamesList, litDel)
            delete &litDel.Current();
        }
      }
    }
    catch( char *pstrError)
    {
      (void) pstrError;
    }
    // if frame name is extracted properly
    if( fnFrameFileName != "")
    {
      // load rest frame
      objRestFrame.LoadAny3DFormat_t( CTString(achrRestFrameFullPath), mStretch);
      // load mip source frame
      objMipSourceFrame.LoadAny3DFormat_t( fnFrameFileName, mStretch);

      // show progres dialog
      CRect rectMainFrameSize;
      CRect rectProgress, rectProgressNew;
      pMainFrame->GetWindowRect( &rectMainFrameSize);
      pMainFrame->m_NewProgress.Create( IDD_NEW_PROGRESS, pMainFrame);
      pMainFrame->m_NewProgress.GetWindowRect( &rectProgress);
      rectProgressNew.left = rectMainFrameSize.Width()/2 - rectProgress.Width()/2;
      rectProgressNew.top = rectMainFrameSize.Height()/2 - rectProgress.Height()/2;
      rectProgressNew.right = rectProgressNew.left + rectProgress.Width();
      rectProgressNew.bottom = rectProgressNew.top + rectProgress.Height();
      pMainFrame->m_NewProgress.MoveWindow( rectProgressNew);
      pMainFrame->m_NewProgress.ShowWindow(SW_SHOW);

      CDlgAutoMipModeling dlgAutoMipModeling;
      if( (dlgAutoMipModeling.DoModal() != IDOK) ||
          (dlgAutoMipModeling.m_iVerticesToRemove<=0) ||
          (dlgAutoMipModeling.m_iSurfacePreservingFactor<1) ||
          (dlgAutoMipModeling.m_iSurfacePreservingFactor>99) )
      {
        pMainFrame->m_NewProgress.DestroyWindow();
        return;
      }
      // create mip models
      pDoc->m_emEditModel.CreateMipModels_t( objRestFrame, objMipSourceFrame,
        dlgAutoMipModeling.m_iVerticesToRemove, dlgAutoMipModeling.m_iSurfacePreservingFactor);
      // copy mapping from main mip model
      pDoc->m_emEditModel.SaveMapping_t( CTString("Temp\\ForAutoMipMapping.map"), 0);
      // paste mapping over all smaller mip models
      INDEX iMipModel=1;
      for( ; iMipModel<pDoc->m_emEditModel.edm_md.md_MipCt; iMipModel++)
      {
        pDoc->m_emEditModel.LoadMapping_t( CTString("Temp\\ForAutoMipMapping.map"), iMipModel);
      }

      for( INDEX iSurface=1; 
        iSurface<pDoc->m_emEditModel.edm_md.md_MipInfos[0].mmpi_MappingSurfaces.Count();
        iSurface++)
      {
        // get rendering flags from main mip model for current surface
        ULONG ulRenderFlags;
        enum SurfaceShadingType sstShading;
        enum SurfaceTranslucencyType sttTranslucency;
        m_ModelObject.GetSurfaceRenderFlags( 0, iSurface, sstShading, sttTranslucency, ulRenderFlags);
        // get color of surface
        UBYTE ubSurfaceTransparency = (UBYTE) (m_ModelObject.GetSurfaceColor( 0, iSurface) & CT_OPAQUE);
        for( iMipModel=1; iMipModel<pDoc->m_emEditModel.edm_md.md_MipCt; iMipModel++)
        {
          // set render flags
          m_ModelObject.SetSurfaceRenderFlags( iMipModel, iSurface, sstShading, sttTranslucency, ulRenderFlags);
          COLOR colSurfaceColor = m_ModelObject.GetSurfaceColor( iMipModel, iSurface);
          // set remembered transparency
          colSurfaceColor = (colSurfaceColor & 0xFFFFFF00) | ubSurfaceTransparency;
          // set new surface color
          m_ModelObject.SetSurfaceColor( iMipModel, iSurface, colSurfaceColor);
        }
      }
      // destroy progres window
      pMainFrame->m_NewProgress.DestroyWindow();
    }
  }
  catch( char *pStrError)
  {
    // destroy progress window
    pMainFrame->m_NewProgress.DestroyWindow();
    AfxMessageBox( CString(pStrError));
  }
  pDoc->SelectMipModel( 0);
  pDoc->SelectSurface( 0, TRUE);
  m_ModelObject.SetManualMipLevel( 0);
  theApp.m_chGlobal.MarkChanged();
  pDoc->UpdateAllViews( NULL);
  pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
  pDoc->SetModifiedFlag();
}

void CModelerView::OnUpdatePickVertex(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

INDEX CModelerView::GetClosestVertex(FLOAT3D &vClosestVertex)
{
  CPerspectiveProjection3D prProjection;
  SetProjectionData( prProjection, m_pDrawPort);
  // set position of document's model
  prProjection.ObjectPlacementL() = m_plModelPlacement;
  return m_ModelObject.PickVertexIndex( m_pDrawPort, &prProjection,
    m_MousePosition.x, m_MousePosition.y, vClosestVertex);
}


void CModelerView::OnPickVertex() 
{
  FLOAT3D vClosestVertex;
  INDEX iClosestVertex = GetClosestVertex( vClosestVertex);
  if( iClosestVertex != -1)
  {
    char achrLine[ 256];
    sprintf( achrLine, "Vertex index = %d (%g, %g, %g)",
    iClosestVertex, vClosestVertex(1), vClosestVertex(2), vClosestVertex(3));
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->m_wndStatusBar.SetPaneText( CLOSEST_SURFACE_PANE, CString(achrLine));
  }
}

void CModelerView::OnDollyMipModeling() 
{
  m_bDollyMipModeling = !m_bDollyMipModeling;
}

void CModelerView::OnUpdateDollyMipModeling(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bDollyMipModeling!=0);
}

void CModelerView::OnAnimPlayOnce() 
{
  if( m_ModelObject.IsPaused()) m_ModelObject.ContinueAnim();  
  m_ModelObject.ResetAnim();
  m_ModelObject.PlayAnim( m_ModelObject.GetAnim(), AOF_NORESTART);

  PlayAnimForAllAttachments( m_ModelObject, AOF_NORESTART);
}

void CModelerView::OnUpdateAnimPlayOnce(CCmdUI* pCmdUI) 
{
	if( m_bMappingMode) pCmdUI->Enable( FALSE);
}

void CModelerView::FillThumbnailSettings( CThumbnailSettings &tsToReceive)
{
  tsToReceive.ts_plLightPlacement = m_plLightPlacement;
  tsToReceive.ts_plModelPlacement = m_plModelPlacement;
  tsToReceive.ts_fTargetDistance = m_fTargetDistance;
  tsToReceive.ts_vTarget = m_vTarget;
  tsToReceive.ts_angViewerOrientation = m_angViewerOrientation;
  tsToReceive.ts_LightDistance = m_LightDistance;
  tsToReceive.ts_LightColor = m_LightColor;
  tsToReceive.ts_colAmbientColor = m_colAmbientColor;
  tsToReceive.ts_PaperColor = m_PaperColor;
  tsToReceive.ts_InkColor = m_InkColor;
  tsToReceive.ts_IsWinBcgTexture = m_IsWinBcgTexture;
  tsToReceive.ts_WinBcgTextureName = m_fnBcgTexture;
  tsToReceive.ts_RenderPrefs = m_RenderPrefs;
}

void CModelerView::ApplyThumbnailSettings( CThumbnailSettings &tsToApply)
{
  m_plLightPlacement = tsToApply.ts_plLightPlacement;
  m_plModelPlacement = tsToApply.ts_plModelPlacement;
  m_fTargetDistance = tsToApply.ts_fTargetDistance;
  m_vTarget = tsToApply.ts_vTarget;
  m_angViewerOrientation = tsToApply.ts_angViewerOrientation;
  m_LightDistance = tsToApply.ts_LightDistance;
  m_LightColor = tsToApply.ts_LightColor;
  m_colAmbientColor = tsToApply.ts_colAmbientColor;
  m_PaperColor = tsToApply.ts_PaperColor;
  m_InkColor = tsToApply.ts_InkColor;
  m_IsWinBcgTexture = tsToApply.ts_IsWinBcgTexture;
  m_fnBcgTexture = tsToApply.ts_WinBcgTextureName;
  m_RenderPrefs = tsToApply.ts_RenderPrefs;
}

void CModelerView::OnTileTexture() 
{
  m_bTileMappingBCG = !m_bTileMappingBCG;
  GetDocument()->UpdateAllViews( NULL);	
}

void CModelerView::OnUpdateTileTexture(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bTileMappingBCG);
}

void CModelerView::OnAddReflectionTexture() 
{
  CTFileName fnChoosedFile = _EngineGUI.FileRequester( "Select reflection texture", 
    FILTER_TEX FILTER_ALL FILTER_END, "Reflection textures directory", "Textures\\");
  if( fnChoosedFile == "") return;
  try
  {
    m_ModelObject.mo_toReflection.SetData_t( fnChoosedFile);
    GetDocument()->m_emEditModel.edm_fnReflectionTexture = fnChoosedFile;
  }
  catch( char *strError)
  {
    WarningMessage( strError);
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnAddSpecular() 
{
  CTFileName fnChoosedFile = _EngineGUI.FileRequester( "Select specular texture", 
    FILTER_TEX FILTER_ALL FILTER_END, "Specularity textures directory", "Textures\\");
  if( fnChoosedFile == "") return;
  try
  {
    m_ModelObject.mo_toSpecular.SetData_t( fnChoosedFile);
    GetDocument()->m_emEditModel.edm_fnSpecularTexture = fnChoosedFile;
  }
  catch( char *strError)
  {
    WarningMessage( strError);
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnAddBumpTexture() 
{
  CTFileName fnChoosedFile = _EngineGUI.FileRequester( "Select bump texture", 
    FILTER_TEX FILTER_ALL FILTER_END, "Bump textures directory", "Textures\\");
  if( fnChoosedFile == "") return;
  try
  {
    m_ModelObject.mo_toBump.SetData_t( fnChoosedFile);
    GetDocument()->m_emEditModel.edm_fnBumpTexture = fnChoosedFile;
  }
  catch( char *strError)
  {
    WarningMessage( strError);
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnRemoveReflection() 
{
  try {
    m_ModelObject.mo_toReflection.SetData_t( CTString(""));
    GetDocument()->m_emEditModel.edm_fnReflectionTexture = CTString("");
  }
  catch( char *strError) { (void) strError;}
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnRemoveSpecular() 
{
  try { 
    m_ModelObject.mo_toSpecular.SetData_t( CTString(""));
    GetDocument()->m_emEditModel.edm_fnSpecularTexture = CTString("");
  }
  catch( char *strError) { (void) strError;}
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnRemoveBumpMap() 
{
  try {
    m_ModelObject.mo_toBump.SetData_t( CTString(""));
    GetDocument()->m_emEditModel.edm_fnBumpTexture = CTString("");
  }
  catch( char *strError) { (void) strError;}
  theApp.m_chGlobal.MarkChanged();
}

void CModelerView::OnUpdateRemoveReflection(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_ModelObject.mo_toReflection.GetData() != NULL);
}

void CModelerView::OnUpdateRemoveSpecular(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_ModelObject.mo_toSpecular.GetData() != NULL);
}

void CModelerView::OnUpdateRemoveBumpMap(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_ModelObject.mo_toBump.GetData() != NULL);
}

void CModelerView::OnSurfaceNumbers() 
{
	m_bPrintSurfaceNumbers = !m_bPrintSurfaceNumbers;	
}

void CModelerView::OnUpdateSurfaceNumbers(CCmdUI* pCmdUI) 
{
	if( !m_bMappingMode)
  {
    pCmdUI->Enable( FALSE);
  }
  else if( m_bPrintSurfaceNumbers)
  {
    pCmdUI->SetCheck(1);
  }
  else
  {
    pCmdUI->SetCheck( 0);
  }
}

void CModelerView::OnExportSurfaces() 
{
  CTFileName fnExportFileName = _EngineGUI.FileRequester( "Select export file name",
                            FILTER_TXT FILTER_END, "Open model directory",
                            "Models\\", "", NULL, FALSE);
  if( fnExportFileName == "") return;

  CModelerDoc* pDoc = GetDocument();
  pDoc->m_emEditModel.ExportSurfaceNumbersAndNames( fnExportFileName);
}

void CModelerView::OnPreviousBcgTexture() 
{
  m_fnBcgTexture = theApp.NextPrevBcgTexture( m_fnBcgTexture, -1);
  Invalidate( FALSE);
}

void CModelerView::OnUpdatePreviousBcgTexture(CCmdUI* pCmdUI) 
{
  if( theApp.m_WorkingTextures.Count() > 1)
  {
    pCmdUI->Enable(TRUE);
  }
  else
  {
    pCmdUI->Enable(FALSE);
  }
}

void CModelerView::OnNextBcgTexture() 
{
  m_fnBcgTexture = theApp.NextPrevBcgTexture( m_fnBcgTexture, 1);
  Invalidate( FALSE);
}

void CModelerView::OnUpdateNextBcgTexture(CCmdUI* pCmdUI) 
{
  if( theApp.m_WorkingTextures.Count() > 1)
  {
    pCmdUI->Enable(TRUE);
  }
  else
  {
    pCmdUI->Enable(FALSE);
  }
}

void CModelerView::OnWindowTogglemax() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->OnWindowTogglemax();	
}

void CModelerView::OnExportForSkining() 
{
  CModelerDoc* pDoc = GetDocument();
  CTFileName fnDocName = CTString(CStringA(pDoc->GetPathName()));
  CTFileName fnDirectory = fnDocName.FileDir();
  CTFileName fnDefaultSelected = fnDocName.FileName()+CTString(".tga");

  try
  {
    fnDirectory.RemoveApplicationPath_t();
  }
  catch( char *str_err)
  {
    AfxMessageBox( CString(str_err));
    return;
  }

  // note: NULL given instead of profile string name, to avoid using last directory
  CTFileName fnExportName = 
    _EngineGUI.FileRequester( "Select name to export mapping",
    "Pictures (*.tga)\0*.tga\0" FILTER_END,
    NULL, fnDirectory, fnDefaultSelected, NULL, FALSE);
  if( fnExportName == "") return;

  CTFileName fnFullPath = _fnmApplicationPath+fnExportName;
  if( GetFileAttributesA( fnFullPath) != -1)
  {
    CTString strMsg;
    strMsg.PrintF( "File \"%s\" already exist. Do you want to replace it?", fnFullPath);
    if( ::MessageBoxA( this->m_hWnd, strMsg, "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1|
               MB_SYSTEMMODAL | MB_TOPMOST) != IDYES)
    {
      return;
    }
  }

  CDlgExportForSkinning dlg( fnExportName);
  if( dlg.DoModal() != IDOK) return;
  COLOR colPaper = dlg.m_ctrlPaperColor.GetColor();
  COLOR colWire = dlg.m_ctrlWireColor.GetColor();

  // calculate exporting picture width and height
  PIX pixWidth = dlg.m_iTextureWidth;
  FLOAT fWHRatio = FLOAT(pDoc->m_emEditModel.edm_md.md_Width)/pDoc->m_emEditModel.edm_md.md_Height;
  PIX pixHeight = PIX( pixWidth/fWHRatio);

  CDrawPort *pdp;
  _pGfx->CreateWorkCanvas( pixWidth, pixHeight, &pdp);
  if( pdp == NULL) return;
  
  if( !pdp->Lock()) return;

  // clear bcg
  pdp->Fill(colPaper|CT_OPAQUE);

 	MEX mexWidth, mexHeight;
  mexWidth = m_ModelObject.GetWidth();
  mexHeight = m_ModelObject.GetHeight();
  FLOAT fMagnifyFit = FLOAT(pixWidth)/mexWidth;

  ModelMipInfo &mmi = pDoc->m_emEditModel.edm_md.md_MipInfos[ 0];
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
  {
    // render surface color
    if( dlg.m_bColoredSurfaces) {
      MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
      pDoc->m_emEditModel.DrawFilledSurface( pdp, 0, iSurface, fMagnifyFit, 0, 0, ms.ms_colColor, ms.ms_colColor);
    }
    // render wire frame
    if( dlg.m_bWireFrame) {
      pDoc->m_emEditModel.DrawWireSurface( pdp, 0, iSurface, fMagnifyFit, 0, 0, colWire, colWire);
    }
  }
  // if we should print surface numbers
  if( dlg.m_bSurfaceNumbers) {
    pDoc->m_emEditModel.PrintSurfaceNumbers( pdp, theApp.m_pfntFont, 0, fMagnifyFit, 0, 0, colWire);
    pDoc->m_emEditModel.ExportSurfaceNumbersAndNames( fnExportName.FileDir()+fnExportName.FileName()+".txt");
  }
  pdp->Unlock();

  // grab draw port ti image info
  CImageInfo iiImageInfo;
  pdp->GrabScreen( iiImageInfo);

  // save picture
  try
  {
    iiImageInfo.SaveTGA_t( fnExportName);
  }
  catch (char *strError)
  {
    AfxMessageBox(CString(strError));
  }
  
  _pGfx->DestroyWorkCanvas( pdp);
}

void CModelerView::OnRenderSurfacesInColors() 
{
  m_bRenderMappingInSurfaceColors = !m_bRenderMappingInSurfaceColors;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateRenderSurfacesInColors(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bRenderMappingInSurfaceColors);
  pCmdUI->Enable( m_bMappingMode);
}

void CModelerView::OnViewAxis() 
{
  if     ( m_atAxisType == AT_NONE) m_atAxisType = AT_MAIN;
  else if( m_atAxisType == AT_MAIN) m_atAxisType = AT_ALL;
  else if( m_atAxisType == AT_ALL ) m_atAxisType = AT_NONE;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateViewAxis(CCmdUI* pCmdUI) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  UINT nIDView, nStyleView;
  int iViewImage;
  pMainFrame->m_RenderControlBar.GetButtonInfo( 13, nIDView, nStyleView, iViewImage);
  // if no axis at all
  if( m_atAxisType == AT_NONE)
  {
    pMainFrame->m_RenderControlBar.SetButtonInfo( 13, nIDView, nStyleView, 13);
    pCmdUI->SetCheck( FALSE);
  }
  // if only axis of main model are visible
  else if( m_atAxisType == AT_MAIN)
  {
    pMainFrame->m_RenderControlBar.SetButtonInfo( 13, nIDView, nStyleView, 14);
    pCmdUI->SetCheck( TRUE);
  }
  // if all axis are visible
  else if( m_atAxisType == AT_ALL)
  {
    pMainFrame->m_RenderControlBar.SetButtonInfo( 13, nIDView, nStyleView, 15);
    pCmdUI->SetCheck( TRUE);
  }
}

void CModelerView::OnViewInfo() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->ToggleInfoWindow();
}


void CModelerView::OnChangeAmbient() 
{
  COLORREF clrfAmbient = CLRF_CLR( m_colAmbientColor);
  if( MyChooseColor( clrfAmbient, *GetParent()))
  {
    m_colAmbientColor = CLR_CLRF( clrfAmbient);
    Invalidate( FALSE);
  }
}

void CModelerView::OnUpdateChangeAmbient(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  GetDocument()->UpdateAllViews( NULL);	
}

void CModelerView::OnToggleAllSurfaces() 
{
  m_ShowAllSurfaces = !m_ShowAllSurfaces;
  GetDocument()->UpdateAllViews( NULL);	
}

void CModelerView::OnUpdateToggleAllSurfaces(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_bMappingMode);
  GetDocument()->UpdateAllViews( NULL);	
}

void CModelerView::OnKeyT() 
{
  if( m_bMappingMode)
  {
    OnTileTexture();
  }
  else
  {
    OnRendUseTexture();
  }

}

void CModelerView::OnAnimFirst() 
{
  m_ModelObject.StartAnim( 0);  
}

void CModelerView::OnAnimLast() 
{
  m_ModelObject.StartAnim( m_ModelObject.GetAnimsCt()-1);  
}

void CModelerView::OnUpdateAnimFirst(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnUpdateAnimLast(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
}

void CModelerView::OnToggleMeasureVtx() 
{
  if( m_bViewMeasureVertex)
  {
    CPlacement3D plMeasureVtx = CPlacement3D(m_vViewMeasureVertex, ANGLE3D(0,0,0));
    plMeasureVtx.AbsoluteToRelative( m_plModelPlacement);
    FLOAT3D vDelta = plMeasureVtx.pl_PositionVector;
    // save closest vertex coordinates into file
    CTString strCoords;
    strCoords.PrintF( "(%gf, %gf, %gf)",vDelta(1), vDelta(2), vDelta(3));
    try {
      strCoords.Save_t(CTString("temp\\VertexCoords.txt"));
    } catch( char *strError) {
      WarningMessage( strError);
    }
  }

  m_bViewMeasureVertex = !m_bViewMeasureVertex;
  //m_vTarget = m_vViewMeasureVertex;
  Invalidate( FALSE);
}

void CModelerView::OnUpdateToggleMeasureVtx(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !m_bMappingMode);
  pCmdUI->SetCheck( m_bViewMeasureVertex);
}

