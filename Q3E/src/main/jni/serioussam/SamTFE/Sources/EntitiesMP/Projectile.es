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

501
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Weapons/Laser/Projectile/LaserProjectile.h"
#include "EntitiesMP/EnemyBase.h"
//#include "EntitiesMP/Dragonman.h"
#include "Models/Enemies/Elementals/Projectile/IcePyramid.h"
#include "Models/Enemies/ElementalLava/Projectile/LavaStone.h"
#include "Models/Enemies/ElementalLava/Projectile/LavaBomb.h"
#include "Models/Enemies/Headman/Projectile/Blade.h"
#include "Models/Enemies/HuanMan/Projectile/Projectile.h"
#include "Models/Enemies/Cyborg/Projectile/LaserProjectile.h"
  
#include "ModelsMP/Enemies/Grunt/Projectile/GruntProjectile.h"
#include "ModelsMP/Enemies/Guffy/Projectile/GuffyProjectile.h"

#include "ModelsMP/Enemies/ExotechLarva/Weapons/PlasmaGun.h"

#include "EntitiesMP/PlayerWeapons.h"
#include "EntitiesMP/Shooter.h"

#define DEVIL_LASER_SPEED 100.0f
#define DEVIL_ROCKET_SPEED 60.0f
%}

uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Light";
uses "EntitiesMP/Flame";

enum ProjectileType {
  0 PRT_ROCKET                "Rocket",   // player rocket
  1 PRT_GRENADE               "Grenade",   // player grenade
  2 PRT_FLAME                 "Flame",   // player flamer flame
  3 PRT_LASER_RAY             "Laser",   // player laser ray
  4 PRT_WALKER_ROCKET         "WalkerRocket",   // walker rocket

 10 PRT_CATMAN_FIRE           "Catman",   // catman fire

 11 PRT_HEADMAN_FIRECRACKER   "Firecracker",   // headman firecracker
 12 PRT_HEADMAN_ROCKETMAN     "Rocketman",   // headman rocketman
 13 PRT_HEADMAN_BOMBERMAN     "Bomberman",   // headman bomberman

 14 PRT_BONEMAN_FIRE          "Boneman",   // boneman fire

 15 PRT_WOMAN_FIRE            "Woman",   // woman fire

 16 PRT_DRAGONMAN_FIRE        "Dragonman",   // dragonman fire
 17 PRT_DRAGONMAN_STRONG_FIRE "Dragonman Strong",   // dragonman strong fire

 18 PRT_STONEMAN_FIRE         "Stoneman",   // stoneman fire rock
 19 PRT_STONEMAN_BIG_FIRE     "Stoneman Big",   // stoneman big fire rock
 20 PRT_STONEMAN_LARGE_FIRE   "Stoneman Large",   // stoneman large fire rock
 21 PRT_LAVAMAN_BIG_BOMB      "Lavaman Big Bomb",   // lavaman big bomb
 22 PRT_LAVAMAN_BOMB          "Lavaman Bomb",   // lavaman bomb
 23 PRT_LAVAMAN_STONE         "Lavaman Stone",   // lavaman rock projectile
 27 PRT_ICEMAN_FIRE           "Iceman",   // iceman ice cube
 28 PRT_ICEMAN_BIG_FIRE       "Iceman Big",   // iceman big ice cube
 29 PRT_ICEMAN_LARGE_FIRE     "Iceman Large",   // iceman large ice cube

 41 PRT_HUANMAN_FIRE          "Huanman",   // huanman fire

 42 PRT_FISHMAN_FIRE          "Fishman",   // fishman fire

 43 PRT_MANTAMAN_FIRE         "Mantaman",   // mantaman fire

 44 PRT_CYBORG_LASER          "Cyborg Laser",   // cyborg laser
 45 PRT_CYBORG_BOMB           "Cyborg Bomb",   // cyborg bomb

 50 PRT_LAVA_COMET            "Lava Comet",  // lava comet
 51 PRT_BEAST_PROJECTILE      "Beast Projectile",   // beast projectile
 52 PRT_BEAST_BIG_PROJECTILE  "Beast Big Projectile",   // big beast projectile
 53 PRT_BEAST_DEBRIS          "Beast Debris",   // beast projectile's debris
 54 PRT_BEAST_BIG_DEBRIS      "Beast Big Debris",   // big beast projectile's debris
 55 PRT_DEVIL_LASER           "Devil Laser",   // devil laser
 56 PRT_DEVIL_ROCKET          "Devil Rocket",   // devil rocket
 57 PRT_DEVIL_GUIDED_PROJECTILE "Devil Guided Projectile",   // devil guided projectile
 60 PRT_GRUNT_PROJECTILE_SOL  "Grunt Soldier Laser",  // grunt laser
 64 PRT_GRUNT_PROJECTILE_COM  "Grunt Commander Laser", // grunt commander laser          
 61 PRT_GUFFY_PROJECTILE      "Guffy Projectile", // guffy rocket
 62 PRT_DEMON_FIREBALL        "Demon Fireball", // demon fireball
 63 PRT_DEMON_FIREBALL_DEBRIS "Demon Fireball Debris", // demon fireball debris
 70 PRT_SHOOTER_WOODEN_DART   "Shooter Wooden Dart", // shooter's wooden dart
 71 PRT_SHOOTER_FIREBALL      "Shooter Fireball", // shooter's fireball
 72 PRT_SHOOTER_FLAME         "Shooter Flame", // shooter's flame
 73 PRT_LARVA_PLASMA          "ExotechLarva Plasma", //exotech larva plasma gun
 74 PRT_LARVA_TAIL_PROJECTILE "ExotechLarva Tail Projectile", //exotech larva tail projectile
 75 PRT_AIRELEMENTAL_WIND     "Air Elemental Wind Blast", //air elemental wind blast
 76 PRT_AFTERBURNER_DEBRIS    "Afterburner debris",
 77 PRT_METEOR                "Meteor",
};

enum ProjectileMovingType {
  0 PMT_FLYING        "",      // flying through space
  1 PMT_SLIDING       "",      // sliding on floor
  2 PMT_GUIDED        "",      // guided projectile
  3 PMT_GUIDED_FAST    "",     // fast guided projectile
  4 PMT_FLYING_REBOUNDING "",  // flying and rebounding from walls a few times
  5 PMT_GUIDED_SLIDING "",     // sliding on floor and guided at the same time
};


// input parameter for launching the projectile
event ELaunchProjectile {
  CEntityPointer penLauncher,     // who launched it
  enum ProjectileType prtType,    // type of projectile
  FLOAT fSpeed,                   // optional - projectile speed (only for some projectiles)
  FLOAT fStretch,                 // optional - projectile stretch (only for some projectiles)
};


%{
#define DRAGONMAN_NORMAL 0
#define DRAGONMAN_STRONG 1

#define ELEMENTAL_LARGE   2
#define ELEMENTAL_BIG     1
#define ELEMENTAL_NORMAL  0

#define ELEMENTAL_STONEMAN 0
#define ELEMENTAL_LAVAMAN  1
#define ELEMENTAL_ICEMAN   2

void CProjectile_OnInitClass(void)
{
}

void CProjectile_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheTexture(TEX_REFL_BWRIPLES01);
  pdec->PrecacheTexture(TEX_REFL_BWRIPLES02);
  pdec->PrecacheTexture(TEX_REFL_LIGHTMETAL01);
  pdec->PrecacheTexture(TEX_REFL_LIGHTBLUEMETAL01);
  pdec->PrecacheTexture(TEX_REFL_DARKMETAL);
  pdec->PrecacheTexture(TEX_REFL_PURPLE01);

  pdec->PrecacheTexture(TEX_SPEC_WEAK);
  pdec->PrecacheTexture(TEX_SPEC_MEDIUM);
  pdec->PrecacheTexture(TEX_SPEC_STRONG);

  switch ((ProjectileType)iUser) {
  case PRT_ROCKET                :
  case PRT_WALKER_ROCKET         :
  case PRT_DEVIL_ROCKET          :
    pdec->PrecacheModel(MODEL_ROCKET  );
    pdec->PrecacheTexture(TEXTURE_ROCKET);
    pdec->PrecacheSound(SOUND_FLYING  );
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_ROCKET);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_ROCKET_PLANE);
    break;
  case PRT_GRENADE:
    pdec->PrecacheModel(MODEL_GRENADE);
    pdec->PrecacheTexture(TEXTURE_GRENADE);
    pdec->PrecacheSound(SOUND_GRENADE_BOUNCE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE_PLANE);
    break;
  
  case PRT_FLAME:
    pdec->PrecacheModel(MODEL_FLAME);
    pdec->PrecacheClass(CLASS_FLAME);
    break;

  case PRT_LASER_RAY:
    pdec->PrecacheModel(MODEL_LASER                   );
    pdec->PrecacheTexture(TEXTURE_GREEN_LASER         );
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_LASERWAVE);
    break;

  case PRT_GRUNT_PROJECTILE_SOL:
    pdec->PrecacheModel(MODEL_GRUNT_PROJECTILE           );
    pdec->PrecacheTexture(TEXTURE_GRUNT_PROJECTILE_01    );
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_LASERWAVE);
    break;

  case PRT_GRUNT_PROJECTILE_COM:
    pdec->PrecacheModel(MODEL_GRUNT_PROJECTILE           );
    pdec->PrecacheTexture(TEXTURE_GRUNT_PROJECTILE_02    );
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_LASERWAVE);
    break;

  case PRT_CATMAN_FIRE:
    pdec->PrecacheModel(MODEL_CATMAN_FIRE             );
    pdec->PrecacheTexture(TEXTURE_CATMAN_FIRE         );
    break;

  case PRT_HEADMAN_FIRECRACKER:
    pdec->PrecacheModel(MODEL_HEADMAN_FIRECRACKER     );
    pdec->PrecacheTexture(TEXTURE_HEADMAN_FIRECRACKER );
    break;
  case PRT_HEADMAN_ROCKETMAN:
    pdec->PrecacheModel(MODEL_HEADMAN_BLADE           );
    pdec->PrecacheTexture(TEXTURE_HEADMAN_BLADE       );
    pdec->PrecacheModel(MODEL_HEADMAN_BLADE_FLAME     );
    pdec->PrecacheTexture(TEXTURE_HEADMAN_BLADE_FLAME );
    break;
  case PRT_HEADMAN_BOMBERMAN:
    pdec->PrecacheModel(MODEL_HEADMAN_BOMB         );
    pdec->PrecacheTexture(TEXTURE_HEADMAN_BOMB     );  
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BOMB);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE_PLANE);
    break;

  case PRT_BONEMAN_FIRE:
    pdec->PrecacheModel(MODEL_BONEMAN_FIRE         );
    pdec->PrecacheTexture(TEXTURE_BONEMAN_FIRE     );
    break;

  case PRT_WOMAN_FIRE:
    pdec->PrecacheModel(MODEL_WOMAN_FIRE           );
    pdec->PrecacheTexture(TEXTURE_WOMAN_FIRE       );
    break;

  case PRT_DRAGONMAN_FIRE:
  case PRT_DRAGONMAN_STRONG_FIRE:
    pdec->PrecacheModel(MODEL_DRAGONMAN_FIRE       );
    pdec->PrecacheTexture(TEXTURE_DRAGONMAN_FIRE1  );
    pdec->PrecacheTexture(TEXTURE_DRAGONMAN_FIRE2  );
    break;

  case PRT_STONEMAN_FIRE:
  case PRT_STONEMAN_BIG_FIRE:
  case PRT_STONEMAN_LARGE_FIRE:
    pdec->PrecacheModel(MODEL_ELEM_STONE           );
    pdec->PrecacheTexture(TEXTURE_ELEM_STONE       ); 
    break;
  case PRT_LAVAMAN_BIG_BOMB:
  case PRT_LAVAMAN_BOMB:
  case PRT_LAVAMAN_STONE:
    pdec->PrecacheModel(MODEL_ELEM_LAVA_STONE);
    pdec->PrecacheModel(MODEL_ELEM_LAVA_STONE_FLARE);
    pdec->PrecacheModel(MODEL_ELEM_LAVA_BOMB);
    pdec->PrecacheModel(MODEL_ELEM_LAVA_BOMB_FLARE);
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_STONE); 
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_STONE_FLARE ); 
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_BOMB); 
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_BOMB_FLARE); 
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
    pdec->PrecacheClass(CLASS_BLOOD_SPRAY);
    break;
  case PRT_METEOR:
    pdec->PrecacheSound(SOUND_FLYING  );
    pdec->PrecacheSound(SOUND_METEOR_BLAST  );
    pdec->PrecacheModel(MODEL_ELEM_LAVA_BOMB);
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_BOMB); 
    pdec->PrecacheClass(CLASS_BLOOD_SPRAY);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON);

    break;

  case PRT_ICEMAN_FIRE:
  case PRT_ICEMAN_BIG_FIRE:
  case PRT_ICEMAN_LARGE_FIRE:
    pdec->PrecacheModel(MODEL_ELEM_ICE          );  
    pdec->PrecacheModel(MODEL_ELEM_ICE_FLARE    );  
    pdec->PrecacheTexture(TEXTURE_ELEM_ICE      );    
  //pdec->PrecacheTexture(TEXTURE_ELEM_ICE_FLARE);    
    break;

  case PRT_HUANMAN_FIRE:
    pdec->PrecacheModel(MODEL_HUANMAN_FIRE      );
    pdec->PrecacheTexture(TEXTURE_HUANMAN_FIRE  );
    pdec->PrecacheModel(MODEL_HUANMAN_FLARE     );
    pdec->PrecacheTexture(TEXTURE_HUANMAN_FLARE );
    break;

  case PRT_FISHMAN_FIRE:
    pdec->PrecacheModel(MODEL_FISHMAN_FIRE      );
    pdec->PrecacheTexture(TEXTURE_FISHMAN_FIRE  );
    break;

  case PRT_MANTAMAN_FIRE:
    pdec->PrecacheModel(MODEL_MANTAMAN_FIRE     );
    pdec->PrecacheTexture(TEXTURE_MANTAMAN_FIRE );
    break;

  case PRT_DEVIL_LASER:         
    /*
    pdec->PrecacheModel(MODEL_DEVIL_LASER      );
    pdec->PrecacheTexture(TEXTURE_DEVIL_LASER  ); 
    break;
    */

  case PRT_CYBORG_LASER:         
  case PRT_CYBORG_BOMB:
    pdec->PrecacheModel(MODEL_CYBORG_LASER      );
    pdec->PrecacheTexture(TEXTURE_CYBORG_LASER  ); 
    pdec->PrecacheModel(MODEL_CYBORG_BOMB       );
    pdec->PrecacheTexture(TEXTURE_CYBORG_BOMB   ); 
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_BOMB);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE_PLANE);
    break;

  case PRT_LAVA_COMET:
    pdec->PrecacheModel(MODEL_ELEM_LAVA_STONE);
    pdec->PrecacheModel(MODEL_ELEM_LAVA_STONE_FLARE);
    pdec->PrecacheModel(MODEL_ELEM_LAVA_BOMB);
    pdec->PrecacheModel(MODEL_ELEM_LAVA_BOMB_FLARE);
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_STONE); 
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_STONE_FLARE ); 
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_BOMB); 
    pdec->PrecacheTexture(TEXTURE_ELEM_LAVA_BOMB_FLARE); 
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
    pdec->PrecacheClass(CLASS_BLOOD_SPRAY);
    pdec->PrecacheModel(MODEL_LAVA          );
    pdec->PrecacheTexture(TEXTURE_LAVA      );
    pdec->PrecacheModel(MODEL_LAVA_FLARE    );
    pdec->PrecacheTexture(TEXTURE_LAVA_FLARE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
    pdec->PrecacheClass(CLASS_BLOOD_SPRAY);
    break;
  case PRT_BEAST_PROJECTILE:
  case PRT_BEAST_DEBRIS:
    pdec->PrecacheSound(SOUND_BEAST_FLYING  );
    pdec->PrecacheModel(MODEL_BEAST_FIRE);
    pdec->PrecacheTexture(TEXTURE_BEAST_FIRE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON);
    break;
  case PRT_GUFFY_PROJECTILE:
    pdec->PrecacheSound(SOUND_FLYING                   );
    pdec->PrecacheModel(MODEL_GUFFY_PROJECTILE         );
    pdec->PrecacheTexture(TEXTURE_GUFFY_PROJECTILE     );
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_ROCKET );
    break;
  case PRT_BEAST_BIG_PROJECTILE:
  case PRT_DEVIL_GUIDED_PROJECTILE:
  case PRT_BEAST_BIG_DEBRIS:
  case PRT_SHOOTER_FIREBALL:
    pdec->PrecacheSound(SOUND_BEAST_FLYING  );
    pdec->PrecacheModel(MODEL_BEAST_FIRE);
    pdec->PrecacheTexture(TEXTURE_BEAST_BIG_FIRE);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_LIGHT_CANNON);
    break;
  case PRT_DEMON_FIREBALL:
  case PRT_DEMON_FIREBALL_DEBRIS:
    pdec->PrecacheSound(SOUND_DEMON_FLYING  );
    pdec->PrecacheModel(MODEL_DEMON_FIREBALL);
    pdec->PrecacheTexture(TEXTURE_DEMON_FIREBALL);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_LIGHT_CANNON);
    break;
  case PRT_LARVA_PLASMA:
    pdec->PrecacheSound(SOUND_DEMON_FLYING  );
    pdec->PrecacheModel(MODEL_LARVA_PLASMA);
    pdec->PrecacheTexture(TEXTURE_LARVA_PLASMA);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_LIGHT_CANNON);    
    break;
  case PRT_LARVA_TAIL_PROJECTILE:
    pdec->PrecacheSound(SOUND_LARVETTE  );
    pdec->PrecacheModel(MODEL_LARVA_TAIL);
    pdec->PrecacheTexture(TEXTURE_LARVA_TAIL);
    pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON);
    break;
  case PRT_SHOOTER_WOODEN_DART:
    pdec->PrecacheModel(MODEL_SHTR_WOODEN_DART);
    pdec->PrecacheTexture(TEX_SHTR_WOODEN_DART);
    break;
  case PRT_SHOOTER_FLAME:
    pdec->PrecacheModel(MODEL_FLAME);
    pdec->PrecacheClass(CLASS_FLAME);
    break;
  case PRT_AIRELEMENTAL_WIND:
    pdec->PrecacheModel(MODEL_WINDBLAST);
    pdec->PrecacheTexture(TEXTURE_WINDBLAST);    
    break;
  case PRT_AFTERBURNER_DEBRIS:
    pdec->PrecacheModel(MODEL_MARKER);
    pdec->PrecacheTexture(TEXTURE_MARKER);    
    break;
  default:
    ASSERT(FALSE);
  }
}
%}


