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
#include <crtdbg.h>
#include <stdarg.h>
#include "vecmat.h"
#include <crtdbg.h>

#include <lwserver.h>
#include <lwmotion.h>
#include <lwxpanel.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "base.h"

static int _iFrame = 0;
static int _ctFrames = 0;
static int _ctBones = 0;
static int ctBoneEnvelopes = 0;
static int ctMorphEnvelopes = 0;

bool bRecordDefaultFrame = false;
extern int ReloadGlobalObjects();
extern bool bExportAbsPositions;  // export first bone absolute position
typedef float Matrix12[12];


void MakeRotationAndPosMatrix(Matrix12 &mrm_f, float *pmrm_vPos, float *pmrm_vRot)
{
  assert(_CrtCheckMemory());
  float mat[3][4];
  float fSinH = sinf(pmrm_vRot[0]);  // heading
  float fCosH = cosf(pmrm_vRot[0]);
  float fSinP = sinf(pmrm_vRot[1]);  // pitch
  float fCosP = cosf(pmrm_vRot[1]);
  float fSinB = sinf(pmrm_vRot[2]);  // banking
  float fCosB = cosf(pmrm_vRot[2]);

  memset(&mat,0,sizeof(mat));

  mat[0][0] = fCosH*fCosB+fSinP*fSinH*fSinB;
  mat[0][1] = fSinP*fSinH*fCosB-fCosH*fSinB;
  mat[0][2] = -(fCosP*fSinH);
  mat[1][0] = fCosP*fSinB;
  mat[1][1] = fCosP*fCosB;
  mat[1][2] = -(-fSinP);
  mat[2][0] = -(fSinP*fCosH*fSinB-fSinH*fCosB);
  mat[2][1] = -(fSinP*fCosH*fCosB+fSinH*fSinB);
  mat[2][2] = fCosP*fCosH;

  // add Position
  mat[0][3] = pmrm_vPos[0];
  mat[1][3] = pmrm_vPos[1];
  mat[2][3] = pmrm_vPos[2];
  memcpy(mrm_f,&mat,sizeof(mat));
}


void MakeIdentityMatrix(Matrix12 &mat)
{
  memset(&mat,0,sizeof(mat));
  mat[0] = 1;
  mat[5] = 1;
  mat[10] = 1;
  //mat[11] = 1;
}


static void MatrixTranspose(Matrix12 &r, const Matrix12 &m)
{
  r[ 0] = m[ 0];
  r[ 5] = m[ 5];
  r[10] = m[10];
  r[ 3] = m[ 3];
  r[ 7] = m[ 7];
  r[11] = m[11];

  r[1] = m[4];
  r[2] = m[8];
  r[4] = m[1];
  r[8] = m[2];
  r[6] = m[9];
  r[9] = m[6];

  r[ 3] = -r[0]*m[3] - r[1]*m[7] - r[ 2]*m[11];
  r[ 7] = -r[4]*m[3] - r[5]*m[7] - r[ 6]*m[11];
  r[11] = -r[8]*m[3] - r[9]*m[7] - r[10]*m[11];
}


// concatenate two 3x4 matrices [conc = MxN]
void MatrixMultiply(Matrix12 &c,const Matrix12 &m, const Matrix12 &n)
{
  c[0] = m[0]*n[0] + m[1]*n[4] + m[2]*n[8];
  c[1] = m[0]*n[1] + m[1]*n[5] + m[2]*n[9];
  c[2] = m[0]*n[2] + m[1]*n[6] + m[2]*n[10];
  c[3] = m[0]*n[3] + m[1]*n[7] + m[2]*n[11] + m[3];

  c[4] = m[4]*n[0] + m[5]*n[4] + m[6]*n[8];
  c[5] = m[4]*n[1] + m[5]*n[5] + m[6]*n[9];
  c[6] = m[4]*n[2] + m[5]*n[6] + m[6]*n[10];
  c[7] = m[4]*n[3] + m[5]*n[7] + m[6]*n[11] + m[7];

  c[8] = m[8]*n[0] + m[9]*n[4] + m[10]*n[8];
  c[9] = m[8]*n[1] + m[9]*n[5] + m[10]*n[9];
  c[10] = m[8]*n[2] + m[9]*n[6] + m[10]*n[10];
  c[11] = m[8]*n[3] + m[9]*n[7] + m[10]*n[11] + m[11];
}


// matrix copy
void MatrixCopy(Matrix12 &c, const Matrix12 &m)
{
  memcpy(&c,&m,sizeof(c));
}


