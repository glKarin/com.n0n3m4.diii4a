//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_bind_menu_key ("bind_menu_key", "=", "Binds specified key for opening bots menu.", false);
ConVar cv_ignore_cvars_on_changelevel ("ignore_cvars_on_changelevel", "yb_quota,yb_autovacate", "Specifies comma separated list of bot cvars, that will not be overwritten by config on changelevel.", false);

BotConfig::BotConfig () {
   m_chat.resize (Chat::Count);
   m_chatter.resize (Chatter::Count);

   m_weaponProps.resize (kMaxWeapons);
}

void BotConfig::loadConfigs () {
   setupMemoryFiles ();

   loadCustomConfig ();
   loadNamesConfig ();
   loadChatConfig ();
   loadChatterConfig ();
   loadWeaponsConfig ();
   loadLanguageConfig ();
   loadLogosConfig ();
   loadAvatarsConfig ();
   loadDifficultyConfig ();
}

void BotConfig::loadMainConfig (bool isFirstLoad) {
   if (game.is (GameFlags::Legacy)) {
      util.setNeedForWelcome (true);
   }
   setupMemoryFiles ();

   auto needsToIgnoreVar = [] (StringArray &list, const char *needle) {
      for (const auto &var : list) {
         if (var == needle) {
            return true;
         }
      }
      return false;
   };

   String line {};
   MemFile file {};

   // this is does the same as exec of engine, but not overwriting values of cvars specified in cv_ignore_cvars_on_changelevel
   if (openConfig (product.nameLower, "Bot main config file is not found.", &file, false)) {
      while (file.getLine (line)) {
         line.trim ();

         if (isCommentLine (line)) {
            continue;
         }

         if (isFirstLoad) {
            game.serverCommand (line.chars ());
            continue;
         }
         auto keyval = line.split (" ");

         if (keyval.length () > 1) {
            auto ignore = String (cv_ignore_cvars_on_changelevel.as <StringRef> ()).split (",");

            auto key = keyval[0].trim ().chars ();
            auto cvar = engfuncs.pfnCVarGetPointer (key);

            if (cvar != nullptr) {
               auto value = const_cast <char *> (keyval[1].trim ().trim ("\"").trim ().chars ());

               if (needsToIgnoreVar (ignore, key) && !strings.matches (value, cvar->string)) {

                  // preserve quota number if it's zero
                  if (cv_quota.name () == cvar->name && cv_quota.as <int> () <= 0) {
                     engfuncs.pfnCvar_DirectSet (cvar, value);
                     continue;
                  }
                  ctrl.msg ("Bot CVAR '%s' differs from the stored in the config (%s/%s). Ignoring.", cvar->name, cvar->string, value);

                  // ensure cvar will have old value
                  engfuncs.pfnCvar_DirectSet (cvar, cvar->string);
               }
               else {
                  engfuncs.pfnCvar_DirectSet (cvar, value);
               }
            }
            else {
               game.serverCommand (line.chars ());
            }
         }
      }
      file.close ();
   }
   else {
      game.serverCommand (strings.format ("%s cvars save", product.cmdPri));
   }

   // android is a bit hard to play, lower the difficulty by default
   if (plat.android && cv_difficulty.as <int> () > 3) {
      cv_difficulty.set (3);
   }

   // preload custom config
   conf.loadCustomConfig ();

   // bind the correct menu key for bot menu...
   if (!game.isDedicated ()) {
      auto val = cv_bind_menu_key.as <StringRef> ();

      if (!val.empty ()) {
         game.serverCommand ("bind \"%s\" \"yb menu\"", val);
      }
   }
   static const bool disableLogWrite = conf.fetchCustom ("DisableLogFile").startsWith ("yes");

   // disable logger if requested
   logger.disableLogWrite (disableLogWrite);

   if (disableLogWrite) {
      game.print ("Bot logging is disabled.");
   }
}

void BotConfig::loadNamesConfig () {
   setupMemoryFiles ();

   String line {};
   MemFile file {};

   constexpr auto kMaxNameLen = 32;

   // naming initialization
   if (openConfig ("names", "Name configuration file not found.", &file, true)) {
      m_botNames.clear ();

      while (file.getLine (line)) {
         line.trim ();

         if (isCommentLine (line)) {
            continue;
         }
         // max botname is 32 characters
         if (line.length () > kMaxNameLen - 1) {
            line[kMaxNameLen - 1] = kNullChar;
         }
         m_botNames.emplace (line, -1);
      }
      file.close ();
   }
   m_botNames.shuffle ();
}

