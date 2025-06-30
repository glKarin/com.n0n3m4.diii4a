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

#include <Engine/Math/Object3D.h>

#include <Engine/Base/Shell.h>
#include <Engine/Math/Float.h>
#include <Engine/Math/Projection_DOUBLE.h>

#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>

inline void Clear(CObjectEdge *poed) {};

/*
 * Default constructor.
 */
CObject3D::CObject3D() {
};

/*
 * Destructor.
 */
CObject3D::~CObject3D() {
  Clear();
};

void CObject3D::Clear(void)
{
  ob_aoscSectors.Clear();           // clear sectors array
}
/*
 * Create indices for all sectors.
 */
void CObject3D::CreateSectorIndices(void)
{
  ob_aoscSectors.Lock();

  // get the number of sectors in object
  INDEX ctSectors = ob_aoscSectors.Count();
  // set sectors indices
  for(INDEX iSector=0; iSector<ctSectors; iSector++) {
    ob_aoscSectors[iSector].osc_Index = iSector;
  }

  ob_aoscSectors.Unlock();
}

BOOL CObject3D::ArePolygonsPlanar(void)
{
  // for all sectors
  FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itsc)
  {
    if( !itsc->ArePolygonsPlanar()) return FALSE;
  }
  return TRUE;
}

/*
 * Project the whole object into some other space.
 */
void CObject3D::Project(CSimpleProjection3D_DOUBLE &pr)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // check if projection is mirrored
  const FLOAT3D &vObjectStretch = pr.ObjectStretchR();
  BOOL bXInverted = vObjectStretch(1)<0;
  BOOL bYInverted = vObjectStretch(2)<0;
  BOOL bZInverted = vObjectStretch(3)<0;
  BOOL bInverted = (bXInverted != bYInverted) != bZInverted;

  // for all sectors
  FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itsc) {

    // for all vertices in sector
    FOREACHINDYNAMICARRAY(itsc->osc_aovxVertices, CObjectVertex, itvx) {
      // project the vertex
      pr.ProjectCoordinate(*itvx, *itvx);
    }

    /* NOTE: We must project polygons _before_ planes, since projecting
       of texture mapping coefficients requires unprojected plane! */
    // for all polygons in sector
    FOREACHINDYNAMICARRAY(itsc->osc_aopoPolygons, CObjectPolygon, itpo) {
      // project mapping
      pr.ProjectMapping(itpo->opo_amdMappings[0], *itpo->opo_Plane, itpo->opo_amdMappings[0]);
      pr.ProjectMapping(itpo->opo_amdMappings[1], *itpo->opo_Plane, itpo->opo_amdMappings[1]);
      pr.ProjectMapping(itpo->opo_amdMappings[2], *itpo->opo_Plane, itpo->opo_amdMappings[2]);
      pr.ProjectMapping(itpo->opo_amdMappings[3], *itpo->opo_Plane, itpo->opo_amdMappings[3]);
      // if projection is inverted
      if (bInverted) {
        // invert all polygon edges
        {FOREACHINDYNAMICARRAY(itpo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
          CObjectPolygonEdge &ope = *itope;
          ope.ope_Backward = !ope.ope_Backward;
        }}
      }
    }
    // for all planes in sector
    FOREACHINDYNAMICARRAY(itsc->osc_aoplPlanes, CObjectPlane, itpl) {
      // project the plane
      pr.Project(*itpl, *itpl);
    }
  }
}

/*
 * Assignment operator.
 */
CObject3D &CObject3D::operator=(CObject3D &obOriginal)
{
  // copy array of sectors from original object, sectors will copy their contents
  ob_aoscSectors = obOriginal.ob_aoscSectors;

  return *this;
}

/*
 * Create BSP trees for all sectors.
 */
void CObject3D::CreateSectorBSPs(void)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // for each sector in object
  FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // create its BSP tree
    itosc->CreateBSP();
  }
}

/*
 * Remove sectors with no polygons.
 */
void CObject3D::RemoveEmptySectors(void)
{
  // create a container for empty sectors
  CDynamicContainer<CObjectSector> coscEmpty;

  // for all sectors in object
  {FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // if it has no polygons
    if (itosc->osc_aopoPolygons.Count() == 0) {
      // add the sector to the container of empty sectors
      coscEmpty.Add(&itosc.Current());
    }
  }}

  // for all empty sectors
  {FOREACHINDYNAMICCONTAINER(coscEmpty, CObjectSector, itoscEmpty) {
    // delete the sector from object
    ob_aoscSectors.Delete(&itoscEmpty.Current());
  }}
}

/*
 * Remove unused and replicated elements.
 */
void CObject3D::Optimize(void)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);

  // for all sectors in the object
  {FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // optimize the sector
    itosc->Optimize();
  }}

  // remove sectors that have no polygons
  RemoveEmptySectors();
}

/*
 * Turn all sectors in object inside-out. (not recommended for multi sector objects)
 */
void CObject3D::Inverse(void)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // for all sectors in object
  {FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // inverse the sector
    itosc->Inverse();
  }}
}

