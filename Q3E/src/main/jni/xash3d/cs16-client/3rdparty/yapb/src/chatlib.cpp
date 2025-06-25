//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_chat ("chat", "1", "Enables or disables bots chat functionality.");
ConVar cv_chat_percent ("chat_percent", "30", "Bot chances to send random dead chat when killed.", true, 0.0f, 100.0f);

BotChatManager::BotChatManager () {
   m_clanTags = {
      { "[[", "]]" }, { "-=", "=-" }, { "-[", "]-" }, { "-]", "[-" },
      { "-}", "{-" }, { "-{", "}-" }, { "<[", "]>" }, { "<]", "[>" },
      { "[-", "-]" }, { "]-", "-[" }, { "{-", "-}" }, { "}-", "-{" },

      { "[", "]" }, { "{", "}" }, { "<", "[" }, { ">", "<" }, { ")", "(" },
      { "-", "-" }, { "|", "|" }, { "=", "=" }, { "+", "+" }, { "(", ")" },
   };
}

void BotChatManager::stripTags (String &line) {
   if (line.empty ()) {
      return;
   }

   for (const auto &tag : m_clanTags) {
      const size_t start = line.find (tag.first, 0);

      if (start != String::InvalidIndex) {
         const size_t end = line.find (tag.second, start);
         const size_t diff = end - start;

         if (end != String::InvalidIndex && end > start && diff < 32 && diff > 1) {
            line.erase (start, diff + tag.second.length ());
            continue;
         }
      }
   }
}

void BotChatManager::humanizePlayerName (String &playerName) {
   if (playerName.empty ()) {
      return;
   }

   // drop tag marks, 80 percent of time
   if (rg.chance (80)) {
      stripTags (playerName);
   }
   else {
      playerName.trim ();
   }

   // sometimes switch name to lower characters, only valid for the english languge
   if (rg.chance (8) && cv_language.as <StringRef> () == "en") {
      playerName.lowercase ();
   }
}

void BotChatManager::addChatErrors (String &line) {
   // sometimes switch name to lower characters, only valid for the english languge
   if (rg.chance (8) && cv_language.as <StringRef> () == "en") {
      line.lowercase ();
   }
   const auto length = static_cast <int32_t> (line.length ());

   if (length > 15) {
      const auto percentile = length / 2;

      // "length / 2" percent of time drop a character
      if (rg.chance (percentile)) {
         auto pos = rg (length / 8, length - length / 8);
         line.erase (static_cast <size_t> (pos));
      }

      // "length" / 4 precent of time swap character
      if (rg.chance (percentile / 2)) {
         auto pos = static_cast <size_t> (rg (length / 8, 3 * length / 8)); // choose random position in string
         cr::swap (line[pos], line[pos + 1]);
      }
   }
}

bool BotChatManager::checkKeywords (StringRef line, String &reply) {
   // this function checks is string contain keyword, and generates reply to it

   if (!cv_chat || line.empty ()) {
      return false;
   }

   for (auto &factory : conf.getReplies ()) {
      for (const auto &keyword : factory.keywords) {

         // check is keyword has occurred in message
         if (line.find (keyword) != String::InvalidIndex) {
            auto &usedReplies = factory.usedReplies;

            if (usedReplies.length () >= factory.replies.length () / 4) {
               usedReplies.clear ();
            }

            if (!factory.replies.empty ()) {
               bool replyUsed = false;
               StringRef choosenReply = factory.replies.random ();

               // don't say this twice
               for (auto &used : usedReplies) {
                  if (used.contains (choosenReply)) {
                     replyUsed = true;
                     break;
                  }
               }

               // reply not used, so use it
               if (!replyUsed) {
                  reply.assign (choosenReply); // update final buffer
                  usedReplies.push (choosenReply); // add to ignore list

                  return true;
               }
            }
         }
      }
   }
   // didn't find a keyword? 70% of the time use some universal reply
   if (rg.chance (70) && conf.hasChatBank (Chat::NoKeyword)) {
      reply.assign (conf.pickRandomFromChatBank (Chat::NoKeyword));
      return true;
   }
   return false;
}

