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
#include <Engine/Ska/ModelInstance.h>
#include <Engine/Ska/Skeleton.h>
#include <Engine/Ska/Render.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Ska/ParsingSmbs.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Math/Quaternion.h>
#include <Engine/Templates/DynamicStackArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/Stock_CMesh.h>
#include <Engine/Templates/Stock_CSkeleton.h>
#include <Engine/Templates/Stock_CAnimSet.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CShader.h>

// does parser remember smc source files?
BOOL bRememberSourceFN = FALSE;

// pointer to model instance for parser
CModelInstance *_yy_mi = NULL;

// calculate fade factor of animation list in animqueue
FLOAT CalculateFadeFactor(AnimList &alList)
{
  if(alList.al_fFadeTime==0) {
    return 1.0f;
  }

  FLOAT fFadeFactor = (_pTimer->GetLerpedCurrentTick() - alList.al_fStartTime) / alList.al_fFadeTime;
  return Clamp(fFadeFactor,0.0f,1.0f);
}
// create model instance
CModelInstance *CreateModelInstance(CTString strName)
{
  CModelInstance *pmi = new CModelInstance;
  pmi->SetName(strName);
  return pmi;
}
void DeleteModelInstance(CModelInstance *pmi)
{
  ASSERT(pmi!=NULL);
  // if model instance is valid
  if(pmi!=NULL) {
    // Clear model instance
    pmi->Clear();
    // Delete model instance
    delete pmi;
    pmi = NULL;
  }
}

// Parse smc file in existing model instance
void ParseSmcFile_t(CModelInstance &mi, const CTString &fnSmcFile)
{
  // Clear given model instance before parsing
  mi.Clear();

  CTFileName fnFileName = fnSmcFile;
  try {
    fnFileName.RemoveApplicationPath_t();
  } catch (const char *) {
  }

  CTString strIncludeFile;
  strIncludeFile.Load_t(fnFileName);

  _yy_mi = &mi;
  SMCPushBuffer(fnFileName, strIncludeFile, TRUE);
#ifdef __GNUC__
  engine_ska_yyparse();
#else
  syyparse();
#endif
}

// Create model instance and parse smc file in it
CModelInstance *ParseSmcFile_t(const CTString &fnSmcFile)
{
  _yy_mi = NULL;
  // Create new model instance for parser
  _yy_mi = CreateModelInstance("Temp");
  // Parse given smc file
  ParseSmcFile_t(*_yy_mi,fnSmcFile);

  return _yy_mi;
}

CModelInstance::CModelInstance()
{
  mi_psklSkeleton = NULL;
  mi_iParentBoneID = -1;
  mi_colModelColor = 0;
  mi_vStretch = FLOAT3D(1,1,1);
  mi_colModelColor = 0xFFFFFFFF;
  mi_aqAnims.aq_Lists.SetAllocationStep(1);
  mi_cmiChildren.SetAllocationStep(1);
  memset(&mi_qvOffset,0,sizeof(QVect));
  mi_qvOffset.qRot.q_w = 1;
  mi_iCurentBBox = -1;
  // set default all frames bbox
//  mi_cbAllFramesBBox.SetName("All Frames Bounding box");
  mi_cbAllFramesBBox.SetMin(FLOAT3D(-0.5,0,-0.5));
  mi_cbAllFramesBBox.SetMax(FLOAT3D(0.5,2,0.5));
  // Set default model instance name
//  SetName("Noname");
}

CModelInstance::~CModelInstance()
{
}
// copy constructor
CModelInstance::CModelInstance(CModelInstance &miOther)
{
  // Forbiden
  ASSERT(FALSE);
}
void CModelInstance::operator=(CModelInstance &miOther)
{
  // Forbiden
  ASSERT(FALSE);
}

void CModelInstance::GetAllFramesBBox(FLOATaabbox3D &aabbox)
{
  aabbox = FLOATaabbox3D(mi_cbAllFramesBBox.Min(),mi_cbAllFramesBBox.Max());
}

// fills curent colision box info
void CModelInstance::GetCurrentColisionBox(FLOATaabbox3D &aabbox)
{
  ColisionBox &cb = GetCurrentColisionBox();
  aabbox = FLOATaabbox3D(cb.Min(),cb.Max());
}

ColisionBox &CModelInstance::GetCurrentColisionBox()
{
  ASSERT(mi_iCurentBBox>=0);
  ASSERT(mi_iCurentBBox<mi_cbAABox.Count());
  ASSERT(mi_cbAABox.Count()>0);

  return mi_cbAABox[mi_iCurentBBox];
}


INDEX CModelInstance::GetColisionBoxIndex(INDEX iBoxID)
{
  INDEX ctcb = mi_cbAABox.Count();
  // for each existing box
  for(SLONG icb=0;icb<ctcb;icb++) {
    ColisionBox &cb = mi_cbAABox[icb];
    // if this is searched box
    if(cb.GetID() == iBoxID) {
      // return index of box
      return icb;
    }
  }
  // colision box was not found, return default (0)
  SKAASSERT(FALSE);
  return 0;
}

