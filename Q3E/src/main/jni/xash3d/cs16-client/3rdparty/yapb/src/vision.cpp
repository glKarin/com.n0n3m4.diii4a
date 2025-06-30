//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_max_nodes_for_predict ("max_nodes_for_predict", "22", "Maximum number for path length, to predict the enemy.", true, 15.0f, 256.0f);
ConVar cv_whose_your_daddy ("whose_your_daddy", "0", "Enables or disables extra hard difficulty for bots.");

// game console variables
ConVar mp_flashlight ("mp_flashlight", nullptr, Var::GameRef);

float Bot::isInFOV (const Vector &destination) {
   const float entityAngle = cr::wrapAngle360 (destination.yaw ()); // find yaw angle from source to destination...
   const float viewAngle = cr::wrapAngle360 (pev->v_angle.y); // get bot's current view angle...

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead, 45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle
   const float absAngle = cr::abs (viewAngle - entityAngle);

   if (absAngle > 180.0f) {
      return 360.0f - absAngle;
   }
   return absAngle;
}

bool Bot::isInViewCone (const Vector &origin) {
   // this function returns true if the spatial vector location origin is located inside
   // the field of view cone of the bot entity, false otherwise. It is assumed that entities
   // have a human-like field of view, that is, about 90 degrees.

   return util.isInViewCone (origin, ent ());
}

bool Bot::seesItem (const Vector &destination, StringRef classname) {
   TraceResult tr {};

   // trace a line from bot's eyes to destination..
   game.testLine (getEyesPos (), destination, TraceIgnore::None, ent (), &tr);

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction < 1.0f && tr.pHit && !tr.fStartSolid) {
      return classname == tr.pHit->v.classname.str ();
   }
   return true;
}

bool Bot::seesEntity (const Vector &dest, bool fromBody) {
   TraceResult tr {};

   // trace a line from bot's eyes to destination...
   game.testLine (fromBody ? pev->origin : getEyesPos (), dest, TraceIgnore::Everything, ent (), &tr);

   // check if line of sight to object is not blocked (i.e. visible)
   return tr.flFraction >= 1.0f;
}

void Bot::checkDarkness () {

   // do not check for darkness at the start of the round
   if (m_spawnTime + 5.0f > game.time () || !graph.exists (m_currentNodeIndex)) {
      return;
   }

   // do not check every frame
   if (m_checkDarkTime > game.time () || cr::fequal (m_path->light, kInvalidLightLevel)) {
      return;
   }

   const auto lightLevel = m_path->light;
   const auto skyColor = illum.getSkyColor ();
   const auto flashOn = (pev->effects & EF_DIMLIGHT);

   if (mp_flashlight && !m_hasNVG) {
      const auto tid = getCurrentTaskId ();

      if (!flashOn &&
         tid != Task::Camp
         && tid != Task::Attack
         && m_heardSoundTime + 3.0f < game.time ()
         && m_flashLevel > 30
         && ((skyColor > 50.0f && lightLevel < 10.0f) || (skyColor <= 50.0f && lightLevel < 40.0f))) {

         pev->impulse = 100;
      }
      else if (flashOn
         && (((lightLevel > 15.0f && skyColor > 50.0f) || (lightLevel > 45.0f && skyColor <= 50.0f))
            || tid == Task::Camp || tid == Task::Attack || m_flashLevel <= 0 || m_heardSoundTime + 3.0f >= game.time ())) {

         pev->impulse = 100;
      }
   }
   else if (m_hasNVG) {
      if (flashOn) {
         pev->impulse = 100;
      }
      else if (!m_usesNVG && ((skyColor > 50.0f && lightLevel < 15.0f) || (skyColor <= 50.0f && lightLevel < 40.0f))) {
         issueCommand ("nightvision");
      }
      else if (m_usesNVG && ((lightLevel > 20.0f && skyColor > 50.0f) || (lightLevel > 45.0f && skyColor <= 50.0f))) {
         issueCommand ("nightvision");
      }
   }
   m_checkDarkTime = game.time () + rg (2.0f, 4.0f);
}

