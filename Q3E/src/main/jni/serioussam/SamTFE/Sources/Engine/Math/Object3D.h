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

#ifndef SE_INCL_OBJECT3D_H
#define SE_INCL_OBJECT3D_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/TextureMapping.h>
#include <Engine/Math/Projection.h>
#include <Engine/Templates/BSP.h>

// a vertex in 3d object
class CObjectVertex : public DOUBLE3D {
public:
  CObjectVertex *ovx_Remap;         // pointer for index remapping
  ULONG ovx_Tag;                    // tag for format-specific data
  INDEX ovx_Index;                  // index for easier conversions
  // flags 0-7 are used by CObjectVertex, flags 8-31 are reserved for custom formats
  ULONG ovx_ulFlags;                // flags

  /* Default constructor. */
  inline CObjectVertex(void) { ovx_ulFlags = 0;};
  /* Constructor from vector. */
  inline CObjectVertex(const DOUBLE3D &v) : DOUBLE3D(v) { ovx_ulFlags = 0;};
  /* Clear the object. */
  inline void Clear(void) {};
};

// a plane in 3d object
class CObjectPlane : public DOUBLEplane3D {
public:
  CObjectPlane *opl_Remap;          // pointer for index remapping
  ULONG opl_Tag;                    // tag for format-specific data
  INDEX opl_Index;                  // index for easier conversions

  /* Default constructor. */
  inline CObjectPlane(void) {};
  /* Constructor from plane. */
  inline CObjectPlane(const DOUBLEplane3D &pl) : DOUBLEplane3D(pl) {};
  /* Clear the object. */
  inline void Clear(void) {};
};

/* rcg10042001 named anonymous structs. */
// an edge in 3d object
class CObjectEdge {
public:
  ULONG oed_Tag;                  // tag for format-specific data
  INDEX oed_Index;                // index for easier conversions
  union {
    struct {  // used in optimization
      CObjectEdge *oed_Remap;         // pointer for index remapping
      CObjectEdge *oed_InverseEdge;   // pointer to inverse edge, if exists
    } optimize;
    struct {  // used in splitting collinear edges
      CObjectEdge *oed_FirstChild;    // pointer to first part of split edge
      CObjectEdge *oed_NextSibling;   // pointer to next part of split edge
    } colinear1;
    struct {  // used in splitting collinear edges
      class CEdgeEx *oed_pedxLine;          // pointer to line information
    } colinear2;
  };

  CObjectVertex *oed_Vertex0;     // start vertex
  CObjectVertex *oed_Vertex1;     // end vertex

  /* Default constructor. */
  inline CObjectEdge(void) {};
  /* Constructor from vertex references. */
  inline CObjectEdge(CObjectVertex &ovxVertex0, CObjectVertex &ovxVertex1)
    : oed_Vertex0(&ovxVertex0), oed_Vertex1(&ovxVertex1) {};
  /* Clear the object. */
  inline void Clear(void) {};
};

// a material in 3d object
class CObjectMaterial {
public:
  ULONG omt_Tag;                  // tag for format-specific data
  INDEX omt_Index;                // index for easier conversions
  CTString omt_Name;              // name of this material
  CTString omt_strName2;
  CTString omt_strName3;
  COLOR omt_Color;                // color of this material (surface)

  /* Default constructor. */
  inline CObjectMaterial(void) : omt_Name("<unnamed>") {};
  /* Constructor from string. */
  inline CObjectMaterial(const CTString &strName) : omt_Name(strName) {};
  /* Copy constructor. */
  inline CObjectMaterial(const CObjectMaterial &omt) {
    omt_Name = omt.omt_Name;
    omt_strName2 = omt.omt_strName2;
    omt_strName3 = omt.omt_strName3;
    omt_Color = omt.omt_Color;
  };
  /* Destructor. */
  inline ~CObjectMaterial(void) {};
  /* Clear the object. */
  inline void Clear(void) {
    omt_Name.Clear();
    omt_strName2.Clear();
    omt_strName3.Clear();
  };

  /* Assignment. */
  inline const CObjectMaterial &operator=(const CObjectMaterial &omt) {
    omt_Name = omt.omt_Name;
    omt_strName2 = omt.omt_strName2;
    omt_strName3 = omt.omt_strName3;
    omt_Color = omt.omt_Color;
    return *this;
  };
  /* Color assignment. */
  inline void SetColor(const COLOR &clrColor) { omt_Color = clrColor; };
};

