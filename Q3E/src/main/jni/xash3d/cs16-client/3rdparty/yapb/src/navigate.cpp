//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_has_team_semiclip ("has_team_semiclip", "0", "When enabled, bots will not try to avoid teammates on their way. Assuming some of the semiclip plugins are in use.");
ConVar cv_graph_slope_height ("graph_slope_height", "24.0", "Determines the maximum slope height change between the current and next node to consider the current link as a jump link. Only for generated graphs.", true, 12.0f, 48.0f);

int Bot::findBestGoal () {
   if (m_isCreature) {
      if (!graph.m_terrorPoints.empty ()) {
         return graph.m_terrorPoints.random ();
      }

      if (!graph.m_goalPoints.empty ()) {
         return graph.m_goalPoints.random ();
      }
      return graph.random ();
   }

   // chooses a destination (goal) node for a bot
   if (m_team == Team::Terrorist && game.mapIs (MapFlags::Demolition)) {
      auto result = findBestGoalWhenBombAction ();

      if (graph.exists (result)) {
         return result;
      }
   }

   // path finding behavior depending on map type
   float offensive = 0.0f;
   float defensive = 0.0f;

   IntArray *offensiveNodes = nullptr;
   IntArray *defensiveNodes = nullptr;

   switch (m_team) {
   case Team::Terrorist:
      offensiveNodes = &graph.m_ctPoints;
      defensiveNodes = &graph.m_terrorPoints;
      break;

   case Team::CT:
   default:
      offensiveNodes = &graph.m_terrorPoints;
      defensiveNodes = &graph.m_ctPoints;
      break;
   }

   // terrorist carrying the C4?
   if (m_hasC4 || m_isVIP) {
      return findGoalPost (GoalTactic::Goal, defensiveNodes, offensiveNodes);
   }
   else if (m_team == Team::CT && m_hasHostage) {
      bool hasMoreHostagesAround = false;

      // try to search nearby-unused hostage, and if so, go to next goal
      if (bots.hasInterestingEntities ()) {
         const auto &interesting = bots.getInterestingEntities ();

         // search world for hostages
         for (const auto &ent : interesting) {
            if (!util.isHostageEntity (ent)) {
               continue;
            }
            bool hostageInUse = false;

            // do not stole from bots (ignore humans, fuck them)
            for (const auto &other : bots) {
               if (!other->m_isAlive) {
                  continue;
               }

               for (const auto &hostage : other->m_hostages) {
                  if (hostage == ent) {
                     hostageInUse = true;
                     break;
                  }
               }
            }

            // in-use, skip
            if (hostageInUse) {
               continue;
            }
            const auto &origin = game.getEntityOrigin (ent);

            // too far, go to rescue point
            if (origin.distanceSq2d (pev->origin) > cr::sqrf (1024.0f)) {
               continue;
            }
            hasMoreHostagesAround = true;
            break;
         }
      }
      return findGoalPost (hasMoreHostagesAround ? GoalTactic::Goal : GoalTactic::RescueHostage, defensiveNodes, offensiveNodes);
   }
   constexpr float kBehaviorBase = 30.0f;
   const auto difficulty = static_cast <float> (m_difficulty);

   offensive = m_agressionLevel * 100.0f;
   defensive = m_fearLevel * 100.0f;

   if (game.mapIs (MapFlags::Assassination | MapFlags::HostageRescue)) {
      if (m_team == Team::Terrorist) {
         if (m_personality == Personality::Rusher) {
            defensive -= kBehaviorBase - difficulty * 0.5f;
            offensive += kBehaviorBase + difficulty * 5.0f;
         }
         else if (m_personality == Personality::Normal && rg.chance (40)) {
            defensive -= kBehaviorBase;
            offensive += kBehaviorBase;
         }
         else {
            defensive += kBehaviorBase;
            offensive -= kBehaviorBase;
         }
      }
      else if (m_team == Team::CT) {
         // on hostage maps force more bots to save hostages
         if (game.mapIs (MapFlags::HostageRescue)) {
            defensive -= kBehaviorBase - difficulty * 0.5f;
            offensive += kBehaviorBase + difficulty * 5.0f;
         }
         else {
            defensive -= kBehaviorBase;
            offensive += kBehaviorBase;
         }
      }
   }
   else if (game.mapIs (MapFlags::Demolition) && m_team == Team::CT) {
      if (bots.isBombPlanted () && getCurrentTaskId () != Task::EscapeFromBomb && !graph.getBombOrigin ().empty ()) {

         if (bots.hasBombSay (BombPlantedSay::ChatSay)) {
            pushChatMessage (Chat::Plant);
            bots.clearBombSay (BombPlantedSay::ChatSay);
         }
         return m_chosenGoalIndex = findBombNode ();
      }
      defensive += kBehaviorBase + difficulty * 5.0f;
      offensive -= kBehaviorBase - difficulty * 0.5f;

      if (m_personality != Personality::Rusher) {
         defensive += 10.0f;
      }
   }
   else if (game.mapIs (MapFlags::Demolition) && m_team == Team::Terrorist && bots.getRoundStartTime () + 10.0f < game.time ()) {
      // send some terrorists to guard planted bomb
      if (!m_defendedBomb && bots.isBombPlanted () && getCurrentTaskId () != Task::EscapeFromBomb && getBombTimeleft () >= 15.0f) {
         return m_chosenGoalIndex = findDefendNode (graph.getBombOrigin ());
      }
   }
   else if (game.mapIs (MapFlags::Escape)) {
      if (m_team == Team::Terrorist) {
         offensive += kBehaviorBase + difficulty * 5.0f;
         defensive -= kBehaviorBase - difficulty * 0.5f;
      }
      else if (m_team == Team::CT) {
         offensive -= kBehaviorBase - difficulty * 5.0f;
         defensive += kBehaviorBase + difficulty * 0.5f;
      }
   }

   const float goalDesire = rg (0.0f, 100.0f) + offensive;
   const float forwardDesire = rg (0.0f, 100.0f) + offensive;
   const float backoffDesire = rg (0.0f, 100.0f) + defensive;

   float campDesire = rg (0.0f, 100.0f) + defensive;

   if (!usesCampGun ()) {
      campDesire *= 0.5f;
   }

   int tactic = GoalTactic::Defensive;
   float tacticChoice = backoffDesire;

   if (campDesire > tacticChoice) {
      tacticChoice = campDesire;
      tactic = GoalTactic::Camp;
   }

   if (forwardDesire > tacticChoice) {
      tacticChoice = forwardDesire;
      tactic = GoalTactic::Offensive;
   }

   if (goalDesire > tacticChoice) {
      tacticChoice = goalDesire;
      tactic = GoalTactic::Goal;
   }
   return findGoalPost (tactic, defensiveNodes, offensiveNodes);
}

int Bot::findBestGoalWhenBombAction () {
   int result = kInvalidNodeIndex;

   if (!bots.isBombPlanted ()) {
      game.searchEntities ("classname", "weaponbox", [&] (edict_t *ent) {
         if (util.isModel (ent, "backpack.mdl")) {
            result = graph.getNearest (game.getEntityOrigin (ent));

            if (graph.exists (result)) {

               // if bomb entity is bot's ignore list, clear ignore list
               if (isIgnoredItem (ent)) {
                  m_ignoredItems.clear ();
               }
               return EntitySearchResult::Break;
            }
         }
         return EntitySearchResult::Continue;
      });

      // found one ?
      if (graph.exists (result)) {
         return m_loosedBombNodeIndex = result;
      }

      // forcing terrorist bot to not move to another bomb spot
      if (m_inBombZone && !m_hasProgressBar && m_hasC4) {
         return graph.getNearest (pev->origin, 1024.0f, NodeFlag::Goal);
      }
   }
   else if (!m_defendedBomb) {
      const auto &bombOrigin = graph.getBombOrigin ();

      if (!bombOrigin.empty ()) {
         m_defendedBomb = true;

         result = findDefendNode (bombOrigin);
         const auto &path = graph[result];

         const float bombTimer = mp_c4timer.as <float> ();
         const float timeMidBlowup = bots.getTimeBombPlanted () + (bombTimer * 0.5f + bombTimer * 0.25f) - graph.calculateTravelTime (pev->maxspeed, pev->origin, path.origin);

         if (timeMidBlowup > game.time ()) {
            clearTask (Task::MoveToPosition); // remove any move tasks

            startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, timeMidBlowup, true); // push camp task on to stack
            startTask (Task::MoveToPosition, TaskPri::MoveToPosition, result, timeMidBlowup, true); // push  move command

            // decide to duck or not to duck
            selectCampButtons (result);

            if (rg.chance (90)) {
               pushChatterMessage (Chatter::DefendingBombsite);
            }
         }
         else {
            pushRadioMessage (Radio::ShesGonnaBlow); // issue an additional radio message
         }
         return result;
      }
   }
   return result;
}

int Bot::findGoalPost (int tactic, IntArray *defensive, IntArray *offensive) {
   int goalChoices[4] {};

   for (int i = 0; i < 4; ++i) {
      goalChoices[i] = kInvalidNodeIndex;
   }

   if (tactic == GoalTactic::Defensive && !(*defensive).empty ()) { // careful goal
      postProcessGoals (*defensive, goalChoices);
   }
   else if (tactic == GoalTactic::Camp && !graph.m_campPoints.empty ()) {  // camp node goal
      // pickup sniper points if possible for sniping bots
      if (!graph.m_sniperPoints.empty () && usesSniper ()) {
         postProcessGoals (graph.m_sniperPoints, goalChoices);
      }
      else {
         postProcessGoals (graph.m_campPoints, goalChoices);
      }
   }
   else if (tactic == GoalTactic::Offensive && !(*offensive).empty ()) { // offensive goal
      postProcessGoals (*offensive, goalChoices);
   }
   else if (tactic == GoalTactic::Goal && !graph.m_goalPoints.empty ()) // map goal node
   {
      // force bomber to select closest goal, if round-start goal was reset by something
      if (m_hasC4 && bots.getRoundStartTime () + 20.0f < game.time ()) {
         float nearestDistanceSq = kInfiniteDistance;
         int count = 0;

         for (const auto &point : graph.m_goalPoints) {
            const float distanceSq = graph[point].origin.distanceSq (pev->origin);

            if (distanceSq > cr::sqrf (1024.0f)) {
               continue;
            }
            if (distanceSq < nearestDistanceSq) {
               goalChoices[count] = point;

               if (++count > 3) {
                  count = 0;
               }
               nearestDistanceSq = distanceSq;
            }
         }

         for (auto &choice : goalChoices) {
            if (choice == kInvalidNodeIndex) {
               choice = graph.m_goalPoints.random ();
            }
         }
      }
      else {
         postProcessGoals (graph.m_goalPoints, goalChoices);
      }
   }
   else if (tactic == GoalTactic::RescueHostage && !graph.m_rescuePoints.empty ()) {
      // force ct with hostage(s) to select closest rescue goal
      float nearestDistanceSq = kInfiniteDistance;
      int count = 0;

      for (const auto &point : graph.m_rescuePoints) {
         const float distanceSq = graph[point].origin.distanceSq (pev->origin);

         if (distanceSq < nearestDistanceSq) {
            goalChoices[count] = point;

            if (++count > 3) {
               count = 0;
            }
            nearestDistanceSq = distanceSq;
         }
      }

      for (auto &choice : goalChoices) {
         if (choice == kInvalidNodeIndex) {
            choice = graph.m_rescuePoints.random ();
         }
      }
   }
   ensureCurrentNodeIndex ();

   // rusher bots does not care any danger (idea from pbmm)
   if (m_personality == Personality::Rusher) {
      const auto randomGoal = goalChoices[rg (0, 3)];

      if (graph.exists (randomGoal)) {
         return m_chosenGoalIndex = randomGoal;
      }
   }
   bool sorting = false;

   do {
      sorting = false;

      for (int i = 0; i < 3; ++i) {
         if (goalChoices[i + 1] < 0) {
            break;
         }

         if (practice.getValue (m_team, m_currentNodeIndex, goalChoices[i]) < practice.getValue (m_team, m_currentNodeIndex, goalChoices[i + 1])) {
            cr::swap (goalChoices[i + 1], goalChoices[i]);
            sorting = true;
         }
      }
   } while (sorting);

   // the most worst case
   if (goalChoices[0] == kInvalidNodeIndex) {
      return m_chosenGoalIndex = graph.random ();
   }
   return m_chosenGoalIndex = goalChoices[0]; // return and store goal
}

void Bot::postProcessGoals (const IntArray &goals, int result[]) {
   // this function filters the goals, so new goal is not bot's old goal, and array of goals doesn't contains duplicate goals

   int recurseCount = 0;

   auto isRecentOrHistorical = [&] (int index) -> bool {
      if (m_prevGoalIndex == index || m_previousNodes[0] == index) {
         return true;
      }

      // check if historical goal
      for (const auto &hg : m_goalHist) {
         if (hg == index) {
            return true;
         }
      }

      for (size_t i = 0; i < 4; ++i) {
         if (result[i] == index) {
            return true;
         }
      }
      return isOccupiedNode (index, true);
   };

   static IntArray resulting {};
   resulting.clear ();

   // too less to choice from, this is usually waypointer mistake, number of def/offensive points count should be minimum of 4
   if (goals.length () < 4) {
      auto refill = [] (const IntArray &data) {
         if (!data.empty ()) {
            resulting.insert (0, data);
         }
      };

      // combine defensive and offensive and goals together to give a chance to proceed
      refill (graph.m_ctPoints);
      refill (graph.m_terrorPoints);
      refill (graph.m_goalPoints);
   }
   else {
      if (!goals.empty ()) {
         resulting.insert (0, goals);
      }
   }

   for (int index = 0; index < 4; ++index) {
      const auto goal = resulting.random ();

      if (recurseCount <= cr::max (4, resulting.length <int> ()) && isRecentOrHistorical (goal)) {
         if (index > 0) {
            index--;
         }
         ++recurseCount;
         continue;
      }
      result[index] = goal;
   }
}