void PrintMatrix(FILE *_f, Matrix12 &mat, int ctSpaces)
{
  char str_Spaces[16];
  memset(&str_Spaces,32,sizeof(str_Spaces));
  str_Spaces[15] = 0;
  str_Spaces[ctSpaces] = 0;


  fprintf(_f,"%s",str_Spaces);
  fprintf(_f,"%g,%g,%g,%g,\t"   ,mat[0],mat[1],mat[2],mat[3]);
  fprintf(_f,"%g,%g,%g,%g,\t"   ,mat[4],mat[5],mat[6],mat[7]);
  fprintf(_f,"%g,%g,%g,%g;" ,mat[8],mat[9],mat[10],mat[11]);
}



void MakeRotationMatrix(Matrix12 &mRotation,float *afAngles)
{
  float fSinH = (float) sin(afAngles[0]);  // heading
  float fCosH = (float) cos(afAngles[0]);
  float fSinP = (float) sin(afAngles[1]);  // pitch
  float fCosP = (float) cos(afAngles[1]);
  float fSinB = (float) sin(afAngles[2]);  // banking
  float fCosB = (float) cos(afAngles[2]);

  mRotation[0]  = fCosH*fCosB+fSinP*fSinH*fSinB;
  mRotation[1]  = fSinP*fSinH*fCosB-fCosH*fSinB;
  mRotation[2]  = fCosP*fSinH;
  mRotation[3]  = 0;
  mRotation[4]  = fCosP*fSinB;
  mRotation[5]  = fCosP*fCosB;
  mRotation[6]  = -fSinP;
  mRotation[7]  = 0;
  mRotation[8]  = fSinP*fCosH*fSinB-fSinH*fCosB;
  mRotation[9]  = fSinP*fCosH*fCosB+fSinH*fSinB;
  mRotation[10] = fCosP*fCosH;
  mRotation[11]  = 0;

  for (int i=0;i<12;i++) if(fabs(mRotation[i]) < 0.001) mRotation[i] = 0;
}



/*
 * Decompose rotation matrix into angles in 3D.
 */
// NOTE: for derivation of the algorithm, see mathlib.doc
void DecomposeRotationMatrixNoSnap(const Matrix12 &mRotation,float *afAngles)
{
  float &h=afAngles[0];  // heading
  float &p=afAngles[1];  // pitch
  float &b=afAngles[2];  // banking
  float a;            // temporary

  // calculate pitch
  float f23 = mRotation[6];
  p = (float) asin(-f23);
  a = (float) sqrt(1.0f-f23*f23);

  // if pitch makes banking beeing the same as heading
  if (a<0.001) {
    // we choose to have banking of 0
    b = 0;
    // and calculate heading for that
    assert(fabs(mRotation[6])>0.5); // must be around 1, what is far from 0
    h = (float) atan2(mRotation[1]/(-mRotation[6]), mRotation[0]);  // no division by 0
  // otherwise
  } else {
    // calculate banking and heading normally
    b = (float) atan2(mRotation[4], mRotation[5]);
    h = (float) atan2(mRotation[2], mRotation[10]);
  }
}



void MatchGoalOrientation(LWItemID objectID,float *frot,double time) 
{
  LWItemID goalID,parentID;
  double rot[3]; 
  Matrix12 mGoal,mBone,mTemp,mMul;
  int bGoalOrient;

  goalID = _iti->goal(objectID);

  // the rotation of this bone must be the same as the rotation of it's goal object.
  // first get the absolute rotation of the goal object
  _iti->param(goalID,LWIP_ROTATION,time,rot);
  frot[0] = (float) rot[0];
  frot[1] = (float) rot[1];
  frot[2] = (float) rot[2];
  MakeRotationMatrix(mGoal,frot);

  parentID = _iti->parent(goalID);
  while (parentID != LWITEM_NULL) {
    _iti->param(parentID,LWIP_ROTATION,time,rot);
    frot[0] = (float) rot[0];
    frot[1] = (float) rot[1];
    frot[2] = (float) rot[2];
    MakeRotationMatrix(mTemp,frot);
    MatrixMultiply(mMul,mTemp,mGoal);
    MatrixCopy(mGoal,mMul);
    parentID = _iti->parent(parentID);
  }

  
  // now get the absolute rotation of this bone
  MakeIdentityMatrix(mBone);

  parentID = _iti->parent(objectID);
  while (parentID != LWITEM_NULL) {
    bGoalOrient	= _iti->flags(parentID) & LWITEMF_GOAL_ORIENT;
    if (bGoalOrient) {
      MatchGoalOrientation(parentID,frot,time);
    } else {
      _iti->param(parentID,LWIP_ROTATION,time,rot);
      frot[0] = (float) rot[0];
      frot[1] = (float) rot[1];
      frot[2] = (float) rot[2];
    }
    MakeRotationMatrix(mTemp,frot);
    MatrixMultiply(mMul,mTemp,mBone);
    MatrixCopy(mBone,mMul);
    parentID = _iti->parent(parentID);  
  }   

  MatrixTranspose(mTemp,mBone);
  MatrixMultiply(mMul,mTemp,mGoal);


  DecomposeRotationMatrixNoSnap(mMul,frot);
};



