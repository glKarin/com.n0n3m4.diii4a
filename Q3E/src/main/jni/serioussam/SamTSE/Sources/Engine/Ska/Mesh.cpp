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
#include "Mesh.h"

#define MESH_VERSION  12
#define MESH_ID       "MESH"

#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Math/Projection.h>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/Stock_CShader.h>

INDEX AreVerticesDiferent(INDEX iCurentIndex, INDEX iLastIndex);

struct VertexLocator
{
  INDEX vl_iIndex;
  INDEX vl_iSubIndex;
};

struct SortArray
{
  INDEX sa_iNewIndex;
  INDEX sa_iSurfaceIndex;
  CStaticArray<struct VertexLocator> sa_aWeightMapList;
  CStaticArray<struct VertexLocator> sa_aMorphMapList;
};

static CStaticArray <struct SortArray> _aSortArray;
CStaticArray <INDEX> _aiOptimizedIndex;
CStaticArray <INDEX> _aiSortedIndex;

MeshLOD *pMeshLOD;// curent mesh lod (for quick sort)
MeshLOD mshOptimized;

CMesh::CMesh()
{
}

CMesh::~CMesh()
{
}

// release old shader and obtain new shader for mesh surface (expand ShaderParams if needed)
void ChangeSurfaceShader_t(MeshSurface &msrf,CTString fnNewShader)
{
  CShader *pShaderNew = _pShaderStock->Obtain_t(fnNewShader);
  ASSERT(pShaderNew!=NULL);
  if(msrf.msrf_pShader!=NULL) _pShaderStock->Release(msrf.msrf_pShader);
  msrf.msrf_pShader = pShaderNew;
  // get new shader description
  ShaderDesc shDesc;
  msrf.msrf_pShader->GetShaderDesc(shDesc);
  // if needed expand size of arrays for new shader
  // reset new values!!!!
  INDEX ctOldTextureIDs = msrf.msrf_ShadingParams.sp_aiTextureIDs.Count();
  INDEX ctNewTextureIDs = shDesc.sd_astrTextureNames.Count();
  INDEX ctOldUVMaps     = msrf.msrf_ShadingParams.sp_aiTexCoordsIndex.Count();
  INDEX ctNewUVMaps     = shDesc.sd_astrTexCoordNames.Count();
  INDEX ctOldColors     = msrf.msrf_ShadingParams.sp_acolColors.Count();
  INDEX ctNewColors     = shDesc.sd_astrColorNames.Count();
  INDEX ctOldFloats     = msrf.msrf_ShadingParams.sp_afFloats.Count();
  INDEX ctNewFloats     = shDesc.sd_astrFloatNames.Count();
  if(ctOldTextureIDs<ctNewTextureIDs) {
    // expand texture IDs array
    msrf.msrf_ShadingParams.sp_aiTextureIDs.Expand(ctNewTextureIDs);
    // set new texture IDs to 0
    for(INDEX itx=ctOldTextureIDs;itx<ctNewTextureIDs;itx++) {
      msrf.msrf_ShadingParams.sp_aiTextureIDs[itx] = -1;
    }
  }
  // expand array of uvmaps if needed
  if(ctOldUVMaps<ctNewUVMaps) {
    // expand uvmaps IDs array
    msrf.msrf_ShadingParams.sp_aiTexCoordsIndex.Expand(ctNewUVMaps);
    // set new uvmaps indices to 0
    for(INDEX itxc=ctOldUVMaps;itxc<ctNewUVMaps;itxc++) {
      msrf.msrf_ShadingParams.sp_aiTexCoordsIndex[itxc] = 0;
    }
  }
  // expand array of colors if needed
  if(ctOldColors<ctNewColors) {
    // expand color array
    msrf.msrf_ShadingParams.sp_acolColors.Expand(ctNewColors);
    // set new colors indices white
    for(INDEX icol=ctOldUVMaps;icol<ctNewColors;icol++) {
      msrf.msrf_ShadingParams.sp_acolColors[icol] = 0xFFFFFFFF;
    }
  }
  // expand array of floats if needed
  if(ctOldFloats<ctNewFloats) {
    // expand float array
    msrf.msrf_ShadingParams.sp_afFloats.Expand(ctNewFloats);
    // set new floats to 0
    for(INDEX ifl=ctOldFloats;ifl<ctNewFloats;ifl++) {
      msrf.msrf_ShadingParams.sp_afFloats[ifl] = 0;
    }
  }
}