void Bot::changePitch (float speed) {
   // this function turns a bot towards its ideal_pitch

   const float idealPitch = cr::wrapAngle (pev->idealpitch);
   const float curent = cr::wrapAngle (pev->v_angle.x);

   // turn from the current v_angle pitch to the idealpitch by selecting
   // the quickest way to turn to face that direction

   // find the difference in the curent and ideal angle
   float normalizePitch = cr::wrapAngle (idealPitch - curent);

   if (normalizePitch > 0.0f) {
      if (normalizePitch > speed) {
         normalizePitch = speed;
      }
   }
   else {
      if (normalizePitch < -speed) {
         normalizePitch = -speed;
      }
   }
   pev->v_angle.x = cr::wrapAngle (curent + normalizePitch);

   if (pev->v_angle.x > 89.9f) {
      pev->v_angle.x = 89.9f;
   }

   if (pev->v_angle.x < -89.9f) {
      pev->v_angle.x = -89.9f;
   }
   pev->angles.x = -pev->v_angle.x / 3;
}

void Bot::changeYaw (float speed) {
   // this function turns a bot towards its ideal_yaw

   const float idealPitch = cr::wrapAngle (pev->ideal_yaw);
   const float curent = cr::wrapAngle (pev->v_angle.y);

   // turn from the current v_angle yaw to the ideal_yaw by selecting
   // the quickest way to turn to face that direction

   // find the difference in the curent and ideal angle
   float normalizePitch = cr::wrapAngle (idealPitch - curent);

   if (normalizePitch > 0.0f) {
      if (normalizePitch > speed) {
         normalizePitch = speed;
      }
   }
   else {
      if (normalizePitch < -speed) {
         normalizePitch = -speed;
      }
   }
   pev->v_angle.y = cr::wrapAngle (curent + normalizePitch);
   pev->angles.y = pev->v_angle.y;
}

void Bot::updateBodyAngles () {
   constexpr float kValue = 1.0f / 3.0f;

   // set the body angles to point the gun correctly
   pev->angles.x = -pev->v_angle.x * kValue;
   pev->angles.y = pev->v_angle.y;

   pev->angles.clampAngles ();

   // calculate frustum plane data here, since look angles update functions call this last one
   frustum.calculate (m_viewFrustum, pev->v_angle, getEyesPos ());
}

void Bot::updateLookAngles () {
   const float delta = cr::clamp (game.time () - m_lookUpdateTime, cr::kFloatEqualEpsilon, kViewFrameUpdate);
   m_lookUpdateTime = game.time ();

   // adjust all body and view angles to face an absolute vector
   Vector direction = (m_lookAt - getEyesPos ()).angles ();
   direction.x = -direction.x; // invert for engine

   direction.clampAngles ();

   // lower skilled bot's have lower aiming
   if (m_difficulty == Difficulty::Noob) {
      updateLookAnglesNewbie (direction, delta);
      updateBodyAngles ();

      return;
   }

   // just force directioon
   if (m_difficulty == Difficulty::Expert
      && (m_aimFlags & AimFlags::Enemy)
      && (m_wantsToFire || usesSniper ())
      && cv_whose_your_daddy) {

      pev->v_angle = direction;
      pev->v_angle.clampAngles ();

      updateBodyAngles ();
      return;
   }
   const bool importantAimFlags = (m_aimFlags & (AimFlags::Enemy | AimFlags::Grenade));

   float accelerate = 3000.0f;
   float stiffness = 200.0f;
   float damping = 25.0f;

   if ((importantAimFlags || m_wantsToFire) && m_difficulty > Difficulty::Normal) {
      if (m_difficulty == Difficulty::Expert) {
         accelerate += 300.0f;
      }
      stiffness += 100.0f;
      damping -= 5.0f;
   }
   m_idealAngles = pev->v_angle;

   float angleDiffPitch = cr::anglesDifference (direction.x, m_idealAngles.x);
   float angleDiffYaw = cr::anglesDifference (direction.y, m_idealAngles.y);

   // prevent reverse facing angles  when navigating normally
   if (m_moveToGoal && !importantAimFlags && !m_pathOrigin.empty ()) {
      const float forward = (m_lookAt - pev->origin).yaw ();

      if (!cr::fzero (forward)) {
         const float current = cr::wrapAngle (pev->v_angle.y - forward);
         const float target = cr::wrapAngle (direction.y - forward);

         if (current * target < 0.0f) {
            if (cr::abs (current - target) >= 180.0f) {
               if (angleDiffYaw > 0.0f) {
                  angleDiffYaw -= 360.0f;
               }
               else {
                  angleDiffYaw += 360.0f;
               }
            }
         }
      }
   }

   if (cr::abs (angleDiffYaw) < 1.0f) {
      m_lookYawVel = 0.0f;
      m_idealAngles.y = direction.y;
   }
   else {
      const float accel = cr::clamp (stiffness * angleDiffYaw - damping * m_lookYawVel, -accelerate, accelerate);

      m_lookYawVel += delta * accel;
      m_idealAngles.y += delta * m_lookYawVel;
   }
   const float accel = cr::clamp (2.0f * stiffness * angleDiffPitch - damping * m_lookPitchVel, -accelerate, accelerate);

   m_lookPitchVel += delta * accel;
   m_idealAngles.x += delta * m_lookPitchVel;

   m_idealAngles.x = cr::clamp (m_idealAngles.x, -89.0f, 89.0f);

   pev->v_angle = m_idealAngles;
   pev->v_angle.z = 0.0f;

   updateBodyAngles ();
}

