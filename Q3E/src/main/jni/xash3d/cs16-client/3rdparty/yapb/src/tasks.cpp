//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_walking_allowed ("walking_allowed", "1", "Specifies whether bots able to use 'shift' if they thinks that enemy is near.");
ConVar cv_camping_allowed ("camping_allowed", "1", "Allows or disallows bots to camp. Doesn't affects bomb/hostage defending tasks.");

ConVar cv_camping_time_min ("camping_time_min", "15.0", "Lower bound of time from which time for camping is calculated", true, 5.0f, 90.0f);
ConVar cv_camping_time_max ("camping_time_max", "45.0", "Upper bound of time until which time for camping is calculated", true, 15.0f, 120.0f);

ConVar cv_random_knife_attacks ("random_knife_attacks", "1", "Allows or disallows the ability for random knife attacks when bot is rushing and no enemy is nearby.");

void Bot::normal_ () {
   m_aimFlags |= AimFlags::Nav;

   const int debugGoal = cv_debug_goal.as <int> ();

   // user forced a node as a goal?
   if (debugGoal != kInvalidNodeIndex) {
      if (getTask ()->data != debugGoal) {
         clearSearchNodes ();

         getTask ()->data = debugGoal;
         m_chosenGoalIndex = debugGoal;
      }

      // stop the bot if precisely reached debug goal
      if (m_currentNodeIndex == debugGoal) {
         const auto &debugOrigin = graph[debugGoal].origin;

         if (debugOrigin.distanceSq2d (pev->origin) < cr::sqrf (22.0f) && util.isVisible (debugOrigin, ent ())) {
            m_moveToGoal = false;
            m_checkTerrain = false;

            m_moveSpeed = 0.0f;
            m_strafeSpeed = 0.0f;

            return;
         }
      }
   }

   // bots rushing with knife, when have no enemy (thanks for idea to nicebot project)
   if (cv_random_knife_attacks
      && usesKnife ()
      && (game.isNullEntity (m_lastEnemy) || !util.isAlive (m_lastEnemy))
      && game.isNullEntity (m_enemy)
      && m_knifeAttackTime < game.time ()
      && !m_hasHostage
      && !hasShield ()
      && numFriendsNear (pev->origin, 96.0f) == 0) {

      if (rg.chance (40)) {
         pev->button |= IN_ATTACK;
      }
      else {
         pev->button |= IN_ATTACK2;
      }
      m_knifeAttackTime = game.time () + rg (2.5f, 6.0f);
   }
   const auto &prop = conf.getWeaponProp (m_currentWeapon);

   if (m_reloadState == Reload::None && getAmmo () != 0 && getAmmoInClip () < 5 && prop.ammo1 != -1) {
      m_reloadState = Reload::Primary;
   }

   // if bomb planted and it's a CT calculate new path to bomb point if he's not already heading for
   if (!m_bombSearchOverridden
      && bots.isBombPlanted ()
      && m_team == Team::CT
      && getTask ()->data != kInvalidNodeIndex
      && !(graph[getTask ()->data].flags & NodeFlag::Goal)
      && getCurrentTaskId () != Task::EscapeFromBomb) {

      clearSearchNodes ();
      getTask ()->data = kInvalidNodeIndex;
   }

   // reached the destination (goal) node?
   if (updateNavigation ()) {
      // if we're reached the goal, and there is not enemies, notify the team
      if (!bots.isBombPlanted ()
         && m_currentNodeIndex != kInvalidNodeIndex
         && (m_pathFlags & NodeFlag::Goal)
         && rg.chance (15)
         && numEnemiesNear (pev->origin, 650.0f) == 0) {

         pushRadioMessage (Radio::SectorClear);
      }

      completeTask ();
      m_prevGoalIndex = kInvalidNodeIndex;

      // spray logo sometimes if allowed to do so
      if (!(m_states & (Sense::SeeingEnemy | Sense::SuspectEnemy))
         && m_seeEnemyTime + 5.0f < game.time ()
         && m_reloadState ==  Reload::None
         && m_timeLogoSpray < game.time ()
         && cv_spraypaints
         && pev->groundentity == game.getStartEntity ()
         && m_moveSpeed >= getShiftSpeed ()
         && game.isNullEntity (m_pickupItem)) {

         if (!(game.mapIs (MapFlags::Demolition) && bots.isBombPlanted () && m_team == Team::CT)) {
            startTask (Task::Spraypaint, TaskPri::Spraypaint, kInvalidNodeIndex, game.time () + 1.0f, false);
         }
      }

      // reached node is a camp node
      if ((m_pathFlags & NodeFlag::Camp) && !game.is (GameFlags::CSDM) && cv_camping_allowed && !isKnifeMode ()) {
         const bool allowedCampWeapon = hasPrimaryWeapon ()
            || hasShield ()
            || (hasSecondaryWeapon () && !hasPrimaryWeapon () && m_numFriendsLeft > game.maxClients () / 6);

         // check if bot has got a primary weapon and hasn't camped before
         if (allowedCampWeapon && m_timeCamping + 10.0f < game.time () && !m_hasHostage) {
            bool campingAllowed = true;

            // Check if it's not allowed for this team to camp here
            if (m_team == Team::Terrorist) {
               if (m_pathFlags & NodeFlag::CTOnly) {
                  campingAllowed = false;
               }
            }
            else {
               if (m_pathFlags & NodeFlag::TerroristOnly) {
                  campingAllowed = false;
               }
            }

            // don't allow vip on as_ maps to camp + don't allow terrorist carrying c4 to camp
            if (campingAllowed
               && (m_isVIP || (game.mapIs (MapFlags::Demolition) && m_team == Team::Terrorist && !bots.isBombPlanted () && m_hasC4))) {
               campingAllowed = false;
            }

            // check if another bot is already camping here
            if (campingAllowed && isOccupiedNode (m_currentNodeIndex)) {
               campingAllowed = false;
            }

            // skip sniper node if we don't have sniper weapon
            if (campingAllowed && !usesSniper () && (m_pathFlags & NodeFlag::Sniper)) {
               campingAllowed = false;
            }

            // if the bot is about to come to the camp spot, but there is already someone else camping
            if (!campingAllowed && getTask ()->data == m_currentNodeIndex && getTask ()->data != kInvalidNodeIndex) {
               clearSearchNodes ();
               getTask ()->data = kInvalidNodeIndex;
            }

            if (campingAllowed) {
               // crouched camping here?
               if (m_pathFlags & NodeFlag::Crouch) {
                  m_campButtons = IN_DUCK;
               }
               else {
                  m_campButtons = 0;
               }
               selectBestWeapon ();

               if (!(m_states & (Sense::SeeingEnemy | Sense::HearingEnemy)) && !m_reloadState) {
                  m_reloadState = Reload::Primary;
               }
               m_timeCamping = game.time () + rg (cv_camping_time_min.as <float> (), cv_camping_time_max.as <float> ());
               startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, m_timeCamping, true);

               m_lookAtSafe = m_pathOrigin + m_path->start.forward () * 500.0f;
               m_aimFlags |= AimFlags::Camp;
               m_campDirection = 0;

               // tell the world we're camping
               if (rg.chance (25)) {
                  pushRadioMessage (Radio::ImInPosition);
               }
               m_moveToGoal = false;
               m_checkTerrain = false;

               m_moveSpeed = 0.0f;
               m_strafeSpeed = 0.0f;
            }
         }
      }
      else {
         // some goal nodes are map dependent so check it out...
         if (game.mapIs (MapFlags::HostageRescue)) {
            // CT Bot has some hostages following?
            if (m_team == Team::CT && m_hasHostage) {
               // and reached a rescue point?
               if (m_pathFlags & NodeFlag::Rescue) {
                  m_hostages.clear ();
               }
            }
            else if (m_team == Team::Terrorist && rg.chance (75) && !game.mapIs (MapFlags::Demolition)) {
               const int index = findDefendNode (m_path->origin);

               startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + rg (60.0f, 120.0f), true); // push camp task on to stack
               startTask (Task::MoveToPosition, TaskPri::MoveToPosition, index, game.time () + rg (5.0f, 10.0f), true); // push move command

               // decide to duck or not to duck
               selectCampButtons (index);
            }
         }

         // was elseif here but brokes csde_ scenario
         if (game.mapIs (MapFlags::Demolition) && (m_pathFlags & NodeFlag::Goal) && m_inBombZone) {
            // is it a terrorist carrying the bomb?
            if (m_hasC4) {
               if ((m_states & Sense::SeeingEnemy) && numFriendsNear (pev->origin, 768.0f) == 0) {
                  // request an help also
                  pushRadioMessage (Radio::NeedBackup);
                  pushChatterMessage (Chatter::ScaredEmotion);

                  startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + rg (4.0f, 8.0f), true);
               }
               else {
                  startTask (Task::PlantBomb, TaskPri::PlantBomb, kInvalidNodeIndex, 0.0f, false);
               }
            }
            else if (m_team == Team::CT) {
               if (!bots.isBombPlanted () && numFriendsNear (pev->origin, 210.0f) < 4) {
                  const int index = findDefendNode (m_path->origin);
                  float campTime = rg (25.0f, 40.0f);

                  // rusher bots don't like to camp too much
                  if (m_personality == Personality::Rusher) {
                     campTime *= 0.5f;
                  }
                  startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + campTime, true); // push camp task on to stack
                  startTask (Task::MoveToPosition, TaskPri::MoveToPosition, index, game.time () + rg (5.0f, 11.0f), true); // push move command

                  // decide to duck or not to duck
                  selectCampButtons (index);

                  pushChatterMessage (Chatter::DefendingBombsite); // play info about that
               }
            }
         }
      }
   }
   // no more nodes to follow - search new ones (or we have a bomb)
   else if (!hasActiveGoal ()) {
      ignoreCollision ();

      // did we already decide about a goal before?
      const auto currIndex = getTask ()->data;
      auto destIndex = graph.exists (currIndex) ? currIndex : findBestGoal ();

      // check for existence (this is fail over, for i.e. CSDM, this should be not true with normal game play, only when spawned outside of covered area)
      if (!graph.exists (destIndex)) {
         destIndex = graph.getFarest (pev->origin, 1024.0f);
      }
      m_prevGoalIndex = destIndex;

      // remember index
      getTask ()->data = destIndex;

      auto pathSearchType = m_pathType;

      // override with fast path
      if (game.mapIs (MapFlags::Demolition) && bots.isBombPlanted ()) {
         pathSearchType = rg.chance (80) ? FindPath::Fast : FindPath::Optimal;
      }
      ensureCurrentNodeIndex ();

      // do pathfinding if it's not the current
      if (destIndex != m_currentNodeIndex) {
         findPath (m_currentNodeIndex, destIndex, pathSearchType);
      }
   }
   else {
      if (!isDucking () && !usesKnife () && !cr::fequal (m_minSpeed, pev->maxspeed) && m_minSpeed > 1.0f) {
         m_moveSpeed = m_minSpeed;
      }
   }
   const float shiftSpeed = getShiftSpeed ();

   if ((!cr::fzero (m_moveSpeed) && m_moveSpeed > shiftSpeed) && (cv_walking_allowed && mp_footsteps)
      && m_difficulty >= Difficulty::Normal
      && (m_heardSoundTime + 6.0f >= game.time () || (m_states & Sense::HearingEnemy))
      && numEnemiesNear (pev->origin, 768.0f) >= 1
      && !isKnifeMode ()
      && !bots.isBombPlanted ()) {

      m_moveSpeed = shiftSpeed;
   }

   // bot hasn't seen anything in a long time and is asking his teammates to report in
   if (cv_radio_mode.as <int> () > 1
      && bots.getLastRadio (m_team) != Radio::ReportInTeam
      && bots.getRoundStartTime () + 20.0f < game.time ()
      && m_askCheckTime < game.time () && rg.chance (15)
      && m_seeEnemyTime + rg (45.0f, 80.0f) < game.time ()
      && numFriendsNear (pev->origin, 1024.0f) == 0) {

      pushRadioMessage (Radio::ReportInTeam);

      m_askCheckTime = game.time () + rg (45.0f, 80.0f);

      // make sure everyone else will not ask next few moments
      for (const auto &bot : bots) {
         if (bot->m_isAlive) {
            bot->m_askCheckTime = game.time () + rg (5.0f, 30.0f);
         }
      }
   }
}