// quck sort func for comparing vertices
static int qsort_CompareArray(const void *pVx1, const void *pVx2)
{
  INDEX *n1 = ((INDEX*)pVx1);
  INDEX *n2 = ((INDEX*)pVx2);
  return AreVerticesDiferent(*n1,*n2);
}
// clear array of sort vertices
void ClearSortArray(INDEX ctOldVertices)
{
  for(int iv=0;iv<ctOldVertices;iv++)
  {
    _aSortArray[iv].sa_aWeightMapList.Clear();
    _aSortArray[iv].sa_aMorphMapList.Clear();
  }
  _aiOptimizedIndex.Clear();
  _aiSortedIndex.Clear();
  _aSortArray.Clear();
}
// optimize mesh
void CMesh::Optimize(void)
{
  INDEX ctmshlods = msh_aMeshLODs.Count();
  for(int imshlod=0;imshlod<ctmshlods;imshlod++)
  {
    // optimize each lod in mesh
    OptimizeLod(msh_aMeshLODs[imshlod]);
  }
}
// optimize lod of mesh
void CMesh::OptimizeLod(MeshLOD &mLod)
{
  INDEX ctVertices  = mLod.mlod_aVertices.Count();
  INDEX ctSurfaces  = mLod.mlod_aSurfaces.Count();
  INDEX ctUVMaps    = mLod.mlod_aUVMaps.Count();
  INDEX ctWeightMaps = mLod.mlod_aWeightMaps.Count();
  INDEX ctMorphMaps = mLod.mlod_aMorphMaps.Count();

  if(ctVertices<=0) return;
  
  // create array for sorting
  _aSortArray.New(ctVertices);
  _aiSortedIndex.New(ctVertices);
  _aiOptimizedIndex.New(ctVertices);
  // put original vertex indices in SortArray
  for(int iv=0;iv<ctVertices;iv++)
  {
    _aiSortedIndex[iv] = iv;
  }
  // loop each surface and expand SurfaceList in SortArray
  int is;
  for(is=0;is<ctSurfaces;is++)
  {
    INDEX ctts=mLod.mlod_aSurfaces[is].msrf_aTriangles.Count();
    for(int its=0;its<ctts;its++)
    {
      MeshTriangle &mtTriangle = mLod.mlod_aSurfaces[is].msrf_aTriangles[its];

      _aSortArray[mtTriangle.iVertex[0]].sa_iSurfaceIndex = is;
      _aSortArray[mtTriangle.iVertex[1]].sa_iSurfaceIndex = is;
      _aSortArray[mtTriangle.iVertex[2]].sa_iSurfaceIndex = is;
    }
  }
  // loop each weightmap and expand sa_aWeightMapList in SortArray
  for(INDEX iw=0;iw<ctWeightMaps;iw++)
  {
    // loop each wertex weight array in weight map array
    INDEX ctwm = mLod.mlod_aWeightMaps[iw].mwm_aVertexWeight.Count();
    for(INDEX iwm=0;iwm<ctwm;iwm++)
    {
      MeshVertexWeight &mwwWeight = mLod.mlod_aWeightMaps[iw].mwm_aVertexWeight[iwm];
      // get curent list num of weightmaps  
      INDEX ctWeightMapList = _aSortArray[mwwWeight.mww_iVertex].sa_aWeightMapList.Count();
      // expand array of sufrace lists for 1
      _aSortArray[mwwWeight.mww_iVertex].sa_aWeightMapList.Expand(ctWeightMapList+1);
      // set vl_iIndex to index of surface
      // set vl_iSubIndex to index in triangle set
      VertexLocator &vxLoc = _aSortArray[mwwWeight.mww_iVertex].sa_aWeightMapList[ctWeightMapList];
      vxLoc.vl_iIndex = iw;
      vxLoc.vl_iSubIndex = iwm;
    }
  }
  // loop each morphmap and expand sa_aMorphMapList in SortArray
  for(INDEX im=0;im<ctMorphMaps;im++)
  {
    // loop each morph map in array
    INDEX ctmm = mLod.mlod_aMorphMaps[im].mmp_aMorphMap.Count();
    for(INDEX imm=0;imm<ctmm;imm++)
    {
      MeshVertexMorph &mwmMorph = mLod.mlod_aMorphMaps[im].mmp_aMorphMap[imm];
      // get curent list num of morphmaps  
      INDEX ctMorphMapList = _aSortArray[mwmMorph.mwm_iVxIndex].sa_aMorphMapList.Count();
      // expand array of sufrace lists for 1
      _aSortArray[mwmMorph.mwm_iVxIndex].sa_aMorphMapList.Expand(ctMorphMapList+1);
      // set vl_iIndex to index of surface
      // set vl_iSubIndex to index in triangle set
      VertexLocator &vxLoc = _aSortArray[mwmMorph.mwm_iVxIndex].sa_aMorphMapList[ctMorphMapList];
      vxLoc.vl_iIndex = im;
      vxLoc.vl_iSubIndex = imm;
    }
  }
  // set global pMeshLOD pointer used by quicksort
  pMeshLOD = &mLod;
  // sort array
  qsort(&_aiSortedIndex[0],ctVertices,sizeof(&_aiSortedIndex[0]),qsort_CompareArray);
  
  // compare vertices
  INDEX iDiferentVertices = 1;
  INDEX iLastIndex = _aiSortedIndex[0];
  _aSortArray[iLastIndex].sa_iNewIndex = 0;
  _aiOptimizedIndex[0] = iLastIndex;
  
  for(INDEX isa=1;isa<ctVertices;isa++)
  {
    INDEX iCurentIndex = _aiSortedIndex[isa];
    // check if vertices are diferent
    if(AreVerticesDiferent(iLastIndex,iCurentIndex))
    {
      // add Curent index to Optimized index array
      _aiOptimizedIndex[iDiferentVertices] = iCurentIndex;
      iDiferentVertices++;
      iLastIndex = iCurentIndex;
    }
    _aSortArray[iCurentIndex].sa_iNewIndex = iDiferentVertices-1;
  }

  // create new mesh
  INDEX ctNewVertices = iDiferentVertices;
  mshOptimized.mlod_aVertices.New(ctNewVertices);
  mshOptimized.mlod_aNormals.New(ctNewVertices);
  mshOptimized.mlod_aUVMaps.New(ctUVMaps);
  for(INDEX iuvm=0;iuvm<ctUVMaps;iuvm++)
  {
    mshOptimized.mlod_aUVMaps[iuvm].muv_aTexCoords.New(ctNewVertices);
  }

  // add new vertices and normals to mshOptimized
  for(INDEX iNewVx=0;iNewVx<ctNewVertices;iNewVx++)
  {
    mshOptimized.mlod_aVertices[iNewVx] = mLod.mlod_aVertices[_aiOptimizedIndex[iNewVx]];
    mshOptimized.mlod_aNormals[iNewVx] = mLod.mlod_aNormals[_aiOptimizedIndex[iNewVx]];
    for(INDEX iuvm=0;iuvm<ctUVMaps;iuvm++)
    {
      //???
      mshOptimized.mlod_aUVMaps[iuvm].muv_iID = mLod.mlod_aUVMaps[iuvm].muv_iID;
      mshOptimized.mlod_aUVMaps[iuvm].muv_aTexCoords[iNewVx] = mLod.mlod_aUVMaps[iuvm].muv_aTexCoords[_aiOptimizedIndex[iNewVx]];
    }
  }
  // remap surface triangles
  for(is=0;is<ctSurfaces;is++)
  {
    MeshSurface &msrf = mLod.mlod_aSurfaces[is];
    INDEX iMinIndex = ctNewVertices+1;
    INDEX iMaxIndex = -1;
    INDEX ctts=msrf.msrf_aTriangles.Count();
    // for each triangle in this surface
    INDEX its;
    for(its=0;its<ctts;its++)
    {
      MeshTriangle &mtTriangle = msrf.msrf_aTriangles[its];
      // for each vertex in triangle
      for(INDEX iv=0;iv<3;iv++)
      {
        mtTriangle.iVertex[iv] = _aSortArray[mtTriangle.iVertex[iv]].sa_iNewIndex;
        // find first index in this surface
        if(mtTriangle.iVertex[iv]<iMinIndex) iMinIndex = mtTriangle.iVertex[iv];
        // find last index in this surface
        if(mtTriangle.iVertex[iv]>iMaxIndex) iMaxIndex = mtTriangle.iVertex[iv];
      }
    }
    // remember first index in vertices array
    msrf.msrf_iFirstVertex = iMinIndex;
    // remember vertices count
    msrf.msrf_ctVertices = iMaxIndex-iMinIndex+1;

    // for each triangle in surface
    for(its=0;its<ctts;its++)
    {
      MeshTriangle &mtTriangle = msrf.msrf_aTriangles[its];
      // for each vertex in triangle
      for(INDEX iv=0;iv<3;iv++)
      {
        // substract vertex index in triangle with first vertex in surface
        mtTriangle.iVertex[iv] -= msrf.msrf_iFirstVertex;
        ASSERT(mtTriangle.iVertex[iv]<msrf.msrf_ctVertices);
      }
    }
  }

  // remap weightmaps
  mshOptimized.mlod_aWeightMaps.New(ctWeightMaps);
  // expand wertex veights array for each vertex
  INDEX ivx;
  for(ivx=0;ivx<ctNewVertices;ivx++)
  {
    INDEX ioptVx = _aiOptimizedIndex[ivx];
    for(INDEX iwl=0;iwl<_aSortArray[ioptVx].sa_aWeightMapList.Count();iwl++)
    {
      VertexLocator &wml = _aSortArray[ioptVx].sa_aWeightMapList[iwl];
      INDEX wmIndex = wml.vl_iIndex;
      INDEX wwIndex = wml.vl_iSubIndex;
      INDEX ctww = mshOptimized.mlod_aWeightMaps[wmIndex].mwm_aVertexWeight.Count();
      MeshWeightMap &mwm = mshOptimized.mlod_aWeightMaps[wmIndex];
      MeshVertexWeight &mww = mLod.mlod_aWeightMaps[wmIndex].mwm_aVertexWeight[wwIndex];

      mwm.mwm_iID = mLod.mlod_aWeightMaps[wmIndex].mwm_iID;
      mwm.mwm_aVertexWeight.Expand(ctww+1);
      mwm.mwm_aVertexWeight[ctww].mww_fWeight = mww.mww_fWeight;
      mwm.mwm_aVertexWeight[ctww].mww_iVertex = ivx;
    }
  }

  // remap morphmaps
  mshOptimized.mlod_aMorphMaps.New(ctMorphMaps);
  // expand morph maps array for each vertex
  for(ivx=0;ivx<ctNewVertices;ivx++)
  {
    INDEX ioptVx = _aiOptimizedIndex[ivx];
    for(INDEX iml=0;iml<_aSortArray[ioptVx].sa_aMorphMapList.Count();iml++)
    {
      VertexLocator &mml = _aSortArray[ioptVx].sa_aMorphMapList[iml];
      INDEX mmIndex = mml.vl_iIndex;
      INDEX mwmIndex = mml.vl_iSubIndex;
      INDEX ctmwm = mshOptimized.mlod_aMorphMaps[mmIndex].mmp_aMorphMap.Count();
      MeshMorphMap &mmm = mshOptimized.mlod_aMorphMaps[mmIndex];
      MeshVertexMorph &mwm = mLod.mlod_aMorphMaps[mmIndex].mmp_aMorphMap[mwmIndex];

      mmm.mmp_iID = mLod.mlod_aMorphMaps[mmIndex].mmp_iID;
      mmm.mmp_bRelative = mLod.mlod_aMorphMaps[mmIndex].mmp_bRelative;

      mmm.mmp_aMorphMap.Expand(ctmwm+1);
      mmm.mmp_aMorphMap[ctmwm].mwm_iVxIndex = ivx;
      mmm.mmp_aMorphMap[ctmwm].mwm_x = mwm.mwm_x;
      mmm.mmp_aMorphMap[ctmwm].mwm_y = mwm.mwm_y;
      mmm.mmp_aMorphMap[ctmwm].mwm_z = mwm.mwm_z;
      mmm.mmp_aMorphMap[ctmwm].mwm_nx = mwm.mwm_nx;
      mmm.mmp_aMorphMap[ctmwm].mwm_ny = mwm.mwm_ny;
      mmm.mmp_aMorphMap[ctmwm].mwm_nz = mwm.mwm_nz;
    }
  }

  mLod.mlod_aVertices.CopyArray(mshOptimized.mlod_aVertices);
  mLod.mlod_aNormals.CopyArray(mshOptimized.mlod_aNormals);
  mLod.mlod_aMorphMaps.CopyArray(mshOptimized.mlod_aMorphMaps);
  mLod.mlod_aWeightMaps.CopyArray(mshOptimized.mlod_aWeightMaps);
  mLod.mlod_aUVMaps.CopyArray(mshOptimized.mlod_aUVMaps);

  // clear memory
  ClearSortArray(ctVertices);
  mshOptimized.mlod_aVertices.Clear();
  mshOptimized.mlod_aNormals.Clear();
  mshOptimized.mlod_aWeightMaps.Clear();
  mshOptimized.mlod_aMorphMaps.Clear();
  mshOptimized.mlod_aUVMaps.Clear();
}

