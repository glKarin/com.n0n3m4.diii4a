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

#ifndef SE_INCL_BRUSH_H
#define SE_INCL_BRUSH_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/Relations.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/TextureMapping.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Math/Projection.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/ShadowMap.h>
#include <Engine/Brushes/BrushBase.h>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/Selection.h>

// a vertex in brush
#define BVXF_DRAWNINWIREFRAME     (1L<<0)  // vertex is already drawn in wireframe
#define BVXF_SELECTED             (1L<<1)  // vertex is selected
class ENGINE_API CBrushVertex {
public:
  class CWorkingVertex *bvx_pwvxWorking;  // used for rendering and ray casting
  FLOAT3D  bvx_vAbsolute;
  FLOAT3D  bvx_vRelative;               // relative coordinates used for collision
  DOUBLE3D bvx_vdPreciseRelative;       // precise relative coordinates used for editing
  DOUBLE3D *bvx_pvdPreciseAbsolute;     // precise vertex coordinates in absolute space
  ULONG    bvx_ulFlags;                 // flags
  CBrushSector *bvx_pbscSector;         // back-pointer to sector

  /* Default constructor. */
  inline CBrushVertex(void) : bvx_pwvxWorking(NULL), bvx_ulFlags(0) {};
  /* Clear the object. */
  inline void Clear(void) {};

  // vertices may be selected
  IMPLEMENT_SELECTING(bvx_ulFlags)

  // set new absolute position for the vertex
  void SetAbsolutePosition(const DOUBLE3D &vAbsolute);

  // get amount of memory used by this object
  inline SLONG GetUsedMemory(void) { return sizeof(CBrushVertex); };
};


// selection of brush vertices
typedef CSelection<CBrushVertex, BVXF_SELECTED>  CBrushVertexSelection;

// a plane in brush
class ENGINE_API CBrushPlane {
public:
  class CWorkingPlane *bpl_pwplWorking;  // use for rendering and ray casting
  FLOATplane3D bpl_plAbsolute;
  FLOATplane3D bpl_plRelative;    // relative coordinates used for collision
  DOUBLEplane3D *bpl_ppldPreciseAbsolute; // precise relative plane coordinates in absolute space
  DOUBLEplane3D bpl_pldPreciseRelative;           // precise coordinates used for editing
  INDEX bpl_iPlaneMajorAxis1;  // major axes of the plane in apsolute space
  INDEX bpl_iPlaneMajorAxis2;

  /* Default constructor. */
  inline CBrushPlane(void) : bpl_pwplWorking(NULL) {};
  /* Clear the object. */
  inline void Clear(void) {};

  // get amount of memory used by this object
  inline SLONG GetUsedMemory(void) { return sizeof(CBrushPlane); };
};


// an edge in brush
class ENGINE_API CBrushEdge {
public:
  CBrushVertex *bed_pbvxVertex0;   // start vertex
  CBrushVertex *bed_pbvxVertex1;   // end vertex
  CWorkingEdge *bed_pwedWorking;   // pointer to screen edge if active in rendering

  /* Default constructor. */
  inline CBrushEdge(void) : bed_pwedWorking(NULL) {};
  /* Constructor with two vertices. */
  inline CBrushEdge(CBrushVertex *pbvx0, CBrushVertex *pbvx1)
    : bed_pbvxVertex0(pbvx0), bed_pbvxVertex1(pbvx1), bed_pwedWorking(NULL) {};
  /* Clear the object. */
  inline void Clear(void) {};
  /* Test if this edge touches another one. */
  BOOL TouchesInSameSector(CBrushEdge &bedOther);
  BOOL TouchesInAnySector(CBrushEdge &bedOther);

  // get amount of memory used by this object
  inline SLONG GetUsedMemory(void) { return sizeof(CBrushEdge); };
};

// a reference to edge used in brush polygon
class ENGINE_API CBrushPolygonEdge {
public:
  CBrushEdge *bpe_pbedEdge;         // pointer to the edge
  BOOL bpe_bReverse;                // true if the vertex0 and vertex1 must be swapped

