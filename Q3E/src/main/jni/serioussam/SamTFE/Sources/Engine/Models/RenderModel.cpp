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

#include "Engine/StdH.h"

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Math/Float.h>
#include <Engine/Models/ModelObject.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/ModelProfile.h>
#include <Engine/Models/RenderModel.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Base/Lists.inl>
#include <Engine/Math/OBBox.h>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/StaticStackArray.cpp>

CStaticStackArray<CRenderModel> _armRenderModels;


// texture used for simple model shadows
CTextureObject _toSimpleModelShadow;

static INDEX _iRenderingType = 0; // 0=none, 1=view, 2=mask

extern FLOAT mdl_fLODMul;
extern FLOAT mdl_fLODAdd;
extern INDEX mdl_iShadowQuality;

CAnyProjection3D _aprProjection;
CDrawPort *_pdp = NULL;
UBYTE *_pubMask = NULL;
SLONG _slMaskWidth  = 0;
SLONG _slMaskHeight = 0;
static enum FPUPrecisionType _fpuOldPrecision;


// begin/end model rendering to screen
void BeginModelRenderingView( CAnyProjection3D &prProjection, CDrawPort *pdp)
{
  ASSERT( _iRenderingType==0 && _pdp==NULL);

  // set 3D projection
  _iRenderingType = 1;
  _pdp = pdp;
  prProjection->ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  prProjection->Prepare();
  // in case of mirror projection, move mirror clip plane a bit father from the mirrored models,
  // so we have less clipping (for instance, player feet)
  if( prProjection->pr_bMirror) prProjection->pr_plMirrorView.pl_distance -= 0.06f; // -0.06 is because entire projection is offseted by +0.05
  _aprProjection = prProjection;
  _pdp->SetProjection( _aprProjection);
  // make FPU precision low
  _fpuOldPrecision = GetFPUPrecision(); 
  SetFPUPrecision(FPT_24BIT);

  // prepare common arrays for simple shadows rendering
  _avtxCommon.PopAll();
  _atexCommon.PopAll();
  _acolCommon.PopAll();

  // eventually setup truform
  extern INDEX gap_bForceTruform;
  extern INDEX ogl_bTruformLinearNormals;
  if( ogl_bTruformLinearNormals) ogl_bTruformLinearNormals = 1;
  if( gap_bForceTruform) {
    gap_bForceTruform = 1;
    gfxSetTruform( _pGfx->gl_iTessellationLevel, ogl_bTruformLinearNormals);
  }
}


void EndModelRenderingView( BOOL bRestoreOrtho/*=TRUE*/)
{
  ASSERT( _iRenderingType==1 && _pdp!=NULL);
  // assure that FPU precision was low all the model rendering time, then revert to old FPU precision
  ASSERT( GetFPUPrecision()==FPT_24BIT);
  SetFPUPrecision(_fpuOldPrecision);
  // restore front face direction
  gfxFrontFace(GFX_CCW);
  // render all batched shadows
  extern void RenderBatchedSimpleShadows_View(void);
  RenderBatchedSimpleShadows_View();
  // back to 2D projection?
  if( bRestoreOrtho) _pdp->SetOrtho();
  _iRenderingType = 0;
  _pdp = NULL;
  // eventually disable re-enable clipping
  gfxEnableClipping();
  if( _aprProjection->pr_bMirror || _aprProjection->pr_bWarp) gfxEnableClipPlane();
}



// begin/end model rendering to shadow mask
void BeginModelRenderingMask( CAnyProjection3D &prProjection, UBYTE *pubMask, SLONG slMaskWidth, SLONG slMaskHeight)
{
  ASSERT( _iRenderingType==0);
  _iRenderingType = 2;
  _aprProjection  = prProjection;
  _pubMask      = pubMask;
  _slMaskWidth  = slMaskWidth; 
  _slMaskHeight = slMaskHeight; 
}

void EndModelRenderingMask(void)
{
  ASSERT( _iRenderingType==2);
  _iRenderingType = 0;
}




