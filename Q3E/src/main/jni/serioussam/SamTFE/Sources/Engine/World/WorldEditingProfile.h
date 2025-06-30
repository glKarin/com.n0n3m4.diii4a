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

#ifndef SE_INCL_WORLDEDITINGPROFILE_H
#define SE_INCL_WORLDEDITINGPROFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Profiling.h>

/* Class for holding profiling information for world editing operations. */
class CWorldEditingProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex {
    PTI_CSGTOTAL,               // total time spent for CSG operations
    PTI_OBJECTOPTIMIZE,         // time spent in CObject3D::Optimize()
    PTI_OBJECTCSG,              // time spent in CObject3D::CSGxxxx()
    PTI_ADDOBJECTTOBRUSH,       // AddObjectToBrush()
    PTI_FINDSHADOWLAYERS,       // FindShadowLayers()
    PTI_CALCULATEBOUNDINGBOXES, // CalculateBoundingBoxes()
    PTI_ADDFROMOBJECT3D,        // AddFromObject3D()
    PTI_TRIANGULATE,
    PTI_TRISTRIPMODELS,

    PTI_DUMMY1,

    PTI_LINKENTITIESTOSECTORS,  // LinkEntititesToSectors()
    PTI_LINKPORTALSANDSECTORS,  // LinkPortalsAndSectors()
    PTI_READBRUSHES,            // ReadBrushes()
    PTI_READBSP,                // ReadBSP()
    PTI_READPORTALSECTORLINKS,  // ReadPortalSectorLinks()
    PTI_READSTATE,              // ReadState()
    PTI_REINITIALIZEENTITIES,

    PTI_DUMMY2,

    PTI_MAKESHADOWMAP,          // MakeShadowMap()
    PTI_RENDERSHADOWS,          // RenderShadows()

    PTI_DUMMY3,

    PTI_MIXLAYERS,              // CLayerMixer::MixLayers()
    PTI_CALCULATEDATA,          // CLayerMixer::CalculateData()
    PTI_AMBIENTFILL,            // CLayerMixer::FillWithAmbientLight()
    PTI_ADDONELAYERPOINT,       // CLayerMixer::AddOneLayerPoint()
    PTI_ADDONELAYERDIRECTIONAL, // CLayerMixer::AddOneLayerDirectional()

    PTI_COUNT
  };
  enum ProfileCounterIndex {
    PCI_SECTORSOPTIMIZED,       // total number of sectors optimized
    PCI_SHADOWIMAGES,           // number of shadow images generated
    PCI_SHADOWCLUSTERS,         // total number of shadow clusters generated in all images
    PCI_POLYGONSHADOWS,         // total number of polygon shadows cast
    PCI_COUNT
  };
  // constructor
  CWorldEditingProfile(void);
};


#endif  /* include-once check. */

