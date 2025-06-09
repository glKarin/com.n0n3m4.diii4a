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



#include "base.h"
#include <assert.h>

FILE *_fpOutput = NULL;

EDStateRef _state;

extern int msgbox_modeler( LWXPanelFuncs *xpanf, const char* msg );


extern LWItemID _objid;
LWSurfaceID *_asurSurfaces;
extern char *_strFileName;

// arrays of all point and polygon ids
extern int _ctPntIDs;
extern int _iPnt;
extern LWPntID *_aidPntIDs;

extern int _ctPolIDs;
extern int _iPol;
extern LWPolID *_aidPolIDs;

// point coordinates
extern float (*_avPnts)[3];
// polygon normals
extern float (*_avPolNormals)[3];
// point normals
extern float (*_avPntNormals)[3];

// uvmapnames
extern const char **_astrUVMapNames;
extern int _ctUVMapNames;
extern int _ctUsedUVMapNames;
// weightmapnames
extern const char **_astrWeightMapNames;
extern int *_actWeightMapCounts;
extern int _ctWeightMapNames;
extern int _ctUsedWeightMapNames;

// relmorphmapnames
extern const char **_astrRelMorphMapNames;
extern int *_actRelMorphMapCounts;
extern int _ctRelMorphMapNames;
extern int _ctUsedRelMorphMapNames;

// absmorphmapnames
extern const char **_astrAbsMorphMapNames;
extern int *_actAbsMorphMapCounts;
extern int _ctAbsMorphMapNames;
extern int _ctUsedAbsMorphMapNames;

// here we store ids for all point and polygon combinations in order by polygons and by points, effectively
// having all info needed to handle the unwelded object and info needed to calculate normals
extern int _ctPolPnts;
extern PolPnt *_appPolPnts;
extern PolPnt *_appPntPols;

extern int _ctSurfs;




int EnumPnts_modeler(void *, const EDPointInfo *poiPointInfo)
{
  _aidPntIDs[_iPnt++] = poiPointInfo->pnt;
  return 0;
}

int EnumPols_modeler(void *, const EDPolygonInfo *pliPollyInfo)
{

  _aidPolIDs[_iPol++] = pliPollyInfo->pol;

  if (pliPollyInfo->numPnts != 3) {
    _msg->error("All objects must be triangles!", NULL);
  }

  _ctPolPnts += pliPollyInfo->numPnts;

  return 0;
}



int __cdecl qsort_CompareIDs_modeler(const void *elem1, const void *elem2)
{
  return *(int*)elem1-*(int*)elem2;
}


int __cdecl qsort_ComparePolPntsByPnt_modeler(const void *elem1, const void *elem2)
{
  PolPnt *pp1 = (struct PolPnt *)elem1;
  PolPnt *pp2 = (struct PolPnt *)elem2;

  return (int)pp1->pp_idPnt-(int)pp2->pp_idPnt;
}



// fill base vertex coordinates (without morphing)
void FillOriginalVertexCoords_modeler(void)
{
	EDPointInfo * poiPoint;

  // for each vertex
  for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
    // get the coords
		poiPoint = _meshEditOperations->pointInfo(_state, _aidPntIDs[iPnt]);
		_avPnts[iPnt][0] = (float) poiPoint->position[0];
		_avPnts[iPnt][1] = (float) poiPoint->position[1];
		_avPnts[iPnt][2] = (float) poiPoint->position[2];
  }

}


// fill vertex coordinates with relative morphing
void FillRelativeMorphVertexCoords_modeler(const char *strRelMorphMapName)
{
  // fill base vertex coordinates (without morphing)
  FillOriginalVertexCoords_modeler();

	void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_MORF, strRelMorphMapName);
  
  // for each vertex
  {for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
    // if morphed
    float v[3];
    if (_meshEditOperations->pointVGet(_state, _aidPntIDs[iPnt], v)) {
      // apply the morph
      _avPnts[iPnt][0]+=v[0];
      _avPnts[iPnt][1]+=v[1];
      _avPnts[iPnt][2]+=v[2];
    }
  }}
}
// fill vertex coordinates with absolute morphing
void FillAbsoluteMorphVertexCoords_modeler(const char *strAbsMorphMapName)
{
  // fill base vertex coordinates (without morphing)
  FillOriginalVertexCoords_modeler();

	void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_SPOT, strAbsMorphMapName);
 
  // for each vertex
  {for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
    // if morphed
    float v[3];
    if (_meshEditOperations->pointVGet(_state, _aidPntIDs[iPnt], v)) {
      // apply the morph
      _avPnts[iPnt][0] = v[0];
      _avPnts[iPnt][1] = v[1];
      _avPnts[iPnt][2] = v[2];
    }
  }}
}