// calculate models bounding box (needed for simple shadows and trivial rejection of in-fog/haze-case)
static void CalculateBoundingBox( CModelObject *pmo, CRenderModel &rm)
{
  if( rm.rm_ulFlags & RMF_BBOXSET) return;
  // get model's data and lerp info
  rm.rm_pmdModelData = (CModelData*)pmo->GetData();
  pmo->GetFrame( rm.rm_iFrame0, rm.rm_iFrame1, rm.rm_fRatio);
  // calculate projection model bounding box in object space
  const FLOAT3D &vMin0 = rm.rm_pmdModelData->md_FrameInfos[rm.rm_iFrame0].mfi_Box.Min();
  const FLOAT3D &vMax0 = rm.rm_pmdModelData->md_FrameInfos[rm.rm_iFrame0].mfi_Box.Max();
  const FLOAT3D &vMin1 = rm.rm_pmdModelData->md_FrameInfos[rm.rm_iFrame1].mfi_Box.Min();
  const FLOAT3D &vMax1 = rm.rm_pmdModelData->md_FrameInfos[rm.rm_iFrame1].mfi_Box.Max();
  rm.rm_vObjectMinBB = Lerp( vMin0, vMin1, rm.rm_fRatio);
  rm.rm_vObjectMaxBB = Lerp( vMax0, vMax1, rm.rm_fRatio);
  rm.rm_vObjectMinBB(1) *= pmo->mo_Stretch(1);  rm.rm_vObjectMaxBB(1) *= pmo->mo_Stretch(1);
  rm.rm_vObjectMinBB(2) *= pmo->mo_Stretch(2);  rm.rm_vObjectMaxBB(2) *= pmo->mo_Stretch(2);
  rm.rm_vObjectMinBB(3) *= pmo->mo_Stretch(3);  rm.rm_vObjectMaxBB(3) *= pmo->mo_Stretch(3);
  rm.rm_ulFlags |= RMF_BBOXSET;
}




CRenderModel::CRenderModel(void)
{
  rm_ulFlags  = 0;
  rm_colBlend = C_WHITE|0xFF;
  (INDEX&)rm_fDistanceFactor = 12345678;  // mip factor readjustment needed
  rm_iTesselationLevel = 0;
}


CRenderModel::~CRenderModel(void)
{
  if( !(rm_ulFlags&RMF_ATTACHMENT)) _armRenderModels.PopAll();
}


// set placement of the object
void CRenderModel::SetObjectPlacement( const CPlacement3D &pl)
{
  rm_vObjectPosition = pl.pl_PositionVector;
  MakeRotationMatrixFast( rm_mObjectRotation, pl.pl_OrientationAngle);
}

void CRenderModel::SetObjectPlacement( const FLOAT3D &v, const FLOATmatrix3D &m)
{
  rm_vObjectPosition = v;
  rm_mObjectRotation = m;
}


// set modelview matrix if not already set
void CRenderModel::SetModelView(void)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SETMODELVIEW);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SETMODELVIEW);

  // adjust clipping to frustum
  if( rm_ulFlags & RMF_INSIDE) gfxDisableClipping();
  else gfxEnableClipping();

  // adjust clipping to mirror-plane (if any)
  extern INDEX gap_iOptimizeClipping;
  if( gap_iOptimizeClipping>0 && (_aprProjection->pr_bMirror || _aprProjection->pr_bWarp)) {
    if( rm_ulFlags & RMF_INMIRROR) gfxDisableClipPlane();
    else gfxEnableClipPlane();
  }

  // make transform matrix 
  const FLOATmatrix3D &m = rm_mObjectToView;
  const FLOAT3D       &v = rm_vObjectToView;
  FLOAT glm[16];
  glm[0] = m(1,1);  glm[4] = m(1,2);  glm[ 8] = m(1,3);  glm[12] = v(1);
  glm[1] = m(2,1);  glm[5] = m(2,2);  glm[ 9] = m(2,3);  glm[13] = v(2);
  glm[2] = m(3,1);  glm[6] = m(3,2);  glm[10] = m(3,3);  glm[14] = v(3);
  glm[3] = 0;       glm[7] = 0;       glm[11] = 0;       glm[15] = 1;
  gfxSetViewMatrix(glm);

  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SETMODELVIEW);
}



