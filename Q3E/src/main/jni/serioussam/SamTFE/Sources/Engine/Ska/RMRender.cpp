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

#include <Engine/StdH.h>
#include <Engine/Base/Console.h>
#include <Engine/Math/Projection.h>
#include <Engine/Math/Float.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Quaternion.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Clipping.inl>
#include <Engine/Ska/ModelInstance.h>
#include <Engine/Ska/Render.h>
#include <Engine/Ska/Mesh.h>
#include <Engine/Ska/Skeleton.h>
#include <Engine/Ska/AnimSet.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/Fog_internal.h>
#include <Engine/Base/Statistics_Internal.h>

static CAnyProjection3D _aprProjection;
static CDrawPort *_pdp = NULL;
static enum FPUPrecisionType _fpuOldPrecision;
static INDEX _iRenderingType = 0; // 0=none, 1=view, 2=mask

static FLOAT3D _vLightDir;          // Light direction
static FLOAT3D _vLightDirInView;    // Light direction transformed in view space
static COLOR   _colAmbient;         // Ambient color
static COLOR   _colLight;           // Light color
static FLOAT   _fDistanceFactor;    // Distance to object from viewer
static Matrix12 _mObjectToAbs;      // object to absolute
static Matrix12 _mAbsToViewer;      // absolute to viewer
static Matrix12 _mObjToView;        // object to viewer
static Matrix12 _mObjToViewStretch; // object to viewer, stretch by root model instance stretch factor

ULONG _ulFlags = RMF_SHOWTEXTURE;
static ULONG _ulRenFlags = 0;
static FLOAT _fCustomMlodDistance=-1; // custom distance for mesh lods
static FLOAT _fCustomSlodDistance=-1; // custom distance for skeleton lods
extern FLOAT ska_fLODMul;
extern FLOAT ska_fLODAdd;

// mask shader (for rendering models' shadows to shadowmaps)
static CShader _shMaskShader;

// temporary rendering structures
static CStaticStackArray<struct RenModel> _aRenModels;
static CStaticStackArray<struct RenBone> _aRenBones;
static CStaticStackArray<struct RenMesh> _aRenMesh;
static CStaticStackArray<struct RenMorph> _aRenMorph;
static CStaticStackArray<struct RenWeight> _aRenWeights;
static CStaticStackArray<struct MeshVertex> _aMorphedVtxs;
static CStaticStackArray<struct MeshNormal> _aMorphedNormals;
static CStaticStackArray<struct MeshVertex> _aFinalVtxs;
static CStaticStackArray<struct MeshNormal> _aFinalNormals;
static CStaticStackArray<struct GFXColor> _aMeshColors;
static CStaticStackArray<struct GFXTexCoord> _aTexMipFogy;
static CStaticStackArray<struct GFXTexCoord> _aTexMipHazey;

static MeshVertex *_pavFinalVertices = NULL;  // pointer to final arrays
static MeshNormal *_panFinalNormals = NULL;   // pointer to final normals
static INDEX _ctFinalVertices;                // final vertices count
BOOL _bTransformBonelessModelToViewSpace = TRUE; // are boneless models transformed to view space

// Pointers for bone adjustment function
static void (*_pAdjustBonesCallback)(void *pData) = NULL;
static void *_pAdjustBonesData = NULL;
// Pointers for shader params adjustment function
static void (*_pAdjustShaderParams)(void *pData, INDEX iSurfaceID, CShader *pShader,ShaderParams &shParams) = NULL;
static void *_pAdjustShaderData = NULL;

static BOOL FindRenBone(RenModel &rm,int iBoneID,INDEX *piBoneIndex);
static void PrepareMeshForRendering(RenMesh &rmsh, INDEX iSkeletonlod);
static void CalculateRenderingData(CModelInstance &mi);
static void ClearRenArrays();

// load our 3x4 matrix from old-fashioned matrix+vector combination
inline void MatrixVectorToMatrix12(Matrix12 &m12,const FLOATmatrix3D &m, const FLOAT3D &v)
{
  m12[ 0] = m(1,1); m12[ 1] = m(1,2); m12[ 2] = m(1,3); m12[ 3] = v(1); 
  m12[ 4] = m(2,1); m12[ 5] = m(2,2); m12[ 6] = m(2,3); m12[ 7] = v(2); 
  m12[ 8] = m(3,1); m12[ 9] = m(3,2); m12[10] = m(3,3); m12[11] = v(3); 
}

// convert matrix12 to old matrix 3x3 and vector
inline void Matrix12ToMatrixVector(FLOATmatrix3D &c, FLOAT3D &v, const Matrix12 &m12)
{
  c(1,1) = m12[ 0]; c(1,2) = m12[ 1]; c(1,3) = m12[ 2]; v(1) = m12[ 3]; 
  c(2,1) = m12[ 4]; c(2,2) = m12[ 5]; c(2,3) = m12[ 6]; v(2) = m12[ 7]; 
  c(3,1) = m12[ 8]; c(3,2) = m12[ 9]; c(3,3) = m12[10]; v(3) = m12[11]; 
}

// create matrix from vector without rotations
inline static void MakeStretchMatrix(Matrix12 &c, const FLOAT3D &v)
{
  c[ 0] = v(1); c[ 1] = 0.0f; c[ 2] = 0.0f; c[ 3] = 0.0f; 
  c[ 4] = 0.0f; c[ 5] = v(2); c[ 6] = 0.0f; c[ 7] = 0.0f; 
  c[ 8] = 0.0f; c[ 9] = 0.0f; c[10] = v(3); c[11] = 0.0f; 
}

// Remove rotation from matrix (make it front face)
inline static void RemoveRotationFromMatrix(Matrix12 &mat)
{
  mat[ 0] = 1; mat[ 1] = 0; mat[ 2] = 0; 
  mat[ 4] = 0; mat[ 5] = 1; mat[ 6] = 0; 
  mat[ 8] = 0; mat[ 9] = 0; mat[10] = 1;
}

// set given matrix as identity matrix
inline static void MakeIdentityMatrix(Matrix12 &mat)
{
  memset(&mat,0,sizeof(mat));
  mat[0]  = 1;
  mat[5]  = 1;
  mat[10] = 1;
}

// transform vector with given matrix
inline static void TransformVector(FLOAT3 &v, const Matrix12 &m)
{
  float x = v[0];
  float y = v[1];
  float z = v[2];
  v[0] = m[0]*x + m[1]*y + m[ 2]*z + m[ 3];
  v[1] = m[4]*x + m[5]*y + m[ 6]*z + m[ 7];
  v[2] = m[8]*x + m[9]*y + m[10]*z + m[11];
}
inline void TransformVertex(GFXVertex &v, const Matrix12 &m)
{
  float x = v.x;
  float y = v.y;
  float z = v.z;
  v.x = m[0]*x + m[1]*y + m[ 2]*z + m[ 3];
  v.y = m[4]*x + m[5]*y + m[ 6]*z + m[ 7];
  v.z = m[8]*x + m[9]*y + m[10]*z + m[11];
}

// rotate vector with given matrix ( does not translate vector )
inline void RotateVector(FLOAT3 &v, const Matrix12 &m)
{
  float x = v[0];
  float y = v[1];
  float z = v[2];
  v[0] = m[0]*x + m[1]*y + m[ 2]*z;
  v[1] = m[4]*x + m[5]*y + m[ 6]*z;
  v[2] = m[8]*x + m[9]*y + m[10]*z;
}

// copy one matrix12 to another
inline void MatrixCopy(Matrix12 &c, const Matrix12 &m)
{
  memcpy(&c,&m,sizeof(c));
}

// convert 3x4 matrix to QVect 
inline void Matrix12ToQVect(QVect &qv,const Matrix12 &m12)
{
  FLOATmatrix3D m;
  m(1,1) = m12[ 0]; m(1,2) = m12[ 1]; m(1,3) = m12[ 2]; 
  m(2,1) = m12[ 4]; m(2,2) = m12[ 5]; m(2,3) = m12[ 6]; 
  m(3,1) = m12[ 8]; m(3,2) = m12[ 9]; m(3,3) = m12[10]; 
  
  qv.qRot.FromMatrix(m);
  qv.vPos(1) = m12[3];
  qv.vPos(2) = m12[7];
  qv.vPos(3) = m12[11];
}

// covert QVect to matrix 3x4
inline void QVectToMatrix12(Matrix12 &m12, const QVect &qv)
{
  FLOATmatrix3D m;
  qv.qRot.ToMatrix(m);
  MatrixVectorToMatrix12(m12,m,qv.vPos);
}

// concatenate two 3x4 matrices C=(MxN)
inline void MatrixMultiply(Matrix12 &c,const Matrix12 &m, const Matrix12 &n)
{
  c[0] = m[0]*n[0] + m[1]*n[4] + m[2]*n[8];
  c[1] = m[0]*n[1] + m[1]*n[5] + m[2]*n[9];
  c[2] = m[0]*n[2] + m[1]*n[6] + m[2]*n[10];
  c[3] = m[0]*n[3] + m[1]*n[7] + m[2]*n[11] + m[3];

  c[4] = m[4]*n[0] + m[5]*n[4] + m[6]*n[8];
  c[5] = m[4]*n[1] + m[5]*n[5] + m[6]*n[9];
  c[6] = m[4]*n[2] + m[5]*n[6] + m[6]*n[10];
  c[7] = m[4]*n[3] + m[5]*n[7] + m[6]*n[11] + m[7];

  c[8] = m[8]*n[0] + m[9]*n[4] + m[10]*n[8];
  c[9] = m[8]*n[1] + m[9]*n[5] + m[10]*n[9];
  c[10] = m[8]*n[2] + m[9]*n[6] + m[10]*n[10];
  c[11] = m[8]*n[3] + m[9]*n[7] + m[10]*n[11] + m[11];
}

// multiply two matrices into first one
void MatrixMultiplyCP(Matrix12 &c,const Matrix12 &m, const Matrix12 &n)
{
  Matrix12 mTemp;
  MatrixMultiply(mTemp,m,n);
  MatrixCopy(c,mTemp);
}

// make transpose matrix 
inline void MatrixTranspose(Matrix12 &r, const Matrix12 &m)
{
  r[ 0] = m[ 0];
  r[ 5] = m[ 5];
  r[10] = m[10];
  r[ 3] = m[ 3];
  r[ 7] = m[ 7];
  r[11] = m[11];

  r[1] = m[4];
  r[2] = m[8];
  r[4] = m[1];
  r[8] = m[2];
  r[6] = m[9];
  r[9] = m[6];

  r[ 3] = -r[0]*m[3] - r[1]*m[7] - r[ 2]*m[11];
  r[ 7] = -r[4]*m[3] - r[5]*m[7] - r[ 6]*m[11];
  r[11] = -r[8]*m[3] - r[9]*m[7] - r[10]*m[11];
}

// viewer absolute and object space projection
static FLOAT3D _vViewer;
static FLOAT3D _vViewerObj;
static FLOAT3D _vLightObj;
// returns haze/fog value in vertex 
static FLOAT3D _vZDirView, _vHDirView;
static FLOAT   _fFogAddZ, _fFogAddH;
static FLOAT   _fHazeAdd;

// check vertex against fog
static void GetFogMapInVertex( GFXVertex4 &vtx, GFXTexCoord &tex)
{
  const FLOAT fD = vtx.x*_vZDirView(1) + vtx.y*_vZDirView(2) + vtx.z*_vZDirView(3);
  const FLOAT fH = vtx.x*_vHDirView(1) + vtx.y*_vHDirView(2) + vtx.z*_vHDirView(3);
  tex.st.s = (fD+_fFogAddZ) * _fog_fMulZ;
//  tex.st.s = (vtx.z) * _fog_fMulZ;
  tex.st.t = (fH+_fFogAddH) * _fog_fMulH;
}

// check vertex against haze
static void GetHazeMapInVertex( GFXVertex4 &vtx, FLOAT &tx1)
{
  const FLOAT fD = vtx.x*_vViewerObj(1) + vtx.y*_vViewerObj(2) + vtx.z*_vViewerObj(3);
  tx1 = (fD+_fHazeAdd) * _haze_fMul;
}