/* Recalculate all planes from vertices. (used when stretching vertices) */
void CObject3D::RecalculatePlanes(void)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // for all sectors in object
  {FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // recalculate all planes in sector
    itosc->RecalculatePlanes();
  }}
}

/*
 * Turn all portals to walls.
 */
void CObject3D::TurnPortalsToWalls(void)
{
  // for all sectors in object
  {FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // for all polygons
    {FOREACHINDYNAMICARRAY(itosc->osc_aopoPolygons, CObjectPolygon, itopo) {
      // clear the portal flag
      itopo->opo_ulFlags &= ~OPOF_PORTAL;
    }}
  }}
}

/*
 * Find bounding box of the object.
 */
void CObject3D::GetBoundingBox(DOUBLEaabbox3D &boxObject)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // clear the bounding box
  boxObject = DOUBLEaabbox3D();
  // for each sector in the object
  FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {
    // get box of the sector
    DOUBLEaabbox3D boxSector;
    itosc->GetBoundingBox(boxSector);
    // add the sector box to the bounding box
    boxObject |= boxSector;
  }
}

/* Dump the object 3D to debug window. */
void CObject3D::DebugDump(void)
{
#ifndef NDEBUG

  _RPT0(_CRT_WARN, "Object3D dump BEGIN:\n");

  _RPT1(_CRT_WARN, "Sectors: %d\n", ob_aoscSectors.Count());
  {FOREACHINDYNAMICARRAY(ob_aoscSectors, CObjectSector, itosc) {

    _RPT1(_CRT_WARN, "SC%d:\n", ob_aoscSectors.Index(&itosc.Current()));

    _RPT1(_CRT_WARN, "Vertices: %d\n", itosc->osc_aovxVertices.Count());
    {FOREACHINDYNAMICARRAY(itosc->osc_aovxVertices, CObjectVertex, itovx) {
      _RPT4(_CRT_WARN, "VX%d: (%f, %f, %f)\n", itosc->osc_aovxVertices.Index(itovx),
          (*itovx)(1), (*itovx)(2), (*itovx)(3));
    }}

    _RPT1(_CRT_WARN, "Planes: %d\n", itosc->osc_aoplPlanes.Count());
    {FOREACHINDYNAMICARRAY(itosc->osc_aoplPlanes, CObjectPlane, itopl) {
      _RPT4(_CRT_WARN, "PL%d: (%g, %g, %g)", itosc->osc_aoplPlanes.Index(&itopl.Current()),
          itopl.Current()(1), itopl.Current()(2), itopl.Current()(3));
      _RPT1(_CRT_WARN, ":%g\n", itopl->Distance());
    }}

    _RPT1(_CRT_WARN, "Edges: %d\n", itosc->osc_aoedEdges.Count());
    itosc->osc_aovxVertices.Lock();
    {FOREACHINDYNAMICARRAY(itosc->osc_aoedEdges, CObjectEdge, itoed) {
      _RPT3(_CRT_WARN, "ED%d: VX%d -> VX%d\n", itosc->osc_aoedEdges.Index(&itoed.Current()),
        itosc->osc_aovxVertices.Index(itoed->oed_Vertex0),
        itosc->osc_aovxVertices.Index(itoed->oed_Vertex1));
    }}
    itosc->osc_aovxVertices.Unlock();


    _RPT1(_CRT_WARN, "Polygons: %d\n", itosc->osc_aopoPolygons.Count());
    itosc->osc_aovxVertices.Lock();
    itosc->osc_aoedEdges.Lock();
    itosc->osc_aoplPlanes.Lock();
    {FOREACHINDYNAMICARRAY(itosc->osc_aopoPolygons, CObjectPolygon, itopo) {
      _RPT3(_CRT_WARN, "PO%d (PL%d): Edges: %d\n  ",
        itosc->osc_aopoPolygons.Index(&itopo.Current()),
        itosc->osc_aoplPlanes.Index(itopo->opo_Plane),
        itopo->opo_PolygonEdges.Count());
      {FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
        _RPT1(_CRT_WARN, "ED%d", itosc->osc_aoedEdges.Index(itope->ope_Edge));
        CObjectVertex *povx0, *povx1;
        if (itope->ope_Backward) {
          povx0 = itope->ope_Edge->oed_Vertex1;
          povx1 = itope->ope_Edge->oed_Vertex0;
        } else {
          povx0 = itope->ope_Edge->oed_Vertex0;
          povx1 = itope->ope_Edge->oed_Vertex1;
        }
        _RPT4(_CRT_WARN, " (VX%d (%f,%f,%f)",
          itosc->osc_aovxVertices.Index(povx0), (*povx0)(1), (*povx0)(2), (*povx0)(3));
        _RPT4(_CRT_WARN, "->VX%d (%f,%f,%f)) ",
          itosc->osc_aovxVertices.Index(povx1), (*povx1)(1), (*povx1)(2), (*povx1)(3));
      }}
      _RPT0(_CRT_WARN, "\n");
    }}
    itosc->osc_aoplPlanes.Unlock();
    itosc->osc_aoedEdges.Unlock();
    itosc->osc_aovxVertices.Unlock();
  }}

  _RPT0(_CRT_WARN, "Object3D dump END:\n");

#endif // NDEBUG
}

