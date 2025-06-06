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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <lwsurf.h>
#include <lwhost.h>
#include <lwserver.h>
#include <lwgeneric.h>
#include "vecmat.h"
#include "base.h"


static LWItemID _objid = NULL;
static LWSurfaceID *_asurSurfaces = NULL;
static char *_strFileName = NULL;

// arrays of all point and polygon ids
int _ctPntIDs = 0;
int _iPnt = 0;
LWPntID *_aidPntIDs = NULL;

int _ctPolIDs = 0;
int _iPol = 0;
LWPolID *_aidPolIDs = NULL;

// point coordinates
float (*_avPnts)[3] = NULL;
// polygon normals
float (*_avPolNormals)[3] = NULL;
// point normals
float (*_avPntNormals)[3] = NULL;

// uvmapnames
const char **_astrUVMapNames = NULL;
int _ctUVMapNames = 0;
int _ctUsedUVMapNames = 0;
// weightmapnames
const char **_astrWeightMapNames = NULL;
int *_actWeightMapCounts = NULL;
int _ctWeightMapNames = 0;
int _ctUsedWeightMapNames = 0;

// relmorphmapnames
const char **_astrRelMorphMapNames = NULL;
int *_actRelMorphMapCounts = NULL;
int _ctRelMorphMapNames = 0;
int _ctUsedRelMorphMapNames = 0;

// absmorphmapnames
const char **_astrAbsMorphMapNames = NULL;
int *_actAbsMorphMapCounts = NULL;
int _ctAbsMorphMapNames = 0;
int _ctUsedAbsMorphMapNames = 0;

// here we store ids for all point and polygon combinations in order by polygons and by points, effectively
// having all info needed to handle the unwelded object and info needed to calculate normals
int _ctPolPnts = 0;
PolPnt *_appPolPnts = NULL;
PolPnt *_appPntPols = NULL;

int _ctSurfs = 0;


extern int ReloadGlobalObjects();

// helper functions for ID sorting/searching
int __cdecl qsort_CompareIDs(const void *elem1, const void *elem2)
{
  return *(int*)elem1-*(int*)elem2;
}
int __cdecl qsort_ComparePolPntsByPnt(const void *elem1, const void *elem2)
{
  PolPnt *pp1 = (struct PolPnt *)elem1;
  PolPnt *pp2 = (struct PolPnt *)elem2;

  return (int)pp1->pp_idPnt-(int)pp2->pp_idPnt;
}
int GetPntIndex(LWPntID id)
{
  LWPntID *p = (LWPntID *)bsearch(&id, _aidPntIDs, _ctPntIDs, sizeof(id), qsort_CompareIDs);
  if (p==NULL) {
    assert(false);
    return 0;
  } else {
    return p-_aidPntIDs;
  }
}
int GetPolIndex(LWPolID id)
{
  LWPolID *p = (LWPolID *)bsearch(&id, _aidPolIDs, _ctPolIDs, sizeof(id), qsort_CompareIDs);
  if (p==NULL) {
    assert(false);
    return 0;
  } else {
    return p-_aidPolIDs;
  }
}

int EnumPnts(void *, LWPntID id)
{
  _aidPntIDs[_iPnt++] = id;
  return 0;
}

int EnumPols(void *, LWPolID id)
{
  _aidPolIDs[_iPol++] = id;
  return 0;
}

// fill base vertex coordinates (without morphing)
void FillOriginalVertexCoords(void)
{
  // for each vertex
  {for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
    // get the coords
    _pmesh->pntBasePos(_pmesh, _aidPntIDs[iPnt], _avPnts[iPnt]);
  }}
}
// fill vertex coordinates with relative morphing
void FillRelativeMorphVertexCoords(const char *strRelMorphMapName)
{
  // fill base vertex coordinates (without morphing)
  FillOriginalVertexCoords();

  void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_MORF, strRelMorphMapName);
  _pmesh->pntVSelect(_pmesh, pMap);
  // for each vertex
  {for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
    // if morphed
    float v[3];
    if (_pmesh->pntVGet(_pmesh, _aidPntIDs[iPnt], v)) {
      // apply the morph
      _avPnts[iPnt][0]+=v[0];
      _avPnts[iPnt][1]+=v[1];
      _avPnts[iPnt][2]+=v[2];
    }
  }}
}
// fill vertex coordinates with absolute morphing
void FillAbsoluteMorphVertexCoords(const char *strAbsMorphMapName)
{
  // fill base vertex coordinates (without morphing)
  FillOriginalVertexCoords();

  void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_SPOT, strAbsMorphMapName);
  _pmesh->pntVSelect(_pmesh, pMap);
  // for each vertex
  {for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
    // if morphed
    float v[3];
    if (_pmesh->pntVGet(_pmesh, _aidPntIDs[iPnt], v)) {
      // apply the morph
      _avPnts[iPnt][0] = v[0];
      _avPnts[iPnt][1] = v[1];
      _avPnts[iPnt][2] = v[2];
    }
  }}
}