// transform a vertex in model with lerping
void CModelObject::UnpackVertex( CRenderModel &rm, const INDEX iVertex, FLOAT3D &vVertex)
{
  if( ((CModelData*)GetData())->md_Flags & MF_COMPRESSED_16BIT) {
    // get 16 bit packed vertices
    const SWPOINT3D &vsw0 = rm.rm_pFrame16_0[iVertex].mfv_SWPoint;
    const SWPOINT3D &vsw1 = rm.rm_pFrame16_1[iVertex].mfv_SWPoint;
    // convert them to float and lerp between them
    vVertex(1) = (Lerp( (FLOAT)vsw0(1), (FLOAT)vsw1(1), rm.rm_fRatio) -rm.rm_vOffset(1)) * rm.rm_vStretch(1);
    vVertex(2) = (Lerp( (FLOAT)vsw0(2), (FLOAT)vsw1(2), rm.rm_fRatio) -rm.rm_vOffset(2)) * rm.rm_vStretch(2);
    vVertex(3) = (Lerp( (FLOAT)vsw0(3), (FLOAT)vsw1(3), rm.rm_fRatio) -rm.rm_vOffset(3)) * rm.rm_vStretch(3);
  } else {
    // get 8 bit packed vertices
    const SBPOINT3D &vsb0 = rm.rm_pFrame8_0[iVertex].mfv_SBPoint;
    const SBPOINT3D &vsb1 = rm.rm_pFrame8_1[iVertex].mfv_SBPoint;
    // convert them to float and lerp between them
    vVertex(1) = (Lerp( (FLOAT)vsb0(1), (FLOAT)vsb1(1), rm.rm_fRatio) -rm.rm_vOffset(1)) * rm.rm_vStretch(1);
    vVertex(2) = (Lerp( (FLOAT)vsb0(2), (FLOAT)vsb1(2), rm.rm_fRatio) -rm.rm_vOffset(2)) * rm.rm_vStretch(2);
    vVertex(3) = (Lerp( (FLOAT)vsb0(3), (FLOAT)vsb1(3), rm.rm_fRatio) -rm.rm_vOffset(3)) * rm.rm_vStretch(3);
  }
}