#if 0 // DG: unused
// check model's bounding box against fog
static BOOL IsModelInFog( FLOAT3D &vMin, FLOAT3D &vMax)
{
  GFXTexCoord tex;
  GFXVertex4  vtx;
  vtx.x=vMin(1); vtx.y=vMin(2); vtx.z=vMin(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMin(2); vtx.z=vMax(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMax(2); vtx.z=vMin(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMax(2); vtx.z=vMax(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMin(2); vtx.z=vMin(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMin(2); vtx.z=vMax(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMax(2); vtx.z=vMin(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMax(2); vtx.z=vMax(3); GetFogMapInVertex(vtx,tex); if(InFog(tex.st.t)) return TRUE;
  return FALSE;
}

// check model's bounding box against haze
static BOOL IsModelInHaze( FLOAT3D &vMin, FLOAT3D &vMax)
{
  FLOAT fS;
  GFXVertex4 vtx; 
  vtx.x=vMin(1); vtx.y=vMin(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMin(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMax(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMax(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMin(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMin(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMax(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMax(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  return FALSE;
}
#endif // 0 (unused)

BOOL PrepareHaze(void)
{
  ULONG &ulRenFlags = RM_GetRenderFlags();
  if( ulRenFlags & SRMF_HAZE) {
    _fHazeAdd  = _haze_hp.hp_fNear;
    _fHazeAdd += -_mObjToView[11];
/*
    // get viewer -z in viewer space
    _vZDirView = FLOAT3D(0,0,-1);
    // get fog direction in viewer space
    // _vHDirView = _fog_vHDirAbs;
    // RotateVector(_vHDirView.vector, _mAbsToViewer);
    _vHDirView = _fog_vHDirView;
    // get viewer offset
    // _fFogAddZ = _vViewer % (rm.rm_vObjectPosition - _aprProjection->pr_vViewerPosition);  // BUG in compiler !!!!
    _fFogAddZ = -_mObjToView[11];
    // get fog offset
    _fFogAddH = _fog_fAddH; // (
      _vHDirView(1)*_mObjToView[3] +
      _vHDirView(2)*_mObjToView[7] +
      _vHDirView(3)*_mObjToView[11]) + _fog_fp.fp_fH3;
      CPrintF("hdir:%g,%g,%g addz:%g addh:%g\n", _vHDirView(1), _vHDirView(2), _vHDirView(3), _fFogAddZ, _fFogAddH);
*/
    return TRUE;
  }
  return FALSE;
}

BOOL PrepareFog(void)
{
  ULONG &ulRenFlags = RM_GetRenderFlags();

  if( ulRenFlags & SRMF_FOG) {
    // get viewer -z in viewer space
    _vZDirView = FLOAT3D(0,0,-1);
    // get fog direction in viewer space
    // _vHDirView = _fog_vHDirAbs;
    // RotateVector(_vHDirView.vector, _mAbsToViewer);
    _vHDirView = _fog_vHDirView;
    // get viewer offset
    // _fFogAddZ = _vViewer % (rm.rm_vObjectPosition - _aprProjection->pr_vViewerPosition);  // BUG in compiler !!!!
    _fFogAddZ = -_mObjToView[11];
    // get fog offset
    _fFogAddH = _fog_fAddH;/*(
      _vHDirView(1)*_mObjToView[3] +
      _vHDirView(2)*_mObjToView[7] +
      _vHDirView(3)*_mObjToView[11]) + _fog_fp.fp_fH3;
      CPrintF("hdir:%g,%g,%g addz:%g addh:%g\n", _vHDirView(1), _vHDirView(2), _vHDirView(3), _fFogAddZ, _fFogAddH);*/
    return TRUE;
  }
  return FALSE;
}

// Update model for fog and haze
void RM_DoFogAndHaze(BOOL bOpaqueSurface)
{
  // get current surface vertex array
  GFXVertex4 *paVertices;
  GFXColor *paColors;
  GFXColor *paHazeColors;
  INDEX ctVertices = shaGetVertexCount();
  
  paVertices = shaGetVertexArray();
  paColors = shaGetColorArray();
  paHazeColors = shaGetNewColorArray();

  // if this is opaque surface
  if(bOpaqueSurface) {
    // 
    if(PrepareFog()) {
      _aTexMipFogy.PopAll();
      _aTexMipFogy.Push(ctVertices);
      // setup tex coords only
      for( INDEX ivtx=0; ivtx<ctVertices; ivtx++) {
        GetFogMapInVertex( paVertices[ivtx], _aTexMipFogy[ivtx]);
      }
      shaSetFogUVMap(&_aTexMipFogy[0]);
    }
    // 
    if(PrepareHaze()) {
      _aTexMipHazey.PopAll();
      _aTexMipHazey.Push(ctVertices);
      const COLOR colH = AdjustColor( _haze_hp.hp_colColor, _slTexHueShift, _slTexSaturation);
      GFXColor colHaze(colH);

      // setup haze tex coords and color
      for( INDEX ivtx=0; ivtx<ctVertices; ivtx++) {
		GetHazeMapInVertex(paVertices[ivtx], _aTexMipHazey[ivtx].st.s);
		_aTexMipHazey[ivtx].st.t = 0.0f;
        paHazeColors[ivtx] = colHaze;
      }
      shaSetHazeUVMap(&_aTexMipHazey[0]);
      shaSetHazeColorArray(&paHazeColors[0]);
    }
  // surface is translucent
  } else {
    // 
    if(PrepareFog()) {
      GFXTexCoord tex;
      for( INDEX ivtx=0; ivtx<ctVertices; ivtx++) {
        GetFogMapInVertex( paVertices[ivtx], tex);
        UBYTE ub = GetFogAlpha(tex) ^255;
        paColors[ivtx].AttenuateA( ub);
      }
    }
    // 
    if(PrepareHaze()) {
      
      FLOAT tx1;
      for( INDEX ivtx=0; ivtx<ctVertices; ivtx++) {
        GetHazeMapInVertex( paVertices[ivtx], tx1);
        FLOAT ub = GetHazeAlpha(tx1) ^255;
        paHazeColors[ivtx] = paColors[ivtx];
        paHazeColors[ivtx].AttenuateA( ub);
      }
      shaSetHazeColorArray(&paHazeColors[0]);
    }
  }
}

// LOD factor management
void RM_SetCurrentDistance(FLOAT fDistFactor)
{
  _fCustomMlodDistance = fDistFactor;
  _fCustomSlodDistance = fDistFactor;
}

FLOAT RM_GetMipFactor(void)
{
  return 0;
}


// fill given array with array of transformed vertices
void RM_GetModelVertices( CModelInstance &mi, CStaticStackArray<FLOAT3D> &avVertices, FLOATmatrix3D &mRotation,
                                     FLOAT3D &vPosition, FLOAT fNormalOffset, FLOAT fDistance)
{
  // Transform all vertices in view space
  BOOL bTemp = _bTransformBonelessModelToViewSpace;
  _bTransformBonelessModelToViewSpace = TRUE;

  // only root model instances
  ASSERT(mi.mi_iParentBoneID==-1);
  // remember parent bone ID
  INDEX iOldParentBoneID = mi.mi_iParentBoneID;
  // set parent bone ID as -1
  mi.mi_iParentBoneID = -1;
  
  // Reset abs to viewer matrix
  MakeIdentityMatrix(_mAbsToViewer);
  RM_SetCurrentDistance(fDistance);
  CalculateRenderingData(mi);

  // for each ren model
  INDEX ctrmsh = _aRenModels.Count();
  for(int irmsh=1;irmsh<ctrmsh;irmsh++) {
    RenModel &rm = _aRenModels[irmsh];
    INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
    // for each mesh in renmodel
    for(int imsh=rm.rm_iFirstMesh;imsh<ctmsh;imsh++) {
      // prepare mesh for rendering
      RenMesh &rmsh = _aRenMesh[imsh];
      PrepareMeshForRendering(rmsh,rm.rm_iSkeletonLODIndex);
      INDEX ctvtx = _ctFinalVertices;
      INDEX ctvtxGiven = avVertices.Count();
      avVertices.Push(ctvtx);
      // for each vertex in prepared mesh
      for(INDEX ivtx=0;ivtx<ctvtx;ivtx++) {
        //#pragma message(">> Fix this")
        FLOAT3D vVtx = FLOAT3D(_pavFinalVertices[ivtx].x,_pavFinalVertices[ivtx].y,_pavFinalVertices[ivtx].z);
        FLOAT3D vNor = FLOAT3D(_panFinalNormals[ivtx].nx,_panFinalNormals[ivtx].ny,_panFinalNormals[ivtx].nz);
        // add vertex to given vertex array
        avVertices[ivtx+ctvtxGiven] = vVtx+(vNor*fNormalOffset);
      }
    }
  }
  // restore old bone parent ID
  mi.mi_iParentBoneID = iOldParentBoneID;
  ClearRenArrays();
  _bTransformBonelessModelToViewSpace = bTemp;
}





FLOAT RM_TestRayCastHit( CModelInstance &mi, FLOATmatrix3D &mRotation, FLOAT3D &vPosition,const FLOAT3D &vOrigin,
                        const FLOAT3D &vTarget,FLOAT fOldDistance,INDEX *piBoneID)
{
	FLOAT fDistance = 1E6f;
	//static int i=0;
	//i++;

	BOOL bTemp = _bTransformBonelessModelToViewSpace;
	_bTransformBonelessModelToViewSpace = TRUE;

	// ASSERT((CProjection3D *)_aprProjection!=NULL);
	RM_SetObjectPlacement(mRotation,vPosition);
	// Reset abs to viewer matrix
	MakeIdentityMatrix(_mAbsToViewer);
  // allways use the first LOD
  RM_SetCurrentDistance(0);
	CalculateRenderingData(mi);
	// for each ren model
	INDEX ctrmsh = _aRenModels.Count();
	for(int irmsh=1;irmsh<ctrmsh;irmsh++) {
		RenModel &rm = _aRenModels[irmsh];
		INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
		// for each mesh in renmodel
		for(int imsh=rm.rm_iFirstMesh;imsh<ctmsh;imsh++) {
			// prepare mesh for rendering
			RenMesh &rmsh = _aRenMesh[imsh];
			PrepareMeshForRendering(rmsh,rm.rm_iSkeletonLODIndex);
			MeshLOD &mshlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
			INDEX ctsurf = mshlod.mlod_aSurfaces.Count();
			for(int isurf=0;isurf<ctsurf;isurf++) {
				MeshSurface &mshsurf = mshlod.mlod_aSurfaces[isurf];
				INDEX cttri = mshsurf.msrf_aTriangles.Count();
				for (int itri=0; itri<cttri;itri++) {
					Vector<FLOAT,3> vVertex0(_pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[0]].x,
  															   _pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[0]].y,
																   _pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[0]].z);

					Vector<FLOAT,3> vVertex1(_pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[1]].x,
					 											   _pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[1]].y,
																   _pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[1]].z);

					Vector<FLOAT,3> vVertex2(_pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[2]].x,
																   _pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[2]].y,
																   _pavFinalVertices[mshsurf.msrf_aTriangles[itri].iVertex[2]].z);

					Plane <float,3> plTriPlane(vVertex0,vVertex1,vVertex2);
					FLOAT fDistance0 = plTriPlane.PointDistance(vOrigin);
					FLOAT fDistance1 = plTriPlane.PointDistance(vTarget);

					// if the ray hits the polygon plane
					if (fDistance0>=0 && fDistance0>=fDistance1) {
						// calculate fraction of line before intersection
						FLOAT fFraction = fDistance0/(fDistance0-fDistance1);
						// calculate intersection coordinate
						FLOAT3D vHitPoint = vOrigin+(vTarget-vOrigin)*fFraction;
						// calculate intersection distance
						FLOAT fHitDistance = (vHitPoint-vOrigin).Length();
						// if the hit point can not be new closest candidate
						if (fHitDistance>fOldDistance) {
							// skip this triangle
							continue;
						}

						// find major axes of the polygon plane
						INDEX iMajorAxis1, iMajorAxis2;
						GetMajorAxesForPlane(plTriPlane, iMajorAxis1, iMajorAxis2);

						// create an intersector
						CIntersector isIntersector(vHitPoint(iMajorAxis1), vHitPoint(iMajorAxis2));


						// check intersections for all three edges of the polygon
						isIntersector.AddEdge(
								vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
								vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
						isIntersector.AddEdge(
								vVertex1(iMajorAxis1), vVertex1(iMajorAxis2),
								vVertex2(iMajorAxis1), vVertex2(iMajorAxis2));
						isIntersector.AddEdge(
								vVertex2(iMajorAxis1), vVertex2(iMajorAxis2),
								vVertex0(iMajorAxis1), vVertex0(iMajorAxis2));



						
						// if the polygon is intersected by the ray, and it is the closest intersection so far
						if (isIntersector.IsIntersecting() && (fHitDistance < fDistance)) {
							// remember hit coordinates
							fDistance = fHitDistance;

							
							// do we neet to find the bone hit by the ray?
							if (piBoneID != NULL) {
								INDEX iClosestVertex;
								// find the vertex closest to the intersection
								FLOAT fDist0 = (vHitPoint - vVertex0).Length();
								FLOAT fDist1 = (vHitPoint - vVertex1).Length();
								FLOAT fDist2 = (vHitPoint - vVertex2).Length();
								if (fDist0 < fDist1) {
									if (fDist0 < fDist2) {
										iClosestVertex = mshsurf.msrf_aTriangles[itri].iVertex[0];
									} else {
										iClosestVertex = mshsurf.msrf_aTriangles[itri].iVertex[2];
									}
								} else {
									if (fDist1 < fDist2) {
										iClosestVertex = mshsurf.msrf_aTriangles[itri].iVertex[1];
									} else {
										iClosestVertex = mshsurf.msrf_aTriangles[itri].iVertex[2];
									}								
								}
							
								// now find the weightmap with the largest weight for this vertex
								INDEX ctwmaps = mshlod.mlod_aWeightMaps.Count();
								FLOAT fMaxVertexWeight = 0.0f;
								INDEX iMaxWeightMap = -1;
								for (int iwmap=0;iwmap<ctwmaps;iwmap++) {
									MeshWeightMap& wtmap = mshlod.mlod_aWeightMaps[iwmap];
									INDEX ctvtx = wtmap.mwm_aVertexWeight.Count();
									for (int ivtx=0;ivtx<ctvtx;ivtx++) {
										if ((wtmap.mwm_aVertexWeight[ivtx].mww_iVertex == iClosestVertex) && (wtmap.mwm_aVertexWeight[ivtx].mww_fWeight > fMaxVertexWeight)) {
											fMaxVertexWeight = wtmap.mwm_aVertexWeight[ivtx].mww_fWeight;
											iMaxWeightMap = wtmap.mwm_iID;
											break;
										}	
									}
								}

								*piBoneID = iMaxWeightMap;
								
							}
						}
					}
				}
			}
		}
	}

	ClearRenArrays();
	_bTransformBonelessModelToViewSpace = bTemp;

	return fDistance;

}


// add simple model shadow
void RM_AddSimpleShadow_View(CModelInstance &mi, const FLOAT fIntensity, const FLOATplane3D &plShadowPlane)
{
  // _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);
  // _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);

  // get viewer in absolute space
  FLOAT3D vViewerAbs = _aprProjection->ViewerPlacementR().pl_PositionVector;
  // if shadow destination plane is not visible, don't cast shadows
  if( plShadowPlane.PointDistance(vViewerAbs)<0.01f) {
    // _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);
    return;
  }

  // _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SIMP_CALC);
  // _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SIMP_CALC);

  // get shadow plane in object space
  FLOATmatrix3D mAbsToObj;
  FLOAT3D vAbsToObj;

  // Fix this
  Matrix12ToMatrixVector(mAbsToObj,vAbsToObj,_mObjectToAbs);
  FLOATplane3D plShadowPlaneObj = (plShadowPlane-vAbsToObj) * !mAbsToObj;

  // project object handle so we can calc how it is far away from viewer
  FLOAT3D vRef = plShadowPlaneObj.ProjectPoint(FLOAT3D(0,0,0));
  TransformVector(vRef.vector,_mObjToViewStretch);
  plShadowPlaneObj.pl_distance += ClampDn( -vRef(3)*0.001f, 0.01f); // move plane towards the viewer a bit to avoid z-fighting

  FLOATaabbox3D box;
  mi.GetCurrentColisionBox( box);
  // find points on plane nearest to bounding box edges
  FLOAT3D vMin = box.Min() * 1.25f;
  FLOAT3D vMax = box.Max() * 1.25f;
  if( _ulRenFlags & SRMF_SPECTATOR) { vMin*=2; vMax*=2; } // enlarge shadow for 1st person view
  FLOAT3D v00 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMin(1),vMin(2),vMin(3)));
  FLOAT3D v01 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMin(1),vMin(2),vMax(3)));
  FLOAT3D v10 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMax(1),vMin(2),vMin(3)));
  FLOAT3D v11 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMax(1),vMin(2),vMax(3)));
  TransformVector(v00.vector,_mObjToViewStretch);
  TransformVector(v01.vector,_mObjToViewStretch);
  TransformVector(v10.vector,_mObjToViewStretch);
  TransformVector(v11.vector,_mObjToViewStretch);

  // calc done
  // _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SIMP_CALC);

  // _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SIMP_COPY);
  // _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SIMP_COPY);

  // prepare color
  ASSERT( fIntensity>=0 && fIntensity<=1);
  ULONG ulAAAA = NormFloatToByte(fIntensity);
  ulAAAA |= (ulAAAA<<8) | (ulAAAA<<16); // alpha isn't needed

  // add to vertex arrays
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  // vertices
  pvtx[0].x = v00(1);  pvtx[0].y = v00(2);  pvtx[0].z = v00(3);
  pvtx[2].x = v11(1);  pvtx[2].y = v11(2);  pvtx[2].z = v11(3);
  if( _ulRenFlags & SRMF_INVERTED) { // must re-adjust order for mirrored projection
    pvtx[1].x = v10(1);  pvtx[1].y = v10(2);  pvtx[1].z = v10(3);
    pvtx[3].x = v01(1);  pvtx[3].y = v01(2);  pvtx[3].z = v01(3);
  } else {
    pvtx[1].x = v01(1);  pvtx[1].y = v01(2);  pvtx[1].z = v01(3);
    pvtx[3].x = v10(1);  pvtx[3].y = v10(2);  pvtx[3].z = v10(3);
  }
  // texture coords
  ptex[0].st.s = 0;  ptex[0].st.t = 0;
  ptex[1].st.s = 0;  ptex[1].st.t = 1;
  ptex[2].st.s = 1;  ptex[2].st.t = 1;
  ptex[3].st.s = 1;  ptex[3].st.t = 0;
  // colors
  pcol[0].ul.abgr = ulAAAA;
  pcol[1].ul.abgr = ulAAAA;
  pcol[2].ul.abgr = ulAAAA;
  pcol[3].ul.abgr = ulAAAA;

  // if this model has fog
  if( _ulRenFlags & SRMF_FOG)
  { // for each vertex in shadow quad
    GFXTexCoord tex;
    for( INDEX i=0; i<4; i++) {
      GFXVertex &vtx = pvtx[i];
      // get distance along viewer axis and fog axis and map to texture and attenuate shadow color
      const FLOAT fH = vtx.x*_fog_vHDirView(1) + vtx.y*_fog_vHDirView(2) + vtx.z*_fog_vHDirView(3);
	  tex.st.s = -vtx.z *_fog_fMulZ;
	  tex.st.t = (fH + _fog_fAddH) *_fog_fMulH;
      pcol[i].AttenuateRGB(GetFogAlpha(tex)^255);
    }
  }
  // if this model has haze
  if( _ulRenFlags & SRMF_HAZE)
  { // for each vertex in shadow quad
    for( INDEX i=0; i<4; i++) {
      // get distance along viewer axis map to texture  and attenuate shadow color
      const FLOAT fS = (_haze_fAdd-pvtx[i].z) *_haze_fMul;
      pcol[i].AttenuateRGB(GetHazeAlpha(fS)^255);
    }
  }

  // one simple shadow added to rendering queue
  // _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SIMP_COPY);
  // _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);
}

