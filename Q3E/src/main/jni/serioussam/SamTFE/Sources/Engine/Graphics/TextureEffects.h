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

#ifndef SE_INCL_TEXTUREEFFECTS_H
#define SE_INCL_TEXTUREEFFECTS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Base/Updateable.h>
#include <Engine/Base/Stream.h>

struct TextureEffectPixel {
  char tepp_achDummy[8];
};

static __forceinline CTStream &operator>>(CTStream &strm, TextureEffectPixel &t) {
  strm.Read_t(t.tepp_achDummy, sizeof (t.tepp_achDummy));  // char[8]
  return(strm);
}

static __forceinline CTStream &operator<<(CTStream &strm, const TextureEffectPixel &t) {
  strm.Write_t(t.tepp_achDummy, sizeof (t.tepp_achDummy));  // char[8]
  return(strm);
}


struct TextureEffectSourceProperties {
  char tesp_achDummy[64];
};

static __forceinline CTStream &operator>>(CTStream &strm, TextureEffectSourceProperties &t) {
  strm.Read_t(t.tesp_achDummy, sizeof (t.tesp_achDummy));  // char[64]
  return(strm);
}

static __forceinline CTStream &operator<<(CTStream &strm, const TextureEffectSourceProperties &t) {
  strm.Write_t(t.tesp_achDummy, sizeof (t.tesp_achDummy));  // char[64]
  return(strm);
}

class CTextureEffectSource {
public:
  class CTextureEffectGlobal *tes_ptegGlobalEffect;  // global effect of this effect source
  inline void Clear(void) {};
  ULONG tes_ulEffectSourceType;                 // effect source type
  struct TextureEffectSourceProperties tes_tespEffectSourceProperties;
  CStaticArray<struct TextureEffectPixel> tes_atepPixels;

  // Initialize the texture effect source.
  void Initialize(
   class CTextureEffectGlobal *ptegGlobalEffect,ULONG ulEffectSourceType,
   PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1);
  // Animate the texture effect source.
  void Animate(void);
};

class CTextureEffectGlobal {
public:
  CTextureData *teg_ptdTexture;  // texture of this global effect
  ULONG teg_ulEffectType;
  CUpdateable teg_updTexture;   // when the texture was last updated
  CDynamicArray<CTextureEffectSource> teg_atesEffectSources;

  // Constructor.
  CTextureEffectGlobal(CTextureData *ptdTexture, ULONG ulGlobalEffect);

  // Add a new effect source.
  ENGINE_API void AddEffectSource( ULONG ulEffectSourceType, PIX pixU0, PIX pixV0,
                                                             PIX pixU1, PIX pixV1);
  // animate effect texture
  void Animate(void);
  // render effect texture in required mip level
  void Render( INDEX iWantedMipLevel, PIX pixTexWidth, PIX pixTexHeight);

  // get effect type (true if water type effect, false if plasma or fire effect)
  BOOL IsWater(void);
  // returns number of second it took to render effect texture
  ENGINE_API DOUBLE GetRenderingTime(void);
};

struct TextureEffectSourceType {
  const char *test_strName;    // name used for browsing
  void (*test_Initialize)(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1);   // function for initalization of a new effect source
  void (*test_Animate)(CTextureEffectSource *ptes);   // function for animation of one effect source
};

struct TextureEffectGlobalType {
  const char *tegt_strName;    // name used for browsing
  void (*tegt_Initialize)(void);
  void (*tegt_Animate)(void);

  // all types of effect sources
  INDEX tet_ctEffectSourceTypes;
  struct TextureEffectSourceType *tet_atestEffectSourceTypes;
};

struct TextureEffectGlobalProperties_Water {
  UBYTE tegpw_ubPhong;    // 0 for no phong
};

struct TextureEffectGlobalProperties_Fire {
  UBYTE tegpw_ubRising;    // 0 for no rising
};

ENGINE_API extern INDEX _ctTextureEffectGlobalPresets;
ENGINE_API extern struct TextureEffectGlobalType _ategtTextureEffectGlobalPresets[];


#endif  /* include-once check. */

