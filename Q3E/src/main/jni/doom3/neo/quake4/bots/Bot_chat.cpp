// Bot_Chat.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

const char *bot_kill_insult[] = {
	"%s: Try aiming next time %s",
	"%s: Trust me when I say this %s, you suck.",
	"%s: Maybe you should go back to go cart racing %s?",
	"%s: Your technique reminds me of a story %s, a very dull one."
};

const char* bot_death_insult[] = {
	"%s: Mmmmmmm. Was it good for you too %s?",
	"%s: Well I guess %s won the lottery.",	
	"%s: My mother fragged me once %s. Once.",
	"%s: I'm gonna pull out your bowels %s",
	"%s: Beginners luck %s... Again",
	"%s: So %s... you're up to what ... 3 frags an hour?"
};

const char* bot_death_praise[] = {
	"%s: %s not bad for an amateur.",
	"%s: %s alright that was pretty good. Your still a dousche.",
	"%s: That was definitely ... um ... pretty good %s",
	"%s: I've seen better %s, but not many.",
	"%s: Take a moment to reflect on your accomplishment %s",
	"%s: Your pretty good for a dousche %s"
};

/*
====================
rvmBot::BotSendChatMessage
====================
*/
void rvmBot::BotSendChatMessage(botChat_t chat, const char* targetName) {
	switch (chat)
	{
		case KILL:
			gameLocal.mpGame.AddChatLine(bot_kill_insult[rvRandom::irand(0, 3)], GetNetName(), targetName);
			break;
		case DEATH:
			if (rvRandom::irand(0, 10) < 5)
			{
				gameLocal.mpGame.AddChatLine(bot_death_insult[rvRandom::irand(0, 5)], GetNetName(), targetName);
			}
			else
			{
				gameLocal.mpGame.AddChatLine(bot_death_praise[rvRandom::irand(0, 5)], GetNetName(), targetName);
			}
			break;
	}
}
