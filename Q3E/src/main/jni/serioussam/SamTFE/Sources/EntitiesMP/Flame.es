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

504
%{
#include "EntitiesMP/StdH/StdH.h"
#define TM_APPLY_DAMAGE_QUANTUM 0.25f
#define TM_APPLY_WHOLE_DAMAGE 7.5f
#define DAMAGE_AMMOUNT 30.0f
#define MIN_DAMAGE_QUANTUM (DAMAGE_AMMOUNT/TM_APPLY_WHOLE_DAMAGE*TM_APPLY_DAMAGE_QUANTUM)
#define MAX_DAMAGE_QUANTUM (MIN_DAMAGE_QUANTUM*10.0f)
#define DEATH_BURN_TIME 4.0f

#include "EntitiesMP/MovingBrush.h"
%}

uses "EntitiesMP/Light";

// input parameter for flame
event EFlame {
  CEntityPointer penOwner,        // entity which owns it
  CEntityPointer penAttach,       // entity on which flame is attached (his parent)
};

// event for stop burning
event EStopFlaming {
  BOOL m_bNow,
};

%{
void CFlame_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheModel(MODEL_FLAME);
  pdec->PrecacheTexture(TEXTURE_FLAME);
  pdec->PrecacheSound(SOUND_FLAME);
}
%}

class CFlame : CMovableModelEntity {
name      "Flame";
thumbnail "";
features "ImplementsOnPrecache", "CanBePredictable";

properties:
  1 CEntityPointer m_penOwner,    // entity which owns it
  2 CEntityPointer m_penAttach,   // entity on which flame is attached (his parent)
  5 BOOL m_bLoop = FALSE,                 // internal for loops
  8 FLOAT3D m_vHitPoint = FLOAT3D(0.0f, 0.0f, 0.0f), // where the flame hit the entity

 10 CSoundObject m_soEffect,      // sound channel


 20 FLOAT m_tmStart = 0.0f,
 21 FLOAT m_fDamageToApply = 0.0f,
 22 FLOAT m_fDamageStep = 0.0f,
 23 FLOAT m_fAppliedDamage=0.0f,
 24 FLOAT m_tmFirstStart = 0.0f,   // when the burning started

