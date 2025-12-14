//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_autovacate ("autovacate", "1", "Kicks bots to automatically make room for human players.");
ConVar cv_autovacate_keep_slots ("autovacate_keep_slots", "1", "How many slots the autovacate feature should keep for human players.", true, 1.0f, 8.0f);
ConVar cv_kick_after_player_connect ("kick_after_player_connect", "1", "Kicks the bot immediately when a human player joins the server (yb_autovacate must be enabled).");

ConVar cv_quota ("quota", "9", "Specifies the number of bots to be added to the game.", true, 0.0f, static_cast <float> (kGameMaxPlayers));
ConVar cv_quota_mode ("quota_mode", "normal", "Specifies the type of quota.\nAllowed values: 'normal', 'fill', and 'match'.\nIf 'fill', the server will adjust bots to keep N players in the game, where N is yb_quota.\nIf 'match', the server will maintain a 1:N ratio of humans to bots, where N is yb_quota_match.", false);
ConVar cv_quota_match ("quota_match", "0", "Number of players to match if yb_quota_mode is set to 'match'.", true, 0.0f, static_cast <float> (kGameMaxPlayers));
ConVar cv_think_fps ("think_fps", "30.0", "Specifies how many times per second the bot code will run.", true, 24.0f, 90.0f);
ConVar cv_think_fps_disable ("think_fps_disable", "1", "Allows to completely disable think fps on Xash3D.", true, 0.0f, 1.0f, Var::Xash3D);

ConVar cv_autokill_delay ("autokill_delay", "0.0", "Specifies the amount of time in seconds after which bots will be killed if no humans are left alive.", true, 0.0f, 90.0f);
ConVar cv_first_human_restart ("first_human_restart", "0", "Restart the game if the first human player joins a bot game.");

ConVar cv_join_after_player ("join_after_player", "0", "Specifies whether bots should join the server only when at least one human player is in the game.");
ConVar cv_join_team ("join_team", "any", "Forces all bots to join the team specified here.", false);
ConVar cv_join_delay ("join_delay", "5.0", "Specifies after how many seconds bots should start to join the game after the changelevel.", true, 0.0f, 30.0f);
ConVar cv_name_prefix ("name_prefix", "", "All bot names will be prefixed with the string specified by this cvar.", false);

ConVar cv_difficulty ("difficulty", "3", "All bots difficulty level. Changing at runtime will affect already created bots.", true, 0.0f, 4.0f);

ConVar cv_difficulty_min ("difficulty_min", "-1", "Lower bound of random difficulty on bot creation. Only affects newly created bots. -1 means only yb_difficulty is used.", true, -1.0f, 4.0f);
ConVar cv_difficulty_max ("difficulty_max", "-1", "Upper bound of random difficulty on bot creation. Only affects newly created bots. -1 means only yb_difficulty is used.", true, -1.0f, 4.0f);
ConVar cv_difficulty_auto ("difficulty_auto", "0", "Allows each bot to balance its own difficulty based on the kd-ratio of the team.", true, 0.0f, 1.0f);
ConVar cv_difficulty_auto_balance_interval ("difficulty_auto_balance_interval", "30", "Interval at which bots will balance their difficulty.", true, 30.0f, 240.0f);

ConVar cv_show_avatars ("show_avatars", "0", "Enables or disables displaying bot avatars in front of their names in the scoreboard. Note that currently you can only see avatars of your Steam friends.");
ConVar cv_show_latency ("show_latency", "0", "Enables latency display in the scoreboard.\nAllowed values: '0', '1', '2'.\nIf '0', there is nothing displayed.\nIf '1', there is a 'BOT' is displayed.\nIf '2' fake ping is displayed.", true, 0.0f, 2.0f);

ConVar cv_save_bots_names ("save_bots_names", "1", "Allows saving bot names upon changelevel, so bot names will be the same after a map change.", true, 0.0f, 1.0f);

ConVar cv_botskin_t ("botskin_t", "0", "Specifies the bot's wanted skin for the Terrorist team.", true, 0.0f, 5.0f);
ConVar cv_botskin_ct ("botskin_ct", "0", "Specifies the bot's wanted skin for the CT team.", true, 0.0f, 5.0f);
ConVar cv_preferred_personality ("preferred_personality", "none", "Sets the default personality when creating bots with quota management.\nAllowed values: 'none', 'normal', 'careful', 'rusher'.\nIf 'none' is specified personality chosen randomly.", false);

ConVar cv_quota_adding_interval ("quota_adding_interval", "0.10", "Interval at which bots are added to the game.", true, 0.10f, 1.0f);
ConVar cv_quota_maintain_interval ("quota_maintain_interval", "0.40", "Interval at which the overall bot quota is checked.", true, 0.40f, 2.0f);

ConVar cv_language ("language", "en", "Specifies the language for bot messages and menus.", false);

ConVar cv_rotate_bots ("rotate_bots", "0", "Randomly disconnects and connects bots, simulating players joining/quitting.");
ConVar cv_rotate_stay_min ("rotate_stay_min", "360.0", "Specifies the minimum amount of seconds a bot stays connected, if rotation is active.", true, 120.0f, 7200.0f);
ConVar cv_rotate_stay_max ("rotate_stay_max", "3600.0", "Specifies the maximum amount of seconds a bot stays connected, if rotation is active.", true, 1800.0f, 14400.0f);

ConVar cv_restricted_weapons ("restricted_weapons", "", "", false);

ConVar mp_limitteams ("mp_limitteams", nullptr, Var::GameRef);
ConVar mp_autoteambalance ("mp_autoteambalance", nullptr, Var::GameRef);
ConVar mp_roundtime ("mp_roundtime", nullptr, Var::GameRef);
ConVar mp_timelimit ("mp_timelimit", nullptr, Var::GameRef);
ConVar mp_freezetime ("mp_freezetime", nullptr, Var::GameRef, true, "0");

BotManager::BotManager () {
   // this is a bot manager class constructor

   for (auto &td : m_teamData) {
      td.leaderChoosen = false;
      td.positiveEco = true;
      td.lastRadioSlot = kInvalidRadioSlot;
      td.lastRadioTimestamp = 0.0f;
   }
   reset ();

   m_addRequests.clear ();
   m_killerEntity = nullptr;

   initFilters ();
}

void BotManager::createKillerEntity () {
   // this function creates single trigger_hurt for using in Bot::kill, to reduce lags, when killing all the bots

   m_killerEntity = engfuncs.pfnCreateNamedEntity ("trigger_hurt");

   m_killerEntity->v.dmg = kInfiniteDistance;
   m_killerEntity->v.dmg_take = 1.0f;
   m_killerEntity->v.dmgtime = 2.0f;
   m_killerEntity->v.effects |= EF_NODRAW;

   engfuncs.pfnSetOrigin (m_killerEntity, Vector (-kInfiniteDistance, -kInfiniteDistance, -kInfiniteDistance));
   MDLL_Spawn (m_killerEntity);
}

void BotManager::destroyKillerEntity () {
   if (!game.isNullEntity (m_killerEntity)) {
      engfuncs.pfnRemoveEntity (m_killerEntity);
      m_killerEntity = nullptr;
   }
}

void BotManager::touchKillerEntity (Bot *bot) {

   // bot is already dead.
   if (!bot->m_isAlive) {
      return;
   }

   if (game.isNullEntity (m_killerEntity)) {
      createKillerEntity ();

      if (game.isNullEntity (m_killerEntity)) {
         MDLL_ClientKill (bot->ent ());
         return;
      }
   }
   const auto &prop = conf.getWeaponProp (bot->m_currentWeapon);

   m_killerEntity->v.classname = prop.classname.chars ();
   m_killerEntity->v.dmg_inflictor = bot->ent ();
   m_killerEntity->v.dmg = (bot->pev->health + bot->pev->armorvalue) * 4.0f;

   KeyValueData kv {};
   kv.szClassName = prop.classname.chars ();
   kv.szKeyName = "damagetype";
   kv.szValue = strings.format ("%d", cr::bit (4));
   kv.fHandled = HLFalse;

   MDLL_KeyValue (m_killerEntity, &kv);
   MDLL_Touch (m_killerEntity, bot->ent ());
}

void BotManager::execGameEntity (edict_t *ent) {
   // this function calls gamedll player() function, in case to create player entity in game

   if (game.is (GameFlags::Metamod)) {
      MUTIL_CallGameEntity (PLID, "player", &ent->v);
      return;
   }

   if (!entlink.callPlayerFunction (ent)) {
      for (const auto &bot : m_bots) {
         if (bot->ent () == ent) {
            bot->kick ();
            break;
         }
      }
   }
}

void BotManager::forEach (ForEachBot handler) {
   for (const auto &bot : m_bots) {
      if (handler (bot.get ())) {
         return;
      }
   }
}