  /* Clear the object. */
  inline void Clear(void) {};
  /* Get coordinates of the end vertices. */
  inline void GetVertices(CBrushVertex *&pbvx0, CBrushVertex *&pbvx1) {
    if (bpe_bReverse) {
      pbvx0 = bpe_pbedEdge->bed_pbvxVertex1;
      pbvx1 = bpe_pbedEdge->bed_pbvxVertex0;
    } else {
      pbvx0 = bpe_pbedEdge->bed_pbvxVertex0;
      pbvx1 = bpe_pbedEdge->bed_pbvxVertex1;
    }
  }
  inline void GetVertexCoordinatesAbsolute(FLOAT3D &v0, FLOAT3D &v1) {
    if (bpe_bReverse) {
      v0 = bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
      v1 = bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
    } else {
      v0 = bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
      v1 = bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
    }
  };
  inline void GetVertexCoordinatesRelative(FLOAT3D &v0, FLOAT3D &v1) {
    if (bpe_bReverse) {
      v0 = bpe_pbedEdge->bed_pbvxVertex1->bvx_vRelative;
      v1 = bpe_pbedEdge->bed_pbvxVertex0->bvx_vRelative;
    } else {
      v0 = bpe_pbedEdge->bed_pbvxVertex0->bvx_vRelative;
      v1 = bpe_pbedEdge->bed_pbvxVertex1->bvx_vRelative;
    }
  };
  inline void GetVertexCoordinatesPreciseRelative(DOUBLE3D &v0, DOUBLE3D &v1) {
    if (bpe_bReverse) {
      v0 = bpe_pbedEdge->bed_pbvxVertex1->bvx_vdPreciseRelative;
      v1 = bpe_pbedEdge->bed_pbvxVertex0->bvx_vdPreciseRelative;
    } else {
      v0 = bpe_pbedEdge->bed_pbvxVertex0->bvx_vdPreciseRelative;
      v1 = bpe_pbedEdge->bed_pbvxVertex1->bvx_vdPreciseRelative;
    }
  };
  inline void GetVertexCoordinatesPreciseAbsolute(DOUBLE3D &v0, DOUBLE3D &v1) {
    if (bpe_bReverse) {
      v0 = *bpe_pbedEdge->bed_pbvxVertex1->bvx_pvdPreciseAbsolute;
      v1 = *bpe_pbedEdge->bed_pbvxVertex0->bvx_pvdPreciseAbsolute;
    } else {
      v0 = *bpe_pbedEdge->bed_pbvxVertex0->bvx_pvdPreciseAbsolute;
      v1 = *bpe_pbedEdge->bed_pbvxVertex1->bvx_pvdPreciseAbsolute;
    }
  };
};


// one layer on brush shadow map (cross link between brush shadow map and light source)
#define BSLF_CALCULATED (1L<<0)
#define BSLF_RECTANGLE  (1L<<1)   // new version of layer with only influenced rectangle
#define BSLF_ALLDARK    (1L<<2)   // polygon is not lighted by the light at all
#define BSLF_ALLLIGHT   (1L<<3)   // whole polygon is lighted by the light (there are no shadows)
class ENGINE_API CBrushShadowLayer {
public:
// implementation:
  ULONG bsl_ulFlags;    // flags
  CListNode bsl_lnInShadowMap;    // node in list of all layers of a shadow map
  CListNode bsl_lnInLightSource;  // node in list of all layers of a light source
  class CBrushShadowMap *bsl_pbsmShadowMap; // the shadow map
  class CLightSource *bsl_plsLightSource;   // the light source
  PIX bsl_pixMinU;  // rectangle where the light influences the polygon
  PIX bsl_pixMinV;
  PIX bsl_pixSizeU;
  PIX bsl_pixSizeV;
  SLONG bsl_slSizeInPixels; // size of bit mask in pixels (with all mip-maps)
  UBYTE *bsl_pubLayer;  // bit mask set where the polygon is lighted
  COLOR bsl_colLastAnim;  // last animating color cached

// interface:
  CBrushShadowLayer();
  ~CBrushShadowLayer(void);
  // discard shadows but keep the layer
  void DiscardShadows(void);
  // get shadow/light percentage at given coordinates in shadow layer
  FLOAT GetLightStrength(PIX pixU, PIX pixV, FLOAT fLt, FLOAT fUp);

  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
};


class ENGINE_API CBrushShadowMap : public CShadowMap {
public:
// implementation:
  // for linking in list of all shadow maps that need calculation
  CListNode bsm_lnInUncalculatedShadowMaps;

  CListHead bsm_lhLayers;     // list of all layers of this shadow map
  UBYTE *bsm_pubPolygonMask;  // bit packed polygon mask

  // get pointer to embedding brush polygon
  inline CBrushPolygon *GetBrushPolygon(void);

  // overrides from CShadowMap:
  // mix all layers into cached shadow map
  virtual void MixLayers(INDEX iFirstMip, INDEX iLastMip, BOOL bDynamic=FALSE);  // iFirstMip<iLastMip
  // read/write layers from/to stream
  virtual void ReadLayers_t( CTStream *pstrm);  // throw char *
  virtual void WriteLayers_t( CTStream *pstrm); // throw char *
  // check if all layers are up to date
  virtual void CheckLayersUpToDate(void);
  // test if there is any dynamic layer
  virtual BOOL HasDynamicLayers(void);

  // returns TRUE if shadowmap is all flat along with colFlat variable set to that color
  virtual BOOL IsShadowFlat( COLOR &colFlat);

  // calculate the rectangle where a light influences the shadow map
  void FindLightRectangle(CLightSource &ls, class CLightRectangle &lr);
  // queue the shadow map for calculation
  void QueueForCalculation(void);
// interface:
  // constructor
  CBrushShadowMap(void);
  // destructor
  ~CBrushShadowMap(void);
  // discard all layers on this shadow map
  void DiscardAllLayers(void);
  // discard shadows on all layers on this shadow map
  void DiscardShadows(void);
  // remove shadow layers without valid light source
  void RemoveDummyLayers(void);

  // get number of shadow layers
  INDEX GetShadowLayersCount(void) { return bsm_lhLayers.Count(); };
  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
};


