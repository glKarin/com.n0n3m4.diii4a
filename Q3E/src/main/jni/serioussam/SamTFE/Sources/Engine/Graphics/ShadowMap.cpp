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

#include <Engine/StdH.h>

#include <Engine/Graphics/ShadowMap.h>

#include <Engine/Base/Console.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/GfxProfile.h>
#include <Engine/Brushes/Brush.h>

#include <Engine/Base/Statistics_Internal.h>


#define SHADOWMAXBYTES (256*256*4*4/3)

extern INDEX shd_iStaticSize;    
extern INDEX shd_iDynamicSize;    
extern INDEX shd_bFineQuality;
extern INDEX shd_iDithering;
extern INDEX shd_bDynamicMipmaps;

extern INDEX gap_bAllowSingleMipmap;
extern FLOAT gfx_tmProbeDecay;

extern BOOL _bShadowsUpdated;
extern BOOL _bMultiPlayer;


/*
 * Routines that manipulates with shadow cluster map class
 */

CShadowMap::CShadowMap()
{
  sm_pulCachedShadowMap = NULL;
  sm_pulDynamicShadowMap = NULL;
  sm_slMemoryUsed = 0;
  sm_ulObject = NONE;
  sm_ulProbeObject = NONE;
  sm_ulInternalFormat = NONE;
  sm_iRenderFrame = -1;
  sm_ulFlags = NONE;
  Clear();
}


CShadowMap::~CShadowMap()
{
  Clear();
}


// report shadowmap memory usage (in bytes)
ULONG CShadowMap::GetShadowSize(void)
{
  CBrushPolygon *pbpo=((CBrushShadowMap *)this)->GetBrushPolygon();
  ULONG ulFlags=pbpo->bpo_ulFlags;
  BOOL bIsTransparent = (ulFlags&BPOF_PORTAL) && !(ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT));
  BOOL bTakesShadow = !(ulFlags&BPOF_FULLBRIGHT);
  BOOL bIsFlat = sm_pulCachedShadowMap==&sm_colFlat;
  if( bIsTransparent || !bTakesShadow || bIsFlat) return 0;  // not influenced
  const PIX pixSizeU = sm_mexWidth >>sm_iFirstMipLevel;
  const PIX pixSizeV = sm_mexHeight>>sm_iFirstMipLevel;
  return pixSizeU*pixSizeV *BYTES_PER_TEXEL;
}