// Create render model structure for rendering an attached model
BOOL CModelObject::CreateAttachment( CRenderModel &rmMain, CAttachmentModelObject &amo)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_CREATEATTACHMENT);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_CREATEATTACHMENT);
  CRenderModel &rmAttached = *amo.amo_prm;
  rmAttached.rm_ulFlags = (rmMain.rm_ulFlags & (RMF_FOG|RMF_HAZE|RMF_WEAPON)) | RMF_ATTACHMENT;

  // get the position
  rmMain.rm_pmdModelData->md_aampAttachedPosition.Lock();
  const CAttachedModelPosition &amp = rmMain.rm_pmdModelData->md_aampAttachedPosition[amo.amo_iAttachedPosition];
  rmMain.rm_pmdModelData->md_aampAttachedPosition.Unlock();

  // copy common values
  rmAttached.rm_vLightDirection = rmMain.rm_vLightDirection;
  rmAttached.rm_fDistanceFactor = rmMain.rm_fDistanceFactor;
  rmAttached.rm_colLight   = rmMain.rm_colLight;
  rmAttached.rm_colAmbient = rmMain.rm_colAmbient;
  rmAttached.rm_colBlend   = rmMain.rm_colBlend;

  // unpack the reference vertices
  FLOAT3D vCenter, vFront, vUp;
  const INDEX iCenter = amp.amp_iCenterVertex;
  const INDEX iFront  = amp.amp_iFrontVertex;
  const INDEX iUp     = amp.amp_iUpVertex;
  UnpackVertex( rmMain, iCenter, vCenter);
  UnpackVertex( rmMain, iFront,  vFront);
  UnpackVertex( rmMain, iUp,     vUp);

  // create front and up direction vectors
  FLOAT3D vY = vUp - vCenter;
  FLOAT3D vZ = vCenter - vFront;
  // project center and directions from object to absolute space
  const FLOATmatrix3D &mO2A = rmMain.rm_mObjectRotation;
  const FLOAT3D &vO2A = rmMain.rm_vObjectPosition;
  vCenter = vCenter*mO2A +vO2A;
  vY = vY *mO2A;
  vZ = vZ *mO2A;

  // make a rotation matrix from the direction vectors
  FLOAT3D vX = vY*vZ;
  vY = vZ*vX;
  vX.Normalize();
  vY.Normalize();
  vZ.Normalize();
  FLOATmatrix3D mOrientation;
  mOrientation(1,1) = vX(1);  mOrientation(1,2) = vY(1);  mOrientation(1,3) = vZ(1);
  mOrientation(2,1) = vX(2);  mOrientation(2,2) = vY(2);  mOrientation(2,3) = vZ(2);
  mOrientation(3,1) = vX(3);  mOrientation(3,2) = vY(3);  mOrientation(3,3) = vZ(3);

  // adjust for relative placement of the attachment
  FLOAT3D vOffset;
  FLOATmatrix3D mRelative;
  MakeRotationMatrixFast( mRelative, amo.amo_plRelative.pl_OrientationAngle);
  vOffset(1) = amo.amo_plRelative.pl_PositionVector(1) * mo_Stretch(1);
  vOffset(2) = amo.amo_plRelative.pl_PositionVector(2) * mo_Stretch(2);
  vOffset(3) = amo.amo_plRelative.pl_PositionVector(3) * mo_Stretch(3);
  FLOAT3D vO = vCenter + vOffset * mOrientation;
  mOrientation *= mRelative; // convert absolute to relative orientation
  rmAttached.SetObjectPlacement( vO, mOrientation);

  // done here if clipping optimizations are not allowed
  extern INDEX gap_iOptimizeClipping;
  if( gap_iOptimizeClipping<1) { 
    gap_iOptimizeClipping = 0;
    _pfModelProfile.StopTimer( CModelProfile::PTI_CREATEATTACHMENT);
    return TRUE;
  }

  // test attachment to frustum and/or mirror
  FLOAT3D vHandle;
  _aprProjection->PreClip( vO, vHandle);
  CalculateBoundingBox( &amo.amo_moModelObject, rmAttached);

  // compose view-space bounding box and sphere of an attacment
  const FLOAT fR = Max( rmAttached.rm_vObjectMinBB.Length(), rmAttached.rm_vObjectMaxBB.Length());
  const FLOATobbox3D boxEntity( FLOATaabbox3D(rmAttached.rm_vObjectMinBB, rmAttached.rm_vObjectMaxBB),
                                vHandle, _aprProjection->pr_ViewerRotationMatrix*mOrientation);
  // frustum test?
  if( gap_iOptimizeClipping>1) {
    // test sphere against frustrum
    INDEX iFrustumTest = _aprProjection->TestSphereToFrustum(vHandle,fR);
    if( iFrustumTest==0) {
      // test box if sphere cut one of frustum planes
      iFrustumTest = _aprProjection->TestBoxToFrustum(boxEntity);
    }
    // mark if attachment is fully inside frustum
         if( iFrustumTest>0) rmAttached.rm_ulFlags |= RMF_INSIDE; 
    else if( iFrustumTest<0) { // if completely outside of frustum
      // signal skip rendering only if doesn't have any attachments
      _pfModelProfile.StopTimer( CModelProfile::PTI_CREATEATTACHMENT);
      return !amo.amo_moModelObject.mo_lhAttachments.IsEmpty(); 
    }
  }
  // test sphere against mirror/warp plane (if any)
  if( _aprProjection->pr_bMirror || _aprProjection->pr_bWarp) {
    INDEX iMirrorPlaneTest;
    const FLOAT fPlaneDistance = _aprProjection->pr_plMirrorView.PointDistance(vHandle);
         if( fPlaneDistance < -fR) iMirrorPlaneTest = -1;
    else if( fPlaneDistance > +fR) iMirrorPlaneTest = +1;
    else { // test box if sphere cut mirror plane
      iMirrorPlaneTest = (INDEX) (boxEntity.TestAgainstPlane(_aprProjection->pr_plMirrorView));
    }
    // mark if attachment is fully inside mirror
         if( iMirrorPlaneTest>0) rmAttached.rm_ulFlags |= RMF_INMIRROR; 
    else if( iMirrorPlaneTest<0) { // if completely outside mirror
      // signal skip rendering only if doesn't have any attachments
      _pfModelProfile.StopTimer( CModelProfile::PTI_CREATEATTACHMENT);
      return !amo.amo_moModelObject.mo_lhAttachments.IsEmpty(); 
    }
  } 
  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_CREATEATTACHMENT);
  return TRUE;
}


//--------------------------------------------------------------------------------------------
/*
 * Render model using preferences given trough _mrpModelRenderPrefs global variable
 */
//--------------------------------------------------------------------------------------------