// one texture on a brush polygon
#define BPTF_CLAMPU         (1U<<0)    // clamp u coordinate in texture
#define BPTF_CLAMPV         (1U<<1)    // clamp v coordinate in texture
#define BPTF_DISCARDABLE    (1U<<2)    // texture doesn't have to be drawn
#define BPTF_AFTERSHADOW    (1U<<3)    // texture is to be applied after shadow
#define BPTF_REFLECTION     (1U<<4)    // texture will be reflection-mapped

// first few blending type must be these:
#define BPT_BLEND_OPAQUE  0
#define BPT_BLEND_SHADE   1
#define BPT_BLEND_BLEND   2
#define BPT_BLEND_ADD     3

class ENGINE_API CBrushPolygonTexture {
public:
  CTextureObject bpt_toTexture;     // texture object
  CMappingDefinition bpt_mdMapping; // mapping of texture on polygon

  union {
    struct {
      UBYTE bpt_ubScroll;               // texture scroll
      UBYTE bpt_ubBlend;                // type of texture blending used
      UBYTE bpt_ubFlags;                // additional flags
      UBYTE bpt_ubDummy;                // unused (alignment)
      COLOR bpt_colColor;               // defines constant color and alpha of polygon
    } s;
    UBYTE bpt_auProperties[8];
  };

  // ATTENTION! If you add/edit/remove any data member, PLEASE update the
  //  operator = method, below!  --ryan.
  CBrushPolygonTexture& operator =(const CBrushPolygonTexture &src)
  {
    if (this != &src)
    {
      bpt_toTexture = src.bpt_toTexture;
      bpt_mdMapping = src.bpt_mdMapping;
      memcpy(&bpt_auProperties, &src.bpt_auProperties, sizeof (bpt_auProperties));
    }
    return *this;
  }


  CBrushPolygonTexture(void)
    {
        s.bpt_ubScroll = 0;
        s.bpt_ubBlend = 0;
        s.bpt_ubFlags = BPTF_DISCARDABLE;
        s.bpt_ubDummy = 0;
        s.bpt_colColor = 0xFFFFFFFF;
    }

  /* Copy polygon properties */
  CBrushPolygonTexture &CopyTextureProperties(CBrushPolygonTexture &bptOther, BOOL bCopyMapping) {
    bpt_toTexture.SetData( bptOther.bpt_toTexture.GetData());
    s.bpt_ubScroll = bptOther.s.bpt_ubScroll;
    s.bpt_ubBlend = bptOther.s.bpt_ubBlend;
    s.bpt_ubFlags = bptOther.s.bpt_ubFlags;
    s.bpt_ubDummy = bptOther.s.bpt_ubDummy;
    s.bpt_colColor = bptOther.s.bpt_colColor;
    if( bCopyMapping) bpt_mdMapping = bptOther.bpt_mdMapping;
    return *this;
  };
  void Clear(void) {
    bpt_toTexture.SetData(NULL);
  };
  // read/write to stream
  void Read_t( CTStream &strm); // throw char *
  void Write_t( CTStream &strm);  // throw char *

  // get amount of memory used by this object
  inline SLONG GetUsedMemory(void) { return sizeof(CBrushPolygonTexture); };
};


// a polygon in brush
// Flags
// flags 0-2 are used by CObjectPolygon, flags 3-31 are used by CBrushPolygon
// OPOF_PORTAL  // set if the polygon is a portal - used for CSG in CObject3D
#define BPOF_DOUBLESIDED            (1UL<< 3)    // polygon is renderable from both sides
#define BPOF_SHOOTTHRU              (1UL<< 4)    // physical ray-casts can pass through the polygon, even if it is not passable
#define BPOF_TRANSPARENT            (1UL<< 5)    // render with alpha-testing and write z-buffer
#define BPOF_RENDERASPORTAL         (1UL<< 6)    // internal used in rendering
#define BPOF_STAIRS                 (1UL<< 7)    // polygon is part of a staircase
#define BPOF_SELECTED               (1UL<< 8)    // set if the polygon is selected
#define BPOF_SELECTEDFORCSG         (1UL<< 9)    // set if the polygon is selected for CSG
#define BPOF_WASPORTAL              (1UL<<10)    // set if it was portal before CSG
#define BPOF_PASSABLE               (1UL<<11)    // set if not a physical barrier
#define BPOF_DOESNOTCASTSHADOW      (1UL<<12)    // set to make a wall passable for light beams
#define BPOF_WASBRUSHPOLYGON        (1UL<<13)    // this polygon was brush polygon before (not just created)
#define BPOF_FULLBRIGHT             (1UL<<14)    // set to make a wall full-bright
#define BPOF_TRANSLUCENT            (1UL<<15)    // set for translucent portals
#define BPOF_HASDIRECTIONALLIGHT    (1UL<<16)    // set if polygon accepts directional lights
#define BPOF_INVISIBLE              (1UL<<17)    // set if the polygon is ignored during rendering
#define BPOF_DARKCORNERS            (1UL<<18)    // polygons will have dark corners (here was gouraud!!!!)
#define BPOF_RENDERTRANSLUCENT      (1UL<<19)    // internal used in rendering
#define BPOF_NOPLANEDIFFUSION       (1UL<<20)    // plane normal is ignored when shading
#define BPOF_DETAILPOLYGON          (1UL<<21)    // not used for visibility determination
#define BPOF_PORTAL                 (1UL<<22)    // should behave like a portal (not same as OPOF_PORTAL)
#define BPOF_ACCURATESHADOWS        (1UL<<23)    // shadows are calculated for each mip-map independently
#define BPOF_HASDIRECTIONALAMBIENT  (1UL<<24)    // set if polygon accepts directional ambient
#define BPOF_MARKEDLAYER            (1UL<<25)    // used in FindShadowLayers()
#define BPOF_DYNAMICLIGHTSONLY      (1UL<<26)    // only dynamic lights used in shadow map
#define BPOF_DOESNOTRECEIVESHADOW   (1UL<<27)    // has lightmap, but no shadows on it
#define BPOF_NODYNAMICLIGHTS        (1UL<<28)    // dynamic lights do not influence it
#define BPOF_INVALIDTRIANGLES       (1UL<<29)    // polygons could not be triangulated well
#define BPOF_OCCLUDER               (1UL<<30)    // occluder polygon
#define BPOF_MARKED_FOR_USE         (1UL<<31)    // used in triangularization when polygon vertex is moved