bool Bot::hasActiveGoal () {
   const auto goal = getTask ()->data;

   if (goal == kInvalidNodeIndex) { // not decided about a goal
      return false;
   }
   else if (goal == m_currentNodeIndex) { // no nodes needed
      m_goalHist.push (goal);
      return true;
   }
   else if (m_pathWalk.empty ()) { // no path calculated
      return false;
   }
   return goal == m_pathWalk.last (); // got path - check if still valid
}

void Bot::resetCollision () {
   m_collideTime = 0.0f;
   m_probeTime = 0.0f;

   m_collisionProbeBits = 0;
   m_collisionState = CollisionState::Undecided;
   m_collStateIndex = 0;

   for (auto &collideMove : m_collideMoves) {
      collideMove = 0;
   }
}

void Bot::ignoreCollision () {
   resetCollision ();

   m_lastCollTime = game.time () + 0.5f;
   m_checkTerrain = false;
}

void Bot::doPlayerAvoidance (const Vector &normal) {
   if (isOnLadder () || cv_has_team_semiclip || game.is (GameFlags::FreeForAll)) {
      return; // no player avoiding when with semiclip plugin
   }
   m_hindrance = nullptr;
   float distanceSq = cr::sqrf (pev->maxspeed);

   auto clearCamp = [&] (edict_t *ent) {
      auto bot = bots[ent];

      if (bot) {
         const auto tid = bot->getCurrentTaskId ();
         const auto tid2 = getCurrentTaskId ();

         if ((m_currentNodeIndex == bot->m_currentNodeIndex)
            && (tid == Task::Camp || tid == Task::Hide || tid == Task::Pause)) {
            bot->completeTask ();
         }
         if ((m_currentNodeIndex == bot->m_currentNodeIndex)
            && (tid2 == Task::Camp || tid2 == Task::Hide || tid2 == Task::Pause)) {
            completeTask ();
            findValidNode ();
         }
      }
   };
   const auto ownPrio = bots.getPlayerPriority (ent ());

   // find nearest player to bot
   for (const auto &client : util.getClients ()) {
      if (!(client.flags & ClientFlags::Used) || !(client.flags & ClientFlags::Alive) || client.team != m_team || client.ent == ent ()) {
         continue;
      }
      const auto otherPrio = bots.getPlayerPriority (client.ent);

      // give some priorities to bot avoidance
      if (ownPrio < otherPrio) {
         clearCamp (client.ent);
         continue;
      }

      // they are higher priority - make way, unless we're already making way for someone more important
      if (!game.isNullEntity (m_hindrance) && m_hindrance != client.ent) {
         const auto avoidPrio = bots.getPlayerPriority (m_hindrance);

         // ignore because we're already avoiding someone better
         if (avoidPrio < otherPrio) {
            clearCamp (m_hindrance);
            continue;
         }
      }
      const auto nearestDistanceSq = client.ent->v.origin.distanceSq (pev->origin);

      if (nearestDistanceSq < cr::sqrf (pev->maxspeed) && nearestDistanceSq < distanceSq) {
         m_hindrance = client.ent;
         distanceSq = nearestDistanceSq;
      }
   }

   // found somebody?
   if (game.isNullEntity (m_hindrance)) {
      return;
   }

   // if we're stuck with a hindrance, probably we're in bad place, find path to single goal for both bots
   if (m_isStuck) {
      auto other = bots[m_hindrance];

      if (other != nullptr) {
         m_prevGoalIndex = other->m_prevGoalIndex;
         m_chosenGoalIndex = other->m_chosenGoalIndex;

         auto destIndex = m_chosenGoalIndex;

         if (!graph.exists (destIndex)) {
            destIndex = m_prevGoalIndex;
         }

         if (graph.exists (destIndex)) {
            findPath (m_currentNodeIndex, destIndex, other->m_pathType);
            other->findPath (m_currentNodeIndex, destIndex);
         }
      }
   }
   const float interval = m_frameInterval * (!isDucking () && pev->velocity.lengthSq2d () > 0.0f ? 6.0f : 2.0f);

   // use our movement angles, try to predict where we should be next frame
   Vector right {}, forward {};
   m_moveAngles.angleVectors (&forward, &right, nullptr);

   Vector predict = pev->origin + forward * pev->maxspeed * interval;

   predict += right * m_strafeSpeed * interval;
   predict += pev->velocity * interval;

   const auto movedDistanceSq = m_hindrance->v.origin.distanceSq (predict);
   const auto nextFrameDistanceSq = pev->origin.distanceSq (m_hindrance->v.origin + m_hindrance->v.velocity * interval);

   // is player that near now or in future that we need to steer away?
   if (movedDistanceSq <= cr::sqrf (64.0f) || (distanceSq <= cr::sqrf (96.0f) && nextFrameDistanceSq < distanceSq)) {
      const auto &dir = (pev->origin - m_hindrance->v.origin).normalize2d_apx ();

      // to start strafing, we have to first figure out if the target is on the left side or right side
      if ((dir | right.normalize2d_apx ()) > 0.0f) {
         // start strafing
         setStrafeSpeed (normal, pev->maxspeed);
      }
      else {
         // start strafing
         setStrafeSpeed (normal, -pev->maxspeed);
      }

      if (distanceSq < cr::sqrf (96.0f)) {
         if ((dir | forward.normalize2d_apx ()) < 0.0f) {
            m_moveSpeed = -pev->maxspeed;
         }
      }
   }
}

void Bot::checkTerrain (float movedDistance, const Vector &dirNormal) {

   // if avoiding someone do not consider stuck
   TraceResult tr {};
   m_isStuck = false;

   const auto tid = getCurrentTaskId ();

   // minimal speed for consider stuck
   const float minimalSpeed = isDucking () ? kMinMovedDistance : kMinMovedDistance * 4;

   // standing still, no need to check?
   if ((cr::abs (m_moveSpeed) >= minimalSpeed || cr::abs (m_strafeSpeed) >= minimalSpeed)
      && m_lastCollTime < game.time ()
      && tid != Task::Attack
      && tid != Task::Camp) {

      // special case for creatures
      if (m_lastDamageTimestamp >= game.time () && m_isCreature) {
         m_lastCollTime = m_lastDamageTimestamp + 0.2f;
         m_firstCollideTime = 0.0f;
      }
      // didn't we move enough previously?
      else if (movedDistance < kMinMovedDistance && m_prevSpeed > 20.0f) {
         m_prevTime = game.time (); // then consider being stuck
         m_isStuck = true;

         if (cr::fzero (m_firstCollideTime)) {
            m_firstCollideTime = game.time () + 0.2f;
         }
      }
      // not stuck yet
      else {
         bool isOnLadderPath = false;

         if ((m_pathFlags & NodeFlag::Ladder) && isPreviousLadder ()) {
            isOnLadderPath = true;
         }

         // test if there's something ahead blocking the way
         if (!isOnLadderPath && !isOnLadder () && isBlockedForward (dirNormal, &tr)) {
            if (cr::fzero (m_firstCollideTime)) {
               m_firstCollideTime = game.time () + 0.2f;
            }
            else if (m_firstCollideTime <= game.time ()) {
               m_isStuck = true;
            }
         }
         else {
            m_firstCollideTime = 0.0f;
         }
      }

      // not stuck?
      if (!m_isStuck) {
         if (m_probeTime + rg (0.75f, 1.15f) < game.time ()) {
            resetCollision (); // reset collision memory if not being stuck for 0.5 secs
         }
         else {
            // remember to keep pressing stuff if it was necessary ago
            if (m_collideMoves[m_collStateIndex] == CollisionState::Duck && (isOnFloor () || isInWater ())) {
               pev->button |= IN_DUCK;
            }
            else if (m_collideMoves[m_collStateIndex] == CollisionState::StrafeLeft) {
               setStrafeSpeed (dirNormal, -pev->maxspeed);
            }
            else if (m_collideMoves[m_collStateIndex] == CollisionState::StrafeRight) {
               setStrafeSpeed (dirNormal, pev->maxspeed);
            }
         }
         return;
      }

      // bot is stuck, but not yet decided what to do?
      if (m_collisionState == CollisionState::Undecided) {
         uint32_t bits = 0;

         if (isOnLadder ()) {
            bits |= CollisionProbe::Strafe;
         }
         else {
            bits |= (CollisionProbe::Strafe | CollisionProbe::Jump);
         }

         // try to duck when graph analyzed
         if (graph.isAnalyzed ()) {
            bits |= CollisionProbe::Duck;
         }

         // collision check allowed if not flying through the air
         if (isOnFloor () || isOnLadder () || isInWater ()) {
            uint32_t state[kMaxCollideMoves * 2 + 1] {};
            int i = 0;

            Vector src {}, dst {};

            // first 4 entries hold the possible collision states
            state[i++] = CollisionState::Jump;
            state[i++] = CollisionState::StrafeLeft;
            state[i++] = CollisionState::StrafeRight;
            state[i++] = CollisionState::Duck;

            // now weight all possible states
            if (bits & CollisionProbe::Jump) {
               state[i] = 0;

               if (canJumpUp (dirNormal)) {
                  state[i] += 10;
               }

               if (m_destOrigin.z >= pev->origin.z + 18.0f) {
                  state[i] += 5;
               }

               if (seesEntity (m_destOrigin)) {
                  const auto &right = m_moveAngles.right ();

                  src = getEyesPos ();
                  src = src + right * 15.0f;

                  game.testLine (src, m_destOrigin, TraceIgnore::Everything, ent (), &tr);

                  if (tr.flFraction >= 1.0f) {
                     src = getEyesPos ();
                     src = src - right * 15.0f;

                     game.testLine (src, m_destOrigin, TraceIgnore::Everything, ent (), &tr);

                     if (tr.flFraction >= 1.0f) {
                        state[i] += 5;
                     }
                  }
               }
               if (isDucking ()) {
                  src = pev->origin;
               }
               else {
                  src = pev->origin + Vector (0.0f, 0.0f, -17.0f);
               }
               dst = src + dirNormal * 30.0f;
               game.testLine (src, dst, TraceIgnore::Everything, ent (), &tr);

               if (!cr::fequal (tr.flFraction, 1.0f)) {
                  state[i] += 10;
               }
            }
            else {
               state[i] = 0;
            }
            ++i;


            if (bits & CollisionProbe::Strafe) {
               state[i] = 0;
               state[i + 1] = 0;

               // to start strafing, we have to first figure out if the target is on the left side or right side
               Vector right {}, forward {};
               m_moveAngles.angleVectors (&forward, &right, nullptr);

               const Vector &dirToPoint = (pev->origin - m_destOrigin).normalize2d_apx ();
               const Vector &rightSide = right.normalize2d_apx ();

               bool dirRight = false;
               bool dirLeft = false;
               bool blockedLeft = false;
               bool blockedRight = false;

               if ((dirToPoint | rightSide) > 0.0f) {
                  dirRight = true;
               }
               else {
                  dirLeft = true;
               }
               const auto &testDir = m_moveSpeed > 0.0f ? forward : -forward;
               constexpr float kBlockDistance = 32.0f;

               // now check which side is blocked
               src = pev->origin + right * kBlockDistance;
               dst = src + testDir * kBlockDistance;

               game.testHull (src, dst, TraceIgnore::Monsters, head_hull, ent (), &tr);

               if (!cr::fequal (tr.flFraction, 1.0f)) {
                  blockedRight = true;
               }
               src = pev->origin - right * kBlockDistance;
               dst = src + testDir * kBlockDistance;

               game.testHull (src, dst, TraceIgnore::Monsters, head_hull, ent (), &tr);

               if (!cr::fequal (tr.flFraction, 1.0f)) {
                  blockedLeft = true;
               }

               if (dirLeft) {
                  state[i] += 5;
               }
               else {
                  state[i] -= 5;
               }

               if (blockedLeft) {
                  state[i] -= 5;
               }
               ++i;

               if (dirRight) {
                  state[i] += 5;
               }
               else {
                  state[i] -= 5;
               }

               if (blockedRight) {
                  state[i] -= 5;
               }
            }

            if (bits & CollisionProbe::Duck) {
               state[i] = 0;

               if (canDuckUnder (dirNormal)) {
                  state[i] += 10;
               }

               if ((m_destOrigin.z + 36.0f <= pev->origin.z) && seesEntity (m_destOrigin)) {
                  state[i] += 5;
               }
            }
            else {
               state[i] = 0;
            }
            ++i;

            // weighted all possible moves, now sort them to start with most probable
            bool sorting = false;

            do {
               sorting = false;

               for (i = 0; i < 3; ++i) {
                  if (state[i + kMaxCollideMoves] < state[i + kMaxCollideMoves + 1]) {
                     cr::swap (state[i], state[i + 1]);
                     cr::swap (state[i + kMaxCollideMoves], state[i + kMaxCollideMoves + 1]);

                     sorting = true;
                  }
               }
            } while (sorting);

            for (int j = 0; j < kMaxCollideMoves; ++j) {
               m_collideMoves[j] = state[j];
            }

            m_collideTime = game.time ();
            m_probeTime = game.time () + 0.5f;
            m_collisionProbeBits = bits;
            m_collisionState = CollisionState::Probing;
            m_collStateIndex = 0;
         }
      }

      if (m_collisionState == CollisionState::Probing) {
         if (m_probeTime < game.time ()) {
            m_collStateIndex++;
            m_probeTime = game.time () + 0.5f;

            if (m_collStateIndex >= kMaxCollideMoves) {
               m_navTimeset = game.time () - 5.0f;
               resetCollision ();
            }
         }

         if (m_collStateIndex < kMaxCollideMoves) {
            switch (m_collideMoves[m_collStateIndex]) {
            case CollisionState::Jump:
               if ((isOnFloor () || isInWater ()) && !isOnLadder ()) {
                  pev->button |= IN_JUMP;
               }
               break;

            case CollisionState::Duck:
               if (isOnFloor () || isInWater ()) {
                  pev->button |= IN_DUCK;
               }
               break;

            case CollisionState::StrafeLeft:
               setStrafeSpeed (dirNormal, -pev->maxspeed);
               break;

            case CollisionState::StrafeRight:
               setStrafeSpeed (dirNormal, pev->maxspeed);
               break;
            }
         }
      }
   }
}