// a reference to edge in polygon
class ENGINE_API CObjectPolygonEdge {
public:
  CObjectEdge *ope_Edge;          // pointer to the edge
  BOOL ope_Backward;              // true if vertex0 and vertex1 must be swapped

  /* Default constructor. */
  inline CObjectPolygonEdge(void) : ope_Backward(FALSE) {};
  /* Constructor from edge pointer. */
  inline CObjectPolygonEdge(CObjectEdge *poed) : ope_Edge(poed), ope_Backward(FALSE) {};
  /* Constructor from edge pointer and reverse marker. */
  inline CObjectPolygonEdge(CObjectEdge *poed, BOOL bReverse)
    : ope_Edge(poed), ope_Backward(bReverse) {};
  /* Clear the object. */
  inline void Clear(void) {};
  /* Get start and end vertices. */
  inline void GetVertices(CObjectVertex *&povxStart, CObjectVertex *&povxEnd)
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
};

// a polygon in 3d object
// Flags
// flags 0-2 are used by CObjectPolygon, flags 3-31 are reserved for custom formats
#define OPOF_PORTAL       (1L<<0)         // set if the polygon is portal
#define OPOF_IGNOREDBYCSG (1L<<1)         // set if the polygon is ignored when doing CSG

class ENGINE_API CObjectPolygon {
public:
  ULONG opo_Tag;                                  // tag for format-specific data
  INDEX opo_Index;                                // index for easier conversions

#define OPO_MAXUSERDATA 64
  UBYTE opo_ubUserData[OPO_MAXUSERDATA];          // reserved space for other data

  CObjectPlane *opo_Plane;                        // plane of this polygon
  CDynamicArray<CObjectPolygonEdge> opo_PolygonEdges;  // edges in this polygon
  CObjectMaterial *opo_Material;                  // material of this polygon
  CMappingDefinition opo_amdMappings[4];          // mapping of textures on this polygon
  ULONG opo_ulFlags;                              // various flags
  COLOR opo_colorColor;                           // color of this polygon
  void *opo_pvOriginal;                           // used for format conversions

  /* Default constructor. */
  inline CObjectPolygon(void) : opo_Material(NULL), opo_ulFlags(0), opo_pvOriginal(NULL)
    { memset(&opo_ubUserData, 0, sizeof(opo_ubUserData)); };
  /* Clear the object. */
  inline void Clear(void) { opo_PolygonEdges.Clear(); };
  /* Join polygon edges that are collinear and continuing. */
  void JoinContinuingEdges(CDynamicArray<CObjectEdge> &oedEdges);
  /* Remove polygon edges that are used twice from a polygon. */
  void RemoveRedundantEdges(void);
  /* Remove polygon edges that are marked as unused (oed_Edge==NULL) from polygon. */
  void RemoveMarkedEdges(INDEX ctNonMarkedEdges);
  /* Remove polygon edges that have zero length from a polygon. */
  void RemoveDummyEdgeReferences(void);
};

// a sector in 3d object
class CObjectSector {
public:

  /* Create a new edge with two coordinates. */
  inline CObjectEdge &CreateEdge(INDEX ivx0, INDEX ivx1) {
    // lock vertex array
    osc_aovxVertices.Lock();
    // create edge
    CObjectEdge &oeResult =
      *osc_aoedEdges.New(1) = CObjectEdge(osc_aovxVertices[ivx0], osc_aovxVertices[ivx1]);
    // unlock vertex array
    osc_aovxVertices.Unlock();
    return oeResult;
  }

  /* Create a new edge with two coordinates. */
  inline CObjectEdge &CreateEdge(const DOUBLE3D &vx0, const DOUBLE3D &vx1) {
    // create end vertices for the part
    CObjectVertex *aovxs = osc_aovxVertices.New(2);
    aovxs[0] = vx0;
    aovxs[1] = vx1;
    // create edge
    return *osc_aoedEdges.New(1) = CObjectEdge(aovxs[0], aovxs[1]);
  }

  /* Create an edge and add it to a polygon in this sector. */
  inline void CreateEdgeInPolygon(CObjectPolygon &opoPolygon,
    const DOUBLE3D &vVertex0, const DOUBLE3D &vVertex1) {
    *opoPolygon.opo_PolygonEdges.New(1) =
      CObjectPolygonEdge( &CreateEdge(vVertex0, vVertex1));
  };

