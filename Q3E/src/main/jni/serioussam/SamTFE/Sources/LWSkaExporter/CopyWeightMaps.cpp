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


#define LAYER_BACKGROUND	1
#define LAYER_FOREGROUND	2


// weightmapnames
extern const char **_astrWeightMapNames;
extern int _ctWeightMapNames;
extern int _ctUsedWeightMapNames;

extern EDStateRef _state;

// points
int _ctPointsInBackground;
EDPointInfo *_ppiPoint;

int _ctGlobalCounter;


struct BackgroundPointInfo {
	double  position[3];
	int			*aiWmapIndices;
	float		*aiWmapWeights;
	int			iNumWmaps;
};
BackgroundPointInfo  *_aPointsInBackground;




void ListWeightMaps() {

	// list all the weightmaps used 
  _ctWeightMapNames = _objfunc->numVMaps(LWVMAP_WGHT);
  _astrWeightMapNames = (const char**) malloc(_ctWeightMapNames*sizeof(char*));
  memset(_astrWeightMapNames, 0, _ctWeightMapNames*sizeof(char*));
  _ctUsedWeightMapNames = 0;

  for(int iWeightMap=0; iWeightMap<_ctWeightMapNames; iWeightMap++) {
    const char *strName = _objfunc->vmapName(LWVMAP_WGHT, iWeightMap);
      _astrWeightMapNames[iWeightMap] = strName;
   }
}



EDError ScanBackgroundPoints (void *userdata, const EDPointInfo *ppiPointInfo) {
	static float fwmapval;
								
	_aPointsInBackground[_ctGlobalCounter].position[0] =  ppiPointInfo->position[0];
	_aPointsInBackground[_ctGlobalCounter].position[1] =  ppiPointInfo->position[1];
	_aPointsInBackground[_ctGlobalCounter].position[2] =  ppiPointInfo->position[2];

	_aPointsInBackground[_ctGlobalCounter].aiWmapIndices = new int[_ctWeightMapNames];
	_aPointsInBackground[_ctGlobalCounter].aiWmapWeights = new float[_ctWeightMapNames];
  _aPointsInBackground[_ctGlobalCounter].iNumWmaps = 0;

	for (int iwmap=0;iwmap<_ctWeightMapNames;iwmap++) {
		_meshEditOperations->pointVSet(_state,NULL,LWVMAP_WGHT,_astrWeightMapNames[iwmap]);
		if (_meshEditOperations->pointVGet(_state,ppiPointInfo->pnt,&fwmapval)) {
			_aPointsInBackground[_ctGlobalCounter].aiWmapIndices[_aPointsInBackground[_ctGlobalCounter].iNumWmaps] = iwmap;		
			_aPointsInBackground[_ctGlobalCounter].aiWmapWeights[_aPointsInBackground[_ctGlobalCounter].iNumWmaps] = fwmapval;
			_aPointsInBackground[_ctGlobalCounter].iNumWmaps++;
		}

	}

	_ctGlobalCounter++;

	return EDERR_NONE;
};


void ScanBackground() {
	_ctPointsInBackground = _meshEditOperations->pointCount(_state,OPLYR_BG,EDCOUNT_ALL);

	_aPointsInBackground = new BackgroundPointInfo [_ctPointsInBackground];
	_ctGlobalCounter = 0;

	_meshEditOperations->pointScan(_state,ScanBackgroundPoints,NULL,OPLYR_BG);
}


void FreeMem() {
	if (_aPointsInBackground != NULL) {
		for (int i=0;i<_ctPointsInBackground;i++) {
			if (_aPointsInBackground[i].aiWmapIndices != NULL) {
				delete []_aPointsInBackground[i].aiWmapIndices;
			}
			if (_aPointsInBackground[i].aiWmapWeights != NULL) {
				delete []_aPointsInBackground[i].aiWmapWeights;
			}
		}
		delete []_aPointsInBackground;
	}
	if (_astrWeightMapNames != NULL) {
		delete []_astrWeightMapNames;
	}
}


EDError CopyWeightMaps (void *userdata, const EDPointInfo *ppiPointInfo) 
{

	for (int ctpnt=0;ctpnt<_ctPointsInBackground;ctpnt++) {
		if ((fabs(ppiPointInfo->position[0] - _aPointsInBackground[ctpnt].position[0]) < 0.001f) &&
				(fabs(ppiPointInfo->position[1] - _aPointsInBackground[ctpnt].position[1]) < 0.001f) &&
				(fabs(ppiPointInfo->position[2] - _aPointsInBackground[ctpnt].position[2]) < 0.001f)) {
			for (int iwmaps=0;iwmaps < _aPointsInBackground[ctpnt].iNumWmaps;iwmaps++) {
				_meshEditOperations->pntVMap(_state,ppiPointInfo->pnt,LWVMAP_WGHT,_astrWeightMapNames[_aPointsInBackground[ctpnt].aiWmapIndices[iwmaps]],1,&_aPointsInBackground[ctpnt].aiWmapWeights[iwmaps]);
			}
		}
	}

	return EDERR_NONE;
};