ColisionBox &CModelInstance::GetColisionBox(INDEX icb)
{
  ASSERT(icb>=0);
  ASSERT(icb<mi_cbAABox.Count());
  return mi_cbAABox[icb];
}

FLOAT3D CModelInstance::GetCollisionBoxMin(INDEX iCollisionBox/*=0*/)
{
  INDEX iCollisionBoxClamped = Clamp(iCollisionBox, (INDEX)0, mi_cbAABox.Count()-1);
  FLOAT3D vMin = mi_cbAABox[ iCollisionBoxClamped].Min();
  return vMin;
};

FLOAT3D CModelInstance::GetCollisionBoxMax(INDEX iCollisionBox/*=0*/)
{
  INDEX iCollisionBoxClamped = Clamp(iCollisionBox, (INDEX)0, mi_cbAABox.Count()-1);
  FLOAT3D vMax = mi_cbAABox[ iCollisionBoxClamped].Max();
  return vMax;
};

// returns HEIGHT_EQ_WIDTH, LENGHT_EQ_WIDTH or LENGHT_EQ_HEIGHT
INDEX CModelInstance::GetCollisionBoxDimensionEquality(INDEX iCollisionBox/*=0*/)
{
  // if colision box does not exists
  if(iCollisionBox>=mi_cbAABox.Count()) {
    // give last colision box
    iCollisionBox = mi_cbAABox.Count()-1;
  }
  // check if error is fixed
  ASSERT(mi_cbAABox.Count()>iCollisionBox);

  ColisionBox &cb = this->mi_cbAABox[iCollisionBox];
  FLOAT fWeigth = cb.Max()(1) - cb.Min()(1);
  FLOAT fHeight = cb.Max()(2) - cb.Min()(2);
  FLOAT fLength = cb.Max()(3) - cb.Min()(3);
  if(fLength == fHeight) {
    return SKA_LENGTH_EQ_HEIGHT;
  } else if(fHeight == fWeigth) {
    return SKA_HEIGHT_EQ_WIDTH;
  // default fLength == fWeight
  } else {
    return SKA_LENGTH_EQ_WIDTH;
  }
};

// add colision box to model instance
void CModelInstance::AddColisionBox(CTString strName,FLOAT3D vMin,FLOAT3D vMax)
{
  INDEX ctcb = mi_cbAABox.Count();
  mi_cbAABox.Expand(ctcb+1);
  
  ColisionBox &cb = mi_cbAABox[ctcb];
  cb.SetName(strName);
  cb.SetMin(vMin);
  cb.SetMax(vMax);
  mi_iCurentBBox = 0;
}
// remove colision box from model instance
void CModelInstance::RemoveColisionBox(INDEX iIndex)
{
  INDEX ctcb = mi_cbAABox.Count();
  INDEX icbNew = 0;
  CStaticArray<struct ColisionBox> aColisionBoxesTemp;
  aColisionBoxesTemp.New(ctcb-1);
  for(INDEX icb=0;icb<ctcb;icb++) {
    if(iIndex != icb) { 
      aColisionBoxesTemp[icbNew] = mi_cbAABox[icb];
      icbNew++;
    }
  }
  mi_cbAABox = aColisionBoxesTemp;
}

// add child to modelinstance
void CModelInstance::AddChild(CModelInstance *pmi, INDEX iParentBoneID /* = -1 */)
{
  SKAASSERT(pmi!=NULL);
  if(pmi==NULL) return;
  mi_cmiChildren.Add(pmi);
  if (iParentBoneID>0) {
    pmi->SetParentBone(iParentBoneID);
  }
}

// remove model instance child
void CModelInstance::RemoveChild(CModelInstance *pmi)
{
  ASSERT(pmi!=this);
  SKAASSERT(pmi!=NULL);
  // aditional check
  if(pmi==NULL) return;
  if(pmi==this) return;

  mi_cmiChildren.Remove(pmi);
}
// set new parent bone index
void CModelInstance::SetParentBone(INDEX iParentBoneID)
{
  mi_iParentBoneID = iParentBoneID;
}

// Model instance offsets from parent model
void CModelInstance::SetOffset(FLOAT fOffset[6])
{
  FLOAT3D fRot(fOffset[3],fOffset[4],fOffset[5]);
  mi_qvOffset.qRot.FromEuler(fRot);
  mi_qvOffset.vPos = FLOAT3D(fOffset[0],fOffset[1],fOffset[2]);
}

void CModelInstance::SetOffsetPos(FLOAT3D vPos)
{
  mi_qvOffset.vPos = vPos;
}

void CModelInstance::SetOffsetRot(ANGLE3D aRot)
{
  mi_qvOffset.qRot.FromEuler(aRot);
}

FLOAT3D CModelInstance::GetOffsetPos()
{
  return mi_qvOffset.vPos;
}

