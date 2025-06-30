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
#include <Engine/Templates/BSP_internal.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>

// define this for full debugging of all algorithms
#define FULL_DEBUG

// epsilon value used for vertex comparison
//#define VTX_EPSILON (0.015625f)       // 1/2^6 ~= 1.5 cm
#define VTX_EPSILON DOUBLE((1.0/65536.0)/16.0*mth_fCSGEpsilon) // 1/2^16
//#define VTX_EPSILON DOUBLE(0.00390625) // 1/2^8
// grid for snapping vertices
#define VTX_SNAP (VTX_EPSILON/2.0)

// epsilon value used for plane comparison
#define PLX_EPSILON (VTX_EPSILON/16.0*mth_fCSGEpsilon)
// grid for snapping planes
#define PLX_SNAP (PLX_EPSILON/2.0)

// epsilon value used for edge line comparison
//#define EDX_EPSILON (1E-3f)       // 1E-3 == 1 mm
//#define EDX_EPSILON (0.015625f)   // 1/2^6 ~= 1.5 cm
//#define EDX_EPSILON DOUBLE(1.0/65536.0) // 1/2^16
#define EDX_EPSILON DOUBLE(0.00390625*mth_fCSGEpsilon) // 1/2^8

// use O(nlogn) instead O(n2) algorithms for object optimization
#ifdef PLATFORM_UNIX
INDEX wld_bFastObjectOptimization = 1;
FLOAT mth_fCSGEpsilon = 1.0f;
#else
extern INDEX wld_bFastObjectOptimization = 1.0;
extern FLOAT mth_fCSGEpsilon = 1.0f;
#endif

/*
 * Compare two vertices.
 */
inline int CompareVertices(const CObjectVertex &vx0, const CObjectVertex &vx1)
{
       if (vx0(1)-vx1(1) < -VTX_EPSILON) return -1;
  else if (vx0(1)-vx1(1) > +VTX_EPSILON) return 1;
  else if (vx0(2)-vx1(2) < -VTX_EPSILON) return -1;
  else if (vx0(2)-vx1(2) > +VTX_EPSILON) return 1;
  else if (vx0(3)-vx1(3) < -VTX_EPSILON) return -1;
  else if (vx0(3)-vx1(3) > +VTX_EPSILON) return 1;
  else                    return 0;
}

/*
 * Compare two vertices for quick-sort.
 */
static int qsort_CompareVertices(const void *pvVertex0, const void *pvVertex1)
{
  CObjectVertex &vx0 = **(CObjectVertex **)pvVertex0;
  CObjectVertex &vx1 = **(CObjectVertex **)pvVertex1;
  return CompareVertices(vx0, vx1);
}

/*
 * Compare two along line vertices.
 */
static INDEX sortvertices_iMaxAxis;
inline int CompareVerticesAlongLine(const CObjectVertex &vx0, const CObjectVertex &vx1)
{
       if (vx0(sortvertices_iMaxAxis)-vx1(sortvertices_iMaxAxis) < -VTX_EPSILON) return -1;
  else if (vx0(sortvertices_iMaxAxis)-vx1(sortvertices_iMaxAxis) > +VTX_EPSILON) return +1;
  else                                                                           return  0;
}
/*
 * Compare two vertices along a line for quick-sort.
 */
static int qsort_CompareVerticesAlongLine(const void *pvVertex0, const void *pvVertex1)
{
  CObjectVertex &vx0 = **(CObjectVertex **)pvVertex0;
  CObjectVertex &vx1 = **(CObjectVertex **)pvVertex1;
  return CompareVerticesAlongLine(vx0, vx1);
}
#if 0 // DG: unused.
/*
 * Compare two vertices along a line for quick-sort - reversely.
 */
static int qsort_CompareVerticesAlongLineReversely(const void *pvVertex0, const void *pvVertex1)
{
  CObjectVertex &vx0 = **(CObjectVertex **)pvVertex0;
  CObjectVertex &vx1 = **(CObjectVertex **)pvVertex1;
  return -CompareVerticesAlongLine(vx0, vx1);
}
#endif // 0 (unused)


/*
 * Compare two planes.
 */
inline int ComparePlanes(const CObjectPlane &pl0, const CObjectPlane &pl1)
{
       if (pl0(1)-pl1(1)                 < -PLX_EPSILON) return -1;
  else if (pl0(1)-pl1(1)                 > +PLX_EPSILON) return +1;
  else if (pl0(2)-pl1(2)                 < -PLX_EPSILON) return -1;
  else if (pl0(2)-pl1(2)                 > +PLX_EPSILON) return +1;
  else if (pl0(3)-pl1(3)                 < -PLX_EPSILON) return -1;
  else if (pl0(3)-pl1(3)                 > +PLX_EPSILON) return +1;
  else if (pl0.Distance()-pl1.Distance() < -PLX_EPSILON) return -1;
  else if (pl0.Distance()-pl1.Distance() > +PLX_EPSILON) return +1;
  else                                                   return 0;
}

/*
 * Compare two planes for quick-sort.
 */
static int qsort_ComparePlanes(const void *pvPlane0, const void *pvPlane1)
{
  CObjectPlane &pl0 = **(CObjectPlane **)pvPlane0;
  CObjectPlane &pl1 = **(CObjectPlane **)pvPlane1;
  return ComparePlanes(pl0, pl1);
}

/*
 * Compare two edges.
 */
inline int CompareEdges(const CObjectEdge &ed0, const CObjectEdge &ed1)
{
       if (ed0.oed_Vertex0<ed1.oed_Vertex0) return -1;
  else if (ed0.oed_Vertex0>ed1.oed_Vertex0) return +1;
  else if (ed0.oed_Vertex1<ed1.oed_Vertex1) return -1;
  else if (ed0.oed_Vertex1>ed1.oed_Vertex1) return +1;
  else                                      return 0;
}

/*
 * Compare two edges for quick-sort.
 */
static int qsort_CompareEdges(const void *pvEdge0, const void *pvEdge1)
{
  CObjectEdge &ed0 = **(CObjectEdge **)pvEdge0;
  CObjectEdge &ed1 = **(CObjectEdge **)pvEdge1;
  return CompareEdges(ed0, ed1);
}

/*
 * Compare two edge lines.
 */
inline int CompareEdgeLines(const CEdgeEx &edx0, const CEdgeEx &edx1)
{
       if (edx0.edx_vDirection(1)-edx1.edx_vDirection(1) < -EDX_EPSILON) return -1;
  else if (edx0.edx_vDirection(1)-edx1.edx_vDirection(1) > +EDX_EPSILON) return +1;
  else if (edx0.edx_vDirection(2)-edx1.edx_vDirection(2) < -EDX_EPSILON) return -1;
  else if (edx0.edx_vDirection(2)-edx1.edx_vDirection(2) > +EDX_EPSILON) return +1;
  else if (edx0.edx_vDirection(3)-edx1.edx_vDirection(3) < -EDX_EPSILON) return -1;
  else if (edx0.edx_vDirection(3)-edx1.edx_vDirection(3) > +EDX_EPSILON) return +1;
  else if (edx0.edx_vReferencePoint(1)-edx1.edx_vReferencePoint(1) < -EDX_EPSILON) return -1;
  else if (edx0.edx_vReferencePoint(1)-edx1.edx_vReferencePoint(1) > +EDX_EPSILON) return +1;
  else if (edx0.edx_vReferencePoint(2)-edx1.edx_vReferencePoint(2) < -EDX_EPSILON) return -1;
  else if (edx0.edx_vReferencePoint(2)-edx1.edx_vReferencePoint(2) > +EDX_EPSILON) return +1;
  else if (edx0.edx_vReferencePoint(3)-edx1.edx_vReferencePoint(3) < -EDX_EPSILON) return -1;
  else if (edx0.edx_vReferencePoint(3)-edx1.edx_vReferencePoint(3) > +EDX_EPSILON) return +1;
  else                                     return  0;
}

//#define EDX_EPSILON_LOOSE DOUBLE(0.00390625) // 1/2^8
/*
 * Compare two edge lines.
 */
/*
inline int CompareEdgeLines_loosely(const CEdgeEx &edx0, const CEdgeEx &edx1)
{
       if (edx0.edx_vDirection(1)-edx1.edx_vDirection(1) < -EDX_EPSILON_LOOSE) return -1;
  else if (edx0.edx_vDirection(1)-edx1.edx_vDirection(1) > +EDX_EPSILON_LOOSE) return +1;
  else if (edx0.edx_vDirection(2)-edx1.edx_vDirection(2) < -EDX_EPSILON_LOOSE) return -1;
  else if (edx0.edx_vDirection(2)-edx1.edx_vDirection(2) > +EDX_EPSILON_LOOSE) return +1;
  else if (edx0.edx_vDirection(3)-edx1.edx_vDirection(3) < -EDX_EPSILON_LOOSE) return -1;
  else if (edx0.edx_vDirection(3)-edx1.edx_vDirection(3) > +EDX_EPSILON_LOOSE) return +1;
  else if (edx0.edx_vReferencePoint(1)-edx1.edx_vReferencePoint(1) < -EDX_EPSILON_LOOSE) return -1;
  else if (edx0.edx_vReferencePoint(1)-edx1.edx_vReferencePoint(1) > +EDX_EPSILON_LOOSE) return +1;
  else if (edx0.edx_vReferencePoint(2)-edx1.edx_vReferencePoint(2) < -EDX_EPSILON_LOOSE) return -1;
  else if (edx0.edx_vReferencePoint(2)-edx1.edx_vReferencePoint(2) > +EDX_EPSILON_LOOSE) return +1;
  else if (edx0.edx_vReferencePoint(3)-edx1.edx_vReferencePoint(3) < -EDX_EPSILON_LOOSE) return -1;
  else if (edx0.edx_vReferencePoint(3)-edx1.edx_vReferencePoint(3) > +EDX_EPSILON_LOOSE) return +1;
  else                                     return  0;
}*/

