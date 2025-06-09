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
#include <Engine/Base/Stream.h>
#include <Engine/Math/Projection.h>
#include <Engine/Ska/Render.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Fog_internal.h>

static INDEX _ctVertices =-1;
static INDEX _ctIndices  =-1;
static INDEX _ctTextures =-1;
static INDEX _ctUVMaps   =-1;
static INDEX _ctColors   =-1;
static INDEX _ctFloats   =-1;
static INDEX _ctLights   =-1;

static CAnyProjection3D *_paprProjection;  // current projection
static Matrix12 *_pmObjToView = NULL;
static Matrix12 *_pmObjToAbs  = NULL;

static CShader     *_pShader     = NULL;   // current shader
static GFXTexCoord *_pCurrentUVMap = NULL; // current UVMap

static GFXVertex4  *_paVertices  = NULL;   // array of vertices
static GFXNormal   *_paNormals   = NULL;   // array of normals
static GFXTexCoord **_paUVMaps    = NULL;   // array of uvmaps to chose from

static GFXTexCoord *_paFogUVMap  = NULL;   // UVMap for fog pass
static GFXTexCoord *_paHazeUVMap = NULL;   // UVMap for haze pass
static GFXColor    *_pacolVtxHaze = NULL;  // array of vertex colors for haze

static CTextureObject **_paTextures = NULL;// array of textures to chose from
static INDEX_T       *_paIndices   = NULL;   // current array of triangle indices

static GFXColor    _colAmbient = 0x000000FF;     // Ambient color
static COLOR       _colModel   = 0x000000FF;     // Model color
static GFXColor    _colLight   = 0x000000FF;     // Light color
static FLOAT3D     _vLightDir  = FLOAT3D(0,0,0); // Light direction

static COLOR       _colConstant  = 0;     // current set color
static COLOR       *_paColors    = NULL;  // array of colors to chose from
static FLOAT       *_paFloats    = NULL;  // array of floats to chose from
static ULONG       _ulFlags      = 0;

// Vertex colors
static CStaticStackArray<GFXColor> _acolVtxColors;        // array of color values for each vertex
static CStaticStackArray<GFXColor> _acolVtxModifyColors;  // array of color modified values for each vertex
GFXColor *_pcolVtxColors = NULL;    // pointer to vertex color array (points to current array of vertex colors)

// vertex array that is returned if shader request vertices for modify
static CStaticStackArray<GFXVertex4>  _vModifyVertices;
static CStaticStackArray<GFXTexCoord> _uvUVMapForModify;


// Begin shader using
void shaBegin(CAnyProjection3D &aprProjection,CShader *pShader)
{
  // Chech that last shading ended with shaEnd
  ASSERT(_pShader==NULL);
  // Chech if shader exists
  ASSERT(pShader!=NULL);
  // Set current projection
  _paprProjection = &aprProjection;
  // Set pointer to shader
  _pShader = pShader;
}

// End shader using
void shaEnd(void)
{
  // Chech if shader exists
  ASSERT(_pShader!=NULL);
  // Call shader function
  _pShader->ShaderFunc();
  // Clean used values
  shaClean();

  _pShader = NULL;
}

// Render given model
void shaRender(void)
{
  ASSERT(_ctVertices>0);
  ASSERT(_ctIndices>0);
  ASSERT(_paVertices!=NULL);
  ASSERT(_paIndices!=NULL);

  // Set vertices
  gfxSetVertexArray(_paVertices,_ctVertices);
  gfxLockArrays();

  // if there is valid UVMap
  if(_pCurrentUVMap!=NULL) {
    gfxSetTexCoordArray(_pCurrentUVMap, FALSE);
  }

  // if there is valid vertex color array
  if(_pcolVtxColors!=NULL) {
    gfxSetColorArray(_pcolVtxColors);
  }

  // draw model with set params
  gfxDrawElements( _ctIndices, _paIndices);
  gfxUnlockArrays();
}

