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

#include <Engine/Models/ModelObject.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/RenderModel.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Models/Normals.h>
#include <Engine/Base/Lists.inl>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/StaticStackArray.cpp>

#include <Engine/Templates/Stock_CModelData.h>

extern FLOAT mdl_fLODMul;
extern FLOAT mdl_fLODAdd;
extern const FLOAT *pfSinTable;
extern const FLOAT *pfCosTable;


static BOOL _b16Bit;
static FLOAT _fRatio;
static FLOAT3D _vStretch;
static FLOAT3D _vOffset;
static struct ModelFrameVertex8  *_pFrame8_0;    // ptr to last frame
static struct ModelFrameVertex16 *_pFrame16_0;
static struct ModelFrameVertex8  *_pFrame8_1;    // ptr to next frame
static struct ModelFrameVertex16 *_pFrame16_1;

void UnpackVertex( const INDEX iVertex, FLOAT3D &vVertex)
{
  if( _b16Bit ) {
    // get 16 bit packed vertices
    const SWPOINT3D &vsw0 = _pFrame16_0[iVertex].mfv_SWPoint;
    const SWPOINT3D &vsw1 = _pFrame16_1[iVertex].mfv_SWPoint;
    // convert them to float and lerp between them
    vVertex(1) = (Lerp( (FLOAT)vsw0(1), (FLOAT)vsw1(1), _fRatio) -_vOffset(1)) * _vStretch(1);
    vVertex(2) = (Lerp( (FLOAT)vsw0(2), (FLOAT)vsw1(2), _fRatio) -_vOffset(2)) * _vStretch(2);
    vVertex(3) = (Lerp( (FLOAT)vsw0(3), (FLOAT)vsw1(3), _fRatio) -_vOffset(3)) * _vStretch(3);
  } else {
    // get 8 bit packed vertices
    const SBPOINT3D &vsb0 = _pFrame8_0[iVertex].mfv_SBPoint;
    const SBPOINT3D &vsb1 = _pFrame8_1[iVertex].mfv_SBPoint;
    // convert them to float and lerp between them
    vVertex(1) = (Lerp( (FLOAT)vsb0(1), (FLOAT)vsb1(1), _fRatio) -_vOffset(1)) * _vStretch(1);
    vVertex(2) = (Lerp( (FLOAT)vsb0(2), (FLOAT)vsb1(2), _fRatio) -_vOffset(2)) * _vStretch(2);
    vVertex(3) = (Lerp( (FLOAT)vsb0(3), (FLOAT)vsb1(3), _fRatio) -_vOffset(3)) * _vStretch(3);
  }
}