// set callback function for bone adjustment
void RM_SetBoneAdjustCallback(void (*pAdjustBones)(void *pData), void *pData)
{
  _pAdjustBonesCallback = pAdjustBones;
  _pAdjustBonesData = pData;
}

void RM_SetShaderParamsAdjustCallback(void (*pAdjustShaderParams)(void *pData, INDEX iSurfaceID,CShader *pShader,ShaderParams &spParams),void *pData)
{
  _pAdjustShaderParams = pAdjustShaderParams;
  _pAdjustShaderData = pData;
}

// show gound for ska studio
void RM_RenderGround(CTextureObject &to)
{
  gfxSetConstantColor(0xFFFFFFFF);
  gfxEnableDepthTest();
  gfxEnableDepthWrite();
  gfxDisableAlphaTest();
  gfxDisableBlend();
  gfxCullFace(GFX_NONE);
  CTextureData *ptd = (CTextureData *)to.GetData();
  ptd->SetAsCurrent();

  FLOAT3D vVtx = FLOAT3D(45,0,45);

  GFXVertex vBoxVtxs[4];
  GFXTexCoord tcBoxTex[4];
  INDEX_T aiIndices[6] = {0, 2, 1, 0 ,3 ,2};

  // set ground vertices
  vBoxVtxs[0].x =  vVtx(1); vBoxVtxs[0].y =  vVtx(2); vBoxVtxs[0].z = -vVtx(3);
  vBoxVtxs[1].x = -vVtx(1); vBoxVtxs[1].y =  vVtx(2); vBoxVtxs[1].z = -vVtx(3);
  vBoxVtxs[2].x = -vVtx(1); vBoxVtxs[2].y =  vVtx(2); vBoxVtxs[2].z =  vVtx(3);
  vBoxVtxs[3].x =  vVtx(1); vBoxVtxs[3].y =  vVtx(2); vBoxVtxs[3].z =  vVtx(3);
  // set ground texcoords
  tcBoxTex[0].uv.u = vVtx(1); tcBoxTex[0].uv.v = 0;
  tcBoxTex[1].uv.u = 0; tcBoxTex[1].uv.v = 0;
  tcBoxTex[2].uv.u = 0; tcBoxTex[2].uv.v = vVtx(3);
  tcBoxTex[3].uv.u = vVtx(1); tcBoxTex[3].uv.v = vVtx(3);

  for(INDEX ivx=0;ivx<4;ivx++) {
    TransformVertex(vBoxVtxs[ivx],_mAbsToViewer);
  }
  /*aiIndices[0] = 0; aiIndices[1] = 2; aiIndices[2] = 1;
  aiIndices[3] = 0; aiIndices[4] = 3; aiIndices[5] = 2;*/

  gfxSetVertexArray(vBoxVtxs,4);
  gfxSetTexCoordArray(tcBoxTex, FALSE);
  gfxDrawElements(6,aiIndices);
}

// render wirerame bounding box
static void RenderWireframeBox(FLOAT3D vMinVtx, FLOAT3D vMaxVtx, COLOR col)
{
  // prepare wireframe settings
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

  for(INDEX iwx=0;iwx<8;iwx++) TransformVector(vBoxVtxs[iwx].vector,_mObjToViewStretch);

  // connect vertices into lines of bounding box
  INDEX iBoxLines[12][2];
  iBoxLines[ 0][0] = 0;  iBoxLines[ 0][1] = 1;  iBoxLines[ 1][0] = 1;  iBoxLines[ 1][1] = 2;
  iBoxLines[ 2][0] = 2;  iBoxLines[ 2][1] = 3;  iBoxLines[ 3][0] = 3;  iBoxLines[ 3][1] = 0;
  iBoxLines[ 4][0] = 0;  iBoxLines[ 4][1] = 4;  iBoxLines[ 5][0] = 1;  iBoxLines[ 5][1] = 5;
  iBoxLines[ 6][0] = 2;  iBoxLines[ 6][1] = 6;  iBoxLines[ 7][0] = 3;  iBoxLines[ 7][1] = 7;
  iBoxLines[ 8][0] = 4;  iBoxLines[ 8][1] = 5;  iBoxLines[ 9][0] = 5;  iBoxLines[ 9][1] = 6;
  iBoxLines[10][0] = 6;  iBoxLines[10][1] = 7;  iBoxLines[11][0] = 7;  iBoxLines[11][1] = 4;
  // for all vertices in bounding box
  for( INDEX i=0; i<12; i++) {
    // get starting and ending vertices of one line
    FLOAT3D &v0 = vBoxVtxs[iBoxLines[i][0]];
    FLOAT3D &v1 = vBoxVtxs[iBoxLines[i][1]];
    _pdp->DrawLine3D(v0,v1,col);
  } 
}

// render bounding box
static void RenderBox(FLOAT3D vMinVtx, FLOAT3D vMaxVtx, COLOR col)
{
  // prepare settings
  gfxDisableTexture();
  gfxEnableBlend();
  gfxBlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxCullFace(GFX_NONE);
  gfxDisableDepthWrite();

  gfxSetConstantColor(col);
  // fill vertex array so it represents bounding box
  GFXVertex vBoxVtxs[8];
  vBoxVtxs[0].x = vMinVtx(1); vBoxVtxs[0].y = vMaxVtx(2); vBoxVtxs[0].z = vMinVtx(3);
  vBoxVtxs[1].x = vMinVtx(1); vBoxVtxs[1].y = vMaxVtx(2); vBoxVtxs[1].z = vMaxVtx(3);
  vBoxVtxs[2].x = vMaxVtx(1); vBoxVtxs[2].y = vMaxVtx(2); vBoxVtxs[2].z = vMinVtx(3);
  vBoxVtxs[3].x = vMaxVtx(1); vBoxVtxs[3].y = vMaxVtx(2); vBoxVtxs[3].z = vMaxVtx(3);

  vBoxVtxs[4].x = vMinVtx(1); vBoxVtxs[4].y = vMinVtx(2); vBoxVtxs[4].z = vMinVtx(3);
  vBoxVtxs[5].x = vMinVtx(1); vBoxVtxs[5].y = vMinVtx(2); vBoxVtxs[5].z = vMaxVtx(3);
  vBoxVtxs[6].x = vMaxVtx(1); vBoxVtxs[6].y = vMinVtx(2); vBoxVtxs[6].z = vMinVtx(3);
  vBoxVtxs[7].x = vMaxVtx(1); vBoxVtxs[7].y = vMinVtx(2); vBoxVtxs[7].z = vMaxVtx(3);

  for(INDEX iwx=0;iwx<8;iwx++) {
    TransformVertex(vBoxVtxs[iwx],_mObjToViewStretch);
  }
  INDEX_T aiIndices[36] = { 
    0, 3, 1,
    0, 2, 3,
    5, 1, 3,
    7, 5, 3,
    2, 7, 3,
    6, 7, 2,
    4, 2, 0,
    4, 6, 2,
    5, 0, 1,
    5, 4, 0,
    4, 5, 7,
    6, 4, 7
  };
  /*aiIndices[ 0] = 0; aiIndices[ 1] = 3; aiIndices[ 2] = 1;
  aiIndices[ 3] = 0; aiIndices[ 4] = 2; aiIndices[ 5] = 3;
  aiIndices[ 6] = 5; aiIndices[ 7] = 1; aiIndices[ 8] = 3;
  aiIndices[ 9] = 7; aiIndices[10] = 5; aiIndices[11] = 3;
  aiIndices[12] = 2; aiIndices[13] = 7; aiIndices[14] = 3;
  aiIndices[15] = 6; aiIndices[16] = 7; aiIndices[17] = 2;
  aiIndices[18] = 4; aiIndices[19] = 2; aiIndices[20] = 0;
  aiIndices[21] = 4; aiIndices[22] = 6; aiIndices[23] = 2;
  aiIndices[24] = 5; aiIndices[25] = 0; aiIndices[26] = 1;
  aiIndices[27] = 5; aiIndices[28] = 4; aiIndices[29] = 0;
  aiIndices[30] = 4; aiIndices[31] = 5; aiIndices[32] = 7;
  aiIndices[33] = 6; aiIndices[34] = 4; aiIndices[35] = 7;*/

  gfxSetVertexArray(vBoxVtxs,8);
  gfxDrawElements(36,aiIndices);

  gfxDisableBlend();
  gfxEnableDepthTest();

  RenderWireframeBox(vMinVtx,vMaxVtx,C_BLACK|CT_OPAQUE);
  gfxEnableDepthWrite();
  gfxDisableDepthBias();
}

// render bounding box on screen
void RM_RenderColisionBox(CModelInstance &mi,ColisionBox &cb, COLOR col)
{
  //ColisionBox &cb = mi.GetColisionBox(icb);
  gfxSetViewMatrix(NULL);
  if(RM_GetFlags() & RMF_WIREFRAME) {
    RenderWireframeBox(cb.Min(),cb.Max(),col|CT_OPAQUE);
  } else {
    gfxEnableBlend();
    gfxBlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    RenderBox(cb.Min(),cb.Max(),col|0x7F);
    gfxDisableBlend();
  }
}

// draw wireframe mesh on screen
static void RenderMeshWireframe(RenMesh &rmsh)
{
  MeshLOD &mlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
  // count surfaces in mesh
  INDEX ctsrf = mlod.mlod_aSurfaces.Count();
  // for each surface
  for(INDEX isrf=0; isrf<ctsrf; isrf++)
  {
    MeshSurface &msrf = mlod.mlod_aSurfaces[isrf];
    COLOR colErrColor = 0xCDCDCDFF;
    // surface has no shader, just show vertices
    shaClean();
    shaSetVertexArray((GFXVertex4*)&_pavFinalVertices[msrf.msrf_iFirstVertex],msrf.msrf_ctVertices);
    shaSetIndices(&msrf.msrf_aTriangles[0].iVertex[0],msrf.msrf_aTriangles.Count()*3);
    shaSetTexture(-1);
    shaSetColorArray(&colErrColor,1);
    shaSetColor(0);
    shaDisableBlend();
    shaRender();
    shaClean();
  }
}

// render model wireframe
static void RenderModelWireframe(RenModel &rm)
{
  INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
  // for each mesh in renmodel
  for(int imsh=rm.rm_iFirstMesh;imsh<ctmsh;imsh++) {
    // render mesh
    RenMesh &rmsh = _aRenMesh[imsh];
    PrepareMeshForRendering(rmsh,rm.rm_iSkeletonLODIndex);
    RenderMeshWireframe(rmsh);
  }
}

// render normals
static void RenderNormals()
{
  // only if rendering to view
  if( _iRenderingType!=1) return;

  gfxDisableTexture();
  INDEX ctNormals = _aFinalNormals.Count();
  for(INDEX ivx=0;ivx<ctNormals;ivx++)
  {
    FLOAT3D vNormal = FLOAT3D(_panFinalNormals[ivx].nx,_panFinalNormals[ivx].ny,_panFinalNormals[ivx].nz);
    // vNormal.Normalize();
    FLOAT3D vVtx1 = FLOAT3D(_pavFinalVertices[ivx].x,_pavFinalVertices[ivx].y,_pavFinalVertices[ivx].z);
    FLOAT3D vVtx2 = vVtx1 + (vNormal/5);
    _pdp->DrawLine3D(vVtx1,vVtx2,0xFFFFFFFF);
  }
}