ANGLE3D CModelInstance::GetOffsetRot()
{
  ANGLE3D aRot;
  FLOATmatrix3D mat;
  mi_qvOffset.qRot.ToMatrix(mat);
  DecomposeRotationMatrix(aRot,mat);
  return aRot;
}

// Stretch model instance
void CModelInstance::StretchModel(const FLOAT3D &vStretch)
{
  mi_vStretch = vStretch;
}

// Stretch model instance without attachments
void CModelInstance::StretchSingleModel(const FLOAT3D &vStretch)
{
  mi_vStretch = vStretch;
  // for each child of model instance
  INDEX ctch = mi_cmiChildren.Count();
  for(INDEX ich=0;ich<ctch;ich++) {
    // set new stretch of model instance
    CModelInstance &chmi = mi_cmiChildren[ich];
    chmi.StretchSingleModel(FLOAT3D(1/vStretch(1),1/vStretch(2),1/vStretch(3)));
  }
}

// Add mesh to ModelInstance
void CModelInstance::AddMesh_t(CTFileName fnMesh)
{
  int ctMeshInst = mi_aMeshInst.Count();
  mi_aMeshInst.Expand(ctMeshInst+1);
  memset(&mi_aMeshInst[ctMeshInst],0,sizeof(mi_aMeshInst[ctMeshInst]));
  mi_aMeshInst[ctMeshInst].mi_pMesh = _pMeshStock->Obtain_t(fnMesh);
}

// Add skeleton to ModelInstance
void CModelInstance::AddSkeleton_t(CTFileName fnSkeleton)
{
  mi_psklSkeleton = _pSkeletonStock->Obtain_t(fnSkeleton);
}

// Add AnimSet to ModelInstance
void CModelInstance::AddAnimSet_t(CTFileName fnAnimSet)
{
  CAnimSet *Anim = _pAnimSetStock->Obtain_t(fnAnimSet);
  mi_aAnimSet.Add(Anim);
}

// Add texture to ModelInstance (if no mesh instance given, add texture to last mesh instance)
void CModelInstance::AddTexture_t(CTFileName fnTexture, CTString strTexID,MeshInstance *pmshi)
{
  if(pmshi == NULL) {
    INDEX ctMeshInst = mi_aMeshInst.Count();
    if(ctMeshInst<=0) throw("Error adding texture\nMesh instance does not exists");
    pmshi = &mi_aMeshInst[ctMeshInst-1];
  }

  INDEX ctTextInst = pmshi->mi_tiTextures.Count();
  pmshi->mi_tiTextures.Expand(ctTextInst+1);
  pmshi->mi_tiTextures[ctTextInst].ti_toTexture.SetData_t(fnTexture);
  pmshi->mi_tiTextures[ctTextInst].SetName(strTexID);
}

// Remove one texture from model instance
void CModelInstance::RemoveTexture(TextureInstance *ptiRemove,MeshInstance *pmshi)
{
  ASSERT(pmshi!=NULL);
  CStaticArray<struct TextureInstance> atiTextures;
  INDEX ctti=pmshi->mi_tiTextures.Count();
  atiTextures.New(ctti-1);
  // for each texture instance in mesh instance
  INDEX iIndexSrc=0;
  for(INDEX iti=0;iti<ctti;iti++)
  {
    TextureInstance *pti = &pmshi->mi_tiTextures[iti];
    // if texture instance is different from selected one 
    if(pti != ptiRemove) {
      // copy it to new array of texture isntances
      atiTextures[iIndexSrc] = pmshi->mi_tiTextures[iti];
      iIndexSrc++;
    }
  }
  // copy new texture instances array in mesh instance
  pmshi->mi_tiTextures.CopyArray(atiTextures);
  // clear temp texture isntances array
  atiTextures.Clear();
}

// Find texture instance in all mesh instances in model instance
TextureInstance *CModelInstance::FindTexureInstance(INDEX iTexID)
{
  // for each mesh instance
  INDEX ctmshi = mi_aMeshInst.Count();
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshi = mi_aMeshInst[imshi];
    // for each texture instance in meshinstance
    INDEX ctti = mshi.mi_tiTextures.Count();
    for(INDEX iti=0;iti<ctti;iti++) {
      TextureInstance &ti = mshi.mi_tiTextures[iti];
      // if this is texinstance that is beeing serched for
      if(ti.GetID() == iTexID) {
        // return it
        return &ti;
      }
    }
  }
  // texture instance wasn't found
  return NULL;
}

// Find texture instance in given mesh instance
TextureInstance *CModelInstance::FindTexureInstance(INDEX iTexID, MeshInstance &mshi)
{
  // for each texture instance in given mesh instance
  INDEX ctti = mshi.mi_tiTextures.Count();
  for(INDEX iti=0;iti<ctti;iti++) {
    TextureInstance &ti = mshi.mi_tiTextures[iti];
    // if this is texinstance that is beeing serched for
    if(ti.GetID() == iTexID) {
      // return it
      return &ti;
    }
  }
  // texture instance wasn't found
  return NULL;
}

