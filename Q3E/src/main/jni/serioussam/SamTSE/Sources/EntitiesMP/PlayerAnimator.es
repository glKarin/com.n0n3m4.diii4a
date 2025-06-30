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

406
%{
#include "EntitiesMP/StdH/StdH.h"

#include "ModelsMP/Player/SeriousSam/Player.h"
#include "ModelsMP/Player/SeriousSam/Body.h"
#include "ModelsMP/Player/SeriousSam/Head.h"

#include "Models/Weapons/Knife/KnifeItem.h"
#include "Models/Weapons/Colt/ColtItem.h"
#include "Models/Weapons/Colt/ColtMain.h"
#include "Models/Weapons/SingleShotgun/SingleShotgunItem.h"
#include "Models/Weapons/SingleShotgun/Barrels.h"
#include "Models/Weapons/DoubleShotgun/DoubleShotgunItem.h"
#include "Models/Weapons/DoubleShotgun/Dshotgunbarrels.h"
#include "Models/Weapons/TommyGun/TommyGunItem.h"
#include "Models/Weapons/TommyGun/Body.h"
#include "Models/Weapons/MiniGun/MiniGunItem.h"
#include "Models/Weapons/MiniGun/Body.h"
#include "Models/Weapons/GrenadeLauncher/GrenadeLauncherItem.h"
#include "Models/Weapons/RocketLauncher/RocketLauncherItem.h"
#include "ModelsMP/Weapons/Sniper/SniperItem.h"
#include "ModelsMP/Weapons/Sniper/Sniper.h"
//#include "Models/Weapons/Pipebomb/StickItem.h"
#include "ModelsMP/Weapons/Flamer/FlamerItem.h"
#include "ModelsMP/Weapons/Flamer/Body.h"
//#include "ModelsMP/Weapons/ChainSaw/ChainsawItem.h"
#include "ModelsMP/Weapons/ChainSaw/ChainsawForPlayer.h"
#include "ModelsMP/Weapons/ChainSaw/BladeForPlayer.h"
#include "ModelsMP/Weapons/ChainSaw/Body.h"
#include "Models/Weapons/Laser/LaserItem.h"
//#include "Models/Weapons/GhostBuster/GhostBusterItem.h"
//#include "Models/Weapons/GhostBuster/Effect01.h"
#include "Models/Weapons/Cannon/Cannon.h"
%}

uses "EntitiesMP/Player";
uses "EntitiesMP/PlayerWeapons";

// input parameter for animator
event EAnimatorInit {
  CEntityPointer penPlayer,            // player owns it
};

%{
// animator action
enum AnimatorAction {
  AA_JUMPDOWN = 0,
  AA_CROUCH,
  AA_RISE,
  AA_PULLWEAPON,
  AA_ATTACK,
};

// fire flare specific
#define FLARE_NONE 0
#define FLARE_REMOVE 1
#define FLARE_ADD 2


extern FLOAT plr_fBreathingStrength;
extern FLOAT plr_fViewDampFactor;
extern FLOAT plr_fViewDampLimitGroundUp;
extern FLOAT plr_fViewDampLimitGroundDn;
extern FLOAT plr_fViewDampLimitWater;
extern FLOAT wpn_fRecoilSpeed[17];
extern FLOAT wpn_fRecoilLimit[17];
extern FLOAT wpn_fRecoilDampUp[17];
extern FLOAT wpn_fRecoilDampDn[17];
extern FLOAT wpn_fRecoilOffset[17];
extern FLOAT wpn_fRecoilFactorP[17];
extern FLOAT wpn_fRecoilFactorZ[17];


void CPlayerAnimator_Precache(ULONG ulAvailable)
{
  CDLLEntityClass *pdec = &CPlayerAnimator_DLLClass;

  pdec->PrecacheTexture(TEX_REFL_BWRIPLES01      );
  pdec->PrecacheTexture(TEX_REFL_BWRIPLES02      );
  pdec->PrecacheTexture(TEX_REFL_LIGHTMETAL01    );
  pdec->PrecacheTexture(TEX_REFL_LIGHTBLUEMETAL01);
  pdec->PrecacheTexture(TEX_REFL_DARKMETAL       );
  pdec->PrecacheTexture(TEX_REFL_PURPLE01        );
  pdec->PrecacheTexture(TEX_SPEC_WEAK            );
  pdec->PrecacheTexture(TEX_SPEC_MEDIUM          );
  pdec->PrecacheTexture(TEX_SPEC_STRONG          );
  pdec->PrecacheModel(MODEL_FLARE02);
  pdec->PrecacheTexture(TEXTURE_FLARE02);
  pdec->PrecacheModel(MODEL_GOLDAMON);
  pdec->PrecacheTexture(TEXTURE_GOLDAMON);
  pdec->PrecacheTexture(TEX_REFL_GOLD01);
  pdec->PrecacheClass(CLASS_REMINDER);

  // precache shells that drop when firing
  extern void CPlayerWeaponsEffects_Precache(void);
  CPlayerWeaponsEffects_Precache();

  // precache weapons player has
  if ( ulAvailable&(1<<(WEAPON_KNIFE-1)) ) {
    pdec->PrecacheModel(MODEL_KNIFE                 );
    pdec->PrecacheTexture(TEXTURE_KNIFE);
  }

  if ( ulAvailable&(1<<(WEAPON_COLT-1)) ) {
    pdec->PrecacheModel(MODEL_COLT                  );
    pdec->PrecacheModel(MODEL_COLTCOCK              );
    pdec->PrecacheModel(MODEL_COLTMAIN              );
    pdec->PrecacheModel(MODEL_COLTBULLETS           );
    pdec->PrecacheTexture(TEXTURE_COLTMAIN          );  
    pdec->PrecacheTexture(TEXTURE_COLTBULLETS       );  
    pdec->PrecacheTexture(TEXTURE_COLTBULLETS       );  
  }

  if ( ulAvailable&(1<<(WEAPON_SINGLESHOTGUN-1)) ) {
    pdec->PrecacheModel(MODEL_SINGLESHOTGUN     );    
    pdec->PrecacheModel(MODEL_SS_SLIDER         );    
    pdec->PrecacheModel(MODEL_SS_HANDLE         );    
    pdec->PrecacheModel(MODEL_SS_BARRELS        );    
    pdec->PrecacheTexture(TEXTURE_SS_HANDLE);      
    pdec->PrecacheTexture(TEXTURE_SS_BARRELS);      
  }

  if ( ulAvailable&(1<<(WEAPON_DOUBLESHOTGUN-1)) ) {
    pdec->PrecacheModel(MODEL_DOUBLESHOTGUN        ); 
    pdec->PrecacheModel(MODEL_DS_HANDLE            ); 
    pdec->PrecacheModel(MODEL_DS_BARRELS           ); 
    pdec->PrecacheModel(MODEL_DS_SWITCH            ); 
    pdec->PrecacheTexture(TEXTURE_DS_HANDLE        );   
    pdec->PrecacheTexture(TEXTURE_DS_BARRELS       );   
    pdec->PrecacheTexture(TEXTURE_DS_SWITCH        );   
  }

  if ( ulAvailable&(1<<(WEAPON_TOMMYGUN-1)) ) {
    pdec->PrecacheModel(MODEL_TOMMYGUN              );
    pdec->PrecacheModel(MODEL_TG_BODY               );
    pdec->PrecacheModel(MODEL_TG_SLIDER             );
    pdec->PrecacheTexture(TEXTURE_TG_BODY           );  
  }

  if ( ulAvailable&(1<<(WEAPON_SNIPER-1)) ) {
    pdec->PrecacheModel(MODEL_SNIPER          ); 
    pdec->PrecacheModel(MODEL_SNIPER_BODY     ); 
    pdec->PrecacheTexture(TEXTURE_SNIPER_BODY );   
  }

  if ( ulAvailable&(1<<(WEAPON_MINIGUN-1)) ) {
    pdec->PrecacheModel(MODEL_MINIGUN          );     
    pdec->PrecacheModel(MODEL_MG_BARRELS       );     
    pdec->PrecacheModel(MODEL_MG_BODY          );     
    pdec->PrecacheModel(MODEL_MG_ENGINE        );     
    pdec->PrecacheTexture(TEXTURE_MG_BODY      );       
    pdec->PrecacheTexture(TEXTURE_MG_BARRELS   );       
  }
                                         
  if ( ulAvailable&(1<<(WEAPON_ROCKETLAUNCHER-1)) ) {
    pdec->PrecacheModel(MODEL_ROCKETLAUNCHER   );
    pdec->PrecacheModel(MODEL_RL_BODY          );
    pdec->PrecacheModel(MODEL_RL_ROTATINGPART  );
    pdec->PrecacheModel(MODEL_RL_ROCKET        );
    pdec->PrecacheTexture(TEXTURE_RL_BODY  );
    pdec->PrecacheTexture(TEXTURE_RL_ROCKET);
    pdec->PrecacheTexture(TEXTURE_RL_ROTATINGPART);
  }                                        

  if ( ulAvailable&(1<<(WEAPON_GRENADELAUNCHER-1)) ) {
    pdec->PrecacheModel(MODEL_GRENADELAUNCHER       ); 
    pdec->PrecacheModel(MODEL_GL_BODY               ); 
    pdec->PrecacheModel(MODEL_GL_MOVINGPART         ); 
    pdec->PrecacheModel(MODEL_GL_GRENADE            ); 
    pdec->PrecacheTexture(TEXTURE_GL_BODY           );   
    pdec->PrecacheTexture(TEXTURE_GL_MOVINGPART     );   
  }

/*
  if ( ulAvailable&(1<<(WEAPON_PIPEBOMB-1)) ) {
    pdec->PrecacheModel(MODEL_PIPEBOMB_STICK        );
    pdec->PrecacheModel(MODEL_PB_BUTTON             );
    pdec->PrecacheModel(MODEL_PB_SHIELD             );
    pdec->PrecacheModel(MODEL_PB_STICK              );
    pdec->PrecacheModel(MODEL_PB_BOMB               );
    pdec->PrecacheTexture(TEXTURE_PB_STICK          );  
    pdec->PrecacheTexture(TEXTURE_PB_BOMB           );  
  }
*/
  if ( ulAvailable&(1<<(WEAPON_FLAMER-1)) ) {
    pdec->PrecacheModel(MODEL_FLAMER      );
    pdec->PrecacheModel(MODEL_FL_BODY     );
    pdec->PrecacheModel(MODEL_FL_RESERVOIR);
    pdec->PrecacheModel(MODEL_FL_FLAME    );
    pdec->PrecacheTexture(TEXTURE_FL_BODY );  
    pdec->PrecacheTexture(TEXTURE_FL_FLAME);  
  }
  
  if ( ulAvailable&(1<<(WEAPON_CHAINSAW-1)) ) {
    pdec->PrecacheModel(MODEL_CHAINSAW      );
    pdec->PrecacheModel(MODEL_CS_BODY       );
    pdec->PrecacheModel(MODEL_CS_BLADE      );
    pdec->PrecacheModel(MODEL_CS_TEETH      );
    pdec->PrecacheTexture(TEXTURE_CS_BODY   );  
    pdec->PrecacheTexture(TEXTURE_CS_BLADE  );  
    pdec->PrecacheTexture(TEXTURE_CS_TEETH  );  
  }

  if ( ulAvailable&(1<<(WEAPON_LASER-1)) ) {
    pdec->PrecacheModel(MODEL_LASER     );
    pdec->PrecacheModel(MODEL_LS_BODY   );
    pdec->PrecacheModel(MODEL_LS_BARREL );
    pdec->PrecacheTexture(TEXTURE_LS_BODY  );  
    pdec->PrecacheTexture(TEXTURE_LS_BARREL);  
  }
/*
  if ( ulAvailable&(1<<(WEAPON_GHOSTBUSTER-1)) ) {
    pdec->PrecacheModel(MODEL_GHOSTBUSTER     );
    pdec->PrecacheModel(MODEL_GB_BODY         );
    pdec->PrecacheModel(MODEL_GB_ROTATOR      );
    pdec->PrecacheModel(MODEL_GB_EFFECT1      );
    pdec->PrecacheModel(MODEL_GB_EFFECT1FLARE );
    pdec->PrecacheTexture(TEXTURE_GB_ROTATOR  );  
    pdec->PrecacheTexture(TEXTURE_GB_BODY     );  
    pdec->PrecacheTexture(TEXTURE_GB_LIGHTNING);  
    pdec->PrecacheTexture(TEXTURE_GB_FLARE    );  
  }
*/
  if ( ulAvailable&(1<<(WEAPON_IRONCANNON-1)) /*||
       ulAvailable&(1<<(WEAPON_NUKECANNON-1)) */) {
    pdec->PrecacheModel(MODEL_CANNON    );
    pdec->PrecacheModel(MODEL_CN_BODY   );
//    pdec->PrecacheModel(MODEL_CN_NUKEBOX);
//    pdec->PrecacheModel(MODEL_CN_LIGHT);
    pdec->PrecacheTexture(TEXTURE_CANNON);
  }
}
%}

