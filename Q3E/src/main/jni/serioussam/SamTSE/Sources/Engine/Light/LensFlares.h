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

#ifndef SE_INCL_LENSFLARES_H
#define SE_INCL_LENSFLARES_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Texture.h>
#include <Engine/Templates/StaticArray.h>

/*
 * Structure describing one flare in lens flare effect.
 */
#define OLF_FADESIZE            (1L<<0)   // change size when fading
#define OLF_FADEINTENSITY       (1L<<1)   // change intensity when fading
#define OLF_FADEOFCENTER        (1L<<2)   // fade if away from screen center

class ENGINE_API COneLensFlare {
public:
  CTextureObject olf_toTexture;       // texture used for this flare
  FLOAT olf_fReflectionPosition;      // 0=light 1=center of screen
  FLOAT olf_fSizeIOverScreenSizeI;    // flare dimensions relative to screen size
  FLOAT olf_fSizeJOverScreenSizeI;
  FLOAT olf_fLightAmplification;      // amplification for light color
  FLOAT olf_fLightDesaturation;       // desaturation factor for light color (1=monochrome)
  FLOAT oft_fFallOffFactor;           // flare falloff relative to light falloff
  ANGLE olf_aRotationFactor;          // flare rotation (deg/screen width)
  ULONG olf_ulFlags;
};

/*
 * Structure describing specific kind of lens flare effect.
 */
class ENGINE_API CLensFlareType {
public:
  CStaticArray<COneLensFlare> lft_aolfFlares; // all flares for this effect
  // glaring when light source is near screen center
  FLOAT lft_fGlareCompression;        // glaring compression towards center
  FLOAT lft_fGlareIntensity;          // maximum glare intensity
  FLOAT lft_fGlareFallOffFactor;      // glare falloff relative to light falloff
  FLOAT lft_fGlareDesaturation;  // desaturation factor for center glare (1=monochrome)
};


#endif  /* include-once check. */

