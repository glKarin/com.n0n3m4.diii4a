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

#include <Engine/Rendering/RenderProfile.h>

// profile form for profiling world editing
CRenderProfile rpRenderProfile;
CProfileForm &_pfRenderProfile = rpRenderProfile;

// profile form for profiling world rendering
CRenderProfile::CRenderProfile(void) :
  CProfileForm ("Rendering", "frames", CRenderProfile::PCI_COUNT, CRenderProfile::PTI_COUNT)
{
  // initialize rendering profile form
  SETTIMERNAME(CRenderProfile::PTI_RENDERING,              "rendering", "");
  SETTIMERNAME(CRenderProfile::PTI_INITIALIZATION,         " initialization", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDINITIAL,             " adding initial sectors", "");
  SETTIMERNAME(CRenderProfile::PTI_CLEANUP,                " clean-up", "");
  SETTIMERNAME(CRenderProfile::PTI_RENDERSCENE,            " RenderScene()", "");
  SETTIMERNAME(CRenderProfile::PTI_RENDERMODELS,           " RenderModels()", "");
  SETTIMERNAME(CRenderProfile::PTI_RENDERONEMODEL,         "  RenderOneModel()", "");
  SETTIMERNAME(CRenderProfile::PTI_FINDSHADINGINFO,        "   FindShadingInfo() during RenderOneModel()", "finding");
  SETTIMERNAME(CRenderProfile::PTI_FINDLIGHTS,             "   searching for lights in RenderOneModel()", "");
  SETTIMERNAME(CRenderProfile::PTI_RENDERPARTICLES,        " RenderParticles()", "");

  SETTIMERNAME(CRenderProfile::PTI_SCANEDGES,              " ScanEdges()", "");
  SETTIMERNAME(CRenderProfile::PTI_INITSCANEDGES,          "  InitScanEdges()", "");
  SETTIMERNAME(CRenderProfile::PTI_ENDSCANEDGES,           "  EndScanEdges()", "");
  SETTIMERNAME(CRenderProfile::PTI_SCANONELINE,            "  ScanOneLine()", "");
  SETTIMERNAME(CRenderProfile::PTI_PASSPORTAL,             "  PassPortal()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDSPANSTOSCENE,        "  AddSpansToScene()", "");
  SETTIMERNAME(CRenderProfile::PTI_PROCESSTRANSPORTAL,     "  processing translucent portals", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDTRANSSPANSTOSCENE,   "  AddTranslucentSpansToScene()", "");
  SETTIMERNAME(CRenderProfile::PTI_STEPANDRESORT,          "  StepAndResortActiveList()", "");
  SETTIMERNAME(CRenderProfile::PTI_REMREMLIST,             "  RemRemoveListFromActiveList()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDADDLIST,             "  AddAddListToActiveList()", "");

  SETTIMERNAME(CRenderProfile::PTI_ADDNONZONINGBRUSH,      " AddNonZoningBrush()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDMODELENTITY,         " AddModelEntity()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDZONINGSECTORS,       " AddZoningSectors()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDENTITIESINSECTOR,    " AddEntitiesInSector()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDENTITIESINBOX,       " AddEntitiesInBox()", "");

  SETTIMERNAME(CRenderProfile::PTI_PREPAREBRUSH,           " PrepareBrush()", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDSECTOR,              " AddSector()", "");
  SETTIMERNAME(CRenderProfile::PTI_TRANSFORMVERTICES,           "  transforming vertices", "vertex");
  SETTIMERNAME(CRenderProfile::PTI_TRANSFORMPLANES,             "  transforming planes", "plane");
  SETTIMERNAME(CRenderProfile::PTI_MAKENONDETAILSCREENPOLYGONS, "  makenondetailscreenpolygons", "");
  SETTIMERNAME(CRenderProfile::PTI_CLIPTOALLPLANES,             "  cliptoallplanes", "");
  SETTIMERNAME(CRenderProfile::PTI_PROJECTVERTICES,             "  projectvertices", "");
  SETTIMERNAME(CRenderProfile::PTI_MAKEFINALPOLYGONEDGES,       "  makefinalpolygonedges", "");
  SETTIMERNAME(CRenderProfile::PTI_ADDSCREENEDGES,              "  addscreenedges", "");
  SETTIMERNAME(CRenderProfile::PTI_MAKEDETAILSCREENPOLYGONS,    "  makedetailscreenpolygons", "");
  SETTIMERNAME(CRenderProfile::PTI_MAKESCREENEDGE,         "    MakeScreenEdge()", "edge");
  SETTIMERNAME(CRenderProfile::PTI_ADDEDGETOADDLIST,       "  AddEdgeToAddList()", "adding");
  SETTIMERNAME(CRenderProfile::PTI_MAKESCREENPOLYGON,      "  MakeScreenPolygon()", "polygon");
  SETTIMERNAME(CRenderProfile::PTI_REDRAWVIEW,             "CSCapeLibrary::RedrawView()", "");
  SETTIMERNAME(CRenderProfile::PTI_RENDERINTERFACE,        " CPlayerEntity::RenderInterface()", "");

  SETCOUNTERNAME(CRenderProfile::PCI_TRANSFORMEDSECTORS,  "transformed sectors");
  SETCOUNTERNAME(CRenderProfile::PCI_TRANSFORMEDVERTICES, "transformed vertices");
  SETCOUNTERNAME(CRenderProfile::PCI_TRANSFORMEDPLANES,   "transformed planes");
  SETCOUNTERNAME(CRenderProfile::PCI_TRANSFORMEDEDGES,    "transformed edges");
  SETCOUNTERNAME(CRenderProfile::PCI_NONDETAILPOLYGONS,   "nondetail polygons");
  SETCOUNTERNAME(CRenderProfile::PCI_DETAILPOLYGONS,   "detail polygons");

  SETCOUNTERNAME(CRenderProfile::PCI_EDGETRANSITIONS, "edge transitions");
  SETCOUNTERNAME(CRenderProfile::PCI_SWAPEDGETRANSITIONS, "edge transitions with swap");
  SETCOUNTERNAME(CRenderProfile::PCI_OVERALLSCANLINES, "total scan lines");
  SETCOUNTERNAME(CRenderProfile::PCI_SCANLINEPORTALRETRIES, "portal scan line retries");
  SETCOUNTERNAME(CRenderProfile::PCI_COHERENTSCANLINES, "coherent scan lines");
  SETCOUNTERNAME(CRenderProfile::PCI_SPANS, "total generated spans");
  SETCOUNTERNAME(CRenderProfile::PCI_TRAPEZOIDS, "total generated trapezoids");
}