// change parent of model instance
void CModelInstance::ChangeParent(CModelInstance *pmiOldParent, CModelInstance *pmiNewParent)
{
  SKAASSERT(pmiOldParent!=NULL);
  SKAASSERT(pmiNewParent!=NULL);

  if(pmiOldParent == NULL) {
    CPrintF("Model Instance doesn't have a parent\n");
    return;
  }
  if(pmiNewParent == NULL) {
    CPrintF("New parent of model instance is NULL\n");
    return;
  }
  pmiOldParent->mi_cmiChildren.Remove(this);
  pmiNewParent->mi_cmiChildren.Add(this);
}

// return parent of this model instance
// must suply first model instance in hierarchy cos model instance does not have its parent remembered
CModelInstance *CModelInstance::GetParent(CModelInstance *pmiStartFrom)
{
  ASSERT(pmiStartFrom!=NULL);
  // aditional check
  if(pmiStartFrom==NULL) return NULL;
  // if 'this' is member of pmiStartFrom return it
  if(pmiStartFrom->mi_cmiChildren.IsMember(this)) {
    return pmiStartFrom;
  }
  // count childrent of pmiStartFrom
  INDEX ctcmi = pmiStartFrom->mi_cmiChildren.Count();
  // for each child of pmiStartFrom
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    // if some of children have 'this' as member return them as parent
    CModelInstance *pmiReturned = GetParent(&pmiStartFrom->mi_cmiChildren[icmi]);
    if(pmiReturned != NULL) {
      return pmiReturned;
    }
  }
  return NULL;
}

// returns child with specified id
/*CModelInstance *CModelInstance::GetChild(INDEX iChildID, BOOL bRecursive)
{
  INDEX ctcmi = mi_cmiChildren.Count();
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    CModelInstance *pmi = &mi_cmiChildren[icmi];
    if(pmi->mi_iModelID == iChildID) {
      return pmi;
    }
  }
  return NULL;
}*/
CModelInstance *CModelInstance::GetChild(INDEX iChildID, BOOL bRecursive/*=FALSE*/)
{
  INDEX ctcmi = mi_cmiChildren.Count();
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    CModelInstance *pmi = &mi_cmiChildren[icmi];
    if(pmi->mi_iModelID == iChildID) {
      return pmi;
    }
    // if child has own children, go recursive
    if(bRecursive && pmi->mi_cmiChildren.Count()>0) {      
      pmi = pmi->GetChild(iChildID, TRUE);
      if (pmi!=NULL) return pmi;
    }    
  }
  return NULL;
}

// returns parent that is not included in his parents smc file
CModelInstance *CModelInstance::GetFirstNonReferencedParent(CModelInstance *pmiRoot)
{
  ASSERT(this!=NULL);
  ASSERT(pmiRoot!=NULL);
  CModelInstance *pmiParent = this->GetParent(pmiRoot);
  CModelInstance *pmiLast = this;
  while(pmiParent != NULL)
  {
    if(pmiParent->mi_fnSourceFile != mi_fnSourceFile)
    {
      return pmiLast;
    }
    pmiLast = pmiParent;
    pmiParent = pmiParent->GetParent(pmiRoot);
  }
  return NULL;//return pmiRoot
}

// add animation to ModelInstance
void CModelInstance::AddAnimation(INDEX iAnimID, ULONG ulFlags, FLOAT fStrength, INDEX iGroupID, FLOAT fSpeedMul/*=1.0f*/)
{

#ifdef SKADEBUG
// see whether this animation even exists in the current skeleton
INDEX iDummy1, iDummy2;
if (!FindAnimationByID(iAnimID, &iDummy1, &iDummy2)) { 
  /*this ModelInstance does not contain the required animation!!!*/
  SKAASSERT(FALSE);
}
#endif

  fSpeedMul = 1/fSpeedMul;
  // if no restart flag was set
  if(ulFlags&AN_NORESTART) {
    // if given animtion is allready playing
    if(IsAnimationPlaying(iAnimID)) {
      if(ulFlags&AN_LOOPING) {
        AddFlagsToPlayingAnim(iAnimID,AN_LOOPING);
      }
      // return without adding animtion
      return;
    }
  }

  // if flag for new cleared state is set
  if(ulFlags&AN_CLEAR) {
    // do new clear state with default length
    NewClearState(CLEAR_STATE_LENGTH);
  // if flag for new cloned state is set
  } else if(ulFlags&AN_CLONE) {
    // do new clear state with default length
    NewClonedState(CLONED_STATE_LENGTH);
  }

  // if anim queue is empty 
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  if(ctal == 0) {
    // add new clear state
    NewClearState(0);
  }

  ctal = mi_aqAnims.aq_Lists.Count();
  AnimList &alList = mi_aqAnims.aq_Lists[ctal-1];

  // if flag is set not to sort anims
  if(ulFlags&AN_NOGROUP_SORT) {
    // just add new animations to end of list
    PlayedAnim &plAnim = alList.al_PlayedAnims.Push();
    plAnim.pa_iAnimID = iAnimID;
    plAnim.pa_fSpeedMul = fSpeedMul;
    plAnim.pa_fStartTime = _pTimer->CurrentTick();
    plAnim.pa_Strength = fStrength;
    plAnim.pa_ulFlags = ulFlags;
    plAnim.pa_GroupID = iGroupID;
  // no flag set, sort animation by groupID
  } else {
    // add one animation to anim list
    alList.al_PlayedAnims.Push();
    INDEX ctpa = alList.al_PlayedAnims.Count();

    INDEX ipa=ctpa-1;
    if(ipa>0) {
      // for each old animation from last to first
      for(;ipa>0;ipa--) {
        PlayedAnim &pa = alList.al_PlayedAnims[ipa-1];
        PlayedAnim &paNext = alList.al_PlayedAnims[ipa];
        // if anim group id is larger than new group id
        if(pa.pa_GroupID>iGroupID) {
          // move animation in array to right
          paNext = pa;
        } else break;
      }
    }
    // set new animation as current index in anim list
    PlayedAnim &plAnim = alList.al_PlayedAnims[ipa];
    plAnim.pa_iAnimID = iAnimID;
    plAnim.pa_fSpeedMul = fSpeedMul;
    plAnim.pa_fStartTime = _pTimer->CurrentTick();
    plAnim.pa_Strength = fStrength;
    plAnim.pa_ulFlags = ulFlags;
    plAnim.pa_GroupID = iGroupID;
  }
}

