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

#undef TEXTURE_COUNT
#undef UVMAPS_COUNT
#undef COLOR_COUNT
#undef FLOAT_COUNT
#undef FLAGS_COUNT

#define TEXTURE_COUNT 2
#define UVMAPS_COUNT  1
#define COLOR_COUNT   2
#define FLOAT_COUNT   1
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


SHADER_MAIN(Detail)
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

  /*
  FLOAT fMul = shaGetFloat(DETAIL_TILING);

  shaSetTexture(BASE_TEXTURE);
  shaSetUVMap(BASE_UVMAP);
  shaSetColor(BASE_COLOR);
  shaCalculateLight();
  shaRender();
  shaEnableBlend(TRUE);
  shaEnableAlphaTest(FALSE);
  shaBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR);
  shaSetTexture(DETAIL_TEXTURE);
  shaSetUVMap(DETAIL_UVMAP);
  shaSetColor(DETAIL_COLOR);

  INDEX ctTexCoords=0;
  GFXTexCoord *ptxcOld = shaGetUVMap(0,ctTexCoords);
  GFXTexCoord *ptxcNew = shaGetEmptyUVMap(ctTexCoords);
  if(ctTexCoords>0)
  {
    for(INDEX itxc=0;itxc<ctTexCoords;itxc++)
    {
      ptxcNew[itxc].u = ptxcOld[itxc].u * fMul;
      ptxcNew[itxc].v = ptxcOld[itxc].v * fMul;
    }
    shaSetUVMap(ptxcNew,ctTexCoords);
  }
  shaRender();
  shaEnableBlend(FALSE);
*/
  if(shaOverBrightningEnabled()) shaSetTextureModulation(1);
}

SHADER_DESC(Detail,ShaderDesc &shDesc)
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
  shDesc.sd_astrFloatNames[0] = "UVMap factor";
  shDesc.sd_astrFlagNames[0] = "Double sided";
  shDesc.sd_astrFlagNames[1] = "Full bright";
  shDesc.sd_strShaderInfo = "Detail shader";
}
