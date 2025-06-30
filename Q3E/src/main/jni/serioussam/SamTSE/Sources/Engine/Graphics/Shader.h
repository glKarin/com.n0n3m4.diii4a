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

#ifndef SE_INCL_SHADER_H
#define SE_INCL_SHADER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>
#include <Engine/Base/Serial.h>
#include <Engine/Math/Vector.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/GfxLibrary.h>

// Shader flags
#define BASE_DOUBLE_SIDED (1UL<<0) // Double sided
#define BASE_FULL_BRIGHT  (1UL<<1) // Full bright


struct ShaderDesc
{
  CStaticArray<class CTString> sd_astrTextureNames;
  CStaticArray<class CTString> sd_astrTexCoordNames;
  CStaticArray<class CTString> sd_astrColorNames;
  CStaticArray<class CTString> sd_astrFloatNames;
  CStaticArray<class CTString> sd_astrFlagNames;
  CTString sd_strShaderInfo;
};

struct ShaderParams
{
  ShaderParams() {
    sp_ulFlags = 0;
  }
  ~ShaderParams() {
    sp_aiTextureIDs.Clear();
    sp_aiTexCoordsIndex.Clear();
    sp_acolColors.Clear();
    sp_afFloats.Clear();
  }
  CStaticArray<INDEX> sp_aiTextureIDs;
  CStaticArray<INDEX> sp_aiTexCoordsIndex;
  CStaticArray<COLOR> sp_acolColors;
  CStaticArray<FLOAT> sp_afFloats;
  ULONG               sp_ulFlags;
};

class ENGINE_API CShader : public CSerial
{
public:
  CShader();
  ~CShader();
  
  CDynamicLoader *hLibrary;
  void (*ShaderFunc)(void);
  void (*GetShaderDesc)(ShaderDesc &shDesc);

  void Read_t( CTStream *istrFile); // throw char *
  void Write_t( CTStream *ostrFile); // throw char *
  void Clear(void);
  SLONG GetUsedMemory(void);
};

// Begin shader using
ENGINE_API void shaBegin(CAnyProjection3D &aprProjection,CShader *pShader);
// End shader using
ENGINE_API void shaEnd(void);
// Render given model
ENGINE_API void shaRender(void);
// Render aditional pass for fog and haze
ENGINE_API void shaDoFogPass(void);
// Modify color for fog
ENGINE_API void shaModifyColorForFog(void);
// Calculate lightning for given model
ENGINE_API void shaCalculateLight(void);
// Calculate lightning for given model (for specular shader)
ENGINE_API void shaCalculateLightForSpecular(void);
// Clear temp vars used by shader
ENGINE_API void shaClean(void);


// Set array of vertices
ENGINE_API void shaSetVertexArray(GFXVertex4 *paVertices,INDEX ctVertices);
// Set array of normals
ENGINE_API void shaSetNormalArray(GFXNormal *paNormals);
// Set array of indices
ENGINE_API void shaSetIndices(INDEX_T *paIndices, INDEX ctIndices);
// Set array of texture objects for shader
ENGINE_API void shaSetTextureArray(CTextureObject **paTextureObject, INDEX ctTextures);
// Set array of uv maps
ENGINE_API void shaSetUVMapsArray(GFXTexCoord **paUVMaps, INDEX ctUVMaps);
// Set array of shader colors
ENGINE_API void shaSetColorArray(COLOR *paColors, INDEX ctColors);
// Set array of floats for shader
ENGINE_API void shaSetFloatArray(FLOAT *paFloats, INDEX ctFloats);
// Set shading flags
ENGINE_API void shaSetFlags(ULONG ulFlags);
// Set base color of model 
ENGINE_API void shaSetModelColor(COLOR &colModel);
// Set light direction
ENGINE_API void shaSetLightDirection(const FLOAT3D &vLightDir);
// Set light color
ENGINE_API void shaSetLightColor(COLOR colAmbient, COLOR colLight);
// Set object to view matrix
ENGINE_API void shaSetObjToViewMatrix(Matrix12 &mat);
// Set object to abs matrix
ENGINE_API void shaSetObjToAbsMatrix(Matrix12 &mat);


