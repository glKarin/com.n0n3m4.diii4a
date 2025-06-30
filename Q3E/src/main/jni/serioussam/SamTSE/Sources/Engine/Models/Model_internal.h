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

#ifndef SE_INCL_MODEL_INTERNAL_H
#define SE_INCL_MODEL_INTERNAL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/AABBox.h>
#include <Engine/Graphics/RenderPoly.h>

#define MODEL_VERSION_WITHOUT_STRETCH_CENTER "V002"
#define MODEL_VERSION_WITHOUT_MULTIPLE_COLLISION_BOXES "V003"
#define MODEL_VERSION_WITHOUT_ATTACHED_POSITIONS "V004"
#define MODEL_VERSION_WITHOUT_POLYGONAL_PATCHES "V005"
#define MODEL_VERSION_WITHOUT_POLYGONS_PER_SURFACE "V006"
#define MODEL_VERSION_WITHOUT_16_BIT_COMPRESSION "V007"
#define MODEL_VERSION_WITHOUT_REFLECTION_AND_SPECULARITY "V008"
#define MODEL_VERSION_WITHOUT_DIFFUSE_COLOR "V009"
#define MODEL_VERSION "V010"


// surface shading types
enum SurfaceShadingType {
  SST_INVALID    = -1,
  SST_FULLBRIGHT =  0,
  SST_MATTE      =  1,
  SST_FLAT       =  2,
};

// surface translucency types
enum SurfaceTranslucencyType {
  STT_INVALID      = -1,
  STT_OPAQUE       =  0,
  STT_TRANSLUCENT  =  1,
  STT_ALPHAGOURAUD =  2,  // obsolete!
  STT_ADD          =  3,
  STT_TRANSPARENT  =  4,
  STT_MULTIPLY     =  5,
};                   

// surface rendering flags
#define SRF_DOUBLESIDED         (1L<<0)     // double sided polygons
#define SRF_CLIPPOLYGON         (1L<<1)     // polygons are allways cliped, this is old flag
#define SRF_REFLECTIONS         (1L<<2)     // use reflection mapping on this surface
#define SRF_INVISIBLE           (1L<<3)     // if this surface is ignored in rendering process
#define SRF_SPECULAR            (1L<<4)     // use specularity on this surface
#define SRF_DIFFUSE             (1L<<5)     // use difuse texture for this surface
#define SRF_BUMP                (1L<<6)     // use bump on this surface
#define SRF_NEW_TEXTURE_FORMAT  (1L<<7)     // used for reading of old mapping files (ther don't have diffuse flag set)
#define SRF_CYLINDRICAL_MAPPING (1L<<8)     // surface is mapped using spherical mapping
#define SRF_SPHERICAL_MAPPING   (1L<<9)     // surface is mapped using cylindrical mapping
#define SRF_FOG                 (1L<<10)    // used internally for fog
#define SRF_HAZE                (1L<<11)    // used internally for haze
#define SRF_DETAIL              (1L<<12)    // use detail on this surface instead of bump
#define SRF_SELECTED            (1L<<13)    // for editing purposes
#define SRF_OPAQUE              (1L<<14)    // writes to z-buffer
// ! WARNING: this flags are copied to mipinfo var flags which has reserved bits 25 and above !


// constant used for spreading mip-switching distances
#define MAX_SWITCH_FACTOR 8.0f

// render defines for model
#define  RT_WIRE_ON         ((1L) << 0)
#define  RT_HIDDEN_LINES    ((1L) << 1)
//-------------------------------------
#define  RT_NO_POLYGON_FILL ((1L) << 2)
#define  RT_WHITE_TEXTURE   ((1L) << 3)
#define  RT_SURFACE_COLORS  ((1L) << 4)
#define  RT_ON_COLORS       ((1L) << 5)
#define  RT_OFF_COLORS      ((1L) << 6)
#define  RT_TEXTURE         ((1L) << 7)
//-------------------------------------
#define  RT_SHADING_NONE    ((1L) << 8)
#define  RT_SHADING_LAMBERT ((1L) << 9)
#define  RT_SHADING_PHONG   ((1L) <<10)
//-------------------------------------
#define  RT_SHADING_MASK  (RT_SHADING_NONE | RT_SHADING_LAMBERT | RT_SHADING_PHONG)
#define  RT_TEXTURE_MASK  (RT_NO_POLYGON_FILL | RT_WHITE_TEXTURE | RT_SURFACE_COLORS | \
                           RT_ON_COLORS | RT_OFF_COLORS | RT_TEXTURE)

// Model flags
#define MF_FACE_FORWARD      ((1L)<<0)
#define MF_REFLECTIONS       ((1L)<<1)
#define MF_REFLECTIONS_HALF  ((1L)<<2)
#define MF_HALF_FACE_FORWARD ((1L)<<3)
#define MF_COMPRESSED_16BIT  ((1L)<<4)
#define MF_STRETCH_DETAIL    ((1L)<<5)