void Bot::updateLookAnglesNewbie (const Vector &direction, float delta) {
   Vector spring { 13.0f, 13.0f, 0.0f };
   Vector damperCoefficient { 0.22f, 0.22f, 0.0f };

   const float offset = cr::clamp (static_cast <float> (m_difficulty), 1.0f, 4.0f) * 25.0f;

   Vector influence = Vector (0.25f, 0.17f, 0.0f) * (100.0f - offset) / 100.f;
   Vector randomization = Vector (2.0f, 0.18f, 0.0f) * (100.0f - offset) / 100.f;

   const float noTargetRatio = 0.3f;
   const float offsetDelay = 1.2f;

   Vector stiffness {};

   m_idealAngles = direction.get2d ();
   m_idealAngles.clampAngles ();

   if (m_aimFlags & (AimFlags::Enemy | AimFlags::Entity)) {
      m_playerTargetTime = game.time ();
      m_randomizedIdealAngles = m_idealAngles;

      stiffness = spring * (0.2f + offset / 125.0f);
   }
   else {
      // is it time for bot to randomize the aim direction again (more often where moving) ?
      if (m_randomizeAnglesTime < game.time ()
         && ((pev->velocity.length () > 1.0f
            && m_angularDeviation.length () < 5.0f) || m_angularDeviation.length () < 1.0f)) {

         Vector randomize {};

         // is the bot standing still ?
         if (pev->velocity.length () < 1.0f) {
            randomize = randomization * 0.2f; // randomize less
         }
         else {
            randomize = randomization;
         }
         // randomize targeted location bit (slightly towards the ground)
         m_randomizedIdealAngles = m_idealAngles + Vector (rg (-randomize.x * 0.5f, randomize.x * 1.5f), rg (-randomize.y, randomize.y), 0.0f);

         // set next time to do this
         m_randomizeAnglesTime = game.time () + rg (0.4f, offsetDelay);
      }
      float stiffnessMultiplier = noTargetRatio;

      // take in account whether the bot was targeting someone in the last N seconds
      if (game.time () - (m_playerTargetTime + offsetDelay) < noTargetRatio * 10.0f) {
         stiffnessMultiplier = 1.0f - (game.time () - m_timeLastFired) * 0.1f;

         // don't allow that stiffness multiplier less than zero
         if (stiffnessMultiplier < 0.0f) {
            stiffnessMultiplier = 0.5f;
         }
      }

      // also take in account the remaining deviation (slow down the aiming in the last 10°)
      stiffnessMultiplier *= m_angularDeviation.length () * 0.1f * 0.5f;

      // but don't allow getting below a certain value
      if (stiffnessMultiplier < 0.35f) {
         stiffnessMultiplier = 0.35f;
      }
      stiffness = spring * stiffnessMultiplier; // increasingly slow aim
   }
   // compute randomized angle deviation this time
   m_angularDeviation = m_randomizedIdealAngles - pev->v_angle;
   m_angularDeviation.clampAngles ();

   // spring/damper model aiming
   m_aimSpeed.x = stiffness.x * m_angularDeviation.x - damperCoefficient.x * m_aimSpeed.x;
   m_aimSpeed.y = stiffness.y * m_angularDeviation.y - damperCoefficient.y * m_aimSpeed.y;

   // influence of y movement on x axis and vice versa (less influence than x on y since it's
   // easier and more natural for the bot to "move its mouse" horizontally than vertically)
   m_aimSpeed.x += cr::clamp (m_aimSpeed.y * influence.y, -50.0f, 50.0f);
   m_aimSpeed.y += cr::clamp (m_aimSpeed.x * influence.x, -200.0f, 200.0f);

   // move the aim cursor
   pev->v_angle = pev->v_angle + delta * Vector (m_aimSpeed.x, m_aimSpeed.y, 0.0f);
   pev->v_angle.clampAngles ();
}