// remove played anim from stack
void CModelInstance::RemAnimation(INDEX iAnimID)
{
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  // if anim queue is empty
  if(ctal < 1) {
    SKAASSERT(FALSE);
    // no anim to remove
    return;
  }

  // get last anim list in queue
  AnimList &alList = mi_aqAnims.aq_Lists[ctal-1];
  // count played anims in anim list
  INDEX ctpa = alList.al_PlayedAnims.Count();
  // loop each played anim in anim list
  for(int ipa=0;ipa<ctpa;ipa++) {
    PlayedAnim &paAnim = alList.al_PlayedAnims[ipa];
    // remove if same ID
    if(paAnim.pa_iAnimID == iAnimID) {
      // copy all latter anims over this one
      for(int ira=ipa;ira<ctpa-1;ira++) {
        alList.al_PlayedAnims[ira] = alList.al_PlayedAnims[ira+1];
      }
      // decrease played anims count
      ctpa--;
      // remove last anim
      alList.al_PlayedAnims.Pop();
    }
  }
}

// Remove all anims with GroupID
void CModelInstance::RemAnimsWithID(INDEX iGroupID)
{
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  // if anim queue is empty
  if(ctal < 1) {
    SKAASSERT(FALSE);
    // no anim to remove
    return;
  }

  // get last anim list in queue
  AnimList &alList = mi_aqAnims.aq_Lists[ctal-1];
  // count played anims in anim list
  INDEX ctpa = alList.al_PlayedAnims.Count();
  // loop each played anim in anim list
  int ipa;
  for(ipa=0;ipa<ctpa;ipa++) {
    PlayedAnim &paAnim = alList.al_PlayedAnims[ipa];
    // remove if same Group ID
    if(paAnim.pa_GroupID == iGroupID) {
      // copy all latter anims over this one
      for(int ira=ipa;ira<ctpa-1;ira++) {
        alList.al_PlayedAnims[ira] = alList.al_PlayedAnims[ira+1];
      }
      // decrease played anims count
      ctpa--;
      // remove last anim
      alList.al_PlayedAnims.Pop();
    }
  }
}

// remove unused anims from queue
void CModelInstance::RemovePassedAnimsFromQueue()
{
  // count AnimLists
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  // find newes animlist that has fully faded in
  INDEX iFirstAnimList = -1;
  // for each anim list from last to first
  INDEX ial;
  for(ial=ctal-1;ial>=0;ial--)
  {
    AnimList &alList = mi_aqAnims.aq_Lists[ial];
    // calculate fade factor for this animlist
    FLOAT fFadeFactor = CalculateFadeFactor(alList);
    // if factor is 1 remove all animlists before this one
    if(fFadeFactor >= 1.0f) {
      iFirstAnimList = ial;
      break;
    }
  }
  if(iFirstAnimList <= 0) return;
  // move later anim lists to first pos
  for(ial=iFirstAnimList;ial<ctal;ial++)
  {
    mi_aqAnims.aq_Lists[ial-iFirstAnimList] = mi_aqAnims.aq_Lists[ial];
    mi_aqAnims.aq_Lists[ial].al_PlayedAnims.PopAll();
  }
  // remove all Anim list before iFirstAnimList
  mi_aqAnims.aq_Lists.PopUntil(ctal-iFirstAnimList-1);
}