/*
 * Compare two edge lines for quick-sort.
 */
static int qsort_CompareEdgeLines(const void *pvEdgeEx0, const void *pvEdgeEx1)
{
  CEdgeEx &edx0 = **(CEdgeEx **)pvEdgeEx0;
  CEdgeEx &edx1 = **(CEdgeEx **)pvEdgeEx1;
  return CompareEdgeLines(edx0, edx1);
}

/*
 * Find an edge with given vertices in sorted array of pointers to edges.
 */
BOOL FindEdge( CStaticArray<CObjectEdge *> &apedSorted,
               CObjectVertex &vx0, CObjectVertex &vx1,
               CObjectEdge **ppedResult)
{
  // create a ghost edge with given vertices
  CObjectEdge edWanted(vx0, vx1);
  CObjectEdge *pedWanted = &edWanted;

  // invoke binary search on the array with ghost edge
  CObjectEdge **ppedFound = (CObjectEdge **) bsearch(
    &pedWanted, &(apedSorted[0]), apedSorted.Count(), sizeof(CObjectEdge *),
    qsort_CompareEdges);

  // if some edge was found
  if (ppedFound!=NULL) {
    // return it
    *ppedResult= *ppedFound;
    return TRUE;
  // otherwise
  } else {
    // there is no such edge
    return FALSE;
  }
}

/*
 * Get start and end vertices.
 */
// rcg10162001 wtf...I had to move this into the class definition itself.
//  I think it's an optimization bug; I didn't have this problem when I
//  didn't give GCC the "-O2" option.
#if 0
void CObjectPolygonEdge::GetVertices(CObjectVertex *&povxStart, CObjectVertex *&povxEnd)
{
  ASSERT(ope_Edge!=NULL);
  if (ope_Backward) {
    povxStart = ope_Edge->oed_Vertex1;
    povxEnd = ope_Edge->oed_Vertex0;
  } else {
    povxStart = ope_Edge->oed_Vertex0;
    povxEnd = ope_Edge->oed_Vertex1;
  }
}
#endif


/*
 * Default constructor.
 */
CObjectSector::CObjectSector(void) :
  osc_colColor(0), osc_colAmbient(0), osc_strName("")
{
  osc_ulFlags[0] = 0;
  osc_ulFlags[1] = 0;
  osc_ulFlags[2] = 0;
}
/*
 * Destructor.
 */
CObjectSector::~CObjectSector(void)
{
}

/*
 * Initialize the structure for the given edge.
 */
inline void CEdgeEx::Initialize(CObjectEdge *poedEdge)
{
  // remember edge
  edx_poedEdge = poedEdge;

  // initialize for edge's end vertices
  DOUBLE3D vPoint0 = *poedEdge->oed_Vertex0;
  DOUBLE3D vPoint1 = *poedEdge->oed_Vertex1;
  Initialize(&vPoint0, &vPoint1);
}

inline void CEdgeEx::Initialize(const DOUBLE3D *pvPoint0, const DOUBLE3D *pvPoint1)
{
  // if the vertices are reversed
  if (CompareVertices(*pvPoint0, *pvPoint1)<0) {
    // swap them
    Swap(pvPoint0, pvPoint1);
    // mark edge as reversed
    edx_bReverse = TRUE;

  // if the vertices are not reversed
  } else {
    // mark edge as not reversed
    edx_bReverse = FALSE;
  }
  const DOUBLE3D &vPoint0=*pvPoint0;
  const DOUBLE3D &vPoint1=*pvPoint1;

  // normalize the direction
  edx_vDirection = (vPoint1-vPoint0).Normalize();
  DOUBLE fDirectionLen = edx_vDirection.Length();
  ASSERT(0.999<fDirectionLen && fDirectionLen<1.001);
  /* calculate the reference point on the line from any point on line (p) and direction (d):
    r = p - ((p.d)/(d.d))*d
  */
  edx_vReferencePoint = vPoint0 -
    edx_vDirection * ((vPoint0 % edx_vDirection))/(edx_vDirection % edx_vDirection);
}

/* Test if a point is on the edge line. */
inline BOOL CEdgeEx::PointIsOnLine(const DOUBLE3D &vPointToTest) const
{
  // if it is the reference point
  if (CompareVertices(edx_vReferencePoint, vPointToTest)==0) {
    // it is on the line
    return TRUE;
  }

  // create edge line from line reference point to given point
  CEdgeEx edxPointToTest;
  edxPointToTest.Initialize(&edx_vReferencePoint, &vPointToTest);
  // test if it is same as this edge line
  return CompareEdgeLines(*this, edxPointToTest)==0;
}


void CObjectSector::Clear(void)
{
  osc_aovxVertices.Clear();
	osc_aoplPlanes.Clear();
	osc_aoedEdges.Clear();
	osc_aopoPolygons.Clear();
	osc_aomtMaterials.Clear();
}

/*
 * Lock all object sector dynamic arrays.
 */
void CObjectSector::LockAll( void)
{
  osc_aovxVertices.Lock();
	osc_aoplPlanes.Lock();
	osc_aoedEdges.Lock();
	osc_aopoPolygons.Lock();
	osc_aomtMaterials.Lock();
}

/*
 * Unlocks all object sector dynamic arrays.
 */
void CObjectSector::UnlockAll( void)
{
  osc_aovxVertices.Unlock();
	osc_aoplPlanes.Unlock();
	osc_aoedEdges.Unlock();
	osc_aopoPolygons.Unlock();
	osc_aomtMaterials.Unlock();
}

/*
 * Create indices for all members of all arrays.
 */
void CObjectSector::CreateIndices(void)
{
  // lock all arrays
  LockAll();

  // get the number of vertices in object
  INDEX ctVertices = osc_aovxVertices.Count();
  // set vertex indices
  for(INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
    osc_aovxVertices[iVertex].ovx_Index = iVertex;
  }

  // get the number of planes in object
  INDEX ctPlanes = osc_aoplPlanes.Count();
  // set plane indices
  for(INDEX iPlane=0; iPlane<ctPlanes; iPlane++) {
    osc_aoplPlanes[iPlane].opl_Index = iPlane;
  }

  // get the number of materials in object
  INDEX ctMaterials = osc_aomtMaterials.Count();
  // set plane indices
  for(INDEX iMaterial=0; iMaterial<ctMaterials; iMaterial++) {
    osc_aomtMaterials[iMaterial].omt_Index = iMaterial;
  }

  // get the number of edges in object
  INDEX ctEdges = osc_aoedEdges.Count();
  // set edge indices
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    osc_aoedEdges[iEdge].oed_Index = iEdge;
  }

  // get the number of polygons in object
  INDEX ctPolygons = osc_aopoPolygons.Count();
  // set polygon indices
  for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++) {
    osc_aopoPolygons[iPolygon].opo_Index = iPolygon;
  }

  // unlock all arrays
  UnlockAll();
}

/*
 * Create a new polygon in given sector.
 */