INDEX AreVerticesDiferent(INDEX iCurentIndex, INDEX iLastIndex)
{
#define CHECK(x,y) if(((x)-(y))!=0) return (INDEX) ((x)-(y))
#define CHECKF(x,y) if(((x)-(y))!=0) return (INDEX) Sgn((x)-(y))
  
  // check surfaces
  CHECK(_aSortArray[iCurentIndex].sa_iSurfaceIndex,_aSortArray[iLastIndex].sa_iSurfaceIndex);
  // check vertices
  CHECKF(pMeshLOD->mlod_aVertices[iCurentIndex].y,pMeshLOD->mlod_aVertices[iLastIndex].y);
  CHECKF(pMeshLOD->mlod_aVertices[iCurentIndex].x,pMeshLOD->mlod_aVertices[iLastIndex].x);
  CHECKF(pMeshLOD->mlod_aVertices[iCurentIndex].z,pMeshLOD->mlod_aVertices[iLastIndex].z);
  // check normals
  CHECKF(pMeshLOD->mlod_aNormals[iCurentIndex].ny,pMeshLOD->mlod_aNormals[iLastIndex].ny);
  CHECKF(pMeshLOD->mlod_aNormals[iCurentIndex].nx,pMeshLOD->mlod_aNormals[iLastIndex].nx);
  CHECKF(pMeshLOD->mlod_aNormals[iCurentIndex].nz,pMeshLOD->mlod_aNormals[iLastIndex].nz);
  // check uvmaps
  INDEX ctUVMaps = pMeshLOD->mlod_aUVMaps.Count();
  for(INDEX iuvm=0;iuvm<ctUVMaps;iuvm++)
  {
    CHECKF(pMeshLOD->mlod_aUVMaps[iuvm].muv_aTexCoords[iCurentIndex].u,pMeshLOD->mlod_aUVMaps[iuvm].muv_aTexCoords[iLastIndex].u);
    CHECKF(pMeshLOD->mlod_aUVMaps[iuvm].muv_aTexCoords[iCurentIndex].v,pMeshLOD->mlod_aUVMaps[iuvm].muv_aTexCoords[iLastIndex].v);
  }
  // count weight and morph maps
  INDEX ctwmCurent  = _aSortArray[iCurentIndex].sa_aWeightMapList.Count();
  INDEX ctwmLast    = _aSortArray[iLastIndex].sa_aWeightMapList.Count();
  INDEX ctmmCurent  = _aSortArray[iCurentIndex].sa_aMorphMapList.Count();
  INDEX ctmmLast    = _aSortArray[iLastIndex].sa_aMorphMapList.Count();
  // check if vertices have same weight and morph maps count
  CHECK(ctwmCurent,ctwmLast);
  CHECK(ctmmCurent,ctmmLast);
  // check if vertices have same weight map factors
  for(INDEX iwm=0;iwm<ctwmCurent;iwm++)
  {
    // get weight map indices
    INDEX iwmCurent = _aSortArray[iCurentIndex].sa_aWeightMapList[iwm].vl_iIndex;
    INDEX iwmLast   = _aSortArray[iLastIndex].sa_aWeightMapList[iwm].vl_iIndex;
    // get wertex weight indices
    INDEX iwwCurent = _aSortArray[iCurentIndex].sa_aWeightMapList[iwm].vl_iSubIndex;
    INDEX iwwLast   = _aSortArray[iLastIndex].sa_aWeightMapList[iwm].vl_iSubIndex;
    // if weight map factors are diferent
    CHECKF(pMeshLOD->mlod_aWeightMaps[iwmCurent].mwm_aVertexWeight[iwwCurent].mww_fWeight,pMeshLOD->mlod_aWeightMaps[iwmLast].mwm_aVertexWeight[iwwLast].mww_fWeight);
  }

  // check if vertices have same morph map factors
  for(INDEX imm=0;imm<ctmmCurent;imm++)
  {
    // get morph map indices
    INDEX immCurent = _aSortArray[iCurentIndex].sa_aMorphMapList[imm].vl_iIndex;
    INDEX immLast   = _aSortArray[iLastIndex].sa_aMorphMapList[imm].vl_iIndex;
    // get mesh vertex morph indices
    INDEX imwmCurent  = _aSortArray[iCurentIndex].sa_aMorphMapList[imm].vl_iSubIndex;
    INDEX imwmLast    = _aSortArray[iLastIndex].sa_aMorphMapList[imm].vl_iSubIndex;
    
    // if mesh morph map params are diferent return
    CHECKF(pMeshLOD->mlod_aMorphMaps[immCurent].mmp_aMorphMap[imwmCurent].mwm_x,
      pMeshLOD->mlod_aMorphMaps[immLast].mmp_aMorphMap[imwmLast].mwm_x);
    CHECKF(pMeshLOD->mlod_aMorphMaps[immCurent].mmp_aMorphMap[imwmCurent].mwm_y,
      pMeshLOD->mlod_aMorphMaps[immLast].mmp_aMorphMap[imwmLast].mwm_y);
    CHECKF(pMeshLOD->mlod_aMorphMaps[immCurent].mmp_aMorphMap[imwmCurent].mwm_z,
      pMeshLOD->mlod_aMorphMaps[immLast].mmp_aMorphMap[imwmLast].mwm_z);
    CHECKF(pMeshLOD->mlod_aMorphMaps[immCurent].mmp_aMorphMap[imwmCurent].mwm_nx,
      pMeshLOD->mlod_aMorphMaps[immLast].mmp_aMorphMap[imwmLast].mwm_nx);
    CHECKF(pMeshLOD->mlod_aMorphMaps[immCurent].mmp_aMorphMap[imwmCurent].mwm_ny,
      pMeshLOD->mlod_aMorphMaps[immLast].mmp_aMorphMap[imwmLast].mwm_ny);
    CHECKF(pMeshLOD->mlod_aMorphMaps[immCurent].mmp_aMorphMap[imwmCurent].mwm_nz,
      pMeshLOD->mlod_aMorphMaps[immLast].mmp_aMorphMap[imwmLast].mwm_nz);
  }
  return 0;
}
// normalize weights in mlod
void CMesh::NormalizeWeightsInLod(MeshLOD &mlod)
{
  CStaticArray<float> aWeightFactors;
  int ctvtx = mlod.mlod_aVertices.Count();
  int ctwm = mlod.mlod_aWeightMaps.Count();
  // create array for weights
  aWeightFactors.New(ctvtx);
  memset(&aWeightFactors[0],0,sizeof(aWeightFactors[0])*ctvtx);
  int iwm;
  for(iwm=0;iwm<ctwm;iwm++)
  {
    MeshWeightMap &mwm = mlod.mlod_aWeightMaps[iwm];
    for(int iww=0;iww<mwm.mwm_aVertexWeight.Count();iww++)
    {
      MeshVertexWeight &mwh = mwm.mwm_aVertexWeight[iww];
       aWeightFactors[mwh.mww_iVertex] += mwh.mww_fWeight;
    }
  }

  for(iwm=0;iwm<ctwm;iwm++)
  {
    MeshWeightMap &mwm = mlod.mlod_aWeightMaps[iwm];
    for(int iww=0;iww<mwm.mwm_aVertexWeight.Count();iww++)
    {
      MeshVertexWeight &mwh = mwm.mwm_aVertexWeight[iww];
      mwh.mww_fWeight /= aWeightFactors[mwh.mww_iVertex];
    }
  }
  // clear weight array
  aWeightFactors.Clear();
}
// normalize weights in mesh
void CMesh::NormalizeWeights()
{
  INDEX ctmlods = msh_aMeshLODs.Count();
  for(INDEX imlod=0;imlod<ctmlods;imlod++)
  {
    // normalize each lod
    NormalizeWeightsInLod(msh_aMeshLODs[imlod]);
  }
}
// add new mesh lod to mesh
void CMesh::AddMeshLod(MeshLOD &mlod)
{
  INDEX ctmlods = msh_aMeshLODs.Count();
  msh_aMeshLODs.Expand(ctmlods+1);
  msh_aMeshLODs[ctmlods] = mlod;
}
// remove mesh lod from mesh
void CMesh::RemoveMeshLod(MeshLOD *pmlodRemove)
{
  INDEX ctmlod = msh_aMeshLODs.Count();
  // create temp space for skeleton lods
  CStaticArray<struct MeshLOD> aTempMLODs;
  aTempMLODs.New(ctmlod-1);
  INDEX iIndexSrc=0;

  // for each skeleton lod in skeleton
  for(INDEX imlod=0;imlod<ctmlod;imlod++)
  {
    MeshLOD *pmlod = &msh_aMeshLODs[imlod];
    // copy all skeleton lods except the selected one
    if(pmlod != pmlodRemove)
    {
      aTempMLODs[iIndexSrc] = *pmlod;
      iIndexSrc++;
    }
  }
  // copy temp array of skeleton lods back in skeleton
  msh_aMeshLODs.CopyArray(aTempMLODs);
  // clear temp skleletons lods array
  aTempMLODs.Clear();
}
// write to stream
void CMesh::Write_t(CTStream *ostrFile)
{
  INDEX ctmlods = msh_aMeshLODs.Count();

  // write id
  ostrFile->WriteID_t(CChunkID(MESH_ID));
  // write version
  (*ostrFile)<<(INDEX)MESH_VERSION;
  // write mlod count
  (*ostrFile)<<ctmlods;
  // for each lod in mesh
  for(INDEX imlod=0;imlod<ctmlods;imlod++) {
    MeshLOD &mLod = msh_aMeshLODs[imlod];

    INDEX ctVx = mLod.mlod_aVertices.Count();   // vertex count
    INDEX ctUV = mLod.mlod_aUVMaps.Count();     // uvmaps count
    INDEX ctSf = mLod.mlod_aSurfaces.Count();   // surfaces count
    INDEX ctWM = mLod.mlod_aWeightMaps.Count(); // weight maps count
    INDEX ctMM = mLod.mlod_aMorphMaps.Count();  // morph maps count
    // write source file name
    (*ostrFile)<<mLod.mlod_fnSourceFile;
    // write max distance
    (*ostrFile)<<mLod.mlod_fMaxDistance;
    // write flags
    (*ostrFile)<<mLod.mlod_ulFlags;

    // write wertex count
    (*ostrFile)<<ctVx;
    // write wertices
    ostrFile->Write_t(&mLod.mlod_aVertices[0],sizeof(MeshVertex)*ctVx);
    // write normals
    ostrFile->Write_t(&mLod.mlod_aNormals[0],sizeof(MeshNormal)*ctVx);

    // write uvmaps count
    (*ostrFile)<<ctUV;
    // write uvmaps
    for(int iuv=0;iuv<ctUV;iuv++) {
      // write uvmap ID
      CTString strNameID = ska_GetStringFromTable(mLod.mlod_aUVMaps[iuv].muv_iID);
      (*ostrFile)<<strNameID;
      // write uvmaps texcordinates
      ostrFile->Write_t(&mLod.mlod_aUVMaps[iuv].muv_aTexCoords[0],sizeof(MeshTexCoord)*ctVx);
    }

    // write surfaces count
    ostrFile->Write_t(&ctSf,sizeof(INDEX));
    // write surfaces
    for(INDEX isf=0;isf<ctSf;isf++) {
      MeshSurface &msrf = mLod.mlod_aSurfaces[isf];
      INDEX ctTris = msrf.msrf_aTriangles.Count();
      CTString strSurfaceID = ska_GetStringFromTable(msrf.msrf_iSurfaceID);
      // write surface ID
      (*ostrFile)<<strSurfaceID;
      // write first vertex
      (*ostrFile)<<msrf.msrf_iFirstVertex;
      // write vertices count
      (*ostrFile)<<msrf.msrf_ctVertices;
      // write tris count
      (*ostrFile)<<ctTris;
      // write triangles
      ostrFile->Write_t(&mLod.mlod_aSurfaces[isf].msrf_aTriangles[0],sizeof(MeshTriangle)*ctTris);

      // write bool that this surface has a shader
      INDEX bShaderExists = (msrf.msrf_pShader!=NULL);
      (*ostrFile)<<bShaderExists;
      if(bShaderExists) {
        // get shader decription
        ShaderDesc shDesc;
        msrf.msrf_pShader->GetShaderDesc(shDesc);
        INDEX cttx=shDesc.sd_astrTextureNames.Count();
        INDEX cttc=shDesc.sd_astrTexCoordNames.Count();
        INDEX ctcol=shDesc.sd_astrColorNames.Count();
        INDEX ctfl=shDesc.sd_astrFloatNames.Count();
        // data count must be at same as size defined in shader or higher
        ASSERT(cttx<=msrf.msrf_ShadingParams.sp_aiTextureIDs.Count());
        ASSERT(cttc<=msrf.msrf_ShadingParams.sp_aiTexCoordsIndex.Count());
        ASSERT(ctcol<=msrf.msrf_ShadingParams.sp_acolColors.Count());
        ASSERT(ctfl<=msrf.msrf_ShadingParams.sp_afFloats.Count());
        ASSERT(msrf.msrf_pShader->GetShaderDesc!=NULL);
        // write texture count 
        (*ostrFile)<<cttx;
        // write texture coords count 
        (*ostrFile)<<cttc;
        // write color count 
        (*ostrFile)<<ctcol;
        // write float count 
        (*ostrFile)<<ctfl;

        ASSERT(msrf.msrf_pShader!=NULL);
        // write shader name
        CTString strShaderName;
        strShaderName = msrf.msrf_pShader->GetName();
        (*ostrFile)<<strShaderName;
        // write shader texture IDs
        for(INDEX itx=0;itx<cttx;itx++)
        {
          INDEX iTexID = msrf.msrf_ShadingParams.sp_aiTextureIDs[itx];
          (*ostrFile)<<ska_GetStringFromTable(iTexID);
        }
        // write shader texture coords indices
        for(INDEX itc=0;itc<cttc;itc++)
        {
          INDEX iTexCoorsIndex = msrf.msrf_ShadingParams.sp_aiTexCoordsIndex[itc];
          (*ostrFile)<<iTexCoorsIndex;
        }
        // write shader colors
        for(INDEX icol=0;icol<ctcol;icol++)
        {
          COLOR colColor = msrf.msrf_ShadingParams.sp_acolColors[icol];
          (*ostrFile)<<colColor;
        }
        // write shader floats
        for(INDEX ifl=0;ifl<ctfl;ifl++)
        {
          FLOAT fFloat = msrf.msrf_ShadingParams.sp_afFloats[ifl];
          (*ostrFile)<<fFloat;
        }
        // write shader flags
        ULONG ulFlags = msrf.msrf_ShadingParams.sp_ulFlags;
        (*ostrFile)<<ulFlags;
      }
    }

    // write weightmaps count
    (*ostrFile)<<ctWM;
    // for each weightmap in array
    for(INDEX iwm=0;iwm<ctWM;iwm++)
    {
      INDEX ctWw = mLod.mlod_aWeightMaps[iwm].mwm_aVertexWeight.Count();
      // write wertex weight map ID
      CTString pstrNameID = ska_GetStringFromTable(mLod.mlod_aWeightMaps[iwm].mwm_iID);
      (*ostrFile)<<pstrNameID;
      // write wertex weights count
      (*ostrFile)<<ctWw;
      // write wertex weights
      ostrFile->Write_t(&mLod.mlod_aWeightMaps[iwm].mwm_aVertexWeight[0],sizeof(MeshVertexWeight)*ctWw);
    }

    // write morphmaps count
    (*ostrFile)<<ctMM;
    for(INDEX imm=0;imm<ctMM;imm++)
    {
      INDEX ctms = mLod.mlod_aMorphMaps[imm].mmp_aMorphMap.Count();
      // write ID
      CTString pstrNameID = ska_GetStringFromTable(mLod.mlod_aMorphMaps[imm].mmp_iID);
      (*ostrFile)<<pstrNameID;
      // write bRelative
      (*ostrFile)<<mLod.mlod_aMorphMaps[imm].mmp_bRelative;
      //ostrFile->Write_t(&mLod.mlod_aMorphMaps[imm].mmp_bRelative,sizeof(BOOL));
      // write morph sets count
      ostrFile->Write_t(&ctms,sizeof(INDEX));
      // write morph sets
      ostrFile->Write_t(&mLod.mlod_aMorphMaps[imm].mmp_aMorphMap[0],sizeof(MeshVertexMorph)*ctms);
    }
  }
}

