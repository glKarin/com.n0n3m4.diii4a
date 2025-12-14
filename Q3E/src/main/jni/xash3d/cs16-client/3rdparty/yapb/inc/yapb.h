//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/crlib.h>

#include <linkage/goldsrc.h>
#include <linkage/metamod.h>
#include <linkage/physint.h>

// use all the cr-library
using namespace cr;

#include <product.h>
#include <module.h>
#include <constant.h>
#include <chatlib.h>

// tasks definition
struct BotTask {
   using Function = void (Bot:: *) ();

public:
   Function func {}; // corresponding exec function in bot class
   Task id {}; // major task/action carried out
   float desire {}; // desire (filled in) for this task
   int data {}; // additional data (node index)
   float time {}; // time task expires
   bool resume {}; // if task can be continued if interrupted

public:
   BotTask (Function func, Task id, float desire, int data, float time, bool resume) : func (func), id (id), desire (desire), data (data), time (time), resume (resume) {}
};

// weapon properties structure
struct WeaponProp {
   String classname {};
   int ammo1 {}; // ammo index for primary ammo
   int ammo1Max {}; // max primary ammo
   int slot {}; // HUD slot (0 based)
   int pos {}; // slot position
   int id {}; // weapon ID
   int flags {}; // flags???
};

// weapon info structure
struct WeaponInfo {
   int id {}; // the weapon id value
   StringRef name {}; // name of the weapon when selecting it
   StringRef model {}; // model name to separate cs weapons
   StringRef alias {}; // alias name for weapon
   int price {}; // price when buying
   int minPrimaryAmmo {}; // minimum primary ammo
   int teamStandard {}; // used by team (number) (standard map)
   int teamAS {}; // used by team (as map)
   int buyGroup {}; // group in buy menu (standard map)
   int buySelect {}; // select item in buy menu (standard map)
   int buySelectT {}; // for counter-strike v1.6
   int buySelectCT {}; // for counter-strike v1.6
   int penetratePower {}; // penetrate power
   int maxClip {}; // max ammo in clip
   int type {}; // weapon class
   bool primaryFireHold {}; // hold down primary fire button to use?

public:
   WeaponInfo (int id,
      StringRef name,
      StringRef model,
      int price,
      int minPriAmmo,
      int teamStd,
      int teamAs,
      int buyGroup,
      int buySelect,
      int buySelectT,
      int buySelectCT,
      int penetratePower,
      int maxClip,
      int type,
      bool fireHold) : id (id), name (name), model (model), price (price), minPrimaryAmmo (minPriAmmo), teamStandard (teamStd),
      teamAS (teamAs), buyGroup (buyGroup), buySelect (buySelect), buySelectT (buySelectT), buySelectCT (buySelectCT),
      penetratePower (penetratePower), maxClip (maxClip), type (type), primaryFireHold (fireHold) {}
};

// clients noise
struct ClientNoise {
   Vector pos {};
   float dist {};
   float last {};
};

// array of clients struct
struct Client {
   edict_t *ent {}; // pointer to actual edict
   Vector origin {}; // position in the world
   int team {}; // bot team
   int team2 {}; // real bot team in free for all mode (csdm)
   int flags {}; // client flags
   int radio {}; // radio orders
   int menu {}; // identifier to opened menu
   int iconFlags[kGameMaxPlayers] {}; // flag holding chatter icons
   float iconTimestamp[kGameMaxPlayers] {}; // timers for chatter icons
   ClientNoise noise {};
};

// think delay mapping
struct FrameDelay {
   float interval {};
   float time {};
};

// shared team data for bot
struct BotTeamData {
   bool leaderChoosen {}; // is team leader choose thees round
   bool positiveEco {};  // is team able to buy anything
   float lastRadioTimestamp {}; // global radio time
   int32_t lastRadioSlot = { kInvalidRadioSlot }; // last radio message for team
};

// bot difficulty data
struct BotDifficultyData {
   float reaction[2] {};
   int32_t headshotPct {};
   int32_t seenThruPct {};
   int32_t hearThruPct {};
   int32_t maxRecoil {};
   Vector aimError {};
};

// include bot graph stuff
#include <graph.h>
#include <vision.h>

// this structure links nodes returned from pathfinder
class PathWalk final : public NonCopyable {
private:
   size_t m_cursor {};
   size_t m_length {};

   UniquePtr <int32_t[]> m_path {};

public:
   explicit PathWalk () = default;
   ~PathWalk () = default;

public:
   int32_t &nextX2 () {
      return at (2);
   }

   int32_t &next () {
      return at (1);
   }

   int32_t &first () {
      return at (0);
   }

   int32_t &last () {
      return at (length () - 1);
   }

   int32_t &at (size_t index) {
      return m_path[m_cursor + index];
   }

