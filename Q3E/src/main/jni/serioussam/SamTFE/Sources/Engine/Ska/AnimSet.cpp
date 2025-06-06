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

#include "Engine/StdH.h"
#include <Engine/Ska/AnimSet.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Base/CTString.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/Geometry.h>
#include <Engine/Base/Timer.h>

#define ANIMSET_VERSION  14
#define ANIMSET_ID       "ANIM"

// table for removed frames
static CStaticArray<BOOL> aiRemFrameTable;
// precalculated angles for rotations
static CStaticArray<ANGLE3D> aangAngles;

// if rotations are compresed does loader also fills array of uncompresed rotations
static BOOL bAllRotations = FALSE;
void RememberUnCompresedRotatations(BOOL bRemember)
{
  bAllRotations = bRemember;
}
CAnimSet::CAnimSet()
{

}
CAnimSet::~CAnimSet()
{
}
// conpres normal
static void CompressAxis(const FLOAT3D &vNormal, UWORD &ubH, UWORD &ubP)
{
  ANGLE h, p;

  const FLOAT &x = vNormal(1);
  const FLOAT &y = vNormal(2);
  const FLOAT &z = vNormal(3);

  // calculate pitch
  p = ASin(y);

  // if y is near +1 or -1
  if (y>0.99999 || y<-0.99999) {
    // heading is irrelevant
    h = 0;
  // otherwise
  } else {
    // calculate heading
    h = ATan2(-x, -z);
  }

  h = (h/360.0f)+0.5f;
  p = (p/360.0f)+0.5f;
  ASSERT(h>=0 && h<=1);
  ASSERT(p>=0 && p<=1);
  ubH = UWORD(h*65535);
  ubP = UWORD(p*65535);
}
// try to remove 2. keyframe in rotation
BOOL RemoveRotFrame(AnimRot &ar1,AnimRot &ar2,AnimRot &ar3,FLOAT fTreshold)
{
  ANGLE3D ang1,ang2,ang2i,ang3;
  FLOATmatrix3D m2i;
  // calculate slerp factor for ar2'
  FLOAT fSlerpFactor = (FLOAT)(ar2.ar_iFrameNum - ar1.ar_iFrameNum)/(FLOAT)(ar3.ar_iFrameNum - ar1.ar_iFrameNum);
  // calculate ar2'
  FLOATquat3D q2i = Slerp(fSlerpFactor,ar1.ar_qRot,ar3.ar_qRot);
  // read precalculated values
  ang1 = aangAngles[ar1.ar_iFrameNum];
  ang2 = aangAngles[ar2.ar_iFrameNum];
  ang3 = aangAngles[ar3.ar_iFrameNum];
  q2i.ToMatrix(m2i);
  DecomposeRotationMatrixNoSnap(ang2i,m2i);

  for(INDEX i=1;i<4;i++)
  {
    if( ((ang2(i) < ang3(i)) && (ang2(i) < ang1(i))) || ((ang2(i) > ang3(i)) && (ang2(i) > ang1(i))) )
    {
      // this is extrem
      if(Abs(ang2(i)) > 0.1f) return FALSE;
    }
    FLOAT fErr = Abs(ang2(i)-ang2i(i)) / Abs(ang3(i) - ang1(i));
    if(Abs(ang2(i)-ang2i(i)) < 0.1f) continue;
    if(fErr>fTreshold) return FALSE;
  }
  return TRUE;
}
// try to remove 2. keyrame in translation
BOOL RemovePosFrame(AnimPos &ap1,AnimPos &ap2,AnimPos &ap3,FLOAT fTreshold)
{
  FLOAT fLerpFactor = (FLOAT)(ap2.ap_iFrameNum - ap1.ap_iFrameNum)/(FLOAT)(ap3.ap_iFrameNum - ap1.ap_iFrameNum);
  FLOAT3D v2i = Lerp(ap1.ap_vPos,ap3.ap_vPos,fLerpFactor);

  FLOAT3D v1 = ap1.ap_vPos;
  FLOAT3D v2 = ap2.ap_vPos;
  FLOAT3D v3 = ap3.ap_vPos;

  for(INDEX i=1;i<4;i++)
  {
    if( ((v2(i) < v3(i)) && (v2(i) < v1(i))) || ((v2(i) > v3(i)) && (v2(i) > v1(i))) )
    {
      // extrem
      if(Abs(v2(i)) > 0.001f) return FALSE;
    }
    FLOAT fErr = Abs(v2(i)-v2i(i)) / Abs(v3(i) - v1(i));
    if(Abs(v2(i)-v2i(i)) < 0.001f) continue;
    if(fErr>fTreshold) return FALSE;
  }
  return TRUE;
}
// find next keyframe that havent been marked as removed
INDEX FindNextFrame(INDEX ifnToFind)
{
  INDEX ctfn = aiRemFrameTable.Count();
  if(ifnToFind >= ctfn) return -1;
  if(aiRemFrameTable[ifnToFind] == FALSE) return ifnToFind;
  for(INDEX ifn=ifnToFind;ifn<ctfn;ifn++)
  {
    if(aiRemFrameTable[ifn] == FALSE) return ifn;
  }
  return -1;
}
// optimize all animations
void CAnimSet::Optimize()
{
  INDEX ctan=as_Anims.Count();
  for(INDEX ian=0;ian<ctan;ian++)
  {
    Animation &an = as_Anims[ian];
    //CalculateExtraSpins(an);
    OptimizeAnimation(an,an.an_fTreshold);
  }
}
// optimize animation
void CAnimSet::OptimizeAnimation(Animation &an, FLOAT fTreshold)
{
  INDEX ctfn = an.an_iFrames;
  INDEX ctbe = an.an_abeBones.Count();
  aiRemFrameTable.Clear();
  aiRemFrameTable.New(ctfn);
  aangAngles.Clear();
  aangAngles.New(ctfn);

  for(INDEX ibe=0;ibe<ctbe;ibe++)
  {
    BoneEnvelope &be = an.an_abeBones[ibe];
    // calculate length on bone in default pos
    be.be_OffSetLen = (FLOAT3D(be.be_mDefaultPos[3],be.be_mDefaultPos[7],be.be_mDefaultPos[11])).Length();
    // create a table for removed frames
    memset(&aiRemFrameTable[0],0,sizeof(BOOL)*ctfn);
    memset(&aangAngles[0],0,sizeof(ANGLE3D)*ctfn);
    // fill array of decomposed matrices
    FLOATmatrix3D mat;
    for(INDEX im=0;im<ctfn;im++)
    {
      be.be_arRot[im].ar_qRot.ToMatrix(mat);
      DecomposeRotationMatrixNoSnap(aangAngles[im],mat);
    }
    // try to remove rotations, stepping by 2
    INDEX iloop;
    for(iloop=0;iloop<ctfn;iloop++)
    {
      INDEX ctRemoved=0;
      // for each frame in bone envelope
      for(INDEX ifn=0;ifn<ctfn;ifn+=2)
      {
        INDEX iInd1 = FindNextFrame(ifn);
        INDEX iInd2 = FindNextFrame(iInd1+1);
        INDEX iInd3 = FindNextFrame(iInd2+1);
        // !!!! try only ind3
        if((iInd1 < 0)||(iInd2 < 0)||(iInd3 < 0)) break;

        AnimRot *parCurent = &be.be_arRot[iInd1];
        AnimRot *parNext   = &be.be_arRot[iInd2];
        AnimRot *parLast   = &be.be_arRot[iInd3];
        if(RemoveRotFrame(*parCurent,*parNext,*parLast,fTreshold))
        {
          aiRemFrameTable[iInd2] = TRUE;
          ctRemoved++;
        }
      }
      if(ctRemoved==0)
      {
        // exit if no keyframe has been removed
        break;
      }
    }
    // create temp array for rotations that are not removed
    CStaticStackArray<struct AnimRot> arRot;
    // for each removed frame
    for(INDEX ifnr=0;ifnr<ctfn;ifnr++)
    {
      // if frame is not in table for removed frames add it to temp arRot array
      if(!aiRemFrameTable[ifnr])
      {
        AnimRot &ar = arRot.Push();
        ar = be.be_arRot[ifnr];
      }
    }
    // count frames that are left
    INDEX ctfl = arRot.Count();
    // create new array for bone envelope
    be.be_arRot.Clear();
    be.be_arRot.New(ctfl);
    // copy array of rotaions
    INDEX fl;
    for(fl=0;fl<ctfl;fl++)
    {
      be.be_arRot[fl] = arRot[fl];
    }
    arRot.Clear();
    
    // do same thing for positions
    // clear table for removed frames
    memset(&aiRemFrameTable[0],0,sizeof(BOOL)*ctfn);
    // try to remove translations stepping by 2
    for(iloop=0;iloop<ctfn;iloop++)
    {
      INDEX ctRemoved=0;
      for(INDEX ifn=0;ifn<ctfn;ifn+=2)
      {
        INDEX iInd1 = FindNextFrame(ifn);
        INDEX iInd2 = FindNextFrame(iInd1+1);
        INDEX iInd3 = FindNextFrame(iInd2+1);
        // !!!! try only ind3
        if((iInd1 < 0)||(iInd2 < 0)||(iInd3 < 0)) break;

        AnimPos *papCurent = &be.be_apPos[iInd1];
        AnimPos *papNext   = &be.be_apPos[iInd2];
        AnimPos *papLast   = &be.be_apPos[iInd3];
        if(RemovePosFrame(*papCurent,*papNext,*papLast,fTreshold))
        {
          aiRemFrameTable[iInd2] = TRUE;
          ctRemoved++;
        }
      }
      if(ctRemoved==0)
      {
        // exit if no keyframe has been removed
        break;
      }
    }
    CStaticStackArray<struct AnimPos> apPos;
    // count removed frames
    for(INDEX ifr=0;ifr<ctfn;ifr++)
    {
      if(!aiRemFrameTable[ifr])
      {
        AnimPos &ap = apPos.Push();
        ap = be.be_apPos[ifr];
      }
    }
    // count frames that are left
    ctfl = apPos.Count();
    // create new array for bone envelope
    be.be_apPos.Clear();
    be.be_apPos.New(ctfl);
    // copy array of translations
    for(fl=0;fl<ctfl;fl++)
    {
      be.be_apPos[fl] = apPos[fl];
    }
    apPos.Clear();
  }
  aiRemFrameTable.Clear();

  // if morph envelope has all factors 0 remove it
  CStaticStackArray<struct MorphEnvelope> aMorphs;

  INDEX ctme = an.an_ameMorphs.Count();
  for(INDEX ime=0;ime<ctme;ime++)
  {
    MorphEnvelope &me = an.an_ameMorphs[ime];
    // morph factors count
    INDEX ctwm=me.me_aFactors.Count();
    // index of wertex morph
    INDEX iwm=0;
    BOOL bMorphIsZero = TRUE;
    while(bMorphIsZero)
    {
      if(iwm>=ctwm) break;
      FLOAT &fMorphFactor = me.me_aFactors[iwm];
      // check if morph factor is 0
      bMorphIsZero = fMorphFactor == 0;
      iwm++;
    }
    // dont remove this morph envelope
    if(!bMorphIsZero)
    {
      // copy this morphmap to temp array of morph envelopes
      MorphEnvelope &meNew = aMorphs.Push();
      meNew = me;
    }
  }
  INDEX ctmeNew = aMorphs.Count();
  // crate new array for morph envelopes
  an.an_ameMorphs.Clear();
  an.an_ameMorphs.New(ctmeNew);
  // copy morph back to animations array of morph envelopes
  for(INDEX imeNew=0;imeNew<ctmeNew;imeNew++)
  {
    an.an_ameMorphs[imeNew] = aMorphs[imeNew];
  }
}
// add animation to animset
void CAnimSet::AddAnimation(Animation *pan)
{
  INDEX ctan = as_Anims.Count();
  as_Anims.Expand(ctan+1);
  Animation &an = as_Anims[ctan];
  an = *pan;
}
// remove animation from animset
void CAnimSet::RemoveAnimation(Animation *pan)
{
  INDEX ctan = as_Anims.Count();
  ASSERT(ctan>0);
  ASSERT(pan!=NULL);
  
  // copy all animations to temp array
  CStaticArray<struct Animation> animsTemp;
  animsTemp.New(ctan-1);
  INDEX ianNew=0;
  for(INDEX ian=0;ian<ctan;ian++)
  {
    Animation *panTemp = &as_Anims[ian];
    if(panTemp != pan)
    {
      // copy anims
      animsTemp[ianNew] = *panTemp;
      ianNew++;
    }
  }
  as_Anims = animsTemp;  
}
// write to stream
void CAnimSet::Write_t(CTStream *ostrFile)
{
  // write id
  ostrFile->WriteID_t(CChunkID(ANIMSET_ID));
  // write version
  (*ostrFile)<<(INDEX)ANIMSET_VERSION;

  INDEX ctan = as_Anims.Count();
  (*ostrFile)<<ctan;

  for(int ian=0;ian<ctan;ian++)
  {
    Animation &an = as_Anims[ian];
    CTString pstrNameID = ska_GetStringFromTable(an.an_iID);
    // write anim source file
    (*ostrFile)<<an.an_fnSourceFile;
    // write anim id
    (*ostrFile)<<pstrNameID;
    // write secperframe
    (*ostrFile)<<an.an_fSecPerFrame;
    // write num of frames
    (*ostrFile)<<an.an_iFrames;
    // write treshold
    (*ostrFile)<<an.an_fTreshold;
    // write if compresion is used
    (*ostrFile)<<an.an_bCompresed;
    // write bool if animation uses custom speed
    (*ostrFile)<<an.an_bCustomSpeed;
    
    INDEX ctbe = an.an_abeBones.Count();
    INDEX ctme = an.an_ameMorphs.Count();

    // write bone envelopes count
    (*ostrFile)<<ctbe;
    // for each bone envelope
    for(int ibe=0;ibe<ctbe;ibe++)
    {
      BoneEnvelope &be = an.an_abeBones[ibe];
      CTString pstrNameID = ska_GetStringFromTable(be.be_iBoneID);
      // write bone envelope ID
      (*ostrFile)<<pstrNameID;
      // write default pos(matrix12)
      ostrFile->Write_t(&be.be_mDefaultPos[0],sizeof(FLOAT)*12);
      // count positions
      INDEX ctp = be.be_apPos.Count();
      // write position count
      (*ostrFile)<<ctp;
      // for each position
      for(INDEX ip=0;ip<ctp;ip++)
      {
        // write position
        ostrFile->Write_t(&be.be_apPos[ip],sizeof(AnimPos));
      }
      // count rotations
      INDEX ctRotations = be.be_arRot.Count();
      (*ostrFile)<<ctRotations;
      // for each rotation
      for(INDEX ir=0;ir<ctRotations;ir++)
      {
        // write rotation
        AnimRot &arRot = be.be_arRot[ir];
        ostrFile->Write_t(&arRot,sizeof(AnimRot));
      }
      INDEX ctOptRotations = be.be_arRotOpt.Count();
      if(ctOptRotations>0)
      {
        // OPTIMISED ROTATIONS ARE NOT SAVED !!!
        // use RememberUnCompresedRotatations();
        ASSERT(ctRotations>=ctOptRotations);
      }
      // write offsetlen
      (*ostrFile)<<be.be_OffSetLen;
    }

    // write morph envelopes
    (*ostrFile)<<ctme;
    for(int ime=0;ime<ctme;ime++)
    {
      MorphEnvelope &me = an.an_ameMorphs[ime];
      CTString pstrNameID = ska_GetStringFromTable(me.me_iMorphMapID);
      // write morph map ID
      (*ostrFile)<<pstrNameID;
      // write morph factors count
      INDEX ctmf = me.me_aFactors.Count();
      (*ostrFile)<<ctmf;
      ostrFile->Write_t(&me.me_aFactors[0],sizeof(FLOAT)*ctmf);
    }
  }
}
// read from stream
void CAnimSet::Read_t(CTStream *istrFile)
{
  INDEX iFileVersion;
  // read chunk id
  istrFile->ExpectID_t(CChunkID(ANIMSET_ID));
  // check file version
  (*istrFile)>>iFileVersion;
  if(iFileVersion != ANIMSET_VERSION)
  {
		ThrowF_t(TRANS("File '%s'.\nInvalid animset file version. Expected Ver \"%d\" but found \"%d\"\n"),
      (const char*)istrFile->GetDescription(),ANIMSET_VERSION,iFileVersion);
  }
  INDEX ctan;
  // read anims count
  (*istrFile)>>ctan;
  // create anims array
  as_Anims.New(ctan);

  for(int ian=0;ian<ctan;ian++)
  {
    Animation &an = as_Anims[ian];
    CTString pstrNameID;
    // read anim source file
    (*istrFile)>>an.an_fnSourceFile;
    // read Anim ID
    (*istrFile>>pstrNameID);
    an.an_iID = ska_GetIDFromStringTable(pstrNameID);
    // read secperframe
    (*istrFile)>>an.an_fSecPerFrame;
    // read num of frames
    (*istrFile)>>an.an_iFrames;
    // read treshold
    (*istrFile)>>an.an_fTreshold;
    // read if compresion is used
    (*istrFile)>>an.an_bCompresed;
    // read bool if animation uses custom speed
    (*istrFile)>>an.an_bCustomSpeed;
    
    INDEX ctbe;
    INDEX ctme;
    // read bone envelopes count
    (*istrFile)>>ctbe;
    // create bone envelopes array
    an.an_abeBones.New(ctbe);
    // read bone envelopes
    for(int ibe=0;ibe<ctbe;ibe++)
    {
      BoneEnvelope &be = an.an_abeBones[ibe];
      CTString pstrNameID;
      (*istrFile)>>pstrNameID;
      // read bone envelope ID
      be.be_iBoneID = ska_GetIDFromStringTable(pstrNameID);
      // read default pos(matrix12)
      for (int i = 0; i < 12; i++)
        (*istrFile) >> be.be_mDefaultPos[i];

      INDEX ctp;
      // read pos array
      (*istrFile)>>ctp;
      be.be_apPos.New(ctp);
      for(INDEX ip=0;ip<ctp;ip++)
      {
        (*istrFile)>>be.be_apPos[ip];
      }
      INDEX ctr;
      // read rot array count
      (*istrFile)>>ctr;
      if(!an.an_bCompresed)
      {
        // create array for uncompresed rotations
        be.be_arRot.New(ctr);
      }
      else
      {
        // if flag is set to remember uncompresed rotations
        if(bAllRotations)
        {
          // create array for uncompresed rotations
          be.be_arRot.New(ctr);
        }
        // create array for compresed rotations
        be.be_arRotOpt.New(ctr);
      }
      for(INDEX ir=0;ir<ctr;ir++)
      {
        AnimRot arRot;// = be.be_arRot[ir];
        (*istrFile) >> arRot;
        if(!an.an_bCompresed)
        {
          be.be_arRot[ir] = arRot;
        }
        else
        {
          if(bAllRotations)
          {
            // fill uncompresed rotations
            be.be_arRot[ir] = arRot;
          }
          // optimize quaternions
          FLOAT3D vAxis;
          ANGLE aAngle;
          UWORD ubH,ubP;
          FLOATquat3D &qRot = arRot.ar_qRot;
          AnimRotOpt &aroRot = be.be_arRotOpt[ir];
          qRot.ToAxisAngle(vAxis,aAngle);
          CompressAxis(vAxis,ubH,ubP);

          // compress angle
          aroRot.aro_aAngle = aAngle * ANG_COMPRESIONMUL;
          aroRot.aro_iFrameNum = arRot.ar_iFrameNum;
          aroRot.aro_ubH = ubH;
          aroRot.aro_ubP = ubP;
          be.be_arRotOpt[ir] = aroRot;
        }
      }
      // read offsetlen
      (*istrFile)>>be.be_OffSetLen;
    }

    // read morph envelopes
    (*istrFile)>>ctme;
    // create morph envelopes array
    an.an_ameMorphs.New(ctme);
    // read morph envelopes
    for(int ime=0;ime<ctme;ime++)
    {
      MorphEnvelope &me = an.an_ameMorphs[ime];
      CTString pstrNameID;
      // read morph envelope ID
      (*istrFile)>>pstrNameID;
      me.me_iMorphMapID = ska_GetIDFromStringTable(pstrNameID);
      INDEX ctmf;
      // read morph factors count
      (*istrFile)>>ctmf;
      // create morph factors array
      me.me_aFactors.New(ctmf);
      // read morph factors
      for (INDEX i = 0; i < ctmf; i++)
        (*istrFile) >> me.me_aFactors[i];
    }
  }
}
// clear animset
void CAnimSet::Clear(void)
{
  INDEX ctAnims = as_Anims.Count();
  for(INDEX iAnims=0;iAnims<ctAnims;iAnims++)
  {
    Animation &an = as_Anims[iAnims];
    INDEX ctBoneEnv = an.an_abeBones.Count();
    INDEX ctMorphEnv = an.an_ameMorphs.Count();
    for(INDEX iBoneEnv=0;iBoneEnv<ctBoneEnv;iBoneEnv++)
    {
      BoneEnvelope &be = an.an_abeBones[iBoneEnv];
      //be.be_aqvPlacement.Clear();
      be.be_apPos.Clear();
      be.be_arRot.Clear();
    }
    for(INDEX iMorphEnv=0;iMorphEnv<ctMorphEnv;iMorphEnv++)
    {
      MorphEnvelope &me = an.an_ameMorphs[iMorphEnv];
      me.me_aFactors.Clear();
    }
    an.an_abeBones.Clear();
    an.an_ameMorphs.Clear();
  }
  as_Anims.Clear();
}