// Render aditional pass for fog and haze
void shaDoFogPass(void)
{
  // if full bright 
  if(shaGetFlags()&BASE_FULL_BRIGHT) {
    // no fog pass 
    return;
  }

  ASSERT(_paFogUVMap==NULL);
  ASSERT(_paHazeUVMap==NULL);

  // Calculate fog and haze uvmap for this opaque surface
  RM_DoFogAndHaze(TRUE);
  // if fog uvmap has been given
  if(_paFogUVMap!=NULL) {
    // setup texture/color arrays and rendering mode
    gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
    gfxSetTexture( _fog_ulTexture, _fog_tpLocal);
    gfxSetTexCoordArray(_paFogUVMap, FALSE);
    gfxSetConstantColor(_fog_fp.fp_colColor);
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxEnableBlend();
    // render fog pass
    gfxDrawElements( _ctIndices, _paIndices);
  }
  // if haze uvmap has been given
  if(_paHazeUVMap!=NULL) {
    gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
    gfxSetTexture( _haze_ulTexture, _haze_tpLocal);
    gfxSetTexCoordArray(_paHazeUVMap, TRUE);
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxEnableBlend();
    // set vertex color array for haze
    if(_pacolVtxHaze !=NULL ) {
      gfxSetColorArray( _pacolVtxHaze);
    }

    // render fog pass
    gfxDrawElements( _ctIndices, _paIndices);
  }
}

// Modify color for fog
void shaModifyColorForFog(void)
{
  // if full bright
  if(shaGetFlags()&BASE_FULL_BRIGHT) {
    // no fog colors
    return;
  }

  // Update this surface color array if fog or haze exists
  RM_DoFogAndHaze(FALSE);
}

// Calculate lightning for given model
void shaCalculateLight(void)
{
  // if full bright
  if(shaGetFlags()&BASE_FULL_BRIGHT) {
    GFXColor colLight = _colConstant;
    GFXColor colAmbient;
    GFXColor colConstant;
    // is over brightning enabled
    if(shaOverBrightningEnabled()) {
      colAmbient = 0x7F7F7FFF;
    } else {
      colAmbient = 0xFFFFFFFF;
    }
    colConstant.MultiplyRGBA(colLight,colAmbient);
	shaSetConstantColor(ByteSwap(colConstant.ul.abgr));
    // no vertex colors
    return;
  }

  ASSERT(_paNormals!=NULL);
  _acolVtxColors.PopAll();
  _acolVtxColors.Push(_ctVertices);

  GFXColor colModel   = _colModel;   // Model color
  const GFXColor &colAmbient = _colAmbient; // Ambient color
  const GFXColor &colLight   = _colLight;   // Light color
  const GFXColor &colSurface = _colConstant; // shader color

  colModel.MultiplyRGBA(colModel,colSurface);

  UBYTE ubColShift = 8;
  SLONG slar = colAmbient.ub.r;
  SLONG slag = colAmbient.ub.g;
  SLONG slab = colAmbient.ub.b;

  if(shaOverBrightningEnabled()) {
    slar = ClampUp(slar, (SLONG)127);
    slag = ClampUp(slag, (SLONG)127);
    slab = ClampUp(slab, (SLONG)127);
    ubColShift = 8;
  } else {
    slar*=2;
    slag*=2;
    slab*=2;
    ubColShift = 7;
  }

  // for each vertex color
  for(INDEX ivx=0;ivx<_ctVertices;ivx++) {
    // calculate vertex light
    const FLOAT3D &vNorm = FLOAT3D(_paNormals[ivx].nx,_paNormals[ivx].ny,_paNormals[ivx].nz);
    FLOAT fDot = vNorm % _vLightDir;
    fDot = Clamp(fDot,0.0f,1.0f);
    SLONG slDot = NormFloatToByte(fDot);
	_acolVtxColors[ivx].ub.r = ClampUp(colModel.ub.r * (slar + ((colLight.ub.r * slDot) >> ubColShift)) >> (SLONG)8, (SLONG)255);
	_acolVtxColors[ivx].ub.g = ClampUp(colModel.ub.g * (slag + ((colLight.ub.g * slDot) >> ubColShift)) >> (SLONG)8, (SLONG)255);
	_acolVtxColors[ivx].ub.b = ClampUp(colModel.ub.b * (slab + ((colLight.ub.b * slDot) >> ubColShift)) >> (SLONG)8, (SLONG)255);
	_acolVtxColors[ivx].ub.a = colModel.ub.a;//slDot;
  }
  // Set current vertex color array 
  _pcolVtxColors = &_acolVtxColors[0];
}

