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

0
%{
#include "EntitiesMP/StdH/StdH.h"
%}

/*
 *
 *  --->>>   DON'T INSTANTIATE THIS CLASS   <<<---
 *
 */

event EStop {         // stop your actions
};                    
event EStart {        // start your actions
  CEntityPointer penCaused,   // who caused the trigger (transitive)
};                    
event EActivate {     // activate class (usually touch field)
};
event EDeactivate {   // deactivate class (usually touch field)
};
event EEnvironmentStart {   // activate environment classes
};
event EEnvironmentStop {    // deactivate environment classes
};
event EEnd {          // general purpose end of procedure event
};                    
event ETrigger {      // sent by trigger class
  CEntityPointer penCaused,   // who caused the trigger (transitive)
};
event ETeleportMovingBrush {    // teleport moving brush
};
event EReminder {     // reminder event
  INDEX iValue,       // value for return
};
event EStartAttack {  // OBSOLETE!
};
event EStopAttack {  // OBSOLETE!
};
event EStopBlindness {  // make enemy not blind any more
};
event EStopDeafness {  // make enemy not blind any more
};
event EReceiveScore { // sent to player when enemy is killed
  INDEX iPoints
};
event EKilledEnemy { // sent to player when enemy is killed
};
event ESecretFound { // sent to player secret is found
};

enum BoolEType {
  0 BET_TRUE      "True",   // true
  1 BET_FALSE     "False",  // false
  2 BET_IGNORE    "Ignore", // ignore
};

enum EventEType {
  0 EET_START             "Start event",              // start event
  1 EET_STOP              "Stop event",               // stop event
  2 EET_TRIGGER           "Trigger event",            // trigger event
  3 EET_IGNORE            "Don't send event",         // don't send event (ignore)
  4 EET_ACTIVATE          "Activate event",           // activate event
  5 EET_DEACTIVATE        "Deactivate event",         // deactivate event
  6 EET_ENVIRONMENTSTART  "Start environment event",  // start environment event
  7 EET_ENVIRONMENTSTOP   "Stop environment event",   // stop environment event
  8 EET_STARTATTACK       "OBSOLETE! - Start attack event",       // start attack enemy
  9 EET_STOPATTACK        "OBSOLETE! - Stop attack event",        // stop attack enemy
 10 EET_STOPBLINDNESS     "Stop blindness event",       // enemy stop being blind
 11 EET_STOPDEAFNESS      "Stop deafness event",        // enemy stop being deaf
 12 EET_TELEPORTMOVINGBRUSH "Teleport moving brush",    // moving brush teleporting event
};


// entity info structure enums
enum EntityInfoBodyType {
  1 EIBT_FLESH  "Flesh",
  2 EIBT_WATER  "Water",
  3 EIBT_ROCK   "Rock ",
  4 EIBT_FIRE   "Fire ",
  5 EIBT_AIR    "Air  ",
  6 EIBT_BONES  "Bones",
  7 EIBT_WOOD   "Wood ",
  8 EIBT_METAL  "Metal",
  9 EIBT_ROBOT  "Robot",
 10 EIBT_ICE    "Ice",
};

enum MessageSound {
  0 MSS_NONE   "None",    // no sound
  1 MSS_INFO   "Info",    // just simple info
};

enum ParticleTexture {
  1   PT_STAR01 "Star01",
  2   PT_STAR02 "Star02",
  3   PT_STAR03 "Star03",
  4   PT_STAR04 "Star04",
  5   PT_STAR05 "Star05",
  6   PT_STAR06 "Star06",
  7   PT_STAR07 "Star07",
  8   PT_STAR08 "Star08",
  9   PT_BOUBBLE01 "Boubble01",
  10  PT_BOUBBLE02 "Boubble02",
  11  PT_WATER01 "Water01",
  12  PT_WATER02 "Water02",
  13  PT_SANDFLOW "Sand flow",
  14  PT_WATERFLOW "Water flow",
  15  PT_LAVAFLOW "Lava flow",
};

enum SoundType {
  0 SNDT_NONE         "",     // internal
  1 SNDT_SHOUT        "",     // enemy shout when see player
  2 SNDT_YELL         "",     // enemy is wounded (or death)
  3 SNDT_EXPLOSION    "",     // explosion of rocket or grenade (or similar)
  4 SNDT_PLAYER       "",     // sound from player weapon or player is wounded
};

// hit enum
enum BulletHitType {
  0 BHT_NONE                "",     // none
  1 BHT_FLESH               "",     // flesh
  2 BHT_BRUSH_STONE         "",     // brush stone
  3 BHT_BRUSH_SAND          "",     // brush sand
  4 BHT_BRUSH_WATER         "",     // brush water
  5 BHT_BRUSH_UNDER_WATER   "",     // brush under water
  6 BHT_ACID                "",     // acid
  7 BHT_BRUSH_RED_SAND      "",     // brush red sand
  8 BHT_BRUSH_GRASS         "",     // brush grass
  9 BHT_BRUSH_WOOD          "",     // brush wood
 10 BHT_BRUSH_SNOW          "",     // brush snow
};