void Bot::spraypaint_ () {
   m_aimFlags |= AimFlags::Entity;

   // bot didn't spray this round?
   if (m_timeLogoSpray <= game.time () && getTask ()->time > game.time ()) {
      const auto &forward = pev->v_angle.forward ();
      Vector sprayOrigin = getEyesPos () + forward * 128.0f;

      TraceResult tr {};
      game.testLine (getEyesPos (), sprayOrigin, TraceIgnore::Monsters, ent (), &tr);

      // no wall in front?
      if (tr.flFraction >= 1.0f) {
         sprayOrigin.z -= 128.0f;
      }
      m_entity = sprayOrigin;

      if (getTask ()->time - 0.5f < game.time ()) {
         // emit spray can sound
         engfuncs.pfnEmitSound (ent (), CHAN_VOICE, "player/sprayer.wav", 1.0f, ATTN_NORM, 0, 100);

         game.testLine (getEyesPos (), getEyesPos () + forward * 128.0f, TraceIgnore::Monsters, ent (), &tr);

         // paint the actual logo decal
         util.decalTrace (&tr, m_logoDecalIndex);
         m_timeLogoSpray = game.time () + rg (60.0f, 90.0f);
      }
   }
   else {
      completeTask ();
   }
   m_moveToGoal = false;
   m_checkTerrain = false;

   m_navTimeset = game.time ();
   m_moveSpeed = 0.0f;
   m_strafeSpeed = 0.0f;

   ignoreCollision ();
}

