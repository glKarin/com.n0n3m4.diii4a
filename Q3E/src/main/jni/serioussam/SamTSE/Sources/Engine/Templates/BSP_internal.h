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

#ifndef SE_INCL_BSP_INTERNAL_H
#define SE_INCL_BSP_INTERNAL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

extern FLOAT mth_fCSGEpsilon;

#ifdef __arm__
#define SPHERE_HACK
#endif

/*
 * Type used to identify BSP-node locations
 */
enum BSPNodeLocation {
  BNL_ILLEGAL=0,    // for illegal value
  BNL_INSIDE,       // inside leaf node
  BNL_OUTSIDE,      // outside leaf node
  BNL_BRANCH,       // branch node, unspecified location
};

/*
 * Template class for BSP vertex
 */
template<class Type, int iDimensions>
class BSPVertex : public Vector<Type, iDimensions> {
public:
  /* Default constructor. */
  inline BSPVertex(void) {};

  /* Assignment operator with coordinates only. */
  inline BSPVertex<Type, iDimensions> &operator=(const Vector<Type, iDimensions> &vCoordinates);
};

/*
 * Template class for BSP vertex container
 */
template<class Type, int iDimensions>
class BSPVertexContainer {
public:
  INDEX bvc_iMaxAxis;                     // index of largest axis of direction
  Type  bvc_tMaxAxisSign;                 // sign of largest axis of direction

  CStaticStackArray<BSPVertex<Type, iDimensions> > bvc_aVertices;  // array of vertices
public:
  Vector<Type, iDimensions> bvc_vDirection;                  // direction of the split line

  /* Default constructor. */
  BSPVertexContainer(void);

  /* Initialize for a direction. */
  void Initialize(const Vector<Type, iDimensions> &vDirection);
  /* Uninitialize. */
  void Uninitialize(void);

  /* Check if this container is in an unusable state (polygon coplanar with the splitter).*/
  inline BOOL IsPlannar(void) { return bvc_iMaxAxis==0; };

  /* Add a new vertex. */
  inline void AddVertex(const Vector<Type, iDimensions> &vPoint);

  /* Sort vertices in this container along the largest axis of container direction. */
  void Sort(void);
  /* Elliminate paired vertices. */
  void ElliminatePairedVertices(void);
  /* Create edges from vertices in one container -- must be sorted before. */
  void CreateEdges(CDynamicArray<BSPEdge<Type, iDimensions> > &abedAll, size_t ulEdgeTag);
};

/*
 * Template class for BSP edge
 */
template<class Type, int iDimensions>
class BSPEdge {
public:
  Vector<Type, iDimensions> bed_vVertex0;  // edge vertices
  Vector<Type, iDimensions> bed_vVertex1;
  size_t bed_ulEdgeTag;   // tags for BSPs with tagged edges/planes

  /* Default constructor. */
  inline BSPEdge(void) {};
  /* Constructor with two vectors. */
  inline BSPEdge(const Vector<Type, iDimensions> &vVertex0, const Vector<Type, iDimensions> &vVertex1, size_t ulTag)
    : bed_vVertex0(vVertex0), bed_vVertex1(vVertex1), bed_ulEdgeTag(ulTag) {}

  /* Clear the object. */
  inline void Clear(void) {};
  // remove all edges marked for removal
  static void RemoveMarkedBSPEdges(CDynamicArray<BSPEdge<Type, iDimensions> > &abed);
  // optimize a polygon made out of BSP edges using tag information
  static void OptimizeBSPEdges(CDynamicArray<BSPEdge<Type, iDimensions> > &abed);
};

/*
 * Template class for polygons used in creating BSP-trees
 */
template<class Type, int iDimensions>
class BSPPolygon : public Plane<Type, iDimensions> {
public:
  CDynamicArray<BSPEdge<Type, iDimensions> > bpo_abedPolygonEdges;  // array of edges in the polygon
  size_t bpo_ulPlaneTag;         // tags for BSPs with tagged planes (-1 for no tag)

  /* Add an edge to the polygon. */
  inline void AddEdge(const Vector<Type, iDimensions> &vPoint0, const Vector<Type, iDimensions> &vPoint1, size_t ulTag);

  /* Default constructor. */
  inline BSPPolygon(void) : bpo_ulPlaneTag(-1) {};
  /* Constructor with array of edges and plane. */
  inline BSPPolygon(
    Plane<Type, iDimensions> &plPlane, CDynamicArray<BSPEdge<Type, iDimensions> > abedPolygonEdges, size_t ulPlaneTag)
    : Plane<Type, iDimensions>(plPlane)
    , bpo_abedPolygonEdges(abedPolygonEdges)
    , bpo_ulPlaneTag(ulPlaneTag)
    {};

