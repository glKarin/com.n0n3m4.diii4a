//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

void MessageDispatcher::netMsgTextMsg () {
   enum args { msg = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }

   // lookup cached message
   const auto cached = m_textMsgCache[m_args[msg].chars_];

   // check if we're need to handle message
   if (!(cached & TextMsgCache::NeedHandle)) {
      return;
   }

   // reset bomb position for all the bots
   const auto resetBombPosition = [] () -> void {
      if (game.mapIs (MapFlags::Demolition)) {
         graph.setBombOrigin (true);
      }
   };

   if (cached & TextMsgCache::Commencing) {
      util.setNeedForWelcome (true);
   }
   else if (cached & TextMsgCache::CounterWin) {
      bots.setLastWinner (Team::CT); // update last winner for economics
      resetBombPosition ();
   }
   else if (cached & TextMsgCache::RestartRound) {
      bots.updateTeamEconomics (Team::CT, true);
      bots.updateTeamEconomics (Team::Terrorist, true);

      // set balance for all players
      bots.forEach ([] (Bot *bot) {
         bot->m_moneyAmount = mp_startmoney.as <int> ();
         return false;
      });

      resetBombPosition ();
   }
   else if (cached & TextMsgCache::TerroristWin) {
      bots.setLastWinner (Team::Terrorist); // update last winner for economics
      resetBombPosition ();
   }
   else if ((cached & TextMsgCache::BombPlanted) && !bots.isBombPlanted ()) {
      bots.setBombPlanted (true);

      for (const auto &notify : bots) {
         if (notify->m_isAlive) {
            notify->clearSearchNodes ();

            // clear only camp tasks
            notify->clearTask (Task::Camp);

            if (cv_radio_mode.as <int> () == 2 && rg.chance (55) && notify->m_team == Team::CT) {
               notify->pushChatterMessage (Chatter::WhereIsTheC4);
            }
         }
      }
      graph.setBombOrigin ();
   }

   // check for burst fire message
   if (m_bot) {
      if (cached & TextMsgCache::BurstOn) {
         m_bot->m_weaponBurstMode = BurstMode::On;
      }
      else if (cached & TextMsgCache::BurstOff) {
         m_bot->m_weaponBurstMode = BurstMode::Off;
      }
   }
}

void MessageDispatcher::netMsgVGUIMenu () {
   // this message is sent when a VGUI menu is displayed.

   enum args { menu = 0, min = 1 };

   // check the minimum states or existence of bot
   if (m_args.length () < min || !m_bot) {
      return;
   }

   switch (m_args[menu].long_) {
   case GuiMenu::TeamSelect:
      m_bot->m_startAction = BotMsg::TeamSelect;
      break;

   case GuiMenu::TerroristSelect:
   case GuiMenu::CTSelect:
      m_bot->m_startAction = BotMsg::ClassSelect;
      break;
   }
}

void MessageDispatcher::netMsgShowMenu () {
   // this message is sent when a text menu is displayed.

   enum args { menu = 3, min = 4 };

   // check the minimum states or existence of bot
   if (m_args.length () < min || !m_bot) {
      return;
   }
   const auto cached = m_showMenuCache[m_args[menu].chars_];

   // only assign if non-zero
   if (cached > 0) {
      m_bot->m_startAction = cached;
   }
}

void MessageDispatcher::netMsgWeaponList () {
   // this message is sent when a client joins the game. All of the weapons are sent with the weapon ID and information about what ammo is used.

   enum args { classname = 0, ammo_index_1 = 1, max_ammo_1 = 2, slot = 5, slot_pos = 6, id = 7, flags = 8, min = 9 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }

   // store away this weapon with it's ammo information...
   auto &prop = conf.getWeaponProp (m_args[id].long_);

   prop.classname = m_args[classname].chars_;
   prop.ammo1 = m_args[ammo_index_1].long_;
   prop.ammo1Max = m_args[max_ammo_1].long_;
   prop.slot = m_args[slot].long_;
   prop.pos = m_args[slot_pos].long_;
   prop.id = m_args[id].long_;
   prop.flags = m_args[flags].long_;
}