// cache the shadow map
void CShadowMap::Cache( INDEX iWantedMipLevel)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_CACHESHADOW);
  _bShadowsUpdated = TRUE;

  // level must be in valid range and caching has to be needed
  ASSERT( iWantedMipLevel>=sm_iFirstMipLevel && iWantedMipLevel<=sm_iLastMipLevel);
  ASSERT( sm_pulCachedShadowMap==NULL || iWantedMipLevel<sm_iFirstCachedMipLevel);

  // dynamic layers are invalid when shadowmap is cached
  sm_ulFlags |= SMF_DYNAMICINVALID;
  if( sm_pulDynamicShadowMap!=NULL) {
    FreeMemory( sm_pulDynamicShadowMap);
    sm_pulDynamicShadowMap = NULL;
  }

  // calculate (new) amount of memory
  const PIX pixSizeU  = sm_mexWidth >>iWantedMipLevel;
  const PIX pixSizeV  = sm_mexHeight>>iWantedMipLevel;
  const SLONG slSize  = GetMipmapOffset( 15, pixSizeU, pixSizeV) *BYTES_PER_TEXEL;
  const BOOL bWasFlat = sm_pulCachedShadowMap==&sm_colFlat;
  const BOOL bCached  = sm_pulCachedShadowMap!=NULL;

  // determine whether shadowmap is all flat (once flat - always flat!)
  if( IsShadowFlat(sm_colFlat)) {
    // release memory if allocated
    if( bCached) {
      ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
      if( !bWasFlat) FreeMemory( sm_pulCachedShadowMap);
    }
    sm_pulCachedShadowMap = &sm_colFlat;
    sm_iFirstCachedMipLevel = iWantedMipLevel;
    sm_slMemoryUsed = slSize;
    // add it to shadow list
    if( !sm_lnInGfx.IsLinked()) _pGfx->gl_lhCachedShadows.AddTail(sm_lnInGfx);
    _pfGfxProfile.StopTimer( CGfxProfile::PTI_CACHESHADOW);
    return;
  }

  // if not yet allocated
  if( !bCached || bWasFlat)
  {
    // allocate the memory
    sm_pulCachedShadowMap = (ULONG*)AllocMemory(slSize);
    sm_slMemoryUsed = slSize;
    ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
  }
  // if already allocated, but too small
  else if( iWantedMipLevel<sm_iFirstCachedMipLevel)
  {
    // allocate new block
    ULONG *pulNew = (ULONG*)AllocMemory(slSize);
    ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
    if( slSize>sm_slMemoryUsed && !bWasFlat) {
      // copy old shadow map at the end of buffer
      memcpy( pulNew + (slSize-sm_slMemoryUsed)/BYTES_PER_TEXEL, sm_pulCachedShadowMap, sm_slMemoryUsed);
    } // free old block if needed and use the new one
    if( !bWasFlat) FreeMemory( sm_pulCachedShadowMap); 
    sm_pulCachedShadowMap = pulNew;
    sm_slMemoryUsed = slSize;
    ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
  } else {
    // WHAT?
    ASSERTALWAYS( "Trying to cache shadowmap again in the same mipmap!");
  }

  // let the higher level driver mix its layers
  INDEX iLastMipLevelToCache = Min( sm_iLastMipLevel, sm_iFirstCachedMipLevel-1);
  sm_iFirstCachedMipLevel = iWantedMipLevel;
  ASSERT( iWantedMipLevel <= iLastMipLevelToCache); 

  // colorize shadowmap?
  extern INDEX shd_bColorize;
  if( _bMultiPlayer) shd_bColorize = FALSE; // don't allow in multiplayer mode!
  if( shd_bColorize) {
    #define GSIZE 4.0f
    #define RSIZE 8.0f
    FLOAT fLogSize = Log2((sm_mexWidth>>sm_iFirstCachedMipLevel) * (sm_mexHeight>>sm_iFirstCachedMipLevel)) /2;
    fLogSize = Max(fLogSize,GSIZE) -GSIZE;
    FLOAT fR = fLogSize / (RSIZE-GSIZE);
    COLOR colSize;
    if( fR>0.5f) colSize = LerpColor( C_dYELLOW, C_dRED,  (fR-0.5f)*2);
    else         colSize = LerpColor( C_dGREEN, C_dYELLOW, fR*2);
    // fill!
    for( INDEX iPix=0; iPix<sm_slMemoryUsed/4; iPix++) sm_pulCachedShadowMap[iPix] = ByteSwap(colSize);
  }
  // no colorization - just mix the layers in
  else MixLayers( iWantedMipLevel, iLastMipLevelToCache);

  // add it to shadow list
  if( !sm_lnInGfx.IsLinked()) _pGfx->gl_lhCachedShadows.AddTail( sm_lnInGfx);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_CACHESHADOW);
}


// update dynamic layers of the shadow map
// (returns mip in which shadow needs to be uploaded)
ULONG CShadowMap::UpdateDynamicLayers(void)
{
  // call this only if needed
  ASSERT( sm_ulFlags&SMF_DYNAMICINVALID);
  sm_ulFlags &= ~SMF_DYNAMICINVALID;
  
  // if there are no dynamic layers
  if( !HasDynamicLayers()) {
    // free dynamic shadows if allocated
    if( sm_pulDynamicShadowMap!=NULL) {
      _bShadowsUpdated = TRUE;
      ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
      FreeMemory( sm_pulDynamicShadowMap);
      sm_pulDynamicShadowMap = NULL;
      return sm_iFirstCachedMipLevel;
    } else return 31;
  }

  _pfGfxProfile.StartTimer( CGfxProfile::PTI_CACHESHADOW);

  // allocate the memory if not yet allocated
  if( sm_pulDynamicShadowMap==NULL) {
    ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
    sm_pulDynamicShadowMap = (ULONG*)AllocMemory(sm_slMemoryUsed);
  }

  // determine and clamp to max allowed dynamic shadow dimension
  const INDEX iMinSize  = Max( shd_iStaticSize-2L, 5L);
  // shd_iDynamicSize must be equal to shd_iStaticSize otherwise whole sectors are illuminated during fire.
  shd_iDynamicSize      = shd_iStaticSize; // Clamp( shd_iDynamicSize, iMinSize, shd_iStaticSize);
  PIX pixClampAreaSize  = 1L<<(shd_iDynamicSize*2);
  INDEX iFinestMipLevel = sm_iFirstCachedMipLevel + 
                          ClampTextureSize( pixClampAreaSize, _pGfx->gl_pixMaxTextureDimension,
                                            sm_mexWidth>>sm_iFirstCachedMipLevel, sm_mexHeight>>sm_iFirstCachedMipLevel);
  // check if need to generate only one mip-map
  INDEX iLastMipLevel = sm_iLastMipLevel;
  if( !shd_bDynamicMipmaps && gap_bAllowSingleMipmap) iLastMipLevel = iFinestMipLevel;
  // let the higher level driver mix its layers
  MixLayers( iFinestMipLevel, iLastMipLevel, TRUE);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_CACHESHADOW);

  // skip if there was nothing to mix-in
  if( sm_ulFlags&SMF_DYNAMICBLACK) return 31;
  _bShadowsUpdated = TRUE;
  return iFinestMipLevel;
}