// render one renbone
static void RenderBone(RenBone &rb, COLOR col)
{
  FLOAT fSize = rb.rb_psbBone->sb_fBoneLength / 20;
  FLOAT3D vBoneStart = FLOAT3D(rb.rb_mBonePlacement[3],rb.rb_mBonePlacement[7],rb.rb_mBonePlacement[11]);
  FLOAT3D vBoneEnd = FLOAT3D(0,0,-rb.rb_psbBone->sb_fBoneLength);
  FLOAT3D vRingPt[4];
  
  vRingPt[0] = FLOAT3D(-fSize,-fSize,-fSize*2);
  vRingPt[1] = FLOAT3D( fSize,-fSize,-fSize*2);
  vRingPt[2] = FLOAT3D( fSize, fSize,-fSize*2);
  vRingPt[3] = FLOAT3D(-fSize, fSize,-fSize*2);
  TransformVector(vBoneEnd.vector,rb.rb_mBonePlacement);
  TransformVector(vRingPt[0].vector,rb.rb_mBonePlacement);
  TransformVector(vRingPt[1].vector,rb.rb_mBonePlacement);
  TransformVector(vRingPt[2].vector,rb.rb_mBonePlacement);
  TransformVector(vRingPt[3].vector,rb.rb_mBonePlacement);

  // connect start point of bone with end point
  INDEX il;
  for(il=0;il<4;il++) {
    _pdp->DrawLine3D(vBoneStart,vRingPt[il],col);
    _pdp->DrawLine3D(vBoneEnd,vRingPt[il],col);
  }

  // draw ring
  for(il=0;il<3;il++) {
    _pdp->DrawLine3D(vRingPt[il],vRingPt[il+1],col);
  }
  _pdp->DrawLine3D(vRingPt[0],vRingPt[3],col);
}

// render one bone in model instance
void RM_RenderBone(CModelInstance &mi,INDEX iBoneID)
{
  UBYTE ubFillColor = 127;
  CStaticStackArray<INDEX> aiRenModelIndices;
  CStaticStackArray<INDEX> aiRenMeshIndices;

  CalculateRenderingData(mi);

  gfxEnableBlend();
  gfxEnableDepthTest();

  INDEX iBoneIndex = -1; // index of selected bone in renbone array
  INDEX iWeightIndex = -1; // index of weight that have same id as bone
  
  // find all renmeshes that uses this bone weightmap
  INDEX ctrm = _aRenModels.Count();
  // for each renmodel
  for(INDEX irm=1;irm<ctrm;irm++) {
    RenModel &rm = _aRenModels[irm];
    // try to find bone in this renmodel
    if(FindRenBone(rm,iBoneID,&iBoneIndex)) {
      // for each renmesh in rm
      INDEX ctmsh = rm.rm_iFirstMesh+rm.rm_ctMeshes;
      for(INDEX imsh=rm.rm_iFirstMesh;imsh<ctmsh;imsh++) {
        RenMesh &rm = _aRenMesh[imsh];
        // for each weightmap in this renmesh
        INDEX ctwm = rm.rmsh_iFirstWeight+rm.rmsh_ctWeights;
        for(INDEX iwm=rm.rmsh_iFirstWeight;iwm<ctwm;iwm++) {
          RenWeight &rw = _aRenWeights[iwm];
          // if weight map id is same as bone id
          if(rw.rw_pwmWeightMap->mwm_iID == iBoneID) {
            INDEX &irmi = aiRenModelIndices.Push();
            INDEX &irmshi = aiRenMeshIndices.Push();
            // rememeber this weight map 
            irmi = irm;
            irmshi = imsh;
            iWeightIndex = iwm;
          }
        }
      }
    }
  }

  // if weightmap is found
  if(iWeightIndex>=0) {
    // show wertex weights for each mesh that uses this bones weightmap
    INDEX ctmshi=aiRenMeshIndices.Count();
    for(INDEX imshi=0;imshi<ctmshi;imshi++)
    {
      INDEX iMeshIndex = aiRenMeshIndices[imshi]; // index of mesh that uses selected bone
      INDEX iModelIndex = aiRenModelIndices[imshi]; // index of model in witch is mesh
      RenModel &rm = _aRenModels[iModelIndex];
      RenMesh &rmsh = _aRenMesh[iMeshIndex];
      MeshLOD &mlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
      
      // Create array of color
      INDEX ctVertices = mlod.mlod_aVertices.Count();
      _aMeshColors.PopAll();
      _aMeshColors.Push(ctVertices);
      memset(&_aMeshColors[0],ubFillColor,sizeof(_aMeshColors[0])*ctVertices);
      // prepare this mesh for rendering
      PrepareMeshForRendering(rmsh,rm.rm_iSkeletonLODIndex);

      // all vertices by default are not visible ( have alpha set to 0 )
      for(INDEX ivx=0;ivx<ctVertices;ivx++) {
	    _aMeshColors[ivx].ub.a = 0;
      }
    
      INDEX ctwm = rmsh.rmsh_iFirstWeight+rmsh.rmsh_ctWeights;
      // for each weightmap in this mesh
      for(INDEX irw=rmsh.rmsh_iFirstWeight;irw<ctwm;irw++) {
        RenWeight &rw = _aRenWeights[irw];
        if(rw.rw_iBoneIndex != iBoneIndex) continue;
        INDEX ctvw = rw.rw_pwmWeightMap->mwm_aVertexWeight.Count();
        // for each vertex in this veight
        for(int ivw=0; ivw<ctvw; ivw++)
        {
          // modify color and alpha value of this vertex 
          MeshVertexWeight &vw = rw.rw_pwmWeightMap->mwm_aVertexWeight[ivw];
          INDEX ivx = vw.mww_iVertex;
		  _aMeshColors[ivx].ub.r = 255;
		  _aMeshColors[ivx].ub.g = 127;
		  _aMeshColors[ivx].ub.b = 0;
		  _aMeshColors[ivx].ub.a += (UBYTE)(vw.mww_fWeight * 255); // _aMeshColors[ivx].ub.a = 255;
        }
      }

      // count surfaces in mesh
      INDEX ctsrf = mlod.mlod_aSurfaces.Count();
      // for each surface
      for(INDEX isrf=0; isrf<ctsrf; isrf++) {
        MeshSurface &msrf = mlod.mlod_aSurfaces[isrf];
        shaSetVertexArray((GFXVertex4*)&_pavFinalVertices[msrf.msrf_iFirstVertex],msrf.msrf_ctVertices);
        shaSetNormalArray((GFXNormal*)&_panFinalNormals[msrf.msrf_iFirstVertex]);
        shaSetIndices(&msrf.msrf_aTriangles[0].iVertex[0],msrf.msrf_aTriangles.Count()*3);
        shaSetTexture(-1);
        shaCalculateLight();
        GFXColor *paColors = shaGetColorArray();
        // replace current color array with weight color array
        memcpy(paColors,&_aMeshColors[msrf.msrf_iFirstVertex],sizeof(COLOR)*msrf.msrf_ctVertices);
        shaEnableBlend();
        shaBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
        // render surface
        shaRender();
        shaClean();
      }
    }
  }
  
  // draw bone
  if(iBoneIndex>=0) {
    gfxSetViewMatrix(NULL);
    gfxDisableDepthTest();
    // show bone in yellow color
    RenderBone(_aRenBones[iBoneIndex],0xFFFF00FF);
  }

  gfxDisableBlend();
  aiRenModelIndices.Clear();
  aiRenMeshIndices.Clear();     
  ClearRenArrays();
}

// render skeleton hierarchy
static void RenderSkeleton(void)
{
  gfxSetViewMatrix(NULL);
  // for each bone, except the dummy one
  for(int irb=1; irb<_aRenBones.Count(); irb++)
  {
    RenBone &rb = _aRenBones[irb];
    RenderBone(rb,0x5A5ADCFF); // render in blue color
  }
}

static void RenderActiveBones(RenModel &rm)
{
  CModelInstance *pmi = rm.rm_pmiModel;
  if(pmi==NULL) return;
  // count animlists
  INDEX ctal = pmi->mi_aqAnims.aq_Lists.Count();
  // find newes animlist that has fully faded in
  INDEX iFirstAnimList = 0;
  // loop from newer to older
  INDEX ial;
  for(ial=ctal-1;ial>=0;ial--) {
    AnimList &alList = pmi->mi_aqAnims.aq_Lists[ial];
    // calculate fade factor
    FLOAT fFadeFactor = CalculateFadeFactor(alList);
    if(fFadeFactor >= 1.0f) {
      iFirstAnimList = ial;
      break;
    }
  }
  // for each anim list after iFirstAnimList
  for(ial=iFirstAnimList;ial<ctal;ial++) {
    AnimList &alList = pmi->mi_aqAnims.aq_Lists[ial];
    INDEX ctpa = alList.al_PlayedAnims.Count();
    // for each played anim
    for(INDEX ipa=0;ipa<ctpa;ipa++) {
      PlayedAnim &pa = alList.al_PlayedAnims[ipa];
      INDEX iAnimSet,iAnimIndex;
      pmi->FindAnimationByID(pa.pa_iAnimID,&iAnimSet,&iAnimIndex);
      CAnimSet &as = pmi->mi_aAnimSet[iAnimSet];
      Animation &an = as.as_Anims[iAnimIndex];
      INDEX ctbe = an.an_abeBones.Count();
      // for each bone envelope
      for(INDEX ibe=0;ibe<ctbe;ibe++) {
        BoneEnvelope &be = an.an_abeBones[ibe];
        INDEX iBoneIndex = 0;
        // try to find renbone for this bone envelope
        if(FindRenBone(rm,be.be_iBoneID,&iBoneIndex)) {
          RenBone &rb = _aRenBones[iBoneIndex];
          // render bone
          RenderBone(rb,0x00FF00FF);
        }
      }

    }
  }
}

static void RenderActiveBones(void)
{
  gfxSetViewMatrix(NULL);
  // for each renmodel
  INDEX ctrm = _aRenModels.Count();
  for(SLONG irm=0;irm<ctrm;irm++) {
    RenModel &rm = _aRenModels[irm];
    RenderActiveBones(rm);
  }
}



// get render flags for model
ULONG &RM_GetRenderFlags()
{
  return _ulRenFlags;
}
// set new flag
void RM_SetFlags(ULONG ulNewFlags)
{
  _ulFlags = ulNewFlags;
}
// get curent flags
ULONG RM_GetFlags()
{
  return _ulFlags;
}
// add flag
void RM_AddFlag(ULONG ulFlag)
{
   _ulFlags |= ulFlag;
}
// remove flag
void RM_RemoveFlag(ULONG ulFlag)
{
  _ulFlags &= ~ulFlag;
}

// find texture data id 

static void FindTextureData(CTextureObject **ptoTextures, INDEX iTextureID, MeshInstance &mshi)
{
  // for each texture instances
  INDEX ctti=mshi.mi_tiTextures.Count();
  for(INDEX iti=0;iti<ctti;iti++)
  {
    TextureInstance &ti = mshi.mi_tiTextures[iti];
    if(ti.GetID() == iTextureID)
    {
      *ptoTextures = &ti.ti_toTexture;
      return;
    }
  }
  *ptoTextures = NULL;
}

//  find frame (binary) index in compresed array of rotations, positions or opt_rotations
static INDEX FindFrame(UBYTE *pFirstMember, INDEX iFind, INDEX ctfn, UINT uiSize)
{
  INDEX iHigh = ctfn-1;
  INDEX iLow = 0;
  INDEX iMid;

  UWORD iHighFrameNum = *(UWORD*)(pFirstMember+(uiSize*iHigh));
  if(iFind == iHighFrameNum) return iHigh;

  while(TRUE) {
    iMid = (iHigh+iLow)/2;
    UWORD iMidFrameNum = *(UWORD*)(pFirstMember+(uiSize*iMid));
    UWORD iMidFrameNumPlusOne = *(UWORD*)(pFirstMember+(uiSize*(iMid+1)));
    if(iFind < iMidFrameNum) iHigh = iMid;
    else if((iMid == iHigh) || (iMidFrameNumPlusOne > iFind)) return iMid;
    else iLow = iMid;
  }
}

// Find renbone in given renmodel
static BOOL FindRenBone(RenModel &rm,int iBoneID,INDEX *piBoneIndex)
{
  int ctb = rm.rm_iFirstBone + rm.rm_ctBones;
  // for each renbone in this ren model
  for(int ib=rm.rm_iFirstBone;ib<ctb;ib++) {
    // if bone id's match 
    if(iBoneID == _aRenBones[ib].rb_psbBone->sb_iID) {
      // return index of this renbone
      *piBoneIndex = ib;
      return TRUE;
    }
  }
  return FALSE;
}

// Find renbone in whole array on renbones
RenBone *RM_FindRenBone(INDEX iBoneID)
{
  INDEX ctrb=_aRenBones.Count();
  // for each renbone
  for(INDEX irb=1;irb<ctrb;irb++) {
    RenBone &rb = _aRenBones[irb];
    // if bone id's match
    if(rb.rb_psbBone->sb_iID == iBoneID) {
      // return this renbone
      return &rb;
    }
  }
  return NULL;
}

// Return array of renbones
RenBone *RM_GetRenBoneArray(INDEX &ctrb)
{
  ctrb = _aRenBones.Count();
  if(ctrb>0) {
    return &_aRenBones[0];
  } else {
    return NULL;
  }
}

// find renmoph in given renmodel
static BOOL FindRenMorph(RenModel &rm,int iMorphID,INDEX *piMorphIndex)
{
  // for each renmesh in given renmodel
  INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
  for(INDEX irmsh=rm.rm_iFirstMesh;irmsh<ctmsh;irmsh++) {
    // for each renmorph in this renmesh
    INDEX ctmm = _aRenMesh[irmsh].rmsh_iFirstMorph + _aRenMesh[irmsh].rmsh_ctMorphs;
    for(INDEX imm=_aRenMesh[irmsh].rmsh_iFirstMorph;imm<ctmm;imm++) {
      // if id's match
      if(iMorphID == _aRenMorph[imm].rmp_pmmmMorphMap->mmp_iID) {
        // return this renmorph
        *piMorphIndex = imm;
        return TRUE;
      }
    }
  }
  // renmorph was not found
  return FALSE;
}

