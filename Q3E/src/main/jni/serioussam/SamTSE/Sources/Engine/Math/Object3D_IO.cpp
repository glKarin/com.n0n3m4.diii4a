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

// If you happen to have the Exploration 3D library (in Engine/exploration3d/), you can enable its features here.
#define USE_E3D 0

#include "Engine/StdH.h"

#include <Engine/Math/Object3D.h>

#include <Engine/Base/Registry.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Graphics/Color.h>


#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

#if USE_E3D
#include <Engine/exploration3d/e3ext.h>
#include <Engine/exploration3d/explor3d.h>
#endif

#undef W
#undef NONE

void FillConversionArrays_t(const FLOATmatrix3D &mTransform);
void ClearConversionArrays( void);
void RemapVertices(BOOL bAsOpened);


/*
 *  Intermediate structures used for converting from Exploration 3D data format into O3D
 */
struct ConversionTriangle {
  INDEX ct_iVtx[3];     // indices of vertices
  INDEX ct_iTVtx[3];    // indices of texture vertices
  INDEX ct_iMaterial;   // index of material
};

struct ConversionMaterial {
  ULONG cm_ulTag;                           // for recognition of material
  CTString cm_strName;                      // material's name
  COLOR cm_colColor;                        // material's color
  CDynamicContainer<INDEX> ms_Polygons;     // indices of polygons in this material
};
// conversion arrays
CDynamicContainer<ConversionMaterial> acmMaterials;
CStaticArray<ConversionTriangle> actTriangles;
CStaticArray<FLOAT3D> avVertices;
CStaticStackArray<FLOAT3D> avDst;
CStaticArray<FLOAT2D> avTextureVertices;
CStaticArray<INDEX> aiRemap;

/////////////////////////////////////////////////////////////////////////////
// Helper functions

//--------------------------------------------------------------------------------------------
class CObjectSectorLock {
private:
	CObjectSector *oscl_posc;						// ptr to object sector that will do lock/unlock
public:
	CObjectSectorLock( CObjectSector *posc);		// lock all object sector arrays
	~CObjectSectorLock();										// unlock all object sector arrays
};

//--------------------------------------------------------------------------------------------
/*
 * To lock all object 3D dyna arrays one must create an instance of CObject3DLock.
 * Locking job is done inside class constructor
 */
CObjectSectorLock::CObjectSectorLock( CObjectSector *posc) {
	ASSERT( posc != NULL);
  oscl_posc = posc;
  posc->LockAll();
}

//--------------------------------------------------------------------------------------------
/*
 * Unlocking of all object 3D dynamic arrays will occur automatically when exiting
 * current scope (routine). This is done in class destructor
 */
CObjectSectorLock::~CObjectSectorLock() {
  oscl_posc->UnlockAll();
}

//--------------------------------------------------------------------------------------------
// function makes Little-Big indian conversion of 4 bytes and returns valid SLONG
inline SLONG ConvertLong( SBYTE *pfm)
{
  UBYTE i;
  UBYTE ret_long[ 4];

  for( i=0; i<4; i++)
    ret_long[ i] = *((UBYTE *) pfm + 3 - i);
  return( *((SLONG *) ret_long) );
};

//--------------------------------------------------------------------------------------------
// function makes Little-Big indian conversion of 2 bytes and returns valid WORD
inline INDEX ConvertWord( SBYTE *pfm)
{
  char aret_word[ 2];

  aret_word[ 0] = *(pfm+1);
  aret_word[ 1] = *(pfm+0);
  INDEX ret_word = (INDEX) *((SWORD *) aret_word);
	return( ret_word);
};

//--------------------------------------------------------------------------------------------
// function makes Little-Big indian conversion of 4 bytes representing float and returns valid float
inline float ConvertFloat( SBYTE *pfm)
{
  UBYTE i;
  char float_no[ 4];

  for( i=0; i<4; i++)
    float_no[ i] = *( pfm + 3 - i);
  return( *((float *) float_no) );
};