BotCreateResult BotManager::create (StringRef name, int difficulty, int personality, int team, int skin) {
   // this function completely prepares bot entity (edict) for creation, creates team, difficulty, sets named etc, and
   // then sends result to bot constructor

   edict_t *bot = nullptr;
   String resultName {};

   // do not allow create bots when there is no graph
   if (!graph.length ()) {
      ctrl.msg ("There is no graph found. Cannot create bot.");
      return BotCreateResult::GraphError;
   }

   // don't allow creating bots with changed graph (distance tables are messed up)
   else if (graph.hasChanged ()) {
      ctrl.msg ("Graph has been changed. Load graph again...");
      return BotCreateResult::GraphError;
   }
   else if (team != -1 && isTeamStacked (team - 1)) {
      ctrl.msg ("Desired team is stacked. Unable to proceed with bot creation.");
      return BotCreateResult::TeamStacked;
   }
   if (difficulty < Difficulty::Noob || difficulty > Difficulty::Expert) {
      difficulty = cv_difficulty.as <int> ();

      if (difficulty < Difficulty::Noob || difficulty > Difficulty::Expert) {
         difficulty = rg (3, 4);
         cv_difficulty.set (difficulty);
      }
   }

   // try to set proffered personality
   static HashMap <String, Personality> personalityMap {
      { "normal", Personality::Normal },
      { "careful", Personality::Careful },
      { "rusher", Personality::Rusher },
   };

   // set personality if requested
   if (personality < Personality::Normal || personality > Personality::Careful) {

      // assign preferred if we're forced with cvar
      if (personalityMap.exists (cv_preferred_personality.as <StringRef> ())) {
         personality = personalityMap[cv_preferred_personality.as <StringRef> ()];
      }

      // do a holy random
      else {
         if (rg.chance (50)) {
            personality = Personality::Normal;
         }
         else {
            if (rg.chance (50)) {
               personality = Personality::Rusher;
            }
            else {
               personality = Personality::Careful;
            }
         }
      }
   }
   BotName *botName = nullptr;

   // setup name
   if (name.empty ()) {
      botName = conf.pickBotName ();

      if (botName) {
         resultName = botName->name;
      }
      else {
         resultName.assignf ("%s_%d.%d", product.nameLower, rg (100, 10000), rg (100, 10000)); // just pick ugly random name
      }
   }
   else {
      resultName = name;
   }
   const bool hasNamePrefix = !cv_name_prefix.as <StringRef> ().empty ();

   // disable save bots names if prefix is enabled
   if (hasNamePrefix && cv_save_bots_names) {
      cv_save_bots_names.set (0);
   }

   if (hasNamePrefix) {
      String prefixed {}; // temp buffer for storing modified name
      prefixed.assignf ("%s %s", cv_name_prefix.as <StringRef> (), resultName);

      // buffer has been modified, copy to real name
      resultName = cr::move (prefixed);
   }
   bot = game.createFakeClient (resultName);

   if (game.isNullEntity (bot)) {
      return BotCreateResult::MaxPlayersReached;
   }
   auto object = cr::makeUnique <Bot> (bot, difficulty, personality, team, skin);
   const auto index = object->index ();

   // assign owner of bot name
   if (botName != nullptr) {
      botName->usedBy = index; // save by who name is used
   }
   else {
      conf.setBotNameUsed (index, resultName);
   }
   m_bots.push (cr::move (object));

   ctrl.msg ("Connecting Bot...");

   return BotCreateResult::Success;
}

Bot *BotManager::findBotByIndex (int index) {
   // this function finds a bot specified by index, and then returns pointer to it (using own bot array)

   if (index < 0 || index >= kGameMaxPlayers) {
      return nullptr;
   }

   for (const auto &bot : m_bots) {
      if (bot->m_index == index) {
         return bot.get ();
      }
   }
   return nullptr; // no bot
}

Bot *BotManager::findBotByEntity (edict_t *ent) {
   // same as above, but using bot entity

   return findBotByIndex (game.indexOfPlayer (ent));
}

Bot *BotManager::findAliveBot () {
   // this function finds one bot, alive bot :)

   for (const auto &bot : m_bots) {
      if (bot->m_isAlive) {
         return bot.get ();
      }
   }
   return nullptr;
}

void BotManager::frame () {
   // this function calls showframe function for all available at call moment bots

   for (const auto &bot : m_bots) {
      bot->frame ();
   }
}

void BotManager::addbot (StringRef name, int difficulty, int personality, int team, int skin, bool manual) {
   // this function putting bot creation process to queue to prevent engine crashes

   BotRequest request {};

   // fill the holder
   request.name = name;
   request.difficulty = difficulty;
   request.personality = personality;
   request.team = team;
   request.skin = skin;
   request.manual = manual;

   // restore the bot name
   if (cv_save_bots_names && name.empty () && !m_saveBotNames.empty ()) {
      request.name = m_saveBotNames.popFront ();
   }

   // put to queue
   m_addRequests.emplaceLast (cr::move (request));
}

void BotManager::addbot (StringRef name, StringRef difficulty, StringRef personality, StringRef team, StringRef skin, bool manual) {
   // this function is same as the function above, but accept as parameters string instead of integers

   BotRequest request {};
   constexpr StringRef ANY = "*";

   auto handleParam = [&ANY] (StringRef value) {
      return value.empty () || value == ANY ? -1 : value.as <int> ();
   };

   request.name = name.empty () || name == ANY ? StringRef ("\0") : name;
   request.difficulty = handleParam (difficulty);
   request.team = handleParam (team);
   request.skin = handleParam (skin);
   request.personality = handleParam (personality);
   request.manual = manual;

   addbot (request.name, request.difficulty, request.personality, request.team, request.skin, request.manual);
}

void BotManager::maintainQuota () {
   // this function keeps number of bots up to date, and don't allow to maintain bot creation
   // while creation process in process.

   if (!m_holdQuotaManagementTimer.elapsed ()) {
      return;
   }

   if (graph.length () < 1 || graph.hasChanged ()) {
      if (cv_quota.as <int> () > 0) {
         ctrl.msg ("There is no graph found. Cannot create bot.");
      }
      cv_quota.set (0);
      return;
   }

   if (analyzer.isAnalyzing ()) {
      ctrl.msg ("Can't create bot during map analysis process.");
      return;
   }
   const int maxClients = game.maxClients ();
   const int botsInGame = getBotCount ();

   // bot's creation update
   if (!m_addRequests.empty () && m_maintainTime < game.time ()) {
      const auto &request = m_addRequests.popFront ();
      const auto createResult = create (request.name, request.difficulty, request.personality, request.team, request.skin);

      if (request.manual) {
         cv_quota.set (cr::min (cv_quota.as <int> () + 1, maxClients));
      }

      // check the result of creation
      if (createResult == BotCreateResult::GraphError) {
         m_addRequests.clear (); // something wrong with graph, reset tab of creation
         cv_quota.set (0); // reset quota
      }
      else if (createResult == BotCreateResult::MaxPlayersReached) {
         ctrl.msg ("Maximum players reached (%d/%d). Unable to create Bot.", maxClients, maxClients);

         m_addRequests.clear (); // maximum players reached, so set quota to maximum players
         cv_quota.set (botsInGame);
      }
      else if (createResult == BotCreateResult::TeamStacked) {
         ctrl.msg ("Could not add bot to the game: Team is stacked (to disable this check, set mp_limitteams and mp_autoteambalance to zero and restart the round)");

         m_addRequests.clear ();
         cv_quota.set (botsInGame);
      }
      m_maintainTime = game.time () + cv_quota_adding_interval.as <float> ();
   }

   // now keep bot number up to date
   if (m_quotaMaintainTime > game.time ()) {
      return;
   }
   int desiredBotCount = cv_quota.as <int> ();

   // only assign if out of range
   if (desiredBotCount < 0 || desiredBotCount > maxClients) {
      cv_quota.set (cr::clamp (desiredBotCount, 0, maxClients));
   }
   const int totalHumansInGame = getHumansCount ();
   const int humanPlayersInGame = getHumansCount (true);

   if (!game.isDedicated () && !totalHumansInGame) {
      return;
   }

   if (cv_quota_mode.as <StringRef> () == "fill") {
      desiredBotCount = cr::max (0, desiredBotCount - humanPlayersInGame);
   }
   else if (cv_quota_mode.as <StringRef> () == "match") {
      const int detectQuotaMatch = cv_quota_match.as <int> () == 0 ? cv_quota.as <int> () : cv_quota_match.as <int> ();

      desiredBotCount = cr::max (0, detectQuotaMatch * humanPlayersInGame);
   }

   if (cv_join_after_player && humanPlayersInGame == 0) {
      desiredBotCount = 0;
   }

   if (cv_autovacate) {
      if (cv_kick_after_player_connect) {
         desiredBotCount = cr::min (desiredBotCount, maxClients - (totalHumansInGame + cv_autovacate_keep_slots.as <int> ()));
      }
      else {
         desiredBotCount = cr::min (desiredBotCount, maxClients - (humanPlayersInGame + cv_autovacate_keep_slots.as <int> ()));
      }
   }
   else {
      desiredBotCount = cr::min (desiredBotCount, maxClients - humanPlayersInGame);
   }
   auto maxSpawnCount = game.getSpawnCount (Team::Terrorist) + game.getSpawnCount (Team::CT) - humanPlayersInGame;

   // if has some custom spawn points, max out spawn point counter
   if (desiredBotCount >= botsInGame && hasCustomCSDMSpawnEntities ()) {
      maxSpawnCount = maxClients + 1;
   }

   // sent message only to console from here
   ctrl.setFromConsole (true);

   // add bots if necessary
   if (desiredBotCount > botsInGame && botsInGame < maxSpawnCount) {
      createRandom ();
   }
   else if (desiredBotCount < botsInGame) {
      balancedKickRandom (false);
   }
   else {
      // clear the saved names when quota balancing ended
      if (cv_save_bots_names && !m_saveBotNames.empty ()) {
         m_saveBotNames.clear ();
      }
   }
   m_quotaMaintainTime = game.time () + cv_quota_maintain_interval.as <float> ();
}

