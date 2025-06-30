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

#include <Engine/Brushes/Brush.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Math/Float.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>

// define this to dump all triangularization steps to debugging output
//#define DUMP_ALLSTEPS 1

//#define OPERATEIN2D 1

// this must not be caught by the dependency catcher!
static CTFileName _fnmDebugOutput(CTString("Temp\\Triangularization.out"));
// set if debug output file is open
static BOOL _bDebugOutputOpen = FALSE;
// file where debugging output is written to
static CTFileStream _strmDebugOutput;

// edge splitting epsilon
//#define EPSILON 0.0
//#define EPSILON (1.0/8388608.0) // 1/2^23
#define EPSILON (1.0f/65536.0f) // 1/2^16
//#define EPSILON DOUBLE(0.00390625) // 1/2^8 
//#define EPSILON 0.0009765625f // 1/2^10
//#define EPSILON 0.03125f    // 1/2^5
//#define EPSILON 0.00390625f // 1/2^8 

// minimum triangle quality that is trivially accepted
#define QUALITY_ACCEPTTRIVIALLY DOUBLE(0.01)

// this function is here to satisfy compiler's weird need when compiling 
// the template for CDynamicArray<CBrushVertex *>
inline void Clear(CBrushVertex *pbvx) {
  (void)pbvx;
};

/*
Algorithm used:

We take each edge and each vertex that is left of that edge, and try to find out 
if the edge and the vertex can span a triangle on the polygon without itersecting 
any other edges.

A simple convex BSP is used for testing intersection. The triangle edges are 
represented by planes oriented outwards (to the right side of the edge).

Triangle quality is defined as division of the area and square of the length of
the longest edge. We always try to extract the triangle with the best quality. Note
that clockwise triangles will have negative area.

Also we remember if there is left and/or right edge that connects 
the edge and the vertex. That makes it posible to geometricaly remove the triangle 
from the array of edges if it satisfies.

Although it is teoretically true that every polygon can be triangularized this way,
it is not always possible to find triangle with positive area that doesn't intersect
any edges in the polygon. This is due to the numerical inprecisions created during CSG 
or similar operations.
Therefore, we define comparation between two triangles this way:
  (1) Triangle that doesn't intersect any edges is better than one that does.
  (2) When (1) is indeterminate, the one with greater quality is considered better.

Note that a triangle with negative area can be found best. In such case, it is not really
created, but only removed from the polygon. This is mostly the case with some small 
degenerate parts of polygons. This way, the problematic case is just eliminated without
loosing too much precision in the triangularized representation of the polygon.

This method can diverge by adding and removing same triangle repeatedly. Therefore,
we don't always start at testing at the first edge in polygon, but move the starting
point when we find some triangle, 'shuffling' the choices of new triangles that way.
*/

// a class that handles triangularization of a polygon
class CTriangularizer {
public:
  DOUBLE3D tr_vPolygonNormal;
  // reference to original polygon  (just for debugging)
  CBrushPolygon &tr_bpoOriginalPolygon;
  // reference to original array of polygon edges
  CStaticArray<CBrushPolygonEdge> &tr_abpeOriginalEdges;
  // temporary array of edges
  CDynamicArray<CBrushEdge> tr_abedEdges;

  // major axes of the polygon plane
  INDEX tr_iAxis0;
  INDEX tr_iAxis1;
  
  // these describe the triangle curently considered
  CBrushEdge *tr_pbedLeft;        // left edge of the triangle if found
  CBrushEdge *tr_pbedRight;       // right edge of the triangle if found
  CBrushEdge *tr_pbedBottom;      // bottom edge of the triangle
  CBrushVertex *tr_pbvxTopVertex; // top vertex of the triangle
  DOUBLEplane3D tr_plLeft;         // splitter plane of left edge
  DOUBLEplane3D tr_plRight;        // splitter plane of right edge
  DOUBLEplane3D tr_plBottom;       // splitter plane of bottom edge
  DOUBLEplane3D tr_plEdgeConsidered; // splitter plane of the edge considered bottom

  // these describe the best triangle found yet
  DOUBLE tr_fQualityBest;             // quality of the best triangle
  BOOL tr_bIntersectedBest;           // set if best triangle intersects some edges
  CBrushEdge *tr_pbedLeftBest;        // left edge of the triangle if found
  CBrushEdge *tr_pbedRightBest;       // right edge of the triangle if found
  CBrushEdge *tr_pbedBottomBest;      // bottom edge of the triangle
  CBrushVertex *tr_pbvxTopVertexBest; // top vertex of the triangle

