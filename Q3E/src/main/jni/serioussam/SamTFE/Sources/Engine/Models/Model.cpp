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

#include <Engine/Models/ModelObject.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/RenderModel.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Models/Normals.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/CTString.inl>
#include <Engine/Math/Clipping.inl>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/DrawPort.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>

#include <Engine/Templates/Stock_CModelData.h>

template class CStaticArray<MappingSurface>;
template class CStaticArray<ModelPolygon>;
template class CStaticArray<ModelPolygonVertex>;
template class CStaticArray<ModelTextureVertex>;
template class CStaticArray<PolygonsPerPatch>;
template class CDynamicArray<CAttachedModelPosition>;


extern UBYTE aubGouraudConv[16384];

// model LOD biasing control
extern FLOAT mdl_fLODMul;
extern FLOAT mdl_fLODAdd;
extern INDEX mdl_iLODDisappear; // 0=never, 1=ignore bias, 2=with bias
extern INDEX mdl_bFineQuality;  // 0=force to 8-bit, 1=optimal



CModelData::CModelData(const CModelData &c) { ASSERT(FALSE); };
CModelData &CModelData::operator=(const CModelData &c){ ASSERT(FALSE); return *this; }; 

// if any surface in model that we are currently reading has any transparency
BOOL _bHasAlpha;

// colors used to represent on and off bits
COLOR PaletteColorValues[] =
{
  C_RED, C_GREEN, C_BLUE, C_CYAN,
  C_MAGENTA, C_YELLOW, C_ORANGE, C_BROWN,
  C_PINK, C_dGRAY, C_GRAY, C_lGRAY,
  C_dRED, C_lRED, C_dGREEN, C_lGREEN,
  C_dBLUE, C_lBLUE, C_dCYAN, C_lCYAN,
  C_dMAGENTA, C_lMAGENTA, C_dYELLOW, C_lYELLOW,
  C_dORANGE, C_lORANGE, C_dBROWN, C_lBROWN,
  C_dPINK, C_lPINK, C_WHITE, C_BLACK,
};

/*
 * Instanciated global rendering preferences object containing
 * info about rendering of models
 */
CModelRenderPrefs _mrpModelRenderPrefs;

/*
 * Functions dealing with 16-bit normal compression
 */
void CompressNormal_HQ(const FLOAT3D &vNormal, UBYTE &ubH, UBYTE &ubP)
{
  ANGLE h, p;

  const FLOAT &x = vNormal(1);
  const FLOAT &y = vNormal(2);
  const FLOAT &z = vNormal(3);

  // calculate pitch
  p = ASin(y);

  // if y is near +1 or -1
  if (y>0.99 || y<-0.99) {
    // heading is irrelevant
    h = 0;
  // otherwise
  } else {
    // calculate heading
    h = ATan2(-x, -z);
  }

  h = (h/360.0f)+0.5f;
  p = (p/360.0f)+0.5f;
  ASSERT(h>=0 && h<=1);
  ASSERT(p>=0 && p<=1);
  ubH = UBYTE(h*255);
  ubP = UBYTE(p*255);
}

void DecompressNormal_HQ(FLOAT3D &vNormal, UBYTE ubH, UBYTE ubP)
{
  ANGLE h = (ubH/255.0f)*360.0f-180.0f;
  ANGLE p = (ubP/255.0f)*180.0f-90.0f;

  FLOAT &x = vNormal(1);
  FLOAT &y = vNormal(2);
  FLOAT &z = vNormal(3);

  x = -Sin(h)*Cos(p);
  y = Sin(p);
  z = -Cos(h)*Cos(p);
}

//--------------------------------------------------------------------------------------------
/*
 * Function returns number of first setted bit in ULONG
 */
INDEX GetBit( ULONG ulSource)
{
  for( INDEX i=0; i<32; i++)
  {
    if( (ulSource & (1<<i)) != 0) return i;
  }
  return 0;
}

//--------------------------------------------------------------------------------------------
/*
 * Default constructor sets default rendering preferences
 */
CModelRenderPrefs::CModelRenderPrefs()
{
  rp_BBoxFrameVisible = FALSE;
  rp_BBoxAllVisible = FALSE;
  rp_InkColor = C_BLACK;
  rp_PaperColor = C_lGRAY;
  rp_RenderType = RT_TEXTURE | RT_SHADING_PHONG;
  rp_ShadowQuality = 0;
}
/*
 * Routines managing (get/set) rendering preferences
 */
void CModelRenderPrefs::SetRenderType(ULONG rtNew)
{
  rp_RenderType = rtNew;
}
void CModelRenderPrefs::SetTextureType(ULONG rtNew)
{
  rp_RenderType = (rp_RenderType & (~RT_TEXTURE_MASK)) | rtNew;
}
void CModelRenderPrefs::SetShadingType(ULONG rtNew)
{
  rp_RenderType = (rp_RenderType & (~RT_SHADING_MASK)) | rtNew;
}
void CModelRenderPrefs::SetWire(BOOL bWireOn)
{
  if( bWireOn)
  {
    rp_RenderType = rp_RenderType | RT_WIRE_ON;
  }
  else
  {
    rp_RenderType = rp_RenderType & (~RT_WIRE_ON);
  }
}
void CModelRenderPrefs::SetHiddenLines(BOOL bHiddenLinesOn)
{
  if( bHiddenLinesOn)
  {
    rp_RenderType = rp_RenderType | RT_HIDDEN_LINES;
  }
  else
  {
    rp_RenderType = rp_RenderType & (~RT_HIDDEN_LINES);
  }
}
ULONG CModelRenderPrefs::GetRenderType()
{
  return( rp_RenderType);
}
void CModelRenderPrefs::SetShadowQuality(INDEX iNewQuality)
{
  ASSERT( iNewQuality >= 0);
  rp_ShadowQuality = iNewQuality;
}
void CModelRenderPrefs::DesreaseShadowQuality(void)
{
  rp_ShadowQuality += 1;
}
void CModelRenderPrefs::IncreaseShadowQuality(void)
{
  if( rp_ShadowQuality > 0)
    rp_ShadowQuality -= 1;
}
INDEX CModelRenderPrefs::GetShadowQuality()
{
  return( rp_ShadowQuality);
}
BOOL CModelRenderPrefs::BBoxFrameVisible()
{
  return( rp_BBoxFrameVisible);
}
void CModelRenderPrefs::BBoxFrameShow( BOOL bShow)
{
  rp_BBoxFrameVisible = bShow;
}
BOOL CModelRenderPrefs::BBoxAllVisible()
{
  return( rp_BBoxAllVisible);
}
void CModelRenderPrefs::BBoxAllShow( BOOL bShow)
{
  rp_BBoxAllVisible = bShow;
}
BOOL CModelRenderPrefs::WireOn()
{
  return( (rp_RenderType & RT_WIRE_ON) != 0);
}
BOOL CModelRenderPrefs::HiddenLines()
{
  return( (rp_RenderType & RT_HIDDEN_LINES) != 0);
}
void CModelRenderPrefs::SetInkColor(COLOR clrNew)
{
  rp_InkColor = clrNew;
}
COLOR CModelRenderPrefs::GetInkColor()
{
  return rp_InkColor;
}
void CModelRenderPrefs::SetPaperColor(COLOR clrNew)
{
  rp_PaperColor = clrNew;
}
COLOR CModelRenderPrefs::GetPaperColor()
{
  return rp_PaperColor;
}
// read and write functions
void CModelRenderPrefs::Read_t( CTStream *istrFile) // throw char *
{
}
void CModelRenderPrefs::Write_t( CTStream *ostrFile) // throw char *
{
}

//--------------------------------------------------------------------------------------------
CModelPatch::CModelPatch(void)
{
  mp_strName = "";
  mp_mexPosition = MEX2D( 1024, 1024);
  mp_fStretch = 1.0f;
}

void CModelPatch::Read_t(CTStream *strFile)
{
  *strFile >> mp_strName;
  CTFileName fnPatchTexture;
  *strFile >> fnPatchTexture;
  try
  {
    mp_toTexture.SetData_t( fnPatchTexture);
  }
  catch (const char *strError)
  {
    (void) strError;
  }
  *strFile >> mp_mexPosition;
  *strFile >> mp_fStretch;
}

void CModelPatch::Write_t(CTStream *strFile)
{
  *strFile << mp_strName;
  *strFile << mp_toTexture.GetName();
  *strFile << mp_mexPosition;
  *strFile << mp_fStretch;
}

//--------------------------------------------------------------------------------------------
/*
 * Default constructor sets invalid data
 */
CModelData::CModelData()
{
	INDEX i;
  md_bPreparedForRendering = FALSE;

	md_VerticesCt = 0;															// number of vertices in model
  md_FramesCt = 0;																// number of all frames used by this model
  md_MipCt = 0;																		// number of mip-models

  md_bIsEdited = FALSE; // not edited by default

	// invalidate mip-model info data
  for( i=0; i<MAX_MODELMIPS; i++) {
		md_MipInfos[i].mmpi_PolygonsCt = 0;
	}

  md_Flags = 0;		                                // model flags (flat, reflection mapping)
  md_ShadowQuality = 0;
  md_Stretch = FLOAT3D(1,1,1);                 		// stretch vector (static one, dynamic one is in model object)
  md_bCollideAsCube = FALSE;                      // collide as sphere
  md_colDiffuse = C_WHITE|CT_OPAQUE;
  md_colReflections = C_WHITE|CT_OPAQUE;
  md_colSpecular = C_WHITE|CT_OPAQUE;
  md_colBump = C_WHITE|CT_OPAQUE;
}


/*
 * Clear all model data arrays
 */
void CModelData::Clear(void)
{
  INDEX i;
  md_bPreparedForRendering = FALSE;

  CAnimData::Clear();

  md_FrameVertices16.Clear();
  md_FrameVertices8.Clear();
  md_FrameInfos.Clear();
  md_MainMipVertices.Clear();
  md_TransformedVertices.Clear();
  md_VertexMipMask.Clear();
  md_aampAttachedPosition.Clear();
  md_acbCollisionBox.Clear();

  for( i=0; i<md_MipCt;           i++) md_MipInfos[i].Clear();
  for( i=0; i<MAX_COLOR_NAMES;    i++) md_ColorNames[i].Clear();
  for( i=0; i<MAX_TEXTUREPATCHES; i++) md_mpPatches[i].mp_toTexture.SetData_t( CTString(""));

  md_VerticesCt = 0;
  md_FramesCt = 0;
  md_MipCt = 0;
}


// get amount of memory used by this object
SLONG CModelData::GetUsedMemory(void)
{
  SLONG slUsed = sizeof(*this)+CAnimData::GetUsedMemory()-sizeof(CAnimData);

  slUsed += md_FrameVertices16.Count()*sizeof(struct ModelFrameVertex16);
  slUsed += md_FrameVertices8.Count()*sizeof(struct ModelFrameVertex8);
  slUsed += md_FrameInfos.Count()*sizeof(struct ModelFrameInfo);
  slUsed += md_MainMipVertices.Count()*sizeof(FLOAT3D);
  slUsed += md_TransformedVertices.Count()*sizeof(struct TransformedVertexData);
  slUsed += md_VertexMipMask.Count()*sizeof(ULONG);
  slUsed += md_acbCollisionBox.Count()*sizeof(CModelCollisionBox);
  slUsed += md_aampAttachedPosition.Count()*sizeof(CAttachedModelPosition);

  for(INDEX i=0; i<md_MipCt; i++) {
    slUsed += md_MipInfos[i].mmpi_aPolygonsPerPatch.Count()*sizeof(struct PolygonsPerPatch);
	  slUsed += md_MipInfos[i].mmpi_Polygons.Count()*sizeof(struct ModelPolygon);
    slUsed += md_MipInfos[i].mmpi_TextureVertices.Count()*sizeof(struct ModelTextureVertex);
    slUsed += md_MipInfos[i].mmpi_MappingSurfaces.Count()*sizeof(struct MappingSurface);
  }

  return slUsed;
}
// check if this kind of objects is auto-freed
BOOL CModelData::IsAutoFreed(void)
{
  return FALSE;
}

void CModelData::ClearAnimations(void)
{
  CAnimData::Clear();

  md_FrameVertices16.Clear();
  md_FrameVertices8.Clear();
	md_FrameInfos.Clear();
  md_FramesCt = 0;
}

//--------------------------------------------------------------------------------------------
// riches texture dimensions
void CModelData::GetTextureDimensions( MEX &mexWidth, MEX &mexHeight)
{
  mexWidth = md_Width;
  mexHeight = md_Height;
}

//--------------------------------------------------------------------------------------------
// calculates bounding box of all frames
void CModelData::GetAllFramesBBox( FLOATaabbox3D &MaxBB)
{
  for( INDEX i=0; i<md_FramesCt; i++)
  {
    MaxBB |= md_FrameInfos[ i].mfi_Box;
  }
}

FLOAT3D CModelData::GetCollisionBoxMin(INDEX iCollisionBox)
{
  md_acbCollisionBox.Lock();
  INDEX iCollisionBoxClamped = Clamp(iCollisionBox, (INDEX)0, md_acbCollisionBox.Count()-1);
  FLOAT3D vMin = md_acbCollisionBox[ iCollisionBoxClamped].mcb_vCollisionBoxMin;
  md_acbCollisionBox.Unlock();
  return vMin;
};

FLOAT3D CModelData::GetCollisionBoxMax(INDEX iCollisionBox=0)
{
  md_acbCollisionBox.Lock();
  INDEX iCollisionBoxClamped = Clamp(iCollisionBox, (INDEX)0, md_acbCollisionBox.Count()-1);
  FLOAT3D vMax = md_acbCollisionBox[ iCollisionBoxClamped].mcb_vCollisionBoxMax;
  md_acbCollisionBox.Unlock();
  return vMax;
};

// returns HEIGHT_EQ_WIDTH, LENGTH_EQ_WIDTH or LENGTH_EQ_HEIGHT
INDEX CModelData::GetCollisionBoxDimensionEquality(INDEX iCollisionBox=0)
{
  md_acbCollisionBox.Lock();
  iCollisionBox = Clamp(iCollisionBox, (INDEX)0, md_acbCollisionBox.Count()-1);
  INDEX iDimEq = md_acbCollisionBox[ iCollisionBox].mcb_iCollisionBoxDimensionEquality;
  md_acbCollisionBox.Unlock();
  return iDimEq;
};


ULONG CModelData::GetFlags(void)
{
  return md_Flags;
};