void BotManager::maintainLeaders () {
   if (game.is (GameFlags::FreeForAll)) {
      return;
   }

   // select leader each team somewhere in round start
   if (gameState.getRoundStartTime () + rg (1.5f, 3.0f) < game.time ()) {
      for (int team = 0; team < kGameTeamNum; ++team) {
         selectLeaders (team, false);
      }
   }
}

void BotManager::maintainRoundRestart () {
   if (!cv_first_human_restart || !game.isDedicated ()) {
      return;
   }
   const int totalHumans = getHumansCount (true);
   const int totalBots = getBotCount ();

   if (totalHumans > 0
      && m_numPreviousPlayers == 0
      && totalHumans == 1
      && totalBots > 0
      && !gameState.isResetHUD ()) {

      static ConVarRef sv_restartround ("sv_restartround");

      if (sv_restartround.exists ()) {
         sv_restartround.set ("1");
      }
   }
   m_numPreviousPlayers = totalHumans;
   gameState.setResetHUD (false);
}

void BotManager::maintainAutoKill () {
   const float killDelay = cv_autokill_delay.as <float> ();

   if (killDelay < 1.0f || gameState.isRoundOver ()) {
      return;
   }

   // check if we're reached the delay, so kill out bots
   if (!cr::fzero (m_autoKillCheckTime) && m_autoKillCheckTime < game.time ()) {
      killAllBots (-1, true);
      m_autoKillCheckTime = 0.0f;

      return;
   }
   int aliveBots = 0;

   // do not interrupt bomb-defuse scenario
   if (game.mapIs (MapFlags::Demolition) && gameState.isBombPlanted ()) {
      return;
   }
   const int totalHumans = getHumansCount (true); // we're ignore spectators intentionally 

   // if we're have no humans in teams do not bother to proceed
   if (!totalHumans) {
      return;
   }

   for (const auto &bot : m_bots) {
      if (bot->m_isAlive) {
         ++aliveBots;

         // do not interrupt assassination scenario, if vip is a bot
         if (game.is (MapFlags::Assassination) && game.isPlayerVIP (bot->ent ())) {
            return;
         }
      }
   }
   const int aliveHumans = getAliveHumansCount ();

   // check if we're have no alive players and some alive bots, and start autokill timer
   if (!aliveHumans && aliveBots > 0 && cr::fzero (m_autoKillCheckTime)) {
      m_autoKillCheckTime = game.time () + killDelay;
   }
}

void BotManager::reset () {
   m_plantSearchUpdateTime = 0.0f;
   m_lastChatTime = 0.0f;
   m_bombSayStatus = BombPlantedSay::ChatSay | BombPlantedSay::Chatter;
}

void BotManager::initFilters () {
   // table with all available actions for the bots (filtered in & out in bot::setconditions) some of them have subactions included

   m_filters = {
      { &Bot::normal_, Task::Normal, 0.0f, kInvalidNodeIndex, 0.0f, true },
      { &Bot::pause_, Task::Pause, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::moveToPos_, Task::MoveToPosition, 0.0f, kInvalidNodeIndex, 0.0f, true },
      { &Bot::followUser_, Task::FollowUser, 0.0f, kInvalidNodeIndex, 0.0f, true },
      { &Bot::pickupItem_, Task::PickupItem, 0.0f, kInvalidNodeIndex, 0.0f, true },
      { &Bot::camp_, Task::Camp, 0.0f, kInvalidNodeIndex, 0.0f, true },
      { &Bot::plantBomb_, Task::PlantBomb, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::defuseBomb_, Task::DefuseBomb, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::attackEnemy_, Task::Attack, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::huntEnemy_, Task::Hunt, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::seekCover_, Task::SeekCover, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::throwExplosive_, Task::ThrowExplosive, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::throwFlashbang_, Task::ThrowFlashbang, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::throwSmoke_, Task::ThrowSmoke, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::doublejump_, Task::DoubleJump, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::escapeFromBomb_, Task::EscapeFromBomb, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::shootBreakable_, Task::ShootBreakable, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::hide_, Task::Hide, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::blind_, Task::Blind, 0.0f, kInvalidNodeIndex, 0.0f, false },
      { &Bot::spraypaint_, Task::Spraypaint, 0.0f, kInvalidNodeIndex, 0.0f, false }
   };
}

void BotManager::resetFilters () {
   for (auto &task : m_filters) {
      task.time = 0.0f;
   }
}

void BotManager::decrementQuota (int by) {
   if (by != 0) {
      cv_quota.set (cr::clamp <int> (cv_quota.as <int> () - by, 0, cv_quota.as <int> ()));
      return;
   }
   cv_quota.set (0);
}

void BotManager::initQuota () {
   m_maintainTime = game.time () + cv_join_delay.as <float> ();
   m_quotaMaintainTime = game.time () + cv_join_delay.as <float> ();

   m_addRequests.clear ();
}

void BotManager::serverFill (int selection, int personality, int difficulty, int numToAdd) {
   // this function fill server with bots, with specified team & personality

   // always keep one slot
   const int maxClients = cv_autovacate
      ? game.maxClients () - cv_autovacate_keep_slots.as <int> () - (game.isDedicated () ? 0 : getHumansCount ()) : game.maxClients ();

   if (getBotCount () >= maxClients - getHumansCount ()) {
      return;
   }
   if (selection == 1 || selection == 2) {
      mp_limitteams.set (0);
      mp_autoteambalance.set (0);
   }
   else {
      selection = 5;
   }
   const auto maxToAdd = maxClients - (getHumansCount () + getBotCount ());

   constexpr char kTeams[6][12] = { "", { "Terrorists" }, { "CTs" }, "", "", { "Random" }, };
   auto toAdd = numToAdd == -1 ? maxToAdd : numToAdd;

   // limit manually added count as well
   if (toAdd > maxToAdd - 1) {
      toAdd = maxToAdd - 1;
   }

   for (int i = 0; i <= toAdd; ++i) {
      addbot ("", difficulty, personality, selection, -1, true);
   }
   ctrl.msg ("Fill server with %s bots...", &kTeams[selection][0]);
}

void BotManager::kickEveryone (bool instant, bool zeroQuota) {
   // this function drops all bot clients from server (this function removes only yapb's)

   if (cv_quota && hasBotsOnline ()) {
      ctrl.msg ("Bots are removed from server.");
   }

   if (zeroQuota) {
      decrementQuota (0);
   }

   // if everyone is kicked, clear the saved bot names
   if (cv_save_bots_names && !m_saveBotNames.empty ()) {
      m_saveBotNames.clear ();
   }

   if (instant) {
      for (const auto &bot : m_bots) {
         if (!game.isNullEntity (bot->ent ())) {
            bot->kick (true);
         }
      }
   }
   m_addRequests.clear ();
}

void BotManager::kickFromTeam (Team team, bool removeAll) {
   // this function remove random bot from specified team (if removeAll value = 1 then removes all players from team)

   if (removeAll) {
      const auto &counts = countTeamPlayers ();

      m_quotaMaintainTime = game.time () + 3.0f;
      m_addRequests.clear ();

      if (team == Team::Terrorist) {
         decrementQuota (counts.first);
      }
      else {
         decrementQuota (counts.second);
      }
   }

   for (const auto &bot : m_bots) {
      if (team == game.getRealPlayerTeam (bot->ent ())) {
         bot->kick (removeAll);

         if (!removeAll) {
            decrementQuota ();
            break;
         }
      }
   }
}

void BotManager::killAllBots (int team, bool silent) {
   // this function kills all bots on server (only this dll controlled bots)

   for (const auto &bot : m_bots) {
      if (team != Team::Invalid && game.getRealPlayerTeam (bot->ent ()) != team) {
         continue;
      }
      bot->kill ();
   }

   if (!silent) {
      ctrl.msg ("All bots died...");
   }
}

void BotManager::kickBot (int index) {
   auto bot = findBotByIndex (index);

   if (bot) {
      bot->kick ();
      decrementQuota ();
   }
}

bool BotManager::kickRandom (bool decQuota, Team fromTeam) {
   // this function removes random bot from server (only yapb's)

   // if forTeam is unassigned, that means random team
   bool deadBotFound = false;

   auto updateQuota = [&] () {
      if (decQuota) {
         decrementQuota ();
      }
   };

   auto belongsTeam = [&] (Bot *bot) {
      if (fromTeam == Team::Unassigned) {
         return true;
      }
      return game.getRealPlayerTeam (bot->ent ()) == fromTeam;
   };

   // first try to kick the bot that is currently dead
   for (const auto &bot : m_bots) {

      // is this slot used?
      if (!bot->m_isAlive && belongsTeam (bot.get ())) {
         updateQuota ();
         bot->kick ();

         deadBotFound = true;
         break;
      }
   }

   if (deadBotFound) {
      return true;
   }

   // if no dead bots found try to find one with lowest amount of frags
   Bot *selected = nullptr;
   float score = kInfiniteDistance;

   // search bots in this team
   for (const auto &bot : m_bots) {
      if (bot->pev->frags < score && belongsTeam (bot.get ())) {
         selected = bot.get ();
         score = bot->pev->frags;
      }
   }

   // if found some bots
   if (selected != nullptr) {
      updateQuota ();
      selected->kick ();

      return true;
   }
   static Array <Bot *> kickable;
   kickable.clear ();

   // worst case, just kick some random bot
   for (const auto &bot : m_bots) {

      // is this slot used?
      if (belongsTeam (bot.get ())) {
         kickable.push (bot.get ());
      }
   }

   // kick random from collected
   if (!kickable.empty ()) {
      auto bot = kickable.random ();

      if (bot) {
         updateQuota ();
         bot->kick ();

         return true;
      }
   }
   return false;
}