void BotConfig::loadWeaponsConfig () {
   setupMemoryFiles ();

   auto addWeaponEntries = [] (SmallArray <WeaponInfo> &weapons, bool as, StringRef name, const StringArray &data) {

      // we're have null terminator element in weapons array...
      if (data.length () + 1 != weapons.length ()) {
         logger.error ("%s entry in weapons config is not valid or malformed (%d/%d).", name, data.length (), weapons.length ());

         return;
      }

      for (size_t i = 0; i < data.length (); ++i) {
         if (as) {
            weapons[i].teamAS = data[i].as <int> ();
         }
         else {
            weapons[i].teamStandard = data[i].as <int> ();
         }
      }
   };

   auto addIntEntries = [] (SmallArray <int32_t> &to, StringRef name, const StringArray &data) {
      if (data.length () != to.length ()) {
         logger.error ("%s entry in weapons config is not valid or malformed (%d/%d).", name, data.length (), to.length ());
         return;
      }

      for (size_t i = 0; i < to.length (); ++i) {
         to[i] = data[i].as <int> ();
      }
   };
   String line {};
   MemFile file {};

   // weapon data initialization
   if (openConfig ("weapon", "Weapon configuration file not found. Loading defaults.", &file)) {
      while (file.getLine (line)) {
         line.trim ();

         if (isCommentLine (line)) {
            continue;
         }
         auto pair = line.split ("=");

         if (pair.length () != 2) {
            continue;
         }

         for (auto &trim : pair) {
            trim.trim ();
         }
         auto splitted = pair[1].split (",");

         if (pair[0].startsWith ("MapStandard")) {
            addWeaponEntries (m_weapons, false, pair[0], splitted);
         }
         else if (pair[0].startsWith ("MapAS")) {
            addWeaponEntries (m_weapons, true, pair[0], splitted);
         }

         else if (pair[0].startsWith ("GrenadePercent")) {
            addIntEntries (m_grenadeBuyPrecent, pair[0], splitted);
         }
         else if (pair[0].startsWith ("Economics")) {
            addIntEntries (m_botBuyEconomyTable, pair[0], splitted);
         }
         else if (pair[0].startsWith ("PersonalityNormal")) {
            addIntEntries (m_normalWeaponPrefs, pair[0], splitted);
         }
         else if (pair[0].startsWith ("PersonalityRusher")) {
            addIntEntries (m_rusherWeaponPrefs, pair[0], splitted);
         }
         else if (pair[0].startsWith ("PersonalityCareful")) {
            addIntEntries (m_carefulWeaponPrefs, pair[0], splitted);
         }
      }
      file.close ();
   }
}