  /* Check the optimization algorithm. */
  void CheckOptimizationAlgorithm(void);
  /* See if all polygons in sector are valid (planar). */
  BOOL ArePolygonsPlanar(void);
  /* Create indices for all members of all arrays. */
  void CreateIndices(void);
  /* Create BSP tree for sector. */
  void CreateBSP(void);

  // optimization functions
  /* Remap different vertices with same coordinates to use only one of each. */
  void RemapClonedVertices(void);
  /* Remap different edges with same or reverse vertices to use only one of each. */
  void RemapClonedEdges(void);
  /* Remap different planes with same coordinates to use only one of each. */
  void RemapClonedPlanes(void);

  /* Remove vertices that are not used by any edge. */
  void RemoveUnusedVertices(void);
  /* Remove edges that are not used by any polygon. */
  void RemoveUnusedEdges(void);
  /* Remove planes that are not used by any polygon. */
  void RemoveUnusedPlanes(void);

  /* Create array of extended edge infos. */
  void CreateEdgeLines(CStaticArray<CEdgeEx> &aedxEdgeLines,
    CStaticArray<CEdgeEx *> &apedxSortedEdgeLines, INDEX &ctEdges);
  /* Join polygon edges that are collinear and continuing. */
  void JoinContinuingPolygonEdges(void);
  /* Find collinear edges and cross-split them with each other. */
  void SplitCollinearEdges(void);
  /* Split a run of collinear edges. */
  void SplitCollinearEdgesRun(CStaticArray<CEdgeEx *> &apedxSortedEdgeLines,
    INDEX iFirstInRun, INDEX iLastInRun);
  /* Split an edge with a run of vertices. */
  inline void SplitEdgeWithVertices(CObjectEdge &oedOriginal,
    CStaticArray<CObjectVertex *> &apvxVertices);
  /* Remove polygon edges that are used twice from all polygons. */
  void RemoveRedundantPolygonEdges(void);

  /* Remove polygons with less than 3 edges. */
  void RemoveDummyPolygons(void);
  /* Find edges that have zero length and remove them from all polygons. */
  void RemoveDummyEdgeReferences(void);

  /* Remove unused and replicated elements. */
  void Optimize(void);

  /* Turn sector inside-out. */
  void Inverse(void);
  /* Recalculate all planes from vertices. (used when stretching vertices) */
  void RecalculatePlanes(void);

public:
  CDynamicArray<CObjectVertex> osc_aovxVertices;      // vertices
  CDynamicArray<CObjectPlane> osc_aoplPlanes;         // planes
  CDynamicArray<CObjectMaterial> osc_aomtMaterials;   // material info
  CDynamicArray<CObjectEdge> osc_aoedEdges;           // edges
  CDynamicArray<CObjectPolygon> osc_aopoPolygons;     // polygons

  ULONG osc_Tag;                                      // tag for format-specific data
  INDEX osc_Index;                                    // index for easier conversions
  COLOR osc_colColor;                                 // color of this sector
  COLOR osc_colAmbient;                               // ambient lightning of this sector
  ULONG osc_ulFlags[3];                               // flags (not used in CTMath)

  CTString osc_strName;     // name of the sector

  DOUBLEbsptree3D osc_BSPTree;                        // BSP tree of this sector

  /* Default constructor. */
  ENGINE_API CObjectSector(void);
  /* Destructor. */
  ENGINE_API ~CObjectSector(void);

  /* Clear the object. */
  void Clear(void);
  /* Assignment operator. */
  ENGINE_API CObjectSector &operator=(CObjectSector &oscOriginal);

  /* Locks all object sector dynamic arrays */
  void LockAll( void);
  /* Unlocks all object sector dynamic arrays */
  void UnlockAll( void);

  /* Create a new polygon in given sector. */
  ENGINE_API CObjectPolygon *CreatePolygon(INDEX ctVertices, INDEX aivVertices[],
    CObjectMaterial &omaMaterial, ULONG ulFlags, BOOL bReverse);
  /* Create a new polygon in given sector. */
  ENGINE_API CObjectPolygon *CreatePolygon(INDEX ctVertices, DOUBLE3D avVertices[],
    CObjectMaterial &omaMaterial, ULONG ulFlags, BOOL bReverse);
  /* Find bounding box of the sector. */
  void GetBoundingBox(DOUBLEaabbox3D &boxSector);
};