bool BotManager::balancedKickRandom (bool decQuota) {
   const auto &tp = countTeamPlayers ();
   bool isKicked = false;

   if (tp.first > tp.second) {
      isKicked = kickRandom (decQuota, Team::Terrorist);
   }
   else if (tp.first < tp.second) {
      isKicked = kickRandom (decQuota, Team::CT);
   }
   else {
      isKicked = kickRandom (decQuota, Team::Unassigned);
   }

   // if we can't kick player from correct team, just kick any random to keep quota control work
   if (!isKicked) {
      isKicked = kickRandom (decQuota, Team::Unassigned);
   }
   return isKicked;
}

bool BotManager::hasCustomCSDMSpawnEntities () {
   if (!game.is (GameFlags::CSDM | GameFlags::FreeForAll)) {
      return false;
   }
   auto customSpawnClass = conf.fetchCustom ("CustomCSDMSpawnPoint");

   // check for custom entity
   return game.hasEntityInGame (customSpawnClass);
}

void BotManager::setLastWinner (int winner) {
   m_lastWinner = winner;
   gameState.setRoundOver (true);

   if (cv_radio_mode.as <int> () != 2) {
      return;
   }
   auto notify = findAliveBot ();

   if (notify) {
      if (notify->m_team == winner) {
         if (gameState.getRoundMidTime () > game.time ()) {
            notify->pushChatterMessage (Chatter::QuickWonRound);
         }
         else {
            notify->pushChatterMessage (Chatter::WonTheRound);
         }
      }
   }
}

void BotManager::checkBotModel (edict_t *ent, char *infobuffer) {
   for (const auto &bot : bots) {
      if (bot->ent () == ent) {
         bot->refreshCreatureStatus (infobuffer);
         break;
      }
   }
}

void BotManager::checkNeedsToBeKicked () {
   // this is called to check if bot is leaving server due to pathfinding error
   // so this will cause to delay quota management stuff from executing for some time

   for (const auto &bot : bots) {
      if (bot->m_kickMeFromServer) {
         m_holdQuotaManagementTimer.start (10.0f);
         m_addRequests.clear ();

         bot->kick (); // kick bot from server if requested
      }
   }
}

void BotManager::refreshCreatureStatus () {
   if (!game.is (GameFlags::ZombieMod)) {
      return;
   }

   for (const auto &bot : bots) {
      if (bot->m_isAlive) {
         bot->refreshCreatureStatus (nullptr);
      }
   }
}

void BotManager::setWeaponMode (int selection) {
   // this function sets bots weapon mode

   selection--;

   constexpr int kStdMaps[7][kNumWeapons] = {
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // Knife only
      { -1, -1, -1, 2, 2, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // Pistols only
      { -1, -1, -1, -1, -1, -1, -1, 2, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // Shotgun only
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 1, 2, 0, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1 }, // Machine Guns only
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 1, 0, 1, 1, -1, -1, -1, -1, -1, -1 }, // Rifles only
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 0, 1, -1, -1 }, // Snipers only
      { -1, -1, -1, 2, 2, 0, 1, 2, 2, 2, 1, 2, 0, 2, 0, 0, 1, 0, 1, 1, 2, 2, 0, 1, 2, 1 } // Standard
   };

   constexpr int kAsMaps[7][kNumWeapons] = {
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // Knife only
      { -1, -1, -1, 2, 2, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // Pistols only
      { -1, -1, -1, -1, -1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // Shotgun only
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 0, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1 }, // Machine Guns only
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 1, 0, 1, 1, -1, -1, -1, -1, -1, -1 }, // Rifles only
      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 1, -1, -1 }, // Snipers only
      { -1, -1, -1, 2, 2, 0, 1, 1, 1, 1, 1, 1, 0, 2, 0, -1, 1, 0, 1, 1, 0, 0, -1, 1, 1, 1 } // Standard
   };
   constexpr char kModes[7][12] = { { "Knife" }, { "Pistol" }, { "Shotgun" }, { "Machine Gun" }, { "Rifle" }, { "Sniper" }, { "Standard" } };

   // get the raw weapons array
   auto tab = conf.getRawWeapons ();

   // set the correct weapon mode
   for (int i = 0; i < kNumWeapons; ++i) {
      tab[i].teamStandard = kStdMaps[selection][i];
      tab[i].teamAS = kAsMaps[selection][i];
   }
   cv_jasonmode.set (selection == 0 ? 1 : 0);

   ctrl.msg ("%s weapon mode selected.", &kModes[selection][0]);
}

void BotManager::listBots () {
   // this function list's bots currently playing on the server

   ctrl.msg ("%-3.5s\t%-19.16s\t%-10.12s\t%-3.4s\t%-3.4s\t%-3.4s\t%-3.6s\t%-3.5s\t%-3.8s", "index", "name", "personality", "team", "difficulty", "frags", "deaths", "alive", "timeleft");

   auto botTeam = [] (edict_t *ent) -> StringRef {
      const auto team = game.getRealPlayerTeam (ent);

      switch (team) {
      case Team::CT:
         return "CT";

      case Team::Terrorist:
         return "TE";

      case Team::Unassigned:
      default:
         return "UN";

      case Team::Spectator:
         return "SP";
      }
   };

   for (const auto &bot : bots) {
      auto timelimitStr = cv_rotate_bots ? strings.format ("%-3.0f secs", bot->m_stayTime - game.time ()) : "unlimited";

      ctrl.msg ("[%-2.1d]\t%-22.16s\t%-10.12s\t%-3.4s\t%-3.1d\t%-3.1d\t%-3.1d\t%-3.4s\t%s",
         bot->index (),
         bot->pev->netname.chars (),
         bot->m_personality == Personality::Rusher ? "rusher" : bot->m_personality == Personality::Normal ? "normal" : "careful",

         botTeam (bot->ent ()),
         bot->m_difficulty,
         static_cast <int> (bot->pev->frags),

         bot->m_deathCount,
         bot->m_isAlive ? "yes" : "no",

         timelimitStr);
   }
   ctrl.msg ("%d bots", m_bots.length ());
}

float BotManager::getConnectionTimes (StringRef name, float original) {
   // this function get's fake bot player time.

   for (const auto &bot : m_bots) {
      if (name.startsWith (bot->pev->netname.chars ())) {
         return bot->getConnectionTime ();
      }
   }
   return original;
}

float BotManager::getAverageTeamKPD (bool calcForBots) {
   Twin <float, int32_t> calc {};

   for (const auto &client : util.getClients ()) {
      if (!(client.flags & ClientFlags::Used)) {
         continue;
      }
      auto bot = bots[client.ent];

      if (calcForBots && bot) {
         calc.first += client.ent->v.frags;
         calc.second++;
      }
      else if (!calcForBots && !bot) {
         calc.first += client.ent->v.frags;
         calc.second++;
      }
   }

   if (calc.second > 0) {
      return calc.first / static_cast <float> (cr::max (1, calc.second));
   }
   return 0.0f;
}

Twin <int, int> BotManager::countTeamPlayers () {
   int ts = 0, cts = 0;

   for (const auto &client : util.getClients ()) {
      if (client.flags & ClientFlags::Used) {
         if (client.team2 == Team::Terrorist) {
            ++ts;
         }
         else if (client.team2 == Team::CT) {
            ++cts;
         }
      }
   }
   return { ts, cts };
}

Bot *BotManager::findHighestFragBot (int team) {
   Twin <int32_t, float> best {};

   // search bots in this team
   for (const auto &bot : bots) {
      if (bot->m_isAlive && game.getRealPlayerTeam (bot->ent ()) == team) {
         if (bot->pev->frags > best.second) {
            best.first = bot->index ();
            best.second = bot->pev->frags;
         }
      }
   }
   return findBotByIndex (best.first);
}

void BotManager::updateTeamEconomics (int team, bool setTrue) {
   // this function decides is players on specified team is able to buy primary weapons by calculating players
   // that have not enough money to buy primary (with economics), and if this result higher 80%, player is can't
   // buy primary weapons.

   auto &ecoStatus = m_teamData[team].positiveEco;

   if (setTrue || !cv_economics_rounds) {
      ecoStatus = true;
      return; // don't check economics while economics disable
   }
   const auto econLimit = conf.getEconLimit ();

   int numPoorPlayers = 0;
   int numTeamPlayers = 0;

   // start calculating
   for (const auto &bot : m_bots) {
      if (bot->m_team == team) {
         if (bot->m_moneyAmount <= econLimit[EcoLimit::PrimaryGreater]) {
            ++numPoorPlayers;
         }
         ++numTeamPlayers; // update count of team
      }
   }
   ecoStatus = true;

   if (numTeamPlayers <= 1) {
      return;
   }
   // if 80 percent of team have no enough money to purchase primary weapon
   if ((numTeamPlayers * 80) / 100 <= numPoorPlayers) {
      ecoStatus = false;
   }

   // winner must buy something!
   if (m_lastWinner == team) {
      ecoStatus = true;
   }
}

