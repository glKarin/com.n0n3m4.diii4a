/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "NetworkSystem.h"

idNetworkSystem		networkSystemLocal;
idNetworkSystem 	*networkSystem = &networkSystemLocal;


/*
==================
idNetworkSystem::ServerSendReliableMessage
==================
*/
void idNetworkSystem::ServerSendReliableMessage(int clientNum, const idBitMsg &msg)
{
	if (idAsyncNetwork::server.IsActive()) {
		idAsyncNetwork::server.SendReliableGameMessage(clientNum, msg);
	}
}

/*
==================
idNetworkSystem::ServerSendReliableMessageExcluding
==================
*/
void idNetworkSystem::ServerSendReliableMessageExcluding(int clientNum, const idBitMsg &msg)
{
	if (idAsyncNetwork::server.IsActive()) {
		idAsyncNetwork::server.SendReliableGameMessageExcluding(clientNum, msg);
	}
}

/*
==================
idNetworkSystem::ServerGetClientPing
==================
*/
int idNetworkSystem::ServerGetClientPing(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientPing(clientNum);
	}

	return 0;
}

/*
==================
idNetworkSystem::ServerGetClientPrediction
==================
*/
int idNetworkSystem::ServerGetClientPrediction(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientPrediction(clientNum);
	}

	return 0;
}

/*
==================
idNetworkSystem::ServerGetClientTimeSinceLastPacket
==================
*/
int idNetworkSystem::ServerGetClientTimeSinceLastPacket(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientTimeSinceLastPacket(clientNum);
	}

	return 0;
}

/*
==================
idNetworkSystem::ServerGetClientTimeSinceLastInput
==================
*/
int idNetworkSystem::ServerGetClientTimeSinceLastInput(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientTimeSinceLastInput(clientNum);
	}

	return 0;
}

/*
==================
idNetworkSystem::ServerGetClientOutgoingRate
==================
*/
int idNetworkSystem::ServerGetClientOutgoingRate(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientOutgoingRate(clientNum);
	}

	return 0;
}

/*
==================
idNetworkSystem::ServerGetClientIncomingRate
==================
*/
int idNetworkSystem::ServerGetClientIncomingRate(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientIncomingRate(clientNum);
	}

	return 0;
}

/*
==================
idNetworkSystem::ServerGetClientIncomingPacketLoss
==================
*/
float idNetworkSystem::ServerGetClientIncomingPacketLoss(int clientNum)
{
	if (idAsyncNetwork::server.IsActive()) {
		return idAsyncNetwork::server.GetClientIncomingPacketLoss(clientNum);
	}

	return 0.0f;
}

/*
==================
idNetworkSystem::ClientSendReliableMessage
==================
*/
void idNetworkSystem::ClientSendReliableMessage(const idBitMsg &msg)
{
	if (idAsyncNetwork::client.IsActive()) {
		idAsyncNetwork::client.SendReliableGameMessage(msg);
	} else if (idAsyncNetwork::server.IsActive()) {
		idAsyncNetwork::server.LocalClientSendReliableMessage(msg);
	}
}

/*
==================
idNetworkSystem::ClientGetPrediction
==================
*/
int idNetworkSystem::ClientGetPrediction(void)
{
	if (idAsyncNetwork::client.IsActive()) {
		return idAsyncNetwork::client.GetPrediction();
	}

	return 0;
}

/*
==================
idNetworkSystem::ClientGetTimeSinceLastPacket
==================
*/
int idNetworkSystem::ClientGetTimeSinceLastPacket(void)
{
	if (idAsyncNetwork::client.IsActive()) {
		return idAsyncNetwork::client.GetTimeSinceLastPacket();
	}

	return 0;
}

/*
==================
idNetworkSystem::ClientGetOutgoingRate
==================
*/
int idNetworkSystem::ClientGetOutgoingRate(void)
{
	if (idAsyncNetwork::client.IsActive()) {
		return idAsyncNetwork::client.GetOutgoingRate();
	}

	return 0;
}

/*
==================
idNetworkSystem::ClientGetIncomingRate
==================
*/
int idNetworkSystem::ClientGetIncomingRate(void)
{
	if (idAsyncNetwork::client.IsActive()) {
		return idAsyncNetwork::client.GetIncomingRate();
	}

	return 0;
}

/*
==================
idNetworkSystem::ClientGetIncomingPacketLoss
==================
*/
float idNetworkSystem::ClientGetIncomingPacketLoss(void)
{
	if (idAsyncNetwork::client.IsActive()) {
		return idAsyncNetwork::client.GetIncomingPacketLoss();
	}

	return 0.0f;
}

#ifdef _RAVEN
#include "../Session_local.h"
extern idSessionLocal		sessLocal;

void idNetworkSystem::SetLoadingText(const char* loadingText)
{
	if(sessLocal.guiLoading)
		sessLocal.guiLoading->SetStateString("server_loadinfo", loadingText);
}

void idNetworkSystem::AddLoadingIcon(const char* icon)
{
	const char *name;
	int i;

	if(sessLocal.guiLoading)
	{
		for(i = 0; i < MAX_MP_LOADING_GUI_ICONS; i++)
		{
			name = va("load_icon_%d", i + 1);
			if(sessLocal.guiLoading->GetStateBool(name))
				continue; // used
			sessLocal.guiLoading->SetStateBool(name, true);
			name = va("load_icon_img_%d", i + 1);
			sessLocal.guiLoading->SetStateString(name, icon);
			break;
		}
		sessLocal.guiLoading->SetStateFloat("load_icons", Min(i + 1, MAX_MP_LOADING_GUI_ICONS));
	}
}

const char* idNetworkSystem::GetServerAddress(void)
{
	if(idAsyncNetwork::client.IsActive())
		return Sys_NetAdrToString(idAsyncNetwork::client.serverAddress);
	else if(idAsyncNetwork::server.IsActive())
		return Sys_NetAdrToString(idAsyncNetwork::server.serverPort.GetAdr());
	else
		return "";
}

int idNetworkSystem::GetNumScannedServers(void) { return 0; }

const scannedServer_t* idNetworkSystem::GetScannedServerInfo(int serverNum) { (void)serverNum; return 0; }

void idNetworkSystem::UseSortFunction(const sortInfo_t& sortInfo, bool use) { (void)sortInfo; (void)use; }

void idNetworkSystem::AddSortFunction(const sortInfo_t& sortInfo) { (void)sortInfo; }

bool idNetworkSystem::RemoveSortFunction(const sortInfo_t& sortInfo) { (void)sortInfo; return 0; }

const char* idNetworkSystem::GetClientGUID(int clientNum) { (void)clientNum; return 0; }

int idNetworkSystem::ServerGetClientNum(int clientId) { (void)clientId; return 0; }

int idNetworkSystem::ServerGetServerTime(void) { return 0; }

void idNetworkSystem::AddFriend(int clientNum) { (void)clientNum; }

void idNetworkSystem::RemoveFriend(int clientNum) { (void)clientNum; }
#endif

