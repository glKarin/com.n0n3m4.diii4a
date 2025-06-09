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

#include <Engine/Templates/BSP.h>
#include <Engine/Templates/BSP_internal.h>

#include <Engine/Base/Stream.h>
#include <Engine/Base/CRC.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/OBBox.h>
#include <Engine/Math/Functions.h>

#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>


// epsilon value used for BSP cutting
//#define BSP_EPSILON ((Type) 0.015625)       // 1/2^6 ~= 1.5 cm
#define BSP_EPSILON Type((1.0/65536.0)*4*mth_fCSGEpsilon) // 1/2^16
//#define BSP_EPSILON Type(0.00390625) // 1/2^8
//#define EPSILON (1.0f/8388608.0f) // 1/2^23
//#define EPSILON 0.0009765625f // 1/2^10
//#define EPSILON 0.03125f    // 1/2^5
//#define EPSILON 0.00390625f // 1/2^8

template <class Type>
inline BOOL EpsilonEq(const Type &a, const Type &b) { return Abs(a-b)<=BSP_EPSILON; };
template <class Type>
inline BOOL EpsilonNe(const Type &a, const Type &b) { return Abs(a-b)> BSP_EPSILON; };

/////////////////////////////////////////////////////////////////////
// BSP vertex

/*
 * Assignment operator with coordinates only.
 */
template<class Type, int iDimensions>
BSPVertex<Type, iDimensions> &BSPVertex<Type, iDimensions>::operator=(const Vector<Type, iDimensions> &vCoordinates)
{
  *(Vector<Type, iDimensions> *)this = vCoordinates;
  return *this;
}

/////////////////////////////////////////////////////////////////////
// BSP vertex container

/*
 * Default constructor.
 */
template<class Type, int iDimensions>
BSPVertexContainer<Type, iDimensions>::BSPVertexContainer(void)
{
}

template<class Type, int iDimensions>
void BSPVertexContainer<Type, iDimensions>::AddVertex(const Vector<Type, iDimensions> &vPoint)
{
  bvc_aVertices.Push() = vPoint;
}

/*
 * Initialize for a direction.
 */
template<class Type, int iDimensions>
void BSPVertexContainer<Type, iDimensions>::Initialize(const Vector<Type, iDimensions> &vDirection)
{
  bvc_vDirection = vDirection;

  // init array of vertices
  bvc_aVertices.SetAllocationStep(32);

  // find largest axis of direction vector
  INDEX iMaxAxis = 0;
  Type tMaxAxis = (Type)0;//vDirection(1);
  for( INDEX iAxis=1; iAxis<=iDimensions; iAxis++) {
    if( Abs(vDirection(iAxis)) > Abs(tMaxAxis) ) {
      tMaxAxis = vDirection(iAxis);
      iMaxAxis = iAxis;
    }
  }

/* This assert would seem natural here, but it is not possible because of parallel planes!
    // must be greater or equal than minimal max axis of any normalized vector in that space
    ASSERT( Abs(tMaxAxis) > (1.0/sqrt(double(iDimensions))-0.01) );
*/

  // remember that axis index and sign for sorting
  bvc_iMaxAxis = iMaxAxis;
  bvc_tMaxAxisSign = Sgn(tMaxAxis);
}

/*
 * Unnitialize.
 */
template<class Type, int iDimensions>
void BSPVertexContainer<Type, iDimensions>::Uninitialize(void)
{
  // delete array of vertices
  bvc_aVertices.Delete();
  // destroy axis index and sign
  bvc_iMaxAxis = -1;
  bvc_tMaxAxisSign = (Type)0;
}

static INDEX qsort_iCompareAxis;

template<class Type, int iDimensions>
class CVertexComparator {
public:
  /*
   * Compare two vertices.
   */
  static inline int CompareVertices(const Vector<Type, iDimensions> &vx0, const Vector<Type, iDimensions> &vx1, INDEX iAxis)
  {
         if (vx0(iAxis)<vx1(iAxis)) return -1;
    else if (vx0(iAxis)>vx1(iAxis)) return 1;
    else                            return 0;
  }

  /*
   * Compare two vertices for quick-sort.
   */
  static int qsort_CompareVertices_plus( const void *pvVertex0, const void *pvVertex1)
  {
    BSPVertex<Type, iDimensions> &vx0 = *(BSPVertex<Type, iDimensions> *)pvVertex0;
    BSPVertex<Type, iDimensions> &vx1 = *(BSPVertex<Type, iDimensions> *)pvVertex1;
    return +CompareVertices(vx0, vx1, qsort_iCompareAxis);
  }
  static int qsort_CompareVertices_minus( const void *pvVertex0, const void *pvVertex1)
  {
    BSPVertex<Type, iDimensions> &vx0 = *(BSPVertex<Type, iDimensions> *)pvVertex0;
    BSPVertex<Type, iDimensions> &vx1 = *(BSPVertex<Type, iDimensions> *)pvVertex1;
    return -CompareVertices(vx0, vx1, qsort_iCompareAxis);
  }
};
/*
 * Sort vertices in this container along the largest axis of container direction.
 */
template<class Type, int iDimensions>
void BSPVertexContainer<Type, iDimensions>::Sort(void)
{
  // if there are no vertices, or the container is not line
  if (bvc_aVertices.Count()==0 || IsPlannar()) {
    // do not attempt to sort
    return;
  }

  // sort by max. axis
  qsort_iCompareAxis = bvc_iMaxAxis;

  // if the sign of axis is positive
  if (bvc_tMaxAxisSign>0) {
    // sort them normally
    if (bvc_aVertices.Count()>0) {
      qsort(&bvc_aVertices[0], bvc_aVertices.Count(), sizeof(BSPVertex<Type, iDimensions>),
        CVertexComparator<Type, iDimensions>::qsort_CompareVertices_plus);
    }
  // if it is negative
  } else {
    // sort them inversely
    if (bvc_aVertices.Count()>0) {
      qsort(&bvc_aVertices[0], bvc_aVertices.Count(), sizeof(BSPVertex<Type, iDimensions>),
          CVertexComparator<Type, iDimensions>::qsort_CompareVertices_minus);
    }
  }
}

/*
 * Elliminate paired vertices.
 */
template<class Type, int iDimensions>
void BSPVertexContainer<Type, iDimensions>::ElliminatePairedVertices(void)
{
  // FIXME: DG: am I missing something or is this function not actually changing anything?

  // if there are no vertices, or the container is not line
  if (bvc_aVertices.Count()==0 || IsPlannar()) {
    // do not attempt to sort
    return;
  }

  // initially, last vertices are far away
  Type tLastInside;  tLastInside  = (Type)32000;
  BSPVertex<Type, iDimensions> *pbvxLastInside  = NULL;

  // for all vertices in container
  for (INDEX iVertex=0; iVertex<bvc_aVertices.Count(); iVertex++) {
    BSPVertex<Type, iDimensions> &bvx = bvc_aVertices[iVertex];    // reference to this vertex
    Type t = bvx(bvc_iMaxAxis);                 // coordinate along max. axis

    // if last inside vertex is next to this one
    if ( EpsilonEq(t, tLastInside) ) {
      // last vertex is far away
      tLastInside  = (Type)32000;
      IFDEBUG(pbvxLastInside = NULL);

    // otherwise
    } else {
      // make this last inside vertex
      tLastInside = t;
      pbvxLastInside = &bvx;
    }
  }
}