void Bot::huntEnemy_ () {
   m_aimFlags |= AimFlags::Nav;

   // if we've got new enemy...
   if (!game.isNullEntity (m_enemy) || game.isNullEntity (m_lastEnemy)) {

      // forget about it...
      clearTask (Task::Hunt);
      m_prevGoalIndex = kInvalidNodeIndex;
   }
   else if (game.getTeam (m_lastEnemy) == m_team) {

      // don't hunt down our teammate...
      clearTask (Task::Hunt);

      m_prevGoalIndex = kInvalidNodeIndex;
      m_lastEnemy = nullptr;
   }
   else if (updateNavigation ()) // reached last enemy pos?
   {
      // forget about it...
      completeTask ();

      m_prevGoalIndex = kInvalidNodeIndex;
      m_lastEnemyOrigin.clear ();
   }

   // do we need to calculate a new path?
   else if (!hasActiveGoal ()) {
      int destIndex = kInvalidNodeIndex;
      const int goal = getTask ()->data;

      // is there a remembered index?
      if (graph.exists (goal)) {
         destIndex = goal;
      }

      // find new one instead
      else {
         destIndex = graph.getNearest (m_lastEnemyOrigin);
      }

      // remember index
      m_prevGoalIndex = destIndex;
      getTask ()->data = destIndex;

      if (destIndex != m_currentNodeIndex) {
         findPath (m_currentNodeIndex, destIndex, FindPath::Fast);
      }
   }

   // bots skill higher than 60?
   if (cv_walking_allowed && mp_footsteps && m_difficulty >= Difficulty::Normal && !isKnifeMode ()) {
      // then make him move slow if near enemy
      if (!(m_currentTravelFlags & PathFlag::Jump)) {
         if (m_currentNodeIndex != kInvalidNodeIndex) {
            if (m_path->radius < 32.0f && !isOnLadder () && !isInWater () && m_seeEnemyTime + 4.0f > game.time ()) {
               m_moveSpeed = getShiftSpeed ();
            }
         }
      }
   }
}

void Bot::seekCover_ () {
   m_aimFlags |= AimFlags::Nav;

   if (!util.isAlive (m_lastEnemy)) {
      completeTask ();
      m_prevGoalIndex = kInvalidNodeIndex;
   }

   // reached final node?
   else if (updateNavigation ()) {
      // yep. activate hide behavior
      completeTask ();
      m_prevGoalIndex = kInvalidNodeIndex;

      // start hide task
      startTask (Task::Hide, TaskPri::Hide, kInvalidNodeIndex, game.time () + rg (3.0f, 12.0f), false);

      // get a valid look direction
      const auto &dest = getCampDirection (m_lastEnemyOrigin);

      m_aimFlags |= AimFlags::Camp;
      m_lookAtSafe = dest;
      m_campDirection = 0;

      // chosen node is a camp node?
      if (m_pathFlags & NodeFlag::Camp) {
         // use the existing camp node prefs
         if (m_pathFlags & NodeFlag::Crouch) {
            m_campButtons = IN_DUCK;
         }
         else {
            m_campButtons = 0;
         }
      }
      else {
         // choose a crouch or stand pos
         if (m_path->vis.crouch <= m_path->vis.stand) {
            m_campButtons = IN_DUCK;
         }
         else {
            m_campButtons = 0;
         }

         // enter look direction from previously calculated positions
         if (!dest.empty ()) {
            m_lookAtSafe = dest;
         }
      }

      if (m_reloadState == Reload::None && getAmmoInClip () < 5 && getAmmo () != 0) {
         m_reloadState = Reload::Primary;
      }
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;

      m_moveToGoal = false;
      m_checkTerrain = false;
   }
   else if (!hasActiveGoal ()) {
      int destIndex = kInvalidNodeIndex;

      if (getTask ()->data != kInvalidNodeIndex) {
         destIndex = getTask ()->data;
      }
      else {
         destIndex = findCoverNode (900.0f);

         if (destIndex == kInvalidNodeIndex) {
            m_retreatTime = game.time () + rg (1.0f, 2.0f);
            m_prevGoalIndex = kInvalidNodeIndex;

            completeTask ();
            return;
         }
      }
      m_campDirection = 0;

      m_prevGoalIndex = destIndex;
      getTask ()->data = destIndex;

      ensureCurrentNodeIndex ();

      if (destIndex != m_currentNodeIndex) {
         findPath (m_currentNodeIndex, destIndex, FindPath::Fast);
      }
   }
}

void Bot::attackEnemy_ () {
   m_moveToGoal = false;
   m_checkTerrain = false;

   // always ignore collision checks in this task
   ignoreCollision ();

   if (!game.isNullEntity (m_enemy)) {
      attackMovement ();

      if (usesKnife () && !m_enemyOrigin.empty ()) {
         m_destOrigin = m_enemyOrigin;
      }
   }
   else {
      completeTask ();
      findNextBestNode ();

      if (!m_lastEnemyOrigin.empty ()) {
         m_destOrigin = m_lastEnemyOrigin;
      }
   }
   m_navTimeset = game.time ();
}

void Bot::pause_ () {
   m_moveToGoal = false;
   m_checkTerrain = false;

   m_navTimeset = game.time ();
   m_moveSpeed = 0.0f;
   m_strafeSpeed = 0.0f;

   m_aimFlags |= AimFlags::Nav;

   // is bot blinded and above average difficulty?
   if (m_viewDistance < 500.0f && m_difficulty >= Difficulty::Normal) {
      // go mad!
      m_moveSpeed = -cr::abs ((m_viewDistance - 500.0f) * 0.5f);

      if (m_moveSpeed < -pev->maxspeed) {
         m_moveSpeed = -pev->maxspeed;
      }
      m_lookAtSafe = getEyesPos () + pev->v_angle.forward () * 500.0f;

      m_aimFlags |= AimFlags::Override;
      m_wantsToFire = true;
   }
   else {
      pev->button |= m_campButtons;
   }

   // stop camping if time over or gets hurt by something else than bullets
   if (getTask ()->time < game.time () || m_lastDamageType > 0) {
      completeTask ();
   }
}

void Bot::blind_ () {
   m_moveToGoal = false;
   m_checkTerrain = false;
   m_navTimeset = game.time ();

   // if bot remembers last enemy position
   if (rg.chance (50)
      && m_difficulty >= Difficulty::Normal
      && !m_lastEnemyOrigin.empty ()
      && util.isPlayer (m_lastEnemy)
      && !usesSniper ()) {

      auto error = kSprayDistance * m_lastEnemyOrigin.distance (pev->origin) / 2048.0f;
      auto origin = m_lastEnemyOrigin;

      origin.x = origin.x + rg (-error, error);
      origin.y = origin.y + rg (-error, error);

      m_lookAt = origin; // face last enemy
      m_wantsToFire = true; // and shoot it
   }

   if (m_difficulty >= Difficulty::Normal && graph.exists (m_blindNodeIndex)) {
      if (updateNavigation ()) {
         if (m_blindTime < game.time ()) {
            completeTask ();
         }
         m_prevGoalIndex = kInvalidNodeIndex;
         m_blindNodeIndex = kInvalidNodeIndex;

         m_blindMoveSpeed = 0.0f;
         m_blindSideMoveSpeed = 0.0f;
         m_blindButton = 0;

         m_states |= Sense::SuspectEnemy;
      }
      else if (!hasActiveGoal ()) {
         ensureCurrentNodeIndex ();

         m_prevGoalIndex = m_blindNodeIndex;
         getTask ()->data = m_blindNodeIndex;

         findPath (m_currentNodeIndex, m_blindNodeIndex, FindPath::Fast);
      }
   }
   else {
      m_moveSpeed = m_blindMoveSpeed;
      m_strafeSpeed = m_blindSideMoveSpeed;
      pev->button |= m_blindButton;

      m_states |= Sense::SuspectEnemy;
   }

   if (m_blindTime < game.time ()) {
      completeTask ();
   }
}

