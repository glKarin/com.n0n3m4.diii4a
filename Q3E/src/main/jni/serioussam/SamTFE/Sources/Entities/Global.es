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
#include "Entities/StdH/StdH.h"
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

event ESound {
  enum SoundType EsndtSound,
  CEntityPointer penTarget,
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