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

804
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Items/ItemHolder/ItemHolder.h"
%}

uses "EntitiesMP/Item";

// health type 
enum ArmorItemType {
  0 ARIT_SHARD        "Shard",    // shard
  1 ARIT_SMALL        "Small",    // small armor
  2 ARIT_MEDIUM       "Medium",   // medium armor
  3 ARIT_STRONG       "Strong",   // strong armor
  4 ARIT_SUPER        "Super",    // super armor
  5 ARIT_HELM         "Helm",     // helm
};

// event for sending through receive item
event EArmor {
  FLOAT fArmor,         // armor to receive
  BOOL bOverTopArmor,   // can be received over top armor
};

class CArmorItem : CItem {
name      "Armor Item";
thumbnail "Thumbnails\\ArmorItem.tbn";

properties:
  1 enum ArmorItemType m_EaitType     "Type" 'Y' = ARIT_SHARD,    // armor type
  2 BOOL m_bOverTopArmor  = FALSE,   // can be received over top armor
  3 INDEX m_iSoundComponent = 0,

components:
  0 class   CLASS_BASE        "Classes\\Item.ecl",

// ********* SHARD *********
  1 model   MODEL_1        "Models\\Items\\Armor\\Armor_1.mdl",
  2 texture TEXTURE_1      "Models\\Items\\Armor\\Armor_1.tex",

// ********* SMALL ARMOR *********
 10 model   MODEL_25       "Models\\Items\\Armor\\Armor_25.mdl",
 11 texture TEXTURE_25     "Models\\Items\\Armor\\Armor_25.tex",

// ********* MEDIUM ARMOR *********
 20 model   MODEL_50       "Models\\Items\\Armor\\Armor_50.mdl",
 21 texture TEXTURE_50     "Models\\Items\\Armor\\Armor_50.tex",

// ********* STRONG ARMOR *********
 22 model   MODEL_100      "Models\\Items\\Armor\\Armor_100.mdl",
 23 texture TEXTURE_100    "Models\\Items\\Armor\\Armor_100.tex",

// ********* SUPER ARMOR *********
 40 model   MODEL_200      "Models\\Items\\Armor\\Armor_200.mdl",
 41 texture TEXTURE_200    "Models\\Items\\Armor\\Armor_200.tex",

// ********* HELM *********
 50 model   MODEL_5        "ModelsMP\\Items\\Armor\\Armor_5.mdl",
 51 texture TEXTURE_5      "ModelsMP\\Items\\Armor\\Armor_5.tex",

// ************** FLARE FOR EFFECT **************
100 texture TEXTURE_FLARE  "Models\\Items\\Flares\\Flare.tex",
101 model   MODEL_FLARE    "Models\\Items\\Flares\\Flare.mdl",

// ************** REFLECTIONS **************
200 texture TEX_REFL_LIGHTMETAL01       "Models\\ReflectionTextures\\LightMetal01.tex",

// ************** SPECULAR **************
210 texture TEX_SPEC_MEDIUM             "Models\\SpecularTextures\\Medium.tex",

// ************** SOUNDS **************
301 sound   SOUND_SHARD        "Sounds\\Items\\ArmourShard.wav",
302 sound   SOUND_SMALL        "Sounds\\Items\\ArmourSmall.wav",
303 sound   SOUND_MEDIUM       "Sounds\\Items\\ArmourMedium.wav",
304 sound   SOUND_STRONG       "Sounds\\Items\\ArmourStrong.wav",
305 sound   SOUND_SUPER        "Sounds\\Items\\ArmourSuper.wav",
306 sound   SOUND_HELM         "SoundsMP\\Items\\ArmourHelm.wav",

functions:
  void Precache(void) {
    switch (m_EaitType) {
      case ARIT_SHARD:  PrecacheSound(SOUND_SHARD ); break;
      case ARIT_SMALL:  PrecacheSound(SOUND_SMALL ); break;                                      
      case ARIT_MEDIUM: PrecacheSound(SOUND_MEDIUM); break;
      case ARIT_STRONG: PrecacheSound(SOUND_STRONG); break;
      case ARIT_SUPER:  PrecacheSound(SOUND_SUPER ); break;
      case ARIT_HELM:   PrecacheSound(SOUND_HELM  ); break;
    }
  }
  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    pes->es_strName = "Armor"; 
    pes->es_ctCount = 1;
    pes->es_ctAmmount = (INDEX) m_fValue;
    pes->es_fValue = m_fValue*2;
    pes->es_iScore = 0;//m_iScore;
    switch (m_EaitType) {
      case ARIT_SHARD:  pes->es_strName+=" shard";  break;
      case ARIT_SMALL:  pes->es_strName+=" small";  break;                                      
      case ARIT_MEDIUM: pes->es_strName+=" medium"; break;
      case ARIT_STRONG: pes->es_strName+=" strong"; break;
      case ARIT_SUPER:  pes->es_strName+=" super";  break;
      case ARIT_HELM:   pes->es_strName+=" helm";   break;
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
    switch (m_EaitType) {
      case ARIT_SHARD:
        Particles_Emanate(this, 0.75f*0.75f, 0.75f*0.75f, PT_STAR04, 8, 7.0f);
        break;
      case ARIT_SMALL:
        Particles_Emanate(this, 1.0f*0.75f, 1.0f*0.75f, PT_STAR04, 32, 7.0f);
        break;                                      
      case ARIT_MEDIUM:
        Particles_Emanate(this, 1.5f*0.75f, 1.5f*0.75f, PT_STAR04, 64, 7.0f);
        break;
      case ARIT_STRONG:                              
        Particles_Emanate(this, 2.0f*0.75f, 1.25f*0.75f, PT_STAR04, 96, 7.0f);
        break;
      case ARIT_SUPER:
        Particles_Emanate(this, 2.5f*0.75f, 1.5f*0.75f, PT_STAR04, 128, 7.0f);
        break;
      case ARIT_HELM:
        Particles_Emanate(this, 0.875f*0.75f, 0.875f*0.75f, PT_STAR04, 16, 7.0f);
        break;      
    }
  }