// invalidate the shadow map
void CShadowMap::Invalidate( BOOL bDynamicOnly/*=FALSE*/)
{
  // if only dynamic layers are to be uncached
  if( bDynamicOnly) {
    // just mark that they are not valid any more
    sm_ulFlags |= SMF_DYNAMICINVALID;
  // if static layers are to be uncached
  } else {
    // mark that no mipmaps are cached
    sm_iFirstCachedMipLevel = 31;
  }
}


// mark that shadow has been drawn
void CShadowMap::MarkDrawn(void)
{
  // remove from list 
  ASSERT( sm_lnInGfx.IsLinked());
  sm_lnInGfx.Remove();
  // set time stamp
  sm_tvLastDrawn = _pTimer->GetLowPrecisionTimer();
  // put at the end of the list
  _pGfx->gl_lhCachedShadows.AddTail(sm_lnInGfx);
}


// uncache the shadow map (returns total ammount of memory that has been freed)
SLONG CShadowMap::Uncache( void)
{
  _bShadowsUpdated = TRUE;
  // discard uploaded portion
  if( sm_ulObject!=NONE) {
    gfxDeleteTexture(sm_ulObject);
    gfxDeleteTexture(sm_ulProbeObject);
    sm_ulInternalFormat = NONE;
  }
  SLONG slFreed = 0;
  // if dynamic allocated, release memory
  if( sm_pulDynamicShadowMap != NULL) {
    FreeMemory( sm_pulDynamicShadowMap);
    sm_pulDynamicShadowMap = NULL;
    ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
    slFreed += sm_slMemoryUsed;
  }
  // if static non-flat has been allocated 
  if( sm_pulCachedShadowMap!=NULL) {
    // release memory
    ASSERT( sm_slMemoryUsed>0 && sm_slMemoryUsed<=SHADOWMAXBYTES);
    if( sm_pulCachedShadowMap!=&sm_colFlat) {
      FreeMemory( sm_pulCachedShadowMap);
      slFreed += sm_slMemoryUsed;
    } else slFreed += sizeof(sm_colFlat);
  }
  // reset params
  sm_iFirstCachedMipLevel = 31;
  sm_pulCachedShadowMap = NULL;
  sm_slMemoryUsed = 0;
  sm_tvLastDrawn = (__int64) 0;
  sm_iRenderFrame = -1;
  sm_ulFlags = NONE;
  sm_tpLocal.Clear();
  // if added to list of all shadows,  remove from there
  if( sm_lnInGfx.IsLinked()) sm_lnInGfx.Remove();
  return slFreed;
}


// clear the object
void CShadowMap::Clear()
{
  // uncache the shadow map
  Uncache();
  // reset structure members
  sm_pulCachedShadowMap  = NULL;
  sm_pulDynamicShadowMap = NULL;
  sm_iFirstMipLevel = 0;
  sm_slMemoryUsed = 0;
  sm_tvLastDrawn = (__int64) 0;
  sm_mexOffsetX = 0;
  sm_mexOffsetY = 0;
  sm_mexWidth  = 0;
  sm_mexHeight = 0;
  sm_ulFlags = NONE;
}