void Bot::camp_ () {
   if (!cv_camping_allowed || m_isCreature) {
      completeTask ();
      return;
   }

   m_aimFlags |= AimFlags::Camp;
   m_checkTerrain = false;
   m_moveToGoal = false;

   if (m_team == Team::CT && bots.isBombPlanted () && m_defendedBomb && !isBombDefusing (graph.getBombOrigin ()) && !isOutOfBombTimer ()) {
      m_defendedBomb = false;
      completeTask ();
   }
   ignoreCollision ();

   // half the reaction time if camping because you're more aware of enemies if camping
   setIdealReactionTimers ();
   m_idealReactionTime *= 0.5f;

   m_navTimeset = game.time ();
   m_timeCamping = game.time ();

   m_moveSpeed = 0.0f;
   m_strafeSpeed = 0.0f;

   findValidNode ();

   // random camp dir, or prediction
   auto useRandomCampDirOrPredictEnemy = [&] () {
      if (!m_lastEnemyOrigin.empty () && util.isAlive (m_lastEnemy)) {
         auto pathLength = m_lastPredictLength;
         auto predictNode = m_lastPredictIndex;

         if (isNodeValidForPredict (predictNode) && pathLength > 1) {
            m_lookAtSafe = graph[predictNode].origin + pev->view_ofs;
         }
         else {
            pathLength = 0;
            predictNode = findAimingNode (m_lastEnemyOrigin, pathLength);

            if (isNodeValidForPredict (predictNode) && pathLength > 1) {
               m_lookAtSafe = graph[predictNode].origin + pev->view_ofs;
            }
         }
      }
      else {
         m_lookAtSafe = graph[getRandomCampDir ()].origin + pev->view_ofs;
      }
   };

   if (m_nextCampDirTime < game.time ()) {
      if (m_pathFlags & NodeFlag::Camp) {
         Vector dest {};

         // switch from 1 direction to the other
         if (m_campDirection < 1) {
            dest = m_path->start;
            m_campDirection ^= 1;
         }
         else {
            dest = m_path->end;
            m_campDirection ^= 1;
         }
         dest.z = 0.0f;

         // check if after the conversion camp start and camp end are broken, and bot will look into the wall
         TraceResult tr {};

         // and use real angles to check it
         const auto &to = m_pathOrigin + dest.forward () * 500.0f;

         // let's check the destination
         game.testLine (getEyesPos (), to, TraceIgnore::Monsters, ent (), &tr);

         // we're probably facing the wall, so ignore the flags provided by graph, and use our own
         if (tr.flFraction < 0.5f) {
            useRandomCampDirOrPredictEnemy ();
         }
         else {
            m_lookAtSafe = to;
         }
      }
      else {
         useRandomCampDirOrPredictEnemy ();
      }
      m_nextCampDirTime = game.time () + rg (1.0f, 4.0f);
   }
   // press remembered crouch button
   pev->button |= m_campButtons;

   // stop camping if time over or gets hurt by something else than bullets
   if (getTask ()->time < game.time () || m_lastDamageType > 0) {
      completeTask ();
   }
}

void Bot::hide_ () {
   if (m_isCreature) {
      completeTask ();
      return;
   };

   m_aimFlags |= AimFlags::Camp;
   m_checkTerrain = false;
   m_moveToGoal = false;

   // half the reaction time if camping
   setIdealReactionTimers ();
   m_idealReactionTime *= 0.5f;

   m_navTimeset = game.time ();
   m_moveSpeed = 0.0f;
   m_strafeSpeed = 0.0f;

   findValidNode ();

   if (hasShield () && !m_isReloading) {
      if (!isShieldDrawn ()) {
         pev->button |= IN_ATTACK2; // draw the shield!
      }
      else {
         pev->button |= IN_DUCK; // duck under if the shield is already drawn
      }
   }

   // if we see an enemy and aren't at a good camping point leave the spot
   if ((m_states & Sense::SeeingEnemy) || m_inBombZone) {
      if (!(m_pathFlags & NodeFlag::Camp)) {
         completeTask ();

         m_campButtons = 0;
         m_prevGoalIndex = kInvalidNodeIndex;

         return;
      }
   }

   // if we don't have an enemy we're also free to leave
   else if (m_lastEnemyOrigin.empty ()) {
      completeTask ();

      m_campButtons = 0;
      m_prevGoalIndex = kInvalidNodeIndex;

      if (getCurrentTaskId () == Task::Hide) {
         completeTask ();
      }
      return;
   }

   pev->button |= m_campButtons;
   m_navTimeset = game.time ();

   if (!m_isReloading) {
      checkReload ();
   }

   // stop camping if time over or gets hurt by something else than bullets
   if (getTask ()->time < game.time () || m_lastDamageType > 0) {
      completeTask ();
   }
}

void Bot::moveToPos_ () {
   m_aimFlags |= AimFlags::Nav;

   if (isShieldDrawn ()) {
      pev->button |= IN_ATTACK2;
   }

   auto ensureDestIndexOK = [&] (int &index) {
      if (!m_position.empty () && isOccupiedNode (index)) {
         index = findDefendNode (m_position);
      }
   };

   // reached destination?
   if (updateNavigation ()) {
      completeTask (); // we're done

      m_prevGoalIndex = kInvalidNodeIndex;
      m_position.clear ();
   }
   // didn't choose goal node yet?
   else if (!hasActiveGoal ()) {
      int destIndex = kInvalidNodeIndex;
      const int goal = getTask ()->data;

      if (graph.exists (goal)) {
         destIndex = goal;

         // check if we're ok
         ensureDestIndexOK (destIndex);
      }
      else {
         destIndex = graph.getNearest (m_position);

         // check if we're ok
         ensureDestIndexOK (destIndex);
      }

      if (graph.exists (destIndex)) {
         m_prevGoalIndex = destIndex;
         getTask ()->data = destIndex;

         ensureCurrentNodeIndex ();
         findPath (m_currentNodeIndex, destIndex, m_isCreature ? FindPath::Fast : m_pathType);
      }
      else {
         completeTask ();
      }
   }
}