void MessageDispatcher::netMsgCurWeapon () {
   // this message is sent when a weapon is selected (either by the bot choosing a weapon or by the server auto assigning the bot a weapon). In CS it's also called when Ammo is increased/decreased

   enum args { state = 0, id = 1, clip = 2, min = 3 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }

   if (m_args[id].long_ < kMaxWeapons) {
      if (m_args[state].long_ != 0) {
         m_bot->m_currentWeapon = m_args[id].long_;
         m_bot->m_weaponType = conf.getWeaponType (m_args[id].long_);
      }

      // ammo amount decreased ? must have fired a bullet...
      if (m_args[id].long_ == m_bot->m_currentWeapon && m_bot->m_ammoInClip[m_args[id].long_] > m_args[clip].long_) {
         m_bot->m_timeLastFired = game.time (); // remember the last bullet time
      }
      m_bot->m_ammoInClip[m_args[id].long_] = m_args[clip].long_;
   }
}

void MessageDispatcher::netMsgAmmoX () {
   // this message is sent whenever ammo amounts are adjusted (up or down). NOTE: Logging reveals that CS uses it very unreliable!

#if 1
   netMsgAmmoPickup ();
#else
   enum args { index = 0, value = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   m_bot->m_ammo[m_args[index].long_] = m_args[value].long_; // store it away
#endif
}

void MessageDispatcher::netMsgAmmoPickup () {
   // this message is sent when the bot picks up some ammo (AmmoX messages are also sent so this message is probably
   // not really necessary except it allows the HUD to draw pictures of ammo that have been picked up.  The bots
   // don't really need pictures since they don't have any eyes anyway.

   enum args { index = 0, value = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   m_bot->m_ammo[m_args[index].long_] = m_args[value].long_; // store it away
}

void MessageDispatcher::netMsgDamage () {
   // this message gets sent when the bots are getting damaged.

   enum args { armor = 0, health = 1, bits = 2, min = 3 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }

   // handle damage if any
   if (m_args[armor].long_ > 0 || m_args[health].long_) {
      m_bot->takeDamage (m_bot->pev->dmg_inflictor, m_args[health].long_, m_args[armor].long_, m_args[bits].long_);
   }
}

void MessageDispatcher::netMsgMoney () {
   // this message gets sent when the bots money amount changes

   enum args { money = 0, min = 1 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   auto amount = m_args[money].long_;

   if (amount < 0) {
      amount = 800;
   }
   else if (amount >= INT32_MAX) {
      amount = 16000;
   }
   m_bot->m_moneyAmount = amount;
}

void MessageDispatcher::netMsgStatusIcon () {
   enum args { enabled = 0, icon = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   // lookup cached icon
   const auto cached = m_statusIconCache[m_args[icon].chars_];

   // check if we're need to handle message
   if (!(cached & TextMsgCache::NeedHandle)) {
      return;
   }

   // handle cases
   if (cached & StatusIconCache::BuyZone) {
      m_bot->m_inBuyZone = (m_args[enabled].long_ != 0);

      // try to equip in buyzone
      m_bot->enteredBuyZone (BuyState::PrimaryWeapon);
   }
   else if (cached & StatusIconCache::Escape) {
      m_bot->m_inEscapeZone = (m_args[enabled].long_ != 0);
   }
   else if (cached & StatusIconCache::Rescue) {
      m_bot->m_inRescueZone = (m_args[enabled].long_ != 0);
   }
   else if (cached & StatusIconCache::VipSafety) {
      m_bot->m_inVIPZone = (m_args[enabled].long_ != 0);
   }
   else if (cached & StatusIconCache::C4) {
      m_bot->m_inBombZone = (m_args[enabled].long_ == 2);
   }
   else if (cached & StatusIconCache::Defuser) {
      m_bot->m_hasDefuser = (m_args[enabled].long_ != 0);
   }
}

void MessageDispatcher::netMsgDeathMsg () {
   // this message gets sent when player kills player

   enum args { killer = 0, victim = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }

   auto killerEntity = game.entityOfIndex (m_args[killer].long_);
   auto victimEntity = game.entityOfIndex (m_args[victim].long_);

   if (game.isNullEntity (killerEntity) || game.isNullEntity (victimEntity) || victimEntity == killerEntity) {
      return;
   }
   bots.handleDeath (killerEntity, victimEntity);
}

void MessageDispatcher::netMsgScreenFade () {
   // this message gets sent when the screen fades (flashbang)

   enum args { r = 3, g = 4, b = 5, alpha = 6, min = 7 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }

   // screen completely faded ?
   if (m_args[r].long_ >= 255 && m_args[g].long_ >= 255 && m_args[b].long_ >= 255 && m_args[alpha].long_ > 170) {
      m_bot->takeBlind (m_args[alpha].long_);
   }
}

void MessageDispatcher::netMsgHLTV () {
   // this message gets sent when new round is started in modern cs versions

   enum args { players = 0, fov = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }

   // need to start new round ? (we're tracking FOV reset message)
   if (m_args[players].long_ == 0 && m_args[fov].long_ == 0) {
      bots.initRound ();
   }
}

void MessageDispatcher::netMsgTeamInfo () {
   // this message gets sent when player team index is changed

   enum args { index = 0, team = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }
   auto &client = util.getClient (m_args[index].long_ - 1);

   // update player team
   client.team2 = m_teamInfoCache[m_args[team].chars_]; // update real team
   client.team = game.is (GameFlags::FreeForAll) ? m_args[index].long_ : client.team2;
}

void MessageDispatcher::netMsgScoreInfo () {
   // this message gets sent when scoreboard info is update, we're use it to track k-d ratio

   enum args { index = 0, score = 1, deaths = 2, class_id = 3, team_id = 4, min = 5 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }
   auto bot = pickBot (index);

   // if we're have bot, set the kd ratio
   if (bot != nullptr) {
      bot->m_kpdRatio = bot->pev->frags / cr::max (static_cast <float> (m_args[deaths].long_), 1.0f);
      bot->m_deathCount = m_args[deaths].long_;
   }
}

void MessageDispatcher::netMsgScoreAttrib () {
   // this message updates the scoreboard attribute for the specified player

   enum args { index = 0, flags = 1, min = 2 };

   // check the minimum states
   if (m_args.length () < min) {
      return;
   }
   auto bot = pickBot (index);

   // if we're have bot, set the vip state
   if (bot != nullptr) {
      constexpr int32_t kPlayerIsVIP = cr::bit (2);

      bot->m_isVIP = !!(m_args[flags].long_ & kPlayerIsVIP);
   }
}

void MessageDispatcher::netMsgBarTime () {
   enum args { enabled = 0, min = 1 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }

   // check if has progress bar
   if (m_args[enabled].long_ > 0) {
      m_bot->m_hasProgressBar = true; // the progress bar on a hud

      // notify bots about defusing has started
      if (game.mapIs (MapFlags::Demolition) && bots.isBombPlanted () && m_bot->m_team == Team::CT) {
         bots.notifyBombDefuse ();
      }
   }
   else {
      m_bot->m_hasProgressBar = false; // no progress bar or disappeared
   }
}

void MessageDispatcher::netMsgItemStatus () {
   enum args { value = 0, min = 1 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   const auto mask = m_args[value].long_;

   m_bot->m_hasNVG = !!(mask & ItemStatus::Nightvision);
   m_bot->m_hasDefuser = !!(mask & ItemStatus::DefusalKit);
}

void MessageDispatcher::netMsgNVGToggle () {
   enum args { value = 0, min = 1 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   m_bot->m_usesNVG = m_args[value].long_ > 0;
}

void MessageDispatcher::netMsgFlashBat () {
   enum args { value = 0, min = 1 };

   // check the minimum states
   if (m_args.length () < min || !m_bot) {
      return;
   }
   m_bot->m_flashLevel = m_args[value].long_;
}

void MessageDispatcher::netMsgResetHUD () {
   if (m_bot) {
      m_bot->spawned ();
   }
   bots.setResetHUD (true);
}

MessageDispatcher::MessageDispatcher () {

   // register wanted message
   auto addWanted = [&] (StringRef name, NetMsg id, MsgFunc handler) -> void {
      m_wanted[name] = id;
      m_handlers[id] = handler;
   };
   reset ();

   // we want to handle next messages
   addWanted ("TextMsg", NetMsg::TextMsg, &MessageDispatcher::netMsgTextMsg);
   addWanted ("VGUIMenu", NetMsg::VGUIMenu, &MessageDispatcher::netMsgVGUIMenu);
   addWanted ("ShowMenu", NetMsg::ShowMenu, &MessageDispatcher::netMsgShowMenu);
   addWanted ("WeaponList", NetMsg::WeaponList, &MessageDispatcher::netMsgWeaponList);
   addWanted ("CurWeapon", NetMsg::CurWeapon, &MessageDispatcher::netMsgCurWeapon);
   addWanted ("AmmoX", NetMsg::AmmoX, &MessageDispatcher::netMsgAmmoX);
   addWanted ("AmmoPickup", NetMsg::AmmoPickup, &MessageDispatcher::netMsgAmmoPickup);
   addWanted ("Damage", NetMsg::Damage, &MessageDispatcher::netMsgDamage);
   addWanted ("Money", NetMsg::Money, &MessageDispatcher::netMsgMoney);
   addWanted ("StatusIcon", NetMsg::StatusIcon, &MessageDispatcher::netMsgStatusIcon);
   addWanted ("DeathMsg", NetMsg::DeathMsg, &MessageDispatcher::netMsgDeathMsg);
   addWanted ("ScreenFade", NetMsg::ScreenFade, &MessageDispatcher::netMsgScreenFade);
   addWanted ("HLTV", NetMsg::HLTV, &MessageDispatcher::netMsgHLTV);
   addWanted ("TeamInfo", NetMsg::TeamInfo, &MessageDispatcher::netMsgTeamInfo);
   addWanted ("BarTime", NetMsg::BarTime, &MessageDispatcher::netMsgBarTime);
   addWanted ("ItemStatus", NetMsg::ItemStatus, &MessageDispatcher::netMsgItemStatus);
   addWanted ("NVGToggle", NetMsg::NVGToggle, &MessageDispatcher::netMsgNVGToggle);
   addWanted ("FlashBat", NetMsg::FlashBat, &MessageDispatcher::netMsgFlashBat);
   addWanted ("ScoreInfo", NetMsg::ScoreInfo, &MessageDispatcher::netMsgScoreInfo);
   addWanted ("ScoreAttrib", NetMsg::ScoreAttrib, &MessageDispatcher::netMsgScoreAttrib);
   addWanted ("ResetHUD", NetMsg::ResetHUD, &MessageDispatcher::netMsgResetHUD);

   // we're need next messages IDs but we're won't handle them, so they will be removed from wanted list as soon as they get engine IDs
   addWanted ("BotVoice", NetMsg::BotVoice, nullptr);
   addWanted ("SendAudio", NetMsg::SendAudio, nullptr);
   addWanted ("SayText", NetMsg::SayText, nullptr);

   // register text msg cache
   m_textMsgCache["#CTs_Win"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#Bomb_Defused"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#Bomb_Planted"] = TextMsgCache::NeedHandle | TextMsgCache::BombPlanted;
   m_textMsgCache["#Terrorists_Win"] = TextMsgCache::NeedHandle | TextMsgCache::TerroristWin;
   m_textMsgCache["#Round_Draw"] = TextMsgCache::NeedHandle | TextMsgCache::RestartRound;
   m_textMsgCache["#All_Hostages_Rescued"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#Target_Saved"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#Hostages_Not_Rescued"] = TextMsgCache::NeedHandle | TextMsgCache::TerroristWin;
   m_textMsgCache["#Terrorists_Not_Escaped"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#VIP_Not_Escaped"] = TextMsgCache::NeedHandle | TextMsgCache::TerroristWin;
   m_textMsgCache["#Escaping_Terrorists_Neutralized"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#VIP_Assassinated"] = TextMsgCache::NeedHandle | TextMsgCache::TerroristWin;
   m_textMsgCache["#VIP_Escaped"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#Terrorists_Escaped"] = TextMsgCache::NeedHandle | TextMsgCache::TerroristWin;
   m_textMsgCache["#CTs_PreventEscape"] = TextMsgCache::NeedHandle | TextMsgCache::CounterWin;
   m_textMsgCache["#Target_Bombed"] = TextMsgCache::NeedHandle | TextMsgCache::TerroristWin;
   m_textMsgCache["#Game_Commencing"] = TextMsgCache::NeedHandle | TextMsgCache::Commencing;
   m_textMsgCache["#Game_will_restart_in"] = TextMsgCache::NeedHandle | TextMsgCache::RestartRound;
   m_textMsgCache["#Switch_To_BurstFire"] = TextMsgCache::NeedHandle | TextMsgCache::BurstOn;
   m_textMsgCache["#Switch_To_SemiAuto"] = TextMsgCache::NeedHandle | TextMsgCache::BurstOff;
   m_textMsgCache["#Switch_To_FullAuto"] = TextMsgCache::NeedHandle | TextMsgCache::BurstOff;

   // register show menu cache
   m_showMenuCache["#Team_Select"] = BotMsg::TeamSelect;
   m_showMenuCache["#Team_Select_Spect"] = BotMsg::TeamSelect;
   m_showMenuCache["#IG_Team_Select_Spect"] = BotMsg::TeamSelect;
   m_showMenuCache["#IG_Team_Select"] = BotMsg::TeamSelect;
   m_showMenuCache["#IG_VIP_Team_Select"] = BotMsg::TeamSelect;
   m_showMenuCache["#IG_VIP_Team_Select_Spect"] = BotMsg::TeamSelect;
   m_showMenuCache["#Terrorist_Select"] = BotMsg::ClassSelect;
   m_showMenuCache["#CT_Select"] = BotMsg::ClassSelect;

   // register status icon cache
   m_statusIconCache["buyzone"] = StatusIconCache::NeedHandle | StatusIconCache::BuyZone;
   m_statusIconCache["escape"] = StatusIconCache::NeedHandle | StatusIconCache::Escape;
   m_statusIconCache["rescue"] = StatusIconCache::NeedHandle | StatusIconCache::Rescue;
   m_statusIconCache["vipsafety"] = StatusIconCache::NeedHandle | StatusIconCache::VipSafety;
   m_statusIconCache["c4"] = StatusIconCache::NeedHandle | StatusIconCache::C4;
   m_statusIconCache["defuser"] = StatusIconCache::NeedHandle | StatusIconCache::Defuser;

   // register team info cache
   m_teamInfoCache["TERRORIST"] = Team::Terrorist;
   m_teamInfoCache["UNASSIGNED"] = Team::Unassigned;
   m_teamInfoCache["SPECTATOR"] = Team::Spectator;
   m_teamInfoCache["CT"] = Team::CT;
}

int32_t MessageDispatcher::add (StringRef name, int32_t id) {
   if (!m_wanted.exists (name)) {
      return id;
   }

   m_maps[m_wanted[name]] = id; // add message from engine regusermsg
   m_reverseMap[id] = m_wanted[name]; // add message from engine regusermsg

   return id;
}

void MessageDispatcher::start (edict_t *ent, int32_t type) {
   reset ();

   if (game.is (GameFlags::Metamod)) {
      ensureMessages ();
   }

   // search if we need to handle this message
   if (m_reverseMap.exists (type)) {
      const auto msg = m_reverseMap[type];
      m_current = m_handlers[msg] ? msg : NetMsg::None;
   }

   // no message no processing
   if (m_current == NetMsg::None) {
      return;
   }

   // message for bot bot?
   if (!game.isNullEntity (ent) && !(ent->v.flags & FL_DORMANT)) {
      m_bot = bots[ent];

      if (!m_bot) {
         stopCollection ();
         return;
      }
   }
   m_args.clear (); // clear previous args
}

void MessageDispatcher::stop () {
   if (m_current == NetMsg::None) {
      return;
   }
   (this->*m_handlers[m_current]) ();

   stopCollection ();
}

void MessageDispatcher::ensureMessages () {
   // we're getting messages ids in regusermsg for metamod, but when we're unloaded, we're lost our ids on next 'meta load'.
   // this function tries to associate appropriate message ids.

   // check if we're have one
   if (m_maps.exists (NetMsg::Money)) {
      return;
   }

   // re-register our message
   m_wanted.foreach ([&] (const String &key, const int32_t &) {
      add (key, MUTIL_GetUserMsgID (PLID, key.chars (), nullptr));
   });
}

int32_t MessageDispatcher::id (NetMsg msg) {
   return m_maps[msg];
}

Bot *MessageDispatcher::pickBot (int32_t index) {
   const auto &client = util.getClient (m_args[index].long_ - 1);

   // get the bot in this message
   return bots[client.ent];
}