void BotConfig::loadChatterConfig () {
   setupMemoryFiles ();

   String line {};
   MemFile file {};

   // chatter initialization
   if (game.is (GameFlags::HasBotVoice) && cv_radio_mode.as <int> () == 2
      && openConfig ("chatter", "Couldn't open chatter system configuration", &file)) {

      m_chatter.clear ();

      static constexpr struct EventMap {
         StringRef name {};
         int code {};
         float repeat {};
      } chatterEventMap[] = {
         { "Radio_CoverMe", Radio::CoverMe, kMaxChatterRepeatInterval },
         { "Radio_YouTakePoint", Radio::YouTakeThePoint, kMaxChatterRepeatInterval },
         { "Radio_HoldPosition", Radio::HoldThisPosition, 10.0f },
         { "Radio_RegroupTeam", Radio::RegroupTeam, 10.0f },
         { "Radio_FollowMe", Radio::FollowMe, 15.0f },
         { "Radio_TakingFire", Radio::TakingFireNeedAssistance, 5.0f },
         { "Radio_GoGoGo", Radio::GoGoGo, kMaxChatterRepeatInterval },
         { "Radio_Fallback", Radio::TeamFallback, kMaxChatterRepeatInterval },
         { "Radio_StickTogether", Radio::StickTogetherTeam, kMaxChatterRepeatInterval },
         { "Radio_GetInPosition", Radio::GetInPositionAndWaitForGo, kMaxChatterRepeatInterval },
         { "Radio_StormTheFront", Radio::StormTheFront, kMaxChatterRepeatInterval },
         { "Radio_ReportTeam", Radio::ReportInTeam, kMaxChatterRepeatInterval },
         { "Radio_Affirmative", Radio::RogerThat, kMaxChatterRepeatInterval },
         { "Radio_EnemySpotted", Radio::EnemySpotted, 4.0f },
         { "Radio_NeedBackup", Radio::NeedBackup, 5.0f },
         { "Radio_SectorClear", Radio::SectorClear, 10.0f },
         { "Radio_InPosition", Radio::ImInPosition, 10.0f },
         { "Radio_ReportingIn", Radio::ReportingIn, 3.0f },
         { "Radio_ShesGonnaBlow", Radio::ShesGonnaBlow, kMaxChatterRepeatInterval },
         { "Radio_Negative", Radio::Negative, kMaxChatterRepeatInterval },
         { "Radio_EnemyDown", Radio::EnemyDown, 10.0f },
         { "Chatter_DiePain", Chatter::DiePain, kMaxChatterRepeatInterval },
         { "Chatter_GoingToPlantBomb", Chatter::GoingToPlantBomb, 5.0f },
         { "Chatter_GoingToGuardEscapeZone", Chatter::GoingToGuardEscapeZone, kMaxChatterRepeatInterval },
         { "Chatter_GoingToGuardRescueZone", Chatter::GoingToGuardRescueZone, kMaxChatterRepeatInterval },
         { "Chatter_GoingToGuardVIPSafety", Chatter::GoingToGuardVIPSafety, kMaxChatterRepeatInterval },
         { "Chatter_RescuingHostages", Chatter::RescuingHostages, kMaxChatterRepeatInterval },
         { "Chatter_TeamKill", Chatter::TeamKill, kMaxChatterRepeatInterval },
         { "Chatter_GuardingEscapeZone", Chatter::GuardingEscapeZone, kMaxChatterRepeatInterval },
         { "Chatter_GuardingVipSafety", Chatter::GuardingVIPSafety, kMaxChatterRepeatInterval },
         { "Chatter_PlantingC4", Chatter::PlantingBomb, 10.0f },
         { "Chatter_InCombat", Chatter::InCombat,  kMaxChatterRepeatInterval },
         { "Chatter_SeeksEnemy", Chatter::SeekingEnemies, kMaxChatterRepeatInterval },
         { "Chatter_Nothing", Chatter::Nothing,  kMaxChatterRepeatInterval },
         { "Chatter_EnemyDown", Chatter::EnemyDown, 10.0f },
         { "Chatter_UseHostage", Chatter::UsingHostages, kMaxChatterRepeatInterval },
         { "Chatter_WonTheRound", Chatter::WonTheRound, kMaxChatterRepeatInterval },
         { "Chatter_QuicklyWonTheRound", Chatter::QuickWonRound, kMaxChatterRepeatInterval },
         { "Chatter_NoEnemiesLeft", Chatter::NoEnemiesLeft, kMaxChatterRepeatInterval },
         { "Chatter_FoundBombPlace", Chatter::FoundC4Plant, 15.0f },
         { "Chatter_WhereIsTheBomb", Chatter::WhereIsTheC4, kMaxChatterRepeatInterval },
         { "Chatter_DefendingBombSite", Chatter::DefendingBombsite, kMaxChatterRepeatInterval },
         { "Chatter_BarelyDefused", Chatter::BarelyDefused, kMaxChatterRepeatInterval },
         { "Chatter_NiceshotCommander", Chatter::NiceShotCommander, 10.0f },
         { "Chatter_ReportingIn", Chatter::ReportingIn, 10.0f },
         { "Chatter_SpotTheBomber", Chatter::SpotTheBomber, 4.3f },
         { "Chatter_VIPSpotted", Chatter::VIPSpotted, 5.3f },
         { "Chatter_FriendlyFire", Chatter::FriendlyFire, 2.1f },
         { "Chatter_GotBlinded", Chatter::Blind, 12.0f },
         { "Chatter_GuardingPlantedC4", Chatter::GuardingPlantedC4, 3.0f },
         { "Chatter_DefusingC4", Chatter::DefusingBomb, 3.0f },
         { "Chatter_FoundC4", Chatter::FoundC4, 5.5f },
         { "Chatter_ScaredEmotion", Chatter::ScaredEmotion, 6.1f },
         { "Chatter_HeardEnemy", Chatter::HeardTheEnemy, 12.8f },
         { "Chatter_SpottedOneEnemy", Chatter::SpottedOneEnemy, 4.0f },
         { "Chatter_SpottedTwoEnemies", Chatter::SpottedTwoEnemies, 4.0f },
         { "Chatter_SpottedThreeEnemies", Chatter::SpottedThreeEnemies, 4.0f },
         { "Chatter_TooManyEnemies", Chatter::TooManyEnemies, 4.0f },
         { "Chatter_SniperWarning", Chatter::SniperWarning, 14.3f },
         { "Chatter_SniperKilled", Chatter::SniperKilled, 12.1f },
         { "Chatter_OneEnemyLeft", Chatter::OneEnemyLeft, 12.5f },
         { "Chatter_TwoEnemiesLeft", Chatter::TwoEnemiesLeft, 12.5f },
         { "Chatter_ThreeEnemiesLeft", Chatter::ThreeEnemiesLeft, 12.5f },
         { "Chatter_NiceshotPall", Chatter::NiceShotPall, 2.0f },
         { "Chatter_GoingToGuardHostages", Chatter::GoingToGuardHostages, 3.0f },
         { "Chatter_GoingToGuardDroppedBomb", Chatter::GoingToGuardDroppedC4, 6.0f },
         { "Chatter_OnMyWay", Chatter::OnMyWay, 1.5f },
         { "Chatter_LeadOnSir", Chatter::LeadOnSir, 5.0f },
         { "Chatter_Pinned_Down", Chatter::PinnedDown, 5.0f },
         { "Chatter_GottaFindTheBomb", Chatter::GottaFindC4, 3.0f },
         { "Chatter_You_Heard_The_Man", Chatter::YouHeardTheMan, 3.0f },
         { "Chatter_Lost_The_Commander", Chatter::LostCommander, 4.5f },
         { "Chatter_NewRound", Chatter::NewRound, 3.5f },
         { "Chatter_CoverMe", Chatter::CoverMe, 3.5f },
         { "Chatter_BehindSmoke", Chatter::BehindSmoke, 3.5f },
         { "Chatter_BombSiteSecured", Chatter::BombsiteSecured, 3.5f },
         { "Chatter_GoingToCamp", Chatter::GoingToCamp, 30.0f },
         { "Chatter_Camp", Chatter::Camping, 10.0f },
         { "Chatter_OnARoll", Chatter::OnARoll, kMaxChatterRepeatInterval},
      };

      while (file.getLine (line)) {
         line.trim ();

         if (isCommentLine (line)) {
            continue;
         }

         StringRef rewriteKey = "RewritePath";
         StringRef eventKey = "Event";

         if (line.startsWith (rewriteKey)) {
            cv_chatter_path.set (line.substr (rewriteKey.length ()).trim ().chars ());
         }
         else if (line.startsWith (eventKey)) {
            auto items = line.substr (eventKey.length ()).split ("=");

            if (items.length () != 2) {
               logger.error ("Error in chatter config file syntax... Please correct all errors.");
               continue;
            }

            for (auto &item : items) {
               item.trim ();
            }
            items[1].trim ("(;)");

            for (const auto &event : chatterEventMap) {
               if (event.name == items.first ().chars ()) {
                  // this does common work of parsing comma-separated chatter line
                  auto sentences = items[1].split (",");
                  sentences.shuffle ();

                  for (auto &sound : sentences) {
                     sound.trim ().trim ("\"");
                     const auto duration = util.getWaveFileDuration (sound.chars ());

                     if (duration > 0.0f) {
                        m_chatter[event.code].emplace (cr::move (sound), event.repeat, duration);
                     }
                     else {
                        game.print ("Warning: Couldn't get duration of sound '%s.wav.", sound);
                     }
                  }
                  sentences.clear ();
               }
            }
         }
      }
      file.close ();
   }
   else {
      cv_radio_mode.set (1);


      // only notify if has bot voice, but failed to open file
      if (game.is (GameFlags::HasBotVoice)) {
         game.print ("Bots chatter communication disabled.");
      }
   }
}