// initialize the shadow map
void CShadowMap::Initialize( INDEX iMipLevel, MEX mexOffsetX, MEX mexOffsetY, MEX mexWidth, MEX mexHeight)
{
  // clear old shadow
  Clear();
  // just remember new values
  sm_iFirstMipLevel = iMipLevel;
  sm_mexOffsetX = mexOffsetX;
  sm_mexOffsetY = mexOffsetY;
  sm_mexWidth   = mexWidth;
  sm_mexHeight  = mexHeight;
  sm_iLastMipLevel = FastLog2( Min(mexWidth, mexHeight));
  ASSERT( (mexWidth >>sm_iLastMipLevel)>=1);
  ASSERT( (mexHeight>>sm_iLastMipLevel)>=1);
}


// skip old shadows saved in stream
void CShadowMap::Read_old_t(CTStream *pstrm) // throw char *
{
  Clear();
  // read shadow map header
  //  pstrm->ExpectID_t( CChunkID("CTSM")); // read in Read_t()
  if( pstrm->GetSize_t() != 5*4) throw( TRANS("Invalid shadow cluster map file."));

  INDEX idx; // this was the only way I could coerce GCC into playing. --ryan.

  *pstrm >> idx;
  sm_iFirstMipLevel = idx;
  INDEX iNoOfMipmaps;
  *pstrm >> iNoOfMipmaps;
  *pstrm >> idx;
  sm_mexOffsetX = idx;
  *pstrm >> idx;
  sm_mexOffsetY = idx;
  *pstrm >> idx;
  sm_mexWidth = idx;
  *pstrm >> idx;
  sm_mexHeight = idx;

  BOOL bStaticImagePresent, bAnimatingImagePresent;
  *pstrm >> (INDEX&)bStaticImagePresent;
  *pstrm >> (INDEX&)bAnimatingImagePresent;

  // skip mip-map offsets
  pstrm->Seek_t( MAX_MEX_LOG2*4, CTStream::SD_CUR);

  // skip the shadow map data
  if( bStaticImagePresent) {
    pstrm->ExpectID_t("SMSI");
    SLONG slSize;
    *pstrm>>slSize;
    pstrm->Seek_t(slSize, CTStream::SD_CUR);
  }
  if( bAnimatingImagePresent) {
    pstrm->ExpectID_t("SMAI");
    SLONG slSize;
    *pstrm>>slSize;
    pstrm->Seek_t(slSize, CTStream::SD_CUR);
  }
}


// reads image info raw format from file
void CShadowMap::Read_t( CTStream *pstrm)   // throw char *
{
  Clear();

  // read the header chunk ID
  CChunkID cidHeader = pstrm->GetID_t();
  // if it is the old shadow format
  if (cidHeader==CChunkID("CTSM")) {
    // read shadows in old format
    Read_old_t(pstrm);
    return;
  // if it is not the new shadow format
  } else if (!(cidHeader==CChunkID("LSHM"))) { // layered shadow map
    // error
    FatalError(TRANS("Error loading shadow map! Wrong header chunk."));
  }

  // load the shadow map data
  *pstrm >> sm_ulFlags;
  *pstrm >> sm_iFirstMipLevel;
  *pstrm >> sm_mexOffsetX;
  *pstrm >> sm_mexOffsetY;
  *pstrm >> sm_mexWidth;
  *pstrm >> sm_mexHeight;

  sm_iLastMipLevel = FastLog2( Min(sm_mexWidth, sm_mexHeight));
  ASSERT((sm_mexWidth >>sm_iLastMipLevel)>=1);
  ASSERT((sm_mexHeight>>sm_iLastMipLevel)>=1);

  // read the layers of the shadow
  ReadLayers_t(pstrm);
}


// writes shadow cluster map format to file
void CShadowMap::Write_t( CTStream *pstrm) // throw char *
{
  pstrm->WriteID_t("LSHM"); // layered shadow map

  // load the shadow map data
  *pstrm << sm_ulFlags;
  *pstrm << sm_iFirstMipLevel;
  *pstrm << sm_mexOffsetX;
  *pstrm << sm_mexOffsetY;
  *pstrm << sm_mexWidth;
  *pstrm << sm_mexHeight;
  // write the layers of the shadow
  WriteLayers_t(pstrm);
}


