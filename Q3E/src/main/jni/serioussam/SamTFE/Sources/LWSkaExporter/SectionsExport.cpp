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
static LWItemID idMasterObjID = 0;

extern bool bRecordDefaultFrame;
extern int ReloadGlobalObjects();
extern bool bExportAbsPositions;  // export first bone absolute position
typedef float Matrix12[12];



static int _ctNumBones;
static FILE *_f = NULL;
static char *_strFileName = NULL;

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
  pii->bi_strName = strdup(_iti->name(item));  

  _ctBones++;

  // get parent bone name
  LWItemID pParentID = _iti->parent(item);
  //strdup()
  pii->bi_strParentName = strdup(_iti->name(pParentID));
  if(pParentID == LWITEM_NULL) {
    // this is root bone
    pii->bi_strParentName = "";
  }
  // get item type
  pii->bi_lwItemType = _iti->type(item);

  // allocate space for storing frames
  pii->bi_abfFrames = (BoneFrame*)malloc(sizeof(BoneFrame)*_ctFrames);
  
  // if first time here
/*  if(_pbiFirst==NULL)
  {
    // allocate space for storing absolute position for root bone
    _pmRootBoneAbs = (Matrix12*)malloc(sizeof(Matrix12)*_ctFrames);
    for(int ifr=0;ifr<_ctFrames;ifr++)
    {
      // reset matrices of root bone for all frames
      MakeIdentityMatrix(_pmRootBoneAbs[ifr]);
      int a=0;
    }
  }
*/
  // link into list
  pii->bi_pbiNext = _pbiFirst;

  _pbiFirst = pii;
  return pii;
};


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
  sprintf( desc, "SE Sections Motion Export Handler");
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
  LWItemID parentID,parentparentID;
  double pos[3], rot[3];
 	double dvParentPos[3] = {0,0,0};
	double dvParentRot[3] = {0,0,0};


  access->getParam( LWIP_POSITION, access->time, pos);
  access->getParam( LWIP_ROTATION, access->time, rot);
  parentID = _iti->parent(access->item);


  LWItemID bone = access->item;

	if (access->item != idMasterObjID) {
		parentID = _iti->parent(bone);
  
    _pmesh = _obi->meshInfo(parentID, 0);
    if (_pmesh != NULL) {
      parentparentID = _iti->parent(parentID);

      _pmesh = _obi->meshInfo(parentparentID, 0);
      if (_pmesh == NULL) {
        _iti->param(parentparentID,LWIP_POSITION,0,dvParentPos);     
      } else {
			  _iti->param(parentID,LWIP_POSITION,0,dvParentPos);
      }
    }  else {
        parentparentID = _iti->parent(parentID);

        _pmesh = _obi->meshInfo(parentparentID, 0);
        if (_pmesh == NULL) {
          _iti->param(parentID,LWIP_POSITION,0,dvParentPos);        
        }
      }
	}
  
  

  pii->bi_abfFrames[_iFrame].fi_vPos[0] = (float)(pos[0] - dvParentPos[0]);
  pii->bi_abfFrames[_iFrame].fi_vPos[1] = (float)(pos[1] - dvParentPos[1]);
  pii->bi_abfFrames[_iFrame].fi_vPos[2] = (float)(pos[2] - dvParentPos[2]);
  pii->bi_abfFrames[_iFrame].fi_vRot[0] = (float)(rot[0] - dvParentRot[0]);
  pii->bi_abfFrames[_iFrame].fi_vRot[1] = (float)(rot[1] - dvParentRot[1]);
  pii->bi_abfFrames[_iFrame].fi_vRot[2] = (float)(rot[2] - dvParentRot[2]);

  /*// get pivot position
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
  MatrixMultiply(_pmRootBoneAbs[_iFrame],mObject,mTemp);*/
  pii->bi_abfFrames[_iFrame].fi_vPos[2] *=-1;

};