bool ExecCmd(const char *strFormat, ...)
{
  char strCommand[256];
  va_list arg;
  va_start(arg, strFormat);
  vsprintf(strCommand, strFormat, arg);
  {int iOk = _evaluate(_serverData, strCommand);
  if (iOk==0) {
    _msg->error("Can't execute command", strCommand);
    return false;
  }}

  return true;
}

// enumeraion function used to test if the currently selected vmap is used by the current mesh
static int CheckPointVmap(void *dummy, LWPntID id)
{
  float v[3];
  if (_pmesh->pntVGet(_pmesh, id, v)) {
    return 1;
  } else {
    return 0;
  }
}

static char *_strFileName = NULL;
static const char *_strSceneName = NULL;

static FILE *_f = NULL;


static BoneInfo *_pbiFirst = NULL; // linked list of all instances
static MorphInfo *_pmiFirst = NULL; // linked list of all instances
static Matrix12 *_pmRootBoneAbs=NULL;

/*
======================================================================
Create()

Handler callback.  Allocate and initialize instance data.
====================================================================== */

XCALL_( static LWInstance )
Create( void *priv, LWItemID item, LWError *err )
{
  // create the instance
  BoneInfo *pii = (BoneInfo*)malloc(sizeof(BoneInfo));
  pii->bi_strName = _iti->name(item);
  _ctBones++;

  // get parent bone name
  LWItemID pParentID = _iti->parent(item);
  //strdup()
  pii->bi_strParentName = _iti->name(pParentID);
  if(_iti->type(pParentID) != LWI_BONE) {
    // this is root bone
    pii->bi_strParentName = "";
  }
  // get item type
  pii->bi_lwItemType = _iti->type(item);
  pii->bi_uiFlags = _pbi->flags(item);

  // allocate space for storing frames
  pii->bi_abfFrames = (BoneFrame*)malloc(sizeof(BoneFrame)*_ctFrames);
  
  // if first time here
  if(_pbiFirst==NULL)
  {
    // allocate space for storing absolute position for root bone
    _pmRootBoneAbs = (Matrix12*)malloc(sizeof(Matrix12)*_ctFrames);
    for(int ifr=0;ifr<_ctFrames;ifr++)
    {
      // reset matrices of root bone for all frames
      MakeIdentityMatrix(_pmRootBoneAbs[ifr]);
    }
  }

  // link into list
  pii->bi_pbiNext = _pbiFirst;

  _pbiFirst = pii;
  return pii;
}


/*
======================================================================
Destroy()

Handler callback.  Free resources allocated by Create().
====================================================================== */

XCALL_( static void )
Destroy( BoneInfo *inst)
{
  free(inst);
  //free(_pmRootBoneAbs);
}


/*
======================================================================
Copy()

Handler callback.  Copy instance data.
====================================================================== */

XCALL_( static LWError )
Copy( BoneInfo *to, BoneInfo *from )
{
  *to = *from;
  return NULL;
}


/*
======================================================================
Load()

Handler callback.  Read instance data.
====================================================================== */

XCALL_( static LWError )
Load( BoneInfo *inst, const LWLoadState *ls )
{
  return NULL;
}


/*
======================================================================
Save()

Handler callback.  Write instance data.
====================================================================== */

XCALL_( static LWError )
Save( BoneInfo *inst, const LWSaveState *ss )
{
   return NULL;
}


/*
======================================================================
Describe()

Handler callback.  Write a short, human-readable string describing
the instance data.
====================================================================== */

XCALL_( static const char * )
Describe( BoneInfo *inst )
{
  static char desc[ 80 ];
  sprintf( desc, "SE Motion Export Handler");
  return desc;
}


/*
======================================================================
Flags()

Handler callback.
====================================================================== */

XCALL_( static int )
Flags( BoneInfo *inst )
{
   return LWIMF_AFTERIK;
}


/*
======================================================================
Evaluate()

Handler callback.  This is where we can modify the item's motion.
====================================================================== */

