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

#include "EntitiesMP/StdH/StdH.h"

#define ID_EMITER_VER "EMT0"

CEmittedParticle::CEmittedParticle(void)
{
  ep_tmEmitted=-1;
  ep_tmLife=1.0f;
  ep_colColor=C_WHITE|CT_OPAQUE;
  ep_vSpeed=FLOAT3D(0,0,0);
  ep_vPos=FLOAT3D(0,0,0);
  ep_fStretch=1;
  ep_fRot=0;
  ep_fRotSpeed=0;
}

void CEmittedParticle::Write_t( CTStream &strm)
{
  strm<<ep_vLastPos;
  strm<<ep_vPos;
  strm<<ep_fLastRot;
  strm<<ep_fRot;
  strm<<ep_fRotSpeed;
  strm<<ep_vSpeed;
  strm<<ep_colLastColor;
  strm<<ep_colColor;
  strm<<ep_tmEmitted;
  strm<<ep_tmLife;
  strm<<ep_fStretch;
}

void CEmittedParticle::Read_t( CTStream &strm)
{
  strm>>ep_vLastPos;
  strm>>ep_vPos;
  strm>>ep_fLastRot;
  strm>>ep_fRot;
  strm>>ep_fRotSpeed;
  strm>>ep_vSpeed;
  strm>>ep_colLastColor;
  strm>>ep_colColor;
  strm>>ep_tmEmitted;
  strm>>ep_tmLife;
  strm>>ep_fStretch;
}

CEmiter::CEmiter(void)
{
  em_etType=ET_AIR_ELEMENTAL;
  em_bInitialized=FALSE;
  em_tmStart=-1;
  em_tmLife=0;
  em_vG=FLOAT3D(0,1,0);
  em_colGlobal=C_WHITE|CT_OPAQUE;
  em_iGlobal=0;
  em_aepParticles.Clear();
}

void CEmiter::Initialize(CEntity *pen)
{
  em_vG=GetGravity(pen);
  em_bInitialized=TRUE;
  em_tmStart=_pTimer->CurrentTick();
  em_iGlobal=0;
}

FLOAT3D CEmiter::GetGravity(CEntity *pen)
{
  if(pen->GetPhysicsFlags()&EPF_MOVABLE)
  {
    return ((CMovableEntity *)pen)->en_vGravityDir*
           ((CMovableEntity *)pen)->en_fGravityA;
  }
  return FLOAT3D(0,-10.0f,0);
}

void CEmiter::AddParticle(FLOAT3D vPos, FLOAT3D vSpeed, FLOAT fRot, FLOAT fRotSpeed, 
                          FLOAT tmBirth, FLOAT tmLife, FLOAT fStretch, COLOR colColor)
{
  CEmittedParticle &em=em_aepParticles.Push();
  em.ep_fLastRot=fRot;
  em.ep_fRot=fRot;
  em.ep_fRotSpeed=fRotSpeed;
  em.ep_vLastPos=vPos;
  em.ep_vPos=vPos;
  em.ep_vSpeed=vSpeed;
  em.ep_colLastColor=colColor;
  em.ep_colColor=colColor;
  em.ep_tmEmitted=tmBirth;
  em.ep_tmLife=tmLife;
  em.ep_fStretch=fStretch;
}

void CEmiter::AnimateParticles(void)
{
  FLOAT tmNow=_pTimer->CurrentTick();
  INDEX ctCount=em_aepParticles.Count();
  INDEX iCurrent=0;
  while( iCurrent<ctCount)
  {
    CEmittedParticle &ep=em_aepParticles[iCurrent];
    // not yet alive
    if(ep.ep_tmEmitted<0)
    {
      iCurrent++;
    }
    // if shouldn't live any more
    else if( tmNow>ep.ep_tmEmitted+ep.ep_tmLife)
    {
      CEmittedParticle &emLast=em_aepParticles[ctCount-1];
      ep=emLast;
      ctCount--;
    }
    // it is alive, animate it
    else
    {
      // animate position
      ep.ep_vLastPos=ep.ep_vPos;
      ep.ep_vSpeed=ep.ep_vSpeed+em_vG*_pTimer->TickQuantum;
      ep.ep_vPos=ep.ep_vPos+ep.ep_vSpeed*_pTimer->TickQuantum;
      // animate rotation
      ep.ep_fLastRot=ep.ep_fRot;
      ep.ep_fRot+=ep.ep_fRotSpeed*_pTimer->TickQuantum;
      // animate color
      //FLOAT fRatio=CalculateRatio(tmNow, ep.ep_tmEmitted, ep.ep_tmEmitted+ep.ep_tmLife, 1, 0);
      ep.ep_colLastColor=ep.ep_colColor;
      iCurrent++;
    }
  }
  if( em_aepParticles.Count()==0)
  {
    em_aepParticles.PopAll();
  }
  else if( em_aepParticles.Count()!=ctCount)
  {
    em_aepParticles.PopUntil(ctCount-1);
  }
}

void CEmiter::RenderParticles(void)
{
  switch(em_etType)
  {
  case ET_AIR_ELEMENTAL:
    Particles_AirElementalBlow(*(CEmiter*)this);
    break;
  case ET_SUMMONER_STAFF:
    Particles_SummonerStaff(*(CEmiter*)this);
    break;
  case ET_FIREWORKS01:
    Particles_Fireworks01(*(CEmiter*)this);
    break;
  default:
    {
    }
  }
}

void CEmiter::Read_t( CTStream &strm)
{
  if (strm.PeekID_t()!=CChunkID(ID_EMITER_VER)) return;
  strm.GetID_t();
  INDEX ctMaxParticles;
  strm>>ctMaxParticles;
  
  em_bInitialized=TRUE;
  INDEX ietType;
  strm>>ietType;
  em_etType=(CEmiterType) ietType;
  strm>>em_tmStart;
  strm>>em_tmLife;
  strm>>em_vG;
  strm>>em_colGlobal;
  strm>>em_iGlobal;

  if(ctMaxParticles==0) return;
  em_aepParticles.Push(ctMaxParticles);

  for(INDEX i=0; i<em_aepParticles.Count(); i++)
  {
    CEmittedParticle &em=em_aepParticles[i];
    em.Read_t(strm);
  }
}

void CEmiter::Write_t( CTStream &strm)
{
  if( !em_bInitialized) return;
  INDEX ctMaxParticles=em_aepParticles.Count();
  strm.WriteID_t(CChunkID(ID_EMITER_VER));
  strm<<ctMaxParticles;

  strm<<INDEX(em_etType);
  strm<<em_tmStart;
  strm<<em_tmLife;
  strm<<em_vG;
  strm<<em_colGlobal;
  strm<<em_iGlobal;

  for(INDEX i=0; i<em_aepParticles.Count(); i++)
  {
    CEmittedParticle &em=em_aepParticles[i];
    em.Write_t(strm);
  }
}