   void shift () {
      ++m_cursor;
   }

   void reverse () {
      for (size_t i = 0; i < m_length / 2; ++i) {
         cr::swap (m_path[i], m_path[m_length - 1 - i]);
      }
   }

   size_t length () const {
      if (m_cursor >= m_length) {
         return 0;
      }
      return m_length - m_cursor;
   }

   bool hasNext () const {
      return length () - m_cursor > 1;
   }

   bool empty () const {
      return !length ();
   }

   void add (int32_t node) {
      m_path[m_length++] = node;
   }

   void clear () {
      m_cursor = 0;
      m_length = 0;

      m_path[0] = 0;
   }

   void init (size_t length) {
      m_path = cr::makeUnique <int32_t[]> (length);
   }
};

// main bot class
class Bot final {
public:
   friend class BotManager;

private:
   mutable Mutex m_pathFindLock {};
   mutable Mutex m_predictLock {};

   float f_wpt_tim_str_chg;

private:
   uint32_t m_states {}; // sensing bitstates
   uint32_t m_collideMoves[kMaxCollideMoves] {}; // sorted array of movements
   uint32_t m_collisionProbeBits {}; // bits of possible collision moves
   uint32_t m_collStateIndex {}; // index into collide moves
   uint32_t m_aimFlags {}; // aiming conditions
   uint32_t m_currentTravelFlags {}; // connection flags like jumping

   int m_oldButtons {}; // our old buttons
   int m_reloadState {}; // current reload state
   int m_voicePitch {}; // bot voice pitch
   int m_loosedBombNodeIndex {}; // nearest to loosed bomb node
   int m_plantedBombNodeIndex {}; // nearest to planted bomb node
   int m_currentNodeIndex {}; // current node index
   int m_travelStartIndex {}; // travel start index to double jump action
   int m_previousNodes[5] {}; // previous node indexes from node find
   int m_pathFlags {}; // current node flags
   int m_needAvoidGrenade {}; // which direction to strafe away
   int m_campDirection {}; // camp Facing direction
   int m_campButtons {}; // buttons to press while camping
   int m_tryOpenDoor {}; // attempt's to open the door
   int m_liftState {}; // state of lift handling
   int m_radioSelect {}; // radio entry
   int m_radioPercent {}; // radio usage percent (in response)
   int m_killsCount {}; // the kills count of a bot

   int m_lastPredictIndex {}; // last predicted path index
   int m_lastPredictLength {}; // last predicted path length
   int m_pickupType {}; // type of entity which needs to be used/picked up

   float m_headedTime {}; // last time followed by radio entity
   float m_prevTime {}; // time previously checked movement speed
   float m_heavyTimestamp {}; // is it time to execute heavy-weight functions
   float m_prevSpeed {}; // speed some frames before
   float m_timeDoorOpen {}; // time to next door open check
   float m_timeHitDoor {}; // specific time after hitting the door
   float m_lastChatTime {}; // time bot last chatted
   float m_timeLogoSpray {}; // time bot last spray logo
   float m_knifeAttackTime {}; // time to rush with knife (at the beginning of the round)
   float m_duckDefuseCheckTime {}; // time to check for ducking for defuse
   float m_frameInterval {}; // bot's frame interval
   float m_previousThinkTime {}; // time bot last thinked
   float m_reloadCheckTime {}; // time to check reloading
   float m_zoomCheckTime {}; // time to check zoom again
   float m_shieldCheckTime {}; // time to check shield drawing again
   float m_grenadeCheckTime {}; // time to check grenade usage
   float m_sniperStopTime {}; // bot switched to other weapon?
   float m_lastEquipTime {}; // last time we equipped in buyzone
   float m_duckTime {}; // time to duck
   float m_jumpTime {}; // time last jump happened
   float m_soundUpdateTime {}; // time to update the sound
   float m_heardSoundTime {}; // last time noise is heard
   float m_buttonPushTime {}; // time to push the button
   float m_liftUsageTime {}; // time to use lift
   float m_askCheckTime {}; // time to ask team
   float m_collideTime {}; // time last collision
   float m_firstCollideTime {}; // time of first collision
   float m_probeTime {}; // time of probing different moves
   float m_lastCollTime {}; // time until next collision check
   float m_lookYawVel {}; // look yaw velocity
   float m_lookPitchVel {}; // look pitch velocity
   float m_lookUpdateTime {}; // lookangles update time
   float m_aimErrorTime {}; // time to update error vector
   float m_nextCampDirTime {}; // time next camp direction change
   float m_lastFightStyleCheck {}; // time checked style
   float m_strafeSetTime {}; // time strafe direction was set
   float m_randomizeAnglesTime {}; // time last randomized location
   float m_playerTargetTime {}; // time last targeting
   float m_timeCamping {}; // time to camp
   float m_lastUsedNodesTime {}; // last time bot followed nodes
   float m_shootAtDeadTime {}; // time to shoot at dying players
   float m_followWaitTime {}; // wait to follow time
   float m_chatterTimes[Chatter::Count] {}; // chatter command timers
   float m_navTimeset {}; // time node chosen by Bot
   float m_moveSpeed {}; // current speed forward/backward
   float m_strafeSpeed {}; // current speed sideways
   float m_minSpeed {}; // minimum speed in normal mode
   float m_oldCombatDesire {}; // holds old desire for filtering
   float m_itemCheckTime {}; // time next search for items needs to be done
   float m_joinServerTime {}; // time when bot joined the game
   float m_playServerTime {}; // time bot spent in the game
   float m_changeViewTime {}; // timestamp to change look at while at freezetime
   float m_breakableTime {}; // breakable acquired time
   float m_timeDebugUpdateTime {}; // time to update last debug timestamp
   float m_lastVictimTime {}; // time when bot killed an enemy
   float m_killsInterval {}; // interval between kills
   float m_lastDamageTimestamp {}; // last damage from take damage fn
   float m_movedDistance {}; // bot moved distance