#define BPOF_MASK_FOR_COPYING \
  (BPOF_PASSABLE|BPOF_DOESNOTCASTSHADOW|BPOF_FULLBRIGHT|BPOF_TRANSLUCENT|BPOF_TRANSPARENT|\
   BPOF_HASDIRECTIONALLIGHT|BPOF_INVISIBLE|BPOF_NOPLANEDIFFUSION|\
   BPOF_DETAILPOLYGON|BPOF_PORTAL|BPOF_ACCURATESHADOWS|BPOF_HASDIRECTIONALAMBIENT|\
   BPOF_DYNAMICLIGHTSONLY|BPOF_DOESNOTRECEIVESHADOW|BPOF_NODYNAMICLIGHTS|BPOF_DARKCORNERS|BPOF_OCCLUDER)

// properties that are retained in conversions to/from CObjectPolygon
struct CBrushPolygonProperties {
  UBYTE bpp_ubSurfaceType;        // surface type on this polygon
  UBYTE bpp_ubIlluminationType;   // type of illuminating polygon, 0 if not illuminating
  UBYTE bpp_ubShadowBlend;        // type of texture blending used for shadow map
  UBYTE bpp_ubMirrorType;         // mirror or warp
  UBYTE bpp_ubGradientType;       // for gradiental shadows
  SBYTE bpp_sbShadowClusterSize;  // size of shadow clusters (size=(1<<ub)*0.5m)
  UWORD bpp_uwPretenderDistance;  // distance for pretender switching [m]
  /* Default constructor. */
  CBrushPolygonProperties(void) { memset(this, 0, sizeof(*this)); };
#ifdef PLATFORM_UNIX
  friend __forceinline CTStream &operator>>(CTStream &strm, CBrushPolygonProperties &cbpp)
  {
    strm>>cbpp.bpp_ubSurfaceType;
    strm>>cbpp.bpp_ubIlluminationType;
    strm>>cbpp.bpp_ubShadowBlend;
    strm>>cbpp.bpp_ubMirrorType;
    strm>>cbpp.bpp_ubGradientType;
    strm>>cbpp.bpp_sbShadowClusterSize;
    strm>>cbpp.bpp_uwPretenderDistance;
    return strm;
  }
  friend __forceinline CTStream &operator<<(CTStream &strm, const CBrushPolygonProperties &cbpp)
  {
    strm<<cbpp.bpp_ubSurfaceType;
    strm<<cbpp.bpp_ubIlluminationType;
    strm<<cbpp.bpp_ubShadowBlend;
    strm<<cbpp.bpp_ubMirrorType;
    strm<<cbpp.bpp_ubGradientType;
    strm<<cbpp.bpp_sbShadowClusterSize;
    strm<<cbpp.bpp_uwPretenderDistance;
    return strm;
  }
#endif
};

class ENGINE_API CBrushPolygon {
public:
// implementation:
  /* Calculate area of the polygon. */
  DOUBLE CalculateArea(void);
  /* Calculate bounding box of this polygon. */
  void CalculateBoundingBox(void);
  /* Create a BSP polygon from this polygon. */
  void CreateBSPPolygon(BSPPolygon<DOUBLE, 3> &bspo);
  void CreateBSPPolygonNonPrecise(BSPPolygon<DOUBLE, 3> &bspo);
  void CreateBSPPolygon(BSPPolygon<FLOAT, 3> &bspo);
  void CreateBSPPolygonNonPrecise(BSPPolygon<FLOAT, 3> &bspo);
  /* Create shadow map for the polygon. */
  void MakeShadowMap(CWorld *pwoWorld, BOOL bDoDirectionalLights);
  /* Initialize shadow map for the polygon. */
  void InitializeShadowMap(void);
  // discard all cached shading info for models
  void DiscardShadingInfos(void);
  // move edges from another polygon into this one
  void MovePolygonEdges(CBrushPolygon &bpoSource);
  /* Test if this polygon touches another one. */
  BOOL TouchesInSameSector(CBrushPolygon &bpoOther);
  BOOL TouchesInAnySector(CBrushPolygon &bpoOther);
  // make triangular representation of the polygon
  void Triangulate(void);
public:
// interface:
  FLOATaabbox3D bpo_boxBoundingBox;           // bounding box
  ULONG bpo_ulFlags;                          // flags