  // get a vertex coordinates
  inline DOUBLE3D GetVertex(CBrushVertex *pbvx) const;

  /* Create a splitter plane for an edge. */
  inline void EdgeToPlane(const DOUBLE3D &vVertex0, const DOUBLE3D &vVertex1,
    DOUBLEplane3D &plPlane) const;
  /* Clip edge to a plane and check if something remains behind. */
  BOOL ClipEdge(DOUBLE3D &vVertex0, DOUBLE3D &vVertex1, const DOUBLEplane3D &plPlane) const;
  /* Check if an edge is entirely or partially inside considered triangle. */
  BOOL EdgeInsideTriangle(const CBrushEdge *pbed) const;
  /* Calculate the quality of currently considered triangle. */
  DOUBLE TriangleQuality(void) const;

  /* Check that duplicate or reverse edges do not exist. */
  void CheckForInvalidEdges(void);
  /* Add given edge to temporary array. */
  void AddEdge(CBrushVertex *pbvxVertex0, CBrushVertex *pbvxVertex1);

  /* Create an array of forward oriented edges from polygon edges of a polygon. */
  void MakeEdgesForTriangularization(void);
  /* Test all edges for intersection with considered triangle. */
  BOOL CheckTriangleAgainstEdges(void);
  /* Find best triangle in array of edges. */
  void FindBestTriangle(void);
  /* Find if left/right triangle edges already exist. */
  void FindExistingTriangleEdges(void);
  /* Remove best triangle from edges. */
  void RemoveBestTriangleFromPolygon(void);
  /* Add best triangle to triangles. */
  void AddBestTriangleToTriangles(void);

  /* Print a statement to debugging output file. */
  void DPrintF(const char *strFormat, ...);
  /* Dump triangle edges to debug output. */
  void DumpEdges(void);

  // find vertices used without duplication
  void FindVerticesUsed(void);
  // build array of element indices
  void MakeElements(void);

public:
  // target array for resulting triangles
  CDynamicArray<CBrushVertex *> tr_apbvxTriangleVertices;
  // array for vertices without duplicates
  CStaticStackArray<CBrushVertex *> tr_apbvxVertices;
  // array for triangle elements
  CStaticArray<INDEX> tr_aiElements;

  /* Constructor - do triangularization for a polygon. */
  CTriangularizer(CBrushPolygon &bpoOriginalPolygon);

  INDEX tr_iError;
};
// get a vertex coordinates
inline DOUBLE3D CTriangularizer::GetVertex(CBrushVertex *pbvx) const
{
#ifdef OPERATEIN2D
  return DOUBLE3D(
    pbvx->bvx_vdPreciseRelative(tr_iAxis0), 
    pbvx->bvx_vdPreciseRelative(tr_iAxis1), 
    0);
#else
  return pbvx->bvx_vdPreciseRelative;
#endif
}                  
 
/*
 * Create a splitter plane for an edge.
 */
inline void CTriangularizer::EdgeToPlane(const DOUBLE3D &vVertex0, const DOUBLE3D &vVertex1,
  DOUBLEplane3D &plPlane) const
{
  // make a plane containing the edge, perpendicular to the polygon plane, looking
  // to the right of the edge
  plPlane = DOUBLEplane3D((vVertex1-vVertex0)*tr_vPolygonNormal, vVertex0);
}

/*
 * Clip edge to a plane and check if something remains behind.
 */
BOOL CTriangularizer::ClipEdge(DOUBLE3D &vVertex0, DOUBLE3D &vVertex1, 
                               const DOUBLEplane3D &plPlane) const
{
  // calculate point distances from clip plane
  DOUBLE fDistance0 = plPlane.PointDistance(vVertex0);
  DOUBLE fDistance1 = plPlane.PointDistance(vVertex1);
                   
  /* ---- first point behind plane ---- */
  if (fDistance0 < -EPSILON) {

    // if both are back
    if (fDistance1 < -EPSILON) {
      // whole edge is behind
      return TRUE;

    // if first is back, second front
    } else if (fDistance1 > +EPSILON) {
      // calculate intersection coordinates
      vVertex1 = vVertex0-(vVertex0-vVertex1)*fDistance0/(fDistance0-fDistance1);
      // the remaining part is behind
      return TRUE;

    // if first is back, second on the plane
    } else {
      // the whole edge is back
      return TRUE;
    }

  /* ---- first point in front of plane ---- */
  } else if (fDistance0 > +EPSILON) {

    // if first is front, second back
    if (fDistance1 < -EPSILON) {
      // calculate intersection coordinates
      vVertex0 = vVertex1-(vVertex1-vVertex0)*fDistance1/(fDistance1-fDistance0);
      // the remaining part is behind
      return TRUE;

    // if both are front
    } else if (fDistance1 > +EPSILON) {
      // whole edge is front
      return FALSE;

    // if first is front, second on the plane
    } else {
      // whole edge is front
      return FALSE;
    }

  /* ---- first point on the plane ---- */
  } else {
    // if first is on the plane, second back
    if (fDistance1 < -EPSILON) {
      // the whole edge is back
      return TRUE;

    // if first is on the plane, second front
    } else if (fDistance1 > +EPSILON) {
      // whole edge is front
      return FALSE;

    // if both are on the plane
    } else {
      // assume the whole edge is back (!!!! is this correct?)
      return TRUE;
    }
  }
}