class export CPlayerAnimator: CRationalEntity {
name      "Player Animator";
thumbnail "";
features "CanBePredictable";

properties:
  1 CEntityPointer m_penPlayer,               // player which owns it

  5 BOOL m_bReference=FALSE,                  // player has reference (floor)
  6 FLOAT m_fLastActionTime = 0.0f,           // last action time for boring weapon animations
  7 INDEX m_iContent = 0,                     // content type index
  8 BOOL m_bWaitJumpAnim = FALSE,             // wait legs anim (for jump end)
  9 BOOL m_bCrouch = FALSE,                   // player crouch state
 10 BOOL m_iCrouchDownWait = FALSE,           // wait for crouch down
 11 BOOL m_iRiseUpWait = FALSE,               // wait for rise up
 12 BOOL m_bChangeWeapon = FALSE,             // wait for weapon change
 13 BOOL m_bSwim = FALSE,                     // player in water
 14 INDEX m_iFlare = FLARE_REMOVE,            // 0-none, 1-remove, 2-add
 15 INDEX m_iSecondFlare = FLARE_REMOVE,      // 0-none, 1-remove, 2-add
 16 BOOL m_bAttacking = FALSE,                // currently firing weapon/swinging knife
 19 FLOAT m_tmAttackingDue = -1.0f,           // when firing animation is due
 17 FLOAT m_tmFlareAdded = -1.0f,             // for better flare add/remove
 18 BOOL m_bDisableAnimating = FALSE,

// player soft eyes on Y axis
 20 FLOAT3D m_vLastPlayerPosition = FLOAT3D(0,0,0), // last player position for eyes movement
 21 FLOAT m_fEyesYLastOffset = 0.0f,                 // eyes offset from player position
 22 FLOAT m_fEyesYOffset = 0.0f,
 23 FLOAT m_fEyesYSpeed = 0.0f,                      // eyes speed
 27 FLOAT m_fWeaponYLastOffset = 0.0f,                 // eyes offset from player position
 28 FLOAT m_fWeaponYOffset = 0.0f,
 29 FLOAT m_fWeaponYSpeed = 0.0f,                      // eyes speed
 // recoil pitch
// 24 FLOAT m_fRecoilLastOffset = 0.0f,   // eyes offset from player position
// 25 FLOAT m_fRecoilOffset = 0.0f,
// 26 FLOAT m_fRecoilSpeed = 0.0f,        // eyes speed

// player banking when moving
 30 BOOL m_bMoving = FALSE,
 31 FLOAT m_fMoveLastBanking = 0.0f,
 32 FLOAT m_fMoveBanking = 0.0f,
 33 BOOL m_iMovingSide = 0,
 34 BOOL m_bSidestepBankingLeft = FALSE,
 35 BOOL m_bSidestepBankingRight = FALSE,
 36 FLOAT m_fSidestepLastBanking = 0.0f,
 37 FLOAT m_fSidestepBanking = 0.0f,
 38 INDEX m_iWeaponLast = -1,
 39 FLOAT m_fBodyAnimTime = -1.0f,

{
  CModelObject *pmoModel;
}

components:
  1 class   CLASS_REMINDER              "Classes\\Reminder.ecl",
// ************** KNIFE **************
 20 model   MODEL_KNIFE                 "Models\\Weapons\\Knife\\KnifeItem.mdl",
 22 texture TEXTURE_KNIFE               "Models\\Weapons\\Knife\\KnifeItem.tex",
 
// ************** COLT **************
 30 model   MODEL_COLT                  "Models\\Weapons\\Colt\\ColtItem.mdl",
 31 model   MODEL_COLTCOCK              "Models\\Weapons\\Colt\\ColtCock.mdl",
 32 model   MODEL_COLTMAIN              "Models\\Weapons\\Colt\\ColtMain.mdl",
 33 model   MODEL_COLTBULLETS           "Models\\Weapons\\Colt\\ColtBullets.mdl",
 34 texture TEXTURE_COLTBULLETS         "Models\\Weapons\\Colt\\ColtBullets.tex",
 35 texture TEXTURE_COLTMAIN            "Models\\Weapons\\Colt\\ColtMain.tex",
 36 texture TEXTURE_COLTCOCK            "Models\\Weapons\\Colt\\ColtCock.tex",

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

// ************** SNIPER **************
110 model   MODEL_SNIPER                "ModelsMP\\Weapons\\Sniper\\Sniper.mdl",
111 model   MODEL_SNIPER_BODY           "ModelsMP\\Weapons\\Sniper\\Body.mdl",
112 texture TEXTURE_SNIPER_BODY         "ModelsMP\\Weapons\\Sniper\\Body.tex",

/*
// ************** PIPEBOMB **************
110 model   MODEL_PIPEBOMB_STICK        "Models\\Weapons\\Pipebomb\\StickItem.mdl",
112 model   MODEL_PB_BUTTON             "Models\\Weapons\\Pipebomb\\Button.mdl",
113 model   MODEL_PB_SHIELD             "Models\\Weapons\\Pipebomb\\Shield.mdl",
114 model   MODEL_PB_STICK              "Models\\Weapons\\Pipebomb\\Stick.mdl",
115 model   MODEL_PB_BOMB               "Models\\Weapons\\Pipebomb\\Bomb.mdl",
116 texture TEXTURE_PB_STICK            "Models\\Weapons\\Pipebomb\\Stick.tex",
117 texture TEXTURE_PB_BOMB             "Models\\Weapons\\Pipebomb\\Bomb.tex",
*/

// ************** FLAMER **************
130 model   MODEL_FLAMER                "ModelsMP\\Weapons\\Flamer\\FlamerItem.mdl",
131 model   MODEL_FL_BODY               "ModelsMP\\Weapons\\Flamer\\Body.mdl",
132 model   MODEL_FL_RESERVOIR          "ModelsMP\\Weapons\\Flamer\\FuelReservoir.mdl",
133 model   MODEL_FL_FLAME              "ModelsMP\\Weapons\\Flamer\\Flame.mdl",
134 texture TEXTURE_FL_BODY             "ModelsMP\\Weapons\\Flamer\\Body.tex",
135 texture TEXTURE_FL_FLAME            "ModelsMP\\Effects\\Flame\\Flame.tex",
136 texture TEXTURE_FL_FUELRESERVOIR    "ModelsMP\\Weapons\\Flamer\\FuelReservoir.tex",

// ************** LASER **************
140 model   MODEL_LASER                 "Models\\Weapons\\Laser\\LaserItem.mdl",
141 model   MODEL_LS_BODY               "Models\\Weapons\\Laser\\Body.mdl",
142 model   MODEL_LS_BARREL             "Models\\Weapons\\Laser\\Barrel.mdl",
143 texture TEXTURE_LS_BODY             "Models\\Weapons\\Laser\\Body.tex",
144 texture TEXTURE_LS_BARREL           "Models\\Weapons\\Laser\\Barrel.tex",

// ************** CHAINSAW **************
150 model   MODEL_CHAINSAW              "ModelsMP\\Weapons\\Chainsaw\\ChainsawForPlayer.mdl",
151 model   MODEL_CS_BODY               "ModelsMP\\Weapons\\Chainsaw\\BodyForPlayer.mdl",
152 model   MODEL_CS_BLADE              "ModelsMP\\Weapons\\Chainsaw\\Blade.mdl",
153 model   MODEL_CS_TEETH              "ModelsMP\\Weapons\\Chainsaw\\Teeth.mdl",
154 texture TEXTURE_CS_BODY             "ModelsMP\\Weapons\\Chainsaw\\Body.tex",
155 texture TEXTURE_CS_BLADE            "ModelsMP\\Weapons\\Chainsaw\\Blade.tex",
156 texture TEXTURE_CS_TEETH            "ModelsMP\\Weapons\\Chainsaw\\Teeth.tex",

// ************** GHOSTBUSTER **************
/*
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

// ************** AMON STATUE **************
180 model   MODEL_GOLDAMON                "Models\\Ages\\Egypt\\Gods\\Amon\\AmonGold.mdl",
181 texture TEXTURE_GOLDAMON              "Models\\Ages\\Egypt\\Gods\\Amon\\AmonGold.tex",

// ************** REFLECTIONS **************
200 texture TEX_REFL_BWRIPLES01         "Models\\ReflectionTextures\\BWRiples01.tex",
201 texture TEX_REFL_BWRIPLES02         "Models\\ReflectionTextures\\BWRiples02.tex",
202 texture TEX_REFL_LIGHTMETAL01       "Models\\ReflectionTextures\\LightMetal01.tex",
203 texture TEX_REFL_LIGHTBLUEMETAL01   "Models\\ReflectionTextures\\LightBlueMetal01.tex",
204 texture TEX_REFL_DARKMETAL          "Models\\ReflectionTextures\\DarkMetal.tex",
205 texture TEX_REFL_PURPLE01           "Models\\ReflectionTextures\\Purple01.tex",
206 texture TEX_REFL_GOLD01               "Models\\ReflectionTextures\\Gold01.tex",

// ************** SPECULAR **************
210 texture TEX_SPEC_WEAK               "Models\\SpecularTextures\\Weak.tex",
211 texture TEX_SPEC_MEDIUM             "Models\\SpecularTextures\\Medium.tex",
212 texture TEX_SPEC_STRONG             "Models\\SpecularTextures\\Strong.tex",

// ************** FLARES **************
250 model   MODEL_FLARE02               "Models\\Effects\\Weapons\\Flare02\\Flare.mdl",
251 texture TEXTURE_FLARE02             "Models\\Effects\\Weapons\\Flare02\\Flare.tex",


functions:
  
  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  { 
    CRationalEntity::Read_t(istr);
  }

  void Precache(void)
  {
    INDEX iAvailableWeapons = ((CPlayerWeapons&)*(((CPlayer&)*m_penPlayer).m_penWeapons)).m_iAvailableWeapons;
    CPlayerAnimator_Precache(iAvailableWeapons);
  }
  
  CPlayer *GetPlayer(void)
  {
    return ((CPlayer*) m_penPlayer.ep_pen);
  }
  CModelObject *GetBody(void)
  {
    CAttachmentModelObject *pamoBody = GetPlayer()->GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO);
    if (pamoBody==NULL) {
      return NULL;
    }
    return &pamoBody->amo_moModelObject;
  }
  CModelObject *GetBodyRen(void)
  {
    CAttachmentModelObject *pamoBody = GetPlayer()->m_moRender.GetAttachmentModel(PLAYER_ATTACHMENT_TORSO);
    if (pamoBody==NULL) {
      return NULL;
    }
    return &pamoBody->amo_moModelObject;
  }

  // Set components
  void SetComponents(CModelObject *mo, ULONG ulIDModel, ULONG ulIDTexture,
                     ULONG ulIDReflectionTexture, ULONG ulIDSpecularTexture, ULONG ulIDBumpTexture) {
    // model data
    mo->SetData(GetModelDataForComponent(ulIDModel));
    // texture data
    mo->mo_toTexture.SetData(GetTextureDataForComponent(ulIDTexture));
    // reflection texture data
    if (ulIDReflectionTexture>0) {
      mo->mo_toReflection.SetData(GetTextureDataForComponent(ulIDReflectionTexture));
    } else {
      mo->mo_toReflection.SetData(NULL);
    }
    // specular texture data
    if (ulIDSpecularTexture>0) {
      mo->mo_toSpecular.SetData(GetTextureDataForComponent(ulIDSpecularTexture));
    } else {
      mo->mo_toSpecular.SetData(NULL);
    }
    // bump texture data
    if (ulIDBumpTexture>0) {
      mo->mo_toBump.SetData(GetTextureDataForComponent(ulIDBumpTexture));
    } else {
      mo->mo_toBump.SetData(NULL);
    }
    ModelChangeNotify();
  };

  // Add attachment model
  void AddAttachmentModel(CModelObject *mo, INDEX iAttachment, ULONG ulIDModel, ULONG ulIDTexture,
                          ULONG ulIDReflectionTexture, ULONG ulIDSpecularTexture, ULONG ulIDBumpTexture) {
    SetComponents(&mo->AddAttachmentModel(iAttachment)->amo_moModelObject, ulIDModel, 
                  ulIDTexture, ulIDReflectionTexture, ulIDSpecularTexture, ulIDBumpTexture);
  };

  // Add weapon attachment
  void AddWeaponAttachment(INDEX iAttachment, ULONG ulIDModel, ULONG ulIDTexture,
                           ULONG ulIDReflectionTexture, ULONG ulIDSpecularTexture, ULONG ulIDBumpTexture) {
    AddAttachmentModel(pmoModel, iAttachment, ulIDModel, ulIDTexture,
                       ulIDReflectionTexture, ulIDSpecularTexture, ulIDBumpTexture);
  };

  // set active attachment (model)
  void SetAttachment(INDEX iAttachment) {
    pmoModel = &(pmoModel->GetAttachmentModel(iAttachment)->amo_moModelObject);
  };

  // synchronize any possible weapon attachment(s) with default appearance
  void SyncWeapon(void)
  {
    CModelObject *pmoBodyRen = GetBodyRen();
    CModelObject *pmoBodyDef = GetBody();
    // for each weapon attachment
    for (INDEX iWeapon = BODY_ATTACHMENT_COLT_RIGHT; iWeapon<=BODY_ATTACHMENT_ITEM; iWeapon++) {
      CAttachmentModelObject *pamoWeapDef = pmoBodyDef->GetAttachmentModel(iWeapon);
      CAttachmentModelObject *pamoWeapRen = pmoBodyRen->GetAttachmentModel(iWeapon);
      // if it doesn't exist in either
      if (pamoWeapRen==NULL && pamoWeapDef==NULL) {
        // just skip it
        NOTHING;

      // if exists only in rendering model
      } else if (pamoWeapRen!=NULL && pamoWeapDef==NULL) {
        // remove it from rendering
        delete pamoWeapRen;

      // if exists only in default
      } else if (pamoWeapRen==NULL && pamoWeapDef!=NULL) {
        // add it to rendering
        pamoWeapRen = pmoBodyRen->AddAttachmentModel(iWeapon);
        pamoWeapRen->amo_plRelative = pamoWeapDef->amo_plRelative;
        pamoWeapRen->amo_moModelObject.Copy(pamoWeapDef->amo_moModelObject);

      // if exists in both
      } else {
        // just synchronize
        pamoWeapRen->amo_plRelative = pamoWeapDef->amo_plRelative;
        pamoWeapRen->amo_moModelObject.Synchronize(pamoWeapDef->amo_moModelObject);
      }
    }
  }

  // set weapon
  void SetWeapon(void) {
    INDEX iWeapon = ((CPlayerWeapons&)*(((CPlayer&)*m_penPlayer).m_penWeapons)).m_iCurrentWeapon;
    m_iWeaponLast = iWeapon;
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
    switch (iWeapon) {
    // *********** KNIFE ***********
      case WEAPON_KNIFE:
        AddWeaponAttachment(BODY_ATTACHMENT_KNIFE, MODEL_KNIFE,
                            TEXTURE_KNIFE, TEX_REFL_BWRIPLES02, TEX_SPEC_WEAK, 0);
        break;

    // *********** DOUBLE COLT ***********
      case WEAPON_DOUBLECOLT:
        AddWeaponAttachment(BODY_ATTACHMENT_COLT_LEFT, MODEL_COLT, TEXTURE_COLTMAIN, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_COLT_LEFT);
        AddWeaponAttachment(COLTITEM_ATTACHMENT_BULLETS, MODEL_COLTBULLETS,
                            TEXTURE_COLTBULLETS, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(COLTITEM_ATTACHMENT_COCK, MODEL_COLTCOCK,
                            TEXTURE_COLTCOCK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(COLTITEM_ATTACHMENT_BODY, MODEL_COLTMAIN,
                            TEXTURE_COLTMAIN, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        SetAttachment(COLTITEM_ATTACHMENT_BODY);
        AddWeaponAttachment(COLTMAIN_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        // reset to player body
        pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);

    // *********** COLT ***********
      case WEAPON_COLT:
        AddWeaponAttachment(BODY_ATTACHMENT_COLT_RIGHT, MODEL_COLT, TEXTURE_COLTMAIN, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_COLT_RIGHT);
        AddWeaponAttachment(COLTITEM_ATTACHMENT_BULLETS, MODEL_COLTBULLETS,
                            TEXTURE_COLTBULLETS, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(COLTITEM_ATTACHMENT_COCK, MODEL_COLTCOCK,
                            TEXTURE_COLTCOCK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(COLTITEM_ATTACHMENT_BODY, MODEL_COLTMAIN,
                            TEXTURE_COLTMAIN, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        SetAttachment(COLTITEM_ATTACHMENT_BODY);
        AddWeaponAttachment(COLTMAIN_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        break;

    // *********** SINGLE SHOTGUN ***********
      case WEAPON_SINGLESHOTGUN:
        AddWeaponAttachment(BODY_ATTACHMENT_SINGLE_SHOTGUN, MODEL_SINGLESHOTGUN, TEXTURE_SS_HANDLE, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_SINGLE_SHOTGUN);
        AddWeaponAttachment(SINGLESHOTGUNITEM_ATTACHMENT_BARRELS, MODEL_SS_BARRELS,
                            TEXTURE_SS_BARRELS, TEX_REFL_DARKMETAL, TEX_SPEC_WEAK, 0);
        AddWeaponAttachment(SINGLESHOTGUNITEM_ATTACHMENT_HANDLE, MODEL_SS_HANDLE,
                            TEXTURE_SS_HANDLE, TEX_REFL_DARKMETAL, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(SINGLESHOTGUNITEM_ATTACHMENT_SLIDER, MODEL_SS_SLIDER,
                            TEXTURE_SS_BARRELS, TEX_REFL_DARKMETAL, TEX_SPEC_MEDIUM, 0);
        SetAttachment(SINGLESHOTGUNITEM_ATTACHMENT_BARRELS);
        AddWeaponAttachment(BARRELS_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        break;

    // *********** DOUBLE SHOTGUN ***********
      case WEAPON_DOUBLESHOTGUN:
        AddWeaponAttachment(BODY_ATTACHMENT_DOUBLE_SHOTGUN, MODEL_DOUBLESHOTGUN, TEXTURE_DS_HANDLE, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_DOUBLE_SHOTGUN);
        AddWeaponAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_BARRELS, MODEL_DS_BARRELS,
                            TEXTURE_DS_BARRELS, TEX_REFL_BWRIPLES01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_HANDLE, MODEL_DS_HANDLE,
                            TEXTURE_DS_HANDLE, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_SWITCH, MODEL_DS_SWITCH,
                            TEXTURE_DS_SWITCH, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        SetAttachment(DOUBLESHOTGUNITEM_ATTACHMENT_BARRELS);
        AddWeaponAttachment(DSHOTGUNBARRELS_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        break;


    // *********** TOMMYGUN ***********
      case WEAPON_TOMMYGUN:
        AddWeaponAttachment(BODY_ATTACHMENT_TOMMYGUN, MODEL_TOMMYGUN, TEXTURE_TG_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_TOMMYGUN);
        AddWeaponAttachment(TOMMYGUNITEM_ATTACHMENT_BODY, MODEL_TG_BODY, TEXTURE_TG_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(TOMMYGUNITEM_ATTACHMENT_SLIDER, MODEL_TG_SLIDER, TEXTURE_TG_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        SetAttachment(TOMMYGUNITEM_ATTACHMENT_BODY);
        AddWeaponAttachment(BODY_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        break;

    // *********** SNIPER ***********
      case WEAPON_SNIPER:
        AddWeaponAttachment(BODY_ATTACHMENT_FLAMER, MODEL_SNIPER, TEXTURE_SNIPER_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_FLAMER);
        AddWeaponAttachment(SNIPERITEM_ATTACHMENT_BODY, MODEL_SNIPER_BODY, TEXTURE_SNIPER_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        SetAttachment(SNIPERITEM_ATTACHMENT_BODY);
        AddWeaponAttachment(BODY_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        break;

    // *********** MINIGUN ***********
      case WEAPON_MINIGUN:
        AddWeaponAttachment(BODY_ATTACHMENT_MINIGUN, MODEL_MINIGUN, TEXTURE_MG_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_MINIGUN);
        AddWeaponAttachment(MINIGUNITEM_ATTACHMENT_BARRELS, MODEL_MG_BARRELS, TEXTURE_MG_BARRELS, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0); 
        AddWeaponAttachment(MINIGUNITEM_ATTACHMENT_BODY, MODEL_MG_BODY, TEXTURE_MG_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);          
        AddWeaponAttachment(MINIGUNITEM_ATTACHMENT_ENGINE, MODEL_MG_ENGINE, TEXTURE_MG_BARRELS, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);      
        SetAttachment(MINIGUNITEM_ATTACHMENT_BODY);
        AddWeaponAttachment(BODY_ATTACHMENT_FLARE, MODEL_FLARE02, TEXTURE_FLARE02, 0, 0, 0);
        break;

    // *********** ROCKET LAUNCHER ***********
      case WEAPON_ROCKETLAUNCHER:
        AddWeaponAttachment(BODY_ATTACHMENT_ROCKET_LAUNCHER, MODEL_ROCKETLAUNCHER, TEXTURE_RL_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_ROCKET_LAUNCHER);
        AddWeaponAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_BODY, MODEL_RL_BODY, TEXTURE_RL_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROTATINGPART, MODEL_RL_ROTATINGPART, TEXTURE_RL_ROTATINGPART, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET1, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET2, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET3, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(ROCKETLAUNCHERITEM_ATTACHMENT_ROCKET4, MODEL_RL_ROCKET, TEXTURE_RL_ROCKET, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        break;

    // *********** GRENADE LAUNCHER ***********
      case WEAPON_GRENADELAUNCHER:
        AddWeaponAttachment(BODY_ATTACHMENT_GRENADE_LAUNCHER, MODEL_GRENADELAUNCHER, TEXTURE_GL_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_GRENADE_LAUNCHER);
        AddWeaponAttachment(GRENADELAUNCHERITEM_ATTACHMENT_BODY, MODEL_GL_BODY, TEXTURE_GL_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(GRENADELAUNCHERITEM_ATTACHMENT_MOVING_PART, MODEL_GL_MOVINGPART, TEXTURE_GL_MOVINGPART, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(GRENADELAUNCHERITEM_ATTACHMENT_GRENADE, MODEL_GL_GRENADE, TEXTURE_GL_MOVINGPART, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        break;

/*    // *********** PIPEBOMB ***********
      case WEAPON_PIPEBOMB:
        AddWeaponAttachment(BODY_ATTACHMENT_COLT_RIGHT, MODEL_PIPEBOMB_STICK, TEXTURE_PB_STICK, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_COLT_RIGHT);
        AddWeaponAttachment(STICKITEM_ATTACHMENT_STICK, MODEL_PB_STICK, TEXTURE_PB_STICK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(STICKITEM_ATTACHMENT_SHIELD, MODEL_PB_SHIELD, TEXTURE_PB_STICK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(STICKITEM_ATTACHMENT_BUTTON, MODEL_PB_BUTTON, TEXTURE_PB_STICK, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        // reset to player body
        pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
        AddWeaponAttachment(BODY_ATTACHMENT_COLT_LEFT, MODEL_PB_BOMB, TEXTURE_PB_BOMB, TEX_REFL_LIGHTBLUEMETAL01, TEX_SPEC_MEDIUM, 0);
        break;
*/
    // *********** FLAMER ***********
      case WEAPON_FLAMER:
        AddWeaponAttachment(BODY_ATTACHMENT_FLAMER, MODEL_FLAMER, TEXTURE_FL_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_FLAMER);
        AddWeaponAttachment(FLAMERITEM_ATTACHMENT_BODY, MODEL_FL_BODY, TEXTURE_FL_BODY, TEX_REFL_BWRIPLES02, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(FLAMERITEM_ATTACHMENT_FUEL, MODEL_FL_RESERVOIR, TEXTURE_FL_FUELRESERVOIR, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(FLAMERITEM_ATTACHMENT_FLAME, MODEL_FL_FLAME, TEXTURE_FL_FLAME, 0, 0, 0);
        break;

    // *********** CHAINSAW ***********
      case WEAPON_CHAINSAW: {
        AddWeaponAttachment(BODY_ATTACHMENT_MINIGUN, MODEL_CHAINSAW, TEXTURE_CS_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_MINIGUN);
        AddWeaponAttachment(CHAINSAWFORPLAYER_ATTACHMENT_CHAINSAW, MODEL_CS_BODY, TEXTURE_CS_BODY, TEX_REFL_BWRIPLES02, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(CHAINSAWFORPLAYER_ATTACHMENT_BLADE, MODEL_CS_BLADE, TEXTURE_CS_BLADE, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        CModelObject *pmo = pmoModel;
        SetAttachment(CHAINSAWFORPLAYER_ATTACHMENT_BLADE);
        AddWeaponAttachment(BLADEFORPLAYER_ATTACHMENT_TEETH, MODEL_CS_TEETH, TEXTURE_CS_TEETH, 0, 0, 0);
        break; }

    // *********** LASER ***********
      case WEAPON_LASER:
        AddWeaponAttachment(BODY_ATTACHMENT_LASER, MODEL_LASER, TEXTURE_LS_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_LASER);
        AddWeaponAttachment(LASERITEM_ATTACHMENT_BODY, MODEL_LS_BODY, TEXTURE_LS_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(LASERITEM_ATTACHMENT_LEFTUP,    MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(LASERITEM_ATTACHMENT_LEFTDOWN,  MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(LASERITEM_ATTACHMENT_RIGHTUP,   MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(LASERITEM_ATTACHMENT_RIGHTDOWN, MODEL_LS_BARREL, TEXTURE_LS_BARREL, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        break;

/*
    // *********** GHOSTBUSTER ***********
      case WEAPON_GHOSTBUSTER: {
        AddWeaponAttachment(BODY_ATTACHMENT_LASER, MODEL_GHOSTBUSTER, TEXTURE_GB_BODY, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_LASER);
        AddWeaponAttachment(GHOSTBUSTERITEM_ATTACHMENT_BODY, MODEL_GB_BODY, TEXTURE_GB_BODY, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(GHOSTBUSTERITEM_ATTACHMENT_ROTATOR, MODEL_GB_ROTATOR, TEXTURE_GB_ROTATOR, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        AddWeaponAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT01, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        AddWeaponAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT02, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        AddWeaponAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT03, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        AddWeaponAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT04, MODEL_GB_EFFECT1, TEXTURE_GB_LIGHTNING, 0, 0, 0);
        CModelObject *pmo = pmoModel;
        SetAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT01);
        AddWeaponAttachment(EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        pmoModel = pmo;
        SetAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT02);
        AddWeaponAttachment(EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        pmoModel = pmo;
        SetAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT03);
        AddWeaponAttachment(EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        pmoModel = pmo;
        SetAttachment(GHOSTBUSTERITEM_ATTACHMENT_EFFECT04);
        AddWeaponAttachment(EFFECT01_ATTACHMENT_FLARE, MODEL_GB_EFFECT1FLARE, TEXTURE_GB_FLARE, 0, 0, 0);
        break; }
*/
    // *********** CANNON ***********
      case WEAPON_IRONCANNON:
//      case WEAPON_NUKECANNON:
        AddWeaponAttachment(BODY_ATTACHMENT_CANNON, MODEL_CANNON, TEXTURE_CANNON, 0, 0, 0);
        SetAttachment(BODY_ATTACHMENT_CANNON);
        AddWeaponAttachment(CANNON_ATTACHMENT_BODY, MODEL_CN_BODY, TEXTURE_CANNON, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
//        AddWeaponAttachment(CANNON_ATTACHMENT_NUKEBOX, MODEL_CN_NUKEBOX, TEXTURE_CANNON, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
//        AddWeaponAttachment(CANNON_ATTACHMENT_LIGHT, MODEL_CN_LIGHT, TEXTURE_CANNON, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
        break;
      default:
        ASSERTALWAYS("Unknown weapon.");
    }
    // sync apperances
    SyncWeapon();
  };

  // set item
  void SetItem(CModelObject *pmo) {
    pmoModel = &(GetPlayer()->GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
    AddWeaponAttachment(BODY_ATTACHMENT_ITEM, MODEL_GOLDAMON,
                        TEXTURE_GOLDAMON, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
    if (pmo!=NULL) {
      CPlayer &pl = (CPlayer&)*m_penPlayer;
      CAttachmentModelObject *pamo = pl.GetModelObject()->GetAttachmentModelList(PLAYER_ATTACHMENT_TORSO, BODY_ATTACHMENT_ITEM, -1);
      pmoModel = &(pamo->amo_moModelObject);
      pmoModel->Copy(*pmo);
      pmoModel->StretchModel(FLOAT3D(1,1,1));
      pamo->amo_plRelative = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
    }
    // sync apperances
    SyncWeapon();
  }

  // set player body animation
  void SetBodyAnimation(INDEX iAnimation, ULONG ulFlags) {
    // on weapon change skip anim
    if (m_bChangeWeapon) { return; }
    // on firing skip anim
    if (m_bAttacking) { return; }
    // play body anim
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    CModelObject &moBody = pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject;
    moBody.PlayAnim(iAnimation, ulFlags);
    m_fBodyAnimTime = moBody.GetAnimLength(iAnimation);     // anim length
  };


/************************************************************
 *                      INITIALIZE                          *
 ************************************************************/
  void Initialize(void) {
    // set internal properties
    m_bReference = TRUE;
    m_bWaitJumpAnim = FALSE;
    m_bCrouch = FALSE;
    m_iCrouchDownWait = 0;
    m_iRiseUpWait = 0;
    m_bChangeWeapon = FALSE;
    m_bSwim = FALSE;
    m_bAttacking = FALSE;

    // clear eyes offsets
    m_fEyesYLastOffset = 0.0f;
    m_fEyesYOffset = 0.0f;
    m_fEyesYSpeed = 0.0f;
    m_fWeaponYLastOffset = 0.0f;
    m_fWeaponYOffset = 0.0f;
    m_fWeaponYSpeed = 0.0f;
//    m_fRecoilLastOffset = 0.0f;
//    m_fRecoilOffset = 0.0f;
//    m_fRecoilSpeed = 0.0f;
    
    // clear moving banking
    m_bMoving = FALSE;
    m_fMoveLastBanking = 0.0f;
    m_fMoveBanking = 0.0f;
    m_iMovingSide = 0;
    m_bSidestepBankingLeft = FALSE;
    m_bSidestepBankingRight = FALSE;
    m_fSidestepLastBanking = 0.0f;
    m_fSidestepBanking = 0.0f;

    // weapon
    SetWeapon();
    SetBodyAnimation(BODY_ANIM_COLT_STAND, AOF_LOOPING|AOF_NORESTART);
  };


/************************************************************
 *                ANIMATE BANKING AND SOFT EYES             *
 ************************************************************/
  // store for lerping
  void StoreLast(void) {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    m_vLastPlayerPosition = pl.GetPlacement().pl_PositionVector;  // store last player position
    m_fEyesYLastOffset = m_fEyesYOffset;                          // store last eyes offset
    m_fWeaponYLastOffset = m_fWeaponYOffset;
//    m_fRecoilLastOffset = m_fRecoilOffset;
    m_fMoveLastBanking = m_fMoveBanking;                          // store last banking for lerping
    m_fSidestepLastBanking = m_fSidestepBanking;
  };

  // animate banking
  void AnimateBanking(void) {
    // moving -> change banking
    if (m_bMoving) {
      // move banking left
      if (m_iMovingSide == 0) {
        m_fMoveBanking += 0.35f;
        if (m_fMoveBanking > 1.0f) { 
          m_fMoveBanking = 1.0f;
          m_iMovingSide = 1;
        }
      // move banking right
      } else {
        m_fMoveBanking -= 0.35f;
        if (m_fMoveBanking < -1.0f) {
          m_fMoveBanking = -1.0f;
          m_iMovingSide = 0;
        }
      }
      const FLOAT fBankingSpeed = 0.4f;

      // sidestep banking left
      if (m_bSidestepBankingLeft) {
        m_fSidestepBanking += fBankingSpeed;
        if (m_fSidestepBanking > 1.0f) { m_fSidestepBanking = 1.0f; }
      }
      // sidestep banking right
      if (m_bSidestepBankingRight) {
        m_fSidestepBanking -= fBankingSpeed;
        if (m_fSidestepBanking < -1.0f) { m_fSidestepBanking = -1.0f; }
      }

    // restore banking
    } else {
      // move banking
      if (m_fMoveBanking > 0.0f) {
        m_fMoveBanking -= 0.1f;
        if (m_fMoveBanking < 0.0f) { m_fMoveBanking = 0.0f; }
      } else if (m_fMoveBanking < 0.0f) {
        m_fMoveBanking += 0.1f;
        if (m_fMoveBanking > 0.0f) { m_fMoveBanking = 0.0f; }
      }
      // sidestep banking
      if (m_fSidestepBanking > 0.0f) {
        m_fSidestepBanking -= 0.4f;
        if (m_fSidestepBanking < 0.0f) { m_fSidestepBanking = 0.0f; }
      } else if (m_fSidestepBanking < 0.0f) {
        m_fSidestepBanking += 0.4f;
        if (m_fSidestepBanking > 0.0f) { m_fSidestepBanking = 0.0f; }
      }
    }

    if (GetPlayer()->GetSettings()->ps_ulFlags&PSF_NOBOBBING) {
      m_fSidestepBanking = m_fMoveBanking = 0.0f;
    }
  };

  // animate soft eyes
  void AnimateSoftEyes(void) {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    // find eyes offset and speed (differential formula realized in numerical mathematics)
    FLOAT fRelY = (pl.GetPlacement().pl_PositionVector-m_vLastPlayerPosition) %
                  FLOAT3D(pl.en_mRotation(1, 2), pl.en_mRotation(2, 2), pl.en_mRotation(3, 2));

    // if just jumped
    if (pl.en_tmJumped>_pTimer->CurrentTick()-0.5f) {
      fRelY = ClampUp(fRelY, 0.0f);
    }
    m_fEyesYOffset -= fRelY;
    m_fWeaponYOffset -= ClampUp(fRelY, 0.0f);

    plr_fViewDampFactor      = Clamp(plr_fViewDampFactor      ,0.0f,1.0f);
    plr_fViewDampLimitGroundUp = Clamp(plr_fViewDampLimitGroundUp ,0.0f,2.0f);
    plr_fViewDampLimitGroundDn = Clamp(plr_fViewDampLimitGroundDn ,0.0f,2.0f);
    plr_fViewDampLimitWater  = Clamp(plr_fViewDampLimitWater  ,0.0f,2.0f);

    m_fEyesYSpeed = (m_fEyesYSpeed - m_fEyesYOffset*plr_fViewDampFactor) * (1.0f-plr_fViewDampFactor);
    m_fEyesYOffset += m_fEyesYSpeed;
    
    m_fWeaponYSpeed = (m_fWeaponYSpeed - m_fWeaponYOffset*plr_fViewDampFactor) * (1.0f-plr_fViewDampFactor);
    m_fWeaponYOffset += m_fWeaponYSpeed;

    if (m_bSwim) {
      m_fEyesYOffset = Clamp(m_fEyesYOffset, -plr_fViewDampLimitWater,  +plr_fViewDampLimitWater);
      m_fWeaponYOffset = Clamp(m_fWeaponYOffset, -plr_fViewDampLimitWater,  +plr_fViewDampLimitWater);
    } else {
      m_fEyesYOffset = Clamp(m_fEyesYOffset, -plr_fViewDampLimitGroundDn,  +plr_fViewDampLimitGroundUp);
      m_fWeaponYOffset = Clamp(m_fWeaponYOffset, -plr_fViewDampLimitGroundDn,  +plr_fViewDampLimitGroundUp);
    }
  };

  /*
  // animate view pitch (for recoil)
  void AnimateRecoilPitch(void)
  {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    INDEX iWeapon = ((CPlayerWeapons&)*pl.m_penWeapons).m_iCurrentWeapon;

    wpn_fRecoilDampUp[iWeapon] = Clamp(wpn_fRecoilDampUp[iWeapon],0.0f,1.0f);
    wpn_fRecoilDampDn[iWeapon] = Clamp(wpn_fRecoilDampDn[iWeapon],0.0f,1.0f);

    FLOAT fDamp;
    if (m_fRecoilSpeed>0) {
      fDamp = wpn_fRecoilDampUp[iWeapon];
    } else {
      fDamp = wpn_fRecoilDampDn[iWeapon];
    }
    m_fRecoilSpeed = (m_fRecoilSpeed - m_fRecoilOffset*fDamp)* (1.0f-fDamp);

    m_fRecoilOffset += m_fRecoilSpeed;

    if (m_fRecoilOffset<0.0f) {
      m_fRecoilOffset = 0.0f;
    }
    if (m_fRecoilOffset>wpn_fRecoilLimit[iWeapon]) {
      m_fRecoilOffset = wpn_fRecoilLimit[iWeapon];
      m_fRecoilSpeed = 0.0f;
    }
  };
  */
  // change view
  void ChangeView(CPlacement3D &pl) {
    TIME tmNow = _pTimer->GetLerpedCurrentTick();

    if (!(GetPlayer()->GetSettings()->ps_ulFlags&PSF_NOBOBBING)) {
      // banking
      FLOAT fBanking = Lerp(m_fMoveLastBanking, m_fMoveBanking, _pTimer->GetLerpFactor());
      fBanking = fBanking * fBanking * Sgn(fBanking) * 0.25f;
      fBanking += Lerp(m_fSidestepLastBanking, m_fSidestepBanking, _pTimer->GetLerpFactor());
      fBanking = Clamp(fBanking, -5.0f, 5.0f);
      pl.pl_OrientationAngle(3) += fBanking;
    }

/*
    // recoil pitch
    INDEX iWeapon = ((CPlayerWeapons&)*((CPlayer&)*m_penPlayer).m_penWeapons).m_iCurrentWeapon;
    FLOAT fRecoil = Lerp(m_fRecoilLastOffset, m_fRecoilOffset, _pTimer->GetLerpFactor());
    FLOAT fRecoilP = wpn_fRecoilFactorP[iWeapon]*fRecoil;
    pl.pl_OrientationAngle(2) += fRecoilP;
    // adjust recoil pitch handle
    FLOAT fRecoilH = wpn_fRecoilOffset[iWeapon];
    FLOAT fDY = fRecoilH*(1.0f-Cos(fRecoilP));
    FLOAT fDZ = fRecoilH*Sin(fRecoilP);
    pl.pl_PositionVector(2)-=fDY;
    pl.pl_PositionVector(3)+=fDZ+wpn_fRecoilFactorZ[iWeapon]*fRecoil;
    */

    // swimming
    if (m_bSwim) {
      pl.pl_OrientationAngle(1) += sin(tmNow*0.9)*2.0f;
      pl.pl_OrientationAngle(2) += sin(tmNow*1.7)*2.0f;
      pl.pl_OrientationAngle(3) += sin(tmNow*2.5)*2.0f;
    }
    // eyes up/down for jumping and breathing
    FLOAT fEyesOffsetY = Lerp(m_fEyesYLastOffset, m_fEyesYOffset, _pTimer->GetLerpFactor());
    fEyesOffsetY+= sin(tmNow*1.5)*0.05f * plr_fBreathingStrength;
    fEyesOffsetY = Clamp(fEyesOffsetY, -1.0f, 1.0f);
    pl.pl_PositionVector(2) += fEyesOffsetY;
  }



/************************************************************
 *                     ANIMATE PLAYER                       *
 ************************************************************/
  // body and head animation
  void BodyAndHeadOrientation(CPlacement3D &plView) {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    CAttachmentModelObject *pamoBody = pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO);
    ANGLE3D a = plView.pl_OrientationAngle;
    if (!(pl.GetFlags()&ENF_ALIVE)) {
      a = ANGLE3D(0,0,0);
    }
    pamoBody->amo_plRelative.pl_OrientationAngle = a;
    pamoBody->amo_plRelative.pl_OrientationAngle(3) *= 4.0f;
    
    CAttachmentModelObject *pamoHead = (pamoBody->amo_moModelObject).GetAttachmentModel(BODY_ATTACHMENT_HEAD);
    pamoHead->amo_plRelative.pl_OrientationAngle = a;
    pamoHead->amo_plRelative.pl_OrientationAngle(1) = 0.0f;
    pamoHead->amo_plRelative.pl_OrientationAngle(2) = 0.0f;
    pamoHead->amo_plRelative.pl_OrientationAngle(3) *= 4.0f;

    // forbid players from cheating by kissing their @$$
    const FLOAT fMaxBanking = 5.0f;
    pamoBody->amo_plRelative.pl_OrientationAngle(3) = Clamp(pamoBody->amo_plRelative.pl_OrientationAngle(3), -fMaxBanking, fMaxBanking);
    pamoHead->amo_plRelative.pl_OrientationAngle(3) = Clamp(pamoHead->amo_plRelative.pl_OrientationAngle(3), -fMaxBanking, fMaxBanking);
  };

  // animate player
  void AnimatePlayer(void) {
    if (m_bDisableAnimating) {
      return;
    }
    CPlayer &pl = (CPlayer&)*m_penPlayer;

    FLOAT3D vDesiredTranslation = pl.en_vDesiredTranslationRelative;
    FLOAT3D vCurrentTranslation = pl.en_vCurrentTranslationAbsolute * !pl.en_mRotation;
    ANGLE3D aDesiredRotation = pl.en_aDesiredRotationRelative;
    //ANGLE3D aCurrentRotation = pl.en_aCurrentRotationAbsolute;

    // if player is moving
    if (vDesiredTranslation.ManhattanNorm()>0.01f
      ||aDesiredRotation.ManhattanNorm()>0.01f) {
      // prevent idle weapon animations
      m_fLastActionTime = _pTimer->CurrentTick();
    }

    // swimming
    if (m_bSwim) {
      if (vDesiredTranslation.Length()>1.0f && vCurrentTranslation.Length()>1.0f) {
        pl.StartModelAnim(PLAYER_ANIM_SWIM, AOF_LOOPING|AOF_NORESTART);
      } else {
        pl.StartModelAnim(PLAYER_ANIM_SWIMIDLE, AOF_LOOPING|AOF_NORESTART);
      }
      BodyStillAnimation();

    // stand
    } else {
      // has reference (floor)
      if (m_bReference) {
        // jump
        if (pl.en_tmJumped+_pTimer->TickQuantum>=_pTimer->CurrentTick() &&
            pl.en_tmJumped<=_pTimer->CurrentTick()) {
          m_bReference = FALSE;
          pl.StartModelAnim(PLAYER_ANIM_JUMPSTART, AOF_NORESTART);
          BodyStillAnimation();
          m_fLastActionTime = _pTimer->CurrentTick();

        // not in jump anim and in stand mode change
        } else if (!m_bWaitJumpAnim && m_iCrouchDownWait==0 && m_iRiseUpWait==0) {
          // standing
          if (!m_bCrouch) {
            // running anim
            if (vDesiredTranslation.Length()>5.0f && vCurrentTranslation.Length()>5.0f) {
              if (vCurrentTranslation(3)<0) {
                pl.StartModelAnim(PLAYER_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
              } else {
                pl.StartModelAnim(PLAYER_ANIM_BACKPEDALRUN, AOF_LOOPING|AOF_NORESTART);
              }
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // walking anim
            } else if (vDesiredTranslation.Length()>2.0f && vCurrentTranslation.Length()>2.0f) {
              if (vCurrentTranslation(3)<0) {
                pl.StartModelAnim(PLAYER_ANIM_NORMALWALK, AOF_LOOPING|AOF_NORESTART);
              } else {
                pl.StartModelAnim(PLAYER_ANIM_BACKPEDAL, AOF_LOOPING|AOF_NORESTART);
              }
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // left rotation anim
            } else if (aDesiredRotation(1)>0.5f) {
              pl.StartModelAnim(PLAYER_ANIM_TURNLEFT, AOF_LOOPING|AOF_NORESTART);
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // right rotation anim
            } else if (aDesiredRotation(1)<-0.5f) {
              pl.StartModelAnim(PLAYER_ANIM_TURNRIGHT, AOF_LOOPING|AOF_NORESTART);
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // standing anim
            } else {
              pl.StartModelAnim(PLAYER_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
              BodyStillAnimation();
            }
          // crouch
          } else {
            // walking anim
            if (vDesiredTranslation.Length()>2.0f && vCurrentTranslation.Length()>2.0f) {
              if (vCurrentTranslation(3)<0) {
                pl.StartModelAnim(PLAYER_ANIM_CROUCH_WALK, AOF_LOOPING|AOF_NORESTART);
              } else {
                pl.StartModelAnim(PLAYER_ANIM_CROUCH_WALKBACK, AOF_LOOPING|AOF_NORESTART);
              }
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // left rotation anim
            } else if (aDesiredRotation(1)>0.5f) {
              pl.StartModelAnim(PLAYER_ANIM_CROUCH_TURNLEFT, AOF_LOOPING|AOF_NORESTART);
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // right rotation anim
            } else if (aDesiredRotation(1)<-0.5f) {
              pl.StartModelAnim(PLAYER_ANIM_CROUCH_TURNRIGHT, AOF_LOOPING|AOF_NORESTART);
              BodyStillAnimation();
              m_fLastActionTime = _pTimer->CurrentTick();
            // standing anim
            } else {
              pl.StartModelAnim(PLAYER_ANIM_CROUCH_IDLE, AOF_LOOPING|AOF_NORESTART);
              BodyStillAnimation();
            }
          }

        }

      // no reference (in air)
      } else {                           
        // touched reference
        if (pl.en_penReference!=NULL) {
          m_bReference = TRUE;
          pl.StartModelAnim(PLAYER_ANIM_JUMPEND, AOF_NORESTART);
          BodyStillAnimation();
          SpawnReminder(this, pl.GetModelObject()->GetAnimLength(PLAYER_ANIM_JUMPEND), (INDEX) AA_JUMPDOWN);
          m_bWaitJumpAnim = TRUE;
        }
      }
    }

    // boring weapon animation
    if (_pTimer->CurrentTick()-m_fLastActionTime > 10.0f) {
      m_fLastActionTime = _pTimer->CurrentTick();
      ((CPlayerWeapons&)*pl.m_penWeapons).SendEvent(EBoringWeapon());
    }

    // moving view change
    // translating -> change banking
    if (m_bReference && vDesiredTranslation.Length()>1.0f && vCurrentTranslation.Length()>1.0f) {
      m_bMoving = TRUE;
      // sidestep banking
      FLOAT vSidestepSpeedDesired = vDesiredTranslation(1);
      FLOAT vSidestepSpeedCurrent = vCurrentTranslation(1);
      // right
      if (vSidestepSpeedDesired>1.0f && vSidestepSpeedCurrent>1.0f) {
        m_bSidestepBankingRight = TRUE;
        m_bSidestepBankingLeft = FALSE;
      // left
      } else if (vSidestepSpeedDesired<-1.0f && vSidestepSpeedCurrent<-1.0f) {
        m_bSidestepBankingLeft = TRUE;
        m_bSidestepBankingRight = FALSE;
      // none
      } else {
        m_bSidestepBankingLeft = FALSE;
        m_bSidestepBankingRight = FALSE;
      }
    // in air (space) or not moving
    } else {
      m_bMoving = FALSE;
      m_bSidestepBankingLeft = FALSE;
      m_bSidestepBankingRight = FALSE;
    }
  };

  // crouch
  void Crouch(void) {
    if (m_bDisableAnimating) {
      return;
    }
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pl.StartModelAnim(PLAYER_ANIM_CROUCH, AOF_NORESTART);
    SpawnReminder(this, pl.GetModelObject()->GetAnimLength(PLAYER_ANIM_CROUCH), (INDEX) AA_CROUCH);
    m_iCrouchDownWait++;
    m_bCrouch = TRUE;
	m_bSwim = FALSE;
  };

  // rise
  void Rise(void) {
    if (m_bDisableAnimating) {
      return;
    }
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pl.StartModelAnim(PLAYER_ANIM_RISE, AOF_NORESTART);
    SpawnReminder(this, pl.GetModelObject()->GetAnimLength(PLAYER_ANIM_RISE), (INDEX) AA_RISE);
    m_iRiseUpWait++;
    m_bCrouch = FALSE;
	m_bSwim = FALSE;
  };

  // fall
  void Fall(void) {
    if (m_bDisableAnimating) {
      return;
    }
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pl.StartModelAnim(PLAYER_ANIM_JUMPSTART, AOF_NORESTART);
    if (_pNetwork->ga_ulDemoMinorVersion>6) { m_bCrouch = FALSE; }
    m_bReference = FALSE;
  };

  // swim
  void Swim(void) {
    if (m_bDisableAnimating) {
      return;
    }
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pl.StartModelAnim(PLAYER_ANIM_SWIM, AOF_LOOPING|AOF_NORESTART);
    if (_pNetwork->ga_ulDemoMinorVersion>2) { m_bCrouch = FALSE; }
    m_bSwim = TRUE;
  };

  // stand
  void Stand(void) {
    if (m_bDisableAnimating) {
      return;
    }
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pl.StartModelAnim(PLAYER_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
    if (_pNetwork->ga_ulDemoMinorVersion>2) { m_bCrouch = FALSE; }
    m_bSwim = FALSE;
  };

  // fire/attack
  void FireAnimation(INDEX iAnim, ULONG ulFlags) {
    if (m_bSwim) {
      INDEX iWeapon = ((CPlayerWeapons&)*(((CPlayer&)*m_penPlayer).m_penWeapons)).m_iCurrentWeapon;
      switch (iWeapon) {
        case WEAPON_NONE:
          break;
        case WEAPON_KNIFE: case WEAPON_COLT: case WEAPON_DOUBLECOLT: //case WEAPON_PIPEBOMB:
          iAnim += BODY_ANIM_COLT_SWIM_STAND-BODY_ANIM_COLT_STAND;
          break;
        case WEAPON_SINGLESHOTGUN: case WEAPON_DOUBLESHOTGUN: case WEAPON_TOMMYGUN:
        case WEAPON_SNIPER: case WEAPON_LASER: case WEAPON_FLAMER: //case WEAPON_GHOSTBUSTER:
          iAnim += BODY_ANIM_SHOTGUN_SWIM_STAND-BODY_ANIM_SHOTGUN_STAND;
          break;
        case WEAPON_MINIGUN: case WEAPON_ROCKETLAUNCHER: case WEAPON_GRENADELAUNCHER:
        case WEAPON_IRONCANNON: case WEAPON_CHAINSAW: // case WEAPON_NUKECANNON:
          iAnim += BODY_ANIM_MINIGUN_SWIM_STAND-BODY_ANIM_MINIGUN_STAND;
          break;
      }
    }
    m_bAttacking = FALSE;
    m_bChangeWeapon = FALSE;
    SetBodyAnimation(iAnim, ulFlags);
    if (!(ulFlags&AOF_LOOPING)) {
      SpawnReminder(this, m_fBodyAnimTime, (INDEX) AA_ATTACK);
      m_tmAttackingDue = _pTimer->CurrentTick()+m_fBodyAnimTime;
    }
    m_bAttacking = TRUE;
  };
  void FireAnimationOff(void) {
    m_bAttacking = FALSE;
  };


  
/************************************************************
 *                  CHANGE BODY ANIMATION                   *
 ************************************************************/
  // body animation template
  void BodyAnimationTemplate(INDEX iNone, INDEX iColt, INDEX iShotgun, INDEX iMinigun, ULONG ulFlags) {
    INDEX iWeapon = ((CPlayerWeapons&)*(((CPlayer&)*m_penPlayer).m_penWeapons)).m_iCurrentWeapon;
    switch (iWeapon) {
      case WEAPON_NONE:
        SetBodyAnimation(iNone, ulFlags);
        break;
      case WEAPON_KNIFE: case WEAPON_COLT: case WEAPON_DOUBLECOLT: // case WEAPON_PIPEBOMB:
        if (m_bSwim) { iColt += BODY_ANIM_COLT_SWIM_STAND-BODY_ANIM_COLT_STAND; }
        SetBodyAnimation(iColt, ulFlags);
        break;
      case WEAPON_SINGLESHOTGUN: case WEAPON_DOUBLESHOTGUN: case WEAPON_TOMMYGUN:
      case WEAPON_SNIPER: case WEAPON_LASER: case WEAPON_FLAMER: //case WEAPON_GHOSTBUSTER:
        if (m_bSwim) { iShotgun += BODY_ANIM_SHOTGUN_SWIM_STAND-BODY_ANIM_SHOTGUN_STAND; }
        SetBodyAnimation(iShotgun, ulFlags);
        break;
      case WEAPON_MINIGUN: case WEAPON_ROCKETLAUNCHER: case WEAPON_GRENADELAUNCHER:
      case WEAPON_IRONCANNON: case WEAPON_CHAINSAW: // case WEAPON_NUKECANNON:
        if (m_bSwim) { iMinigun+=BODY_ANIM_MINIGUN_SWIM_STAND-BODY_ANIM_MINIGUN_STAND; }
        SetBodyAnimation(iMinigun, ulFlags);
        break;
      default: ASSERTALWAYS("Player Animator - Unknown weapon");
    }
  };

  // walk
  void BodyWalkAnimation() {
    BodyAnimationTemplate(BODY_ANIM_NORMALWALK, 
      BODY_ANIM_COLT_STAND, BODY_ANIM_SHOTGUN_STAND, BODY_ANIM_MINIGUN_STAND, 
      AOF_LOOPING|AOF_NORESTART);
  };

  // stand
  void BodyStillAnimation() {
    BodyAnimationTemplate(BODY_ANIM_WAIT, 
      BODY_ANIM_COLT_STAND, BODY_ANIM_SHOTGUN_STAND, BODY_ANIM_MINIGUN_STAND, 
      AOF_LOOPING|AOF_NORESTART);
  };

  // push weapon
  void BodyPushAnimation() {
    m_bAttacking = FALSE;
    m_bChangeWeapon = FALSE;
    BodyAnimationTemplate(BODY_ANIM_WAIT, 
      BODY_ANIM_COLT_REDRAW, BODY_ANIM_SHOTGUN_REDRAW, BODY_ANIM_MINIGUN_REDRAW, 0);
    m_bChangeWeapon = TRUE;
  };

  // remove weapon attachment
  void RemoveWeapon(void) 
  {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
    switch (m_iWeaponLast) {
      case WEAPON_NONE:
      case WEAPON_KNIFE:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_KNIFE);
        break;
      case WEAPON_DOUBLECOLT:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_COLT_LEFT);
        // reset to player body
        pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
      case WEAPON_COLT:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_COLT_RIGHT);
        break;
      case WEAPON_SINGLESHOTGUN:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_SINGLE_SHOTGUN);
        break;
      case WEAPON_DOUBLESHOTGUN:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_DOUBLE_SHOTGUN);
        break;
      case WEAPON_TOMMYGUN:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_TOMMYGUN);
        break;
      case WEAPON_SNIPER:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_FLAMER);
        break;
      case WEAPON_MINIGUN:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_MINIGUN);
        break;
      case WEAPON_ROCKETLAUNCHER:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_ROCKET_LAUNCHER);
        break;
      case WEAPON_GRENADELAUNCHER:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_GRENADE_LAUNCHER);
        break;
/*    case WEAPON_PIPEBOMB:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_COLT_RIGHT);
        // reset to player body
        pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_COLT_LEFT);
        break;      */
      case WEAPON_FLAMER:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_FLAMER);
        break;
      case WEAPON_CHAINSAW:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_MINIGUN);
        break;
      case WEAPON_LASER:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_LASER);
        break;
/*    case WEAPON_GHOSTBUSTER:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_LASER);
        break;      */
      case WEAPON_IRONCANNON:
//      case WEAPON_NUKECANNON:
        pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_CANNON);
        break;
      default:
        ASSERT(FALSE);
    }
    // sync apperances
    SyncWeapon();
  }

  // pull weapon
  void BodyPullAnimation() {
    // remove old weapon
    RemoveWeapon();

    // set new weapon
    SetWeapon();

    // pull weapon
    m_bChangeWeapon = FALSE;
    BodyAnimationTemplate(BODY_ANIM_WAIT, 
      BODY_ANIM_COLT_DRAW, BODY_ANIM_SHOTGUN_DRAW, BODY_ANIM_MINIGUN_DRAW, 0);
    INDEX iWeapon = ((CPlayerWeapons&)*(((CPlayer&)*m_penPlayer).m_penWeapons)).m_iCurrentWeapon;
    if (iWeapon!=WEAPON_NONE) {
      m_bChangeWeapon = TRUE;
      SpawnReminder(this, m_fBodyAnimTime, (INDEX) AA_PULLWEAPON);
    }
    // sync apperances
    SyncWeapon();
  };

  // pull item
  void BodyPullItemAnimation() {
    // remove old weapon
    RemoveWeapon();

    // pull item
    m_bChangeWeapon = FALSE;
    SetBodyAnimation(BODY_ANIM_STATUE_PULL, 0);
    m_bChangeWeapon = TRUE;
    SpawnReminder(this, m_fBodyAnimTime, (INDEX) AA_PULLWEAPON);
    // sync apperances
    SyncWeapon();
  };

  // pick item
  void BodyPickItemAnimation() {
    // remove old weapon
    RemoveWeapon();

    // pick item
    m_bChangeWeapon = FALSE;
    SetBodyAnimation(BODY_ANIM_KEYLIFT, 0);
    m_bChangeWeapon = TRUE;
    SpawnReminder(this, m_fBodyAnimTime, (INDEX) AA_PULLWEAPON);
    // sync apperances
    SyncWeapon();
  };

  // remove item
  void BodyRemoveItem() {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    pmoModel = &(pl.GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject);
    pmoModel->RemoveAttachmentModel(BODY_ATTACHMENT_ITEM);
    // sync apperances
    SyncWeapon();
  };


