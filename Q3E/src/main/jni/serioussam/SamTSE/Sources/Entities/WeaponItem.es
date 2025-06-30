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

802
%{
#include "Entities/StdH/StdH.h"
#include "Models/Items/ItemHolder/ItemHolder.h"
#include "Models/Weapons/Colt/ColtItem.h"
#include "Models/Weapons/SingleShotgun/SingleShotgunItem.h"
#include "Models/Weapons/DoubleShotgun/DoubleShotgunItem.h"
#include "Models/Weapons/TommyGun/TommyGunItem.h"
#include "Models/Weapons/MiniGun/MiniGunItem.h"
#include "Models/Weapons/GrenadeLauncher/GrenadeLauncherItem.h"
#include "Models/Weapons/RocketLauncher/RocketLauncherItem.h"
//#include "Models/Weapons/Pipebomb/StickItem.h"
//#include "Models/Weapons/Flamer/FlamerItem.h"
#include "Models/Weapons/Laser/LaserItem.h"
//#include "Models/Weapons/GhostBuster/GhostBusterItem.h"
//#include "Models/Weapons/GhostBuster/Effect01.h"
#include "Models/Weapons/Cannon/Cannon.h"

#include "Entities/PlayerWeapons.h"
%}

uses "Entities/Item";

// health type 
enum WeaponItemType {
  1 WIT_COLT              "Colt",
  2 WIT_SINGLESHOTGUN     "Single shotgun",
  3 WIT_DOUBLESHOTGUN     "Double shotgun",
  4 WIT_TOMMYGUN          "Tommygun",
  5 WIT_MINIGUN           "Minigun",
  6 WIT_ROCKETLAUNCHER    "Rocket launcher",
  7 WIT_GRENADELAUNCHER   "Grenade launcher",
  8 WIT_PIPEBOMB          "obsolete",
  9 WIT_FLAMER            "obsolete",
 10 WIT_LASER             "Laser",
 11 WIT_GHOSTBUSTER       "obsolete",
 12 WIT_CANNON            "Cannon",
};

// event for sending through receive item
event EWeaponItem {
  INDEX  iWeapon,   // weapon collected
  INDEX  iAmmo,     // weapon ammo (used only for leaving weapons, -1 for deafult ammount)
  BOOL bDropped,    // for dropped weapons (can be picked even if weapons stay)
};

%{
extern void CPlayerWeapons_Precache(ULONG ulAvailable);
%}