/*
 * Create edges from vertices in one container -- must be sorted before.
 */
template<class Type, int iDimensions>
void BSPVertexContainer<Type, iDimensions>::CreateEdges(CDynamicArray<BSPEdge<Type, iDimensions> > &abed, size_t ulEdgeTag)
{
  // if there are no vertices, or the container is not line
  if (bvc_aVertices.Count()==0 || IsPlannar()) {
    // do not attempt to sort
    return;
  }

  // initially, edge is inactive
  BOOL bActive = FALSE;
  BSPEdge<Type, iDimensions> *pbed = NULL;

  // for all vertices in container
  for (INDEX iVertex=0; iVertex<bvc_aVertices.Count(); iVertex++) {
    BSPVertex<Type, iDimensions> &bvx = bvc_aVertices[iVertex];    // reference to this vertex

    // if edge is inactive
    if (!bActive) {
      // create new edge
      pbed = abed.New();
      pbed->bed_ulEdgeTag = ulEdgeTag;
      // set start vertex
      pbed->bed_vVertex0 = bvx;
    } else {
      // set end vertex
      pbed->bed_vVertex1 = bvx;
      // trash edge pointer
      IFDEBUG(pbed = NULL);
    }
    // toggle edge
    bActive = !bActive;
  }
}

/////////////////////////////////////////////////////////////////////
// BSP edge

// remove all edges marked for removal
template<class Type, int iDimensions>
void BSPEdge<Type, iDimensions>::RemoveMarkedBSPEdges(CDynamicArray<BSPEdge<Type, iDimensions> > &abed)
{
  typedef BSPEdge<Type, iDimensions> edge_t; // local declaration, to fix macro expansion in FOREACHINDYNAMICARRAY
  // conut edges left
  INDEX ctEdgesLeft = 0;
  {FOREACHINDYNAMICARRAY(abed, edge_t, itbed) {
    if (itbed->bed_ulEdgeTag != 0) {
      ctEdgesLeft++;
    }
  }}
  // make a copy of array without removed edges
  CDynamicArray<BSPEdge<Type, iDimensions> > abed2;
  abed2.New(ctEdgesLeft);
  abed2.Lock();
  INDEX iedNew = 0;
  {FOREACHINDYNAMICARRAY(abed, edge_t, itbed) {
    edge_t &bed = *itbed;
    if (bed.bed_ulEdgeTag != 0) {
      abed2[iedNew] = bed;
      iedNew++;
    }
  }}
  abed2.Unlock();
  // use that copy instead the original array
  abed.Clear();
  abed.MoveArray(abed2);
}

// optimize a polygon made out of BSP edges using tag information
template<class Type, int iDimensions>
void BSPEdge<Type, iDimensions>::OptimizeBSPEdges(CDynamicArray<BSPEdge<Type, iDimensions> > &abed)
{
  typedef BSPEdge<Type, iDimensions> edge_t; // local declaration, to fix macro expansion in FOREACHINDYNAMICARRAY

  // if there are no edges
  if (abed.Count()==0) {
    // do nothing
    return;
  }
  BOOL bSomeJoined;
  // repeat
  do {
    bSomeJoined = FALSE;
    // for each edge
    {FOREACHINDYNAMICARRAY(abed, edge_t, itbed1) {
      edge_t &bed1 = *itbed1;
      // if it is already marked
      if (bed1.bed_ulEdgeTag == 0) {
        // skip it
        continue;
      }
      // if it is dummy edge
      if (bed1.bed_vVertex0==bed1.bed_vVertex1) {
        // mark it for removal
        bSomeJoined = TRUE;
        bed1.bed_ulEdgeTag = 0;
        // skip it
        continue;
      }

      // for each other edge
      {FOREACHINDYNAMICARRAY(abed, edge_t, itbed2) {
        edge_t &bed2 = *itbed2;
        if (&bed1==&bed2) {
          continue;
        }
        // if it is already marked
        if (bed2.bed_ulEdgeTag == 0) {
          // skip it
          continue;
        }
        // if they originate from same edge (plane)
        if (bed1.bed_ulEdgeTag == bed2.bed_ulEdgeTag) {
          // if they are complemented
          if (bed1.bed_vVertex0==bed2.bed_vVertex1 && bed1.bed_vVertex1==bed2.bed_vVertex0) {
            // marked them both
            bSomeJoined = TRUE;
            bed1.bed_ulEdgeTag = 0;
            bed2.bed_ulEdgeTag = 0;
            // skip them both
            break;
          }
          // if second one continues after first one
          if (bed1.bed_vVertex1==bed2.bed_vVertex0) {
            // extend end of first edge to the end of second one
            bed1.bed_vVertex1=bed2.bed_vVertex1;
            bSomeJoined = TRUE;
            // marked second edge
            bed2.bed_ulEdgeTag = 0;
          // if second one continues before first one
          } else if (bed1.bed_vVertex0==bed2.bed_vVertex1) {
            // extend start of first edge to the start of second one
            bed1.bed_vVertex0=bed2.bed_vVertex0;
            bSomeJoined = TRUE;
            // marked second edge
            bed2.bed_ulEdgeTag = 0;
          }
        }
      }}
    }}
    // while some edges can be joined
  } while(bSomeJoined);

  // remove all marked edges
  RemoveMarkedBSPEdges(abed);
}

/////////////////////////////////////////////////////////////////////
// BSP polygon

/*
 * Add an edge to the polygon.
 */
template<class Type, int iDimensions>
inline void BSPPolygon<Type, iDimensions>::AddEdge(const Vector<Type, iDimensions> &vPoint0, const Vector<Type, iDimensions> &vPoint1, size_t ulTag)
{
  *bpo_abedPolygonEdges.New() = BSPEdge<Type, iDimensions>(vPoint0, vPoint1, ulTag);
}

/////////////////////////////////////////////////////////////////////
// BSP node

/*
 * Recursive destructor.
 */
template<class Type, int iDimensions>
void BSPNode<Type, iDimensions>::DeleteBSPNodeRecursively(void)
{
  // delete sub-trees first, before deleting this node
  if (bn_pbnFront!=NULL) {
    bn_pbnFront->DeleteBSPNodeRecursively();
  }
  if (bn_pbnBack!=NULL) {
    bn_pbnBack->DeleteBSPNodeRecursively();
  }
  delete this;
}

/*
 * Constructor for a leaf node.
 */
template<class Type, int iDimensions>
BSPNode<Type, iDimensions>::BSPNode(enum BSPNodeLocation bnl)
  : bn_bnlLocation(bnl)
  , bn_pbnFront(NULL)
  , bn_pbnBack(NULL)
{
  ASSERT(bnl == BNL_INSIDE || bnl == BNL_OUTSIDE);
}

/*
 * Constructor for a branch node.
 */
template<class Type, int iDimensions>
BSPNode<Type, iDimensions>::BSPNode(const Plane<Type, iDimensions> &plSplitPlane, size_t ulPlaneTag,
                 BSPNode<Type, iDimensions> &bnFront, BSPNode<Type, iDimensions> &bnBack)
  : Plane<Type, iDimensions>(plSplitPlane)
  , bn_bnlLocation(BNL_BRANCH)
  , bn_pbnFront(&bnFront)
  , bn_pbnBack(&bnBack)
  , bn_ulPlaneTag(ulPlaneTag)
{
}

/*
 * Constructor for cloning a bsp (sub)tree.
 */