//--------------------------------------------------------------------------------------------
void CModelData::SpreadMipSwitchFactors( INDEX iFirst, float fStartingFactor)
{
  // set switch steep to cover all mips until max default range reached
  FLOAT fSteep;
  // if we allready skipped max range or we don't have any more mips
  if( (fStartingFactor > MAX_SWITCH_FACTOR) || ((md_MipCt-iFirst) <= 0) )
  {
    // define next switch factor offset
    fSteep = 1.2f;
  }

  // else divide factor from starting factor to max switch factor with number of mip
  // models left
  else
  {
    fSteep = (MAX_SWITCH_FACTOR - fStartingFactor) / (md_MipCt-iFirst);
  }

  // spread mip switch factors for rougher mip models
  for( INDEX i=iFirst; i<md_MipCt; i++)
  {
    md_MipSwitchFactors[ i] = fStartingFactor + (i-iFirst+1)*fSteep;
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Default destructor asserts invalid data and clears valid ones
 */
CModelData::~CModelData()
{
  Clear();
}
//--------------------------------------------------------------------------------------------
ModelTextureVertex::ModelTextureVertex(void)
{
  mtv_iTransformedVertex = 0;
  mtv_Done = FALSE;
}
//------------------------------------------ WRITE
void ModelPolygonVertex::Write_t( CTStream *pFile)  // throw char *
{
  // DG: this looks like a 64-bit issue, but most probably isn't as PtrToIndices() should have been called before this
  (*pFile) << (INDEX) (size_t) mpv_ptvTransformedVertex;
  (*pFile) << (INDEX) (size_t) mpv_ptvTextureVertex;
}
//------------------------------------------ READ
void ModelPolygonVertex::Read_t( CTStream *pFile) // throw char *
{
  INDEX itmp;

  // DG: this looks like a 64-bit issue, but most probably isn't as IndicesToPtrs() should be
  //     called afterwards to restore actually valid pointers.
  (*pFile) >> itmp;
  mpv_ptvTransformedVertex = (struct TransformedVertexData *) (size_t) itmp;
  (*pFile) >> itmp;
  mpv_ptvTextureVertex = (ModelTextureVertex *) (size_t) itmp;
}
//--------------------------------------------------------------------------------------------
//------------------------------------------ WRITE
void ModelPolygon::Write_t( CTStream *pFile) // throw char *
{
  pFile->WriteID_t( CChunkID("MDP2"));
  INDEX ctVertices = mp_PolygonVertices.Count();
  (*pFile) << ctVertices;
  {FOREACHINSTATICARRAY(mp_PolygonVertices, ModelPolygonVertex, it)
    { it.Current().Write_t( pFile);}}
  (*pFile) << mp_RenderFlags;
  (*pFile) << mp_ColorAndAlpha;
  (*pFile) << mp_Surface;
};
//------------------------------------------ READ
void ModelPolygon::Read_t( CTStream *pFile)  // throw char *
{
  INDEX ctVertices;
  ULONG ulDummy;
  if( pFile->PeekID_t() == CChunkID("MDPL"))
  {
    pFile->ExpectID_t( CChunkID("MDPL"));
    pFile->ReadFullChunk_t( CChunkID("IMPV"), &ctVertices, sizeof(INDEX));
    BYTESWAP(ctVertices);
    mp_PolygonVertices.New( ctVertices);

    {FOREACHINSTATICARRAY(mp_PolygonVertices, ModelPolygonVertex, it)
      { it.Current().Read_t( pFile);}}
    (*pFile) >> mp_RenderFlags;
    (*pFile) >> mp_ColorAndAlpha;
    (*pFile) >> mp_Surface;
    (*pFile) >> ulDummy;  // ex on color
    (*pFile) >> ulDummy;  // ex off color
  }
  else
  {
    pFile->ExpectID_t( CChunkID("MDP2"));
    (*pFile) >> ctVertices;
    mp_PolygonVertices.New( ctVertices);

    {FOREACHINSTATICARRAY(mp_PolygonVertices, ModelPolygonVertex, it)
      { it.Current().Read_t( pFile);}}
    (*pFile) >> mp_RenderFlags;
    (*pFile) >> mp_ColorAndAlpha;
    (*pFile) >> mp_Surface;
  }
};
//----------------------------------------------------------------------------------
// TEMPORARY - REMOVE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// POLYGON RENDER CONSTANTS
// new_render_types = rendertype (0-7)<<0 + phongstrength(0-7)<<3 + alphatype(0-3)<<6
// poly render types
#define PR_PHONGSHADING     (0x00<<0)     // textures phong shading
#define PR_LAMBERTSHADING   (0x01<<0)     // textures lambert shading (no phong/gouraud)
#define PR_ALPHAGOURAUD     (0x02<<0)     // alpha gouraud shading
#define PR_FRONTPROJECTION  (0x03<<0)     // front projection shading (no rotation, 2D zoom)
#define PR_SHADOWBLENDING   (0x04<<0)     // shadow blending (black alpha gouraud)
#define PR_COLORFILLING     (0x05<<0)     // flat filling (single color poly, no texture)

#define PR_MASK             (0x07<<0)     // mask for render types

// phong strengths (shading types)
#define PS_MATTE  (0x00<<3)     // same as the gouraud
#define PS_SHINY  (0x01<<3)     // mild shininig
#define PS_METAL  (0x02<<3)     // shine as hell

#define PS_MASK   (0x07<<3)     // mask for phong strengths

// texture's alpha channel flag
#define AC_SKIPALPHACHANNEL (0x00<<6) // opaque rendering regardless of alpha channel presence
#define AC_USEALPHACHANNEL  (0x01<<6) // texture's alpha ch. will be taken in consideration
#define AC_ZEROTRANSPARENCY (0x02<<6) // texture's zero values are transparent (no alpha ch.)

#define AC_MASK             (0x03<<6) // mask for texture's alpha flag

// polygon's control flags
#define PCF_DOUBLESIDED (0x01<<9)     // double sided polygon
#define PCF_NOSHADING   (0x02<<9)     // this polygon will not be shaded anyhow (?)
#define PCF_CLIPPOLYGON (0x04<<9)     // polygon is clipped, instead rejected
#define PCF_REFLECTIONS (0x08<<9)     // use reflection mapping
//----------------------------------------------------------------------------------

BOOL MappingSurface::operator==(const MappingSurface &msOther) const {
  return msOther.ms_Name == ms_Name;
};

// convert old polygon flags from CTGfx into new rendering parameters
void MappingSurface::SetRenderingParameters(ULONG ulOldFlags)
{
  // find rendering type
  if (ulOldFlags&PCF_NOSHADING) {
    ms_sstShadingType = SST_FULLBRIGHT;
  } else {
    switch (ulOldFlags&PS_MASK) {
    case PS_MATTE:
      ms_sstShadingType = SST_MATTE;
      break;
    case PS_SHINY:
      ms_sstShadingType = SST_MATTE;
      break;
    case PS_METAL:
      ms_sstShadingType = SST_MATTE;
      break;
    default:
      ms_sstShadingType = SST_MATTE;
    }
  }
  // find translucency type
  if ((ulOldFlags&PR_MASK)==PR_ALPHAGOURAUD) {
    ms_sttTranslucencyType = STT_ALPHAGOURAUD;
  } else if ((ulOldFlags&AC_MASK)==AC_ZEROTRANSPARENCY) {
    ms_sttTranslucencyType = STT_TRANSLUCENT;
  } else {
    ms_sttTranslucencyType = STT_OPAQUE;
  }
  // find flags
  ms_ulRenderingFlags = 0;
  if (ulOldFlags&PCF_DOUBLESIDED) {
    ms_ulRenderingFlags|=SRF_DOUBLESIDED;
  }
  if (ulOldFlags&PCF_REFLECTIONS) {
    ms_ulRenderingFlags|=SRF_REFLECTIONS;
  }
}
//------------------------------------------ WRITE
void MappingSurface::Write_t( CTStream *pFile)  // throw char *
{
  (*pFile) << ms_Name;
  pFile->Write_t( &ms_vSurface2DOffset, sizeof(FLOAT3D));
  pFile->Write_t( &ms_HPB, sizeof(FLOAT3D));
  pFile->Write_t( &ms_Zoom, sizeof(float));

  pFile->Write_t( &ms_sstShadingType, sizeof(SurfaceShadingType));
  pFile->Write_t( &ms_sttTranslucencyType, sizeof(SurfaceTranslucencyType));
  (*pFile) << ms_ulRenderingFlags;

  INDEX ctPolygons = ms_aiPolygons.Count();
  (*pFile) << ctPolygons;
  if( ctPolygons != 0)
  {
    pFile->Write_t( &ms_aiPolygons[0], sizeof( INDEX)*ctPolygons);
  }

  INDEX ctTextureVertices = ms_aiTextureVertices.Count();
  (*pFile) << ctTextureVertices;
  if( ctTextureVertices != 0)
  {
    pFile->Write_t( &ms_aiTextureVertices[0], sizeof( INDEX)*ctTextureVertices);
  }

  (*pFile) << ms_colColor;

  (*pFile) << ms_colDiffuse;
  (*pFile) << ms_colReflections;
  (*pFile) << ms_colSpecular;
  (*pFile) << ms_colBump;
  
  (*pFile) << ms_ulOnColor;
  (*pFile) << ms_ulOffColor;
}

void MappingSurface::WriteSettings_t( CTStream *pFile)  // throw char *
{
  (*pFile) << ms_Name;
  (*pFile) << (INDEX &)ms_sstShadingType;
  (*pFile) << (INDEX &)ms_sttTranslucencyType;
  (*pFile) << ms_ulRenderingFlags;
  (*pFile) << ms_colDiffuse;
  (*pFile) << ms_colReflections;
  (*pFile) << ms_colSpecular;
  (*pFile) << ms_colBump;
  (*pFile) << ms_ulOnColor;
  (*pFile) << ms_ulOffColor;
}

void MappingSurface::ReadSettings_t( CTStream *pFile)  // throw char *
{
  (*pFile) >> ms_Name;
  (*pFile) >> (INDEX&) ms_sstShadingType;
  (*pFile) >> (INDEX&) ms_sttTranslucencyType;
  (*pFile) >> ms_ulRenderingFlags;
  (*pFile) >> ms_colDiffuse;
  (*pFile) >> ms_colReflections;
  (*pFile) >> ms_colSpecular;
  (*pFile) >> ms_colBump;
  (*pFile) >> ms_ulOnColor;
  (*pFile) >> ms_ulOffColor;
}

//------------------------------------------ READ
void MappingSurface::Read_t( CTStream *pFile, BOOL bReadPolygonsPerSurface,
                            BOOL bReadSurfaceColors) // throw char *
{
  (*pFile) >> ms_Name;
  (*pFile) >> ms_vSurface2DOffset;
  (*pFile) >> ms_HPB;
  (*pFile) >> ms_Zoom;

  if( bReadPolygonsPerSurface)
  {
    (*pFile) >> (INDEX &) ms_sstShadingType;
    // WARNING !!! All shading types bigger than matte will be remaped into flat shading
    // this was done when SHINY and METAL were removed
    if( ms_sstShadingType > SST_MATTE)
    {
      ms_sstShadingType = SST_FLAT;
    }
    (*pFile) >> (INDEX &) ms_sttTranslucencyType;
    (*pFile) >> ms_ulRenderingFlags;
    if( (ms_ulRenderingFlags&SRF_NEW_TEXTURE_FORMAT) == 0)
      ms_ulRenderingFlags |= SRF_DIFFUSE|SRF_NEW_TEXTURE_FORMAT;
    if (ms_sttTranslucencyType==STT_TRANSLUCENT || ms_sttTranslucencyType==STT_ALPHAGOURAUD
      ||ms_sttTranslucencyType==STT_ADD||ms_sttTranslucencyType==STT_MULTIPLY) {
      _bHasAlpha = TRUE;
    }

    ms_aiPolygons.Clear();
    INDEX ctPolygons;
    (*pFile) >> ctPolygons;
    ms_aiPolygons.New( ctPolygons);
    for (INDEX i = 0; i < ctPolygons; i++)
      (*pFile) >> ms_aiPolygons[i];

    ms_aiTextureVertices.Clear();
    INDEX ctTextureVertices;
    (*pFile) >> ctTextureVertices;
    ms_aiTextureVertices.New( ctTextureVertices);
    for (INDEX i = 0; i < ctTextureVertices; i++)
      (*pFile) >> ms_aiTextureVertices[i];

    (*pFile) >> ms_colColor;
  }
  if( bReadSurfaceColors)
  {
    (*pFile) >> ms_colDiffuse;
    (*pFile) >> ms_colReflections;
    (*pFile) >> ms_colSpecular;
    (*pFile) >> ms_colBump;
    (*pFile) >> ms_ulOnColor;
    (*pFile) >> ms_ulOffColor;
  }
}


void CModelData::LinkDataForSurfaces(BOOL bFirstMip)
{
  INDEX iMipStart=1;
  if( bFirstMip)
  {
    iMipStart=0;
  }
  // for each mip model
  for( INDEX iMip=iMipStart; iMip<md_MipCt; iMip ++)
  {
    ModelMipInfo *pMMI = &md_MipInfos[ iMip];

    // --------------------- Set index of transformed vertex to texture vertex
    {for( INDEX iPolygon = 0; iPolygon<pMMI->mmpi_Polygons.Count(); iPolygon++)
    {
      for( INDEX iVertex = 0; iVertex<pMMI->mmpi_Polygons[iPolygon].mp_PolygonVertices.Count(); iVertex++)
      {
        ModelPolygonVertex *pmpvPolygonVertex = &pMMI->mmpi_Polygons[iPolygon].mp_PolygonVertices[iVertex];
        INDEX iTransformed = md_TransformedVertices.Index( pmpvPolygonVertex->mpv_ptvTransformedVertex);
        pmpvPolygonVertex->mpv_ptvTextureVertex->mtv_iTransformedVertex = iTransformed;
      }
    }}

    // --------------------- Linking polygons for surface
    // array telling how many polygons are in each surface
    CStaticArray<INDEX> actPolygonsInSurface;
    INDEX ctSurfaces = pMMI->mmpi_MappingSurfaces.Count();
    actPolygonsInSurface.New( ctSurfaces);
    // for each surface
    {for( INDEX iSurface=0; iSurface<ctSurfaces; iSurface++)
    {
      // reset count of polygons
      actPolygonsInSurface[iSurface] = 0;
    }}
    // for each polygon
    {for( INDEX iPolygon=0; iPolygon<pMMI->mmpi_Polygons.Count(); iPolygon++)
    {
      // increment count of polygons in its surface
      actPolygonsInSurface[ pMMI->mmpi_Polygons[iPolygon].mp_Surface]++;
    }}
    // for each surface
    {for( INDEX iSurface=0; iSurface<ctSurfaces; iSurface++)
    {
      // allocate array for polygon indices
      pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons.Clear();
      pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons.New( actPolygonsInSurface[iSurface]);
      if( actPolygonsInSurface[iSurface] != 0)
      {
        // last place in array will contain counter of added polygons
        pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons[actPolygonsInSurface[iSurface]-1] = 0;
      }
    }}
    // for each polygon
    for( INDEX iPolygon=0; iPolygon<pMMI->mmpi_Polygons.Count(); iPolygon++)
    {
      // get surface, polygons in surface and last remembered index of polygon in surface
      INDEX iSurface = pMMI->mmpi_Polygons[iPolygon].mp_Surface;
      INDEX ctPolygonsInSurface = actPolygonsInSurface[iSurface];
      if( ctPolygonsInSurface != 0)
      {
        INDEX iLastSet = pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons[ ctPolygonsInSurface-1];
        // remember last set polygon index
        pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons[ ctPolygonsInSurface-1] = iLastSet+1;
        // remember index of polygon
        pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons[ iLastSet] = iPolygon;
      }
    }

    // --------------------- Linking texture vertices for surface
    // for each surface
    {for( INDEX iSurface=0; iSurface<ctSurfaces; iSurface++)
    {
      CDynamicContainer<ModelTextureVertex> cmtvInSurface;
      // for each polygon in surface
      FOREACHINSTATICARRAY( pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons, INDEX, itimpo)
      {
        ModelPolygon &mpPolygon = pMMI->mmpi_Polygons[itimpo.Current()];
        // for each vertex in polygon
        for( INDEX iVertex=0; iVertex<mpPolygon.mp_PolygonVertices.Count(); iVertex++)
        {
          // get texture vertex
          ModelTextureVertex *ptv =
            mpPolygon.mp_PolygonVertices[ iVertex].mpv_ptvTextureVertex;
          // if it is not added yet
          if( !cmtvInSurface.IsMember( ptv))
          {
            // add it
            cmtvInSurface.Add( ptv);
          }
        }
      }
      // add needed number of texture vertices
      pMMI->mmpi_MappingSurfaces[iSurface].ms_aiTextureVertices.Clear();
      pMMI->mmpi_MappingSurfaces[iSurface].ms_aiTextureVertices.New( cmtvInSurface.Count());
      INDEX cttv = 0;
      // for each texture vertex in container
      FOREACHINDYNAMICCONTAINER(cmtvInSurface, ModelTextureVertex, itmtv)
      {
        INDEX idxtv = pMMI->mmpi_TextureVertices.Index( itmtv);
        pMMI->mmpi_MappingSurfaces[iSurface].ms_aiTextureVertices[cttv] = idxtv;
        cttv++;
      }
    }}
  }
}

// Default constructor
ModelMipInfo::ModelMipInfo(void)
{
  mmpi_ulFlags = MM_PATCHES_VISIBLE | MM_ATTACHED_MODELS_VISIBLE;
}

//--------------------------------------------------------------------------------------------
//------------------------------------------ WRITE
/*
 * This is write function of one mip-model. It saves all mip's arrays eather by saving
 * them really or calling their write functions.
 */
void ModelMipInfo::Write_t( CTStream *pFile)  // throw char *
{
  INDEX iMembersCt;

  // Save count, call write for array of model polygons
  pFile->WriteFullChunk_t( CChunkID("IPOL"), &mmpi_PolygonsCt, sizeof(INDEX));
  {FOREACHINSTATICARRAY(mmpi_Polygons, ModelPolygon, it)
    { it.Current().Write_t( pFile);}}

  // Save count, array of texture vertices
  iMembersCt = mmpi_TextureVertices.Count();
  (*pFile) << iMembersCt;
  pFile->WriteFullChunk_t( CChunkID("TXV2"), &mmpi_TextureVertices[ 0], iMembersCt *
                           sizeof(struct ModelTextureVertex));

  // Save count, call write for array of mapping surfaces
  iMembersCt = mmpi_MappingSurfaces.Count();
  (*pFile) << iMembersCt;
  {FOREACHINSTATICARRAY(mmpi_MappingSurfaces, MappingSurface, it)
    { it.Current().Write_t( pFile);}}

  // write mip model flags
  (*pFile) << mmpi_ulFlags;
  // write info of polygons occupied by patch
  INDEX ctPatches = mmpi_aPolygonsPerPatch.Count();
  (*pFile) << ctPatches;
  // for each patch
  for( INDEX iPatch=0; iPatch<ctPatches; iPatch++)
  {
    // write no of occupied polygons
    INDEX ctOccupied = mmpi_aPolygonsPerPatch[iPatch].ppp_iPolygons.Count();
    (*pFile) << ctOccupied;
    if( ctOccupied != 0)
    {
      pFile->WriteFullChunk_t( CChunkID("OCPL"),
        &mmpi_aPolygonsPerPatch[iPatch].ppp_iPolygons[ 0], ctOccupied * sizeof(INDEX));
    }
  }
}
//------------------------------------------ READ
/*
 * This is read function of one mip-model
 */
void ModelMipInfo::Read_t(CTStream *pFile,
                          BOOL bReadPolygonalPatches,
                          BOOL bReadPolygonsPerSurface,
                          BOOL bReadSurfaceColors)
{
  INDEX iMembersCt;

  // Load count, allocate array and call Read for array of model polygons
  pFile->ReadFullChunk_t( CChunkID("IPOL"), &mmpi_PolygonsCt, sizeof(INDEX));
  BYTESWAP(mmpi_PolygonsCt);
  mmpi_Polygons.New( mmpi_PolygonsCt);
  {FOREACHINSTATICARRAY(mmpi_Polygons, ModelPolygon, it)
  {
    it.Current().Read_t( pFile);
  }}

  // Load count, allocate and load array of texture vertices
  (*pFile) >> iMembersCt;
  mmpi_TextureVertices.New( iMembersCt);
  if( bReadPolygonsPerSurface)
  {
    // chunk ID will tell us if we should read new format that contains bump normals
    CChunkID idChunk = pFile->GetID_t();
    // jump over chunk size
    ULONG ulDummySize;
    (*pFile) >> ulDummySize;
    // if bump normals are saved (new format)
    if( idChunk == CChunkID("TXV2"))
    {
      for (SLONG i = 0; i < iMembersCt; i++)
        (*pFile)>>mmpi_TextureVertices[i];
    } else {
      // bump normals are not saved
      for( INDEX iVertex = 0; iVertex<iMembersCt; iVertex++)
      {
        (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_UVW;
        (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_UV;
        (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_Done;
        (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_iTransformedVertex;
        mmpi_TextureVertices[ iVertex].mtv_vU = FLOAT3D(0,0,0);
        mmpi_TextureVertices[ iVertex].mtv_vV = FLOAT3D(0,0,0);
      }
    }
  }
  else
  {
    pFile->ExpectID_t( CChunkID("TXVT"));
    // jump over chunk size
    ULONG ulDummySize;
    (*pFile) >> ulDummySize;
    // read models in old format
    for( INDEX iVertex = 0; iVertex<iMembersCt; iVertex++)
    {
      (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_UVW;
      (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_UV;
      (*pFile)>>mmpi_TextureVertices[ iVertex].mtv_Done;
      mmpi_TextureVertices[ iVertex].mtv_iTransformedVertex = 0;
      mmpi_TextureVertices[ iVertex].mtv_vU = FLOAT3D(0,0,0);
      mmpi_TextureVertices[ iVertex].mtv_vV = FLOAT3D(0,0,0);
    }
  }

  // Load count, allcate array and call Read for array of mapping surfaces
  (*pFile) >> iMembersCt;
  mmpi_MappingSurfaces.New( iMembersCt);
  INDEX iIndexOfSurface = 0;
  {FOREACHINSTATICARRAY(mmpi_MappingSurfaces, MappingSurface, it)
  {
    it.Current().Read_t( pFile, bReadPolygonsPerSurface, bReadSurfaceColors);
    // obtain color per surface from polygons (old model format)
    if( !bReadPolygonsPerSurface)
    {
      // we will copy color from first polygon found with this surface into color of surface
      it->ms_colColor = C_WHITE;
      // for all polygons in this mip level
      for( INDEX iPolygon=0;iPolygon<mmpi_PolygonsCt;iPolygon++)
      {
        if( mmpi_Polygons[ iPolygon].mp_Surface == iIndexOfSurface)
        {
          it->ms_colColor = mmpi_Polygons[ iPolygon].mp_ColorAndAlpha;
          break;
        }
      }
      iIndexOfSurface++;
    }
  }}

  if( bReadPolygonalPatches)
  {
    // read mip model flags
    (*pFile) >> mmpi_ulFlags;
    // read no of patches
    INDEX ctPatches;
    (*pFile) >> ctPatches;
    if( ctPatches != 0)
    {
      mmpi_aPolygonsPerPatch.New( ctPatches);
      // read info for polygonal patches
      for( INDEX iPatch=0; iPatch<ctPatches; iPatch++)
      {
        // read no of occupied polygons
        INDEX ctOccupied;
        (*pFile) >> ctOccupied;
        if( ctOccupied != 0)
        {
          mmpi_aPolygonsPerPatch[iPatch].ppp_iPolygons.New( ctOccupied);
          pFile->ReadFullChunk_t( CChunkID("OCPL"),
            &mmpi_aPolygonsPerPatch[iPatch].ppp_iPolygons[ 0], ctOccupied * sizeof(INDEX));

          #if PLATFORM_BIGENDIAN
          for (INDEX i = 0; i < ctOccupied; i++)
          {
            BYTESWAP(mmpi_aPolygonsPerPatch[iPatch].ppp_iPolygons[i]);
          }
          #endif
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
/*
 * Routine converts mpv_ptvTransformedVertex and mpv_ptvTextureVertex from ptrs to Indices
 */
void CModelData::PtrsToIndices()
{
  INDEX i, j;

  for( i=0; i<md_MipCt; i++)
  {
    FOREACHINSTATICARRAY(md_MipInfos[ i].mmpi_Polygons, ModelPolygon, it1)
    {
      FOREACHINSTATICARRAY(it1.Current().mp_PolygonVertices, ModelPolygonVertex, it2)
      {
        for( j=0; j<md_TransformedVertices.Count(); j++)
        {
          if( it2.Current().mpv_ptvTransformedVertex == &md_TransformedVertices[ j])
            break;
        }
        it2.Current().mpv_ptvTransformedVertex = (struct TransformedVertexData *)(size_t)j;

        for( j=0; j<md_MipInfos[ i].mmpi_TextureVertices.Count(); j++)
        {
          if( it2.Current().mpv_ptvTextureVertex == &md_MipInfos[ i].mmpi_TextureVertices[ j])
            break;
        }
        it2.Current().mpv_ptvTextureVertex = (ModelTextureVertex *)(size_t)j;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
/*
 * Routine converts mpv_ptvTransformedVertex and mpv_ptvTextureVertex from Indices to ptrs
 */
void CModelData::IndicesToPtrs()
{
  INDEX i, j;

  for( i=0; i<md_MipCt; i++)
  {
    FOREACHINSTATICARRAY(md_MipInfos[ i].mmpi_Polygons, ModelPolygon, it1)
    {
      FOREACHINSTATICARRAY(it1.Current().mp_PolygonVertices, ModelPolygonVertex, it2)
      {
        //struct ModelPolygonVertex * pMPV = &it2.Current();
        // DG: this looks like a 64-bit issue but is most probably ok, as the pointers
        //     should contain indices from PtrToIndices()
        j = (INDEX) (size_t) it2.Current().mpv_ptvTransformedVertex;
        it2.Current().mpv_ptvTransformedVertex = &md_TransformedVertices[ j];
        // DG: same here
        j = (INDEX) (size_t) it2.Current().mpv_ptvTextureVertex;
        it2.Current().mpv_ptvTextureVertex = &md_MipInfos[ i].mmpi_TextureVertices[ j];
      }
    }
  }
}

CModelCollisionBox::CModelCollisionBox(void)
{
  mcb_vCollisionBoxMin = FLOAT3D( -0.5f, 0.0f,-0.5f);
  mcb_vCollisionBoxMax = FLOAT3D(  0.5f, 2.0f, 0.5f);
  mcb_iCollisionBoxDimensionEquality = LENGTH_EQ_WIDTH;
  mcb_strName = "PART_NAME";
}

void CModelCollisionBox::Read_t(CTStream *istrFile)
{
  // Read collision box min
  (*istrFile) >> mcb_vCollisionBoxMin;
  // Read collision box size
  (*istrFile) >> mcb_vCollisionBoxMax;
  // Get "colision box dimensions equality" value
  if( (mcb_vCollisionBoxMax(2)-mcb_vCollisionBoxMin(2)) ==
      (mcb_vCollisionBoxMax(1)-mcb_vCollisionBoxMin(1)) )
  {
    mcb_iCollisionBoxDimensionEquality = HEIGHT_EQ_WIDTH;
  }
  else if( (mcb_vCollisionBoxMax(3)-mcb_vCollisionBoxMin(3)) ==
           (mcb_vCollisionBoxMax(1)-mcb_vCollisionBoxMin(1)) )
  {
    mcb_iCollisionBoxDimensionEquality = LENGTH_EQ_WIDTH;
  }
  else if( (mcb_vCollisionBoxMax(3)-mcb_vCollisionBoxMin(3)) ==
           (mcb_vCollisionBoxMax(2)-mcb_vCollisionBoxMin(2)) )
  {
    mcb_iCollisionBoxDimensionEquality = LENGTH_EQ_HEIGHT;
  }
  else
  {
    /*
    // Force them to be legal (Length = Width)
    mcb_vCollisionBoxMax(3) = mcb_vCollisionBoxMin(3) +
                             (mcb_vCollisionBoxMax(1)-mcb_vCollisionBoxMin(1));
                             */
    mcb_iCollisionBoxDimensionEquality = LENGTH_EQ_WIDTH;
  }
}

void CModelCollisionBox::ReadName_t(CTStream *istrFile)
{
  // read collision box name
  (*istrFile)>>mcb_strName;
}

void CModelCollisionBox::Write_t(CTStream *ostrFile)
{
  // Write collision box min
  (*ostrFile) << mcb_vCollisionBoxMin;
  // Write collision box size
  (*ostrFile) << mcb_vCollisionBoxMax;
  // write collision box name
  (*ostrFile) << mcb_strName;
}

CAttachedModelPosition::CAttachedModelPosition( void)
{
  amp_iCenterVertex = 0;
  amp_iFrontVertex = 1;
  amp_iUpVertex = 2;
  amp_plRelativePlacement = CPlacement3D( FLOAT3D(0,0,0), ANGLE3D(0,0,0));
}

void CAttachedModelPosition::Read_t( CTStream *strFile)
{
  *strFile >> amp_iCenterVertex;
  *strFile >> amp_iFrontVertex;
  *strFile >> amp_iUpVertex;
  *strFile >> amp_plRelativePlacement;
}

void CAttachedModelPosition::Write_t( CTStream *strFile)
{
  *strFile << amp_iCenterVertex;
  *strFile << amp_iFrontVertex;
  *strFile << amp_iUpVertex;
  *strFile << amp_plRelativePlacement;
}

//--------------------------------------------------------------------------------------------
//------------------------------------------ WRITE
void CModelData::Write_t( CTStream *pFile)  // throw char *
{
	INDEX i;

  PtrsToIndices();
  // Save main ID
  pFile->WriteID_t( CChunkID("MDAT"));

  // Save version number
  pFile->WriteID_t( CChunkID( MODEL_VERSION));

  // Save flags
  (*pFile) << md_Flags;

  #if PLATFORM_BIGENDIAN
  STUBBED("byte order");
  #endif

  // Save vertices and frames ct
  pFile->WriteFullChunk_t( CChunkID("IVTX"), &md_VerticesCt, sizeof(INDEX));
  pFile->WriteFullChunk_t( CChunkID("IFRM"), &md_FramesCt, sizeof(INDEX));

  // write array of 8-bit or 16-bit compressed vertices
  if( md_Flags & MF_COMPRESSED_16BIT)
  {
    pFile->WriteFullChunk_t( CChunkID("AV17"), &md_FrameVertices16[ 0], md_VerticesCt * md_FramesCt *
                           sizeof(struct ModelFrameVertex16));
  }
  else
  {
    pFile->WriteFullChunk_t( CChunkID("AFVX"), &md_FrameVertices8[ 0], md_VerticesCt * md_FramesCt *
                           sizeof(struct ModelFrameVertex8));
  }
  // Save frame info array
  pFile->WriteFullChunk_t( CChunkID("AFIN"), &md_FrameInfos[ 0], md_FramesCt *
                         sizeof(struct ModelFrameInfo));
  // Save frame main mip vertices array
  pFile->WriteFullChunk_t( CChunkID("AMMV"), &md_MainMipVertices[ 0], md_VerticesCt *
                         sizeof(FLOAT3D));
  // Save vertex mip-mask array
  pFile->WriteFullChunk_t( CChunkID("AVMK"), &md_VertexMipMask[ 0], md_VerticesCt *
                         sizeof(ULONG));
  // Save mip levels counter
  pFile->WriteFullChunk_t( CChunkID("IMIP"), &md_MipCt, sizeof(INDEX));

  // Save mip factors array
  pFile->WriteFullChunk_t( CChunkID("FMIP"), &md_MipSwitchFactors[ 0], MAX_MODELMIPS * sizeof(float));

  // Save all model mip infos
  for( i=0; i<md_MipCt; i++) md_MipInfos[ i].Write_t( pFile);

  // Save patches
  pFile->WriteID_t( CChunkID("PTC2"));
  for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++) md_mpPatches[ iPatch].Write_t(pFile);

  // Save texture width and height in MEX-es
  pFile->WriteFullChunk_t( CChunkID("STXW"), &md_Width, sizeof(MEX));
  pFile->WriteFullChunk_t( CChunkID("STXH"), &md_Height, sizeof(MEX));

  // Save value for shading type
  pFile->Write_t( &md_ShadowQuality, sizeof(SLONG));

  // Save static stretch value
  pFile->Write_t( &md_Stretch, sizeof(FLOAT3D));

  // Save model offset
  pFile->Write_t( &md_vCenter, sizeof(FLOAT3D));

  // Save count of collision boxes
  INDEX ctCollisionBoxes = md_acbCollisionBox.Count();
  pFile->Write_t( &ctCollisionBoxes, sizeof(INDEX));
  md_acbCollisionBox.Lock();
  // save all collision boxes
  for( INDEX iCollisionBox=0; iCollisionBox<ctCollisionBoxes; iCollisionBox++)
  {
    // save current collision box
    md_acbCollisionBox[ iCollisionBox].Write_t( pFile);
  }
  md_acbCollisionBox.Unlock();

  // save boolean defining collision type for this model
  pFile->WriteID_t( CChunkID( "COLI"));
  *pFile << md_bCollideAsCube;

  // Save count of attached positions
  INDEX ctAttachedPositions = md_aampAttachedPosition.Count();
  *pFile << ctAttachedPositions;
  FOREACHINDYNAMICARRAY(md_aampAttachedPosition, CAttachedModelPosition, itamp)
  {
    itamp->Write_t(pFile);
  }

  // Save color names (get count of valid names, write count and then write existing names)
  INDEX iValidColorsCt = 0;
  for( i=0; i<MAX_COLOR_NAMES; i++)
    if( md_ColorNames[ i] != "")
      iValidColorsCt++;
  pFile->WriteFullChunk_t( CChunkID("ICLN"), &iValidColorsCt, sizeof(INDEX));
  for( i=0; i<MAX_COLOR_NAMES; i++)
  {
    if( md_ColorNames[ i] != "")
    {
      *pFile << i;
      *pFile << md_ColorNames[ i];
    }
  }

  // Save AnimData
  CAnimData::Write_t( pFile);
  IndicesToPtrs();

  *pFile << md_colDiffuse;
  *pFile << md_colReflections;
  *pFile << md_colSpecular;
  *pFile << md_colBump;
}


//------------------------------------------ READ
void CModelData::Read_t( CTStream *pFile) // throw char *
{
  INDEX i;

  _bHasAlpha = FALSE;
  // Read main ID
  pFile->ExpectID_t( CChunkID("MDAT"));

  // Check version number
  BOOL bHasSavedCenter = FALSE;
  BOOL bHasMultipleCollisionBoxes = FALSE;
  BOOL bHasAttachedPositions = FALSE;
  BOOL bHasPolygonalPatches = FALSE;
  BOOL bHasPolygonsPerSurface = FALSE;
  BOOL bHasSavedFlagsOnStart = FALSE;
  BOOL bHasColorForReflectionAndSpecularity = FALSE;
  BOOL bHasDiffuseColor = FALSE;
  // get version ID
  CChunkID idVersion = pFile->GetID_t();
  // if this is version without stretch center then it doesn't contain multiple
  // collision boxes also
  if( CChunkID( MODEL_VERSION_WITHOUT_STRETCH_CENTER) == idVersion)
  {
  }
  // if model has stretch center but does not have multiple collision boxes
  else if( CChunkID( MODEL_VERSION_WITHOUT_MULTIPLE_COLLISION_BOXES) == idVersion)
  {
    bHasSavedCenter = TRUE;
  }
  else if( CChunkID( MODEL_VERSION_WITHOUT_ATTACHED_POSITIONS) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
  }
  else if( CChunkID( MODEL_VERSION_WITHOUT_POLYGONAL_PATCHES) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
    bHasAttachedPositions = TRUE;
  }
  else if( CChunkID( MODEL_VERSION_WITHOUT_POLYGONS_PER_SURFACE) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
    bHasAttachedPositions = TRUE;
    bHasPolygonalPatches = TRUE;
  }
  else if( CChunkID( MODEL_VERSION_WITHOUT_16_BIT_COMPRESSION) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
    bHasAttachedPositions = TRUE;
    bHasPolygonalPatches = TRUE;
    bHasPolygonsPerSurface = TRUE;
  }
  // if has saved flags on start - because 16-bit compression
  else if( CChunkID( MODEL_VERSION_WITHOUT_REFLECTION_AND_SPECULARITY) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
    bHasAttachedPositions = TRUE;
    bHasPolygonalPatches = TRUE;
    bHasPolygonsPerSurface = TRUE;
    bHasSavedFlagsOnStart = TRUE;
  }
  // has saved color for reflection and specularity
  else if( CChunkID( MODEL_VERSION_WITHOUT_DIFFUSE_COLOR) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
    bHasAttachedPositions = TRUE;
    bHasPolygonalPatches = TRUE;
    bHasPolygonsPerSurface = TRUE;
    bHasSavedFlagsOnStart = TRUE;
    bHasColorForReflectionAndSpecularity = TRUE;
  }
  // has saved diffuse color
  else if( CChunkID( MODEL_VERSION) == idVersion)
  {
    bHasSavedCenter = TRUE;
    bHasMultipleCollisionBoxes = TRUE;
    bHasAttachedPositions = TRUE;
    bHasPolygonalPatches = TRUE;
    bHasPolygonsPerSurface = TRUE;
    bHasSavedFlagsOnStart = TRUE;
    bHasColorForReflectionAndSpecularity = TRUE;
    bHasDiffuseColor = TRUE;
  }
  else
  {
    throw(TRANS("Invalid model version."));
  }

  if( bHasSavedFlagsOnStart)
  {
    (*pFile)>>md_Flags;
  }

  // Read vertices and frames ct
  pFile->ReadFullChunk_t( CChunkID("IVTX"), &md_VerticesCt, sizeof(INDEX));
  BYTESWAP(md_VerticesCt);
  md_TransformedVertices.New( md_VerticesCt);
  pFile->ReadFullChunk_t( CChunkID("IFRM"), &md_FramesCt, sizeof(INDEX));
  BYTESWAP(md_FramesCt);

  // read array of 8-bit or 16-bit compressed vertices
  if( md_Flags & MF_COMPRESSED_16BIT)
  {
    md_FrameVertices16.New( md_VerticesCt * md_FramesCt);
    CChunkID cidVerticesChunk = pFile->PeekID_t();
    // if we are loading model in old 16-bit compressed format (normals use 1 byte)
    if( cidVerticesChunk == CChunkID("AV16"))
    {
      CChunkID cidDummy = pFile->GetID_t();
      (void)cidDummy; // shut up about unused variable, compiler.
      ULONG ulDummy;
      // skip chunk size
      *pFile >> ulDummy;
      (void)ulDummy; // shut up about unused variable, compiler.
      for( INDEX iVtx=0; iVtx<md_VerticesCt * md_FramesCt; iVtx++)
      {
        (*pFile)>>md_FrameVertices16[iVtx];
        // convert 8-bit normal from index into normal defined using heading and pitch
        INDEX i8BitNormalIndex = md_FrameVertices16[iVtx].mfv_ubNormH;
        const FLOAT3D &vNormal = avGouraudNormals[i8BitNormalIndex];
        CompressNormal_HQ( vNormal, md_FrameVertices16[iVtx].mfv_ubNormH, md_FrameVertices16[iVtx].mfv_ubNormP);
      }
    }
    // load new 16-bit compressed format (normals use 2 byte) model
    else if( cidVerticesChunk == CChunkID("AV17"))
    {
      pFile->ReadFullChunk_t( CChunkID("AV17"), &md_FrameVertices16[ 0], md_VerticesCt * md_FramesCt *
                             sizeof(struct ModelFrameVertex16));

      #if PLATFORM_BIGENDIAN
      for (ULONG i = 0; i < md_VerticesCt * md_FramesCt; i++)
      {
          BYTESWAP(md_FrameVertices16[i].mfv_SWPoint.vector[0]);
          BYTESWAP(md_FrameVertices16[i].mfv_SWPoint.vector[1]);
          BYTESWAP(md_FrameVertices16[i].mfv_SWPoint.vector[2]);
      }
      #endif
    }
    else
    {
      ThrowF_t( TRANS("Expecting chunk ID for model frame vertices but found %s"), (const char *) cidVerticesChunk);
    }
  }
  else
  {
    md_FrameVertices8.New( md_VerticesCt * md_FramesCt);
    pFile->ReadFullChunk_t( CChunkID("AFVX"), &md_FrameVertices8[ 0], md_VerticesCt * md_FramesCt *
                           sizeof(struct ModelFrameVertex8));
  }

  // Allocate and Read frame info array
  md_FrameInfos.New( md_FramesCt);
  pFile->ReadFullChunk_t( CChunkID("AFIN"), &md_FrameInfos[0], md_FramesCt * sizeof(struct ModelFrameInfo));
  #if PLATFORM_BIGENDIAN
  for (ULONG i = 0; i < md_FramesCt; i++)
  {
    BYTESWAP(md_FrameInfos[i].mfi_Box.minvect.vector[0]);
    BYTESWAP(md_FrameInfos[i].mfi_Box.minvect.vector[1]);
    BYTESWAP(md_FrameInfos[i].mfi_Box.minvect.vector[2]);
    BYTESWAP(md_FrameInfos[i].mfi_Box.maxvect.vector[0]);
    BYTESWAP(md_FrameInfos[i].mfi_Box.maxvect.vector[1]);
    BYTESWAP(md_FrameInfos[i].mfi_Box.maxvect.vector[2]);
  }
  #endif

  // Allocate Read frame main mip vertices array
  md_MainMipVertices.New( md_VerticesCt);
  pFile->ReadFullChunk_t( CChunkID("AMMV"), &md_MainMipVertices[0], md_VerticesCt * sizeof(FLOAT3D));
  #if PLATFORM_BIGENDIAN
  for (ULONG i = 0; i < md_VerticesCt; i++)
  {
    BYTESWAP(md_MainMipVertices[i].vector[0]);
    BYTESWAP(md_MainMipVertices[i].vector[1]);
    BYTESWAP(md_MainMipVertices[i].vector[2]);
  }
  #endif

  // Allocate and Read vertex mip-mask array
  md_VertexMipMask.New( md_VerticesCt);
  pFile->ReadFullChunk_t( CChunkID("AVMK"), &md_VertexMipMask[0], md_VerticesCt * sizeof(ULONG));
  #if PLATFORM_BIGENDIAN
  for (ULONG i = 0; i < md_VerticesCt; i++)
  {
    BYTESWAP(md_VertexMipMask[i]);
  }
  #endif

  // Read mip levels counter
  pFile->ReadFullChunk_t( CChunkID("IMIP"), &md_MipCt, sizeof(INDEX));
  BYTESWAP(md_MipCt);

  // Read mip factors array
  pFile->ReadFullChunk_t( CChunkID("FMIP"), &md_MipSwitchFactors[0], MAX_MODELMIPS * sizeof(float));
  #if PLATFORM_BIGENDIAN
  for (ULONG i = 0; i < MAX_MODELMIPS; i++)
  {
    BYTESWAP(md_MipSwitchFactors[i]);
  }
  #endif

  // Read all model mip infos
  INDEX ctMipsRejected=0;
  // force set mdl_bFineQuality TRUE because otherwise weapon models can break.
  mdl_bFineQuality = TRUE;
  for( i=0; i<md_MipCt; i++) { 
    ModelMipInfo mmiDummy; // need one dummy mipmodel info in case of mip level rejection
    // reject mip model in case its even, and not last
    if( !mdl_bFineQuality && (i%2)==1 && i!=(md_MipCt-1)) {
      mmiDummy.Read_t( pFile, bHasPolygonalPatches, bHasPolygonsPerSurface, bHasDiffuseColor);
      mmiDummy.Clear();
      ctMipsRejected++;
    } else {
      // Notice that model's difuse color has been saved in same model format change when surface color has been added
      md_MipInfos[i-ctMipsRejected].Read_t( pFile, bHasPolygonalPatches, bHasPolygonsPerSurface, bHasDiffuseColor);
      // readjust mip scaling factors (if)
      if( i>0) md_MipSwitchFactors[i-ctMipsRejected-1] = md_MipSwitchFactors[i-1];
    }
  } 
  // readjust last mip scaling factor
  md_MipSwitchFactors[i-ctMipsRejected-1] = md_MipSwitchFactors[i-1];
  // reduce mip level count
  md_MipCt -= ctMipsRejected;

  // if patches are saved in old format
  CChunkID cidPatchChunkID = pFile->PeekID_t();
  if( cidPatchChunkID == CChunkID("STMK"))
  {
    ULONG ulOldExistingPatches;
    pFile->ReadFullChunk_t( CChunkID("STMK"), &ulOldExistingPatches, sizeof(ULONG));
    BYTESWAP(ulOldExistingPatches);

    for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
    {
      if( ((1UL << iPatch) & ulOldExistingPatches) != 0)
      {
        CTFileName fnPatchName;
        *pFile >> fnPatchName;
        try
        {
          md_mpPatches[ iPatch].mp_toTexture.SetData_t( fnPatchName);
        }
        catch (const char *strError)
        {
          (void) strError;
        }
      }
    }
  }
  // if patches are saved in new format
  else if( cidPatchChunkID == CChunkID("PTC2"))
  {
    pFile->ExpectID_t( CChunkID("PTC2"));
    for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
    {
      try
      {
        md_mpPatches[ iPatch].Read_t(pFile);
      }
      catch (const char *strError)
      {
        (void) strError;
      }
    }
  }
  else
  {
    ThrowF_t(TRANS("Expecting chunk containing patch data but found unrecognisable chunk ID."));
  }

  // Read texture width and height in MEX-es
  pFile->ReadFullChunk_t( CChunkID("STXW"), &md_Width, sizeof(MEX));
  pFile->ReadFullChunk_t( CChunkID("STXH"), &md_Height, sizeof(MEX));
  BYTESWAP(md_Width);
  BYTESWAP(md_Height);

  // in old patch format, now patch postiions are loaded
  if( cidPatchChunkID == CChunkID("STMK"))
  {
    pFile->ExpectID_t( CChunkID("POSS"));
    ULONG ulChunkSize;
    *pFile >> ulChunkSize;
    for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
    {
      // Read patch position
      *pFile >> md_mpPatches[ iPatch].mp_mexPosition;
    }
  }


  if( !bHasSavedFlagsOnStart)
  {
    // Read flags
    (*pFile)>>md_Flags;
  }

  // Read value for shading type
  (*pFile)>>md_ShadowQuality;

  // Read static stretch value
  (*pFile)>>md_Stretch;

  // if this is model with saved center
  if( bHasSavedCenter) { // read it
    (*pFile)>>md_vCenter;
  }
  // this model has been saved without center point
  else { // so just reset it
    md_vCenter = FLOAT3D(0,0,0);
  }

  // convert model to 8-bit if requested and needed
  // force set mdl_bFineQuality TRUE because otherwise weapon models can break.
  mdl_bFineQuality = TRUE;
  if( !mdl_bFineQuality && (md_Flags&MF_COMPRESSED_16BIT))
  {
    // prepare 8-bit frame vertices array
    const INDEX ctVtx = md_VerticesCt * md_FramesCt;
    md_FrameVertices8.New(ctVtx);

    // loop thru vertices
    for( INDEX iVtx=0; iVtx<ctVtx; iVtx++) { 
      ModelFrameVertex16 &mfv16 = md_FrameVertices16[iVtx];
      ModelFrameVertex8  &mfv8  = md_FrameVertices8[iVtx];
      // convert vertex coordinate
      mfv8.mfv_SBPoint(1) = mfv16.mfv_SWPoint(1) >>8;
      mfv8.mfv_SBPoint(2) = mfv16.mfv_SWPoint(2) >>8;
      mfv8.mfv_SBPoint(3) = mfv16.mfv_SWPoint(3) >>8;
      // convert normal
      const INDEX iHofs = mfv16.mfv_ubNormH>>1;
      const INDEX iPofs = mfv16.mfv_ubNormP>>1;
      mfv8.mfv_NormIndex = aubGouraudConv[iHofs*128+iPofs]; 
    }

    // done with conversion
    md_Stretch *= 256.0f;
    md_FrameVertices16.Clear();
    md_Flags &= ~MF_COMPRESSED_16BIT;
  }

  // create compressed vector center that will be used for setting object handle
  md_vCompressedCenter(1) = -md_vCenter(1)/md_Stretch(1);
  md_vCompressedCenter(2) = -md_vCenter(2)/md_Stretch(2);
  md_vCompressedCenter(3) = -md_vCenter(3)/md_Stretch(3);

  // if model has been saved with multiple collision boxes
  if( bHasMultipleCollisionBoxes)
  {
    INDEX ctCollisionBoxes;
    // get count of collision boxes
    (*pFile)>>ctCollisionBoxes;
    // add needed ammount of members
    md_acbCollisionBox.New( ctCollisionBoxes);
    md_acbCollisionBox.Lock();
    // for all saved collision boxes
    for( INDEX iCollisionBox=0; iCollisionBox<ctCollisionBoxes; iCollisionBox++)
    {
      // load current collision box from stream (without name)
      md_acbCollisionBox[iCollisionBox].Read_t( pFile);
      // load name manualy
      md_acbCollisionBox[iCollisionBox].ReadName_t( pFile);
    }
    md_acbCollisionBox.Unlock();
  }
  // else add one collision box and load it manually
  else
  {
    // add one collision box
    md_acbCollisionBox.New();
    md_acbCollisionBox.Lock();
    // read one collision box manualy (without name)
    md_acbCollisionBox[ 0].Read_t( pFile);
    md_acbCollisionBox.Unlock();
  }

  // peek chunk ID and see if we should read boolean defining collision type (speheres or cube)
  if ( pFile->PeekID_t()==CChunkID("COLI"))
  {
    pFile->ExpectID_t("COLI");
    *pFile >> md_bCollideAsCube;
  }
  else
  {
    md_bCollideAsCube = FALSE;
  }

  // if we should read attached positions
  if( bHasAttachedPositions)
  {
    // read count of attached positions
    INDEX ctAttachedPositions;
    *pFile >> ctAttachedPositions;
    md_aampAttachedPosition.New(ctAttachedPositions);
    FOREACHINDYNAMICARRAY(md_aampAttachedPosition, CAttachedModelPosition, itamp)
    {
      itamp->Read_t(pFile);
      // clamp vertices to no of model data vertices
      itamp->amp_iCenterVertex = Clamp( itamp->amp_iCenterVertex, (INDEX) 0, md_MainMipVertices.Count());
      itamp->amp_iFrontVertex = Clamp( itamp->amp_iFrontVertex, (INDEX) 0, md_MainMipVertices.Count());
      itamp->amp_iUpVertex = Clamp( itamp->amp_iUpVertex, (INDEX) 0, md_MainMipVertices.Count());
    }
  }

  // Read color names (Read count, read existing names)
  INDEX iValidColorsCt;
  pFile->ReadFullChunk_t( CChunkID("ICLN"), &iValidColorsCt, sizeof(INDEX));
  for( i=0; i<iValidColorsCt; i++)
  {
    INDEX iExistingColorName;
    *pFile >> iExistingColorName;
    *pFile >> md_ColorNames[ iExistingColorName];
  }

  // Read AnimData
  CAnimData::Read_t( pFile);
  IndicesToPtrs();

  // old models don't have saved polygons per surface
  if( !bHasPolygonsPerSurface)
  {
    // so link them manually
    LinkDataForSurfaces(TRUE);

    // for each mip model
    for( INDEX iMip = 0; iMip<md_MipCt; iMip ++)
    {
      ModelMipInfo *pMMI = &md_MipInfos[ iMip];
      // for each surface
      for( INDEX iSurface=0; iSurface<pMMI->mmpi_MappingSurfaces.Count(); iSurface++)
      {
        // convert rendering flags into new flags format (per surface)
        if (pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons.Count()>0)
        {
          ULONG ulFlags = pMMI->mmpi_Polygons[pMMI->mmpi_MappingSurfaces[iSurface].ms_aiPolygons[0]].mp_RenderFlags;
          pMMI->mmpi_MappingSurfaces[iSurface].SetRenderingParameters(ulFlags);
        }
      }
    }
  }

  // turn on diffuse map for all models of old format
  if( !bHasColorForReflectionAndSpecularity)
  {
    for( INDEX iMip = 0; iMip<md_MipCt; iMip ++)
    {
      ModelMipInfo *pMMI = &md_MipInfos[ iMip];
      for( INDEX iSurface=0; iSurface<pMMI->mmpi_MappingSurfaces.Count(); iSurface++)
      {
        pMMI->mmpi_MappingSurfaces[iSurface].ms_ulRenderingFlags |= SRF_DIFFUSE|SRF_NEW_TEXTURE_FORMAT;
      }
    }
  }

  if( bHasDiffuseColor)
  {
    *pFile >> md_colDiffuse;
  }

  // old models don't have saved colors for reflection and specularity
  if( bHasColorForReflectionAndSpecularity)
  {
    // load colors for reflections, specularity and bump
    *pFile >> md_colReflections;
    *pFile >> md_colSpecular;
    *pFile >> md_colBump;
  }

  md_bHasAlpha = _bHasAlpha;

  // precalculate rendering data
  extern void PrepareModelForRendering(CModelData &md);
  PrepareModelForRendering(*this);
}


// reference counting (override from CAnimData)
void CModelData::RemReference_internal(void)
{
  // if this model is part of edit model object
  if (md_bIsEdited) {
    // just unreference it
    MarkUnused();
  // if this model is part of model stock
  } else {
    // release it
    _pModelStock->Release(this);
  }
}


/* Get the description of this object. */
CTString CModelData::GetDescription(void)
{
  CTString str;

  ModelMipInfo &mmi0 = md_MipInfos[0];
  str.PrintF("%d mips, %d anims, %d frames, %d vtx, %d svx, %d tri",
    md_MipCt, ad_NumberOfAnims, md_FramesCt,
    mmi0.mmpi_ctMipVx, mmi0.mmpi_ctSrfVx, mmi0.mmpi_ctTriangles);

  return str;
}

//--------------------------------------------------------------------------------------------
void ModelMipInfo::Clear()
{
  mmpi_PolygonsCt = 0;                                      // reset number of polygons
  mmpi_Polygons.Clear();                                    // clear static arrays ...
  mmpi_TextureVertices.Clear();
  mmpi_MappingSurfaces.Clear();
  mmpi_aPolygonsPerPatch.Clear();
}
//--------------------------------------------------------------------------------------------
ModelPolygon::ModelPolygon()
{
  mp_RenderFlags = 0;
}
//--------------------------------------------------------------------------------------------
ModelPolygon::~ModelPolygon()
{
  mp_PolygonVertices.Clear();
}
//--------------------------------------------------------------------------------------------
ModelMipInfo::~ModelMipInfo()
{
  Clear();
}
//--------------------------------------------------------------------------------------------
MappingSurface::~MappingSurface()
{
}
//--------------------------------------------------------------------------------------------
MappingSurface::MappingSurface()
{
  ms_ulRenderingFlags &= ~SRF_SELECTED;
  ms_colDiffuse = C_WHITE|CT_OPAQUE;
  ms_colReflections = C_WHITE|CT_OPAQUE;
  ms_colSpecular = C_WHITE|CT_OPAQUE;
  ms_colBump = C_WHITE|CT_OPAQUE;
	ms_ulOnColor = SC_ALLWAYS_ON;
	ms_ulOffColor = SC_ALLWAYS_OFF;
}
//--------------------------------------------------------------------------------------------
/*
 * Object constructor
 */
CModelObject::CModelObject()
{
  mo_colBlendColor = 0xFFFFFFFF;
  mo_ColorMask = 0x7FFFFFFF;
  mo_PatchMask = 0;
  mo_iManualMipLevel = 0;
  mo_AutoMipModeling = TRUE;
  mo_Stretch = FLOAT3D(1,1,1);
}
/*
 * Destructor
 */
CModelObject::~CModelObject()
{
  for(INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    HidePatch( iPatch);
  }

  RemoveAllAttachmentModels();
}
// copy from another object of same class
void CModelObject::Copy(CModelObject &moOther)
{
  CAnimObject::Copy(moOther);
  
  mo_PatchMask        = moOther.mo_PatchMask      ;
	mo_iManualMipLevel  = moOther.mo_iManualMipLevel;
	mo_AutoMipModeling  = moOther.mo_AutoMipModeling;
  mo_Stretch              = moOther.mo_Stretch            ;
  mo_ColorMask            = moOther.mo_ColorMask          ;
  mo_iLastRenderMipLevel  = moOther.mo_iLastRenderMipLevel;
  mo_colBlendColor        = moOther.mo_colBlendColor      ;

  mo_toTexture    .Copy(moOther.mo_toTexture    );
  mo_toReflection .Copy(moOther.mo_toReflection );
  mo_toSpecular   .Copy(moOther.mo_toSpecular   );
  mo_toBump       .Copy(moOther.mo_toBump       );

  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, moOther.mo_lhAttachments, itamo) {
    CAttachmentModelObject &amoOther = *itamo;
    CAttachmentModelObject &amo = *AddAttachmentModel(amoOther.amo_iAttachedPosition);
    amo.amo_plRelative = amoOther.amo_plRelative;
    amo.amo_moModelObject.Copy(amoOther.amo_moModelObject);
  }
}

// synchronize with another model (copy animations/attachments positions etc from there)
void CModelObject::Synchronize(CModelObject &moOther)
{
  // synchronize animation
  CAnimObject::Synchronize(moOther);

  // synchronize misc parameters
  mo_PatchMask            = moOther.mo_PatchMask          ;
  mo_Stretch              = moOther.mo_Stretch            ;
  mo_ColorMask            = moOther.mo_ColorMask          ;
  mo_colBlendColor        = moOther.mo_colBlendColor      ;

  CModelData *pmd = GetData();
  CModelData *pmdOther = moOther.GetData();
  if (pmd==NULL || pmdOther==NULL) {
    return;
  }

  // for each attachment in another object
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, moOther.mo_lhAttachments, itamo) {
    CAttachmentModelObject *pamoOther = itamo;
    INDEX iap = pamoOther ->amo_iAttachedPosition;
    // get one here with same index
    CAttachmentModelObject *pamo = GetAttachmentModel(iap);
    // if found
    if (pamo!=NULL) {
  
      // sync the model itself
      pamo->amo_moModelObject.Synchronize(pamoOther->amo_moModelObject);

      // get original placements of both attachments
      pmd->md_aampAttachedPosition.Lock();
      pmdOther->md_aampAttachedPosition.Lock();
      CPlacement3D plOrg = pmd->md_aampAttachedPosition[iap].amp_plRelativePlacement;
      CPlacement3D plOtherOrg = pmdOther->md_aampAttachedPosition[iap].amp_plRelativePlacement;
      pmd->md_aampAttachedPosition.Unlock();
      pmdOther->md_aampAttachedPosition.Unlock();
      FLOAT3D &v2 = pamo->amo_plRelative.pl_PositionVector;
      FLOAT3D &v1 = pamoOther->amo_plRelative.pl_PositionVector;
      FLOAT3D &v2O = plOrg.pl_PositionVector;
      FLOAT3D &v1O = plOtherOrg.pl_PositionVector;
      ANGLE3D &a2 = pamo->amo_plRelative.pl_OrientationAngle;
      ANGLE3D &a1 = pamoOther->amo_plRelative.pl_OrientationAngle;
      ANGLE3D &a2O = plOrg.pl_OrientationAngle;
      ANGLE3D &a1O = plOtherOrg.pl_OrientationAngle;
      v2 = v2O+v1-v1O;
      a2 = a2O+a1-a1O;
    }
  }
}

//--------------------------------------------------------------------------------------------
//------------------------------------------ WRITE
void CModelObject::Write_t( CTStream *pFile)  // throw char *
{
  CAnimObject::Write_t( pFile);

  pFile->WriteID_t( CChunkID( "MODT"));
  *pFile << mo_colBlendColor;
  pFile->Write_t( &mo_PatchMask, sizeof(ULONG));
  pFile->Write_t( &mo_Stretch, sizeof(FLOAT3D));
  pFile->Write_t( &mo_ColorMask, sizeof(ULONG));
}
//------------------------------------------ READ
void CModelObject::Read_t( CTStream *pFile) // throw char *
{
  CAnimObject::Read_t( pFile);

  if( pFile->PeekID_t()==CChunkID("MODT"))
  {
    pFile->ExpectID_t( CChunkID( "MODT"));
    *pFile >> mo_colBlendColor;
  }
  // model object is saved without dynamic blend color
  else
  {
    mo_colBlendColor = 0xFFFFFFFF;
  }

  (*pFile)>>mo_PatchMask;
  (*pFile)>>mo_Stretch;
  (*pFile)>>mo_ColorMask;
  for( INDEX i=0; i<MAX_TEXTUREPATCHES; i++)
  {
    if( (mo_PatchMask & ((1UL) << i)) != 0)
    {
      ShowPatch( i);
    }
  }
}

// retrieves model's texture width
MEX CModelObject::GetWidth()
{
  return GetData()->md_Width;
};
// retrieves model's texture height
MEX CModelObject::GetHeight()
{
  return GetData()->md_Height;
};

//--------------------------------------------------------------------------------------------
/*
 * Routine retrives full model data
 */
void CModelObject::GetModelInfo(CModelInfo &miInfo)
{
  CModelData *pMD = (CModelData *) GetData();
  ASSERT( pMD != NULL);
  // copy model data values
  miInfo.mi_VerticesCt = pMD->md_VerticesCt;
  miInfo.mi_FramesCt = pMD->md_FramesCt;
  miInfo.mi_MipCt = pMD->md_MipCt;
  for( INDEX i=0; i<pMD->md_MipCt; i++)
  {
    miInfo.mi_MipInfos[ i].mi_PolygonsCt = pMD->md_MipInfos[ i].mmpi_PolygonsCt;

    // calculate triangeles
    miInfo.mi_MipInfos[ i].mi_TrianglesCt = 0;
    for( INDEX iPolygon = 0; iPolygon<pMD->md_MipInfos[ i].mmpi_PolygonsCt; iPolygon++)
    {
      miInfo.mi_MipInfos[ i].mi_TrianglesCt +=
        pMD->md_MipInfos[ i].mmpi_Polygons[ iPolygon].mp_PolygonVertices.Count()-2;
    }

    ULONG ulMipMask = (1L) << i;      // working mip model's mask
    INDEX iVertexCt = 0;
    // count vertices that exists in this mip model
    for( INDEX j=0; j<pMD->md_VerticesCt; j++)
      if( pMD->md_VertexMipMask[ j] & ulMipMask)
        iVertexCt ++;
    miInfo.mi_MipInfos[ i].mi_VerticesCt = iVertexCt;
  }
  miInfo.mi_Width = pMD->md_Width;
  miInfo.mi_Height = pMD->md_Height;
  miInfo.mi_Flags = pMD->md_Flags;
  miInfo.mi_ShadowQuality = pMD->md_ShadowQuality;
  miInfo.mi_Stretch = pMD->md_Stretch;
}

//--------------------------------------------------------------------------------------------
/*
 * Is model visible for given mip factor
 */
BOOL CModelObject::IsModelVisible( FLOAT fMipFactor)
{
  CModelData *pMD = (CModelData*)GetData();
  ASSERT( pMD != NULL);
  ASSERT( pMD->md_MipCt>0);
  // visible if no mip models or disappearence not allowed
  if( pMD->md_MipCt==0 || mdl_iLODDisappear==0) return TRUE;
  // adjust mip factor in case of dynamic stretch factor
  if( mo_Stretch != FLOAT3D(1,1,1)) {
    fMipFactor -= Log2( Max(mo_Stretch(1),Max(mo_Stretch(2),mo_Stretch(3))));
  }
  // eventually adjusted mip factor with LOD control variables
  if( mdl_iLODDisappear==2) fMipFactor = fMipFactor*mdl_fLODMul +mdl_fLODAdd;
  // return true if mip factor is smaller than last in model's mip switch factors array
  return( fMipFactor < pMD->md_MipSwitchFactors[pMD->md_MipCt-1]);
}

//--------------------------------------------------------------------------------------------
/*
 * Routine retrieves activ mip model's index
 */
INDEX CModelObject::GetMipModel( FLOAT fMipFactor)
{
  CModelData *pMD = (CModelData*)GetData();
  ASSERT( pMD != NULL);
  if( !mo_AutoMipModeling) return mo_iManualMipLevel;
  // calculate current mip model
  INDEX i;
  for( i=0; i<pMD->md_MipCt; i++) {
    if( fMipFactor < pMD->md_MipSwitchFactors[i]) return i;
  }
  return i-1;
}

//--------------------------------------------------------------------------------------------
/*
 * retrieves bounding box of given frame
 */
FLOATaabbox3D CModelObject::GetFrameBBox( INDEX iFrameNo)
{
  CModelData *pMD = (CModelData *) GetData();
  ASSERT( pMD != NULL);
  return pMD->md_FrameInfos[ iFrameNo].mfi_Box;
}

//--------------------------------------------------------------------------------------------
/*
 * Routine returns mo_AutoMipModeling flag
 */
BOOL CModelObject::IsAutoMipModeling()
{
  return mo_AutoMipModeling;
}

//--------------------------------------------------------------------------------------------
/*
 * Sets mo_AutoMipModeling flag to on
 */
void CModelObject::AutoMipModelingOn()
{
  mo_AutoMipModeling = TRUE;
  MarkChanged();
}

//--------------------------------------------------------------------------------------------
/*
 * Sets mo_AutoMipModeling flag to off
 */
void CModelObject::AutoMipModelingOff()
{
  mo_AutoMipModeling = FALSE;
  MarkChanged();
}

//--------------------------------------------------------------------------------------------
/*
 * Routine retrieves current mip level
 */
INDEX CModelObject::GetManualMipLevel()
{
  return mo_iManualMipLevel;
}
//--------------------------------------------------------------------------------------------
/*
 * Routine sets current mip level
 */
void CModelObject::SetManualMipLevel(INDEX iNewMipLevel)
{
  mo_iManualMipLevel = iNewMipLevel;
  MarkChanged();
}
//--------------------------------------------------------------------------------------------
/*
 * Routine sets given mip-level's switch factor
 */
void CModelObject::SetMipSwitchFactor(INDEX iMipLevel, float fMipFactor)
{
  CModelData *pMD = (CModelData *) GetData();
  ASSERT( iMipLevel < pMD->md_MipCt);
  pMD->md_MipSwitchFactors[ iMipLevel] = fMipFactor;
  MarkChanged();
}
//--------------------------------------------------------------------------------------------
/*
 * Select one rougher mip model level
 */
void CModelObject::NextManualMipLevel()
{
  CModelData *pMD = (CModelData *) GetData();
  if( mo_iManualMipLevel < pMD->md_MipCt-1)
  {
    mo_iManualMipLevel += 1;
    MarkChanged();
  }
}

//--------------------------------------------------------------------------------------------
/*
 * Select one more precize mip model level
 */
void CModelObject::PrevManualMipLevel()
{
  if( mo_iManualMipLevel > 0)
  {
    mo_iManualMipLevel -= 1;
    MarkChanged();
  }
}


// this function returns current value of patches mask
ULONG CModelObject::GetPatchesMask()
{
  return mo_PatchMask;
};
// use this function to set new patches combination
void CModelObject::SetPatchesMask(ULONG new_patches_mask)
{
  mo_PatchMask = new_patches_mask;
}

//--------------------------------------------------------------------------------------------
/*
 * Sets new name to a color with given index
 */
void CModelObject::SetColorName( INDEX iColor, CTString &strNewName)
{
  CModelData *pMD = (CModelData *) GetData();
  ASSERT( iColor < MAX_COLOR_NAMES);
  pMD->md_ColorNames[ iColor] = strNewName;
}
//--------------------------------------------------------------------------------------------
/*
 * Retrieves name of color with given index
 */
CTString CModelObject::GetColorName( INDEX iColor)
{
  CModelData *pMD = (CModelData *) GetData();
  ASSERT( iColor < MAX_COLOR_NAMES);
  return pMD->md_ColorNames[ iColor];
}
//--------------------------------------------------------------------------------------------
/*
 * Retrieves color of given surface
 */
COLOR CModelObject::GetSurfaceColor( INDEX iCurrentMip, INDEX iCurrentSurface)
{
  struct ModelPolygon *pPoly;
  CModelData *pMD = (CModelData *) GetData();
  if( (iCurrentMip>=pMD->md_MipCt) ||
      (iCurrentSurface>=pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces.Count()) )
  {
    return (COLOR) -1;  // !!! FIXME COLOR is unsigned, right?
  }
  for( INDEX i=0; i<pMD->md_MipInfos[ iCurrentMip].mmpi_PolygonsCt; i++)
  {
    pPoly = &pMD->md_MipInfos[ iCurrentMip].mmpi_Polygons[ i];
    if( pPoly->mp_Surface == iCurrentSurface)
    {
      return pPoly->mp_ColorAndAlpha;
    }
  }
  return 0;
}
//--------------------------------------------------------------------------------------------
/*
 * Changes color of given surface
 */
void CModelObject::SetSurfaceColor( INDEX iCurrentMip, INDEX iCurrentSurface,
                                    COLOR colNewColorAndAlpha)
{
  struct ModelPolygon *pPoly;
  CModelData *pMD = (CModelData *) GetData();
  if( (iCurrentMip>=pMD->md_MipCt) ||
      (iCurrentSurface>=pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces.Count()) )
  {
    return;
  }
  pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces[iCurrentSurface].ms_colColor = colNewColorAndAlpha;
  for( INDEX i=0; i<pMD->md_MipInfos[ iCurrentMip].mmpi_PolygonsCt; i++)
  {
    pPoly = &pMD->md_MipInfos[ iCurrentMip].mmpi_Polygons[ i];
    if( pPoly->mp_Surface == iCurrentSurface)
    {
      pPoly->mp_ColorAndAlpha = colNewColorAndAlpha;
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Retrieves rendering flags of given surface
 */
void CModelObject::GetSurfaceRenderFlags( INDEX iCurrentMip, INDEX iCurrentSurface,
      enum SurfaceShadingType &sstShading, enum SurfaceTranslucencyType &sttTranslucency,
      ULONG &ulRenderingFlags)
{
  CModelData *pMD = (CModelData *) GetData();
  if( (iCurrentMip>=pMD->md_MipCt) ||
      (iCurrentSurface>=pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces.Count()) )
  {
    return;
  }
  MappingSurface *pms = &pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces[iCurrentSurface];
  sstShading = pms->ms_sstShadingType;
  sttTranslucency = pms->ms_sttTranslucencyType;
  ulRenderingFlags = pms->ms_ulRenderingFlags;
}
//--------------------------------------------------------------------------------------------
/*
 * Changes rendering of given surface
 */
void CModelObject::SetSurfaceRenderFlags( INDEX iCurrentMip, INDEX iCurrentSurface,
      enum SurfaceShadingType sstShading, enum SurfaceTranslucencyType sttTranslucency,
      ULONG ulRenderingFlags)
{
  CModelData *pMD = (CModelData *) GetData();
  if( (iCurrentMip>=pMD->md_MipCt) ||
      (iCurrentSurface>=pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces.Count()) )
  {
    return;
  }
  // convert surface rendering parameters from old polygon flags  -- temporary !!!!
  MappingSurface *pms = &pMD->md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces[iCurrentSurface];
  pms->ms_sstShadingType = sstShading;
  pms->ms_sttTranslucencyType = sttTranslucency;
  pms->ms_ulRenderingFlags = ulRenderingFlags;
}

//--------------------------------------------------------------------------------------------
void CModelObject::ProjectFrameVertices( CProjection3D *pProjection, INDEX iMipModel)
{
  FLOAT3D f3dVertex;

  CModelData *pMD = (CModelData *) GetData();
  pProjection->ObjectHandleL() = pMD->md_vCompressedCenter;
  pProjection->ObjectStretchL() = pMD->md_Stretch;
  // apply dynamic stretch
  pProjection->ObjectStretchL()(1) *= mo_Stretch(1);
  pProjection->ObjectStretchL()(2) *= mo_Stretch(2);
  pProjection->ObjectStretchL()(3) *= mo_Stretch(3);
  pProjection->ObjectFaceForwardL() = pMD->md_Flags & (MF_FACE_FORWARD|MF_HALF_FACE_FORWARD);
  pProjection->ObjectHalfFaceForwardL() = pMD->md_Flags & MF_HALF_FACE_FORWARD;
  pProjection->Prepare();

  INDEX iCurrentFrame = GetFrame();
  ULONG ulVtxMask = (1L) << iMipModel;

  if( pMD->md_Flags & MF_COMPRESSED_16BIT)
  {
    ModelFrameVertex16 *pFrame = &pMD->md_FrameVertices16[ iCurrentFrame * pMD->md_VerticesCt];
    for( INDEX i=0; i<pMD->md_VerticesCt; i++)
    {
      if( pMD->md_VertexMipMask[ i] & ulVtxMask)
      {
        f3dVertex(1) = (float) pFrame[ i].mfv_SWPoint(1);
        f3dVertex(2) = (float) pFrame[ i].mfv_SWPoint(2);
        f3dVertex(3) = (float) pFrame[ i].mfv_SWPoint(3);
        pProjection->ProjectCoordinate( f3dVertex, pMD->md_TransformedVertices[ i].tvd_TransformedPoint);
      }
    }
  }
  else
  {
    ModelFrameVertex8 *pFrame = &pMD->md_FrameVertices8[ iCurrentFrame * pMD->md_VerticesCt];
    for( INDEX i=0; i<pMD->md_VerticesCt; i++)
    {
      if( pMD->md_VertexMipMask[ i] & ulVtxMask)
      {
        f3dVertex(1) = (float) pFrame[ i].mfv_SBPoint(1);
        f3dVertex(2) = (float) pFrame[ i].mfv_SBPoint(2);
        f3dVertex(3) = (float) pFrame[ i].mfv_SBPoint(3);
        pProjection->ProjectCoordinate( f3dVertex, pMD->md_TransformedVertices[ i].tvd_TransformedPoint);
      }
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Colorizes surfaces touching given box
 */
void CModelObject::ColorizeRegion( CDrawPort *pDP, CProjection3D *pProjection, PIXaabbox2D box,
                                   INDEX iChoosedColor, BOOL bOnColorMode)
{
  struct ModelPolygon *pPoly;
  CModelData *pMD = (CModelData *) GetData();
  struct TransformedVertexData *pTransformedVertice;
  PIX pixDPHeight = pDP->GetHeight();
  // project vertices for given mip model
  ProjectFrameVertices( pProjection, mo_iLastRenderMipLevel);
  for( INDEX j=0; j<pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_PolygonsCt; j++)
  {
    pPoly = &pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_Polygons[ j];
    for( INDEX i=0; i<pPoly->mp_PolygonVertices.Count(); i++)
    {
      pTransformedVertice = pPoly->mp_PolygonVertices[ i].mpv_ptvTransformedVertex;
      PIXaabbox2D ptBox = PIXaabbox2D( PIX2D( (SWORD) pTransformedVertice->tvd_TransformedPoint(1),
                                       pixDPHeight - (SWORD) pTransformedVertice->tvd_TransformedPoint(2)));
      if( !((box & ptBox).IsEmpty()) )
      {
        MappingSurface &ms = pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_MappingSurfaces[ pPoly->mp_Surface];
        if( bOnColorMode)
        {
          //pPoly->mp_OnColor = 1UL << iChoosedColor;
          ms.ms_ulOnColor = 1UL << iChoosedColor;
        }
        else
        {
          //pPoly->mp_OffColor = 1UL << iChoosedColor;
          ms.ms_ulOffColor = 1UL << iChoosedColor;
        }
        break;
      }
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Colorizes polygons touching given box
 */
void CModelObject::ApplySurfaceToPolygonsInRegion( CDrawPort *pDP, CProjection3D *pProjection,
                         PIXaabbox2D box, INDEX iSurface, COLOR colSurfaceColor)
{
  // project vertices for given mip model
  ProjectFrameVertices( pProjection, mo_iLastRenderMipLevel);

  struct ModelPolygon *pPoly;
  struct TransformedVertexData *pTransformedVertice;
  CModelData *pMD = (CModelData *) GetData();
  PIX pixDPHeight = pDP->GetHeight();

  for( INDEX j=0; j<pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_PolygonsCt; j++)
  {
    pPoly = &pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_Polygons[ j];
    for( INDEX i=0; i<pPoly->mp_PolygonVertices.Count(); i++)
    {
      pTransformedVertice = pPoly->mp_PolygonVertices[ i].mpv_ptvTransformedVertex;
      PIXaabbox2D ptBox = PIXaabbox2D( PIX2D( (SWORD) pTransformedVertice->tvd_TransformedPoint(1),
                                       pixDPHeight - (SWORD) pTransformedVertice->tvd_TransformedPoint(2)));
      if( !((box & ptBox).IsEmpty()) )
      {
        pPoly->mp_Surface = iSurface;
        pPoly->mp_ColorAndAlpha = colSurfaceColor;
        break;
      }
    }
  }
}

// unpack a vertex
void CModelObject::UnpackVertex(INDEX iFrame, INDEX iVertex, FLOAT3D &vVertex)
{
  CModelData *pmd = (CModelData *) GetData();
  // get decompression/stretch factors
  FLOAT3D &vDataStretch = pmd->md_Stretch;
  FLOAT3D &vObjectStretch = mo_Stretch;
  FLOAT3D vStretch;
  vStretch(1) = vDataStretch(1)*vObjectStretch(1);
  vStretch(2) = vDataStretch(2)*vObjectStretch(2);
  vStretch(3) = vDataStretch(3)*vObjectStretch(3);
  FLOAT3D vOffset = pmd->md_vCompressedCenter;

  if( pmd->md_Flags & MF_COMPRESSED_16BIT)
  {
    struct ModelFrameVertex16 *pFrame16 = &pmd->md_FrameVertices16[iFrame * pmd->md_VerticesCt];
    vVertex(1) = (pFrame16[iVertex].mfv_SWPoint(1)-vOffset(1))*vStretch(1);
    vVertex(2) = (pFrame16[iVertex].mfv_SWPoint(2)-vOffset(2))*vStretch(2);
    vVertex(3) = (pFrame16[iVertex].mfv_SWPoint(3)-vOffset(3))*vStretch(3);
  } else {
    struct ModelFrameVertex8 *pFrame8 = &pmd->md_FrameVertices8[iFrame * pmd->md_VerticesCt];
    vVertex(1) = (pFrame8[iVertex].mfv_SBPoint(1)-vOffset(1))*vStretch(1);
    vVertex(2) = (pFrame8[iVertex].mfv_SBPoint(2)-vOffset(2))*vStretch(2);
    vVertex(3) = (pFrame8[iVertex].mfv_SBPoint(3)-vOffset(3))*vStretch(3);
  }
}

CPlacement3D CModelObject::GetAttachmentPlacement(CAttachmentModelObject &amo)
{
  // project reference points to view space
  FLOAT3D vCenter, vFront, vUp;
  CModelData *pmd = (CModelData *) GetData();
  pmd->md_aampAttachedPosition.Lock();
  INDEX iPosition = amo.amo_iAttachedPosition;
  INDEX iCenter = pmd->md_aampAttachedPosition[iPosition].amp_iCenterVertex;
  INDEX iFront = pmd->md_aampAttachedPosition[iPosition].amp_iFrontVertex;
  INDEX iUp = pmd->md_aampAttachedPosition[iPosition].amp_iUpVertex;
  INDEX iFrame = GetFrame();

  UnpackVertex( iFrame, iCenter, vCenter);
  UnpackVertex( iFrame, iFront, vFront);
  UnpackVertex( iFrame, iUp, vUp);

  // make axis vectors in absolute space
  FLOAT3D &vO = vCenter;
  FLOAT3D vY = vUp-vCenter;
  FLOAT3D vZ = vCenter-vFront;
  FLOAT3D vX = vY*vZ;
  vY = vZ*vX;
  // make a rotation matrix from those vectors
  vX.Normalize();
  vY.Normalize();
  vZ.Normalize();
  FLOATmatrix3D mOrientation;
  mOrientation(1,1) = vX(1); mOrientation(1,2) = vY(1); mOrientation(1,3) = vZ(1);
  mOrientation(2,1) = vX(2); mOrientation(2,2) = vY(2); mOrientation(2,3) = vZ(2);
  mOrientation(3,1) = vX(3); mOrientation(3,2) = vY(3); mOrientation(3,3) = vZ(3);

  // make reference placement in absolute space
  CPlacement3D plPoints;
  plPoints.pl_PositionVector = vO;
  DecomposeRotationMatrixNoSnap(plPoints.pl_OrientationAngle, mOrientation);
  CPlacement3D pl = amo.amo_plRelative;
  pl.RelativeToAbsoluteSmooth(plPoints);
  pmd->md_aampAttachedPosition.Unlock();
  return pl;
}

//--------------------------------------------------------------------------------------------
/*
 * Find hitted polygon
 */
struct ModelPolygon *CModelObject::PolygonHit(
  CPlacement3D plRay, CPlacement3D plObject, INDEX iCurrentMip, FLOAT &fHitDistance)
{
  struct ModelPolygon *pResultPoly = NULL;

  fHitDistance = 100000.0f;

  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    FLOAT fHit;
    CPlacement3D plAttachment = GetAttachmentPlacement(*itamo);
    plAttachment.RelativeToAbsolute(plObject);
    struct ModelPolygon *pmp = itamo->amo_moModelObject.PolygonHit(plRay, plAttachment, iCurrentMip, fHit);
    if (fHit < fHitDistance) {
      fHitDistance = fHit;
      pResultPoly = pmp;
    }
  }
  FLOAT fHit;
  struct ModelPolygon *pmp = PolygonHitModelData((CModelData*)GetData(), plRay, plObject, iCurrentMip, fHit);
  if (fHit < fHitDistance) {
    fHitDistance = fHit;
    pResultPoly = pmp;
  }

  return pResultPoly;
}

struct ModelPolygon *CModelObject::PolygonHitModelData(CModelData *pMD,
  CPlacement3D plRay, CPlacement3D plObject, INDEX iCurrentMip, FLOAT &fHitDistance)
{
  FLOAT fClosest = -100000.0f;
  struct ModelPolygon *pPoly, *pResultPoly = NULL;
  CIntersector Intersector;

  CSimpleProjection3D spProjection;
  spProjection.ViewerPlacementL() = plRay;
  spProjection.ObjectPlacementL() = plObject;
  // project vertices for given mip model
  ProjectFrameVertices( &spProjection, iCurrentMip);

  for( INDEX j=0; j<pMD->md_MipInfos[ iCurrentMip].mmpi_PolygonsCt; j++)
  {
    Intersector.Clear();
    pPoly = &pMD->md_MipInfos[ iCurrentMip].mmpi_Polygons[ j];
    for( INDEX i=0; i<pPoly->mp_PolygonVertices.Count(); i++)
    {
      // get next vertex index (first is i)
      INDEX next = (i+1) % pPoly->mp_PolygonVertices.Count();
      // add edge to intersection object
      Intersector.AddEdge( pPoly->mp_PolygonVertices[ i].mpv_ptvTransformedVertex->tvd_TransformedPoint(1),
                         pPoly->mp_PolygonVertices[ i].mpv_ptvTransformedVertex->tvd_TransformedPoint(2),
                         pPoly->mp_PolygonVertices[ next].mpv_ptvTransformedVertex->tvd_TransformedPoint(1),
                         pPoly->mp_PolygonVertices[ next].mpv_ptvTransformedVertex->tvd_TransformedPoint(2));
    }
    if( Intersector.IsIntersecting())
    {
      FLOAT3D f3dTr0 = pPoly->mp_PolygonVertices[ 0].mpv_ptvTransformedVertex->tvd_TransformedPoint;
      FLOAT3D f3dTr1 = pPoly->mp_PolygonVertices[ 1].mpv_ptvTransformedVertex->tvd_TransformedPoint;
      FLOAT3D f3dTr2 = pPoly->mp_PolygonVertices[ 2].mpv_ptvTransformedVertex->tvd_TransformedPoint;
      FLOATplane3D fplPlane = FLOATplane3D( f3dTr0, f3dTr1, f3dTr2);

      FLOAT3D f3dHitted3DPoint = FLOAT3D(0,0,0);
      fplPlane.GetCoordinate( 3, f3dHitted3DPoint);
      if( f3dHitted3DPoint(3)<=0.0f &&  f3dHitted3DPoint(3)> fClosest)
      {
        fClosest = f3dHitted3DPoint(3);
        pResultPoly = pPoly;
      }
    }
  }
  // return closest hit polygon and the distance where it was hit
  fHitDistance = -fClosest;
  return pResultPoly;
}


//--------------------------------------------------------------------------------------------
/*
 * Colorizes hitted polygon
 */
void CModelObject::ColorizePolygon( CDrawPort *pDP, CProjection3D *projection, PIX x1, PIX y1,
                                    INDEX iChoosedColor, BOOL bOnColorMode)
{
  CPlacement3D plRay;
  CPlacement3D plObjectPlacement;

  projection->Prepare();
  projection->RayThroughPoint( FLOAT3D( (FLOAT)x1, (FLOAT)(pDP->GetHeight()-y1), 0.0f),
                               plRay);
  plObjectPlacement = projection->ObjectPlacementR();

  FLOAT fHitDistance;
  struct ModelPolygon *pPoly = PolygonHit( plRay, plObjectPlacement, mo_iLastRenderMipLevel, fHitDistance);
  if( pPoly != NULL)
  {
    CModelData *pMD = (CModelData *) GetData();
    MappingSurface &ms = pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_MappingSurfaces[ pPoly->mp_Surface];
    if( bOnColorMode)
    {
      //pPoly->mp_OnColor = 1UL << iChoosedColor;
      ms.ms_ulOnColor = 1UL << iChoosedColor;
    }
    else
    {
      //pPoly->mp_OffColor = 1UL << iChoosedColor;
      ms.ms_ulOffColor = 1UL << iChoosedColor;
    }
  }
}

void CModelObject::ApplySurfaceToPolygon( CDrawPort *pDP, CProjection3D *projection,
             PIX x1, PIX y1, INDEX iSurface, COLOR colSurfaceColor)
{
  CPlacement3D plRay;
  CPlacement3D plObjectPlacement;

  projection->Prepare();
  projection->RayThroughPoint( FLOAT3D( (FLOAT)x1, (FLOAT)(pDP->GetHeight()-y1), 0.0f),
                               plRay);
  plObjectPlacement = projection->ObjectPlacementR();

  FLOAT fHitDistance;
  struct ModelPolygon *pPoly = PolygonHit( plRay, plObjectPlacement, mo_iLastRenderMipLevel, fHitDistance);
  if( pPoly != NULL)
  {
    pPoly->mp_ColorAndAlpha = colSurfaceColor;
    pPoly->mp_Surface = iSurface;
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Picks color from hitted polygon
 */
void CModelObject::PickPolyColor(  CDrawPort *pDP, CProjection3D *projection, PIX x1, PIX y1,
                                   INDEX &iPickedColorNo, BOOL bOnColorMode)
{
  CPlacement3D plRay;
  CPlacement3D plObjectPlacement;

  projection->Prepare();
  projection->RayThroughPoint( FLOAT3D( (FLOAT)x1, (FLOAT)(pDP->GetHeight()-y1), 0.0f),
                               plRay);
  plObjectPlacement = projection->ObjectPlacementR();

  FLOAT fHitDistance;
  struct ModelPolygon *pPoly = PolygonHit( plRay, plObjectPlacement, mo_iLastRenderMipLevel, fHitDistance);
  if( pPoly != NULL)
  {
    CModelData *pMD = (CModelData *) GetData();
    MappingSurface &ms = pMD->md_MipInfos[ mo_iLastRenderMipLevel].mmpi_MappingSurfaces[ pPoly->mp_Surface];
    
    if( bOnColorMode)
    {
      iPickedColorNo = GetBit( ms.ms_ulOnColor);
    }
    else
    {
      iPickedColorNo = GetBit( ms.ms_ulOffColor);
    }
  }
}
//--------------------------------------------------------------------------------------------
INDEX CModelObject::PickPolySurface( CDrawPort *pDP, CProjection3D *projection, PIX x1, PIX y1)
{
  CPlacement3D plRay;
  CPlacement3D plObjectPlacement;

  projection->Prepare();
  projection->RayThroughPoint( FLOAT3D( (FLOAT)x1, (FLOAT)(pDP->GetHeight()-y1), 0.0f),
                               plRay);
  plObjectPlacement = projection->ObjectPlacementR();

  FLOAT fHitDistance;
  struct ModelPolygon *pPoly = PolygonHit( plRay, plObjectPlacement, mo_iLastRenderMipLevel, fHitDistance);
  if( pPoly != NULL)
  {
    return pPoly->mp_Surface;
  }
  return -1;
}

//--------------------------------------------------------------------------------------------
/*
 * Obtains index of closest vertex
 */
INDEX CModelObject::PickVertexIndex( CDrawPort *pDP, CProjection3D *pProjection, PIX x1, PIX y1,
                                     FLOAT3D &vClosestVertex)
{
  CModelData *pMD = (CModelData *) GetData();
  // project vertices for given mip model
  ProjectFrameVertices( pProjection, mo_iLastRenderMipLevel);

  FLOAT fClosest = 64.0f;
  FLOAT iClosest = -1;
  INDEX iCurrentFrame = GetFrame();
  FLOAT3D vTargetPoint = FLOAT3D( x1, pDP->GetHeight()-y1, 0.0f);
  ULONG ulVtxMask = (1L) << mo_iLastRenderMipLevel;
  // Find closest vertice
  for( INDEX iVertex=0; iVertex<pMD->md_VerticesCt; iVertex++)
  {
    if( pMD->md_VertexMipMask[ iVertex] & ulVtxMask)
    {
      FLOAT3D vProjected = pMD->md_TransformedVertices[ iVertex].tvd_TransformedPoint;
      vProjected(3) = 0.0f;
      FLOAT3D vUncompressedVertex;
      if( pMD->md_Flags & MF_COMPRESSED_16BIT)
      {
        ModelFrameVertex16 *pFrame = &pMD->md_FrameVertices16[ iCurrentFrame * pMD->md_VerticesCt];
        vUncompressedVertex(1) = (float) pFrame[ iVertex].mfv_SWPoint(1);
        vUncompressedVertex(2) = (float) pFrame[ iVertex].mfv_SWPoint(2);
        vUncompressedVertex(3) = (float) pFrame[ iVertex].mfv_SWPoint(3);
      }
      else
      {
        ModelFrameVertex8 *pFrame = &pMD->md_FrameVertices8[ iCurrentFrame * pMD->md_VerticesCt];
        vUncompressedVertex(1) = (float) pFrame[ iVertex].mfv_SBPoint(1);
        vUncompressedVertex(2) = (float) pFrame[ iVertex].mfv_SBPoint(2);
        vUncompressedVertex(3) = (float) pFrame[ iVertex].mfv_SBPoint(3);
      }

      FLOAT fDistance = Abs( ( vProjected-vTargetPoint).Length());
      if( fDistance < fClosest)
      {
        fClosest = fDistance;
        iClosest = iVertex;
        vClosestVertex(1) = vUncompressedVertex(1)*pMD->md_Stretch(1);
        vClosestVertex(2) = vUncompressedVertex(2)*pMD->md_Stretch(2);
        vClosestVertex(3) = vUncompressedVertex(3)*pMD->md_Stretch(3);
      }
    }
  }
  return ((INDEX) iClosest);
}

/*
 * Retrieves current frame's bounding box
 */
void CModelObject::GetCurrentFrameBBox( FLOATaabbox3D &MaxBB)
{
  // obtain model data ptr
  CModelData *pMD = (CModelData *)GetData();
  ASSERT( pMD != NULL);
  // get current frame
  INDEX iCurrentFrame = GetFrame();
  ASSERT( iCurrentFrame < pMD->md_FramesCt);
  // set current frame's bounding box
  MaxBB = pMD->md_FrameInfos[ iCurrentFrame].mfi_Box;
}

/*
 * Retrieves bounding box of all frames
 */
void CModelObject::GetAllFramesBBox( FLOATaabbox3D &MaxBB)
{
  // obtain model data ptr
  CModelData *pMD = (CModelData *)GetData();
  ASSERT( pMD != NULL);
  // get all frames bounding box
  pMD->GetAllFramesBBox( MaxBB);
}

FLOAT3D CModelObject::GetCollisionBoxMin(INDEX iCollisionBox)
{
  return GetData()->GetCollisionBoxMin(iCollisionBox);
}

FLOAT3D CModelObject::GetCollisionBoxMax(INDEX iCollisionBox)
{
  return GetData()->GetCollisionBoxMax(iCollisionBox);
}

// returns HEIGHT_EQ_WIDTH, LENGTH_EQ_WIDTH or LENGTH_EQ_HEIGHT
INDEX CModelObject::GetCollisionBoxDimensionEquality(INDEX iCollisionBox)
{
  return GetData()->GetCollisionBoxDimensionEquality(iCollisionBox);
}
// test it the model has alpha blending
BOOL CModelObject::HasAlpha(void)
{
  return GetData()->md_bHasAlpha || (mo_colBlendColor&0xFF)!=0xFF;
}

// retrieves number of surfaces used in given mip model
INDEX CModelObject::SurfacesCt(INDEX iMipModel){
  ASSERT( GetData() != NULL);
  return GetData()->md_MipInfos[ iMipModel].mmpi_MappingSurfaces.Count();
};
// retrieves number of polygons in given surface in given mip model
INDEX CModelObject::PolygonsInSurfaceCt(INDEX iMipModel, INDEX iSurface)
{
  ASSERT( GetData() != NULL);
  return GetData()->md_MipInfos[ iMipModel].mmpi_MappingSurfaces[iSurface].ms_aiPolygons.Count();
};

//--------------------------------------------------------------------------------------------
/*
 * Adds and shows given patch
 */
void CModelObject::ShowPatch( INDEX iMaskBit)
{
  CModelData *pMD = (CModelData *)GetData();
  ASSERT( pMD != NULL);
  if( pMD == NULL) return;
  if( (mo_PatchMask & ((1UL) << iMaskBit)) != 0) return;
  mo_PatchMask |= (1UL) << iMaskBit;
}
//--------------------------------------------------------------------------------------------
/*
 * Hides given patch
 */
void CModelObject::HidePatch( INDEX iMaskBit)
{
  CModelData *pMD = (CModelData *)GetData();
  if( pMD == NULL) return;
  if( (mo_PatchMask & ((1UL) << iMaskBit)) == 0) return;
  mo_PatchMask &= ~((1UL) << iMaskBit);
}
//--------------------------------------------------------------------------------------------
/*
 * Retrieves index of mip model that casts shadow
 */
BOOL CModelObject::HasShadow(INDEX iModelMip)
{
  CModelData *pMD = (CModelData *) GetData();
  SLONG slShadowQuality = _mrpModelRenderPrefs.GetShadowQuality();
  ASSERT( slShadowQuality >= 0);
  SLONG res = iModelMip + slShadowQuality + pMD->md_ShadowQuality;
  if( res >= pMD->md_MipCt)
    return FALSE;
  return TRUE;
}

/*
 * Set texture data for main texture in surface of this model.
 */
void CModelObject::SetTextureData(CTextureData *ptdNewMainTexture)
{
  mo_toTexture.SetData(ptdNewMainTexture);
}

CTFileName CModelObject::GetName(void)
{
  CModelData *pmd = (CModelData *) GetData();
  if( pmd == NULL) return CTString( "");
  return pmd->GetName();
}

// obtain model and set it for this object
void CModelObject::SetData_t(const CTFileName &fnmModel) // throw char *
{
  // if the filename is empty
  if (fnmModel=="") {
    // release current texture
    SetData(NULL);

  // if the filename is not empty
  } else {
    // obtain it (adds one reference)
    CModelData *pmd = _pModelStock->Obtain_t(fnmModel);
    // set it as data (adds one more reference, and remove old reference)
    SetData(pmd);
    // release it (removes one reference)
    _pModelStock->Release(pmd);
    // total reference count +1+1-1 = +1 for new data -1 for old data
  }
}

void CModelObject::SetData(CModelData *pmd)
{
  RemoveAllAttachmentModels();
  CAnimObject::SetData(pmd);
}

CModelData *CModelObject::GetData(void)
{
  return (CModelData*)CAnimObject::GetData();
}

void CModelObject::AutoSetTextures(void)
{
  CTFileName fnModel = GetName();
  CTFileName fnDiffuse;
  INDEX ctDiffuseTextures;
  CTFileName fnReflection;
  CTFileName fnSpecular;
  CTFileName fnBump;
  // extract from model's ini file informations about attachment model's textures
  try
  {
    CTFileName fnIni = fnModel.NoExt()+".ini";
    CTFileStream strmIni;
    strmIni.Open_t( fnIni);
    SLONG slFileSize = strmIni.GetStreamSize();
    // NEVER!NEVER! read after EOF
    while(strmIni.GetPos_t()<(slFileSize-4))
    {
      CChunkID id = strmIni.PeekID_t();
      if( id == CChunkID("WTEX"))
      {
        CChunkID idDummy = strmIni.GetID_t();
        (void)idDummy; // shut up about unused variable, compiler.
        strmIni >> ctDiffuseTextures;
        strmIni >> fnDiffuse;
      }
      else if( id == CChunkID("FXTR"))
      {
        CChunkID idDummy = strmIni.GetID_t();
        (void)idDummy; // shut up about unused variable, compiler.
        strmIni >> fnReflection;
      }
      else if( id == CChunkID("FXTS"))
      {
        CChunkID idDummy = strmIni.GetID_t();
        (void)idDummy; // shut up about unused variable, compiler.
        strmIni >> fnSpecular;
      }
      else if( id == CChunkID("FXTB"))
      {
        CChunkID idDummy = strmIni.GetID_t();
        (void)idDummy; // shut up about unused variable, compiler.
        strmIni >> fnBump;
      }
      else
      {
        strmIni.Seek_t(1,CTStream::SD_CUR);
      }
    }
  }
  catch (const char *strError){ (void) strError;}

  try
  {
    if( fnDiffuse != "") mo_toTexture.SetData_t( fnDiffuse);
    if( fnReflection != "") mo_toReflection.SetData_t( fnReflection);
    if( fnSpecular != "") mo_toSpecular.SetData_t( fnSpecular);
    if( fnBump != "") mo_toBump.SetData_t( fnBump);
  }
  catch (const char *strError){ (void) strError;}
}

void CModelObject::AutoSetAttachments(void)
{
  CTFileName fnModel = GetName();
  RemoveAllAttachmentModels();

  // extract from model's ini file informations about attachment model's textures
  try
  {
    CTFileName fnIni = fnModel.NoExt()+".ini";
    CTFileStream strmIni;
    strmIni.Open_t( fnIni);
    SLONG slFileSize = strmIni.GetStreamSize();
    // NEVER!NEVER! read after EOF
    while(strmIni.GetPos_t()<(slFileSize-4))
    {
      CChunkID id = strmIni.PeekID_t();
      if( id == CChunkID("ATTM"))
      {
        CChunkID idDummy = strmIni.GetID_t();
        (void)idDummy; // shut up about unused variable, compiler.
        // try to load attached models
        INDEX ctAttachedModels;
        strmIni >> ctAttachedModels;
        // read all attached models
        for( INDEX iAtt=0; iAtt<ctAttachedModels; iAtt++)
        {
          BOOL bVisible;
          CTString strName;
          CTFileName fnModel, fnDummy;
          strmIni >> bVisible;
          strmIni >> strName;
          // this data is used no more
          strmIni >> fnModel;

          INDEX iAnimation = 0;
          // new attached model format has saved index of animation
          if( strmIni.PeekID_t() == CChunkID("AMAN"))
          {
            strmIni.ExpectID_t( CChunkID( "AMAN"));
            strmIni >> iAnimation;
          }
          else
          {
            strmIni >> fnDummy; // ex model's texture
          }

          if( bVisible)
          {
            CAttachmentModelObject *pamo = AddAttachmentModel( iAtt);
            pamo->amo_moModelObject.SetData_t( fnModel);
            pamo->amo_moModelObject.AutoSetTextures();
            pamo->amo_moModelObject.StartAnim( iAnimation);
          }
        }
      }
      else
      {
        strmIni.Seek_t(1,CTStream::SD_CUR);
      }
    }
  }
  catch (const char *strError)
  {
    (void) strError;
    RemoveAllAttachmentModels();
  }

  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo)
  {
    itamo->amo_moModelObject.AutoSetAttachments();
  }
}

CAttachmentModelObject *CModelObject::AddAttachmentModel( INDEX iAttachedPosition)
{
  CModelData *pMD = (CModelData *) GetData();

  if (pMD->md_aampAttachedPosition.Count()==0) {
    return NULL;
  }
  ASSERT( iAttachedPosition >= 0);
  ASSERT( iAttachedPosition < pMD->md_aampAttachedPosition.Count());
  iAttachedPosition = Clamp(iAttachedPosition, INDEX(0), INDEX(pMD->md_aampAttachedPosition.Count()-1));
  CAttachmentModelObject *pamoNew = new CAttachmentModelObject;
  mo_lhAttachments.AddTail( pamoNew->amo_lnInMain);
  pamoNew->amo_iAttachedPosition = iAttachedPosition;
  pMD->md_aampAttachedPosition.Lock();
  pamoNew->amo_plRelative = pMD->md_aampAttachedPosition[iAttachedPosition].amp_plRelativePlacement;
  pMD->md_aampAttachedPosition.Unlock();

  return pamoNew;
}

CAttachmentModelObject *CModelObject::GetAttachmentModel( INDEX ipos)
{
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    CAttachmentModelObject &amo = *itamo;
    if (amo.amo_iAttachedPosition == ipos) {
      return &amo;
    }
  }
  return NULL;
}
CAttachmentModelObject *CModelObject::GetAttachmentModelList( INDEX ipos, ...)
{
  va_list marker;
  va_start(marker, ipos);

  CAttachmentModelObject *pamo = NULL;
  CModelObject *pmo = this;

  // while not end of list
  while(ipos>=0) {
    // get attachment
    pamo = pmo->GetAttachmentModel(ipos);
    // if not found
    if (pamo==NULL) {
      // return failure
      va_end(marker);
      return NULL;
    }
    // get next attachment in list
    pmo = &pamo->amo_moModelObject;
    ipos = va_arg( marker, INDEX);
  }
  va_end(marker);

  // return current attachment
  ASSERT(pamo!=NULL);
  return pamo;
}

void CModelObject::ResetAttachmentModelPosition( INDEX iAttachedPosition)
{
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo)
  {
    if (itamo->amo_iAttachedPosition == iAttachedPosition)
    {
      CModelData *pMD = (CModelData *) GetData();
      pMD->md_aampAttachedPosition.Lock();
      itamo->amo_plRelative = pMD->md_aampAttachedPosition[iAttachedPosition].amp_plRelativePlacement;
      pMD->md_aampAttachedPosition.Unlock();
      return;
    }
  }
}

void CModelObject::RemoveAttachmentModel( INDEX iAttachedPosition)
{
  FORDELETELIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    if (itamo->amo_iAttachedPosition == iAttachedPosition) {
      itamo->amo_lnInMain.Remove();
      delete &*itamo;
      return;
    }
  }
}

void CModelObject::RemoveAllAttachmentModels(void)
{
  FORDELETELIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    itamo->amo_lnInMain.Remove();
    delete &*itamo;
  }
}

void CModelObject::StretchModel(const FLOAT3D &vStretch)
{
  mo_Stretch = vStretch;
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    itamo->amo_moModelObject.StretchModel(vStretch);
  }
}

void CModelObject::StretchModelRelative(const FLOAT3D &vStretch)
{
  mo_Stretch(1) *= vStretch(1);
  mo_Stretch(2) *= vStretch(2);
  mo_Stretch(3) *= vStretch(3);
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itamo) {
    itamo->amo_moModelObject.StretchModelRelative(vStretch);
  }
}

void CModelObject::StretchSingleModel(const FLOAT3D &vStretch)
{
  mo_Stretch = vStretch;
}


// get amount of memory used by this object
SLONG CModelObject::GetUsedMemory(void)
{
  // initial size
  SLONG slUsedMemory = sizeof(CModelObject);
  // add attachment(s) size
  FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo_lhAttachments, itat) {
    slUsedMemory += sizeof(CAttachmentModelObject) - sizeof(CModelObject);
    itat->amo_moModelObject.GetUsedMemory();
  }
  // done
  return slUsedMemory;
}