void Bot::checkFall () {
   if (isPreviousLadder ()) {
      return;
   }
   else if (graph.exists (m_currentNodeIndex)) {
      if (graph[m_currentNodeIndex].flags & NodeFlag::Ladder) {
         return;
      }
   }

   if (!m_checkFall) {
      if (isOnFloor ()) {
         m_checkFallPoint[0] = pev->origin;

         if (!game.isNullEntity (m_enemy)) {
            m_checkFallPoint[1] = game.getEntityOrigin (m_enemy);
         }
         else if (m_currentNodeIndex != kInvalidNodeIndex) {
            m_checkFallPoint[1] = m_pathOrigin;
         }
         else {
            m_checkFallPoint[1].clear ();
         }
      }
      else if (!isOnLadder () && !isInWater ()) {
         if (!m_checkFallPoint[0].empty () && !m_checkFallPoint[1].empty ()) {
            m_checkFall = true;
         }
      }
   }

   if (!m_checkFall || !isOnFloor () || !m_fixFallTimer.elapsed ()) {
      return;
   }
   m_checkFall = false;
   bool fixFall = false;

   const float baseDistanceSq = m_checkFallPoint[0].distanceSq (m_checkFallPoint[1]);
   const float nowDistanceSq = pev->origin.distanceSq (m_checkFallPoint[1]);

   if (nowDistanceSq > baseDistanceSq
      && (nowDistanceSq > baseDistanceSq * 1.8f || nowDistanceSq > baseDistanceSq + 260.0f)
      && baseDistanceSq >= cr::sqrf (124.0f) && nowDistanceSq >= cr::sqrf (146.0f)) {
      fixFall = true;
   }
   else if (cr::abs (m_checkFallPoint[1].z) > cr::abs (pev->origin.z) + 138.0f
      || cr::abs (m_checkFallPoint[0].z) > cr::abs (pev->origin.z) + 138.0f) {
      fixFall = true;
   }
   else if (m_currentNodeIndex != kInvalidNodeIndex
      && nowDistanceSq > cr::sqrf (32.0f)
      && cr::abs (m_checkFallPoint[1].z) > cr::abs (pev->origin.z) + 72.0f) {
      fixFall = true;
   }

   if (fixFall) {
      m_currentNodeIndex = kInvalidNodeIndex;
      findValidNode ();

      m_fixFallTimer.start (1.0f);
   }
}

void Bot::moveToGoal () {
   findValidNode ();

   // press duck button if we need to
   if (m_pathFlags & NodeFlag::Crouch) {
      bool pressDuck = true;

      if (m_pathFlags & (NodeFlag::Camp | NodeFlag::Goal)) {
         TraceResult tr {};

         auto src = m_path->origin;
         auto dst = m_path->origin;

         src.z += 12.0f;
         dst.z += 18.0f + 28.0f;

         game.testLine (src, dst, TraceIgnore::Everything, ent (), &tr);

         if (tr.flFraction >= 0.95f) {
            pressDuck = false;
         }
      }

      // press duck if not canceled by visibility count check only and it's end of the route
      if (pressDuck) {
         pev->button |= IN_DUCK;
      }
   }

   // press jump button if we need to leave the ladder
   if (!(m_pathFlags & NodeFlag::Ladder)
      && isPreviousLadder ()
      && isOnFloor ()
      && isOnLadder ()
      && m_moveSpeed > 50.0f
      && pev->velocity.lengthSq () < 50.0f) {

      pev->button |= IN_JUMP;
      m_jumpTime = game.time () + 1.0f;
   }
   const auto distanceSq2d = m_destOrigin.distanceSq2d (pev->origin + pev->velocity * m_frameInterval);

   if (distanceSq2d < cr::sqrf (m_moveSpeed) * m_frameInterval && getTask ()->data != kInvalidNodeIndex) {
      m_moveSpeed = distanceSq2d;
   }
   if (m_moveSpeed > pev->maxspeed) {
      m_moveSpeed = pev->maxspeed;
   }
   m_lastUsedNodesTime = game.time ();

   // special movement for swimming here
   if (isInWater ()) {
      // check if we need to go forward or back press the correct buttons
      if (isInFOV (m_destOrigin - getEyesPos ()) > 90.0f) {
         pev->button |= IN_BACK;
      }
      else {
         pev->button |= IN_FORWARD;
      }

      if (m_moveAngles.x > 60.0f) {
         pev->button |= IN_DUCK;
      }
      else if (m_moveAngles.x < -60.0f) {
         pev->button |= IN_JUMP;
      }
   }
}

void Bot::resetMovement () {
   pev->button = 0;

   m_moveSpeed = 0.0f;
   m_strafeSpeed = 0.0f;
   m_moveAngles.clear ();
}

void Bot::translateInput () {
   if (m_duckTime >= game.time ()) {
      pev->button |= IN_DUCK;
   }

   if (pev->button & IN_JUMP) {
      m_jumpTime = game.time ();
   }

   if (m_jumpTime + 0.85f > game.time ()) {
      if (!isOnFloor () && !isInWater () && !isOnLadder ()) {
         pev->button |= IN_DUCK;
      }
   }

   if (!(pev->button & (IN_FORWARD | IN_BACK))) {
      if (m_moveSpeed > 0.0f) {
         pev->button |= IN_FORWARD;
      }
      else if (m_moveSpeed < 0.0f) {
         pev->button |= IN_BACK;
      }
   }

   if (!(pev->button & (IN_MOVELEFT | IN_MOVERIGHT))) {
      if (m_strafeSpeed > 0.0f) {
         pev->button |= IN_MOVERIGHT;
      }
      else if (m_strafeSpeed < 0.0f) {
         pev->button |= IN_MOVELEFT;
      }
   }
}

bool Bot::updateNavigation () {
   // this function is a main path navigation

   // check if we need to find a node...
   if (m_currentNodeIndex == kInvalidNodeIndex) {
      findValidNode ();
      setPathOrigin ();

      m_navTimeset = game.time ();
   }
   m_destOrigin = m_pathOrigin;

   // this node has additional travel flags - care about them
   if (m_currentTravelFlags & PathFlag::Jump) {

      // bot is not jumped yet?
      if (!m_jumpFinished) {

         // if bot's on the ground or on the ladder we're free to jump. actually setting the correct velocity is cheating.
         // pressing the jump button gives the illusion of the bot actual jumping.
         if (isOnFloor () || isOnLadder ()) {
            if (m_desiredVelocity.length2d () > 0.0f) {
               pev->velocity = m_desiredVelocity;
            }
            else {
               auto feet = pev->origin + pev->mins;
               auto node = Vector { m_pathOrigin.x, m_pathOrigin.y, m_pathOrigin.z - ((m_pathFlags & NodeFlag::Crouch) ? 18.0f : 36.0f) };

               if (feet.z > pev->origin.z) {
                  feet = pev->origin + pev->maxs;
               }
               feet = { pev->origin.x, pev->origin.y, feet.z };

               // calculate like we do with grenades
               auto velocity = calcThrow (feet, node);

               if (velocity.lengthSq () < 100.0f) {
                  velocity = calcToss (feet, node);
               }
               velocity = velocity + velocity * 0.45f;

               // set the bot "grenade" velocity
               if (velocity.length2d () > 0.0f) {
                  pev->velocity = velocity;
                  pev->velocity.z = 0.0f;
               }
               else {
                  pev->velocity = pev->velocity + pev->velocity * m_frameInterval * 2.0f;
                  pev->velocity.z = 0.0f;
               }
            }
            pev->button |= IN_JUMP;

            m_jumpFinished = true;
            m_checkTerrain = false;
            m_desiredVelocity.clear ();

            // cool down a little if next path after current will be jump
            if (m_jumpSequence) {
               startTask (Task::Pause, TaskPri::Pause, kInvalidNodeIndex, game.time () + rg (0.75f, 1.2f) + m_frameInterval, false);
               m_jumpSequence = false;
            }
         }
      }
      else if (!cv_jasonmode && usesKnife () && isOnFloor () && getCurrentTaskId () != Task::EscapeFromBomb) {
         selectBestWeapon ();
      }
   }

   if (m_pathFlags & NodeFlag::Ladder) {
      constexpr auto kLadderOffset = Vector (0.0f, 0.0f, 36.0f);

      const auto prevNodeIndex = m_previousNodes[0];
      const float ladderDistance = pev->origin.distance (m_pathOrigin);

      // do a precise movement when very near
      if (graph.exists (prevNodeIndex) && !(graph[prevNodeIndex].flags & NodeFlag::Ladder) && ladderDistance < 64.0f) {
         if (m_pathOrigin.z >= pev->origin.z + 16.0f) {
            m_pathOrigin = m_path->origin + kLadderOffset;
         }
         else if (m_pathOrigin.z < pev->origin.z - 16.0f) {
            m_pathOrigin = m_path->origin - kLadderOffset;
         }

         if (!isDucking ()) {
            m_moveSpeed = pev->maxspeed * 0.4f;
         }

         // do not duck while not on ladder
         if (!isOnLadder ()) {
            pev->button &= ~IN_DUCK;
         }
         m_approachingLadderTimer.start (m_frameInterval * 6.0f);
      }

      if (!isOnLadder () && isOnFloor () && !isDucking ()) {
         if (!isPreviousLadder ()) {
            m_moveSpeed = ladderDistance;
         }

         if (m_moveSpeed < 150.0f) {
            m_moveSpeed = 150.0f;
         }
         else if (m_moveSpeed > pev->maxspeed) {
            m_moveSpeed = pev->maxspeed;
         }
      }

      // special detection if someone is using the ladder (to prevent to have bots-towers on ladders)
      for (const auto &client : util.getClients ()) {
         if (!(client.flags & ClientFlags::Used)
            || !(client.flags & ClientFlags::Alive)
            || client.team != m_team
            || client.ent == ent ()
            || !(client.ent->v.movetype == MOVETYPE_FLY)) {

            continue;
         }

         bool foundGround = false;

         if (!isPreviousLadder ()
            && client.ent->v.origin.distanceSq (pev->origin) < cr::sqrf (128.0f)
            && cr::abs (client.ent->v.origin.z - pev->origin.z) > 17.0f) {
            foundGround = true;
         }

         if (foundGround) {
            changeNodeIndex (prevNodeIndex);
         }
      }
   }

   // special lift handling (code merged from podbotmm)
   if (m_pathFlags & NodeFlag::Lift) {
      if (updateLiftHandling ()) {
         if (!updateLiftStates ()) {
            return false;
         }
      }
      else {
         return false;
      }
   }
   TraceResult tr {};

   // check if we are going through a door...
   if (game.mapIs (MapFlags::HasDoors) || (m_pathFlags & NodeFlag::Button)) {
      game.testLine (pev->origin, m_pathOrigin, TraceIgnore::Monsters, ent (), &tr);

      if (!game.isNullEntity (tr.pHit) && game.isNullEntity (m_liftEntity) && util.isDoorEntity (tr.pHit)) {
         const auto &origin = game.getEntityOrigin (tr.pHit);
         const float distanceSq = pev->origin.distanceSq (origin);

         // if the door is near enough...
         if (distanceSq < cr::sqrf (56.0f)) {
            ignoreCollision (); // don't consider being stuck

            // also 'use' the door randomly
            if (rg.chance (50)) {
               // do not use door directly under xash, or we will get failed assert in gamedll code
               if (game.is (GameFlags::Xash3D)) {
                  pev->button |= IN_USE;
               }
               else {
                  MDLL_Use (tr.pHit, ent ());
               }
               m_buttonPushTime = game.time () + 1.5f;
            }
         }

         // make sure we are always facing the door when going through it
         m_aimFlags &= ~(AimFlags::LastEnemy | AimFlags::PredictPath);
         m_canChooseAimDirection = false;

         // delay task
         if (m_buttonPushTime < game.time ()) {
            auto button = lookupButton (tr.pHit->v.targetname.chars (), true);

            // check if we got valid button
            if (!game.isNullEntity (button)) {
               m_pickupItem = button;
               m_pickupType = Pickup::Button;

               m_navTimeset = game.time ();
            }
         }

         // if bot hits the door, then it opens, so wait a bit to let it open safely
         if (pev->velocity.lengthSq2d () < cr::sqrf (10.0f) && m_timeDoorOpen < game.time ()) {
            if (m_timeHitDoor >= game.time ()) {
               startTask (Task::Pause, TaskPri::Pause, kInvalidNodeIndex, game.time () + 0.5f, false);
            }
            m_timeDoorOpen = game.time () + 1.0f; // retry in 1 sec until door is open

            ++m_tryOpenDoor;

            if (m_tryOpenDoor > 1 && m_tryOpenDoor < 4) {
               edict_t *nearest = nullptr;

               // try to find nearest enemy (maybe behind a door)
               util.findNearestPlayer (reinterpret_cast <void **> (&nearest), ent (), 256.0f, false, false, true, true, false);

               // check if enemy is penetrable
               if (util.isAlive (nearest) && isPenetrableObstacle (nearest->v.origin) && !cv_ignore_enemies) {
                  m_seeEnemyTime = game.time ();

                  m_states |= Sense::SeeingEnemy | Sense::SuspectEnemy;
                  m_aimFlags |= AimFlags::Enemy;

                  m_enemy = nearest;
                  m_lastEnemyOrigin = nearest->v.origin;

                  m_tryOpenDoor = 0;
               }
            }
            else if (m_tryOpenDoor > 4) {
               m_tryOpenDoor = 0;

               if (rg.chance (50)) {
                  const auto prevNode = m_previousNodes[0];

                  // go back to prev node, if blocked for long time
                  if (graph.exists (prevNode)) {
                     changeNodeIndex (prevNode);
                  }
               }
               else {
                  const auto &dirToPoint = (pev->origin - origin).normalize2d_apx ();
                  const auto &forwardMove = m_moveAngles.forward ().normalize2d_apx ();

                  if (distanceSq < cr::sqrf (80.0f)) {
                     if ((dirToPoint | forwardMove) < 0.0f && !checkWallOnBehind ()) {
                        m_moveSpeed = -pev->maxspeed;
                     }
                  }
               }
               m_timeHitDoor = game.time () + 3.0f;
            }
         }
      }
   }

   float desiredDistanceSq = cr::sqrf (8.0f);
   float nodeDistanceSq = pev->origin.distanceSq (m_pathOrigin);

   // initialize the radius for a special node type, where the node is considered to be reached
   if (m_pathFlags & NodeFlag::Lift) {
      desiredDistanceSq = cr::sqrf (50.0f);
   }
   else if (isDucking () || (m_pathFlags & NodeFlag::Goal)) {
      desiredDistanceSq = cr::sqrf (9.0f);

      // on cs_ maps goals are usually hostages, so increase reachability distance for them, they (hostages) picked anyway
      if (game.mapIs (MapFlags::HostageRescue) && (m_pathFlags & NodeFlag::Goal)) {
         desiredDistanceSq = cr::sqrf (128.0f);
      }
   }
   else if (m_pathFlags & NodeFlag::Ladder) {
      desiredDistanceSq = cr::sqrf (16.0f);
   }
   else if (m_currentTravelFlags & PathFlag::Jump) {
      desiredDistanceSq = 0.0f;
   }
   else if (m_path->number == cv_debug_goal.as <int> ()) {
      desiredDistanceSq = 0.0f;
   }
   else {
      desiredDistanceSq = cr::max (cr::sqrf (m_path->radius), desiredDistanceSq);
   }
   bool pathHasFlags = false;

   // check if node has a special travel flags, so they need to be reached more precisely
   for (const auto &link : m_path->links) {
      if (link.flags != 0) {
         desiredDistanceSq = 0.0f;
         pathHasFlags = true;

         break;
      }
   }

   // make sure reach exactly, if just lost on path
   if (!m_lostReachableNodeTimer.elapsed ()) {
      desiredDistanceSq = 0.0f;
   }

   // if just recalculated path, assume reached current node
   if (!m_repathTimer.elapsed () && !pathHasFlags) {
      desiredDistanceSq = cr::sqrf (72.0f);
   }

   // needs precise placement - check if we get past the point
   if (desiredDistanceSq < cr::sqrf (20.0f) && nodeDistanceSq < cr::sqrf (30.0f)) {
      const auto predictRangeSq = m_pathOrigin.distanceSq (pev->origin + pev->velocity * m_frameInterval);

      if (predictRangeSq >= nodeDistanceSq || predictRangeSq <= desiredDistanceSq) {
         desiredDistanceSq = nodeDistanceSq + 1.0f;
      }
   }

   // this allows us to prevent stupid bot behavior when he reaches almost end point of this route, but some one  (other bot eg)
   // is sitting there, so the bot is unable to reach the node because of other player on it, and he starts to jumping and so on
   // here we're clearing task memory data (important!), since task executor may restart goal with one from memory, so this process
   // will go in cycle, and forcing bot to re-create new route.
   if (m_pathWalk.hasNext () && m_pathWalk.next () == m_pathWalk.last () && isOccupiedNode (m_pathWalk.next (), pathHasFlags)) {
      getTask ()->data = kInvalidNodeIndex;

      m_currentNodeIndex = kInvalidNodeIndex;
      m_chosenGoalIndex = kInvalidNodeIndex;

      return true;
   }

   if (nodeDistanceSq < desiredDistanceSq) {
      // did we reach a destination node?
      if (getTask ()->data == m_currentNodeIndex) {
         if (m_chosenGoalIndex != kInvalidNodeIndex) {
            constexpr int kMaxGoalValue = PracticeLimit::Goal;

            // add goal values
            int goalValue = practice.getValue (m_team, m_chosenGoalIndex, m_currentNodeIndex);
            const int addedValue = static_cast <int> (m_healthValue * 0.5f + m_goalValue * 0.5f);

            goalValue = cr::clamp (goalValue + addedValue, -kMaxGoalValue, kMaxGoalValue);

            // update the practice for team
            practice.setValue (m_team, m_chosenGoalIndex, m_currentNodeIndex, goalValue);
         }
         return true;
      }
      else if (m_pathWalk.empty ()) {
         return false;
      }
      const int taskTarget = getTask ()->data;

      if (game.mapIs (MapFlags::Demolition)
         && bots.isBombPlanted ()
         && m_team == Team::CT
         && getCurrentTaskId () != Task::EscapeFromBomb
         && taskTarget != kInvalidNodeIndex) {

         const auto &bombOrigin = isBombAudible ();

         // bot within 'hearable' bomb tick noises?
         if (!bombOrigin.empty ()) {
            const float distanceSq = bombOrigin.distanceSq (graph[taskTarget].origin);

            if (distanceSq > cr::sqrf (512.0f)) {
               if (rg.chance (50) && !graph.isVisited (taskTarget)) {
                  pushRadioMessage (Radio::SectorClear);
               }
               graph.setVisited (taskTarget); // doesn't hear so not a good goal
            }
         }
         else {
            if (rg.chance (50) && !graph.isVisited (taskTarget)) {
               pushRadioMessage (Radio::SectorClear);
            }
            graph.setVisited (taskTarget); // doesn't hear so not a good goal
         }
      }
      advanceMovement (); // do the actual movement checking
   }
   return false;
}