void CModelObject::GetModelVertices( CStaticStackArray<FLOAT3D> &avVertices, FLOATmatrix3D &mRotation,
                                     FLOAT3D &vPosition, FLOAT fNormalOffset, FLOAT fMipFactor)
{
  FLOAT3D f3dVertex;
  INDEX iFrame0, iFrame1;
  FLOAT fLerpRatio;
  CModelData *pmd = GetData();

  struct ModelFrameVertex16 *pFrame16_0, *pFrame16_1;
  struct ModelFrameVertex8  *pFrame8_0,  *pFrame8_1;

  // get lerp information
  GetFrame( iFrame0, iFrame1, fLerpRatio);

  // set pFrame to point to last and next frames' vertices
  if( pmd->md_Flags & MF_COMPRESSED_16BIT)
  {
    pFrame16_0 = &pmd->md_FrameVertices16[ iFrame0 * pmd->md_VerticesCt];
    pFrame16_1 = &pmd->md_FrameVertices16[ iFrame1 * pmd->md_VerticesCt];
  } else {
    pFrame8_0 = &pmd->md_FrameVertices8[ iFrame0 * pmd->md_VerticesCt];
    pFrame8_1 = &pmd->md_FrameVertices8[ iFrame1 * pmd->md_VerticesCt];
  }

  // Apply stretch factors
  FLOAT3D &vDataStretch = pmd->md_Stretch;
  FLOAT3D &vObjectStretch = mo_Stretch;
  FLOAT3D vStretch, vOffset;
  vStretch(1) = vDataStretch(1)*vObjectStretch(1);
  vStretch(2) = vDataStretch(2)*vObjectStretch(2);
  vStretch(3) = vDataStretch(3)*vObjectStretch(3);
  _vStretch = vStretch;
  _vOffset = vOffset = pmd->md_vCompressedCenter;
  
  // check if object is inverted (in mirror)
  //BOOL bXInverted = vStretch(1)<0;
  //BOOL bYInverted = vStretch(2)<0;
  //BOOL bZInverted = vStretch(3)<0;
  //BOOL bInverted  = bXInverted!=bYInverted!=bZInverted;

  // if dynamic stretch factor should be applied
  if( mo_Stretch != FLOAT3D( 1.0f, 1.0f, 1.0f)) {
    fMipFactor -= Log2(Max(mo_Stretch(1),Max(mo_Stretch(2),mo_Stretch(3))));
  }
  // adjust mip factor by custom settings
  fMipFactor = fMipFactor*mdl_fLODMul+mdl_fLODAdd;

  // get current mip model using mip factor
  INDEX iMipLevel = GetMipModel( fMipFactor);
  // get current vertices mask
  //ULONG ulVtxMask = (1L) << iMipLevel;
  struct ModelMipInfo *pmmiMip = &pmd->md_MipInfos[iMipLevel];

  // allocate space for vertices
  FLOAT3D *pvFirstVtx = avVertices.Push( pmmiMip->mmpi_ctMipVx);

  // Transform a vertex in model with lerping
  if( pmd->md_Flags & MF_COMPRESSED_16BIT) {
    // for each vertex in mip
    for( INDEX iMipVx=0; iMipVx<pmmiMip->mmpi_ctMipVx; iMipVx++) {
      // get destination for unpacking
      INDEX iMdlVx = pmmiMip->mmpi_auwMipToMdl[iMipVx];
      ModelFrameVertex16 &mfv0 = pFrame16_0[iMdlVx];
      ModelFrameVertex16 &mfv1 = pFrame16_1[iMdlVx];
      FLOAT3D &v = *pvFirstVtx;
      v(1) = (Lerp((FLOAT)mfv0.mfv_SWPoint(1), (FLOAT)mfv1.mfv_SWPoint(1), fLerpRatio)-vOffset(1))*vStretch(1);
      v(2) = (Lerp((FLOAT)mfv0.mfv_SWPoint(2), (FLOAT)mfv1.mfv_SWPoint(2), fLerpRatio)-vOffset(2))*vStretch(2);
      v(3) = (Lerp((FLOAT)mfv0.mfv_SWPoint(3), (FLOAT)mfv1.mfv_SWPoint(3), fLerpRatio)-vOffset(3))*vStretch(3);


      FLOAT fSinH = pfSinTable[mfv0.mfv_ubNormH];
      FLOAT fCosH = pfCosTable[mfv0.mfv_ubNormH];
      FLOAT fSinP = pfSinTable[mfv0.mfv_ubNormP];
      FLOAT fCosP = pfCosTable[mfv0.mfv_ubNormP];
      FLOAT fX0 = -fSinH*fCosP;
      FLOAT fY0 = +fSinP;
      FLOAT fZ0 = -fCosH*fCosP;

      fSinH = pfSinTable[mfv1.mfv_ubNormH];
      fCosH = pfCosTable[mfv1.mfv_ubNormH];
      fSinP = pfSinTable[mfv1.mfv_ubNormP];
      fCosP = pfCosTable[mfv1.mfv_ubNormP];
      FLOAT fX1 = -fSinH*fCosP;
      FLOAT fY1 = +fSinP;
      FLOAT fZ1 = -fCosH*fCosP;

      FLOAT3D vNor;
      vNor(1) = Lerp(fX0, fX1, fLerpRatio);
      vNor(2) = Lerp(fY0, fY1, fLerpRatio);
      vNor(3) = Lerp(fZ0, fZ1, fLerpRatio);

      v=(v+vNor*fNormalOffset)*mRotation+vPosition;
      pvFirstVtx++;
    }
  } else {
    // for each vertex in mip
    for( INDEX iMipVx=0; iMipVx<pmmiMip->mmpi_ctMipVx; iMipVx++) {
      // get destination for unpacking
      INDEX iMdlVx = pmmiMip->mmpi_auwMipToMdl[iMipVx];
      // get 16 bit packed vertices
      ModelFrameVertex8 &mfv0 = pFrame8_0[iMdlVx];
      ModelFrameVertex8 &mfv1 = pFrame8_1[iMdlVx];
      FLOAT3D &v = *pvFirstVtx;
      // convert them to float and lerp between them
      v(1) = (Lerp((FLOAT)mfv0.mfv_SBPoint(1), (FLOAT)mfv1.mfv_SBPoint(1), fLerpRatio)-vOffset(1))*vStretch(1);
      v(2) = (Lerp((FLOAT)mfv0.mfv_SBPoint(2), (FLOAT)mfv1.mfv_SBPoint(2), fLerpRatio)-vOffset(2))*vStretch(2);
      v(3) = (Lerp((FLOAT)mfv0.mfv_SBPoint(3), (FLOAT)mfv1.mfv_SBPoint(3), fLerpRatio)-vOffset(3))*vStretch(3);

      FLOAT3D vNor;
      const FLOAT3D &vNormal0 = avGouraudNormals[mfv0.mfv_NormIndex];
      const FLOAT3D &vNormal1 = avGouraudNormals[mfv1.mfv_NormIndex];
      vNor(1) = Lerp((FLOAT)vNormal0(1), (FLOAT)vNormal1(1), fLerpRatio);
      vNor(2) = Lerp((FLOAT)vNormal0(2), (FLOAT)vNormal1(2), fLerpRatio);
      vNor(3) = Lerp((FLOAT)vNormal0(3), (FLOAT)vNormal1(3), fLerpRatio);

      v=(v+vNor*fNormalOffset)*mRotation+vPosition;
      pvFirstVtx++;
    }
  }

  // for each attachment on this model object
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    CAttachmentModelObject *pamo = itamo;
    CModelData *pmd=pamo->amo_moModelObject.GetData();
    ASSERT(pmd!=NULL);
    if(pmd==NULL || (pmd->md_Flags & (MF_FACE_FORWARD|MF_HALF_FACE_FORWARD))) continue;
    FLOATmatrix3D mNew = mRotation;
    FLOAT3D vNew = vPosition;
    // get new rotation and position matrices
    GetAttachmentMatrices(pamo, mNew, vNew);
    // recursion will concate attached model's vertices in absolute space to array
    pamo->amo_moModelObject.GetModelVertices(avVertices, mNew, vNew, fNormalOffset, fMipFactor);
  }
}

