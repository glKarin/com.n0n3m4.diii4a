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

#ifndef __ENGINE_BASE_PROFILING_H__
#include <Engine/Base/Profiling.h>
#endif

class CRenderProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex {
    PTI_RENDERING,          // overall rendering time for CRenderer
      PTI_INITIALIZATION,     // time spent in initialization
      PTI_ADDINITIAL,         // time spent adding initial sectors
      PTI_CLEANUP,            // time spent in destructor
      PTI_RENDERSCENE,        // time spent in RenderScene()
      PTI_RENDERMODELS,       // time spent in RenderModels()
        PTI_RENDERONEMODEL,   // time spent in RenderOneModel()
          PTI_FINDSHADINGINFO,   // time spent in FindShadingInfo() during RenderOneModel()
          PTI_FINDLIGHTS,        // time spent in searching for lights in RenderOneModel()
      PTI_RENDERPARTICLES,    // time spent in RenderParticles()

      PTI_SCANEDGES,          // time spent in ScanEdges()
        PTI_INITSCANEDGES,      // time spent in InitScanEdges()
        PTI_ENDSCANEDGES,       // time spent in EndScanEdges()
        PTI_SCANONELINE,        // time spent in ScanOneLine()
        PTI_PASSPORTAL,         // time spent in PassPortal()
        PTI_ADDSPANSTOSCENE,    // time spent in AddSpansToScene()
        PTI_PROCESSTRANSPORTAL, // time spent processing translucent portals
        PTI_ADDTRANSSPANSTOSCENE,// time spent in AddTranslucentSpansToScene()
        PTI_STEPANDRESORT,      // time spent in StepAndResortActiveList()
        PTI_REMREMLIST,         // time spent in RemRemoveListFromActiveList()
        PTI_ADDADDLIST,         // time spent in AddAddListToActiveList()
      PTI_ADDNONZONINGBRUSH,  // time spent in AddNonZoningBrush()
      PTI_ADDMODELENTITY,       // time spent in AddModelEntity()
      PTI_ADDZONINGSECTORS,   // time spent in AddZoningSectors()
      PTI_ADDENTITIESINSECTOR,   // time spent in AddEntitiesInSector()
      PTI_ADDENTITIESINBOX,      // time spent in AddEntitiesInBox()
      PTI_PREPAREBRUSH,          // time spent in PrepareBrush()
      PTI_ADDSECTOR,          // time spent in AddSector()
        PTI_TRANSFORMVERTICES,  // time spent transforming sector vertices
        PTI_TRANSFORMPLANES,    // time spent transforming sector planes
        PTI_MAKENONDETAILSCREENPOLYGONS,
        PTI_CLIPTOALLPLANES,
        PTI_PROJECTVERTICES,
        PTI_MAKEFINALPOLYGONEDGES,
        PTI_ADDSCREENEDGES,
        PTI_MAKEDETAILSCREENPOLYGONS,
        PTI_MAKESCREENEDGE,   // time spent in MakeScreenEdge()
        PTI_ADDEDGETOADDLIST, // time spent in AddEdgeToAddList()
        PTI_MAKESCREENPOLYGON,  // time spent in MakeScreenPolygon()
    PTI_REDRAWVIEW,           // time spent in CSCapeLibrary::RedrawView()
      PTI_RENDERINTERFACE,    // time spent in CPlayerEntity::RenderInterface()
    PTI_COUNT
  };
  enum ProfileCounterIndex {
    PCI_POLYGONSRENDERED,       // total number of polygons rendered

    PCI_TRANSFORMEDSECTORS,     // total number of transformed sectors
    PCI_TRANSFORMEDVERTICES,    // total number of transformed vertices
    PCI_TRANSFORMEDPLANES,      // total number of transformed planes
    PCI_TRANSFORMEDEDGES,       // total number of transformed edges
    PCI_NONDETAILPOLYGONS,
    PCI_DETAILPOLYGONS,   

    PCI_EDGETRANSITIONS,        // total number of edge transitions during scanning
    PCI_SWAPEDGETRANSITIONS,    // number of edge transitions with swap polygons
    PCI_OVERALLSCANLINES,       // total number of scan lines processed
    PCI_SCANLINEPORTALRETRIES,  // scan line retries due to portal encounters
    PCI_COHERENTSCANLINES,      // scan lines that were coherent with previous one
    PCI_SPANS,                  // total generated spans
    PCI_TRAPEZOIDS,             // total generated trapezoids
    PCI_COUNT
  };

  CRenderProfile(void);
};