// Find bone by ID (bone index must be set!)
static BOOL FindBone(int iBoneID, INDEX *piBoneIndex, CModelInstance *pmi,INDEX iSkeletonLod)
{
  // if model instance does not have skeleton
  if(pmi->mi_psklSkeleton == NULL) return FALSE;
  // if current skeleton lod is invalid
  if(iSkeletonLod < 0) return FALSE;

  INDEX ctslods = pmi->mi_psklSkeleton->skl_aSkeletonLODs.Count();
  // if skeleton lods count is invalid
  if(ctslods<1) return FALSE;
  // if skeleton lod is larger than lod count
  if(iSkeletonLod >= ctslods) {
    // use skeleton finest skeleton lod
    //#pragma message(">> Check if this is ok")
    iSkeletonLod = 0;
    ASSERT(FALSE);
  }

  SkeletonLOD &slod = pmi->mi_psklSkeleton->skl_aSkeletonLODs[iSkeletonLod];
  // for each bone in skeleton lod
  for(int i=0;i<slod.slod_aBones.Count();i++) {
    // check if bone id's match
    if(iBoneID == slod.slod_aBones[i].sb_iID) {
      // bone index is allready set just return true
      return TRUE;
    }
    *piBoneIndex += 1;
  }

  // for each child of given model instance
  INDEX ctmich = pmi->mi_cmiChildren.Count();
  for(INDEX imich =0;imich<ctmich;imich++) {
    // try to find bone in child model instance
    if(FindBone(iBoneID,piBoneIndex,&pmi->mi_cmiChildren[imich],iSkeletonLod)) 
      return TRUE;
  }
  // bone was not found
  return FALSE;
}

// decompres axis for quaternion if animations are optimized
static void DecompressAxis(FLOAT3D &vNormal, UWORD ubH, UWORD ubP)
{
  ANGLE h = (ubH/65535.0f)*360.0f-180.0f;
  ANGLE p = (ubP/65535.0f)*360.0f-180.0f;

  FLOAT &x = vNormal(1);
  FLOAT &y = vNormal(2);
  FLOAT &z = vNormal(3);

  x = -Sin(h)*Cos(p);
  y = Sin(p);
  z = -Cos(h)*Cos(p);
}

// initialize batch model rendering
void RM_BeginRenderingView(CAnyProjection3D &apr, CDrawPort *pdp)
{
  // remember parameters
  _iRenderingType = 1;
  _pdp = pdp;
  // prepare and set the projection
  apr->ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  apr->Prepare();
  // in case of mirror projection, move mirror clip plane a bit father from the mirrored models,
  // so we have less clipping (for instance, player feet)
  if( apr->pr_bMirror) apr->pr_plMirrorView.pl_distance -= 0.06f; // -0.06 is because entire projection is offseted by +0.05
  _aprProjection = apr;
  _pdp->SetProjection( _aprProjection);

  // remember the abs to viewer transformation
  MatrixVectorToMatrix12(_mAbsToViewer,
    _aprProjection->pr_ViewerRotationMatrix, 
    -_aprProjection->pr_vViewerPosition*_aprProjection->pr_ViewerRotationMatrix);

  // make FPU precision low
  _fpuOldPrecision = GetFPUPrecision(); 
  SetFPUPrecision(FPT_24BIT);

}


// cleanup after batch model rendering
void RM_EndRenderingView( BOOL bRestoreOrtho/*=TRUE*/)
{
  ASSERT( _iRenderingType==1 && _pdp!=NULL);

  // assure that FPU precision was low all the model rendering time, then revert to old FPU precision
  ASSERT( GetFPUPrecision()==FPT_24BIT);
  SetFPUPrecision(_fpuOldPrecision);

  // back to 2D projection?
  if( bRestoreOrtho) _pdp->SetOrtho();
  _pdp->SetOrtho();
  _iRenderingType = 0;
  _pdp = NULL;
}



// for mark renderer
extern CAnyProjection3D _aprProjection;
extern UBYTE *_pubMask;
extern SLONG _slMaskWidth;
extern SLONG _slMaskHeight;

// begin/end model rendering to shadow mask
void RM_BeginModelRenderingMask( CAnyProjection3D &prProjection, UBYTE *pubMask, SLONG slMaskWidth, SLONG slMaskHeight)
{
  ASSERT( _iRenderingType==0);
  _iRenderingType = 2;
  _aprProjection  = prProjection;
  _pubMask      = pubMask;
  _slMaskWidth  = slMaskWidth; 
  _slMaskHeight = slMaskHeight; 

  // prepare and set the projection
  _aprProjection->ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  _aprProjection->Prepare();
  // remember the abs to viewer transformation
  MatrixVectorToMatrix12(_mAbsToViewer,
    _aprProjection->pr_ViewerRotationMatrix, 
    -_aprProjection->pr_vViewerPosition*_aprProjection->pr_ViewerRotationMatrix);

  // set mask shader
  extern void InternalShader_Mask(void);
  extern void InternalShaderDesc_Mask(ShaderDesc &shDesc);
  _shMaskShader.ShaderFunc    = InternalShader_Mask;
  _shMaskShader.GetShaderDesc = InternalShaderDesc_Mask;
}


void RM_EndModelRenderingMask(void)
{
  ASSERT( _iRenderingType==2);
  _iRenderingType = 0;
}






// setup light parameters
void RM_SetLightColor(COLOR colAmbient, COLOR colLight)
{
  _colAmbient = colAmbient;
  _colLight = colLight;
}
void RM_SetLightDirection(FLOAT3D &vLightDir)
{
  _vLightDir = vLightDir * (-1);
}

// calculate object matrices for givem model instance
void RM_SetObjectMatrices(CModelInstance &mi)
{
  ULONG ulFlags = RM_GetRenderFlags();

  // adjust clipping to frustum
  if( ulFlags & SRMF_INSIDE) gfxDisableClipping();
  else gfxEnableClipping();

  // adjust clipping to mirror-plane (if any)
  extern INDEX gap_iOptimizeClipping;

	if((CProjection3D *)_aprProjection != NULL) {
		if( gap_iOptimizeClipping>0 && (_aprProjection->pr_bMirror || _aprProjection->pr_bWarp)) {
			if( ulFlags & SRMF_INMIRROR) {
				gfxDisableClipPlane();
				gfxFrontFace( GFX_CCW);
			} else {
				gfxEnableClipPlane();
				gfxFrontFace( GFX_CW);
			}
		}
	}

  MatrixMultiply(_mObjToView,_mAbsToViewer, _mObjectToAbs);

  Matrix12 mStretch;
  MakeStretchMatrix(mStretch, mi.mi_vStretch);
  MatrixMultiply(_mObjToViewStretch,_mObjToView,mStretch);
}

// setup object position
void RM_SetObjectPlacement(const CPlacement3D &pl)
{
  FLOATmatrix3D m;
  MakeRotationMatrixFast( m, pl.pl_OrientationAngle);
  MatrixVectorToMatrix12(_mObjectToAbs,m, pl.pl_PositionVector);
}

void RM_SetObjectPlacement(const FLOATmatrix3D &m, const FLOAT3D &v)
{
  MatrixVectorToMatrix12(_mObjectToAbs,m, v);
}


// sets custom mesh lod
void RM_SetCustomMeshLodDistance(FLOAT fMeshLod)
{
  _fCustomMlodDistance = fMeshLod;
}
// sets custom skeleton lod
void RM_SetCustomSkeletonLodDistance(FLOAT fSkeletonLod)
{
  _fCustomSlodDistance = fSkeletonLod;
}

// Returns index of skeleton lod at given distance
INDEX GetSkeletonLOD(CSkeleton &sk, FLOAT fDistance)
{
  FLOAT fMinDistance = 1000000.0f;
  INDEX iSkeletonLod = -1;

  // if custom lod distance is set
  if(_fCustomSlodDistance!=-1) {
    // set object distance as custom distance
    fDistance = _fCustomSlodDistance;
  }
  // for each lod in skeleton
  INDEX ctslods = sk.skl_aSkeletonLODs.Count();
  for(INDEX islod=0;islod<ctslods;islod++) {
    SkeletonLOD &slod = sk.skl_aSkeletonLODs[islod];
    // adjust lod distance by custom settings
    FLOAT fLodMaxDistance = slod.slod_fMaxDistance*ska_fLODMul+ska_fLODAdd;

    // check if this lod max distance is smaller than distance to object
    if(fDistance < fLodMaxDistance && fLodMaxDistance < fMinDistance) {
      // remember this lod
      fMinDistance = fLodMaxDistance;
      iSkeletonLod = islod;
    }
  }
  return iSkeletonLod;
}

// Returns index of mesh lod at given distance
INDEX GetMeshLOD(CMesh &msh, FLOAT fDistance)
{
  FLOAT fMinDistance = 1000000.0f;
  INDEX iMeshLod = -1;

  // if custom lod distance is set
  if(_fCustomMlodDistance!=-1) {
    // set object distance as custom distance
    fDistance = _fCustomMlodDistance;
  }
  // for each lod in mesh
  INDEX ctmlods = msh.msh_aMeshLODs.Count();
  for(INDEX imlod=0;imlod<ctmlods;imlod++) {
    MeshLOD &mlod = msh.msh_aMeshLODs[imlod];
    // adjust lod distance by custom settings
    FLOAT fLodMaxDistance = mlod.mlod_fMaxDistance*ska_fLODMul+ska_fLODAdd;

    // check if this lod max distance is smaller than distance to object
    if(fDistance<fLodMaxDistance && fLodMaxDistance<fMinDistance) {
      // remember this lod
      fMinDistance = fLodMaxDistance;
      iMeshLod = imlod;
    }
  }
  return iMeshLod;
}

// create first dummy model that serves as parent for the entire hierarchy
void MakeRootModel(void)
{
  // create the model with one bone
  RenModel &rm = _aRenModels.Push();
  rm.rm_pmiModel = NULL;
  rm.rm_iFirstBone = 0;
  rm.rm_ctBones = 1;
  rm.rm_iParentBoneIndex = -1;
  rm.rm_iParentModelIndex = -1;
  
  // add the default bone
  RenBone &rb = _aRenBones.Push();
  rb.rb_iParentIndex = -1;
  rb.rb_psbBone = NULL;
  memset(&rb.rb_apPos,0,sizeof(AnimPos));
  memset(&rb.rb_arRot,0,sizeof(AnimRot));
}

// build model hierarchy
static INDEX BuildHierarchy(CModelInstance *pmiModel, INDEX irmParent)
{
  INDEX ctrm = _aRenModels.Count();
  // add one renmodel
  RenModel &rm = _aRenModels.Push();
  RenModel &rmParent = _aRenModels[irmParent];

  rm.rm_pmiModel = pmiModel;
  rm.rm_iParentModelIndex = irmParent;
  rm.rm_iNextSiblingModel = -1;
  rm.rm_iFirstBone = _aRenBones.Count();
  rm.rm_ctBones = 0;

  // if this model is root model
  if(pmiModel->mi_iParentBoneID == (-1)) {
    // set is parent bone index as 0
    rm.rm_iParentBoneIndex = rmParent.rm_iFirstBone;
  // model instance is attached to another model's bone 
  } else {
    INDEX iParentBoneIndex = -1;
    // does parent model insntance has a skeleton
    if(rmParent.rm_pmiModel->mi_psklSkeleton != NULL && rmParent.rm_iSkeletonLODIndex>=0)  {
      // get index of parent bone
      iParentBoneIndex = rmParent.rm_pmiModel->mi_psklSkeleton->FindBoneInLOD(pmiModel->mi_iParentBoneID,rmParent.rm_iSkeletonLODIndex);
    // model instance does not have skeleton
    } else {
      // do not draw this model
      _aRenModels.Pop();
      return -1;
    }
    // if parent bone index was not found ( not visible in current lod)
    if(iParentBoneIndex == (-1)) {
      // do not draw this model
      _aRenModels.Pop();
      return -1;
    // parent bone exists and its visible
    } else {
      // set this model parent bone index in array of renbones
      rm.rm_iParentBoneIndex = iParentBoneIndex + rmParent.rm_iFirstBone;
    }
  }
 
  // if this model instance has skeleton
  if(pmiModel->mi_psklSkeleton!=NULL) {
    // adjust mip factor in case of dynamic stretch factor
    FLOAT fDistFactor = _fDistanceFactor;
    FLOAT3D &vStretch = pmiModel->mi_vStretch;
    // if model is stretched 
    if( vStretch != FLOAT3D(1,1,1)) {
      // calculate new distance factor
      fDistFactor = fDistFactor / Max(vStretch(1),Max(vStretch(2),vStretch(3)));
    }
    // calulate its current skeleton lod
    rm.rm_iSkeletonLODIndex = GetSkeletonLOD(*pmiModel->mi_psklSkeleton,fDistFactor);
    // if current skeleton lod is valid and visible
    if(rm.rm_iSkeletonLODIndex > -1) {
      // count all bones in this skeleton
      INDEX ctsb = pmiModel->mi_psklSkeleton->skl_aSkeletonLODs[rm.rm_iSkeletonLODIndex].slod_aBones.Count();
      // for each bone in skeleton
      for(INDEX irb=0;irb<ctsb;irb++) {
        SkeletonBone *pSkeletonBone = &pmiModel->mi_psklSkeleton->skl_aSkeletonLODs[rm.rm_iSkeletonLODIndex].slod_aBones[irb];
        // add one renbone
        RenBone &rb = _aRenBones.Push();
        rb.rb_psbBone = pSkeletonBone;
        rb.rb_iRenModelIndex = ctrm;
        rm.rm_ctBones++;
        // add default bone position (used if no animations)
        rb.rb_apPos.ap_vPos = pSkeletonBone->sb_qvRelPlacement.vPos;
        rb.rb_arRot.ar_qRot = pSkeletonBone->sb_qvRelPlacement.qRot;

        // if this is root bone for this model instance
        if(pSkeletonBone->sb_iParentID == (-1)) {
          // set its parent bone index to be parent bone of this model instance
          rb.rb_iParentIndex = rm.rm_iParentBoneIndex;
        // this is child bone
        } else {
          // get parent index in array of renbones
          INDEX rb_iParentIndex = pmiModel->mi_psklSkeleton->FindBoneInLOD(pSkeletonBone->sb_iParentID,rm.rm_iSkeletonLODIndex);
          rb.rb_iParentIndex = rb_iParentIndex + rm.rm_iFirstBone;
        }
      }
    }
  }
  
  rm.rm_iFirstMesh = _aRenMesh.Count();
  rm.rm_ctMeshes = 0;

  INDEX ctm = pmiModel->mi_aMeshInst.Count();
  // for each mesh instance in this model instance
  for(INDEX im=0;im<ctm;im++) {
    // adjust mip factor in case of dynamic stretch factor
    FLOAT fDistFactor = _fDistanceFactor;
    FLOAT3D &vStretch = pmiModel->mi_vStretch;
    // if model is stretched 
    if( vStretch != FLOAT3D(1,1,1)) {
      // calculate new distance factor
      fDistFactor = fDistFactor / Max(vStretch(1),Max(vStretch(2),vStretch(3)));// Log2( Max(vStretch(1),Max(vStretch(2),vStretch(3))));
    }

    // calculate current mesh lod
    INDEX iMeshLodIndex = GetMeshLOD(*pmiModel->mi_aMeshInst[im].mi_pMesh,fDistFactor);
    // if mesh lod is visible
    if(iMeshLodIndex > -1) {
      // add one ren mesh
      RenMesh &rmsh = _aRenMesh.Push();
      rm.rm_ctMeshes++;
      rmsh.rmsh_iRenModelIndex = ctrm;
      rmsh.rmsh_pMeshInst = &pmiModel->mi_aMeshInst[im];
      rmsh.rmsh_iFirstMorph = _aRenMorph.Count();
      rmsh.rmsh_iFirstWeight = _aRenWeights.Count();
      rmsh.rmsh_ctMorphs = 0;
      rmsh.rmsh_ctWeights = 0;
      rmsh.rmsh_bTransToViewSpace = FALSE;
      // set mesh lod index for this ren mesh
      rmsh.rmsh_iMeshLODIndex = iMeshLodIndex;

      // for each morph map in this mesh lod
      INDEX ctmm = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aMorphMaps.Count();
      for(INDEX imm=0;imm<ctmm;imm++) {
        // add this morph map in array of renmorphs
        RenMorph &rm = _aRenMorph.Push();
        rmsh.rmsh_ctMorphs++;
        rm.rmp_pmmmMorphMap = &rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aMorphMaps[imm];
        rm.rmp_fFactor = 0;
      }

      // for each weight map in this mesh lod
      INDEX ctw = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aWeightMaps.Count();
      for(INDEX iw=0;iw<ctw;iw++) {
        // add this weight map in array of renweights
        RenWeight &rw = _aRenWeights.Push();
        MeshWeightMap &mwm = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aWeightMaps[iw];
        rw.rw_pwmWeightMap = &mwm;
        rmsh.rmsh_ctWeights++;
        rw.rw_iBoneIndex = rm.rm_iFirstBone;
        // find bone of this weight in current skeleton lod and get its index for this renweight
        if(!FindBone(mwm.mwm_iID,&rw.rw_iBoneIndex,pmiModel,rm.rm_iSkeletonLODIndex))
        {
          // if bone not found, set boneindex in renweight to -1
          rw.rw_iBoneIndex = -1;
        }
      }
    }
  }

  rm.rm_iFirstChildModel = -1;
  // for each child in this model instance
  INDEX ctmich = pmiModel->mi_cmiChildren.Count();
  for(int imich=0;imich<ctmich;imich++) {
    // build hierarchy for child model instance
    INDEX irmChildIndex = BuildHierarchy(&pmiModel->mi_cmiChildren[imich],ctrm);
    // if child is visible 
    if(irmChildIndex != (-1)) {
      // set model sibling
      _aRenModels[irmChildIndex].rm_iNextSiblingModel = rm.rm_iFirstChildModel;
      rm.rm_iFirstChildModel = irmChildIndex;
    }
  }
  return ctrm;
}