void BotManager::updateBotDifficulties () {
   // if min/max difficulty is specified  this should not have effect
   if (cv_difficulty_min.as <int> () != Difficulty::Invalid || cv_difficulty_max.as <int> () != Difficulty::Invalid || cv_difficulty_auto) {
      return;
   }
   const auto difficulty = cv_difficulty.as <int> ();

   if (difficulty != m_lastDifficulty) {

      // sets new difficulty for all bots
      for (const auto &bot : m_bots) {
         bot->setNewDifficulty (difficulty);
      }
      m_lastDifficulty = difficulty;
   }
}

void BotManager::balanceBotDifficulties () {
   // difficulty changing once per round (time)
   auto updateDifficulty = [] (Bot *bot, int32_t offset) {
      bot->setNewDifficulty (cr::clamp (static_cast <Difficulty> (bot->m_difficulty + offset), Difficulty::Noob, Difficulty::Expert));
   };

   // with nightmare difficulty, there is no balance
   if (cv_whose_your_daddy) {
      return;
   }

   if (cv_difficulty_auto && m_difficultyBalanceTime < game.time ()) {
      const auto ratioPlayer = getAverageTeamKPD (false);
      const auto ratioBots = getAverageTeamKPD (true);

      // calculate for each the bot
      for (auto &bot : m_bots) {
         const float score = bot->m_kpdRatio;

         // if kd ratio is going to go to low, we need to try to set higher difficulty
         if (score < 0.8f || (score <= 1.2f && ratioBots < ratioPlayer)) {
            updateDifficulty (bot.get (), +1);
         }
         else if (score > 4.0f || (score >= 2.5f && ratioBots > ratioPlayer)) {
            updateDifficulty (bot.get (), -1);
         }
      }
      m_difficultyBalanceTime = game.time () + cv_difficulty_auto_balance_interval.as <float> ();
   }
}

void BotManager::destroy () {
   // this function free all bots slots (used on server shutdown)

   m_holdQuotaManagementTimer.invalidate (); // restart quota manager

   for (auto &bot : m_bots) {
      bot->markStale ();
   }
   m_bots.clear ();
}

Bot::Bot (edict_t *bot, int difficulty, int personality, int team, int skin) {
   // this function does core operation of creating bot, it's called by addbot (),
   // when bot setup completed, (this is a bot class constructor)

   const int clientIndex = game.indexOfEntity (bot);
   pev = &bot->v;

   // create the player entity by calling MOD's player function
   bots.execGameEntity (bot);

   // set all info buffer keys for this bot
   auto buffer = engfuncs.pfnGetInfoKeyBuffer (bot);

   engfuncs.pfnSetClientKeyValue (clientIndex, buffer, "_vgui_menus", "0");
   engfuncs.pfnSetClientKeyValue (clientIndex, buffer, "_ah", "0");

   if (!game.is (GameFlags::Legacy)) {
      if (cv_show_latency.as <int> () == 1) {
         engfuncs.pfnSetClientKeyValue (clientIndex, buffer, "*bot", "1");
      }
      const auto &avatar = conf.getRandomAvatar ();

      if (cv_show_avatars && !avatar.empty ()) {
         engfuncs.pfnSetClientKeyValue (clientIndex, buffer, "*sid", avatar.chars ());
      }
   }

   char reject[128] = { 0, };
   MDLL_ClientConnect (bot, bot->v.netname.chars (), strings.format ("127.0.0.%d", clientIndex + 100), reject);

   if (!strings.isEmpty (reject)) {
      logger.error ("Server refused '%s' connection (%s)", bot->v.netname.chars (), reject);
      game.serverCommand ("kick \"%s\"", bot->v.netname.chars ()); // kick the bot player if the server refused it

      bot->v.flags |= FL_KILLME;
      return;
   }

   MDLL_ClientPutInServer (bot);
   bot->v.flags |= FL_CLIENT | FL_FAKECLIENT; // set this player as fake client

   // initialize all the variables for this bot...
   m_notStarted = true; // hasn't joined game yet
   m_forceRadio = false;

   m_index = clientIndex - 1;
   m_startAction = BotMsg::None;
   m_retryJoin = 0;
   m_moneyAmount = 0;
   m_logoDecalIndex = conf.getRandomLogoDecalIndex ();

   if (cv_rotate_bots) {
      m_stayTime = game.time () + rg (cv_rotate_stay_min.as <float> (), cv_rotate_stay_max.as <float> ());
   }
   else {
      m_stayTime = game.time () + kInfiniteDistance;
   }

   // assign how talkative this bot will be
   m_sayTextBuffer.chatDelay = rg (3.8f, 10.0f);
   m_sayTextBuffer.chatProbability = rg (10, 100);

   m_isAlive = false;
   m_weaponBurstMode = BurstMode::Off;
   setNewDifficulty (cr::clamp (static_cast <Difficulty> (difficulty), Difficulty::Noob, Difficulty::Expert));

   auto minDifficulty = cv_difficulty_min.as <int> ();
   auto maxDifficulty = cv_difficulty_max.as <int> ();

   // if we're have min/max difficulty specified, choose value from they
   if (minDifficulty != Difficulty::Invalid && maxDifficulty != Difficulty::Invalid) {
      if (maxDifficulty > minDifficulty) {
         cr::swap (maxDifficulty, minDifficulty);
      }
      setNewDifficulty (rg (minDifficulty, maxDifficulty));
   }
   m_pingBase = fakeping.randomBase ();
   m_ping = fakeping.randomBase ();

   m_previousThinkTime = game.time () - 0.1f;
   m_frameInterval = game.time ();
   m_heavyTimestamp = game.time ();
   m_slowFrameTimestamp = 0.0f;
   m_kpdRatio = 0.0f;
   m_deathCount = 0;

   // stuff from jk_botti
   m_playServerTime = 60.0f * rg (30.0f, 240.0f);
   m_joinServerTime = plat.seconds () - m_playServerTime * rg (0.2f, 0.8f);

   switch (personality) {
   case 1:
      m_personality = Personality::Rusher;
      m_baseAgressionLevel = rg (0.7f, 1.0f);
      m_baseFearLevel = rg (0.0f, 0.4f);
      break;

   case 2:
      m_personality = Personality::Careful;
      m_baseAgressionLevel = rg (0.2f, 0.5f);
      m_baseFearLevel = rg (0.7f, 1.0f);
      break;

   default:
      m_personality = Personality::Normal;
      m_baseAgressionLevel = rg (0.4f, 0.7f);
      m_baseFearLevel = rg (0.4f, 0.7f);
      break;
   }
   clearAmmoInfo ();

   m_currentWeapon = 0; // current weapon is not assigned at start
   m_weaponType = WeaponType::None; // current weapon type is not assigned at start

   m_voicePitch = rg (85, 115); // assign voice pitch

   // copy them over to the temp level variables
   m_agressionLevel = m_baseAgressionLevel;
   m_fearLevel = m_baseFearLevel;
   m_nextEmotionUpdate = game.time () + 0.5f;
   m_healthValue = bot->v.health;

   // just to be sure
   m_msgQueue.clear ();

   // init async planner
   m_planner = cr::makeUnique <AStarAlgo> (graph.length ());

   // init path walker
   m_pathWalk.init (m_planner->getMaxLength ());

   // init player models parts enumerator
   if (cv_use_hitbox_enemy_targeting) {
      m_hitboxEnumerator = cr::makeUnique <PlayerHitboxEnumerator> ();
   }

   // bot is not kicked by rotation
   m_kickedByRotation = false;

   // assign team and class
   m_wantedTeam = team;
   m_wantedSkin = skin;

   m_tasks.reserve (Task::Max);

   newRound ();
}

void Bot::clearAmmoInfo () {
   plat.bzero (&m_ammoInClip, sizeof (m_ammoInClip));
   plat.bzero (&m_ammo, sizeof (m_ammo));
}

float Bot::getConnectionTime () {
   const auto current = plat.seconds ();

   if (current - m_joinServerTime > m_playServerTime || current - m_joinServerTime <= 0.0f) {
      m_playServerTime = 60.0f * rg (30.0f, 240.0f);
      m_joinServerTime = current - m_playServerTime * rg (0.2f, 0.8f);
   }
   return current - m_joinServerTime;
}

int BotManager::getHumansCount (bool ignoreSpectators) {
   // this function returns number of humans playing on the server

   int count = 0;

   for (const auto &client : util.getClients ()) {
      if ((client.flags & ClientFlags::Used) && !bots[client.ent] && !(client.ent->v.flags & FL_FAKECLIENT)) {
         if (ignoreSpectators && client.team2 != Team::Terrorist && client.team2 != Team::CT) {
            continue;
         }
         ++count;
      }
   }
   return count;
}

int BotManager::getAliveHumansCount () {
   // this function returns number of humans playing on the server

   int count = 0;

   for (const auto &client : util.getClients ()) {
      if ((client.flags & ClientFlags::Alive)
         && !bots[client.ent]
         && !(client.ent->v.flags & FL_FAKECLIENT)) {
         ++count;
      }
   }
   return count;
}