CObjectPolygon *CObjectSector::CreatePolygon(INDEX ctVertices, DOUBLE3D avVertices[],
  CObjectMaterial &omaMaterial, ULONG ulFlags, BOOL bReverse)
{
  // reset areas on axial planes
  DOUBLE fXY=0.0, fXZ=0.0, fYZ=0.0;
  // for each two vertices
  {for (INDEX ivx=0; ivx<ctVertices; ivx++){
    const DOUBLE3D &v0 = avVertices[ivx];
    const DOUBLE3D &v1 = avVertices[(ivx+1)%ctVertices];
    // add the edge to all three areas
    fXY+=(v1(1)-v0(1))*(v0(2)+v1(2))/2.0;
    fXZ+=(v1(1)-v0(1))*(v0(3)+v1(3))/2.0;
    fYZ+=(v1(2)-v0(2))*(v0(3)+v1(3))/2.0;
  }}
  // find maximum absolute area
  fXY = Abs(fXY); fXZ = Abs(fXZ); fYZ = Abs(fYZ);
  DOUBLE fMaxArea = Max(Max(fXY, fXZ), fYZ);
  // if maximum area is too small
  if(fMaxArea<1E-8) {
    // do not create polygon
    return NULL;
  }

  // create a new polygon in object
  CObjectPolygon &opoPolygon = *osc_aopoPolygons.New();
  // create as much polygon edges as there are vertices in the polygon
  CObjectPolygonEdge *popePolygonEdges = opoPolygon.opo_PolygonEdges.New(ctVertices);

  // for each vertex
  {for (INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
    DOUBLE3D &v0=avVertices[iVertex];    // that vertex
    DOUBLE3D &v1=avVertices[(iVertex+1)%ctVertices];    // its next neighbour
    // create an edge between it and its neighbour and add it to the polygon
    if (bReverse) {
      popePolygonEdges[iVertex] = CObjectPolygonEdge(&CreateEdge(v1, v0));
    } else {
      popePolygonEdges[iVertex] = CObjectPolygonEdge(&CreateEdge(v0, v1));
    }
  }}

  // clear plane normal
  DOUBLE3D vNormal = DOUBLE3D(0.0f,0.0f,0.0f);
  // for all vertices in polygon
  {for(INDEX iEdge=0; iEdge<ctVertices; iEdge++) {
		// get two neighbouring edges
    CObjectEdge &oed0 = *popePolygonEdges[iEdge].ope_Edge;
    CObjectEdge &oed1 = *popePolygonEdges[(iEdge+1)%ctVertices].ope_Edge;
		// make their vectors
  	DOUBLE3D vVector1 = *oed0.oed_Vertex0 - *oed0.oed_Vertex1;
		DOUBLE3D vVector2 = *oed1.oed_Vertex1 - *oed1.oed_Vertex0;
  	// add vector product of edges to the plane normal
    vNormal += vVector2*vVector1;
  }}
  // add one plane to planes array
  CObjectPlane *pplPlane = osc_aoplPlanes.New();
  // construct this plane from normal vector and one point
  if (bReverse) {
	  *pplPlane= DOUBLEplane3D( -vNormal, *popePolygonEdges[0].ope_Edge->oed_Vertex0);
  } else {
	  *pplPlane= DOUBLEplane3D( vNormal, *popePolygonEdges[0].ope_Edge->oed_Vertex0);
  }

  // set this polygon's plane pointer
	opoPolygon.opo_Plane = pplPlane;
  // set this polygon's material pointer and color
	opoPolygon.opo_Material = &omaMaterial;
	opoPolygon.opo_colorColor = omaMaterial.omt_Color;
  // set flags
	opoPolygon.opo_ulFlags = ulFlags;

  return &opoPolygon;
}

/*
 * Create a new polygon in given sector.
 */
CObjectPolygon *CObjectSector::CreatePolygon(INDEX ctVertices, INDEX aivVertices[],
  CObjectMaterial &omaMaterial, ULONG ulFlags, BOOL bReverse)
{
  // create a new polygon in object
  CObjectPolygon &opoPolygon = *osc_aopoPolygons.New();
  // create as much polygon edges as there are vertices in the polygon
  CObjectPolygonEdge *popePolygonEdges = opoPolygon.opo_PolygonEdges.New(ctVertices);

  // for each vertex
  {for (INDEX iVertex=0; iVertex<ctVertices; iVertex++)
  {
    INDEX iNextVertex = (iVertex+1)%ctVertices;
    // create an edge between it and its neighbour and add it to the polygon
    if (bReverse)
    {
      popePolygonEdges[iVertex] = CObjectPolygonEdge(
        &CreateEdge(aivVertices[ iVertex], aivVertices[ iNextVertex]) );
    }
    else
    {
      popePolygonEdges[iVertex] = CObjectPolygonEdge(
        &CreateEdge(aivVertices[ iNextVertex], aivVertices[ iVertex]) );
    }
  }}

  // clear plane normal
  DOUBLE3D vNormal = DOUBLE3D(0.0f,0.0f,0.0f);
  // for all vertices in polygon
  {for(INDEX iEdge=0; iEdge<ctVertices; iEdge++) {
		// get two neighbouring edges
    CObjectEdge &oed0 = *popePolygonEdges[iEdge].ope_Edge;
    CObjectEdge &oed1 = *popePolygonEdges[(iEdge+1)%ctVertices].ope_Edge;
		// make their vectors
  	DOUBLE3D vVector1 = *oed0.oed_Vertex0 - *oed0.oed_Vertex1;
		DOUBLE3D vVector2 = *oed1.oed_Vertex1 - *oed1.oed_Vertex0;
  	// add vector product of edges to the plane normal
    vNormal += vVector2*vVector1;
  }}
  // add one plane to planes array
  CObjectPlane *pplPlane = osc_aoplPlanes.New();
  // construct this plane from normal vector and one point
  if (bReverse) {
	  *pplPlane= DOUBLEplane3D( -vNormal, *popePolygonEdges[0].ope_Edge->oed_Vertex0);
  } else {
	  *pplPlane= DOUBLEplane3D( vNormal, *popePolygonEdges[0].ope_Edge->oed_Vertex0);
  }

  // set this polygon's plane pointer
	opoPolygon.opo_Plane = pplPlane;
  // set this polygon's material pointer and color
	opoPolygon.opo_Material = &omaMaterial;
	opoPolygon.opo_colorColor = omaMaterial.omt_Color;
  // set flags
	opoPolygon.opo_ulFlags = ulFlags;

  return &opoPolygon;
}
/*
 * Check the optimization algorithm.
 */
void CObjectSector::CheckOptimizationAlgorithm(void)
{
  // for vertices
#if 0 // DG: this doesn't really do anything?!
  FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itvx1) {
    FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itvx2) {
      CObjectVertex &vx1 = itvx1.Current();
      CObjectVertex &vx2 = itvx2.Current();
      // !!!!why this fails sometimes ?(on spheres) ASSERT( (&vx1 == &vx2) || (CompareVertices(vx1, vx2)!=0) );
    }
  }
#endif // 0

  // for planes
  FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itpl1) {
    FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itpl2) {
      CObjectPlane &pl1 = itpl1.Current();
      CObjectPlane &pl2 = itpl2.Current();
      ASSERT( (&pl1 == &pl2) || (ComparePlanes(pl1, pl2)!=0));

    }
  }

  // for edges
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited1) {
    FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited2) {
      CObjectEdge &ed1 = ited1.Current();
      CObjectEdge &ed2 = ited2.Current();
      ASSERT( (&ed1 == &ed2)
        || (ed1.oed_Vertex0 != ed2.oed_Vertex0) || (ed1.oed_Vertex1 != ed2.oed_Vertex1) );
    }
  }}

  // for inverse edges
#ifndef NDEBUG
  INDEX iInversesFound = 0;
#endif // NDEBUG
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited1) {
    FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited2) {
      CObjectEdge &ed1 = ited1.Current();
      CObjectEdge &ed2 = ited2.Current();
      if (&ed1 == &ed2) continue;
#ifndef NDEBUG
      BOOL inv1, inv2;
      inv1 = (&ed1 == ed2.optimize.oed_InverseEdge);
      inv2 = (&ed2 == ed1.optimize.oed_InverseEdge);
      // check against cross linked inverses
      ASSERT(!(inv1&&inv2));
      if (inv1) {
        iInversesFound++;
      }
#endif // NDEBUG
      // check against inverses that have not been found
      ASSERT( (inv1 || inv2) || (ed1.oed_Vertex0 != ed2.oed_Vertex1) || (ed1.oed_Vertex1 != ed2.oed_Vertex0) );
    }
  }}

  // for each polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // for each edge in polygon
    {FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      //CObjectEdge &oedThis = *itope->ope_Edge;
      CObjectVertex *povxStartThis, *povxEndThis;
      // get start and end vertices
      itope->GetVertices(povxStartThis, povxEndThis);
      // if start vertex is not on the plane
      DOUBLE fDistance = itopo->opo_Plane->PointDistance(*povxStartThis);
      if (Abs(fDistance)>(VTX_EPSILON*16.0)) {
        // report it
        _RPT4(_CRT_WARN, "(%f, %f, %f) is 1/%f from plane ",
          (*povxStartThis)(1),
          (*povxStartThis)(2),
          (*povxStartThis)(3),
          1.0/fDistance);
        _RPT4(_CRT_WARN, "(%f, %f, %f):%f\n",
          (*itopo->opo_Plane)(1),
          (*itopo->opo_Plane)(2),
          (*itopo->opo_Plane)(3),
          itopo->opo_Plane->Distance());
      }
    }}
  }}
}

BOOL CObjectSector::ArePolygonsPlanar(void)
{
  // for each polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // for each edge in polygon
    {FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      //CObjectEdge &oedThis = *itope->ope_Edge;
      CObjectVertex *povxStartThis, *povxEndThis;
      // get start and end vertices
      itope->GetVertices(povxStartThis, povxEndThis);
      // if start vertex is not on the plane
      DOUBLE fDistance = itopo->opo_Plane->PointDistance(*povxStartThis);
      if (Abs(fDistance)>(VTX_EPSILON*16.0)) {
        return FALSE;
      }
    }}
  }}
  return TRUE;
}

/*
 * Remap different planes with same coordinates to use only one of each.
 */
