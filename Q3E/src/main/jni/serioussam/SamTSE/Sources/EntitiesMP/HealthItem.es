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

801
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Items/ItemHolder/ItemHolder.h"
%}

uses "EntitiesMP/Item";

// health type 
enum HealthItemType {
  0 HIT_PILL      "Pill",       // pill health
  1 HIT_SMALL     "Small",      // small health
  2 HIT_MEDIUM    "Medium",     // medium health
  3 HIT_LARGE     "Large",      // large health
  4 HIT_SUPER     "Super",      // super health
};

// event for sending through receive item
event EHealth {
  FLOAT fHealth,        // health to receive
  BOOL bOverTopHealth,  // can be received over top health
};

class CHealthItem : CItem {
name      "Health Item";
thumbnail "Thumbnails\\HealthItem.tbn";

properties:
  1 enum HealthItemType m_EhitType    "Type" 'Y' = HIT_SMALL,     // health type
  2 BOOL m_bOverTopHealth             = FALSE,  // can be received over top health
  3 INDEX m_iSoundComponent = 0,

components:
  0 class   CLASS_BASE        "Classes\\Item.ecl",

// ********* PILL HEALTH *********
  1 model   MODEL_PILL        "Models\\Items\\Health\\Pill\\Pill.mdl",
  2 texture TEXTURE_PILL      "Models\\Items\\Health\\Pill\\Pill.tex",
  3 texture TEXTURE_PILL_BUMP "Models\\Items\\Health\\Pill\\PillBump.tex",

// ********* SMALL HEALTH *********
 10 model   MODEL_SMALL       "Models\\Items\\Health\\Small\\Small.mdl",
 11 texture TEXTURE_SMALL     "Models\\Items\\Health\\Small\\Small.tex",

// ********* MEDIUM HEALTH *********
 20 model   MODEL_MEDIUM      "Models\\Items\\Health\\Medium\\Medium.mdl",
 21 texture TEXTURE_MEDIUM    "Models\\Items\\Health\\Medium\\Medium.tex",

// ********* LARGE HEALTH *********
 30 model   MODEL_LARGE       "Models\\Items\\Health\\Large\\Large.mdl",
 31 texture TEXTURE_LARGE     "Models\\Items\\Health\\Large\\Large.tex",

// ********* SUPER HEALTH *********
 40 model   MODEL_SUPER     "Models\\Items\\Health\\Super\\Super.mdl",
 41 texture TEXTURE_SUPER   "Models\\Items\\Health\\Super\\Super.tex",

// ********* MISC *********
 50 texture TEXTURE_SPECULAR_STRONG "Models\\SpecularTextures\\Strong.tex",
 51 texture TEXTURE_SPECULAR_MEDIUM "Models\\SpecularTextures\\Medium.tex",
 52 texture TEXTURE_REFLECTION_LIGHTMETAL01 "Models\\ReflectionTextures\\LightMetal01.tex",
 53 texture TEXTURE_REFLECTION_GOLD01 "Models\\ReflectionTextures\\Gold01.tex",
 54 texture TEXTURE_REFLECTION_PUPLE01 "Models\\ReflectionTextures\\Purple01.tex",
 55 texture TEXTURE_FLARE "Models\\Items\\Flares\\Flare.tex",
 56 model   MODEL_FLARE "Models\\Items\\Flares\\Flare.mdl",

// ************** SOUNDS **************
301 sound   SOUND_PILL         "Sounds\\Items\\HealthPill.wav",
302 sound   SOUND_SMALL        "Sounds\\Items\\HealthSmall.wav",
303 sound   SOUND_MEDIUM       "Sounds\\Items\\HealthMedium.wav",
304 sound   SOUND_LARGE        "Sounds\\Items\\HealthLarge.wav",
305 sound   SOUND_SUPER        "Sounds\\Items\\HealthSuper.wav",

functions:
  void Precache(void) {
    switch (m_EhitType) {
      case HIT_PILL:   PrecacheSound(SOUND_PILL  ); break;
      case HIT_SMALL:  PrecacheSound(SOUND_SMALL ); break;                                      
      case HIT_MEDIUM: PrecacheSound(SOUND_MEDIUM); break;
      case HIT_LARGE:  PrecacheSound(SOUND_LARGE ); break;
      case HIT_SUPER:  PrecacheSound(SOUND_SUPER ); break;
    }
  }
  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    pes->es_strName = "Health"; 
    pes->es_ctCount = 1;
    pes->es_ctAmmount = (INDEX) m_fValue;
    pes->es_fValue = m_fValue;
    pes->es_iScore = 0;//m_iScore;
    
    switch (m_EhitType) {
      case HIT_PILL:  pes->es_strName+=" pill";   break;
      case HIT_SMALL: pes->es_strName+=" small";  break;
      case HIT_MEDIUM:pes->es_strName+=" medium"; break;
      case HIT_LARGE: pes->es_strName+=" large";  break;
      case HIT_SUPER: pes->es_strName+=" super";  break;
    }