void BotConfig::loadChatConfig () {
   setupMemoryFiles ();

   String line {};
   MemFile file {};

   // chat config initialization
   if (openConfig ("chat", "Chat file not found.", &file, true)) {
      StringArray *chat = nullptr;

      StringArray keywords {};
      StringArray replies {};

      // clear all the stuff before loading new one
      for (auto &item : m_chat) {
         item.clear ();
      }
      m_replies.clear ();

      while (file.getLine (line)) {
         line.trim ();

         if (isCommentLine (line)) {
            continue;
         }

         if (line.startsWith ("[KILLED]")) {
            chat = &m_chat[Chat::Kill];
            continue;
         }
         else if (line.startsWith ("[BOMBPLANT]")) {
            chat = &m_chat[Chat::Plant];
            continue;
         }
         else if (line.startsWith ("[DEADCHAT]")) {
            chat = &m_chat[Chat::Dead];
            continue;
         }
         else if (line.startsWith ("[REPLIES]")) {
            chat = nullptr;
            continue;
         }
         else if (line.startsWith ("[UNKNOWN]")) {
            chat = &m_chat[Chat::NoKeyword];
            continue;
         }
         else if (line.startsWith ("[TEAMATTACK]")) {
            chat = &m_chat[Chat::TeamAttack];
            continue;
         }
         else if (line.startsWith ("[WELCOME]")) {
            chat = &m_chat[Chat::Hello];
            continue;
         }
         else if (line.startsWith ("[TEAMKILL]")) {
            chat = &m_chat[Chat::TeamKill];
            continue;
         }

         if (chat != nullptr) {
            chat->push (line);

         }
         else {
            if (line.startsWith ("@KEY")) {
               if (!keywords.empty () && !replies.empty ()) {
                  m_replies.emplace (keywords, replies);

                  keywords.clear ();
                  replies.clear ();
               }
               keywords.clear ();

               for (const auto &key : line.substr (4).split (",")) {
                  keywords.emplace (utf8tools.strToUpper (key));
               }

               for (auto &keyword : keywords) {
                  keyword.trim ().trim ("\"");
               }
            }
            else if (!keywords.empty () && !line.empty ()) {
               replies.push (line);
            }
         }
      }

      // shuffle chat a bit
      for (auto &item : m_chat) {
         item.shuffle ();
         item.shuffle ();
      }
      file.close ();
   }
   else {
      cv_chat.set (0);
   }
}