void Bot::prepareChatMessage (StringRef message) {
   // this function parses messages from the botchat, replaces keywords and converts names into a more human style

   if (!cv_chat || message.empty ()) {
      return;
   }
   m_chatBuffer = message;

   // must be called before return or on the end
   auto addChatErrors = [&] () {
      if (!m_chatBuffer.empty ()) {
         chatlib.addChatErrors (m_chatBuffer);
      }
   };

   // need to check if we're have special symbols
   size_t pos = message.find ('%');

   // nothing found, bail out
   if (pos == String::InvalidIndex || pos >= message.length ()) {
      addChatErrors ();
      return;
   }

   // get the humanized name out of client
   auto humanizedName = [] (int index) -> String {
      auto ent = game.playerOfIndex (index);

      if (!util.isPlayer (ent)) {
         return "unknown";
      }
      String playerName = ent->v.netname.chars ();
      chatlib.humanizePlayerName (playerName);

      return playerName;
   };

   // find highfrag player
   auto getHighfragPlayer = [&] () -> String {
      int highestFrags = -1;
      int index = 0;

      for (int i = 0; i < game.maxClients (); ++i) {
         const Client &client = util.getClient (i);

         if (!(client.flags & ClientFlags::Used) || client.ent == ent ()) {
            continue;
         }
         const auto frags = static_cast <int> (client.ent->v.frags);

         if (frags > highestFrags) {
            highestFrags = frags;
            index = i;
         }
      }
      return humanizedName (index);
   };

   // get roundtime
   auto getRoundTime = [] () -> String {
      auto roundTimeSecs = static_cast <int> (bots.getRoundEndTime () - game.time ());

      String roundTime {};
      roundTime.assignf ("%02d:%02d", cr::clamp (roundTimeSecs / 60, 0, 59), cr::clamp (cr::abs (roundTimeSecs % 60), 0, 59));

      return roundTime;
   };

   // get bot's victim
   auto getMyVictim = [&] () -> String {
      return humanizedName (game.indexOfPlayer (m_lastVictim));
   };

   // get the game name alias
   auto getGameName = [] () -> String {
      String gameName {};

      if (game.is (GameFlags::ConditionZero)) {
         if (rg.chance (30)) {
            gameName = "CZ";
         }
         else {
            gameName = "Condition Zero";
         }
      }
      else if (game.is (GameFlags::Modern) || game.is (GameFlags::Legacy)) {
         if (rg.chance (30)) {
            gameName = "CS";
         }
         else {
            gameName = "Counter-Strike";
         }
      }
      return gameName;
   };

   // get enemy or teammate alive
   auto getPlayerAlive = [&] (bool needsEnemy) -> String {
      for (const auto &client : util.getClients ()) {
         if (!(client.flags & ClientFlags::Used) || !(client.flags & ClientFlags::Alive) || client.ent == ent ()) {
            continue;
         }
         const auto playerIndex = game.indexOfPlayer (client.ent);

         if (needsEnemy && m_team != client.team) {
            return humanizedName (playerIndex);
         }
         else if (!needsEnemy && m_team == client.team) {
            if (util.isPlayer (pev->dmg_inflictor)
               && game.getRealTeam (pev->dmg_inflictor) == m_team) {

               return humanizedName (game.indexOfPlayer (pev->dmg_inflictor));
            }
            return humanizedName (playerIndex);
         }
      }
      return getHighfragPlayer ();
   };
   size_t replaceCounter = 0;

   while (replaceCounter < 6 && (pos = m_chatBuffer.find ('%')) != String::InvalidIndex) {
      const auto replacePosition = pos + 1;

      if (replacePosition > m_chatBuffer.length ()) {
         continue;
      }

      // found one, let's do replace
      switch (m_chatBuffer[replacePosition]) {

         // the highest frag player
      case 'f':
         m_chatBuffer.replace ("%f", getHighfragPlayer ());
         break;

         // current map name
      case 'm':
         m_chatBuffer.replace ("%m", game.getMapName ());
         break;

         // round time
      case 'r':
         m_chatBuffer.replace ("%r", getRoundTime ());
         break;

         // chat reply
      case 's':
         m_chatBuffer.replace ("%s", m_sayTextBuffer.entityIndex != -1 ? humanizedName (m_sayTextBuffer.entityIndex) : getHighfragPlayer ());
         break;

         // last bot victim
      case 'v':
         m_chatBuffer.replace ("%v", getMyVictim ());
         break;

         // game name
      case 'd':
         m_chatBuffer.replace ("%d", getGameName ());
         break;

         // teammate alive
      case 't':
         m_chatBuffer.replace ("%t", getPlayerAlive (false));
         break;

         // enemy alive
      case 'e':
         m_chatBuffer.replace ("%e", getPlayerAlive (true));
         break;

      case 'g':
         m_chatBuffer.replace ("%g", graph.getAuthor ());
         break;
      };
      ++replaceCounter;
   }
   addChatErrors ();
}

bool Bot::checkChatKeywords (String &reply) {
   // this function parse chat buffer, and prepare buffer to keyword searching

   return chatlib.checkKeywords (utf8tools.strToUpper (m_sayTextBuffer.sayText), reply);
}

bool Bot::isReplyingToChat () {
   // this function sends reply to a player

   if (m_sayTextBuffer.entityIndex != -1 && !m_sayTextBuffer.sayText.empty ()) {
      // check is time to chat is good
      if (m_sayTextBuffer.timeNextChat < game.time () + rg (m_sayTextBuffer.chatDelay / 2, m_sayTextBuffer.chatDelay)) {
         String replyText {};

         if (rg.chance (m_sayTextBuffer.chatProbability + rg (40, 70)) && checkChatKeywords (replyText)) {
            prepareChatMessage (replyText);
            pushMsgQueue (BotMsg::Say);

            m_sayTextBuffer.entityIndex = -1;
            m_sayTextBuffer.timeNextChat = game.time () + m_sayTextBuffer.chatDelay;
            m_sayTextBuffer.sayText.clear ();

            return true;
         }
         m_sayTextBuffer.entityIndex = -1;
         m_sayTextBuffer.sayText.clear ();
      }
   }
   return false;
}