//--------------------------------------------------------------------------------------------
// function recognizes and loads many 3D file formats, throws char* errors
#if USE_E3D
HINSTANCE _h3dExploration = NULL;
TInitExploration3D _Init3d;
e3_API*_api;
e3_SCENE*_pe3Scene;
e3_OBJECT *_pe3Object;
HWND _hwnd;
BOOL _bBatchLoading = FALSE;
#endif

// start/end batch loading of 3d objects
void CObject3D::BatchLoading_t(BOOL bOn)
{
#if USE_E3D
  // check for dummy calls
  if (!_bBatchLoading==!bOn) {
    return;
  }

  // if turning on
  if (bOn) {
    // if exploration library not yet loaded
    if( _h3dExploration == NULL) {
      // prepare registry
      REG_SetString("HKEY_LOCAL_MACHINE\\SOFTWARE\\X Dimension\\SeriousEngine\\Plugins\\LWO\\BreakObject", "0");
      REG_SetString("HKEY_LOCAL_MACHINE\\SOFTWARE\\X Dimension\\SeriousEngine\\Plugins\\LWO\\textures", "1");
      REG_SetString("HKEY_LOCAL_MACHINE\\SOFTWARE\\X Dimension\\SeriousEngine\\Plugins\\LWO\\chkwrap", "1");
      REG_SetString("HKEY_LOCAL_MACHINE\\SOFTWARE\\X Dimension\\SeriousEngine\\Plugins\\LWO\\readUView", "1");
      // load the dll
      _h3dExploration = LoadLibrary(EXPLORATION_LIBRRAY);
      // if library not opened
      if(_h3dExploration == NULL) {
        throw("3D Exploration dll not found !");
      }
      _Init3d=(TInitExploration3D)GetProcAddress(_h3dExploration,"InitExploration3D");
      CTString strPlugins = _fnmApplicationPath+"Bin\\3DExplorationPlugins";
      e3_INIT init;
      memset(&init,0,sizeof(init));
      init.e_size     = sizeof(init);
      init.e_registry = "Software\\X Dimension\\SeriousEngine";
      init.e_plugins  = (char*)(const char*)strPlugins;
      if(_Init3d) {
        _api=_Init3d(&init);
      } else  {
		    throw("Unable to initialize 3D object library");
      }
    }

    // if 3dexp window not open yet
    if (_hwnd==NULL) {
      // obtain window needed for 3D exploration library to work
      _hwnd=CreateWindow(EXPLORATION_WINDOW,"Object Loader",0,100,100,100,50,NULL,0,(HINSTANCE) GetModuleHandle( NULL),0);
      //ShowWindow(_hwnd, SW_HIDE);
    }

  // if turning off
  } else {
    // if 3dexp window is open
    if (_hwnd!=NULL) {
      // close it
      DestroyWindow(_hwnd);
      _hwnd = NULL;
    }
  }
  _bBatchLoading = bOn;
#else
  throw("3D Exploration is disabled in this build.");
#endif
}