// create new state, copy last state in it and give it a fade time
void CModelInstance::NewClonedState(FLOAT fFadeTime)
{
  RemovePassedAnimsFromQueue();
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  if(ctal == 0) 
  {
  // if anim queue is empty add new clear state
    NewClearState(fFadeTime);
    ctal = 1;
  }
  // create new Anim list
  AnimList &alNewList = mi_aqAnims.aq_Lists.Push();
  alNewList.al_PlayedAnims.SetAllocationStep(1);
  AnimList &alList = mi_aqAnims.aq_Lists[ctal-1];
  // copy anims to new List
  alNewList.al_PlayedAnims = alList.al_PlayedAnims;
  alNewList.al_fFadeTime = fFadeTime;
  alNewList.al_fStartTime = _pTimer->CurrentTick();
}

// create new cleared state and give it a fade time
void CModelInstance::NewClearState(FLOAT fFadeTime)
{
  RemovePassedAnimsFromQueue();
  // add new empty list
  AnimList &alNewList = mi_aqAnims.aq_Lists.Push();
  alNewList.al_PlayedAnims.SetAllocationStep(1);
  alNewList.al_fFadeTime = fFadeTime;
  alNewList.al_fStartTime = _pTimer->CurrentTick();
  alNewList.al_PlayedAnims.PopAll();
}

// stop all animations in this model instance and its children
void CModelInstance::StopAllAnimations(FLOAT fFadeTime)
{
  INDEX ctmi = mi_cmiChildren.Count();
  for(INDEX imi=0;imi<ctmi;imi++)
  {
    CModelInstance &cmi = mi_cmiChildren[imi];
    cmi.StopAllAnimations(fFadeTime);
  }
  NewClearState(fFadeTime);
}

// Offset all animations in anim queue
void CModelInstance::OffSetAnimationQueue(TIME fOffsetTime)
{
  // for each anim list in anim queue
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  for(INDEX ial=0;ial<ctal;ial++) {
    AnimList &al = mi_aqAnims.aq_Lists[ial];
    // Modify anim list start time
    al.al_fStartTime +=fOffsetTime;
  }
}

// Find animation by ID
BOOL CModelInstance::FindAnimationByID(int iAnimID,INDEX *piAnimSetIndex,INDEX *piAnimIndex)
{
  INDEX ctas = mi_aAnimSet.Count();
  if (ctas<=0) return FALSE;
  // for each animset
  for(int ias=ctas-1;ias>=0;ias--) {
    CAnimSet &asAnimSet = mi_aAnimSet[ias];
    INDEX ctan = asAnimSet.as_Anims.Count();
    // for each animation
    for(int ian=0;ian<ctan;ian++) {
      Animation &an = asAnimSet.as_Anims[ian];
      // if this is animation to find
      if(an.an_iID == iAnimID) {
        // set pointers of indices to animset and animation
        *piAnimSetIndex = ias;
        *piAnimIndex = ian;
        // retrun succesfully
        return TRUE;
      }
    }
  }
  // animation was't found
  return FALSE;
}

// Find animation by ID
INDEX CModelInstance::FindFirstAnimationID()
{
  INDEX ctas = mi_aAnimSet.Count();
  // for each animset
  for(int ias=0; ias<ctas; ias--) {
    CAnimSet &asAnimSet = mi_aAnimSet[ias];
    INDEX ctan = asAnimSet.as_Anims.Count();
    // for each animation
    for(int ian=0;ian<ctan;ian++) {
      Animation &an = asAnimSet.as_Anims[ian];
      return an.an_iID;      
    }
  }
  // never should get here
  return -1;
}

// get animation length
FLOAT CModelInstance::GetAnimLength(INDEX iAnimID)
{
  INDEX iAnimSetIndex,iAnimIndex;
  FindAnimationByID(iAnimID,&iAnimSetIndex,&iAnimIndex);
  CAnimSet &as = mi_aAnimSet[iAnimSetIndex];
  Animation &an = as.as_Anims[iAnimIndex];
  return an.an_fSecPerFrame * an.an_iFrames;
}

// Check if given animation is currently playing
BOOL CModelInstance::IsAnimationPlaying(INDEX iAnimID)
{
  // check last anim list if animation iAnimID exists in it
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  // if there are anim lists in animqueue
  if(ctal>0) {
    // check last one
    AnimList &al = mi_aqAnims.aq_Lists[ctal-1];
    INDEX ctpa = al.al_PlayedAnims.Count();
    for(INDEX ipa=0;ipa<ctpa;ipa++) {
      PlayedAnim &pa = al.al_PlayedAnims[ipa];
      if(pa.pa_iAnimID == iAnimID) {
        // found it
        return TRUE;
      }
    }
  }
  // this animation is currently not playing
  return FALSE;
}

// Add flags to animation playing in anim queue
BOOL CModelInstance::AddFlagsToPlayingAnim(INDEX iAnimID, ULONG ulFlags)
{
  // check last anim list if animation iAnimID exists in it
  INDEX ctal = mi_aqAnims.aq_Lists.Count();
  // if there are anim lists in animqueue
  if(ctal>0) {
    // check last one
    AnimList &al = mi_aqAnims.aq_Lists[ctal-1];
    INDEX ctpa = al.al_PlayedAnims.Count();
    for(INDEX ipa=0;ipa<ctpa;ipa++) {
      PlayedAnim &pa = al.al_PlayedAnims[ipa];
      if(pa.pa_iAnimID == iAnimID) {
        pa.pa_ulFlags |= ulFlags;
        // found it
        return TRUE;
      }
    }
  }
  // this animation is currently not playing
  return FALSE;
}