int BotManager::getPlayerPriority (edict_t *ent) {
   constexpr auto kHighPriority = 1024;

   // always check for only our own bots
   auto bot = bots[ent];

   // if player just return high prio
   if (!bot) {
      return game.indexOfEntity (ent) + kHighPriority * 2;
   }

   // give bots some priority
   if (bot->m_hasC4 || bot->m_isVIP || bot->m_hasHostage || (bot->m_currentTravelFlags & PathFlag::Jump)) {
      return bot->entindex () + kHighPriority;
   }
   const auto task = bot->getCurrentTaskId ();

   // higher priority if important task
   if (task == Task::MoveToPosition
      || task == Task::SeekCover
      || task == Task::Camp
      || task == Task::Hide) {

      return bot->entindex () + kHighPriority;
   }
   return bot->entindex ();
}

bool BotManager::isTeamStacked (int team) {
   if (team != Team::CT && team != Team::Terrorist) {
      return false;
   }
   const int limitTeams = mp_limitteams.as <int> ();

   if (!limitTeams) {
      return false;
   }
   int teamCount[kGameTeamNum] = { 0, };

   for (const auto &client : util.getClients ()) {
      if ((client.flags & ClientFlags::Used) && client.team2 != Team::Unassigned && client.team2 != Team::Spectator) {
         ++teamCount[client.team2];
      }
   }
   return teamCount[team] + 1 > teamCount[team == Team::CT ? Team::Terrorist : Team::CT] + limitTeams;
}

void BotManager::disconnectBot (Bot *bot) {
   for (auto &e : m_bots) {
      if (e.get () != bot) {
         continue;
      }
      bot->markStale ();

      if (!bot->m_kickedByRotation && cv_save_bots_names) {
         m_saveBotNames.emplaceLast (bot->pev->netname.str ());
      }

      const auto index = m_bots.index (e);
      e.reset ();

      m_bots.erase (index, 1); // remove from bots array
      bot = nullptr;

      break;
   }
}

void BotManager::handleDeath (edict_t *killer, edict_t *victim) {
   const auto killerTeam = game.getRealPlayerTeam (killer);
   const auto victimTeam = game.getRealPlayerTeam (victim);

   if (cv_radio_mode.as <int> () == 2) {
      // need to send congrats on well placed shot
      for (const auto &notify : bots) {
         if (notify->m_isAlive
            && killerTeam == notify->m_team
            && killerTeam != victimTeam
            && killer != notify->ent ()
            && notify->seesEntity (victim->v.origin)) {

            if (!(killer->v.flags & FL_FAKECLIENT)) {
               notify->pushChatterMessage (Chatter::NiceShotCommander);
            }
            else {
               notify->pushChatterMessage (Chatter::NiceShotPall);
            }
            break;
         }
      }
   }
   Bot *killerBot = nullptr;
   Bot *victimBot = nullptr;

   // notice nearby to victim teammates, that attacker is near
   for (const auto &notify : bots) {
      if (notify->m_difficulty >= Difficulty::Hard
         && killerTeam != victimTeam
         && notify->m_seeEnemyTime + 2.0f < game.time ()
         && notify->m_isAlive
         && notify->m_team == victimTeam
         && game.isNullEntity (notify->m_enemy)
         && game.isNullEntity (notify->m_lastEnemy)
         && util.isVisible (killer->v.origin, notify->ent ())) {

         // make bot look at last enemy position
         notify->m_actualReactionTime = 0.0f;
         notify->m_seeEnemyTime = game.time ();
         notify->m_enemy = killer;
         notify->m_lastEnemy = killer;
         notify->m_lastEnemyOrigin = killer->v.origin;
      }

      if (notify->ent () == killer) {
         killerBot = notify.get ();
      }
      else if (notify->ent () == victim) {
         victimBot = notify.get ();
      }
   }

   // mark bot as "spawned", and reset it to new-round state when it dead (for csdm/zombie only)
   if (victimBot != nullptr) {
      victimBot->spawned ();

      victimBot->m_isAlive = false;
   }

   // is this message about a bot who killed somebody?
   if (killerBot != nullptr) {
      killerBot->setLastVictim (victim);
   }

   // did a human kill a bot on his team?
   else {
      if (victimBot != nullptr) {
         if (killerTeam == victimBot->m_team) {
            victimBot->m_voteKickIndex = game.indexOfEntity (killer);

            for (const auto &notify : bots) {
               if (notify->seesEntity (victim->v.origin)) {
                  notify->pushChatterMessage (Chatter::TeamKill);
               }
            }
         }
         victimBot->m_isAlive = false;
      }
   }
}

void Bot::newRound () {
   // this function initializes a bot after creation & at the start of each round

   // delete all allocated path nodes
   clearSearchNodes ();

   m_pathOrigin.clear ();
   m_destOrigin.clear ();

   m_path = nullptr;
   m_currentTravelFlags = 0;
   m_desiredVelocity.clear ();
   m_currentNodeIndex = kInvalidNodeIndex;
   m_prevGoalIndex = kInvalidNodeIndex;
   m_chosenGoalIndex = kInvalidNodeIndex;
   m_loosedBombNodeIndex = kInvalidNodeIndex;
   m_plantedBombNodeIndex = kInvalidNodeIndex;

   m_moveToC4 = false;
   m_defuseNotified = false;
   m_duckDefuse = false;
   m_duckDefuseCheckTime = 0.0f;
   m_timeDebugUpdateTime = 0.0f;
   m_lastDamageTimestamp = 0.0f;

   m_numFriendsLeft = 0;
   m_numEnemiesLeft = 0;
   m_oldButtons = pev->button;

   for (auto &node : m_previousNodes) {
      node = kInvalidNodeIndex;
   }
   m_navTimeset = game.time ();
   m_team = game.getPlayerTeam (ent ());

   resetPathSearchType ();

   // clear all states & tasks
   m_states = 0;
   clearTasks ();

   m_isLeader = false;
   m_hasProgressBar = false;
   m_canSetAimDirection = true;
   m_preventFlashing = 0.0f;

   m_timeTeamOrder = 0.0f;
   m_askCheckTime = rg (30.0f, 90.0f);
   m_minSpeed = 260.0f;
   m_prevSpeed = 0.0f;
   m_prevOrigin = Vector (kInfiniteDistance, kInfiniteDistance, kInfiniteDistance);
   m_prevTime = game.time ();
   m_lookUpdateTime = game.time ();
   m_changeViewTime = game.time () + (rg.chance (25) ? mp_freezetime.as <float> () : 0.0f);
   m_aimErrorTime = game.time ();

   m_viewDistance = Frustum::kMaxViewDistance;
   m_maxViewDistance = Frustum::kMaxViewDistance;

   m_liftEntity = nullptr;
   m_pickupItem = nullptr;
   m_itemCheckTime = 0.0f;
   m_ignoredItems.clear ();

   m_breakableEntity = nullptr;
   m_breakableOrigin.clear ();
   m_lastBreakable = nullptr;

   m_timeDoorOpen = 0.0f;
   m_timeHitDoor = 0.0f;

   for (auto &fall : m_checkFallPoint) {
      fall.clear ();
   }
   m_checkFall = false;

   resetCollision ();
   resetDoubleJump ();

   m_enemy = nullptr;
   m_lastVictim = nullptr;
   m_lastEnemy = nullptr;
   m_lastEnemyOrigin.clear ();
   m_lastVictimOrigin.clear ();
   m_trackingEdict = nullptr;
   m_enemyBodyPartSet = nullptr;
   m_timeNextTracking = 0.0f;
   m_lastPredictIndex = kInvalidNodeIndex;
   m_lastPredictLength = kInfiniteDistanceLong;

   m_buttonPushTime = 0.0f;
   m_enemyUpdateTime = 0.0f;
   m_retreatTime = 0.0f;
   m_seeEnemyTime = 0.0f;
   m_shootAtDeadTime = 0.0f;
   m_oldCombatDesire = 0.0f;
   m_liftUsageTime = 0.0f;
   m_breakableTime = 0.0f;

   m_avoidGrenade = nullptr;
   m_needAvoidGrenade = 0;

   m_lastDamageType = -1;
   m_voteKickIndex = 0;
   m_lastVoteKick = 0;
   m_voteMap = 0;
   m_tryOpenDoor = 0;
   m_aimFlags = 0;
   m_liftState = 0;

   m_aimLastError.clear ();
   m_position.clear ();
   m_liftTravelPos.clear ();

   setIdealReactionTimers (true);

   m_targetEntity = nullptr;
   m_followWaitTime = 0.0f;

   m_hostages.clear ();

   if (cv_use_hitbox_enemy_targeting) {
      if (m_hitboxEnumerator) {
         m_hitboxEnumerator->reset ();
      }
      else {
         m_hitboxEnumerator = cr::makeUnique <PlayerHitboxEnumerator> ();
      }
   }
   showChatterIcon (false);

   m_approachingLadderTimer.invalidate ();
   m_forgetLastVictimTimer.invalidate ();
   m_lostReachableNodeTimer.invalidate ();
   m_fixFallTimer.invalidate ();
   m_repathTimer.invalidate ();

   for (auto &timer : m_chatterTimes) {
      timer = kMaxChatterRepeatInterval;
   }
   refreshCreatureStatus (nullptr);

   m_isReloading = false;
   m_reloadState = Reload::None;

   m_reloadCheckTime = 0.0f;
   m_shootTime = game.time ();
   m_playerTargetTime = game.time ();
   m_firePause = 0.0f;
   m_timeLastFired = 0.0f;

   m_sniperStopTime = 0.0f;
   m_grenadeCheckTime = 0.0f;
   m_isUsingGrenade = false;
   m_bombSearchOverridden = false;
   m_fireHurtsFriend = false;

   m_blindButton = 0;
   m_blindTime = 0.0f;
   m_jumpTime = 0.0f;
   m_jumpFinished = false;
   m_isStuck = false;

   m_sayTextBuffer.timeNextChat = game.time ();
   m_sayTextBuffer.entityIndex = -1;
   m_sayTextBuffer.sayText.clear ();

   m_buyState = BuyState::PrimaryWeapon;
   m_lastEquipTime = 0.0f;

   // setup radio percent each round
   const auto badMorale = m_fearLevel > m_agressionLevel ? rg.chance (75) : rg.chance (35);

   switch (m_personality) {
   case Personality::Normal:
   default:
      m_radioPercent = badMorale ? rg (50, 75) : rg (25, 50);
      break;

   case Personality::Rusher:
      m_radioPercent = badMorale ? rg (35, 50) : rg (15, 35);
      break;

   case Personality::Careful:
      m_radioPercent = badMorale ? rg (70, 90) : rg (50, 70);
      break;
   }

   // if bot died, clear all weapon stuff and force buying again
   if (!m_isAlive) {
      clearAmmoInfo ();

      m_currentWeapon = 0;
      m_weaponType = 0;
   }
   m_flashLevel = 100;
   m_checkDarkTime = game.time ();

   m_knifeAttackTime = game.time () + rg (1.3f, 2.6f);
   m_nextBuyTime = game.time () + rg (0.6f, 2.0f);

   m_buyPending = false;
   m_inBombZone = false;
   m_ignoreBuyDelay = false;
   m_hasC4 = false;
   m_hasHostage = false;

   m_fallDownTime = 0.0f;
   m_shieldCheckTime = 0.0f;
   m_zoomCheckTime = 0.0f;
   m_strafeSetTime = 0.0f;
   m_dodgeStrafeDir = Dodge::None;
   m_fightStyle = Fight::None;
   m_lastFightStyleCheck = 0.0f;

   m_checkWeaponSwitch = true;
   m_checkKnifeSwitch = true;
   m_buyingFinished = false;

   m_radioEntity = nullptr;
   m_radioOrder = 0;
   m_defendedBomb = false;
   m_defendHostage = false;
   m_headedTime = 0.0f;

   m_timeLogoSpray = game.time () + rg (5.0f, 30.0f);
   m_spawnTime = game.time ();
   m_lastChatTime = game.time ();

   m_timeCamping = 0.0f;
   m_campDirection = 0;
   m_nextCampDirTime = 0;
   m_campButtons = 0;

   m_soundUpdateTime = 0.0f;
   m_heardSoundTime = game.time ();

   m_msgQueue.clear ();
   m_goalHist.clear ();
   m_ignoredBreakable.clear ();

   // ignore enemies for some time if needed
   if (cv_ignore_enemies_after_spawn_time.as <float> () > 0.0f) {
      m_enemyIgnoreTimer = game.time () + cv_ignore_enemies_after_spawn_time.as <float> ();
   }
   else {
      m_enemyIgnoreTimer = 0.0f;
   }

   // and put buying into its message queue
   pushMsgQueue (BotMsg::Buy);
   startTask (Task::Normal, TaskPri::Normal, kInvalidNodeIndex, 0.0f, true);

   // restore fake client bit, just in case
   pev->flags |= FL_CLIENT | FL_FAKECLIENT;

   if (rg.chance (50)) {
      pushChatterMessage (Chatter::NewRound);
   }
   auto thinkFps = cr::clamp (cv_think_fps.as <float> (), 30.0f, 90.0f);
   auto thinkInterval = 1.0f / thinkFps;

   if (game.is (GameFlags::Xash3D)) {
      if (thinkFps < 50) {
         thinkInterval = 1.0f / 50.0f; // xash3d works acceptable at 50fps
      }
   }

   // legacy games behaves strange, when this enabled, disable for xash3d as well if requested
   if (bots.isFrameSkipDisabled ()) {
      thinkInterval = 0.0f;
   }
   m_thinkTimer.interval = thinkInterval;
}