void CObjectSector::RemapClonedPlanes(void)
{
  // if there are no planes in the sector
  if (osc_aoplPlanes.Count()==0) {
    // do nothing (optimization algorithms assume at least one plane present)
    return;
  }

  osc_aoplPlanes.Lock();

  /* Prepare sorted array for remapping planes. */

  // create an array of pointers for sorting planes
  INDEX ctPlanes = osc_aoplPlanes.Count();
  CStaticArray<CObjectPlane *> applSortedPlanes;
  applSortedPlanes.New(ctPlanes);
  // for all vertices
  for(INDEX iPlane=0; iPlane<ctPlanes; iPlane++) {
    // set the pointer in sorting array
    applSortedPlanes[iPlane] = &osc_aoplPlanes[iPlane];
    // set remap pointer to itself
    osc_aoplPlanes[iPlane].opl_Remap = &osc_aoplPlanes[iPlane];
  }

  if( wld_bFastObjectOptimization) {
    // sort the array of pointers, so that same planes get next to each other
    qsort(&applSortedPlanes[0], ctPlanes, sizeof(CObjectPlane *), qsort_ComparePlanes);

    /* Create remapping pointers. */

    // for all planes in sorted array, except the last one
    for(INDEX iSortedPlane=0; iSortedPlane<ctPlanes-1; iSortedPlane++) {
      // if the next plane is same as this one
      if ( ComparePlanes(*applSortedPlanes[iSortedPlane],
                         *applSortedPlanes[iSortedPlane+1]) == 0 ) {
        // set its remap pointer to same as this remap pointer
        applSortedPlanes[iSortedPlane+1]->opl_Remap = applSortedPlanes[iSortedPlane]->opl_Remap;
      }
    }

  } else {
    // for all pairs of planes
    {for(INDEX iPlane1=0; iPlane1<ctPlanes; iPlane1++) {
      CObjectPlane &pl1 = osc_aoplPlanes[iPlane1];
      if (pl1.opl_Remap != &pl1) {
        continue;
      }
      for(INDEX iPlane2=iPlane1+1; iPlane2<ctPlanes; iPlane2++) {
        CObjectPlane &pl2 = osc_aoplPlanes[iPlane2];
        if (ComparePlanes(pl1, pl2)==0) {
          pl2.opl_Remap = pl1.opl_Remap;
        }
      }
    }}
  }

  /* Remap all references to planes. */
  // for all polygons in object
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // remap plane pointers
    itopo->opo_Plane = itopo->opo_Plane->opl_Remap;
  }}

  osc_aoplPlanes.Unlock();
}

/*
 * Remap different vertices with same coordinates to use only one of each.
 */
//static BOOL bBug=FALSE;
void CObjectSector::RemapClonedVertices(void)
{
  // if there are no vertices in the sector
  if (osc_aovxVertices.Count()==0) {
    // do nothing (optimization algorithms assume at least one vertex present)
    return;
  }

  osc_aovxVertices.Lock();

  /* Prepare sorted array for remapping vertices. */

  // create an array of pointers for sorting vertices
  INDEX ctVertices = osc_aovxVertices.Count();
  CStaticArray<CObjectVertex *> apvxSortedVertices;
  apvxSortedVertices.New(ctVertices);
  // for all vertices
  for(INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
    // set the pointer in sorting array
    apvxSortedVertices[iVertex] = &osc_aovxVertices[iVertex];
    // set remap pointer to itself
    osc_aovxVertices[iVertex].ovx_Remap = &osc_aovxVertices[iVertex];
  }

  if( wld_bFastObjectOptimization) {
    // sort the array of pointers, so that same vertices get next to each other
    qsort(&apvxSortedVertices[0], ctVertices, sizeof(CObjectVertex *), qsort_CompareVertices);

    /* Create remapping pointers. */

    // for all vertices in sorted array, except the last one
    for(INDEX iSortedVertex=0; iSortedVertex<ctVertices-1; iSortedVertex++) {
      // if the next vertex is same as this one
      if ( CompareVertices(*apvxSortedVertices[iSortedVertex],
                           *apvxSortedVertices[iSortedVertex+1]) == 0 ) {
        // set its remap pointer to same as this remap pointer
        apvxSortedVertices[iSortedVertex+1]->ovx_Remap = apvxSortedVertices[iSortedVertex]->ovx_Remap;

      // otherwise
      } else {
        #ifndef NDEBUG
        // check that it is really ok
        CObjectVertex &vx1 = *apvxSortedVertices[iSortedVertex];
        CObjectVertex &vx2 = *apvxSortedVertices[iSortedVertex+1];
        ASSERT( (&vx1 == &vx2) || (CompareVertices(vx1, vx2)!=0));
        #endif
      }
    }
  } else {
    // for all pairs of vertices
    {for(INDEX iVertex1=0; iVertex1<ctVertices; iVertex1++) {
      CObjectVertex &vx1 = osc_aovxVertices[iVertex1];
      if (vx1.ovx_Remap != &vx1) {
        continue;
      }
      for(INDEX iVertex2=iVertex1+1; iVertex2<ctVertices; iVertex2++) {
        CObjectVertex &vx2 = osc_aovxVertices[iVertex2];
        if (CompareVertices(vx1, vx2)==0) {
          vx2.ovx_Remap = vx1.ovx_Remap;
        }
      }
    }}
  }

  /* Remap all references to vertices. */
  // for all edges in object
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited) {
    // remap vertex pointers
    ited->oed_Vertex0 = ited->oed_Vertex0->ovx_Remap;
    ited->oed_Vertex1 = ited->oed_Vertex1->ovx_Remap;
  }}

  osc_aovxVertices.Unlock();

#if 0
#ifndef NDEBUG
  if (bBug) goto skipbug;
  // for vertices
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itvx1) {
    FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itvx2) {
      CObjectVertex &vx1 = *(itvx1->ovx_Remap);
      CObjectVertex &vx2 = *(itvx2->ovx_Remap);
      //ASSERT( (&vx1 == &vx2) || (CompareVertices(vx1, vx2)!=0));
      if (!( (&vx1 == &vx2) || (CompareVertices(vx1, vx2)!=0))) {
        goto bug;
      }
    }
  }}
  goto skipbug;
bug:
  bBug = TRUE;

  // for all vertices in sorted array, except the last one
  {for(INDEX iSortedVertex=0; iSortedVertex<ctVertices-1; iSortedVertex++) {
    CPrintF("(%f,%f,%f)-(%f,%f,%f) ",
      (*apvxSortedVertices[iSortedVertex])(1),
      (*apvxSortedVertices[iSortedVertex])(2),
      (*apvxSortedVertices[iSortedVertex])(3),
      (*apvxSortedVertices[iSortedVertex+1])(1),
      (*apvxSortedVertices[iSortedVertex+1])(2),
      (*apvxSortedVertices[iSortedVertex+1])(3)
      );
    CPrintF("%d\n", CompareVertices(*apvxSortedVertices[iSortedVertex],
                         *apvxSortedVertices[iSortedVertex+1])
      );
  }}

skipbug:;
#endif //NDEBUG
#endif //0
}

/*
 * Remove vertices that are not used by any edge.
 */
void CObjectSector::RemoveUnusedVertices(void)
{
  // if there are no vertices in the sector
  if (osc_aovxVertices.Count()==0) {
    // do nothing (optimization algorithms assume at least one vertex present)
    return;
  }

  // clear all vertex tags
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    itovx->ovx_Tag = FALSE;
  }}

  // mark all vertices that are used by some edge
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited) {
    ited->oed_Vertex0->ovx_Tag = TRUE;
    ited->oed_Vertex1->ovx_Tag = TRUE;
  }}

  // find number of used vertices
  INDEX ctUsedVertices = 0;
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    if (itovx->ovx_Tag) {
      ctUsedVertices++;
    }
  }}

  // create a new array with as much vertices as we have counted in last pass
  CDynamicArray<CObjectVertex> aovxNew;
  CObjectVertex *povxUsed = aovxNew.New(ctUsedVertices);

  // for each vertex
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    // if it is used
    if (itovx->ovx_Tag) {
      // copy it to new array
      *povxUsed = itovx.Current();
      // set its remap pointer into new array
      itovx->ovx_Remap = povxUsed;
      povxUsed++;
    // if it is not used
    } else {
      // clear its remap pointer (for debugging)
      #ifndef NDEBUG
      itovx->ovx_Remap = NULL;
      #endif
    }
  }}

  // for all edges in object
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited) {
    // remap vertex pointers
    ited->oed_Vertex0 = ited->oed_Vertex0->ovx_Remap;
    ited->oed_Vertex1 = ited->oed_Vertex1->ovx_Remap;
  }}

  // use new array of vertices instead of the old one
  osc_aovxVertices.Clear();
  osc_aovxVertices.MoveArray(aovxNew);
}

/*
 * Remove edges that are not used by any polygon.
 */