int GetPntIndex_modeler(LWPntID id)
{
  LWPntID *p = (LWPntID *)bsearch(&id, _aidPntIDs, _ctPntIDs, sizeof(id), qsort_CompareIDs_modeler);
  if (p==NULL) {
    assert(false);
    return 0;
  } else {
    return p-_aidPntIDs;
  }
}
int GetPolIndex_modeler(LWPolID id)
{
  LWPolID *p = (LWPolID *)bsearch(&id, _aidPolIDs, _ctPolIDs, sizeof(id), qsort_CompareIDs_modeler);
  if (p==NULL) {
    assert(false);
    return 0;
  } else {
    return p-_aidPolIDs;
  }
}


void MakeNormals_modeler(void)
{


  // generate polygon normals
  for(int iPol=0; iPol<_ctPolIDs; iPol++) {
    LWPolID idPol = _aidPolIDs[iPol];
		double dPolNormal[3];
		_meshEditOperations->polyNormal(_state,idPol,dPolNormal);

		_avPolNormals[iPol][0] = (float) dPolNormal[0];
		_avPolNormals[iPol][1] = (float) dPolNormal[1];
		_avPolNormals[iPol][2] = (float) dPolNormal[2];

  }

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
        int iLastPnt = GetPntIndex_modeler(idLastPnt);
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
    int iPol = GetPolIndex_modeler(_appPntPols[iPntPol].pp_idPol);
    vNormalSum[0] += _avPolNormals[iPol][0];
    vNormalSum[1] += _avPolNormals[iPol][1];
    vNormalSum[2] += _avPolNormals[iPol][2];
    
    idLastPnt=idThis;
  }}

  int iLastPnt = GetPntIndex_modeler(idLastPnt);
  normalize(vNormalSum);
  _avPntNormals[iLastPnt][0] = vNormalSum[0];
  _avPntNormals[iLastPnt][1] = vNormalSum[1];
  _avPntNormals[iLastPnt][2] = vNormalSum[2];
}