   bool m_moveToGoal {}; // bot currently moving to goal??
   bool m_isStuck {}; // bot is stuck
   bool m_isStale {}; // bot is leaving server
   bool m_isReloading {}; // bot is reloading a gun
   bool m_forceRadio {}; // should bot use radio anyway?
   bool m_defendedBomb {}; // defend action issued
   bool m_defendHostage {}; // defend action issued
   bool m_duckDefuse {}; // should or not bot duck to defuse bomb
   bool m_checkKnifeSwitch {}; // is time to check switch to knife action
   bool m_checkWeaponSwitch {}; // is time to check weapon switch
   bool m_isUsingGrenade {}; // bot currently using grenade??
   bool m_bombSearchOverridden {}; // use normal node if applicable when near the bomb
   bool m_wantsToFire {}; // bot needs consider firing
   bool m_jumpFinished {}; // has bot finished jumping
   bool m_isLeader {}; // bot is leader of his team
   bool m_checkTerrain {}; // check for terrain
   bool m_moveToC4 {}; // ct is moving to bomb
   bool m_needToSendWelcomeChat {}; // bot needs to greet people on server?
   bool m_isCreature {}; // bot is not a player, but something else ? zombie ?
   bool m_isOnInfectedTeam {}; // bot is zombie (this assumes bot is a creature)
   bool m_infectedEnemyTeam {}; // the enemy is a zombie (assumed to be a hostile creature)
   bool m_defuseNotified {}; // bot is notified about bomb defusion
   bool m_jumpSequence {}; // next path link will be jump link
   bool m_checkFall {}; // check bot fall
   bool m_botMovement {}; // bot movement allowed ?

   PathWalk m_pathWalk {}; // pointer to current node from path
   Dodge m_dodgeStrafeDir {}; // direction to strafe
   Fight m_fightStyle {}; // combat style to use
   CollisionState m_collisionState {}; // collision State
   FindPath m_pathType {}; // which pathfinder to use
   int8_t m_enemyParts {}; // visibility flags
   uint16_t m_modelMask {}; // model mask bits
   UniquePtr <class AStarAlgo> m_planner {};

   edict_t *m_pickupItem {}; // pointer to entity of item to use/pickup
   edict_t *m_liftEntity {}; // pointer to lift entity
   edict_t *m_breakableEntity {}; // pointer to breakable entity
   edict_t *m_lastBreakable {}; // last acquired breakable
   edict_t *m_targetEntity {}; // the entity that the bot is trying to reach
   edict_t *m_avoidGrenade {}; // pointer to grenade entity to avoid
   edict_t *m_hindrance {}; // the hindrance
   edict_t *m_hearedEnemy {}; // the heared enemy

   Vector m_liftTravelPos {}; // lift travel position
   Vector m_moveAngles {}; // bot move angles
   Vector m_idealAngles {}; // angle wanted
   Vector m_randomizedIdealAngles {}; // angle wanted with noise
   Vector m_angularDeviation {}; // angular deviation from current to ideal angles
   Vector m_aimSpeed {}; // aim speed calculated
   Vector m_aimLastError {}; // last calculated aim error
   Vector m_prevOrigin {}; // origin some frames before
   Vector m_lookAt {}; // vector bot should look at
   Vector m_throw {}; // origin of node to throw grenades
   Vector m_enemyOrigin {}; // target origin chosen for shooting
   Vector m_grenade {}; // calculated vector for grenades
   Vector m_entity {}; // origin of entities like buttons etc
   Vector m_lookAtSafe {}; // aiming vector when camping
   Vector m_lookAtPredict {}; // aiming vector when predicting
   Vector m_desiredVelocity {}; // desired velocity for jump nodes
   Vector m_breakableOrigin {}; // origin of breakable
   Vector m_checkFallPoint[2] {}; // check fall point

