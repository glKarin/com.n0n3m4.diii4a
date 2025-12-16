//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

class BotSupport final : public Singleton <BotSupport> {
   using AliasInfo = Twin <StringRef, StringRef>;
   using AliasMap = HashMap <int32_t, AliasInfo, EmptyHash <int32_t> >;

private:
   bool m_needToSendWelcome {};
   float m_welcomeReceiveTime {};

   StringArray m_sentences {};
   SmallArray <Client> m_clients {};

   AliasMap m_weaponAliases {};

public:
   BotSupport ();
   ~BotSupport () = default;

public:
   // need to send welcome message ?
   void checkWelcome ();

   // converts weapon id to alias name
   StringRef weaponIdToAlias (int32_t id);

   // nearest player search helper
   bool findNearestPlayer (void **holder, edict_t *to, float searchDistance = 4096.0, bool sameTeam = false, bool needBot = false, bool needAlive = false, bool needDrawn = false, bool needBotWithC4 = false);

   // tracing decals for bots spraying logos
   void decalTrace (TraceResult *trace, int decalIndex);

   // update stats on clients
   void updateClients ();

   // check if origin is visible from the entity side
   bool isVisible (const Vector &origin, edict_t *ent);

   // get the current date and time as string
   String getCurrentDateTime ();

   // generates fake steam id from bot name
   StringRef getFakeSteamId (edict_t *ent);

   // get's the wave length
   float getWaveFileDuration (StringRef filename);

   // set custom cvar descriptions
   void setCustomCvarDescriptions ();

   // check if line of sight blocked by a smoke
   bool isLineBlockedBySmoke (const Vector &from, const Vector &to);

public:

   // re-show welcome after changelevel ?
   void setNeedForWelcome (bool need) {
      m_needToSendWelcome = need;
      m_welcomeReceiveTime = -1.0f;
   }

   // get array of clients
   SmallArray <Client> &getClients () {
      return m_clients;
   }

   // get clients as const-reference
   const SmallArray <Client> &getClients () const {
      return m_clients;
   }

   // get single client as ref
   Client &getClient (const int index) {
      return m_clients[index];
   }

   // gets the shooting cone deviation
   float getConeDeviation (edict_t *ent, const Vector &pos) const {
      return ent->v.v_angle.forward () | (pos - (ent->v.origin + ent->v.view_ofs)).normalize (); // he's facing it, he meant it
   }

   // check if position is inside view cone of entity
   bool isInViewCone (const Vector &pos, edict_t *ent) const {
      return getConeDeviation (ent, pos) >= cr::cosf (cr::deg2rad ((ent->v.fov > 0 ? ent->v.fov : 90.0f) * 0.5f));
   }
};

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (BotSupport, util);