void Bot::plantBomb_ () {
   m_aimFlags |= AimFlags::Camp;

   // we're still got the C4?
   if (m_hasC4 && !isKnifeMode ()) {
      if (m_currentWeapon != Weapon::C4) {
         selectWeaponById (Weapon::C4);
      }

      if (util.isAlive (m_enemy) || !m_inBombZone) {
         completeTask ();
      }
      else {
         m_moveToGoal = false;
         m_checkTerrain = false;
         m_navTimeset = game.time ();

         if (m_pathFlags & NodeFlag::Crouch) {
            pev->button |= (IN_ATTACK | IN_DUCK);
         }
         else {
            pev->button |= IN_ATTACK;
         }
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;
      }
   }

   // done with planting
   else {
      completeTask ();

      // tell teammates to move over here...
      if (numFriendsNear (pev->origin, 1200.0f) > 0) {
         pushRadioMessage (Radio::NeedBackup);
      }
      const auto index = findDefendNode (pev->origin);
      const auto guardTime = mp_c4timer.as <float> () * 0.5f + mp_c4timer.as <float> () * 0.25f;

      // push camp task on to stack
      startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + guardTime, true);

      // push move command
      startTask (Task::MoveToPosition, TaskPri::MoveToPosition, index, game.time () + guardTime, true);

      // decide to duck or not to duck
      selectCampButtons (index);
   }
}

void Bot::defuseBomb_ () {
   const float fullDefuseTime = m_hasDefuser ? 7.0f : 12.0f;
   const float timeToBlowUp = getBombTimeleft ();

   float defuseRemainingTime = fullDefuseTime;

   if (m_hasProgressBar /*&& isOnFloor ()*/) {
      defuseRemainingTime = fullDefuseTime - game.time ();
   }

   const auto &bombPos = graph.getBombOrigin ();
   bool defuseError = false;

   // exception: bomb has been defused
   if (bombPos.empty ()) {
      // fix for stupid behavior of CT's when bot is defused
      for (const auto &bot : bots) {
         if (bot->m_team == m_team && bot->m_isAlive) {
            auto defendPoint = graph.getFarest (bot->pev->origin);

            startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + rg (30.0f, 60.0f), true); // push camp task on to stack
            startTask (Task::MoveToPosition, TaskPri::MoveToPosition, defendPoint, game.time () + rg (3.0f, 6.0f), true); // push move command
         }
      }
      graph.setBombOrigin (true);

      if (m_numFriendsLeft != 0 && rg.chance (50)) {
         if (timeToBlowUp <= 3.0f) {
            if (cv_radio_mode.as <int> () == 2) {
               pushChatterMessage (Chatter::BarelyDefused);
            }
            else if (cv_radio_mode.as <int> () == 1) {
               pushRadioMessage (Radio::SectorClear);
            }
         }
         else {
            pushRadioMessage (Radio::SectorClear);
         }
      }
      return;
   }
   else if (defuseRemainingTime > timeToBlowUp) {
      defuseError = true;
   }
   else if (m_states & Sense::SeeingEnemy) {
      const int friends = numFriendsNear (pev->origin, 768.0f);

      if (friends < 2 && defuseRemainingTime < timeToBlowUp) {
         defuseError = true;

         if (defuseRemainingTime + 2.0f > timeToBlowUp) {
            defuseError = false;
         }

         if (m_numEnemiesLeft > 0 && m_numFriendsLeft > friends) {
            pushRadioMessage (Radio::NeedBackup);
         }
      }
   }

   // one of exceptions is thrown. finish task.
   if (defuseError) {
      m_entity.clear ();

      m_pickupItem = nullptr;
      m_pickupType = Pickup::None;

      selectBestWeapon ();
      resetCollision ();

      completeTask ();

      return;
   }

   // to revert from pause after reload  ting && just to be sure
   m_moveToGoal = false;
   m_checkTerrain = false;

   m_moveSpeed = pev->maxspeed;
   m_strafeSpeed = 0.0f;

   // bot is reloading and we close enough to start defusing
   if (m_isReloading && bombPos.distanceSq2d (pev->origin) < cr::sqrf (80.0f)) {
      if (m_numEnemiesLeft == 0
         || timeToBlowUp < fullDefuseTime + 7.0f
         || ((getAmmoInClip () > 8 && m_reloadState == Reload::Primary) || (getAmmoInClip () > 5 && m_reloadState == Reload::Secondary))) {

         const int weaponIndex = getBestOwnedWeapon ();

         // just select knife and then select weapon
         selectWeaponById (Weapon::Knife);

         if (weaponIndex > 0 && weaponIndex < kNumWeapons) {
            selectWeaponByIndex (weaponIndex);
         }
         m_isReloading = false;
      }
      else {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;
      }
   }

   // head to bomb and press use button
   m_aimFlags |= AimFlags::Entity;

   m_destOrigin = bombPos;
   m_entity = bombPos;

   pev->button |= IN_USE;

   // if defusing is not already started, maybe crouch before
   if (!m_hasProgressBar && m_duckDefuseCheckTime < game.time ()) {
      Vector botDuckOrigin {}, botStandOrigin {};

      if (pev->button & IN_DUCK) {
         botDuckOrigin = pev->origin;
         botStandOrigin = pev->origin + Vector (0.0f, 0.0f, 18.0f);
      }
      else {
         botDuckOrigin = pev->origin - Vector (0.0f, 0.0f, 18.0f);
         botStandOrigin = pev->origin;
      }

      const float duckDistanceSq = m_entity.distanceSq (botDuckOrigin);
      const float standDistanceSq = m_entity.distanceSq (botStandOrigin);

      if (duckDistanceSq > cr::sqrf (75.0f) || standDistanceSq > cr::sqrf (75.0f)) {
         if (standDistanceSq < duckDistanceSq) {
            m_duckDefuse = false; // stand
         }
         else {
            m_duckDefuse = m_difficulty >= Difficulty::Normal && m_numEnemiesLeft != 0; // duck
         }
      }
      m_duckDefuseCheckTime = game.time () + 5.0f;
   }

   // press duck button
   if (m_duckDefuse || (m_oldButtons & IN_DUCK)) {
      pev->button |= IN_DUCK;
   }
   else {
      pev->button &= ~IN_DUCK;
   }

   // we are defusing bomb
   if (m_hasProgressBar || (m_oldButtons & IN_USE) || !game.isNullEntity (m_pickupItem)) {
      pev->button |= IN_USE;

      if (!game.isNullEntity (m_pickupItem)) {
         MDLL_Use (m_pickupItem, ent ());
      }

      m_reloadState = Reload::None;
      m_navTimeset = game.time ();

      // don't move when defusing
      m_moveToGoal = false;
      m_checkTerrain = false;

      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;

      // notify team
      if (m_numFriendsLeft > 0) {
         pushChatterMessage (Chatter::DefusingBomb);

         if (m_numEnemiesLeft > 0 && numFriendsNear (pev->origin, 512.0f) < 2) {
            pushRadioMessage (Radio::NeedBackup);
         }
      }
   }
   else {
      completeTask ();
   }
}