// Calculate lightning for given model
void shaCalculateLightForSpecular(void)
{
  ASSERT(_paNormals!=NULL);
  _acolVtxColors.PopAll();
  _acolVtxColors.Push(_ctVertices);

  GFXColor colModel   = (GFXColor)_colModel;   // Model color
  const GFXColor &colAmbient = (GFXColor)_colAmbient; // Ambient color
  const GFXColor &colLight   = (GFXColor)_colLight;   // Light color
  const GFXColor &colSurface = (GFXColor)_colConstant; // shader color

  // colModel = MulColors(colModel.r,colSurface.abgr);
  colModel.MultiplyRGBA(colModel,colSurface);

  UBYTE ubColShift = 8;
  SLONG slar = colAmbient.ub.r;
  SLONG slag = colAmbient.ub.g;
  SLONG slab = colAmbient.ub.b;

  if(shaOverBrightningEnabled()) {
    slar = ClampUp(slar, (SLONG)127);
    slag = ClampUp(slag, (SLONG)127);
    slab = ClampUp(slab, (SLONG)127);
    ubColShift = 8;
  } else {
    slar*=2;
    slag*=2;
    slab*=2;
    ubColShift = 7;
  }

  // for each vertex color
  for(INDEX ivx=0;ivx<_ctVertices;ivx++) {
    // calculate vertex light
    const FLOAT3D &vNorm = FLOAT3D(_paNormals[ivx].nx,_paNormals[ivx].ny,_paNormals[ivx].nz);
    FLOAT fDot = vNorm % _vLightDir;
    fDot = Clamp(fDot,0.0f,1.0f);
    SLONG slDot = NormFloatToByte(fDot);

	_acolVtxColors[ivx].ub.r = ClampUp(colModel.ub.r * (slar + ((colLight.ub.r * slDot) >> ubColShift)) >> (SLONG)8, (SLONG)255);
	_acolVtxColors[ivx].ub.g = ClampUp(colModel.ub.g * (slag + ((colLight.ub.g * slDot) >> ubColShift)) >> (SLONG)8, (SLONG)255);
	_acolVtxColors[ivx].ub.b = ClampUp(colModel.ub.b * (slab + ((colLight.ub.b * slDot) >> ubColShift)) >> (SLONG)8, (SLONG)255);
	_acolVtxColors[ivx].ub.a = slDot;//colModel.ub.a;//slDot;
  }
  // Set current wertex array 
  _pcolVtxColors = &_acolVtxColors[0];
}

// Clean all values
void shaClean(void)
{
  _ctVertices   = -1;
  _ctIndices    = -1;
  _ctColors     = -1;
  _ctTextures   = -1;
  _ctUVMaps     = -1;
  _ctLights     = -1;

  _colConstant   = 0;
  _ulFlags       = 0;

  _pShader        = NULL;
  _paVertices     = NULL;
  _paNormals      = NULL;
  _paIndices      = NULL;
  _paUVMaps       = NULL;
  _paTextures     = NULL;
  _paColors       = NULL;
  _paFloats       = NULL;
  _pCurrentUVMap  = NULL;
  _pcolVtxColors  = NULL;

  _paFogUVMap     = NULL;
  _paHazeUVMap    = NULL;
  _pacolVtxHaze   = NULL;
  _pmObjToView    = NULL;
  _pmObjToAbs     = NULL;
  _paprProjection = NULL;

  _acolVtxColors.PopAll();
  _acolVtxModifyColors.PopAll();
  _vModifyVertices.PopAll();
  _uvUVMapForModify.PopAll();
  shaCullFace(GFX_BACK);
}


/*
 * Shader value setting
 */

// Set array of vertices
void shaSetVertexArray(GFXVertex4 *paVertices,INDEX ctVertices)
{
  ASSERT(paVertices!=NULL);
  ASSERT(ctVertices>0);
  // set pointer to new vertex array
  _paVertices = paVertices;
  _ctVertices = ctVertices;
}

// Set array of normals
void shaSetNormalArray(GFXNormal *paNormals)
{
  ASSERT(paNormals!=NULL);

  _paNormals = paNormals;
}

// Set array of indices
void shaSetIndices(INDEX_T *paIndices,INDEX ctIndices)
{
  ASSERT(paIndices!=NULL);
  ASSERT(ctIndices>0);

  _paIndices = paIndices;
  _ctIndices = ctIndices;
}

