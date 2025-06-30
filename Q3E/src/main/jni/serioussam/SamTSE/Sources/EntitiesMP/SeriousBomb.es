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

354
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/BackgroundViewer.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/EnemyBase.h"
%}

// event for initialisation
event ESeriousBomb {
  CEntityPointer penOwner,
};

%{
void CSeriousBomb_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->  PrecacheSound(SOUND_BLOW);
};
%}

class CSeriousBomb : CRationalEntity {
name      "Serious Bomb";
thumbnail "";
features  "ImplementsOnPrecache";

properties:
  1 CEntityPointer m_penOwner,     // entity which owns it  
 20 CSoundObject m_soBlow,

components:
  
  //0 class   CLASS_BASE      "Classes\\Item.ecl",

  //1 model   MODEL_BOMB   "ModelsMP\\Items\\PowerUps\\SeriousBomb\\SeriousBomb.mdl",
  //2 texture TEXTURE_BOMB "ModelsMP\\Items\\PowerUps\\SeriousBomb\\SeriousBomb.tex",

  100 sound   SOUND_BLOW   "SoundsMP\\Weapons\\SeriousBombBlow.wav",


functions:
  
  void ShakeItBaby(FLOAT tmShaketime, FLOAT fPower, FLOAT fFade, BOOL bFadeIn)
  {
    CWorldSettingsController *pwsc = GetWSC(this);
    if (pwsc!=NULL) {
      pwsc->m_tmShakeStarted = tmShaketime;
      pwsc->m_vShakePos = GetPlacement().pl_PositionVector;
      pwsc->m_fShakeFalloff = 450.0f;
      pwsc->m_fShakeFade = fFade;

      pwsc->m_fShakeIntensityZ = 0;
      pwsc->m_tmShakeFrequencyZ = 5.0f;
      pwsc->m_fShakeIntensityY = 0.1f*fPower;
      pwsc->m_tmShakeFrequencyY = 5.0f;
      pwsc->m_fShakeIntensityB = 2.5f*fPower;
      pwsc->m_tmShakeFrequencyB = 7.2f;

      pwsc->m_bShakeFadeIn = bFadeIn;
    }
  }

  void Glare(FLOAT fStart, FLOAT fEnd, FLOAT fFinR, FLOAT fFoutR)
  {
    CWorldSettingsController *pwsc = GetWSC(this);
    if (pwsc!=NULL)
    {
      pwsc->m_colGlade=C_WHITE;
      pwsc->m_tmGlaringStarted = _pTimer->CurrentTick()+fStart;
      pwsc->m_tmGlaringEnded = pwsc->m_tmGlaringStarted+fEnd;
      pwsc->m_fGlaringFadeInRatio = fFinR;
      pwsc->m_fGlaringFadeOutRatio = fFoutR;
    }
  }
    

  void ExplodeBomb( void )
  {
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(this->GetWorld()->wo_cenEntities, CEntity, iten) {
      CEntity *pen = iten;
      if (IsDerivedFromClass(pen, "Enemy Base")) {
        CEnemyBase *penEnemy = (CEnemyBase *)pen;
        if (penEnemy->m_bBoss==TRUE || DistanceTo(this, penEnemy)>250.0f) {
          continue;
        }
        this->InflictDirectDamage(pen, this, DMT_EXPLOSION, penEnemy->GetHealth()+100.0f, pen->GetPlacement().pl_PositionVector, FLOAT3D(0,1,0));
      }
    }}
  }
  
procedures:
  
  Main(ESeriousBomb esb)
  {
    InitAsVoid();

    if (esb.penOwner) {
      m_penOwner = esb.penOwner;
      
      m_soBlow.Set3DParameters(500.0f, 250.0f, 3.0f, 1.0f);
      PlaySound(m_soBlow, SOUND_BLOW, SOF_3D);
      if(_pNetwork->IsPlayerLocal(m_penOwner)) {IFeel_PlayEffect("SeriousBombBlow");}
      
      //Glare(tmp_af[5], tmp_af[6], tmp_af[7], tmp_af[8]);
      Glare(1.0f, 2.8f, 0.3f, 0.3f);
      
      ShakeItBaby(_pTimer->CurrentTick(), 4.0f, 1.0f, TRUE);
      autowait(1.5f);

      // fading shake     
      ShakeItBaby(_pTimer->CurrentTick(), 8.0f, 2.0f, FALSE);
      
      // explode bomb twicejust to be sure
      ExplodeBomb();
      autowait(0.25f);
      ExplodeBomb();
      autowait(1.75f);   

    }
    Destroy();
    return;
  };

};