// calculate mesh normals using local vertex coordinates (to be able to calculate normals for morphmaps)
void MakeNormals(void)
{
  // generate polygon normals
  {for(int iPol=0; iPol<_ctPolIDs; iPol++) {
    LWPolID idPol = _aidPolIDs[iPol];
    int ctInThisPol = _pmesh->polSize(_pmesh, idPol);
    if (ctInThisPol<3) {
      _avPolNormals[iPol][0] = 0.0f;
      _avPolNormals[iPol][1] = 0.0f;
      _avPolNormals[iPol][2] = 0.0f;
    }
    LWPntID idPnt0 = _pmesh->polVertex(_pmesh, idPol, ctInThisPol-1);
    LWPntID idPnt1 = _pmesh->polVertex(_pmesh, idPol, 0);
    LWPntID idPnt2 = _pmesh->polVertex(_pmesh, idPol, 1);
    float v0[3], v1[3], v2[3];
    _pmesh->pntBasePos(_pmesh, idPnt0, v0);
    _pmesh->pntBasePos(_pmesh, idPnt1, v1);
    _pmesh->pntBasePos(_pmesh, idPnt2, v2);
    float d1[3], d2[3];
    for (int i=0; i<3; i++) {
      d1[i] = v2[i] - v1[i];
      d2[i] = v0[i] - v1[i];
    }
    cross(d1, d2, _avPolNormals[iPol]);
    normalize( _avPolNormals[iPol]);
  }}

  // generate point normals
  memset(_avPntNormals, 0, _ctPntIDs*sizeof(float)*3);

  float vNormalSum[3];
  LWPntID idLastPnt = NULL;

  // for each point polygon
  {for(int iPntPol=0; iPntPol<_ctPolPnts; iPntPol++) {
    LWPntID idThis = _appPntPols[iPntPol].pp_idPnt;
    // if new point
    if (idThis!=idLastPnt) {
      // store value for the last point (unless it was the first one)
      if (idLastPnt!=NULL) {
        int iLastPnt = GetPntIndex(idLastPnt);
        normalize(vNormalSum);
        _avPntNormals[iLastPnt][0] = vNormalSum[0];
        _avPntNormals[iLastPnt][1] = vNormalSum[1];
        _avPntNormals[iLastPnt][2] = vNormalSum[2];
      }
      // reset averaging values
      vNormalSum[0] = 0.0f;
      vNormalSum[1] = 0.0f;
      vNormalSum[2] = 0.0f;
    }
    // add the polygon normal to the averaging values
    int iPol = GetPolIndex(_appPntPols[iPntPol].pp_idPol);
    vNormalSum[0] += _avPolNormals[iPol][0];
    vNormalSum[1] += _avPolNormals[iPol][1];
    vNormalSum[2] += _avPolNormals[iPol][2];
    
    idLastPnt=idThis;
  }}

  int iLastPnt = GetPntIndex(idLastPnt);
  normalize(vNormalSum);
  _avPntNormals[iLastPnt][0] = vNormalSum[0];
  _avPntNormals[iLastPnt][1] = vNormalSum[1];
  _avPntNormals[iLastPnt][2] = vNormalSum[2];
}