  CBrushPlane *bpo_pbplPlane;                 // plane of this polygon
  CStaticArray<CBrushPolygonEdge> bpo_abpePolygonEdges;   // edges in this polygon
  CStaticArray<CBrushVertex *> bpo_apbvxTriangleVertices; // triangle vertices
  CStaticArray<INDEX> bpo_aiTriangleElements; // element indices inside vertex arrays
  CBrushPolygonTexture bpo_abptTextures[3];   // texture on this polygon
  COLOR bpo_colColor;                         // color of this polygon
  COLOR bpo_colShadow;                        // color of shadow on this polygon
  CBrushShadowMap bpo_smShadowMap;            // shadow map of this polygon
  CMappingDefinition bpo_mdShadow;            // mapping of shadow on polygon
  CBrushPolygonProperties bpo_bppProperties;  // additional properties
  class CScreenPolygon *bpo_pspoScreenPolygon;  // used in rendering

  CBrushSector *bpo_pbscSector;               // sector of this polygon

  CRelationSrc bpo_rsOtherSideSectors;        // relation to sectors on other side of portal
  CListHead bpo_lhShadingInfos;               // for linking shading infos of entities
  INDEX bpo_iInWorld;   // index of the polygon in entire world

  /* Default constructor. */
  inline CBrushPolygon(void) : bpo_ulFlags(0) {};
  /* Clear the object. */
  void Clear(void);
  /* Destructor. */
  inline ~CBrushPolygon(void) { Clear(); };
  CBrushPolygon &CopyPolygon(CBrushPolygon &bp);
  /* Copy polygon within same sector. */
  void CopyFromSameSector(CBrushPolygon &bpoOriginal);
  
  /* Copy polygon properties */
  CBrushPolygon &CopyProperties(CBrushPolygon &bpoOther, BOOL bCopyMapping = TRUE);
  /* Copy polygon properties without texture */
  CBrushPolygon &CopyPropertiesWithoutTexture(CBrushPolygon &bpoOther);
  /* Copy polygon's textures */
  CBrushPolygon &CopyTextures(CBrushPolygon &bpoOther);
  
  // polygons may be selected
  IMPLEMENT_SELECTING(bpo_ulFlags)

  /* Select group of adjacent polygons with same color. */
  void SelectSimilarByColor(CSelection<CBrushPolygon, BPOF_SELECTED> &selbpoSimilar);
  /* Select group of adjacent polygons with same texture. */
  void SelectSimilarByTexture(CSelection<CBrushPolygon, BPOF_SELECTED> &selbpoSimilar, INDEX iTexture);
  /* Select all polygons in sector with same texture. */
  void SelectByTextureInSector(CSelection<CBrushPolygon, BPOF_SELECTED> &selbpoSimilar, INDEX iTexture);
  /* Select all polygons in sector with same color. */
  void SelectByColorInSector(CSelection<CBrushPolygon, BPOF_SELECTED> &selbpoSimilar);

  /* Discard shadows on the polygon. */
  void DiscardShadows(void);

  // find minimum distance of a given point from the polygon edges
  FLOAT GetDistanceFromEdges(const FLOAT3D &v);

  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
}
#ifdef __arm__
__attribute__((aligned(64)))
#endif
;

// get pointer to embedding brush polygon
inline CBrushPolygon *CBrushShadowMap::GetBrushPolygon(void) {
  return (CBrushPolygon *) ((UBYTE*)this-_offsetof(CBrushPolygon, bpo_smShadowMap));
  return(NULL);
}


// selection of brush polygons
typedef CSelection<CBrushPolygon, BPOF_SELECTED>       CBrushPolygonSelection;
// selection of brush polygons used for CSG
typedef CSelection<CBrushPolygon, BPOF_SELECTEDFORCSG> CBrushPolygonSelectionForCSG;

// sector flags
#define BSCF_SELECTED           (1L<<0)   // set if the sector is selected
#define BSCF_HIDDEN             (1L<<1)   // set if the sector is hidden (for editing)
#define BSCF_SELECTEDFORCSG     (1L<<2)   // set if the sector is selected for CSG
#define BSCF_OPENSECTOR         (1L<<3)   // set if the sector polygons are facing outwards
#define BSCF_NEARTESTED         (1L<<4)   // already tested for near polygon
#define BSCF_INVISIBLE          (1L<<5)   // active, but not visible
#define BSCF_RAYTESTED          (1L<<6)   // already tested by ray
#define BSCF_NEEDSCLIPPING      (1L<<7)   // set if its polygons needs clipping
#define BSCB_CONTENTTYPE  24      // upper 8 bits are used for sector content type
#define BSCB_FORCETYPE    16      // next 8 bits are used for sector gravity type
#define BSCB_FOGTYPE      12      // 4 bits for fog
#define BSCB_HAZETYPE      8      // 4 bits for haze