// transform model to view space
static void PrepareView( CRenderModel &rm) 
{
  // prepare projections
  const FLOATmatrix3D &mViewer = _aprProjection->pr_ViewerRotationMatrix;
  const FLOAT3D &vViewer = _aprProjection->pr_vViewerPosition;
  FLOATmatrix3D &m = rm.rm_mObjectToView;
  // if half face forward
  if( rm.rm_pmdModelData->md_Flags&MF_HALF_FACE_FORWARD) {
    // get the y-axis vector of object rotation
    FLOAT3D vY(rm.rm_mObjectRotation(1,2), rm.rm_mObjectRotation(2,2), rm.rm_mObjectRotation(3,2));
    // find z axis of viewer
    FLOAT3D vViewerZ( mViewer(3,1), mViewer(3,2), mViewer(3,3));
    // calculate x and z axis vectors to make object head towards viewer
    FLOAT3D vX = (-vViewerZ)*vY;
    vX.Normalize();
    FLOAT3D vZ = vY*vX;
    // compose the rotation matrix back from those angles
    m(1,1) = vX(1);  m(1,2) = vY(1);  m(1,3) = vZ(1);
    m(2,1) = vX(2);  m(2,2) = vY(2);  m(2,3) = vZ(2);
    m(3,1) = vX(3);  m(3,2) = vY(3);  m(3,3) = vZ(3);
    // add viewer rotation to that
    m = mViewer * m;
  } // if full face forward
  else if( rm.rm_pmdModelData->md_Flags&MF_FACE_FORWARD) {
    // use just object banking for rotation
    FLOAT fSinP = -rm.rm_mObjectRotation(2,3);
    FLOAT fCosP = Sqrt(1-fSinP*fSinP);
    FLOAT fSinB, fCosB;
    if( fCosP>0.001f) {
      const FLOAT f1oCosP = 1.0f/fCosP;
      fSinB = rm.rm_mObjectRotation(2,1)*f1oCosP;
      fCosB = rm.rm_mObjectRotation(2,2)*f1oCosP;
    } else {
      fSinB = 0.0f;
      fCosB = 1.0f;
    }
    m(1,1) = +fCosB;  m(1,2) = -fSinB;  m(1,3) = 0;
    m(2,1) = +fSinB;  m(2,2) = +fCosB;  m(2,3) = 0;
    m(3,1) =      0;  m(3,2) =      0;  m(3,3) = 1;
  } // if normal model
  else {
    // use viewer and object orientation
    m = mViewer * rm.rm_mObjectRotation;
  }
  // find translation vector
  rm.rm_vObjectToView  = rm.rm_vObjectPosition - vViewer;
  rm.rm_vObjectToView *= mViewer;
}