void BotConfig::loadLanguageConfig () {
   setupMemoryFiles ();

   if (game.is (GameFlags::Legacy)) {
      return; // legacy versions will use only english translation
   }
   String line {};
   MemFile file {};

   // localizer initialization
   if (openConfig ("lang", "Specified language not found.", &file, true)) {
      String temp {};
      Twin <String, String> lang {};

      auto trimWithoutWs = [] (String in) -> String {
         return in.trim ("\r\n");
      };

      auto pushTranslatedMsg = [&] () {
         m_language[hashLangString (trimWithoutWs (lang.first).chars ())] = trimWithoutWs (lang.second);
      };

      // clear all the translations before new load
      m_language.clear ();

      while (file.getLine (line)) {
         if (isCommentLine (line)) {
            continue;
         }

         if (line.startsWith ("[ORIGINAL]")) {
            if (!temp.empty ()) {
               lang.second = cr::move (temp);
            }

            if (!lang.second.empty () && !lang.first.empty ()) {
               pushTranslatedMsg ();
            }
         }
         else if (line.startsWith ("[TRANSLATED]") && !temp.empty ()) {
            lang.first = cr::move (temp);
         }
         else {
            temp += line;
         }

         // make sure last string is translated
         if (file.eof () && !lang.first.empty ()) {
            lang.second = trimWithoutWs (line);
            pushTranslatedMsg ();
         }
      }
      file.close ();
   }
   else if (cv_language.as <StringRef> () != "en") {
      logger.error ("Couldn't load language configuration");
   }
}

void BotConfig::loadAvatarsConfig () {
   setupMemoryFiles ();

   if (game.is (GameFlags::Legacy) || game.is (GameFlags::Xash3D)) {
      return;
   }

   String line {};
   MemFile file {};

   // avatars initialization
   if (openConfig ("avatars", "Avatars config file not found. Avatars will not be displayed.", &file)) {
      m_avatars.clear ();

      while (file.getLine (line)) {
         if (isCommentLine (line)) {
            continue;
         }
         m_avatars.push (cr::move (line.trim ()));
      }
   }
}