XCALL_( static void )
Evaluate( BoneInfo *pii, const LWItemMotionAccess *access )
{
  double pos[3], rot[3], pivotpos[3], pivotrot[3];

  access->getParam( LWIP_POSITION, access->time, pos);
  access->getParam( LWIP_ROTATION, access->time, rot);
  //access->getParam( LWIP_PIVOT, access->time, rot2);
  
  LWItemID bone = access->item;

  int bGoalOrient	= _iti->flags(bone) & LWITEMF_GOAL_ORIENT;

  if(bRecordDefaultFrame)
  {
    // default position
    _pbi->restParam( bone, LWIP_POSITION, pos );
    _pbi->restParam( bone, LWIP_ROTATION, rot );

    pii->fRestLength = (float)_pbi->restLength(bone);

    pii->bi_abfDefault.fi_vPos[0] = (float)pos[0];
    pii->bi_abfDefault.fi_vPos[1] = (float)pos[1];
    pii->bi_abfDefault.fi_vPos[2] = (float)pos[2];

    if (fabs(rot[0]) < 0.001) rot[0] = 0;
    if (fabs(rot[1]) < 0.001) rot[1] = 0;
    if (fabs(rot[2]) < 0.001) rot[2] = 0;

    
    pii->bi_abfDefault.fi_vRot[0] = (float) rot[0];
    pii->bi_abfDefault.fi_vRot[1] = (float) rot[1];
    pii->bi_abfDefault.fi_vRot[2] = (float) rot[2];
    
    LWDVector vMin;
    LWDVector vMax;
    unsigned int uiRet;

    uiRet = _iti->limits(bone,3,vMin,vMax);
  }
  else
  {
    pii->bi_abfFrames[_iFrame].fi_vPos[0] = (float)pos[0];
    pii->bi_abfFrames[_iFrame].fi_vPos[1] = (float)pos[1];
    pii->bi_abfFrames[_iFrame].fi_vPos[2] = (float)pos[2];

    if (bGoalOrient) {
      MatchGoalOrientation(bone,pii->bi_abfFrames[_iFrame].fi_vRot,access->time);
    } else {      
      pii->bi_abfFrames[_iFrame].fi_vRot[0] = (float)rot[0];
      pii->bi_abfFrames[_iFrame].fi_vRot[1] = (float)rot[1];
      pii->bi_abfFrames[_iFrame].fi_vRot[2] = (float)rot[2];
    }


    // if this is not bone
    if((pii->bi_lwItemType!=LWI_BONE))// && (pii->bi_pbiNext!=NULL) && (pii->bi_pbiNext->bi_lwItemType == LWI_BONE))
    {
      // get pivot position
      _iti->param(bone,LWIP_PIVOT,access->time,pivotpos);
      _iti->param(bone,LWIP_PIVOT_ROT,access->time,pivotrot);
      float fPivotPos[3], fPivotRot[3];
      for(int ia=0;ia<3;ia++)
      {
        fPivotPos[ia] = (float)pivotpos[ia];
        fPivotRot[ia] = (float)pivotrot[ia];
      }
      fPivotPos[2] *=-1;
      pii->bi_abfFrames[_iFrame].fi_vPos[2] *=-1;
      
      // get object pivot matix
      Matrix12 mPivot,mPivotInvert;
      MakeRotationAndPosMatrix(mPivot,fPivotPos,fPivotRot);
      MatrixTranspose(mPivotInvert,mPivot);
      // get object matrix
      Matrix12 mTemp,mObject;
      MakeRotationAndPosMatrix(mObject,&pii->bi_abfFrames[_iFrame].fi_vPos[0],&pii->bi_abfFrames[_iFrame].fi_vRot[0]);
      MatrixCopy(mTemp,mObject);
      MatrixMultiply(mObject,mTemp,mPivotInvert);

      //MakeIdentityMatrix(mTemp);
      //MatrixCopy(mTemp,mPivot);

      // add its position and rotation to abs matrix
      //MakeIdentityMatrix(mTemp);
      MatrixCopy(mTemp,_pmRootBoneAbs[_iFrame]);
      MatrixMultiply(_pmRootBoneAbs[_iFrame],mObject,mTemp);
      pii->bi_abfFrames[_iFrame].fi_vPos[2] *=-1;
    }
    
  }
  //_RPT3(_CRT_WARN, "item: %s, frame: %d, time: %g\n", _iti->name(access->item), access->frame, access->time);
  //_RPT3(_CRT_WARN, "pos: %g, %g, %g\n", pos[0], pos[1], pos[2]);
  //_RPT3(_CRT_WARN, "rot: %g, %g, %g\n", rot[0], rot[1], rot[2]);
}


/*
======================================================================
Handler()

Handler activation function.  Check the version and fill in the
callback fields of the handler structure.
====================================================================== */

