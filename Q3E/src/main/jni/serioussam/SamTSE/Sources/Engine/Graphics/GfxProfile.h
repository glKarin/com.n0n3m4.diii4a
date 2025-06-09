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

#ifndef SE_INCL_GFXPROFILE_H
#define SE_INCL_GFXPROFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Profiling.h>

/* Class for holding profiling information for gfx. */
class CGfxProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex
  {
    PTI_LOCKDRAWPORT,
    PTI_SWAPBUFFERS,
    PTI_PUTTEXTURE,
    PTI_PUTTEXT,
    PTI_CACHESHADOW,
    PTI_SETCURRENTTEXTURE,
    PTI_TEXTUREPARAMS,
    PTI_TEXTUREUPLOADING,
    PTI_MAKEMIPMAPS,
    PTI_DITHERBITMAP,
    PTI_FILTERBITMAP,

    PTI_RENDERSCENE,
    PTI_RENDERSCENE_BCG,
    PTI_RENDERSCENE_ZONLY,
    PTI_RS_LOCKARRAYS,
    PTI_RS_DRAWELEMENTS,
    PTI_RS_REMOVEDUMMY,
    PTI_RS_CHECKLAYERSUPTODATE,
    PTI_RS_BINTOGROUPS,
    PTI_RS_MAKEMIPFACTOR,
    PTI_RS_MAKEVERTEXCOORDS,
    PTI_RS_SETCOLORS,
    PTI_RS_SETTEXCOORDS,
    PTI_RS_RENDERGROUP,
    PTI_RS_RENDERGROUPINTERNAL,
    PTI_RS_BINBYMULTITEXTURING,
    PTI_RS_RENDERSHADOWS,
    PTI_RS_RENDERTEXTURES,
    PTI_RS_RENDERMT,
    PTI_RS_RENDERFOG,
    PTI_RS_RENDERHAZE,

    PTI_COUNT
  };

  enum ProfileCounterIndex
  {
    PCI_TEXTUREUPLOADS,     // how many times a texture was uploaded
    PCI_TEXTUREUPLOADBYTES, // how many texture bytes were uploaded
    PCI_TEXTUREPREPARES,    // how many times a texture was prepared
    PCI_TEXTUREBYTESALLOWED,
    PCI_CACHEDSHADOWS,      // number of shadowmaps cached
    PCI_FLATSHADOWS,        // number of flat shadowmaps
    PCI_CACHEDSHADOWBYTES,  // shadowmap bytes cached
    PCI_DYNAMICSHADOWS,      
    PCI_DYNAMICSHADOWBYTES,  

    PCI_RS_TRIANGLES,
    PCI_RS_TRIANGLEPASSESORG,
    PCI_RS_TRIANGLEPASSESOPT,
    PCI_RS_POLYGONGROUPS,
    PCI_COUNT,
  };
  // constructor
  CGfxProfile(void);
};


#endif  /* include-once check. */