class CWeaponItem : CItem {
name      "Weapon Item";
thumbnail "Thumbnails\\WeaponItem.tbn";

properties:
  1 enum WeaponItemType m_EwitType    "Type" 'Y' = WIT_COLT,     // weapon

components:
  0 class   CLASS_BASE        "Classes\\Item.ecl",

// ************** COLT **************
 30 model   MODEL_COLT                  "Models\\Weapons\\Colt\\ColtItem.mdl",
 31 model   MODEL_COLTCOCK              "Models\\Weapons\\Colt\\ColtCock.mdl",
 32 model   MODEL_COLTMAIN              "Models\\Weapons\\Colt\\ColtMain.mdl",
 33 model   MODEL_COLTBULLETS           "Models\\Weapons\\Colt\\ColtBullets.mdl",
 34 texture TEXTURE_COLTMAIN            "Models\\Weapons\\Colt\\ColtMain.tex",
 35 texture TEXTURE_COLTCOCK            "Models\\Weapons\\Colt\\ColtCock.tex",
 36 texture TEXTURE_COLTBULLETS         "Models\\Weapons\\Colt\\ColtBullets.tex",

// ************** SINGLE SHOTGUN ************
 40 model   MODEL_SINGLESHOTGUN         "Models\\Weapons\\SingleShotgun\\SingleShotgunItem.mdl",
 41 model   MODEL_SS_SLIDER             "Models\\Weapons\\SingleShotgun\\Slider.mdl",
 42 model   MODEL_SS_HANDLE             "Models\\Weapons\\SingleShotgun\\Handle.mdl",
 43 model   MODEL_SS_BARRELS            "Models\\Weapons\\SingleShotgun\\Barrels.mdl",
 44 texture TEXTURE_SS_HANDLE           "Models\\Weapons\\SingleShotgun\\Handle.tex",
 45 texture TEXTURE_SS_BARRELS          "Models\\Weapons\\SingleShotgun\\Barrels.tex",

// ************** DOUBLE SHOTGUN **************
 50 model   MODEL_DOUBLESHOTGUN         "Models\\Weapons\\DoubleShotgun\\DoubleShotgunItem.mdl",
 51 model   MODEL_DS_HANDLE             "Models\\Weapons\\DoubleShotgun\\Dshotgunhandle.mdl",
 52 model   MODEL_DS_BARRELS            "Models\\Weapons\\DoubleShotgun\\Dshotgunbarrels.mdl",
 54 model   MODEL_DS_SWITCH             "Models\\Weapons\\DoubleShotgun\\Switch.mdl",
 56 texture TEXTURE_DS_HANDLE           "Models\\Weapons\\DoubleShotgun\\Handle.tex",
 57 texture TEXTURE_DS_BARRELS          "Models\\Weapons\\DoubleShotgun\\Barrels.tex",
 58 texture TEXTURE_DS_SWITCH           "Models\\Weapons\\DoubleShotgun\\Switch.tex",

// ************** TOMMYGUN **************
 70 model   MODEL_TOMMYGUN              "Models\\Weapons\\TommyGun\\TommyGunItem.mdl",
 71 model   MODEL_TG_BODY               "Models\\Weapons\\TommyGun\\Body.mdl",
 72 model   MODEL_TG_SLIDER             "Models\\Weapons\\TommyGun\\Slider.mdl",
 73 texture TEXTURE_TG_BODY             "Models\\Weapons\\TommyGun\\Body.tex",

// ************** MINIGUN **************
 80 model   MODEL_MINIGUN               "Models\\Weapons\\MiniGun\\MiniGunItem.mdl",
 81 model   MODEL_MG_BARRELS            "Models\\Weapons\\MiniGun\\Barrels.mdl",
 82 model   MODEL_MG_BODY               "Models\\Weapons\\MiniGun\\Body.mdl",
 83 model   MODEL_MG_ENGINE             "Models\\Weapons\\MiniGun\\Engine.mdl",
 84 texture TEXTURE_MG_BODY             "Models\\Weapons\\MiniGun\\Body.tex",
 99 texture TEXTURE_MG_BARRELS          "Models\\Weapons\\MiniGun\\Barrels.tex",

// ************** ROCKET LAUNCHER **************
 90 model   MODEL_ROCKETLAUNCHER        "Models\\Weapons\\RocketLauncher\\RocketLauncherItem.mdl",
 91 model   MODEL_RL_BODY               "Models\\Weapons\\RocketLauncher\\Body.mdl",
 92 texture TEXTURE_RL_BODY             "Models\\Weapons\\RocketLauncher\\Body.tex",
 93 model   MODEL_RL_ROTATINGPART       "Models\\Weapons\\RocketLauncher\\RotatingPart.mdl",
 94 texture TEXTURE_RL_ROTATINGPART     "Models\\Weapons\\RocketLauncher\\RotatingPart.tex",
 95 model   MODEL_RL_ROCKET             "Models\\Weapons\\RocketLauncher\\Projectile\\Rocket.mdl",
 96 texture TEXTURE_RL_ROCKET           "Models\\Weapons\\RocketLauncher\\Projectile\\Rocket.tex",

// ************** GRENADE LAUNCHER **************
100 model   MODEL_GRENADELAUNCHER       "Models\\Weapons\\GrenadeLauncher\\GrenadeLauncherItem.mdl",
101 model   MODEL_GL_BODY               "Models\\Weapons\\GrenadeLauncher\\Body.mdl",
102 model   MODEL_GL_MOVINGPART         "Models\\Weapons\\GrenadeLauncher\\MovingPipe.mdl",
103 model   MODEL_GL_GRENADE            "Models\\Weapons\\GrenadeLauncher\\GrenadeBack.mdl",
104 texture TEXTURE_GL_BODY             "Models\\Weapons\\GrenadeLauncher\\Body.tex",
105 texture TEXTURE_GL_MOVINGPART       "Models\\Weapons\\GrenadeLauncher\\MovingPipe.tex",

/*
// ************** PIPEBOMB **************
110 model   MODEL_PIPEBOMB_STICK        "Models\\Weapons\\Pipebomb\\StickItem.mdl",
112 model   MODEL_PB_BUTTON             "Models\\Weapons\\Pipebomb\\Button.mdl",
113 model   MODEL_PB_SHIELD             "Models\\Weapons\\Pipebomb\\Shield.mdl",
114 model   MODEL_PB_STICK              "Models\\Weapons\\Pipebomb\\Stick.mdl",
116 texture TEXTURE_PB_STICK            "Models\\Weapons\\Pipebomb\\Stick.tex",

// ************** FLAMER **************
130 model   MODEL_FLAMER                "Models\\Weapons\\Flamer\\FlamerItem.mdl",
131 model   MODEL_FL_BODY               "Models\\Weapons\\Flamer\\Body.mdl",
132 model   MODEL_FL_RESERVOIR          "Models\\Weapons\\Flamer\\FuelReservoir.mdl",
133 model   MODEL_FL_FLAME              "Models\\Weapons\\Flamer\\Flame.mdl",
134 texture TEXTURE_FL_BODY             "Models\\Weapons\\Flamer\\Body.tex",
135 texture TEXTURE_FL_FLAME            "Models\\Weapons\\Flamer\\Flame.tex",
136 texture TEXTURE_FL_FUELRESERVOIR    "Models\\Weapons\\Flamer\\FuelReservoir.tex",
*/
// ************** LASER **************
140 model   MODEL_LASER                 "Models\\Weapons\\Laser\\LaserItem.mdl",
141 model   MODEL_LS_BODY               "Models\\Weapons\\Laser\\Body.mdl",
142 model   MODEL_LS_BARREL             "Models\\Weapons\\Laser\\Barrel.mdl",
143 texture TEXTURE_LS_BODY             "Models\\Weapons\\Laser\\Body.tex",
144 texture TEXTURE_LS_BARREL           "Models\\Weapons\\Laser\\Barrel.tex",
/*
// ************** GHOSTBUSTER **************
150 model   MODEL_GHOSTBUSTER           "Models\\Weapons\\GhostBuster\\GhostBusterItem.mdl",
151 model   MODEL_GB_BODY               "Models\\Weapons\\GhostBuster\\Body.mdl",
152 model   MODEL_GB_ROTATOR            "Models\\Weapons\\GhostBuster\\Rotator.mdl",
153 model   MODEL_GB_EFFECT1            "Models\\Weapons\\GhostBuster\\Effect01.mdl",
154 model   MODEL_GB_EFFECT1FLARE       "Models\\Weapons\\GhostBuster\\EffectFlare01.mdl",
155 texture TEXTURE_GB_ROTATOR          "Models\\Weapons\\GhostBuster\\Rotator.tex",
156 texture TEXTURE_GB_BODY             "Models\\Weapons\\GhostBuster\\Body.tex",
157 texture TEXTURE_GB_LIGHTNING        "Models\\Weapons\\GhostBuster\\Lightning.tex",
158 texture TEXTURE_GB_FLARE            "Models\\Weapons\\GhostBuster\\EffectFlare.tex",
*/
// ************** CANNON **************
170 model   MODEL_CANNON                "Models\\Weapons\\Cannon\\Cannon.mdl",
171 model   MODEL_CN_BODY               "Models\\Weapons\\Cannon\\Body.mdl",
173 texture TEXTURE_CANNON              "Models\\Weapons\\Cannon\\Body.tex",
//174 model   MODEL_CN_NUKEBOX            "Models\\Weapons\\Cannon\\NukeBox.mdl",
//175 model   MODEL_CN_LIGHT              "Models\\Weapons\\Cannon\\Light.mdl",

// ************** FLARE FOR EFFECT **************
190 texture TEXTURE_FLARE "Models\\Items\\Flares\\Flare.tex",
191 model   MODEL_FLARE "Models\\Items\\Flares\\Flare.mdl",

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
213 sound SOUND_PICK             "Sounds\\Items\\Weapon.wav",

functions:
  void Precache(void) {
    PrecacheSound(SOUND_PICK);
    switch (m_EwitType) {
      case WIT_COLT:            CPlayerWeapons_Precache(1<<(INDEX(WEAPON_COLT           )-1)); break;
      case WIT_SINGLESHOTGUN:   CPlayerWeapons_Precache(1<<(INDEX(WEAPON_SINGLESHOTGUN  )-1)); break;
      case WIT_DOUBLESHOTGUN:   CPlayerWeapons_Precache(1<<(INDEX(WEAPON_DOUBLESHOTGUN  )-1)); break;
      case WIT_TOMMYGUN:        CPlayerWeapons_Precache(1<<(INDEX(WEAPON_TOMMYGUN       )-1)); break;
      case WIT_MINIGUN:         CPlayerWeapons_Precache(1<<(INDEX(WEAPON_MINIGUN        )-1)); break;
      case WIT_ROCKETLAUNCHER:  CPlayerWeapons_Precache(1<<(INDEX(WEAPON_ROCKETLAUNCHER )-1)); break;
      case WIT_GRENADELAUNCHER: CPlayerWeapons_Precache(1<<(INDEX(WEAPON_GRENADELAUNCHER)-1)); break;
//      case WIT_PIPEBOMB:        CPlayerWeapons_Precache(1<<(INDEX(WEAPON_PIPEBOMB       )-1)); break;
//      case WIT_FLAMER:          CPlayerWeapons_Precache(1<<(INDEX(WEAPON_FLAMER         )-1)); break;
      case WIT_LASER:           CPlayerWeapons_Precache(1<<(INDEX(WEAPON_LASER          )-1)); break;
//      case WIT_GHOSTBUSTER:     CPlayerWeapons_Precache(1<<(INDEX(WEAPON_GHOSTBUSTER    )-1)); break;
      case WIT_CANNON:          CPlayerWeapons_Precache(1<<(INDEX(WEAPON_IRONCANNON     )-1)); break;
    }
  }
  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    pes->es_strName = m_strDescription; 
    pes->es_ctCount = 1;
    pes->es_ctAmmount = 1;
    pes->es_fValue = 1;
    pes->es_iScore = 0;//m_iScore;
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
    switch (m_EwitType) {
      case WIT_COLT:             Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_SINGLESHOTGUN:    Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_DOUBLESHOTGUN:    Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_TOMMYGUN:         Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_MINIGUN:          Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_ROCKETLAUNCHER:   Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_GRENADELAUNCHER:  Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_PIPEBOMB:         Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_FLAMER:           Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_LASER:            Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_GHOSTBUSTER:      Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
      case WIT_CANNON:           Particles_Atomic(this, 1.5f, 1.5f, PT_STAR07, 12);  break;
    }
  }


  // set weapon properties depending on weapon type
  void SetProperties(void)
  {
    BOOL bDM = FALSE;//m_bRespawn || m_bDropped;
    FLOAT3D vDMStretch = FLOAT3D( 2.0f, 2.0f, 2.0f);
    
    switch (m_EwitType) {
    // *********** COLT ***********
      case WIT_COLT:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Colt");
        AddItem(MODEL_COLT, TEXTURE_COLTMAIN, 0, 0, 0);
        AddItemAttachment(COLTITEM_ATTACHMENT_BULLETS, MODEL_COLTBULLETS, TEXTURE_COLTBULLETS, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(COLTITEM_ATTACHMENT_COCK, MODEL_COLTCOCK, TEXTURE_COLTCOCK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(COLTITEM_ATTACHMENT_BODY, MODEL_COLTMAIN, TEXTURE_COLTMAIN, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ?  vDMStretch : FLOAT3D(4.5f, 4.5f, 4.5f));
        break;

    // *********** SINGLE SHOTGUN ***********
      case WIT_SINGLESHOTGUN:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Single Shotgun");
        AddItem(MODEL_SINGLESHOTGUN, TEXTURE_SS_HANDLE, 0, 0, 0);
        AddItemAttachment(SINGLESHOTGUNITEM_ATTACHMENT_BARRELS, MODEL_SS_BARRELS, TEXTURE_SS_BARRELS, TEX_REFL_DARKMETAL, TEX_SPEC_WEAK, 0);
        AddItemAttachment(SINGLESHOTGUNITEM_ATTACHMENT_HANDLE, MODEL_SS_HANDLE, TEXTURE_SS_HANDLE, TEX_REFL_DARKMETAL, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(SINGLESHOTGUNITEM_ATTACHMENT_SLIDER, MODEL_SS_SLIDER, TEXTURE_SS_BARRELS, TEX_REFL_DARKMETAL, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(3.5f, 3.5f, 3.5f)) );
        break;

    // *********** DOUBLE SHOTGUN ***********
      case WIT_DOUBLESHOTGUN:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Double Shotgun");
        AddItem(MODEL_DOUBLESHOTGUN, TEXTURE_DS_HANDLE, 0, 0, 0);
        AddItemAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_BARRELS, MODEL_DS_BARRELS, TEXTURE_DS_BARRELS, TEX_REFL_BWRIPLES01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_HANDLE, MODEL_DS_HANDLE, TEXTURE_DS_HANDLE, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_SWITCH, MODEL_DS_SWITCH, TEXTURE_DS_SWITCH, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(3.0f, 3.0f, 3.0f)));
        break;


    // *********** TOMMYGUN ***********
      case WIT_TOMMYGUN:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Tommygun");
        AddItem(MODEL_TOMMYGUN, TEXTURE_TG_BODY, 0, 0, 0);
        AddItemAttachment(TOMMYGUNITEM_ATTACHMENT_BODY, MODEL_TG_BODY, TEXTURE_TG_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(TOMMYGUNITEM_ATTACHMENT_SLIDER, MODEL_TG_SLIDER, TEXTURE_TG_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(3.0f, 3.0f, 3.0f)));
        break;

    // *********** MINIGUN ***********
      case WIT_MINIGUN:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Minigun");
        AddItem(MODEL_MINIGUN, TEXTURE_MG_BODY, 0, 0, 0);
        AddItemAttachment(MINIGUNITEM_ATTACHMENT_BARRELS, MODEL_MG_BARRELS, TEXTURE_MG_BARRELS, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(MINIGUNITEM_ATTACHMENT_BODY, MODEL_MG_BODY, TEXTURE_MG_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(MINIGUNITEM_ATTACHMENT_ENGINE, MODEL_MG_ENGINE, TEXTURE_MG_BARRELS, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(1.75f, 1.75f, 1.75f)));
        break;

    // *********** ROCKET LAUNCHER ***********
      case WIT_ROCKETLAUNCHER:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Rocket launcher");
        AddItem(MODEL_ROCKETLAUNCHER, TEXTURE_RL_BODY, 0, 0, 0);
        AddItemAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_BODY, MODEL_RL_BODY, TEXTURE_RL_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROTATINGPART, MODEL_RL_ROTATINGPART, TEXTURE_RL_ROTATINGPART, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET1, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET2, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET3, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET4, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(2.5f, 2.5f, 2.5f)));
        break;

    // *********** GRENADE LAUNCHER ***********
      case WIT_GRENADELAUNCHER:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Grenade launcher");
        AddItem(MODEL_GRENADELAUNCHER, TEXTURE_GL_BODY, 0, 0, 0);
        AddItemAttachment(GRENADELAUNCHERITEM_ATTACHMENT_BODY, MODEL_GL_BODY, TEXTURE_GL_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);                   
        AddItemAttachment(GRENADELAUNCHERITEM_ATTACHMENT_MOVING_PART, MODEL_GL_MOVINGPART, TEXTURE_GL_MOVINGPART, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(GRENADELAUNCHERITEM_ATTACHMENT_GRENADE, MODEL_GL_GRENADE, TEXTURE_GL_MOVINGPART, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);       
        StretchItem( bDM ? vDMStretch : (FLOAT3D(2.5f, 2.5f, 2.5f)));
        break;

/*
    // *********** PIPEBOMB ***********
      case WIT_PIPEBOMB:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Pipebomb");
        AddItem(MODEL_PIPEBOMB_STICK, TEXTURE_PB_STICK, 0, 0, 0);
        AddItemAttachment(STICKITEM_ATTACHMENT_STICK, MODEL_PB_STICK,
                          TEXTURE_PB_STICK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(STICKITEM_ATTACHMENT_SHIELD, MODEL_PB_SHIELD,
                          TEXTURE_PB_STICK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(STICKITEM_ATTACHMENT_BUTTON, MODEL_PB_BUTTON,
                          TEXTURE_PB_STICK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(6.0f, 6.0f, 6.0f)));
        break;

    // *********** FLAMER ***********
      case WIT_FLAMER:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Flamer");
        AddItem(MODEL_FLAMER, TEXTURE_FL_BODY, 0, 0, 0);
        AddItemAttachment(FLAMERITEM_ATTACHMENT_BODY, MODEL_FL_BODY,
                          TEXTURE_FL_BODY, TEX_REFL_BWRIPLES02, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(FLAMERITEM_ATTACHMENT_FUEL, MODEL_FL_RESERVOIR,
                          TEXTURE_FL_FUELRESERVOIR, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(FLAMERITEM_ATTACHMENT_FLAME, MODEL_FL_FLAME,
                          TEXTURE_FL_FLAME, 0, 0, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(2.5f, 2.5f, 2.5f)));
        break;
*/
    // *********** LASER ***********
      case WIT_LASER:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("Laser");
        AddItem(MODEL_LASER, TEXTURE_LS_BODY, 0, 0, 0);
        AddItemAttachment(LASERITEM_ATTACHMENT_BODY, MODEL_LS_BODY, TEXTURE_LS_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);         
        AddItemAttachment(LASERITEM_ATTACHMENT_LEFTUP,    MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(LASERITEM_ATTACHMENT_LEFTDOWN,  MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(LASERITEM_ATTACHMENT_RIGHTUP,   MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(LASERITEM_ATTACHMENT_RIGHTDOWN, MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(2.5f, 2.5f, 2.5f)));
        break;
/*
    // *********** GHOSTBUSTER ***********
      case WIT_GHOSTBUSTER:
        m_fRespawnTime = 10.0f;
        m_strDescription.PrintF("GhostBuster");
        AddItem(MODEL_GHOSTBUSTER, TEXTURE_GB_BODY, 0, 0, 0);
        AddItemAttachment(GHOSTBUSTERITEM_ATTACHMENT_BODY, MODEL_GB_BODY, TEXTURE_GB_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(GHOSTBUSTERITEM_ATTACHMENT_ROTATOR, MODEL_GB_ROTATOR, TEXTURE_GB_ROTATOR, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddItemAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT01, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        AddItemAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT02, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        AddItemAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT03, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        AddItemAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT04, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        CModelObject *pmoMain, *pmo;
        pmoMain = &(GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_ITEM)->amo_moModelObject);
        pmo = &(pmoMain->GetAttachmentModel(GHOSTBUSTERITEM_ATTACHMENT_EFFECT01)->amo_moModelObject);
        AddAttachmentToModel(this, *pmo, EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        pmo = &(pmoMain->GetAttachmentModel(GHOSTBUSTERITEM_ATTACHMENT_EFFECT02)->amo_moModelObject);
        AddAttachmentToModel(this, *pmo, EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        pmo = &(pmoMain->GetAttachmentModel(GHOSTBUSTERITEM_ATTACHMENT_EFFECT03)->amo_moModelObject);
        AddAttachmentToModel(this, *pmo, EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        pmo = &(pmoMain->GetAttachmentModel(GHOSTBUSTERITEM_ATTACHMENT_EFFECT04)->amo_moModelObject);
        AddAttachmentToModel(this, *pmo, EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(3.25f, 3.25f, 3.25f)));
        break;
        */

    // *********** CANNON ***********
      case WIT_CANNON:
        m_fRespawnTime = 30.0f;
        m_strDescription.PrintF("Cannon");
        AddItem(MODEL_CANNON, TEXTURE_CANNON, 0, 0, 0);
        AddItemAttachment(CANNON_ATTACHMENT_BODY, MODEL_CN_BODY, TEXTURE_CANNON, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
//        AddItemAttachment(CANNON_ATTACHMENT_NUKEBOX, MODEL_CN_NUKEBOX, TEXTURE_CANNON, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
//        AddItemAttachment(CANNON_ATTACHMENT_LIGHT, MODEL_CN_LIGHT, TEXTURE_CANNON, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        StretchItem( bDM ? vDMStretch : (FLOAT3D(3.0f, 3.0f, 3.0f)));
        break;
    }
      // add flare
    AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.6f,0), FLOAT3D(3,3,0.3f) );
};

procedures:
  ItemCollected(EPass epass) : CItem::ItemCollected {
    ASSERT(epass.penOther!=NULL);

    // if weapons stays
    if (GetSP()->sp_bWeaponsStay && !m_bPickupOnce) {
      // if already picked by this player
      BOOL bWasPicked = MarkPickedBy(epass.penOther);
      if (bWasPicked) {
        // don't pick again
        return;
      }
    }

    // send weapon to entity
    EWeaponItem eWeapon;
    eWeapon.iWeapon = m_EwitType;
    eWeapon.iAmmo = -1; // use default ammo amount
    eWeapon.bDropped = m_bDropped;
    // if weapon is received
    if (epass.penOther->ReceiveItem(eWeapon)) {
      // play the pickup sound
      if(_pNetwork->IsPlayerLocal(epass.penOther)) {IFeel_PlayEffect("PU_Weapon");}
      m_soPick.Set3DParameters(50.0f, 1.0f, 1.0f, 1.0f);
      PlaySound(m_soPick, SOUND_PICK, SOF_3D);
      m_fPickSoundLen = GetSoundLength(SOUND_PICK);
      if (!GetSP()->sp_bWeaponsStay || m_bDropped || m_bPickupOnce) {
        jump CItem::ItemReceived();
      }
    }
    return;
  };

  Main()
  {
    if (m_EwitType==WIT_PIPEBOMB||
        m_EwitType==WIT_FLAMER||
        m_EwitType==WIT_GHOSTBUSTER) {
      m_EwitType=WIT_LASER;
    }

    Initialize();     // initialize base class
    StartModelAnim(ITEMHOLDER_ANIM_BIGOSCILATION, AOF_LOOPING|AOF_NORESTART);
    ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_BIG);
    SetProperties();  // set properties

    if (!m_bDropped) {
      jump CItem::ItemLoop();
    } else if (TRUE) {
      wait() {
        on (EBegin) : {
          SpawnReminder(this, m_fRespawnTime, 0);
          call CItem::ItemLoop();
        }
        on (EReminder) : {
          SendEvent(EEnd()); 
          resume;
        }
      }
    }
  };
};