// render bounding box
static void RenderWireframeBox( FLOAT3D vMinVtx, FLOAT3D vMaxVtx, COLOR col)
{
  // only for OpenGL (for now)
  if( _pGfx->gl_eCurrentAPI!=GAT_OGL) return;

  // prepare wireframe OpenGL settings
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableBlend();
  gfxDisableAlphaTest();
  gfxDisableTexture();
  // fill vertex array so it represents bounding box
  FLOAT3D vBoxVtxs[8];
  vBoxVtxs[0] = FLOAT3D( vMinVtx(1), vMinVtx(2), vMinVtx(3));
  vBoxVtxs[1] = FLOAT3D( vMaxVtx(1), vMinVtx(2), vMinVtx(3));
  vBoxVtxs[2] = FLOAT3D( vMaxVtx(1), vMinVtx(2), vMaxVtx(3));
  vBoxVtxs[3] = FLOAT3D( vMinVtx(1), vMinVtx(2), vMaxVtx(3));
  vBoxVtxs[4] = FLOAT3D( vMinVtx(1), vMaxVtx(2), vMinVtx(3));
  vBoxVtxs[5] = FLOAT3D( vMaxVtx(1), vMaxVtx(2), vMinVtx(3));
  vBoxVtxs[6] = FLOAT3D( vMaxVtx(1), vMaxVtx(2), vMaxVtx(3));
  vBoxVtxs[7] = FLOAT3D( vMinVtx(1), vMaxVtx(2), vMaxVtx(3));
  // connect vertices into lines of bounding box
  INDEX iBoxLines[12][2];
  iBoxLines[ 0][0] = 0;  iBoxLines[ 0][1] = 1;  iBoxLines[ 1][0] = 1;  iBoxLines[ 1][1] = 2;
  iBoxLines[ 2][0] = 2;  iBoxLines[ 2][1] = 3;  iBoxLines[ 3][0] = 3;  iBoxLines[ 3][1] = 0;
  iBoxLines[ 4][0] = 0;  iBoxLines[ 4][1] = 4;  iBoxLines[ 5][0] = 1;  iBoxLines[ 5][1] = 5;
  iBoxLines[ 6][0] = 2;  iBoxLines[ 6][1] = 6;  iBoxLines[ 7][0] = 3;  iBoxLines[ 7][1] = 7;
  iBoxLines[ 8][0] = 4;  iBoxLines[ 8][1] = 5;  iBoxLines[ 9][0] = 5;  iBoxLines[ 9][1] = 6;
  iBoxLines[10][0] = 6;  iBoxLines[10][1] = 7;  iBoxLines[11][0] = 7;  iBoxLines[11][1] = 4;
  // for all vertices in bounding box
  glCOLOR(col);
#ifdef _GLES //karin: no glBegin/glEnd on GLES
  GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
  GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
  GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
  GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
  if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
  if(te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
  if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
  if(ce) pglDisableClientState(GL_COLOR_ARRAY);
  GLfloat vs[12 * 6];
  for( INDEX i=0; i<12; i++) {
    // get starting and ending vertices of one line
    FLOAT3D &v0 = vBoxVtxs[iBoxLines[i][0]];
    FLOAT3D &v1 = vBoxVtxs[iBoxLines[i][1]];
    vs[i * 6] = v0(1);
	vs[i * 6 + 1] = v0(2);
	vs[i * 6 + 2] = v0(3);
    vs[i * 6 + 3] = v1(1);
	vs[i * 6 + 4] = v1(2);
	vs[i * 6 + 5] = v1(3);
  } 
  pglDrawArrays(GL_LINES, 0, 12 * 2);
  if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
  if(te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
  if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
  if(ce) pglEnableClientState(GL_COLOR_ARRAY);
#else
  pglBegin( GL_LINES);
  for( INDEX i=0; i<12; i++) {
    // get starting and ending vertices of one line
    FLOAT3D &v0 = vBoxVtxs[iBoxLines[i][0]];
    FLOAT3D &v1 = vBoxVtxs[iBoxLines[i][1]];
    pglVertex3f( v0(1), v0(2), v0(3));
    pglVertex3f( v1(1), v1(2), v1(3));
  } 
  pglEnd();
#endif
  OGL_CHECKERROR;
}




// setup CRenderModel class for rendering one model and eventually it's shadow(s)
void CModelObject::SetupModelRendering( CRenderModel &rm)
{
  _sfStats.IncrementCounter( CStatForm::SCI_MODELS);
  _pfModelProfile.StartTimer( CModelProfile::PTI_INITMODELRENDERING);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_INITMODELRENDERING);

  // get model's data and lerp info
  rm.rm_pmdModelData = (CModelData*)GetData();
  GetFrame( rm.rm_iFrame0, rm.rm_iFrame1, rm.rm_fRatio);
  const INDEX ctVertices = rm.rm_pmdModelData->md_VerticesCt;
  if( rm.rm_pmdModelData->md_Flags & MF_COMPRESSED_16BIT) {
    // set pFrame to point to last and next frames' vertices
    rm.rm_pFrame16_0 = &rm.rm_pmdModelData->md_FrameVertices16[rm.rm_iFrame0 *ctVertices];
    rm.rm_pFrame16_1 = &rm.rm_pmdModelData->md_FrameVertices16[rm.rm_iFrame1 *ctVertices];
  } else {
    // set pFrame to point to last and next frames' vertices
    rm.rm_pFrame8_0 = &rm.rm_pmdModelData->md_FrameVertices8[rm.rm_iFrame0 *ctVertices];
    rm.rm_pFrame8_1 = &rm.rm_pmdModelData->md_FrameVertices8[rm.rm_iFrame1 *ctVertices];
  }

  // obtain current rendering preferences
  rm.rm_rtRenderType = _mrpModelRenderPrefs.GetRenderType();
  // remember blending color
  rm.rm_colBlend = MulColors( rm.rm_colBlend, mo_colBlendColor);

  // get decompression/stretch factors
  FLOAT3D &vDataStretch = rm.rm_pmdModelData->md_Stretch;
  rm.rm_vStretch(1) = vDataStretch(1) * mo_Stretch(1);
  rm.rm_vStretch(2) = vDataStretch(2) * mo_Stretch(2);
  rm.rm_vStretch(3) = vDataStretch(3) * mo_Stretch(3);
  rm.rm_vOffset     = rm.rm_pmdModelData->md_vCompressedCenter;
  // check if object is inverted (in mirror)
  BOOL bXInverted = rm.rm_vStretch(1) < 0;
  BOOL bYInverted = rm.rm_vStretch(2) < 0;
  BOOL bZInverted = rm.rm_vStretch(3) < 0;
  rm.rm_ulFlags &= ~RMF_INVERTED;
  if( ((bXInverted != bYInverted) != bZInverted) != _aprProjection->pr_bInverted) rm.rm_ulFlags |= RMF_INVERTED;

  // prepare projections
  _pfModelProfile.StartTimer( CModelProfile::PTI_INITPROJECTION);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_INITPROJECTION);
  PrepareView(rm);
  _pfModelProfile.StopTimer( CModelProfile::PTI_INITPROJECTION);

  // get mip factor from projection (if needed)
  if( (INDEX&)rm.rm_fDistanceFactor==12345678) {
    FLOAT3D vObjectAbs;
    _aprProjection->PreClip( rm.rm_vObjectPosition, vObjectAbs);
    rm.rm_fDistanceFactor = _aprProjection->MipFactor( Min(vObjectAbs(3), 0.0f));
  }
  // adjust mip factor in case of dynamic stretch factor
  if( mo_Stretch != FLOAT3D(1,1,1)) {
    rm.rm_fMipFactor = rm.rm_fDistanceFactor - Log2( Max(mo_Stretch(1),Max(mo_Stretch(2),mo_Stretch(3))));
  } else {
    rm.rm_fMipFactor = rm.rm_fDistanceFactor;
  }
  // adjust mip factor by custom settings
  rm.rm_fMipFactor = rm.rm_fMipFactor*mdl_fLODMul +mdl_fLODAdd;

  // get current mip model using mip factor
  rm.rm_iMipLevel = GetMipModel( rm.rm_fMipFactor);
  mo_iLastRenderMipLevel = rm.rm_iMipLevel;
  // get current vertices mask
  rm.rm_pmmiMip = &rm.rm_pmdModelData->md_MipInfos[rm.rm_iMipLevel];

  // don't allow any shading, if shading is turned off
  if( rm.rm_rtRenderType & RT_SHADING_NONE) {
    rm.rm_colAmbient = C_WHITE|CT_OPAQUE;
    rm.rm_colLight   = C_BLACK;
  }

  // calculate light vector as seen from model, so that vertex normals
  // do not need to be transformed for lighting calculations
  FLOAT fLightDirection=(rm.rm_vLightDirection).Length();
  if( fLightDirection>0.001f) {
    rm.rm_vLightDirection /= fLightDirection;
  } else {
    rm.rm_vLightDirection = FLOAT3D(0,0,0);
  }
  rm.rm_vLightObj = rm.rm_vLightDirection * !rm.rm_mObjectRotation;

  // precalculate rendering data if needed
  extern void PrepareModelForRendering( CModelData &md);
  PrepareModelForRendering( *rm.rm_pmdModelData);

  // done with setup if viewing from this model
  if( rm.rm_ulFlags&RMF_SPECTATOR) {
    _pfModelProfile.StopTimer( CModelProfile::PTI_INITMODELRENDERING);
    return;
  }

  _pfModelProfile.StartTimer( CModelProfile::PTI_INITATTACHMENTS);
  // for each attachment on this model object
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_INITATTACHMENTS);
    CAttachmentModelObject *pamo = itamo;
    // create new render model structure
    pamo->amo_prm = &_armRenderModels.Push();
    const BOOL bVisible = CreateAttachment( rm, *pamo);
    if( !bVisible) { // skip if not visible
      pamo->amo_prm = NULL;
      _armRenderModels.Pop();
      continue;
    } // prepare if visible
    _pfModelProfile.StopTimer( CModelProfile::PTI_INITMODELRENDERING);
    _pfModelProfile.StopTimer( CModelProfile::PTI_INITATTACHMENTS);
    pamo->amo_moModelObject.SetupModelRendering( *pamo->amo_prm);
    _pfModelProfile.StartTimer( CModelProfile::PTI_INITATTACHMENTS);
    _pfModelProfile.StartTimer( CModelProfile::PTI_INITMODELRENDERING);
  }
  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_INITATTACHMENTS);
  _pfModelProfile.StopTimer( CModelProfile::PTI_INITMODELRENDERING);
}