class export CProjectile : CMovableModelEntity {
name      "Projectile";
thumbnail "";
features "ImplementsOnInitClass", "ImplementsOnPrecache", "CanBePredictable";

properties:
  1 CEntityPointer m_penLauncher,     // who lanuched it
  2 enum ProjectileType m_prtType = PRT_ROCKET,       // type of the projectile
  3 enum ProjectileMovingType m_pmtMove = PMT_FLYING, // projectile moving type
  4 CEntityPointer m_penParticles,    // another entity for particles
  5 CEntityPointer m_penTarget,       // guided projectile's target
  6 CEntityPointer m_penLastDamaged,  // last entity this projectile damaged

 10 FLOAT m_fSpeed = 0.0f,                   // projectile speed (optional, only for some projectiles)
 11 FLOAT m_fIgnoreTime = 0.0f,              // time when laucher will be ignored
 12 FLOAT m_fFlyTime = 0.0f,                 // fly time before explode/disappear
 13 FLOAT m_fStartTime = 0.0f,               // start time when launched
 14 FLOAT m_fDamageAmount = 0.0f,            // damage amount when hit something
 15 FLOAT m_fRangeDamageAmount = 0.0f,       // range damage amount
 16 FLOAT m_fDamageHotSpotRange = 0.0f,      // hot spot range damage for exploding projectile
 17 FLOAT m_fDamageFallOffRange = 0.0f,      // fall off range damage for exploding projectile
 18 FLOAT m_fSoundRange = 0.0f,              // sound range where explosion can be heard
 19 BOOL m_bExplode = FALSE,                 // explode -> range damage
 20 BOOL m_bLightSource = FALSE,             // projectile is also light source
 21 BOOL m_bCanHitHimself = FALSE,           // projectile can him himself
 22 BOOL m_bCanBeDestroyed = FALSE,          // projectile can be destroyed from something else
 23 FLOAT m_fWaitAfterDeath = 0.0f,          // wait after death for particles
 24 FLOAT m_aRotateSpeed = 0.0f,             // speed of rotation for guided projectiles
 25 FLOAT m_tmExpandBox = 0.0f,              // expand collision after a few seconds
 26 FLOAT m_tmInvisibility = 0.0f,           // don't render before given time
 27 INDEX m_iRebounds = 0,                   // how many times to rebound
 28 FLOAT m_fStretch=1.0f,                   // stretch

 30 CSoundObject m_soEffect,          // sound channel
 31 CSoundObject m_soExplosion,       // sound channel

 35 FLOAT m_fGuidedMaxSpeedFactor = 30.0f,   // speed factor for guided projectiles

 50 BOOL bLockedOn = TRUE,
 51 BOOL m_bLeftFlame = FALSE,
{
  CLightSource m_lsLightSource;
}

components:
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  2 class   CLASS_LIGHT         "Classes\\Light.ecl",
  3 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",
  4 class   CLASS_BLOOD_SPRAY   "Classes\\BloodSpray.ecl",

// ********* PLAYER ROCKET *********
  5 model   MODEL_ROCKET        "Models\\Weapons\\RocketLauncher\\Projectile\\Rocket.mdl",
  6 texture TEXTURE_ROCKET      "Models\\Weapons\\RocketLauncher\\Projectile\\Rocket.tex",
  8 sound   SOUND_FLYING        "Sounds\\Weapons\\RocketFly.wav",
  9 sound   SOUND_BEAST_FLYING  "Sounds\\Weapons\\ProjectileFly.wav",

// ********* PLAYER GRENADE *********
 10 model   MODEL_GRENADE         "Models\\Weapons\\GrenadeLauncher\\Grenade\\Grenade.mdl",
 11 texture TEXTURE_GRENADE       "Models\\Weapons\\GrenadeLauncher\\Grenade\\Grenade.tex",
 12 sound   SOUND_GRENADE_BOUNCE  "Models\\Weapons\\GrenadeLauncher\\Sounds\\Bounce.wav",

// ********* PLAYER FLAME *********
 15 model   MODEL_FLAME         "ModelsMP\\Weapons\\Flamer\\Projectile\\Invisible.mdl",
 16 class   CLASS_FLAME         "Classes\\Flame.ecl",

// ********* CATMAN FIRE *********
 20 model   MODEL_CATMAN_FIRE   "Models\\Enemies\\Catman\\Projectile\\Projectile.mdl",
 21 texture TEXTURE_CATMAN_FIRE "Models\\Enemies\\Catman\\Projectile\\Projectile.tex",

// ********* HEADMAN FIRE *********
 30 model   MODEL_HEADMAN_FIRECRACKER     "Models\\Enemies\\Headman\\Projectile\\FireCracker.mdl",
 31 texture TEXTURE_HEADMAN_FIRECRACKER   "Models\\Enemies\\Headman\\Projectile\\Texture.tex",
 32 model   MODEL_HEADMAN_BLADE           "Models\\Enemies\\Headman\\Projectile\\Blade.mdl",
 33 texture TEXTURE_HEADMAN_BLADE         "Models\\Enemies\\Headman\\Projectile\\Blade.tex",
 34 model   MODEL_HEADMAN_BLADE_FLAME     "Models\\Enemies\\Headman\\Projectile\\FireTrail.mdl",
 35 texture TEXTURE_HEADMAN_BLADE_FLAME   "Models\\Enemies\\Headman\\Projectile\\FireTrail.tex",
 36 model   MODEL_HEADMAN_BOMB            "Models\\Enemies\\Headman\\Projectile\\Bomb.mdl",
 37 texture TEXTURE_HEADMAN_BOMB          "Models\\Enemies\\Headman\\Projectile\\Bomb.tex",

// ********* LAVA *********
 40 model   MODEL_LAVA                    "Models\\Effects\\Debris\\Lava01\\Lava.mdl",
 41 texture TEXTURE_LAVA                  "Models\\Effects\\Debris\\Lava01\\Lava.tex",
 42 model   MODEL_LAVA_FLARE              "Models\\Effects\\Debris\\Lava01\\LavaFlare.mdl",
 43 texture TEXTURE_LAVA_FLARE            "Models\\Effects\\Debris\\Lava01\\Flare.tex",

// ********* PLAYER LASER *********
 50 model   MODEL_LASER           "Models\\Weapons\\Laser\\Projectile\\LaserProjectile.mdl",
 51 texture TEXTURE_GREEN_LASER   "Models\\Weapons\\Laser\\Projectile\\LaserProjectile.tex",
 52 texture TEXTURE_BLUE_LASER    "Models\\Weapons\\Laser\\Projectile\\LaserProjectileBlue.tex",

// ********* BONEMAN FIRE *********
 60 model   MODEL_BONEMAN_FIRE    "Models\\Enemies\\Boneman\\Projectile\\Projectile.mdl",
 61 texture TEXTURE_BONEMAN_FIRE  "Models\\Enemies\\Boneman\\Projectile\\Projectile.tex",

// ********* WOMAN FIRE *********
 65 model   MODEL_WOMAN_FIRE      "Models\\Enemies\\Woman\\Projectile\\Projectile.mdl",
 66 texture TEXTURE_WOMAN_FIRE    "Models\\Enemies\\Woman\\Projectile\\Projectile.tex",

// ********* DRAGONMAN FIRE *********
 70 model   MODEL_DRAGONMAN_FIRE    "Models\\Enemies\\Dragonman\\Projectile\\Projectile.mdl",
 71 texture TEXTURE_DRAGONMAN_FIRE1 "Models\\Enemies\\Dragonman\\Projectile\\Projectile1.tex",
 72 texture TEXTURE_DRAGONMAN_FIRE2 "Models\\Enemies\\Dragonman\\Projectile\\Projectile2.tex",

// ********* ELEMENTAL FIRE *********
 80 model   MODEL_ELEM_STONE            "Models\\Enemies\\Elementals\\Projectile\\Stone.mdl",
 81 model   MODEL_ELEM_ICE              "Models\\Enemies\\Elementals\\Projectile\\IcePyramid.mdl",
 82 model   MODEL_ELEM_ICE_FLARE        "Models\\Enemies\\Elementals\\Projectile\\IcePyramidFlare.mdl",
 83 model   MODEL_ELEM_LAVA_BOMB        "Models\\Enemies\\Elementals\\Projectile\\LavaBomb.mdl",
 84 model   MODEL_ELEM_LAVA_BOMB_FLARE  "Models\\Enemies\\Elementals\\Projectile\\LavaBombFlare.mdl",
 85 model   MODEL_ELEM_LAVA_STONE       "Models\\Enemies\\Elementals\\Projectile\\LavaStone.mdl",
 86 model   MODEL_ELEM_LAVA_STONE_FLARE "Models\\Enemies\\Elementals\\Projectile\\LavaStoneFlare.mdl",

 90 texture TEXTURE_ELEM_STONE            "Models\\Enemies\\Elementals\\Projectile\\Stone.tex",
 91 texture TEXTURE_ELEM_ICE              "Models\\Enemies\\Elementals\\Projectile\\IcePyramid.tex",
 //92 texture TEXTURE_ELEM_ICE_FLARE        "Textures\\Effects\\Flares\\03\\Flare06.tex",
 93 texture TEXTURE_ELEM_LAVA_BOMB        "Models\\Enemies\\Elementals\\Projectile\\LavaBomb.tex",
 94 texture TEXTURE_ELEM_LAVA_BOMB_FLARE  "Models\\Enemies\\Elementals\\Projectile\\LavaBombFlare.tex",
 95 texture TEXTURE_ELEM_LAVA_STONE       "Models\\Enemies\\Elementals\\Projectile\\LavaStone.tex",
 96 texture TEXTURE_ELEM_LAVA_STONE_FLARE "Models\\Enemies\\Elementals\\Projectile\\LavaBombFlare.tex",

// ********* HUANMAN FIRE *********
105 model   MODEL_HUANMAN_FIRE      "Models\\Enemies\\Huanman\\Projectile\\Projectile.mdl",
106 texture TEXTURE_HUANMAN_FIRE    "Models\\Enemies\\Huanman\\Projectile\\Projectile.tex",
107 model   MODEL_HUANMAN_FLARE     "Models\\Enemies\\Huanman\\Projectile\\Flare.mdl",
108 texture TEXTURE_HUANMAN_FLARE   "Textures\\Effects\\Flares\\01\\WhiteRedRing66.tex",

// ********* FISHMAN FIRE *********
110 model   MODEL_FISHMAN_FIRE      "Models\\Enemies\\Fishman\\Projectile\\Projectile.mdl",
111 texture TEXTURE_FISHMAN_FIRE    "Models\\Enemies\\Fishman\\Projectile\\Water.tex",

// ********* FISHMAN FIRE *********
120 model   MODEL_MANTAMAN_FIRE     "Models\\Enemies\\Mantaman\\Projectile\\Projectile.mdl",
121 texture TEXTURE_MANTAMAN_FIRE   "Models\\Enemies\\Mantaman\\Projectile\\Water.tex",

// ********* CYBORG FIRE *********
130 model   MODEL_CYBORG_LASER      "Models\\Weapons\\Laser\\Projectile\\LaserProjectile.mdl",
132 texture TEXTURE_CYBORG_LASER    "Models\\Weapons\\Laser\\Projectile\\LaserProjectileBlue.tex",
133 model   MODEL_CYBORG_BOMB       "Models\\Enemies\\Cyborg\\Projectile\\Projectile.mdl",
134 texture TEXTURE_CYBORG_BOMB     "Models\\Enemies\\Cyborg\\Projectile\\Projectile.tex",

// ********* GRUNT PROJECTILES *********
135 model   MODEL_GRUNT_PROJECTILE      "ModelsMP\\Enemies\\Grunt\\Projectile\\GruntProjectile.mdl",
136 texture TEXTURE_GRUNT_PROJECTILE_01 "ModelsMP\\Enemies\\Grunt\\Projectile\\GruntProjectileSoldier.tex",
137 texture TEXTURE_GRUNT_PROJECTILE_02 "ModelsMP\\Enemies\\Grunt\\Projectile\\GruntProjectileCommander.tex",

// ********* DEVIL FIRE *********
/*
135 model   MODEL_DEVIL_LASER       "Models\\Enemies\\Devil\\Weapons\\DevilLaserProjectile.mdl",
136 texture TEXTURE_DEVIL_LASER     "Models\\Enemies\\Devil\\Weapons\\DevilLaserProjectile.tex",
*/

// ********* BEAST FIRE *********
140 model   MODEL_BEAST_FIRE       "Models\\Enemies\\Beast\\Projectile\\Projectile.mdl",
141 texture TEXTURE_BEAST_FIRE     "Models\\Enemies\\Beast\\Projectile\\Projectile.tex",
142 texture TEXTURE_BEAST_BIG_FIRE "Models\\Enemies\\Beast\\Projectile\\ProjectileBig.tex",

// ********* DEMON FIREBALL *********
150 model   MODEL_DEMON_FIREBALL    "ModelsMP\\Enemies\\Demon\\Projectile\\Projectile.mdl",
151 texture TEXTURE_DEMON_FIREBALL  "ModelsMP\\Enemies\\Demon\\Projectile\\Projectile.tex",
152 sound   SOUND_DEMON_FLYING      "SoundsMP\\Weapons\\ProjectileFly.wav",

// ********** SHOOTERS **********
160 model   MODEL_SHTR_WOODEN_DART  "ModelsMP\\Enemies\\Shooters\\Arrow01.mdl",
161 texture TEX_SHTR_WOODEN_DART    "ModelsMP\\Enemies\\Shooters\\Arrow01.tex",

// ********** GUFFY PROJECTILE **********
170 model   MODEL_GUFFY_PROJECTILE   "ModelsMP\\Enemies\\Guffy\\Projectile\\GuffyProjectile.mdl",
171 texture TEXTURE_GUFFY_PROJECTILE "ModelsMP\\Enemies\\Guffy\\Projectile\\GuffyProjectile.tex",

// ********** LARVA PROJECTILES **********
172 model   MODEL_LARVA_PLASMA        "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\Projectile.mdl",
173 texture TEXTURE_LARVA_PLASMA      "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\Projectile.tex",
174 model   MODEL_LARVA_PLASMA_BALL   "ModelsMP\\Enemies\\ExotechLarva\\Weapons\\PlasmaGun.mdl",
175 texture TEXTURE_LARVA_PLASMA_BALL "ModelsMP\\Enemies\\ExotechLarva\\Weapons\\PlasmaGun.tex",

176 model   MODEL_LARVA_TAIL          "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\TailProjectile.mdl",
177 texture TEXTURE_LARVA_TAIL        "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\TailProjectile.tex",
178 sound   SOUND_LARVETTE            "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\Squeak.wav",

// ********** AIR ELEMENTAL WIND BLAST **********
180 model   MODEL_WINDBLAST          "ModelsMP\\Enemies\\AirElemental\\Projectile\\WindBlast.mdl",
181 texture TEXTURE_WINDBLAST        "ModelsMP\\Enemies\\AirElemental\\Projectile\\WindBlast.tex",

// ****************** METEOR  ******************
185 sound   SOUND_METEOR_BLAST       "SoundsMP\\Weapons\\MeteorBlast.wav",

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

220 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
221 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"

functions:
  // premoving
  void PreMoving(void) {
    if (m_tmExpandBox>0) {
      if (_pTimer->CurrentTick()>m_fStartTime+m_tmExpandBox) {
        ChangeCollisionBoxIndexWhenPossible(1);
        m_tmExpandBox = 0;
      }
    }
    CMovableModelEntity::PreMoving();
  }