void CObject3D::LoadAny3DFormat_t(
  const CTFileName &fnmFileName,
  const FLOATmatrix3D &mTransform,
  enum LoadType ltLoadType/*= LT_NORMAL*/)
{
#if USE_E3D
  BOOL bWasOn = _bBatchLoading;
  try {
    if (!_bBatchLoading) {
      BatchLoading_t(TRUE);
    }
    // call file load with file's full path name
    CTString strFile = _fnmApplicationPath+fnmFileName;
    char acFile[MAX_PATH];
    wsprintf(acFile,"%s",strFile);
    e3_LoadFile(_hwnd, acFile);
    _pe3Scene=e3_GetScene(_hwnd);    
    // if scene is successefuly loaded
    if(_pe3Scene != NULL)
    {
      _pe3Object = _pe3Scene->GetObject3d( 0);
      // use different methods to convert into Object3D
      switch( ltLoadType)
      {
      case LT_NORMAL:
        FillConversionArrays_t(mTransform);
        ConvertArraysToO3D();
        break;
      case LT_OPENED:
        FillConversionArrays_t(mTransform);
        RemapVertices(TRUE);
        ConvertArraysToO3D();
        break;
      case LT_UNWRAPPED:
        FLOATmatrix3D mOne;
        mOne.Diagonal(1.0f);
        FillConversionArrays_t(mOne);
        if( avTextureVertices.Count() == 0)
        {
    		  ThrowF_t("Unable to import mapping from 3D object because it doesn't contain mapping coordinates.");
        }

        RemapVertices(FALSE);
        ConvertArraysToO3D();
        break;
      }
      ClearConversionArrays();
    }
    else 
    {
		  ThrowF_t("Unable to load 3D object: %s", (const char *)fnmFileName);
    }
  
    if (!bWasOn) {
      BatchLoading_t(FALSE);
    }
  } catch (const char *) {
    if (!bWasOn) {
      BatchLoading_t(FALSE);
    }
    throw;
  }
#endif
}


/*
 * Converts data from Exploration3D format into arrays used for conversion to O3D
 */