/************************************************************
 *                      FIRE FLARE                          *
 ************************************************************/
  void OnPreRender(void) {
    ControlFlareAttachment();

    // Minigun Specific
    CPlayerWeapons &plw = (CPlayerWeapons&)*(((CPlayer&)*m_penPlayer).m_penWeapons);
    if (plw.m_iCurrentWeapon==WEAPON_MINIGUN) {
      ANGLE aAngle = Lerp(plw.m_aMiniGunLast, plw.m_aMiniGun, _pTimer->GetLerpFactor());
      // rotate minigun barrels
      CPlayer &pl = (CPlayer&)*m_penPlayer;
      CAttachmentModelObject *pamo = pl.GetModelObject()->GetAttachmentModelList(
        PLAYER_ATTACHMENT_TORSO, BODY_ATTACHMENT_MINIGUN, MINIGUNITEM_ATTACHMENT_BARRELS, -1);
      if (pamo!=NULL) {
        pamo->amo_plRelative.pl_OrientationAngle(3) = aAngle;
      }
    }
  };

  // show flare
  void ShowFlare(INDEX iAttachWeapon, INDEX iAttachObject, INDEX iAttachFlare) {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    CAttachmentModelObject *pamo = pl.GetModelObject()->GetAttachmentModelList(
      PLAYER_ATTACHMENT_TORSO, iAttachWeapon, iAttachObject, iAttachFlare, -1);
    if (pamo!=NULL) {
      pamo->amo_plRelative.pl_OrientationAngle(3) = (rand()*360.0f)/(float)(RAND_MAX);
      CModelObject &mo = pamo->amo_moModelObject;
      mo.StretchModel(FLOAT3D(1, 1, 1));
    }
  };

  // hide flare
  void HideFlare(INDEX iAttachWeapon, INDEX iAttachObject, INDEX iAttachFlare) {
    CPlayer &pl = (CPlayer&)*m_penPlayer;
    CAttachmentModelObject *pamo = pl.GetModelObject()->GetAttachmentModelList(
      PLAYER_ATTACHMENT_TORSO, iAttachWeapon, iAttachObject, iAttachFlare, -1);
    if (pamo!=NULL) {
      CModelObject &mo = pamo->amo_moModelObject;
      mo.StretchModel(FLOAT3D(0, 0, 0));
    }
  };

  // flare attachment
  void ControlFlareAttachment(void) 
  {
/*    if(!IsPredictionHead()) {
      return;
    }
    */

    // get your prediction tail
    CPlayerAnimator *pen = (CPlayerAnimator *)GetPredictionTail();

    INDEX iWeapon = ((CPlayerWeapons&)*(((CPlayer&)*pen->m_penPlayer).m_penWeapons)).m_iCurrentWeapon;
    // second colt only
    if (iWeapon==WEAPON_DOUBLECOLT) {
      // add flare
      if (pen->m_iSecondFlare==FLARE_ADD) {
        pen->m_iSecondFlare = FLARE_REMOVE;
        ShowFlare(BODY_ATTACHMENT_COLT_LEFT, COLTITEM_ATTACHMENT_BODY, COLTMAIN_ATTACHMENT_FLARE);
      // remove flare
      } else if (m_iSecondFlare==FLARE_REMOVE) {
        HideFlare(BODY_ATTACHMENT_COLT_LEFT, COLTITEM_ATTACHMENT_BODY, COLTMAIN_ATTACHMENT_FLARE);
      }
    }

    // add flare
    if (pen->m_iFlare==FLARE_ADD) {
      pen->m_iFlare = FLARE_REMOVE;
      pen->m_tmFlareAdded = _pTimer->CurrentTick();
      switch(iWeapon) {
        case WEAPON_DOUBLECOLT: case WEAPON_COLT:
          ShowFlare(BODY_ATTACHMENT_COLT_RIGHT, COLTITEM_ATTACHMENT_BODY, COLTMAIN_ATTACHMENT_FLARE);
          break;
        case WEAPON_SINGLESHOTGUN:
          ShowFlare(BODY_ATTACHMENT_SINGLE_SHOTGUN, SINGLESHOTGUNITEM_ATTACHMENT_BARRELS, BARRELS_ATTACHMENT_FLARE);
          break;
        case WEAPON_DOUBLESHOTGUN:
          ShowFlare(BODY_ATTACHMENT_DOUBLE_SHOTGUN, DOUBLESHOTGUNITEM_ATTACHMENT_BARRELS, DSHOTGUNBARRELS_ATTACHMENT_FLARE);
          break;
        case WEAPON_TOMMYGUN:
          ShowFlare(BODY_ATTACHMENT_TOMMYGUN, TOMMYGUNITEM_ATTACHMENT_BODY, BODY_ATTACHMENT_FLARE);
          break;
        case WEAPON_SNIPER:
          ShowFlare(BODY_ATTACHMENT_FLAMER, SNIPERITEM_ATTACHMENT_BODY, BODY_ATTACHMENT_FLARE);
          break;
        case WEAPON_MINIGUN:
          ShowFlare(BODY_ATTACHMENT_MINIGUN, MINIGUNITEM_ATTACHMENT_BODY, BODY_ATTACHMENT_FLARE);
          break;
      }
    // remove
    } else if (m_iFlare==FLARE_REMOVE &&
      _pTimer->CurrentTick()>pen->m_tmFlareAdded+_pTimer->TickQuantum) {
      switch(iWeapon) {
        case WEAPON_DOUBLECOLT: case WEAPON_COLT:
          HideFlare(BODY_ATTACHMENT_COLT_RIGHT, COLTITEM_ATTACHMENT_BODY, COLTMAIN_ATTACHMENT_FLARE);
          break;
        case WEAPON_SINGLESHOTGUN:
          HideFlare(BODY_ATTACHMENT_SINGLE_SHOTGUN, SINGLESHOTGUNITEM_ATTACHMENT_BARRELS, BARRELS_ATTACHMENT_FLARE);
          break;
        case WEAPON_DOUBLESHOTGUN:
          HideFlare(BODY_ATTACHMENT_DOUBLE_SHOTGUN, DOUBLESHOTGUNITEM_ATTACHMENT_BARRELS, DSHOTGUNBARRELS_ATTACHMENT_FLARE);
          break;
        case WEAPON_TOMMYGUN:
          HideFlare(BODY_ATTACHMENT_TOMMYGUN, TOMMYGUNITEM_ATTACHMENT_BODY, BODY_ATTACHMENT_FLARE);
          break;
        case WEAPON_SNIPER:
          HideFlare(BODY_ATTACHMENT_FLAMER, SNIPERITEM_ATTACHMENT_BODY, BODY_ATTACHMENT_FLARE);
          break;
        case WEAPON_MINIGUN:
          HideFlare(BODY_ATTACHMENT_MINIGUN, MINIGUNITEM_ATTACHMENT_BODY, BODY_ATTACHMENT_FLARE);
          break;
      }
    }
  };