// mix all layers into cached shadow map
void CShadowMap::MixLayers( INDEX iFirstMip, INDEX iLastMip, BOOL bDynamic/*=FALSE*/)
{
  // base function is used only for testing
  (void)iFirstMip;
  (void)iLastMip;
  // just fill with white
  ULONG ulValue = ByteSwap(C_WHITE);
  ASSERT( sm_pulCachedShadowMap!=NULL);
  if( sm_pulCachedShadowMap==NULL || sm_pulCachedShadowMap==&sm_colFlat) return;
  for( INDEX i=0; i<(sm_mexWidth>>sm_iFirstMipLevel)*(sm_mexHeight>>sm_iFirstMipLevel); i++) {
    sm_pulCachedShadowMap[i] = ulValue;
  }
}


// check if all layers are up to date
void CShadowMap::CheckLayersUpToDate(void)
{
  NOTHING;
}


// test if there is any dynamic layer
BOOL CShadowMap::HasDynamicLayers(void)
{
  return FALSE;
}


void CShadowMap::ReadLayers_t( CTStream *pstrm)  // throw char *
{
}

void CShadowMap::WriteLayers_t( CTStream *pstrm) // throw char *
{
}



// prepare shadow map for upload and bind (returns whether the shadowmap is flat or not)
void CShadowMap::Prepare(void)
{
  // determine probing
  ASSERT(this!=NULL);
  extern BOOL ProbeMode( CTimerValue tvLast);
  BOOL bUseProbe = ProbeMode(sm_tvLastDrawn);

  // determine and clamp to max allowed shadow dimension
  shd_iStaticSize = Clamp( shd_iStaticSize, (INDEX)5, (INDEX)8);
  PIX pixClampAreaSize = 1L<<(shd_iStaticSize*2);
  // determine largest allowed mip level
  INDEX iFinestMipLevel = sm_iFirstMipLevel + 
                          ClampTextureSize( pixClampAreaSize, _pGfx->gl_pixMaxTextureDimension,
                                            sm_mexWidth>>sm_iFirstMipLevel, sm_mexHeight>>sm_iFirstMipLevel);
  // make sure we didn't run out of range
  INDEX iWantedMipLevel   = ClampUp( iFinestMipLevel, sm_iLastMipLevel);
  sm_iFirstUploadMipLevel = 31;
  const PIX pixShadowSize = (sm_mexWidth>>iFinestMipLevel) * (sm_mexHeight>>iFinestMipLevel);

  // see if shadowmap can be pulled out of probe mode
  if( pixShadowSize<=16*16*4 || ((sm_ulFlags&SMF_PROBED) && _pGfx->gl_slAllowedUploadBurst>=0)) bUseProbe = FALSE;
  if( bUseProbe) {
    // adjust mip-level for probing
    ULONG *pulDummy = NULL;
    PIX pixProbeWidth  = sm_mexWidth >>iFinestMipLevel;
    PIX pixProbeHeight = sm_mexHeight>>iFinestMipLevel;
    INDEX iMipOffset = GetMipmapOfSize( 16*16, pulDummy, pixProbeWidth, pixProbeHeight);
    if( iMipOffset<2) bUseProbe = FALSE;
    else iWantedMipLevel += iMipOffset;
  }

  // cache if it is not cached at all of not in this mip level
  if( sm_pulCachedShadowMap==NULL || iWantedMipLevel<sm_iFirstCachedMipLevel) {
    Cache( iWantedMipLevel);
    ASSERT( sm_iFirstCachedMipLevel<31);
    sm_iFirstUploadMipLevel = sm_iFirstCachedMipLevel;
  }

  // update the dynamic layers if they're invalid
  if( sm_ulFlags&SMF_DYNAMICINVALID) {
    INDEX iRet = UpdateDynamicLayers();
    if( iRet<31) sm_iFirstUploadMipLevel = iRet;
  }

  // update statistics if not updated already for this frame
  if( sm_iRenderFrame != _pGfx->gl_iFrameNumber) {
    sm_iRenderFrame = _pGfx->gl_iFrameNumber;
    // determine size and update
    SLONG slBytes = pixShadowSize * gfxGetFormatPixRatio(sm_ulInternalFormat);
    if( !sm_tpLocal.tp_bSingleMipmap) slBytes = slBytes *4/3;
    _sfStats.IncrementCounter( CStatForm::SCI_SHADOWBINDS, 1);
    _sfStats.IncrementCounter( CStatForm::SCI_SHADOWBINDBYTES, slBytes);
  }

  // reduce allowed burst value if upload is required in non-probe mode
  if( !bUseProbe && sm_iFirstUploadMipLevel<31) {
    const PIX pixWidth   = sm_mexWidth  >>sm_iFirstUploadMipLevel;
    const PIX pixHeight  = sm_mexHeight >>sm_iFirstUploadMipLevel;
    const INDEX iPixSize = shd_bFineQuality ? 4 : 2;
    SLONG slSize = pixWidth*pixHeight *iPixSize;
   _pGfx->gl_slAllowedUploadBurst -= slSize;
  }
  // update probe requirements
  if( bUseProbe) sm_ulFlags |= SMF_WANTSPROBE;
  else           sm_ulFlags &=~SMF_WANTSPROBE;
}