// extract polygon and point info
void ExtractMeshData(void)
{
  // extract point and poly ids, so we can walk them later
  _ctPntIDs = _pmesh->numPoints(_pmesh);
  _aidPntIDs = (LWPntID*)malloc(_ctPntIDs*sizeof(LWPntID));
  _iPnt = 0;
  _pmesh->scanPoints(_pmesh, EnumPnts, NULL);
  qsort(_aidPntIDs, _ctPntIDs, sizeof(LWPntID), qsort_CompareIDs);

  _ctPolIDs = _pmesh->numPolygons(_pmesh);
  _aidPolIDs = (LWPolID*)malloc(_ctPolIDs*sizeof(LWPolID));
  _iPol = 0;
  _pmesh->scanPolys(_pmesh, EnumPols, NULL);
  qsort(_aidPolIDs, _ctPolIDs, sizeof(LWPolID), qsort_CompareIDs);

  // find the total number of points in all polygons
  _ctPolPnts = 0;
  for(int iPol=0; iPol<_ctPolIDs; iPol++) {
    int ct = _pmesh->polSize(_pmesh, _aidPolIDs[iPol]);
    if (ct!=3) {
      _msg->error("All objects must be triangles!", NULL);
    }
    _ctPolPnts += ct;
  }
  _appPolPnts = (PolPnt *)malloc(_ctPolPnts*sizeof(PolPnt));
  _appPntPols = (PolPnt *)malloc(_ctPolPnts*sizeof(PolPnt));
  // fill in all point and polygon combinations
  {int iPolPnt = 0;
  for(int iPol=0; iPol<_ctPolIDs; iPol++) {
    LWPolID idPol = _aidPolIDs[iPol];
    int ctInThisPol = _pmesh->polSize(_pmesh, idPol);
    for (int iPnt=0; iPnt<ctInThisPol; iPnt++) {
      LWPntID idPnt = _pmesh->polVertex(_pmesh, idPol, iPnt);
      _appPolPnts[iPolPnt].pp_idPol = idPol;
      _appPolPnts[iPolPnt].pp_idPnt = idPnt;
      iPolPnt++;
    }
  }}
  // copy to per-point array and sort by points
  memcpy(_appPntPols, _appPolPnts, _ctPolPnts*sizeof(PolPnt));
  qsort(_appPntPols, _ctPolPnts, sizeof(PolPnt), qsort_ComparePolPntsByPnt);

  // allocate point coords and polygon and point normals
  _avPnts = (float(*)[3])malloc(_ctPntIDs*sizeof(float)*3);
  _avPolNormals = (float(*)[3])malloc(_ctPolIDs*sizeof(float)*3);
  _avPntNormals = (float(*)[3])malloc(_ctPntIDs*sizeof(float)*3);

  // calculate mesh normals using base vertex coordinates (without morphing)
  FillOriginalVertexCoords();
  MakeNormals();

  // list all the uvmaps used by this object
  _ctUVMapNames = _obf->numVMaps(LWVMAP_TXUV);
  _astrUVMapNames = (const char**) malloc(_ctUVMapNames*sizeof(char*));
  memset(_astrUVMapNames, 0, _ctUVMapNames*sizeof(char*));
  _ctUsedUVMapNames = 0;
  {for(int iUVMap=0; iUVMap<_ctUVMapNames; iUVMap++) {
    const char *strName = _obf->vmapName(LWVMAP_TXUV, iUVMap);
    void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_TXUV, strName);
    _pmesh->pntVSelect(_pmesh, pMap);
    bool bExists = false;
    for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
      float v[2];
      if (_pmesh->pntVGet(_pmesh, _aidPntIDs[iPnt], v)) {
        bExists = true;
        break;
      }
    }
    if (bExists) {
      _astrUVMapNames[_ctUsedUVMapNames++] = strName;
    }
  }}

  // list all the weightmaps used by this object
  _ctWeightMapNames = _obf->numVMaps(LWVMAP_WGHT);
  _astrWeightMapNames = (const char**) malloc(_ctWeightMapNames*sizeof(char*));
  _actWeightMapCounts = (int *) malloc(_ctWeightMapNames*sizeof(int));
  memset(_astrWeightMapNames, 0, _ctWeightMapNames*sizeof(char*));
  memset(_actWeightMapCounts, 0, _ctWeightMapNames*sizeof(int));
  _ctUsedWeightMapNames = 0;
  {for(int iWeightMap=0; iWeightMap<_ctWeightMapNames; iWeightMap++) {
    const char *strName = _obf->vmapName(LWVMAP_WGHT, iWeightMap);
    void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_WGHT, strName);
    _pmesh->pntVSelect(_pmesh, pMap);
    int ct = 0;
    // for each polygonvertex
    for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the weight
      float v[1];
      if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, v) && v[0]!=0.0f) {
        ct++;
      }
    }
    if (ct>0)
    {
      _astrWeightMapNames[_ctUsedWeightMapNames] = strName;
      _actWeightMapCounts[_ctUsedWeightMapNames] = ct;
      _ctUsedWeightMapNames++;
    }
  }}

  // list all the relmorphmaps used by this object
  _ctRelMorphMapNames = _obf->numVMaps(LWVMAP_MORF);
  _astrRelMorphMapNames = (const char**) malloc(_ctRelMorphMapNames*sizeof(char*));
  _actRelMorphMapCounts = (int *) malloc(_ctRelMorphMapNames*sizeof(int));
  memset(_astrRelMorphMapNames, 0, _ctRelMorphMapNames*sizeof(char*));
  memset(_actRelMorphMapCounts, 0, _ctRelMorphMapNames*sizeof(int));
  _ctUsedRelMorphMapNames = 0;
  {for(int iRelMorphMap=0; iRelMorphMap<_ctRelMorphMapNames; iRelMorphMap++) {
    const char *strName = _obf->vmapName(LWVMAP_MORF, iRelMorphMap);
    void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_MORF, strName);
    _pmesh->pntVSelect(_pmesh, pMap);
    int ct = 0;
    // for each polygonvertex
    for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the morphpos
      float v[3];
      if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, v)) {
        ct++;
      }
    }
    if (ct>0) {
      _astrRelMorphMapNames[_ctUsedRelMorphMapNames] = strName;
    }
    _actRelMorphMapCounts[_ctUsedRelMorphMapNames] = ct;
    _ctUsedRelMorphMapNames++;
  }}

  // list all the absmorphmaps used by this object
  _ctAbsMorphMapNames = _obf->numVMaps(LWVMAP_SPOT);
  _astrAbsMorphMapNames = (const char**) malloc(_ctAbsMorphMapNames*sizeof(char*));
  _actAbsMorphMapCounts = (int *) malloc(_ctAbsMorphMapNames*sizeof(int));
  memset(_astrAbsMorphMapNames, 0, _ctAbsMorphMapNames*sizeof(char*));
  memset(_actAbsMorphMapCounts, 0, _ctAbsMorphMapNames*sizeof(int));
  _ctUsedAbsMorphMapNames = 0;
  {for(int iAbsMorphMap=0; iAbsMorphMap<_ctAbsMorphMapNames; iAbsMorphMap++) {
    const char *strName = _obf->vmapName(LWVMAP_SPOT, iAbsMorphMap);
    void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_SPOT, strName);
    _pmesh->pntVSelect(_pmesh, pMap);
    int ct = 0;
    // for each polygonvertex
    for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the morphpos
      float v[3];
      if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, v)) {
        ct++;
      }
    }
    if (ct>0) {
      _astrAbsMorphMapNames[_ctUsedAbsMorphMapNames] = strName;
    }
    _actAbsMorphMapCounts[_ctUsedAbsMorphMapNames] = ct;
    _ctUsedAbsMorphMapNames++;
  }}
}