bool Frustum::isObjectInsidePlane (const Plane &plane, const Vector &center, float height, float radius) const {
   auto isPointInsidePlane = [&] (const Vector &point) -> bool {
      return plane.result + (plane.normal | point) >= 0.0f;
   };

   const auto &test = plane.normal.get2d ();
   const auto &top = center + Vector (0.0f, 0.0f, height * 0.5f) + test * radius;
   const auto &bottom = center - Vector (0.0f, 0.0f, height * 0.5f) + test * radius;

   return isPointInsidePlane (top) || isPointInsidePlane (bottom);
}

void Frustum::calculate (Planes &planes, const Vector &viewAngle, const Vector &viewOffset) {
   Vector forward {}, right {}, up {};
   viewAngle.angleVectors (&forward, &right, &up);

   auto fc = viewOffset + forward * kMaxViewDistance;
   auto nc = viewOffset + forward * kMinViewDistance;

   auto up_half_far = up * m_farHeight * 0.5f;
   auto right_half_far = right * m_farWidth * 0.5f;
   auto up_half_near = up * m_nearHeight * 0.5f;
   auto right_half_near = right * m_nearWidth * 0.5f;

   auto fbl = fc - right_half_far + up_half_far;
   auto fbr = fc + right_half_far + up_half_far;
   auto ftl = fc - right_half_far - up_half_far;
   auto ftr = fc + right_half_far - up_half_far;
   auto nbl = nc - right_half_near + up_half_near;
   auto nbr = nc + right_half_near + up_half_near;
   auto ntl = nc - right_half_near - up_half_near;
   auto ntr = nc + right_half_near - up_half_near;

   auto setPlane = [&] (PlaneSide side, const Vector &v1, const Vector &v2, const Vector &v3) {
      auto &plane = planes[static_cast <int> (side)];

      plane.normal = ((v2 - v1) ^ (v3 - v1)).normalize ();
      plane.point = v2;

      plane.result = -(plane.normal | plane.point);
   };

   setPlane (PlaneSide::Top, ftl, ntl, ntr);
   setPlane (PlaneSide::Bottom, fbr, nbr, nbl);
   setPlane (PlaneSide::Left, fbl, nbl, ntl);
   setPlane (PlaneSide::Right, ftr, ntr, nbr);
   setPlane (PlaneSide::Near, nbr, ntr, ntl);
   setPlane (PlaneSide::Far, fbl, ftl, ftr);
}

bool Frustum::check (const Planes &planes, edict_t *ent) const {
   constexpr auto kOffset = Vector (0.0f, 0.0f, 5.0f);
   const auto &origin = ent->v.origin - kOffset;

   for (const auto &plane : planes) {
      if (!isObjectInsidePlane (plane, origin, 60.0f, 16.0f)) {
         return false;
      }
   }
   return true;
}