// Set array of texture objects for shader
void shaSetTextureArray(CTextureObject **paTextureObject, INDEX ctTextures)
{
  _paTextures = paTextureObject;
  _ctTextures = ctTextures;
}

// Set array of uv maps
void shaSetUVMapsArray(GFXTexCoord **paUVMaps, INDEX ctUVMaps)
{
  ASSERT(paUVMaps!=NULL);
  ASSERT(ctUVMaps>0);

  _paUVMaps = paUVMaps;
  _ctUVMaps = ctUVMaps;
}

// Set array of shader colors
void shaSetColorArray(COLOR *paColors, INDEX ctColors)
{
  ASSERT(paColors!=NULL);
  ASSERT(ctColors>0);

  _paColors = paColors;
  _ctColors = ctColors;
}

// Set array of floats for shader
void shaSetFloatArray(FLOAT *paFloats, INDEX ctFloats)
{
  ASSERT(paFloats!=NULL);
  _paFloats = paFloats;
  _ctFloats = ctFloats;
}

// Set shading flags
void shaSetFlags(ULONG ulFlags)
{
  _ulFlags = ulFlags;
}

// Set base color of model 
void shaSetModelColor(COLOR &colModel)
{
  _colModel = colModel;
}

// Set light direction
void shaSetLightDirection(const FLOAT3D &vLightDir)
{
  _vLightDir = vLightDir;
}

// Set light color
void shaSetLightColor(COLOR colAmbient, COLOR colLight)
{
  _colAmbient = colAmbient;
  _colLight = colLight;
}

// Set object to view matrix
void shaSetObjToViewMatrix(Matrix12 &mat)
{
  _pmObjToView = &mat;
}

// Set object to abs matrix
void shaSetObjToAbsMatrix(Matrix12 &mat)
{
  _pmObjToAbs = &mat;
}



// Set current texture index
void shaSetTexture(INDEX iTextureIndex)
{
  if(_paTextures==NULL || iTextureIndex<0 || iTextureIndex>=_ctTextures ||  _paTextures[iTextureIndex] == NULL) {
    gfxDisableTexture();
    return;
  }
  ASSERT(iTextureIndex<_ctTextures);

  CTextureObject *pto = _paTextures[iTextureIndex];
  ASSERT(pto!=NULL);

  CTextureData *pTextureData = (CTextureData*)pto->GetData();
  const INDEX iFrameNo = pto->GetFrame();
  pTextureData->SetAsCurrent(iFrameNo);
}

// Set current uvmap index
void shaSetUVMap(INDEX iUVMapIndex)
{
  ASSERT(iUVMapIndex>=0);
  if(iUVMapIndex<=_ctUVMaps) {
    _pCurrentUVMap = _paUVMaps[iUVMapIndex];
  }
}

// Set current color index
void shaSetColor(INDEX icolIndex)
{
  ASSERT(icolIndex>=0);

  if(icolIndex>=_ctColors) {
    _colConstant = C_WHITE|CT_OPAQUE;
  } else {
    _colConstant = _paColors[icolIndex];
  }
  // Set this color as constant color
  gfxSetConstantColor(_colConstant);
}

// Set array of texcoords index
void shaSetTexCoords(GFXTexCoord *uvNewMap)
{
  _pCurrentUVMap = uvNewMap;
}

// Set array of vertex colors
void shaSetVertexColors(GFXColor *paColors)
{
  _pcolVtxColors = paColors;
}

// Set constant color
void shaSetConstantColor(const COLOR colConstant)
{
  gfxSetConstantColor(colConstant);
}


/*
 * Shader value getting
 */ 

// Get vertex count
INDEX shaGetVertexCount(void)
{
  return _ctVertices;
}

// Get index count
INDEX shaGetIndexCount(void)
{
  return _ctIndices;
}

// Get float from array of floats
FLOAT shaGetFloat(INDEX iFloatIndex)
{
  ASSERT(iFloatIndex>=0);
  ASSERT(iFloatIndex<_ctFloats);
  return _paFloats[iFloatIndex];
}

// Get texture from array of textures
CTextureObject *shaGetTexture( INDEX iTextureIndex)
{
  ASSERT( iTextureIndex>=0);
  if( _paTextures==NULL || iTextureIndex>=_ctTextures || _paTextures[iTextureIndex]==NULL) return NULL;
  else return _paTextures[iTextureIndex];
}