/*
 * Check if an edge is entirely or partially inside considered triangle.
 */
BOOL CTriangularizer::EdgeInsideTriangle(const CBrushEdge *pbed) const
{
  // copy edge vertices
  DOUBLE3D vVertex0, vVertex1;
  vVertex0 = GetVertex(pbed->bed_pbvxVertex0);
  vVertex1 = GetVertex(pbed->bed_pbvxVertex1);

  // try to clip it to each triangle edge in turn, 
  // if anything remains behind - it is inside
  return (ClipEdge(vVertex0, vVertex1, tr_plLeft)
        &&ClipEdge(vVertex0, vVertex1, tr_plRight)
        &&ClipEdge(vVertex0, vVertex1, tr_plBottom));
}

/*
 * Calculate the quality of currently considered triangle.
 */
DOUBLE CTriangularizer::TriangleQuality(void) const
{
  DOUBLE3D vEdgeBottom = GetVertex(tr_pbedBottom->bed_pbvxVertex1) 
                       - GetVertex(tr_pbedBottom->bed_pbvxVertex0);

  DOUBLE3D vEdgeLeft   = GetVertex(tr_pbedBottom->bed_pbvxVertex0)
                       - GetVertex(tr_pbvxTopVertex);

  DOUBLE3D vEdgeRight  = GetVertex(tr_pbvxTopVertex)
                       - GetVertex(tr_pbedBottom->bed_pbvxVertex1);

  // calculate triangle normal as cross product of two edges
  DOUBLE3D vNormal = vEdgeLeft*(-vEdgeRight);
  // calculate area as half the length of the normal
  DOUBLE fArea = vNormal.Length()/ DOUBLE(2.0);
  // area must be initially positive
  ASSERT(fArea>=0);
  // if triangle normal is opposite to the polygon normal
  if (vNormal%tr_vPolygonNormal<0) {
    // make area negative
    fArea = -fArea;
  }

  // find length of all edges
  DOUBLE fLengthBottom = vEdgeBottom.Length();
  DOUBLE fLengthLeft   = vEdgeLeft  .Length();
  DOUBLE fLengthRight  = vEdgeRight .Length();
  // find maximum length
  DOUBLE fLengthMax = Max(fLengthBottom, Max(fLengthLeft, fLengthRight));
  // maximum length must be positive
  ASSERT(fLengthMax>0.0f);

  // quality is division of the area and square of the length of the longest edge
  return fArea/(fLengthMax*fLengthMax);
}

/*
 * Create an array of forward oriented edges from polygon edges of a polygon.
 */
void CTriangularizer::MakeEdgesForTriangularization(void)
{
  // get number of edges in polygon
  INDEX ctEdges = tr_abpeOriginalEdges.Count();
  // create that much edges in the array
  /* CBrushEdge *pbedEdges = */ tr_abedEdges.New(ctEdges);

  tr_abedEdges.Lock();

  // for each edge
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    CBrushPolygonEdge &bpe = tr_abpeOriginalEdges[iEdge];
    CBrushEdge &bed = tr_abedEdges[iEdge];
    // if polygon edge is reversed
    if (bpe.bpe_bReverse) {
      // make edge in array reversed
      bed.bed_pbvxVertex0 = bpe.bpe_pbedEdge->bed_pbvxVertex1;
      bed.bed_pbvxVertex1 = bpe.bpe_pbedEdge->bed_pbvxVertex0;
    // if polygon edge is not reversed
    } else {
      // make edge in array normal
      bed.bed_pbvxVertex0 = bpe.bpe_pbedEdge->bed_pbvxVertex0;
      bed.bed_pbvxVertex1 = bpe.bpe_pbedEdge->bed_pbvxVertex1;
    }
  }

  tr_abedEdges.Unlock();
}

/*
 * Add given edge to temporary array.
 */