// Mip model flags
#define MM_PATCHES_VISIBLE         ((1L)<<0)
#define MM_ATTACHED_MODELS_VISIBLE ((1L)<<1)


// colors used to represent on and off bits
ENGINE_API extern COLOR PaletteColorValues[];

typedef Vector<SWORD,3> SWPOINT3D;
typedef Vector<SBYTE,3> SBPOINT3D;
typedef Vector<MEX,2>	  MEX2D;


struct ENGINE_API ModelFrameVertex8 {
	SBPOINT3D mfv_SBPoint;													// 8-bit compressed vertex
	UBYTE mfv_NormIndex;
};

struct ENGINE_API ModelFrameVertex16_old {
	SWPOINT3D mfv_SWPoint;													// 16-bit compressed vertex
	UBYTE mfv_NormIndex;
};

struct ENGINE_API ModelFrameVertex16 {
	SWPOINT3D mfv_SWPoint;													// 16-bit compressed vertex
	UBYTE mfv_ubNormH, mfv_ubNormP;
};

static inline CTStream &operator>>(CTStream &strm, ModelFrameVertex16 &mfv)
{
    strm>>mfv.mfv_SWPoint;
	strm>>mfv.mfv_ubNormH;
    strm>>mfv.mfv_ubNormP;
    return strm;
}

static inline CTStream &operator<<(CTStream &strm, const ModelFrameVertex16 &mfv)
{
    strm<<mfv.mfv_SWPoint;
	strm<<mfv.mfv_ubNormH;
    strm<<mfv.mfv_ubNormP;
    return strm;
}

struct ENGINE_API ModelFrameInfo {
	FLOATaabbox3D mfi_Box;													// bounding box info for each frame
};

struct ENGINE_API ModelTextureVertex {
  ModelTextureVertex(void);
	FLOAT3D mtv_UVW;																// 3D coordinate of one texture vertex
	MEX2D mtv_UV;									  							  // U,V mapping coordinates
  union {
	  BOOL mtv_Done;																	// flag used for mapping
    INDEX mtv_iSurfaceVx;
  };
  INDEX mtv_iTransformedVertex;
  FLOAT3D mtv_vU, mtv_vV;                         // bump directions
};

static inline CTStream &operator>>(CTStream &strm, ModelTextureVertex &mtv)
{
    strm>>mtv.mtv_UVW;
    strm>>mtv.mtv_UV;
    strm>>mtv.mtv_iSurfaceVx;
    strm>>mtv.mtv_iTransformedVertex;
    strm>>mtv.mtv_vU;
    strm>>mtv.mtv_vV;
    return(strm);
}

static inline CTStream &operator<<(CTStream &strm, const ModelTextureVertex &mtv)
{
    strm<<mtv.mtv_UVW;
    strm<<mtv.mtv_UV;
    strm<<mtv.mtv_iSurfaceVx;
    strm<<mtv.mtv_iTransformedVertex;
    strm<<mtv.mtv_vU;
    strm<<mtv.mtv_vV;
    return(strm);
}

struct ENGINE_API ModelPolygonVertex
{
	struct TransformedVertexData *mpv_ptvTransformedVertex; // buffer where vertices really rotate
	struct ModelTextureVertex *mpv_ptvTextureVertex;		// needed by modeler to calculate U,V
	void Read_t( CTStream *istrFile);   // throw char *
	void Write_t( CTStream *ostrFile);  // throw char *
};

#define	SC_ALLWAYS_ON (1UL << 30)
#define	SC_ALLWAYS_OFF (1UL << 31)

struct ENGINE_API ModelPolygon
{
	ModelPolygon();                                 // constructor
	~ModelPolygon();                                // destructor
	CStaticArray<struct ModelPolygonVertex> mp_PolygonVertices;	// this polygon's vertices
	ULONG mp_RenderFlags;														// flags which define rendering of this polygon
	ULONG mp_ColorAndAlpha;							      			// color and global alpha for this polygon
	INDEX mp_Surface;																// in which surface this polygon belongs
	SLONG mp_slVisibility;                          // how this poligon is visible
  BOOL  mp_bClipped;                              // is polygon clipped
  void Read_t( CTStream *istrFile);       // throw char *
	void Write_t( CTStream *ostrFile);      // throw char *
};

struct ENGINE_API MappingSurface
{
  MappingSurface();                               // constructor
  ~MappingSurface();                              // destructor
  CTString ms_Name;																// name of this surface
  COLOR   ms_colColor;														// color of this surface
  FLOAT3D ms_vSurface2DOffset;										// center of this surface
  FLOAT3D ms_HPB;																	// orientation of this surface
  FLOAT   ms_Zoom;																// current zoom value
  FLOAT3D ms_vSurfaceCenterOffset;                // vector of surface center offseting

