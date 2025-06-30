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

#define TEXTURE_COUNT 1
#define UVMAPS_COUNT  1
#define COLOR_COUNT   1
#define FLOAT_COUNT   0

#define BASE_TEXTURE 0
#define BASE_UVMAP   0
#define BASE_COLOR   0
#define BASE_FLOAT   0

SHADER_MAIN(AddDS)
{
#if 0
  shaSetTexture(BASE_TEXTURE);
  shaSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  shaSetUVMap(BASE_UVMAP);
  shaSetColor(BASE_COLOR);
  shaEnableDepthTest();
  shaDepthFunc(GFX_LESS_EQUAL);
  shaCullFace(GFX_NONE);
  shaCalculateLight();

  shaBlendFunc( GFX_SRC_ALPHA, GFX_ONE);
  shaEnableBlend();
  shaDisableAlphaTest();
  shaDisableDepthWrite();
/*
  COLOR colModelColor = MulColors(shaGetModelColor(),shaGetConstantColor());
  BOOL bOpaque = (colModelColor&0xFF)==0xFF;
  // if fully opaque
  if(bOpaque) {
    shaEnableAlphaTest(FALSE);
    shaEnableBlend(FALSE);
    shaEnableDepthWrite(TRUE);
  // if translucent
  } else {
    shaEnableBlend(TRUE);
    shaBlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    shaEnableDepthWrite(FALSE);
    shaModifyColorForFog();
  }
*/
  if(shaOverBrightningEnabled()) shaSetTextureModulation(2);
  shaRender();
  if(shaOverBrightningEnabled()) shaSetTextureModulation(1);
/*
  if(bOpaque) {
    shaDoFogPass();
  }*/
#endif
}

SHADER_DESC(AddDS,ShaderDesc &shDesc)
{
  shDesc.sd_astrTextureNames.New(TEXTURE_COUNT);
  shDesc.sd_astrTexCoordNames.New(UVMAPS_COUNT);
  shDesc.sd_astrColorNames.New(COLOR_COUNT);
  shDesc.sd_astrFloatNames.New(FLOAT_COUNT);

  shDesc.sd_astrTextureNames[0] = "AddDS texture";
  shDesc.sd_astrTexCoordNames[0] = "AddDS uvmap";
  shDesc.sd_astrColorNames[0] = "AddDS color";
  shDesc.sd_strShaderInfo = "AddDS shader";
}