// Count used memory
SLONG CAnimSet::GetUsedMemory(void)
{
  SLONG slMemoryUsed = sizeof(*this);
  INDEX ctAnims = as_Anims.Count();
  for(INDEX ias=0;ias<ctAnims;ias++) {
    Animation &an = as_Anims[ias];
    slMemoryUsed+=sizeof(an);
    // for each bone envelope
    INDEX ctbe = an.an_abeBones.Count();
    for(INDEX ibe=0;ibe<ctbe;ibe++) {
      BoneEnvelope &be = an.an_abeBones[ibe];
      slMemoryUsed+=sizeof(be);
      slMemoryUsed+=be.be_apPos.Count() * sizeof(AnimPos);
      slMemoryUsed+=be.be_arRot.Count() * sizeof(AnimRot);
      slMemoryUsed+=be.be_arRotOpt.Count() * sizeof(AnimRotOpt);
    }
    // for each morph envelope
    INDEX ctme = an.an_ameMorphs.Count();
    for(INDEX ime=0;ime<ctme;ime++) {
      MorphEnvelope &me = an.an_ameMorphs[ime];
      slMemoryUsed+=sizeof(me);
      slMemoryUsed+=sizeof(FLOAT) * me.me_aFactors.Count() + 12;
    }
  }
  return slMemoryUsed;
}