    return TRUE;
  }

  // render particles
  void RenderParticles(void) {
    // no particles when not existing or in DM modes
    if (GetRenderType()!=CEntity::RT_MODEL || GetSP()->sp_gmGameMode>CSessionProperties::GM_COOPERATIVE
      || !ShowItemParticles())
    {
      return;
    }
    switch (m_EhitType) {
      case HIT_PILL:
        Particles_Stardust(this, 0.9f*0.75f, 0.70f*0.75f, PT_STAR08, 32);
        break;
      case HIT_SMALL:
        Particles_Stardust(this, 1.0f*0.75f, 0.75f*0.75f, PT_STAR08, 128);
        break;
      case HIT_MEDIUM:
        Particles_Stardust(this, 1.0f*0.75f, 0.75f*0.75f, PT_STAR08, 128);
        break;
      case HIT_LARGE:
        Particles_Stardust(this, 2.0f*0.75f, 1.0f*0.75f, PT_STAR08, 192);
        break;
      case HIT_SUPER:
        Particles_Stardust(this, 2.3f*0.75f, 1.5f*0.75f, PT_STAR08, 320);
        break;
    }
  }

  // set health properties depending on health type
  void SetProperties(void) {
    switch (m_EhitType) {
      case HIT_PILL:
        StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_SMALL);
        m_fValue = 1.0f;
        m_bOverTopHealth = TRUE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 10.0f; 
        m_strDescription.PrintF("Pill - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_PILL, TEXTURE_PILL, 0, TEXTURE_SPECULAR_STRONG, TEXTURE_PILL_BUMP);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f*0.75f, 1.0f*0.75f, 1.0f*0.75));
        m_iSoundComponent = SOUND_PILL;
        break;
      case HIT_SMALL:
        StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 10.0f;
        m_bOverTopHealth = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 10.0f; 
        m_strDescription.PrintF("Small - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_SMALL, TEXTURE_SMALL, TEXTURE_REFLECTION_LIGHTMETAL01, TEXTURE_SPECULAR_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.4f,0), FLOAT3D(2,2,0.4f) );
        StretchItem(FLOAT3D(1.0f*0.75f, 1.0f*0.75f, 1.0f*0.75));
        m_iSoundComponent = SOUND_SMALL;
        break;                                                                 // add flare

      case HIT_MEDIUM:
        StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 25.0f;
        m_bOverTopHealth = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 25.0f; 
        m_strDescription.PrintF("Medium - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_MEDIUM, TEXTURE_MEDIUM, TEXTURE_REFLECTION_LIGHTMETAL01, TEXTURE_SPECULAR_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(2.5f,2.5f,0.5f) );
        StretchItem(FLOAT3D(1.5f*0.75f, 1.5f*0.75f, 1.5f*0.75));
        m_iSoundComponent = SOUND_MEDIUM;
        break;
      case HIT_LARGE:
        StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 50.0f;
        m_bOverTopHealth = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 60.0f; 
        m_strDescription.PrintF("Large - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_LARGE, TEXTURE_LARGE, TEXTURE_REFLECTION_GOLD01, TEXTURE_SPECULAR_STRONG, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.8f,0), FLOAT3D(2.8f,2.8f,1.0f) );
        StretchItem(FLOAT3D(1.2f*0.75f, 1.2f*0.75f, 1.2f*0.75));
        m_iSoundComponent = SOUND_LARGE;
        break;
      case HIT_SUPER:
        StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 100.0f;
        m_bOverTopHealth = TRUE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 120.0f; 
        m_strDescription.PrintF("Super - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_SUPER, TEXTURE_SUPER, 0, TEXTURE_SPECULAR_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,1.0f,0), FLOAT3D(3,3,1.0f) );
        StretchItem(FLOAT3D(1.0f*0.75f, 1.0f*0.75f, 1.0f*0.75));
        CModelObject &mo = GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_ITEM)->amo_moModelObject;
        mo.PlayAnim(0, AOF_LOOPING);

        m_iSoundComponent = SOUND_SUPER;
        break;
    }
  };

  void AdjustDifficulty(void)
  {
    if (!GetSP()->sp_bAllowHealth && m_penTarget==NULL) {
      Destroy();
    }
  }
procedures:
  ItemCollected(EPass epass) : CItem::ItemCollected {
    ASSERT(epass.penOther!=NULL);

    // if health stays
    if (GetSP()->sp_bHealthArmorStays && !(m_bPickupOnce||m_bRespawn)) {
      // if already picked by this player
      BOOL bWasPicked = MarkPickedBy(epass.penOther);
      if (bWasPicked) {
        // don't pick again
        return;
      }
    }

    // send health to entity
    EHealth eHealth;
    eHealth.fHealth = m_fValue;
    eHealth.bOverTopHealth = m_bOverTopHealth;
    // if health is received
    if (epass.penOther->ReceiveItem(eHealth)) {

      if(_pNetwork->IsPlayerLocal(epass.penOther))
      {
        switch (m_EhitType)
        {
          case HIT_PILL:  IFeel_PlayEffect("PU_HealthPill"); break;
          case HIT_SMALL: IFeel_PlayEffect("PU_HealthSmall"); break;
          case HIT_MEDIUM:IFeel_PlayEffect("PU_HealthMedium"); break;
          case HIT_LARGE: IFeel_PlayEffect("PU_HealthLarge"); break;
          case HIT_SUPER: IFeel_PlayEffect("PU_HealthSuper"); break;
        }
      }

      // play the pickup sound
      m_soPick.Set3DParameters(50.0f, 1.0f, 1.0f, 1.0f);
      PlaySound(m_soPick, m_iSoundComponent, SOF_3D);
      m_fPickSoundLen = GetSoundLength(m_iSoundComponent);
      if (!GetSP()->sp_bHealthArmorStays || (m_bPickupOnce||m_bRespawn)) {
        jump CItem::ItemReceived();
      }
    }
    return;
  };

  Main() {
    Initialize();     // initialize base class
    SetProperties();  // set properties

    jump CItem::ItemLoop();
  };
};