// extract polygon and point info
void ExtractMeshData_modeler(void)
{
  // extract point and poly ids, so we can walk them later
  _aidPntIDs = (LWPntID*)malloc(_ctPntIDs*sizeof(LWPntID));
  _iPnt = 0;
	_meshEditOperations->pointScan(_state,EnumPnts_modeler,NULL,OPLYR_FG);
  qsort(_aidPntIDs, _ctPntIDs, sizeof(LWPntID), qsort_CompareIDs_modeler);

  _aidPolIDs = (LWPolID*)malloc(_ctPolIDs*sizeof(LWPolID));
  _iPol = 0;
	_ctPolPnts = 0;
	_meshEditOperations->polyScan(_state,EnumPols_modeler,NULL,OPLYR_FG);
  qsort(_aidPolIDs, _ctPolIDs, sizeof(LWPolID), qsort_CompareIDs_modeler);

  _appPolPnts = (PolPnt *)malloc(_ctPolPnts*sizeof(PolPnt));
  _appPntPols = (PolPnt *)malloc(_ctPolPnts*sizeof(PolPnt));
  // fill in all point and polygon combinations
  {int iPolPnt = 0;
	 EDPolygonInfo *pliPolly;
  for(int iPol=0; iPol<_ctPolIDs; iPol++) {
    LWPolID idPol = _aidPolIDs[iPol];
		pliPolly = _meshEditOperations->polyInfo(_state,idPol);
    int ctInThisPol = pliPolly->numPnts;
    for (int iPnt=0; iPnt<ctInThisPol; iPnt++) {
      _appPolPnts[iPolPnt].pp_idPol = idPol;
      _appPolPnts[iPolPnt].pp_idPnt = pliPolly->points[iPnt];
      iPolPnt++;
    }
  }}
  // copy to per-point array and sort by points
  memcpy(_appPntPols, _appPolPnts, _ctPolPnts*sizeof(PolPnt));
  qsort(_appPntPols, _ctPolPnts, sizeof(PolPnt), qsort_ComparePolPntsByPnt_modeler);


	// allocate point coords and polygon and point normals
  _avPnts = (float(*)[3])malloc(_ctPntIDs*sizeof(float)*3);
  _avPolNormals = (float(*)[3])malloc(_ctPolIDs*sizeof(float)*3);
  _avPntNormals = (float(*)[3])malloc(_ctPntIDs*sizeof(float)*3);

  // calculate mesh normals using base vertex coordinates (without morphing)
  FillOriginalVertexCoords_modeler();
  MakeNormals_modeler();


	// list all the uvmaps used by this object
  _ctUVMapNames = _objfunc->numVMaps(LWVMAP_TXUV);
  _astrUVMapNames = (const char**) malloc(_ctUVMapNames*sizeof(char*));
  memset(_astrUVMapNames, 0, _ctUVMapNames*sizeof(char*));
  _ctUsedUVMapNames = 0;
  {for(int iUVMap=0; iUVMap<_ctUVMapNames; iUVMap++) {
    const char *strName = _objfunc->vmapName(LWVMAP_TXUV, iUVMap);
    void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_TXUV, strName);
    bool bExists = false;
    for(int iPnt=0; iPnt<_ctPntIDs; iPnt++) {
      float v[2];
      if (_meshEditOperations->pointVGet(_state, _aidPntIDs[iPnt], v)) {
        bExists = true;
        break;
      }
    }
    if (bExists) {
      _astrUVMapNames[_ctUsedUVMapNames++] = strName;
    }
  }}


	// list all the weightmaps used by this object
  _ctWeightMapNames = _objfunc->numVMaps(LWVMAP_WGHT);
  _astrWeightMapNames = (const char**) malloc(_ctWeightMapNames*sizeof(char*));
  _actWeightMapCounts = (int *) malloc(_ctWeightMapNames*sizeof(int));
  memset(_astrWeightMapNames, 0, _ctWeightMapNames*sizeof(char*));
  memset(_actWeightMapCounts, 0, _ctWeightMapNames*sizeof(int));
  _ctUsedWeightMapNames = 0;
  {for(int iWeightMap=0; iWeightMap<_ctWeightMapNames; iWeightMap++) {
    const char *strName = _objfunc->vmapName(LWVMAP_WGHT, iWeightMap);
		void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_WGHT, strName);
    int ct = 0;
    // for each polygonvertex
    for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the weight
      float v[1];
      if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, v) && v[0]!=0.0f) {
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
  _ctRelMorphMapNames = _objfunc->numVMaps(LWVMAP_MORF);
  _astrRelMorphMapNames = (const char**) malloc(_ctRelMorphMapNames*sizeof(char*));
  _actRelMorphMapCounts = (int *) malloc(_ctRelMorphMapNames*sizeof(int));
  memset(_astrRelMorphMapNames, 0, _ctRelMorphMapNames*sizeof(char*));
  memset(_actRelMorphMapCounts, 0, _ctRelMorphMapNames*sizeof(int));
  _ctUsedRelMorphMapNames = 0;
  {for(int iRelMorphMap=0; iRelMorphMap<_ctRelMorphMapNames; iRelMorphMap++) {
    const char *strName = _objfunc->vmapName(LWVMAP_MORF, iRelMorphMap);
		void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_MORF, strName);
    int ct = 0;
    // for each polygonvertex
    for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the morphpos
      float v[3];
      if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, v)) {
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
  _ctAbsMorphMapNames = _objfunc->numVMaps(LWVMAP_SPOT);
  _astrAbsMorphMapNames = (const char**) malloc(_ctAbsMorphMapNames*sizeof(char*));
  _actAbsMorphMapCounts = (int *) malloc(_ctAbsMorphMapNames*sizeof(int));
  memset(_astrAbsMorphMapNames, 0, _ctAbsMorphMapNames*sizeof(char*));
  memset(_actAbsMorphMapCounts, 0, _ctAbsMorphMapNames*sizeof(int));
  _ctUsedAbsMorphMapNames = 0;
  {for(int iAbsMorphMap=0; iAbsMorphMap<_ctAbsMorphMapNames; iAbsMorphMap++) {
    const char *strName = _objfunc->vmapName(LWVMAP_SPOT, iAbsMorphMap);
		void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_SPOT, strName);
    int ct = 0;
    // for each polygonvertex
    for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the morphpos
      float v[3];
      if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, v)) {
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



void ExportMesh_modeler(int iFaceForward) {  
	char fnmOut[256];
	char *strFileName;

	if (_meshEditOperations == NULL) {
		_msg->error("Error!", "Error _meshEditOperations is NULL!");
		return;
	}

	_state = _meshEditOperations->state; 

  _ctPntIDs = _meshEditOperations->pointCount(_state,OPLYR_FG,EDCOUNT_ALL);
	_ctPolIDs = _meshEditOperations->polyCount(_state,OPLYR_FG,EDCOUNT_ALL);

  strFileName = strdup(_statequery->object());
  
	strcpy(fnmOut, strFileName);
  char *pchDot = strrchr(fnmOut, '.');
  if (pchDot!=NULL) {
    strcpy(pchDot, ".am");
  }

	//msgbox(_xpanf,fnmOut);

	ExtractMeshData_modeler();

	_fpOutput = fopen(fnmOut, "w");
  if (_fpOutput==NULL) {
    msgbox_modeler(_xpanf, "Can't open file!");
	return;
  }

  // write the mesh header
  fprintf(_fpOutput, "SE_MESH %s;\n\n",SE_ANIM_VER);
  // is mesh face forward

  if(iFaceForward==ML_HALF_FACE_FORWARD) {
    fprintf(_fpOutput, "HALF_FACE_FORWARD TRUE;\n\n");
  } else if(iFaceForward==ML_FULL_FACE_FORWARD) {
    fprintf(_fpOutput, "FULL_FACE_FORWARD TRUE;\n\n");
  }

	// write the vertex header
  fprintf(_fpOutput, "VERTICES %d\n", _ctPolPnts);
  fprintf(_fpOutput, "{\n");
  // for each polygonvertex
  {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
    // get the coords
		EDPointInfo * poiPoint;
		double v[3];
		poiPoint = _meshEditOperations->pointInfo(_state, _appPolPnts[iPolPnt].pp_idPnt);
		v[0] = (float) poiPoint->position[0];
		v[1] = (float) poiPoint->position[1];
		v[2] = (float) poiPoint->position[2];

    fprintf(_fpOutput, "  %g, %g, %g;\n", v[0], v[1], -v[2]);
  }}
  fprintf(_fpOutput, "}\n\n");

  // write the normal header
  fprintf(_fpOutput, "NORMALS %d\n", _ctPolPnts);
  fprintf(_fpOutput, "{\n");
  // for each polygonvertex
  {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
    // get the normal
    int iPnt = GetPntIndex_modeler(_appPolPnts[iPolPnt].pp_idPnt);
    fprintf(_fpOutput, "  %g, %g, %g;\n", _avPntNormals[iPnt][0], _avPntNormals[iPnt][1], -_avPntNormals[iPnt][2]);
  }}
  fprintf(_fpOutput, "}\n\n");