  // postmoving
  void PostMoving(void) {
    CMovableModelEntity::PostMoving();
    // if flamer flame
    if (m_prtType==PRT_FLAME || m_prtType==PRT_SHOOTER_FLAME) {
      // if came to water
      CContentType &ctDn = GetWorld()->wo_actContentTypes[en_iDnContent];
      // stop existing
      if (!(ctDn.ct_ulFlags&CTF_BREATHABLE_LUNGS)) {
        m_fWaitAfterDeath = 0.0f;   // immediate stop
        SendEvent(EEnd());
      }
    }
  };

  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CMovableModelEntity::Read_t(istr);
    // setup light source
    if( m_bLightSource) {
      SetupLightSource(TRUE);
    }
  }

  // dump sync data to text file
  export void DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck)  // throw char *
  {
    CMovableModelEntity ::DumpSync_t(strm, iExtensiveSyncCheck);
    strm.FPrintF_t("projectile type: %d\n", m_prtType);
    strm.FPrintF_t("launcher:");
    if (m_penLauncher!=NULL) {
      strm.FPrintF_t("id:%05d '%s'(%s) (%g, %g, %g)\n", 
        m_penLauncher->en_ulID,
        (const char *) m_penLauncher->GetName(), (const char *) m_penLauncher->GetClass()->ec_pdecDLLClass->dec_strName,
        m_penLauncher->GetPlacement().pl_PositionVector(1),
        m_penLauncher->GetPlacement().pl_PositionVector(2),
        m_penLauncher->GetPlacement().pl_PositionVector(3));
    } else {
      strm.FPrintF_t("<none>\n");
    }
  }

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    if( m_bLightSource && !IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  }

  export void Copy(CEntity &enOther, ULONG ulFlags)
  {
    CMovableModelEntity::Copy(enOther, ulFlags);
    CProjectile *penOther = (CProjectile *)(&enOther);
    if (ulFlags&COPY_PREDICTOR) {
      //m_lsLightSource;
      //SetupLightSource(); //? is this ok !!!!
      m_bLightSource = FALSE;
    }
  }

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    // if time now is inside invisibility time, don't render model
    CModelObject *pmo = GetModelObject();
    if ( (pmo != NULL) && (_pTimer->GetLerpedCurrentTick() < (m_fStartTime+m_tmInvisibility) ) )
    {
      // make it invisible
      pmo->mo_colBlendColor = 0;
    }
    else
    {
      // make it visible
      pmo->mo_colBlendColor = C_WHITE|CT_OPAQUE;
    }
    return CEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  }

  // Setup light source
  void SetupLightSource(BOOL bLive)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    lsNew.ls_rHotSpot = 0.0f;
    switch (m_prtType) {
      case PRT_ROCKET:
      case PRT_WALKER_ROCKET:
      case PRT_DEVIL_ROCKET:
        if( bLive)
        {
          lsNew.ls_colColor = 0xA0A080FF;
        }
        else
        {
          lsNew.ls_colColor = C_BLACK|CT_OPAQUE;
        }
        lsNew.ls_rFallOff = 5.0f;
        lsNew.ls_plftLensFlare = &_lftYellowStarRedRingFar;
        break;
      case PRT_GUFFY_PROJECTILE:
        lsNew.ls_colColor = C_BLUE;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
      case PRT_GRENADE:
        lsNew.ls_colColor = 0x2F1F0F00;
        lsNew.ls_rFallOff = 2.0f;
        lsNew.ls_rHotSpot = 0.2f;
        lsNew.ls_plftLensFlare = &_lftYellowStarRedRingFar;
        break;
      case PRT_FLAME:
        lsNew.ls_colColor = C_dORANGE;
        lsNew.ls_rFallOff = 1.0f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_LASER_RAY:
        lsNew.ls_colColor = C_vdGREEN;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_GRUNT_PROJECTILE_SOL:
        lsNew.ls_colColor = C_vdRED;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_GRUNT_PROJECTILE_COM:
        lsNew.ls_colColor = C_vdRED;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_CATMAN_FIRE:
        lsNew.ls_colColor = C_BLUE;
        lsNew.ls_rFallOff = 3.5f;
        lsNew.ls_plftLensFlare = &_lftCatmanFireGlow;
        break;
      case PRT_HEADMAN_FIRECRACKER:
        lsNew.ls_colColor = C_ORANGE;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_HEADMAN_ROCKETMAN:
        lsNew.ls_colColor = C_YELLOW;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_WOMAN_FIRE:
        lsNew.ls_colColor = C_WHITE;
        lsNew.ls_rFallOff = 3.5f;
        lsNew.ls_plftLensFlare = &_lftCatmanFireGlow;
        break;
      case PRT_DRAGONMAN_FIRE:
        lsNew.ls_colColor = C_YELLOW;
        lsNew.ls_rFallOff = 3.5f;
        lsNew.ls_plftLensFlare = &_lftProjectileYellowBubbleGlow;
        break;
      case PRT_DRAGONMAN_STRONG_FIRE:
        lsNew.ls_colColor = C_RED;
        lsNew.ls_rFallOff = 3.5f;
        lsNew.ls_plftLensFlare = &_lftProjectileStarGlow;
        break;
      case PRT_HUANMAN_FIRE:
        lsNew.ls_colColor = C_lBLUE;
        lsNew.ls_rFallOff = 2.0f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_FISHMAN_FIRE:
        lsNew.ls_colColor = C_lBLUE;
        lsNew.ls_rFallOff = 2.0f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_MANTAMAN_FIRE:
        lsNew.ls_colColor = C_lBLUE;
        lsNew.ls_rFallOff = 2.0f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_CYBORG_LASER:
        lsNew.ls_colColor = C_dBLUE;
        lsNew.ls_rFallOff = 1.5f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      case PRT_DEVIL_LASER:
        lsNew.ls_colColor = C_dBLUE;
        lsNew.ls_rFallOff = 5.0f;
        lsNew.ls_plftLensFlare = &_lftYellowStarRedRingFar;
        break;
      case PRT_LARVA_PLASMA:
        lsNew.ls_colColor = C_dBLUE;
        lsNew.ls_rFallOff = 5.0f;
        lsNew.ls_plftLensFlare = &_lftCatmanFireGlow;
        break;
      case PRT_SHOOTER_FIREBALL:
        lsNew.ls_colColor = C_dORANGE;
        lsNew.ls_rFallOff = 5.0f;
        lsNew.ls_plftLensFlare = &_lftYellowStarRedRingFar;
        break;
      case PRT_SHOOTER_FLAME:
        lsNew.ls_colColor = C_dORANGE;
        lsNew.ls_rFallOff = 1.0f;
        lsNew.ls_plftLensFlare = NULL;
        break;
      default:
        ASSERTALWAYS("Unknown light source");
    }
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  }

  // render particles
  void RenderParticles(void) {
    switch (m_prtType) {
      case PRT_ROCKET:
      case PRT_WALKER_ROCKET: Particles_RocketTrail(this, 1.0f); break;
      case PRT_DEVIL_ROCKET: Particles_RocketTrail(this, 8.0f); break;
      case PRT_GUFFY_PROJECTILE: break;// Particles_RocketTrail(this, 1.0f); break;
      case PRT_GRENADE: {
        //Particles_GrenadeTrail(this);
        FLOAT fSpeedRatio = en_vCurrentTranslationAbsolute.Length()/140.0f;
        Particles_CannonBall(this, fSpeedRatio);
        break;
                        }
      case PRT_FLAME: {
        // elapsed time
        FLOAT fLeaderLiving, fFollowerLiving, fInFrontLiving;
        fInFrontLiving=0.05f;
        fLeaderLiving = _pTimer->GetLerpedCurrentTick() - m_fStartTime;
        // not NULL or deleted
        if (m_penParticles!=NULL && !(m_penParticles->GetFlags()&ENF_DELETED)) {
          FLOAT3D vDirLeader=en_vCurrentTranslationAbsolute;
          vDirLeader.Normalize();
          // if last is not flame thrower pipe
          if(IsOfClass(m_penParticles, "Projectile"))
          {
            CProjectile &prLast=(CProjectile &)*m_penParticles;
            // if pre last is flame thrower pipe
            if( IsOfClass(prLast.m_penParticles, "Player Weapons"))
            {
              CPlayerWeapons &plw=(CPlayerWeapons&)*prLast.m_penParticles;
              if(!(plw.GetPlayer()->GetFlags()&ENF_ALIVE))
              {
                return;
              }
              CPlacement3D plPipe, plInFrontOfPipe;
              ((CPlayerWeapons&)*prLast.m_penParticles).GetFlamerSourcePlacement(plPipe, plInFrontOfPipe);
              fFollowerLiving = _pTimer->GetLerpedCurrentTick() - ((CProjectile&)*m_penParticles).m_fStartTime;
              FLOAT3D vDirPipeFront;
              AnglesToDirectionVector( plInFrontOfPipe.pl_OrientationAngle, vDirPipeFront);
              vDirPipeFront.Normalize();
              Particles_FlameThrower(GetLerpedPlacement(), plInFrontOfPipe,
                                     vDirLeader, vDirPipeFront,
                                     fLeaderLiving, fInFrontLiving, en_ulID, FALSE);
            }
            // draw particles with another projectile
            else
            {
              fFollowerLiving = _pTimer->GetLerpedCurrentTick() - ((CProjectile&)*m_penParticles).m_fStartTime;
              FLOAT3D vDirFollower = ((CMovableModelEntity*)(CEntity*)m_penParticles)->en_vCurrentTranslationAbsolute;
              vDirFollower.Normalize();
              Particles_FlameThrower(GetLerpedPlacement(), m_penParticles->GetLerpedPlacement(),
                                     vDirLeader, vDirFollower, fLeaderLiving, fFollowerLiving, en_ulID, FALSE);
            }
          // draw particles with player weapons
          } else if (IsOfClass(m_penParticles, "Player Weapons")) {
            CPlayerWeapons &plw=(CPlayerWeapons&)*m_penParticles;
            if(!(plw.GetPlayer()->GetFlags()&ENF_ALIVE))
            {
              return;
            }
            CPlacement3D plPipe, plInFrontOfPipe;
            plw.GetFlamerSourcePlacement(plPipe, plInFrontOfPipe);
            FLOAT3D vDirPipeFront;
            AnglesToDirectionVector( plInFrontOfPipe.pl_OrientationAngle, vDirPipeFront);
            FLOAT3D vViewDir;
            AnglesToDirectionVector( plPipe.pl_OrientationAngle, vViewDir);
            FLOAT3D vDirFollower = vViewDir.Normalize();
            
            /*
            Particles_FlameThrower(GetLerpedPlacement(), plPipe,
              vDirLeader, vDirFollower,
              fLeaderLiving, 0.0f, en_ulID, TRUE);
              */
            
            Particles_FlameThrower(plInFrontOfPipe, plPipe,
              vDirPipeFront, vDirFollower,
              fInFrontLiving, 0.0f, en_ulID, TRUE);

            Particles_FlameThrowerStart(plPipe, plw.m_tmFlamerStart, plw.m_tmFlamerStop);
          }
        }
        break;
      }
      case PRT_CATMAN_FIRE: Particles_RocketTrail(this, 1.0f); break;
      case PRT_HEADMAN_FIRECRACKER: Particles_FirecrackerTrail(this); break;
      case PRT_HEADMAN_ROCKETMAN: Particles_Fireball01Trail(this); break;
      case PRT_HEADMAN_BOMBERMAN: Particles_BombTrail(this); break;
      case PRT_LAVA_COMET: Particles_LavaTrail(this); break;
      case PRT_LAVAMAN_BIG_BOMB: Particles_LavaBombTrail(this, 4.0f); break;
      case PRT_LAVAMAN_BOMB: Particles_LavaBombTrail(this, 1.0f); break;
      case PRT_BEAST_PROJECTILE: Particles_BeastProjectileTrail( this, 2.0f, 0.25f, 48); break;
      case PRT_BEAST_BIG_PROJECTILE:
      case PRT_DEMON_FIREBALL:
        Particles_BeastBigProjectileTrail( this, 4.0f, 0.25f, 0.0f, 64);
        Particles_AfterBurner( this, m_fStartTime, 1.0f);
        break;
      case PRT_DEVIL_GUIDED_PROJECTILE:
        Particles_BeastBigProjectileTrail( this, 6.0f, 0.375f, 0.0f, 64);
        break;
      case PRT_BEAST_DEBRIS: Particles_BeastProjectileDebrisTrail(this, 0.20f); break;
      case PRT_BEAST_BIG_DEBRIS: Particles_BeastProjectileDebrisTrail(this, 0.25f); break;
      case PRT_SHOOTER_WOODEN_DART: Particles_RocketTrail(this, 0.25f); break;
      case PRT_SHOOTER_FIREBALL: Particles_Fireball01Trail(this); break;
      case PRT_SHOOTER_FLAME: {
        // elapsed time
        FLOAT fTimeElapsed, fParticlesTimeElapsed;
        fTimeElapsed = _pTimer->GetLerpedCurrentTick() - m_fStartTime;
        // not NULL or deleted
        if (m_penParticles!=NULL && !(m_penParticles->GetFlags()&ENF_DELETED)) {
          // draw particles with another projectile
          if (IsOfClass(m_penParticles, "Projectile")) {
            fParticlesTimeElapsed = _pTimer->GetLerpedCurrentTick() - ((CProjectile&)*m_penParticles).m_fStartTime;
            Particles_ShooterFlame(GetLerpedPlacement(), m_penParticles->GetLerpedPlacement(),
                                   fTimeElapsed, fParticlesTimeElapsed);
          } else if (IsOfClass(m_penParticles, "Shooter")) {
            Particles_ShooterFlame(GetLerpedPlacement(),
              ((CShooter&)*m_penParticles).GetPlacement(),
                                   fTimeElapsed, 0.0f);
          }
        }
        break;
      }
      case PRT_METEOR:
        Particles_MeteorTrail(this, m_fStretch, 1.0f, en_vCurrentTranslationAbsolute);
        Particles_AfterBurner(this, m_fStartTime, m_fStretch*4.0f, 2);
        break;
      case PRT_AFTERBURNER_DEBRIS:
        Particles_AfterBurner(this, m_fStartTime, m_fStretch);
        break;
      case PRT_AIRELEMENTAL_WIND:
        Particles_Windblast(this, m_fStretch/4.0f, m_fStartTime+3.0f);
        break;
    }
  }




