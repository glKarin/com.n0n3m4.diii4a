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

#ifndef SE_INCL_RENDER_H
#define SE_INCL_RENDER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif
#include <Engine/Math/Vector.h>
#include <Engine/Math/Quaternion.h>
#include <Engine/Ska/ModelInstance.h>
#include <Engine/Ska/Skeleton.h>
#include <Engine/Ska/Mesh.h>
#include <Engine/World/WorldRayCasting.h>


#define RMF_WIREFRAME           (1UL<<0) // set wireframe on
#define RMF_SHOWTEXTURE         (1UL<<1) // show texture
#define RMF_SHOWNORMALS         (1UL<<2) // show normalas  
#define RMF_SHOWSKELETON        (1UL<<3) // show skeleton
#define RMF_SHOWACTIVEBONES     (1UL<<4) // show active bones

#define SRMF_ATTACHMENT         (1UL<<0) // set for attachment render models
#define SRMF_FOG                (1UL<<1) // render in fog
#define SRMF_HAZE               (1UL<<2) // render in haze
#define SRMF_SPECTATOR          (1UL<<3) // model will not be rendered but shadows might
#define SRMF_INVERTED           (1UL<<4) // stretch is inverted
#define SRMF_BBOXSET            (1UL<<5) // bounding box has been calculated
#define SRMF_INSIDE             (1UL<<6) // doesn't need clipping to frustum
#define SRMF_INMIRROR           (1UL<<7) // doesn't need clipping to mirror/warp plane
#define SRMF_WEAPON             (1UL<<8) // TEMP: weapon model is rendering so don't use ATI's Truform!

typedef FLOAT FLOAT3[3];
// Rendering structures
struct RenModel
{
  CModelInstance *rm_pmiModel;// pointer to model instance
  INDEX rm_iParentModelIndex; // index of parent renmodel
  INDEX rm_iParentBoneIndex;  // index of parent bone this model is attached to
  Matrix12 rm_mTransform;     // Tranform matrix for models without skeletons
  Matrix12 rm_mStrTransform;  // Stretch transform matrix for models without skeleton
  INDEX rm_iSkeletonLODIndex; // index of current skeleton lod
  
  INDEX rm_iFirstBone;        // index if first renbone
  INDEX rm_ctBones;           // renbones count for this renmodel
  INDEX rm_iFirstMesh;        // index of first renmesh
  INDEX rm_ctMeshes;          // meshes count for this renmodel
  INDEX rm_iFirstChildModel;
  INDEX rm_iNextSiblingModel;
};

struct RenBone
{
  SkeletonBone *rb_psbBone;   // pointer to skeleton bone
  INDEX rb_iParentIndex;      // index of parent renbone
  INDEX rb_iRenModelIndex;    // index of renmodel
  AnimPos rb_apPos;
  AnimRot rb_arRot;
  Matrix12 rb_mTransform;     // Transformation matrix for this ren bone
  Matrix12 rb_mStrTransform;  // Stretched transformation matrix for this ren bone
  Matrix12 rb_mBonePlacement; // Placement of bone in absolute space
};

struct RenMorph
{
  MeshMorphMap *rmp_pmmmMorphMap;
  FLOAT rmp_fFactor;
};

struct RenWeight
{
  MeshWeightMap *rw_pwmWeightMap;
  INDEX rw_iBoneIndex;
};

struct RenMesh
{
  struct MeshInstance *rmsh_pMeshInst;
  INDEX rmsh_iRenModelIndex;
  INDEX rmsh_iFirstWeight;
  INDEX rmsh_ctWeights;
  INDEX rmsh_iFirstMorph;
  INDEX rmsh_ctMorphs;
  INDEX rmsh_iMeshLODIndex;           // curent LOD index of msh_aMeshLODs array in Mesh
  BOOL  rmsh_bTransToViewSpace;       // Is mesh transformed to view space
};

// initialize batch model rendering
ENGINE_API void RM_BeginRenderingView(CAnyProjection3D &apr, CDrawPort *pdp);
ENGINE_API void RM_BeginModelRenderingMask( CAnyProjection3D &prProjection,
                                            UBYTE *pubMask, SLONG slMaskWidth, SLONG slMaskHeight);
// cleanup after batch model rendering
ENGINE_API void RM_EndRenderingView( BOOL bRestoreOrtho=TRUE);
ENGINE_API void RM_EndModelRenderingMask(void);