XCALL_( int )
Animation_Handler( long version, GlobalFunc *_global, LWItemMotionHandler *local,
   void *serverData)
{
  if ( version != LWITEMMOTION_VERSION ) return AFUNC_BADVERSION;

  _iti = (LWItemInfo *)_global(LWITEMINFO_GLOBAL, GFUSE_TRANSIENT);
  if (_iti==NULL) {
    return AFUNC_BADGLOBAL;
  }

  local->inst->create  = Create;
  local->inst->destroy = (void (*)(void *))Destroy;
  local->inst->load    = (const char *(__cdecl *)(void *,const struct st_LWLoadState *))Load;
  local->inst->save    = (const char *(__cdecl *)(void *,const struct st_LWSaveState *))Save;
  local->inst->copy    = (const char *(__cdecl *)(void *,void *))Copy;
  local->inst->descln  = (const char *(__cdecl *)(void *))Describe;
  local->evaluate      = (void (__cdecl *)(void *,const struct st_LWItemMotionAccess *))Evaluate;
  local->flags         = (unsigned int (__cdecl *)(void *))Flags;

  return AFUNC_OK;
}

static bool ApplyExportHander(LWItemID itemID)
{
  if (!ExecCmd("SelectItem %x", itemID)) {
    return false;
  }
  if (!ExecCmd("ApplyServer ItemMotionHandler " DEBUGEXT "internal_SEAnimExport")) {
    return false;
  }
  return true;
}

static bool ActivateExportHandler(LWItemID itemID)
{
  LWItemID boneid = _iti->first(LWI_BONE, itemID);
  // if no bones in the scene
  if (!boneid) {
    // this is not a fatal error
    return true;
  }
  // get root bone parent
  LWItemID pParentID = _iti->parent(boneid);

  // apply export handeler for all bones in scene
  while (boneid!=LWITEM_NULL) {
    if(!ApplyExportHander(boneid)) 
    {
      // failed
      return false;
    }
    boneid = _iti->next(boneid);
  }

  // add motion handler to all objects in hierarchy before skeleton if exportabspositions is turned on
  if(bExportAbsPositions)
  {
    while(pParentID != LWITEM_NULL)
    {
      // apply it only if item isn't bone
      if(_iti->type(pParentID) != LWI_BONE)
      {
        if(!ApplyExportHander(pParentID))
        {
          return false;
        }
      }
      // get parents parent
      const char *bi_strParentName = _iti->name(pParentID);
      pParentID = _iti->parent(pParentID);
    }
  }

  return true;
}

static bool RemoveExportHander(LWItemID itemID)
{
  if (!ExecCmd("SelectItem %x", itemID)) {
    return false;
  }
  for(int iServer=1;;iServer++) {
    const char *strServer = _iti->server(itemID, "ItemMotionHandler", iServer);
    if (strServer==NULL) {
      break;
    }
    if (strcmp(strServer, DEBUGEXT "internal_SEAnimExport")==0) {
      if (!ExecCmd("RemoveServer ItemMotionHandler %d", iServer)) {
        return false;
      }
    }
  }
  return true;
}

static void DeactivateExportHandler(LWItemID itemID)
{
  LWItemID boneid = _iti->first(LWI_BONE, itemID);

  // remove motion handler from all objects in hierarchy before skeleton if exportabspositions is turned on
  if(bExportAbsPositions)
  {
    LWItemID pParentID = _iti->parent(boneid);
    while(pParentID != LWITEM_NULL)
    {
      // apply it only if item isn't bone
      if(_iti->type(pParentID) != LWI_BONE)
      {
        if(!RemoveExportHander(pParentID))
        {
          return;
        }
      }
      // get parents parent
      const char *bi_strParentName = _iti->name(pParentID);
      pParentID = _iti->parent(pParentID);
    }
  }


  // remove export handeler for all bones in scene
  while (boneid!=LWITEM_NULL) {
    if(!RemoveExportHander(boneid))
    {
      return;
    }
    boneid = _iti->next(boneid);
  }
  return;
}

// find all morph channels for the current mesh
void FindMorphChannels(LWChanGroupID idParentGroup)
{
  // for each group in the given parent
  for(LWChanGroupID idGroup = _chi->nextGroup(idParentGroup, NULL); 
      idGroup!=NULL; 
      idGroup = _chi->nextGroup(idParentGroup, idGroup)) {
    const char *strGroupName = _chi->groupName(idGroup);
    if (idParentGroup==NULL && strcmp(strGroupName, _iti->name(_objid))!=0) {
      continue;
    }

    // for each channel in the group
    for(LWChannelID idChan = _chi->nextChannel(idGroup, NULL); 
        idChan!=NULL; 
        idChan = _chi->nextChannel(idGroup, idChan)) {
      // generate morhpmap name from the info about the channel and its parents
      const char *strName = _chi->channelName(idChan);
      char strMapName[256] = "";
      if (idParentGroup!=NULL) {
        strcat(strMapName, strGroupName);
        strcat(strMapName, ".");
      }
      strcat(strMapName, strName);

      // if the morphmap does not exist, skip the channel
      void *pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_MORF, strMapName);
      if (pMap==NULL) {
        pMap = _pmesh->pntVLookup(_pmesh, LWVMAP_SPOT, strMapName);
      }
      if (pMap==NULL) {
        continue;
      }
      // select that morhpmap
      _pmesh->pntVSelect(_pmesh, pMap);

      // check if any point uses that vmap
      int iUsed = _pmesh->scanPoints(_pmesh, CheckPointVmap, NULL);
      // if not used
      if (iUsed==0) {
        // skip it
        continue;
      }

      // -- if we get here it means that the channel is really a morph map for this object
      
      // generate morph info
      MorphInfo *pmi = (MorphInfo *)malloc(sizeof(MorphInfo));
      pmi->mi_strName = strdup(strMapName);
      pmi->mi_idChannel = idChan;
      pmi->mi_pmiNext = _pmiFirst;
      _pmiFirst = pmi;
      pmi->mi_afFrames = (float *)malloc(sizeof(float)*_ctFrames);
    }

    // enum all subgroups of this group
    FindMorphChannels(idGroup);
  }

}