void CTriangularizer::AddEdge(CBrushVertex *pbvxVertex0, CBrushVertex *pbvxVertex1)
{
  #ifndef NDEBUG
  // for each edge
  {FOREACHINDYNAMICARRAY(tr_abedEdges, CBrushEdge, itbed) {

    // must not exist
    if (itbed->bed_pbvxVertex0 == pbvxVertex0
      &&itbed->bed_pbvxVertex1 == pbvxVertex1) {
      return;
    }
    ASSERT(itbed->bed_pbvxVertex0 != pbvxVertex0
         ||itbed->bed_pbvxVertex1 != pbvxVertex1);

    // must not have reverse
    ASSERT(itbed->bed_pbvxVertex0 != pbvxVertex1
         ||itbed->bed_pbvxVertex1 != pbvxVertex0);
  }}
  #endif

  // add new edge
  CBrushEdge *pbedNew = tr_abedEdges.New(1);
  pbedNew->bed_pbvxVertex0 = pbvxVertex0;
  pbedNew->bed_pbvxVertex1 = pbvxVertex1;
}

/*
 * Check that duplicate or reverse edges do not exist.
 */
void CTriangularizer::CheckForInvalidEdges(void)
{
  // for each edge
  {FOREACHINDYNAMICARRAY(tr_abedEdges, CBrushEdge, itbed1) {
    // for each edge
    {FOREACHINDYNAMICARRAY(tr_abedEdges, CBrushEdge, itbed2) {
      // if same edge
      if (&*itbed1 == &*itbed2) {
        // skip
        continue;
      }
      CBrushEdge *pbed1 = itbed1;
      CBrushEdge *pbed2 = itbed2;

      // must not exist
      ASSERT(pbed1->bed_pbvxVertex0 != pbed2->bed_pbvxVertex0
           ||pbed1->bed_pbvxVertex1 != pbed2->bed_pbvxVertex1);

      // must not have reverse
      ASSERT(pbed1->bed_pbvxVertex0 != pbed2->bed_pbvxVertex1
           ||pbed1->bed_pbvxVertex1 != pbed2->bed_pbvxVertex0);
    }}
  }}
}

/*
 * Add best triangle to triangles.
 */
void CTriangularizer::AddBestTriangleToTriangles(void)
{
  // add the triangle to triangles
  CBrushVertex **ppbvxTriangle = tr_apbvxTriangleVertices.New(3);
  ppbvxTriangle[0] = tr_pbedBottomBest->bed_pbvxVertex0;
  ppbvxTriangle[1] = tr_pbedBottomBest->bed_pbvxVertex1;
  ppbvxTriangle[2] = tr_pbvxTopVertexBest;
}

/*
 * Remove best triangle from edges.
 */
void CTriangularizer::RemoveBestTriangleFromPolygon(void)
{
  tr_pbedBottom     = tr_pbedBottomBest;
  tr_pbvxTopVertex  = tr_pbvxTopVertexBest;
  FindExistingTriangleEdges();

  tr_pbedLeftBest       = tr_pbedLeft;
  tr_pbedRightBest      = tr_pbedRight;
  tr_pbedBottomBest     = tr_pbedBottom;
  tr_pbvxTopVertexBest  = tr_pbvxTopVertex;

  // if left edge was found
  if (tr_pbedLeftBest!=NULL) {
    // remove the left edge from the edges
    tr_abedEdges.Delete(tr_pbedLeftBest);
  // if left edge was not found
  } else {
    // add reverse of the left edge to the edges
    AddEdge(tr_pbedBottomBest->bed_pbvxVertex0, tr_pbvxTopVertexBest);
  }

  // if right edge was found
  if (tr_pbedRightBest!=NULL) {
    // remove the right edge from the edges
    tr_abedEdges.Delete(tr_pbedRightBest);
  // if right edge was not found
  } else {
    // add reverse of the right edge to the edges
    AddEdge(tr_pbvxTopVertexBest, tr_pbedBottomBest->bed_pbvxVertex1);
  }

  // remove the base edge from the edges
  tr_abedEdges.Delete(tr_pbedBottomBest);
}

/*
 * Find if left/right triangle edges already exist.
 */