bool Bot::updateLiftHandling () {
   bool liftClosedDoorExists = false;

   // update node time set
   m_navTimeset = game.time ();

   TraceResult tr {};

   // wait for something about for lift
   auto wait = [&] () {
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;
      m_checkTerrain = false;

      m_navTimeset = game.time ();
      m_aimFlags |= AimFlags::Nav;

      pev->button &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);

      ignoreCollision ();
   };

   // need to wait?
   auto checkNeedToWait = [&] (float limitSq = 22.0f) {
      if (pev->origin.distanceSq (m_destOrigin) < cr::sqrf (limitSq)) {
         wait ();
      }
   };

   // trace line to door
   game.testLine (pev->origin, m_pathOrigin, TraceIgnore::Everything, ent (), &tr);

   if (tr.flFraction < 1.0f
      && util.isDoorEntity (tr.pHit)
      && (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside)
      && pev->groundentity != tr.pHit) {

      if (m_liftState == LiftState::None) {
         m_liftState = LiftState::LookingButtonOutside;
         m_liftUsageTime = game.time () + 7.0f;
      }
      liftClosedDoorExists = true;
   }

   // helper
   auto isFunc = [] (StringRef cls) -> bool {
      return cls.startsWith ("func_door") || cls == "func_plat" || cls == "func_train";
   };

   // trace line down
   game.testLine (m_path->origin, m_pathOrigin + Vector (0.0f, 0.0f, -50.0f), TraceIgnore::Everything, ent (), &tr);

   // if trace result shows us that it is a lift
   if (!game.isNullEntity (tr.pHit) && !m_pathWalk.empty () && isFunc (tr.pHit->v.classname.str ()) && !liftClosedDoorExists) {
      if ((m_liftState == LiftState::None
         || m_liftState == LiftState::WaitingFor
         || m_liftState == LiftState::LookingButtonOutside) && cr::fzero (tr.pHit->v.velocity.z)) {

         if (cr::abs (pev->origin.z - tr.vecEndPos.z) < 70.0f) {
            m_liftEntity = tr.pHit;
            m_liftState = LiftState::EnteringIn;
            m_liftTravelPos = m_pathOrigin;
            m_liftUsageTime = game.time () + 5.0f;
         }
      }
      else if (m_liftState == LiftState::TravelingBy) {
         m_liftState = LiftState::Leaving;
         m_liftUsageTime = game.time () + 7.0f;
      }
   }
   else if (!m_pathWalk.empty ()) { // no lift found at node
      if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor) && m_pathWalk.hasNext ()) {
         const auto nextNode = m_pathWalk.next ();

         if (graph.exists (nextNode) && (graph[nextNode].flags & NodeFlag::Lift)) {
            game.testLine (m_path->origin, graph[nextNode].origin, TraceIgnore::Everything, ent (), &tr);

            if (!game.isNullEntity (tr.pHit) && isFunc (tr.pHit->v.classname.str ())) {
               m_liftEntity = tr.pHit;
            }
         }
         m_liftState = LiftState::LookingButtonOutside;
         m_liftUsageTime = game.time () + 15.0f;
      }
   }

   // bot is going to enter the lift
   if (m_liftState == LiftState::EnteringIn) {
      m_destOrigin = m_liftTravelPos;

      // check if we enough to destination
      if (pev->origin.distanceSq (m_destOrigin) < cr::sqrf (22.0f)) {
         wait ();

         // need to wait our following teammate ?
         bool needWaitForTeammate = false;

         // if some bot is following a bot going into lift - he should take the same lift to go
         for (const auto &bot : bots) {
            if (!bot->m_isAlive || bot->m_team != m_team || bot->m_targetEntity != ent () || bot->getCurrentTaskId () != Task::FollowUser) {
               continue;
            }

            if (bot->pev->groundentity == m_liftEntity && bot->isOnFloor ()) {
               break;
            }

            bot->m_liftEntity = m_liftEntity;
            bot->m_liftState = LiftState::EnteringIn;
            bot->m_liftTravelPos = m_liftTravelPos;

            needWaitForTeammate = true;
         }

         if (needWaitForTeammate) {
            m_liftState = LiftState::WaitingForTeammates;
            m_liftUsageTime = game.time () + 8.0f;
         }
         else {
            m_liftState = LiftState::LookingButtonInside;
            m_liftUsageTime = game.time () + 10.0f;
         }
      }
   }

   // bot is waiting for his teammates
   if (m_liftState == LiftState::WaitingForTeammates) {
      // need to wait our following teammate ?
      bool needWaitForTeammate = false;

      for (const auto &bot : bots) {
         if (!bot->m_isAlive
            || bot->m_team != m_team
            || bot->m_targetEntity != ent ()
            || bot->getCurrentTaskId () != Task::FollowUser
            || bot->m_liftEntity != m_liftEntity) {

            continue;
         }

         if (bot->pev->groundentity == m_liftEntity || !bot->isOnFloor ()) {
            needWaitForTeammate = true;
            break;
         }
      }

      // need to wait for teammate
      if (needWaitForTeammate) {
         m_destOrigin = m_liftTravelPos;
         checkNeedToWait ();
      }

      // else we need to look for button
      if (!needWaitForTeammate || m_liftUsageTime < game.time ()) {
         m_liftState = LiftState::LookingButtonInside;
         m_liftUsageTime = game.time () + 10.0f;
      }
   }

   // bot is trying to find button inside a lift
   if (m_liftState == LiftState::LookingButtonInside) {
      m_checkTerrain = false;

      auto button = lookupButton (m_liftEntity->v.targetname.str (), true);

      // got a valid button entity ?
      if (!game.isNullEntity (button)
         && pev->groundentity == m_liftEntity
         && m_buttonPushTime + 1.0f < game.time ()
         && cr::fzero (m_liftEntity->v.velocity.z)
         && isOnFloor ()) {

         auto buttonWithLineOfSight = lookupButton (m_liftEntity->v.targetname.str (), false);

         if (!game.isNullEntity (buttonWithLineOfSight)) {
            m_pickupItem = buttonWithLineOfSight;
            m_pickupType = Pickup::Button;
         }
         else {
            MDLL_Use (button, ent ());
         }
         const auto prevNode = m_previousNodes[0];

         if (graph.exists (prevNode) && pev->origin.distanceSq2d (graph[prevNode].origin) < cr::sqrf (72.0f)) {
            wait ();
         }
      }
      ignoreCollision ();
   }

   // is lift activated and bot is standing on it and lift is moving ?
   if (m_liftState == LiftState::LookingButtonInside
      || m_liftState == LiftState::EnteringIn
      || m_liftState == LiftState::WaitingForTeammates
      || m_liftState == LiftState::WaitingFor) {

      const auto prevNode = m_previousNodes[0];

      if (pev->groundentity == m_liftEntity
         && !cr::fzero (m_liftEntity->v.velocity.z)
         && isOnFloor ()
         && ((graph.exists (prevNode) && (graph[prevNode].flags & NodeFlag::Lift))
            || !game.isNullEntity (m_targetEntity))) {

         m_liftState = LiftState::TravelingBy;
         m_liftUsageTime = game.time () + 14.0f;

         checkNeedToWait ();
      }
   }

   // bots is currently moving on lift
   if (m_liftState == LiftState::TravelingBy) {
      m_destOrigin = Vector (m_liftTravelPos.x, m_liftTravelPos.y, pev->origin.z);

      checkNeedToWait ();
   }

   // need to find a button outside the lift
   if (m_liftState == LiftState::LookingButtonOutside) {

      // button has been pressed, lift should come
      if (m_buttonPushTime + 8.0f >= game.time ()) {
         if (graph.exists (m_previousNodes[0])) {
            m_destOrigin = graph[m_previousNodes[0]].origin;
         }
         else {
            m_destOrigin = pev->origin;
         }
         checkNeedToWait (64.0f);
      }
      else if (!game.isNullEntity (m_liftEntity)) {
         auto button = lookupButton (m_liftEntity->v.targetname.str (), true);

         // if we got a valid button entity
         if (!game.isNullEntity (button)) {
            // lift is already used ?
            bool liftUsed = false;

            // iterate though clients, and find if lift already used
            for (const auto &client : util.getClients ()) {
               if (!(client.flags & ClientFlags::Used)
                  || !(client.flags & ClientFlags::Alive)
                  || client.team != m_team
                  || client.ent == ent ()
                  || game.isNullEntity (client.ent->v.groundentity)) {
                  continue;
               }

               if (client.ent->v.groundentity == m_liftEntity) {
                  liftUsed = true;
                  break;
               }
            }

            // lift is currently used
            if (liftUsed) {
               if (graph.exists (m_previousNodes[0])) {
                  m_destOrigin = graph[m_previousNodes[0]].origin;
               }
               else {
                  m_destOrigin = button->v.origin;
               }
               checkNeedToWait (64.0f);
            }
            else {
               m_pickupItem = button;
               m_pickupType = Pickup::Button;

               m_liftState = LiftState::WaitingFor;

               m_navTimeset = game.time ();
               m_liftUsageTime = game.time () + 20.0f;
            }
         }
         else {
            m_liftState = LiftState::WaitingFor;
            m_liftUsageTime = game.time () + 15.0f;
         }
      }
   }

   // bot is waiting for lift
   if (m_liftState == LiftState::WaitingFor) {
      if (graph.exists (m_previousNodes[0])) {
         if (!(graph[m_previousNodes[0]].flags & NodeFlag::Lift)) {
            m_destOrigin = graph[m_previousNodes[0]].origin;
         }
         else if (graph.exists (m_previousNodes[1])) {
            m_destOrigin = graph[m_previousNodes[1]].origin;
         }
      }
      checkNeedToWait (64.0f);
   }

   // if bot is waiting for lift, or going to it
   if (m_liftState == LiftState::WaitingFor || m_liftState == LiftState::EnteringIn) {
      // bot fall down somewhere inside the lift's groove :)
      if (pev->groundentity != m_liftEntity && graph.exists (m_previousNodes[0])) {
         if ((graph[m_previousNodes[0]].flags & NodeFlag::Lift)
            && (m_path->origin.z - pev->origin.z) > 50.0f
            && (graph[m_previousNodes[0]].origin.z - pev->origin.z) > 50.0f) {

            m_liftState = LiftState::None;
            m_liftEntity = nullptr;
            m_liftUsageTime = 0.0f;

            clearSearchNodes ();
            findNextBestNode ();

            if (graph.exists (m_previousNodes[2])) {
               findPath (m_currentNodeIndex, m_previousNodes[2], FindPath::Fast);
            }
            return false;
         }
      }
   }
   return true;
}