template<class Type, int iDimensions>
BSPNode<Type, iDimensions>::BSPNode(BSPNode<Type, iDimensions> &bnRoot)
  : Plane<Type, iDimensions>(bnRoot)                             // copy the plane
  , bn_bnlLocation(bnRoot.bn_bnlLocation)     // copy the location
  , bn_ulPlaneTag(bnRoot.bn_ulPlaneTag)       // copy the plane tag
{
  // if this has a front child
  if (bnRoot.bn_pbnFront != NULL) {
    // clone front sub tree
    bn_pbnFront = new BSPNode<Type, iDimensions>(*bnRoot.bn_pbnFront);
  // otherwise
  } else {
    // no front sub tree
    bn_pbnFront = NULL;
  }

  // if this has a back child
  if (bnRoot.bn_pbnBack != NULL) {
    // clone back sub tree
    bn_pbnBack  = new BSPNode<Type, iDimensions>(*bnRoot.bn_pbnBack);
  // otherwise
  } else {
    // no back sub tree
    bn_pbnBack  = NULL;
  }
}

/* Test if a sphere is inside, outside, or intersecting. (Just a trivial rejection test) */
template<class Type, int iDimensions>
FLOAT BSPNode<Type, iDimensions>::TestSphere(const Vector<Type, iDimensions> &vSphereCenter, Type tSphereRadius) const
{
  // if this is an inside node
  if (bn_bnlLocation == BNL_INSIDE) {
    // it is inside
    return 1;
  // if this is an outside node
  } else if (bn_bnlLocation == BNL_OUTSIDE) {
    // it is outside
    return -1;
  // if this is a branch
  } else {
    ASSERT(bn_bnlLocation == BNL_BRANCH);
    // test the sphere against the split plane
    Type tCenterDistance = this->PointDistance(vSphereCenter);
    // if the sphere is in front of the plane
    if (tCenterDistance > +tSphereRadius) {
      // recurse down the front node
      return bn_pbnFront->TestSphere(vSphereCenter, tSphereRadius);
    // if the sphere is behind the plane
    } else if (tCenterDistance < -tSphereRadius) {
      // recurse down the back node
      return bn_pbnBack->TestSphere(vSphereCenter, tSphereRadius);
    // if the sphere is split by the plane
    } else {
      // if front node touches
      FLOAT fFront = bn_pbnFront->TestSphere(vSphereCenter, tSphereRadius);
      if (fFront==0) {
        // it touches
        return 0;
      }
      // if back node touches
      FLOAT fBack = bn_pbnBack->TestSphere(vSphereCenter, tSphereRadius);
      if (fBack==0) {
        // it touches
        return 0;
      }
      // if front and back have same classification
      if (fFront==fBack) {
        // return it
        return fFront;
      // if front and back have different classification
      } else {
        // it touches
        return 0;
      }
    }
  }
}

#ifdef SPHERE_HACK
// truncate doubles in d0-d3 to floats in d0-d1
// destroys d2-d7
#define doubles_to_floats \
	  "vmov.i32  q3, #0xff800000\n" \
	  "vshrn.i64 d4, q0, #29\n" \
	  "vshrn.i64 d5, q1, #29\n" \
	  "vshrn.i64 d0, q0, #32\n" \
	  "vshrn.i64 d1, q1, #32\n" \
	  "vsub.i32  q2, q3\n" \
	  "vbic.i32  q3, #0xc0000000\n" \
	  "vshl.i32  q1, q0, #1\n" \
	  "vceq.i32  q1, q1, #0\n" \
	  "vadd.i32  q2, q3\n" \
	  "vorr.i32  q1, #0x80000000\n" \
	  "vbif.32   q0, q2, q1\n"

// params[6]: vec[3], -1, radius, -radius
template<>
int BSPNode<double, 3>::TestSphere_hack(const FLOAT *params) const
{
  const BSPNode<double, 3> *node, *next;

#ifdef __arm__
  register double params_q8 __asm__("q8");
  __asm__ __volatile__ (
    "vld1.64   {d16,d17},  [%[prm], :64]\n"
    :       "=w"(params_q8)
    : [prm] "r"(params)
  );
#endif

  node = this;
  for (;;)
  {
#ifdef __arm__
    register double vec_q0 __asm__("q0");
    register double vec_d2 __asm__("d2");
    __asm__ __volatile__ (
      "vld1.64   {d0,d1,d2}, [%[vec], :64]\n"
      :       "=w"(vec_q0), "=w"(vec_d2)
      : [vec] "r"(node->vector)
    );
#endif
    // if this is an inside node
    if (node->bn_bnlLocation == BNL_INSIDE) {
      // it is inside
      return 1;
    // if this is an outside node
    } else if (node->bn_bnlLocation == BNL_OUTSIDE) {
      // it is outside
      return -1;
    // if this is a branch
    } else {
      ASSERT(node->bn_bnlLocation == BNL_BRANCH);
      // test the sphere against the split plane
      //double tCenterDistance = node->PointDistance(vSphereCenter);
#if defined(__arm__) && !defined(ANDROID) //karin: unuse asm
      register double dist_d3 __asm__("d3") = node->pl_distance;

      __asm__ __volatile__ (
        doubles_to_floats
        "vmul.f32  q0, q0, q8\n"
        "ldr       r2, %[pbnF]\n"
        "vldr      d2, %[rad]\n"
        "ldr       r3, %[pbnB]\n"
        "vpadd.f32 d0, d0, d1\n"
        "vmov      d3, r3, r2\n" // pbnF.pbnB
        "vpadd.f32 d0, d0, d0\n" // tCenterDistance
        "vcgt.f32  d4, d0, d2\n" // [0] tCenterDistance > radius
        "vcgt.f32  d5, d2, d0\n" // [1] -radius > tCenterDistance
        "vext.32   d0, d5, d4, #1\n" // [0].[1]
        "vand      d0, d3\n"
        "vpadd.i32 d0, d3\n"
        "vmov.i32  %[next], d0[0]\n"
        : [next] "=r"(next),
          "=w"(vec_q0), "=w"(vec_d2), "=w"(dist_d3)
        : "w"(vec_q0),  "w"(vec_d2),  "w"(dist_d3), "w"(params_q8),
          [pbnF] "m"(node->bn_pbnFront),
          [pbnB] "m"(node->bn_pbnBack),
          [rad]  "m"(params[4])
        : "r2", "r3", "q2", "q3"
      );
#else
      float tCenterDistance =
        node->vector[0] * params[0] +
        node->vector[1] * params[1] +
        node->vector[2] * params[2] - node->pl_distance;

      // if the sphere is in front of the plane
      if (tCenterDistance > +params[4]) {
        next = node->bn_pbnFront;
      // if the sphere is behind the plane
      } else if (tCenterDistance < params[5]) {
        next = node->bn_pbnBack;
      // if the sphere is split by the plane
      } else {
        next = NULL;
      }
#endif
      if (next == NULL)
        break;
      node = next;
    }
  }

  // if front node touches
  int iFront = node->bn_pbnFront->TestSphere_hack(params);
  if (iFront==0) {
    // it touches
    return 0;
  }
  // if back node touches
  int iBack = node->bn_pbnBack->TestSphere_hack(params);
  if (iBack==0) {
    // it touches
    return 0;
  }
  // if front and back have same classification
  if (iFront==iBack) {
    // return it
    return iFront;
  // if front and back have different classification
  } else {
    // it touches
    return 0;
  }
}