  COLOR ms_colDiffuse;														// difuse color of this surface
  COLOR ms_colReflections;												// reflections color of this surface
  COLOR ms_colSpecular;														// specular color of this surface
  COLOR ms_colBump;															  // bump color of this surface

  ULONG ms_ulOnColor;                             // on and off colors for surface
  ULONG ms_ulOffColor;

  ULONG ms_ulRenderingFlags;                           // surface rendering flags
  enum SurfaceShadingType      ms_sstShadingType;      // surface shading types
  enum SurfaceTranslucencyType ms_sttTranslucencyType; // surface translucency types
  CStaticArray<INDEX> ms_aiPolygons;	            // indices of all polygons in surface
  CStaticArray<INDEX> ms_aiTextureVertices;	      // indices of all texture vertices in surface

  // rendering data
  INDEX ms_iSrfVx0;  // first surface vertex
  INDEX ms_ctSrfVx;  // number of surface vertices
  INDEX ms_ctSrfEl;  // number of surface elemtns

  BOOL operator==(const MappingSurface &msOther) const;

  // convert old polygon flags from CTGfx into new rendering parameters
  void SetRenderingParameters(ULONG ulOldFlags);
  void Read_t( CTStream *istrFile, BOOL bReadPolygonsPerSurface, BOOL bReadSurfaceColors);       // throw char *
	void Write_t( CTStream *ostrFile);      // throw char *
  void ReadSettings_t( CTStream *istrFile);       // throw char *
	void WriteSettings_t( CTStream *ostrFile);      // throw char *
};


// struct containing list of polygons that are touched by current patch
struct ENGINE_API PolygonsPerPatch
{
	CStaticArray<INDEX> ppp_iPolygons;  // indices of polygons occupied by this patch
  // rendering info
  CStaticArray<UWORD> ppp_auwElements; // elements for drawing
  inline ~PolygonsPerPatch() { Clear();};
  inline void Clear() { ppp_iPolygons.Clear();};
};

/* rcg 10042001 removed anonymous structs, dangerous union. */
struct ENGINE_API TransformedVertexData {
  FLOAT3D tvd_TransformedPoint;                   // for transformed point vector
  PolyVertex2D tvd_pv2;         // vertex structure for software
  FLOAT tvd_fX, tvd_fY, tvd_fZ; // view space original coords
  FLOAT tvd_fU, tvd_fV;         // texture mapping temp vars for clipping purposes
  BOOL  tvd_bClipped;           // is clipped to near clip plane or screen boundaries?
};

class ENGINE_API CModelCollisionBox {
public:
  FLOAT3D mcb_vCollisionBoxMin;                   // min vector of collision box
  FLOAT3D mcb_vCollisionBoxMax;                   // max vector of collision box
  ULONG mcb_iCollisionBoxDimensionEquality;       // HEIGHT_EQ_WIDTH or LENGTH_EQ_WIDTH or LENGTH_EQ_HEIGHT
  CTString mcb_strName;                           // name of collision box (exported as define)
// functions
  CModelCollisionBox(void);
  void Read_t(CTStream *istrFile);
  void ReadName_t(CTStream *istrFile);
  void Write_t(CTStream *ostrFile);
  /* Clear the object. */
  inline void Clear(void) {};
};

class ENGINE_API CAttachedModelPosition {
public:
  CAttachedModelPosition(void);
  INDEX amp_iCenterVertex;
  INDEX amp_iFrontVertex;
  INDEX amp_iUpVertex;
  CPlacement3D amp_plRelativePlacement;
  void Read_t( CTStream *strFile); // throw char *
  void Write_t( CTStream *strFile); // throw char *
  inline void Clear(void) {}; // clear the object.
};


class ENGINE_API CMipInfo {
public:
	INDEX mi_TrianglesCt;													// how many triangles in this mip-model
	INDEX mi_PolygonsCt;													// how many polygons in this mip-model
	INDEX mi_VerticesCt;													// how many vertices in this mip-model
};

class ENGINE_API CModelInfo {
public:
	INDEX mi_VerticesCt;														// number of vertices in model
  INDEX mi_FramesCt;															// number of all frames used by this model
  INDEX mi_MipCt;																	// number of mip-models
  CMipInfo mi_MipInfos[MAX_MODELMIPS];	          // contains count of polygons and vertices
  CTFileName mi_PatchFiles[MAX_TEXTUREPATCHES];	    // and their names
  MEX mi_Width, mi_Height;												// size of mapping texture
  BOOL mi_Flags;      														// model flags (MF_...)
  SLONG mi_ShadowQuality;	  	  								  // how models will be shaded
  FLOAT3D mi_Stretch;															// stretch vector (static one, dynamic one is in model object)
};


#endif  /* include-once check. */