enum EffectParticlesType {
  0 EPT_NONE                  "",     // no partcicles
  1 EPT_BULLET_STONE          "",    // bullet particles on stone
  2 EPT_BULLET_SAND           "",    // bullet particles on sand
  3 EPT_BULLET_WATER          "",    // bullet particles on water
  4 EPT_BULLET_UNDER_WATER    "",    // bullet particles underwater
  5 EPT_BULLET_RED_SAND       "",    // bullet particles on red sand
  6 EPT_BULLET_GRASS          "",    // bullet particles on sand
  7 EPT_BULLET_WOOD           "",    // bullet particles on sand
  8 EPT_BULLET_SNOW           "",    // bullet particles on snow
};

enum SprayParticlesType {
  0 SPT_NONE        "None",         // no particles
  1 SPT_BLOOD       "Blood",        // blood
  2 SPT_BONES       "Bones",        // bones
  3 SPT_FEATHER     "Feather",      // feather
  4 SPT_STONES      "Stones",       // stones
  5 SPT_WOOD        "Wood",         // wood
  6 SPT_SLIME       "Slime",        // gizmo/beast slime
  7 SPT_LAVA_STONES "Lava Stones",  // lava stones
  8 SPT_ELECTRICITY_SPARKS "Electricity sparks",  // electricity sparks with blood
  9 SPT_BEAST_PROJECTILE_SPRAY "Beast projectile spray", // beast projectile explosion sparks
 10 SPT_SMALL_LAVA_STONES "Small Lava Stones",  // small lava stones
 11 SPT_AIRSPOUTS   "Air",          // air
 12 SPT_ELECTRICITY_SPARKS_NO_BLOOD "Electricity w/o blood", // no blood electricity
 13 SPT_PLASMA      "Plasma",       // plasma   
 14 SPT_GOO         "Goo",          // yellow bloodlike substance
 15 SPT_TREE01      "Tree 01",      // tree 01
 16 SPT_COLOREDSTONE "Colored stone", // colored stone
};

// weapon bits
enum WeaponBits {
  0 WB_00   "Knife",
  1 WB_01   "Colt",
  2 WB_02   "Double colt",
  3 WB_03   "Single shotgun",
  4 WB_04   "Double shotgun",
  5 WB_05   "Tommygun",
  6 WB_06   "Minigun",
  7 WB_07   "Rocket launcher",
  8 WB_08   "Grenade launcher",
  9 WB_09   "Chainsaw",
 10 WB_10   "Flamer",
 11 WB_11   "Laser",
 12 WB_12   "Sniper",
 13 WB_13   "Ironcannon",
 14 WB_14   "",
 15 WB_15   "",
 16 WB_16   "",
 17 WB_17   "",
 18 WB_18   "",
 19 WB_19   "",
 20 WB_20   "",
 21 WB_21   "",
 22 WB_22   "",
 23 WB_23   "",
 24 WB_24   "",
 25 WB_25   "",
 26 WB_26   "",
 27 WB_27   "",
 28 WB_28   "",
 29 WB_29   "",
 30 WB_30   "",
 31 WB_31   "",
};

// clasification bits
enum ClasificationBits {
 16 CB_00   "Bit 16",
 17 CB_01   "Bit 17",
 18 CB_02   "Bit 18",
 19 CB_03   "Bit 19",
 20 CB_04   "Bit 20",
 21 CB_05   "Bit 21",
 22 CB_06   "Bit 22",
 23 CB_07   "Bit 23",
 24 CB_08   "Bit 24",
 25 CB_09   "Bit 25",
 26 CB_10   "Bit 26",
 27 CB_11   "Bit 27",
 28 CB_12   "Bit 28",
 29 CB_13   "Bit 29",
 30 CB_14   "Bit 30",
 31 CB_15   "Bit 31",
};

// visibility bits
enum VisibilityBits {
  0 VB_00   "Bit 00",
  1 VB_01   "Bit 01",
  2 VB_02   "Bit 02",
  3 VB_03   "Bit 03",
  4 VB_04   "Bit 04",
  5 VB_05   "Bit 05",
  6 VB_06   "Bit 06",
  7 VB_07   "Bit 07",
  8 VB_08   "Bit 08",
  9 VB_09   "Bit 09",
 10 VB_10   "Bit 10",
 11 VB_11   "Bit 11",
 12 VB_12   "Bit 12",
 13 VB_13   "Bit 13",
 14 VB_14   "Bit 14",
 15 VB_15   "Bit 15",
};

event ESound {
  enum SoundType EsndtSound,
  CEntityPointer penTarget,
};

event EScroll {
  BOOL bStart,
  CEntityPointer penSender,
};

event ETextFX {
  BOOL bStart,
  CEntityPointer penSender,
};

event EHudPicFX {
  BOOL bStart,
  CEntityPointer penSender,
};

event ECredits {
  BOOL bStart,
  CEntityPointer penSender,
};

// event for printing centered message
event ECenterMessage {
  CTString strMessage,          // the message
  TIME tmLength,                // how long to keep it
  enum MessageSound mssSound,   // sound to play
};

// event for sending computer message to a player
event EComputerMessage {
  CTFileName fnmMessage,        // the message file
};

// event for voice message to a player
event EVoiceMessage {
  CTFileName fnmMessage,        // the message file
};

event EHitBySpaceShipBeam {
};

class CGlobal : CEntity {
name      "";
thumbnail "";

properties:
components:
functions:
procedures:
  Main(EVoid) {
    ASSERTALWAYS("DON'T INSTANTIATE THIS CLASS");
  }
};