void BotConfig::loadDifficultyConfig () {
   setupMemoryFiles ();

   String line {};
   MemFile file {};

   // initialize defaults
   m_difficulty[Difficulty::Noob] = {
      { 0.8f, 1.0f }, 5, 0, 0, 38, { 30.0f, 30.0f, 40.0f }
   };

   m_difficulty[Difficulty::Easy] = {
      { 0.6f, 0.8f }, 30, 10, 10, 32, { 15.0f, 15.0f, 24.0f }
   };

   m_difficulty[Difficulty::Normal] = {
      { 0.4f, 0.6f }, 50, 30, 40, 26, { 5.0f, 5.0f, 10.0f }
   };

   m_difficulty[Difficulty::Hard] = {
      { 0.2f, 0.4f }, 75, 60, 70, 23, { 0.0f, 0.0f, 0.0f }
   };

   m_difficulty[Difficulty::Expert] = {
      {  0.1f, 0.2f }, 100, 90, 90, 21, { 0.0f, 0.0f, 0.0f }
   };

   // currently, mindelay, maxdelay, headprob, seenthruprob, heardthruprob, recoil, aim_error {x,y,z}
   constexpr uint32_t kMaxDifficultyValues = 9;

   // has errors ?
   int errorCount = 0;

   // helper for parsing each level
   auto parseLevel = [&] (int32_t level, StringRef data) {
      auto values = data.split <String> (",");

      if (values.length () != kMaxDifficultyValues) {
         ++errorCount;
         return;
      }
      auto diff = &m_difficulty[level];

      diff->reaction[0] = values[0].as <float> ();
      diff->reaction[1] = values[1].as <float> ();
      diff->headshotPct = values[2].as <int> ();
      diff->seenThruPct = values[3].as <int> ();
      diff->hearThruPct = values[4].as <int> ();
      diff->maxRecoil = values[5].as <int> ();
      diff->aimError.x = values[6].as <float> ();
      diff->aimError.y = values[7].as <float> ();
      diff->aimError.z = values[8].as <float> ();
   };

   // difficulty initialization
   if (openConfig ("difficulty", "Difficulty config file not found. Loading defaults.", &file)) {

      while (file.getLine (line)) {
         if (isCommentLine (line) || line.length () < 3) {
            continue;
         }
         auto items = line.split ("=");

         if (items.length () != 2) {
            logger.error ("Error in difficulty config file syntax... Please correct all errors.");
            continue;
         }
         const auto &key = items[0].trim ();

         // get our keys
         if (key == "Noob") {
            parseLevel (Difficulty::Noob, items[1]);
         }
         else if (key == "Easy") {
            parseLevel (Difficulty::Easy, items[1]);
         }
         else if (key == "Normal") {
            parseLevel (Difficulty::Normal, items[1]);
         }
         else if (key == "Hard") {
            parseLevel (Difficulty::Hard, items[1]);
         }
         else if (key == "Expert") {
            parseLevel (Difficulty::Expert, items[1]);
         }
      }

      // if some errors occurred, notify user
      if (errorCount > 0) {
         logger.error ("Config file: difficulty.%s has a bad syntax. Probably out of date.", kConfigExtension);
      }
   }
}

void BotConfig::loadMapSpecificConfig () {
   auto mapSpecificConfig = strings.joinPath (folders.config, "maps", strings.format ("%s.%s", game.getMapName (), kConfigExtension));

   // check existence of file
   if (plat.fileExists (strings.joinPath (bstor.getRunningPath (), mapSpecificConfig).chars ())) {
      auto mapSpecificConfigForExec = strings.joinPath (bstor.getRunningPathVFS (), mapSpecificConfig);
      mapSpecificConfigForExec.replace ("\\", "/");

      game.serverCommand ("exec %s", mapSpecificConfigForExec);
      ctrl.msg ("Executed map-specific config: %s", mapSpecificConfigForExec);
   }
}

void BotConfig::loadCustomConfig () {
   String line {};
   MemFile file {};

   auto setDefaults = [&] () {
      m_custom = {
         { "C4ModelName",  "c4.mdl" },
         { "AMXParachuteCvar",  "sv_parachute" },
         { "CustomCSDMSpawnPoint",  "view_spawn" },
         { "CSDMDetectCvar", "csdm_active" },
         { "ZMDetectCvar", "zp_delay" },
         { "ZMDelayCvar",  "zp_delay" },
         { "ZMInfectedTeam", "T" },
         { "EnableFakeBotFeatures", "no" },
         { "DisableLogFile", "no" }
      };
   };

   setDefaults ();

   // has errors ?
   int errorCount = 0;

   // custom initialization
   if (openConfig ("custom", "Custom config file not found. Loading defaults.", &file)) {
      m_custom.clear ();

      // set defaults anyway
      setDefaults ();

      while (file.getLine (line)) {
         line.trim ();

         if (isCommentLine (line)) {
            continue;
         }
         auto values = line.split ("=");

         if (values.length () != 2) {
            ++errorCount;
            continue;
         }
         auto kv = Twin <String, String> (values[0].trim (), values[1].trim ());

         if (!kv.first.empty () && !kv.second.empty ()) {
            m_custom[kv.first] = kv.second;
         }
      }

      // if some errors occurred, notify user
      if (errorCount > 0) {
         logger.error ("Config file: custom.%s has a bad syntax. Probably out of date.", kConfigExtension);
      }
   }
}

void BotConfig::loadLogosConfig () {
   setupMemoryFiles ();

   String line {};
   MemFile file {};

   auto addLogoIndex = [&] (StringRef logo) {
      const auto index = engfuncs.pfnDecalIndex (logo.chars ());

      if (index > 0) {
         m_logosIndices.push (index);
      }
   };
   m_logosIndices.clear ();

   // logos initialization
   if (openConfig ("logos", "Logos config file not found. Loading defaults.", &file)) {
      while (file.getLine (line)) {
         if (isCommentLine (line)) {
            continue;
         }
         addLogoIndex (line);
      }
   }

   // use defaults
   if (m_logosIndices.empty ()) {
      auto defaults = String { "{biohaz;{graf003;{graf004;{graf005;{lambda06;{target;{hand1;{spit2;{bloodhand6;{foot_l;{foot_r" }.split (";");

      for (const auto &logo : defaults) {
         addLogoIndex (logo);
      }
   }
}

