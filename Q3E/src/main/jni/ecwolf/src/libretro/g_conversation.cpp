/*
** g_conversation.h
**
**---------------------------------------------------------------------------
** Copyright 2014 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** This ended up being similar to the USDF parser in ZDoom in many ways.
**
*/

#include <climits>

#include "g_conversation.h"
#include "m_classes.h"
#include "m_random.h"
#include "scanner.h"
#include "tarray.h"
#include "w_wad.h"

#include "a_inventory.h"
#include "farchive.h"
#include "g_mapinfo.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "language.h"
#include "v_text.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "thingdef/thingdef.h"
#include "state_machine.h"

namespace Dialog {

// ----------------------------------------------------------------------------


void StartConversation(AActor *npc)
{
	const Page **page = FindConversation(npc);
	if(!page)
		return;

	QuizMenu *quiz = new QuizMenu();
	quiz->loadQuestion(*page);
	Menu::closeMenus(false); // Clear out any main menu state
	g_state.isInQuiz = true;
	g_state.quizMenu = quiz;
	g_state.quizPage = *page;
	g_state.quizNpc = npc;
}

void closeQuiz(wl_state_t *state)
{
	state->isInQuiz = false;
	delete state->quizMenu;
	state->quizMenu = NULL;

	// Fix screen
	StatusBar->RefreshBackground();
	DrawPlayScreen();
}
	
static void updateQuestion(wl_state_t *state, Page *nextPage) {
	state->quizPage = state->quizPage->Choices[0].NextPage;
	if (!state->quizNpc)
		return;
	const Page **page = FindConversation(state->quizNpc);
	if(!page)
		return;
	*page = nextPage;
}
	
bool quizHandle(wl_state_t *state, const wl_input_state_t *input)
{
	if (input->menuBack)
	{
		// If the player escapes then they get nothing.

		// For S3DNA, proceed to the next page. Probably should factor this
		// into an option.
		if(state->quizPage->Choices.Size() > 0) {
			updateQuestion(state, state->quizPage->Choices[0].NextPage);
		}
		closeQuiz(state);
		return false;
	}

	if (input->menuEnter)
	{
		int answer = state->quizMenu->getCurrentPosition();
		const Choice &choice = state->quizPage->Choices[answer];
		FString response = choice.YesMessage;
		if(response[0] == '$')
			response = language[response.Mid(1)];
		GiveConversationItem(players[0].mo, choice.GiveItem);

		state->quizMenu->drawBackground();

		int size = BigFont->StringWidth(response);
		screen->DrawText(BigFont, gameinfo.FontColors[GameInfo::MENU_SELECTION], 160-size/2, 88, response,
				 DTA_Clean, true,
				 TAG_DONE);
		
		VW_UpdateScreen();

		updateQuestion(state, choice.NextPage);
		if(choice.CloseDialog)
			closeQuiz(state);
		else
			state->quizMenu->loadQuestion(state->quizPage);
		State_Delay(state, 140);
		return true;
	}

	
	state->quizMenu->handleStep(state, input);

	return true;
}

void quizSerialize(wl_state_t *state, FArchive &arc)
{
	const Conversation *conv = NULL;
	arc << state->quizNpc;

	if (state->quizNpc)
	{
		const unsigned int id = state->quizNpc->GetClass()->Meta.GetMetaInt(AMETA_ConversationID);
		for(unsigned int i = LoadedModules.Size();i-- > 0;)
		{
			conv = LoadedModules[i].Find(id);
			if (conv)
				break;
		}
	}

	DWORD pos = 0;
	DWORD pagenum = 0;
	if (arc.IsStoring()) {
		pos = state->quizMenu ? state->quizMenu->getCurrentPosition() : 0;
		if (conv != NULL && conv->Pages.Size() && state->quizPage) {
			pagenum = state->quizPage - &conv->Pages[0];
			if (pagenum > conv->Pages.Size())
				pagenum = 0;
		}
	}
	
	arc << pagenum;
	arc << pos;
	if (!arc.IsStoring() && g_state.isInQuiz) {
		Page *page = &conv->Pages[pagenum];

		QuizMenu *quiz = new QuizMenu();
		quiz->loadQuestion(page);
		g_state.quizMenu = quiz;
		g_state.quizPage = page;
	}
}

}