bool Bot::updateLiftStates () {
   if (!game.isNullEntity (m_liftEntity) && !(m_pathFlags & NodeFlag::Lift)) {
      if (m_liftState == LiftState::TravelingBy) {
         m_liftState = LiftState::Leaving;
         m_liftUsageTime = game.time () + 10.0f;
      }
      if (m_liftState == LiftState::Leaving && m_liftUsageTime < game.time () && pev->groundentity != m_liftEntity) {
         m_liftState = LiftState::None;
         m_liftUsageTime = 0.0f;

         m_liftEntity = nullptr;
      }
   }

   if (m_liftUsageTime < game.time () && !cr::fzero (m_liftUsageTime)) {
      m_liftEntity = nullptr;
      m_liftState = LiftState::None;
      m_liftUsageTime = 0.0f;

      clearSearchNodes ();

      if (graph.exists (m_previousNodes[0])) {
         if (!(graph[m_previousNodes[0]].flags & NodeFlag::Lift)) {
            changeNodeIndex (m_previousNodes[0]);
         }
         else {
            findNextBestNode ();
         }
      }
      else {
         findNextBestNode ();
      }
      return false;
   }
   return true;
}

void Bot::clearSearchNodes () {
   m_pathWalk.clear ();
   m_chosenGoalIndex = kInvalidNodeIndex;
}

int Bot::findAimingNode (const Vector &to, int &pathLength) {
   // return the most distant node which is seen from the bot to the target and is within count
   ensureCurrentNodeIndex ();

   const int destIndex = graph.getNearest (to);
   int bestIndex = m_currentNodeIndex;

   if (destIndex == kInvalidNodeIndex) {
      return kInvalidNodeIndex;
   }

   auto result = planner.find (destIndex, m_currentNodeIndex, [&] (int index) {
      ++pathLength;

      if (vistab.visible (m_currentNodeIndex, index)) {
         bestIndex = index;
         return false;
      }
      return true;
   });

   if (result && bestIndex == m_currentNodeIndex) {
      return kInvalidNodeIndex;
   }
   return bestIndex;
}

bool Bot::findNextBestNode () {
   // this function find a node in the near of the bot if bot had lost his path of pathfinder needs
   // to be restarted over again.

   const auto &origin = pev->origin + Vector { pev->velocity.x, pev->velocity.y, 0.0f } * m_frameInterval;
   const auto &bucket = graph.getNodesInBucket (origin);

   // maximum number of nodes to recheck without buckets
   constexpr auto kNearestRecheckThreshold = 1200;

   // try to search in buckets first
   if (!findNextBestNodeEx (bucket, graph.length () < kNearestRecheckThreshold ? true : false)) {

      // fallback to nearest search instead
      return findNextBestNodeEx (graph.getNodeNumbers (), false);
   }
   return true;
}

bool Bot::findNextBestNodeEx (const IntArray &data, bool handleFails) {
   // this function find a node in the near of the bot if bot had lost his path of pathfinder needs
   // to be restarted over again.

   float lessDist[3] {};
   int lessIndex[3] {};

   for (int i = 0; i < 3; ++i) {
      lessDist[i] = kInfiniteDistance;
      lessIndex[i] = kInvalidNodeIndex;
   }

   // in case low node density do not skip previous ones, in case of fail reduce max nodes to skip
   const auto &numToSkip = graph.length () < 512 ? 0 : (handleFails ? 1 : rg (1, 4));

   for (const auto &i : data) {
      const auto &path = graph[i];

      if (!graph.exists (path.number)) {
         continue;
      }
      bool skip = !!(path.number == m_currentNodeIndex);

      // skip the current node, if any
      if (skip && numToSkip > 0) {
         continue;
      }

      // skip current or recent previous node
      for (int j = 0; j < numToSkip; ++j) {
         if (path.number == m_previousNodes[j]) {
            skip = true;
            break;
         }
      }

      // skip node from recent list
      if (skip) {
         continue;
      }

      // cts with hostages should not pick nodes with no hostage flag
      if (game.mapIs (MapFlags::HostageRescue) && m_team == Team::CT && (graph[path.number].flags & NodeFlag::NoHostage) && m_hasHostage) {
         continue;
      }

      // check we're have link to it
      if (m_currentNodeIndex != kInvalidNodeIndex && !graph.isConnected (m_currentNodeIndex, path.number)) {
         continue;
      }

      // ignore non-reacheable nodes...
      if (!isReachableNode (path.number)) {
         continue;
      }

      // if we're still here, find some close nodes
      const float distanceSq = pev->origin.distanceSq (path.origin);

      if (distanceSq < lessDist[0]) {
         lessDist[0] = distanceSq;
         lessIndex[0] = path.number;
      }
      else if (distanceSq < lessDist[1]) {
         lessDist[1] = distanceSq;
         lessIndex[1] = path.number;
      }
      else if (distanceSq < lessDist[2]) {
         lessDist[2] = distanceSq;
         lessIndex[2] = path.number;
      }
   }
   int selected = kInvalidNodeIndex;

   // now pick random one from chosen
   int index = 0;

   // choice from found
   if (lessIndex[2] != kInvalidNodeIndex) {
      index = rg (0, 2);
   }
   else if (lessIndex[1] != kInvalidNodeIndex) {
      index = rg (0, 1);
   }
   else if (lessIndex[0] != kInvalidNodeIndex) {
      index = 0;
   }
   selected = lessIndex[index];

   // worst case... find at least something
   if (selected == kInvalidNodeIndex) {
      if (handleFails) {
         return false;
      }
      selected = findNearestNode ();
   }

   // mark bot as searching for new best next node
   if (selected != kInvalidNodeIndex) {
      m_lostReachableNodeTimer.start (pev->origin.distanceSq (graph[selected].origin) / cr::sqrf (pev->maxspeed) * 4.0f);
   }
   changeNodeIndex (selected);

   return true;
}

float Bot::getEstimatedNodeReachTime () {
   const bool longTermReachability = (m_pathFlags & NodeFlag::Crouch)
      || (m_pathFlags & NodeFlag::Ladder)
      || (pev->button & IN_DUCK)
      || (m_oldButtons & IN_DUCK);

   float estimatedTime = longTermReachability ? 8.5f : 3.5f;

   // if just fired at enemy, increase reachability
   if (m_shootTime + 0.25f > game.time ()) {
      return estimatedTime;
   }

   // calculate 'real' time that we need to get from one node to another
   if (graph.exists (m_currentNodeIndex) && graph.exists (m_previousNodes[0])) {
      const float distanceSq = graph[m_previousNodes[0]].origin.distanceSq (graph[m_currentNodeIndex].origin);

      // calculate estimated time
      estimatedTime = 5.0f * (distanceSq / cr::sqrf (m_moveSpeed + 1.0f));

      // check for special nodes, that can slowdown our movement
      if (longTermReachability) {
         estimatedTime *= 2.0f;
      }
      estimatedTime = cr::clamp (estimatedTime, 3.0f, longTermReachability ? 8.0f : 3.5f);
   }
   return estimatedTime;
}

void Bot::findValidNode () {
   // checks if the last node the bot was heading for is still valid

   // if bot hasn't got a node we need a new one anyway or if time to get there expired get new one as well
   if (m_currentNodeIndex == kInvalidNodeIndex) {
      clearSearchNodes ();
      findNextBestNode ();
   }
   else if (m_navTimeset + getEstimatedNodeReachTime () < game.time ()) {
      constexpr int kMaxDamageValue = PracticeLimit::Damage;

      // increase danger for both teams
      for (int team = Team::Terrorist; team < kGameTeamNum; ++team) {
         int damageValue = practice.getDamage (team, m_currentNodeIndex, m_currentNodeIndex);
         damageValue = cr::clamp (damageValue + 100, 0, kMaxDamageValue);

         // affect nearby connected with victim nodes
         for (const auto &neighbour : m_path->links) {
            if (graph.exists (neighbour.index)) {
               int neighbourValue = practice.getDamage (team, neighbour.index, neighbour.index);
               neighbourValue = cr::clamp (neighbourValue + 100, 0, kMaxDamageValue);

               practice.setDamage (m_team, neighbour.index, neighbour.index, neighbourValue);
            }
         }
         practice.setDamage (m_team, m_currentNodeIndex, m_currentNodeIndex, damageValue);
      }
      clearSearchNodes ();
      findNextBestNode ();
   }
}

int Bot::changeNodeIndex (int index) {
   if (index == kInvalidNodeIndex) {
      return kInvalidNodeIndex;
   }
   m_previousNodes[4] = m_previousNodes[3];
   m_previousNodes[3] = m_previousNodes[2];
   m_previousNodes[2] = m_previousNodes[1];
   m_previousNodes[0] = m_currentNodeIndex;

   m_currentNodeIndex = index;

   m_navTimeset = game.time ();
   m_collideTime = game.time ();

   m_path = &graph[m_currentNodeIndex];
   m_pathFlags = m_path->flags;
   m_pathOrigin = m_path->origin;

   return m_currentNodeIndex; // to satisfy static-code analyzers
}

int Bot::findNearestNode () {
   // get the current nearest node to bot with visibility checks

   constexpr float kMaxDistance = 1024.0f;

   int index = kInvalidNodeIndex;
   float nearestDistanceSq = cr::sqrf (kMaxDistance);

   const auto &origin = pev->origin + Vector { pev->velocity.x, pev->velocity.y, 0.0f } * m_frameInterval;
   const auto &bucket = graph.getNodesInBucket (origin);

   for (const auto &i : bucket) {
      const auto &path = graph[i];

      if (!graph.exists (path.number)) {
         continue;
      }
      const float distanceSq = path.origin.distanceSq (origin);

      if (distanceSq < nearestDistanceSq) {
         // if bot doing navigation, make sure node really visible and reachable
         if (isReachableNode (path.number)) {
            index = path.number;
            nearestDistanceSq = distanceSq;
         }
      }
   }

   // try to search ANYTHING that can be reached
   if (!graph.exists (index)) {
      nearestDistanceSq = cr::sqrf (kMaxDistance);
      const auto &nearestNodes = graph.getNodeNumbers ();

      for (const auto &i : nearestNodes) {
         const auto &path = graph[i];

         if (!graph.exists (path.number)) {
            continue;
         }
         const float distanceSq = path.origin.distanceSq (origin);

         if (distanceSq < nearestDistanceSq) {
            TraceResult tr {};
            game.testLine (getEyesPos (), path.origin, TraceIgnore::Monsters, ent (), &tr);

            if (tr.flFraction >= 1.0f && !tr.fStartSolid) {
               index = path.number;
               nearestDistanceSq = distanceSq;
            }
         }
      }

      // if we're got something just return here
      if (graph.exists (index)) {
         return index;
      }
   }

   // worst case, take any node...
   if (!graph.exists (index)) {
      index = graph.getNearestNoBuckets (origin);
   }
   return index;
}

int Bot::findBombNode () {
   // this function finds the best goal (bomb) node for CTs when searching for a planted bomb.

   const auto &goals = graph.m_goalPoints;

   const auto &bomb = graph.getBombOrigin ();
   const auto &audible = isBombAudible ();

   // take the nearest to bomb nodes instead of goal if close enough
   if (pev->origin.distanceSq (bomb) < cr::sqrf (96.0f)) {
      const int node = graph.getNearest (bomb, 420.0f);

      m_bombSearchOverridden = true;

      if (node != kInvalidNodeIndex) {
         return node;
      }
   }
   else if (!audible.empty ()) {
      m_bombSearchOverridden = true;
      return graph.getNearest (audible, 240.0f);
   }
   else if (goals.empty ()) {
      return graph.getNearest (bomb, 512.0f); // reliability check
   }

   int goal = 0, count = 0;
   float lastDistanceSq = kInfiniteDistance;

   // find nearest goal node either to bomb (if "heard" or player)
   for (const auto &point : goals) {
      const float distanceSq = bomb.distanceSq (graph[point].origin);

      // check if we got more close distance
      if (distanceSq < lastDistanceSq) {
         goal = point;
         lastDistanceSq = distanceSq;
      }
   }

   while (graph.isVisited (goal)) {
      goal = goals.random ();

      if (count++ >= static_cast <int> (goals.length ())) {
         break;
      }
   }
   return goal;
}