/************************************************************
 *              PLAYER ROCKET / GRENADE                     *
 ************************************************************/
void PlayerRocket(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_ROCKET);
  SetModelMainTexture(TEXTURE_ROCKET);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  // play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  m_fFlyTime = 30.0f;
  if( GetSP()->sp_bCooperative)
  {
    m_fDamageAmount = 100.0f;
    m_fRangeDamageAmount = 50.0f;
  }
  else
  {
    m_fDamageAmount = 75.0f;
    m_fRangeDamageAmount = 75.0f;
  }
  m_fDamageHotSpotRange = 4.0f;
  m_fDamageFallOffRange = 8.0f;
  m_fSoundRange = 50.0f;
  m_bExplode = TRUE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 1.125f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(5.0f);
  m_pmtMove = PMT_FLYING;
};

void WalkerRocket(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_ROCKET);
  SetModelMainTexture(TEXTURE_ROCKET);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  // play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  m_fFlyTime = 30.0f;
  if (GetSP()->sp_gdGameDifficulty<=CSessionProperties::GD_EASY) {
    m_fDamageAmount = 40.0f;
    m_fRangeDamageAmount = 20.0f;
  } else {
    m_fDamageAmount = 100.0f;
    m_fRangeDamageAmount = 50.0f;
  }
  m_fDamageHotSpotRange = 4.0f;
  m_fDamageFallOffRange = 8.0f;
  m_fSoundRange = 50.0f;
  m_bExplode = TRUE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 1.125f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(5.0f);
  m_pmtMove = PMT_FLYING;
};

void WalkerRocketExplosion(void) {
  PlayerRocketExplosion();
}

void PlayerRocketExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_ROCKET;
  ese.vStretch = FLOAT3D(1,1,1);
  SpawnEffect(GetPlacement(), ese);
  // spawn sound event in range
  if( IsDerivedFromClass( m_penLauncher, "Player")) {
    SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, m_fSoundRange);
  }

  // explosion debris
  ese.betType = BET_EXPLOSION_DEBRIS;
  SpawnEffect(GetPlacement(), ese);

  // explosion smoke
  ese.betType = BET_EXPLOSION_SMOKE;
  SpawnEffect(GetPlacement(), ese);

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // shock wave
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // second explosion on plane
      ese.betType = BET_ROCKET_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
};


void PlayerGrenade(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_GRENADE);
  SetModelMainTexture(TEXTURE_GRENADE);
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 5.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, FRnd()*120.0f+120.0f, FRnd()*250.0f-125.0f));
  en_fBounceDampNormal   = 0.75f;
  en_fBounceDampParallel = 0.6f;
  en_fJumpControlMultiplier = 0.0f;
  en_fCollisionSpeedLimit = 45.0f;
  en_fCollisionDamageFactor = 10.0f;
  m_fFlyTime = 3.0f;
  m_fDamageAmount = 75.0f;
  m_fRangeDamageAmount = 100.0f;
  m_fDamageHotSpotRange = 4.0f;
  m_fDamageFallOffRange = 8.0f;
  m_fSoundRange = 50.0f;
  m_bExplode = TRUE;
  en_fDeceleration = 25.0f;
  m_bLightSource = TRUE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  SetHealth(20.0f);
  m_pmtMove = PMT_SLIDING;
  m_tmInvisibility = 0.05f;
  m_tmExpandBox = 0.1f;
};

void PlayerGrenadeExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_GRENADE;
  ese.vStretch = FLOAT3D(1,1,1);
  SpawnEffect(GetPlacement(), ese);
  // spawn sound event in range
  if( IsDerivedFromClass( m_penLauncher, "Player")) {
    SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, m_fSoundRange);
  }

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // wall stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // shock wave
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // second explosion on plane
      ese.betType = BET_GRENADE_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
};



/************************************************************
 *                    PLAYER FLAME                          *
 ************************************************************/
void PlayerFlame(void) {
  // set appearance
  InitAsEditorModel();
  SetPhysicsFlags(EPF_MODEL_SLIDING&~EPF_TRANSLATEDBYGRAVITY&~EPF_ORIENTEDBYGRAVITY);
  //SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_FLAME);
  //SetModel(MODEL_BEAST_FIRE);
  //SetModelMainTexture(TEXTURE_ROCKET);

  // add player's forward velocity to flame
  CMovableEntity *penPlayer = (CMovableEntity*)(CEntity*)m_penLauncher;
  FLOAT3D vDirection = penPlayer->en_vCurrentTranslationAbsolute;
  FLOAT3D vFront = -GetRotationMatrix().GetColumn(3);
  FLOAT fSpeedFwd = ClampDn( vDirection%vFront, 0.0f);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -(25.0f+fSpeedFwd)), penPlayer);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 1.0f;
  m_fDamageAmount = (GetSP()->sp_bCooperative) ? 10.0f : 4.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.3f;
  m_tmExpandBox = 0.1f;
  m_pmtMove = PMT_SLIDING;
};



/************************************************************
 *                    PLAYER LASER                          *
 ************************************************************/
void PlayerLaserRay(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_LASER);
  CModelObject *pmo = GetModelObject();
  if(pmo != NULL)
  {
    pmo->PlayAnim( LASERPROJECTILE_ANIM_GROW, 0);
  }
  SetModelMainTexture(TEXTURE_GREEN_LASER);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -120.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 3.0f;
  m_fDamageAmount = 20.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_tmExpandBox = 0.1f;
  // time when laser ray becomes visible
  m_tmInvisibility = 0.025f;
  m_pmtMove = PMT_FLYING;
};

void PlayerLaserWave(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // shock wave
      ese.colMuliplier = C_dRED|CT_OPAQUE;
      ese.betType = BET_LASERWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
    }
  }
};



/************************************************************
 *                   CATMAN PROJECTILE                      *
 ************************************************************/
void CatmanProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_CATMAN_FIRE);
  SetModelMainTexture(TEXTURE_CATMAN_FIRE);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -15.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 5.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};



/************************************************************
 *                   HEADMAN PROJECTILE                     *
 ************************************************************/
void HeadmanFirecracker(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_SLIDING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_HEADMAN_FIRECRACKER);
  SetModelMainTexture(TEXTURE_HEADMAN_FIRECRACKER);
  GetModelObject()->StretchModel(FLOAT3D(0.75f, 0.75f, 0.75f));
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -25.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, FRnd()*20.0f-10.0f));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 4.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_SLIDING;
};

void HeadmanRocketman(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetComponents(this, *GetModelObject(), MODEL_HEADMAN_BLADE, TEXTURE_HEADMAN_BLADE,
                TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
  AddAttachmentToModel(this, *GetModelObject(), BLADE_ATTACHMENT_FLAME01,
                       MODEL_HEADMAN_BLADE_FLAME, TEXTURE_HEADMAN_BLADE_FLAME, 0, 0, 0);
  AddAttachmentToModel(this, *GetModelObject(), BLADE_ATTACHMENT_FLAME02,
                       MODEL_HEADMAN_BLADE_FLAME, TEXTURE_HEADMAN_BLADE_FLAME, 0, 0, 0);
  AddAttachmentToModel(this, *GetModelObject(), BLADE_ATTACHMENT_FLAME03,
                       MODEL_HEADMAN_BLADE_FLAME, TEXTURE_HEADMAN_BLADE_FLAME, 0, 0, 0);
  GetModelObject()->StretchModel(FLOAT3D(0.5f, 0.5f, 0.5f));
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 5.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};

void HeadmanBomberman(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_HEADMAN_BOMB);
  SetModelMainTexture(TEXTURE_HEADMAN_BOMB);

  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f));
  m_fFlyTime = 2.5f;
  m_fDamageAmount = 10.0f;
  m_fRangeDamageAmount = 15.0f;
  m_fDamageHotSpotRange = 1.0f;
  m_fDamageFallOffRange = 6.0f;
  m_fSoundRange = 25.0f;
  m_bExplode = TRUE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  SetHealth(5.0f);
  m_pmtMove = PMT_FLYING;
};

void HeadmanBombermanExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_BOMB;
  ese.vStretch = FLOAT3D(1.0f,1.0f,1.0f);
  SpawnEffect(GetPlacement(), ese);
  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // wall stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      ese.betType = BET_GRENADE_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
};

void CyborgBombExplosion(void)
{
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_BOMB;
  ese.vStretch = FLOAT3D(1.0f,1.0f,1.0f);
  SpawnEffect(GetPlacement(), ese);
  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // wall stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      ese.betType = BET_GRENADE_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
};

/************************************************************
 *                  BONEMAN PROJECTILE                      *
 ************************************************************/
void BonemanProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_BONEMAN_FIRE);
  SetModelMainTexture(TEXTURE_BONEMAN_FIRE);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_SLIDING;
};



/************************************************************
 *                   WOMAN PROJECTILE                       *
 ************************************************************/
void WomanProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_WOMAN_FIRE);
  SetModelMainTexture(TEXTURE_WOMAN_FIRE);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 8.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};



/************************************************************
 *                  DRAGONMAN PROJECTILE                    *
 ************************************************************/
void DragonmanProjectile(INDEX iType) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_ONBLOCK_SLIDE|EPF_PUSHABLE|EPF_MOVABLE);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_DRAGONMAN_FIRE);
  if (iType==DRAGONMAN_STRONG) {
    SetModelMainTexture(TEXTURE_DRAGONMAN_FIRE2);
  } else {
    SetModelMainTexture(TEXTURE_DRAGONMAN_FIRE1);
  }
  // start moving
  if (iType==DRAGONMAN_STRONG) {
    LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -40.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
    m_fDamageAmount = 14.0f;
  } else {
    LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
    m_fDamageAmount = 7.0f;
  }
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};



/************************************************************
 *                  ELEMENTAL PROJECTILE                    *
 ************************************************************/
void ElementalRock(INDEX iSize, INDEX iType) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_ONBLOCK_SLIDE|EPF_PUSHABLE|EPF_MOVABLE);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  switch (iType) {
    case ELEMENTAL_STONEMAN:
      SetModel(MODEL_ELEM_STONE);
      SetModelMainTexture(TEXTURE_ELEM_STONE);
      break;
    case ELEMENTAL_LAVAMAN:
      SetModel(MODEL_ELEM_LAVA_STONE);
      SetModelMainTexture(TEXTURE_ELEM_LAVA_STONE);
      AddAttachmentToModel(this, *GetModelObject(), LAVASTONE_ATTACHMENT_FLARE,
        MODEL_ELEM_LAVA_STONE_FLARE, TEXTURE_ELEM_LAVA_STONE_FLARE, 0, 0, 0);
      break;
    case ELEMENTAL_ICEMAN:
      SetModel(MODEL_ELEM_ICE);
      SetModelMainTexture(TEXTURE_ELEM_ICE);
      //AddAttachmentToModel(this, *GetModelObject(), ICEPYRAMID_ATTACHMENT_FLARE,
      //  MODEL_ELEM_ICE_FLARE, TEXTURE_ELEM_ICE_FLARE, 0, 0, 0);
      break;
  }
  if (iSize==ELEMENTAL_LARGE) {
    GetModelObject()->StretchModel(FLOAT3D(2.25f, 2.25f, 2.25f));
  } else if (iSize==ELEMENTAL_BIG) {
    GetModelObject()->StretchModel(FLOAT3D(0.75f, 0.75f, 0.75f));
  } else {
    GetModelObject()->StretchModel(FLOAT3D(0.4f, 0.4f, 0.4f));
  }
  ModelChangeNotify();
  // start moving
  if (iSize==ELEMENTAL_LARGE) {
    LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -80.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
    m_fDamageAmount = 20.0f;
    SetHealth(40.0f);
  } else if (iSize==ELEMENTAL_BIG) {
    LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -50.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
    m_fDamageAmount = 12.5f;
    SetHealth(20.0f);
  } else {
    LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
    m_fDamageAmount = 7.0f;
    SetHealth(10.0f);
  }
  SetDesiredRotation(ANGLE3D(0, 0, FRnd()*1800.0f-900.0f));
  en_fCollisionSpeedLimit = 1000.0f;
  en_fCollisionDamageFactor = 0.0f;
  m_fFlyTime = 5.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_SLIDING;
};

void LavaManBomb(void)
{
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_ELEM_LAVA_BOMB);
  SetModelMainTexture(TEXTURE_ELEM_LAVA_BOMB);
  AddAttachmentToModel(this, *GetModelObject(), LAVABOMB_ATTACHMENT_FLARE,
                     MODEL_ELEM_LAVA_BOMB_FLARE, TEXTURE_ELEM_LAVA_BOMB_FLARE, 0, 0, 0);

  if (m_prtType == PRT_LAVAMAN_BIG_BOMB)
  {
    GetModelObject()->StretchModel(FLOAT3D(6.0f, 6.0f, 6.0f));
    m_fDamageAmount = 20.0f;
    m_fRangeDamageAmount = 10.0f;
    m_fDamageHotSpotRange = 7.5f;
    m_fDamageFallOffRange = 15.0f;
    SetHealth(30.0f);
  }
  else if (m_prtType == PRT_LAVAMAN_BOMB)
  {
    GetModelObject()->StretchModel(FLOAT3D(1.5f, 1.5f, 1.5f));
    m_fDamageAmount = 10.0f;
    m_fRangeDamageAmount =  5.0f;
    m_fDamageHotSpotRange = 5.0f;
    m_fDamageFallOffRange = 10.0f;
    SetHealth(10.0f);
  }
  ModelChangeNotify();
  
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, FRnd()*360.0f-180.0f, 0.0f));
  m_fFlyTime = 20.0f;
  m_fSoundRange = 50.0f;
  m_bExplode = TRUE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_pmtMove = PMT_FLYING;
  m_fWaitAfterDeath = 4.0f;


  if (m_prtType == PRT_LAVAMAN_BIG_BOMB)
  {
    // spawn particle debris
    CPlacement3D plSpray = GetPlacement();
    CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
    penSpray->SetParent( this);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    eSpawnSpray.fDamagePower = 4.0f;
    eSpawnSpray.fSizeMultiplier = 0.5f;
    eSpawnSpray.sptType = SPT_LAVA_STONES;
    eSpawnSpray.vDirection = FLOAT3D(0,-0.5f,0);
    eSpawnSpray.penOwner = this;
    penSpray->Initialize( eSpawnSpray);
  }
}