// calculate transformations for all bones on already built hierarchy
static void CalculateBoneTransforms()
{
  // put basic transformation in first dummy bone
  MatrixCopy(_aRenBones[0].rb_mTransform, _mObjToView);
  MatrixCopy(_aRenBones[0].rb_mStrTransform, _aRenBones[0].rb_mTransform);

  // if callback function was specified
  if(_pAdjustBonesCallback!=NULL) {
    // Call callback function
    _pAdjustBonesCallback(_pAdjustBonesData);
  }

  Matrix12 mStretch;
  // for each renbone after first dummy one
  int irb;
  for(irb=1; irb<_aRenBones.Count(); irb++) {
    Matrix12 mRelPlacement;
    Matrix12 mOffset;
    RenBone &rb = _aRenBones[irb];
    RenBone &rbParent = _aRenBones[rb.rb_iParentIndex];
    // Convert QVect of placement to matrix12
    QVect qv;
    qv.vPos = rb.rb_apPos.ap_vPos;
    qv.qRot = rb.rb_arRot.ar_qRot;
    QVectToMatrix12(mRelPlacement,qv);

    // if this is root bone
    if(rb.rb_psbBone->sb_iParentID == (-1)) {
      // stretch root bone
      RenModel &rm= _aRenModels[rb.rb_iRenModelIndex];
      MakeStretchMatrix(mStretch, rm.rm_pmiModel->mi_vStretch);
      

      RenModel &rmParent = _aRenModels[rb.rb_iRenModelIndex];
      QVectToMatrix12(mOffset,rmParent.rm_pmiModel->mi_qvOffset);
      // add offset to root bone
      MatrixMultiplyCP(mRelPlacement,mOffset,mRelPlacement);

      Matrix12 mStrParentBoneTrans;
      // Create stretch matrix with parent bone transformations
      MatrixMultiplyCP(mStrParentBoneTrans, rbParent.rb_mStrTransform,mStretch);
      // transform bone using stretch parent's transform, relative placement
      MatrixMultiply(rb.rb_mStrTransform, mStrParentBoneTrans, mRelPlacement);
      MatrixMultiply(rb.rb_mTransform,rbParent.rb_mTransform, mRelPlacement);
    } else {
      // transform bone using parent's transform and relative placement
      MatrixMultiply(rb.rb_mStrTransform, rbParent.rb_mStrTransform, mRelPlacement);
      MatrixMultiply(rb.rb_mTransform,rbParent.rb_mTransform,mRelPlacement);
    }
    // remember tranform matrix of bone placement for bone rendering
    MatrixCopy(rb.rb_mBonePlacement,rb.rb_mStrTransform);
  }

  // for each renmodel after first dummy one
  for(int irm=1; irm<_aRenModels.Count(); irm++) {
    // remember transforms for bone-less models for every renmodel, except the dummy one
    Matrix12 mOffset;
    Matrix12 mStretch;
    RenModel &rm = _aRenModels[irm];

    QVectToMatrix12(mOffset,rm.rm_pmiModel->mi_qvOffset);
    MakeStretchMatrix(mStretch,rm.rm_pmiModel->mi_vStretch);

    MatrixMultiply(rm.rm_mTransform,_aRenBones[rm.rm_iParentBoneIndex].rb_mTransform,mOffset);
    MatrixMultiply(rm.rm_mStrTransform,_aRenBones[rm.rm_iParentBoneIndex].rb_mStrTransform,mOffset);
    MatrixMultiplyCP(rm.rm_mStrTransform,rm.rm_mStrTransform,mStretch);
  }

  Matrix12 mInvert;
  // for each renbone
  for(irb=1; irb<_aRenBones.Count(); irb++) {
    RenBone &rb = _aRenBones[irb];
    // multiply every transform with invert matrix of bone abs placement
    MatrixTranspose(mInvert,rb.rb_psbBone->sb_mAbsPlacement);
    // create two versions of transform matrices, stretch and normal for vertices and normals
    MatrixMultiplyCP(_aRenBones[irb].rb_mStrTransform,_aRenBones[irb].rb_mStrTransform,mInvert);
    MatrixMultiplyCP(_aRenBones[irb].rb_mTransform,_aRenBones[irb].rb_mTransform,mInvert);
  }
}

// Match animations in anim queue for bones
static void MatchAnims(RenModel &rm)
{
  const FLOAT fLerpedTick = _pTimer->GetLerpedCurrentTick();

  // return if no animsets
  INDEX ctas = rm.rm_pmiModel->mi_aAnimSet.Count();
  if(ctas == 0) return;
  // count animlists
  INDEX ctal = rm.rm_pmiModel->mi_aqAnims.aq_Lists.Count();
  // find newes animlist that has fully faded in
  INDEX iFirstAnimList = 0;
  // loop from newer to older
  INDEX ial;
  for(ial=ctal-1;ial>=0;ial--) {
    AnimList &alList = rm.rm_pmiModel->mi_aqAnims.aq_Lists[ial];
    // calculate fade factor
    FLOAT fFadeFactor = CalculateFadeFactor(alList);
    if(fFadeFactor >= 1.0f) {
      iFirstAnimList = ial;
      break;
    }
  }

  // for each anim list after iFirstAnimList
  for(ial=iFirstAnimList;ial<ctal;ial++) {
    AnimList &alList = rm.rm_pmiModel->mi_aqAnims.aq_Lists[ial];
    AnimList *palListNext=NULL;
    if(ial+1<ctal) palListNext = &rm.rm_pmiModel->mi_aqAnims.aq_Lists[ial+1];
    
    // calculate fade factor
    FLOAT fFadeFactor = CalculateFadeFactor(alList);

    INDEX ctpa = alList.al_PlayedAnims.Count();
    // for each played anim in played anim list
    for(int ipa=0;ipa<ctpa;ipa++) {
      FLOAT fTime = fLerpedTick;
      PlayedAnim &pa = alList.al_PlayedAnims[ipa];
      BOOL bAnimLooping = pa.pa_ulFlags & AN_LOOPING;

      INDEX iAnimSetIndex;
      INDEX iAnimIndex;
      // find anim by ID in all anim sets within this model
      if(rm.rm_pmiModel->FindAnimationByID(pa.pa_iAnimID,&iAnimSetIndex,&iAnimIndex)) {
        // if found, animate bones
        Animation &an = rm.rm_pmiModel->mi_aAnimSet[iAnimSetIndex].as_Anims[iAnimIndex];
        
        // calculate end time for this animation list
        FLOAT fFadeInEndTime = alList.al_fStartTime + alList.al_fFadeTime;

        // if there is a newer anmimation list
        if(palListNext!=NULL) {
          // freeze time of this one to never overlap with the newer list
          fTime = ClampUp(fTime, palListNext->al_fStartTime);
        }

        // calculate time passed since the animation started
        FLOAT fTimeOffset = fTime - pa.pa_fStartTime;
        // if this animation list is fading in
        if (fLerpedTick < fFadeInEndTime) {
          // offset the time so that it is paused at the end of fadein interval
          fTimeOffset += fFadeInEndTime - fLerpedTick;
        }

        FLOAT f = fTimeOffset / (an.an_fSecPerFrame*pa.pa_fSpeedMul);

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
          iAnimFrame = ClampUp(iCurentFrame,an.an_iFrames-1);
          iNextAnimFrame = ClampUp(iCurentFrame+1,an.an_iFrames-1);
        }
        
        // for each bone envelope
        INDEX ctbe = an.an_abeBones.Count();
        for(int ibe=0;ibe<ctbe;ibe++) {
          INDEX iBoneIndex;
          // find its renbone in array of renbones
          if(FindRenBone(rm,an.an_abeBones[ibe].be_iBoneID, &iBoneIndex)) {
            RenBone &rb = _aRenBones[iBoneIndex];
            BoneEnvelope &be = an.an_abeBones[ibe];

            INDEX iRotFrameIndex;
            INDEX iNextRotFrameIndex;
            INDEX iRotFrameNum;
            INDEX iNextRotFrameNum;
            FLOAT fSlerpFactor;
            FLOATquat3D qRot;
            FLOATquat3D qRotCurrent;
            FLOATquat3D qRotNext;
            FLOATquat3D *pqRotCurrent;
            FLOATquat3D *pqRotNext;
            
            // if animation is not compresed
            if(!an.an_bCompresed) {
              AnimRot *arFirst = &be.be_arRot[0];
              INDEX ctfn = be.be_arRot.Count();
              // find index of closest frame
              iRotFrameIndex = FindFrame((UBYTE*)arFirst,iAnimFrame,ctfn,sizeof(AnimRot));
              
              // get index of next frame
              if(bAnimLooping) {
                iNextRotFrameIndex = (iRotFrameIndex+1) % be.be_arRot.Count();
              } else {
                iNextRotFrameIndex = ClampUp(iRotFrameIndex+1L,be.be_arRot.Count() - 1L);
              }
              
              iRotFrameNum = be.be_arRot[iRotFrameIndex].ar_iFrameNum;
              iNextRotFrameNum = be.be_arRot[iNextRotFrameIndex].ar_iFrameNum;
              pqRotCurrent = &be.be_arRot[iRotFrameIndex].ar_qRot;
              pqRotNext = &be.be_arRot[iNextRotFrameIndex].ar_qRot;
            // animation is not compresed
            } else {
              AnimRotOpt *aroFirst = &be.be_arRotOpt[0];
              INDEX ctfn = be.be_arRotOpt.Count();
              iRotFrameIndex = FindFrame((UBYTE*)aroFirst,iAnimFrame,ctfn,sizeof(AnimRotOpt));

              // get index of next frame
              if(bAnimLooping) { 
                iNextRotFrameIndex = (iRotFrameIndex+1L) % be.be_arRotOpt.Count();
              } else {
                iNextRotFrameIndex = ClampUp(iRotFrameIndex+1L,be.be_arRotOpt.Count() - 1L);
              }

              AnimRotOpt &aroRot = be.be_arRotOpt[iRotFrameIndex];
              AnimRotOpt &aroRotNext = be.be_arRotOpt[iNextRotFrameIndex];
              iRotFrameNum = aroRot.aro_iFrameNum;
              iNextRotFrameNum = aroRotNext.aro_iFrameNum;
              FLOAT3D vAxis;
              ANGLE aAngle;

              // decompress angle
              aAngle = aroRot.aro_aAngle / ANG_COMPRESIONMUL;
              DecompressAxis(vAxis,aroRot.aro_ubH,aroRot.aro_ubP);
              qRotCurrent.FromAxisAngle(vAxis,aAngle);

              aAngle = aroRotNext.aro_aAngle / ANG_COMPRESIONMUL;
              DecompressAxis(vAxis,aroRotNext.aro_ubH,aroRotNext.aro_ubP);
              qRotNext.FromAxisAngle(vAxis,aAngle);
              pqRotCurrent = &qRotCurrent;
              pqRotNext = &qRotNext;
            }

            if(iNextRotFrameNum<=iRotFrameNum) {
              // calculate slerp factor for rotations
              fSlerpFactor = (f-iRotFrameNum) / (an.an_iFrames-iRotFrameNum);
            } else {
              // calculate slerp factor for rotations
              fSlerpFactor = (f-iRotFrameNum) / (iNextRotFrameNum-iRotFrameNum);
            }
            
            // calculate rotation for bone beetwen current and next frame in animation
            qRot = Slerp(fSlerpFactor,*pqRotCurrent,*pqRotNext);
            // and currently playing animation 
            rb.rb_arRot.ar_qRot = Slerp(fFadeFactor*pa.pa_Strength,rb.rb_arRot.ar_qRot,qRot);

            AnimPos *apFirst = &be.be_apPos[0];
            INDEX ctfn = be.be_apPos.Count();
            INDEX iPosFrameIndex = FindFrame((UBYTE*)apFirst,iAnimFrame,ctfn,sizeof(AnimPos));

            INDEX iNextPosFrameIndex;
            // is animation looping
            if(bAnimLooping) { 
              iNextPosFrameIndex = (iPosFrameIndex+1) % be.be_apPos.Count();
            } else {
              iNextPosFrameIndex = ClampUp(iPosFrameIndex+1L,be.be_apPos.Count()-1L);
            }

            INDEX iPosFrameNum = be.be_apPos[iPosFrameIndex].ap_iFrameNum;
            INDEX iNextPosFrameNum = be.be_apPos[iNextPosFrameIndex].ap_iFrameNum;

            FLOAT fLerpFactor;
            if(iNextPosFrameNum<=iPosFrameNum) fLerpFactor = (f-iPosFrameNum) / (an.an_iFrames-iPosFrameNum);
            else fLerpFactor = (f-iPosFrameNum) / (iNextPosFrameNum-iPosFrameNum);
            
            FLOAT3D vPos;
            FLOAT3D vBonePosCurrent = be.be_apPos[iPosFrameIndex].ap_vPos;
            FLOAT3D vBonePosNext = be.be_apPos[iNextPosFrameIndex].ap_vPos;

            // if bone envelope and bone have some length 
            if((be.be_OffSetLen > 0) && (rb.rb_psbBone->sb_fOffSetLen > 0)) {
              // size bone to fit bone envelope
              vBonePosCurrent *= (rb.rb_psbBone->sb_fOffSetLen / be.be_OffSetLen);
              vBonePosNext *= (rb.rb_psbBone->sb_fOffSetLen / be.be_OffSetLen);
            }

            // calculate position for bone beetwen current and next frame in animation
            vPos = Lerp(vBonePosCurrent,vBonePosNext,fLerpFactor);
            // and currently playing animation 
            rb.rb_apPos.ap_vPos = Lerp(rb.rb_apPos.ap_vPos,vPos,fFadeFactor * pa.pa_Strength);
          }
        }

        // for each morphmap
        for(INDEX im=0;im<an.an_ameMorphs.Count();im++) {
          INDEX iMorphIndex;
          // find it in renmorph
          if(FindRenMorph(rm,an.an_ameMorphs[im].me_iMorphMapID,&iMorphIndex)) {
            // lerp morphs
            FLOAT &fCurFactor = an.an_ameMorphs[im].me_aFactors[iAnimFrame];
            FLOAT &fLastFactor = an.an_ameMorphs[im].me_aFactors[iNextAnimFrame];
            FLOAT fFactor = Lerp(fCurFactor,fLastFactor,f-iAnimFrame);

            _aRenMorph[iMorphIndex].rmp_fFactor = Lerp(_aRenMorph[iMorphIndex].rmp_fFactor,
                                                      fFactor,
                                                      fFadeFactor * pa.pa_Strength);
          }
        }
      }
    }
  }
}