// render model
void CModelObject::RenderModel( CRenderModel &rm)
{
  // skip invisible models
  if( mo_Stretch == FLOAT3D(0,0,0)) return;

  _pfModelProfile.StartTimer( CModelProfile::PTI_RENDERMODEL);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_RENDERMODEL);

  // cluster shadows rendering?
  if( _iRenderingType==2) {
    RenderModel_Mask(rm);
    _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERMODEL);
    return;
  }
  ASSERT( _iRenderingType==1);

  // if we should draw polygons and model 
  if( !(rm.rm_ulFlags&RMF_SPECTATOR)   && (!(rm.rm_rtRenderType&RT_NO_POLYGON_FILL)
    || (rm.rm_rtRenderType&RT_WIRE_ON) ||   (rm.rm_rtRenderType&RT_HIDDEN_LINES)) ) {
    // eventually calculate projection model bounding box in object space (needed for fog/haze trivial rejection)
    if( rm.rm_ulFlags&(RMF_FOG|RMF_HAZE)) CalculateBoundingBox( this, rm);
    // render complete model
    rm.SetModelView();
    RenderModel_View(rm);
  }

  // if we should draw current frame bounding box
  if( _mrpModelRenderPrefs.BBoxFrameVisible()) {
    // get min and max coordinates of bounding box
    FLOAT3D vMin = rm.rm_pmdModelData->md_FrameInfos[rm.rm_iFrame0].mfi_Box.Min();
    FLOAT3D vMax = rm.rm_pmdModelData->md_FrameInfos[rm.rm_iFrame0].mfi_Box.Max();
    rm.SetModelView();
    RenderWireframeBox( vMin, vMax, C_dMAGENTA|CT_OPAQUE);
  }

  // if we should draw all frames bounding box
  if( _mrpModelRenderPrefs.BBoxAllVisible()) {
    // calculate all frames bounding box
    FLOATaabbox3D aabbMax;
    for( INDEX i=0; i<rm.rm_pmdModelData->md_FramesCt; i++) {
      aabbMax |= rm.rm_pmdModelData->md_FrameInfos[i].mfi_Box;
    } // pass min and max coordinates of all frames bounding box
    rm.SetModelView();
    RenderWireframeBox( aabbMax.Min(), aabbMax.Max(), C_dGRAY|CT_OPAQUE);
  }

  // render each attachment on this model object
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo)
  { 
    // calculate bounding box of an attachment    
    CAttachmentModelObject *pamo = itamo;
    if( pamo->amo_prm==NULL) continue; // skip view-rejected attachments
    _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERMODEL);
    pamo->amo_moModelObject.RenderModel( *pamo->amo_prm);
    _pfModelProfile.StartTimer( CModelProfile::PTI_RENDERMODEL);
  }
  // done
  _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERMODEL);
}