 29 INDEX m_ctFlames=0,
 30 FLOAT3D m_vPos01=FLOAT3D(0,0,0),
 31 FLOAT3D m_vPos02=FLOAT3D(0,0,0),
 32 FLOAT3D m_vPos03=FLOAT3D(0,0,0),
 33 FLOAT3D m_vPos04=FLOAT3D(0,0,0),
 34 FLOAT3D m_vPos05=FLOAT3D(0,0,0),
 35 FLOAT3D m_vPos06=FLOAT3D(0,0,0),
 36 FLOAT3D m_vPos07=FLOAT3D(0,0,0),
 37 FLOAT3D m_vPos08=FLOAT3D(0,0,0),
 38 FLOAT3D m_vPos09=FLOAT3D(0,0,0),
 39 FLOAT3D m_vPos10=FLOAT3D(0,0,0),
 40 FLOAT3D m_vPlaneNormal=FLOAT3D(0,0,0),
 51 BOOL m_bBurningBrush=FALSE,
 52 FLOAT m_tmDeathParticlesStart=1e6,

{
  CLightSource m_lsLightSource;

}

components:
  1 class   CLASS_LIGHT         "Classes\\Light.ecl",

// ********* FLAME *********
 10 model   MODEL_FLAME         "ModelsMP\\Effects\\Flame\\Flame.mdl",
 11 texture TEXTURE_FLAME       "ModelsMP\\Effects\\Flame\\Flame.tex",
 12 sound   SOUND_FLAME         "SoundsMP\\Fire\\Burning.wav",

functions:
  // add to prediction any entities that this entity depends on
  void AddDependentsToPrediction(void)
  {
    m_penOwner->AddToPrediction();
  }
  // postmoving
  void PostMoving(void) {
    CMovableModelEntity::PostMoving();

    // if no air
    CContentType &ctDn = GetWorld()->wo_actContentTypes[en_iDnContent];
    // stop existing
    if (!(ctDn.ct_ulFlags&CTF_BREATHABLE_LUNGS)) {
      EStopFlaming esf;
      esf.m_bNow = TRUE;
      SendEvent(esf);
    }

    // never remove from list of movers
    en_ulFlags &= ~ENF_INRENDERING;
    // not moving in fact, only moving with its parent
    en_plLastPlacement = en_plPlacement;
  };

  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CMovableModelEntity::Read_t(istr);
    SetupLightSource();
  }

  BOOL IsPointInsidePolygon(const FLOAT3D &vPos, CBrushPolygon *pbpo)
  {
    FLOATplane3D &plPlane=pbpo->bpo_pbplPlane->bpl_plAbsolute;
    // find major axes of the polygon plane
    INDEX iMajorAxis1, iMajorAxis2;
    GetMajorAxesForPlane(plPlane, iMajorAxis1, iMajorAxis2);

    // create an intersector
    CIntersector isIntersector(vPos(iMajorAxis1), vPos(iMajorAxis2));
    // for all edges in the polygon
    FOREACHINSTATICARRAY(pbpo->bpo_abpePolygonEdges, CBrushPolygonEdge, itbpePolygonEdge) {
      // get edge vertices (edge direction is irrelevant here!)
      const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
      const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
      // pass the edge to the intersector
      isIntersector.AddEdge(
        vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
        vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
    }
    // return result of polygon intersection
    return isIntersector.IsIntersecting();
  }

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  }

  // render particles
  void RenderParticles(void)
  {
    FLOAT fTimeFactor=CalculateRatio(_pTimer->CurrentTick(), m_tmFirstStart, m_tmStart+TM_APPLY_WHOLE_DAMAGE, 0.05f, 0.2f);
    FLOAT fDeathFactor=1.0f;
    if( _pTimer->CurrentTick()>m_tmDeathParticlesStart)
    {
      fDeathFactor=1.0f-Clamp((_pTimer->CurrentTick()-m_tmDeathParticlesStart)/DEATH_BURN_TIME, 0.0f, 1.0f);
    }
    CEntity *penParent= GetParent();
    FLOAT fPower=ClampUp(m_fDamageStep-MIN_DAMAGE_QUANTUM, MAX_DAMAGE_QUANTUM)/MAX_DAMAGE_QUANTUM;
    if( penParent!= NULL)
    {
      if( (penParent->en_RenderType==CEntity::RT_MODEL || penParent->en_RenderType==CEntity::RT_EDITORMODEL ||
           penParent->en_RenderType==CEntity::RT_SKAMODEL || penParent->en_RenderType==CEntity::RT_SKAEDITORMODEL) &&
          (Particle_GetViewer()!=penParent) )
      {
        Particles_Burning(penParent, fPower, fTimeFactor*fDeathFactor);
      }
      else
      {
        Particles_BrushBurning(this, &m_vPos01, m_ctFlames, m_vPlaneNormal, fPower, fTimeFactor*fDeathFactor);
      }
    }
  }

  // Setup light source
  void SetupLightSource(void)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    if(m_bBurningBrush)
    {
      UBYTE ubRndH = UBYTE( 25+(FLOAT(rand())/(float)(RAND_MAX)-0.5f)*28);
      UBYTE ubRndS = 166;
      UBYTE ubRndV = 48;
      lsNew.ls_colColor = HSVToColor(ubRndH, ubRndS, ubRndV);
      //lsNew.ls_colColor = 0x3F3F1600;
      lsNew.ls_rFallOff = 4.0f;
      lsNew.ls_rHotSpot = 0.2f;
    }
    else
    {
      lsNew.ls_colColor = 0x8F8F5000;
      lsNew.ls_rFallOff = 6.0f;
      lsNew.ls_rHotSpot = 0.50f;
    }
    lsNew.ls_plftLensFlare = NULL;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  }

