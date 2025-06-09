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

#include "Engine/StdH.h"

#include <Engine/Graphics/GfxProfile.h>

// profile form for profiling gfx
CGfxProfile gpGfxProfile;
CProfileForm &_pfGfxProfile = gpGfxProfile;

CGfxProfile::CGfxProfile(void)
 : CProfileForm ("Gfx", "frames",
    CGfxProfile::PCI_COUNT, CGfxProfile::PTI_COUNT)
{
  // initialize timers
  SETTIMERNAME(PTI_LOCKDRAWPORT, "LockDrawPort()", "");
  SETTIMERNAME(PTI_SWAPBUFFERS,  "SwapBuffers()", "");
  SETTIMERNAME(PTI_PUTTEXTURE,   "PutTexture()", "");
  SETTIMERNAME(PTI_PUTTEXT,      "PutText()", "");
  SETTIMERNAME(PTI_CACHESHADOW,  "CacheShadow", "");
  SETTIMERNAME(PTI_SETCURRENTTEXTURE, "SetCurrentTexture", "setting");
  SETTIMERNAME(PTI_TEXTUREPARAMS,     " TextureParams", "");
  SETTIMERNAME(PTI_TEXTUREUPLOADING,  " TextureUploading", "");
  SETTIMERNAME(PTI_MAKEMIPMAPS,  "MakeMipmaps()", "");
  SETTIMERNAME(PTI_DITHERBITMAP, "DitherBitmap()", "");
  SETTIMERNAME(PTI_FILTERBITMAP, "FilterBitmap()", "");

  SETTIMERNAME(PTI_RENDERSCENE,       "RenderScene", "");
  SETTIMERNAME(PTI_RENDERSCENE_BCG,   "rs_RenderScene_bcg", "");
  SETTIMERNAME(PTI_RENDERSCENE_ZONLY, "rs_RenderScene_zonly", "");
  SETTIMERNAME(PTI_RS_LOCKARRAYS,     "rs_LockArrays", "");
  SETTIMERNAME(PTI_RS_DRAWELEMENTS,   "rs_DrawElements", "");
  SETTIMERNAME(PTI_RS_REMOVEDUMMY,    "rs_RemoveDummy", "");
  SETTIMERNAME(PTI_RS_CHECKLAYERSUPTODATE, "rs_CheckLayers", "");
  SETTIMERNAME(PTI_RS_BINTOGROUPS,         "rs_BinToGroups", "");
  SETTIMERNAME(PTI_RS_MAKEMIPFACTOR,       " rs_MakeMipAdjustMap", "");
  SETTIMERNAME(PTI_RS_MAKEVERTEXCOORDS,    "rs_MakeVertexCoords", "");
  SETTIMERNAME(PTI_RS_SETCOLORS,           "rs_SetColors", "");
  SETTIMERNAME(PTI_RS_SETTEXCOORDS,        "rs_SetTexcoords", "");
  SETTIMERNAME(PTI_RS_RENDERGROUP,         "rs_RenderGroup", "");
  SETTIMERNAME(PTI_RS_RENDERGROUPINTERNAL, " rs_RenderGroupInternal", "");
  SETTIMERNAME(PTI_RS_BINBYMULTITEXTURING, " rs_BinByMultiTexturing", "");
  SETTIMERNAME(PTI_RS_RENDERSHADOWS,  "rs_RenderShadows", "");
  SETTIMERNAME(PTI_RS_RENDERTEXTURES, "rs_RenderTextures", "");
  SETTIMERNAME(PTI_RS_RENDERMT,       "rs_RenderMT", "");
  SETTIMERNAME(PTI_RS_RENDERFOG,      "rs_RenderFog", "");
  SETTIMERNAME(PTI_RS_RENDERHAZE,     "rs_RenderHaze", "");

  // initialize counters
  SETCOUNTERNAME(PCI_TEXTUREPREPARES, "texture prepares");
  SETCOUNTERNAME(PCI_TEXTUREUPLOADS,  "texture uploads");
  SETCOUNTERNAME(PCI_TEXTUREUPLOADBYTES, "texture bytes uploaded");
  SETCOUNTERNAME(PCI_CACHEDSHADOWS,      "number of shadows cached");
  SETCOUNTERNAME(PCI_FLATSHADOWS,        "number of flat shadows");
  SETCOUNTERNAME(PCI_CACHEDSHADOWBYTES,  "shadow bytes cached");
  SETCOUNTERNAME(PCI_DYNAMICSHADOWS,     "number of dynamic shadows cached");
  SETCOUNTERNAME(PCI_DYNAMICSHADOWBYTES, "dynamic shadow bytes cached");
  SETCOUNTERNAME(PCI_RS_TRIANGLES,          "RS: triangles");
  SETCOUNTERNAME(PCI_RS_TRIANGLEPASSESORG,  "RS: triangle*passes");
  SETCOUNTERNAME(PCI_RS_TRIANGLEPASSESOPT,  "RS: triangle*passesMT");
  SETCOUNTERNAME(PCI_RS_POLYGONGROUPS,      "RS: polygon groups");
}