// write the uvmaps header
  fprintf(_fpOutput, "UVMAPS %d\n", _ctUsedUVMapNames);
  fprintf(_fpOutput, "{\n");

  if (_ctUsedUVMapNames == 0) {
    _msg->info("No UV maps in the scene!",NULL);
  }
  // for each uvmap
  {for(int iUVMap=0; iUVMap<_ctUsedUVMapNames; iUVMap++) {
    fprintf(_fpOutput, "  {\n");
    const char *strUVMap = _astrUVMapNames[iUVMap];
    fprintf(_fpOutput, "    NAME \"%s\";\n", strUVMap);
    void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_TXUV, strUVMap);
    fprintf(_fpOutput, "    TEXCOORDS %d\n", _ctPolPnts);
    fprintf(_fpOutput, "    {\n");

    // for each polygonvertex
    {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the coords
      float v[2];
      if (_meshEditOperations->pointVPGet(_state, _appPolPnts[iPolPnt].pp_idPnt, _appPolPnts[iPolPnt].pp_idPol, v)) {
      } else if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, v)) {
      } else {
        v[0] = 0.0f;
        v[1] = 0.0f;
      }
      fprintf(_fpOutput, "      %g, %g;\n", v[0], 1.0f-v[1]);
    }}
    fprintf(_fpOutput, "    }\n");
    fprintf(_fpOutput, "  }\n");
  }}
  fprintf(_fpOutput, "}\n\n");





	// get surfaces
  _asurSurfaces = _srf->byObject(strFileName);

  // count the surfaces
  _ctSurfs = 0;
  while(_asurSurfaces[_ctSurfs]!=NULL) {
    _ctSurfs++;
  }

  // write the surfaces header
  fprintf(_fpOutput, "SURFACES %d\n", _ctSurfs);
  fprintf(_fpOutput, "{\n");
  // for each surface
  {for(int iSurf=0; iSurf<_ctSurfs; iSurf++) {
    fprintf(_fpOutput, "  {\n");
    const char *strSurf = _srf->name(_asurSurfaces[iSurf]);
    fprintf(_fpOutput, "    NAME \"%s\";\n", strSurf);
    // count the polygons
    int iSurfPols = 0;
    {for(int i=0; i<_ctPolIDs; i++) {
      if (strcmp(_meshEditOperations->polyTag(_state, _aidPolIDs[i], LWPTAG_SURF), strSurf)==0) {
        iSurfPols++;
      }
    }}
    // write the polygon set header
    fprintf(_fpOutput, "    TRIANGLE_SET %d\n", iSurfPols);
    fprintf(_fpOutput, "    {\n");
    // for each polygon
    for(int iTri=0; iTri<_ctPolIDs; iTri++) {
      LWPolID idPol = _aidPolIDs[iTri];
      // if not in this surface
      if (strcmp(_meshEditOperations->polyTag(_state, idPol, LWPTAG_SURF), strSurf)!=0) {
        // skip it
        continue;
      }
      fprintf(_fpOutput, "    %d, %d, %d;\n", iTri*3+2, iTri*3+1, iTri*3+0);
    }
    fprintf(_fpOutput, "    }\n");
    fprintf(_fpOutput, "  }\n");
  }}
  fprintf(_fpOutput, "}\n");

	// write the weightmaps header
  fprintf(_fpOutput, "WEIGHTS %d\n", _ctUsedWeightMapNames);
  fprintf(_fpOutput, "{\n");

	// for each weightmap
  {for(int iWeightMap=0; iWeightMap<_ctUsedWeightMapNames; iWeightMap++) {
    fprintf(_fpOutput, "  {\n");
    const char *strWeightMap = _astrWeightMapNames[iWeightMap];
    fprintf(_fpOutput, "    NAME \"%s\";\n", strWeightMap);
		void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_WGHT, strWeightMap);
    fprintf(_fpOutput, "    WEIGHT_SET %d\n", _actWeightMapCounts[iWeightMap]);
    fprintf(_fpOutput, "    {\n");

    // for each polygonvertex
    {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // get the coords
      float v[1];
			if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, v) && v[0]!=0.0f) {
        if (v[0] < 0) {
          _msg->error("Weight map value lesser than zero!",NULL);
        } else {
          fprintf(_fpOutput, "      { %d; %g; }\n", iPolPnt, v[0]);
        }
      }
    }}
    fprintf(_fpOutput, "    }\n");
    fprintf(_fpOutput, "  }\n");
  }}
  fprintf(_fpOutput, "}\n\n");


	// write the morphmaps header
  fprintf(_fpOutput, "MORPHS %d\n", _ctUsedRelMorphMapNames+_ctUsedAbsMorphMapNames);
  fprintf(_fpOutput, "{\n");


	// for each relmorphmap
  {for(int iRelMorphMap=0; iRelMorphMap<_ctUsedRelMorphMapNames; iRelMorphMap++) {
    const char *strRelMorphMap = _astrRelMorphMapNames[iRelMorphMap];
    // calculate mesh normals using the given morphmap
    FillRelativeMorphVertexCoords_modeler(strRelMorphMap);
    MakeNormals_modeler();

    fprintf(_fpOutput, "  {\n");
    fprintf(_fpOutput, "    NAME \"%s\";\n", strRelMorphMap);
    fprintf(_fpOutput, "    RELATIVE TRUE;\n");
		void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_MORF, strRelMorphMap);
    fprintf(_fpOutput, "    MORPH_SET %d\n", _actRelMorphMapCounts[iRelMorphMap]);
    fprintf(_fpOutput, "    {\n");

    // for each polygonvertex
    {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // if morphed here
      float vRel[3];

      if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, vRel)) {
        // write the coords and normal
        int iPnt = GetPntIndex_modeler(_appPolPnts[iPolPnt].pp_idPnt);
        fprintf(_fpOutput, "      { %d; %g, %g, %g; %g, %g, %g; }\n", iPolPnt, 
          _avPnts[iPnt][0], _avPnts[iPnt][1], -_avPnts[iPnt][2], 
          _avPntNormals[iPnt][0], _avPntNormals[iPnt][1], -_avPntNormals[iPnt][2]);
      }
    }}
    fprintf(_fpOutput, "    }\n");
    fprintf(_fpOutput, "  }\n");
  }}
  // for each absmorphmap
  {for(int iAbsMorphMap=0; iAbsMorphMap<_ctUsedAbsMorphMapNames; iAbsMorphMap++) {
    const char *strAbsMorphMap = _astrAbsMorphMapNames[iAbsMorphMap];
    // calculate mesh normals using the given morphmap
    FillAbsoluteMorphVertexCoords_modeler(strAbsMorphMap);
    MakeNormals_modeler();

    fprintf(_fpOutput, "  {\n");
    fprintf(_fpOutput, "    NAME \"%s\";\n", strAbsMorphMap);
    fprintf(_fpOutput, "    RELATIVE FALSE;\n");
		void *pMap = _meshEditOperations->pointVSet(_state,NULL, LWVMAP_MORF, strAbsMorphMap);
    fprintf(_fpOutput, "    MORPH_SET %d\n", _actAbsMorphMapCounts[iAbsMorphMap]);
    fprintf(_fpOutput, "    {\n");

    // for each polygonvertex
    {for(int iPolPnt=0; iPolPnt<_ctPolPnts; iPolPnt++) {
      // if morphed here
      float vRel[3];
      if (_meshEditOperations->pointVGet(_state, _appPolPnts[iPolPnt].pp_idPnt, vRel)) {
        // write the coords and normal
        int iPnt = GetPntIndex_modeler(_appPolPnts[iPolPnt].pp_idPnt);
        fprintf(_fpOutput, "      { %d; %g, %g, %g; %g, %g, %g; }\n", iPolPnt, 
          _avPnts[iPnt][0], _avPnts[iPnt][1], -_avPnts[iPnt][2], 
          _avPntNormals[iPnt][0], _avPntNormals[iPnt][1], -_avPntNormals[iPnt][2]);
      }
    }}
    fprintf(_fpOutput, "    }\n");
    fprintf(_fpOutput, "  }\n");
  }}


  fprintf(_fpOutput, "}\n\n");
  fprintf(_fpOutput, "SE_MESH_END;\n");

  _msg->info("Saved:", fnmOut);

    // close and free everything
    if (_fpOutput!=NULL) {
      fclose(_fpOutput);
      _fpOutput=NULL;
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
    if (strFileName!=NULL) {
      free(strFileName);
      strFileName = NULL;
    }
    if (_astrUVMapNames!=NULL) {
      free(_astrUVMapNames);
      _astrUVMapNames = NULL;
    }
    if (_astrWeightMapNames!=NULL) {
      free(_astrWeightMapNames);
      _astrWeightMapNames = NULL;
    }
   
	
}