void CTriangularizer::FindExistingTriangleEdges(void)
{
  // initialize left edge as not existing
  tr_pbedLeft = NULL;
  // initialize right edge as not existing
  tr_pbedRight = NULL;

  // for each edge
  FOREACHINDYNAMICARRAY(tr_abedEdges, CBrushEdge, itbed) {
    //CBrushEdge *pbed = itbed;

    // if it is the bottom edge of the triangle
    if (tr_pbedBottom == itbed) {
      // do not test it for intersection
      NOTHING;

    // if it is not bottom edge of triangle
    } else {
      ASSERT(itbed->bed_pbvxVertex0 != tr_pbedBottom->bed_pbvxVertex0
           ||itbed->bed_pbvxVertex1 != tr_pbedBottom->bed_pbvxVertex1);

      // if it is the left edge of the triangle
      if (itbed->bed_pbvxVertex1 == tr_pbedBottom->bed_pbvxVertex0
        &&itbed->bed_pbvxVertex0 == tr_pbvxTopVertex) {
        // remember it as the left edge
        ASSERT(tr_pbedLeft==NULL);
        tr_pbedLeft = itbed;

      // if it is the right edge of the triangle
      } else if (itbed->bed_pbvxVertex0 == tr_pbedBottom->bed_pbvxVertex1
        &&itbed->bed_pbvxVertex1 == tr_pbvxTopVertex) {
        // remember it as the right edge
        ASSERT(tr_pbedRight==NULL);
        tr_pbedRight = itbed;
      }
    }
  }
}

/*
 * Test all edges for intersection with considered triangle.
 */
BOOL CTriangularizer::CheckTriangleAgainstEdges(void)
{
  // for each edge
  FOREACHINDYNAMICARRAY(tr_abedEdges, CBrushEdge, itbed) {
    //CBrushEdge *pbed = itbed;

    // if it is the bottom edge of the triangle
    if (tr_pbedBottom == itbed) {
      // do not test it for intersection
      NOTHING;

    // if it is not bottom edge of triangle
    } else {
      ASSERT(itbed->bed_pbvxVertex0 != tr_pbedBottom->bed_pbvxVertex0
           ||itbed->bed_pbvxVertex1 != tr_pbedBottom->bed_pbvxVertex1);

      // if it is the left edge of the triangle
      if (itbed->bed_pbvxVertex1 == tr_pbedBottom->bed_pbvxVertex0
        &&itbed->bed_pbvxVertex0 == tr_pbvxTopVertex) {
        // do not test it for intersection
        NOTHING;

      // if it is the right edge of the triangle
      } else if (itbed->bed_pbvxVertex0 == tr_pbedBottom->bed_pbvxVertex1
        &&itbed->bed_pbvxVertex1 == tr_pbvxTopVertex) {
        // do not test it for intersection
        NOTHING;

      // if it is neither of triangle edges
      } else {
        // if this edge intersects with any of the triangle edges
        if (EdgeInsideTriangle(itbed)) {
          return TRUE;
        }
      }
    }
  }
  // if no intersections are found
  return FALSE;
}

// calculate edge direction (COPIED FROM CTMATH!)
void EdgeDir(const DOUBLE3D &vPoint0, const DOUBLE3D &vPoint1,
             DOUBLE3D &vDirection, DOUBLE3D &vReferencePoint)
{
  // if the vertices are reversed
/*  if (CompareVertices(vPoint0, vPoint1)<0) {
    // swap them
    Swap(vPoint0, vPoint1);
    // mark edge as reversed
    edx_bReverse = TRUE;

  // if the vertices are not reversed
  } else {
    // mark edge as not reversed
    edx_bReverse = FALSE;
  }
  */

  // normalize the direction
  vDirection = (vPoint1-vPoint0).Normalize();
  /* calculate the reference point on the line from any point on line (p) and direction (d):
    r = p - ((p.d)/(d.d))*d
  */
  vReferencePoint = vPoint0 - 
    vDirection * ((vPoint0 % vDirection))/(vDirection % vDirection);
}

/*
 * Print a statement to debugging output file.
 */
void CTriangularizer::DPrintF(const char *strFormat, ...)
{
  char strBuffer[256];
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat);
  vsprintf(strBuffer, strFormat, arg);
  va_end(arg);

  // if the debug output file is not open
  if (!_bDebugOutputOpen) {
    // open it
    try  {
      _strmDebugOutput.Create_t(_fnmDebugOutput, CTStream::CM_TEXT);
      _bDebugOutputOpen = TRUE;
    // if not successful
    } catch (const char *strError) {
      (void) strError;
      // print nothing
      return;
    }
  }
  // write the message to the file
  _strmDebugOutput.Write_t(strBuffer, strlen(strBuffer));
}

/*
 * Dump triangle edges to debug output.
 */