XCALL_( int )
SectionAnimation_Handler( long version, GlobalFunc *_global, LWItemMotionHandler *local,
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






void AddBoneToCount(LWItemID objectid) 
{
	LWItemID childID;

	childID = _iti->firstChild(objectid);
	if(_iti->type(objectid) == LWI_OBJECT)
  {
    _pmesh = _obi->meshInfo(objectid, 0);
    if((_pmesh == NULL) && (childID == LWITEM_NULL))
    {
			return;
		}

		_ctNumBones++;

		while (childID != LWITEM_NULL) {
			AddBoneToCount(childID);
			childID = _iti->nextChild(objectid,childID);
		}

  }

};



void WriteBoneInfo(LWItemID objectid) 
{
	LWItemID childID,parentID,parentparentID;
	double dvPos[3],dvRot[3];
	double dvParentPos[3] = {0,0,0};
	double dvParentRot[3] = {0,0,0};
	float  fvPos[3],fvRot[3];
	Matrix12 bi_mRot;
	const char *strName, *strParentName;
	float fLength = 0.5;


	childID = _iti->firstChild(objectid);
	if(_iti->type(objectid) == LWI_OBJECT)
  {
		
    _pmesh = _obi->meshInfo(objectid, 0);
    if((_pmesh == NULL) && (childID == LWITEM_NULL))
    {
			return;
		}


		_iti->param(objectid,LWIP_POSITION,0,dvPos);
		_iti->param(objectid,LWIP_ROTATION,0,dvRot);

		strName = _iti->name(objectid);
		fprintf(_f, "  NAME \"%s\";\n", strName);
		if (_ctNumBones > 0) {

			parentID = _iti->parent(objectid);
  
      _pmesh = _obi->meshInfo(parentID, 0);
      if (_pmesh != NULL) {
        parentparentID = _iti->parent(parentID);

        _pmesh = _obi->meshInfo(parentparentID, 0);
        if (_pmesh == NULL) {
          _iti->param(parentparentID,LWIP_POSITION,0,dvParentPos);
			    //_iti->param(parentparentID,LWIP_ROTATION,0,dvParentRot);
          
        } else {
			    _iti->param(parentID,LWIP_POSITION,0,dvParentPos);
			    //_iti->param(parentID,LWIP_ROTATION,0,dvParentRot);
        }
      } else {
        parentparentID = _iti->parent(parentID);

        _pmesh = _obi->meshInfo(parentparentID, 0);
        if (_pmesh == NULL) {
          _iti->param(parentID,LWIP_POSITION,0,dvParentPos);
			   // _iti->param(parentID,LWIP_ROTATION,0,dvParentRot);
        
        }
      }


			strParentName =  _iti->name(parentID);			
		} else {
			strParentName = strdup("");
		}

    	
		fprintf(_f, "  PARENT \"%s\";\n", strParentName);
    fprintf(_f, "  LENGTH %g;\n", fLength);
    fprintf(_f, "  {\n");

		for (int i=0;i<3;i++) {
			fvPos[i] = (float) (dvPos[i] - dvParentPos[i]);
			fvRot[i] = 0;//(float) (dvRot[i] - dvParentRot[i]);
		}
		fvPos[2] = -fvPos[2];

		MakeRotationAndPosMatrix(bi_mRot,fvPos,fvRot);
    PrintMatrix(_f,bi_mRot,4);
    fprintf(_f,"\n  }\n");

		_ctNumBones++;

		 while (childID != LWITEM_NULL) {
		 	 WriteBoneInfo(childID);
		 	 childID = _iti->nextChild(objectid,childID);
		 }

  }

};



bool AddMotionHandler(LWItemID objectid) 
{
	LWItemID childID;

	childID = _iti->firstChild(objectid);
	if(_iti->type(objectid) == LWI_OBJECT)
  {
    _pmesh = _obi->meshInfo(objectid, 0);
    if((_pmesh == NULL) && (childID == LWITEM_NULL))
    {
			return false;
		}

		if (!ExecCmd("SelectItem %x", objectid)) {
			_msg->error("ERROR: Item selection\n", NULL);	
			return false;
		}
		if (!ExecCmd("ApplyServer ItemMotionHandler " DEBUGEXT "internal_SESectionAnimExport")) {
			_msg->error("ERROR: Applying ItemMotionHandler\n", NULL);	
			return false;
		}

		while (childID != LWITEM_NULL) {
			AddMotionHandler(childID);
			childID = _iti->nextChild(objectid,childID);
		}

  }

	return true;

};



int RemoveMotionHandler(LWItemID objectid) 
{
	LWItemID childID;

	childID = _iti->firstChild(objectid);
	if(_iti->type(objectid) == LWI_OBJECT)
  {
    _pmesh = _obi->meshInfo(objectid, 0);
    if((_pmesh == NULL) && (childID == LWITEM_NULL))
    {
			return false;
		}

	  if (!ExecCmd("SelectItem %x", objectid)) {
			_msg->error("ERROR: Item selection\n", NULL);	
			return false;
		}
		for(int iServer=1;;iServer++) {
			const char *strServer = _iti->server(objectid, "ItemMotionHandler", iServer);
			if (strServer==NULL) {
				break;
			}
			if (strcmp(strServer, DEBUGEXT "internal_SESectionAnimExport")==0) {
				if (!ExecCmd("RemoveServer ItemMotionHandler %d", iServer)) {
					_msg->error("ERROR: Applying ItemMotionHandler\n", NULL);	
					return false;
				}
			}
		}

		while (childID != LWITEM_NULL) {
			RemoveMotionHandler(childID);
			childID = _iti->nextChild(objectid,childID);
		}
  }

	return true;

};






int ExportBones() 
{
	LWItemID idMasterObjID;
	char strMessage[256];

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

  // find selected object - to be replaced with the master bone
  int ctSelected = 0;
  int ctMeshes=0;
  _objid = _iti->first(LWI_OBJECT,0);
	idMasterObjID = LWITEM_NULL;
  while(_objid != LWITEM_NULL)
  {
    if(_iti->type(_objid) == LWI_OBJECT)
    {
        if(_ifi->itemFlags(_objid) & LWITEMF_SELECTED)
        {
          ctSelected++;
					idMasterObjID = _objid;
        }
      ctMeshes++;
    }
    _objid = _iti->next(_objid);
  }

	if (idMasterObjID == LWITEM_NULL) 
	{
    // lightwave error
    _msg->error("ERROR: Object for top level bone not selected.\n", NULL);
    return AFUNC_BADAPP;
  }

	if (ctSelected > 1) 
	{
    // lightwave error
    _msg->error("ERROR: More than one object selected.\n", NULL);
    return AFUNC_BADAPP;
  }
		
	_ctNumBones = 0;
	AddBoneToCount(idMasterObjID);

	sprintf(strMessage,"Number of bones: %d",_ctNumBones);


  // get scene name
  _strFileName = strdup(_sci->filename);

  // open the file to print into
  char fnmOut[256];
  strcpy(fnmOut, _strFileName);
  char *pchDot = strrchr(fnmOut, '.');
  if (pchDot!=NULL) {
    strcpy(pchDot, ".as");
  }

  _msg->info(strMessage,fnmOut);

	if ((_f = fopen(fnmOut,"w")) == NULL) {
		_msg->error("ERROR: File open.\n", NULL);
    return AFUNC_BADAPP;
	}

	fprintf(_f, "SE_SKELETON %s;\n\n",SE_ANIM_VER);
  fprintf(_f, "BONES %d\n{\n",_ctNumBones);

	_ctNumBones = 0;
	WriteBoneInfo(idMasterObjID);

	fprintf(_f, "}\n\nSE_SKELETON_END;\n");
	fclose(_f);


	return AFUNC_OK;
};






int ExportSecAnim(LWXPanelID pan) 
{

	LWItemID idMasterObjID;
	char strMessage[256];
	Matrix12 bi_mRot;


  _iFrame = 0;
  _ctFrames = 0;
  _ctBones = 0;
  ctBoneEnvelopes = 0;
  ctMorphEnvelopes = 0;

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

  // find selected object - to be replaced with the master bone
  int ctSelected = 0;
  int ctMeshes=0;
  _objid = _iti->first(LWI_OBJECT,0);
	idMasterObjID = LWITEM_NULL;
  while(_objid != LWITEM_NULL)
  {
    if(_iti->type(_objid) == LWI_OBJECT)
    {
        if(_ifi->itemFlags(_objid) & LWITEMF_SELECTED)
        {
          ctSelected++;
					idMasterObjID = _objid;
        }
      ctMeshes++;
    }
    _objid = _iti->next(_objid);
  }

	if (idMasterObjID == LWITEM_NULL) 
	{
    // lightwave error
    _msg->error("ERROR: Object for top level bone not selected.\n", NULL);
    return AFUNC_BADAPP;
  }

	if (ctSelected > 1) 
	{
    // lightwave error
    _msg->error("ERROR: More than one object selected.\n", NULL);
    return AFUNC_BADAPP;
  }
		
  // get scene name
  _strFileName = strdup(_sci->filename);

  // open the file to print into
  char fnmOut[256];
  strcpy(fnmOut, _strFileName);
  char *pchDot = strrchr(fnmOut, '.');
  if (pchDot!=NULL) {
    strcpy(pchDot, ".aa");
  }
	_ctNumBones = 0;
	AddBoneToCount(idMasterObjID);


	if ((_f = fopen(fnmOut,"w")) == NULL) {
		_msg->error("ERROR: File open.\n", NULL);
    return AFUNC_BADAPP;
	}
	
// calculate number of frames to export
  _ctFrames = ((_ifi->previewEnd-_ifi->previewStart)/_ifi->previewStep)+1;
  if (_ctFrames<=0) {
    _ctFrames = 1;
  }


  // find all morph channels for the current mesh
  _pmiFirst = NULL;
  FindMorphChannels(NULL);


	AddMotionHandler(idMasterObjID);

  sprintf(strMessage,"ctBoneEnvelopes: %d",_ctBones);
	_msg->error(strMessage,fnmOut);


  bRecordDefaultFrame = true;
  if (!ExecCmd("GoToFrame 0"))
  {
//    goto end;
  }
  bRecordDefaultFrame = false;


  int bExportAnimBackward = *(int*)_xpanf->formGet( pan, ID_ANIM_ORDER);

  float fTime;
  // export normal order
  if(!bExportAnimBackward) {
    // for each frame in current preview selection
    for (int iFrame=_ifi->previewStart; iFrame<=_ifi->previewEnd; iFrame+=_ifi->previewStep) {
      // go to that frame
      if (!ExecCmd("GoToFrame %d", iFrame)) {
//        goto end;
      }
  
      assert(_iFrame>=0 && _iFrame<_ctFrames);

      // NOTE: walking all frames implicitly lets the internal itemmotion handler record all bone positions
      // we walk the morph maps manually

      _iFrame++;
    }
    // get time
    fTime = (float) GetCurrentTime();
  // export backward
  } else {
    // remember time in last frame
    if (!ExecCmd("GoToFrame %d", _ifi->previewEnd)) {
      //goto end;
      return AFUNC_BADGLOBAL;
    }
    // get time
    fTime = (float) GetCurrentTime();
    // for each frame in current preview selection going from last to first
    for (int iFrame=_ifi->previewEnd; iFrame>=_ifi->previewStart; iFrame-=_ifi->previewStep) {
      // go to that frame
      if (!ExecCmd("GoToFrame %d", iFrame)) {
        //goto end;
        return AFUNC_BADGLOBAL;
      }
  
      assert(_iFrame>=0 && _iFrame<_ctFrames);

      LWTimeInfo *_tmi = (LWTimeInfo *)_global( LWTIMEINFO_GLOBAL, GFUSE_TRANSIENT );

      // NOTE: walking all frames implicitly lets the internal itemmotion handler record all bone positions
      // we walk the morph maps manually

      _iFrame++;
    }
  }


  // find the number of morph envelopes

  for(MorphInfo *ptmpmi=_pmiFirst;ptmpmi!=NULL; ptmpmi = ptmpmi->mi_pmiNext)
    ctMorphEnvelopes++;



   fTime = (float) GetCurrentTime();
	char strAnimID[256];
	strcpy(strAnimID,_strFileName);	
	GetAnimID(strAnimID);
	
  fprintf(_f, "SE_ANIM %s;\n\n",SE_ANIM_VER);
  fprintf(_f, "SEC_PER_FRAME %g;\n",fTime / _ifi->previewEnd * _ifi->previewStep);
  fprintf(_f, "FRAMES %d;\n", _ctFrames);
  fprintf(_f, "ANIM_ID \"%s\";\n\n", strAnimID);


	fprintf(_f, "BONEENVELOPES %d\n{\n", _ctNumBones);





  BoneInfo *pbiLast = NULL;
  for (BoneInfo *pbi=_pbiFirst; pbi!=NULL; pbi = pbi->bi_pbiNext)
  {
    bool bRootBone = false;
    
    // write its info
    fprintf(_f, "  NAME \"%s\"\n", pbi->bi_strName);
    // write first frame - default pose
    fprintf(_f, "  DEFAULT_POSE {");
    BoneFrame &bfDef = pbi->bi_abfFrames[0];
    MakeRotationAndPosMatrix(bi_mRot,bfDef.fi_vPos,bfDef.fi_vRot);
    PrintMatrix(_f,bi_mRot,0);

    fprintf(_f, "}\n");
    fprintf(_f, "  {\n");

    LWItemType itLast;
    if(!pbiLast) itLast = LWI_OBJECT;
    else itLast = pbiLast->bi_lwItemType;


    // write anim
    // for each frame
    for (int iFrame=0; iFrame<_ctFrames; iFrame++)
    {
      // Fill 3x4 matrix and store rotation and position in it
      BoneFrame &bf = pbi->bi_abfFrames[iFrame];
      MakeRotationAndPosMatrix(bi_mRot,bf.fi_vPos,bf.fi_vRot);

      // write matrix to file
      PrintMatrix(_f,bi_mRot,4);
      fprintf(_f,"\n");
      
    }
    pbiLast = pbi;
    fprintf(_f,"  }\n\n");
  }

  fprintf(_f,"}\n");
	
  fprintf(_f, "\nMORPHENVELOPES %d\n{\n", ctMorphEnvelopes);

  // for each morph in list
  {for (MorphInfo *pmi=_pmiFirst; pmi!=NULL; pmi = pmi->mi_pmiNext)
  {
    // write its info
    fprintf(_f, "  NAME \"%s\"\n", pmi->mi_strName);
    fprintf(_f, "  {\n");
    for (int iFrame=0; iFrame<_ctFrames; iFrame++)
    {
      fprintf(_f, "    %g;\n", pmi->mi_afFrames[iFrame]);
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
	fclose(_f);

	RemoveMotionHandler(idMasterObjID);
  _pbiFirst = NULL;

	return AFUNC_OK;

};
