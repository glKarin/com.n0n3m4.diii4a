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

#include "Shaders/StdH.h"
#include <Shaders/Common.h>

void DoSpecularLayer(INDEX iSpeculaTexture,INDEX iSpecularColor)
{
  //GFXVertex4 *paVertices = shaGetVertexArray();
  GFXNormal *paNormals = shaGetNormalArray();
  INDEX ctVertices = shaGetVertexCount();
  FLOAT3D vLightDir = -shaGetLightDirection().Normalize();
  COLOR colLight = ByteSwap(shaGetLightColor());
  COLOR colAmbient = ByteSwap(shaGetAmbientColor());
  GFXTexCoord *ptcUVMap = shaGetNewTexCoordArray();
  Matrix12 &mObjToView = *shaGetObjToViewMatrix();
  shaCalculateLightForSpecular();


  // determine multitexturing capability for overbrighting purposes
  const BOOL bOverbright = shaOverBrightningEnabled();

  // cache light intensities (-1 in case of overbrighting compensation)
  const INDEX iBright = bOverbright ?  0 : 1;
  SLONG slLR = (colLight & CT_RMASK)>>(CT_RSHIFT-iBright);
  SLONG slLG = (colLight & CT_GMASK)>>(CT_GSHIFT-iBright);
  SLONG slLB = (colLight & CT_BMASK)>>(CT_BSHIFT-iBright);
  SLONG slAR = (colAmbient & CT_RMASK)>>(CT_RSHIFT-iBright);
  SLONG slAG = (colAmbient & CT_GMASK)>>(CT_GSHIFT-iBright);
  SLONG slAB = (colAmbient & CT_BMASK)>>(CT_BSHIFT-iBright);
  if( bOverbright) {
    slAR = ClampUp( slAR, (SLONG)127);
    slAG = ClampUp( slAG, (SLONG)127);
    slAB = ClampUp( slAB, (SLONG)127);
  }


  // for each vertex
  INDEX ivx;
  for(ivx=0;ivx<ctVertices;ivx++) {
    // reflect light vector around vertex normal in object space
    GFXNormal &nor = paNormals[ivx];
    FLOAT3D vNot = FLOAT3D(nor.nx,nor.ny,nor.nz);
    // vNot.Normalize();
    //ASSERT(vNot.Normalize() == 1.0f);
    const FLOAT fNL = nor.nx*vLightDir(1) + nor.ny*vLightDir(2) +	nor.nz*vLightDir(3);
    const FLOAT fRx = vLightDir(1) - 2*vNot(1)*fNL;
    const FLOAT fRy = vLightDir(2) - 2*vNot(2)*fNL;
    const FLOAT fRz = vLightDir(3) - 2*vNot(3)*fNL;

    FLOAT3D fRV = FLOAT3D(fRx,fRy,fRz);
    RotateVector(fRV.vector,mObjToView);
    // map reflected vector to texture
    const FLOAT f1oFM = 0.5f / sqrt(2+2*fRV(3));  // was 2*sqrt(2+2*fRVz)
    ptcUVMap[ivx].st.s = fRV(1)*f1oFM +0.5f;
    ptcUVMap[ivx].st.t = fRV(2)*f1oFM +0.5f;
  }

  GFXColor colSrfSpec = shaGetColor(iSpecularColor);
  colSrfSpec.AttenuateRGB( (shaGetModelColor()&CT_AMASK)>>CT_ASHIFT);
  colSrfSpec.ub.r = ClampUp( (colSrfSpec.ub.r *slLR)>>8, (SLONG)255);
  colSrfSpec.ub.g = ClampUp( (colSrfSpec.ub.g *slLG)>>8, (SLONG)255);
  colSrfSpec.ub.b = ClampUp( (colSrfSpec.ub.b *slLB)>>8, (SLONG)255);
  GFXColor *pcolSpec = shaGetNewColorArray();
  GFXColor *pcolBase = shaGetColorArray();;
  
  // for each vertex in the surface
  for(ivx=0;ivx<ctVertices;ivx++) {
    // set specular color
    const SLONG slShade = pcolBase[ivx].ub.a;
    pcolSpec[ivx].ul.abgr =    (((colSrfSpec.ub.r)*slShade)>>8)
                             | (((colSrfSpec.ub.g)*slShade)&0x0000FF00)
                             |((((colSrfSpec.ub.b)*slShade)<<8)&0x00FF0000);
  }
  

  shaSetTexCoords(ptcUVMap);
  shaSetVertexColors(pcolSpec);
  shaSetTexture(iSpeculaTexture);
  shaBlendFunc( GFX_INV_SRC_ALPHA, GFX_ONE);
  shaEnableBlend();
  shaCullFace(GFX_BACK);
  shaRender();
  shaCullFace(GFX_FRONT);
  shaRender();
}