   Array <edict_t *> m_ignoredBreakable {}; // list of ignored breakables
   Array <edict_t *> m_ignoredItems {}; // list of  pointers to entity to ignore for pickup
   Array <edict_t *> m_hostages {}; // pointer to used hostage entities

   UniquePtr <class PlayerHitboxEnumerator> m_hitboxEnumerator {};

   BotDifficultyData *m_difficultyData {};
   Path *m_path {}; // pointer to the current path node
   String m_chatBuffer {}; // space for strings (say text...)
   Frustum::Planes m_viewFrustum {};

   CountdownTimer m_forgetLastVictimTimer {}; // time to forget last victim position ?
   CountdownTimer m_approachingLadderTimer {}; // bot is approaching ladder
   CountdownTimer m_lostReachableNodeTimer {}; // bot's issuing next node, probably he's lost
   CountdownTimer m_fixFallTimer {}; // timer we're fixed fall last time
   CountdownTimer m_repathTimer {}; // bots is going to repath his route

private:
   int pickBestWeapon (Array <int> &vec, int moneySave) const;
   int getRandomCampDir ();
   int findAimingNode (const Vector &to, int &pathLength);
   int findNearestNode ();
   int findBombNode ();
   int findCoverNode (float maxDistance);
   int findDefendNode (const Vector &origin);
   int findBestGoal ();
   int findBestGoalWhenBombAction ();
   int findGoalPost (int tactic, IntArray *defensive, IntArray *offensive);
   int bestPrimaryCarried ();
   int bestSecondaryCarried ();
   int bestGrenadeCarried () const;
   int getBestOwnedWeapon () const;
   int getBestOwnedPistol () const;
   int changeNodeIndex (int index);
   int numEnemiesNear (const Vector &origin, const float radius) const;
   int numFriendsNear (const Vector &origin, const float radius) const;


   float getEstimatedNodeReachTime ();
   float isInFOV (const Vector &dest) const;
   float getShiftSpeed ();
   float calculateScaleFactor (edict_t *ent) const;

   bool canReplaceWeapon ();
   bool canDuckUnder (const Vector &normal);
   bool canJumpUp (const Vector &normal);
   bool doneCanJumpUp (const Vector &normal, const Vector &right);
   bool isBlockedForward (const Vector &normal, TraceResult *tr);
   bool canStrafeLeft (TraceResult *tr);
   bool canStrafeRight (TraceResult *tr);
   bool isBlockedLeft ();
   bool isBlockedRight ();
   bool checkWallOnLeft (float distance = 40.0f);
   bool checkWallOnRight (float distance = 40.0f);
   bool checkWallOnBehind (float distance = 40.0f);
   bool updateNavigation ();
   bool isEnemyThreat ();
   bool isWeaponRestricted (int wid);
   bool isWeaponRestrictedAMX (int wid);
   bool isInViewCone (const Vector &origin);
   bool checkBodyParts (edict_t *target);
   bool checkBodyPartsWithOffsets (edict_t *target);
   bool checkBodyPartsWithHitboxes (edict_t *target);
   bool seesEnemy (edict_t *player);
   bool hasActiveGoal ();
   bool advanceMovement ();
   bool isBombDefusing (const Vector &bombOrigin) const;
   bool isOccupiedNode (int index, bool needZeroVelocity = false);
   bool seesItem (const Vector &dest, StringRef classname);
   bool lastEnemyShootable ();
   bool rateGroundWeapon (edict_t *ent);
   bool reactOnEnemy ();
   bool selectBestNextNode ();
   bool hasAnyWeapons () const;
   bool hasAnyAmmoInClip ();
   bool isKnifeMode ();
   bool isGrenadeWar ();
   bool isDeadlyMove (const Vector &to);
   bool isNotSafeToMove (const Vector &to);
   bool isOutOfBombTimer ();
   bool isWeaponBadAtDistance (int weaponIndex, float distance);
   bool needToPauseFiring (float distance);
   bool checkZoom (float distance);
   bool lookupEnemies ();
   bool isEnemyHidden (edict_t *enemy);
   bool isEnemyInvincible (edict_t *enemy);
   bool isEnemyNoTarget (edict_t *enemy);
   bool isEnemyInDarkArea (edict_t *enemy) const;
   bool isFriendInLineOfFire (float distance) const;
   bool isGroupOfEnemies (const Vector &location);
   bool isPenetrableObstacle (const Vector &dest);
   bool isPenetrableObstacle1 (const Vector &dest, int penetratePower) const;
   bool isPenetrableObstacle2 (const Vector &dest, int penetratePower) const;
   bool isPenetrableObstacle3 (const Vector &dest, int penetratePower) const;
   bool isEnemyBehindShield (edict_t *enemy);
   bool checkChatKeywords (String &reply);
   bool isReplyingToChat ();
   bool isReachableNode (int index);
   bool updateLiftHandling ();
   bool updateLiftStates ();
   bool canRunHeavyWeight ();
   bool isEnemyInSight (Vector &endPos);
   bool isEnemyNoticeable (float range);
   bool isCreature () const;
   bool isPreviousLadder () const;
   bool isIgnoredItem (edict_t *ent);