// a local class that holds extended edge information for finding collinear edges.
class CEdgeEx {
public:
  CObjectEdge *edx_poedEdge;    // the original edge in object
  // constants of the line that the edge is on
  DOUBLE3D edx_vDirection;       // normalized direction vector of the line
  DOUBLE3D edx_vReferencePoint;  // reference point on the line
  BOOL edx_bReverse;            // set if the edge direction is opposite to the line direction

  /* Initialize the structure for the given edge. */
  inline void Initialize(CObjectEdge *poedEdge);
  inline void Initialize(const DOUBLE3D *pvPoint0, const DOUBLE3D *pvPoint1);
  /* Test if a point is on the edge line. */
  inline BOOL PointIsOnLine(const DOUBLE3D &vPoint) const;
};

/*
 * A mathematical format of 3d object used for importing/exporting, CSG etc.
 */
class ENGINE_API CObject3D {
public:
  enum LoadType {
    LT_NORMAL = 0,
    LT_OPENED,
    LT_UNWRAPPED,
  };

  /* Remove sectors with no polygons. */
  void RemoveEmptySectors(void);

  /* Create indices of all sectors (doesn't create vertex indices etc.). */
  void CreateSectorIndices(void);

public:
  CDynamicArray<CObjectSector> ob_aoscSectors;         // sectors

  /* Default constructor. */
  CObject3D(void);
  /* Destructor. */
  ~CObject3D(void);
  /* Clear all arrays. */
  void Clear(void);

  /* Recognize and load any of supported 3D file formats. */
	void LoadAny3DFormat_t( const CTFileName &FileName, const FLOATmatrix3D &mTransform, enum LoadType ltLoadType=LT_NORMAL); // throw (char *)
  // start/end batch loading of 3d objects
  static void BatchLoading_t(BOOL bOn);
  /* Convert from intermediate structures into O3D */
  void ConvertArraysToO3D( void);

  /* Save in LightWave format. */
	void SaveLWO_t( const CTFileName &FileName); // throw (char *)
  /* Save in 3DStudio format. */
	void SaveDXF_t( const CTFileName &FileName); // throw (char *)

  /* Remove unused and replicated elements. */
  void Optimize(void);
  /* See if all polygons in object 3D are valid (planar). */
  BOOL ArePolygonsPlanar(void);
  /* Create BSP trees for all sectors. */
  void CreateSectorBSPs(void);

  /* Project the whole object into some other space. */
  void Project(CSimpleProjection3D_DOUBLE &pr);
  /* Assignment operator. */
  CObject3D &operator=(CObject3D &obOriginal);
  /* Turn all sectors in object inside-out. (not recommended for multi sector objects) */
  void Inverse(void);
  /* Recalculate all planes from vertices. (used when stretching vertices) */
  void RecalculatePlanes(void);

  /**** CSG operations -- they all destroy both operands! ****/
  /* Add rooms. */
  void CSGAddRooms(CObject3D &obA, CObject3D &obB);
  /* Add material from object B to object A. (B should have only one
    open sector and no closed sectors) */
  void CSGAddMaterial(CObject3D &obA, CObject3D &obB);
  void CSGAddMaterialReverse(CObject3D &obA, CObject3D &obB);
  /* Remove material of object B from object A. (B should have only one
    open sector and no closed sectors) */
  void CSGRemoveMaterial(CObject3D &obA, CObject3D &obB);
  /* Split sectors of object A using object B. (B should have only one
    closed sector and no open sectors) */
  void CSGSplitSectors(CObject3D &obA, CObject3D &obB);
  /* Join sectors of object A with sectors of object B. (both A and B should
    have only one sector) */
  void CSGJoinSectors(CObject3D &obA, CObject3D &obB);
  /* Split polygons of object A with sectors of object B. (both A and B should
    have only one sector) */
  void CSGSplitPolygons(CObject3D &obA, CObject3D &obB);
  /* Turn all portals to walls. */
  void TurnPortalsToWalls(void);

  /* Find bounding box of the object. */
  void GetBoundingBox(DOUBLEaabbox3D &boxObject);

  /* Dump the object 3D to debug window. */
  void DebugDump(void);
};


#endif  /* include-once check. */

