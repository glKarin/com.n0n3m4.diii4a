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

#include <Engine/Models/ModelProfile.h>

// profile form for profiling models
CModelProfile mpModelProfile;
CProfileForm &_pfModelProfile = mpModelProfile;

// override to provide external averaging from gfx profile
INDEX CModelProfile::GetAveragingCounter(void)
{
  return _pfGfxProfile.GetAveragingCounter();
}

CModelProfile::CModelProfile(void)
             : CProfileForm ("Model", "frames", CModelProfile::PCI_COUNT, CModelProfile::PTI_COUNT)
{
  // initialize timers
  SETTIMERNAME(PTI_INITMODELRENDERING,    "InitModelRendering", "model");
  SETTIMERNAME(PTI_INITPROJECTION,        "  InitProjection", "model");
  SETTIMERNAME(PTI_INITSHADOWPROJECTION,  "  InitShadowprojection", "model");
  SETTIMERNAME(PTI_INITATTACHMENTS,       "  InitAttachments", "attachment");
  SETTIMERNAME(PTI_CREATEATTACHMENT,      "    CreateAttachment", "attachment");
  SETTIMERNAME(PTI_RENDERMODEL,           "RenderModel", "model");
  SETTIMERNAME(PTI_RENDERSHADOW,          "RenderShadow", "shadow");
  SETTIMERNAME(PTI_RENDERSIMPLESHADOW,    "RenderSimpleShadow", "shadow");

  SETTIMERNAME(PTI_VIEW_PREPAREFORRENDERING, "View_PrepareForRendering", "");
  SETTIMERNAME(PTI_VIEW_SETMODELVIEW,        "  View_SetModelView", "setting");
  SETTIMERNAME(PTI_VIEW_INIT_UNPACK,         "View_Init_Unpack", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_VERTICES,       "View_Init_Vertices", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_FOG_MIP,        "View_Init_Fog_mip ",  "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_HAZE_MIP,       "View_Init_Haze_mip ", "vertex");

  SETTIMERNAME(PTI_VIEW_RENDERMODEL,         "View_RenderModel", "model");
  SETTIMERNAME(PTI_VIEW_INIT_DIFF_SURF,      "  View_Init_Diff_surf", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_BUMP_SURF,      "  View_Init_Bump_surf", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_REFL_MIP ,      "  View_Init_Refl_mip ", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_REFL_SURF,      "  View_Init_Refl_surf", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_SPEC_MIP,       "  View_Init_Spec_mip ", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_SPEC_SURF,      "  View_Init_Spec_surf", "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_FOG_SURF,       "  View_Init_Fog_surf",  "vertex");
  SETTIMERNAME(PTI_VIEW_INIT_HAZE_SURF,      "  View_Init_Haze_surf", "vertex");
  SETTIMERNAME(PTI_VIEW_ATTENUATE_SURF,      "  View_Attenuate_surf", "vertex");
                                                 
  SETTIMERNAME(PTI_VIEW_RENDER_DIFFUSE,      "  View_Render_Diffuse", "model");
  SETTIMERNAME(PTI_VIEW_RENDER_BUMP,         "  View_Render_Bump", "model");
  SETTIMERNAME(PTI_VIEW_RENDER_REFLECTIONS,  "  View_Render_Reflections", "model");
  SETTIMERNAME(PTI_VIEW_RENDER_SPECULAR,     "  View_Render_Specular", "model");
  SETTIMERNAME(PTI_VIEW_RENDER_FOG,          "  View_Render_Fog", "model");
  SETTIMERNAME(PTI_VIEW_RENDER_HAZE,         "  View_Render_Haze", "model");

  SETTIMERNAME(PTI_VIEW_SETTEXTURE,          "View_Settexture", "setting");
  SETTIMERNAME(PTI_VIEW_LOCKARRAYS,          "View_Lockarrays", "vertex");
  SETTIMERNAME(PTI_VIEW_DRAWELEMENTS,        "View_Drawelements", "triangle");
  SETTIMERNAME(PTI_VIEW_ONESIDE,             "View_oneside", "side");
  SETTIMERNAME(PTI_VIEW_ONESIDE_GLSETUP,     "  View_oneside_glSetup", "setup");

  SETTIMERNAME(PTI_VIEW_RENDERPATCHES,      "View_Renderpatches", "model");
  SETTIMERNAME(PTI_VIEW_RENDERSHADOW,       "View_Rendershadow", "shadow");
  SETTIMERNAME(PTI_VIEW_SHAD_INIT_MIP,      "  View_Shad_Init_mip", "vertex");
  SETTIMERNAME(PTI_VIEW_SHAD_INIT_SURF,     "  View_Shad_Init_surf", "vertex");
  SETTIMERNAME(PTI_VIEW_SHAD_GLSETUP  ,     "  View_Shad_glSetup", "shadow"); 
  SETTIMERNAME(PTI_VIEW_SHAD_RENDER   ,     "  View_Shad_Render", "triangle");
  SETTIMERNAME(PTI_VIEW_RENDERSIMPLESHADOW, "View_RenderSimpleShadow", "shadow");
  SETTIMERNAME(PTI_VIEW_SIMP_CALC,          "  View_Simp_calc", "shadow");
  SETTIMERNAME(PTI_VIEW_SIMP_PREP,          "  View_Simp_prep", "shadow");
  SETTIMERNAME(PTI_VIEW_SIMP_COPY   ,       "  View_Simp_copy", "shadow");
  SETTIMERNAME(PTI_VIEW_SIMP_BATCHED,       "  View_Simp_batched", "batch");

  SETTIMERNAME(PTI_MASK_INITMODELRENDERING, "Mask_InitModelRendering", "model");
  SETTIMERNAME(PTI_MASK_RENDERMODEL,        "Mask_RenderModel", "model");

  // initialize counters
  SETCOUNTERNAME(PCI_VERTICES_FIRSTMIP, "Vertices_firstmip");
  SETCOUNTERNAME(PCI_VERTICES_USEDMIP,  "Vertices_usedmip");
  SETCOUNTERNAME(PCI_SURFACEVERTICES_FIRSTMIP, "SurfaceVertices_firstmip");
  SETCOUNTERNAME(PCI_SURFACEVERTICES_USEDMIP,  "SurfaceVertices_usedmip");
  SETCOUNTERNAME(PCI_TRIANGLES_FIRSTMIP, "Triangles_firstmip");
  SETCOUNTERNAME(PCI_TRIANGLES_USEDMIP,  "Triangles_usedmip");
  SETCOUNTERNAME(PCI_SHADOWVERTICES_FIRSTMIP, "ShadowVertices_firstmip");
  SETCOUNTERNAME(PCI_SHADOWVERTICES_USEDMIP,  "ShadowvVrtices_usedmip");
  SETCOUNTERNAME(PCI_SHADOWSURFACEVERTICES_FIRSTMIP, "ShadowSurfaceVertices_firstmip");
  SETCOUNTERNAME(PCI_SHADOWSURFACEVERTICES_USEDMIP,  "ShadowSurfaceVertices_usedmip");
  SETCOUNTERNAME(PCI_SHADOWTRIANGLES_FIRSTMIP, "ShadowTriangles_firstmip");
  SETCOUNTERNAME(PCI_SHADOWTRIANGLES_USEDMIP,  "ShadowTriangles_usedmip");

  SETCOUNTERNAME(PCI_VIEW_TRIANGLES, "View_Triangles");

  SETCOUNTERNAME(PCI_MASK_TRIANGLES, "Mask_Triangles");
  SETCOUNTERNAME(PCI_MASK_POLYGONS,  "Mask_Polygons");
};