#define BSCB2_ENVIRONMENTTYPE      0      // 8 bits for environment sound effects (EAX and similar)
#define BSCF2_VISIBILITYINCLUDE (1<<8)    // toggle include/exclude for visibility (exclude is default)

// vis flags
#define VISM_INCLUDEEXCLUDE   (0x0000FFFF)  // for visibility include/exclude
#define VISM_DONTCLASSIFY     (0xFFFF0000)  // to disable classification in certain sectors

// NOTE on how visibility tweaks are implemented
// - low 16 bits determine visibility from current sector
//   if BSCF2_VISIBILITYINCLUDE is on, entity is visible from sector only if EntityVisTweaks&SectorVisFlags&VISM_INCLUDEEXCLUDE
//   if BSCF2_VISIBILITYINCLUDE is off, entity is visible from sector only if !(EntityVisTweaks&SectorVisFlags&VISM_INCLUDEEXCLUDE)
// - high 16 bits limit entity classification to sectors
//   if EntityVisTweaks&SectorVisFlags&VISM_DONTCLASSIFY, entity is treated as if not classified to that sector

// temporary flags
#define BSCTF_PRELOADEDBSP       (1L<<0)   // bsp is loaded, no need to calculate it
#define BSCTF_PRELOADEDLINKS     (1L<<1)   // portallinks are loaded, no need to calculate them

// a sector in brush
class ENGINE_API CBrushSector {
public:
// implementation:
  /* Fill an object sector from a sector in brush. */
  void ToObjectSector(CObjectSector &osc);
  /* Fill a brush sector from a sector in object3d. */
  void FromObjectSector_t(CObjectSector &osc); // throw char *
  // recalculate planes for polygons from their vertices
  void MakePlanesFromVertices();
  // update changed sector's data after dragging vertices or importing
  void UpdateSector(void);

  /* Calculate volume of the sector. */
  DOUBLE CalculateVolume(void);
  // make triangular representation of the polygons in the sector
  void Triangulate(void);
public:
// implementation:
  CStaticArray<CBrushVertex> bsc_abvxVertices;  // vertices
  CStaticArray<CBrushEdge> bsc_abedEdges;       // edges
  CStaticArray<CBrushPlane> bsc_abplPlanes;     // planes
  CStaticArray<CBrushPolygon> bsc_abpoPolygons; // polygons

  CStaticArray<CWorkingVertex> bsc_awvxVertices;  // working vertices
  CStaticArray<CWorkingPlane> bsc_awplPlanes;     // working planes
  CStaticArray<CWorkingEdge> bsc_awedEdges;       // working edges

  class CBrushMip *bsc_pbmBrushMip;                   // pointer to brush mip of this sector
  COLOR bsc_colColor;                                 // color of this sector
  COLOR bsc_colAmbient;                               // ambient light for that sector
  ULONG bsc_ulFlags;                                  // flags
  ULONG bsc_ulFlags2;                                 // second set of flags
  ULONG bsc_ulTempFlags;                              // flags that are not saved
  ULONG bsc_ulVisFlags;                               // special visibility flags
  FLOATaabbox3D bsc_boxBoundingBox;                   // bounding box in absolute space
  FLOATaabbox3D bsc_boxRelative;                      // bounding box in relative space
  CListNode bsc_lnInActiveSectors; // node in sectors active in some operation (e.g. rendering)
  DOUBLEbsptree3D &bsc_bspBSPTree;  // the local bsp tree of the sector
  CRelationDst bsc_rdOtherSidePortals;  // relation to portals pointing to this sector
  CRelationSrc bsc_rsEntities;     // relation to all entities in this sector
  CTString bsc_strName;   // sector name
  INDEX bsc_iInWorld;   // index of the sector in entire world
  CRelationLnk *bsc_prlLink;    // for optimized link removal
  INDEX bsc_ispo0;   // screen polygons used in rendering
  INDEX bsc_ctspo;
  INDEX bsc_ivvx0;   // view vertices used in rendering

  /* Default constructor. */
  CBrushSector(void);
  ~CBrushSector(void);
  DECLARE_NOCOPYING(CBrushSector);

  /* Clear the object. */
  void Clear(void);
  /* Lock all arrays. */
  void LockAll(void);
  /* Unlock all arrays. */
  void UnlockAll(void);

  /* Update sector after moving vertices */
  void UpdateVertexChanges(void);
  // triangularize given polygon
  void TriangularizePolygon( CBrushPolygon *pbpo);
  /* Triangularize polygons contining vertices from selection */
  void TriangularizeForVertices( CBrushVertexSelection &selVertex);
  // Triangularize marked polygons
  void TriangularizeMarkedPolygons( void);
  // Subdivide given triangles
  void SubdivideTriangles( CBrushPolygonSelection &selPolygon);
  // Insert given vertex into triangle
  void InsertVertexIntoTriangle( CBrushPolygonSelection &selPolygon, FLOAT3D vVertex);
  // Retriple two triangles given trough selection
  BOOL IsReTripleAvailable( CBrushPolygonSelection &selPolygon);
  void ReTriple( CBrushPolygonSelection &selPolygon);

