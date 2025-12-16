//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// links keywords and replies together
struct ChatKeywords {
   StringArray keywords {};
   StringArray replies {};
   StringArray usedReplies {};

public:
   ChatKeywords () = default;

   ChatKeywords (const StringArray &keywords, const StringArray &replies) {
      this->keywords.clear ();
      this->replies.clear ();
      this->usedReplies.clear ();

      this->keywords.insert (0, keywords);
      this->replies.insert (0, replies);
   }
};

// define chatting collection structure
struct ChatCollection {
   int chatProbability {};
   float chatDelay {};
   float timeNextChat {};
   int entityIndex {};
   String sayText {};
   StringArray lastUsedSentences {};
};


// bot's chat manager
class BotChatManager : public Singleton <BotChatManager> {
private:
   SmallArray <Twin <String, String>> m_clanTags {}; // strippable clan tags

public:
   BotChatManager ();
   ~BotChatManager () = default;

public:
   // chat helper to strip the clantags out of the string
   void stripTags (String &line);

   // chat helper to make player name more human-like
   void humanizePlayerName (String &playerName);

   // chat helper to add errors to the bot chat string
   void addChatErrors (String &line);

   // chat helper to find keywords for given string
   bool checkKeywords (StringRef line, String &reply);
};

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (BotChatManager, chatlib);
