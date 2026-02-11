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
**
*/

#ifndef __G_CONVERSATION_H__
#define __G_CONVERSATION_H__

#include <zstring.h>
#include "m_classes.h"

class AActor;

namespace Dialog {

struct Page;
struct ItemCheck
{
	unsigned int Item;
	unsigned int Amount;
};
struct Choice
{
	TArray<ItemCheck> Cost;
	FString Text;
	FString YesMessage, NoMessage;
	FString Log;
	FString SelectSound;
	union
	{
		unsigned int NextPageIndex;
		Page *NextPage;
	};
	unsigned int GiveItem;
	unsigned int Special;
	unsigned int Arg[5];
	bool CloseDialog;
	bool DisplayCost;
};
struct Page
{
	TArray<Choice> Choices;
	TArray<ItemCheck> IfItem;
	FString Name;
	FString Panel;
	FString Voice;
	FString Dialog;
	FString Hint;
	union
	{
		unsigned int LinkIndex; // Valid while parsing
		Page *Link;
	};
	unsigned int Drop;
};

class QuizMenu : public Menu
{
public:
	QuizMenu() : Menu(30, 96, 290, 24) {}

	void loadQuestion(const Page *page);

	void drawBackground() const;

	void draw() const;

private:
	FString question;
	FString hint;
};

struct Conversation
{
	TArray<Page> Pages;
	unsigned int Actor;
	bool RandomStart;
	bool Preserve;

	const Page *Start() const;
};

class ConversationModule
{
public:
	enum ConvNamespace
	{
		NS_Strife,
		NS_Noah
	};

	const Conversation *Find(unsigned int id) const;
	void Load(int lump);

	TArray<FString> Include;
	TMap<unsigned int, Conversation> Conversations;
	ConvNamespace Namespace;
	int Lump;

private:
	void ParseConversation(Scanner &sc);
	template<typename T>
	void ParseBlock(Scanner &sc, T &obj, bool (ConversationModule::*handler)(Scanner &, FName, bool, T &));

	bool ParseConvBlock(Scanner &, FName, bool, Conversation &);
	bool ParsePageBlock(Scanner &, FName, bool, Page &);
	bool ParseChoiceBlock(Scanner &, FName, bool, Choice &);
	bool ParseItemCheckBlock(Scanner &, FName, bool, ItemCheck &);
};


extern void ClearConversations();
// Not yet implemented
//extern void LoadMapModules();
extern void LoadGlobalModule(const char* module);
extern void StartConversation(AActor *npc, AActor *pc);

void GiveConversationItem(AActor *recipient, unsigned int id);
const Page **FindConversation(AActor *npc);
void LoadMapModules();
extern TArray<ConversationModule> LoadedModules;

}

#endif