void Bot::resetPathSearchType () {
   const auto morale = m_fearLevel > m_agressionLevel ? rg.chance (30) : rg.chance (70);

   switch (m_personality) {
   default:
   case Personality::Normal:
      m_pathType = morale ? FindPath::Optimal : FindPath::Fast;
      break;

   case Personality::Rusher:
      m_pathType = morale ? FindPath::Fast : FindPath::Optimal;
      break;

   case Personality::Careful:
      m_pathType = morale ? FindPath::Optimal : FindPath::Safe;
      break;
   }

   // if debug goal - set the fastest
   if (cv_debug_goal.as <int> () != kInvalidNodeIndex) {
      m_pathType = FindPath::Fast;
   }

   // no need to be safe on csdm
   if (game.is (GameFlags::CSDM)) {
      m_pathType = FindPath::Fast;
      return;
   }
}

void Bot::kill () {
   // this function kills a bot (not just using ClientKill, but like the CSBot does)
   // base code courtesy of Lazy (from bots-united forums!)

   bots.touchKillerEntity (this);
}

void Bot::kick (bool silent) {
   // this function kick off one bot from the server.
   auto username = pev->netname.chars ();

   if (!(pev->flags & FL_CLIENT) || (pev->flags & FL_DORMANT) || strings.isEmpty (username)) {
      return;
   }
   markStale ();

   game.serverCommand ("kick \"%s\"", username);

   if (!silent) {
      ctrl.msg ("Bot '%s' kicked.", username);
   }
}

void Bot::markStale () {
   // switch chatter icon off
   showChatterIcon (false, true);

   // reset bots ping to default
   fakeping.reset (ent ());

   // mark bot as leaving
   m_isStale = true;

   // wait till threads tear down
   MutexScopedLock lock1 (m_pathFindLock);
   MutexScopedLock lock2 (m_predictLock);

   // clear the bot name
   conf.clearUsedName (this);

   // clear fakeclient bit
   pev->flags &= ~FL_FAKECLIENT;

   // make as not receiving any messages
   pev->flags |= FL_DORMANT;
}

void Bot::setNewDifficulty (int32_t newDifficulty) {
   if (newDifficulty < Difficulty::Noob || newDifficulty > Difficulty::Expert) {
      m_difficulty = Difficulty::Hard;
      m_difficultyData = conf.getDifficultyTweaks (Difficulty::Hard);
   }

   m_difficulty = newDifficulty;
   m_difficultyData = conf.getDifficultyTweaks (newDifficulty);
}

void Bot::updateTeamJoin () {
   // this function handles the selection of teams & class

   if (!m_notStarted) {
      return;
   }
   const auto botTeam = game.getRealPlayerTeam (ent ());

   // cs prior beta 7.0 uses hud-based motd, so press fire once
   if (game.is (GameFlags::Legacy)) {
      pev->button |= IN_ATTACK;
   }

   // check if something has assigned team to us
   else if (botTeam == Team::Terrorist || botTeam == Team::CT) {
      m_notStarted = false;
   }
   else if (botTeam == Team::Unassigned && m_retryJoin > 2) {
      m_startAction = BotMsg::TeamSelect;
   }

   // if bot was unable to join team, and no menus pop-ups, check for stacked team
   if (m_startAction == BotMsg::None) {
      if (++m_retryJoin > 3 && bots.isTeamStacked (m_wantedTeam - 1)) {
         m_retryJoin = 0;

         ctrl.msg ("Could not add bot to the game: Team is stacked (to disable this check, set mp_limitteams and mp_autoteambalance to zero and restart the round).");
         kick ();

         return;
      }
   }

   // handle counter-strike stuff here...
   if (m_startAction == BotMsg::TeamSelect) {
      m_startAction = BotMsg::None; // switch back to idle

      if (m_wantedTeam == -1) {
         char teamJoin = cv_join_team.as <StringRef> ()[0];

         if (teamJoin == 'C' || teamJoin == 'c') {
            m_wantedTeam = 2;
         }
         else if (teamJoin == 'T' || teamJoin == 't') {
            m_wantedTeam = 1;
         }
      }

      if (m_wantedTeam != 1 && m_wantedTeam != 2) {
         const auto &players = bots.countTeamPlayers ();

         // balance the team upon creation, we can't use game auto select (5) from now, as we use enforced skins belows
         // due to we don't know the team bot selected, and TeamInfo messages still shows us we're spectators..

         if (players.first > players.second) {
            m_wantedTeam = 2;
         }
         else if (players.first < players.second) {
            m_wantedTeam = 1;
         }
         else {
            m_wantedTeam = rg (1, 2);
         }
      }

      // select the team the bot wishes to join...
      issueCommand ("menuselect %d", m_wantedTeam);
   }
   else if (m_startAction == BotMsg::ClassSelect) {
      m_startAction = BotMsg::None; // switch back to idle

      // czero has additional models
      const auto maxChoice = game.is (GameFlags::ConditionZero) ? 5 : 4;
      auto enforcedSkin = 0;

      // setup enforced skin based on selected team
      if (m_wantedTeam == 1 || botTeam == Team::Terrorist) {
         enforcedSkin = cv_botskin_t.as <int> ();
      }
      else if (m_wantedTeam == 2 || botTeam == Team::CT) {
         enforcedSkin = cv_botskin_ct.as <int> ();
      }
      enforcedSkin = cr::clamp (enforcedSkin, 0, maxChoice);

      // try to choice manually
      if (m_wantedSkin < 1 || m_wantedSkin > maxChoice) {
         m_wantedSkin = rg (1, maxChoice); // use random if invalid
      }

      // and set enforced if any
      if (enforcedSkin > 0) {
         m_wantedSkin = enforcedSkin;
      }

      // select the class the bot wishes to use...
      issueCommand ("menuselect %d", m_wantedSkin);

      // bot has now joined the game (doesn't need to be started)
      m_notStarted = false;

      // check for greeting other players, since we connected
      if (rg.chance (20)) {
         m_needToSendWelcomeChat = true;
      }
   }
}