  /* Calculate bounding boxes of all polygons. */
  void CalculateBoundingBoxes(CSimpleProjection3D_DOUBLE &prRelativeToAbsolute);

  // sectors may be selected
  IMPLEMENT_SELECTING(bsc_ulFlags)

  // overrides from CSerial
  /* Read from stream. */
  void Read_t( CTStream *istrFile); // throw char *
  /* Write to stream. */
  void Write_t( CTStream *ostrFile);  // throw char *

  /* Uncache lightmaps on all shadows on the sector. */
  void UncacheLightMaps(void);
  /* Find and remember all entities in this sector. */
  void FindEntitiesInSector(void);

  /* Get/set properties. */
  inline INDEX GetContentType(void) {
    return (bsc_ulFlags>>BSCB_CONTENTTYPE)&0xff;
  }
  void SetContentType(INDEX iNewContent)
  {
    iNewContent&=0xff;
    bsc_ulFlags &= ~(0xff<<BSCB_CONTENTTYPE);
    bsc_ulFlags |= (iNewContent<<BSCB_CONTENTTYPE);
  }
  INDEX GetForceType(void) {
    return (bsc_ulFlags>>BSCB_FORCETYPE)&0xff;
  }
  void SetForceType(INDEX iNewForce) {
    iNewForce&=0xff;
    bsc_ulFlags &= ~(0xff<<BSCB_FORCETYPE);
    bsc_ulFlags |= (iNewForce<<BSCB_FORCETYPE);
  }
  INDEX GetFogType(void) {
    return (bsc_ulFlags>>BSCB_FOGTYPE)&0xf;
  }
  void SetFogType(INDEX iNewForce) {
    iNewForce&=0xf;
    bsc_ulFlags &= ~(0xf<<BSCB_FOGTYPE);
    bsc_ulFlags |= (iNewForce<<BSCB_FOGTYPE);
  }
  INDEX GetHazeType(void) {
    return (bsc_ulFlags>>BSCB_HAZETYPE)&0xf;
  }
  void SetHazeType(INDEX iNewForce) {
    iNewForce&=0xf;
    bsc_ulFlags &= ~(0xf<<BSCB_HAZETYPE);
    bsc_ulFlags |= (iNewForce<<BSCB_HAZETYPE);
  }
  inline INDEX GetEnvironmentType(void) {
    return (bsc_ulFlags2>>BSCB2_ENVIRONMENTTYPE)&0xFF;
  }
  void SetEnvironmentType(INDEX iNewEnvironment)
  {
    iNewEnvironment&=0xFF;
    bsc_ulFlags2 &= ~(0xFF<<BSCB2_ENVIRONMENTTYPE);
    bsc_ulFlags2 |= (iNewEnvironment<<BSCB2_ENVIRONMENTTYPE);
  }

  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
};


// selection of brush sectors
typedef CSelection<CBrushSector, BSCF_SELECTED>       CBrushSectorSelection;
// selection of brush sectors used for CSG
typedef CSelection<CBrushSector, BSCF_SELECTEDFORCSG> CBrushSectorSelectionForCSG;

/*
 * One detail level of a brush.
 */
class ENGINE_API CBrushMip {
public:
// implementation:
  CDynamicArray<CBrushSector> bm_abscSectors;         // sectors
  CBrush3D *bm_pbrBrush;                              // pointer to brush

  /* Select all sectors within a range. */
  void SelectSectorsInRange(CBrushSectorSelectionForCSG &selbscInRange, FLOATaabbox3D boxRange);
  void SelectSectorsInRange(CBrushSectorSelection &selbscInRange, FLOATaabbox3D boxRange);
  /* Select all sectors in brush. */
  void SelectAllSectors(CBrushSectorSelectionForCSG &selbscAll);
  void SelectAllSectors(CBrushSectorSelection &selbscAll);
  /* Select open sector in brush. */
  void SelectOpenSector(CBrushSectorSelectionForCSG &selbscOpen);
  /* Select closed sectors in brush. */
  void SelectClosedSectors(CBrushSectorSelectionForCSG &selbscClosed);
  /* Fill a 3d object from a selection in a brush. */
  void ToObject3D(CObject3D &ob, CBrushSectorSelection &selbscToCopy);
  void ToObject3D(CObject3D &ob, CBrushSectorSelectionForCSG &selbscToCopy);
  /* Add an object3d to brush. (returns pointer to the first created sector) */
  CBrushSector *AddFromObject3D_t(CObject3D &ob); // throw char *
  /* Delete all sectors in a selection. */
  void DeleteSelectedSectors(CBrushSectorSelectionForCSG &selbscToDelete);
  /* Spread all brush mips after this one. */
  void SpreadFurtherMips(void);
  /* Reoptimize all sectors in the brush mip. */
  void Reoptimize(void);
  /* Find all portals that have no links and kill their portal flag. */
  void RemoveDummyPortals(BOOL bClearPortalFlags);
public:
// interface:
  CListNode bm_lnInBrush;     // for linking in list of mip-brushes for a brush
  FLOAT bm_fMaxDistance;     // distance after which this mip-brush is not displayed
  FLOATaabbox3D bm_boxBoundingBox;  // bounding box of entire mip-brush in absolute space
  FLOATaabbox3D bm_boxRelative;     // bounding box of entire mip-brush in relative space