void CTriangularizer::DumpEdges(void)
{
  DPrintF("%d\n", tr_abedEdges.Count());
  FOREACHINDYNAMICARRAY(tr_abedEdges, CBrushEdge, itbed) {
    DPrintF("(%f, %f) ->",
      GetVertex(itbed->bed_pbvxVertex0)(1),
      GetVertex(itbed->bed_pbvxVertex0)(2));
    DPrintF(" (%f, %f) ",
      GetVertex(itbed->bed_pbvxVertex1)(1),
      GetVertex(itbed->bed_pbvxVertex1)(2));

    DOUBLE3D vDirection, vReferencePoint;
    EdgeDir(
      GetVertex(itbed->bed_pbvxVertex0), GetVertex(itbed->bed_pbvxVertex1),
      vDirection, vReferencePoint);

    DPrintF("[(%f, %f), ", 
      vDirection(1),
      vDirection(2));
    DPrintF("(%f, %f)]\n",
      vReferencePoint(1),
      vReferencePoint(2));
  }
}

// edge offsets used to 'shuffle' triangularization
static INDEX iBottomEdgeOffset;
static INDEX iTopEdgeOffset;

/*
 * Find best triangle in array of edges.
 */
/* NOTE: Currently only searching for first acceptable triangle.
*/
void CTriangularizer::FindBestTriangle(void)
{
  // clear best triangle description
  tr_fQualityBest = DOUBLE(-1E30);
  tr_bIntersectedBest = TRUE;
  tr_pbedLeftBest = NULL;
  tr_pbedRightBest = NULL;
  tr_pbedBottomBest = NULL;
  tr_pbvxTopVertexBest = NULL;

  iBottomEdgeOffset++;
  // for each edge 
  tr_abedEdges.Lock();
  INDEX ctEdges = tr_abedEdges.Count();
  for(INDEX ibedBottom=0; ibedBottom<tr_abedEdges.Count(); ibedBottom++) {
    // consider this edge as bottom edge of triangle
    tr_pbedBottom = &tr_abedEdges[(ibedBottom+iBottomEdgeOffset)%ctEdges];

    ASSERT(tr_pbedBottom->bed_pbvxVertex0 != tr_pbedBottom->bed_pbvxVertex1);
    // create bottom splitter
    EdgeToPlane(GetVertex(tr_pbedBottom->bed_pbvxVertex0),
      GetVertex(tr_pbedBottom->bed_pbvxVertex1), tr_plEdgeConsidered);

    iTopEdgeOffset++;
    // for each edge
    for(INDEX ibedTop=0; ibedTop<ctEdges; ibedTop++) {
      // consider first vertex of this edge as top vertex of triangle
      tr_pbvxTopVertex = tr_abedEdges[(ibedTop+ibedBottom+iTopEdgeOffset)%ctEdges].bed_pbvxVertex0;

      // if the top vertex is one of the vertices of bottom edge
      if ( tr_pbvxTopVertex == tr_pbedBottom->bed_pbvxVertex0
        || tr_pbvxTopVertex == tr_pbedBottom->bed_pbvxVertex1) {
        // skip this triangle
        continue;
      }
     
      // create bottom splitter from the bottom edge splitter
      tr_plBottom = tr_plEdgeConsidered;
      // create left splitter
      EdgeToPlane(GetVertex(tr_pbvxTopVertex),
        GetVertex(tr_pbedBottom->bed_pbvxVertex0), tr_plLeft);
      // create right splitter
      EdgeToPlane(GetVertex(tr_pbedBottom->bed_pbvxVertex1),
        GetVertex(tr_pbvxTopVertex), tr_plRight);

      // calculate its quality
      DOUBLE fCurrentQuality = TriangleQuality();
      // if triangle is clockwise
      if (fCurrentQuality<0) {
        continue;
        // reverse its splitters
        tr_plLeft.Flip();
        tr_plRight.Flip();
        tr_plBottom.Flip();
      }
      // check if no edges intersect with considered triangle
      BOOL bCurrentIntersected = CheckTriangleAgainstEdges();

      // if the current triangle is better than the best triangle yet
      if ((tr_bIntersectedBest && !bCurrentIntersected)
        ||(tr_bIntersectedBest==bCurrentIntersected && fCurrentQuality>tr_fQualityBest)) {

        // find if left/right triangle edges already exist.
        FindExistingTriangleEdges();
        // set current triangle as best triangle
        tr_bIntersectedBest   = bCurrentIntersected;
        tr_fQualityBest       = fCurrentQuality;
        tr_pbedLeftBest       = tr_pbedLeft;
        tr_pbedRightBest      = tr_pbedRight;
        tr_pbedBottomBest     = tr_pbedBottom;
        tr_pbvxTopVertexBest  = tr_pbvxTopVertex;
        // if the triangle is trivially acceptable
        if (!bCurrentIntersected && fCurrentQuality>=QUALITY_ACCEPTTRIVIALLY) {
          // finish searching
          tr_abedEdges.Unlock();
          return;
        }
      }
    }
  }
  tr_abedEdges.Unlock();

#if 0
  // if no acceptable triangles have been found
  if (tr_fQualityBest</*???*/sdfsdfd) {
    /* dump all sector's vertices */
    /*
    FOREACHINSTATICARRAY(tr_bpoOriginalPolygon.bpo_pbscSector->bsc_abvxVertices,
      CBrushVertex, itbvx) {
      DPrintF("(0x%p, %f, %f, %f)\n", &(*itbvx),
        (*itbvx)(1), (*itbvx)(2), (*itbvx)(3));
    }
    */

    // dump remaining edges
    DPrintF("Triangularization failed!\n");
    DPrintF("best quality found: %f\n", tr_fQualityBest);
    DPrintF("Remaining edges:\n");
    DumpEdges();
/*    // dump all edges
    tr_abedEdges.Clear();
    MakeEdgesForTriangularization();
    DumpEdges();
    */

    // error!
    ASSERTALWAYS("No acceptable triangles found for triangularization!");
    FatalError("No acceptable triangles found for triangularization!\n"
      "Debugging information written to file '%s'.", _fnmDebugOutput);
  }
#endif
}