void CObjectSector::RemoveUnusedEdges(void)
{
  // if there are no edges in the sector
  if (osc_aoedEdges.Count()==0) {
    // do nothing (optimization algorithms assume at least one edge present)
    return;
  }

  // clear all edge tags
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, itoed) {
    itoed->oed_Tag = FALSE;
  }}

  // mark all edges that are used by some polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    {FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      itope->ope_Edge->oed_Tag = TRUE;
    }}
  }}

  // find number of used edges
  INDEX ctUsedEdges = 0;
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, itoed) {
    if (itoed->oed_Tag) {
      ctUsedEdges++;
    }
  }}

  // create a new array with as much edges as we have counted in last pass
  CDynamicArray<CObjectEdge> aoedNew;
  CObjectEdge *poedUsed = aoedNew.New(ctUsedEdges);

  // for each edge
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, itoed) {
    // if it is used
    if (itoed->oed_Tag) {
      // copy it to new array
      *poedUsed = itoed.Current();
      // set its remap pointer into new array
      itoed->optimize.oed_Remap = poedUsed;
      poedUsed++;
    // if it is not used
    } else {
      // clear its remap pointer (for debugging)
      #ifndef NDEBUG
      itoed->optimize.oed_Remap = NULL;
      #endif
    }
  }}

  // remap edge pointers in all polygons
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    {FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      itope->ope_Edge = itope->ope_Edge->optimize.oed_Remap;
    }}
  }}

  // use new array of edges instead the old one
  osc_aoedEdges.Clear();
  osc_aoedEdges.MoveArray(aoedNew);
}

/*
 * Remove planes that are not used by any polygon.
 */
void CObjectSector::RemoveUnusedPlanes(void)
{
  // if there are no planes in the sector
  if (osc_aoplPlanes.Count()==0) {
    // do nothing (optimization algorithms assume at least one plane present)
    return;
  }

  // clear all plane tags
  {FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itopl) {
    itopl->opl_Tag = FALSE;
  }}

  // mark all planes that are used by some polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    itopo->opo_Plane->opl_Tag = TRUE;
  }}

  // find number of used planes
  INDEX ctUsedPlanes = 0;
  {FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itopl) {
    if (itopl->opl_Tag) {
      ctUsedPlanes++;
    }
  }}

  // create a new array with as much planes as we have counted in last pass
  CDynamicArray<CObjectPlane> aoplNew;
  CObjectPlane *poplUsed = aoplNew.New(ctUsedPlanes);

  // for each plane
  {FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itopl) {
    // if it is used
    if (itopl->opl_Tag) {
      // copy it to new array
      *poplUsed = itopl.Current();
      // set its remap pointer into new array
      itopl->opl_Remap = poplUsed;
      poplUsed++;
    // if it is not used
    } else {
      // clear its remap pointer (for debugging)
      #ifndef NDEBUG
      itopl->opl_Remap = NULL;
      #endif
    }
  }}

  // remap plane pointers in all polygons
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    itopo->opo_Plane = itopo->opo_Plane->opl_Remap;
  }}

  // use new array of planes instead the old one
  osc_aoplPlanes.Clear();
  osc_aoplPlanes.MoveArray(aoplNew);
}

/*
 * Split an edge with a run of vertices.
 * NOTE: 'sortvertices_iMaxAxis' must be set correctly!
 */
void CObjectSector::SplitEdgeWithVertices(CObjectEdge &oedOriginal,
  CStaticArray<CObjectVertex *> &apvxVertices)
{
  CObjectVertex *pvx0 = oedOriginal.oed_Vertex0;
  CObjectVertex *pvx1 = oedOriginal.oed_Vertex1;
  BOOL bReversed;

  // edge must have length
  ASSERT(CompareVertices(*pvx0, *pvx1)!=0);

  // if the edge is going in the direction of the run
  if (CompareVerticesAlongLine(*pvx0, *pvx1)<0) {
    // mark that it is not reversed
    bReversed = FALSE;

  // if the edge is going in opposite direction of the run
  } else {
    // swap vertices
    Swap(pvx0, pvx1);
    // mark that it is reversed
    bReversed = TRUE;
  }

  // init the edge as remap pointer
  oedOriginal.colinear1.oed_FirstChild = NULL;

  INDEX iVertex;  // current vertex in run
  // skip until start vertex is found
  for (iVertex=0; apvxVertices[iVertex]!=pvx0; iVertex++) {
    NOTHING;
  }

  // for all vertices after start vertex and stopping with end vertex
  for (iVertex++; apvxVertices[iVertex-1]!=pvx1; iVertex++) {
    // if vertex is not same as last one
    if (apvxVertices[iVertex]!=apvxVertices[iVertex-1]) {
      // create a new edge between the two vertices
      CObjectEdge *poedNewChild = osc_aoedEdges.New();
      if (bReversed) {
        *poedNewChild = CObjectEdge(*apvxVertices[iVertex], *apvxVertices[iVertex-1]);
      } else {
        *poedNewChild = CObjectEdge(*apvxVertices[iVertex-1], *apvxVertices[iVertex]);
      }
      ASSERT(CompareVertices(*poedNewChild->oed_Vertex0, *poedNewChild->oed_Vertex1)!=0);
      // link it as a child of this edge
      poedNewChild->colinear1.oed_NextSibling = oedOriginal.colinear1.oed_FirstChild;
      oedOriginal.colinear1.oed_FirstChild = poedNewChild;
    }
  }
}

/*
 * Join polygon edges that are collinear are continuing.
 */
void CObjectSector::JoinContinuingPolygonEdges(void)
{
  osc_aoedEdges.Lock();

  // create array of extended edge infosd
  CStaticArray<CEdgeEx> aedxEdgeLines;
  CStaticArray<CEdgeEx *> apedxSortedEdgeLines;
  INDEX ctEdges = osc_aoedEdges.Count();
  CreateEdgeLines(aedxEdgeLines, apedxSortedEdgeLines, ctEdges);
  /* NOTE: The array doesn't have to be sorted because only
     edges in one polygon are considered.
   */
  // for each polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // join continuing edges in it
    itopo->JoinContinuingEdges(osc_aoedEdges);
  }}

  osc_aoedEdges.Unlock();
}
/*
 * Join polygon edges that are collinear and continuing.
 */
/*
 * NOTE: Edge line infos must be prepared before calling this.
 */
void CObjectPolygon::JoinContinuingEdges(CDynamicArray<CObjectEdge> &oedEdges)
{
  // create an empty array for newly created edges
  CDynamicArray<CObjectPolygonEdge> aopeNew;
  // set the counter of edges to current number of edges
  //INDEX ctEdges = opo_PolygonEdges.Count();

  // for each edge
  {FOREACHINDYNAMICARRAY(opo_PolygonEdges, CObjectPolygonEdge, itope) {
    CObjectEdge *poedThis = itope->ope_Edge;

    // if not already marked for removal
    if (poedThis != NULL) {
      CObjectVertex *povxStartThis, *povxEndThis;
      // get start and end vertices
      itope->GetVertices(povxStartThis, povxEndThis);
      // mark the original edge for removal
      itope->ope_Edge = NULL;

      BOOL bChangeDone;   // termination condition flag
      // repeat
      do {
        // mark that nothing is changed
        bChangeDone = FALSE;

        // for each edge
        {FOREACHINDYNAMICARRAY(opo_PolygonEdges, CObjectPolygonEdge, itope2) {
          CObjectEdge *poedOther = itope2->ope_Edge;

          // if not already marked for removal
          if (poedOther != NULL) {
            // if the two edges are collinear
            if ( CompareEdgeLines(*poedThis->colinear2.oed_pedxLine, *poedOther->colinear2.oed_pedxLine)==0) {
              CObjectVertex *povxStartOther, *povxEndOther;
              // get start and end vertices
              itope2->GetVertices(povxStartOther, povxEndOther);

              // if the other edge is continuing to this one
              if ( povxStartOther == povxEndThis ) {
                // extend the current edge by it
                povxEndThis = povxEndOther;
                // mark it for removal
                itope2->ope_Edge = NULL;
                // mark that a change is done
                bChangeDone = TRUE;

              // if the other edge is continued by this one
              } else if ( povxStartThis == povxEndOther) {
                // extend the current edge by it
                povxStartThis = povxStartOther;
                // mark it for removal
                itope2->ope_Edge = NULL;
                // mark that a change is done
                bChangeDone = TRUE;
              }
            }
          }
        }}

      // until a pass is made without making any change
      } while (bChangeDone);

      // create new edge with given start and end vertices
      CObjectEdge *poedNew = oedEdges.New();
      *poedNew = CObjectEdge(*povxStartThis, *povxEndThis);
      // add it to new array
      *aopeNew.New() = CObjectPolygonEdge(poedNew);
    }
  }}

  // replace old array with the new one
  opo_PolygonEdges.Clear();
  opo_PolygonEdges.MoveArray(aopeNew);
}

/*
 * Split a run of collinear edges.
 */