  // set health properties depending on health type
  void SetProperties(void) {
    switch (m_EaitType) {
      case ARIT_SHARD:
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_SMALL);
        m_fValue = 1.0f;
        m_bOverTopArmor = TRUE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 10.0f; 
        m_strDescription.PrintF("Shard - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_1, TEXTURE_1, 0, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.4f,0), FLOAT3D(1.0f,1.0f,0.3f) );
        StretchItem(FLOAT3D(0.75f*0.75f, 0.75f*0.75f, 0.75f*0.75f));
        m_iSoundComponent = SOUND_SHARD;
        break;
      case ARIT_SMALL:
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 25.0f;
        m_bOverTopArmor = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 10.0f; 
        m_strDescription.PrintF("Small - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_25, TEXTURE_25, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(2,2,0.5f) );
        StretchItem(FLOAT3D(2.0f, 2.0f, 2.0f));
        m_iSoundComponent = SOUND_SMALL;
        break;
      case ARIT_MEDIUM: {
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 50.0f;
        m_bOverTopArmor = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 25.0f; 
        m_strDescription.PrintF("Medium - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_50, TEXTURE_50, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,1.0f,0), FLOAT3D(3,3,0.5f) );
        StretchItem(FLOAT3D(2.0f, 2.0f, 2.0f));
        m_iSoundComponent = SOUND_MEDIUM;
                        } break;
      case ARIT_STRONG:
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 100.0f;
        m_bOverTopArmor = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 60.0f; 
        m_strDescription.PrintF("Strong - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_100, TEXTURE_100, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(3.5,3.5,1.0f) );
        StretchItem(FLOAT3D(2.5f, 2.5f, 2.5f));
        m_iSoundComponent = SOUND_STRONG;
        break;
      case ARIT_SUPER:
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
        m_fValue = 200.0f;
        m_bOverTopArmor = TRUE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 120.0f; 
        m_strDescription.PrintF("Super - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance

        AddItem(MODEL_200, TEXTURE_200, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(3,3,1.0f) );
        StretchItem(FLOAT3D(2.5f, 2.5f, 2.5f));
        m_iSoundComponent = SOUND_SUPER;
        break;
      case ARIT_HELM:
        ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_SMALL);
        m_fValue = 5.0f;
        m_bOverTopArmor = FALSE;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 10.0f; 
        m_strDescription.PrintF("Helm - H:%g  T:%g", m_fValue, m_fRespawnTime);
        // set appearance
        AddItem(MODEL_5, TEXTURE_5, 0, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.5f,0), FLOAT3D(1.5f,1.5f,0.4f) );
        StretchItem(FLOAT3D(0.875f*0.75f, 0.875f*0.75f, 0.875f*0.75f));
        m_iSoundComponent = SOUND_HELM;
        break;        
    }
  };

  void AdjustDifficulty(void)
  {
    if (!GetSP()->sp_bAllowArmor && m_penTarget==NULL) {
      Destroy();
    }
  }

procedures:
  ItemCollected(EPass epass) : CItem::ItemCollected {
    ASSERT(epass.penOther!=NULL);

    // if armor stays
    if (GetSP()->sp_bHealthArmorStays && !(m_bPickupOnce||m_bRespawn)) {
      // if already picked by this player
      BOOL bWasPicked = MarkPickedBy(epass.penOther);
      if (bWasPicked) {
        // don't pick again
        return;
      }
    }

    // send health to entity
    EArmor eArmor;
    eArmor.fArmor = m_fValue;
    eArmor.bOverTopArmor = m_bOverTopArmor;
    // if health is received
    if (epass.penOther->ReceiveItem(eArmor)) {

      if(_pNetwork->IsPlayerLocal(epass.penOther))
      {
        switch (m_EaitType)
        {
          case ARIT_SHARD:  IFeel_PlayEffect("PU_ArmourShard"); break;
          case ARIT_SMALL:  IFeel_PlayEffect("PU_ArmourSmall"); break;
          case ARIT_MEDIUM: IFeel_PlayEffect("PU_ArmourMedium"); break;
          case ARIT_STRONG: IFeel_PlayEffect("PU_ArmourStrong"); break; 
          case ARIT_SUPER:  IFeel_PlayEffect("PU_ArmourSuper"); break; 
          case ARIT_HELM:   IFeel_PlayEffect("PU_ArmourHelm"); break; 
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
    StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
    SetProperties();  // set properties

    jump CItem::ItemLoop();
  };
};