/************************************************************
 *                      PROCEDURES                          *
 ************************************************************/
procedures:
  ReminderAction(EReminder er) {
    switch (er.iValue) {
      case AA_JUMPDOWN: m_bWaitJumpAnim = FALSE; break;
      case AA_CROUCH: m_iCrouchDownWait--; ASSERT(m_iCrouchDownWait>=0); break;
      case AA_RISE: m_iRiseUpWait--; ASSERT(m_iRiseUpWait>=0); break;
      case AA_PULLWEAPON: m_bChangeWeapon = FALSE; break;
      case AA_ATTACK: if(m_tmAttackingDue<=_pTimer->CurrentTick()) { m_bAttacking = FALSE; } break;
      default: ASSERTALWAYS("Animator - unknown reminder action.");
    }
    return EBegin();
  };

  Main(EAnimatorInit eInit) {
    // remember the initial parameters
    ASSERT(eInit.penPlayer!=NULL);
    m_penPlayer = eInit.penPlayer;

    // declare yourself as a void
    InitAsVoid();
    SetFlags(GetFlags()|ENF_CROSSESLEVELS);
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // last action time for boring weapon animation
    m_fLastActionTime = _pTimer->CurrentTick();

    wait() {
      on (EBegin) : { resume; }
      on (EReminder er) : { call ReminderAction(er); }
      on (EEnd) : { stop; }
    }

    // cease to exist
    Destroy();

    return;
  };
};