void CObjectSector::SplitCollinearEdgesRun(CStaticArray<CEdgeEx *> &apedxSortedEdgeLines,
  INDEX iFirstInRun, INDEX iLastInRun)
{
  if (iFirstInRun>iLastInRun) {
    return; // this should not happen, but anyway!
  }

  /* set up array of vertex pointers */
/*
  CEdgeEx &edxLine = *apedxSortedEdgeLines[iFirstInRun];  // representative line of the run
  // for each vertex in sector
  INDEX ctVerticesOnLine=0;
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    // if it is on the line
    if (edxLine.PointIsOnLine(*itovx)) {
      // mark it as on the line
      itovx->ovx_Tag = TRUE;
      ctVerticesOnLine++;

    // if it is on the line
    } else {
      // mark it as on not the line
      itovx->ovx_Tag = FALSE;
    }
  }}
  // for all edges in run
  {for(INDEX iEdgeInRun=iFirstInRun; iEdgeInRun<=iLastInRun; iEdgeInRun++) {
    CObjectEdge &oed = *apedxSortedEdgeLines[iEdgeInRun]->edx_poedEdge;
    // if first vertex is not marked
    if (!oed.oed_Vertex0->ovx_Tag) {
      // mark it
      oed.oed_Vertex0->ovx_Tag = TRUE;
      ctVerticesOnLine++;
    }
    // if second vertex is not marked
    if (!oed.oed_Vertex1->ovx_Tag) {
      // mark it
      oed.oed_Vertex1->ovx_Tag = TRUE;
      ctVerticesOnLine++;
    }
  }}

  // if there are no vertices
  if (ctVerticesOnLine==0) {
    // do nothing
    return;
  }

  // create array of vertex pointers
  CStaticArray<CObjectVertex *> apvxSortedVertices;
  apvxSortedVertices.New(ctVerticesOnLine);

  // for each vertex in sector
  INDEX iVertexOnLine=0;
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    // if it is marked
    if (itovx->ovx_Tag) {
      // set its pointer in the array
      apvxSortedVertices[iVertexOnLine++] = &*itovx;
    }
  }}
  ASSERT(iVertexOnLine == ctVerticesOnLine);
  */

  // create array of vertex pointers
  INDEX ctVerticesOnLine = (iLastInRun-iFirstInRun+1)*2;
  // if there are no vertices
  if (ctVerticesOnLine==0) {
    // do nothing
    return;
  }
  CStaticArray<CObjectVertex *> apvxSortedVertices;
  apvxSortedVertices.New(ctVerticesOnLine);
  // for all edges in run
  {for(INDEX iEdgeInRun=iFirstInRun; iEdgeInRun<=iLastInRun; iEdgeInRun++) {
    CObjectEdge &oed = *apedxSortedEdgeLines[iEdgeInRun]->edx_poedEdge;
    // set vertex pointers
    apvxSortedVertices[(iEdgeInRun-iFirstInRun)*2]   = oed.oed_Vertex0;
    apvxSortedVertices[(iEdgeInRun-iFirstInRun)*2+1] = oed.oed_Vertex1;
  }}

  /* sort the array of vertex pointers */

  // get axis direction from any edge in the run
  DOUBLE3D vDirection = apedxSortedEdgeLines[iFirstInRun]->edx_vDirection;
  // find largest axis of direction vector
  INDEX iMaxAxis = 0;
  DOUBLE fMaxAxis = 0.0f;
  for (INDEX iAxis=1; iAxis<=3; iAxis++) {
    if ( Abs(vDirection(iAxis)) > Abs(fMaxAxis) ) {
      fMaxAxis = vDirection(iAxis);
      iMaxAxis = iAxis;
    }
  }

  // set axis for sorting
  sortvertices_iMaxAxis = iMaxAxis;
  DOUBLE fMaxAxisSign = Sgn(fMaxAxis);
  ASSERT(Abs(fMaxAxisSign) > +VTX_EPSILON);

  // if axis is positive
  if (fMaxAxisSign > +VTX_EPSILON) {
    // sort vertex pointers along the line
    if (ctVerticesOnLine>0) {
      qsort(&apvxSortedVertices[0], ctVerticesOnLine, sizeof(CObjectVertex *),
        qsort_CompareVerticesAlongLine);
    }

  // if axis is negative
  } else if (fMaxAxisSign < -VTX_EPSILON) {
    // sort vertex pointers along the line reversely
    if (ctVerticesOnLine>0) {
      qsort(&apvxSortedVertices[0], ctVerticesOnLine, sizeof(CObjectVertex *),
        qsort_CompareVerticesAlongLine);
        //qsort_CompareVerticesAlongLineReversely); ?
    }
  }

  /* split each edge in turn */

  // for all edges in run
  {for(INDEX iEdgeInRun=iFirstInRun; iEdgeInRun<=iLastInRun; iEdgeInRun++) {
    CObjectEdge &oed = *apedxSortedEdgeLines[iEdgeInRun]->edx_poedEdge;
    // split the edge with the vertices
    SplitEdgeWithVertices(oed, apvxSortedVertices);
  }}
}

/*
 * Create array of extended edge infos.
 */
void CObjectSector::CreateEdgeLines(CStaticArray<CEdgeEx> &aedxEdgeLines,
  CStaticArray<CEdgeEx *> &apedxSortedEdgeLines, INDEX &ctEdges)
{
  // create array of extended edge infos
  ctEdges = osc_aoedEdges.Count();
  aedxEdgeLines.New(ctEdges);
  // create array of pointers to extended edge infos for sorting edges
  apedxSortedEdgeLines.New(ctEdges);
  // for all edges
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    // create extended info
    aedxEdgeLines[iEdge].Initialize(&osc_aoedEdges[iEdge]);
    // set the pointer in sorting array
    apedxSortedEdgeLines[iEdge] = &aedxEdgeLines[iEdge];
    // set back-reference in the edge
    osc_aoedEdges[iEdge].colinear2.oed_pedxLine = &aedxEdgeLines[iEdge];
  }
}
/*
 * Find collinear edges and cross-split them with each other.
 */
void CObjectSector::SplitCollinearEdges(void)
{
  osc_aoedEdges.Lock();

  /* Prepare arrays for splitting edges. */

  // create array of extended edge infosd
  CStaticArray<CEdgeEx> aedxEdgeLines;
  CStaticArray<CEdgeEx *> apedxSortedEdgeLines;
  INDEX ctEdges = osc_aoedEdges.Count();
  CreateEdgeLines(aedxEdgeLines, apedxSortedEdgeLines, ctEdges);
  // sort the array of pointers, so that collinear edges get next to each other
  if (ctEdges>0) {
    qsort(&apedxSortedEdgeLines[0], ctEdges, sizeof(CEdgeEx *), qsort_CompareEdgeLines);
  }

  /* Create remapping pointers. */

  // for all edges in normal sorted array, starting at second one
  INDEX iFirstInLine = 0;
  for(INDEX iSortedEdge=1; iSortedEdge<ctEdges; iSortedEdge++) {
    // if the previous edge is not collinear with this one
    if ( CompareEdgeLines(*apedxSortedEdgeLines[iSortedEdge],
                         *apedxSortedEdgeLines[iSortedEdge-1]) != 0 ) {
      // split the last run of collinear edges
      SplitCollinearEdgesRun(apedxSortedEdgeLines, iFirstInLine, iSortedEdge-1);
      // start a new run of collinear edges
      iFirstInLine = iSortedEdge;
    }
  }
  // split the last run of collinear edges
  SplitCollinearEdgesRun(apedxSortedEdgeLines, iFirstInLine, ctEdges-1);

  /* Remap references to edges. */

  // for all polygons in object
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itpo2) {

    /* create new array of polygon edges for child edges */

    // for all of its edge pointers
    INDEX ctNewEdges = 0;
    {FOREACHINDYNAMICARRAY(itpo2->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      // count all children
      for (CObjectEdge *poed = itope->ope_Edge->colinear1.oed_FirstChild;
          poed!=NULL;
          poed = poed->colinear1.oed_NextSibling) {
        ctNewEdges++;
      }
    }}
    // create a new array of edge references
    CDynamicArray<CObjectPolygonEdge> aopoNewPolygonEdges;
    if (ctNewEdges>0) {
      aopoNewPolygonEdges.New(ctNewEdges);
    }
    aopoNewPolygonEdges.Lock();

    /* set the array */

    // for all of its edge pointers
    INDEX iChildEdge = 0;
    {FOREACHINDYNAMICARRAY(itpo2->opo_PolygonEdges, CObjectPolygonEdge, itope) {

      // for all of its children
      for (CObjectEdge *poed = itope->ope_Edge->colinear1.oed_FirstChild;
          poed!=NULL;
          poed = poed->colinear1.oed_NextSibling) {
        // set the child polygon edge
        aopoNewPolygonEdges[iChildEdge] = CObjectPolygonEdge(poed, itope->ope_Backward);
        iChildEdge++;
      }
    }}

    /* replace old array with the new one */
    aopoNewPolygonEdges.Unlock();
    itpo2->opo_PolygonEdges.Clear();
    itpo2->opo_PolygonEdges.MoveArray(aopoNewPolygonEdges);
  }}

  osc_aoedEdges.Unlock();
}

/*
 * Remove polygon edges that are used twice from all polygons.
 */
void CObjectSector::RemoveRedundantPolygonEdges(void)
{
  // for each polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // remove redundant edges from it
    itopo->RemoveRedundantEdges();
  }}
}

/*
 * Remove polygon edges that are used twice from a polygon.
 */
/*
 * We will use ope_Edge set to NULL for marking an edge for removal.
 */
