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

502
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/BasicEffects";
uses "Engine/Classes/MovableEntity";


// input parameters for bullet
event EBulletInit {
  CEntityPointer penOwner,        // who launched it
  FLOAT fDamage,                  // damage
};

%{
void CBullet_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINSTONE);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINSAND);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINREDSAND);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINWATER);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINSTONENOSOUND);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINSANDNOSOUND);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINREDSANDNOSOUND);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETSTAINWATERNOSOUND);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BLOODSPILL);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BULLETTRAIL);
}
%}

class export CBullet : CEntity {
name      "Bullet";
thumbnail "";
features "ImplementsOnPrecache";

properties:
  1 CEntityPointer m_penOwner,        // entity which owns it
  2 FLOAT m_fDamage = 0.0f,                   // damage
  3 FLOAT3D m_vTarget = FLOAT3D(0,0,0),       // bullet target point in space
  4 FLOAT3D m_vTargetCopy = FLOAT3D(0,0,0),   // copy of bullet target point in space for jitter
  6 FLOAT3D m_vHitPoint = FLOAT3D(0,0,0),     // hit point
  8 INDEX m_iBullet = 0,                // bullet for lerped launch
  9 enum DamageType m_EdtDamage = DMT_BULLET,   // damage type
  10 FLOAT m_fBulletSize = 0.0f,      // bullet can have radius, for hitting models only

components:
  1 class   CLASS_BASIC_EFFECT "Classes\\BasicEffect.ecl"

functions:

/************************************************************
 *                      BULLET LAUNCH                       *
 ************************************************************/
  // set bullet damage
  void SetDamage(FLOAT fDamage) {
    m_fDamage = fDamage;
  };