void CModelObject::GetAttachmentMatrices( CAttachmentModelObject *pamo, FLOATmatrix3D &mRotation, FLOAT3D &vPosition)
{
  // get the position
  CModelData *pmdMain = (CModelData *)GetData();
  pmdMain ->md_aampAttachedPosition.Lock();
  const CAttachedModelPosition &amp = pmdMain->md_aampAttachedPosition[pamo->amo_iAttachedPosition];
  pmdMain ->md_aampAttachedPosition.Unlock();

  FLOAT3D &vDataStretch = pmdMain->md_Stretch;
  FLOAT3D &vObjectStretch = mo_Stretch;
  _vStretch(1) = vDataStretch(1)*vObjectStretch(1);
  _vStretch(2) = vDataStretch(2)*vObjectStretch(2);
  _vStretch(3) = vDataStretch(3)*vObjectStretch(3);
  _vOffset = pmdMain->md_vCompressedCenter;

  INDEX iFrame0, iFrame1;
  GetFrame( iFrame0, iFrame1, _fRatio);
  const INDEX ctVertices = pmdMain->md_VerticesCt;
  if( pmdMain->md_Flags & MF_COMPRESSED_16BIT) {
    _b16Bit = TRUE;
    // set pFrame to point to last and next frames' vertices
    _pFrame16_0 = &pmdMain->md_FrameVertices16[iFrame0 *ctVertices];
    _pFrame16_1 = &pmdMain->md_FrameVertices16[iFrame1 *ctVertices];
  } else {
    _b16Bit = FALSE;
    // set pFrame to point to last and next frames' vertices
    _pFrame8_0 = &pmdMain->md_FrameVertices8[iFrame0 *ctVertices];
    _pFrame8_1 = &pmdMain->md_FrameVertices8[iFrame1 *ctVertices];
  }

  // unpack the reference vertices
  FLOAT3D vCenter, vFront, vUp;
  const INDEX iCenter = amp.amp_iCenterVertex;
  const INDEX iFront  = amp.amp_iFrontVertex;
  const INDEX iUp     = amp.amp_iUpVertex;
  ::UnpackVertex( iCenter, vCenter);
  ::UnpackVertex( iFront,  vFront);
  ::UnpackVertex( iUp,     vUp);

  // create front and up direction vectors
  FLOAT3D vY = vUp - vCenter;
  FLOAT3D vZ = vCenter - vFront;
  // project center and directions from object to absolute space
  const FLOATmatrix3D &mO2A = mRotation;
  const FLOAT3D &vO2A = vPosition;
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
  MakeRotationMatrixFast( mRelative, pamo->amo_plRelative.pl_OrientationAngle);
  vOffset(1) = pamo->amo_plRelative.pl_PositionVector(1) * mo_Stretch(1);
  vOffset(2) = pamo->amo_plRelative.pl_PositionVector(2) * mo_Stretch(2);
  vOffset(3) = pamo->amo_plRelative.pl_PositionVector(3) * mo_Stretch(3);
  const FLOAT3D vO = vCenter + vOffset * mOrientation;
  mRotation = mOrientation*mRelative;
  vPosition = vO;
}