// create anim name (from original file name)
void GetAnimID(char *fnAnimFile)
{
  char temp[256];
  char strAnimName[256];

  strcpy(temp,fnAnimFile);
  char *pchDot = strrchr(temp, '.');
  char *pchSlash = strrchr(temp, '\\');

  int IResultS = pchSlash - temp + 1;
  int IResultD = pchDot - temp;

  if((pchDot!=NULL) && (pchSlash!=NULL))
  {
    int len = IResultD-IResultS;
    memcpy(strAnimName,&temp[IResultS],len);
    strAnimName[len] = '_';
    strAnimName[len+1] = 0;
    strcat(strAnimName,_sci->name);
    char *pchDot2 = strrchr(strAnimName, '.');
    if(pchDot2!=NULL) *pchDot2 = 0;
  }
  else
  {
    strcpy(strAnimName,fnAnimFile);
  }
  strcpy(fnAnimFile,strAnimName);
}


double GetCurrentTime()
{
  LWTimeInfo *_tmi = (LWTimeInfo *)_global( LWTIMEINFO_GLOBAL, GFUSE_TRANSIENT );
  return _tmi->time;
}

void WriteAnimFrame(BoneInfo *pbi,int iFrame)
{
  // Fill 3x4 matrix and store rotation and position in it
  Matrix12 bi_mRot;
  BoneFrame &bf = pbi->bi_abfFrames[iFrame];
  bf.fi_vPos[2] *= -1;
  MakeRotationAndPosMatrix(bi_mRot,bf.fi_vPos,bf.fi_vRot);

  // if doesent have parent (root bone)
  if(strlen(pbi->bi_strParentName) == 0) {
    // add position and rotation of parent object to root bone
    Matrix12 mTemp;
    MatrixCopy(mTemp,bi_mRot);
    MatrixMultiply(bi_mRot,_pmRootBoneAbs[iFrame],mTemp);
  }

  // write matrix to file
  PrintMatrix(_f,bi_mRot,4);
  fprintf(_f,"\n");
}