//read from stream
void CMesh::Read_t(CTStream *istrFile)
{
  INDEX ctmlods;
  INDEX iFileVersion;
  // read chunk id
  istrFile->ExpectID_t(CChunkID(MESH_ID));
  // check file version
  (*istrFile)>>iFileVersion;
  
  // if file version is not 11 nor 12
  if(iFileVersion != 11 && iFileVersion!=12) {
		ThrowF_t(TRANS("File '%s'.\nInvalid Mesh file version.\nExpected Ver \"%d\" but found \"%d\"\n"),
      (const char*)istrFile->GetDescription(),MESH_VERSION,iFileVersion);
    return;
  }

  // read mlod count
  (*istrFile)>>ctmlods;
  // for each lod in mesh
  for(INDEX imlod=0;imlod<ctmlods;imlod++) {
    // expand mlod count for one 
    INDEX ctMeshLODs = msh_aMeshLODs.Count();
    msh_aMeshLODs.Expand(ctMeshLODs+1);
    MeshLOD &mLod = msh_aMeshLODs[ctMeshLODs];

    INDEX ctVx;   // vertex count
    INDEX ctUV;   // uvmaps count
    INDEX ctSf;   // surfaces count
    INDEX ctWM;   // weight maps count
    INDEX ctMM;   // morph maps count
    
    // read source file name
    (*istrFile)>>mLod.mlod_fnSourceFile;
    // read max distance
    (*istrFile)>>mLod.mlod_fMaxDistance;
    // read flags
    (*istrFile)>>mLod.mlod_ulFlags;

    // :)
    if(iFileVersion<=11) {
      mLod.mlod_ulFlags = 0;
    }
    if(mLod.mlod_ulFlags==0xCDCDCDCD) {
      mLod.mlod_ulFlags = 0;
    }


    // read vertex count
    (*istrFile)>>ctVx;
    // create vertex and normal arrays
    mLod.mlod_aVertices.New(ctVx);
    mLod.mlod_aNormals.New(ctVx);
    // read vertices
    for (INDEX i = 0; i < ctVx; i++)
      (*istrFile)>>mLod.mlod_aVertices[i];
    // read normals
    for (INDEX i = 0; i < ctVx; i++)
      (*istrFile)>>mLod.mlod_aNormals[i];

    // read uvmaps count
    (*istrFile)>>ctUV;
    // create array for uvmaps
    mLod.mlod_aUVMaps.New(ctUV);
    // read uvmaps
    for(int iuv=0;iuv<ctUV;iuv++) {
      // read uvmap ID
      CTString strNameID;
      (*istrFile)>>strNameID;
      mLod.mlod_aUVMaps[iuv].muv_iID = ska_GetIDFromStringTable(strNameID);
      // create array for uvmaps texcordinates
      mLod.mlod_aUVMaps[iuv].muv_aTexCoords.New(ctVx);
      // read uvmap texcordinates
      for (INDEX i = 0; i < ctVx; i++)
        (*istrFile)>>mLod.mlod_aUVMaps[iuv].muv_aTexCoords[i];
    }
    // read surfaces count
    (*istrFile)>>ctSf;
    // create array for surfaces
    mLod.mlod_aSurfaces.New(ctSf);
    // read surfaces
    for(INDEX isf=0;isf<ctSf;isf++) {
      INDEX ctTris;
      MeshSurface &msrf = mLod.mlod_aSurfaces[isf];
      // read surface ID
      CTString strSurfaceID;
      (*istrFile)>>strSurfaceID;
      msrf.msrf_iSurfaceID = ska_GetIDFromStringTable(strSurfaceID);
      // read first vertex
      (*istrFile)>>msrf.msrf_iFirstVertex;
      // read vertices count
      (*istrFile)>>msrf.msrf_ctVertices;
      // read tris count
      (*istrFile)>>ctTris;
      // create triangles array
      mLod.mlod_aSurfaces[isf].msrf_aTriangles.New(ctTris);
      // read triangles
      for (INDEX i = 0; i < ctTris; i++)
        (*istrFile)>>mLod.mlod_aSurfaces[isf].msrf_aTriangles[i];

      // read bool that this surface has a shader
      INDEX bShaderExists;
      (*istrFile)>>bShaderExists;
      // if shader exists read its params
      if(bShaderExists) {
        INDEX cttx,cttc,ctcol,ctfl;
        // read texture count
        (*istrFile)>>cttx;
        // read texture coords count
        (*istrFile)>>cttc;
        // read color count
        (*istrFile)>>ctcol;
        // read float count
        (*istrFile)>>ctfl;

        //CShader *pshMeshShader = NULL;
        ShaderParams *pshpShaderParams = NULL;
        CShader shDummyShader;            // dummy shader if shader is not found
        ShaderParams shpDummyShaderParams;// dummy shader params if shader is not found
        // read shader name
        CTString strShaderName;
        (*istrFile)>>strShaderName;
        // try to load shader
        try{
          msrf.msrf_pShader = _pShaderStock->Obtain_t(strShaderName);
          //pshMeshShader = msrf.msrf_pShader;
          pshpShaderParams = &msrf.msrf_ShadingParams;
        } catch (const char *strErr) {
          CPrintF("%s\n",strErr);
          msrf.msrf_pShader = NULL;
          //pshMeshShader = &shDummyShader;
          pshpShaderParams = &shpDummyShaderParams;
        }

        // if mesh shader exisits
        if(msrf.msrf_pShader!=NULL) {
          // get shader description
          ShaderDesc shDesc;
          msrf.msrf_pShader->GetShaderDesc(shDesc);
          // check if saved params count match shader params count
          if(shDesc.sd_astrTextureNames.Count() != cttx) ThrowF_t("File '%s'\nWrong texture count %d",(const char*)GetName(),cttx);
          if(shDesc.sd_astrTexCoordNames.Count() != cttc) ThrowF_t("File '%s'\nWrong uvmaps count %d",(const char*)GetName(),cttc);
          if(shDesc.sd_astrColorNames.Count() != ctcol) ThrowF_t("File '%s'\nWrong colors count %d",(const char*)GetName(),ctcol);
          if(shDesc.sd_astrFloatNames.Count() != ctfl) ThrowF_t("File '%s'\nWrong floats count %d",(const char*)GetName(),ctfl);
        }

        // create arrays for shader params
        pshpShaderParams->sp_aiTextureIDs.New(cttx);
        pshpShaderParams->sp_aiTexCoordsIndex.New(cttc);
        pshpShaderParams->sp_acolColors.New(ctcol);
        pshpShaderParams->sp_afFloats.New(ctfl);

        // read shader texture IDs
        for(INDEX itx=0;itx<cttx;itx++) {
          CTString strTexID;
          (*istrFile)>>strTexID;
          INDEX iTexID = ska_GetIDFromStringTable(strTexID);
           pshpShaderParams->sp_aiTextureIDs[itx] = iTexID;
        }
        // read shader texture coords indices
        for(INDEX itc=0;itc<cttc;itc++) {
          INDEX iTexCoorsIndex;
          (*istrFile)>>iTexCoorsIndex;
          pshpShaderParams->sp_aiTexCoordsIndex[itc] = iTexCoorsIndex;
        }
        // read shader colors
        for(INDEX icol=0;icol<ctcol;icol++) {
          COLOR colColor;
          (*istrFile)>>colColor;
          pshpShaderParams->sp_acolColors[icol] = colColor;
        }
        // read shader floats
        for(INDEX ifl=0;ifl<ctfl;ifl++) {
          FLOAT fFloat;
          (*istrFile)>>fFloat;
          pshpShaderParams->sp_afFloats[ifl] = fFloat;
        }
        // there were no flags in shader before ver 12
        if(iFileVersion>11) {
          ULONG ulFlags;
          (*istrFile)>>ulFlags;
          pshpShaderParams->sp_ulFlags = ulFlags;
        } else {
          pshpShaderParams->sp_ulFlags = 0;
        }
      } else {
        // this surface does not have shader
        msrf.msrf_pShader=NULL;
      }
    }

    // read weightmaps count
    (*istrFile)>>ctWM;
    // create weightmap array
     mLod.mlod_aWeightMaps.New(ctWM);
     // read each weightmap
    for(INDEX iwm=0;iwm<ctWM;iwm++) {
      // read weightmap ID
      CTString pstrNameID;
      (*istrFile)>>pstrNameID;
      mLod.mlod_aWeightMaps[iwm].mwm_iID = ska_GetIDFromStringTable(pstrNameID);
      // read wertex weight count
      INDEX ctWw;
      (*istrFile)>>ctWw;
      // create wertex weight array
      mLod.mlod_aWeightMaps[iwm].mwm_aVertexWeight.New(ctWw);
      // read wertex weights
      for (INDEX i = 0; i < ctWw; i++)
        (*istrFile)>>mLod.mlod_aWeightMaps[iwm].mwm_aVertexWeight[i];
    }

    // read morphmap count
    (*istrFile)>>ctMM;
    // create morphmaps array
    mLod.mlod_aMorphMaps.New(ctMM);
    // read morphmaps
    for(INDEX imm=0;imm<ctMM;imm++) {
      // read morphmap ID
      CTString pstrNameID;
      (*istrFile)>>pstrNameID;
      mLod.mlod_aMorphMaps[imm].mmp_iID = ska_GetIDFromStringTable(pstrNameID);
      // read bRelative
      (*istrFile)>>mLod.mlod_aMorphMaps[imm].mmp_bRelative;
      // read morph sets count
      INDEX ctms;
      (*istrFile)>>ctms;
      // create morps sets array
      mLod.mlod_aMorphMaps[imm].mmp_aMorphMap.New(ctms);
      // read morph sets
      for (INDEX i = 0; i < ctms; i++)
        (*istrFile)>>mLod.mlod_aMorphMaps[imm].mmp_aMorphMap[i];
    }
  }
}
// clear mesh
void CMesh::Clear(void)
{
  // for each LOD
  INDEX ctmlod = msh_aMeshLODs.Count();
  for (INDEX imlod=0; imlod<ctmlod; imlod++)
  {
    // for each surface, clear the triangles list
    MeshLOD &mlod = msh_aMeshLODs[imlod];
    INDEX ctsrf = mlod.mlod_aSurfaces.Count();
    for (INDEX isrf=0;isrf<ctsrf;isrf++)
    {
      MeshSurface &msrf = mlod.mlod_aSurfaces[isrf];
      msrf.msrf_aTriangles.Clear();
      // release shader form stock
      if(msrf.msrf_pShader!=NULL) _pShaderStock->Release(msrf.msrf_pShader);
      msrf.msrf_pShader = NULL;
    }
    // clear the surfaces array
    mlod.mlod_aSurfaces.Clear();
    // for each uvmap, clear the texcord list
    INDEX ctuvm = mlod.mlod_aUVMaps.Count();
    for (INDEX iuvm=0;iuvm<ctuvm;iuvm++)
    {
      mlod.mlod_aUVMaps[iuvm].muv_aTexCoords.Clear();
    }
    // clear the uvmaps array
    mlod.mlod_aUVMaps.Clear();
    // clear the vertices array
    mlod.mlod_aVertices.Clear();
    // clear the normals array
    mlod.mlod_aNormals.Clear();
  }
  // in the end, clear all LODs
  msh_aMeshLODs.Clear();
}