void CObjectPolygon::RemoveRedundantEdges(void)
{
  // set the counter of edges to current number of edges
  INDEX ctEdges = opo_PolygonEdges.Count();

  // for each edge
  {FOREACHINDYNAMICARRAY(opo_PolygonEdges, CObjectPolygonEdge, itope) {

    // if not already marked for removal
    if (itope->ope_Edge != NULL) {

      // for each edge
      {FOREACHINDYNAMICARRAY(opo_PolygonEdges, CObjectPolygonEdge, itope2) {
        // if it uses same edge in opposite direction
        if (itope2->ope_Edge == itope->ope_Edge
         && itope2->ope_Backward != itope->ope_Backward ) {
          // mark them both for removal
          itope2->ope_Edge = NULL;
          itope->ope_Edge  = NULL;
          // mark that there are two edges less
          ctEdges -= 2;
          // don't test this edge any more
          break;
        }
      }}
    }
  }}
  // remove polygon edges that are mark as unused
  RemoveMarkedEdges(ctEdges);
}

/*
 * Remove polygon edges that are marked as unused (oed_Edge==NULL) from polygon.
 */
void CObjectPolygon::RemoveMarkedEdges(INDEX ctEdges)
{
  // create a new array of edge references
  CDynamicArray<CObjectPolygonEdge> aoedNewPolygonEdges;
  if (ctEdges>0) {
    aoedNewPolygonEdges.New(ctEdges);
  }
  aoedNewPolygonEdges.Lock();

  // for each edge
  INDEX iNewEdge = 0;
  {FOREACHINDYNAMICARRAY(opo_PolygonEdges, CObjectPolygonEdge, itope) {

    // if it is not marked for removal
    if (itope->ope_Edge != NULL) {
      // copy it
      aoedNewPolygonEdges[iNewEdge++] = itope.Current();
    }
  }}

  // replace old array with the new one
  aoedNewPolygonEdges.Unlock();
  opo_PolygonEdges.Clear();
  opo_PolygonEdges.MoveArray(aoedNewPolygonEdges);
}

/*
 * Remove polygon edges that have zero length from a polygon.
 */
void CObjectPolygon::RemoveDummyEdgeReferences(void)
{
  INDEX ctUsedEdges = opo_PolygonEdges.Count();
  // for all edges in polygon
  {FOREACHINDYNAMICARRAY(opo_PolygonEdges, CObjectPolygonEdge, itope) {
    // if it has zero length
    if (itope->ope_Edge->oed_Vertex0 == itope->ope_Edge->oed_Vertex1) {
      ASSERT(CompareVertices(*itope->ope_Edge->oed_Vertex0, *itope->ope_Edge->oed_Vertex1)==0);
      // mark it for removal
      itope->ope_Edge = NULL;
      ctUsedEdges--;
    } else {
      // !!!! why this fails sometimes? ASSERT(CompareVertices(*itope->ope_Edge->oed_Vertex0, *itope->ope_Edge->oed_Vertex1)!=0);
    }
  }}

  // remove all marked edges from the polygon
  RemoveMarkedEdges(ctUsedEdges);
}

/*
 * Find edges that have zero length and remove them from all polygons.
 */
void CObjectSector::RemoveDummyEdgeReferences(void)
{
  // for each polygon
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // remove zero length edges from it
    itopo->RemoveDummyEdgeReferences();
  }}
}

/*
 * Remap different edges with same or reverse vertices to use only one of each.
 */
void CObjectSector::RemapClonedEdges(void)
{
  // if there are no edges in the sector
  if (osc_aoedEdges.Count()==0) {
    // do nothing
    return;
  }

  osc_aoedEdges.Lock();

  /* Prepare sorted arrays for remapping edges. */

  // create array of pointers for sorting edges
  INDEX ctEdges = osc_aoedEdges.Count();
  CStaticArray<CObjectEdge *> apedSortedEdges;
  apedSortedEdges.New(ctEdges);
  // for all edges
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    // set the pointers in sorting array
    apedSortedEdges[iEdge] = &osc_aoedEdges[iEdge];
    // set remap pointer to itself
    osc_aoedEdges[iEdge].optimize.oed_Remap = &osc_aoedEdges[iEdge];
    // clear the inverse pointer
    osc_aoedEdges[iEdge].optimize.oed_InverseEdge = NULL;
    // clear the edge tag, meaning that this edge is unused
    osc_aoedEdges[iEdge].oed_Tag = FALSE;
  }
  // sort the array of pointers, so that same edges get next to each other
  qsort(&apedSortedEdges[0], ctEdges, sizeof(CObjectEdge *), qsort_CompareEdges);

  /* Create remapping pointers. */

  // for all edges in normal sorted array, except the last one
  for(INDEX iSortedEdge=0; iSortedEdge<ctEdges-1; iSortedEdge++) {
    // if the next plane is same as this one
    if ( CompareEdges(*apedSortedEdges[iSortedEdge],
                         *apedSortedEdges[iSortedEdge+1]) == 0 ) {
      // set its remap pointer to same as this remap pointer
      apedSortedEdges[iSortedEdge+1]->optimize.oed_Remap = apedSortedEdges[iSortedEdge]->optimize.oed_Remap;
    }
  }

  /* Create inverse pointers. */

  // for all edges in sorted array
  for(INDEX iNormalEdge=0; iNormalEdge<ctEdges; iNormalEdge++) {
    CObjectEdge &edNormal = *apedSortedEdges[iNormalEdge];
    CObjectEdge *pedInverse;

    // if this edge is not remapped
    if (edNormal.optimize.oed_Remap == &edNormal) {
      // if some edge with inverse vertices is found
      if (FindEdge(apedSortedEdges, *edNormal.oed_Vertex1, *edNormal.oed_Vertex0, &pedInverse)) {
        // take its remap edge
        CObjectEdge &edInverse = *pedInverse->optimize.oed_Remap;
        // if the inverse edge pointer is less than this pointer
        if (&edInverse < &edNormal) {
          // the inverse edge must not be remapped
          ASSERT(&edInverse == edInverse.optimize.oed_Remap);
          // set the inverse pointer in this edge to the inverse edge
          edNormal.optimize.oed_InverseEdge = &edInverse;
        }
      }
    }
  }

#ifndef NDEBUG
  // for all edges in object
  {FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, ited) {
    CObjectEdge &edInverse = *ited->optimize.oed_InverseEdge;
    // check that no remapped edges have been marked as inverses
    ASSERT( &edInverse==NULL || edInverse.optimize.oed_Remap == &edInverse );
  }}
#endif // NDEBUG

  /* Remap all references to edges. */

  // for all polygons in object
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itpo2) {
    // for all of its edge pointers
    FOREACHINDYNAMICARRAY(itpo2->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      // get the remapped edge pointer
      CObjectEdge *pedNew= itope->ope_Edge->optimize.oed_Remap;

      // if has an inverse edge
      if (pedNew->optimize.oed_InverseEdge!=NULL) {
        // use the inverse edge
        pedNew = pedNew->optimize.oed_InverseEdge;
        // mark that the direction has changed
        itope->ope_Backward = !itope->ope_Backward;
      }

      // use the edge
      itope->ope_Edge = pedNew;
    }
  }}

  osc_aoedEdges.Unlock();
}

/*
 * Remove unused polygons and polygons with less than 3 edges.
 */
void CObjectSector::RemoveDummyPolygons(void)
{
  // for each polygon
  INDEX ctUsedPolygons = 0;
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // if it has more than 2 edges
    if(itopo->opo_PolygonEdges.Count()>2) {
      // mark it as used
      itopo->opo_Tag = TRUE;
      // count it
      ctUsedPolygons++;

    // if it has less than 3 edges
    } else {
      // mark it as unused
      itopo->opo_Tag = FALSE;
    }
  }}

  // create a new array of polygons
  CDynamicArray<CObjectPolygon> aopoNew;
  CObjectPolygon *popoNew = aopoNew.New(ctUsedPolygons);

  // for each polygon in sector
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // if it is used
    if (itopo->opo_Tag) {
      // add it to the new array
      *popoNew++ = itopo.Current();
    }
  }}
  // replace old array with the new one
  osc_aopoPolygons.Clear();
  osc_aopoPolygons.MoveArray(aopoNew);
}

/*
 * Remove unused and replicated elements.
 */