void BotManager::captureChatRadio (StringRef cmd, StringRef arg, edict_t *ent) {
   if (game.isBotCmd ()) {
      return;
   }

   if (cmd.startsWith ("say")) {
      const bool alive = game.isAliveEntity (ent);
      int team = -1;

      if (cmd.endsWith ("team")) {
         team = game.getRealPlayerTeam (ent);
      }

      for (const auto &client : util.getClients ()) {
         if (!(client.flags & ClientFlags::Used) || (team != -1 && team != client.team2) || alive != game.isAliveEntity (client.ent)) {
            continue;
         }
         auto target = bots[client.ent];

         if (target != nullptr) {
            target->m_sayTextBuffer.entityIndex = game.indexOfPlayer (ent);

            if (strings.isEmpty (engfuncs.pfnCmd_Args ())) {
               continue;
            }
            target->m_sayTextBuffer.sayText = engfuncs.pfnCmd_Args ();
            target->m_sayTextBuffer.timeNextChat = game.time () + target->m_sayTextBuffer.chatDelay;
         }
      }
   }
   auto &target = util.getClient (game.indexOfPlayer (ent));

   // check if this player alive, and issue something
   if ((target.flags & ClientFlags::Alive) && target.radio != 0 && cmd.startsWith ("menuselect")) {
      auto radioCommand = arg.as <int> ();

      if (radioCommand != 0) {
         radioCommand += 10 * (target.radio - 1);

         if (radioCommand != Radio::RogerThat && radioCommand != Radio::Negative && radioCommand != Radio::ReportingIn) {
            for (const auto &bot : bots) {

               // validate bot
               if (bot->m_team == target.team && ent != bot->ent () && bot->m_radioOrder == 0) {
                  bot->m_radioOrder = radioCommand;
                  bot->m_radioEntity = ent;
               }
            }
         }
         bots.setLastRadioTimestamp (target.team, game.time ());
      }
      target.radio = 0;
   }
   else if (cmd.startsWith ("radio")) {
      target.radio = cmd.substr (5).as <int> ();
   }
}

void BotManager::notifyBombDefuse () {
   // notify all terrorists that CT is starting bomb defusing

   const auto &bombPos = gameState.getBombOrigin ();

   for (const auto &bot : bots) {
      const auto task = bot->getCurrentTaskId ();

      if (!bot->m_defuseNotified
         && bot->m_isAlive
         && task != Task::MoveToPosition
         && task != Task::DefuseBomb
         && task != Task::EscapeFromBomb) {

         if (bot->m_team == Team::Terrorist && bot->pev->origin.distanceSq (bombPos) < cr::sqrf (512.0f)) {
            bot->clearSearchNodes ();

            bot->m_pathType = FindPath::Fast;
            bot->m_position = bombPos;
            bot->m_defuseNotified = true;

            bot->startTask (Task::MoveToPosition, TaskPri::MoveToPosition, kInvalidNodeIndex, 0.0f, true);
         }
      }
   }
}

void BotManager::selectLeaders (int team, bool reset) {
   auto &leaderChoosen = m_teamData[team].leaderChoosen;

   if (reset) {
      leaderChoosen = false;
      return;
   }

   if (leaderChoosen) {
      return;
   }
   auto &leaderChoosenT = m_teamData[Team::Terrorist].leaderChoosen;
   auto &leaderChoosenCT = m_teamData[Team::CT].leaderChoosen;

   if (game.mapIs (MapFlags::Assassination)) {
      if (team == Team::CT && !leaderChoosenCT) {
         for (const auto &bot : m_bots) {
            if (bot->m_isVIP) {
               bot->m_isLeader = true; // vip bot is the leader

               if (rg.chance (50)) {
                  bot->pushRadioMessage (Radio::FollowMe);
                  bot->m_campButtons = 0;
               }
            }
         }
         leaderChoosenCT = true;
      }
      else if (team == Team::Terrorist && !leaderChoosenT) {
         auto bot = bots.findHighestFragBot (team);

         if (bot != nullptr && bot->m_isAlive) {
            bot->m_isLeader = true;

            if (rg.chance (45)) {
               bot->pushRadioMessage (Radio::FollowMe);
            }
         }
         leaderChoosenT = true;
      }
   }
   else if (game.mapIs (MapFlags::Demolition)) {
      if (team == Team::Terrorist && !leaderChoosenT) {
         for (const auto &bot : m_bots) {
            if (bot->m_hasC4) {
               // bot carrying the bomb is the leader
               bot->m_isLeader = true;

               // terrorist carrying a bomb needs to have some company
               if (rg.chance (75)) {
                  if (cv_radio_mode.as <int> () == 2) {
                     bot->pushChatterMessage (Chatter::GoingToPlantBomb);
                  }
                  else {
                     bot->pushChatterMessage (Radio::FollowMe);
                  }
                  bot->m_campButtons = 0;
               }
            }
         }
         leaderChoosenT = true;
      }
      else if (!leaderChoosenCT) {
         if (auto bot = bots.findHighestFragBot (team)) {
            bot->m_isLeader = true;

            if (rg.chance (30)) {
               bot->pushRadioMessage (Radio::FollowMe);
            }
         }
         leaderChoosenCT = true;
      }
   }
   else if (game.mapIs (MapFlags::Escape | MapFlags::KnifeArena | MapFlags::FightYard)) {
      auto bot = bots.findHighestFragBot (team);

      if (!leaderChoosen && bot) {
         bot->m_isLeader = true;

         if (rg.chance (30)) {
            bot->pushRadioMessage (Radio::FollowMe);
         }
         leaderChoosen = true;
      }
   }
   else {
      auto bot = bots.findHighestFragBot (team);

      if (!leaderChoosen && bot) {
         bot->m_isLeader = true;

         if (rg.chance (team == Team::Terrorist ? 30 : 40)) {
            bot->pushRadioMessage (Radio::FollowMe);
         }
         leaderChoosen = true;
      }
   }
}

void BotManager::initRound () {
   // this is called at the start of each round

   // check team economics
   for (int team = 0; team < kGameTeamNum; ++team) {
      updateTeamEconomics (team);
      selectLeaders (team, true);

      m_teamData[team].lastRadioTimestamp = 0.0f;
   }
   reset ();

   // notify all bots about new round arrived
   for (const auto &bot : bots) {
      bot->newRound ();
   }

   // reset current radio message for all client
   for (auto &client : util.getClients ()) {
      client.radio = 0;
   }
   graph.clearVisited ();

   m_bombSayStatus = BombPlantedSay::ChatSay | BombPlantedSay::Chatter;
   m_plantSearchUpdateTime = 0.0f;
   m_autoKillCheckTime = 0.0f;
   m_botsCanPause = false;

   resetFilters ();
   practice.update (); // update practice data on round start
}

void BotThreadWorker::shutdown () {
   if (!available ()) {
      return;
   }
   game.print ("Shutting down bot thread worker.");
   m_pool->shutdown ();

   // re-start pool completely
   m_pool.release ();
}

void BotThreadWorker::startup (int workers) {
   StringRef disableWorkerEnv = plat.env ("YAPB_SINGLE_THREADED");

   // disable on legacy games
   const bool isLegacyGame = game.is (GameFlags::Legacy);

   // do not do any threading when timescale enabled
   ConVarRef timescale ("sys_timescale");

   // disable worker if requested via env variable or workers are disabled
   if (isLegacyGame
      || workers == 0
      || timescale.value () > 0
      || (!disableWorkerEnv.empty () && disableWorkerEnv == "1")) {
      return;
   }
   m_pool = cr::makeUnique <ThreadPool> ();

   // define worker threads
   const auto count = m_pool->threadCount ();

   if (count > 0) {
      logger.error ("Tried to start thread pool with existing %d threads in pool.", count);
      return;
   }
   const auto maxThreads = plat.hardwareConcurrency ();
   auto requestedThreads = workers;

   if (requestedThreads < 0 || requestedThreads >= maxThreads) {
      requestedThreads = 1;
   }
   requestedThreads = cr::clamp (requestedThreads, 1, maxThreads - 1);

   // notify user
   game.print ("Starting up bot thread worker with %d threads.", requestedThreads);

   // start up the worker
   m_pool->startup (static_cast <size_t> (requestedThreads));
}

bool BotManager::isFrameSkipDisabled () {
   if (game.is (GameFlags::Legacy)) {
      return true;
   }

   if (game.is (GameFlags::Xash3D) && cv_think_fps_disable) {
      static ConVarRef sys_ticrate ("sys_ticrate");

      // ignore think_fps_disable if fps is more than 100 on xash dedicated server
      if (game.isDedicated () && sys_ticrate.value () > 100.0f) {
         cv_think_fps_disable.set (0);

         return false;
      }
      return true;
   }
   return false;
}