// Sets name of model instance
void CModelInstance::SetName(CTString strName)
{
  mi_strName = strName;
  mi_iModelID = ska_GetIDFromStringTable(strName);
}
// Gets name of model instance
const CTString &CModelInstance::GetName()
{
  return mi_strName;
}
// Gets id of model instance
const INDEX &CModelInstance::GetID()
{
  return mi_iModelID;
}

// Get vertex positions in absolute space
void CModelInstance::GetModelVertices( CStaticStackArray<FLOAT3D> &avVertices, FLOATmatrix3D &mRotation,
                                     FLOAT3D &vPosition, FLOAT fNormalOffset, FLOAT fDistance)
{
  RM_SetObjectPlacement(mRotation,vPosition);
  RM_GetModelVertices(*this,avVertices, mRotation, vPosition, fNormalOffset, fDistance);
}

// Model color
COLOR &CModelInstance::GetModelColor(void)
{
  return mi_colModelColor;
}
void CModelInstance::SetModelColor(COLOR colNewColor)
{
  mi_colModelColor = colNewColor;
  INDEX ctch = mi_cmiChildren.Count();
  // for each child
  for(INDEX ich=0;ich<ctch;ich++) {
    CModelInstance &cmi = mi_cmiChildren[ich];
    // set child model color 
    cmi.SetModelColor(colNewColor);
  }
}
// test it the model has alpha blending
BOOL CModelInstance::HasAlpha(void)
{
  return (GetModelColor()&0xFF)!=0xFF;
}

BOOL CModelInstance::IsModelVisible( FLOAT fMipFactor)
{
  //#pragma message(">> IsModelVisible")
  return TRUE;
}

BOOL CModelInstance::HasShadow(FLOAT fMipFactor)
{
  //#pragma message(">> HasShadow")
  return TRUE;
}

// simple shadow rendering
void CModelInstance::AddSimpleShadow( const FLOAT fIntensity, const FLOATplane3D &plShadowPlane)
{

  // if shadows are not rendered for current mip, model is half/full face-forward,
  // intensitiy is too low or projection is not perspective - do nothing!
  //if( !HasShadow(1) || fIntensity<0.01f || !_aprProjection.IsPerspective()
  // || (rm.rm_pmdModelData->md_Flags&(MF_FACE_FORWARD|MF_HALF_FACE_FORWARD))) return;
  // ASSERT( _iRenderingType==1);
  ASSERT( fIntensity>0 && fIntensity<=1);
  // do some rendering
  // _pfModelProfile.StartTimer( CModelProfile::PTI_RENDERSIMPLESHADOW);
  // _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_RENDERSIMPLESHADOW);
  // _sfStats.IncrementCounter( CStatForm::SCI_MODELSHADOWS);
  // calculate projection model bounding box in object space (if needed)
  // CalculateBoundingBox( this, rm);
  // add one simple shadow to batch list
  RM_SetObjectMatrices(*this);
  RM_AddSimpleShadow_View(*this, fIntensity, plShadowPlane);
  // all done
  // _pfModelProfile.StopTimer( CModelProfile::PTI_RENDERSIMPLESHADOW);

}

// Copy mesh instance for other model instance
void CModelInstance::CopyMeshInstance(CModelInstance &miOther)
{
  INDEX ctmshi = miOther.mi_aMeshInst.Count();
  // for each mesh insntace
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshiOther = miOther.mi_aMeshInst[imshi];
    // add its mesh
    AddMesh_t(mshiOther.mi_pMesh->GetName());
    MeshInstance *pmshi = &mi_aMeshInst[imshi];

    INDEX ctti = mshiOther.mi_tiTextures.Count();
    // for each texture in mesh instance
    for(INDEX iti=0;iti<ctti;iti++) {
      TextureInstance &tiOther = mshiOther.mi_tiTextures[iti];
      CTString strTexID = ska_GetStringFromTable(tiOther.GetID());
      // CTextureData *ptd = (CTextureData*)tiOther.ti_toTexture.GetData();
      AddTexture_t(tiOther.ti_toTexture.GetName(),strTexID,pmshi);
    }
  }
}

