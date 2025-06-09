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

#define TEXTURE_COUNT 2
#define UVMAPS_COUNT  1
#define COLOR_COUNT   2
#define FLOAT_COUNT   4
#define FLAGS_COUNT   2

#define BASE_TEXTURE 0
#define BASE_UVMAP   0
#define BASE_COLOR   0

#define DETAIL_TEXTURE 1
#define DETAIL_UVMAP   1
#define DETAIL_COLOR   1
#define DETAIL_TILING  0

#define BASE_DOUBLE_SIDED (1UL<<0) // Double sided
#define BASE_FULL_BRIGHT  (1UL<<1) // Full bright

#define FLOAT_UVMAPF    0
#define FLOAT_AMPLITUDE 1
#define FLOAT_RIPPLES   2
#define FLOAT_FREQUENCY 3

SHADER_MAIN(LavaDisplace)
{
  shaSetTexture(BASE_TEXTURE);
  shaSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  shaSetUVMap(BASE_UVMAP);
  shaSetColor(BASE_COLOR);
  shaEnableDepthTest();
  shaDepthFunc(GFX_LESS_EQUAL);

  COLOR &colModelColor = shaGetModelColor();
  BOOL bDoubleSided = shaGetFlags()&BASE_DOUBLE_SIDED;
  BOOL bOpaque = (colModelColor&0xFF)==0xFF;

  if(bDoubleSided) {
    shaCullFace(GFX_NONE);
  } else {
    shaCullFace(GFX_BACK);
  }

  shaCalculateLight();

  // if fully opaque
  if(bOpaque) {
    // shaEnableAlphaTest(TRUE);
    shaDisableBlend();
    shaEnableDepthWrite();
  // if translucent
  } else {
    // shaEnableAlphaTest(FALSE);
    shaEnableBlend();
    shaBlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    shaDisableDepthWrite();
    shaModifyColorForFog();
  }

  if(shaOverBrightningEnabled()) shaSetTextureModulation(2);
  
  // displace geometry
  GFXVertex4 *paVertices = shaGetVertexArray();
  GFXVertex4 *paNewVertices = shaGetNewVertexArray();
  INDEX ctVertices = shaGetVertexCount();
  // get some values, but clamp them
  FLOAT fAmplitude = Clamp(shaGetFloat(FLOAT_AMPLITUDE), 0.0f, 0.75f);
  FLOAT fRipples   = shaGetFloat(FLOAT_RIPPLES);  
  FLOAT fFrequency = shaGetFloat(FLOAT_FREQUENCY);  
  Matrix12 &mInvAbsToViewer = *shaGetObjToAbsMatrix();
  Matrix12 mAbsToView;
  MatrixTranspose(mAbsToView, mInvAbsToViewer);
   
  // for each vertex
  for(INDEX ivx=0; ivx<ctVertices; ivx++) {
    paNewVertices[ivx] = paVertices[ivx];
    
    TransformVertex(paNewVertices[ivx],mInvAbsToViewer);
    paNewVertices[ivx].x *= 1.0f + fAmplitude * sin((paNewVertices[ivx].y+(_pTimer->GetLerpedCurrentTick()*fFrequency))*fRipples);
    paNewVertices[ivx].z *= 1.0f + fAmplitude * sin((paNewVertices[ivx].y+(_pTimer->GetLerpedCurrentTick()*fFrequency))*fRipples);
    //paNewVertices[ivx].x += fAmplitude * sin((paNewVertices[ivx].y+(_pTimer->GetLerpedCurrentTick()*fFrequency))*fRipples);
    //paNewVertices[ivx].y += vDisplace.y;
    //paNewVertices[ivx].z += vDisplace.z;
    TransformVertex(paNewVertices[ivx],mAbsToView);
  }
  shaSetVertexArray(paNewVertices, ctVertices);
  
  shaRender();

  if(bOpaque) {
    shaDoFogPass();
  }

  // do detail pass
  FLOAT fMul = shaGetFloat(DETAIL_TILING);
  shaBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR);
  shaSetTexture(DETAIL_TEXTURE);
  shaSetUVMap(DETAIL_UVMAP);
  shaSetColor(DETAIL_COLOR);
  shaCalculateLight();

  shaEnableBlend();

  GFXTexCoord *ptxcOld = shaGetUVMap(0);
  GFXTexCoord *ptxcNew = shaGetNewTexCoordArray();
  INDEX ctTexCoords = shaGetVertexCount();
  if(ctTexCoords>0) {
    for(INDEX itxc=0;itxc<ctTexCoords;itxc++)
    {
      ptxcNew[itxc].uv.u = ptxcOld[itxc].uv.u * fMul;
      ptxcNew[itxc].uv.v = ptxcOld[itxc].uv.v * fMul;
    }
    shaSetTexCoords(ptxcNew);
  }
  shaRender();
  shaDisableBlend();

  if(shaOverBrightningEnabled()) shaSetTextureModulation(1);
}

SHADER_DESC(LavaDisplace, ShaderDesc &shDesc)
{
  shDesc.sd_astrTextureNames.New(TEXTURE_COUNT);
  shDesc.sd_astrTexCoordNames.New(UVMAPS_COUNT);
  shDesc.sd_astrColorNames.New(COLOR_COUNT);
  shDesc.sd_astrFloatNames.New(FLOAT_COUNT);
  shDesc.sd_astrFlagNames.New(FLAGS_COUNT);

  shDesc.sd_astrTextureNames[0] = "Base texture";
  shDesc.sd_astrTextureNames[1] = "Detail texture";
  shDesc.sd_astrTexCoordNames[0] = "Base UVMap";
  shDesc.sd_astrColorNames[0] = "Surface color";
  shDesc.sd_astrColorNames[1] = "Detail color";
  shDesc.sd_astrFloatNames[FLOAT_UVMAPF] = "UVMap factor";
  shDesc.sd_astrFlagNames[0] = "Double sided";
  shDesc.sd_astrFlagNames[1] = "Full bright";
  shDesc.sd_strShaderInfo = "Detail shader";
  shDesc.sd_astrFloatNames[FLOAT_AMPLITUDE] = "Amp (max 0.75)";
  shDesc.sd_astrFloatNames[FLOAT_RIPPLES]   = "Ripple density";
  shDesc.sd_astrFloatNames[FLOAT_FREQUENCY]    = "Ripple speed";
}
 