  /* Clear the object. */
  inline void Clear(void) {bpo_abedPolygonEdges.Clear();};
};

template<class Type, int iDimensions>
class BSPLine {
public:
  Type bl_tMin;
  Type bl_tMax;
};

/*
 * Template class for BSP-tree node of arbitrary dimensions and arbitrary type of members
 */
template<class Type, int iDimensions>
class BSPNode : public Plane<Type, iDimensions> {  // split plane
public:
  enum BSPNodeLocation bn_bnlLocation;    // location of bsp node

  BSPNode<Type, iDimensions> *bn_pbnFront;        // pointer to child node in front of split plane
  BSPNode<Type, iDimensions> *bn_pbnBack;         // pointer to child node behind split plane
  size_t bn_ulPlaneTag;         // tags for BSPs with tagged planes (-1 for no tag)

public:
  /* Defualt constructor (for arrays only). */
  inline BSPNode(void) {};
  /* Constructor for a leaf node. */
  inline BSPNode(enum BSPNodeLocation bnl);
  /* Constructor for a branch node. */
  inline BSPNode(const Plane<Type, iDimensions> &plSplitPlane, size_t ulPlaneTag,
    BSPNode<Type, iDimensions> &bnFront, BSPNode<Type, iDimensions> &bnBack);
  /* Constructor for cloning a bsp (sub)tree. */
  BSPNode(BSPNode<Type, iDimensions> &bnRoot);
  /* Recursive destructor. */
  void DeleteBSPNodeRecursively(void);

  // find minimum/maximum parameters of points on a line that are inside - recursive
  void FindLineMinMax(BSPLine<Type, iDimensions> &bl, 
    const Vector<Type, iDimensions> &v0,
    const Vector<Type, iDimensions> &v1,
    Type t0, Type t1);

  /* Test if a sphere is inside, outside, or intersecting. (Just a trivial rejection test) */
  FLOAT TestSphere(const Vector<Type, iDimensions> &vSphereCenter, Type tSphereRadius) const;
  #ifdef SPHERE_HACK
  int   TestSphere_hack(const FLOAT *params) const;
  #endif
  /* Test if a box is inside, outside, or intersecting. (Just a trivial rejection test) */
  FLOAT TestBox(const OBBox<Type> &box) const;
};

/*
 * Template class that performs polygon cuts using BSP-tree
 */
template<class Type, int iDimensions>
class BSPCutter {
public:
  /* Split an edge with a plane. */
  static inline void SplitEdge(const Vector<Type, iDimensions> &vPoint0, const Vector<Type, iDimensions> &vPoint1, size_t ulEdgeTag,
    const Plane<Type, iDimensions> &plSplitPlane,
    BSPPolygon<Type, iDimensions> &abedFront, BSPPolygon<Type, iDimensions> &abedBack,
    BSPVertexContainer<Type, iDimensions> &bvcFront, BSPVertexContainer<Type, iDimensions> &bvcBack);

  /* Cut a polygon with a BSP tree. */
  void CutPolygon(BSPPolygon<Type, iDimensions> &bpoPolygon, BSPNode<Type, iDimensions> &bn);

public:
  CDynamicArray<BSPEdge<Type, iDimensions> > bc_abedInside;       // edges of inside part of polygon
  CDynamicArray<BSPEdge<Type, iDimensions> > bc_abedOutside;      // edges of outside part of polygon
  CDynamicArray<BSPEdge<Type, iDimensions> > bc_abedBorderInside; // edges of border part of polygon facing inwards
  CDynamicArray<BSPEdge<Type, iDimensions> > bc_abedBorderOutside;// edges of border part of polygon facing outwards

  /* Split a polygon with a plane. */
  static inline BOOL SplitPolygon(BSPPolygon<Type, iDimensions> &bpoPolygon, const Plane<Type, iDimensions> &plPlane, size_t ulPlaneTag,
    BSPPolygon<Type, iDimensions> &bpoFront, BSPPolygon<Type, iDimensions> &bpoBack);

  /* Constructor for splitting a polygon with a BSP tree. */
  BSPCutter(BSPPolygon<Type, iDimensions> &bpoPolygon, BSPNode<Type, iDimensions> &bnRoot);
  /* Destructor. */
  ~BSPCutter(void);
};


#endif  /* include-once check. */

