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

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <crtdbg.h>
#include <stdarg.h>

#include <lwsurf.h>
#include <lwhost.h>
#include <lwserver.h>
#include <lwgeneric.h>
#include <lwmonitor.h>
#include <lwrender.h>
#include <lwio.h>
#include <lwdyna.h>
#include <lwpanel.h>
#include <lwshader.h>
#include <lwmotion.h>
#include <lwxpanel.h>
#include <lwmaster.h>
#include <lwserver.h>
#include <lwmeshedt.h>
#include <lwcmdseq.h>

#include "vecmat.h"
		
#ifndef NDEBUG
#define DEBUGEXT "Debug:"
#else	
#define DEBUGEXT ""
#endif
			

#define SE_ANIM_VER "0.1"

#define ML_HALF_FACE_FORWARD (1UL<<0)  // front face forward
#define ML_FULL_FACE_FORWARD (1UL<<1)  // half face fprward

struct SurfaceInstance {
  SurfaceInstance *si_psiNext;
  LWSurfaceID si_idSurface;

  char si_strShader[256];   // the shader filename

  char si_astrTextures[8][64];
  double si_adColors[8][3];
  char desc[80];
};

extern SurfaceInstance *_psiFirst;  // linked list of surface instances

struct MCData {
  int mc_iFaceForward;
  int mc_iAnimOrder;
};

extern MCData *_pmcMaster;  // the master settings for the scene


struct PolPnt {
  LWPntID pp_idPnt;
  LWPolID pp_idPol;
};


struct MorphInfo {
  char *mi_strName;
  LWChannelID mi_idChannel;
  MorphInfo *mi_pmiNext;
  float *mi_afFrames;
};

struct BoneFrame {
  float fi_vPos[3];
  float fi_vRot[3];

};

struct BoneInfo {
  const char *bi_strName;
  const char *bi_strParentName;
  BoneInfo *bi_pbiNext;
  BoneFrame *bi_abfFrames;
  BoneFrame bi_abfDefault;
  LWItemType bi_lwItemType;
  unsigned int bi_uiFlags;
  float fRestLength;
};

typedef float  Matrix12[12];
typedef double Matrixd12[12];

enum { ID_EXPORTMESH = 0x8001, ID_EXPORTSKELETON, ID_EXPORTANIM, ID_FACEFORWARD, ID_EXPORTBONES, ID_EXPORTSECANIM, ID_ANIM_ORDER, ID_ABSPOSITIONS};


extern int ExportMesh(LWXPanelID pan);
extern int ExportAnim(LWXPanelID pan);
extern int ExportSkeleton(void);

extern int ExportBones(void);
extern int ExportSecAnim(LWXPanelID pan) ;

extern void		MakeIdentityMatrix(Matrix12 &mat);
extern void		MatrixTranspose(Matrix12 &r, const Matrix12 &m);
extern void		MatrixMultiply(Matrix12 &c,const Matrix12 &m, const Matrix12 &n);
extern void		MatrixCopy(Matrix12 &c, const Matrix12 &m);
extern void		MakeRotationAndPosMatrix(Matrix12 &mrm_f, float *pmrm_vPos, float *pmrm_vRot);
extern void		PrintMatrix(FILE *_f, Matrix12 &mat, int ctSpaces);
extern double GetCurrentTime();
extern void		GetAnimID(char *fnAnimFile);
extern bool		ExecCmd(const char *strFormat, ...);
extern void FindMorphChannels(LWChanGroupID idParentGroup);

extern void ExportMesh_modeler(int iFaceForward);

extern LWSurfaceID *_asurSurfaces;

extern LWMessageFuncs *_msg;
extern LWItemInfo *_iti;
extern LWObjectFuncs *_obf;
extern LWObjectInfo *_obi;
extern LWSceneInfo *_sci;
extern LWInterfaceInfo *_ifi;
extern LWItemID _objid;
extern LWMeshInfo *_pmesh;
extern LWChannelInfo *_chi;
extern LWSurfaceFuncs *_srf;
extern LWBoneInfo *_pbi;
extern GlobalFunc *_global;
extern LWXPanelFuncs *_xpanf;
extern LWColorActivateFunc *_colorpick;
extern LWInstUpdate *_lwupdate;

extern MeshEditOp *_meshEditOperations;
extern LWObjectFuncs *_objfunc;
extern LWStateQueryFuncs *_statequery;


extern const LWMasterAccess *_local;
extern void *_serverData;
extern int (*_evaluate) (void *, const char *command);


extern EDError CopyWeightMaps (void *strSurf, const EDPointInfo *ppliPolyInfo);
extern void ListWeightMaps();
extern void ScanBackground();
extern void FreeMem();