void LavamanBombExplosion(void)
{
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;
  
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge))
  {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f)
    {
      // shock wave
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
    }
  }

  // shock wave
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(4,4,4);
  SpawnEffect(GetPlacement(), ese);

  // spawn particle debris
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 4.0f;
  eSpawnSpray.fSizeMultiplier = 0.5f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);

  // spawn smaller lava bombs
  for( INDEX iDebris = 0; iDebris < static_cast<INDEX>(3+IRnd()%3); iDebris++)
  {
    FLOAT fHeading = (FRnd()-0.5f)*180.0f;
    FLOAT fPitch = 10.0f+FRnd()*40.0f;
    FLOAT fSpeed = 10.0f+FRnd()*50.0f;

    // launch
    CPlacement3D pl = GetPlacement();
    pl.pl_PositionVector(2) += 2.0f;
    pl.pl_OrientationAngle = m_penLauncher->GetPlacement().pl_OrientationAngle;
    pl.pl_OrientationAngle(1) += AngleDeg(fHeading);
    pl.pl_OrientationAngle(2) = AngleDeg(fPitch);

    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_LAVAMAN_BOMB;
    eLaunch.fSpeed = fSpeed;
    penProjectile->Initialize(eLaunch);

    // spawn particle debris
    CPlacement3D plSpray = pl;
    CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
    penSpray->SetParent( penProjectile);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    eSpawnSpray.fDamagePower = 1.0f;
    eSpawnSpray.fSizeMultiplier = 0.5f;
    eSpawnSpray.sptType = SPT_LAVA_STONES;
    eSpawnSpray.vDirection = FLOAT3D(0,-0.5f,0);
    eSpawnSpray.penOwner = penProjectile;
    penSpray->Initialize( eSpawnSpray);
  }
};

void LavamanBombDebrisExplosion(void)
{
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // spawn shock wave
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge))
  {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f)
    {
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
    }
  }

  // spawn explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(2,2,2);
  SpawnEffect(GetPlacement(), ese);

  // spawn particle debris
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fSizeMultiplier = 4.0f;
  eSpawnSpray.fDamagePower = 2.0f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/16.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
}

/************************************************************
 *                   HUANMAN PROJECTILE                     *
 ************************************************************/
void HuanmanProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetComponents(this, *GetModelObject(), MODEL_HUANMAN_FIRE, TEXTURE_HUANMAN_FIRE,
                TEX_REFL_LIGHTMETAL01, TEX_SPEC_STRONG, 0);
  AddAttachmentToModel(this, *GetModelObject(), PROJECTILE_ATTACHMENT_FLARE,
                       MODEL_HUANMAN_FLARE, TEXTURE_HUANMAN_FLARE, 0, 0, 0);
  GetModelObject()->StretchModel(FLOAT3D(0.5f, 0.5f, 0.5f));
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};

/************************************************************
 *                   BEAST PROJECTILE                       *
 ************************************************************/
void BeastProjectile(void) {
  // we need target for guied misile
  if (IsDerivedFromClass(m_penLauncher, "Enemy Base")) {
    m_penTarget = ((CEnemyBase *) m_penLauncher.ep_pen)->m_penEnemy;
  }
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_FREE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  
  SetModel(MODEL_BEAST_FIRE);
  SetModelMainTexture(TEXTURE_BEAST_FIRE);
  GetModelObject()->StretchModel(FLOAT3D(1.5f, 1.5f, 1.5f));

  ModelChangeNotify();
  // play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_BEAST_FLYING, SOF_3D|SOF_LOOP);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -60.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 10.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_GUIDED;
  m_fGuidedMaxSpeedFactor = 30.0f;
  m_aRotateSpeed = 175.0f;
  SetHealth(10.0f);
};

void BeastBigProjectile(void) {
  // we need target for guided misile
  if (IsDerivedFromClass(m_penLauncher, "Enemy Base")) {
    m_penTarget = ((CEnemyBase *) m_penLauncher.ep_pen)->m_penEnemy;
  }
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_FREE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_BEAST_FIRE);
  SetModelMainTexture(TEXTURE_BEAST_BIG_FIRE);
  GetModelObject()->StretchModel(FLOAT3D(2.5f, 2.5f, 2.5f));

  ModelChangeNotify();
  // play the flying sound
  m_soEffect.Set3DParameters(50.0f, 2.0f, 1.0f, 0.75f);
  PlaySound(m_soEffect, SOUND_BEAST_FLYING, SOF_3D|SOF_LOOP);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -60.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 10.0f;
  m_fDamageAmount = 20.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_GUIDED_FAST;
  m_fGuidedMaxSpeedFactor = 90.0f;
  SetHealth(10000.0f);
  m_aRotateSpeed = 100.0f;
};

void BeastDebris(void)
{
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_BEAST_FIRE);
  GetModelObject()->StretchModel(FLOAT3D(0.75f, 0.75f, 0.75f));
  SetModelMainTexture(TEXTURE_BEAST_FIRE);
  GetModelObject()->StartAnim(1+(ULONG)FRnd()*5.0f);

  ModelChangeNotify();
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -20.0f), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 10.0f;
  m_fDamageAmount = 0.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
  SetHealth(1.0f);
  m_aRotateSpeed = 100.0f;
};

void BeastBigDebris(void)
{
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_BEAST_FIRE);
  SetModelMainTexture(TEXTURE_BEAST_BIG_FIRE);
  GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));
  GetModelObject()->StartAnim(1+(ULONG)FRnd()*5.0f);
  
  ModelChangeNotify();
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -20.0f), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 10.0f;
  m_fDamageAmount = 0.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
  SetHealth(1.0f);
  m_aRotateSpeed = 100.0f;
};

void BeastDebrisExplosion(void)
{
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_GREEN|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(0.75,0.75,0.75);
  SpawnEffect(GetPlacement(), ese);

  // spawn particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 2.0f;
  eSpawnSpray.fSizeMultiplier = 0.75f;
  eSpawnSpray.sptType = SPT_BEAST_PROJECTILE_SPRAY;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/64.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
}

void BeastBigDebrisExplosion(void)
{
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(1,1,1);
  SpawnEffect(GetPlacement(), ese);

  // spawn particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 2.0f;
  eSpawnSpray.fSizeMultiplier = 1.0f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/64.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
}

void BeastProjectileExplosion(void)
{
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_GREEN|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(1.25,1.25,1.25);
  SpawnEffect(GetPlacement(), ese);

  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 2.0f;
  eSpawnSpray.fSizeMultiplier = 1.0f;
  eSpawnSpray.sptType = SPT_BEAST_PROJECTILE_SPRAY;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/64.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);

  FLOAT fHeading = 20.0f+(FRnd()-0.5f)*60.0f;
  // debris
  for( INDEX iDebris=0; iDebris<2; iDebris++)
  {
    FLOAT fPitch = 10.0f+FRnd()*10.0f;
    FLOAT fSpeed = 5.0f+FRnd()*20.0f;

    // launch
    CPlacement3D pl = GetPlacement();
    pl.pl_OrientationAngle(1) += AngleDeg(fHeading);
    // turn to other way
    fHeading = -fHeading;
    pl.pl_OrientationAngle(2) = AngleDeg(fPitch);

    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_BEAST_DEBRIS;
    eLaunch.fSpeed = fSpeed;
    penProjectile->Initialize(eLaunch);

    // spawn particle debris
    CPlacement3D plSpray = pl;
    CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
    penSpray->SetParent( penProjectile);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    eSpawnSpray.fDamagePower = 0.5f;
    eSpawnSpray.fSizeMultiplier = 0.25f;
    eSpawnSpray.sptType = SPT_BEAST_PROJECTILE_SPRAY;
    eSpawnSpray.vDirection = FLOAT3D(0,-0.5f,0);
    eSpawnSpray.penOwner = penProjectile;
    penSpray->Initialize( eSpawnSpray);
  }
}

void BeastBigProjectileExplosion(void)
{
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(2,2,2);
  SpawnEffect(GetPlacement(), ese);

  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 4.0f;
  eSpawnSpray.fSizeMultiplier = 0.5f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);

  // debris
  for( INDEX iDebris =0 ; iDebris < static_cast<INDEX>(3+IRnd()%2); iDebris++)
  {
    FLOAT fHeading = (FRnd()-0.5f)*180.0f;
    FLOAT fPitch = 10.0f+FRnd()*40.0f;
    FLOAT fSpeed = 10.0f+FRnd()*50.0f;

    // launch
    CPlacement3D pl = GetPlacement();
    pl.pl_OrientationAngle(1) += AngleDeg(fHeading);
    pl.pl_OrientationAngle(2) += AngleDeg(fPitch);

    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_BEAST_BIG_DEBRIS;
    eLaunch.fSpeed = fSpeed;
    penProjectile->Initialize(eLaunch);

    // spawn particle debris
    CPlacement3D plSpray = pl;
    CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
    penSpray->SetParent( penProjectile);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    eSpawnSpray.fDamagePower = 1.0f;
    eSpawnSpray.fSizeMultiplier = 0.5f;
    eSpawnSpray.sptType = SPT_LAVA_STONES;
    eSpawnSpray.vDirection = FLOAT3D(0,-0.5f,0);
    eSpawnSpray.penOwner = penProjectile;
    penSpray->Initialize( eSpawnSpray);
  }
}

/************************************************************
 *                   FISHMAN PROJECTILE                     *
 ************************************************************/
void FishmanProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetComponents(this, *GetModelObject(), MODEL_FISHMAN_FIRE, TEXTURE_FISHMAN_FIRE, 0, 0, 0);
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 5.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};



/************************************************************
 *                   MANTAMAN PROJECTILE                    *
 ************************************************************/
void MantamanProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetComponents(this, *GetModelObject(), MODEL_MANTAMAN_FIRE, TEXTURE_MANTAMAN_FIRE, 0, 0, 0);
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -35.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 7.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};


/************************************************************
 *               DEVIL PROJECTILES                          *
 ************************************************************/
void DevilLaser(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetComponents(this, *GetModelObject(), MODEL_CYBORG_LASER, TEXTURE_CYBORG_LASER, 0, 0, 0);
  GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 2.0f));
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -DEVIL_LASER_SPEED), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};

void DevilRocket(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_ROCKET);
  SetModelMainTexture(TEXTURE_ROCKET);
  GetModelObject()->StretchModel(FLOAT3D(12.0f, 12.0f, 8.0f));
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -DEVIL_ROCKET_SPEED), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  // play the flying sound
  m_soEffect.Set3DParameters(100.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  m_fFlyTime = 50.0f;
  m_fDamageAmount = 50.0f;
  m_fRangeDamageAmount = 50.0f;
  m_fDamageHotSpotRange = 2.0f;
  m_fDamageFallOffRange = 10.0f;
  m_fSoundRange = 100.0f;
  m_bExplode = TRUE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 1.125f;
  m_tmExpandBox = 10000.0f;
  m_tmInvisibility = 0.05f;
  SetHealth(25.0f);
  m_pmtMove = PMT_FLYING;
};

void DevilRocketExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_GRENADE;
  ese.vStretch = FLOAT3D(2,2,2);
  SpawnEffect(GetPlacement(), ese);
  // spawn sound event in range
  if( IsDerivedFromClass( m_penLauncher, "Player")) {
    SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, m_fSoundRange);
  }

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      ese.vStretch = FLOAT3D(2,2,2);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // shock wave
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      ese.vStretch = FLOAT3D(2,2,2);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // second explosion on plane
      ese.betType = BET_GRENADE_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      ese.vStretch = FLOAT3D(2,2,2);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
};

void DevilGuidedProjectile(void) {
  // we need target for guied misile
  if (IsDerivedFromClass(m_penLauncher, "Enemy Base")) {
    m_penTarget = ((CEnemyBase *) m_penLauncher.ep_pen)->m_penEnemy;
  }
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_FREE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_BEAST_FIRE);
  SetModelMainTexture(TEXTURE_BEAST_BIG_FIRE);
  GetModelObject()->StretchModel(FLOAT3D(2.5f, 2.5f, 2.5f));
  ModelChangeNotify();
  // play the flying sound
  m_soEffect.Set3DParameters(250.0f, 2.0f, 1.0f, 0.75f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -80.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 20.0f;
  m_fDamageAmount = 20.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_GUIDED;
  m_fGuidedMaxSpeedFactor = 30.0f;
  SetHealth(30.0f);
  m_aRotateSpeed = 100.0f;
};

void DevilGuidedProjectileExplosion(void)
{
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(4,4,4);
  SpawnEffect(GetPlacement(), ese);

  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower =  8.0f;
  eSpawnSpray.fSizeMultiplier = 1.0f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);

  // debris
  for( INDEX iDebris = 0; iDebris < static_cast<INDEX>(3+IRnd()%2); iDebris++)
  {
    FLOAT fHeading = (FRnd()-0.5f)*180.0f;
    FLOAT fPitch = 10.0f+FRnd()*40.0f;
    FLOAT fSpeed = 10.0f+FRnd()*50.0f;

    // launch
    CPlacement3D pl = GetPlacement();
    pl.pl_OrientationAngle(1) += AngleDeg(fHeading);
    pl.pl_OrientationAngle(2) += AngleDeg(fPitch);

    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_BEAST_BIG_DEBRIS;
    eLaunch.fSpeed = fSpeed;
    penProjectile->Initialize(eLaunch);

    // spawn particle debris
    CPlacement3D plSpray = pl;
    CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
    penSpray->SetParent( penProjectile);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    eSpawnSpray.fDamagePower = 2.0f;
    eSpawnSpray.fSizeMultiplier = 1.0f;
    eSpawnSpray.sptType = SPT_LAVA_STONES;
    eSpawnSpray.vDirection = FLOAT3D(0,-0.5f,0);
    eSpawnSpray.penOwner = penProjectile;
    penSpray->Initialize( eSpawnSpray);
  }
}

/************************************************************
 *               CYBORG LASER / PROJECTILE                  *
 ************************************************************/
void CyborgLaser(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetComponents(this, *GetModelObject(), MODEL_CYBORG_LASER, TEXTURE_CYBORG_LASER, 0, 0, 0);
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -60.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 4.0f;
  m_fDamageAmount = 5.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};

void CyborgBomb(void)
{
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_CYBORG_BOMB);
  SetModelMainTexture(TEXTURE_CYBORG_BOMB);
  ModelChangeNotify();
  // just freefall
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 2.5f;
  m_fDamageAmount = 10.0f;
  m_fRangeDamageAmount = 15.0f;
  m_fDamageHotSpotRange = 1.0f;
  m_fDamageFallOffRange = 6.0f;
  m_fSoundRange = 25.0f;
  m_bExplode = TRUE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  SetHealth(5.0f);
  m_pmtMove = PMT_FLYING;
};



/************************************************************
 *                        LAVA BALL                         *
 ************************************************************/
void LavaBall(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_FALL);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetModel(MODEL_LAVA);
  SetModelMainTexture(TEXTURE_LAVA);
  AddAttachment(0, MODEL_LAVA_FLARE, TEXTURE_LAVA_FLARE);

  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 5.0f;
  m_fRangeDamageAmount = 5.0f;
  m_fDamageHotSpotRange = 1.0f;
  m_fDamageFallOffRange = 4.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = TRUE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_FLYING;
};

void LavaBallExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // shock wave
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
    }
  }
};

/************************************************************
 *                 G R U N T   L A S E R                    *
 ************************************************************/

void GruntSoldierLaser(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_GRUNT_PROJECTILE);
  CModelObject *pmo = GetModelObject();
  if(pmo != NULL)
  {
    pmo->PlayAnim(GRUNTPROJECTILE_ANIM_DEFAULT, 0);
  }
  SetModelMainTexture(TEXTURE_GRUNT_PROJECTILE_01);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -45.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 3.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_tmExpandBox = 0.1f;
  // time when laser ray becomes visible
  m_tmInvisibility = 0.025f;
  m_pmtMove = PMT_FLYING;
};

void GruntCommanderLaser(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_GRUNT_PROJECTILE);
  CModelObject *pmo = GetModelObject();
  if(pmo != NULL)
  {
    pmo->PlayAnim(GRUNTPROJECTILE_ANIM_DEFAULT, 0);
  }
  SetModelMainTexture(TEXTURE_GRUNT_PROJECTILE_02);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -55.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 3.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_tmExpandBox = 0.1f;
  // time when laser ray becomes visible
  m_tmInvisibility = 0.025f;
  m_pmtMove = PMT_FLYING;
};


/************************************************************
 *                G U F F Y   R O C K E T                   *
 ************************************************************/

void GuffyProjectile(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetModel(MODEL_GUFFY_PROJECTILE);
  SetModelMainTexture(TEXTURE_GUFFY_PROJECTILE);
  GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));

  CModelObject *pmo = GetModelObject();
  if(pmo != NULL)
  {
    pmo->PlayAnim(GUFFYPROJECTILE_ANIM_ROTATE01, AOF_LOOPING);
  }
  
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -50.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  // play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  m_fFlyTime = 30.0f;
  m_fDamageAmount = 10.0f;
  m_fRangeDamageAmount = 10.0f;
  m_fDamageHotSpotRange = 4.0f;
  m_fDamageFallOffRange = 8.0f;
  m_fSoundRange = 50.0f;
  m_bExplode = TRUE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 1.125f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(10000.0f);
  m_pmtMove = PMT_FLYING;
};

void GuffyProjectileExplosion(void) {
  PlayerRocketExplosion();
}


/************************************************************
 *                D E M O N   F I R E B A L L               *
 ************************************************************/

void DemonFireball(void) {
  // we need target for guided misile
  if (IsDerivedFromClass(m_penLauncher, "Enemy Base")) {
    m_penTarget = ((CEnemyBase *) m_penLauncher.ep_pen)->m_penEnemy;
  }
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_FREE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_DEMON_FIREBALL);
  SetModelMainTexture(TEXTURE_DEMON_FIREBALL);
  GetModelObject()->StretchModel(FLOAT3D(2.5f, 2.5f, 2.5f));

  ModelChangeNotify();
  // play the flying sound
  m_soEffect.Set3DParameters(50.0f, 2.0f, 1.0f, 0.75f);
  PlaySound(m_soEffect, SOUND_BEAST_FLYING, SOF_3D|SOF_LOOP);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -100.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 12.0f;
  m_fDamageAmount = 20.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_GUIDED_FAST;
  m_fGuidedMaxSpeedFactor = 90.0f;
  SetHealth(10000.0f);
  m_aRotateSpeed = 200.0f;
};

void DemonFireballExplosion(void)
{
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(2,2,2);
  SpawnEffect(GetPlacement(), ese);

  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 4.0f;
  eSpawnSpray.fSizeMultiplier = 0.5f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
}

/************************************************************
 *              L A R V A   P R O J E C T I L E S           *
 ************************************************************/

void LarvaPlasma(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetModel(MODEL_LARVA_PLASMA_BALL);
  SetModelMainTexture(TEXTURE_LARVA_PLASMA_BALL);
  AddAttachmentToModel(this, *GetModelObject(), PLASMAGUN_ATTACHMENT_PROJECTILE,
        MODEL_LARVA_PLASMA, TEXTURE_LARVA_PLASMA, 0, 0, 0);
      
  GetModelObject()->StretchModel(FLOAT3D(2.5f, 2.5f, 2.5f));
  ModelChangeNotify();

  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -60.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  // play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  m_fFlyTime = 30.0f;
  if( GetSP()->sp_bCooperative)
  {
    m_fDamageAmount = 30.0f;
    m_fRangeDamageAmount = 30.0f;
  }
  else
  {
    m_fDamageAmount = 25.0f;
    m_fRangeDamageAmount = 25.0f;
  }
  m_fDamageHotSpotRange = 4.0f;
  m_fDamageFallOffRange = 8.0f;
  m_fSoundRange = 50.0f;
  m_bExplode = TRUE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.05f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(100.0f);
  m_iRebounds = 4;
  m_pmtMove = PMT_FLYING_REBOUNDING;
}

void LarvaPlasmaExplosion(void) {
  // explosion
  ESpawnEffect ese;
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_LIGHT_CANNON;
  ese.vStretch = FLOAT3D(2,2,2);
  SpawnEffect(GetPlacement(), ese);

  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 1.0f;
  eSpawnSpray.fSizeMultiplier = 0.25f;
  eSpawnSpray.sptType = SPT_PLASMA;
  eSpawnSpray.vDirection = FLOAT3D(0.0f, 2.5f, 0.0f);
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
}

void LarvaTail(void) {
  
  // we need target for guied misile
  if (IsDerivedFromClass(m_penLauncher, "Enemy Base")) {
    m_penTarget = ((CEnemyBase *) m_penLauncher.ep_pen)->m_penEnemy;
  }
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_SLIDING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  
  SetModel(MODEL_LARVA_TAIL);
  SetModelMainTexture(TEXTURE_LARVA_TAIL);
  GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));

  ModelChangeNotify();
  // play the flying sound
  m_soEffect.Set3DParameters(50.0f, 10.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_LARVETTE, SOF_3D|SOF_LOOP);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 12.0f;
  m_fDamageAmount = 10.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_GUIDED_SLIDING;
  m_aRotateSpeed = 275.0f;
  SetHealth(10.0f);
}

void LarvaTailExplosion(void) {
  PlayerRocketExplosion();
}


/*****************************************************************
 *       A I R   E L E M E N T A L   P R O J E C T I L E S       *
 *****************************************************************/

void WindBlast(void) {
  // set appearance
  InitAsEditorModel();
  SetPhysicsFlags(EPF_MODEL_SLIDING);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_WINDBLAST);
  SetModelMainTexture(TEXTURE_WINDBLAST);
  GetModelObject()->StretchModel(FLOAT3D(3.0f, 3.0f, 3.0f));
  ModelChangeNotify();
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -50.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 5.0f;
  m_fDamageAmount = 20.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.0f;
  m_pmtMove = PMT_SLIDING;
}

/************************************************************
 *                    M E T E O R                           *
 ************************************************************/

void Meteor() {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);

  SetModel(MODEL_ELEM_LAVA_BOMB);
  SetModelMainTexture(TEXTURE_ELEM_LAVA_BOMB);
  /*AddAttachmentToModel(this, *GetModelObject(), LAVABOMB_ATTACHMENT_FLARE,
                     MODEL_ELEM_LAVA_BOMB_FLARE, TEXTURE_ELEM_LAVA_BOMB_FLARE, 0, 0, 0);*/

  GetModelObject()->StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
  ModelChangeNotify();

  Particles_AfterBurner_Prepare(this);

  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  // play the flying sound
  m_soEffect.Set3DParameters(250.0f, 10.0f, 2.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);
  m_fFlyTime = 30.0f;
  m_fDamageAmount = 100.0f;
  m_fRangeDamageAmount = 100.0f;
  m_fDamageHotSpotRange = 15.0f;
  m_fDamageFallOffRange = 30.0f;
  m_fSoundRange = 100.0f;
  m_bExplode = TRUE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = TRUE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = GetSoundLength(SOUND_METEOR_BLAST)+0.25f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(100.0f);
  m_pmtMove = PMT_FLYING;
}

void MeteorExplosion() {
  //LavamanBombExplosion();
  //PlayerRocketExplosion();
  // spawn particle debris
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 4.0f;
  eSpawnSpray.fSizeMultiplier = 0.5f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);

  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_CANNON;
  ese.vStretch = FLOAT3D(5,5,5);
  SpawnEffect(GetPlacement(), ese);
  // spawn sound event in range
  if( IsDerivedFromClass( m_penLauncher, "Player")) {
    SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, m_fSoundRange);
  }

  // explosion debris
  ese.betType = BET_EXPLOSION_DEBRIS;
  SpawnEffect(GetPlacement(), ese);

  // explosion smoke
  ese.betType = BET_EXPLOSION_SMOKE;
  SpawnEffect(GetPlacement(), ese);

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // shock wave
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // second explosion on plane
      ese.betType = BET_ROCKET_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
  m_soExplosion.Set3DParameters(150.0f, 10.0f, 1.5f, 1.0f);
  PlaySound(m_soExplosion, SOUND_METEOR_BLAST, SOF_3D);
}


/************************************************************
 *                    S H O O T E R S                       *
 ************************************************************/

void ShooterWoodenDart(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_SHTR_WOODEN_DART);
  SetModelMainTexture(TEX_SHTR_WOODEN_DART);
  
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  /*// play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);*/
  m_fFlyTime = 10.0f;
  if (GetSP()->sp_gdGameDifficulty<=CSessionProperties::GD_EASY) {
    m_fDamageAmount = 5.0f;
  } else {
    m_fDamageAmount = 10.0f;
  }
  m_bExplode = FALSE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 1.125f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(5.0f);
  m_pmtMove = PMT_FLYING;
};

void ShooterWoodenDartExplosion() {
  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 0.5f;
  eSpawnSpray.fSizeMultiplier = 0.1f;
  eSpawnSpray.sptType = SPT_WOOD;
  eSpawnSpray.vDirection = -en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
};

void ShooterFireball(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetModel(MODEL_BEAST_FIRE);
  SetModelMainTexture(TEXTURE_BEAST_BIG_FIRE);
  GetModelObject()->StretchModel(FLOAT3D(0.25f, 0.25f, 0.25f));
  ModelChangeNotify();

  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  /*// play the flying sound
  m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_FLYING, SOF_3D|SOF_LOOP);*/
  m_fFlyTime = 10.0f;
  if (GetSP()->sp_gdGameDifficulty<=CSessionProperties::GD_EASY) {
    m_fDamageAmount = 7.5f;
  } else {
    m_fDamageAmount = 15.0f;
  }
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = TRUE;
  m_fWaitAfterDeath = 0.125f;
  m_tmExpandBox = 0.1f;
  m_tmInvisibility = 0.05f;
  SetHealth(5.0f);
  m_pmtMove = PMT_FLYING;
};

void ShooterFireballExplosion() {
  // particles
  CPlacement3D plSpray = GetPlacement();
  CEntityPointer penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
  penSpray->SetParent( this);
  ESpawnSpray eSpawnSpray;
  eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
  eSpawnSpray.fDamagePower = 1.0f;
  eSpawnSpray.fSizeMultiplier = 0.5f;
  eSpawnSpray.sptType = SPT_LAVA_STONES;
  eSpawnSpray.vDirection = -en_vCurrentTranslationAbsolute/32.0f;
  eSpawnSpray.penOwner = this;
  penSpray->Initialize( eSpawnSpray);
};

void ShooterFlame(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_PROJECTILE_FLYING);
  SetCollisionFlags(ECF_PROJECTILE_SOLID);
  SetFlags(GetFlags() | ENF_SEETHROUGH);
  SetModel(MODEL_FLAME);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -10.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 1.0f;
  m_fDamageAmount = 3.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = FALSE;
  m_bLightSource = TRUE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 0.3f;
  m_pmtMove = PMT_FLYING;
};

void AfterburnerDebris(void)
{
  Particles_AfterBurner_Prepare(this);
  // set appearance
  InitAsEditorModel();
  SetPhysicsFlags(EPF_MODEL_FALL);
  SetCollisionFlags(ECF_PROJECTILE_MAGIC);
  SetModel(MODEL_MARKER);
  SetModelMainTexture(TEXTURE_MARKER);
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f));
  m_fFlyTime = 10.0f;
  m_fDamageAmount = 0.0f;
  m_fRangeDamageAmount = 0.0f;
  m_fDamageHotSpotRange = 0.0f;
  m_fDamageFallOffRange = 0.0f;
  m_fSoundRange = 0.0f;
  m_bExplode = TRUE;
  m_bLightSource = FALSE;
  m_bCanHitHimself = FALSE;
  m_bCanBeDestroyed = FALSE;
  m_fWaitAfterDeath = 2.0f;
  m_pmtMove = PMT_FLYING;
}


/************************************************************
 *             C O M M O N   F U N C T I O N S              *
 ************************************************************/

// projectile touch his valid target
void ProjectileTouch(CEntityPointer penHit)
{
  // explode if needed
  ProjectileHit();

  // direct damage
  FLOAT3D vDirection;
  FLOAT fTransLen = en_vIntendedTranslation.Length();
  if( fTransLen>0.5f)
  {
    vDirection = en_vIntendedTranslation/fTransLen;
  }
  else
  {
    vDirection = -en_vGravityDir;
  }

  // spawn flame
  const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penLauncher);
  if ((m_prtType==PRT_FLAME||m_prtType==PRT_SHOOTER_FLAME) && m_fWaitAfterDeath>0.0f) {
    // don't burn the same entity twice while passing through it
    if (m_penLastDamaged==penHit) {
      return;
    } else {
      m_penLastDamaged=penHit;
    }

    // don't spawn flame on AirElemental
    BOOL bSpawnFlame=TRUE;
    BOOL bInflictDamage=TRUE;
    if (IsOfClass(penHit, "AirElemental"))
    {
      bSpawnFlame=FALSE;
    }

    EntityInfo *pei=(EntityInfo *)penHit->GetEntityInfo();
    if(pei!=NULL && pei->Eeibt==EIBT_ICE)
    {
      bSpawnFlame=FALSE;
      bInflictDamage=FALSE;
    }

    if( bSpawnFlame)
    {
      SpawnFlame(m_penLauncher, penHit, GetPlacement().pl_PositionVector);
    }
    if(bInflictDamage)
    {
      InflictDirectDamage(penHit, m_penLauncher, DMT_BURNING, m_fDamageAmount*fDamageMul,
                 GetPlacement().pl_PositionVector, vDirection);
    }
  
  // don't damage the same entity twice (wind blast)
  } else if (m_prtType==PRT_AIRELEMENTAL_WIND) {
    if (penHit==m_penLastDamaged) {
      return;   
    } else  {
      m_penLastDamaged=penHit;
    }
    InflictDirectDamage(penHit, m_penLauncher, DMT_PROJECTILE, m_fDamageAmount*fDamageMul,
               GetPlacement().pl_PositionVector, vDirection);
  
  // other projectiles
  } else {
    InflictDirectDamage(penHit, m_penLauncher, DMT_PROJECTILE, m_fDamageAmount*fDamageMul,
               GetPlacement().pl_PositionVector, vDirection);
  }
};


// projectile hit (or time expired or can't move any more)
void ProjectileHit(void)
{
  // explode ...
  if (m_bExplode) {
    const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penLauncher);
    InflictRangeDamage(m_penLauncher, DMT_EXPLOSION, m_fRangeDamageAmount*fDamageMul,
        GetPlacement().pl_PositionVector, m_fDamageHotSpotRange, m_fDamageFallOffRange);
  }
  // sound event
  if (m_fSoundRange>0.0f && IsDerivedFromClass( m_penLauncher, "Player"))
  {
    ESound eSound;
    eSound.EsndtSound = SNDT_EXPLOSION;
    eSound.penTarget = m_penLauncher;
    SendEventInRange(eSound, FLOATaabbox3D(GetPlacement().pl_PositionVector, m_fSoundRange));
  }
};


