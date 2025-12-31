/*
** wl_net.h
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

#ifndef __WL_NET_H__
#define __WL_NET_H__

#include <SDL_net.h>

#include "id_in.h"
#include "wl_def.h"
#include "zstring.h"

namespace Net {

typedef bool (*InitStatusCallback)(FString);

enum Mode
{
	MODE_SinglePlayer,
	MODE_Host,
	MODE_Client
};

enum GameMode
{
	GM_Cooperative,
	GM_Battle
};

struct NetInit
{
	Mode mode;
	GameMode gameMode;
	uint16_t port;
	byte numPlayers;
	const char* joinAddress;
};

extern NetInit InitVars;

bool IsArbiter();
bool IsBlocked();
void BlockPlaysim();
void DebugKey(const struct DebugCmd &cmd);
void EndGame();
void Init(InitStatusCallback callback);
void NewGame(int &difficulty, class FString &map, class FName (&playerClassNames)[MAXPLAYERS]);
void PollControls();

bool CheckAck(bool send);
void StartAck(AckType type);

// TODO: Redo these as proper options (and probably move to wl_game or something)
static bool FriendlyFire() { return InitVars.gameMode == GM_Battle; }
static bool RespawnItems() { return InitVars.gameMode == GM_Battle; }
static bool NoMonsters() { return InitVars.gameMode == GM_Battle; }

}

#endif