FILE *_f = NULL;

int ExportMesh(LWXPanelID pan)
{
  // is mesh face forward
  int iFaceForward = *(int*)_xpanf->formGet( pan, ID_FACEFORWARD);

  // !!!! make it work with a selected object, not the first one in scene
  ReloadGlobalObjects();
  bool bExportOnlySelected = false;
  // count selected objects
  int ctSelectedMeshed = 0;
  int ctMeshes=0;
  _objid = _iti->first(LWI_OBJECT,0);
  while(_objid != LWITEM_NULL)
  {
    if(_iti->type(_objid) == LWI_OBJECT)
    {
      _pmesh = _obi->meshInfo(_objid, 0);
      if(_pmesh != NULL)
      {
        if(_ifi->itemFlags(_objid) & LWITEMF_SELECTED)
        {
          ctSelectedMeshed++;
        }
      }
      ctMeshes++;
    }
    _objid = _iti->next(_objid);
  }

  // get the first object in the scene
  _objid = _iti->first(LWI_OBJECT,0);
  if (!_objid)
  {
    _msg->error("No object in the scene.", NULL);
    return AFUNC_OK;
  }

  // if some objects are selected export only them
  if(ctSelectedMeshed > 0) bExportOnlySelected = true;
  // dont ask to export all meshes if only one mesh in the scene
  if(ctSelectedMeshed == 0)
  {
    if(ctMeshes > 1)
    {
      if(_msg->yesNo("No objects selected","Export all meshes?",NULL) == 0)
        return AFUNC_OK;
      bExportOnlySelected = false;
    }
  }

  // loop each mesh in scene
  while(_objid != LWITEM_NULL)
  {
    // get its mesh
    _pmesh = _obi->meshInfo(_objid, 0);
    if(_pmesh == NULL)
    {
      _objid = _iti->next(_objid);
      continue;
    }
    if(bExportOnlySelected)
    {
      if(!(_ifi->itemFlags(_objid) & LWITEMF_SELECTED))
      {
        _objid = _iti->next(_objid);
        continue;
      }
    }

    // get mesh name
    _strFileName = strdup(_obi->filename(_objid));

    // extract polygon and point info
    ExtractMeshData();

    // open the file to print into
    char fnmOut[256];
    strcpy(fnmOut, _strFileName);
    char *pchDot = strrchr(fnmOut, '.');
    if (pchDot!=NULL) {
      strcpy(pchDot, ".am");
    }
    _f = fopen(fnmOut, "w");
    if (_f==NULL) {
      _msg->error("Can't open file", fnmOut);
      goto end;
    }

    // write the mesh header
    fprintf(_f, "SE_MESH %s;\n\n",SE_ANIM_VER);
    if(iFaceForward==ML_HALF_FACE_FORWARD) {
      fprintf(_f, "HALF_FACE_FORWARD TRUE;\n\n");
    } else if(iFaceForward==ML_FULL_FACE_FORWARD) {
      fprintf(_f, "FULL_FACE_FORWARD TRUE;\n\n");
    }


    // write the vertex header
    fprintf(_f, "VERTICES %d\n", _ctPolPnts);
    fprintf(_f, "{\n");
    // for each polygonvertex
    {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the coords
      float v[3];
      _pmesh->pntBasePos(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, v);
      fprintf(_f, "  %g, %g, %g;\n", v[0], v[1], -v[2]);
    }}
    fprintf(_f, "}\n\n");

    // write the normal header
    fprintf(_f, "NORMALS %d\n", _ctPolPnts);
    fprintf(_f, "{\n");
    // for each polygonvertex
    {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the normal
      int iPnt = GetPntIndex(_appPolPnts[iPolPnt].pp_idPnt);
      fprintf(_f, "  %g, %g, %g;\n", _avPntNormals[iPnt][0], _avPntNormals[iPnt][1], -_avPntNormals[iPnt][2]);
    }}
    fprintf(_f, "}\n\n");

    // write the uvmaps header
    fprintf(_f, "UVMAPS %d\n", _ctUsedUVMapNames);
    fprintf(_f, "{\n");

    // for each uvmap

    if (_ctUsedUVMapNames == 0) {
      _msg->info("No UV maps in the scene!",NULL);
    }
    {for(int iUVMap=0; iUVMap<_ctUsedUVMapNames; iUVMap++) {
      fprintf(_f, "  {\n");
      const char *strUVMap = _astrUVMapNames[iUVMap];
      fprintf(_f, "    NAME \"%s\";\n", strUVMap);
      void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_TXUV, strUVMap);
      _pmesh->pntVSelect(_pmesh, pMap);
      fprintf(_f, "    TEXCOORDS %d\n", _ctPolPnts);
      fprintf(_f, "    {\n");

      // for each polygonvertex
      {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
        // get the coords
        float v[2];
        if (_pmesh->pntVPGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, _appPolPnts[iPolPnt].pp_idPol, v)) {
        } else if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, v)) {
        } else {
          v[0] = 0.0f;
          v[1] = 0.0f;
        }
        fprintf(_f, "      %g, %g;\n", v[0], 1.0f-v[1]);
      }}
      fprintf(_f, "    }\n");
      fprintf(_f, "  }\n");
    }}
    fprintf(_f, "}\n\n");

    // get surfaces
    _asurSurfaces = _srf->byObject(_strFileName);

    // count the surfaces
    _ctSurfs = 0;
    while(_asurSurfaces[_ctSurfs]!=NULL) {
      _ctSurfs++;
    }

    // write the surfaces header
    fprintf(_f, "SURFACES %d\n", _ctSurfs);
    fprintf(_f, "{\n");
    // for each surface
    {for(int iSurf=0; iSurf<_ctSurfs; iSurf++) {
      fprintf(_f, "  {\n");
      const char *strSurf = _srf->name(_asurSurfaces[iSurf]);
      fprintf(_f, "    NAME \"%s\";\n", strSurf);
      // count the polygons
      int iSurfPols = 0;
      {for(int i=0; i<_ctPolIDs; i++) {
        if (strcmp(_pmesh->polTag(_pmesh, _aidPolIDs[i], LWPTAG_SURF), strSurf)==0) {
          iSurfPols++;
        }
      }}
      // write the polygon set header
      fprintf(_f, "    TRIANGLE_SET %d\n", iSurfPols);
      fprintf(_f, "    {\n");
      // for each polygon
      for(int iTri=0; iTri<_ctPolIDs; iTri++) {
        LWPolID idPol = _aidPolIDs[iTri];
        // if not in this surface
        if (strcmp(_pmesh->polTag(_pmesh, idPol, LWPTAG_SURF), strSurf)!=0) {
          // skip it
          continue;
        }
  //      assert(_appPolPnts[iTri*3+0].pp_idPol == idPol);
  //      assert(_appPolPnts[iTri*3+1].pp_idPol == idPol);
  //      assert(_appPolPnts[iTri*3+2].pp_idPol == idPol);
        fprintf(_f, "    %d, %d, %d;\n", iTri*3+2, iTri*3+1, iTri*3+0);
  //        GetPntIndex(_appPolPnts[iTri*3+0].pp_idPnt),
  //        GetPntIndex(_appPolPnts[iTri*3+1].pp_idPnt),
  //        GetPntIndex(_appPolPnts[iTri*3+2].pp_idPnt));
      }
      fprintf(_f, "    }\n");
      fprintf(_f, "  }\n");
    }}
    fprintf(_f, "}\n");

    // write the weightmaps header
    fprintf(_f, "WEIGHTS %d\n", _ctUsedWeightMapNames);
    fprintf(_f, "{\n");

    // for each weightmap
    {for(int iWeightMap=0; iWeightMap<_ctUsedWeightMapNames; iWeightMap++) {
      fprintf(_f, "  {\n");
      const char *strWeightMap = _astrWeightMapNames[iWeightMap];
      fprintf(_f, "    NAME \"%s\";\n", strWeightMap);
      void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_WGHT, strWeightMap);
      _pmesh->pntVSelect(_pmesh, pMap);
      fprintf(_f, "    WEIGHT_SET %d\n", _actWeightMapCounts[iWeightMap]);
      fprintf(_f, "    {\n");

      // for each polygonvertex
      {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
        // get the coords
        float v[1];
        if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, v) && v[0]!=0.0f) {
          if (v[0] < 0) {
            _msg->error("Weight map value lesser than zero!",NULL);
          } else {
            fprintf(_f, "      { %d; %g; }\n", iPolPnt, v[0]);
          }
        }
      }}
      fprintf(_f, "    }\n");
      fprintf(_f, "  }\n");
    }}
    fprintf(_f, "}\n\n");

    // write the morphmaps header
    fprintf(_f, "MORPHS %d\n", _ctUsedRelMorphMapNames+_ctUsedAbsMorphMapNames);
    fprintf(_f, "{\n");

    // for each relmorphmap
    {for(int iRelMorphMap=0; iRelMorphMap<_ctUsedRelMorphMapNames; iRelMorphMap++) {
      const char *strRelMorphMap = _astrRelMorphMapNames[iRelMorphMap];
      // calculate mesh normals using the given morphmap
      FillRelativeMorphVertexCoords(strRelMorphMap);
      MakeNormals();

      fprintf(_f, "  {\n");
      fprintf(_f, "    NAME \"%s\";\n", strRelMorphMap);
      fprintf(_f, "    RELATIVE TRUE;\n");
      void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_MORF, strRelMorphMap);
      _pmesh->pntVSelect(_pmesh, pMap);
      fprintf(_f, "    MORPH_SET %d\n", _actRelMorphMapCounts[iRelMorphMap]);
      fprintf(_f, "    {\n");

      // for each polygonvertex
      {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
        // if morphed here
        float vRel[3];
        if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, vRel)) {
          // write the coords and normal
          int iPnt = GetPntIndex(_appPolPnts[iPolPnt].pp_idPnt);
          fprintf(_f, "      { %d; %g, %g, %g; %g, %g, %g; }\n", iPolPnt, 
            _avPnts[iPnt][0], _avPnts[iPnt][1], -_avPnts[iPnt][2], 
            _avPntNormals[iPnt][0], _avPntNormals[iPnt][1], -_avPntNormals[iPnt][2]);
        }
      }}
      fprintf(_f, "    }\n");
      fprintf(_f, "  }\n");
    }}
    // for each absmorphmap
    {for(int iAbsMorphMap=0; iAbsMorphMap<_ctUsedAbsMorphMapNames; iAbsMorphMap++) {
      const char *strAbsMorphMap = _astrAbsMorphMapNames[iAbsMorphMap];
      // calculate mesh normals using the given morphmap
      FillAbsoluteMorphVertexCoords(strAbsMorphMap);
      MakeNormals();

      fprintf(_f, "  {\n");
      fprintf(_f, "    NAME \"%s\";\n", strAbsMorphMap);
      fprintf(_f, "    RELATIVE FALSE;\n");
      void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_SPOT, strAbsMorphMap);
      _pmesh->pntVSelect(_pmesh, pMap);
      fprintf(_f, "    MORPH_SET %d\n", _actAbsMorphMapCounts[iAbsMorphMap]);
      fprintf(_f, "    {\n");

      // for each polygonvertex
      {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
        // if morphed here
        float vRel[3];
        if (_pmesh->pntVGet(_pmesh, _appPolPnts[iPolPnt].pp_idPnt, vRel)) {
          // write the coords and normal
          int iPnt = GetPntIndex(_appPolPnts[iPolPnt].pp_idPnt);
          fprintf(_f, "      { %d; %g, %g, %g; %g, %g, %g; }\n", iPolPnt, 
            _avPnts[iPnt][0], _avPnts[iPnt][1], -_avPnts[iPnt][2], 
            _avPntNormals[iPnt][0], _avPntNormals[iPnt][1], -_avPntNormals[iPnt][2]);
        }
      }}
      fprintf(_f, "    }\n");
      fprintf(_f, "  }\n");
    }}
    fprintf(_f, "}\n\n");
    fprintf(_f, "SE_MESH_END;\n");

    _msg->info("Saved:", fnmOut);

  end:
    // close and free everything
    if (_f!=NULL) {
      fclose(_f);
      _f=NULL;
    }
    if (_aidPntIDs!=NULL) {
      free(_aidPntIDs);
      _aidPntIDs = NULL;
    }
    if (_aidPolIDs!=NULL) {
      free(_aidPolIDs);
      _aidPolIDs = NULL;
    }
    if (_appPolPnts!=NULL) {
      free(_appPolPnts);
      _appPolPnts = NULL;
    }
    if (_appPntPols!=NULL) {
      free(_appPntPols);
      _appPntPols = NULL;
    }
    if (_avPnts!=NULL) {
      free(_avPnts);
      _avPnts = NULL;
    }
    if (_avPolNormals!=NULL) {
      free(_avPolNormals);
      _avPolNormals = NULL;
    }
    if (_avPntNormals!=NULL) {
      free(_avPntNormals);
      _avPntNormals = NULL;
    }
    if (_strFileName!=NULL) {
      free(_strFileName);
      _strFileName = NULL;
    }
    if (_astrUVMapNames!=NULL) {
      free(_astrUVMapNames);
      _astrUVMapNames = NULL;
    }
    if (_astrWeightMapNames!=NULL) {
      free(_astrWeightMapNames);
      _astrWeightMapNames = NULL;
    }
    // get next mesh obj
    _objid = _iti->next(_objid);
  }

  return AFUNC_OK;
}
