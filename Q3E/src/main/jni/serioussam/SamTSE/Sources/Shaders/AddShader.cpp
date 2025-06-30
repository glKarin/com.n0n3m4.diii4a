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
#define FLAGS_COUNT   2

#define BASE_TEXTURE 0
#define BASE_UVMAP   0
#define BASE_COLOR   0
#define BASE_FLOAT   0

SHADER_MAIN(Add)
{
  shaSetTexture(BASE_TEXTURE);
  shaSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  shaSetUVMap(BASE_UVMAP);
  shaSetColor(BASE_COLOR);
  shaEnableDepthTest();
  shaDepthFunc(GFX_LESS_EQUAL);

  if(shaGetFlags()&BASE_DOUBLE_SIDED) {
    shaCullFace(GFX_NONE);
  } else {
    shaCullFace(GFX_BACK);
  }

  shaCalculateLight();

  shaBlendFunc( GFX_SRC_ALPHA, GFX_ONE);
  shaEnableBlend();
  shaDisableAlphaTest();
  shaDisableDepthWrite();

  if(shaOverBrightningEnabled()) shaSetTextureModulation(2);
  shaRender();
  if(shaOverBrightningEnabled()) shaSetTextureModulation(1);
}

SHADER_DESC(Add,ShaderDesc &shDesc)
{
  shDesc.sd_astrTextureNames.New(TEXTURE_COUNT);
  shDesc.sd_astrTexCoordNames.New(UVMAPS_COUNT);
  shDesc.sd_astrColorNames.New(COLOR_COUNT);
  shDesc.sd_astrFloatNames.New(FLOAT_COUNT);
  shDesc.sd_astrFlagNames.New(FLAGS_COUNT);

  shDesc.sd_astrTextureNames[0] = "Add texture";
  shDesc.sd_astrTexCoordNames[0] = "Add uvmap";
  shDesc.sd_astrColorNames[0] = "Add color";
  shDesc.sd_astrFlagNames[0] = "Double sided";
  shDesc.sd_astrFlagNames[1] = "Full bright";
  shDesc.sd_strShaderInfo = "Add shader";
}