int Bot::findDefendNode (const Vector &origin) {
   // this function tries to find a good position which has a line of sight to a position,
   // provides enough cover point, and is far away from the defending position

   ensureCurrentNodeIndex ();
   TraceResult tr {};

   int nodeIndex[kMaxNodeLinks] {};
   float nearestDistance[kMaxNodeLinks] {};

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      nodeIndex[i] = kInvalidNodeIndex;
      nearestDistance[i] = 128.0f;
   }

   const int posIndex = graph.getNearest (origin);
   int srcIndex = m_currentNodeIndex;

   // max search distance
   const auto kMaxDistance = cr::clamp (static_cast <float> (148 * bots.getBotCount ()), 256.0f, 1024.0f);

   // some of points not found, return random one
   if (srcIndex == kInvalidNodeIndex || posIndex == kInvalidNodeIndex) {
      return graph.random ();
   }

   // find the best node now
   for (const auto &path : graph) {
      // exclude ladder & current nodes
      if ((path.flags & NodeFlag::Ladder) || path.number == srcIndex || !vistab.visible (path.number, posIndex)) {
         continue;
      }

      // use the 'real' path finding distances
      auto distance = planner.dist (srcIndex, path.number);

      // skip nodes too far
      if (distance > kMaxDistance) {
         continue;
      }

      // skip occupied points
      if (isOccupiedNode (path.number)) {
         continue;
      }
      game.testLine (path.origin, graph[posIndex].origin, TraceIgnore::Glass, ent (), &tr);

      // check if line not hit anything
      if (!cr::fequal (tr.flFraction, 1.0f)) {
         continue;
      }

      if (distance > nearestDistance[0]) {
         nodeIndex[0] = path.number;
         nearestDistance[0] = distance;
      }
      else if (distance > nearestDistance[1]) {
         nodeIndex[1] = path.number;
         nearestDistance[1] = distance;
      }
      else if (distance > nearestDistance[2]) {
         nodeIndex[2] = path.number;
         nearestDistance[2] = distance;
      }
      else if (distance > nearestDistance[3]) {
         nodeIndex[3] = path.number;
         nearestDistance[3] = distance;
      }
      else if (distance > nearestDistance[4]) {
         nodeIndex[4] = path.number;
         nearestDistance[4] = distance;
      }
      else if (distance > nearestDistance[5]) {
         nodeIndex[5] = path.number;
         nearestDistance[5] = distance;
      }
      else if (distance > nearestDistance[6]) {
         nodeIndex[6] = path.number;
         nearestDistance[6] = distance;
      }
      else if (distance > nearestDistance[7]) {
         nodeIndex[7] = path.number;
         nearestDistance[7] = distance;
      }
   }

   // use statistic if we have them
   for (int i = 0; i < kMaxNodeLinks; ++i) {
      if (nodeIndex[i] != kInvalidNodeIndex) {
         int practiceDamage = practice.getDamage (m_team, nodeIndex[i], nodeIndex[i]);
         practiceDamage = (practiceDamage * 100) / practice.getHighestDamageForTeam (m_team);

         nearestDistance[i] = static_cast <float> ((practiceDamage * 100) / 8192);
         nearestDistance[i] += static_cast <float> (practiceDamage);
      }
   }
   bool sorting = false;

   // sort resulting nodes for farest distance
   do {
      sorting = false;

      // completely sort the data
      for (int i = 0; i < kMaxNodeLinks - 1; ++i) {
         if (nodeIndex[i] != kInvalidNodeIndex && nodeIndex[i + 1] != kInvalidNodeIndex && nearestDistance[i] > nearestDistance[i + 1]) {
            cr::swap (nodeIndex[i], nodeIndex[i + 1]);
            cr::swap (nearestDistance[i], nearestDistance[i + 1]);

            sorting = true;
         }
      }
   } while (sorting);

   if (nodeIndex[0] == kInvalidNodeIndex) {
      IntArray found {};

      for (const auto &path : graph) {
         if (origin.distanceSq (path.origin) < cr::sqrf (kMaxDistance)
            && vistab.visible (path.number, posIndex)
            && !isOccupiedNode (path.number)) {

            found.push (path.number);
         }
      }

      if (found.empty ()) {
         return graph.random (); // most worst case, since there a evil error in nodes
      }
      return found.random ();
   }
   int index = 0;

   for (; index < kMaxNodeLinks; ++index) {
      if (nodeIndex[index] == kInvalidNodeIndex) {
         break;
      }
   }
   return nodeIndex[rg (0, (index - 1) / 2)];
}

int Bot::findCoverNode (float maxDistance) {
   // this function tries to find a good cover node if bot wants to hide

   const float enemyMaxDistance = m_lastEnemyOrigin.distance (pev->origin);

   // do not move to a position near to the enemy
   if (maxDistance > enemyMaxDistance) {
      maxDistance = enemyMaxDistance;
   }

   if (maxDistance < 300.0f) {
      maxDistance = 300.0f;
   }

   const int srcIndex = m_currentNodeIndex;
   const int enemyIndex = graph.getNearest (m_lastEnemyOrigin);

   IntArray enemies {};

   int nodeIndex[kMaxNodeLinks] {};
   float nearestDistance[kMaxNodeLinks] {};

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      nodeIndex[i] = kInvalidNodeIndex;
      nearestDistance[i] = maxDistance;
   }

   if (enemyIndex == kInvalidNodeIndex) {
      return kInvalidNodeIndex;
   }

   // now get enemies neigbouring points
   for (const auto &link : graph[enemyIndex].links) {
      if (link.index != kInvalidNodeIndex) {
         enemies.push (link.index);
      }
   }

   // ensure we're on valid point
   changeNodeIndex (srcIndex);

   // find the best node now
   for (const auto &path : graph) {
      // exclude ladder, current node and nodes seen by the enemy
      if ((path.flags & NodeFlag::Ladder) || path.number == srcIndex || vistab.visible (enemyIndex, path.number)) {
         continue;
      }
      bool neighbourVisible = false; // now check neighbour nodes for visibility

      for (const auto &enemy : enemies) {
         if (vistab.visible (enemy, path.number)) {
            neighbourVisible = true;
            break;
         }
      }

      // skip visible points
      if (neighbourVisible) {
         continue;
      }

      // use the 'real' pathfinding distances
      const float distance = planner.dist (srcIndex, path.number);
      const float enemyDistance = planner.dist (enemyIndex, path.number);

      if (distance >= enemyDistance) {
         continue;
      }

      if (distance < nearestDistance[0]) {
         nodeIndex[0] = path.number;
         nearestDistance[0] = distance;
      }
      else if (distance < nearestDistance[1]) {
         nodeIndex[1] = path.number;
         nearestDistance[1] = distance;
      }
      else if (distance < nearestDistance[2]) {
         nodeIndex[2] = path.number;
         nearestDistance[2] = distance;
      }
      else if (distance < nearestDistance[3]) {
         nodeIndex[3] = path.number;
         nearestDistance[3] = distance;
      }
      else if (distance < nearestDistance[4]) {
         nodeIndex[4] = path.number;
         nearestDistance[4] = distance;
      }
      else if (distance < nearestDistance[5]) {
         nodeIndex[5] = path.number;
         nearestDistance[5] = distance;
      }
      else if (distance < nearestDistance[6]) {
         nodeIndex[6] = path.number;
         nearestDistance[6] = distance;
      }
      else if (distance < nearestDistance[7]) {
         nodeIndex[7] = path.number;
         nearestDistance[7] = distance;
      }
   }

   // use statistic if we have them
   for (int i = 0; i < kMaxNodeLinks; ++i) {
      if (nodeIndex[i] != kInvalidNodeIndex) {
         int practiceDamage = practice.getDamage (m_team, nodeIndex[i], nodeIndex[i]);
         practiceDamage = (practiceDamage * 100) / practice.getHighestDamageForTeam (m_team);

         nearestDistance[i] = static_cast <float> ((practiceDamage * 100) / 8192);
         nearestDistance[i] += static_cast <float> (practiceDamage);
      }
   }
   bool sorting = false;

   // sort resulting nodes for nearest distance
   do {
      sorting = false;

      for (int i = 0; i < kMaxNodeLinks - 1; ++i) {
         if (nodeIndex[i] != kInvalidNodeIndex && nodeIndex[i + 1] != kInvalidNodeIndex && nearestDistance[i] > nearestDistance[i + 1]) {
            cr::swap (nodeIndex[i], nodeIndex[i + 1]);
            cr::swap (nearestDistance[i], nearestDistance[i + 1]);

            sorting = true;
         }
      }
   } while (sorting);

   TraceResult tr {};

   // take the first one which isn't spotted by the enemy
   for (const auto &index : nodeIndex) {
      if (index != kInvalidNodeIndex) {
         game.testLine (m_lastEnemyOrigin + Vector (0.0f, 0.0f, 36.0f), graph[index].origin, TraceIgnore::Everything, ent (), &tr);

         if (tr.flFraction < 1.0f) {
            return index;
         }
      }
   }

   // if all are seen by the enemy, take the first one
   if (nodeIndex[0] != kInvalidNodeIndex) {
      return nodeIndex[0];
   }
   return kInvalidNodeIndex; // do not use random points
}

bool Bot::selectBestNextNode () {
   // this function does a realtime post processing of nodes return from the
   // pathfinder, to vary paths and find the best node on our way

   const auto nextNodeIndex = m_pathWalk.next ();
   const auto currentNodeIndex = m_pathWalk.first ();
   const auto prevNodeIndex = m_currentNodeIndex;

   if (!isOccupiedNode (currentNodeIndex)) {
      return false;
   }

   // check the links
   for (const auto &link : graph[prevNodeIndex].links) {

      // skip invalid links, or links that points to itself
      if (!graph.exists (link.index) || currentNodeIndex == link.index) {
         continue;
      }

      // skip isn't connected links
      if (!graph.isConnected (link.index, nextNodeIndex) || !graph.isConnected (link.index, prevNodeIndex)) {
         continue;
      }

      // skip isn't visible links
      if (!vistab.visible (link.index, nextNodeIndex) || !vistab.visible (link.index, prevNodeIndex)) {
         continue;
      }

      // don't use ladder nodes as alternative
      if (graph[link.index].flags & (NodeFlag::Ladder | NodeFlag::Camp | PathFlag::Jump)) {
         continue;
      }

      // if not occupied, just set advance
      if (!isOccupiedNode (link.index)) {
         m_pathWalk.first () = link.index;
         return true;
      }
   }
   return false;
}

bool Bot::advanceMovement () {
   // advances in our pathfinding list and sets the appropriate destination origins for this bot

   findValidNode (); // check if old nodes is still reliable

   // no nodes from pathfinding?
   if (m_pathWalk.empty ()) {
      return false;
   }

   m_pathWalk.shift (); // advance in list
   m_currentTravelFlags = 0; // reset travel flags (jumping etc)

   // helper to change bot's goal
   auto changeNextGoal = [&] {
      const int newGoal = findBestGoal ();

      m_prevGoalIndex = newGoal;
      m_chosenGoalIndex = newGoal;

      // remember index
      getTask ()->data = newGoal;

      // do path finding if it's not the current node
      if (newGoal != m_currentNodeIndex) {
         findPath (m_currentNodeIndex, newGoal, m_pathType);
      }
   };

   // we're not at the end of the list?
   if (!m_pathWalk.empty ()) {
      // if in between a route, postprocess the node (find better alternatives)...
      if (m_pathWalk.hasNext () && m_pathWalk.first () != m_pathWalk.last ()) {
         selectBestNextNode ();
         m_minSpeed = pev->maxspeed;

         const auto tid = getCurrentTaskId ();

         // only if we in normal task and bomb is not planted
         if (tid == Task::Normal
            && bots.getRoundMidTime () + 5.0f < game.time ()
            && m_timeCamping + 5.0f < game.time ()
            && !bots.isBombPlanted ()
            && m_personality != Personality::Rusher
            && !m_hasC4 && !m_isVIP
            && m_loosedBombNodeIndex == kInvalidNodeIndex
            && !m_hasHostage && !m_isCreature) {

            m_campButtons = 0;

            const auto nextIndex = m_pathWalk.next ();
            auto kills = static_cast <float> (practice.getDamage (m_team, nextIndex, nextIndex));

            // if damage done higher than one
            if (kills > 1.0f && bots.getRoundMidTime () > game.time ()) {
               switch (m_personality) {
               case Personality::Normal:
                  kills *= 0.33f;
                  break;

               default:
                  kills *= 0.5f;
                  break;
               }

               if (m_baseAgressionLevel < kills && hasPrimaryWeapon ()) {
                  startTask (Task::Camp, TaskPri::Camp, kInvalidNodeIndex, game.time () + rg (static_cast <float> (m_difficulty / 2), static_cast <float> (m_difficulty)) * 5.0f, true);
                  startTask (Task::MoveToPosition, TaskPri::MoveToPosition, findDefendNode (graph[nextIndex].origin), game.time () + rg (3.0f, 10.0f), true);
               }
            }
            else if (bots.canPause () && !isOnLadder () && !isInWater () && !m_currentTravelFlags && isOnFloor ()) {
               if (cr::fequal (kills, m_baseAgressionLevel)) {
                  m_campButtons |= IN_DUCK;
               }
               else if (rg.chance (m_difficulty * 25)) {
                  m_minSpeed = getShiftSpeed ();
               }
            }

            // force terrorist bot to plant bomb
            if (m_inBombZone && !m_hasProgressBar && m_hasC4 && tid != Task::PlantBomb) {
               changeNextGoal ();
               return false;
            }
         }
      }

      if (!m_pathWalk.empty ()) {
         m_jumpSequence = false;

         const auto destIndex = m_pathWalk.first ();
         bool isCurrentJump = false;

         // find out about connection flags
         if (destIndex != kInvalidNodeIndex && m_currentNodeIndex != kInvalidNodeIndex) {
            for (const auto &link : m_path->links) {
               if (link.index == destIndex) {
                  m_currentTravelFlags = link.flags;
                  m_desiredVelocity = link.velocity;
                  m_jumpFinished = false;

                  isCurrentJump = true;
                  break;
               }
            }

            // if graph is analyzed try our special jumps
            if (graph.isAnalyzed () || analyzer.isAnalyzed ()) {
               for (const auto &link : m_path->links) {
                  if (link.index == destIndex) {
                     const float diff = cr::abs (m_path->origin.z - graph[destIndex].origin.z);

                     // if height difference is enough, consider this link as jump link
                     if (graph[destIndex].origin.z > m_path->origin.z && diff > cv_graph_slope_height.as <float> ()) {
                        m_currentTravelFlags |= PathFlag::Jump;
                        m_desiredVelocity.clear (); // make bot compute jump velocity
                        m_jumpFinished = false; // force-mark this path as jump

                        break;
                     }
                  }
               }
            }

            // check if bot is going to jump
            bool willJump = false;
            float jumpDistanceSq = 0.0f;

            Vector src {}, dst {};

            // try to find out about future connection flags
            if (m_pathWalk.hasNext ()) {
               auto nextIndex = m_pathWalk.next ();

               const auto &path = graph[destIndex];
               const auto &next = graph[nextIndex];

               for (const auto &link : path.links) {
                  if (link.index == nextIndex && (link.flags & PathFlag::Jump)) {
                     src = path.origin;
                     dst = next.origin;

                     jumpDistanceSq = src.distanceSq (dst);
                     willJump = true;

                     break;
                  }
               }
            }

            // mark as jump sequence, if the current and next paths are jumps
            if (isCurrentJump) {
               m_jumpSequence = willJump;
            }

            // is there a jump node right ahead and do we need to draw out the light weapon ?
            if (willJump && !usesKnife ()
               && m_currentWeapon != Weapon::Scout
               && !m_isReloading && !usesPistol ()
               && (jumpDistanceSq > cr::sqrf (145.0f) || (dst.z - 32.0f > src.z && jumpDistanceSq > cr::sqrf (125.0f)))
               && !(m_states & Sense::SeeingEnemy)) {

               selectWeaponById (Weapon::Knife); // draw out the knife if we needed
            }

            // bot not already on ladder but will be soon?
            if ((graph[destIndex].flags & NodeFlag::Ladder) && !isOnLadder ()) {

               // get ladder nodes used by other (first moving) bots
               for (const auto &other : bots) {
                  // if another bot uses this ladder, wait 3 secs
                  if (other.get () != this && other->m_isAlive && other->m_currentNodeIndex == destIndex && other->isOnLadder ()) {
                     startTask (Task::Pause, TaskPri::Pause, kInvalidNodeIndex, game.time () + 3.0f, false);
                     return true;
                  }
               }
            }
         }
         changeNodeIndex (destIndex);
      }
   }
   setPathOrigin ();
   m_navTimeset = game.time ();

   return true;
}