// array of pointers to texure data for shader
static CStaticStackArray<class CTextureObject*> _patoTextures;
static CStaticStackArray<struct GFXTexCoord*> _paTexCoords;
// draw mesh on screen
static void RenderMesh(RenMesh &rmsh,RenModel &rm)
{
  ASSERT(_pavFinalVertices!=NULL);
  ASSERT(_panFinalNormals!=NULL);

  MeshLOD &mlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
  // Count surfaces in mesh
  INDEX ctsrf = mlod.mlod_aSurfaces.Count();
  // for each surface
  for(INDEX isrf=0; isrf<ctsrf; isrf++)
  {
    MeshSurface &msrf = mlod.mlod_aSurfaces[isrf];
    CShader  *pShader = msrf.msrf_pShader;
    if( _iRenderingType==2) pShader = &_shMaskShader; // force mask shader for rendering to shadowmaps

    // if this surface has valid shader and show texure flag is set
    if( pShader!=NULL && (RM_GetFlags() & RMF_SHOWTEXTURE))
    {
      // Create copy of shading params
      ShaderParams *pShaderParams = &msrf.msrf_ShadingParams;
      ShaderParams spForAdjustment;

      // if callback function was specified
      if(_pAdjustShaderParams!=NULL) {
        // Call callback function
        spForAdjustment = msrf.msrf_ShadingParams;
        _pAdjustShaderParams( _pAdjustShaderData, msrf.msrf_iSurfaceID, pShader, spForAdjustment);
        pShaderParams = &spForAdjustment;
      }

      // clamp surface texture count to max number of textrues in mesh
      INDEX cttx = pShaderParams->sp_aiTextureIDs.Count();
      //INDEX cttxMax = rmsh.rmsh_pMeshInst->mi_tiTextures.Count();
      // cttx = ClampUp(cttx,cttxMax);

      _patoTextures.PopAll();
      if(cttx>0)_patoTextures.Push(cttx);
      // for each texture ID
      for(INDEX itx=0;itx<cttx;itx++) {
        // find texture in mesh and get pointer to texture by texture ID
        FindTextureData( &_patoTextures[itx], pShaderParams->sp_aiTextureIDs[itx], *rmsh.rmsh_pMeshInst);
      }

      // count uvmaps
      INDEX ctuvm = pShaderParams->sp_aiTexCoordsIndex.Count();
      // ctuvm = ClampUp(ctuvm,mlod.mlod_aUVMaps.Count());
      
      _paTexCoords.PopAll();
      if(ctuvm>0)_paTexCoords.Push(ctuvm);
      // for each uvamp
      for( INDEX iuvm=0; iuvm<ctuvm; iuvm++) {
        // set pointer of uvmap in array of uvmaps for shader
        INDEX iuvmIndex = pShaderParams->sp_aiTexCoordsIndex[iuvm];
        // if mesh lod has this uv map
        if(iuvmIndex<mlod.mlod_aUVMaps.Count()) {
          _paTexCoords[iuvm] = (GFXTexCoord*)&mlod.mlod_aUVMaps[iuvmIndex].muv_aTexCoords[msrf.msrf_iFirstVertex];
        } else {
          _paTexCoords[iuvm] = NULL;
        }
      }

      INDEX ctTextures = _patoTextures.Count();
      INDEX ctTexCoords = _paTexCoords.Count();
      INDEX ctColors = pShaderParams->sp_acolColors.Count();
      INDEX ctFloats = pShaderParams->sp_afFloats.Count();

      // begin model rendering
      const BOOL bModelSetupTimer = _sfStats.CheckTimer(CStatForm::STI_MODELSETUP);
      if( bModelSetupTimer) _sfStats.StopTimer(CStatForm::STI_MODELSETUP);
      _sfStats.StartTimer(CStatForm::STI_MODELRENDERING);

      shaBegin( _aprProjection, pShader);
      shaSetVertexArray((GFXVertex4*)&_pavFinalVertices[msrf.msrf_iFirstVertex],msrf.msrf_ctVertices);
      shaSetNormalArray((GFXNormal*)&_panFinalNormals[msrf.msrf_iFirstVertex]);
      shaSetIndices(&msrf.msrf_aTriangles[0].iVertex[0],msrf.msrf_aTriangles.Count()*3);
      shaSetFlags(msrf.msrf_ShadingParams.sp_ulFlags);

      
      // if mesh is transformed to view space
      if(rmsh.rmsh_bTransToViewSpace) {
        //#pragma message(">> FIX THIS !!!")
        // no ObjToView matrix is needed in shader so set empty matrix
        Matrix12 mIdentity;
        MakeIdentityMatrix(mIdentity);
        shaSetObjToViewMatrix(mIdentity);
        Matrix12 mInvObjToAbs;
        MatrixTranspose(mInvObjToAbs,_mAbsToViewer);
        shaSetObjToAbsMatrix(mInvObjToAbs);
      } else {
        // give shader current ObjToView matrix
        shaSetObjToViewMatrix(_mObjToView);
        shaSetObjToAbsMatrix(_mObjectToAbs);
      }

      // Set light parametars
      shaSetLightColor(_colAmbient,_colLight);
      shaSetLightDirection(_vLightDirInView);
      // Set model color
      shaSetModelColor(rm.rm_pmiModel->mi_colModelColor);

      if(ctTextures>0)  shaSetTextureArray(&_patoTextures[0],ctTextures);
      if(ctTexCoords>0) shaSetUVMapsArray(&_paTexCoords[0],ctTexCoords);
      if(ctColors>0)    shaSetColorArray(&pShaderParams->sp_acolColors[0],ctColors);
      if(ctFloats>0)    shaSetFloatArray(&pShaderParams->sp_afFloats[0],ctFloats);
      shaEnd();

      _sfStats.StopTimer(CStatForm::STI_MODELRENDERING);
      if( bModelSetupTimer) _sfStats.StartTimer(CStatForm::STI_MODELSETUP);
    }
    // surface has no shader or textures are turned off
    else {
      COLOR colErrColor = 0xCDCDCDFF;
      // surface has no shader, just show vertices using custom simple shader 
      shaSetVertexArray((GFXVertex4*)&_pavFinalVertices[msrf.msrf_iFirstVertex],msrf.msrf_ctVertices);
      shaSetNormalArray((GFXNormal*)&_panFinalNormals[msrf.msrf_iFirstVertex]);
      shaSetIndices(&msrf.msrf_aTriangles[0].iVertex[0],msrf.msrf_aTriangles.Count()*3);
      shaSetTexture(-1);
      shaSetColorArray(&colErrColor,1);

      shaSetLightColor(_colAmbient,_colLight);
      shaSetLightDirection(_vLightDirInView);
      shaSetModelColor(rm.rm_pmiModel->mi_colModelColor);

      shaDisableBlend();
      shaEnableDepthTest();
      shaEnableDepthWrite();
      shaSetColor(0);
      shaCalculateLight();
      shaRender();
      shaClean();
    }
  }
}