   void doPlayerAvoidance (const Vector &normal);
   void selectCampButtons (int index);
   void instantChatter (int type) const;
   void update ();
   void runMovement ();
   void checkSpawnConditions ();
   void buyStuff ();
   void changePitch (float speed);
   void changeYaw (float speed);
   void checkMsgQueue ();
   void checkRadioQueue ();
   void checkReload ();
   void avoidGrenades ();
   void checkGrenadesThrow ();
   void checkBurstMode (float distance);
   void checkSilencer ();
   void setAimDirection ();
   void updateLookAngles ();
   void updateBodyAngles ();
   void updateLookAnglesNewbie (const Vector &direction, float delta);
   void setIdealReactionTimers (bool actual = false);
   void updateHearing ();
   void postProcessGoals (const IntArray &goals, int result[]);
   void updatePickups ();
   void ensurePickupEntitiesClear ();
   void checkTerrain (const Vector &dirNormal);
   void checkFall ();
   void checkDarkness ();
   void checkParachute ();
   void updatePracticeValue (int damage) const;
   void updatePracticeDamage (edict_t *attacker, int damage);
   void findShortestPath (int srcIndex, int destIndex);
   void findPath (int srcIndex, int destIndex, FindPath pathType = FindPath::Fast);
   void syncFindPath (int srcIndex, int destIndex, FindPath pathType);
   void debugMsgInternal (StringRef str);
   void frame ();
   void resetCollision ();
   void ignoreCollision ();
   void setConditions ();
   void overrideConditions ();
   void updateEmotions ();
   void setStrafeSpeed (const Vector &moveDir, float strafeSpeed);
   void updateTeamJoin ();
   void updateTeamCommands ();
   void decideFollowUser ();
   void attackMovement ();
   void findValidNode ();
   void setPathOrigin ();
   void fireWeapons ();
   void doFireWeapons ();
   void handleWeapons (float distance, int index, int id, int choosen);
   void focusEnemy ();
   void selectBestWeapon ();
   void selectSecondary ();
   void selectWeaponById (int id);
   void selectWeaponByIndex (int index);
   void syncUpdatePredictedIndex ();
   void updatePredictedIndex ();
   void refreshCreatureStatus (char *infobuffer);
   void donateC4ToHuman ();
   void clearAmmoInfo ();
   void handleChatterTaskChange (Task tid);
   void executeChatterFrameEvents ();

   void completeTask ();
   void executeTasks ();
   void trackEnemies ();
   void logicDuringFreezetime ();
   void translateInput ();
   void moveToGoal ();
   void resetMovement ();
   void refreshEnemyPredict ();
   void setLastVictim (edict_t *victim);

   void normal_ ();
   void spraypaint_ ();
   void huntEnemy_ ();
   void seekCover_ ();
   void attackEnemy_ ();
   void pause_ ();
   void blind_ ();
   void camp_ ();
   void hide_ ();
   void moveToPos_ ();
   void plantBomb_ ();
   void defuseBomb_ ();
   void followUser_ ();
   void throwExplosive_ ();
   void throwFlashbang_ ();
   void throwSmoke_ ();
   void doublejump_ ();
   void escapeFromBomb_ ();
   void pickupItem_ ();
   void shootBreakable_ ();

   edict_t *lookupButton (StringRef target, bool blindTest = false);
   edict_t *lookupBreakable ();
   edict_t *setCorrectGrenadeVelocity (StringRef model);

   Vector getEnemyBodyOffset ();
   Vector calcThrow (const Vector &start, const Vector &stop);
   Vector calcToss (const Vector &start, const Vector &stop);
   Vector isBombAudible ();
   Vector getBodyOffsetError (float distance);
   Vector getCampDirection (const Vector &dest);
   Vector getCustomHeight (float distance) const;

   uint8_t computeMsec () const;

private:
   bool isOnLadder () const {
      return pev->movetype == MOVETYPE_FLY;
   }

   bool isOnFloor () const {
      return !!(pev->flags & (FL_ONGROUND | FL_PARTIALGROUND));
   }

   bool isInWater () const {
      return pev->waterlevel >= 2;
   }

   bool isInNarrowPlace () const {
      return (m_pathFlags & NodeFlag::Narrow);
   }

