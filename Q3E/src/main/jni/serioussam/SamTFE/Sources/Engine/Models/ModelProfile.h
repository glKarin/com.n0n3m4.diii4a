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

#ifndef SE_INCL_MODELPROFILE_H
#define SE_INCL_MODELPROFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Profiling.h>

/* Class for holding profiling information for models. */
class CModelProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex {
    PTI_INITMODELRENDERING,
    PTI_INITPROJECTION,
    PTI_INITSHADOWPROJECTION,
    PTI_INITATTACHMENTS,
    PTI_CREATEATTACHMENT,
    PTI_RENDERMODEL,
    PTI_RENDERSHADOW,
    PTI_RENDERSIMPLESHADOW,

    PTI_VIEW_PREPAREFORRENDERING,
    PTI_VIEW_SETMODELVIEW,
    PTI_VIEW_INIT_UNPACK,
    PTI_VIEW_INIT_VERTICES,
    PTI_VIEW_INIT_FOG_MIP,       
    PTI_VIEW_INIT_HAZE_MIP,       
    PTI_VIEW_RENDERMODEL,
    PTI_VIEW_INIT_DIFF_SURF,
    PTI_VIEW_INIT_BUMP_SURF,
    PTI_VIEW_INIT_REFL_MIP ,
    PTI_VIEW_INIT_REFL_SURF,
    PTI_VIEW_INIT_SPEC_MIP, 
    PTI_VIEW_INIT_SPEC_SURF,
    PTI_VIEW_INIT_FOG_SURF,      
    PTI_VIEW_INIT_HAZE_SURF,      
    PTI_VIEW_ATTENUATE_SURF,

    PTI_VIEW_RENDER_DIFFUSE,
    PTI_VIEW_RENDER_BUMP,
    PTI_VIEW_RENDER_REFLECTIONS,
    PTI_VIEW_RENDER_SPECULAR,
    PTI_VIEW_RENDER_FOG,          
    PTI_VIEW_RENDER_HAZE,         

    PTI_VIEW_SETTEXTURE,
    PTI_VIEW_LOCKARRAYS,
    PTI_VIEW_DRAWELEMENTS,
    PTI_VIEW_ONESIDE,
    PTI_VIEW_ONESIDE_GLSETUP,

    PTI_VIEW_RENDERSHADOW,
    PTI_VIEW_SHAD_INIT_MIP,
    PTI_VIEW_SHAD_INIT_SURF,
    PTI_VIEW_SHAD_GLSETUP,
    PTI_VIEW_SHAD_RENDER,
    PTI_VIEW_RENDERSIMPLESHADOW,
    PTI_VIEW_SIMP_CALC,
    PTI_VIEW_SIMP_PREP,
    PTI_VIEW_SIMP_COPY,
    PTI_VIEW_SIMP_BATCHED,

    PTI_VIEW_RENDERPATCHES,

    PTI_MASK_INITMODELRENDERING,
    PTI_MASK_RENDERMODEL,

    PTI_COUNT
  };

  enum ProfileCounterIndex {

    PCI_VERTICES_FIRSTMIP,
    PCI_VERTICES_USEDMIP,
    PCI_SURFACEVERTICES_FIRSTMIP,
    PCI_SURFACEVERTICES_USEDMIP,
    PCI_TRIANGLES_FIRSTMIP,
    PCI_TRIANGLES_USEDMIP,

    PCI_SHADOWVERTICES_FIRSTMIP,
    PCI_SHADOWVERTICES_USEDMIP,
    PCI_SHADOWSURFACEVERTICES_FIRSTMIP,
    PCI_SHADOWSURFACEVERTICES_USEDMIP,
    PCI_SHADOWTRIANGLES_FIRSTMIP,
    PCI_SHADOWTRIANGLES_USEDMIP,

    PCI_VIEW_TRIANGLES,

    PCI_MASK_TRIANGLES,
    PCI_MASK_POLYGONS,

    PCI_COUNT
  };
  // constructor
  CModelProfile(void);
  // override to provide external averaging from gfx profile
  virtual INDEX GetAveragingCounter(void);
};


#endif  /* include-once check. */