void FillConversionArrays_t(const FLOATmatrix3D &mTransform)
{
#if USE_E3D
  // all polygons must be triangles
  if(_pe3Object->_facecount != 0)
  {
    throw("Error: Not all polygons are triangles!");
  }

  // check if we need flipping (if matrix is flipping, polygons need to be flipped)
  const FLOATmatrix3D &m = mTransform;
  FLOAT fDet = 
    m(1,1)*(m(2,2)*m(3,3)-m(2,3)*m(3,2))+
    m(1,2)*(m(2,3)*m(3,1)-m(2,1)*m(3,3))+
    m(1,3)*(m(2,1)*m(3,2)-m(2,2)*m(3,1));
  FLOAT bFlipped = fDet<0;

  // ------------  Convert object vertices (coordinates)
  INDEX ctVertices = _pe3Object->pointcount;
  avVertices.New(ctVertices);
  // copy vertices
  for( INDEX iVtx=0; iVtx<ctVertices; iVtx++)
  {
    avVertices[iVtx] = ((FLOAT3D &)_pe3Object->points[iVtx])*mTransform;
    avVertices[iVtx](1) = -avVertices[iVtx](1);
    avVertices[iVtx](3) = -avVertices[iVtx](3);
  }

  // ------------ Convert object's mapping vertices (texture vertices)
  INDEX ctTextureVertices = _pe3Object->txtcount;
  avTextureVertices.New(ctTextureVertices);
  // copy texture vertices
  for( INDEX iTVtx=0; iTVtx<ctTextureVertices; iTVtx++)
  {
    avTextureVertices[iTVtx] = (FLOAT2D &)_pe3Object->txtpoints[iTVtx];
  }
  
  // ------------ Organize triangles as list of surfaces
  // allocate triangles
  INDEX ctTriangles = _pe3Object->facecount;
  actTriangles.New(ctTriangles);

  acmMaterials.Lock();
  
  // sort triangles per surfaces
  for( INDEX iTriangle=0; iTriangle<ctTriangles; iTriangle++)
  {
    ConversionTriangle &ctTriangle = actTriangles[iTriangle];
    e3_TFACE *pe3Triangle = _pe3Object->GetFace( iTriangle);
    // copy vertex indices
    if (bFlipped) {
      ctTriangle.ct_iVtx[0] = pe3Triangle->v[2];
      ctTriangle.ct_iVtx[1] = pe3Triangle->v[1];
      ctTriangle.ct_iVtx[2] = pe3Triangle->v[0];
    } else {
      ctTriangle.ct_iVtx[0] = pe3Triangle->v[0];
      ctTriangle.ct_iVtx[1] = pe3Triangle->v[1];
      ctTriangle.ct_iVtx[2] = pe3Triangle->v[2];
    }
    // copy texture vertex indices
    if (bFlipped) {
      ctTriangle.ct_iTVtx[0] = pe3Triangle->t[2];
      ctTriangle.ct_iTVtx[1] = pe3Triangle->t[1];
      ctTriangle.ct_iTVtx[2] = pe3Triangle->t[0];
    } else {
      ctTriangle.ct_iTVtx[0] = pe3Triangle->t[0];
      ctTriangle.ct_iTVtx[1] = pe3Triangle->t[1];
      ctTriangle.ct_iTVtx[2] = pe3Triangle->t[2];
    }

    // obtain material
    e3_MATERIAL *pe3Mat = pe3Triangle->material;
    BOOL bNewMaterial = TRUE;
    // attach triangle into one material
    for( INDEX iMat=0; iMat<acmMaterials.Count(); iMat++)
    {
      // if this material already exist in array of materu
      if( acmMaterials[ iMat].cm_ulTag == (ULONG) pe3Mat)
      {
        // set index of surface
        ctTriangle.ct_iMaterial = iMat;
        // add triangle into surface list of triangles
        INDEX *piNewTriangle = new INDEX(1);
        *piNewTriangle = iTriangle;
        acmMaterials[ iMat].ms_Polygons.Add( piNewTriangle);
        bNewMaterial = FALSE;
        continue;
      }
    }
    // if material hasn't been added yet
    if( bNewMaterial)
    {
      // add new material
      ConversionMaterial *pcmNew = new ConversionMaterial;
      acmMaterials.Unlock();
      acmMaterials.Add( pcmNew);
      acmMaterials.Lock();
      // set polygon's material index 
      INDEX iNewMaterial = acmMaterials.Count()-1;
      ctTriangle.ct_iMaterial = iNewMaterial;
      // add triangle into new surface's list of triangles
      INDEX *piNewTriangle = new INDEX(1);
      *piNewTriangle = iTriangle;
      acmMaterials[ iNewMaterial].ms_Polygons.Add( piNewTriangle);
      
      // remember recognition tag (ptr)
      pcmNew->cm_ulTag = (ULONG) pe3Mat;

      // ---------- Set material's name
      // if not default material
      if( pe3Mat != NULL && pe3Mat->name != NULL)
      {
        acmMaterials[iNewMaterial].cm_strName = CTString(pe3Mat->name);
        // get color
        COLOR colColor = CLR_CLRF( pe3Mat->GetDiffuse().rgb());
        acmMaterials[iNewMaterial].cm_colColor = colColor;
      }
      else
      {
        acmMaterials[iNewMaterial].cm_strName = "Default";
        acmMaterials[iNewMaterial].cm_colColor = C_GRAY;
      }
    }
  }
  acmMaterials.Unlock();
#endif
}

void ClearConversionArrays( void)
{
  acmMaterials.Clear();
  actTriangles.Clear();
  avVertices.Clear();
  avTextureVertices.Clear();
  aiRemap.Clear();
}