void Bot::followUser_ () {
   if (game.isNullEntity (m_targetEntity) || !util.isAlive (m_targetEntity)) {
      m_targetEntity = nullptr;
      completeTask ();

      return;
   }

   if (m_targetEntity->v.button & IN_ATTACK) {
      TraceResult tr {};
      game.testLine (m_targetEntity->v.origin + m_targetEntity->v.view_ofs, m_targetEntity->v.v_angle.forward () * 500.0f, TraceIgnore::Everything, ent (), &tr);

      if (!game.isNullEntity (tr.pHit) && util.isPlayer (tr.pHit) && game.getTeam (tr.pHit) != m_team) {
         m_targetEntity = nullptr;
         m_lastEnemy = tr.pHit;
         m_lastEnemyOrigin = tr.pHit->v.origin;

         completeTask ();
         return;
      }
   }

   if (!cr::fzero (m_targetEntity->v.maxspeed) && m_targetEntity->v.maxspeed < pev->maxspeed) {
      m_moveSpeed = m_targetEntity->v.maxspeed;
   }

   if (m_reloadState == Reload::None && getAmmo () != 0) {
      m_reloadState = Reload::Primary;
   }

   if (m_targetEntity->v.origin.distanceSq (pev->origin) > cr::sqrf (130.0f)) {
      m_followWaitTime = 0.0f;
   }
   else {
      m_moveSpeed = 0.0f;

      if (cr::fzero (m_followWaitTime)) {
         m_followWaitTime = game.time ();
      }
      else {
         if (m_followWaitTime + 3.0f < game.time ()) {
            // stop following if we have been waiting too long
            m_targetEntity = nullptr;

            pushRadioMessage (Radio::YouTakeThePoint);
            completeTask ();

            return;
         }
      }
   }
   m_aimFlags |= AimFlags::Nav;

   if (cv_walking_allowed && m_targetEntity->v.maxspeed < m_moveSpeed && !isKnifeMode ()) {
      m_moveSpeed = getShiftSpeed ();
   }

   if (isShieldDrawn ()) {
      pev->button |= IN_ATTACK2;
   }

   // reached destination?
   if (updateNavigation ()) {
      getTask ()->data = kInvalidNodeIndex;
   }

   // didn't choose goal node yet?
   if (!hasActiveGoal ()) {
      int destIndex = graph.getNearest (m_targetEntity->v.origin);
      auto points = graph.getNearestInRadius (200.0f, m_targetEntity->v.origin);

      for (const auto &newIndex : points) {
         // if node not yet used, assign it as dest
         if (newIndex != m_currentNodeIndex && !isOccupiedNode (newIndex)) {
            destIndex = newIndex;
         }
      }

      if (graph.exists (destIndex) && graph.exists (m_currentNodeIndex)) {
         m_prevGoalIndex = destIndex;
         getTask ()->data = destIndex;

         // always take the shortest path
         findPath (m_currentNodeIndex, destIndex, FindPath::Fast);
      }
      else {
         m_targetEntity = nullptr;
         completeTask ();
      }
   }
}

void Bot::throwExplosive_ () {
   Vector dest = m_throw;

   if (!(m_states & Sense::SeeingEnemy)) {
      m_strafeSpeed = 0.0f;
      m_moveSpeed = 0.0f;
      m_moveToGoal = false;
   }
   else if (!(m_states & Sense::SuspectEnemy) && !game.isNullEntity (m_enemy)) {
      dest = m_enemy->v.origin + m_enemy->v.velocity.get2d ();
   }
   m_isUsingGrenade = true;
   m_checkTerrain = false;

   ignoreCollision ();

   if (!isGrenadeWar () && pev->origin.distanceSq (dest) < cr::sqrf (kGrenadeDamageRadius)) {
      // heck, I don't wanna blow up myself
      m_grenadeCheckTime = game.time () + kGrenadeCheckTime * 2.0f;

      selectBestWeapon ();
      completeTask ();

      return;
   }
   m_grenade = calcThrow (getEyesPos (), dest);

   if (m_grenade.lengthSq () < 100.0f) {
      m_grenade = calcToss (pev->origin, dest);
   }

   if (!isGrenadeWar () && m_grenade.lengthSq () <= 100.0f) {
      m_grenadeCheckTime = game.time () + kGrenadeCheckTime * 2.0f;

      selectBestWeapon ();
      completeTask ();
   }
   else {
      m_aimFlags |= AimFlags::Grenade;

      auto grenade = setCorrectGrenadeVelocity (kExplosiveModelName);

      if (game.isNullEntity (grenade)) {
         if (m_currentWeapon != Weapon::Explosive) {
            if (pev->weapons & cr::bit (Weapon::Explosive)) {
               selectWeaponById (Weapon::Explosive);
            }
            else {
               selectBestWeapon ();
               completeTask ();

               return;
            }
         }
         else if (!(m_oldButtons & IN_ATTACK)) {
            pev->button |= IN_ATTACK;
         }
      }
   }
   pev->button |= m_campButtons;
}

void Bot::throwFlashbang_ () {
   Vector dest = m_throw;

   if (!(m_states & Sense::SeeingEnemy)) {
      m_strafeSpeed = 0.0f;
      m_moveSpeed = 0.0f;
      m_moveToGoal = false;
   }
   else if (!(m_states & Sense::SuspectEnemy) && !game.isNullEntity (m_enemy)) {
      dest = m_enemy->v.origin + m_enemy->v.velocity.get2d ();
   }

   m_isUsingGrenade = true;
   m_checkTerrain = false;

   ignoreCollision ();

   if (pev->origin.distanceSq (dest) < cr::sqrf (kGrenadeDamageRadius)) {
      m_grenadeCheckTime = game.time () + kGrenadeCheckTime * 2.0f; // heck, I don't wanna blow up myself

      selectBestWeapon ();
      completeTask ();

      return;
   }
   m_grenade = calcThrow (getEyesPos (), dest);

   if (m_grenade.lengthSq () < 100.0f) {
      m_grenade = calcToss (pev->origin, dest);
   }

   if (m_grenade.lengthSq () <= 100.0f) {
      m_grenadeCheckTime = game.time () + kGrenadeCheckTime * 2.0f;

      selectBestWeapon ();
      completeTask ();
   }
   else {
      m_aimFlags |= AimFlags::Grenade;

      auto grenade = setCorrectGrenadeVelocity (kFlashbangModelName);

      if (game.isNullEntity (grenade)) {
         if (m_currentWeapon != Weapon::Flashbang) {
            if (pev->weapons & cr::bit (Weapon::Flashbang)) {
               selectWeaponById (Weapon::Flashbang);
            }
            else {
               selectBestWeapon ();
               completeTask ();

               return;
            }
         }
         else if (!(m_oldButtons & IN_ATTACK)) {
            pev->button |= IN_ATTACK;
         }
      }
   }
   pev->button |= m_campButtons;
}

void Bot::throwSmoke_ () {
   if (!(m_states & Sense::SeeingEnemy)) {
      m_strafeSpeed = 0.0f;
      m_moveSpeed = 0.0f;
      m_moveToGoal = false;
   }

   m_checkTerrain = false;
   m_isUsingGrenade = true;

   ignoreCollision ();

   Vector src = m_lastEnemyOrigin - pev->velocity;

   // predict where the enemy is in secs
   if (!game.isNullEntity (m_enemy)) {
      src = src + m_enemy->v.velocity;
   }
   m_grenade = (src - getEyesPos ()).normalize_apx ();

   if (getTask ()->time < game.time ()) {
      completeTask ();
      return;
   }

   if (m_currentWeapon != Weapon::Smoke) {
      m_aimFlags |= AimFlags::Grenade;

      if (pev->weapons & cr::bit (Weapon::Smoke)) {
         selectWeaponById (Weapon::Smoke);
         getTask ()->time = game.time () + kGrenadeCheckTime * 2.0f;
      }
      else {
         selectBestWeapon ();
         completeTask ();

         return;
      }
   }
   else if (!(m_oldButtons & IN_ATTACK)) {
      pev->button |= IN_ATTACK;
   }
   pev->button |= m_campButtons;
}