// Set current texture index
ENGINE_API void shaSetTexture(INDEX iTexture);
// Set current uvmap index
ENGINE_API void shaSetUVMap(INDEX iUVMap);
// Set current color index
ENGINE_API void shaSetColor(INDEX icolIndex);
// Set array of texcoords index
ENGINE_API void shaSetTexCoords(GFXTexCoord *uvNewMap);
// Set array of vertex colors
ENGINE_API void shaSetVertexColors(GFXColor *paColors);
// Set constant color
ENGINE_API void shaSetConstantColor(const COLOR colConstant);


// Get vertex count
ENGINE_API INDEX shaGetVertexCount(void);
// Get index count
ENGINE_API INDEX shaGetIndexCount(void);
// Get float from array of floats
ENGINE_API FLOAT shaGetFloat(INDEX iFloatIndex);
// Get texture from array of textures
ENGINE_API CTextureObject *shaGetTexture( INDEX iTextureIndex);
// Get base color from array of colors
ENGINE_API COLOR &shaGetColor(INDEX iColorIndex);
// Get shading flags
ENGINE_API ULONG &shaGetFlags();
// Get base color of model
ENGINE_API COLOR &shaGetModelColor(void);
// Get light direction
ENGINE_API FLOAT3D &shaGetLightDirection(void);
// Get current light color
ENGINE_API COLOR &shaGetLightColor(void);
// Get current ambient volor
ENGINE_API COLOR &shaGetAmbientColor(void);
// Get current set color
ENGINE_API COLOR &shaGetCurrentColor(void);
// Get vertex array
ENGINE_API GFXVertex4 *shaGetVertexArray(void);
// Get index array
ENGINE_API INDEX_T *shaGetIndexArray(void);
// Get normal array
ENGINE_API GFXNormal *shaGetNormalArray(void);
// Get uvmap array from array of uvmaps
ENGINE_API GFXTexCoord *shaGetUVMap( INDEX iUVMapIndex);
// Get color array
ENGINE_API GFXColor *shaGetColorArray(void);

// Get empty color array for modifying
ENGINE_API GFXColor *shaGetNewColorArray(void);
// Get empty texcoords array for modifying
ENGINE_API GFXTexCoord *shaGetNewTexCoordArray(void);
// Get empty v array for modifying
ENGINE_API GFXVertex   *shaGetNewVertexArray(void);

// Get current projection
ENGINE_API CAnyProjection3D *shaGetProjection(void);
// Get object to view matrix
ENGINE_API Matrix12 *shaGetObjToViewMatrix(void);
// Get object to abs matrix
ENGINE_API Matrix12 *shaGetObjToAbsMatrix(void);


// Set face culling
ENGINE_API void shaCullFace(GfxFace eFace);
// Set blending operations
ENGINE_API void shaBlendFunc(GfxBlend eSrc, GfxBlend eDst);
// Set texture modulation mode
ENGINE_API void shaSetTextureModulation(INDEX iScale);
// Enable/Disable blening
ENGINE_API void shaEnableBlend(void);
ENGINE_API void shaDisableBlend(void);
// Enable/Disable alpha test
ENGINE_API void shaEnableAlphaTest(void);
ENGINE_API void shaDisableAlphaTest(void);
// Enable/Disable depth test
ENGINE_API void shaEnableDepthTest(void);
ENGINE_API void shaDisableDepthTest(void);
// Enable/Disable depth write
ENGINE_API void shaEnableDepthWrite(void);
ENGINE_API void shaDisableDepthWrite(void);
// Set depth buffer compare mode
ENGINE_API void shaDepthFunc(GfxComp eComp);
// Set texture wrapping 
ENGINE_API void shaSetTextureWrapping( enum GfxWrap eWrapU, enum GfxWrap eWrapV);

// Set uvmap for fog
ENGINE_API void shaSetFogUVMap(GFXTexCoord *paFogUVMap);
// Set uvmap for haze
ENGINE_API void shaSetHazeUVMap(GFXTexCoord *paHazeUVMap);
// Set array of vertex colors used in haze
ENGINE_API void shaSetHazeColorArray(GFXColor *paHazeColors);

// Is overbrightning enabled
ENGINE_API BOOL shaOverBrightningEnabled(void);

#ifdef PLATFORM_WIN32
#define SHADER_DECLSPEC _declspec(dllexport)
#else
#define SHADER_DECLSPEC
#endif
#define SHADER_MAIN(name) extern "C" void SHADER_DECLSPEC Shader_##name (void)
#define SHADER_DESC(name,x) extern "C" void SHADER_DECLSPEC Shader_Desc_##name (x)

#endif  /* include-once check. */