// copy from another object of same class
void CModelInstance::Copy(CModelInstance &miOther)
{
  // clear this instance - otherwise it won't work
  Clear();

  mi_aqAnims.aq_Lists = miOther.mi_aqAnims.aq_Lists;
  mi_iCurentBBox  = miOther.mi_iCurentBBox;
  mi_colModelColor = miOther.mi_colModelColor;
  mi_iParentBoneID = miOther.mi_iParentBoneID;
  mi_qvOffset = miOther.mi_qvOffset;
  mi_strName = miOther.mi_strName;
  mi_iModelID = miOther.mi_iModelID;
  mi_cbAABox = miOther.mi_cbAABox;
  mi_fnSourceFile = miOther.mi_fnSourceFile;
  mi_vStretch = miOther.mi_vStretch;

  // copt mesh instance
  CopyMeshInstance(miOther);

  // if skeleton exists 
  if(miOther.mi_psklSkeleton!=NULL) {
    // copy skeleton
    AddSkeleton_t(miOther.mi_psklSkeleton->GetName());
  }

  // copy animsets
  INDEX ctas = miOther.mi_aAnimSet.Count();
  // for each animset
  for(INDEX ias=0;ias<ctas;ias++) {
    // add animset to this model instance
    CAnimSet &asOther = miOther.mi_aAnimSet[ias];
    AddAnimSet_t(asOther.GetName());
  }

  // copy children
  INDEX ctch = miOther.mi_cmiChildren.Count();
  // for each child in other model instance
  for(INDEX ich=0;ich<ctch;ich++) {
    CModelInstance &chmiOther = miOther.mi_cmiChildren[ich];
    CModelInstance *pchmi = CreateModelInstance("Temp");
    pchmi->Copy(chmiOther);
    AddChild(pchmi);
  }
}

// Synchronize with another model (copy animations/attachments positions etc from there)
void CModelInstance::Synchronize(CModelInstance &miOther)
{
  // Sync animations
  mi_aqAnims.aq_Lists = miOther.mi_aqAnims.aq_Lists;
  // Sync misc params
  mi_qvOffset      = miOther.mi_qvOffset;
  mi_iParentBoneID = miOther.mi_iParentBoneID;
  mi_colModelColor = miOther.mi_colModelColor;
  mi_vStretch      = miOther.mi_vStretch;

  // for each child in model instance
  INDEX ctchmi=mi_cmiChildren.Count();
  for(INDEX ichmi=0;ichmi<ctchmi;ichmi++) {
    CModelInstance &chmi = mi_cmiChildren[ichmi];
    CModelInstance *pchmiOther = miOther.GetChild(chmi.GetID(),FALSE);
    // if both model instance have this child 
    if(pchmiOther!=NULL) {
      // sync child
      chmi.Synchronize(*pchmiOther);
    }
  }
}

// clear model instance
void CModelInstance::Clear(void)
{
  // for each child of this model instance
  INDEX ctcmi = mi_cmiChildren.Count();
  for(INDEX icmi=0; icmi<ctcmi; icmi++) {
    // delete child
    CModelInstance *pcmi = &mi_cmiChildren[0];
    mi_cmiChildren.Remove(pcmi);
    DeleteModelInstance(pcmi);
  }

  // release all meshes in stock used by mi
  INDEX ctmshi = mi_aMeshInst.Count();
  for(INDEX imshi=0; imshi<ctmshi; imshi++) {
    MeshInstance &mshi = mi_aMeshInst[imshi];
    CMesh *pmesh = mshi.mi_pMesh;
    if(pmesh != NULL) {
      _pMeshStock->Release(pmesh);
    }
    // release all textures in stock used by mesh
    INDEX ctti = mshi.mi_tiTextures.Count();
    for(INDEX iti=0;iti<ctti;iti++) {
      TextureInstance &ti = mshi.mi_tiTextures[iti];
      ti.ti_toTexture.SetData(NULL);
    }
  }
  mi_aMeshInst.Clear();
  // release skeleton in stock used by mi(if it exist)
  if(mi_psklSkeleton != NULL) {
    _pSkeletonStock->Release(mi_psklSkeleton);
    mi_psklSkeleton = NULL;
  }

  // release all animsets in stock used by mi
  INDEX ctas = mi_aAnimSet.Count();
  for(INDEX ias=0;ias<ctas;ias++) {
    _pAnimSetStock->Release(&mi_aAnimSet[ias]);  
  }
  mi_aAnimSet.Clear();

  // clear all colision boxes 
  mi_cbAABox.Clear();
  // clear anim list
  mi_aqAnims.aq_Lists.Clear();
}

// Count used memory
SLONG CModelInstance::GetUsedMemory(void)
{
  SLONG slMemoryUsed = sizeof(*this);
  // Count mesh instances
  INDEX ctmshi = mi_aMeshInst.Count();
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshi = mi_aMeshInst[imshi];
    slMemoryUsed += mshi.mi_tiTextures.Count() * sizeof(TextureInstance);
  }
  slMemoryUsed += mi_aMeshInst.Count() * sizeof(MeshInstance);
  // Count bounding boxes
  slMemoryUsed += mi_cbAABox.Count() * sizeof(ColisionBox);
  // Cound child model instances
  INDEX ctcmi = mi_cmiChildren.Count();
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    CModelInstance &cmi = mi_cmiChildren[icmi];
    slMemoryUsed += cmi.GetUsedMemory();
  }
  return slMemoryUsed;
}

// set parser to remember file names for modelinstances
void CModelInstance::EnableSrcRememberFN(BOOL bEnable)
{
  bRememberSourceFN = bEnable;
}