void Bot::doublejump_ () {
   if (!util.isAlive (m_doubleJumpEntity)
      || (m_aimFlags & AimFlags::Enemy)
      || (m_travelStartIndex != kInvalidNodeIndex
         && getTask ()->time + (graph.calculateTravelTime (pev->maxspeed, graph[m_travelStartIndex].origin, m_doubleJumpOrigin) + 11.0f) < game.time ())) {
      resetDoubleJump ();
      return;
   }
   m_aimFlags |= AimFlags::Nav;

   if (m_jumpReady) {
      m_moveToGoal = false;
      m_checkTerrain = false;

      m_navTimeset = game.time ();
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;

      bool inJump = (m_doubleJumpEntity->v.button & IN_JUMP) || (m_doubleJumpEntity->v.oldbuttons & IN_JUMP);

      if (m_duckForJump < game.time ()) {
         pev->button |= IN_DUCK;
      }
      else if (inJump && !(m_oldButtons & IN_JUMP)) {
         pev->button |= IN_JUMP;
      }

      const auto &src = pev->origin + Vector (0.0f, 0.0f, 45.0f);
      const auto &dest = src + Vector (0.0f, pev->angles.y, 0.0f).upward () * 256.0f;

      TraceResult tr {};
      game.testLine (src, dest, TraceIgnore::None, ent (), &tr);

      if (tr.flFraction < 1.0f && tr.pHit == m_doubleJumpEntity && inJump) {
         m_duckForJump = game.time () + rg (3.0f, 5.0f);
         getTask ()->time = game.time ();
      }
      return;
   }

   if (m_currentNodeIndex == m_prevGoalIndex) {
      m_pathOrigin = m_doubleJumpOrigin;
      m_destOrigin = m_doubleJumpOrigin;
   }

   if (updateNavigation ()) {
      getTask ()->data = kInvalidNodeIndex;
   }

   // didn't choose goal node yet?
   if (!hasActiveGoal ()) {
      int destIndex = graph.getNearest (m_doubleJumpOrigin);

      if (graph.exists (destIndex)) {
         m_prevGoalIndex = destIndex;
         m_travelStartIndex = m_currentNodeIndex;

         getTask ()->data = destIndex;

         // always take the shortest path
         findPath (m_currentNodeIndex, destIndex, FindPath::Fast);

         if (m_currentNodeIndex == destIndex) {
            m_jumpReady = true;
         }
      }
      else {
         resetDoubleJump ();
      }
   }
}

void Bot::escapeFromBomb_ () {
   m_aimFlags |= AimFlags::Nav;

   if (!bots.isBombPlanted ()) {
      completeTask ();
   }

   if (isShieldDrawn ()) {
      pev->button |= IN_ATTACK2;
   }

   if (!usesKnife () && game.isNullEntity (m_enemy) && !util.isAlive (m_lastEnemy)) {
      selectWeaponById (Weapon::Knife);
   }

   // reached destination?
   if (updateNavigation ()) {
      completeTask (); // we're done

      // press duck button if we still have some enemies
      if (m_numEnemiesLeft > 0) {
         m_campButtons = IN_DUCK;
      }

      // we're reached destination point so just sit down and camp
      startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + 10.0f, true);
   }

   // didn't choose goal node yet?
   else if (!hasActiveGoal ()) {
      int bestIndex = kInvalidNodeIndex;

      const float safeRadius = rg (1513.0f, 2048.0f);
      float nearestDistanceSq = kInfiniteDistance;

      for (const auto &path : graph) {
         if (path.origin.distanceSq (graph.getBombOrigin ()) < cr::sqrf (safeRadius) || isOccupiedNode (path.number)) {
            continue;
         }
         const float distanceSq = pev->origin.distanceSq (path.origin);

         if (nearestDistanceSq > distanceSq) {
            nearestDistanceSq = distanceSq;
            bestIndex = path.number;
         }
      }

      if (bestIndex < 0) {
         bestIndex = graph.getFarest (pev->origin, safeRadius);
      }

      // still no luck?
      if (bestIndex < 0) {
         completeTask (); // we're done

         // we have no destination point, so just sit down and camp
         startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + 10.0f, true);
         return;
      }
      m_prevGoalIndex = bestIndex;
      getTask ()->data = bestIndex;

      findPath (m_currentNodeIndex, bestIndex, FindPath::Fast);
   }
}

void Bot::shootBreakable_ () {

   // breakable destroyed?
   if (!util.isBreakableEntity (m_breakableEntity)) {
      completeTask ();
      return;
   }
   else {
      TraceResult tr {};
      game.testLine (pev->origin, m_breakableOrigin, TraceIgnore::None, ent (), &tr);

      if (tr.pHit != m_breakableEntity || !util.isVisible (tr.vecEndPos, ent ())) {
         m_ignoredBreakable.push (tr.pHit);

         m_breakableEntity = nullptr;
         m_breakableOrigin = nullptr;

         completeTask ();
         return;
      }
   }
   m_aimFlags |= AimFlags::Override;
   pev->button |= m_campButtons;

   m_checkTerrain = false;
   m_moveToGoal = false;

   m_navTimeset = game.time ();
   m_lookAtSafe = m_breakableOrigin;

   const float distToObstacle = pev->origin.distanceSq (m_lookAtSafe);

   // is bot facing the breakable?
   if (util.getConeDeviation (ent (), m_lookAtSafe) >= 0.90f) {
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;

      m_wantsToFire = true;
      m_shootTime = game.time ();

      // enforce shooting
      if (!usesKnife ()  && !m_isReloading && !(pev->button & IN_RELOAD) && getAmmoInClip () > 0) {
         if (!(m_oldButtons & IN_ATTACK)) {
            pev->button |= IN_ATTACK;
         }
      }

      // if with knife with no ammo, recompute breakable distance
      if (!hasAnyAmmoInClip ()
         && usesKnife ()
         && distToObstacle > cr::sqrf (72.0f)) {

         completeTask ();
      }
   }
   else {
      m_checkTerrain = true;
      m_moveToGoal = true;
   }
}