   void dropCurrentWeapon () {
      issueCommand ("drop");
   }

   // ensures current node is ok
   void ensureCurrentNodeIndex () {
      if (m_currentNodeIndex == kInvalidNodeIndex) {
         changeNodeIndex (findNearestNode ());
      }
   }

   // get run player move angles
   const Vector &getRpmAngles ();

public:
   entvars_t *pev {};

   int m_index {}; // saved bot index
   int m_wantedTeam {}; // player team bot wants select
   int m_wantedSkin {}; // player model bot wants to select
   int m_difficulty {}; // bots hard level
   int m_moneyAmount {}; // amount of money in bot's bank

   int m_pingBase {}; // base ping level for randomizing
   int m_ping {}; // bot's acutal ping

   float m_spawnTime {}; // time this bot spawned
   float m_timeTeamOrder {}; // time of last radio command
   float m_slowFrameTimestamp {}; // time to per-second think
   float m_nextBuyTime {}; // next buy time
   float m_checkDarkTime {}; // check for darkness time
   float m_preventFlashing {}; // bot turned away from flashbang
   float m_blindTime {}; // time when bot is blinded
   float m_blindMoveSpeed {}; // mad speeds when bot is blind
   float m_blindSideMoveSpeed {}; // mad side move speeds when bot is blind
   float m_fallDownTime {}; // time bot started to fall 
   float m_duckForJump {}; // is bot needed to duck for double jump
   float m_baseAgressionLevel {}; // base aggression level (on initializing)
   float m_baseFearLevel {}; // base fear level (on initializing)
   float m_agressionLevel {}; // dynamic aggression level (in game)
   float m_fearLevel {}; // dynamic fear level (in game)
   float m_nextEmotionUpdate {}; // next time to sanitize emotions
   float m_goalValue {}; // ranking value for this node
   float m_viewDistance {}; // current view distance
   float m_maxViewDistance {}; // maximum view distance
   float m_retreatTime {}; // time to retreat?
   float m_enemyUpdateTime {}; // time to check for new enemies
   float m_enemyReachableTimer {}; // time to recheck if enemy reachable
   float m_enemyIgnoreTimer {}; // ignore enemy for some time
   float m_seeEnemyTime {}; // time bot sees enemy
   float m_enemySurpriseTime {}; // time of surprise
   float m_idealReactionTime {}; // time of base reaction
   float m_actualReactionTime {}; // time of current reaction time
   float m_timeNextTracking {}; // time node index for tracking player is recalculated
   float m_firePause {}; // time to pause firing
   float m_shootTime {}; // time to shoot
   float m_timeLastFired {}; // time to last firing
   float m_difficultyChange {}; // time when auto-difficulty was last applied to this bot
   float m_kpdRatio {}; // kill per death ratio
   float m_healthValue {}; // clamped bot health
   float m_stayTime {}; // stay time before reconnect

   int m_blindNodeIndex {}; // node index to cover when blind
   int m_flashLevel {}; // flashlight level
   int m_numEnemiesLeft {}; // number of enemies alive left on map
   int m_numFriendsLeft {}; // number of friend alive left on map
   int m_retryJoin {}; // retry count for choosing team/class
   int m_startAction {}; // team/class selection state
   int m_voteKickIndex {}; // index of player to vote against
   int m_lastVoteKick {}; // last index
   int m_voteMap {}; // number of map to vote for
   int m_logoDecalIndex {}; // index for logotype
   int m_buyState {}; // current count in buying
   int m_blindButton {}; // buttons bot press, when blind
   int m_radioOrder {}; // actual command
   int m_prevGoalIndex {}; // holds destination goal node
   int m_chosenGoalIndex {}; // used for experience, same as above
   int m_lastDamageType {}; // stores last damage
   int m_team {}; // bot team
   int m_currentWeapon {}; // one current weapon for each bot
   int m_weaponType {}; // current weapon type
   int m_ammoInClip[kMaxWeapons] {}; // ammo in clip for each weapons
   int m_ammo[MAX_AMMO_SLOTS] {}; // total ammo amounts
   int m_deathCount {}; // number of bot deaths
   int m_ladderDir {}; // ladder move direction