// find vertices used without duplication
void CTriangularizer::FindVerticesUsed(void)
{
  // make an empty array that will contain the vertices
  tr_apbvxVertices.PopAll();
  // for each triangle vertex
  tr_apbvxTriangleVertices.Lock();
  for(INDEX ipbvx=0; ipbvx<tr_apbvxTriangleVertices.Count(); ipbvx++) {
    CBrushVertex *pbvx = tr_apbvxTriangleVertices[ipbvx];
    // for each vertex already added
    BOOL bFound=FALSE;
    for(INDEX ipbvxAdded=0; ipbvxAdded<tr_apbvxVertices.Count(); ipbvxAdded++) {
      // if it is the same one, stop searching
      if (tr_apbvxVertices[ipbvxAdded]==pbvx) {
        bFound = TRUE;
        break;
      }
    }
    // if not found
    if (!bFound) {
      // add it to the array
      tr_apbvxVertices.Push() = pbvx;
    }
  }
  tr_apbvxTriangleVertices.Unlock();
}
// build array of element indices
void CTriangularizer::MakeElements(void)
{
  // make elements array
  INDEX ctElements = tr_apbvxTriangleVertices.Count();
  tr_aiElements.Clear();
  tr_aiElements.New(ctElements);
  // for each triangle vertex
  tr_apbvxTriangleVertices.Lock();
  for(INDEX i=0; i<ctElements; i++) {
    CBrushVertex *pbvx = tr_apbvxTriangleVertices[i];
    // find its element index
#ifndef NDEBUG
    BOOL bFound=FALSE;
#endif // NDEBUG
    for(INDEX ipbvx=0; ipbvx<tr_apbvxVertices.Count(); ipbvx++) {
      // if it is the same one, stop searching
      if (tr_apbvxVertices[ipbvx]==pbvx) {
        tr_aiElements[i] = ipbvx;
#ifndef NDEBUG
        bFound = TRUE;
#endif // NDEBUG
        break;
      }
    }
    ASSERT(bFound);
  }
  tr_apbvxTriangleVertices.Unlock();
}

/*
 * Constructor - do triangularization for a polygon.
 */