void Bot::setAimDirection () {
   uint32_t flags = m_aimFlags;

   // don't allow bot to look at danger positions under certain circumstances
   if (!(flags & (AimFlags::Grenade | AimFlags::Enemy | AimFlags::Entity))) {

      // check if narrow place and we're duck, do not predict enemies in that situation
      const bool duckedInNarrowPlace = isInNarrowPlace () && ((m_pathFlags & NodeFlag::Crouch) || (pev->button & IN_DUCK));

      if (duckedInNarrowPlace || isOnLadder () || isInWater () || (m_pathFlags & NodeFlag::Ladder) || (m_currentTravelFlags & PathFlag::Jump)) {
         flags &= ~(AimFlags::LastEnemy | AimFlags::PredictPath);
         m_canChooseAimDirection = false;
      }

      // don't switch view right away after loosing focus with current enemy 
      if ((m_shootTime + 1.5f > game.time () || m_seeEnemyTime + 1.5f > game.time ())
         && m_forgetLastVictimTimer.elapsed ()
         && !m_lastEnemyOrigin.empty ()
         && util.isAlive (m_lastEnemy)
         && game.isNullEntity (m_enemy)) {

         flags |= AimFlags::LastEnemy;
      }
   }

   if (flags & AimFlags::Override) {
      m_lookAt = m_lookAtSafe;
   }
   else if (flags & AimFlags::Grenade) {
      m_lookAt = m_throw;

      const float throwDistance = m_throw.distance (pev->origin);
      float coordCorrection = 0.0f;

      if (throwDistance > 100.0f && throwDistance < 800.0f) {
         coordCorrection = 0.25f * (m_throw.z - pev->origin.z);
      }
      else if (throwDistance >= 800.0f) {
         float angleCorrection = 37.0f * (throwDistance - 800.0f) / 800.0f;

         if (angleCorrection > 45.0f) {
            angleCorrection = 45.0f;
         }
         coordCorrection = throwDistance * cr::tanf (cr::deg2rad (angleCorrection)) + 0.25f * (m_throw.z - pev->origin.z);
      }
      m_lookAt.z += coordCorrection * 0.5f;
   }
   else if (flags & AimFlags::Enemy) {
      focusEnemy ();
   }
   else if (flags & AimFlags::Entity) {
      m_lookAt = m_entity;

      // do not look at hostages legs
      if (m_pickupType == Pickup::Hostage) {
         m_lookAt.z += 48.0f;
      }
      else if (m_pickupType == Pickup::Weapon) {
         m_lookAt.z += 72.0f;
      }
   }
   else if (flags & AimFlags::LastEnemy) {
      m_lookAt = m_lastEnemyOrigin;

      // did bot just see enemy and is quite aggressive?
      if (m_seeEnemyTime + 2.0f - m_actualReactionTime + m_baseAgressionLevel > game.time ()) {

         // feel free to fire if shootable
         if (!usesSniper () && lastEnemyShootable ()) {
            m_wantsToFire = true;
         }
      }
   }
   else if (flags & AimFlags::PredictPath) {
      bool changePredictedEnemy = true;

      if (m_timeNextTracking < game.time () && m_trackingEdict == m_lastEnemy && util.isAlive (m_lastEnemy)) {
         changePredictedEnemy = false;
      }

      auto doFailPredict = [this] () -> void {
         if (m_lastPredictIndex != m_currentNodeIndex && m_timeNextTracking + 0.5f > game.time ()) {
            return; // do not fail instantly
         }
         m_aimFlags &= ~AimFlags::PredictPath;

         m_trackingEdict = nullptr;
         m_lookAtPredict.clear ();
      };

      auto pathLength = m_lastPredictLength;
      auto predictNode = m_lastPredictIndex;

      auto isPredictedIndexApplicable = [&] () -> bool {
         if (!graph.exists (predictNode)) {
            return false;
         }
         TraceResult result {};
         game.testLine (getEyesPos (), graph[predictNode].origin + pev->view_ofs, TraceIgnore::None, ent (), &result);

         if (result.flFraction < 0.5f) {
            return false;
         }
         const float distToPredictNodeSq = graph[predictNode].origin.distanceSq (pev->origin);

         if (distToPredictNodeSq >= cr::sqrf (2048.0f)) {
            return false;
         }

         if (!vistab.visible (m_currentNodeIndex, predictNode) || !vistab.visible (m_previousNodes[0], predictNode)) {
            predictNode = kInvalidNodeIndex;
            pathLength = kInfiniteDistanceLong;

            return false;
         }
         return isNodeValidForPredict (predictNode) && pathLength < cv_max_nodes_for_predict.as <int> ();
      };

      if (changePredictedEnemy) {
         if (isPredictedIndexApplicable ()) {
            m_lookAtPredict = graph[predictNode].origin;

            m_timeNextTracking = game.time () + 0.75f;
            m_trackingEdict = m_lastEnemy;
         }
         else {
            doFailPredict ();
         }
      }
      else {
         if (!isPredictedIndexApplicable ()) {
            doFailPredict ();
         }
      }

      if (!m_lookAtPredict.empty ()) {
         m_lookAt = m_lookAtPredict;
      }
   }
   else if (flags & AimFlags::Camp) {
      m_lookAt = m_lookAtSafe;
   }
   else if (flags & AimFlags::Nav) {
      const auto &destOrigin = m_destOrigin + pev->view_ofs;
      m_lookAt = destOrigin;

      if (m_moveToGoal && m_seeEnemyTime + 4.0f < game.time ()
         && !m_isStuck && !(pev->button & IN_DUCK)
         && m_currentNodeIndex != kInvalidNodeIndex
         && !(m_pathFlags & (NodeFlag::Ladder | NodeFlag::Crouch))
         && m_pathWalk.hasNext () && !isOnLadder ()
         && pev->origin.distanceSq (destOrigin) < cr::sqrf (512.0f)) {

         const auto nextPathIndex = m_pathWalk.next ();
         const auto nextPathX2 = m_pathWalk.nextX2 ();

         if (vistab.visible (m_currentNodeIndex, nextPathX2)) {
            const auto &gn = graph[nextPathX2];
            m_lookAt = gn.origin + pev->view_ofs;
         }
         else if (vistab.visible (m_currentNodeIndex, nextPathIndex)) {
            const auto &gn = graph[nextPathIndex];
            m_lookAt = gn.origin + pev->view_ofs;
         }
         else {
            m_lookAt = pev->origin + pev->view_ofs + pev->v_angle.forward () * 300.0f;
         }
      }
      else {
         m_lookAt = destOrigin;
      }
      const bool horizontalMovement = (m_pathFlags & NodeFlag::Ladder) || isOnLadder ();

      if (m_numEnemiesLeft > 0
         && m_canChooseAimDirection
         && m_seeEnemyTime + 4.0f < game.time ()
         && m_currentNodeIndex != kInvalidNodeIndex
         && !horizontalMovement) {

         const auto dangerIndex = practice.getIndex (m_team, m_currentNodeIndex, m_currentNodeIndex);

         if (graph.exists (dangerIndex)
            && vistab.visible (m_currentNodeIndex, dangerIndex)
            && !(graph[dangerIndex].flags & NodeFlag::Crouch)) {

            if (pev->origin.distanceSq (graph[dangerIndex].origin) < cr::sqrf (512.0f)) {
               m_lookAt = destOrigin;
            }
            else {
               m_lookAt = graph[dangerIndex].origin + pev->view_ofs;

               // add danger flags
               m_aimFlags |= AimFlags::Danger;
            }
         }
      }

      // try look at next node if on ladder
      if (horizontalMovement && m_pathWalk.hasNext ()) {
         const auto &nextPath = graph[m_pathWalk.next ()];

         if ((nextPath.flags & NodeFlag::Ladder) && m_destOrigin.distanceSq (pev->origin) < cr::sqrf (128.0f) && nextPath.origin.z > m_pathOrigin.z + 26.0f) {
            m_lookAt = nextPath.origin + pev->view_ofs;
         }
      }

      // don't look at bottom of node, if reached it
      if (m_lookAt == destOrigin && !horizontalMovement) {
         m_lookAt.z = getEyesPos ().z;
      }

      // try to look at last victim for a little, maybe there's some one else
      if (game.isNullEntity (m_enemy) && m_difficulty >= Difficulty::Normal && !m_forgetLastVictimTimer.elapsed () && !m_lastVictimOrigin.empty ()) {
         m_lookAt = m_lastVictimOrigin + pev->view_ofs;
      }
   }

   if (m_lookAt.empty ()) {
      m_lookAt = m_destOrigin;
   }
}
