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

803
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Items/ItemHolder/ItemHolder.h"
#include "Models/Items/Ammo/Shells/Shells.h"
#include "Models/Items/Ammo/Bullets/Bullets.h"
#include "Models/Items/Ammo/Rockets/Rockets.h"
#include "Models/Weapons/RocketLauncher/Projectile/Rocket.h"
#include "Models/Items/Ammo/Grenades/Grenades.h"
#include "Models/Items/Ammo/Electricity/Electricity.h"
#include "Models/Items/Ammo/Cannonball/CannonBall.h"
#include "Models/Items/Ammo/Cannonball/CannonBallQuad.h"
#include "ModelsMP/Items/Ammo/SniperBullets/SniperBullets.h"
%}

uses "EntitiesMP/Item";

// ammo type 
enum AmmoItemType {
  1 AIT_SHELLS          "Shells",
  2 AIT_BULLETS         "Bullets",
  3 AIT_ROCKETS         "Rockets",
  4 AIT_GRENADES        "Grenades",
  5 AIT_ELECTRICITY     "Electricity",
  6 AIT_NUKEBALL        "obsolete",
  7 AIT_IRONBALLS       "IronBalls",
  8 AIT_SERIOUSPACK     "SeriousPack - don't use",
  9 AIT_BACKPACK        "BackPack - don't use",
  10 AIT_NAPALM         "Napalm",
  11 AIT_SNIPERBULLETS  "Sniper bullets"
};

// event for sending through receive item
event EAmmoItem {
  enum AmmoItemType EaitType,     // ammo type
  INDEX iQuantity,                // ammo quantity
};

class CAmmoItem : CItem {
name      "Ammo Item";
thumbnail "Thumbnails\\AmmoItem.tbn";

properties:
  1 enum AmmoItemType  m_EaitType    "Type" 'Y' = AIT_SHELLS,     // health type

components:
  0 class   CLASS_BASE        "Classes\\Item.ecl",

// ********* SHELLS *********
  1 model   MODEL_SHELLS          "Models\\Items\\Ammo\\Shells\\Shells.mdl",
  2 texture TEXTURE_SHELLS        "Models\\Items\\Ammo\\Shells\\Shells.tex",

// ********* BULLETS *********
 10 model   MODEL_BULLETS         "Models\\Items\\Ammo\\Bullets\\Bullets.mdl",
 11 texture TEXTURE_BULLETS       "Models\\Items\\Ammo\\Bullets\\Bullets.tex",

// ********* ROCKETS *********
 20 model   MODEL_ROCKETS         "Models\\Items\\Ammo\\Rockets\\Rockets.mdl",
 21 model   MODEL_RC_ROCKET       "Models\\Weapons\\RocketLauncher\\Projectile\\Rocket.mdl",
 22 texture TEXTURE_ROCKET        "Models\\Weapons\\RocketLauncher\\Projectile\\Rocket.tex",

// ********* GRENADES *********
 30 model   MODEL_GRENADES        "Models\\Items\\Ammo\\Grenades\\Grenades.mdl",
 31 model   MODEL_GR_GRENADE      "Models\\Items\\Ammo\\Grenades\\Grenade.mdl",
 32 texture TEXTURE_GRENADES      "Models\\Items\\Ammo\\Grenades\\Grenades.tex",
 33 texture TEXTURE_GR_GRENADE    "Models\\Weapons\\GrenadeLauncher\\Grenade\\Grenade.tex",

// ********* ELECTRICITY *********
 40 model   MODEL_ELECTRICITY     "Models\\Items\\Ammo\\Electricity\\Electricity.mdl",
 41 model   MODEL_EL_EFFECT       "Models\\Items\\Ammo\\Electricity\\Effect.mdl",
 42 model   MODEL_EL_EFFECT2      "Models\\Items\\Ammo\\Electricity\\Effect2.mdl",
 43 texture TEXTURE_ELECTRICITY   "Models\\Items\\Ammo\\Electricity\\Electricity.tex",
 44 texture TEXTURE_EL_EFFECT     "Models\\Items\\Ammo\\Electricity\\Effect.tex",

// ********* CANNON BALLS *********
 50 model   MODEL_CANNONBALL      "Models\\Items\\Ammo\\Cannonball\\Cannonball.mdl",
 51 model   MODEL_CANNONBALLS     "Models\\Items\\Ammo\\Cannonball\\CannonballQuad.mdl",
 52 texture TEXTURE_IRONBALL      "Models\\Weapons\\Cannon\\Projectile\\IronBall.tex",
// 53 texture TEXTURE_NUKEBALL      "Models\\Weapons\\Cannon\\Projectile\\NukeBall.tex",

// ********* BACK PACK *********
 60 model   MODEL_BACKPACK      "Models\\Items\\PowerUps\\BackPack\\BackPack.mdl",
 61 texture TEXTURE_BACKPACK    "Models\\Items\\PowerUps\\BackPack\\BackPack.tex",

// ********* SERIOUS PACK *********
 70 model   MODEL_SERIOUSPACK      "Models\\Items\\PowerUps\\SeriousPack\\SeriousPack.mdl",
 71 texture TEXTURE_SERIOUSPACK    "Models\\Items\\PowerUps\\SeriousPack\\SeriousPack.tex",


// ********* FUEL RESERVOIR *********
 80 model   MODEL_FL_RESERVOIR          "ModelsMP\\Items\\Ammo\\Napalm\\Napalm.mdl",
 81 texture TEXTURE_FL_FUELRESERVOIR    "ModelsMP\\Weapons\\Flamer\\FuelReservoir.tex",
 