// Prepare ren mesh for rendering
static void PrepareMeshForRendering(RenMesh &rmsh, INDEX iSkeletonlod)
{
  // set curent mesh lod
  MeshLOD &mlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
  // clear vertices array
  _aMorphedVtxs.PopAll();
  _aMorphedNormals.PopAll();
  _aFinalVtxs.PopAll();
  _aFinalNormals.PopAll();
  _pavFinalVertices = NULL;
  _panFinalNormals  = NULL;
  // Reset light direction
  _vLightDirInView = _vLightDir;

  
  // Get vertices count
  INDEX ctVertices = mlod.mlod_aVertices.Count();
  // Allocate memory for vertices
  _aMorphedVtxs.Push(ctVertices);
  _aMorphedNormals.Push(ctVertices);
  _aFinalVtxs.Push(ctVertices);
  _aFinalNormals.Push(ctVertices);
  // Remember final vertex count
  _ctFinalVertices = ctVertices;
  
  // Copy original vertices and normals to _aMorphedVtxs
  memcpy(&_aMorphedVtxs[0],&mlod.mlod_aVertices[0],sizeof(mlod.mlod_aVertices[0]) * ctVertices);
  memcpy(&_aMorphedNormals[0],&mlod.mlod_aNormals[0],sizeof(mlod.mlod_aNormals[0]) * ctVertices);
  // Set final vertices and normals to 0
  memset(&_aFinalVtxs[0],0,sizeof(_aFinalVtxs[0])*ctVertices);
  memset(&_aFinalNormals[0],0,sizeof(_aFinalNormals[0])*ctVertices);
  

  INDEX ctmm = rmsh.rmsh_iFirstMorph + rmsh.rmsh_ctMorphs;
  // blend vertices and normals for each RenMorph 
  for(int irm=rmsh.rmsh_iFirstMorph;irm<ctmm;irm++)
  {
    RenMorph &rm = _aRenMorph[irm];
    // blend only if factor is > 0
    if(rm.rmp_fFactor > 0.0f) {
      // for each vertex and normal in morphmap
      for(int ivx=0;ivx<rm.rmp_pmmmMorphMap->mmp_aMorphMap.Count();ivx++) {
        // blend vertices and normals
        if(rm.rmp_pmmmMorphMap->mmp_bRelative) {
          // blend relative (new = cur + f*(dst-src))
          INDEX vtx = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx].mwm_iVxIndex;
          MeshVertex &mvSrc = mlod.mlod_aVertices[vtx];
          MeshNormal &mnSrc = mlod.mlod_aNormals[vtx];
          MeshVertexMorph &mvmDst = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx];
          // blend vertices
          _aMorphedVtxs[vtx].x += rm.rmp_fFactor*(mvmDst.mwm_x - mvSrc.x);
          _aMorphedVtxs[vtx].y += rm.rmp_fFactor*(mvmDst.mwm_y - mvSrc.y);
          _aMorphedVtxs[vtx].z += rm.rmp_fFactor*(mvmDst.mwm_z - mvSrc.z);
          // blend normals
          _aMorphedNormals[vtx].nx += rm.rmp_fFactor*(mvmDst.mwm_nx - mnSrc.nx);
          _aMorphedNormals[vtx].ny += rm.rmp_fFactor*(mvmDst.mwm_ny - mnSrc.ny);
          _aMorphedNormals[vtx].nz += rm.rmp_fFactor*(mvmDst.mwm_nz - mnSrc.nz);
        } else {
          // blend absolute (1-f)*cur + f*dst
          INDEX vtx = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx].mwm_iVxIndex;
          //MeshVertex &mvSrc = mlod.mlod_aVertices[vtx];
          MeshVertexMorph &mvmDst = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx];
          // blend vertices
          _aMorphedVtxs[vtx].x = (1.0f-rm.rmp_fFactor) * _aMorphedVtxs[vtx].x + rm.rmp_fFactor*mvmDst.mwm_x;
          _aMorphedVtxs[vtx].y = (1.0f-rm.rmp_fFactor) * _aMorphedVtxs[vtx].y + rm.rmp_fFactor*mvmDst.mwm_y;
          _aMorphedVtxs[vtx].z = (1.0f-rm.rmp_fFactor) * _aMorphedVtxs[vtx].z + rm.rmp_fFactor*mvmDst.mwm_z;
          // blend normals
          _aMorphedNormals[vtx].nx = (1.0f-rm.rmp_fFactor) * _aMorphedNormals[vtx].nx + rm.rmp_fFactor*mvmDst.mwm_nx;
          _aMorphedNormals[vtx].ny = (1.0f-rm.rmp_fFactor) * _aMorphedNormals[vtx].ny + rm.rmp_fFactor*mvmDst.mwm_ny;
          _aMorphedNormals[vtx].nz = (1.0f-rm.rmp_fFactor) * _aMorphedNormals[vtx].nz + rm.rmp_fFactor*mvmDst.mwm_nz;
        }
      }
    }
  }

  INDEX ctrw = rmsh.rmsh_iFirstWeight + rmsh.rmsh_ctWeights;
  INDEX ctbones = 0;
  CSkeleton *pskl = _aRenModels[rmsh.rmsh_iRenModelIndex].rm_pmiModel->mi_psklSkeleton;
  // if skeleton for this model exists and its currently visible
  if((pskl!=NULL) && (iSkeletonlod > -1)) {
    // count bones in skeleton
    ctbones = pskl->skl_aSkeletonLODs[iSkeletonlod].slod_aBones.Count();
  }

  // if there is skeleton attached to this mesh transfrom all vertices
  if(ctbones > 0 && ctrw>0) {
    // for each renweight
    for(int irw=rmsh.rmsh_iFirstWeight; irw<ctrw; irw++) {
      RenWeight &rw = _aRenWeights[irw];
      Matrix12 mTransform;
      Matrix12 mStrTransform;
      // if no bone for this weight 
      if(rw.rw_iBoneIndex == (-1)) {
        // transform vertex using default model transform matrix (for boneless models)
        MatrixCopy(mStrTransform, _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mStrTransform);
        MatrixCopy(mTransform,    _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mTransform);
      } else {
        // use bone transform matrix
        MatrixCopy(mStrTransform, _aRenBones[rw.rw_iBoneIndex].rb_mStrTransform);
        MatrixCopy(mTransform,    _aRenBones[rw.rw_iBoneIndex].rb_mTransform);
      }

      // if this is front face mesh remove rotation from transfrom matrix
      if(mlod.mlod_ulFlags & ML_FULL_FACE_FORWARD) {
        RemoveRotationFromMatrix(mStrTransform);
      }

      // for each vertex in this weight
      INDEX ctvw = rw.rw_pwmWeightMap->mwm_aVertexWeight.Count();
      for(int ivw=0; ivw<ctvw; ivw++) {
        MeshVertexWeight &vw = rw.rw_pwmWeightMap->mwm_aVertexWeight[ivw];
        INDEX ivx = vw.mww_iVertex;
        MeshVertex mv = _aMorphedVtxs[ivx];
        MeshNormal mn = _aMorphedNormals[ivx];
        
        // transform vertex and normal with this weight transform matrix
        TransformVector((FLOAT3&)mv,mStrTransform);
        RotateVector((FLOAT3&)mn,mTransform); // Don't stretch normals

        // Add new values to final vertices
        _aFinalVtxs[ivx].x += mv.x * vw.mww_fWeight;
        _aFinalVtxs[ivx].y += mv.y * vw.mww_fWeight;
        _aFinalVtxs[ivx].z += mv.z * vw.mww_fWeight;
        _aFinalNormals[ivx].nx += mn.nx * vw.mww_fWeight;
        _aFinalNormals[ivx].ny += mn.ny * vw.mww_fWeight;
        _aFinalNormals[ivx].nz += mn.nz * vw.mww_fWeight;
      }
    }
    _pavFinalVertices = &_aFinalVtxs[0];
    _panFinalNormals  = &_aFinalNormals[0];
    // mesh is in view space so transform light to view space
    RotateVector(_vLightDirInView.vector,_mObjToView);
    // set flag that mesh is in view space
    rmsh.rmsh_bTransToViewSpace = TRUE;
    // reset view matrix bacause model is allready transformed in view space
    gfxSetViewMatrix(NULL);
  // if no skeleton
  } else {
    // if flag is set to transform all vertices to view space
    if(_bTransformBonelessModelToViewSpace) {
      // transform every vertex using default model transform matrix (for boneless models)
      Matrix12 mTransform;
      Matrix12 mStrTransform;
      MatrixCopy(mTransform,    _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mTransform);
      MatrixCopy(mStrTransform, _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mStrTransform);

      // if this is front face mesh remove rotation from transfrom matrix
      if(mlod.mlod_ulFlags & ML_FULL_FACE_FORWARD) {
        RemoveRotationFromMatrix(mStrTransform);
      }
      
      // for each vertex
      for(int ivx=0;ivx<ctVertices;ivx++) {
        MeshVertex &mv = _aMorphedVtxs[ivx];
        MeshNormal &mn = _aMorphedNormals[ivx];
        // Transform vertex
        TransformVector((FLOAT3&)mv,mStrTransform);
        // Rotate normal
        RotateVector((FLOAT3&)mn,mTransform);
        _aFinalVtxs[ivx].x = mv.x;
        _aFinalVtxs[ivx].y = mv.y;
        _aFinalVtxs[ivx].z = mv.z;
        _aFinalNormals[ivx].nx = mn.nx;
        _aFinalNormals[ivx].ny = mn.ny;
        _aFinalNormals[ivx].nz = mn.nz;
      }
      _pavFinalVertices = &_aFinalVtxs[0];
      _panFinalNormals  = &_aFinalNormals[0];
      // mesh is in view space so transform light to view space
      RotateVector(_vLightDirInView.vector,_mObjToView);
      // set flag that mesh is in view space
      rmsh.rmsh_bTransToViewSpace = TRUE;
      // reset view matrix bacause model is allready transformed in view space
      gfxSetViewMatrix(NULL);
    // leave vertices in obj space
    } else {
      Matrix12 &m12 = _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mStrTransform;
      FLOAT gfxm[16];
      //#pragma message(">> Fix face forward meshes, when objects are left in object space")

      // set view matrix to gfx
      gfxm[ 0] = m12[ 0];  gfxm[ 1] = m12[ 4];  gfxm[ 2] = m12[ 8];  gfxm[ 3] = 0;
      gfxm[ 4] = m12[ 1];  gfxm[ 5] = m12[ 5];  gfxm[ 6] = m12[ 9];  gfxm[ 7] = 0;
      gfxm[ 8] = m12[ 2];  gfxm[ 9] = m12[ 6];  gfxm[10] = m12[10];  gfxm[11] = 0;
      gfxm[12] = m12[ 3];  gfxm[13] = m12[ 7];  gfxm[14] = m12[11];  gfxm[15] = 1;
      gfxSetViewMatrix(gfxm);

      RenModel &rm = _aRenModels[rmsh.rmsh_iRenModelIndex];
      RenBone &rb = _aRenBones[rm.rm_iParentBoneIndex];
      RotateVector(_vLightDirInView.vector,rb.rb_mBonePlacement);
      _pavFinalVertices = &mlod.mlod_aVertices[0];
      _panFinalNormals  = &mlod.mlod_aNormals[0];
      // mark this mesh as in object space
      rmsh.rmsh_bTransToViewSpace = FALSE;
    }
  }
}

// render one ren model
static void RenderModel_View(RenModel &rm)
{
  ASSERT( _iRenderingType==1);
  const BOOL bShowNormals = RM_GetFlags() & RMF_SHOWNORMALS;

  // for each mesh in renmodel
  INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
  for( int imsh=rm.rm_iFirstMesh;imsh<ctmsh;imsh++) {
    RenMesh &rmsh = _aRenMesh[imsh];
    // prepare mesh for rendering
    PrepareMeshForRendering(rmsh,rm.rm_iSkeletonLODIndex);
    // render mesh
    RenderMesh(rmsh,rm);
    // show normals in required
    if( bShowNormals) RenderNormals();
  }
}

// render one ren model to shadowmap
static void RenderModel_Mask(RenModel &rm)
{
  ASSERT( _iRenderingType==2);
  // flag to transform all vertices in view space
  const BOOL bTemp = _bTransformBonelessModelToViewSpace;
  _bTransformBonelessModelToViewSpace = TRUE;
  RM_SetCurrentDistance(0);

  INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
  // for each mesh in renmodel
  for(int imsh=rm.rm_iFirstMesh;imsh<ctmsh;imsh++) {
    // render mesh
    RenMesh &rmsh = _aRenMesh[imsh];
    PrepareMeshForRendering(rmsh,rm.rm_iSkeletonLODIndex);
    RenderMesh(rmsh,rm);
  }

  // done
  _bTransformBonelessModelToViewSpace = bTemp;
}

// Get bone abs position
BOOL RM_GetRenBoneAbs(CModelInstance &mi,INDEX iBoneID,RenBone &rb)
{
  // do not transform to view space
  MakeIdentityMatrix(_mAbsToViewer);
  CalculateRenderingData(mi);
  INDEX ctrb = _aRenBones.Count();
  // for each render bone after dummy one
  for(INDEX irb=1;irb<ctrb;irb++) {
    RenBone &rbone = _aRenBones[irb];
    // check if this is serched bone
    if(rbone.rb_psbBone->sb_iID == iBoneID) {
      rb = rbone;
      ClearRenArrays();
      return TRUE;
    }
  }
  // Clear ren arrays
  ClearRenArrays();
  return FALSE;
}

// Returns true if bone exists and sets two given vectors as start and end point of specified bone
BOOL RM_GetBoneAbsPosition(CModelInstance &mi,INDEX iBoneID, FLOAT3D &vStartPoint, FLOAT3D &vEndPoint)
{
  // do not transform to view space
  MakeIdentityMatrix(_mAbsToViewer);
  // use higher lod for bone finding
  RM_SetCurrentDistance(0);
  CalculateRenderingData(mi);
  INDEX ctrb = _aRenBones.Count();
  // for each render bone after dummy one
  for(INDEX irb=1;irb<ctrb;irb++) {
    RenBone &rb = _aRenBones[irb];
    // check if this is serched bone
    if(rb.rb_psbBone->sb_iID == iBoneID) {
      vStartPoint = FLOAT3D(0,0,0);
      vEndPoint   = FLOAT3D(0,0,rb.rb_psbBone->sb_fBoneLength);
      TransformVector(vStartPoint.vector,rb.rb_mBonePlacement);
      TransformVector(vEndPoint.vector,rb.rb_mBonePlacement);
      ClearRenArrays();
      return TRUE;
    }
  }
  // Clear ren arrays
  ClearRenArrays();
  return FALSE;
}

// Calculate complete rendering data for model instance
static void CalculateRenderingData(CModelInstance &mi)
{
  RM_SetObjectMatrices(mi);
  // distance to model is z param in objtoview matrix 
  _fDistanceFactor = -_mObjToView[11];

  // create first dummy model that serves as parent for the entire hierarchy
  MakeRootModel();
  // build entire hierarchy with children
  BuildHierarchy(&mi, 0);

  INDEX ctrm = _aRenModels.Count();
  // for each renmodel 
  for(int irm=1;irm<ctrm;irm++) {
    // match model animations
    MatchAnims(_aRenModels[irm]);
  }
  // Calculate transformations for all bones on already built hierarchy
  CalculateBoneTransforms();
}

// Render one SKA model with its children
void RM_RenderSKA(CModelInstance &mi)
{
  // Calculate all rendering data for this model instance
  //if( _iRenderingType==2) CalculateRenderingData( mi, 0);
  //else 
  CalculateRenderingData(mi);

  // for each renmodel
  INDEX ctrmsh = _aRenModels.Count();
  for(int irmsh=1;irmsh<ctrmsh;irmsh++) {
    RenModel &rm = _aRenModels[irmsh];
    // set object matrices
    RM_SetObjectMatrices(*rm.rm_pmiModel);
    // render this model
    if( _iRenderingType==1) RenderModel_View(rm);
    else RenderModel_Mask(rm);
  }
  // done if cluster shadows were rendered
  if( _iRenderingType==2) {
    // reset arrays
    ClearRenArrays();
    return;
  }

  // no cluster shadows - see if anything else needs to be rendered
  ASSERT( _iRenderingType==1);

  // if render wireframe is requested
  if(RM_GetFlags() & RMF_WIREFRAME) {
    gfxDisableTexture();
    
    // set polygon offset
    gfxPolygonMode(GFX_LINE);
    gfxEnableDepthBias();

    // for each ren model 
    INDEX ctrmsh = _aRenModels.Count();
    for(int irmsh=1;irmsh<ctrmsh;irmsh++)
    {
      RenModel &rm = _aRenModels[irmsh];
      // render renmodel in wireframe
      RenderModelWireframe(rm);
    }

    // restore polygon offset
    gfxDisableDepthBias();
    gfxPolygonMode(GFX_FILL);
  }

  extern INDEX ska_bShowColision;
  extern INDEX ska_bShowSkeleton;

  // show skeleton
  if(ska_bShowSkeleton || RM_GetFlags() & RMF_SHOWSKELETON) {
    gfxDisableTexture();
    gfxDisableDepthTest();
    // render skeleton
    RenderSkeleton();
    gfxEnableDepthTest();
  }
  //#pragma message(">> Add ska_bShowActiveBones")
  if(/*ska_bShowActiveBones || */ RM_GetFlags() & RMF_SHOWACTIVEBONES) {
    gfxDisableTexture();
    gfxDisableDepthTest();
    // render only active bones
    RenderActiveBones();
    gfxEnableDepthTest();
  }

  // show root model instance colision box
  if(ska_bShowColision) {
    RM_SetObjectMatrices(mi);
    if (mi.mi_cbAABox.Count()>0)
    {
      ColisionBox &cb = mi.GetCurrentColisionBox();
      RM_RenderColisionBox(mi,cb,C_mlGREEN);
    }
  }

  // reset arrays
  ClearRenArrays();
}

// clear all ren arrays
static void ClearRenArrays()
{
  _pAdjustBonesCallback = NULL;
  _pAdjustBonesData = NULL;
  _pAdjustShaderParams = NULL;
  _pAdjustShaderData = NULL;

  // clear all arrays
  _aRenModels.PopAll();
  _aRenBones.PopAll();
  _aRenMesh.PopAll();
  _aRenWeights.PopAll();
  _aRenMorph.PopAll();
  _fCustomMlodDistance = -1;
  _fCustomSlodDistance = -1;
}