//--------------------------------------------------------------------------------------------
/*
 * Render shadow of model
 */
//--------------------------------------------------------------------------------------------
void CModelObject::RenderShadow( CRenderModel &rm, const CPlacement3D &plLight,
                                 const FLOAT fFallOff, const FLOAT fHotSpot, const FLOAT fIntensity,
                                 const FLOATplane3D &plShadowPlane)
{
  // if shadows are not rendered for current mip or model is half/full face-forward, do nothing
  if( !HasShadow(rm.rm_iMipLevel)
   || (rm.rm_pmdModelData->md_Flags&(MF_FACE_FORWARD|MF_HALF_FACE_FORWARD))) return;
  ASSERT( _iRenderingType==1);
  ASSERT( fIntensity>=0 && fIntensity<=1);

  _pfModelProfile.StartTimer( CModelProfile::PTI_RENDERSHADOW);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_RENDERSHADOW);
  _sfStats.IncrementCounter( CStatForm::SCI_MODELSHADOWS);

  // call driver function for drawing shadows
  rm.SetModelView();
  RenderShadow_View( rm, plLight, fFallOff, fHotSpot, fIntensity, plShadowPlane);

  // render shadow or each attachment on this model object
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    CAttachmentModelObject *pamo = itamo;
    if( pamo->amo_prm==NULL) continue; // skip view-rejected attachments
    _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERSHADOW);
    pamo->amo_moModelObject.RenderShadow( *pamo->amo_prm, plLight, fFallOff, fHotSpot, fIntensity, plShadowPlane);
    _pfModelProfile.StartTimer( CModelProfile::PTI_RENDERSHADOW);
  }
  _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERSHADOW);
}


// simple shadow rendering
void CModelObject::AddSimpleShadow( CRenderModel &rm, const FLOAT fIntensity, const FLOATplane3D &plShadowPlane)
{
  // if shadows are not rendered for current mip, model is half/full face-forward,
  // intensitiy is too low or projection is not perspective - do nothing!
  if( !HasShadow(rm.rm_iMipLevel) || fIntensity<0.01f || !_aprProjection.IsPerspective()
   || (rm.rm_pmdModelData->md_Flags&(MF_FACE_FORWARD|MF_HALF_FACE_FORWARD))) return;
  ASSERT( _iRenderingType==1);
  ASSERT( fIntensity>0 && fIntensity<=1);
  // do some rendering
  _pfModelProfile.StartTimer( CModelProfile::PTI_RENDERSIMPLESHADOW);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_RENDERSIMPLESHADOW);
  _sfStats.IncrementCounter( CStatForm::SCI_MODELSHADOWS);
  // calculate projection model bounding box in object space (if needed)
  CalculateBoundingBox( this, rm);
  // add one simple shadow to batch list
  AddSimpleShadow_View( rm, fIntensity, plShadowPlane);
  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERSIMPLESHADOW);
}