/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> MAIN
  Main(EFlame ef) {
    // attach to parent (another entity)
    ASSERT(ef.penOwner!=NULL);
    ASSERT(ef.penAttach!=NULL);
    m_penOwner = ef.penOwner;
    m_penAttach = ef.penAttach;
    
    // [SSE] First Flame Fix
    m_fDamageToApply = DAMAGE_AMMOUNT;
    m_fDamageStep=m_fDamageToApply/(TM_APPLY_WHOLE_DAMAGE/TM_APPLY_DAMAGE_QUANTUM);
    //

    m_tmStart = _pTimer->CurrentTick();
    m_tmFirstStart=m_tmStart;
    SetParent(ef.penAttach);
    // initialization
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_FLYING);
    SetCollisionFlags(ECF_FLAME);
    SetFlags(GetFlags() | ENF_SEETHROUGH);

    SetModel(MODEL_FLAME);
    SetModelMainTexture(TEXTURE_FLAME);
    ModelChangeNotify();

    // play the burning sound
    m_soEffect.Set3DParameters(10.0f, 1.0f, 1.0f, 1.0f);
    PlaySound(m_soEffect, SOUND_FLAME, SOF_3D|SOF_LOOP);

    // must always be in movers, to find sector content type
    AddToMovers();

    m_bBurningBrush=FALSE;
    BOOL bAllowFlame=TRUE;
    if( !(ef.penAttach->en_RenderType==CEntity::RT_MODEL || ef.penAttach->en_RenderType==CEntity::RT_EDITORMODEL ||
          ef.penAttach->en_RenderType==CEntity::RT_SKAMODEL || ef.penAttach->en_RenderType==CEntity::RT_SKAEDITORMODEL ))
    {
      m_bBurningBrush=TRUE;
      FLOAT3D vPos=GetPlacement().pl_PositionVector;
      FLOATplane3D plPlane;
      FLOAT fDistanceToEdge;
      FindSectorsAroundEntity();
      CBrushPolygon *pbpo=NULL;
      pbpo=GetNearestPolygon(vPos, plPlane, fDistanceToEdge);
      FLOAT3D vBrushPos = ef.penAttach->GetPlacement().pl_PositionVector;
      FLOATmatrix3D mBrushRotInv = !ef.penAttach->GetRotationMatrix();
      if( pbpo!=NULL && pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity==ef.penAttach)
      {
        plPlane = pbpo->bpo_pbplPlane->bpl_plAbsolute;
        m_vPlaneNormal=(FLOAT3D &)plPlane;
        m_vPlaneNormal.Normalize();
        // ------ Calculate plane-paralel normal vectors
        FLOAT3D vU, vV;
        //CPrintF("plPlane %g\n", plPlane(2));
        if(plPlane(2)<-0.1f)
        {
          bAllowFlame=FALSE;
        }

        // if the plane is mostly horizontal
        if (Abs(plPlane(2))>0.5f) {
          // use cross product of +x axis and plane normal as +s axis
          vU = FLOAT3D(1.0f, 0.0f, 0.0f)*m_vPlaneNormal;
        // if the plane is mostly vertical
        } else {
          // use cross product of +y axis and plane normal as +s axis
          vU = FLOAT3D(0.0f, 1.0f, 0.0f)*m_vPlaneNormal;
        }
        // make +s axis normalized
        vU.Normalize();
        // use cross product of plane normal and +s axis as +t axis
        vV = vU*m_vPlaneNormal;
        vV.Normalize();

        // counter of valid flames
        m_ctFlames=0;
        for(INDEX iTest=0; iTest<20; iTest++)
        {
          FLOAT fA=FRnd()*360.0f;
          FLOAT fR=FRnd()*2.0f;
          FLOAT3D vRndV=vV*fR*SinFast(fA);
          FLOAT3D vRndU=vU*fR*CosFast(fA);
          FLOAT3D vRndPos=vPos;
          if( iTest!=0)
          {
            vRndPos+=vRndV+vRndU;
          }
          // project point to a plane
          FLOAT3D vProjectedRndPos=plPlane.ProjectPoint(vRndPos);
          if( IsPointInsidePolygon(vProjectedRndPos, pbpo))
          {
            (&m_vPos01)[ m_ctFlames]=(vProjectedRndPos-vBrushPos)*mBrushRotInv;
            m_ctFlames++;
            if( m_ctFlames==6) { break; };
          }
        }
      }
      else
      {
        bAllowFlame=FALSE;
      }
    }
    // setup light source
    if( bAllowFlame)
    {
      SetupLightSource();
    }

    m_bLoop = bAllowFlame;
    while(m_bLoop) {
      wait(TM_APPLY_DAMAGE_QUANTUM) {
        // damage to parent
        on (EBegin) : {
          // if parent does not exist anymore
          if (m_penAttach==NULL || (m_penAttach->GetFlags()&ENF_DELETED)) {
            // stop existing
            m_bLoop = FALSE;
            stop;
          }
          // inflict damage to parent
          const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penOwner);
          FLOAT fDamageToApply = fDamageMul*(m_fDamageToApply/TM_APPLY_WHOLE_DAMAGE*TM_APPLY_DAMAGE_QUANTUM)*m_fDamageStep;

          InflictDirectDamage( m_penAttach, m_penOwner, DMT_BURNING, fDamageToApply,
                                            GetPlacement().pl_PositionVector, -en_vGravityDir);
          m_fAppliedDamage += fDamageToApply;

          resume;
        }
        on (EFlame ef) : {
          m_penOwner = ef.penOwner;
          FLOAT fTimeLeft=m_tmStart+TM_APPLY_WHOLE_DAMAGE-_pTimer->CurrentTick();
          FLOAT fDamageLeft=(fTimeLeft/TM_APPLY_DAMAGE_QUANTUM)*m_fDamageStep;
          m_fDamageToApply=ClampUp(fDamageLeft+DAMAGE_AMMOUNT, 80.0f);
          m_tmStart=_pTimer->CurrentTick();
          m_fDamageStep=m_fDamageToApply/(TM_APPLY_WHOLE_DAMAGE/TM_APPLY_DAMAGE_QUANTUM);
          resume;
        };
        on (EStopFlaming esf) : {
          if( !esf.m_bNow)
          {
            m_tmDeathParticlesStart=_pTimer->CurrentTick();
            resume;
          }
          else
          {
            m_bLoop = FALSE;
            stop;
          }
        };
        on (EBrushDestroyed) : { 
          m_bLoop = FALSE;
          stop;
        };
        on (ETimer) : { stop; }
      }
      if(_pTimer->CurrentTick()>m_tmStart+TM_APPLY_WHOLE_DAMAGE)
      {
        m_bLoop = FALSE;
      }
    }

    // cease to exist
    Destroy();
    return;
  }
};
