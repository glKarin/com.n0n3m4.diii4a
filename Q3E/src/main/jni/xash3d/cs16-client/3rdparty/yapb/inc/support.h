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

   // check if origin is visible from the entity side
   bool isVisible (const Vector &origin, edict_t *ent);

   // check if entity is alive
   bool isAlive (edict_t *ent);

   // checks if entity is fakeclient
   bool isFakeClient (edict_t *ent);

   // check if entity is a player
   bool isPlayer (edict_t *ent);

   // check if entity is a monster
   bool isMonster (edict_t *ent);

   // check if entity is a item
   bool isItem (edict_t *ent);

   // check if entity is a vip
   bool isPlayerVIP (edict_t *ent);

   // check if entity is a hostage entity
   bool isHostageEntity (edict_t *ent);

   // check if entity is a door entity
   bool isDoorEntity (edict_t *ent);

   // this function is checking that pointed by ent pointer obstacle, can be destroyed
   bool isBreakableEntity (edict_t *ent, bool initialSeed = false);

   // nearest player search helper
   bool findNearestPlayer (void **holder, edict_t *to, float searchDistance = 4096.0, bool sameTeam = false, bool needBot = false, bool needAlive = false, bool needDrawn = false, bool needBotWithC4 = false);

   // tracing decals for bots spraying logos
   void decalTrace (TraceResult *trace, int decalIndex);

   // update stats on clients
   void updateClients ();

   // checks if same model omitting the models directory
   bool isModel (const edict_t *ent, StringRef model);

   // get the current date and time as string
   String getCurrentDateTime ();

   // generates fake steam id from bot name
   StringRef getFakeSteamId (edict_t *ent);

   // get's the wave length
   float getWaveFileDuration (StringRef filename);

   // set custom cvar descriptions
   void setCustomCvarDescriptions ();

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
