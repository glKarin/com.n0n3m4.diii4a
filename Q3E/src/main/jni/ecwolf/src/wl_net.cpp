/*
** wl_net.cpp
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

#include <SDL.h>
#include <SDL_net.h>

#include "wl_def.h"
#include "id_in.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_net.h"
#include "m_swap.h"
#include "m_random.h"
#include "doomerrors.h"
#include "zdoomsupport.h"
#include "zstring.h"

#define NET_DEFAULT_PORT 5029
#define MAXEXTRATICS 4

namespace Net {

enum
{
	NET_RequestConnection,
	NET_ConnectionStart,
	NET_Ack,
	NET_TicCmd
};

#pragma pack(1)
struct RequestPacket
{
	// This could be static const BYTE, but I know old Mac compilers I use
	// sometimes have problems with that. :(
	enum { Type = NET_RequestConnection };

	BYTE type;
};

struct StartPacket
{
	enum { Type = NET_ConnectionStart };

	BYTE type;
	BYTE playerNumber;
	BYTE numPlayers;
	DWORD rngseed;
	struct Client
	{
		DWORD host;
		WORD port;
	} clients[];
};

struct AckPacket
{
	enum { Type = NET_Ack };

	BYTE type;
	int32_t TimeCount;
};

struct TicCmdPacket
{
	enum { Type = NET_TicCmd };

	BYTE type;
	int32_t TimeCount;
	int controlx;
	int controly;
	int controlstrafe;
	bool buttonstate[NUMBUTTONS];
	bool buttonheld[NUMBUTTONS];
};
#pragma pack()

NetInit InitVars = {
	MODE_SinglePlayer,
	NET_DEFAULT_PORT,
	1
};

struct NetClient
{
	IPaddress address;
	TicCmdPacket extratics[MAXEXTRATICS];
	unsigned short extrapos;
};

static NetClient Client[MAXPLAYERS];
static UDPsocket Socket;
static UDPpacket *Packet;

// Just so that we know something is happening do a little animation.
static const char* const Waiting[4] = {"   ", ".  ", ".. ", "..." };

static FString IPaddressToString(IPaddress address)
{
	FString out;
	out.Format("%u.%u.%u.%u:%d", address.host&0xFF, (address.host&0xFF00)>>8, (address.host&0xFF0000)>>16, (address.host&0xFF000000)>>24, BigShort(address.port));
	return out;
}

static FString BuildClientList(bool acked[MAXPLAYERS])
{
	FString clients;

	for(unsigned i = 1;i < InitVars.numPlayers;++i)
	{
		FString client;
		if(Client[i].address.host)
			client.Format("%2u: %-21s %-6s", i, IPaddressToString(Client[i].address).GetChars(), acked[i] ? "Synced" : "");
		else
			client.Format("%2u: %-21s %-6s", i, "Waiting...", "");

		clients += FString(i != 1 ? "\n": "") + client;
	}
	return clients;
}

// Returns the player number for a given ip address
static int FindClient(IPaddress address)
{
	for(unsigned int i = 0;i < InitVars.numPlayers;++i)
	{
		if(Client[i].address.host == address.host && Client[i].address.port == address.port)
			return i;
	}
	return -1;
}

// Check if we have a potentially valid packet of a certain type
template<typename T>
static bool CheckPacketType(const UDPpacket *packet)
{
	return packet->len >= (signed)sizeof(T) && ((T*)packet->data)->type == T::Type;
}

// Sends an ACK packet to a given address
static void SendAck(IPaddress address, int32_t TimeCount)
{
	AckPacket ackData;
	ackData.type = AckPacket::Type;
	ackData.TimeCount = TimeCount;
	UDPpacket packet = { -1, (Uint8*)&ackData, sizeof(AckPacket), sizeof(AckPacket), 0, address };

	SDLNet_UDP_Send(Socket, -1, &packet);
}

static void StartHost(InitStatusCallback callback)
{
	unsigned int waitpos = 0;
	unsigned int nextclient = 1; // 0 is the host
	bool acked[MAXPLAYERS] = { false };

	if(!(Socket = SDLNet_UDP_Open(InitVars.port)))
		throw CFatalError("Could not open UDP socket.");

	// Step 1: Get connection requests from each player
	Printf("Waiting for %d players:\n   ", InitVars.numPlayers);
	callback(BuildClientList(acked));
	while(nextclient != InitVars.numPlayers)
	{
		waitpos = (waitpos+1)%4;
		Printf("\b\b\b%s", Waiting[waitpos]);
		fflush(stdout);

		if(SDLNet_UDP_Recv(Socket, Packet))
		{
			const RequestPacket *data = reinterpret_cast<RequestPacket*>(Packet->data);
			if(CheckPacketType<RequestPacket>(Packet))
			{
				Printf("\b\b\b");

				int client = FindClient(Packet->address);
				if(client == -1)
				{
					Printf("[%d] New connection from %u.%u.%u.%u:%u!\n", nextclient, Packet->address.host&0xFF, (Packet->address.host&0xFF00)>>8, (Packet->address.host&0xFF0000)>>16, (Packet->address.host&0xFF000000)>>24, BigShort(Packet->address.port));
					Client[nextclient++].address = Packet->address;
					callback(BuildClientList(acked));
				}

				Printf("   ");
			}
		}
		else
		{
			SDL_Delay(100);
			IN_ProcessEvents();
		}
	}

	// Once we found all of the players, send a syncronization packet which
	// contains anything needs to start the game as well as the address of each
	// other player in the game (they already know the host).
	Printf("\b\b\bAll players connected! Sending sync...\n");

	const int startSize = sizeof(StartPacket) + sizeof(StartPacket::Client)*(InitVars.numPlayers-1);
	StartPacket *startData = (StartPacket *)malloc(startSize);
	UDPpacket startPacket = { -1, (Uint8*)startData, startSize, startSize, 0 };
	startData->type = StartPacket::Type;
	startData->numPlayers = InitVars.numPlayers;
	startData->rngseed = rngseed;
	for(unsigned int i = 1;i < InitVars.numPlayers;++i)
	{
		startData->clients[i-1].host = Client[i].address.host;
		startData->clients[i-1].port = Client[i].address.port;
	}

	nextclient = 1;
	while(nextclient != InitVars.numPlayers)
	{
		// Send off start packet to players who have not acked
		for(unsigned int i = 1;i < InitVars.numPlayers;++i)
		{
			if(acked[i])
				continue;

			startPacket.address = Client[i].address;
			startData->playerNumber = i;

			SDLNet_UDP_Send(Socket, -1, &startPacket);
		}

		SDL_Delay(100);
		IN_ProcessEvents();

		// Look for ack packets
		while(SDLNet_UDP_Recv(Socket, Packet))
		{
			const AckPacket *data = reinterpret_cast<AckPacket*>(Packet->data);
			if(CheckPacketType<AckPacket>(Packet))
			{
				int client = FindClient(Packet->address);
				if(client > 0)
				{
					acked[client] = true;
					++nextclient;
					callback(BuildClientList(acked));
				}
			}
		}
	}

	free(startData);

	Printf("All acked starting game!\n");
}

static void StartJoin(InitStatusCallback callback)
{
	unsigned int waitpos = 0;
	if(!(Socket = SDLNet_UDP_Open(InitVars.port)))
		throw CFatalError("Could not open UDP socket.");
	IPaddress address;

	// Convert join string to IPaddress
	FString addrString(InitVars.joinAddress);
	uint16_t port = NET_DEFAULT_PORT;
	if(addrString.IndexOf(':') != -1)
	{
		long pos = addrString.IndexOf(':');
		port = atoi(addrString.Mid(pos+1));
		addrString = addrString.Left(pos);
	}
	SDLNet_ResolveHost(&address, addrString, port);

	Printf("Attempting to connect to %u.%u.%u.%u:%u :\n   ", address.host&0xFF, (address.host&0xFF00)>>8, (address.host&0xFF0000)>>16, (address.host&0xFF000000)>>24, BigShort(address.port));

	// Send a connection request to host
	Uint8 requestData[] = {NET_RequestConnection};
	UDPpacket packet = { -1, requestData, 1, 1, 0, address };

	callback("Waiting for sync");
	for(;;)
	{
		waitpos = (waitpos+1)%4;
		Printf("\b\b\b%s", Waiting[waitpos]);
		fflush(stdout);

		// Send request periodically as a heart beat
		if(waitpos == 0)
		{
			SDLNet_UDP_Send(Socket, -1, &packet);
		}

		// Look for start sync packets
		if(SDLNet_UDP_Recv(Socket, Packet))
		{
			const StartPacket *data = reinterpret_cast<StartPacket *>(Packet->data);
			if(CheckPacketType<StartPacket>(Packet))
			{
				ConsolePlayer = data->playerNumber;
				InitVars.numPlayers = data->numPlayers;
				rngseed = data->rngseed;

				Client[0].address = Packet->address;
				for(unsigned int i = 1;i < InitVars.numPlayers;++i)
				{
					Client[i].address.host = data->clients[i-1].host;
					Client[i].address.port = data->clients[i-1].port;
				}
				break;
			}
		}
		else
		{
			SDL_Delay(100);
			IN_ProcessEvents();
		}
	}

	Printf("\b\b\bRecieved sync from host! Sending ack...\n");
	callback("Sync recieved");

	// Send ACK and forget, if we're waiting for ticcmd and we get a start, we'll send another ack then.
	SendAck(address, 0xFFFFFFFF);
}

static void Shutdown()
{
	SDLNet_FreePacket(Packet);
	SDLNet_UDP_Close(Socket);
}

// Returns true when network init finished
void Init(InitStatusCallback callback)
{
	if(InitVars.mode == MODE_SinglePlayer)
		return;

	if(SDLNet_Init() < 0)
	{
		I_FatalError("Unable to init SDL_net: %s", SDLNet_GetError());
	}

	Packet = SDLNet_AllocPacket(1500);
	atterm(Shutdown);

	if(InitVars.mode == MODE_Host)
		StartHost(callback);
	else
		StartJoin(callback);
}

void PollControls()
{
	bool acked[MAXPLAYERS] = { false };
	bool controls[MAXPLAYERS] = { false };
	unsigned int numAcked = 1;
	unsigned int numControls = 1;
	acked[ConsolePlayer] = true;

	// We need to send a ticcmd to each player in the game.
	TicCmdPacket ticcmdData;
	UDPpacket ticcmdPacket = { -1, (Uint8*)&ticcmdData, sizeof(TicCmdPacket), sizeof(TicCmdPacket), 0 };
	ticcmdData.type = TicCmdPacket::Type;
	ticcmdData.TimeCount = gamestate.TimeCount;
	ticcmdData.controlx = control[ConsolePlayer].controlx;
	ticcmdData.controly = control[ConsolePlayer].controly;
	ticcmdData.controlstrafe = control[ConsolePlayer].controlstrafe;
	memcpy(ticcmdData.buttonstate, control[ConsolePlayer].buttonstate, sizeof(control[ConsolePlayer].buttonstate));
	memcpy(ticcmdData.buttonheld, control[ConsolePlayer].buttonheld, sizeof(control[ConsolePlayer].buttonheld));

	for(unsigned int i = 0;i < MAXEXTRATICS;++i)
	{
		for(unsigned int c = 0;c < InitVars.numPlayers;++c)
		{
			if(c == ConsolePlayer)
				continue;

			if(Client[c].extratics[i].TimeCount == gamestate.TimeCount)
			{
				const TicCmdPacket *data = &Client[c].extratics[i];

				if(controls[c])
					continue;
				controls[c] = true;
				++numControls;

				control[c].controlx = data->controlx;
				control[c].controly = data->controly;
				control[c].controlstrafe = data->controlstrafe;
				memcpy(control[c].buttonstate, data->buttonstate, sizeof(control[c].buttonstate));
				memcpy(control[c].buttonheld, data->buttonheld, sizeof(control[c].buttonheld));
			}
		}
	}

	// We need to keep an eye out for packets, but we also need to periodically
	// resend our ticcmd in case it got lost.
	unsigned int resend = 0;
	while(numAcked != InitVars.numPlayers || numControls != InitVars.numPlayers)
	{
		if(resend == 0)
		{
			for(unsigned int i = 0;i < InitVars.numPlayers;++i)
			{
				if(acked[i])
					continue;

				ticcmdPacket.address = Client[i].address;
				SDLNet_UDP_Send(Socket, -1, &ticcmdPacket);
			}
			resend = 100;
		}

		--resend;
		SDL_Delay(1);
		IN_ProcessEvents();
		while(SDLNet_UDP_Recv(Socket, Packet))
		{
			if(CheckPacketType<TicCmdPacket>(Packet))
			{
				// Controls from another player
				const TicCmdPacket *data = reinterpret_cast<TicCmdPacket *>(Packet->data);
				int client = FindClient(Packet->address);

				if(data->TimeCount != gamestate.TimeCount)
				{
					if(data->TimeCount > gamestate.TimeCount)
					{
						Client[client].extratics[Client[client].extrapos] = *data;
						Client[client].extrapos = (Client[client].extrapos+1)%MAXEXTRATICS;
						SendAck(Packet->address, data->TimeCount);
					}
					continue;
				}

				SendAck(Packet->address, data->TimeCount);

				if(controls[client])
					continue;
				controls[client] = true;
				++numControls;

				control[client].controlx = data->controlx;
				control[client].controly = data->controly;
				control[client].controlstrafe = data->controlstrafe;
				memcpy(control[client].buttonstate, data->buttonstate, sizeof(control[client].buttonstate));
				memcpy(control[client].buttonheld, data->buttonheld, sizeof(control[client].buttonheld));
			}
			else if(CheckPacketType<AckPacket>(Packet))
			{
				const AckPacket *data = reinterpret_cast<AckPacket *>(Packet->data);
				if(data->TimeCount != gamestate.TimeCount)
					continue;

				// Good! A player got our controls.
				int client = FindClient(Packet->address);
				if(acked[client])
					continue;

				acked[FindClient(Packet->address)] = true;
				++numAcked;
			}
			else if(CheckPacketType<StartPacket>(Packet))
			{
				// Host lost our start ack, so send another one
				SendAck(Client[0].address, 0xFFFFFFFF);
			}
		}
	}
}

}