  // calc jitter target
  void CalcTarget(FLOAT fRange) {
    // destination in bullet direction
    AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, m_vTarget);
    m_vTarget *= fRange;
    m_vTarget += GetPlacement().pl_PositionVector;
    m_vTargetCopy = m_vTarget;
  };

  void CalcTarget(CEntity *pen, FLOAT fRange) {
    FLOAT3D vTarget;

    // target body
    EntityInfo *peiTarget = (EntityInfo*) (pen->GetEntityInfo());
    GetEntityInfoPosition(pen, peiTarget->vTargetCenter, vTarget);

    // calculate
    m_vTarget = (vTarget - GetPlacement().pl_PositionVector).Normalize();
    m_vTarget *= fRange;
    m_vTarget += GetPlacement().pl_PositionVector;
    m_vTargetCopy = m_vTarget;
  };

  // calc jitter target - !!! must call CalcTarget first !!!
  void CalcJitterTarget(FLOAT fR) {
    FLOAT3D vJitter;
/* My Sphere
    FLOAT fXZ = FRnd()*360.0f;
    FLOAT fXY = FRnd()*360.0f;

    // sphere
    fR *= FRnd();
    vJitter(1) = CosFast(fXZ)*CosFast(fXY)*fR;
    vJitter(2) = CosFast(fXZ)*SinFast(fXY)*fR;
    vJitter(3) = SinFast(fXZ)*fR;*/
// comp graphics algorithms sphere
    FLOAT fZ = FRnd()*2.0f - 1.0f;
    FLOAT fA = FRnd()*360.0f;
    FLOAT fT = Sqrt(1-(fZ*fZ));
    vJitter(1) = fT * CosFast(fA);
    vJitter(2) = fT * SinFast(fA);
    vJitter(3) = fZ;
    vJitter = vJitter*fR*FRnd();

    // target
    m_vTarget = m_vTargetCopy + vJitter;
  };

  // calc jitter target asymetric - !!! must call CalcTarget first !!!
  void CalcJitterTargetFixed(FLOAT fX, FLOAT fY, FLOAT fJitter) {
    FLOAT fRndX = FRnd()*2.0f - 1.0f;
    FLOAT fRndY = FRnd()*2.0f - 1.0f;
    FLOAT3D vX, vY;
    const FLOATmatrix3D &m=GetRotationMatrix();
    vX(1) = m(1,1); vX(2) = m(2,1); vX(3) = m(3,1);
    vY(1) = m(1,2); vY(2) = m(2,2); vY(3) = m(3,2);
    // target
    m_vTarget = m_vTargetCopy + (vX*(fX+fRndX*fJitter)) + (vY*(fY+fRndY*fJitter));
  };

  // launch one bullet
  void LaunchBullet(BOOL bSound, BOOL bTrail, BOOL bHitFX)
  {
    // cast a ray to find bullet target
    CCastRay crRay( m_penOwner, GetPlacement().pl_PositionVector, m_vTarget);
    crRay.cr_bHitPortals = TRUE;
    crRay.cr_bHitTranslucentPortals = TRUE;
    crRay.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;
    crRay.cr_bPhysical = FALSE;
    crRay.cr_fTestR = m_fBulletSize;
    FLOAT3D vHitDirection;
    AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vHitDirection);

    INDEX ctCasts = 0;
    while( ctCasts<10)
    {
      if(ctCasts == 0)
      {
        // perform first cast
        GetWorld()->CastRay(crRay);       
      }
      else
      {
        // next casts
        GetWorld()->ContinueCast(crRay);
      }
      ctCasts++;

      // stop casting if nothing hit
      if (crRay.cr_penHit==NULL)
      {
        break;
      }
      // apply damage
      const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penOwner);
      InflictDirectDamage(crRay.cr_penHit, m_penOwner, m_EdtDamage, m_fDamage*fDamageMul,
                            crRay.cr_vHit, vHitDirection);

      m_vHitPoint = crRay.cr_vHit;

      // if brush hitted
      if (crRay.cr_penHit->GetRenderType()==RT_BRUSH && crRay.cr_pbpoBrushPolygon!=NULL)
      {
        CBrushPolygon *pbpo = crRay.cr_pbpoBrushPolygon;
        FLOAT3D vHitNormal = FLOAT3D(pbpo->bpo_pbplPlane->bpl_plAbsolute);
        // obtain surface type
        INDEX iSurfaceType = pbpo->bpo_bppProperties.bpp_ubSurfaceType;
        BulletHitType bhtType = BHT_BRUSH_STONE;
        // get content type
        INDEX iContent = pbpo->bpo_pbscSector->GetContentType();
        CContentType &ct = GetWorld()->wo_actContentTypes[iContent];
        
        bhtType=(BulletHitType) GetBulletHitTypeForSurface(iSurfaceType);
        // if this is under water polygon
        if( ct.ct_ulFlags&CTF_BREATHABLE_GILLS)
        {
          // if we hit water surface
          if( iSurfaceType==SURFACE_WATER) 
          {
            vHitNormal = -vHitNormal;

            bhtType=BHT_BRUSH_WATER;
          }   
          // if we hit stone under water
          else
          {
            bhtType=BHT_BRUSH_UNDER_WATER;
          }
        }
        // spawn hit effect
        BOOL bPassable = pbpo->bpo_ulFlags & (BPOF_PASSABLE|BPOF_SHOOTTHRU);
        if (!bPassable || iSurfaceType==SURFACE_WATER) {
          SpawnHitTypeEffect(this, bhtType, bSound, vHitNormal, crRay.cr_vHit, vHitDirection, FLOAT3D(0.0f, 0.0f, 0.0f));
        }
        if(!bPassable) {
          break;
        }
      // if not brush
      } else {

        // if flesh entity
        if (crRay.cr_penHit->GetEntityInfo()!=NULL) {
          if( ((EntityInfo*)crRay.cr_penHit->GetEntityInfo())->Eeibt == EIBT_FLESH)
          {
            CEntity *penOfFlesh = crRay.cr_penHit;
            FLOAT3D vHitNormal = (GetPlacement().pl_PositionVector-m_vTarget).Normalize();
            FLOAT3D vOldHitPos = crRay.cr_vHit;
            FLOAT3D vDistance;

            // look behind the entity (for back-stains)
            GetWorld()->ContinueCast(crRay);
            if( crRay.cr_penHit!=NULL && crRay.cr_pbpoBrushPolygon!=NULL && 
                crRay.cr_penHit->GetRenderType()==RT_BRUSH)
            {
              vDistance = crRay.cr_vHit-vOldHitPos;
              vHitNormal = FLOAT3D(crRay.cr_pbpoBrushPolygon->bpo_pbplPlane->bpl_plAbsolute);
            }
            else
            {
              vDistance = FLOAT3D(0.0f, 0.0f, 0.0f);
              vHitNormal = FLOAT3D(0,0,0);
            }

            if(IsOfClass(penOfFlesh, "Gizmo") ||
               IsOfClass(penOfFlesh, "Beast"))
            {
              // spawn green blood hit spill effect
              SpawnHitTypeEffect(this, BHT_ACID, bSound, vHitNormal, crRay.cr_vHit, vHitDirection, vDistance);
            }
            else
            {
              // spawn red blood hit spill effect
              SpawnHitTypeEffect(this, BHT_FLESH, bSound, vHitNormal, crRay.cr_vHit, vHitDirection, vDistance);
            }
            break;
          }
        }

        // stop casting ray if not brush
        break;
      }
    }

    // [SSE] Bullet trail fix.
    // If we hit nothing then we don't need trail effect which goes to center of map!
    if (crRay.cr_penHit == NULL && m_vHitPoint == FLOAT3D(0.0F, 0.0F, 0.0F)) {
      bTrail = FALSE; 
    }

    if( bTrail)
    {
      SpawnTrail();
    }
  };

  // destroy yourself
  void DestroyBullet(void) {
    Destroy();
  };