void RemapVertices(BOOL bAsOpened)
{
  {INDEX ctSurf = 0;
  // fill remap array with indices of vertices in order how they appear per polygons
  {FOREACHINDYNAMICCONTAINER(acmMaterials, ConversionMaterial, itcm)
  {
    _RPT1(_CRT_WARN, "Indices of polygons in surface %d:", ctSurf);
    // for each polygon in surface
    {FOREACHINDYNAMICCONTAINER(itcm->ms_Polygons, INDEX, itipol)
    {
      _RPT1(_CRT_WARN, " %d,", *itipol);
    }}
    _RPT0(_CRT_WARN, "\n");
    ctSurf++;
  }}
  
  _RPT0(_CRT_WARN, "Polygons and their vertex indices:\n");
  for( INDEX ipol=0; ipol<actTriangles.Count(); ipol++)
  {
    INDEX idxVtx0 = actTriangles[ipol].ct_iVtx[0];
    INDEX idxVtx1 = actTriangles[ipol].ct_iVtx[1];
    INDEX idxVtx2 = actTriangles[ipol].ct_iVtx[2];
    _RPT4(_CRT_WARN, "Indices of vertices in polygon %d : (%d, %d, %d)\n", ipol, idxVtx0, idxVtx1, idxVtx2);
  }}

  INDEX ctVertices = avVertices.Count();
  aiRemap.New(ctVertices);

  // fill remap array with indices of vertices in order how they appear per polygons
  FOREACHINDYNAMICCONTAINER(acmMaterials, ConversionMaterial, itcm)
  {
    // fill remap array with -1
    for( INDEX iRemap=0; iRemap<ctVertices; iRemap++)
    {
      aiRemap[iRemap] = -1;
    }
    // reset 'vertex in surface' counter
    INDEX ctvx = 0;

    // for each polygon in surface
    {FOREACHINDYNAMICCONTAINER(itcm->ms_Polygons, INDEX, itipol)
    {
      INDEX idxPol = *itipol;
      // for each vertex in polygon
      for(INDEX iVtx=0; iVtx<3; iVtx++)
      {
        // get vertex's index
        INDEX idxVtx = actTriangles[idxPol].ct_iVtx[iVtx];
        if( aiRemap[idxVtx] == -1)
        {
          aiRemap[idxVtx] = ctvx;
          ctvx++;
        }
      }
    }}

    INDEX ctOld = avDst.Count();
    // allocate new block of vertices used in this surface
    FLOAT3D *pavDst = avDst.Push( ctvx);

    // for each polygon in surface
    {FOREACHINDYNAMICCONTAINER(itcm->ms_Polygons, INDEX, itipol)
    {
      INDEX iPol=*itipol;
      // for each vertex in polygon
      for(INDEX iVtx=0; iVtx<3; iVtx++)
      {
        // get vertex's index
        INDEX idxVtx = actTriangles[iPol].ct_iVtx[iVtx];
        // get remapped index
        INDEX iRemap = aiRemap[idxVtx];
        // if cutting object
        if( bAsOpened)
        {
          // copy vertex coordinate
          pavDst[ iRemap] = avVertices[idxVtx];
        }
        // if creating unwrapped mapping
        else
        {
          // copy texture coordinate
          FLOAT3D vMap;
          vMap(1) = avTextureVertices[actTriangles[iPol].ct_iTVtx[iVtx]](1);
          vMap(2) = -avTextureVertices[actTriangles[iPol].ct_iTVtx[iVtx]](2);
          vMap(3) = 0;
          pavDst[ iRemap] = vMap;
        }
        // remap index of polygon vertex
        actTriangles[iPol].ct_iVtx[iVtx] = iRemap+ctOld;
      }
    }}
  }
  aiRemap.Clear();
  
  // replace remapped array of vertices over original one
  avVertices.Clear();
  avVertices.New(avDst.Count());
  for( INDEX iVtxNew=0; iVtxNew<avDst.Count(); iVtxNew++)
  {
    avVertices[iVtxNew] = avDst[iVtxNew];
  }
  avDst.PopAll();

  {INDEX ctSurf = 0;
  // fill remap array with indices of vertices in order how they appear per polygons
  {FOREACHINDYNAMICCONTAINER(acmMaterials, ConversionMaterial, itcm)
  {
    _RPT1(_CRT_WARN, "Indices of polygons in surface %d:", ctSurf);
    // for each polygon in surface
    {FOREACHINDYNAMICCONTAINER(itcm->ms_Polygons, INDEX, itipol)
    {
      _RPT1(_CRT_WARN, " %d,", *itipol);
    }}
    _RPT0(_CRT_WARN, "\n");
    ctSurf++;
  }}
  
  _RPT0(_CRT_WARN, "Polygons and their vertex indices:\n");
  for( INDEX ipol=0; ipol<actTriangles.Count(); ipol++)
  {
    INDEX idxVtx0 = actTriangles[ipol].ct_iVtx[0];
    INDEX idxVtx1 = actTriangles[ipol].ct_iVtx[1];
    INDEX idxVtx2 = actTriangles[ipol].ct_iVtx[2];
    _RPT4(_CRT_WARN, "Indices of vertices in polygon %d : (%d, %d, %d)\n", ipol, idxVtx0, idxVtx1, idxVtx2);
  }}
}