void Bot::pickupItem_ () {
   if (game.isNullEntity (m_pickupItem)) {
      m_pickupItem = nullptr;
      completeTask ();

      return;
   }
   const auto &dest = game.getEntityOrigin (m_pickupItem);

   m_destOrigin = dest;
   m_entity = dest;

   // find the distance to the item
   const float itemDistanceSq = dest.distanceSq (pev->origin);

   switch (m_pickupType) {
   case Pickup::DroppedC4:
   case Pickup::None:
   case Pickup::Items:
      break;

   case Pickup::Weapon:
   case Pickup::AmmoAndKits:
      m_aimFlags |= AimFlags::Nav;

      // near to weapon?
      if (itemDistanceSq < cr::sqrf (50.0f)) {
         int index = 0;
         auto &info = conf.getWeapons ();

         for (index = 0; index < kPrimaryWeaponMinIndex; ++index) {
            if (m_pickupItem->v.model.str (9) == info[index].model) {
               break;
            }
         }

         if (index < kPrimaryWeaponMinIndex) {
            // secondary weapon. i.e., pistol
            int weaponIndex = 0;

            for (index = 0; index < kPrimaryWeaponMinIndex; ++index) {
               if (pev->weapons & cr::bit (info[index].id)) {
                  weaponIndex = index;
               }
            }

            if (weaponIndex > 0) {
               selectWeaponByIndex (weaponIndex);
               dropCurrentWeapon ();

               if (hasShield ()) {
                  dropCurrentWeapon (); // discard both shield and pistol
               }
            }
            enteredBuyZone (BuyState::PrimaryWeapon);
         }
         else {
            // primary weapon
            int weaponIndex = getBestOwnedWeapon ();

            const bool niceWeapon = rateGroundWeapon (m_pickupItem);
            const auto tab = conf.getRawWeapons ();

            if ((weaponIndex >= kPrimaryWeaponMinIndex || tab[weaponIndex].id == Weapon::Shield || hasShield ()) && niceWeapon) {
               selectWeaponByIndex (weaponIndex);
               dropCurrentWeapon ();
            }

            if (!weaponIndex || !niceWeapon) {
               m_ignoredItems.push (m_pickupItem);

               m_pickupItem = nullptr;
               m_pickupType = Pickup::None;

               break;
            }
            enteredBuyZone (BuyState::PrimaryWeapon);
         }
         checkSilencer (); // check the silencer
      }
      break;

   case Pickup::Shield:
      m_aimFlags |= AimFlags::Nav;

      if (hasShield ()) {
         m_pickupItem = nullptr;
         break;
      }

      // near to shield?
      else if (itemDistanceSq < cr::sqrf (50.0f)) {
         // get current best weapon to check if it's a primary in need to be dropped
         int weaponIndex = getBestOwnedWeapon ();

         if (weaponIndex > 6) {
            selectWeaponByIndex (weaponIndex);
            dropCurrentWeapon ();
         }
      }
      break;

   case Pickup::PlantedC4:
      m_aimFlags |= AimFlags::Entity;

      if (m_team == Team::CT && itemDistanceSq < cr::sqrf (80.0f)) {
         pushChatterMessage (Chatter::DefusingBomb);

         // notify team of defusing
         if (m_numEnemiesLeft > 0 && m_numFriendsLeft < 3 && rg.chance (90)) {
            pushRadioMessage (Radio::NeedBackup);
         }
         m_moveToGoal = false;
         m_checkTerrain = false;

         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         startTask (Task::DefuseBomb, TaskPri::DefuseBomb, kInvalidNodeIndex, 0.0f, false);
      }
      break;

   case Pickup::Hostage:
      m_aimFlags |= AimFlags::Entity;

      if (!util.isAlive (m_pickupItem)) {
         // don't pickup dead hostages
         m_pickupItem = nullptr;
         completeTask ();

         break;
      }

      if (itemDistanceSq < cr::sqrf (50.0f)) {
         const float angleToEntity = isInFOV (dest - getEyesPos ());

         // bot faces hostage?
         if (angleToEntity <= 10.0f) {
            // use game dll function to make sure the hostage is correctly 'used'
            MDLL_Use (m_pickupItem, ent ());

            if (rg.chance (80)) {
               pushChatterMessage (Chatter::UsingHostages);
            }
            m_hostages.push (m_pickupItem);
            m_pickupItem = nullptr;

            completeTask ();

            float nearestDistanceSq = kInfiniteDistance;
            int nearestHostageNodeIndex = kInvalidNodeIndex;

            // find the nearest 'unused' hostage within the area
            game.searchEntities (pev->origin, 1024.0f, [&] (edict_t *ent) {
               if (!util.isHostageEntity (ent)) {
                  return EntitySearchResult::Continue;
               }

               // check if hostage is dead
               if (game.isNullEntity (ent) || ent->v.health <= 0) {
                  return EntitySearchResult::Continue;
               }

               // check if hostage is with a bot
               for (const auto &other : bots) {
                  if (other->m_isAlive) {
                     for (const auto &hostage : other->m_hostages) {
                        if (hostage == ent) {
                           return EntitySearchResult::Continue;
                        }
                     }
                  }
               }

               // check if hostage is with a human teammate (hack)
               for (const auto &client : util.getClients ()) {
                  if ((client.flags & ClientFlags::Used) && !(client.ent->v.flags & FL_FAKECLIENT) && (client.flags & ClientFlags::Alive) &&
                     client.team == m_team && client.ent->v.origin.distanceSq (ent->v.origin) <= cr::sqrf (240.0f)) {
                     return EntitySearchResult::Continue;
                  }
               }
               const int hostageNodeIndex = graph.getNearest (ent->v.origin);

               if (graph.exists (hostageNodeIndex)) {
                  const float distanceSq = graph[hostageNodeIndex].origin.distanceSq (pev->origin);

                  if (distanceSq < nearestDistanceSq) {
                     nearestDistanceSq = distanceSq;
                     nearestHostageNodeIndex = hostageNodeIndex;
                  }
               }

               return EntitySearchResult::Continue;
            });

            if (nearestHostageNodeIndex != kInvalidNodeIndex) {
               clearTask (Task::MoveToPosition); // remove any move tasks
               startTask (Task::MoveToPosition, TaskPri::MoveToPosition, nearestHostageNodeIndex, 0.0f, true);
            }
         }
         ignoreCollision (); // also don't consider being stuck
      }
      break;

   case Pickup::DefusalKit:
      m_aimFlags |= AimFlags::Nav;

      if (m_hasDefuser) {
         m_pickupItem = nullptr;
         m_pickupType = Pickup::None;
      }
      break;

   case Pickup::Button:
      m_aimFlags |= AimFlags::Entity;

      if (game.isNullEntity (m_pickupItem)) {
         completeTask ();
         m_pickupType = Pickup::None;

         break;
      }
      float distanceToButtonSq = cr::sqrf (90.0f);

      // reduce on lifts
      if (!game.isNullEntity (m_liftEntity)) {
         distanceToButtonSq = cr::sqrf (24.0f);
      }

      // near to the button?
      if (itemDistanceSq < distanceToButtonSq) {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;
         m_moveToGoal = false;
         m_checkTerrain = false;

         // find angles from bot origin to entity...
         const float angleToEntity = isInFOV (dest - getEyesPos ());

         // facing it directly?
         if (angleToEntity <= 10.0f) {
            MDLL_Use (m_pickupItem, ent ());

            m_pickupItem = nullptr;
            m_pickupType = Pickup::None;
            m_buttonPushTime = game.time () + 3.0f;

            completeTask ();
         }
      }
      break;
   }
}