void Bot::checkForChat () {
   // say a text every now and then

   if (m_isAlive || !cv_chat || game.is (GameFlags::CSDM)) {
      return;
   }

   // bot chatting turned on?
   if (rg.chance (cv_chat_percent.as <int> ())
      && m_lastChatTime + rg (6.0f, 10.0f) < game.time ()
      && bots.getLastChatTimestamp () + rg (2.5f, 5.0f) < game.time ()
      && !isReplyingToChat ()) {

      if (conf.hasChatBank (Chat::Dead)) {
         StringRef phrase = conf.pickRandomFromChatBank (Chat::Dead);
         bool sayBufferExists = false;

         // search for last messages, sayed
         for (auto &sentence : m_sayTextBuffer.lastUsedSentences) {
            if (phrase.startsWith (sentence)) {
               sayBufferExists = true;
               break;
            }
         }

         if (!sayBufferExists) {
            prepareChatMessage (phrase);
            pushMsgQueue (BotMsg::Say);

            m_lastChatTime = game.time ();
            bots.setLastChatTimestamp (game.time ());

            // add to ignore list
            m_sayTextBuffer.lastUsedSentences.push (phrase);
         }
      }

      // clear the used line buffer every now and then
      if (static_cast <int> (m_sayTextBuffer.lastUsedSentences.length ()) > rg (4, 6)) {
         m_sayTextBuffer.lastUsedSentences.clear ();
      }
   }
}



void Bot::sendToChat (StringRef message, bool teamOnly) {
   // this function prints saytext message to all players

   if (m_isCreature || message.empty () || !cv_chat) {
      return;
   }

   // special handling for legacy games
   if (game.is (GameFlags::Legacy)) {
      sendToChatLegacy (message, teamOnly);
   }
   else {
      issueCommand ("%s \"%s\"", teamOnly ? "say_team" : "say", message);
   }
}

void Bot::sendToChatLegacy (StringRef message, bool teamOnly) {
   // this function prints saytext message to all players for legacy games (< cs 1.6)

   // note: for some reason using regular say & say_team for sending chat messages on hlds on a legacy games
   // causes buffer overruns somewhere in gamedll Host_Say function, thus crashing the game randomly.
   // so this function mimics what legacy gamedll is doing in their Host_Say.

   bool dedicatedSend = false;

   auto sendChatMsg = [&] (const Client &client, String chatMsg) {
      if (game.isDedicated () && !dedicatedSend) {
         dedicatedSend = true;

         game.print ("%s", chatMsg.trim ());
      }
      auto rcv = bots[client.ent];

      if (rcv != nullptr) {
         rcv->m_sayTextBuffer.entityIndex = m_index;

         rcv->m_sayTextBuffer.sayText = message;
         rcv->m_sayTextBuffer.timeNextChat = game.time () + rcv->m_sayTextBuffer.chatDelay;
      }
      else {
         return; // do not send to controlled bots
      }

      if (((client.flags & ClientFlags::Alive) && m_isAlive)
         || (!(client.flags & ClientFlags::Alive) && m_isAlive)
         || (!(client.flags & ClientFlags::Alive) && !m_isAlive)) {

         MessageWriter (MSG_ONE, msgs.id (NetMsg::SayText), nullptr, client.ent)
            .writeByte (m_index)
            .writeString (chatMsg.chars ());
      }
   };

   if (teamOnly) {
      StringRef teamName {};

      if (m_team == Team::Terrorist) {
         teamName = "(Terrorist)";
      }
      else if (m_team == Team::CT) {
         teamName = "(Counter-Terrorist)";
      }

      for (const auto &client : util.getClients ()) {
         if (!(client.flags & ClientFlags::Used) || client.team2 != m_team || client.ent == ent ()) {
            continue;
         }
         String chatMsg {};

         if (m_isAlive) {
            chatMsg.appendf ("%c%s %c%s%c :  %s\n", 0x01, teamName, 0x03, pev->netname.chars (), 0x01, message);
         }
         else {
            chatMsg.appendf ("%c*DEAD*%s %c%s%c :  %s\n", 0x01, teamName, 0x03, pev->netname.chars (), 0x01, message);
         }
         sendChatMsg (client, chatMsg);
      }
      return;
   }

   for (const auto &client : util.getClients ()) {
      if (!(client.flags & ClientFlags::Used) || client.ent == ent ()) {
         continue;
      }
      String chatMsg {};

      if (m_isAlive) {
         chatMsg.appendf ("%c%s :  %s\n", 0x02, pev->netname.chars (), message);
      }
      else {
         chatMsg.appendf ("%c*DEAD* %c%s%c :  %s\n", 0x01, 0x03, pev->netname.chars (), 0x01, message);
      }
      sendChatMsg (client, chatMsg);
   }
}