// Get color from color array
COLOR &shaGetColor(INDEX iColorIndex)
{
  ASSERT(iColorIndex<_ctColors);
  return _paColors[iColorIndex];
}

// Get shading flags
ULONG &shaGetFlags()
{
  return _ulFlags;
}

// Get base color of model
COLOR &shaGetModelColor(void)
{
  return _colModel;
}

// Get light direction
FLOAT3D &shaGetLightDirection(void)
{
  return _vLightDir;
}

// Get current light color
COLOR &shaGetLightColor(void)
{
  return _colLight.ul.abgr;
}

// Get current ambient volor
COLOR &shaGetAmbientColor(void)
{
  return _colAmbient.ul.abgr;
}

// Get current set color
COLOR &shaGetCurrentColor(void)
{
  return _colConstant;
}

// Get vertex array
GFXVertex4 *shaGetVertexArray(void)
{
  return _paVertices;
}

// Get index array
INDEX_T *shaGetIndexArray(void)
{
  return _paIndices;
}

// Get normal array
GFXNormal *shaGetNormalArray(void)
{
  return _paNormals;
}

// Get uvmap array from array of uvmaps
GFXTexCoord *shaGetUVMap(INDEX iUVMapIndex)
{
  ASSERT( iUVMapIndex>=0);
  if( iUVMapIndex>=_ctUVMaps) return NULL;
  else return _paUVMaps[iUVMapIndex];
}

// Get color array
GFXColor *shaGetColorArray(void)
{
  return &_acolVtxColors[0];
}


// Get empty color array for modifying
GFXColor *shaGetNewColorArray(void)
{
  ASSERT(_ctVertices!=0);
  _acolVtxModifyColors.PopAll();
  _acolVtxModifyColors.Push(_ctVertices);
  return &_acolVtxModifyColors[0];
}

// Get empty texcoords array for modifying
GFXTexCoord *shaGetNewTexCoordArray(void)
{
  ASSERT(_ctVertices!=0);
  _uvUVMapForModify.PopAll();
  _uvUVMapForModify.Push(_ctVertices);
  return &_uvUVMapForModify[0];
}

// Get empty vertex array for modifying
GFXVertex *shaGetNewVertexArray(void)
{
  ASSERT(_ctVertices!=0);
  _vModifyVertices.PopAll();
  _vModifyVertices.Push(_ctVertices);
  return &_vModifyVertices[0];
}

// Get current projection
CAnyProjection3D *shaGetProjection()
{
  return _paprProjection;
}

// Get object to view matrix
Matrix12 *shaGetObjToViewMatrix(void)
{
  ASSERT(_pmObjToView!=NULL);
  return _pmObjToView;
}

// Get object to abs matrix
Matrix12 *shaGetObjToAbsMatrix(void)
{
  ASSERT(_pmObjToAbs!=NULL);
  return _pmObjToAbs;
}



/*
 * Shader states
 */

// Set face culling
void shaCullFace(GfxFace eFace)
{
  if(_paprProjection !=NULL && (*_paprProjection)->pr_bMirror) {
    gfxFrontFace( GFX_CW);
  } else {
    gfxFrontFace( GFX_CCW);
  }
  gfxCullFace(eFace);
}

// Set blending operations
void shaBlendFunc(GfxBlend eSrc, GfxBlend eDst)
{
  gfxBlendFunc(eSrc,eDst);
}

// Set texture modulation mode
void shaSetTextureModulation(INDEX iScale) 
{
  gfxSetTextureModulation(iScale);
}

// Enable/Disable blening
void shaEnableBlend(void)
{
  gfxEnableBlend();
}
void shaDisableBlend(void)
{
  gfxDisableBlend();
}

// Enable/Disable alpha test
void shaEnableAlphaTest(void)
{
  gfxEnableAlphaTest();
}
void shaDisableAlphaTest(void)
{
  gfxDisableAlphaTest();
}

// Enable/Disable depth test
void shaEnableDepthTest(void)
{
  gfxEnableDepthTest();
}
void shaDisableDepthTest(void)
{
  gfxDisableDepthTest();
}

// Enable/Disable depth write
void shaEnableDepthWrite(void)
{
  gfxEnableDepthWrite();
}
void shaDisableDepthWrite(void)
{
  gfxDisableDepthWrite();
}

// Set depth buffer compare mode
void shaDepthFunc(GfxComp eComp)
{
  gfxDepthFunc(eComp);
}