   bool m_isVIP {}; // bot is vip?
   bool m_isAlive {}; // has the player been killed or has he just respawned
   bool m_notStarted {}; // team/class not chosen yet
   bool m_ignoreBuyDelay {}; // when reaching buyzone in the middle of the round don't do pauses
   bool m_inBombZone {}; // bot in the bomb zone or not
   bool m_inBuyZone {}; // bot currently in buy zone
   bool m_inEscapeZone {}; // bot currently in escape zone
   bool m_inRescueZone {}; // bot currently in rescue zone
   bool m_inVIPZone {}; // bot in the vip safety zone
   bool m_buyingFinished {}; // done with buying
   bool m_buyPending {}; // bot buy is pending
   bool m_hasDefuser {}; // does bot has defuser
   bool m_hasNVG {}; // does bot has nightvision goggles
   bool m_usesNVG {}; // does nightvision goggles turned on
   bool m_hasC4 {}; // does bot has c4 bomb
   bool m_hasHostage {}; // does bot owns some hostages
   bool m_hasProgressBar {}; // has progress bar on a HUD
   bool m_jumpReady {}; // is double jump ready
   bool m_canSetAimDirection {}; // can choose aiming direction
   bool m_isEnemyReachable {}; // direct line to enemy
   bool m_kickedByRotation {}; // is bot kicked due to rotation ?
   bool m_kickMeFromServer {}; // kick the bot off the server?
   bool m_fireHurtsFriend {}; // firing at enemy will hurt our friend?

   edict_t *m_doubleJumpEntity {}; // pointer to entity that request double jump
   edict_t *m_radioEntity {}; // pointer to entity issuing a radio command
   edict_t *m_enemy {}; // pointer to enemy entity
   edict_t *m_enemyBodyPartSet {}; // pointer to last enemy body part was set to head
   edict_t *m_lastEnemy {}; // pointer to last enemy entity
   edict_t *m_lastVictim {}; // pointer to killed entity
   edict_t *m_trackingEdict {}; // pointer to last tracked player when camping/hiding

   Vector m_pathOrigin {}; // origin of node
   Vector m_destOrigin {}; // origin of move destination
   Vector m_position {}; // position to move to in move to position task
   Vector m_doubleJumpOrigin {}; // origin of double jump
   Vector m_lastEnemyOrigin {}; // vector to last enemy origin
   Vector m_lastVictimOrigin {}; // last victim origin to watch it

   ChatCollection m_sayTextBuffer {}; // holds the index & the actual message of the last unprocessed text message of a player
   BurstMode m_weaponBurstMode {}; // bot using burst mode? (famas/glock18, but also silencer mode)
   Personality m_personality {}; // bots type
   Array <BotTask> m_tasks {};

   Deque <int32_t> m_msgQueue {};
   Array <int32_t> m_goalHist {};

   FrameDelay m_thinkTimer {};

public:
   Bot (edict_t *bot, int difficulty, int personality, int team, int skin);
   ~Bot () = default;

public:
   void logic (); /// the things that can be executed while skipping frames
   void spawned ();
   void takeBlind (int alpha);
   void takeDamage (edict_t *inflictor, int damage, int armor, int bits);
   void showDebugOverlay ();
   void newRound ();
   void resetPathSearchType ();
   void enteredBuyZone (int buyState);
   void pushMsgQueue (int message);
   void prepareChatMessage (StringRef message);
   void checkForChat ();
   void showChatterIcon (bool show, bool disconnect = false) const;
   void clearSearchNodes ();
   void checkBreakable (edict_t *touch);
   void checkBreakablesAround ();
   void startTask (Task id, float desire, int data, float time, bool resume);
   void clearTask (Task id);
   void filterTasks ();
   void clearTasks ();
   void dropWeaponForUser (edict_t *user, bool discardC4);
   void sendToChat (StringRef message, bool teamOnly);
   void sendToChatLegacy (StringRef message, bool teamOnly);
   void pushChatMessage (int type, bool isTeamSay = false);
   void pushRadioMessage (int message);
   void pushChatterMessage (int message);
   void tryHeadTowardRadioMessage ();
   void kill ();
   void kick (bool silent = false);
   void resetDoubleJump ();
   void startDoubleJump (edict_t *ent);
   void sendBotToOrigin (const Vector &origin);
   void markStale ();
   void setNewDifficulty (int32_t newDifficulty);
   bool hasHostage ();
   bool hasPrimaryWeapon () const;
   bool hasSecondaryWeapon () const;
   bool hasShield ();
   bool isShieldDrawn ();
   bool findNextBestNode ();
   bool findNextBestNodeEx (const IntArray &data, bool handleFails);
   bool seesEntity (const Vector &dest, bool fromBody = false);

   int getAmmo () const;
   int getAmmo (int id) const;
   int getNearestToPlantedBomb ();

   float getConnectionTime ();
   BotTask *getTask ();

public:
   int getAmmoInClip () const {
      return m_ammoInClip[m_currentWeapon];
   }

   bool isDucking () const {
      return !!(pev->flags & FL_DUCKING);
   }

   Vector getCenter () const {
      return (pev->absmax + pev->absmin) * 0.5;
   };

   Vector getEyesPos () const {
      return pev->origin + pev->view_ofs;
   };

   Task getCurrentTaskId () {
      return getTask ()->id;
   }

   edict_t *ent () const {
      return pev->pContainingEntity;
   };

   // bots array index
   int index () const {
      return m_index;
   }