// provide the data for uploading 
void CShadowMap::SetAsCurrent(void)
{
  ASSERT( sm_pulCachedShadowMap!=NULL && sm_iFirstCachedMipLevel<31);

  // eventually re-adjust LOD bias
  extern FLOAT _fCurrentLODBias;
  extern void UpdateLODBias( const FLOAT fLODBias);
  if( _fCurrentLODBias != _pGfx->gl_fTextureLODBias) UpdateLODBias( _pGfx->gl_fTextureLODBias);

  // determine actual need for upload and eventaully colorize shadowmaps
  const BOOL bFlat = IsFlat();

  // done here if flat and non-dynamic
  if( bFlat) {
    // bind flat texture
    _ptdFlat->SetAsCurrent();
    MarkDrawn();
    return;
  }

  // init use probe flag
  BOOL bUseProbe = (sm_ulFlags & SMF_WANTSPROBE);

  // if needs to be uploaded
  if( sm_iFirstUploadMipLevel<31 || ((sm_ulFlags&SMF_DYNAMICUPLOADED) && (sm_ulFlags&SMF_DYNAMICBLACK)))
  { 
    // generate bind number(s) if needed
    if( sm_ulObject==NONE) {
      gfxGenerateTexture( sm_ulObject); 
      sm_pixUploadWidth = sm_pixUploadHeight = 0;
      sm_ulInternalFormat = NONE;
    }
    // determine shadow map pointer (static or dynamic shadow)
    ULONG *pulShadowMap = sm_pulCachedShadowMap;
    BOOL bSingleMipmap  = FALSE;
    sm_ulFlags &= ~SMF_DYNAMICUPLOADED;
    if( sm_pulDynamicShadowMap!=NULL && !(sm_ulFlags&SMF_DYNAMICBLACK)) {
      pulShadowMap = sm_pulDynamicShadowMap;
      if( !shd_bDynamicMipmaps && gap_bAllowSingleMipmap) bSingleMipmap = TRUE;
      sm_ulFlags |= SMF_DYNAMICUPLOADED;
      bUseProbe = FALSE; // don't probe dynamic shadowmaps
    }

    // reset mapping parameters if needed
    if( sm_tpLocal.tp_bSingleMipmap != bSingleMipmap) {
      sm_tpLocal.Clear();
      sm_tpLocal.tp_bSingleMipmap = bSingleMipmap;
      sm_pixUploadWidth = sm_pixUploadHeight = 0; // will not use subimage
    }

    // determine corresponding shadowmap's texture internal format, memory offset and flatness
    if( sm_iFirstUploadMipLevel>30) sm_iFirstUploadMipLevel = sm_iFirstCachedMipLevel;
    PIX pixWidth=1, pixHeight=1;
    pixWidth      = sm_mexWidth  >>sm_iFirstUploadMipLevel;
    pixHeight     = sm_mexHeight >>sm_iFirstUploadMipLevel;
    pulShadowMap += sm_slMemoryUsed/BYTES_PER_TEXEL - GetMipmapOffset( 15, pixWidth, pixHeight);

    // paranoid
    ASSERT( pixWidth>0 && pixHeight>0);

    // determine internal shadow texture format and usage of faster glTexSubImage function instead of slow glTexImage
    BOOL bUseSubImage = TRUE;
    ULONG ulInternalFormat = TS.ts_tfRGB5;
    if( !(_pGfx->gl_ulFlags&GLF_32BITTEXTURES)) shd_bFineQuality = FALSE;
    if( shd_bFineQuality)   ulInternalFormat = TS.ts_tfRGB8;
    if( _slShdSaturation<4) ulInternalFormat = TS.ts_tfL8; // better quality for grayscale shadow mode
    // eventually re-adjust uploading parameters
    if( sm_pixUploadWidth!=pixWidth || sm_pixUploadHeight!=pixHeight || sm_ulInternalFormat!=ulInternalFormat) {
      sm_pixUploadWidth   = pixWidth;
      sm_pixUploadHeight  = pixHeight;
      sm_ulInternalFormat = ulInternalFormat;
      bUseSubImage = FALSE;
    }

    // upload probe (if needed)
    if( bUseProbe) {
      sm_ulFlags |= SMF_PROBED;
      if( sm_ulProbeObject==NONE) gfxGenerateTexture( sm_ulProbeObject); 
      CTexParams tpTmp = sm_tpLocal;
      gfxSetTexture( sm_ulProbeObject, tpTmp);
      gfxUploadTexture( pulShadowMap, pixWidth, pixHeight, TS.ts_tfRGB5, FALSE);
    } else {
      // upload shadow in required format and size
      if( sm_ulFlags&SMF_PROBED) { // cannot subimage shadowmap that has been probed
        bUseSubImage = FALSE;
        sm_ulFlags &= ~SMF_PROBED;
      }
      // colorize mipmaps if needed
      extern INDEX tex_bColorizeMipmaps;
      if( tex_bColorizeMipmaps && pixWidth>1 && pixHeight>1) ColorizeMipmaps( 1, pulShadowMap, pixWidth, pixHeight);
      MarkDrawn(); // mark that shadowmap has been referenced
      gfxSetTexture( sm_ulObject, sm_tpLocal);
      gfxUploadTexture( pulShadowMap, pixWidth, pixHeight, ulInternalFormat, bUseSubImage);
    }
    // paranoid android
    ASSERT( sm_iFirstCachedMipLevel<31 && sm_pulCachedShadowMap!=NULL);
    return;
  }

  // set corresponding probe or texture frame as current
  if( bUseProbe && sm_ulProbeObject!=NONE && (_pGfx->gl_slAllowedUploadBurst<0 || (sm_ulFlags&SMF_PROBED))) {  
    CTexParams tpTmp = sm_tpLocal;
    gfxSetTexture( sm_ulProbeObject, tpTmp);
    return;
  } 

  // set non-probe shadowmap and mark that this shadowmap has been drawn
  gfxSetTexture( sm_ulObject, sm_tpLocal);
  MarkDrawn();
}