// spawn effect
void SpawnEffect(const CPlacement3D &plEffect, const ESpawnEffect &eSpawnEffect) {
  CEntityPointer penEffect = CreateEntity(plEffect, CLASS_BASIC_EFFECT);
  penEffect->Initialize(eSpawnEffect);
};



/************************************************************
 *                      S O U N D S                         *
 ************************************************************/
void BounceSound(void) {
  switch (m_prtType) {
    case PRT_GRENADE:
      if (en_vCurrentTranslationAbsolute.Length() > 3.0f) {
        m_soEffect.Set3DParameters(20.0f, 2.0f, 1.0f, 1.0f);
        PlaySound(m_soEffect, SOUND_GRENADE_BOUNCE, SOF_3D);
      }
      break;
  }
};



// Calculate current rotation speed to rich given orientation in future
ANGLE GetRotationSpeed(ANGLE aWantedAngle, ANGLE aRotateSpeed, FLOAT fWaitFrequency)
{
  ANGLE aResult;
  // if desired position is smaller
  if ( aWantedAngle<-aRotateSpeed*fWaitFrequency)
  {
    // start decreasing
    aResult = -aRotateSpeed;
  }
  // if desired position is bigger
  else if (aWantedAngle>aRotateSpeed*fWaitFrequency)
  {
    // start increasing
    aResult = +aRotateSpeed;
  }
  // if desired position is more-less ahead
  else
  {
    aResult = aWantedAngle/fWaitFrequency;
  }
  return aResult;
}


/* Receive damage */
void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
                   FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
{
 
  // cannonball immediately destroys demons fireball
  if (m_prtType==PRT_DEMON_FIREBALL && dmtType==DMT_CANNONBALL)
  {
    fDamageAmmount*=10001.0f;
  }
  if (m_prtType==PRT_FLAME && IsOfClass(penInflictor, "Moving Brush"))
  {
    Destroy();    
  }

  CMovableModelEntity::ReceiveDamage(penInflictor, 
    dmtType, fDamageAmmount, vHitPoint, vDirection);
}