// 
int ExportAnim(LWXPanelID pan)
{
  if(!_evaluate)
  {
    // lightwave error
    _msg->error("Lightwave process error !\nClose plugins window and try again.\n", NULL);
    return AFUNC_BADAPP;
  }

  bool bDoBones = false;
  ctBoneEnvelopes = 0; 
  ctMorphEnvelopes = 0;

  ReloadGlobalObjects();
  // !!!! make it work with a selected object, not the first one in scene

  bool bExportOnlySelected = false;
  int bExportAnimBackward = *(int*)_xpanf->formGet( pan, ID_ANIM_ORDER);

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
      if(_msg->yesNo("No objects selected","Export animations for all objects?",NULL) == 0)
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

    // open the file to print into
    char fnmOut[256];
    strcpy(fnmOut, _strFileName);
    // get first slash in filename
    char *pchSlash = strrchr(fnmOut, '.');
    if (pchSlash!=NULL) {
      *(pchSlash++) = '_';
      strcpy(pchSlash,_sci->name);
      char *pchDot = strrchr(fnmOut, '.');
      if(pchDot!=NULL)
      {
        strcpy(pchDot, ".aa");
      }
    }
    _f = fopen(fnmOut, "w");
    if (_f==NULL) {
      _msg->error("Can't open file", fnmOut);
      goto end;
    }

    // calculate number of frames to export
    _ctFrames = ((_ifi->previewEnd-_ifi->previewStart)/_ifi->previewStep)+1;
    if (_ctFrames<=0) {
      _ctFrames = 1;
    }
    _iFrame = 0;

    // find all morph channels for the current mesh
    _pmiFirst = NULL;
    FindMorphChannels(NULL);

    // add internal motion handler to each bone
    if (!ActivateExportHandler(_objid)) {
      _msg->error("Cannot apply internal bone motion handler!", NULL);
    }

    bRecordDefaultFrame = true;
    if (!ExecCmd("GoToFrame 0")) {
      goto end;
    }
    bRecordDefaultFrame = false;

    {
    // for each frame in current preview selection
    for (int iFrame=_ifi->previewStart; iFrame<=_ifi->previewEnd; iFrame+=_ifi->previewStep) {
      // go to that frame
      if (!ExecCmd("GoToFrame %d", iFrame)) {
        goto end;
      }
  
      assert(_iFrame>=0 && _iFrame<_ctFrames);

      // NOTE: walking all frames implicitly lets the internal itemmotion handler record all bone positions
      // we walk the morph maps manually

      // for each morph in list
      for (MorphInfo *pmi=_pmiFirst; pmi!=NULL; pmi = pmi->mi_pmiNext) {
        // evaluate the channel value in this frame
        pmi->mi_afFrames[_iFrame] = (float)_chi->channelEvaluate(pmi->mi_idChannel, GetCurrentTime());
      }
      _iFrame++; 
    }
    }

    char strAnimID[256];
    strcpy(strAnimID,_strFileName);
    GetAnimID(strAnimID);
    // write the animation header
    {
      // LWTimeInfo *_tmi = (LWTimeInfo *)_global( LWTIMEINFO_GLOBAL, GFUSE_TRANSIENT );
      fprintf(_f, "SE_ANIM %s;\n\n",SE_ANIM_VER);
      fprintf(_f, "SEC_PER_FRAME %g;\n", GetCurrentTime() / _ifi->previewEnd * _ifi->previewStep);
      fprintf(_f, "FRAMES %d;\n", _ctFrames);
      fprintf(_f, "ANIM_ID \"%s\";\n\n", strAnimID);
    }

    // calculate bone and morph envelopes
    {
      for(BoneInfo *ptmpbi=_pbiFirst; ptmpbi!=NULL; ptmpbi = ptmpbi->bi_pbiNext)
      {
        // LWBONEF_ACTIVE =
        unsigned int uiFlags = ptmpbi->bi_uiFlags;

        // if this is bone and it is active
        if(ptmpbi->bi_lwItemType == LWI_BONE && ptmpbi->bi_uiFlags&LWBONEF_ACTIVE) {
          ctBoneEnvelopes++;
        }
      }

      for(MorphInfo *ptmpmi=_pmiFirst;ptmpmi!=NULL; ptmpmi = ptmpmi->mi_pmiNext)
        ctMorphEnvelopes++;
    }

    Matrix12 bi_mRot;
    // for each bone in list
    fprintf(_f, "BONEENVELOPES %d\n{\n", ctBoneEnvelopes);
  
    // last item
    {
    BoneInfo *pbiLast = NULL;
    for (BoneInfo *pbi=_pbiFirst; pbi!=NULL; pbi = pbi->bi_pbiNext)
    {
      bool bRootBone = false;
      if(pbi->bi_lwItemType == LWI_BONE && pbi->bi_uiFlags&LWBONEF_ACTIVE) {
        // write its info
        fprintf(_f, "  NAME \"%s\"\n", pbi->bi_strName);
        // write first frame - default pose
        fprintf(_f, "  DEFAULT_POSE {");
        BoneFrame &bfDef = pbi->bi_abfDefault;
        bfDef.fi_vPos[2] *= -1;
        MakeRotationAndPosMatrix(bi_mRot,bfDef.fi_vPos,bfDef.fi_vRot);
        PrintMatrix(_f,bi_mRot,0);

        fprintf(_f, "}\n");
        fprintf(_f, "  {\n");

        LWItemType itLast;
        if(!pbiLast) itLast = LWI_OBJECT;
        else itLast = pbiLast->bi_lwItemType;
        // is this root bone
        if(pbi->bi_lwItemType == LWI_BONE && itLast != LWI_BONE)
        {
          // mark as root bone
          bRootBone = true;
        }

        // if export anims backward
        if(bExportAnimBackward) {
          // for each frame
          for (int iFrame=_ctFrames-1; iFrame>=0; iFrame--) {
            // write anim
            WriteAnimFrame(pbi,iFrame);
          }
        // else export normal order
        } else {
          // for each frame
          for (int iFrame=0; iFrame<_ctFrames; iFrame++) {
            // write anim
            WriteAnimFrame(pbi,iFrame);
          }
        }
        fprintf(_f,"  }\n\n");
        pbiLast = pbi;
      }
    }
    }
    fprintf(_f,"}\n");

    fprintf(_f, "\nMORPHENVELOPES %d\n{\n", ctMorphEnvelopes);

    // for each morph in list
    {for (MorphInfo *pmi=_pmiFirst; pmi!=NULL; pmi = pmi->mi_pmiNext)
    {
      // write its info
      fprintf(_f, "  NAME \"%s\"\n", pmi->mi_strName);
      fprintf(_f, "  {\n");
       // if export anims backward
      if(bExportAnimBackward) {
        for (int iFrame=_ctFrames-1; iFrame>=0; iFrame--)
        {
          fprintf(_f, "    %g;\n", pmi->mi_afFrames[iFrame]);
        }
      // if anims order is normal
      } else {
        for (int iFrame=0; iFrame<_ctFrames; iFrame++)
        {
          fprintf(_f, "    %g;\n", pmi->mi_afFrames[iFrame]);
        }
      }
      fprintf(_f,"  }\n\n");
    }
    }

    fprintf(_f,"}\n");

    // free all morph infos
    { MorphInfo *pmi=_pmiFirst;
      MorphInfo *pmiNext=NULL;
      for(;;) {
        if(pmi==NULL) {
          break;
        }
        pmiNext = pmi->mi_pmiNext;

        free(pmi->mi_strName);
        free(pmi->mi_afFrames);
        free(pmi);

        pmi = pmiNext;
    }}

    fprintf(_f, "SE_ANIM_END;\n");

    _msg->info("Saved:", fnmOut);

  end:
    // remove internal motion handler from each bone
    DeactivateExportHandler(_objid);

    // close and free everything
    if (_f!=NULL) {
      fclose(_f);
      _f=NULL;
    }
    _pbiFirst = NULL;
    // get next mesh obj
    _objid = _iti->next(_objid);
  }
  return AFUNC_OK;
}