  /* Constructor. */
  CBrushMip(void);
  /* Free all memory and leave empty brush mip. */
  void Clear(void);
  /* Fill a brush mip from 3d object. */
  void FromObject3D_t(CObject3D &ob); // throw char *
  /* Copy brush mip from another brush mip. */
  void Copy(CBrushMip &bmOther, FLOAT fStretch, BOOL bMirrorX);

  /* Set mip distance of this mip, spread all that are further. */
  void SetMipDistance(FLOAT fMipDistance);
  /* Get mip factor of this mip. */
  FLOAT GetMipDistance(void);
  /* Get mip index of this mip. */
  INDEX GetMipIndex(void);
  // get next brush mip
  CBrushMip *GetNext(void);
  // get previous brush mip
  CBrushMip *GetPrev(void);
  // check if this is the first mip in this brush
  inline BOOL IsFirstMip(void);

  // overrides from CSerial
  /* Read from stream. */
  void Read_new_t( CTStream *istrFile); // throw char *
  void Read_old_t( CTStream *istrFile); // throw char *
  /* Write to stream. */
  void Write_t( CTStream *ostrFile);  // throw char *

  /* Update bounding box from bounding boxes of all sectors. */
  void UpdateBoundingBox(void);
  /* Calculate bounding boxes in all sectors. */
  void CalculateBoundingBoxes(CSimpleProjection3D_DOUBLE &prRelativeToAbsolute);
};

/*
 * Brush class -- a piece of level that can be moved independently
 */
#define BRF_DRAWSELECTED    (1L<<0)   // internal marker for rendering selected brushes
#define BRF_DRAWFIRSTMIP    (1L<<1)   // viewer is inside this brush

class ENGINE_API CBrush3D : public CBrushBase {
public:
// implementation:
  CListNode br_lnInActiveBrushes;     // for linking in list of active brushes in renderer
  CAnyProjection3D br_prProjection;   // projection currently used by this brush
  CEntity *br_penEntity;              // back pointer from brush to its entity
  class CFieldSettings *br_pfsFieldSettings;// field settings for field brushes
  ULONG br_ulFlags;                   // brush flags

  /* Wrapper for CObject3D::Optimize(), updates profiling information. */
  static void OptimizeObject3D(CObject3D &ob);

  /* Prepare a projection from brush space to absolute space. */
  void PrepareRelativeToAbsoluteProjection(CSimpleProjection3D_DOUBLE &prRelativeToAbsolute);
  /* Calculate bounding boxes in all brush mips. */
  void CalculateBoundingBoxes(void);
  void CalculateBoundingBoxesForOneMip(CBrushMip *pbmOnly);  // for only one mip
  INDEX GetBrushType() { return CBrushBase::BT_BRUSH3D; }    // this is brush not terrain

public:
// interface:
  CListHead br_lhBrushMips;           // mip brushes in this brush

  // constructor
  CBrush3D(void);
  // destructor
  ~CBrush3D(void);

  /* Free all memory and leave empty brush. */
  void Clear(void);
  /* Fill a brush from 3d object. */
  void FromObject3D_t(CObject3D &ob); // throw char *
  void AddMipBrushFromObject3D_t(CObject3D &ob, FLOAT fSwitchDistance); // throw char *
  /* Copy brush from another brush with possible mirror and stretch. */
  void Copy(CBrush3D &brOther, FLOAT fStretch, BOOL bMirrorX);

  /* Delete a brush mip with given factor. */
  void DeleteBrushMip(CBrushMip *pbmToDelete);
  /* Create a new brush mip. */
  CBrushMip *NewBrushMipAfter(CBrushMip *pbm, BOOL bCopy);
  CBrushMip *NewBrushMipBefore(CBrushMip *pbm, BOOL bCopy);
  /* Get a brush mip for given mip-factor. */
  CBrushMip *GetBrushMipByDistance(FLOAT fDistance);
  /* Get a brush mip by its given index. */
  CBrushMip *GetBrushMipByIndex(INDEX iMip);
  // get first brush mip
  CBrushMip *GetFirstMip(void);
  // get last brush mip
  CBrushMip *GetLastMip(void);

  // switch from zoning to non-zoning
  void SwitchToNonZoning(void);
  // switch from non-zoning to zoning
  void SwitchToZoning(void);

  // overrides from CSerial
  /* Read from stream. */
  void Read_t( CTStream *istrFile); // throw char *
  void Read_new_t( CTStream *istrFile); // throw char *
  void Read_old_t( CTStream *istrFile); // throw char *
  /* Write to stream. */
  void Write_t( CTStream *ostrFile);  // throw char *
};

// check if this is the first mip in this brush
inline BOOL CBrushMip::IsFirstMip(void) {
  return &bm_pbrBrush->br_lhBrushMips.IterationHead() == &bm_lnInBrush;
};



#endif  /* include-once check. */