/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> PROJECTILE FLY IN SPACE
  ProjectileFly(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      // if flame, continue existing
      /*if (m_prtType==PRT_FLAME && ((CEntity &)*&penObstacle).en_RenderType==RT_MODEL) {
        resume;
      }*/
      return EEnd();
    }
    // fly loop
    wait(m_fFlyTime) {
      on (EBegin) : { resume; }
      on (EPass epass) : {
        BOOL bHit;
        // ignore launcher within 1 second
        bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        // ignore another projectile of same type
        bHit &= !((!m_bCanHitHimself && IsOfClass(epass.penOther, "Projectile") &&
                ((CProjectile*)&*epass.penOther)->m_prtType==m_prtType));
        // ignore twister
        bHit &= !IsOfClass(epass.penOther, "Twister");
        if (bHit) {
          ProjectileTouch(epass.penOther);
          // player flame passes through enemies
          //if (m_prtType==PRT_FLAME && IsDerivedFromClass((CEntity *)&*(epass.penOther), "Enemy Base")) { resume; }
          stop;
        }
        resume;
      }
      on (ETouch etouch) : {
        // clear time limit for launcher
        m_fIgnoreTime = 0.0f;
        // ignore another projectile of same type
        BOOL bHit;
        bHit = !((!m_bCanHitHimself && IsOfClass(etouch.penOther, "Projectile") &&
                 ((CProjectile*)&*etouch.penOther)->m_prtType==m_prtType));     
        
        if (bHit) {
          ProjectileTouch(etouch.penOther);
          stop;
        }
        resume;
      }
      on (EDeath) : {
        if (m_bCanBeDestroyed) {
          ProjectileHit();
          stop;
        }
        resume;
      }
      on (ETimer) : {
        ProjectileHit();
        stop;
      }
    }
    return EEnd();
  };

  // --->>> GUIDED PROJECTILE FLY IN SPACE
  ProjectileGuidedFly(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      return EEnd();
    }
    // fly loop
    while( _pTimer->CurrentTick()<(m_fStartTime+m_fFlyTime))
    {
      FLOAT fWaitFrequency = 0.1f;
      // beast big projectile destroys soon after passing near the player
      /*if (m_prtType==PRT_BEAST_BIG_PROJECTILE &&
          DistanceTo(this, m_penTarget)<20.0f &&
          (m_fStartTime+m_fFlyTime-_pTimer->CurrentTick())>1.5f)
      { 
        m_fFlyTime = _pTimer->CurrentTick() - m_fStartTime + 1.5f;
      }*/
      if (m_penTarget!=NULL) {
        // calculate desired position and angle
        EntityInfo *pei= (EntityInfo*) (m_penTarget->GetEntityInfo());
        FLOAT3D vDesiredPosition;
        GetEntityInfoPosition( m_penTarget, pei->vSourceCenter, vDesiredPosition);
        FLOAT3D vDesiredDirection = (vDesiredPosition-GetPlacement().pl_PositionVector).Normalize();
        // for heading
        ANGLE aWantedHeading = GetRelativeHeading( vDesiredDirection);
        /*if (m_prtType==PRT_BEAST_BIG_PROJECTILE && m_fStartTime+m_fFlyTime-_pTimer->CurrentTick()<1.5f)
        {
          m_aRotateSpeed = 10.0f;
        }*/
        ANGLE aHeading = GetRotationSpeed( aWantedHeading, m_aRotateSpeed, fWaitFrequency);

        // factor used to decrease speed of projectiles oriented opposite of its target
        FLOAT fSpeedDecreasingFactor = ((180-Abs(aWantedHeading))/180.0f);
        // factor used to increase speed when far away from target
        FLOAT fSpeedIncreasingFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/100;
        fSpeedIncreasingFactor = ClampDn(fSpeedIncreasingFactor, 1.0f);
        // decrease speed acodring to target's direction
        FLOAT fMaxSpeed = m_fGuidedMaxSpeedFactor*fSpeedIncreasingFactor;
        FLOAT fMinSpeedRatio = 0.5f;
        FLOAT fWantedSpeed = fMaxSpeed*( fMinSpeedRatio+(1-fMinSpeedRatio)*fSpeedDecreasingFactor);
        // adjust translation velocity
        SetDesiredTranslation( FLOAT3D(0, 0, -fWantedSpeed));
      
        // adjust rotation speed
        m_aRotateSpeed = 75.0f*(1+0.5f*fSpeedDecreasingFactor);
      
        // calculate distance factor
        FLOAT fDistanceFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/50.0;
        fDistanceFactor = ClampUp(fDistanceFactor, 4.0f);
        FLOAT fRNDHeading = (FRnd()-0.5f)*180*fDistanceFactor;
        FLOAT fRNDPitch = (FRnd()-0.5f)*90*fDistanceFactor;

        // if we are looking near direction of target
        if( Abs( aWantedHeading) < 30.0f)
        {
          // calculate pitch speed
          ANGLE aWantedPitch = GetRelativePitch( vDesiredDirection);
          ANGLE aPitch = GetRotationSpeed( aWantedPitch, m_aRotateSpeed*1.5f, fWaitFrequency);
          // adjust heading and pich
          SetDesiredRotation(ANGLE3D(aHeading+fRNDHeading,aPitch+fRNDPitch,0));
        }
        // just adjust heading
        else
        {
          SetDesiredRotation(ANGLE3D(aHeading,fDistanceFactor*40,0));
        }
      }

      wait( fWaitFrequency)
      {
        on (EBegin) : { resume; }
        on (EPass epass) : {
          BOOL bHit;
          // ignore launcher within 1 second
          bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
          // ignore another projectile of same type
          bHit &= !((!m_bCanHitHimself && IsOfClass(epass.penOther, "Projectile") &&
                  ((CProjectile*)&*epass.penOther)->m_prtType==m_prtType));
          // ignore twister
          bHit &= !IsOfClass(epass.penOther, "Twister");
          if (bHit) {
            ProjectileTouch(epass.penOther);
            return EEnd();
          }
          resume;
        }
        on (EDeath) :
        {
          if (m_bCanBeDestroyed)
          {
            ProjectileHit();
            return EEnd();
          }
          resume;
        }
        on (ETimer) :
        {
          stop;
        }
      }
    }
    return EEnd();
  };

  ProjectileGuidedFastFly(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      return EEnd();
    }
    // fly loop
    while( _pTimer->CurrentTick()<(m_fStartTime+m_fFlyTime))
    {
      FLOAT fWaitFrequency = 0.1f;
      // beast big projectile destroys soon after passing near the player
      if (m_prtType==PRT_BEAST_BIG_PROJECTILE &&
          DistanceTo(this, m_penTarget)<20.0f &&
          (m_fStartTime+m_fFlyTime-_pTimer->CurrentTick())>1.5f)
      { 
        m_fFlyTime = _pTimer->CurrentTick() - m_fStartTime + 1.5f;
      }
      if (m_penTarget!=NULL) {
        // calculate desired position and angle
        EntityInfo *pei= (EntityInfo*) (m_penTarget->GetEntityInfo());
        FLOAT3D vDesiredPosition;
        GetEntityInfoPosition( m_penTarget, pei->vSourceCenter, vDesiredPosition);
        FLOAT3D vDesiredDirection = (vDesiredPosition-GetPlacement().pl_PositionVector).Normalize();
        // for heading
        ANGLE aWantedHeading = GetRelativeHeading( vDesiredDirection);
        ANGLE aHeading = GetRotationSpeed( aWantedHeading, 5.0f/*m_aRotateSpeed*/, fWaitFrequency);

        // factor used to decrease speed of projectiles oriented opposite of its target
        FLOAT fSpeedDecreasingFactor = ((180-Abs(aWantedHeading))/180.0f);
        // factor used to increase speed when far away from target
        FLOAT fSpeedIncreasingFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/100;
        fSpeedIncreasingFactor = ClampDn(fSpeedIncreasingFactor, 1.0f);
        // decrease speed acording to target's direction
        FLOAT fMaxSpeed = m_fGuidedMaxSpeedFactor*fSpeedIncreasingFactor;
        FLOAT fMinSpeedRatio = 10.0f;
        FLOAT fWantedSpeed = fMaxSpeed*( fMinSpeedRatio+(1-fMinSpeedRatio)*fSpeedDecreasingFactor);
        // adjust translation velocity
        SetDesiredTranslation( FLOAT3D(0, 0, -fWantedSpeed));
      
        // adjust rotation speed
        m_aRotateSpeed = 110.0f*(1+0.5f*fSpeedDecreasingFactor);
      
        // calculate distance factor
        FLOAT fDistanceFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/50.0;
        fDistanceFactor = ClampUp(fDistanceFactor, 4.0f);
        
        // if we are looking near direction of target
        if( Abs( aWantedHeading) < 30.0f)
        {
          bLockedOn = TRUE;
          // calculate pitch speed
          ANGLE aWantedPitch = GetRelativePitch( vDesiredDirection);
          ANGLE aPitch = GetRotationSpeed( aWantedPitch, m_aRotateSpeed*1.5f, fWaitFrequency);
          // adjust heading and pitch
          SetDesiredRotation(ANGLE3D(aHeading, aPitch, 0));
        }
        // just adjust heading
        else
        {
          if (bLockedOn) // we just missed the player
          {
            ANGLE3D aBankingUp;
            aBankingUp = GetPlacement().pl_OrientationAngle;
            aBankingUp(3) = 0.0f;
            SetPlacement(CPlacement3D(GetPlacement().pl_PositionVector, aBankingUp));
          }
          bLockedOn = FALSE;
          //SetDesiredRotation(ANGLE3D(aHeading,fDistanceFactor*40,0));
          SetDesiredRotation(ANGLE3D(aHeading,400,0));
        }
      }

      wait( fWaitFrequency)
      {
        on (EBegin) : { resume; }
        on (ETouch etouch) : {
          // clear time limit for launcher
          m_fIgnoreTime = 0.0f;
          // ignore itself and the demon
          BOOL bHit;
          bHit = !((!m_bCanHitHimself && IsOfClass(etouch.penOther, "Projectile") &&
            ((CProjectile*)&*etouch.penOther)->m_prtType==m_prtType));     
          bHit &= !IsOfClass(etouch.penOther, "Demon");
          FLOAT3D vTrans = en_vCurrentTranslationAbsolute;
          bHit &= Abs(vTrans.Normalize() % FLOAT3D(etouch.plCollision)) > 0.35f;

          if (bHit) {
            ProjectileTouch(etouch.penOther);
            return EEnd();
          }

          resume;
        }  
        on (EPass epass) : {
          BOOL bHit;
          // ignore launcher within 1 second
          bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
          // ignore another projectile of same type
          bHit &= !((!m_bCanHitHimself && IsOfClass(epass.penOther, "Projectile") &&
                  ((CProjectile*)&*epass.penOther)->m_prtType==m_prtType));
          // ignore twister
          bHit &= !IsOfClass(epass.penOther, "Twister");
          // if demons projectile, ignore all other projectiles
          bHit &= !(m_prtType==PRT_DEMON_FIREBALL && IsOfClass(epass.penOther, "Projectile"));
          bHit &= !(m_prtType==PRT_BEAST_BIG_PROJECTILE && IsOfClass(epass.penOther, "Projectile"));

          if (bHit) {
            ProjectileTouch(epass.penOther);
            return EEnd();
          }
          resume;
        }
        on (EDeath) :
        {
          if (m_bCanBeDestroyed)
          {
            ProjectileHit();
            return EEnd();
          }
          resume;
        }
        on (ETimer) :
        {
          stop;
        }
      }
    }
    return EEnd();
  };

  
  ProjectileGuidedSlide(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      return EEnd();
    }
    // fly loop
    while( _pTimer->CurrentTick()<(m_fStartTime+m_fFlyTime))
    {
      FLOAT fWaitFrequency = 0.1f;
      if (m_penTarget!=NULL) {
        // calculate desired position and angle
        EntityInfo *pei= (EntityInfo*) (m_penTarget->GetEntityInfo());
        FLOAT3D vDesiredPosition;
        GetEntityInfoPosition( m_penTarget, pei->vSourceCenter, vDesiredPosition);
        FLOAT3D vDesiredDirection = (vDesiredPosition-GetPlacement().pl_PositionVector).Normalize();
        // for heading
        ANGLE aWantedHeading = GetRelativeHeading( vDesiredDirection);
        ANGLE aHeading = GetRotationSpeed( aWantedHeading, m_aRotateSpeed, fWaitFrequency);

        // factor used to decrease speed of projectiles oriented opposite of its target
        FLOAT fSpeedDecreasingFactor = ((180-Abs(aWantedHeading))/180.0f);
        // factor used to increase speed when far away from target
        FLOAT fSpeedIncreasingFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/100;
        fSpeedIncreasingFactor = ClampDn(fSpeedIncreasingFactor, 1.0f);
        // decrease speed acodring to target's direction
        FLOAT fMaxSpeed = 30.0f*fSpeedIncreasingFactor;
        FLOAT fMinSpeedRatio = 0.5f;
        FLOAT fWantedSpeed = fMaxSpeed*( fMinSpeedRatio+(1-fMinSpeedRatio)*fSpeedDecreasingFactor);
        // adjust translation velocity
        SetDesiredTranslation( FLOAT3D(0, 0, -fWantedSpeed));
      
        // adjust rotation speed
        m_aRotateSpeed = 75.0f*(1+0.5f*fSpeedDecreasingFactor);
      
        // calculate distance factor
        FLOAT fDistanceFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/50.0;
        fDistanceFactor = ClampUp(fDistanceFactor, 4.0f);
        FLOAT fRNDHeading = (FRnd()-0.5f)*180*fDistanceFactor;
        
        // if we are looking near direction of target
        if( Abs( aWantedHeading) < 30.0f)
        {
          // adjust heading and pich
          SetDesiredRotation(ANGLE3D(aHeading+fRNDHeading,0,0));
        }
        // just adjust heading
        else
        {
          SetDesiredRotation(ANGLE3D(aHeading,0,0));
        }
      }

      wait( fWaitFrequency)
      {
        on (EBegin) : { resume; }
        on (EPass epass) : {
          BOOL bHit;
          // ignore launcher within 1 second
          bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
          // ignore another projectile of same type
          bHit &= !((!m_bCanHitHimself && IsOfClass(epass.penOther, "Projectile") &&
                  ((CProjectile*)&*epass.penOther)->m_prtType==m_prtType));
          // ignore twister
          bHit &= !IsOfClass(epass.penOther, "Twister");
          if (bHit) {
            ProjectileTouch(epass.penOther);
            return EEnd();
          }
          resume;
        }
        on (EDeath) :
        {
          if (m_bCanBeDestroyed)
          {
            ProjectileHit();
            return EEnd();
          }
          resume;
        }
        on (ETimer) :
        {
          stop;
        }
      }
    }
    return EEnd();
  };

  // --->>> PROJECTILE SLIDE ON BRUSH
  ProjectileSlide(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      return EEnd();
    }
    // fly loop
    wait(m_fFlyTime) {
      on (EBegin) : { resume; }
      on (EPass epass) : {
        BOOL bHit;
        // ignore launcher within 1 second
        bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        // ignore another projectile of same type
        bHit &= !((!m_bCanHitHimself && IsOfClass(epass.penOther, "Projectile") &&
                ((CProjectile*)&*epass.penOther)->m_prtType==m_prtType));
        // ignore twister
        bHit &= !IsOfClass(epass.penOther, "Twister");
        if (epass.penOther!=m_penLauncher) {
          // bHit = bHit ; // FIXME: DG: what was this supposed to achieve?
        }
        if (bHit) {
          ProjectileTouch(epass.penOther);
          // player flame passes through enemies
          if (m_prtType==PRT_FLAME && IsDerivedFromClass((CEntity *) (epass.penOther.ep_pen), "Enemy Base")) {
            resume;
          }
          // wind blast passes through movable entities
          if (m_prtType==PRT_AIRELEMENTAL_WIND && IsDerivedFromClass((CEntity *) (epass.penOther.ep_pen), "MovableEntity")) {
            resume;
          }
          
          stop;
        }
        resume;
      }
      on (ETouch etouch) : {
        // clear time limit for launcher
        m_fIgnoreTime = 0.0f;
        // ignore brushes
        BOOL bHit;
        bHit = !(etouch.penOther->GetRenderType() & RT_BRUSH);
        if( m_prtType==PRT_FLAME && !bHit && !m_bLeftFlame)
        {
          SpawnFlame(m_penLauncher, etouch.penOther, GetPlacement().pl_PositionVector);
          m_bLeftFlame=TRUE;
        }
        if (!bHit) { BounceSound(); }
        // ignore another projectile of same type
        bHit &= !((!m_bCanHitHimself && IsOfClass(etouch.penOther, "Projectile") &&
                  ((CProjectile*)&*etouch.penOther)->m_prtType==m_prtType));
        if (bHit) {
          ProjectileTouch(etouch.penOther);
          stop;
        }
        // projectile is moving to slow (stuck somewhere) -> kill it
        if (en_vCurrentTranslationAbsolute.Length() < 0.25f*en_vDesiredTranslationRelative.Length()) {
          ProjectileHit();
          stop;
        }
        resume;
      }
      on (EDeath) : {
        if (m_bCanBeDestroyed) {
          ProjectileHit();
          stop;
        }
        resume;
      }
      on (ETimer) : {
        ProjectileHit();
        stop;
      }
    }
    return EEnd();
  };

  // --->>> PROJECTILE FLY IN SPACE WITH REBOUNDING
  ProjectileFlyRebounding(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      return EEnd();
    }
    // fly loop
    wait(m_fFlyTime) {
      on (EBegin) : { resume; }
      on (EPass epass) : {
        BOOL bHit;
        // ignore launcher within 1 second
        bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        // ignore another projectile of same type
        bHit &= !((!m_bCanHitHimself && IsOfClass(epass.penOther, "Projectile") &&
                ((CProjectile*)&*epass.penOther)->m_prtType==m_prtType));
        // ignore twister
        bHit &= !IsOfClass(epass.penOther, "Twister");
        if (bHit) {
          ProjectileTouch(epass.penOther);
          stop;
        }
        resume;
      }
      on (ETouch etouch) : {
        // clear time limit for launcher
        m_fIgnoreTime = 0.0f;
        
        BOOL bHit;
        
        // if brush hit
        bHit = (etouch.penOther->GetRenderType() == RT_BRUSH);
        
        if (bHit && m_iRebounds>0) {
          //reverse direction
          ReflectDirectionVectorByPlane(etouch.plCollision, en_vCurrentTranslationAbsolute);
          ReflectRotationMatrixByPlane_cols(etouch.plCollision, en_mRotation);
          m_iRebounds--;
        } else {
          // ignore another projectile of same type
          bHit = !((!m_bCanHitHimself && IsOfClass(etouch.penOther, "Projectile") &&
                   ((CProjectile*)&*etouch.penOther)->m_prtType==m_prtType));     
        
          if (bHit) {
            ProjectileTouch(etouch.penOther);
            stop;
          }
        }
        resume;
      }
      on (EDeath) : {
        if (m_bCanBeDestroyed) {
          ProjectileHit();
          stop;
        }
        resume;
      }
      on (ETimer) : {
        ProjectileHit();
        stop;
      }
    }
    return EEnd();
  };

  // --->>> MAIN
  Main(ELaunchProjectile eLaunch) {
    // remember the initial parameters
    ASSERT(eLaunch.penLauncher!=NULL);
    m_penLauncher = eLaunch.penLauncher;
    m_prtType = eLaunch.prtType;
    m_fSpeed = eLaunch.fSpeed;
    m_fStretch=eLaunch.fStretch;
    SetPredictable(TRUE);
    // remember lauching time
    m_fIgnoreTime = _pTimer->CurrentTick() + 1.0f;
    m_penLastDamaged = NULL;

    switch (m_prtType) {
      case PRT_DEVIL_ROCKET:
      case PRT_WALKER_ROCKET:
      case PRT_ROCKET:
      case PRT_SHOOTER_WOODEN_DART:
        {
          Particles_RocketTrail_Prepare(this);
          break;
        }
      case PRT_GUFFY_PROJECTILE: break; //Particles_RocketTrail_Prepare(this); break;
      case PRT_GRENADE: Particles_GrenadeTrail_Prepare(this); break;
      case PRT_CATMAN_FIRE: Particles_RocketTrail_Prepare(this); break;
      case PRT_HEADMAN_FIRECRACKER: Particles_FirecrackerTrail_Prepare(this); break;
      case PRT_HEADMAN_ROCKETMAN: Particles_Fireball01Trail_Prepare(this); break;
      case PRT_HEADMAN_BOMBERMAN: Particles_BombTrail_Prepare(this); break;
      case PRT_LAVA_COMET: Particles_LavaTrail_Prepare(this); break;
      case PRT_LAVAMAN_BIG_BOMB: Particles_LavaBombTrail_Prepare(this); break;
      case PRT_LAVAMAN_BOMB: Particles_LavaBombTrail_Prepare(this); break;
      case PRT_BEAST_PROJECTILE: Particles_Fireball01Trail_Prepare(this); break;
      case PRT_BEAST_BIG_PROJECTILE:
      case PRT_DEVIL_GUIDED_PROJECTILE:
      case PRT_DEMON_FIREBALL:
      //case PRT_METEOR:
         Particles_FirecrackerTrail_Prepare(this);
         break;
      case PRT_SHOOTER_FIREBALL: Particles_Fireball01Trail_Prepare(this); break;
    }
    // projectile initialization
    switch (m_prtType)
    {
      case PRT_WALKER_ROCKET: WalkerRocket(); break;
      case PRT_ROCKET: PlayerRocket(); break;
      case PRT_GRENADE: PlayerGrenade(); break;
      case PRT_FLAME: PlayerFlame(); break;
      case PRT_LASER_RAY: PlayerLaserRay(); break;
      case PRT_CATMAN_FIRE: CatmanProjectile(); break;
      case PRT_HEADMAN_FIRECRACKER: HeadmanFirecracker(); break;
      case PRT_HEADMAN_ROCKETMAN: HeadmanRocketman(); break;
      case PRT_HEADMAN_BOMBERMAN: HeadmanBomberman(); break;
      case PRT_BONEMAN_FIRE: BonemanProjectile(); break;
      case PRT_WOMAN_FIRE: WomanProjectile(); break;
      case PRT_DRAGONMAN_FIRE: DragonmanProjectile(DRAGONMAN_NORMAL); break;
      case PRT_DRAGONMAN_STRONG_FIRE: DragonmanProjectile(DRAGONMAN_STRONG); break;
      case PRT_STONEMAN_FIRE: ElementalRock(ELEMENTAL_NORMAL, ELEMENTAL_STONEMAN); break;
      case PRT_STONEMAN_BIG_FIRE: ElementalRock(ELEMENTAL_BIG, ELEMENTAL_STONEMAN); break;
      case PRT_STONEMAN_LARGE_FIRE: ElementalRock(ELEMENTAL_LARGE, ELEMENTAL_STONEMAN); break;
      case PRT_LAVAMAN_BIG_BOMB: LavaManBomb(); break;
      case PRT_LAVAMAN_BOMB: LavaManBomb(); break;
      case PRT_LAVAMAN_STONE: ElementalRock(ELEMENTAL_NORMAL, ELEMENTAL_LAVAMAN); break;
      case PRT_ICEMAN_FIRE: ElementalRock(ELEMENTAL_NORMAL, ELEMENTAL_ICEMAN); break;
      case PRT_ICEMAN_BIG_FIRE: ElementalRock(ELEMENTAL_BIG, ELEMENTAL_ICEMAN); break;
      case PRT_ICEMAN_LARGE_FIRE: ElementalRock(ELEMENTAL_LARGE, ELEMENTAL_ICEMAN); break;
      case PRT_HUANMAN_FIRE: HuanmanProjectile(); break;
      case PRT_FISHMAN_FIRE: FishmanProjectile(); break;
      case PRT_MANTAMAN_FIRE: MantamanProjectile(); break;
      case PRT_CYBORG_LASER: CyborgLaser(); break;
      case PRT_CYBORG_BOMB: CyborgBomb(); break;
      case PRT_LAVA_COMET: LavaBall(); break;
      case PRT_BEAST_PROJECTILE: BeastProjectile(); break;
      case PRT_BEAST_BIG_PROJECTILE: BeastBigProjectile(); break;
      case PRT_BEAST_DEBRIS: BeastDebris(); break;
      case PRT_BEAST_BIG_DEBRIS: BeastBigDebris(); break;
      case PRT_DEVIL_LASER: DevilLaser(); break;
      case PRT_DEVIL_ROCKET: DevilRocket(); break;
      case PRT_DEVIL_GUIDED_PROJECTILE: DevilGuidedProjectile(); break;
      case PRT_GRUNT_PROJECTILE_SOL: GruntSoldierLaser(); break;
      case PRT_GRUNT_PROJECTILE_COM: GruntCommanderLaser(); break;
      case PRT_GUFFY_PROJECTILE: GuffyProjectile(); break;
      case PRT_DEMON_FIREBALL: DemonFireball(); break;
      case PRT_LARVA_PLASMA: LarvaPlasma(); break;
      case PRT_LARVA_TAIL_PROJECTILE: LarvaTail(); break;
      case PRT_SHOOTER_WOODEN_DART: ShooterWoodenDart(); break;
      case PRT_SHOOTER_FIREBALL: ShooterFireball(); break;
      case PRT_SHOOTER_FLAME: ShooterFlame(); break;
      case PRT_AFTERBURNER_DEBRIS: AfterburnerDebris(); break;
      case PRT_AIRELEMENTAL_WIND: WindBlast(); break;
      case PRT_METEOR: Meteor(); break;
      default: ASSERTALWAYS("Unknown projectile type");
    }

    // setup light source
    if (m_bLightSource) { SetupLightSource(TRUE); }

    // fly
    m_fStartTime = _pTimer->CurrentTick();
    // if guided projectile
    if( m_pmtMove == PMT_GUIDED) {
      autocall ProjectileGuidedFly() EEnd;
    } else if (m_pmtMove==PMT_GUIDED_FAST) {
      autocall ProjectileGuidedFastFly() EEnd;
    } else if (m_pmtMove==PMT_FLYING) {
      autocall ProjectileFly() EEnd;
    } else if (m_pmtMove==PMT_SLIDING) {
      autocall ProjectileSlide() EEnd;
    } else if (m_pmtMove==PMT_FLYING_REBOUNDING) {
      autocall ProjectileFlyRebounding() EEnd;
    } else if (m_pmtMove==PMT_GUIDED_SLIDING) {
      autocall ProjectileGuidedSlide() EEnd;
    }

    // projectile explosion
    switch (m_prtType) {
      case PRT_WALKER_ROCKET: WalkerRocketExplosion(); break;
      case PRT_ROCKET: PlayerRocketExplosion(); break;
      case PRT_GRENADE: PlayerGrenadeExplosion(); break;
      case PRT_LASER_RAY: PlayerLaserWave(); break;
      case PRT_HEADMAN_BOMBERMAN: HeadmanBombermanExplosion(); break;
      case PRT_CYBORG_BOMB: CyborgBombExplosion(); break;
      case PRT_LAVA_COMET: LavamanBombDebrisExplosion(); break;
      case PRT_LAVAMAN_BIG_BOMB: LavamanBombExplosion(); break;
      case PRT_LAVAMAN_BOMB: LavamanBombDebrisExplosion(); break;
      case PRT_BEAST_BIG_PROJECTILE: BeastBigProjectileExplosion(); break;
      case PRT_BEAST_PROJECTILE: BeastProjectileExplosion(); break;
      case PRT_BEAST_DEBRIS: BeastDebrisExplosion(); break;      
      case PRT_BEAST_BIG_DEBRIS: BeastBigDebrisExplosion(); break;      
      case PRT_DEVIL_ROCKET: DevilRocketExplosion(); break;
      case PRT_DEVIL_GUIDED_PROJECTILE: DevilGuidedProjectileExplosion(); break;
      case PRT_GUFFY_PROJECTILE: GuffyProjectileExplosion(); break;
      case PRT_DEMON_FIREBALL: DemonFireballExplosion(); break;  
      case PRT_LARVA_PLASMA: LarvaPlasmaExplosion(); break;
      case PRT_LARVA_TAIL_PROJECTILE: LarvaTailExplosion(); break;
      case PRT_SHOOTER_WOODEN_DART: ShooterWoodenDartExplosion(); break;
      case PRT_SHOOTER_FIREBALL: ShooterFireballExplosion(); break;
      case PRT_METEOR: MeteorExplosion(); break;
    }

    // wait after death
    if (m_fWaitAfterDeath>0.0f) {
      SwitchToEditorModel();
      ForceFullStop();
      SetCollisionFlags(ECF_IMMATERIAL);
      // kill light source
      if (m_bLightSource) { SetupLightSource(FALSE); }
      autowait(m_fWaitAfterDeath);
    }

    Destroy();

    return;
  }
};