CTriangularizer::CTriangularizer(CBrushPolygon &bpoOriginalPolygon)
  : tr_bpoOriginalPolygon(bpoOriginalPolygon)
  , tr_abpeOriginalEdges(bpoOriginalPolygon.bpo_abpePolygonEdges) // remember original edges
{
  // find polygon normal and major axes
#ifdef OPERATEIN2D
  INDEX iMaxNormal = bpoOriginalPolygon.bpo_pbplPlane->bpl_pldPreciseRelative.GetMaxNormal();
  INDEX iMaxSign = Sgn(bpoOriginalPolygon.bpo_pbplPlane->bpl_pldPreciseRelative(iMaxNormal));
  ASSERT(iMaxSign!=0);
  switch(iMaxNormal) {
  case 1: 
    if (iMaxSign==-1) {
      tr_iAxis0 = 3; tr_iAxis1 = 2; tr_vPolygonNormal = DOUBLE3D(-1,0,0);
    } else {
      tr_iAxis0 = 2; tr_iAxis1 = 3; tr_vPolygonNormal = DOUBLE3D(1,0,0);
    }
    break;
  case 2: 
    if (iMaxSign==-1) {
      tr_iAxis0 = 1; tr_iAxis1 = 3; tr_vPolygonNormal = DOUBLE3D(0,-1,0);
    } else {
      tr_iAxis0 = 3; tr_iAxis1 = 1; tr_vPolygonNormal = DOUBLE3D(0,1,0);
    }
    break;
  case 3: 
    if (iMaxSign==-1) {
      tr_iAxis0 = 2; tr_iAxis1 = 1; tr_vPolygonNormal = DOUBLE3D(0,0,-1);
    } else {
      tr_iAxis0 = 1; tr_iAxis1 = 2; tr_vPolygonNormal = DOUBLE3D(0,0,1);
    }
    break;
  default:
    ASSERT(FALSE);
    tr_iAxis0 = 2; tr_iAxis1 = 3; tr_vPolygonNormal = DOUBLE3D(1,0,0);
  }
  tr_vPolygonNormal = DOUBLE3D(0,0,1);
#else
    tr_iAxis0 = -1; tr_iAxis1 = -1; tr_vPolygonNormal = bpoOriginalPolygon.bpo_pbplPlane->bpl_pldPreciseRelative;
#endif
  
  // create a dynamic array of edges
  MakeEdgesForTriangularization();
  tr_iError = -1;

  // estimate max number of triangles
  INDEX ctOrgEdges = tr_abpeOriginalEdges.Count();
  INDEX ctMaxHoles = ctOrgEdges;
  INDEX ctMaxTriangles = ctOrgEdges-2+2*ctMaxHoles;

  iBottomEdgeOffset = 0;
  iTopEdgeOffset = 0;

  #ifdef DUMP_ALLSTEPS
  DPrintF("PolygonBegin\n");
  #endif

//  ASSERT(tr_abedEdges.Count()!=8);
  // while the array of edges is not empty
//*
  INDEX iPasses = 0;
  while (tr_abedEdges.Count()>0) {
    // if total number of triangles is too large, or searching too long
    INDEX ctTriangles = tr_apbvxTriangleVertices.Count()/3;
    if (ctTriangles>ctMaxTriangles*2 || iPasses>ctMaxTriangles*2) {
      // error, quit triangulation
      tr_iError = 2;
      return;
    }

    #ifdef DUMP_ALLSTEPS
    // dump remaining edges
    DumpEdges();
    #endif

    // find best triangle
    FindBestTriangle();
    #ifdef DUMP_ALLSTEPS
    DPrintF("BestQuality=%f\n", tr_fQualityBest);
    #endif
    // if no triangle is found
    if (tr_fQualityBest<0.0) {
      // quit searching
      tr_iError = 1;
      return;
    }
    if (tr_bIntersectedBest) {
      // quit searching
      tr_iError = 3;
      return;
    }

    // if best triangle has positive quality
    if (tr_fQualityBest>0.0) {
      // add it to triangles
      AddBestTriangleToTriangles();
    }
    // remove best triangle from edges
    RemoveBestTriangleFromPolygon();

    #ifndef NDEBUG
    // check that there are no invalid edges after creating
    CheckForInvalidEdges();
    #endif
    iPasses++;
  }
  tr_iError = 0;
//*/
}

/*
 * Make triangular representation of the polygon.
 */
void CBrushPolygon::Triangulate(void)
{
  // if already triangulated
  if (bpo_apbvxTriangleVertices.Count()>0) {
    // do nothing
    return;
  }

  // triangularize the polygon
  CTriangularizer tr(*this);

  // find vertices used without duplication
  tr.FindVerticesUsed();
  // build array of element indices
  tr.MakeElements();

  // if there was an error
  if (tr.tr_iError!=0) {
    // report it
    // CPrintF( TRANS("Cannot properly triangulate a polygon: error %d!\n"), tr.tr_iError);
    // mark the polygon
    bpo_ulFlags|=BPOF_INVALIDTRIANGLES;
  // if there no errors
  } else {
    // mark the polygon as triangulated well
    bpo_ulFlags&=~BPOF_INVALIDTRIANGLES;
  }

  // copy dynamic array of triangles to the triangular representation of the polygon
  INDEX ctVertices = tr.tr_apbvxVertices.Count();
  bpo_apbvxTriangleVertices.Clear();
  bpo_apbvxTriangleVertices.New(ctVertices);
  for (INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
    bpo_apbvxTriangleVertices[iVertex] = tr.tr_apbvxVertices[iVertex];
  }
  bpo_aiTriangleElements = tr.tr_aiElements;

  if (_bDebugOutputOpen) {
    _strmDebugOutput.Close();
    _bDebugOutputOpen = FALSE;
  }
}
