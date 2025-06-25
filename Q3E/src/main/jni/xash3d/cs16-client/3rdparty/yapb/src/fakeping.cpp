//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_ping_base_min ("ping_base_min", "5", "Lower bound for base bot ping shown in scoreboard upon creation.", true, 0.0f, 100.0f);
ConVar cv_ping_base_max ("ping_base_max", "20", "Upper bound for base bot ping shown in scoreboard upon creation.", true, 0.0f, 100.0f);
ConVar cv_ping_count_real_players ("ping_count_real_players", "1", "Count player pings when calculating average ping for bots. If no, some random ping chosen for bots.");
ConVar cv_ping_updater_interval ("ping_updater_interval", "1.25", "Interval in which fakeping get updated in scoreboard.", true, 0.1f, 10.0f);

bool BotFakePingManager::hasFeature () const {
   return game.is (GameFlags::HasFakePings) && cv_show_latency.as <int> () >= 2;
}

void BotFakePingManager::reset (edict_t *to) {

   // no reset if game isn't support them
   if (!hasFeature ()) {
      return;
   }

   for (const auto &client : util.getClients ()) {
      if (!(client.flags & ClientFlags::Used) || util.isFakeClient (client.ent)) {
         continue;
      }
      m_pbm.start (client.ent);

      m_pbm.write (1, PingBitMsg::Single);
      m_pbm.write (game.indexOfPlayer (to), PingBitMsg::PlayerID);
      m_pbm.write (0, PingBitMsg::Ping);
      m_pbm.write (0, PingBitMsg::Loss);

      m_pbm.send ();
   }
   m_pbm.flush ();
}

void BotFakePingManager::syncCalculate () {
   MutexScopedLock lock (m_cs);

   int averagePing {};

   if (cv_ping_count_real_players) {
      int numHumans {};

      for (const auto &client : util.getClients ()) {
         if (!(client.flags & ClientFlags::Used) || util.isFakeClient (client.ent)) {
            continue;
         }
         numHumans++;

         int ping {}, loss {};
         engfuncs.pfnGetPlayerStats (client.ent, &ping, &loss);

         averagePing += ping > 0 && ping < 200 ? ping : randomBase ();
      }

      if (numHumans > 0) {
         averagePing /= numHumans;
      }
      else {
         averagePing = randomBase ();
      }
   }
   else {
      averagePing = randomBase ();
   }

   for (auto &bot : bots) {
      const auto diff = static_cast <int> (static_cast <float> (averagePing) * 0.2f);

      // randomize bot ping
      auto botPing = static_cast <float> (bot->m_pingBase + rg (averagePing - diff, averagePing + diff) + rg (bot->m_difficulty + 3, bot->m_difficulty + 6));

      if (botPing < 5.0f) {
         botPing = rg (10.0f, 15.0f);
      }
      else if (botPing > 75.0f) {
         botPing = rg (30.0f, 40.0f);
      }
      bot->m_ping = static_cast <int> (static_cast <float> (bot->entindex () % 2 == 0 ? botPing * 0.25f : botPing * 0.5f));
   }
}

void BotFakePingManager::calculate () {
   if (!hasFeature ()) {
      return;
   }

   // throttle updating
   if (!m_recalcTime.elapsed ()) {
      return;
   }
   restartTimer ();

   worker.enqueue ([this] () {
      syncCalculate ();
   });
}

void BotFakePingManager::emit (edict_t *ent) {
   if (!util.isPlayer (ent)) {
      return;
   }

   for (const auto &bot : bots) {
      m_pbm.start (ent);

      m_pbm.write (1, PingBitMsg::Single);
      m_pbm.write (bot->entindex () - 1, PingBitMsg::PlayerID);
      m_pbm.write (bot->m_ping, PingBitMsg::Ping);
      m_pbm.write (0, PingBitMsg::Loss);

      m_pbm.send ();
   }
   m_pbm.flush ();
}

void BotFakePingManager::restartTimer () {
   m_recalcTime.start (cv_ping_updater_interval.as <float> ());
}

int BotFakePingManager::randomBase () const {
   return rg (cv_ping_base_min.as <int> (), cv_ping_base_max.as <int> ());
}