void Bot::setPathOrigin () {
   constexpr int kMaxAlternatives = 5;
   const float radius = m_path->radius;

   auto setNonZeroPathOrigin = [&] () -> void {
      m_pathOrigin += Vector { pev->angles.x, cr::wrapAngle (pev->angles.y + rg (-90.0f, 90.0f)), 0.0f }.forward () * rg (0.0f, radius);
   };

   // if node radius non zero vary origin a bit depending on the body angles
   if (radius > 16.0f && !isInNarrowPlace ()) {
      int nearestIndex = kInvalidNodeIndex;

      if (!m_pathWalk.empty () && m_pathWalk.hasNext ()) {
         Vector orgs[kMaxAlternatives] {};

         for (int i = 0; i < kMaxAlternatives; ++i) {
            orgs[i] = m_pathOrigin + Vector (rg (-radius, radius), rg (-radius, radius), 0.0f);
         }
         float nearestDistanceSq = kInfiniteDistance;

         for (int i = 0; i < kMaxAlternatives; ++i) {
            const float distanceSq = pev->origin.distanceSq (orgs[i]);

            if (distanceSq < nearestDistanceSq) {
               nearestIndex = i;
               nearestDistanceSq = distanceSq;
            }
         }

         // set the origin if found alternative
         if (nearestIndex != kInvalidNodeIndex) {
            m_pathOrigin = orgs[nearestIndex];
         }
      }

      if (nearestIndex == kInvalidNodeIndex) {
         setNonZeroPathOrigin ();
      }
   }
   else if (radius > 0.0f) {
      setNonZeroPathOrigin ();
   }

   if (isOnLadder ()) {
      TraceResult tr {};
      game.testLine (Vector (pev->origin.x, pev->origin.y, pev->absmin.z), m_pathOrigin, TraceIgnore::Everything, ent (), &tr);

      if (tr.flFraction < 1.0f) {
         m_pathOrigin = m_pathOrigin + (pev->origin - m_pathOrigin) * 0.5f + Vector (0.0f, 0.0f, 32.0f);
      }
   }
}

void Bot::updateRightRef () {
   m_rightRef = Vector { 0.0f, pev->angles.y, 0.0f }.right (); // convert current view angle to vectors for traceline math...
}

bool Bot::isBlockedForward (const Vector &normal, TraceResult *tr) {
   // checks if bot is blocked in his movement direction (excluding doors)

   // use some TraceLines to determine if anything is blocking the current path of the bot.

   // first do a trace from the bot's eyes forward...
   auto src = getEyesPos ();
   auto forward = src + normal * 24.0f;

   auto checkDoor = [] (TraceResult *result) {
      if (!game.mapIs (MapFlags::HasDoors)) {
         return false;
      }
      return result->flFraction < 1.0f && !util.isDoorEntity (result->pHit);
   };

   auto checkHostage = [&] (TraceResult *result) {
      return result->flFraction < 1.0f && m_team == Team::Terrorist && !util.isHostageEntity (result->pHit);
   };

   // trace from the bot's eyes straight forward...
   game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f) {
      if ((game.mapIs (MapFlags::HasDoors) && util.isDoorEntity (tr->pHit))
         || (m_team == Team::CT && util.isHostageEntity (tr->pHit))) {
         return false;
      }
      return true; // bot's head will hit something
   }
   constexpr auto kVec00N16 = Vector (0.0f, 0.0f, -16.0f);

   // right referential vector
   updateRightRef ();

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder left diagonal forward to the right shoulder...
   src = getEyesPos () + kVec00N16 - m_rightRef * -16.0f;
   forward = getEyesPos () + kVec00N16 + m_rightRef * 16.0f + normal * 24.0f;

   game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (checkDoor (tr) || checkHostage (tr)) {
      return true; // bot's body will hit something
   }

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder right diagonal forward to the left shoulder...
   src = getEyesPos () + kVec00N16 + m_rightRef * 16.0f;
   forward = getEyesPos () + kVec00N16 - m_rightRef * -16.0f + normal * 24.0f;

   game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (checkDoor (tr) || checkHostage (tr)) {
      return true; // bot's body will hit something
   }

   // now check below waist
   if (isDucking ()) {
      src = pev->origin + Vector (0.0f, 0.0f, -19.0f + 19.0f);
      forward = src + Vector (0.0f, 0.0f, 10.0f) + normal * 24.0f;

      game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

      // check if the trace hit something...
      if (checkDoor (tr) || checkHostage (tr)) {
         return true; // bot's body will hit something
      }
      src = pev->origin;
      forward = src + normal * 24.0f;

      game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

      // check if the trace hit something...
      if (checkDoor (tr) || checkHostage (tr)) {
         return true; // bot's body will hit something
      }
   }
   else {
      constexpr auto kVec00N17 = Vector (0.0f, 0.0f, -17.0f);
      constexpr auto kVec00N24 = Vector (0.0f, 0.0f, -24.0f);

      // trace from the left waist to the right forward waist pos
      src = pev->origin + kVec00N17 - m_rightRef * -16.0f;
      forward = pev->origin + kVec00N17 + m_rightRef * 16.0f + normal * 24.0f;

      // trace from the bot's waist straight forward...
      game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

      // check if the trace hit something...
      if (checkDoor (tr) || checkHostage (tr)) {
         return true; // bot's body will hit something
      }

      // trace from the left waist to the right forward waist pos
      src = pev->origin + kVec00N24 + m_rightRef * 16.0f;
      forward = pev->origin + kVec00N24 - m_rightRef * -16.0f + normal * 24.0f;

      game.testLine (src, forward, TraceIgnore::Monsters, ent (), tr);

      // check if the trace hit something...
      if (checkDoor (tr) || checkHostage (tr)) {
         return true; // bot's body will hit something
      }
   }
   return false; // bot can move forward, return false
}

bool Bot::canStrafeLeft (TraceResult *tr) {
   // this function checks if bot can move sideways

   Vector right {}, forward {};
   pev->v_angle.angleVectors (&forward, &right, nullptr);

   Vector src = pev->origin;
   Vector dest = src - right * 40.0f;

   // trace from the bot's waist straight left...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f) {
      return false; // bot's body will hit something
   }
   src = dest;
   dest = dest + forward * 40.0f;

   // trace from the strafe pos straight forward...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f) {
      return false; // bot's body will hit something
   }
   return true;
}

bool Bot::canStrafeRight (TraceResult *tr) {
   // this function checks if bot can move sideways

   Vector right {}, forward {};
   pev->v_angle.angleVectors (&forward, &right, nullptr);

   Vector src = pev->origin;
   Vector dest = src + right * 40.0f;

   // trace from the bot's waist straight right...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f) {
      return false; // bot's body will hit something
   }
   src = dest;
   dest = dest + forward * 40.0f;

   // trace from the strafe pos straight forward...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f) {
      return false; // bot's body will hit something
   }
   return true;
}

bool Bot::canJumpUp (const Vector &normal) {
   // this function check if bot can jump over some obstacle

   TraceResult tr {};

   // can't jump if not on ground and not on ladder/swimming
   if (!isOnFloor () && (isOnLadder () || !isInWater ())) {
      return false;
   }
   updateRightRef ();

   // check for normal jump height first...
   auto src = pev->origin + Vector (0.0f, 0.0f, -36.0f + 45.0f);
   auto dest = src + normal * 32.0f;

   // trace a line forward at maximum jump height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   if (tr.flFraction < 1.0f) {
      return doneCanJumpUp (normal, m_rightRef);
   }
   else {
      // now trace from jump height upward to check for obstructions...
      src = dest;
      dest.z = dest.z + 37.0f;

      game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

      if (tr.flFraction < 1.0f) {
         return false;
      }
   }

   // now check same height to one side of the bot...
   src = pev->origin + m_rightRef * 16.0f + Vector (0.0f, 0.0f, -36.0f + 45.0f);
   dest = src + normal * 32.0f;

   // trace a line forward at maximum jump height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return doneCanJumpUp (normal, m_rightRef);
   }

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37.0f;

   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return false;
   }

   // now check same height on the other side of the bot...
   src = pev->origin + (-m_rightRef * 16.0f) + Vector (0.0f, 0.0f, -36.0f + 45.0f);
   dest = src + normal * 32.0f;

   // trace a line forward at maximum jump height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return doneCanJumpUp (normal, m_rightRef);
   }

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37.0f;

   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   return tr.flFraction > 1.0f;
}

bool Bot::doneCanJumpUp (const Vector &normal, const Vector &right) {
   // use center of the body first... maximum duck jump height is 62, so check one unit above that (63)
   Vector src = pev->origin + Vector (0.0f, 0.0f, -36.0f + 63.0f);
   Vector dest = src + normal * 32.0f;

   TraceResult tr {};

   // trace a line forward at maximum jump height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   if (tr.flFraction < 1.0f) {
      return false;
   }
   else {
      // now trace from jump height upward to check for obstructions...
      src = dest;
      dest.z = dest.z + 37.0f;

      game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

      // if trace hit something, check duckjump
      if (tr.flFraction < 1.0f) {
         return false;
      }
   }

   // now check same height to one side of the bot...
   src = pev->origin + right * 16.0f + Vector (0.0f, 0.0f, -36.0f + 63.0f);
   dest = src + normal * 32.0f;

   // trace a line forward at maximum jump height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return false;
   }

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37.0f;

   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return false;
   }

   // now check same height on the other side of the bot...
   src = pev->origin + (-right * 16.0f) + Vector (0.0f, 0.0f, -36.0f + 63.0f);
   dest = src + normal * 32.0f;

   // trace a line forward at maximum jump height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return false;
   }

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37.0f;

   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   return tr.flFraction > 1.0f;
}

bool Bot::canDuckUnder (const Vector &normal) {
   // this function check if bot can duck under obstacle

   TraceResult tr {};
   Vector baseHeight {};

   // use center of the body first...
   if (isDucking ()) {
      baseHeight = pev->origin + Vector (0.0f, 0.0f, -17.0f);
   }
   else {
      baseHeight = pev->origin;
   }

   Vector src = baseHeight;
   Vector dest = src + normal * 32.0f;

   // trace a line forward at duck height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return false;
   }
   updateRightRef ();

   // now check same height to one side of the bot...
   src = baseHeight + m_rightRef * 16.0f;
   dest = src + normal * 32.0f;

   // trace a line forward at duck height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f) {
      return false;
   }

   // now check same height on the other side of the bot...
   src = baseHeight + (-m_rightRef * 16.0f);
   dest = src + normal * 32.0f;

   // trace a line forward at duck height...
   game.testLine (src, dest, TraceIgnore::Monsters, ent (), &tr);

   // if trace hit something, return false
   return tr.flFraction > 1.0f;
}

bool Bot::isBlockedLeft () {
   TraceResult tr {};
   float direction = 48.0f;

   if (m_moveSpeed < 0.0f) {
      direction = -48.0f;
   }
   Vector right {}, forward {};
   pev->angles.angleVectors (&forward, &right, nullptr);

   // do a trace to the left...
   game.testLine (pev->origin, forward * direction - right * 48.0f, TraceIgnore::Monsters, ent (), &tr);

   // check if the trace hit something...
   if (game.mapIs (MapFlags::HasDoors) && tr.flFraction < 1.0f && !util.isDoorEntity (tr.pHit)) {
      return true; // bot's body will hit something
   }
   return false;
}

bool Bot::isBlockedRight () {
   TraceResult tr {};
   float direction = 48.0f;

   if (m_moveSpeed < 0.0f) {
      direction = -48.0f;
   }
   Vector right {}, forward {};
   pev->angles.angleVectors (&forward, &right, nullptr);

   // do a trace to the right...
   game.testLine (pev->origin, pev->origin + forward * direction + right * 48.0f, TraceIgnore::Monsters, ent (), &tr);

   // check if the trace hit something...
   if (game.mapIs (MapFlags::HasDoors) && tr.flFraction < 1.0f && !util.isDoorEntity (tr.pHit)) {
      return true; // bot's body will hit something
   }
   return false;
}