/************************************************************
 *                        EFFECTS                           *
 ************************************************************/
  // spawn trail of this bullet
  void SpawnTrail(void) 
  {
    // get bullet path positions
    const FLOAT3D &v0 = GetPlacement().pl_PositionVector;
    const FLOAT3D &v1 = m_vHitPoint;
    // calculate distance
    FLOAT3D vD = v1-v0;
    FLOAT fD = vD.Length();
    // if too short
    if (fD<1.0f) {
      // no trail
      return;
    }

    // length must be such that it doesn't get out of path
    FLOAT fLen = Min(20.0f, fD);
    // position is random, but it must not make trail go out of path
    FLOAT3D vPos;
    if (fLen<fD) {
      vPos = Lerp(v0, v1, FRnd()*(fD-fLen)/fD);
    } else {
      vPos = v0;
    }

    ESpawnEffect ese;
    UBYTE ubRndH = UBYTE( 8+FRnd()*32);
    UBYTE ubRndS = UBYTE( 8+FRnd()*32);
    UBYTE ubRndV = UBYTE( 224+FRnd()*32);
    UBYTE ubRndA = UBYTE( 32+FRnd()*128);
    ese.colMuliplier = HSVToColor(ubRndH, ubRndS, ubRndV)|ubRndA;
    ese.betType = BET_BULLETTRAIL;
    ese.vNormal = vD/fD;
    ese.vStretch = FLOAT3D(0.1f, fLen, 1.0f);

    // spawn effect
    FLOAT3D vBulletIncommingDirection;
    vBulletIncommingDirection = (m_vTarget-GetPlacement().pl_PositionVector).Normalize();
    CPlacement3D plHit = CPlacement3D(vPos-vBulletIncommingDirection*0.1f, GetPlacement().pl_OrientationAngle);
    CEntityPointer penHit = CreateEntity(plHit , CLASS_BASIC_EFFECT);
    penHit->Initialize(ese);
  }

procedures:

  Main(EBulletInit eInit)
  {
    // remember the initial parameters
    ASSERT(eInit.penOwner!=NULL);
    m_penOwner = eInit.penOwner;
    m_fDamage = eInit.fDamage;

    InitAsVoid();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // for lerped launch
    m_iBullet = 0;
    return;
  };
};