// returns used memory - static, dynamic and uploaded size separately, slack space ratio (0-1 float)
// and whether the shadowmap is flat or not
BOOL CShadowMap::GetUsedMemory( SLONG &slStaticSize, SLONG &slDynamicSize, SLONG &slUploadSize, FLOAT &fSlackRatio)
{
  const BOOL bFlat = (sm_pulCachedShadowMap==&sm_colFlat);

  // determine static portion size
  slStaticSize = 0;
  if( sm_pulCachedShadowMap!=NULL) slStaticSize = sm_slMemoryUsed;

  // determine dynamic portion size
  slDynamicSize = 0;
  if( sm_pulDynamicShadowMap!=NULL) slDynamicSize = sm_slMemoryUsed;

  // determine uploaded portion size
  slUploadSize = 0;
  const PIX pixMemoryUsed = Max(slStaticSize,slDynamicSize)/BYTES_PER_TEXEL;
  if( pixMemoryUsed==0) return bFlat; // done if no memory is used

  if( sm_ulObject!=NONE) {
    slUploadSize = gfxGetTexturePixRatio(sm_ulObject);
    if( !bFlat || slDynamicSize!=0) slUploadSize *= pixMemoryUsed;
  }
  
  // determine slack space
  const FLOAT fPolySize = sm_pixPolygonSizeU*sm_pixPolygonSizeV;
  fSlackRatio = 1.0f - ClampUp( fPolySize*4/3/pixMemoryUsed, 1.0f);
  return bFlat;
}