bool Bot::checkWallOnLeft (float distance) {
   TraceResult tr {};
   game.testLine (pev->origin, pev->origin - pev->angles.right () * distance, TraceIgnore::Monsters, ent (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f) {
      return true;
   }
   return false;
}

bool Bot::checkWallOnRight (float distance) {
   TraceResult tr {};

   // do a trace to the right...
   game.testLine (pev->origin, pev->origin + pev->angles.right () * distance, TraceIgnore::Monsters, ent (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f) {
      return true;
   }
   return false;
}

bool Bot::checkWallOnBehind (float distance) {
   TraceResult tr {};

   // do a trace to the right...
   game.testLine (pev->origin, pev->origin - pev->angles.forward () * distance, TraceIgnore::Monsters, ent (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f) {
      return true;
   }
   return false;
}

bool Bot::isDeadlyMove (const Vector &to) {
   // this function returns if given location would hurt Bot with falling damage

   TraceResult tr {};

   constexpr auto kUnitsDown = 1000.0f;
   constexpr auto kFallLimit = 160.0f;

   Vector check = to, down = to;
   down.z -= kUnitsDown; // straight down 1000 units

   game.testLine (check, down, TraceIgnore::Monsters, ent (), &tr);

   float lastHeight = tr.flFraction * kUnitsDown; // height from ground
   float distanceSq = to.distanceSq (check); // distance from goal

   if (distanceSq <= cr::sqrf (16.0f) && lastHeight > kFallLimit) {
      return true;
   }
   const auto &moveDir = (to - pev->origin).normalize_apx (); // 1 unit long

   while (distanceSq > cr::sqrf (16.0f)) {
      check = check - moveDir * 16.0f; // move 16 units closer to the goal...

      down = check;
      down.z -= kUnitsDown; // straight down 1000 units

      game.testLine (check, down, TraceIgnore::Monsters, ent (), &tr);

      // wall blocking?
      if (tr.fStartSolid) {
         return false;
      }
      const auto height = tr.flFraction * 1000.0f; // height from ground

      // drops more than 150 units?
      if (lastHeight < height - kFallLimit) {
         return true;
      }
      lastHeight = height;
      distanceSq = to.distanceSq (check); // distance from goal
   }
   return false;
}

bool Bot::isNotSafeToMove (const Vector &to) {
   // simplified version of isDeadlyMove() just for combat movement checking

   constexpr auto kUnitsDown = 1000.0f;
   constexpr auto kFallLimit = 160.0f;

   TraceResult tr {};
   game.testLine (to, to + Vector { 0.0f, 0.0f, -kUnitsDown }, TraceIgnore::Monsters, ent (), &tr);

   return tr.fStartSolid || tr.flFraction * kUnitsDown > kFallLimit;
}

int Bot::getRandomCampDir () {
   // find a good node to look at when camping

   ensureCurrentNodeIndex ();
   constexpr int kMaxNodesToSearch = 5;

   int count = 0, indices[kMaxNodesToSearch] {};
   float distTab[kMaxNodesToSearch] {};
   uint16_t visibility[kMaxNodesToSearch] {};

   for (const auto &path : graph) {
      if (m_currentNodeIndex == path.number || !vistab.visible (m_currentNodeIndex, path.number)) {
         continue;
      }

      if (count < kMaxNodesToSearch) {
         indices[count] = path.number;

         distTab[count] = pev->origin.distanceSq (path.origin);
         visibility[count] = path.vis.crouch + path.vis.stand;

         ++count;
      }
      else {
         const float distanceSq = pev->origin.distanceSq (path.origin);
         uint16_t visBits = path.vis.crouch + path.vis.stand;

         for (int j = 0; j < kMaxNodesToSearch; ++j) {
            if (visBits >= visibility[j] && distanceSq > distTab[j]) {
               indices[j] = path.number;

               distTab[j] = distanceSq;
               visibility[j] = visBits;

               break;
            }
         }
      }
   }
   count--;

   if (count >= 0) {
      return indices[rg (0, count)];
   }
   return graph.random ();
}

void Bot::setStrafeSpeed (const Vector &moveDir, float strafeSpeed) {
   const auto &los = (moveDir - pev->origin).normalize2d_apx ();
   const float dot = los | pev->angles.forward ().get2d ();

   if (dot > 0.0f && !checkWallOnRight ()) {
      m_strafeSpeed = strafeSpeed;
   }
   else if (dot < 0.0f && !checkWallOnLeft ()) {
      m_strafeSpeed = -strafeSpeed;
   }
}

int Bot::getNearestToPlantedBomb () {
   // this function tries to find planted c4 on the defuse scenario map and returns nearest to it node

   if (!game.mapIs (MapFlags::Demolition)) {
      return kInvalidNodeIndex; // don't search for bomb if the player is CT, or it's not defusing bomb
   }

   auto bombModel = conf.getBombModelName ();
   auto result = kInvalidNodeIndex;

   // search the bomb on the map
   game.searchEntities ("classname", "grenade", [&] (edict_t *ent) {
      if (util.isModel (ent, bombModel)) {
         result = graph.getNearest (game.getEntityOrigin (ent));

         if (graph.exists (result)) {

            // if bomb entity is bot's ignore list, clear ignore list
            if (isIgnoredItem (ent)) {
               m_ignoredItems.clear ();
            }
            return EntitySearchResult::Break;
         }
      }
      return EntitySearchResult::Continue;
   });
   return result;
}

bool Bot::isOccupiedNode (int index, bool needZeroVelocity) {
   if (!graph.exists (index)) {
      return true;
   }

   for (const auto &client : util.getClients ()) {
      if (!(client.flags & (ClientFlags::Used | ClientFlags::Alive)) || client.team != m_team || client.ent == ent ()) {
         continue;
      }

      // do not check clients far away from us
      if (pev->origin.distanceSq (client.origin) > cr::sqrf (320.0f)) {
         continue;
      }

      if (needZeroVelocity && client.ent->v.velocity.length2d () > 0.0f) {
         continue;
      }
      const auto distanceSq = client.origin.distanceSq (graph[index].origin);

      if (distanceSq < cr::clamp (cr::sqrf (graph[index].radius) * 2.0f, cr::sqrf (98.0f), cr::sqrf (120.0f))) {
         return true;
      }
      auto bot = bots[client.ent];

      if (bot == nullptr || bot == this || !bot->m_isAlive) {
         continue;
      }
      return bot->m_currentNodeIndex == index || bot->m_previousNodes[0] == index;
   }
   return false;
}

edict_t *Bot::lookupButton (StringRef target, bool blindTest) {
   // this function tries to find nearest to current bot button, and returns pointer to
   // it's entity, also here must be specified the target, that button must open.

   if (target.empty ()) {
      return nullptr;
   }
   float nearestDistanceSq = kInfiniteDistance;
   edict_t *result = nullptr;

   TraceResult tr {};

   // find the nearest button which can open our target
   game.searchEntities ("target", target, [&] (edict_t *ent) {
      const Vector &pos = game.getEntityOrigin (ent);

      if (!blindTest) {
         game.testLine (pev->origin, pos, TraceIgnore::Monsters, this->ent (), &tr);
      }

      // check if this place safe
      if (blindTest || (tr.pHit == ent || tr.flFraction > 0.95f)) {
         const float distanceSq = pev->origin.distanceSq (pos);

         // check if we got more close button
         if (distanceSq < nearestDistanceSq) {
            nearestDistanceSq = distanceSq;
            result = ent;
         }
      }
      return EntitySearchResult::Continue;
   });
   return result;
}

bool Bot::isReachableNode (int index) {
   // this function return whether bot able to reach index node or not, depending on several factors.

   if (!graph.exists (index)) {
      return false;
   }
   const auto &src = pev->origin;
   const auto &dst = graph[index].origin;

   // is the destination close enough?
   if (dst.distanceSq (src) > cr::sqrf (600.0f)) {
      return false;
   }

   // it's should be not a problem to reach node inside water...
   if (pev->waterlevel == 2 || pev->waterlevel == 3) {
      return true;
   }
   const float distanceSq2d = dst.distanceSq2d (src);

   // check for ladder
   const bool nonLadder = !(graph[index].flags & NodeFlag::Ladder) || distanceSq2d > cr::sqrf (16.0f);

   // is dest node higher than src? (62 is max jump height)
   if (nonLadder && dst.z > src.z + 62.0f) {
      return false; // can't reach this one
   }

   // is dest node lower than src?
   if (nonLadder && dst.z < src.z - 100.0f) {
      return false; // can't reach this one
   }

   // some one seems to camp at this node
   if (isOccupiedNode (index, true)) {
      return false; // can't reach this one
   }

   TraceResult tr {};
   game.testLine (src, dst, TraceIgnore::Monsters, ent (), &tr);

   // if node is visible from current position (even behind head)...
   return tr.flFraction >= 1.0f;
}

bool Bot::isPreviousLadder () {
   const auto prevNodeIndex = m_previousNodes[0];

   // bot entered ladder path
   return graph.exists (prevNodeIndex) && (graph[prevNodeIndex].flags & NodeFlag::Ladder);
}

void Bot::findShortestPath (int srcIndex, int destIndex) {
   // this function finds the shortest path from source index to destination index

   clearSearchNodes ();

   m_chosenGoalIndex = srcIndex;
   m_goalValue = 0.0f;

   bool success = planner.find (srcIndex, destIndex, [this] (int index) {
      m_pathWalk.add (index);
      return true;
   });

   if (!success) {
      m_prevGoalIndex = kInvalidNodeIndex;
      getTask ()->data = kInvalidNodeIndex;
   }
}

void Bot::syncFindPath (int srcIndex, int destIndex, FindPath pathType) {
   // this function finds a path from srcIndex to destIndex;

   if (!m_pathFindLock.tryLock ()) {
      return; // allow only single instance of syncFindPath per-bot
   }
   ScopedUnlock <Mutex> unlock (m_pathFindLock);

   if (!graph.exists (srcIndex)) {
      srcIndex = changeNodeIndex (graph.getNearestNoBuckets (pev->origin, 256.0f));

      if (!graph.exists (srcIndex)) {
         fprintf (stderr, "%s source path index not valid (%d).\n", __func__, srcIndex);
         return;
      }
   }
   else if (!graph.exists (destIndex) || destIndex == srcIndex) {
      destIndex = graph.getNearestNoBuckets (pev->origin, kInfiniteDistance, NodeFlag::Goal);

      if (!graph.exists (destIndex) || srcIndex == destIndex) {
         destIndex = graph.random ();

         if (!graph.exists (destIndex)) {
            fprintf (stderr, "%s dest path index not valid (%d).\n", __func__, destIndex);
            return;
         }
      }
   }

   // do not process if src points to dst
   if (srcIndex == destIndex) {
      fprintf (stderr, "%s source path is same as dest (%d).\n", __func__, destIndex);
      return;
   }

   // always use shortest-path algorithm when failed sanity checks within load
   if (planner.isPathsCheckFailed ()) {
      findShortestPath (srcIndex, destIndex);
      return;
   }

   // get correct calculation for heuristic
   if (pathType == FindPath::Optimal) {
      if (game.mapIs (MapFlags::HostageRescue) && m_hasHostage) {
         m_planner->setH (PlannerHeuristic::hfunctionPathDistWithHostage);
         m_planner->setG (PlannerHeuristic::gfunctionKillsDistCTWithHostage);
      }
      else {
         m_planner->setH (PlannerHeuristic::hfunctionPathDist);
         m_planner->setG (PlannerHeuristic::gfunctionKillsDist);
      }
   }
   else if (pathType == FindPath::Safe) {
      if (game.mapIs (MapFlags::HostageRescue) && m_hasHostage) {
         m_planner->setH (PlannerHeuristic::hfunctionNone);
         m_planner->setG (PlannerHeuristic::gfunctionKillsCTWithHostage);
      }
      else {
         m_planner->setH (PlannerHeuristic::hfunctionNone);
         m_planner->setG (PlannerHeuristic::gfunctionKills);
      }
   }
   else {
      if (game.mapIs (MapFlags::HostageRescue) && m_hasHostage) {
         m_planner->setH (PlannerHeuristic::hfunctionPathDistWithHostage);
         m_planner->setG (PlannerHeuristic::gfunctionPathDistWithHostage);
      }
      else {
         m_planner->setH (PlannerHeuristic::hfunctionPathDist);
         m_planner->setG (PlannerHeuristic::gfunctionPathDist);
      }
   }
   clearSearchNodes ();

   m_repathTimer.start (0.5f);
   m_chosenGoalIndex = srcIndex;
   m_goalValue = 0.0f;

   const auto result = m_planner->find (m_team, srcIndex, destIndex, [this] (int index) {
      m_pathWalk.add (index);
      return true;
   });

   // view the results
   switch (result) {
   case AStarResult::Success:
      m_pathWalk.reverse (); // reverse path for path follower
      break;

   case AStarResult::InternalError:
      m_kickMeFromServer = true; // bot should be kicked within main thread, not here

      // bot should not roam when this occurs
      fprintf (stderr, "A* Search for bot \"%s\" failed with internal pathfinder error. Seems to be graph is broken. Bot removed (re-added).\n", pev->netname.chars ());
      break;

   case AStarResult::Failed:
      // fallback to shortest path
      findShortestPath (srcIndex, destIndex); // A* found no path, try floyd pathfinder instead

      if (cv_debug) {
         fprintf (stderr, "A* Search for bot \"%s\" has failed. Falling back to shortest-path algorithm. Seems to be graph is broken.\n", pev->netname.chars ());
      }
      break;
   }
}

void Bot::findPath (int srcIndex, int destIndex, FindPath pathType /*= FindPath::Fast */) {
   // stale bots shouldn't do pathfinding
   if (m_isStale) {
      return;
   }

   worker.enqueue ([this, srcIndex, destIndex, pathType] () {
      syncFindPath (srcIndex, destIndex, pathType);
   });
}