// Count used memory
SLONG CMesh::GetUsedMemory(void)
{
  SLONG slMemoryUsed = sizeof(*this);
  INDEX ctmlods = msh_aMeshLODs.Count();
  for(INDEX imlod=0;imlod<ctmlods;imlod++) {
    MeshLOD &mlod = msh_aMeshLODs[imlod];
    slMemoryUsed+=sizeof(mlod);
    slMemoryUsed+=mlod.mlod_aVertices.Count() * sizeof(MeshVertex);
    slMemoryUsed+=mlod.mlod_aNormals.Count() * sizeof(MeshNormal);

    // for each uvmap
    INDEX ctuvmaps = mlod.mlod_aUVMaps.Count();
    for(INDEX iuvm=0;iuvm<ctuvmaps;iuvm++) {
      MeshUVMap &uvmap = mlod.mlod_aUVMaps[iuvm];
      slMemoryUsed+=sizeof(uvmap);
      slMemoryUsed+=uvmap.muv_aTexCoords.Count() * sizeof(MeshTexCoord);
    }

    // for each surface
    INDEX ctmsrf = mlod.mlod_aSurfaces.Count();
    for(INDEX imsrf=0;imsrf<ctmsrf;imsrf++) {
      MeshSurface &msrf = mlod.mlod_aSurfaces[imsrf];
      slMemoryUsed+=sizeof(msrf);
      slMemoryUsed+=msrf.msrf_aTriangles.Count() * sizeof(MeshTriangle);
      slMemoryUsed+=sizeof(ShaderParams);
      slMemoryUsed+=sizeof(INDEX) * msrf.msrf_ShadingParams.sp_aiTextureIDs.Count();
      slMemoryUsed+=sizeof(INDEX) * msrf.msrf_ShadingParams.sp_aiTexCoordsIndex.Count();
      slMemoryUsed+=sizeof(COLOR) * msrf.msrf_ShadingParams.sp_acolColors.Count();
      slMemoryUsed+=sizeof(FLOAT) * msrf.msrf_ShadingParams.sp_afFloats.Count();
    }
    // for each weight map
    INDEX ctwm = mlod.mlod_aWeightMaps.Count();
    for(INDEX iwm=0;iwm<ctwm;iwm++) {
      MeshWeightMap &mwm = mlod.mlod_aWeightMaps[iwm];
      slMemoryUsed+=sizeof(mwm);
      slMemoryUsed+=mwm.mwm_aVertexWeight.Count() * sizeof(MeshVertexWeight);
    }
    // for each morphmap
    INDEX ctmm = mlod.mlod_aMorphMaps.Count();
    for(INDEX imm=0;imm<ctmm;imm++) {
      MeshMorphMap &mmm = mlod.mlod_aMorphMaps[imm];
      slMemoryUsed+=sizeof(mmm);
      slMemoryUsed+=mmm.mmp_aMorphMap.Count() * sizeof(MeshVertexMorph);
    }
  }
  return slMemoryUsed;
}