int ExportSkeleton(void)
{
  if(!_evaluate)
  {
    // lightwave error
    _msg->error("Lightwave process error !\nClose plugins window and try again.\n", NULL);
    return AFUNC_BADAPP;
  }

  // !!!! make it work with a selected object, not the first one in scene
  ReloadGlobalObjects();

  bool bExportOnlySelected = false;
  int ctSkeletonBones=0;
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
      if(_msg->yesNo("No objects selected","Export skeletons for all objects?",NULL) == 0)
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

    // open the file to print into
    char fnmOut[256];
    strcpy(fnmOut, _strFileName);
    char *pchDot = strrchr(fnmOut, '.');
    if (pchDot!=NULL) {
      strcpy(pchDot, ".as");
    }
    _f = fopen(fnmOut, "w");
    if (_f==NULL) {
      _msg->error("Can't open file", fnmOut);
      goto end;
    }

    // set bones counter to 0
    _ctBones = 0;

    // add internal motion handler to each bone
    if (!ActivateExportHandler(_objid)) {
      _msg->error("Cannot apply internal bone motion handler!", NULL);
    }


    assert(_CrtCheckMemory());
    bRecordDefaultFrame = true;
    if (!ExecCmd("GoToFrame 0"))
    {
      goto end;
    }
    bRecordDefaultFrame = false;

    {
    for(BoneInfo *ptmpbi=_pbiFirst; ptmpbi!=NULL; ptmpbi = ptmpbi->bi_pbiNext)
    {
      if(ptmpbi->bi_lwItemType == LWI_BONE) {
        ctSkeletonBones++;
      }
    }
    }

    fprintf(_f, "SE_SKELETON %s;\n\n",SE_ANIM_VER);
    fprintf(_f, "BONES %d\n{\n",ctSkeletonBones);

    Matrix12 bi_mRot;
    {for (BoneInfo *pbi=_pbiFirst; pbi!=NULL; pbi = pbi->bi_pbiNext)
    {
      if(pbi->bi_lwItemType == LWI_BONE)
      {
        assert(_CrtCheckMemory());
        // write its info
        fprintf(_f, "  NAME \"%s\";\n", pbi->bi_strName);
        fprintf(_f, "  PARENT \"%s\";\n", pbi->bi_strParentName);
        fprintf(_f, "  LENGTH %g;\n", pbi->fRestLength);
        fprintf(_f, "  {\n");

        // write first frame - default pose
        BoneFrame &bfDef = pbi->bi_abfDefault;
        bfDef.fi_vPos[2] *= -1;
        MakeRotationAndPosMatrix(bi_mRot,bfDef.fi_vPos,bfDef.fi_vRot);
        PrintMatrix(_f,bi_mRot,4);
        fprintf(_f,"\n  }\n");
      }
    }
    }

    assert(_CrtCheckMemory());
  
    fprintf(_f, "}\n\nSE_SKELETON_END;\n");
    _msg->info("Saved:", fnmOut);


  end:
    DeactivateExportHandler(_objid);
    if (_f!=NULL)
    {
      fclose(_f);
      _f=NULL;
    }
    _pbiFirst = NULL;
    // get next mesh obj
    _objid = _iti->next(_objid);
  }
  return AFUNC_OK;
}