/*
 * Convert streihgtfowrard from intermediate structures into O3D
 */
void CObject3D::ConvertArraysToO3D( void)
{
  acmMaterials.Lock();
  // create one sector
  CObjectSector &osc = *ob_aoscSectors.New(1);
  // this will lock at the instancing and unlock while destructing all sector arrays
	CObjectSectorLock OSectorLock(&osc);

  // ------------ Vertices
  INDEX ctVertices = avVertices.Count();
  CObjectVertex *pVtx = osc.osc_aovxVertices.New(ctVertices);
  for(INDEX iVtx=0; iVtx<ctVertices; iVtx++)
  {
    pVtx[ iVtx] = FLOATtoDOUBLE( avVertices[iVtx]);
  }
	
  // ------------ Materials
  INDEX ctMaterials = acmMaterials.Count();
  osc.osc_aomtMaterials.New( ctMaterials);
  for( INDEX iMat=0; iMat<ctMaterials; iMat++)
  {
    osc.osc_aomtMaterials[iMat] = CObjectMaterial( acmMaterials[iMat].cm_strName);
    osc.osc_aomtMaterials[iMat].omt_Color = acmMaterials[iMat].cm_colColor;
  }

  // ------------ Edges and polygons
  INDEX ctTriangles = actTriangles.Count();
	CObjectPolygon *popo = osc.osc_aopoPolygons.New(ctTriangles);
	CObjectPlane *popl = osc.osc_aoplPlanes.New(ctTriangles);
  // we need 3 edges for each polygon
  CObjectEdge *poedg = osc.osc_aoedEdges.New(ctTriangles*3);
  for(INDEX iTri=0; iTri<ctTriangles; iTri++)
  {
    // obtain triangle's vertices
    CObjectVertex *pVtx0 = &osc.osc_aovxVertices[ actTriangles[iTri].ct_iVtx[0]];
    CObjectVertex *pVtx1 = &osc.osc_aovxVertices[ actTriangles[iTri].ct_iVtx[1]];
    CObjectVertex *pVtx2 = &osc.osc_aovxVertices[ actTriangles[iTri].ct_iVtx[2]];

    // create edges
    poedg[iTri*3+0] = CObjectEdge( *pVtx0, *pVtx1);
    poedg[iTri*3+1] = CObjectEdge( *pVtx1, *pVtx2);
    poedg[iTri*3+2] = CObjectEdge( *pVtx2, *pVtx0);

    // create polygon edges
    popo[iTri].opo_PolygonEdges.New(3);
    popo[iTri].opo_PolygonEdges.Lock();
    popo[iTri].opo_PolygonEdges[0].ope_Edge = &poedg[iTri*3+0];
    popo[iTri].opo_PolygonEdges[1].ope_Edge = &poedg[iTri*3+1];
    popo[iTri].opo_PolygonEdges[2].ope_Edge = &poedg[iTri*3+2];
    popo[iTri].opo_PolygonEdges.Unlock();

    // set material
    popo[iTri].opo_Material = &osc.osc_aomtMaterials[ actTriangles[iTri].ct_iMaterial];
    popo[iTri].opo_colorColor = popo[iTri].opo_Material->omt_Color;
    
    // create and set plane
    popl[iTri] = DOUBLEplane3D( *pVtx0, *pVtx1, *pVtx2);
    popo[iTri].opo_Plane = &popl[iTri];
  }
  acmMaterials.Unlock();
}