void CObjectSector::Optimize(void)
{
  /*
  // for each vertex
  {FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    // snap the vertex coordinates
    Snap((*itovx)(1), VTX_SNAP);
    Snap((*itovx)(2), VTX_SNAP);
    Snap((*itovx)(3), VTX_SNAP);
  }}
  // for each plane
  {FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itopl) {
    // snap the plane coordinates
    Snap((*itopl)(1), PLX_SNAP);
    Snap((*itopl)(2), PLX_SNAP);
    Snap((*itopl)(3), PLX_SNAP);
    Snap((*itopl).Distance(), PLX_SNAP);
  }}
  */

  // remove polygons with less than 3 edges
  RemoveDummyPolygons();

  // remap different vertices with same coordinates
  RemapClonedVertices();
  // remove unused vertices
  RemoveUnusedVertices();

  // find edges that have zero length and remove them from all polygons
  RemoveDummyEdgeReferences();
  // remap different edges with same or reverse vertices
  RemapClonedEdges();
  // remove unused edges
  RemoveUnusedEdges();

  // find collinear edges and cross split them with each other
  SplitCollinearEdges();
  // find edges that are used twice in same polygon and remove them from the polygon
  RemoveRedundantPolygonEdges();
  // find edges that are collinear and continuing in some polygon and join them
  JoinContinuingPolygonEdges();

  // find edges that have zero length and remove them from all polygons
  RemoveDummyEdgeReferences();
  // remap different edges with same or reverse vertices
  RemapClonedEdges();
  // remove unused edges
  RemoveUnusedEdges();
  // remove unused vertices
  RemoveUnusedVertices();

  // find collinear edges and cross split them with each other
  SplitCollinearEdges();
  // find edges that have zero length and remove them from all polygons
  RemoveDummyEdgeReferences();
  // remap different edges with same or reverse vertices
  RemapClonedEdges();

  // remove unused edges
  RemoveUnusedEdges();
  // remove unused vertices
  RemoveUnusedVertices();

  // find edges that are used twice in same polygon and remove them from the polygon
  RemoveRedundantPolygonEdges();

  // remap different planes with same coordinates
  RemapClonedPlanes();
  // remove unused planes
  RemoveUnusedPlanes();
  // remove polygons with less than 3 edges
  RemoveDummyPolygons();

  #ifndef NDEBUG
  // check the optimization algorithm
  CheckOptimizationAlgorithm();
  #endif
}

/*
 * Assignment operator.
 */
CObjectSector &CObjectSector::operator=(CObjectSector &oscOriginal)
{
  // remove current contents
  Clear();

  // copy basic properties
  osc_colColor    = oscOriginal.osc_colColor;
  osc_colAmbient  = oscOriginal.osc_colAmbient;
  osc_ulFlags[0]  = oscOriginal.osc_ulFlags[0];
  osc_ulFlags[1]  = oscOriginal.osc_ulFlags[1];
  osc_ulFlags[2]  = oscOriginal.osc_ulFlags[2];
  osc_strName     = oscOriginal.osc_strName;

  // create indices in original sector
  oscOriginal.CreateIndices();

  // copy arrays of elements from original sector
  osc_aovxVertices = oscOriginal.osc_aovxVertices;
  osc_aoplPlanes = oscOriginal.osc_aoplPlanes;
  osc_aomtMaterials = oscOriginal.osc_aomtMaterials;
  osc_aoedEdges = oscOriginal.osc_aoedEdges;
  osc_aopoPolygons = oscOriginal.osc_aopoPolygons;

  // lock all arrays
  LockAll();

  // for all edges
  FOREACHINDYNAMICARRAY(osc_aoedEdges, CObjectEdge, itoed) {
    // use vertices in this object with same index
    itoed->oed_Vertex0 = &osc_aovxVertices[itoed->oed_Vertex0->ovx_Index];
    itoed->oed_Vertex1 = &osc_aovxVertices[itoed->oed_Vertex1->ovx_Index];
  }

  // for all polygons
  FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // use plane and material in this sector with same indices
    itopo->opo_Plane = &osc_aoplPlanes[itopo->opo_Plane->opl_Index];
    if (itopo->opo_Material!=NULL) {
      itopo->opo_Material = &osc_aomtMaterials[itopo->opo_Material->omt_Index];
    }
    // for all polygon-edges in this polygon
    FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      // use edge in this sector with same index
      itope->ope_Edge = &osc_aoedEdges[itope->ope_Edge->oed_Index];
    }
  }

  // unlock all arrays
  UnlockAll();

  return *this;
}

/*
 * Find bounding box of the sector.
 */
void CObjectSector::GetBoundingBox(DOUBLEaabbox3D &boxSector)
{
  // clear the bounding box
  boxSector = DOUBLEaabbox3D();
  // for each vertex in the sector
  FOREACHINDYNAMICARRAY(osc_aovxVertices, CObjectVertex, itovx) {
    // add the vertex to the bounding box
    boxSector |= *itovx;
  }
}

/*
 * Create BSP tree for sector.
 */
void CObjectSector::CreateBSP(void)
{
  /* prepare array of bsp polygons */

  // get count of polygons in sector
  const INDEX ctPolygons = osc_aopoPolygons.Count();
  // create array of BSP polygons
  CDynamicArray<DOUBLEbsppolygon3D> arbpo;
  // create as much BSP polygons as there are polygons in this sector
  arbpo.New(ctPolygons);
  arbpo.Lock();
  osc_aopoPolygons.Lock();

  // for each polygon in this sector
  for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++) {
    CObjectPolygon    &opo = osc_aopoPolygons[iPolygon];
    DOUBLEbsppolygon3D &bpo = arbpo[iPolygon];

    // copy the plane
    (DOUBLEplane3D &)bpo = *opo.opo_Plane;
    bpo.bpo_ulPlaneTag = (size_t)opo.opo_Plane;

    // get count of edges in this polygon
    const INDEX ctEdges = opo.opo_PolygonEdges.Count();
    // create that much edges in bsp polygon
    DOUBLEbspedge3D *pbed = bpo.bpo_abedPolygonEdges.New(ctEdges);

    opo.opo_PolygonEdges.Lock();
    // for each edge in this polygon
    for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
      CObjectPolygonEdge &ope = opo.opo_PolygonEdges[iEdge];
      CObjectEdge &oed = *ope.ope_Edge;
      // if the edge is reversed
      if(ope.ope_Backward) {
        // add bsp edge with reversed vertices
        pbed[iEdge] = DOUBLEbspedge3D(*oed.oed_Vertex1, *oed.oed_Vertex0, (size_t)&oed);
      // otherwise
      } else {
        // add normal bsp edge
        pbed[iEdge] = DOUBLEbspedge3D(*oed.oed_Vertex0, *oed.oed_Vertex1, (size_t)&oed);
      }
    }
    opo.opo_PolygonEdges.Unlock();
  }
  arbpo.Unlock();
  osc_aopoPolygons.Unlock();

  /* create bsp tree from array of bsp polygons */
  osc_BSPTree.Create(arbpo);
}

/*
 * Turn sector inside-out.
 */
void CObjectSector::Inverse(void)
{
  // for all planes
  FOREACHINDYNAMICARRAY(osc_aoplPlanes, CObjectPlane, itopl) {
    // flip the plane
    (DOUBLEplane3D &)*itopl = -(DOUBLEplane3D &)*itopl;
  }

  // for all polygons
  FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    // for all polygon-edges in this polygon
    FOREACHINDYNAMICARRAY(itopo->opo_PolygonEdges, CObjectPolygonEdge, itope) {
      // reverse the polygon edge direction
      itope->ope_Backward = !itope->ope_Backward;
    }
  }
}

/* Recalculate all planes from vertices. (used when stretching vertices) */
void CObjectSector::RecalculatePlanes(void)
{
  // create a container for empty polygons
  CDynamicContainer<CObjectPolygon> copoEmpty;

  // for all polygons
  {FOREACHINDYNAMICARRAY(osc_aopoPolygons, CObjectPolygon, itopo) {
    CObjectPolygon &opo = *itopo;
    // clear plane normal
    DOUBLE3D vNormal = DOUBLE3D(0.0f,0.0f,0.0f);

    // for all edges in polygon
    INDEX ctVertices = opo.opo_PolygonEdges.Count();
    opo.opo_PolygonEdges.Lock();
    {for(INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
		  // get its vertices in counterclockwise order
      CObjectPolygonEdge &ope = opo.opo_PolygonEdges[iVertex];
      CObjectVertex *povx0, *povx1;
      if (ope.ope_Backward) {
        povx0 = ope.ope_Edge->oed_Vertex1;
        povx1 = ope.ope_Edge->oed_Vertex0;
      } else {
        povx0 = ope.ope_Edge->oed_Vertex0;
        povx1 = ope.ope_Edge->oed_Vertex1;
      }
      DOUBLE3D vSum = *povx0+*povx1;
      DOUBLE3D vDif = *povx0-*povx1;
		  // add the edge contribution to the normal vector
      vNormal(1) += vDif(2)*vSum(3);
      vNormal(2) += vDif(3)*vSum(1);
      vNormal(3) += vDif(1)*vSum(2);
    }}

    // if the polygon area is too small
    if (vNormal.Length()<1E-8) {
      // mark it for removal
      copoEmpty.Add(&opo);
    // if the polygon area is ok
    } else {
      // add one plane to planes array
      CObjectPlane *pplPlane = osc_aoplPlanes.New();
      // construct this plane from normal vector and one point
	    *pplPlane= DOUBLEplane3D(vNormal, *opo.opo_PolygonEdges[0].ope_Edge->oed_Vertex0);
      opo.opo_Plane = pplPlane;
    }

    opo.opo_PolygonEdges.Unlock();
  }}

  // for all empty polygons
  {FOREACHINDYNAMICCONTAINER(copoEmpty, CObjectPolygon, itopoEmpty) {
    // delete the polygon from sector
    osc_aopoPolygons.Delete(itopoEmpty);
  }}
}