// _ZNK7BSPNodeIdLi3EE10TestSphereERK6VectorIdLi3EEd
template<>
FLOAT BSPNode<double, 3>::TestSphere(const Vector<double, 3> &vSphereCenter, double tSphereRadius) const
{
  float params[6] __attribute__((aligned(8)));

#if defined(__arm__) && !defined(ANDROID) //karin: unuse asm
  register double radius __asm__("d3") = tSphereRadius;
  __asm__ __volatile__ (
    "vld1.64   {d0,d1,d2}, [%[vec], :64]\n"
    doubles_to_floats
    "vmov.i32  d4, #0\n"
    "vmov.i32  d4[1], %[sgn]\n"
    "vmov.f32  d3, #-1.0\n"
    "vdup.32   d2, d1[1]\n"
    "vsli.i64  d1, d3, #32\n"
    "veor      d2, d4\n"
    "vst1.32   {d0,d1,d2}, [%[prm], :64]\n"
    :       "=w"(radius)
    : [vec] "r"(vSphereCenter.vector),
      [prm] "r"(params),
      [sgn] "r"(0x80000000),
            "w"(radius)
    : "q0", "d2", "q2", "q3"
  );
#else
  params[0] = vSphereCenter.vector[0];
  params[1] = vSphereCenter.vector[1];
  params[2] = vSphereCenter.vector[2];
  params[3] = -1.0f;
  params[4] = tSphereRadius;
  params[5] = -tSphereRadius;
#endif

  return TestSphere_hack(params);
}

// params[6]: vec[3], -1, radius, -radius
template<>
int BSPNode<float, 3>::TestSphere_hack(const FLOAT *params) const
{
  const BSPNode<float, 3> *node = this;

#ifdef __arm__
  register double params_q8 __asm__("q8");
  register double radius_d18 __asm__("d18");
  register double cdist_d6 __asm__("d6");
  __asm__ __volatile__ (
    "vld1.32   {d0,d1}, [%[vec]]\n"
    "vldr      s3, %[dist]\n"
    "vld1.64   {d16,d17,d18}, [%[prm], :64]\n"
    "vmul.f32  q0, q0, q8\n"
    "vpadd.f32 d6, d0, d1\n"
    :       "=w"(cdist_d6),
            "=w"(params_q8),
            "=w"(radius_d18)
    : [prm]  "r"(params),
      [vec]  "r"(node->vector),
      [dist] "m"(node->pl_distance)
  );
#endif

  for (;;)
  {
    // if this is an inside node
    if (node->bn_bnlLocation == BNL_INSIDE) {
      // it is inside
      return 1;
    // if this is an outside node
    } else if (node->bn_bnlLocation == BNL_OUTSIDE) {
      // it is outside
      return -1;
    // if this is a branch
    } else {
      ASSERT(node->bn_bnlLocation == BNL_BRANCH);
      // test the sphere against the split plane
      //float tCenterDistance = node->PointDistance(vSphereCenter);
#ifdef __arm__
      int gt_radius, lt_nradius;
      __asm__ __volatile__ (
        "vpadd.f32 d6, d6, d6\n"  // tCenterDistance
        "vld1.32   {d0,d1}, [%[pbnFv]]\n"
        "vldr      s3, %[pbnFd]\n"
        "vcgt.f32  d4, d6, d18\n" // [0] tCenterDistance > radius
        "vcgt.f32  d5, d18, d6\n" // [1] -radius > tCenterDistance
        "vld1.32   {d2,d3}, [%[pbnBv]]\n"
        "vldr      s7, %[pbnBd]\n"
        "vmov.i32  %[gt], d4[0]\n"
        "vmov.i32  %[lt], d5[1]\n"

        "vdup.i32  q2, d5[1]\n"
        "vbit      q0, q1, q2\n"
        "vmul.f32  q0, q0, q8\n"
        "vpadd.f32 d6, d0, d1\n"
        : [gt] "=r"(gt_radius), [lt] "=r"(lt_nradius),
          "=w"(cdist_d6)
        : "w"(cdist_d6), "w"(params_q8), "w"(radius_d18),
          [pbnFv] "r"(node->bn_pbnFront->vector),
          [pbnFd] "m"(node->bn_pbnFront->pl_distance),
          [pbnBv] "r"(node->bn_pbnBack->vector),
          [pbnBd] "m"(node->bn_pbnBack->pl_distance)
        : "q0", "q1", "q2", "d7"
      );

      // if the sphere is in front of the plane
      if (gt_radius) {
        node = node->bn_pbnFront;
      // if the sphere is behind the plane
      } else if (lt_nradius) {
        node = node->bn_pbnBack;
      // if the sphere is split by the plane
      } else {
        break;
      }
#else
      float tCenterDistance =
        node->vector[0] * params[0] +
        node->vector[1] * params[1] +
        node->vector[2] * params[2] - node->pl_distance;

      // if the sphere is in front of the plane
      if (tCenterDistance > +params[4]) {
        node = node->bn_pbnFront;
      // if the sphere is behind the plane
      } else if (tCenterDistance < params[5]) {
        node = node->bn_pbnBack;
      // if the sphere is split by the plane
      } else {
        break;
      }
#endif
    }
  }

  // if front node touches
  int iFront = node->bn_pbnFront->TestSphere_hack(params);
  if (iFront==0) {
    // it touches
    return 0;
  }
  // if back node touches
  int iBack = node->bn_pbnBack->TestSphere_hack(params);
  if (iBack==0) {
    // it touches
    return 0;
  }
  // if front and back have same classification
  if (iFront==iBack) {
    // return it
    return iFront;
  // if front and back have different classification
  } else {
    // it touches
    return 0;
  }
}

template<>
FLOAT BSPNode<float, 3>::TestSphere(const Vector<float, 3> &vSphereCenter, float tSphereRadius) const
{
  float params[6] __attribute__((aligned(8)));

  params[0] = vSphereCenter.vector[0];
  params[1] = vSphereCenter.vector[1];
  params[2] = vSphereCenter.vector[2];
  params[3] = -1.0f;
  params[4] = tSphereRadius;
  params[5] = -tSphereRadius;

  return TestSphere_hack(params);
}

#endif

/* Test if a box is inside, outside, or intersecting. (Just a trivial rejection test) */
template<class Type, int iDimensions>
FLOAT BSPNode<Type, iDimensions>::TestBox(const OBBox<Type> &box) const
{
  // if this is an inside node
  if (bn_bnlLocation == BNL_INSIDE) {
    // it is inside
    return 1;
  // if this is an outside node
  } else if (bn_bnlLocation == BNL_OUTSIDE) {
    // it is outside
    return -1;
  // if this is a branch
  } else {
    ASSERT(bn_bnlLocation == BNL_BRANCH);
    // test the box against the split plane
    Type tTest = box.TestAgainstPlane(*this);
    // if the sphere is in front of the plane
    if (tTest>0) {
      // recurse down the front node
      return bn_pbnFront->TestBox(box);
    // if the sphere is behind the plane
    } else if (tTest<0) {
      // recurse down the back node
      return bn_pbnBack->TestBox(box);
    // if the sphere is split by the plane
    } else {
      // if front node touches
      FLOAT fFront = bn_pbnFront->TestBox(box);
      if (fFront==0) {
        // it touches
        return 0;
      }
      // if back node touches
      FLOAT fBack = bn_pbnBack->TestBox(box);
      if (fBack==0) {
        // it touches
        return 0;
      }
      // if front and back have same classification
      if (fFront==fBack) {
        // return it
        return fFront;
      // if front and back have different classification
      } else {
        // it touches
        return 0;
      }
    }
  }
}