   // entity index with worldspawn shift
   int entindex () const {
      return m_index + 1;
   }

   // get the current node index 
   int getCurrentNodeIndex () const {
      return m_currentNodeIndex;
   }

   // is low on ammo on index?
   bool isLowOnAmmo (const int index, const float factor) const;

   // prints debug message
   template <typename ...Args> void debugMsg (const char *fmt, Args &&...args) {
      debugMsgInternal (strings.format (fmt, cr::forward <Args> (args)...));
   }

   // execute client command helper
   template <typename ...Args> void issueCommand (const char *fmt, Args &&...args);

   // checks if valid prediction index
   bool isNodeValidForPredict (const int index) const {
      return BotGraph::instance ().exists (index) && index != m_currentNodeIndex;
   }

private:
   // returns true if bot is using a sniper rifle
   bool usesSniper () const {
      return m_weaponType == WeaponType::Sniper;
   }

   // returns true if bot is using a rifle
   bool usesRifle () const {
      return usesZoomableRifle () || m_weaponType == WeaponType::Rifle;
   }

   // returns true if bot is using a zoomable rifle
   bool usesZoomableRifle () const {
      return m_weaponType == WeaponType::ZoomRifle;
   }

   // returns true if bot is using a pistol
   bool usesPistol () const {
      return m_weaponType == WeaponType::Pistol;
   }

   // returns true if bot is using a SMG
   bool usesSubmachine () const {
      return m_weaponType == WeaponType::SMG;
   }

   // returns true if bot is using a shotgun
   bool usesShotgun () const {
      return m_weaponType == WeaponType::Shotgun;
   }

   // returns true if bot is using m249
   bool usesHeavy () const {
      return m_weaponType == WeaponType::Heavy;
   }

   // returns true if bot using not very good weapon
   bool usesBadWeapon () const {
      return usesShotgun () || m_currentWeapon == Weapon::UMP45 || m_currentWeapon == Weapon::MAC10 || m_currentWeapon == Weapon::TMP;
   }

   // returns true if bot using a camp gun
   bool usesCampGun () const {
      return usesSubmachine () || usesRifle () || usesSniper () || usesHeavy ();
   }

   // returns true if bot using knife
   bool usesKnife () const {
      return m_weaponType == WeaponType::Melee;
   }

   // checks if weapon recoil is high
   bool isRecoilHigh () const {
      return pev->punchangle.x < -1.45f;
   }
};

#include "config.h"
#include "support.h"
#include "hooks.h"
#include "sounds.h"
#include "message.h"
#include "engine.h"
#include "manager.h"
#include "control.h"
#include "planner.h"
#include "storage.h"
#include "analyze.h"
#include "fakeping.h"

// very global convars
extern ConVar cv_jasonmode;
extern ConVar cv_radio_mode;
extern ConVar cv_ignore_enemies;
extern ConVar cv_ignore_objectives;
extern ConVar cv_chat;
extern ConVar cv_language;
extern ConVar cv_show_latency;
extern ConVar cv_show_avatars;
extern ConVar cv_enable_query_hook;
extern ConVar cv_chatter_path;
extern ConVar cv_quota;
extern ConVar cv_difficulty;
extern ConVar cv_attack_monsters;
extern ConVar cv_pickup_custom_items;
extern ConVar cv_economics_rounds;
extern ConVar cv_shoots_thru_walls;
extern ConVar cv_debug;
extern ConVar cv_debug_goal;
extern ConVar cv_save_bots_names;
extern ConVar cv_rotate_bots;
extern ConVar cv_graph_url;
extern ConVar cv_graph_url_upload;
extern ConVar cv_graph_auto_save_count;
extern ConVar cv_graph_analyze_max_jump_height;
extern ConVar cv_spraypaints;
extern ConVar cv_whose_your_daddy;
extern ConVar cv_grenadier_mode;
extern ConVar cv_ignore_enemies_after_spawn_time;
extern ConVar cv_camping_time_min;
extern ConVar cv_camping_time_max;
extern ConVar cv_smoke_grenade_checks;
extern ConVar cv_smoke_greande_checks_radius;
extern ConVar cv_check_darkness;
extern ConVar cv_use_hitbox_enemy_targeting;
extern ConVar cv_restricted_weapons;

extern ConVar mp_freezetime;
extern ConVar mp_roundtime;
extern ConVar mp_timelimit;
extern ConVar mp_limitteams;
extern ConVar mp_autoteambalance;
extern ConVar mp_footsteps;
extern ConVar mp_startmoney;
extern ConVar mp_c4timer;

// execute client command helper
template <typename ...Args> void Bot::issueCommand (const char *fmt, Args &&...args) {
   game.botCommand (ent (), strings.format (fmt, cr::forward <Args> (args)...));
}