void BotConfig::setupMemoryFiles () {
   static bool setMemoryPointers = true;

   auto wrapLoadFile = [] (const char *filename, int *length) {
      return engfuncs.pfnLoadFileForMe (filename, length);
   };

   auto wrapFreeFile = [] (void *buffer) {
      engfuncs.pfnFreeFile (buffer);
   };

   if (setMemoryPointers) {
      MemFileStorage::instance ().initizalize (cr::move (wrapLoadFile), cr::move (wrapFreeFile));
      setMemoryPointers = false;
   }
}

BotName *BotConfig::pickBotName () {
   if (m_botNames.empty ()) {
      return nullptr;
   }

   for (size_t i = 0; i < m_botNames.length () * 2; ++i) {
      auto bn = &m_botNames.random ();

      if (bn->name.empty () || bn->usedBy != -1) {
         continue;
      }
      return bn;
   }
   return nullptr;
}

void BotConfig::clearUsedName (Bot *bot) {
   for (auto &bn : m_botNames) {
      if (bn.usedBy == bot->index ()) {
         bn.usedBy = -1;
         break;
      }
   }
}

void BotConfig::setBotNameUsed (const int index, StringRef name) {
   for (auto &bn : m_botNames) {
      if (bn.name == name) {
         bn.usedBy = index;
         break;
      }
   }
}