// find minimum/maximum parameters of points on a line that are inside - recursive
template<class Type, int iDimensions>
void BSPNode<Type, iDimensions>::FindLineMinMax(
  BSPLine<Type, iDimensions> &bl, 
  const Vector<Type, iDimensions> &v0,
  const Vector<Type, iDimensions> &v1,
  Type t0, Type t1)
{
  // if this is an inside node
  if (bn_bnlLocation == BNL_INSIDE) {
    // just update min/max
    bl.bl_tMin = Min(bl.bl_tMin, t0);
    bl.bl_tMax = Max(bl.bl_tMax, t1);
    return;
  // if this is an outside node
  } else if (bn_bnlLocation == BNL_OUTSIDE) {
    // do nothing
    return;
  // if this is a branch
  } else {
    ASSERT(bn_bnlLocation == BNL_BRANCH);
    // test the points against the split plane
    Type tD0 = this->PointDistance(v0);
    Type tD1 = this->PointDistance(v1);
    // if both are front
    if (tD0>=0 && tD1>=0) {
      // recurse down the front node
      bn_pbnFront->FindLineMinMax(bl, v0, v1, t0, t1);
      return;
    // if both are back
    } else if (tD0<0 && tD1<0) {
      // recurse down the back node
      bn_pbnBack->FindLineMinMax(bl, v0, v1, t0, t1);
      return;
    // if on different sides
    } else {
      // find split point
      Type tFraction = tD0/(tD0-tD1);
      Vector<Type, iDimensions> vS = v0+(v1-v0)*tFraction;
      Type tS = t0+(t1-t0)*tFraction;
      // if first is front
      if (tD0>=0) {
        // recurse first part down the front node
        bn_pbnFront->FindLineMinMax(bl, v0, vS, t0, tS);
        // recurse second part down the back node
        bn_pbnBack->FindLineMinMax(bl, vS, v1, tS, t1);
        return;
      // if first is back
      } else {
        // recurse first part down the back node
        bn_pbnBack->FindLineMinMax(bl, v0, vS, t0, tS);
        // recurse second part down the front node
        bn_pbnFront->FindLineMinMax(bl, vS, v1, tS, t1);
        return;
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////
// BSP cutter

/*
 * Constructor for splitting a polygon with a BSP tree.
 */
template<class Type, int iDimensions>
BSPCutter<Type, iDimensions>::BSPCutter(BSPPolygon<Type, iDimensions> &bpoPolygon, BSPNode<Type, iDimensions> &bnRoot)
{
  // cut the polygon with entire tree
  CutPolygon(bpoPolygon, bnRoot);
}

/*
 * Destructor.
 */
template<class Type, int iDimensions>
BSPCutter<Type, iDimensions>::~BSPCutter(void)
{
}

/*
 * Cut a polygon with a BSP tree.
 */
template<class Type, int iDimensions>
void BSPCutter<Type, iDimensions>::CutPolygon(BSPPolygon<Type, iDimensions> &bpoPolygon, BSPNode<Type, iDimensions> &bn)
{
  // if the polygon has no edges
  if (bpoPolygon.bpo_abedPolygonEdges.Count()==0) {
    // skip cutting
    return;
  }

  // if this node is inside node
  if (bn.bn_bnlLocation == BNL_INSIDE) {
    // add entire polygon to inside part
    bc_abedInside.MoveArray(bpoPolygon.bpo_abedPolygonEdges);

  // if this node is outside node
  } else if (bn.bn_bnlLocation == BNL_OUTSIDE) {
    // add entire polygon to outside part
    bc_abedOutside.MoveArray(bpoPolygon.bpo_abedPolygonEdges);

  // if this node is a branch
  } else if (bn.bn_bnlLocation == BNL_BRANCH) {
    BSPPolygon<Type, iDimensions> bpoFront;   // part of polygon in front of this splitter
    BSPPolygon<Type, iDimensions> bpoBack;    // part of polygon behind this splitter

    // split the polygon with split plane of this node
    BOOL bOnPlane = SplitPolygon(bpoPolygon, (Plane<Type, iDimensions> &)bn, bn.bn_ulPlaneTag, bpoFront, bpoBack);

    // if the polygon is not on the split plane
    if (!bOnPlane) {
      // recursively split front part with front part of bsp
      CutPolygon(bpoFront, *bn.bn_pbnFront);
      // recursively split back part with back part of bsp
      CutPolygon(bpoBack, *bn.bn_pbnBack);

    // if the polygon is on the split plane
    } else {
      BSPNode<Type, iDimensions> *pbnFront = nullptr;  // front node (relative to the polygon orientation)
      BSPNode<Type, iDimensions> *pbnBack = nullptr;   // back node (relative to the polygon orientation)

      // check the direction of the polygon with the front direction of the split plane
      Type tDirection = (Vector<Type, iDimensions> &)bpoPolygon%(Vector<Type, iDimensions> &)bn;
      // if the directions are same
      if (tDirection > +BSP_EPSILON) {
        // make nodes relative to polygon same as relative to the split plane
        pbnFront = bn.bn_pbnFront;
        pbnBack  = bn.bn_pbnBack;

      // if the directions are opposite
      } else if (tDirection < -BSP_EPSILON) {
        // make nodes relative to polygon opposite as relative to the split plane
        pbnFront = bn.bn_pbnBack;
        pbnBack  = bn.bn_pbnFront;
      // if the directions are indeterminate
      } else {
        // that must not be
        ASSERT(FALSE);
      }

      // cut it with front part of bsp
      BSPCutter<Type, iDimensions> bcFront(bpoPolygon, *pbnFront);
      // there must be no on-border parts
      ASSERT(bcFront.bc_abedBorderInside.Count()==0 && bcFront.bc_abedBorderOutside.Count()==0);

      // make a polygon from parts that are inside in front part of BSP
      BSPPolygon<Type, iDimensions> bpoInsideFront((Plane<Type, iDimensions> &)bpoPolygon, bcFront.bc_abedInside, bpoPolygon.bpo_ulPlaneTag);
      // cut them with back part of bsp
      BSPCutter<Type, iDimensions> bcBackInsideFront(bpoInsideFront, *pbnBack);

      // make a polygon from parts that are outside in front part of BSP
      BSPPolygon<Type, iDimensions> bpoOutsideFront((Plane<Type, iDimensions> &)bpoPolygon, bcFront.bc_abedOutside, bpoPolygon.bpo_ulPlaneTag);
      // cut them with back part of bsp
      BSPCutter<Type, iDimensions> bcBackOutsideFront(bpoOutsideFront, *pbnBack);

      // add parts that are inside both in front and back to inside part
      bc_abedInside.MoveArray(bcBackInsideFront.bc_abedInside);
      // add parts that are outside both in front and back to outside part
      bc_abedOutside.MoveArray(bcBackOutsideFront.bc_abedOutside);

      // add parts that are inside in front and outside back to on-border-inside-part
      bc_abedBorderInside.MoveArray(bcBackInsideFront.bc_abedOutside);
      // add parts that are outside in front and inside back to on-border-outside-part
      bc_abedBorderOutside.MoveArray(bcBackOutsideFront.bc_abedInside);
    }
  } else {
    ASSERTALWAYS("Bad node type");
  }
}

/*
 * Split a polygon with a plane.
 * -- returns FALSE if polygon is laying on the plane
 */
template<class Type, int iDimensions>
BOOL BSPCutter<Type, iDimensions>::SplitPolygon(BSPPolygon<Type, iDimensions> &bpoPolygon, const Plane<Type, iDimensions> &plSplitPlane, size_t ulPlaneTag,
  BSPPolygon<Type, iDimensions> &bpoFront, BSPPolygon<Type, iDimensions> &bpoBack)
{
  (Plane<Type, iDimensions> &)bpoFront = (Plane<Type, iDimensions> &)bpoPolygon;
  bpoFront.bpo_ulPlaneTag = bpoPolygon.bpo_ulPlaneTag;
  (Plane<Type, iDimensions> &)bpoBack = (Plane<Type, iDimensions> &)bpoPolygon;
  bpoBack.bpo_ulPlaneTag = bpoPolygon.bpo_ulPlaneTag;

  // calculate the direction of split line
  Vector<Type, iDimensions> vSplitDirection = ((Vector<Type, iDimensions> &)plSplitPlane) * (Vector<Type, iDimensions> &)bpoPolygon;

  // if the polygon is parallel with the split plane
  if (vSplitDirection.Length() < +BSP_EPSILON) {
    // calculate the distance of the polygon from the split plane
    Type fDistance = plSplitPlane.PlaneDistance(bpoPolygon);

    // if the polygon is in front of plane
    if (fDistance > +BSP_EPSILON) {
      // move all edges to front array
      bpoFront.bpo_abedPolygonEdges.MoveArray(bpoPolygon.bpo_abedPolygonEdges);
      // the polygon is not on the plane
      return FALSE;

    // if the polygon is behind the plane
    } else if (fDistance < -BSP_EPSILON) {
      // move all edges to back array
      bpoBack.bpo_abedPolygonEdges.MoveArray(bpoPolygon.bpo_abedPolygonEdges);
      // the polygon is not on the plane
      return FALSE;

    // if the polygon is on the plane
    } else {
      // just return so
      return TRUE;
    }

  // if the polygon is not parallel with the split plane
  } else {
    // initialize front and back vertex containers
    BSPVertexContainer<Type, iDimensions> bvcFront, bvcBack;
    bvcFront.Initialize(vSplitDirection);
    bvcBack.Initialize(-vSplitDirection);

    typedef BSPEdge<Type, iDimensions> edge_t; // local declaration, to fix macro expansion in FOREACHINDYNAMICARRAY
    // for each edge in polygon
    {FOREACHINDYNAMICARRAY(bpoPolygon.bpo_abedPolygonEdges, edge_t, itbed) {
      // split the edge
      SplitEdge(itbed->bed_vVertex0, itbed->bed_vVertex1, itbed->bed_ulEdgeTag, plSplitPlane,
        bpoFront, bpoBack, bvcFront, bvcBack);
    }}

    // sort vertex containers
    bvcFront.Sort();
    bvcBack.Sort();
    // elliminate paired vertices
    bvcFront.ElliminatePairedVertices();
    bvcBack.ElliminatePairedVertices();
    // create more front polygon edges from front vertex container
    bvcFront.CreateEdges(bpoFront.bpo_abedPolygonEdges, ulPlaneTag);
    // create more back polygon edges from back vertex container
    bvcBack.CreateEdges(bpoBack.bpo_abedPolygonEdges, ulPlaneTag);

    // the polygon is not on the plane
    return FALSE;
  }
}

/*
 * Split an edge with a plane.
 */
template<class Type, int iDimensions>
void BSPCutter<Type, iDimensions>::SplitEdge(const Vector<Type, iDimensions> &vPoint0, const Vector<Type, iDimensions> &vPoint1, size_t ulEdgeTag,
    const Plane<Type, iDimensions> &plSplitPlane,
    BSPPolygon<Type, iDimensions> &bpoFront, BSPPolygon<Type, iDimensions> &bpoBack,
    BSPVertexContainer<Type, iDimensions> &bvcFront, BSPVertexContainer<Type, iDimensions> &bvcBack)
{

  // calculate point distances from clip plane
  Type tDistance0 = plSplitPlane.PointDistance(vPoint0);
  Type tDistance1 = plSplitPlane.PointDistance(vPoint1);

  /* ---- first point behind plane ---- */
  if (tDistance0 < -BSP_EPSILON) {

    // if both are back
    if (tDistance1 < -BSP_EPSILON) {
      // add the whole edge to back node
      bpoBack.AddEdge(vPoint0, vPoint1, ulEdgeTag);
      // no split points

    // if first is back, second front
    } else if (tDistance1 > +BSP_EPSILON) {
      // calculate intersection coordinates
      Vector<Type, iDimensions> vPointMid = vPoint0-(vPoint0-vPoint1)*tDistance0/(tDistance0-tDistance1);
      // add front part to front node
      bpoFront.AddEdge(vPointMid, vPoint1, ulEdgeTag);
      // add back part to back node
      bpoBack.AddEdge(vPoint0, vPointMid, ulEdgeTag);
      // add split point to front _and_ back part of splitter
      bvcFront.AddVertex(vPointMid);
      bvcBack.AddVertex(vPointMid);

    // if first is back, second on the plane
    } else {
      // add the whole edge to back node
      bpoBack.AddEdge(vPoint0, vPoint1, ulEdgeTag);
      // add second point to back part of splitter
      bvcBack.AddVertex(vPoint1);
    }

  /* ---- first point in front of plane ---- */
  } else if (tDistance0 > +BSP_EPSILON) {
    // if first is front, second back
    if (tDistance1 < -BSP_EPSILON) {
      // calculate intersection coordinates
      Vector<Type, iDimensions> vPointMid = vPoint1-(vPoint1-vPoint0)*tDistance1/(tDistance1-tDistance0);
      // add front part to front node
      bpoFront.AddEdge(vPoint0, vPointMid, ulEdgeTag);
      // add back part to back node
      bpoBack.AddEdge(vPointMid, vPoint1, ulEdgeTag);
      // add split point to front _and_ back part of splitter
      bvcFront.AddVertex(vPointMid);
      bvcBack.AddVertex(vPointMid);


    // if both are front
    } else if (tDistance1 > +BSP_EPSILON) {
      // add the whole edge to front node
      bpoFront.AddEdge(vPoint0, vPoint1, ulEdgeTag);
      // no split points

    // if first is front, second on the plane
    } else {
      // add the whole edge to front node
      bpoFront.AddEdge(vPoint0, vPoint1, ulEdgeTag);
      // add second point to front part of splitter
      bvcFront.AddVertex(vPoint1);
    }

  /* ---- first point on the plane ---- */
  } else {
    // if first is on the plane, second back
    if (tDistance1 < -BSP_EPSILON) {
      // add the whole edge to back node
      bpoBack.AddEdge(vPoint0, vPoint1, ulEdgeTag);
      // add first point to back part of splitter
      bvcBack.AddVertex(vPoint0);

    // if first is on the plane, second in front of the plane
    } else if (tDistance1 > +BSP_EPSILON) {
      // add the whole edge to front node
      bpoFront.AddEdge(vPoint0, vPoint1, ulEdgeTag);
      // add first point to front part of splitter
      bvcFront.AddVertex(vPoint0);

    // if both are on the plane
    } else {
      // check the direction of the edge with the front direction of the splitter
      Type tDirection = (vPoint1-vPoint0)%bvcFront.bvc_vDirection;
      // if the directions are same
      if (tDirection > +BSP_EPSILON) {
        // add the whole edge to front node
        bpoFront.AddEdge(vPoint0, vPoint1, ulEdgeTag);
        // add both points to front part of the splitter
        bvcFront.AddVertex(vPoint0);
        bvcFront.AddVertex(vPoint1);


      // if the directions are opposite
      } else if (tDirection < -BSP_EPSILON) {
        // add the whole edge to back node
        bpoBack.AddEdge(vPoint0, vPoint1, ulEdgeTag);
        // add both points to back part of the splitter
        bvcBack.AddVertex(vPoint0);
        bvcBack.AddVertex(vPoint1);


      // if the directions are indeterminate
      } else {
        // that must mean that there is no edge in fact
        //ASSERT(Type(vPoint1-vPoint0) < 2*BSP_EPSILON); //!!!!
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////
// BSP tree

/*
 * Default constructor.
 */
template<class Type, int iDimensions>
BSPTree<Type, iDimensions>::BSPTree(void)
{
  bt_pbnRoot = NULL;
}

/*
 * Destructor.
 */
template<class Type, int iDimensions>
BSPTree<Type, iDimensions>::~BSPTree(void)
{
  Destroy();
}

/*
 * Constructor with array of polygons oriented inwards.
 */
template<class Type, int iDimensions>
BSPTree<Type, iDimensions>::BSPTree(CDynamicArray<BSPPolygon<Type, iDimensions> > &abpoPolygons)
{
  bt_pbnRoot = NULL;
  Create(abpoPolygons);
}

/*
 * Create bsp-subtree from array of polygons oriented inwards.
 */
template<class Type, int iDimensions>
BSPNode<Type, iDimensions> *BSPTree<Type, iDimensions>::CreateSubTree(CDynamicArray<BSPPolygon<Type, iDimensions> > &abpoPolygons)
{
  // local declarations, to fix macro expansion in FOREACHINDYNAMICARRAY
  typedef BSPPolygon<Type, iDimensions> polygon_t;
  ASSERT(abpoPolygons.Count()>=1);

  // use first polygon as splitter
  abpoPolygons.Lock();
  BSPPolygon<Type, iDimensions> bpoSplitter = abpoPolygons[0];
  abpoPolygons.Unlock();
  // tags must be valid
  ASSERT(bpoSplitter.bpo_ulPlaneTag!=-1);

  // create two new polygon arrays - back and front
  CDynamicArray<BSPPolygon<Type, iDimensions> > abpoFront, abpoBack;

  // for each polygon in this array
  {FOREACHINDYNAMICARRAY(abpoPolygons, polygon_t, itbpo) {
    BSPPolygon<Type, iDimensions> bpoFront, bpoBack;

    // tags must be valid
    ASSERT(itbpo->bpo_ulPlaneTag!=-1);
    // if the polygon has plane tag same as the tag of the splitter
    if (itbpo->bpo_ulPlaneTag == bpoSplitter.bpo_ulPlaneTag) {
      // they are assumed coplanar, so skip it
      continue;
    }

    // split it by the plane of splitter polygon
    BOOL bOnPlane = BSPCutter<Type, iDimensions>::SplitPolygon(itbpo.Current(),
      bpoSplitter, bpoSplitter.bpo_ulPlaneTag, bpoFront, bpoBack);

    // if the polygon is not coplanar with the splitter
    if (!bOnPlane) {

      // if there are some parts that are front
      if (bpoFront.bpo_abedPolygonEdges.Count()>0) {
        // create a polygon in front array and add all inside parts to it
        BSPPolygon<Type, iDimensions> *pbpo = abpoFront.New(1);
        pbpo->bpo_abedPolygonEdges.MoveArray(bpoFront.bpo_abedPolygonEdges);
        *(Plane<Type, iDimensions> *)pbpo = itbpo.Current();
        pbpo->bpo_ulPlaneTag = itbpo->bpo_ulPlaneTag;
      }
      // if there are some parts that are back
      if (bpoBack.bpo_abedPolygonEdges.Count()>0) {
        // create a polygon in back array and add all outside parts to it
        BSPPolygon<Type, iDimensions> *pbpo = abpoBack.New(1);
        pbpo->bpo_abedPolygonEdges.MoveArray(bpoBack.bpo_abedPolygonEdges);
        *(Plane<Type, iDimensions> *)pbpo = itbpo.Current();
        pbpo->bpo_ulPlaneTag = itbpo->bpo_ulPlaneTag;
      }
    }
  }}

  // free this array (to not consume too much memory)
  abpoPolygons.Clear();

  BSPNode<Type, iDimensions> *pbnFront, *pbnBack;
  // if there is some polygon in front array
  if (abpoFront.Count()>0) {
    // create front subtree using front array
    pbnFront = CreateSubTree(abpoFront);
  // otherwise
  } else {
    // make front node an inside leaf node
    pbnFront = new BSPNode<Type, iDimensions>(BNL_INSIDE);
  }

  // if there is some polygon in back array
  if (abpoBack.Count()>0) {
    // create back subtree using back array
    pbnBack = CreateSubTree(abpoBack);
  // otherwise
  } else {
    // make back node an outside leaf node
    pbnBack = new BSPNode<Type, iDimensions>(BNL_OUTSIDE);
  }

  // make a splitter node with the front and back nodes
  return new BSPNode<Type, iDimensions>(bpoSplitter, bpoSplitter.bpo_ulPlaneTag, *pbnFront, *pbnBack);
}

/*
 * Create bsp-tree from array of polygons oriented inwards.
 */
template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::Create(CDynamicArray<BSPPolygon<Type, iDimensions> > &abpoPolygons)
{
  // free eventual existing tree
  Destroy();

  // create the tree using the recursive function
  bt_pbnRoot = CreateSubTree(abpoPolygons);
  // move the tree to array
  MoveNodesToArray();
}

/*
 * Destroy bsp-tree.
 */
template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::Destroy(void)
{
  // if tree is in array
  if (bt_abnNodes.Count()>0) {
    // clear array
    bt_abnNodes.Clear();
    bt_pbnRoot = NULL;
  // if there is some free tree
  } else if (bt_pbnRoot != NULL) {
    // delete it
    bt_pbnRoot->DeleteBSPNodeRecursively();
    bt_pbnRoot = NULL;
  }
}

/* Test if a sphere could touch any of inside nodes. (Just a trivial rejection test) */
template<class Type, int iDimensions>
FLOAT BSPTree<Type, iDimensions>::TestSphere(const Vector<Type, iDimensions> &vSphereCenter, Type tSphereRadius) const
{
  if (bt_pbnRoot==NULL) return FALSE;
  // just start recursive testing at root node
  return bt_pbnRoot->TestSphere(vSphereCenter, tSphereRadius);
}
/* Test if a box is inside, outside, or intersecting. (Just a trivial rejection test) */
template<class Type, int iDimensions>
FLOAT BSPTree<Type, iDimensions>::TestBox(const OBBox<Type> &box) const
{
  if (bt_pbnRoot==NULL) return FALSE;
  // just start recursive testing at root node
  return bt_pbnRoot->TestBox(box);
}

// find minimum/maximum parameters of points on a line that are inside
template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::FindLineMinMax(
  const Vector<Type, iDimensions> &v0,
  const Vector<Type, iDimensions> &v1,
  Type &tMin,
  Type &tMax) const
{
  // init line
  BSPLine<Type, iDimensions> bl;
  bl.bl_tMin = UpperLimit(Type(0));
  bl.bl_tMax = LowerLimit(Type(0));

  // recursively split it
  bt_pbnRoot->FindLineMinMax(bl, v0, v1, Type(0), Type(1));

  // return the min/max
  tMin = bl.bl_tMin;
  tMax = bl.bl_tMax;
}

static INDEX _ctNextIndex;
/* Move one subtree to array. */
template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::MoveSubTreeToArray(BSPNode<Type, iDimensions> *pbnSubtree)
{
  // if this is no node
  if (pbnSubtree==NULL) {
    // do nothing
    return;
  }
  // first move all subnodes
  MoveSubTreeToArray(pbnSubtree->bn_pbnFront);
  MoveSubTreeToArray(pbnSubtree->bn_pbnBack);

  // get the node in array
  BSPNode<Type, iDimensions> &bnInArray = bt_abnNodes[_ctNextIndex];
  _ctNextIndex--;

  // copy properties to the array node
  (Plane<Type, iDimensions>&)bnInArray = (Plane<Type, iDimensions>&)*pbnSubtree;
  bnInArray.bn_bnlLocation = pbnSubtree->bn_bnlLocation;
  bnInArray.bn_ulPlaneTag = pbnSubtree->bn_ulPlaneTag;
  // let plane tag hold pointer to node in array
  pbnSubtree->bn_ulPlaneTag = (size_t)&bnInArray;

  // remap pointers to subnodes
  if (pbnSubtree->bn_pbnFront==NULL) {
    bnInArray.bn_pbnFront = NULL;
  } else {
    bnInArray.bn_pbnFront = (BSPNode<Type, iDimensions>*)pbnSubtree->bn_pbnFront->bn_ulPlaneTag;
  }
  if (pbnSubtree->bn_pbnBack==NULL) {
    bnInArray.bn_pbnBack = NULL;
  } else {
    bnInArray.bn_pbnBack = (BSPNode<Type, iDimensions>*)pbnSubtree->bn_pbnBack->bn_ulPlaneTag;
  }
}

/* Count nodes in subtree. */
template<class Type, int iDimensions>
INDEX BSPTree<Type, iDimensions>::CountNodes(BSPNode<Type, iDimensions> *pbnSubtree)
{
  if (pbnSubtree==NULL) {
    return 0;
  } else {
    return 1+
      CountNodes(pbnSubtree->bn_pbnFront)+
      CountNodes(pbnSubtree->bn_pbnBack);
  }
}

/* Move all nodes to array. */
template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::MoveNodesToArray(void)
{
  // if there is no tree
  if (bt_pbnRoot == NULL) {
    // do nothing
    return;
  }

  // count nodes
  INDEX ctNodes = CountNodes(bt_pbnRoot);
  // allocate large enough array
  bt_abnNodes.New(ctNodes);
  // start at the end of array
  _ctNextIndex = ctNodes-1;
  // recusively remap all nodes
  MoveSubTreeToArray(bt_pbnRoot);

  // delete the old nodes
  bt_pbnRoot->DeleteBSPNodeRecursively();

  // first node is always at start of array
  bt_pbnRoot = &bt_abnNodes[0];
}

/* Read/write entire bsp tree to disk. */
template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::Read_t(CTStream &strm) // throw char *
{
  // free eventual existing tree
  Destroy();

  // read current version and size
  INDEX iVersion;
  SLONG slSize;
  strm>>iVersion>>slSize;
  ASSERT(iVersion==1);

  // read count of nodes and create array
  INDEX ctNodes;
  strm>>ctNodes;
  // This assert was less silly when it was basically sizeof (*this), but to serialize this across targets, it looks different now.  --ryan.
  ASSERT(slSize==(SLONG)(sizeof(INDEX)+ctNodes*((sizeof(Type)*(iDimensions+1))+16)));
  bt_abnNodes.New(ctNodes);
  // for each node
  for(INDEX iNode=0; iNode<ctNodes; iNode++) {
    BSPNode<Type, iDimensions> &bn = bt_abnNodes[iNode];
    // read it from disk
    strm.Read_t(&(Plane<Type, iDimensions>&)bn, sizeof(Plane<Type, iDimensions>));
    strm>>(INDEX&)bn.bn_bnlLocation;

    INDEX iFront;
    strm>>iFront;
    if (iFront==-1) {
      bn.bn_pbnFront=NULL;
    } else {
      bn.bn_pbnFront = &bt_abnNodes[iFront];
    }

    INDEX iBack;
    strm>>iBack;
    if (iBack==-1) {
      bn.bn_pbnBack=NULL;
    } else {
      bn.bn_pbnBack = &bt_abnNodes[iBack];
    }
    ULONG ul;
    strm>>ul;
    bn.bn_ulPlaneTag = ul;
  }

  // check end id
  strm.ExpectID_t("BSPE");  // bsp end

  // first node is always at start of array
  if (bt_abnNodes.Count()>0) {
    bt_pbnRoot = &bt_abnNodes[0];
  } else {
    bt_pbnRoot = NULL;
  }
}

template<class Type, int iDimensions>
void BSPTree<Type, iDimensions>::Write_t(CTStream &strm) // throw char *
{
  INDEX ctNodes = bt_abnNodes.Count();

  // calculate size of chunk to write
  SLONG slSize = sizeof(INDEX)+ctNodes*sizeof(BSPNode<Type, iDimensions>);
  // write current version and size
  strm<<INDEX(1)<<slSize;

  // write count of nodes
  strm<<ctNodes;
  // for each node
  for(INDEX iNode=0; iNode<ctNodes; iNode++) {
    BSPNode<Type, iDimensions> &bn = bt_abnNodes[iNode];
    // write it to disk
    strm.Write_t(&(Plane<Type, iDimensions>&)bn, sizeof(Plane<Type, iDimensions>));
    strm<<(INDEX&)bn.bn_bnlLocation;

    INDEX iFront;
    if (bn.bn_pbnFront==NULL) {
      iFront=-1;
    } else {
      iFront = bt_abnNodes.Index(bn.bn_pbnFront);
    }
    strm<<iFront;

    INDEX iBack;
    if (bn.bn_pbnBack==NULL) {
      iBack=-1;
    } else {
      iBack = bt_abnNodes.Index(bn.bn_pbnBack);
    }
    strm<<iBack;

    strm<<IntPtrToID(bn.bn_ulPlaneTag);
  }
  // write end id for checking
  strm.WriteID_t("BSPE");  // bsp end
}

// instantiate template classes implemented here for needed types
#pragma warning (disable: 4660) // if already instantiated by some class

// remove templates
template class BSPVertex<DOUBLE, 3>; //DOUBLEbspvertex3D;
template class BSPVertexContainer<DOUBLE, 3>;
template class BSPEdge<DOUBLE, 3>;
template class BSPNode<DOUBLE, 3>;
template class BSPPolygon<DOUBLE, 3>;
template class BSPTree<DOUBLE, 3>;
template class BSPCutter<DOUBLE, 3>;

template class BSPVertex<FLOAT, 3>;
template class BSPVertexContainer<FLOAT, 3>;
template class BSPEdge<FLOAT, 3>;
template class BSPNode<FLOAT, 3>;
template class BSPPolygon<FLOAT, 3>;
template class BSPTree<FLOAT, 3>;
template class BSPCutter<FLOAT, 3>;

#pragma warning (default: 4660)