 // ******** SNIPER BULLETS *********
 90 model   MODEL_SNIPER_BULLETS        "ModelsMP\\Items\\Ammo\\SniperBullets\\SniperBullets.mdl",
 91 texture TEXTURE_SNIPER_BULLETS      "ModelsMP\\Items\\Ammo\\SniperBullets\\SniperBullets.tex",

// ************** FLARE FOR EFFECT **************
100 texture TEXTURE_FLARE "Models\\Items\\Flares\\Flare.tex",
101 model   MODEL_FLARE "Models\\Items\\Flares\\Flare.mdl",

// ************** REFLECTIONS **************
200 texture TEX_REFL_BWRIPLES01         "Models\\ReflectionTextures\\BWRiples01.tex",
201 texture TEX_REFL_BWRIPLES02         "Models\\ReflectionTextures\\BWRiples02.tex",
202 texture TEX_REFL_LIGHTMETAL01       "Models\\ReflectionTextures\\LightMetal01.tex",
203 texture TEX_REFL_LIGHTBLUEMETAL01   "Models\\ReflectionTextures\\LightBlueMetal01.tex",
204 texture TEX_REFL_DARKMETAL          "Models\\ReflectionTextures\\DarkMetal.tex",
205 texture TEX_REFL_PURPLE01           "Models\\ReflectionTextures\\Purple01.tex",

// ************** SPECULAR **************
210 texture TEX_SPEC_WEAK               "Models\\SpecularTextures\\Weak.tex",
211 texture TEX_SPEC_MEDIUM             "Models\\SpecularTextures\\Medium.tex",
212 texture TEX_SPEC_STRONG             "Models\\SpecularTextures\\Strong.tex",

// ************** SOUNDS **************
213 sound SOUND_PICK             "Sounds\\Items\\Ammo.wav",
214 sound SOUND_DEFAULT          "Sounds\\Default.wav",

functions:
  void Precache(void) {
    PrecacheSound(SOUND_PICK);
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
      case AIT_SHELLS:
        Particles_Spiral(this, 1.0f*0.75f, 1.0f*0.75f, PT_STAR04, 4);
        break;
      case AIT_BULLETS:
        Particles_Spiral(this, 1.5f*0.75f, 1.0f*0.75f, PT_STAR04, 6);
        break;
      case AIT_ROCKETS:
        Particles_Spiral(this, 1.5f*0.75f, 1.25f*0.75f, PT_STAR04, 6);
        break;
      case AIT_GRENADES:
        Particles_Spiral(this, 2.0f*0.75f, 1.25f*0.75f, PT_STAR04, 6);
        break;
      case AIT_ELECTRICITY:
        Particles_Spiral(this, 1.5f*0.75f, 1.125f*0.75f, PT_STAR04, 6);
        break;
      case AIT_NUKEBALL:
        Particles_Spiral(this, 1.25f*0.75f, 1.0f*0.75f, PT_STAR04, 4);
        break;
      case AIT_IRONBALLS:
        Particles_Spiral(this, 2.0f*0.75f, 1.25f*0.75f, PT_STAR04, 8);
        break;
      case AIT_BACKPACK:
        Particles_Spiral(this, 3.0f*0.5f, 2.5f*0.5f, PT_STAR04, 10);
        break;
       case AIT_SERIOUSPACK:
        Particles_Spiral(this, 3.0f*0.5f, 2.5f*0.5f, PT_STAR04, 10);
        break;
       case AIT_NAPALM:
        Particles_Spiral(this, 3.0f*0.5f, 2.5f*0.5f, PT_STAR04, 10);
        break;
       case AIT_SNIPERBULLETS:
        Particles_Spiral(this, 1.5f*0.75f, 1.25f*0.75f, PT_STAR04, 6);
        break;
    }
  }

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    pes->es_ctCount = 1;
    pes->es_ctAmmount = (INDEX) m_fValue;
    switch (m_EaitType) {
      case AIT_SHELLS:      
        pes->es_strName = "Shells"; 
        pes->es_fValue = m_fValue*AV_SHELLS;
        break;
      case AIT_BULLETS:     
        pes->es_strName = "Bullets"; 
        pes->es_fValue = m_fValue*AV_BULLETS;
        break;
      case AIT_ROCKETS:     
        pes->es_strName = "Rockets"; 
        pes->es_fValue = m_fValue*AV_ROCKETS;
        break;
      case AIT_GRENADES:    
        pes->es_strName = "Grenades"; 
        pes->es_fValue = m_fValue*AV_GRENADES;
        break;
      case AIT_ELECTRICITY: 
        pes->es_strName = "Electricity"; 
        pes->es_fValue = m_fValue*AV_ELECTRICITY;
        break;
/*
      case AIT_NUKEBALL:  
        pes->es_strName = "Nukeballs"; 
        pes->es_fValue = m_fValue*AV_NUKEBALLS;
        break;
        */
      case AIT_IRONBALLS: 
        pes->es_strName = "Ironballs"; 
        pes->es_fValue = m_fValue*AV_IRONBALLS;
        break;
      case AIT_SERIOUSPACK: 
        pes->es_strName = "SeriousPack"; 
        pes->es_fValue = m_fValue*100000;
        break;
      case AIT_BACKPACK: 
        pes->es_strName = "BackPack"; 
        pes->es_fValue = m_fValue*100000;
        break;
      case AIT_NAPALM:
        pes->es_strName = "Napalm"; 
        pes->es_fValue = m_fValue*AV_NAPALM;
        break;
      case AIT_SNIPERBULLETS:
        pes->es_strName = "Sniper bullets"; 
        pes->es_fValue = m_fValue*AV_SNIPERBULLETS;
        break;
    }
    pes->es_iScore = 0;//m_iScore;
    return TRUE;
  }


  // set ammo properties depending on ammo type
  void SetProperties(void)
  {
    switch (m_EaitType) {
      case AIT_SHELLS:
        m_fValue = 10.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Shells: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_SHELLS, TEXTURE_SHELLS, 0, 0, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.25f,0), FLOAT3D(1.5,1.5,0.75f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
      case AIT_BULLETS:
        m_fValue = 50.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Bullets: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_BULLETS, TEXTURE_BULLETS, 0, 0, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(3,3,1.0f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
      case AIT_ROCKETS:
        m_fValue = 5.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Rockets: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_ROCKETS, TEXTURE_ROCKET, 0, 0, 0);
        AddItemAttachment(ROCKETS_ATTACHMENT_ROCKET1, MODEL_RC_ROCKET, TEXTURE_ROCKET, 0, 0, 0);
        AddItemAttachment(ROCKETS_ATTACHMENT_ROCKET2, MODEL_RC_ROCKET, TEXTURE_ROCKET, 0, 0, 0);
        AddItemAttachment(ROCKETS_ATTACHMENT_ROCKET3, MODEL_RC_ROCKET, TEXTURE_ROCKET, 0, 0, 0);
        AddItemAttachment(ROCKETS_ATTACHMENT_ROCKET4, MODEL_RC_ROCKET, TEXTURE_ROCKET, 0, 0, 0);
        AddItemAttachment(ROCKETS_ATTACHMENT_ROCKET5, MODEL_RC_ROCKET, TEXTURE_ROCKET, 0, 0, 0);
        SetItemAttachmentAnim(ROCKETS_ATTACHMENT_ROCKET1, ROCKET_ANIM_FORAMMO);
        SetItemAttachmentAnim(ROCKETS_ATTACHMENT_ROCKET2, ROCKET_ANIM_FORAMMO);
        SetItemAttachmentAnim(ROCKETS_ATTACHMENT_ROCKET3, ROCKET_ANIM_FORAMMO);
        SetItemAttachmentAnim(ROCKETS_ATTACHMENT_ROCKET4, ROCKET_ANIM_FORAMMO);
        SetItemAttachmentAnim(ROCKETS_ATTACHMENT_ROCKET5, ROCKET_ANIM_FORAMMO);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(2,2,0.75f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
      case AIT_GRENADES:
        m_fValue = 5.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Grenades: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_GRENADES, TEXTURE_GRENADES, 0, 0, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(4,4,1.0f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
      case AIT_ELECTRICITY:
        m_fValue = 50.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Electricity: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_ELECTRICITY, TEXTURE_ELECTRICITY, TEXTURE_EL_EFFECT, TEXTURE_EL_EFFECT, 0);
        AddItemAttachment(ELECTRICITY_ATTACHMENT_EFFECT1, MODEL_EL_EFFECT, TEXTURE_EL_EFFECT, 0, 0, 0);
        AddItemAttachment(ELECTRICITY_ATTACHMENT_EFFECT2, MODEL_EL_EFFECT, TEXTURE_EL_EFFECT, 0, 0, 0);
        AddItemAttachment(ELECTRICITY_ATTACHMENT_EFFECT3, MODEL_EL_EFFECT2,TEXTURE_EL_EFFECT, 0, 0, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(3,3,0.8f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
/*
      case AIT_NUKEBALL:
        m_fValue = 1.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Nuke ball: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_CANNONBALL, TEXTURE_NUKEBALL, TEX_REFL_BWRIPLES01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.5f,0), FLOAT3D(2,2,0.5f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
        */
      case AIT_IRONBALLS:
        m_fValue = 4.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Iron balls: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_CANNONBALLS, TEXTURE_IRONBALL, TEX_REFL_DARKMETAL, TEX_SPEC_WEAK, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(5,5,1.3f) );
        StretchItem(FLOAT3D(0.75f, 0.75f, 0.75f));
        break;
      case AIT_NAPALM:
        m_fValue = 100.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Napalm: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_FL_RESERVOIR, TEXTURE_FL_FUELRESERVOIR, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(3,3,1.0f) );
        StretchItem(FLOAT3D(1.25f, 1.25f, 1.25f));
        break;
      case AIT_SERIOUSPACK:
        m_fValue = 1.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("SeriousPack: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_SERIOUSPACK, TEXTURE_SERIOUSPACK, 0,0,0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(2,2,1.3f) );
        StretchItem(FLOAT3D(0.5f, 0.5f, 0.5f));
        break;
      case AIT_BACKPACK:
        m_fValue = 1.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("BackPack: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_BACKPACK, TEXTURE_BACKPACK, 0,0,0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(2,2,1.3f) );
        StretchItem(FLOAT3D(0.5f, 0.5f, 0.5f));
        break;
      case AIT_SNIPERBULLETS:
        m_fValue = 5.0f;
        m_fRespawnTime = (m_fCustomRespawnTime>0) ? m_fCustomRespawnTime : 30.0f; 
        m_strDescription.PrintF("Sniper bullets: %d", (int) m_fValue);
        // set appearance
        AddItem(MODEL_SNIPER_BULLETS, TEXTURE_SNIPER_BULLETS, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.75f,0), FLOAT3D(3,3,1.0f) );
        StretchItem(FLOAT3D(1.25f, 1.25f, 1.25f));
        break;
      default: ASSERTALWAYS("Uknown ammo");
    }
  };

  void AdjustDifficulty(void)
  {
    m_fValue = ceil(m_fValue*GetSP()->sp_fAmmoQuantity);

    if (GetSP()->sp_bInfiniteAmmo && m_penTarget==NULL) {
      Destroy();
    }
  }

procedures:
  ItemCollected(EPass epass) : CItem::ItemCollected {
    ASSERT(epass.penOther!=NULL);

    // if ammo stays
    if (GetSP()->sp_bAmmoStays && !(m_bPickupOnce||m_bRespawn)) {
      // if already picked by this player
      BOOL bWasPicked = MarkPickedBy(epass.penOther);
      if (bWasPicked) {
        // don't pick again
        return;
      }
    }

    // send ammo to entity
    EAmmoItem eAmmo;
    eAmmo.EaitType = m_EaitType;
    eAmmo.iQuantity = (INDEX) m_fValue;
    // if health is received
    if (epass.penOther->ReceiveItem(eAmmo)) {
      // play the pickup sound
      m_soPick.Set3DParameters(50.0f, 1.0f, 1.0f, 1.0f);
      if(_pNetwork->IsPlayerLocal(epass.penOther)) {IFeel_PlayEffect("PU_Ammo");}
      if( (m_EaitType == AIT_SERIOUSPACK) || (m_EaitType == AIT_BACKPACK) )
      {
        PlaySound(m_soPick, SOUND_DEFAULT, SOF_3D);
        CPrintF("^cFF0000^f5Warning!!! Replace old serious pack with new, BackPack entity!^r\n");
      }
      else
      {
        PlaySound(m_soPick, SOUND_PICK, SOF_3D);
      }
      m_fPickSoundLen = GetSoundLength(SOUND_PICK);
      if (!GetSP()->sp_bAmmoStays || (m_bPickupOnce||m_bRespawn)) {
        jump CItem::ItemReceived();
      }
    }
    return;
  };

  Main() {
    if (m_EaitType==AIT_NUKEBALL /*|| m_EaitType==AIT_NAPALM*/) {
      m_EaitType=AIT_SHELLS;
    }
    Initialize();     // initialize base class
    StartModelAnim(ITEMHOLDER_ANIM_MEDIUMOSCILATION, AOF_LOOPING|AOF_NORESTART);
    ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_MEDIUM);
    SetProperties();  // set properties

    jump CItem::ItemLoop();
  };
};