void BotConfig::initWeapons () {
   m_weapons.clear ();

   // fill array with available weapons
   m_weapons = {
      { Weapon::Knife, "weapon_knife", "knife.mdl", 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, WeaponType::Melee, true },
      { Weapon::USP, "weapon_usp", "usp.mdl", 500, 1, -1, -1, 1, 1, 2, 2, 0, 12, WeaponType::Pistol, false },
      { Weapon::Glock18, "weapon_glock18", "glock18.mdl", 400, 1, -1, -1, 1, 2, 1, 1, 0, 20, WeaponType::Pistol, false },
      { Weapon::Deagle, "weapon_deagle", "deagle.mdl", 650, 1, 2, 2, 1, 3, 4, 4, 2, 7, WeaponType::Pistol, false },
      { Weapon::P228, "weapon_p228", "p228.mdl", 600, 1, 2, 2, 1, 4, 3, 3, 0, 13, WeaponType::Pistol, false },
      { Weapon::Elite, "weapon_elite", "elite.mdl", 800, 1, 0, 0, 1, 5, 5, 5, 0, 30, WeaponType::Pistol, false },
      { Weapon::FiveSeven, "weapon_fiveseven", "fiveseven.mdl", 750, 1, 1, 1, 1, 6, 5, 5, 0, 20, WeaponType::Pistol, false },
      { Weapon::M3, "weapon_m3", "m3.mdl", 1700, 1, 2, -1, 2, 1, 1, 1, 0, 8, WeaponType::Shotgun, false },
      { Weapon::XM1014, "weapon_xm1014", "xm1014.mdl", 3000, 1, 2, -1, 2, 2, 2, 2, 0, 7, WeaponType::Shotgun, false },
      { Weapon::MP5, "weapon_mp5navy", "mp5.mdl", 1500, 1, 2, 1, 3, 1, 2, 2, 0, 30, WeaponType::SMG, true },
      { Weapon::TMP, "weapon_tmp", "tmp.mdl", 1250, 1, 1, 1, 3, 2, 1, 1, 0, 30, WeaponType::SMG, true },
      { Weapon::P90, "weapon_p90", "p90.mdl", 2350, 1, 2, 1, 3, 3, 4, 4, 0, 50, WeaponType::SMG, true },
      { Weapon::MAC10, "weapon_mac10", "mac10.mdl", 1400, 1, 0, 0, 3, 4, 1, 1, 0, 30, WeaponType::SMG, true },
      { Weapon::UMP45, "weapon_ump45", "ump45.mdl", 1700, 1, 2, 2, 3, 5, 3, 3, 0, 25, WeaponType::SMG, true },
      { Weapon::AK47, "weapon_ak47", "ak47.mdl", 2500, 1, 0, 0, 4, 1, 2, 2, 2, 30, WeaponType::Rifle, true },
      { Weapon::SG552, "weapon_sg552", "sg552.mdl", 3500, 1, 0, -1, 4, 2, 4, 4, 2, 30, WeaponType::ZoomRifle, true },
      { Weapon::M4A1, "weapon_m4a1", "m4a1.mdl", 3100, 1, 1, 1, 4, 3, 3, 3, 2, 30, WeaponType::Rifle, true },
      { Weapon::Galil, "weapon_galil", "galil.mdl", 2000, 1, 0, 0, 4, -1, 1, 1, 2, 35, WeaponType::Rifle, true },
      { Weapon::Famas, "weapon_famas", "famas.mdl", 2250, 1, 1, 1, 4, -1, 1, 1, 2, 25, WeaponType::Rifle, true },
      { Weapon::AUG, "weapon_aug", "aug.mdl", 3500, 1, 1, 1, 4, 4, 4, 4, 2, 30, WeaponType::ZoomRifle, true },
      { Weapon::Scout, "weapon_scout", "scout.mdl", 2750, 1, 2, 0, 4, 5, 3, 2, 3, 10, WeaponType::Sniper, false },
      { Weapon::AWP, "weapon_awp", "awp.mdl", 4750, 1, 2, 0, 4, 6, 5, 6, 3, 10, WeaponType::Sniper, false },
      { Weapon::G3SG1, "weapon_g3sg1", "g3sg1.mdl", 5000, 1, 0, 2, 4, 7, 6, 6, 3, 20, WeaponType::Sniper, false },
      { Weapon::SG550, "weapon_sg550", "sg550.mdl", 4200, 1, 1, 1, 4, 8, 5, 5, 3, 30, WeaponType::Sniper, false },
      { Weapon::M249, "weapon_m249", "m249.mdl", 5750, 1, 2, 1, 5, 1, 1, 1, 2, 100, WeaponType::Heavy, true },
      { Weapon::Shield, "weapon_shield", "shield.mdl", 2200, 0, 1, 1, 8, -1, 8, 8, 0, 0, WeaponType::Pistol, false },

      // not needed actually, but cause too much refactoring for now. todo
      { 0, "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, WeaponType::None, false }
   };
}

void BotConfig::adjustWeaponPrices () {
   // elite price is 1000$ on older versions of cs...
   if (!(game.is (GameFlags::Legacy))) {
      return;
   }

   for (auto &weapon : m_weapons) {
      if (weapon.id == Weapon::Elite) {
         weapon.price = 1000;
         break;
      }
   }
}

WeaponInfo &BotConfig::findWeaponById (const int id) {
   for (auto &weapon : m_weapons) {
      if (weapon.id == id) {
         return weapon;
      }
   }
   return m_weapons.at (0);
}

const char *BotConfig::translate (StringRef input) {
   // this function translate input string into needed language

   if (ctrl.ignoreTranslate ()) {
      return input.chars ();
   }
   auto hash = hashLangString (input.chars ());

   if (m_language.exists (hash)) {
      return m_language[hash].chars ();
   }
   return input.chars (); // nothing found
}

void BotConfig::showCustomValues () {
   ctrl.msg ("Current values for custom config items:");

   m_custom.foreach ([&] (const String &key, const String &val) {
      ctrl.msg ("  %s = %s", key, val);
   });
}

uint32_t BotConfig::hashLangString (StringRef str) {
   auto test = [] (const char ch) {
      return  (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
   };

   String res {};

   for (const auto &ch : str) {
      if (!test (ch)) {
         continue;
      }
      res += ch;
   }
   return res.empty () ? 0 : res.hash ();
}

bool BotConfig::openConfig (StringRef fileName, StringRef errorIfNotExists, MemFile *outFile, bool languageDependant /*= false*/) {
   if (*outFile) {
      outFile->close ();
   }

   // save config dir
   auto configDir = strings.joinPath (bstor.getRunningPathVFS (), folders.config);

   if (languageDependant) {
      if (fileName.startsWith ("lang") && cv_language.as <StringRef> () == "en") {
         return false;
      }
      auto langConfig = strings.joinPath (configDir, folders.lang, strings.format ("%s_%s.%s", cv_language.as <StringRef> (), fileName, kConfigExtension));

      // check is file is exists for this language
      if (!outFile->open (langConfig)) {
         outFile->open (strings.joinPath (configDir, folders.lang, strings.format ("en_%s.%s", fileName, kConfigExtension)));
      }
   }
   else {
      outFile->open (strings.joinPath (configDir, strings.format ("%s.%s", fileName, kConfigExtension)));
   }

   if (!*outFile) {
      logger.error (errorIfNotExists.chars ());
      return false;
   }
   return true;
}