void DoReflectionLayer(INDEX iReflectionTexture,INDEX iReflectionColor,BOOL bFullBright)
{
  //GFXVertex4 *paVertices = NULL;
  GFXNormal *paNormals = NULL;
  //paVertices = shaGetVertexArray();
  paNormals = shaGetNormalArray();
  INDEX ctVertices = shaGetVertexCount();
  GFXTexCoord *ptcUVMap = shaGetNewTexCoordArray();
  //Matrix12 &mObjToView = *shaGetObjToViewMatrix();
  Matrix12 &mObjToAbs  = *shaGetObjToAbsMatrix();
  CAnyProjection3D &apr = *shaGetProjection();

  // calculate projection of viewer in absolute space
  FLOATmatrix3D &mViewer = apr->pr_ViewerRotationMatrix;
  FLOAT3D vViewer = FLOAT3D(-mViewer(3,1),-mViewer(3,2),-mViewer(3,3));


  Matrix12 mTemp,mInvert;
  MatrixVectorToMatrix12(mTemp,mViewer,FLOAT3D(0,0,0));
  MatrixTranspose(mInvert,mTemp);
  // mObjToAbs = !mViewer;

  // for each vertex
  for(INDEX ivx=0;ivx<ctVertices;ivx++) {
    // reflect light vector around vertex normal in object space
    FLOAT3D vNor = FLOAT3D(paNormals[ivx].nx,paNormals[ivx].ny,paNormals[ivx].nz);
    RotateVector(vNor.vector,mObjToAbs);

    // reflect viewer around normal
    const FLOAT fNV  = vNor(1)*vViewer(1) + vNor(2)*vViewer(2) + vNor(3)*vViewer(3);
    const FLOAT fRVx = vViewer(1) - 2*vNor(1)*fNV;
    const FLOAT fRVy = vViewer(2) - 2*vNor(2)*fNV;
    const FLOAT fRVz = vViewer(3) - 2*vNor(3)*fNV;

    // map reflected vector to texture 
    // NOTE: using X and Z axes, so that singularity gets on -Y axis (where it will least probably be seen)
    const FLOAT f1oFM = 0.5f / sqrt(2+2*fRVy);
    ptcUVMap[ivx].st.s = fRVx*f1oFM +0.5f;
    ptcUVMap[ivx].st.t = fRVz*f1oFM +0.5f;
  }

  GFXColor *pcolReflection = shaGetNewColorArray();
  
  // get model reflection color
  GFXColor colSrfRefl;
  colSrfRefl.ul.abgr = ByteSwap(shaGetColor(iReflectionColor));
  colSrfRefl.AttenuateA((shaGetModelColor()&CT_AMASK)>>CT_ASHIFT);

  if(bFullBright) {
    // just copy reflection color
    for( INDEX ivx=0;ivx<ctVertices;ivx++) {
      pcolReflection[ivx] = colSrfRefl;
    }
  } else {
  GFXColor *pcolSrfBase = shaGetColorArray();
    // set reflection color smooth
    for( INDEX ivx=0;ivx<ctVertices;ivx++) {
      pcolReflection[ivx].MultiplyRGBCopyA1( colSrfRefl, pcolSrfBase[ivx]);
    }
  }

  shaSetTexCoords(ptcUVMap);
  shaSetVertexColors(pcolReflection);
  shaSetTexture(iReflectionTexture);
  shaBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  shaEnableBlend();

  BOOL bDoubleSided = shaGetFlags()&BASE_DOUBLE_SIDED;
  if(bDoubleSided) {
    shaCullFace(GFX_FRONT);
    shaRender();
  }
  shaCullFace(GFX_BACK);
  shaRender();
}
