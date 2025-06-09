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

#ifndef SE_INCL_MODELDATA_H
#define SE_INCL_MODELDATA_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/CTString.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Placement.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Models/Model.h>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Templates/StaticArray.h>


#define MMI_OPAQUE      (1L<<25)  // entire mip model writes to z-buffer
#define MMI_TRANSLUCENT (1L<<26)  // entire mip model doesn't write to z-buffer
// ! WARNING: to var that holds these flags is copied from surface flags which has reserved bits 20 and below !


class ENGINE_API CModelPatch {
public:
  CTString mp_strName;
  CTextureObject mp_toTexture;
  MEX2D mp_mexPosition;
  FLOAT mp_fStretch;

  CModelPatch(void);
  void Read_t( CTStream *strFile); // throw char *
  void Write_t( CTStream *strFile); // throw char *
};

struct ENGINE_API ModelMipInfo
{
  ModelMipInfo();                                 // constructor
  ~ModelMipInfo();                                // destructor
	INDEX mmpi_PolygonsCt;													// how many polygons in this mip-model
  ULONG mmpi_ulFlags;                             // flags for this mip model
  CStaticArray<struct PolygonsPerPatch>   mmpi_aPolygonsPerPatch; // for each patch info telling which polygons are occupied by that patch
	CStaticArray<struct ModelPolygon>       mmpi_Polygons;          // array of polygons
  CStaticArray<struct ModelTextureVertex> mmpi_TextureVertices;	  // with U,V and data for calculating U,V (for modeler) [???]
  CStaticArray<struct MappingSurface>     mmpi_MappingSurfaces;	  // with HPB, zoom, name of all surfaces

  // rendering info
  INDEX mmpi_ctMipVx;
  INDEX mmpi_ctSrfVx;                    // total number of surface vertices in this mip
  CStaticArray<UWORD>   mmpi_auwMipToMdl;   // model vertices used in this mip
  CStaticArray<UWORD>   mmpi_auwSrfToMip;     // surface vertices to mip vertices lookup
  CStaticArray<FLOAT2D> mmpi_avmexTexCoord;   // texture coordinates for each surface vertex
  CStaticArray<FLOAT3D> mmpi_avBumpU;   // bump directions for each surface vertex
  CStaticArray<FLOAT3D> mmpi_avBumpV;   // bump directions for each surface vertex
  ULONG mmpi_ulLayerFlags;              // all texture layers needed in this mip
  INDEX mmpi_ctTriangles;               // total triangles in this mip
  CStaticStackArray<INDEX_T> mmpi_aiElements;

	void Clear();								 // clears this mip model's arays and their sub-arrays, dealocates memory
	void Read_t( CTStream *istrFile, BOOL bReadPolygonalPatches, BOOL bReadPolygonsPerSurface,
    BOOL bReadSurfaceColors);  // throw char *
	void Write_t( CTStream *ostrFile); // throw char *
};

class ENGINE_API CModelData : public CAnimData {
public:
	INDEX md_VerticesCt;														// number of vertices in model
  INDEX md_FramesCt;															// number of all frames used by this model
	CStaticArray<struct ModelFrameVertex8> md_FrameVertices8;		// all frames and all their vertices [FramesCt*VerticesCt] (8-bit)
	CStaticArray<struct ModelFrameVertex16> md_FrameVertices16;	// same but 16-bit
  CStaticArray<struct ModelFrameInfo> md_FrameInfos;					// bounding box info [FramesCt]
  CStaticArray<FLOAT3D> md_MainMipVertices;				// global array with coordinates of all vertices of main mip-model, used for creating mip-models [VerticesCt]
  CStaticArray<struct TransformedVertexData> md_TransformedVertices;		// buffer with i,j,k, rotated and stretched gouraud normal [VerticesCt]
  CStaticArray<ULONG> md_VertexMipMask;						// array of vertice masks telling in which mip model some vertex exists [VerticesCt]
  INDEX md_MipCt;																	// number of mip-models
  float md_MipSwitchFactors[MAX_MODELMIPS];			// array of mip factors determing where mip-models switch
  struct ModelMipInfo md_MipInfos[MAX_MODELMIPS];	// contains how many polygons and which contains which vertices (different for each mip-model)
  MEX md_Width, md_Height;												// size of texture, used for checking validiti of surfaces
  CModelPatch md_mpPatches[MAX_TEXTUREPATCHES];    // patches themselfs
	CTString md_ColorNames[MAX_COLOR_NAMES];        // strings containing edited color names
  ULONG md_Flags;		      												// model flags (face froward, reflesction mapping)
  SLONG md_ShadowQuality;	          							// determines maximum shadow quality
	FLOAT3D md_Stretch;															// stretch vector (static one, dynamic one is in model object)
	FLOAT3D md_vCenter;															// center of object
  FLOAT3D md_vCompressedCenter;                   // compressed (0-255) center of object (handle)
  BOOL md_bHasAlpha;                              // set if any surface has alpha blending
  // array of collision boxes for this model
  CDynamicArray<CModelCollisionBox> md_acbCollisionBox;
  BOOL md_bCollideAsCube;                         // if model colide as stretched cube
  CDynamicArray<CAttachedModelPosition> md_aampAttachedPosition;
  BOOL md_bIsEdited;  // set if model is part of CEditModel object
  COLOR md_colDiffuse;
  COLOR md_colReflections;
  COLOR md_colSpecular;
  COLOR md_colBump;

  BOOL md_bPreparedForRendering;  // set if model is prepared for rendering
public:
	CModelData();
	~CModelData();
  DECLARE_NOCOPYING(CModelData);

  // reference counting (override from CAnimData)
  void RemReference_internal(void);

  void PtrsToIndices();
	void IndicesToPtrs();
  void SpreadMipSwitchFactors( INDEX iFirst, float fStartingFactor); // spreads mip switch factors
  void GetTextureDimensions( MEX &mexWidth, MEX &mexHeight); // riches texture dimensions
  void GetAllFramesBBox( FLOATaabbox3D &MaxBB);   // calculates all frame's bounding box
  FLOAT3D GetCollisionBoxMin(INDEX iCollisionBox);
  FLOAT3D GetCollisionBoxMax(INDEX iCollisionBox);
  // returns HEIGHT_EQ_WIDTH, LENGTH_EQ_WIDTH or LENGTH_EQ_HEIGHT
  INDEX GetCollisionBoxDimensionEquality(INDEX iCollisionBox);
  ULONG GetFlags(void);
	void Clear(void);
  // check if this kind of objects is auto-freed
  virtual BOOL IsAutoFreed(void);

  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
	void ClearAnimations(void);
  void LinkDataForSurfaces(BOOL bFirstMip);// calculate polygons and vertices for surface
  void Read_t( CTStream *istrFile);  // throw char *
	void Write_t( CTStream *ostrFile); // throw char *
  /* Get the description of this object. */
  CTString GetDescription(void);

};


#endif  /* include-once check. */