// setup light parameters
ENGINE_API void RM_SetLightColor(COLOR colAmbient, COLOR colLight);
ENGINE_API void RM_SetLightDirection(FLOAT3D &vLightDir);
// LOD factor management
ENGINE_API void RM_SetCurrentDistance(FLOAT fDistFactor);
ENGINE_API FLOAT RM_GetMipFactor(void);
// setup object position
ENGINE_API void RM_SetObjectPlacement(const CPlacement3D &pl);
ENGINE_API void RM_SetObjectPlacement(const FLOATmatrix3D &m, const FLOAT3D &v);
ENGINE_API void RM_SetObjectMatrices(CModelInstance &mi);

// render one SKA model with its children
ENGINE_API void RM_RenderSKA(CModelInstance &mi);
// render one bone in model instance
ENGINE_API void RM_RenderBone(CModelInstance &mi,INDEX iBoneID);
ENGINE_API void RM_RenderColisionBox(CModelInstance &mi,ColisionBox &cb, COLOR col);
// lods
ENGINE_API void RM_SetCustomMeshLodDistance(FLOAT fMeshLod);
ENGINE_API void RM_SetCustomSkeletonLodDistance(FLOAT fSkeletonLod);
ENGINE_API void RM_RenderGround(CTextureObject &to);
// Returns specified renbone
ENGINE_API RenBone *RM_FindRenBone(INDEX iBoneID);
// Returns renbone array and sets renbone count
ENGINE_API RenBone *RM_GetRenBoneArray(INDEX &ctrb);
// Returns true if bone exists and sets two given vectors as start and end point of specified bone
ENGINE_API BOOL RM_GetBoneAbsPosition(CModelInstance &mi,INDEX iBoneID, FLOAT3D &vStartPoint, FLOAT3D &vEndPoint);
// Returns Renbone
ENGINE_API BOOL RM_GetRenBoneAbs(CModelInstance &mi,INDEX iBoneID,RenBone &rb);

ENGINE_API void RM_AddSimpleShadow_View(CModelInstance &mi, const FLOAT fIntensity, const FLOATplane3D &plShadowPlane);

ENGINE_API void RM_GetModelVertices( CModelInstance &mi, CStaticStackArray<FLOAT3D> &avVertices, FLOATmatrix3D &mRotation,
                                     FLOAT3D &vPosition, FLOAT fNormalOffset, FLOAT fDistance);

// test if the ray hit any of model instance's triangles and return 
ENGINE_API FLOAT RM_TestRayCastHit( CModelInstance &mi, FLOATmatrix3D &mRotation, FLOAT3D &vPosition,const FLOAT3D &vOrigin, const FLOAT3D &vTarget,FLOAT fOldDistance,INDEX *piBoneID);

ENGINE_API void RM_SetBoneAdjustCallback(void (*pAdjustBones)(void *pData), void *pData);
ENGINE_API void RM_SetShaderParamsAdjustCallback(void (*pAdjustShaderParams)(void *pData, INDEX iSurfaceID, CShader *pShader,ShaderParams &shParams),void *pData);
// Matrix12 operations
ENGINE_API void Matrix12ToQVect(QVect &qv,const Matrix12 &m12);
ENGINE_API void MatrixVectorToMatrix12(Matrix12 &m12,const FLOATmatrix3D &m, const FLOAT3D &v);
ENGINE_API void Matrix12ToMatrixVector(FLOATmatrix3D &c, FLOAT3D &v, const Matrix12 &m12);

ENGINE_API void QVectToMatrix12(Matrix12 &m12, const QVect &qv);
ENGINE_API void MatrixMultiply(Matrix12 &c,const Matrix12 &m, const Matrix12 &n);
ENGINE_API void MatrixMultiplyCP(Matrix12 &c,const Matrix12 &m, const Matrix12 &n);
ENGINE_API void MatrixTranspose(Matrix12 &r, const Matrix12 &m);
ENGINE_API void TransformVertex(GFXVertex &v, const Matrix12 &m);
ENGINE_API void RotateVector(FLOAT3 &v, const Matrix12 &m);

// model flags
ENGINE_API void RM_SetFlags(ULONG ulNewFlags);
ENGINE_API ULONG RM_GetFlags();
ENGINE_API void RM_AddFlag(ULONG ulFlag);
ENGINE_API void RM_RemoveFlag(ULONG ulFlag);
ENGINE_API ULONG &RM_GetRenderFlags();
ENGINE_API void RM_DoFogAndHaze(BOOL bOpaque);


#endif  /* include-once check. */

