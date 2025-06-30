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
#define UVMAPS_COUNT  2
#define COLOR_COUNT   2
#define FLOAT_COUNT   2
#define FLAGS_COUNT   2

#define LAYER0_TEXTURE 0
#define LAYER0_UVMAP   0
#define LAYER0_COLOR   0
#define LAYER0_TILING  0

#define LAYER1_TEXTURE 1
#define LAYER1_UVMAP   1
#define LAYER1_COLOR   1
#define LAYER1_TILING  1

#define BASE_DOUBLE_SIDED (1UL<<0) // Double sided
#define BASE_FULL_BRIGHT  (1UL<<1) // Full bright

SHADER_MAIN(MultiLayer)
{
  // this will be reused for all layers
  FLOAT fLayerTiling = 1.0f;

  // do 0th layer pass - base layer
  shaSetTexture(LAYER0_TEXTURE);
  shaSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  shaSetUVMap(LAYER0_UVMAP);
  shaSetColor(LAYER0_COLOR);
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

  fLayerTiling = shaGetFloat(LAYER0_TILING);
  if (fLayerTiling!=1.0f) {
    GFXTexCoord *ptxcOld = shaGetUVMap(LAYER0_UVMAP);
    GFXTexCoord *ptxcNew = shaGetNewTexCoordArray();
    INDEX ctTexCoords = shaGetVertexCount();
    if(ctTexCoords>0 && ptxcOld!=NULL) {
      for(INDEX itxc=0;itxc<ctTexCoords;itxc++)
      {
        ptxcNew[itxc].uv.u = ptxcOld[itxc].uv.u * fLayerTiling;
        ptxcNew[itxc].uv.v = ptxcOld[itxc].uv.v * fLayerTiling;
      }
      shaSetTexCoords(ptxcNew);
    }
  }
  shaRender();
  if(bOpaque) {
    shaDoFogPass();
  }

  // do 1st layer pass
  
  fLayerTiling = shaGetFloat(LAYER1_TILING);
  shaBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);  
  shaSetTexture(LAYER1_TEXTURE);
  shaSetUVMap(LAYER1_UVMAP);
  shaSetColor(LAYER1_COLOR);
  shaCalculateLight();
  
  shaEnableBlend();
  
  if (fLayerTiling!=1.0f) {
    GFXTexCoord *ptxcOld = shaGetUVMap(LAYER1_UVMAP);
    GFXTexCoord *ptxcNew = shaGetNewTexCoordArray();
    INDEX ctTexCoords = shaGetVertexCount();
    if(ctTexCoords>0 && ptxcOld!=NULL) {
      for(INDEX itxc=0; itxc<ctTexCoords; itxc++)
      {
        ptxcNew[itxc].uv.u = ptxcOld[itxc].uv.u * fLayerTiling;
        ptxcNew[itxc].uv.v = ptxcOld[itxc].uv.v * fLayerTiling;
      }
      shaSetTexCoords(ptxcNew);
    }
  }
  shaRender();
  shaDisableBlend();

  if(shaOverBrightningEnabled()) shaSetTextureModulation(1);
}

SHADER_DESC(MultiLayer, ShaderDesc &shDesc)
{
  shDesc.sd_astrTextureNames .New(TEXTURE_COUNT);
  shDesc.sd_astrTexCoordNames.New(UVMAPS_COUNT);
  shDesc.sd_astrColorNames   .New(COLOR_COUNT);
  shDesc.sd_astrFloatNames   .New(FLOAT_COUNT);
  shDesc.sd_astrFlagNames    .New(FLAGS_COUNT);

  // layer 0 - base
  shDesc.sd_astrTextureNames [0] = "Layer0* texture";
  shDesc.sd_astrTexCoordNames[0] = "Layer0* UVMap";
  shDesc.sd_astrColorNames   [0] = "Layer0* color";
  shDesc.sd_astrFloatNames   [0] = "Layer0* tiling factor";
  // layer 1
  shDesc.sd_astrTextureNames [1] = "Layer1 texture";
  shDesc.sd_astrTexCoordNames[1] = "Layer1 UVMap";
  shDesc.sd_astrColorNames   [1] = "Layer1 color";
  shDesc.sd_astrFloatNames   [1] = "Layer1 tiling factor";
  
  shDesc.sd_astrFlagNames[0] = "Double sided";
  shDesc.sd_astrFlagNames[1] = "Full bright";
   
  shDesc.sd_strShaderInfo = "Multi Layer shader";
}