void CModelObject::GetAttachmentTransformations( INDEX iAttachment, FLOATmatrix3D &mRotation, FLOAT3D &vPosition, BOOL bDummyAttachment)
{
  // get the position
  CModelData *pmdMain = (CModelData *)GetData();
  pmdMain ->md_aampAttachedPosition.Lock();
  const CAttachedModelPosition &amp = pmdMain->md_aampAttachedPosition[iAttachment];
  pmdMain ->md_aampAttachedPosition.Unlock();

  // Apply stretch factors
  FLOAT3D &vDataStretch = pmdMain->md_Stretch;
  FLOAT3D &vObjectStretch = mo_Stretch;
  FLOAT3D vStretch, vOffset;
  vStretch(1) = vDataStretch(1)*vObjectStretch(1);
  vStretch(2) = vDataStretch(2)*vObjectStretch(2);
  vStretch(3) = vDataStretch(3)*vObjectStretch(3);
  _vStretch = vStretch;
  _vOffset = vOffset = pmdMain->md_vCompressedCenter;

  INDEX iFrame0, iFrame1;
  GetFrame( iFrame0, iFrame1, _fRatio);
  const INDEX ctVertices = pmdMain->md_VerticesCt;
  if( pmdMain->md_Flags & MF_COMPRESSED_16BIT) {
    _b16Bit = TRUE;
    // set pFrame to point to last and next frames' vertices
    _pFrame16_0 = &pmdMain->md_FrameVertices16[iFrame0 *ctVertices];
    _pFrame16_1 = &pmdMain->md_FrameVertices16[iFrame1 *ctVertices];
  } else {
    _b16Bit = FALSE;
    // set pFrame to point to last and next frames' vertices
    _pFrame8_0 = &pmdMain->md_FrameVertices8[iFrame0 *ctVertices];
    _pFrame8_1 = &pmdMain->md_FrameVertices8[iFrame1 *ctVertices];
  }

  // unpack the reference vertices
  FLOAT3D vCenter, vFront, vUp;
  const INDEX iCenter = amp.amp_iCenterVertex;
  const INDEX iFront  = amp.amp_iFrontVertex;
  const INDEX iUp     = amp.amp_iUpVertex;
  ::UnpackVertex( iCenter, vCenter);
  ::UnpackVertex( iFront,  vFront);
  ::UnpackVertex( iUp,     vUp);

  // create front and up direction vectors
  FLOAT3D vY = vUp - vCenter;
  FLOAT3D vZ = vCenter - vFront;
  // project center and directions from object to absolute space
  const FLOATmatrix3D &mO2A = mRotation;
  const FLOAT3D &vO2A = vPosition;
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
  if (!bDummyAttachment) {
    CAttachmentModelObject *amo = GetAttachmentModel(iAttachment);
    ASSERT(amo!=NULL);
    FLOATmatrix3D mRelative;
    MakeRotationMatrixFast( mRelative, amo->amo_plRelative.pl_OrientationAngle);
    vOffset(1) = amo->amo_plRelative.pl_PositionVector(1) * mo_Stretch(1);
    vOffset(2) = amo->amo_plRelative.pl_PositionVector(2) * mo_Stretch(2);
    vOffset(3) = amo->amo_plRelative.pl_PositionVector(3) * mo_Stretch(3);
    const FLOAT3D vO = vCenter + vOffset * mOrientation;
    mRotation = mOrientation*mRelative;
    vPosition = vO;
  }
  else
  {
    mRotation = mOrientation;
    vPosition = vCenter;
  }
}