// Set texture wrapping 
void shaSetTextureWrapping( enum GfxWrap eWrapU, enum GfxWrap eWrapV)
{
  gfxSetTextureWrapping(eWrapU,eWrapV);
}


// Set uvmap for fog
void shaSetFogUVMap(GFXTexCoord *paFogUVMap)
{
  _paFogUVMap = paFogUVMap;
}

// Set uvmap for haze
void shaSetHazeUVMap(GFXTexCoord *paHazeUVMap)
{
  _paHazeUVMap = paHazeUVMap;
}

// Set array of vertex colors used in haze
void shaSetHazeColorArray(GFXColor *paHazeColors)
{
  _pacolVtxHaze = paHazeColors;
}

BOOL shaOverBrightningEnabled(void)
{
  // determine multitexturing capability for overbrighting purposes
  extern INDEX mdl_bAllowOverbright;
  return mdl_bAllowOverbright && _pGfx->gl_ctTextureUnits>1;
}


/*
 * Shader handling
 */

// Constructor
CShader::CShader()
{
  hLibrary = NULL;
  ShaderFunc = NULL;
  GetShaderDesc = NULL;
}

// Destructor
CShader::~CShader()
{
  // Release shader dll
  Clear();
}

// Clear shader 
void CShader::Clear(void)
{
  ShaderFunc = NULL;
  GetShaderDesc = NULL;
  // release dll
  if(hLibrary!=NULL) delete hLibrary;
}

// Count used memory
SLONG CShader::GetUsedMemory(void)
{
  return sizeof(CShader);
}

// Write to stream
void CShader::Write_t(CTStream *ostrFile)
{
}

// Read from stream
void CShader::Read_t(CTStream *istrFile)
{
  // read the dll filename and class name from the stream
  CTFileName fnmDLL;
  CTString strShaderFunc;
  CTString strShaderInfo;

  fnmDLL.ReadFromText_t(*istrFile, "Package: ");
  strShaderFunc.ReadFromText_t(*istrFile, "Name: ");
  strShaderInfo.ReadFromText_t(*istrFile, "Info: ");

  // create name of dll
  #ifdef STATICALLY_LINKED
    #define fnmExpanded NULL
  #else
    #ifndef NDEBUG
      fnmDLL = fnmDLL.FileDir()+"Debug\\"+fnmDLL.FileName()+/*_strModExt+*/"D"+fnmDLL.FileExt();
    #else
      fnmDLL = fnmDLL.FileDir()+fnmDLL.FileName()+/*_strModExt+*/fnmDLL.FileExt();
    #endif
    fnmDLL = CDynamicLoader::ConvertLibNameToPlatform(fnmDLL);
    CTFileName fnmExpanded;
    ExpandFilePath(EFP_READ | EFP_NOZIPS,fnmDLL,fnmExpanded);
  #endif

  // !!! FIXME : rcg12142001 Should I move this into CWin32DynamicLoader?
  #ifdef PLATFORM_WIN32
  // set new error mode
  const UINT iOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
  #endif

  // load dll
  hLibrary = CDynamicLoader::GetInstance(fnmExpanded);

  // !!! FIXME : rcg12142001 Should I move this into CWin32DynamicLoader?
  #ifdef PLATFORM_WIN32
  // return last error mode
  SetErrorMode(iOldErrorMode);
  #endif

  // check if library has loaded
  const char *errmsg = hLibrary->GetError();
  if(errmsg != NULL)
  {
    // report error
    istrFile->Throw_t("Error loading '%s' library: %s",(const char*)fnmExpanded, errmsg);
    return;
  }

  // get pointer to shader render function
  ShaderFunc = (void(*)(void))hLibrary->FindSymbol(strShaderFunc);
  // if error occured
  if(ShaderFunc==NULL)
  {
    // report error
    istrFile->Throw_t("CDynamicLoader::GetSymbol() 'ShaderFunc' Error: %s", hLibrary->GetError());
  }
  // get pointer to shader info function
  GetShaderDesc = (void(*)(ShaderDesc&))hLibrary->FindSymbol(strShaderInfo);
  // if error occured
  if(GetShaderDesc==NULL)
  {
    // report error
    istrFile->Throw_t("CDynamicLoader::GetSymbol() 'ShaderDesc' Error: %s", hLibrary->GetError());
  }
}

