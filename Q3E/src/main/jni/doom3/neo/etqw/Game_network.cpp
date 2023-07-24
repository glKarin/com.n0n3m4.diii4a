// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "gamesys/SysCmds.h"
#include "rules/GameRules.h"
#include "rules/AdminSystem.h"
#include "Entity.h"
#include "Player.h"
#include "structures/TeamManager.h"
#include "client/ClientEntity.h"
#include "client/ClientEffect.h"
#include "structures/DeployRequest.h"
#include "roles/FireTeams.h"
#include "roles/ObjectiveManager.h"
#include "roles/Tasks.h"
#include "demos/DemoManager.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "effects/Effects.h"
#include "effects/Wakes.h"
#include "effects/TireTread.h"
#include "effects/FootPrints.h"
#include "rules/VoteManager.h"
#include "roles/WayPointManager.h"
#include "vehicles/Transport.h"
#include "proficiency/StatsTracker.h"
#include "guis/UserInterfaceManager.h"

#include "../bse/BSEInterface.h"
#include "../bse/BSE_Envelope.h"
#include "../bse/BSE_SpawnDomains.h"
#include "../bse/BSE_Particle.h"
#include "../bse/BSE.h"

#include "botai/BotThreadData.h"
#include "botai/Bot.h"

#include "misc/ProfileHelper.h"
#include "AntiLag.h"

extern rvBSEManager* bse;

/*
===============================================================================

	Client running game code:

===============================================================================
*/

idCVar net_clientShowSnapshot( "net_clientShowSnapshot", "0", CVAR_GAME | CVAR_INTEGER, "", 0, 4, idCmdSystem::ArgCompletion_Integer<0,4> );
idCVar net_clientShowSnapshotRadius( "net_clientShowSnapshotRadius", "128", CVAR_GAME | CVAR_FLOAT, "" );
idCVar net_clientShowAOR( "net_clientShowAOR", "0", CVAR_GAME | CVAR_INTEGER, "", 0, 3, idCmdSystem::ArgCompletion_Integer< 0, 3 > );
idCVar net_clientAORFilter( "net_clientAORFilter", "0", CVAR_GAME, "", 0, 0, idClass::ArgCompletion_ClassName );
idCVar net_clientMaxPrediction( "net_clientMaxPrediction", "1000", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "maximum number of milliseconds a client can predict ahead of server." );
idCVar net_clientLagOMeter( "net_clientLagOMeter", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | CVAR_ARCHIVE | CVAR_PROFILE, "draw prediction graph" );

idCVar net_clientSelfSmoothing( "net_clientSelfSmoothing", "1", CVAR_GAME | CVAR_BOOL, "smooth local client position" );

idCVar net_serverMaxReservedClientSlots( "net_serverMaxReservedClientSlots", "2", CVAR_GAME | CVAR_INTEGER, "maximum number of player slots reserved for session invites", 0, MAX_CLIENTS, idCmdSystem::ArgCompletion_Integer< 0, MAX_CLIENTS > );

const int RESERVEDCLIENTSLOT_TIMEOUT	= 45000;

const int CLIENTBITS					= idMath::BitsForInteger( MAX_CLIENTS );

/*
===============================================================================

	sdReliableServerMessage

===============================================================================
*/
void sdReliableServerMessage::Send( const sdReliableMessageClientInfoBase& info ) const {
#ifdef SD_SUPPORT_REPEATER
	if ( info.SendToRepeaterClients() ) {
		networkSystem->RepeaterSendReliableMessage( info.GetClientNum(), *this, info.DontSendToRelays() );
	}
#endif // SD_SUPPORT_REPEATER
	if ( info.SendToClients() ) {
		networkSystem->ServerSendReliableMessage( info.GetClientNum(), *this );
		if ( info.SendToAll() && g_debugNetworkWrite.GetBool() ) {
			gameLocal.LogNetwork( va( "Reliable Broadcast: %d Size: %d bits\n", type, GetNumBitsWritten() ) );
		}
	}
}

/*
===============================================================================

	sdEntityNetEvent

===============================================================================
*/

/*
================
sdEntityNetEvent::sdEntityNetEvent
================
*/
sdEntityNetEvent::sdEntityNetEvent( void ) {
	node.SetOwner( this );
}

/*
================
sdEntityNetEvent::OutputParms
================
*/
void sdEntityNetEvent::OutputParms( idBitMsg& msg ) const {
	msg.InitRead( paramsBuf, sizeof( paramsBuf ) );
	msg.SetSize( paramsSize );
	msg.BeginReading();
}

/*
================
sdEntityNetEvent::Read
================
*/
void sdEntityNetEvent::Read( const idBitMsg& msg ) {
	spawnId		= msg.ReadLong();
	event		= msg.ReadByte();
	time		= msg.ReadLong();
	saveEvent	= msg.ReadBool();
	paramsSize	= msg.ReadBits( idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );

	if ( paramsSize ) {
		if ( paramsSize > MAX_EVENT_PARAM_SIZE ) {
			gameLocal.NetworkEventWarning( *this, "invalid param size" );
			return;
		}
		msg.ReadData( paramsBuf, paramsSize );
	}
}

/*
================
sdEntityNetEvent::GetTotalSize
================
*/
int sdEntityNetEvent::GetTotalSize( void ) const {
	return 4 + 1 + 4 + ( ( ( idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) + 1 ) + 7 ) >> 3 ) + paramsSize;
}

/*
================
sdEntityNetEvent::Write
================
*/
void sdEntityNetEvent::Write( idBitMsg& msg ) const {
	msg.WriteLong( spawnId );
	msg.WriteByte( event );
	msg.WriteLong( time );
	msg.WriteBool( saveEvent );
	msg.WriteBits( paramsSize, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	if ( paramsSize ) {
		msg.WriteData( paramsBuf, paramsSize );
	}
}

/*
================
sdEntityNetEvent::Read
================
*/
void sdEntityNetEvent::Read( idFile* file ) {
	file->ReadInt( spawnId );
	file->ReadInt( event );
	file->ReadInt( time );
	file->ReadInt( paramsSize );
	file->ReadBool( saveEvent );
	if ( paramsSize ) {
		file->Read( paramsBuf, paramsSize );
	}
}


/*
================
sdEntityNetEvent::Write
================
*/
void sdEntityNetEvent::Write( idFile* file ) const {
	file->WriteInt( spawnId );
	file->WriteInt( event );
	file->WriteInt( time );
	file->WriteBool( saveEvent );
	file->WriteInt( paramsSize );
	if ( paramsSize ) {
		file->Write( paramsBuf, paramsSize );
	}
}

/*
================
sdEntityNetEvent::Create
================
*/
void sdEntityNetEvent::Create( const idEntity* _entity, int _event, bool _saveEvent, const idBitMsg* _msg ) {
	spawnId		= gameLocal.GetSpawnId( _entity );
	event		= _event;
	time		= gameLocal.time;
	saveEvent	= _saveEvent; 

	if ( _msg ) {
		paramsSize = _msg->GetSize();
		memcpy( paramsBuf, _msg->GetData(), _msg->GetSize() );
	} else {
		paramsSize = 0;
	}
}

/*
================
sdEntityNetEvent::Create
================
*/
void sdEntityNetEvent::Create( const sdEntityNetEvent& other ) {
	spawnId		= other.spawnId;
	event		= other.event;
	time		= other.time;
	saveEvent	= other.saveEvent;
	paramsSize	= other.paramsSize;
	memcpy( paramsBuf, other.paramsBuf, other.paramsSize );
}

/*
===============================================================================

	sdUnreliableEntityNetEvent

===============================================================================
*/

/*
================
sdUnreliableEntityNetEvent::sdUnreliableEntityNetEvent
================
*/
sdUnreliableEntityNetEvent::sdUnreliableEntityNetEvent( void ) {
	unreliableNode.SetOwner( this );
#ifdef SD_SUPPORT_REPEATER
	numRepeaterClients = 0;
#endif // SD_SUPPORT_REPEATER
}

/*
================
sdUnreliableEntityNetEvent::ClearSent
================
*/
void sdUnreliableEntityNetEvent::ClearSent( void ) {
	// mark client slots that aren't occupied as already having been sent
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* client = gameLocal.GetClient( i );
		if ( client != NULL && client != gameLocal.GetLocalPlayer() && gameLocal.isServer ) {
			sentClients.Clear( i );
		} else {
			sentClients.Set( i );
		}
	}
	if ( sdDemoManager::GetInstance().GetState() == sdDemoManagerLocal::DS_RECORDING && gameLocal.localClientNum == ASYNC_DEMO_CLIENT_INDEX ) {
		sentClients.Clear( MAX_CLIENTS );
	} else {
		sentClients.Set( MAX_CLIENTS );
	}

#ifdef SD_SUPPORT_REPEATER
	numRepeaterClients = gameLocal.GetNumRepeaterClients();
	sentRepeaterClients.Init( numRepeaterClients );
	sentRepeaterClients.SetAll();
	for ( int i = 0; i < numRepeaterClients; i++ ) {
		if ( gameLocal.IsRepeaterClientConnected( i ) ) {
			sentRepeaterClients.Clear( i );
		}
	}
#endif // SD_SUPPORT_REPEATER
}

/*
================
sdUnreliableEntityNetEvent::SetSent
================
*/
void sdUnreliableEntityNetEvent::SetSent( int clientTo ) {
	assert( clientTo >= 0 && clientTo < ( MAX_CLIENTS + 1 ) );
	sentClients.Set( clientTo );
}

/*
================
sdUnreliableEntityNetEvent::HasExpired
================
*/
bool sdUnreliableEntityNetEvent::HasExpired( void ) const {
	bool allSent = true;
	for ( int i = 0; i < MAX_CLIENTS + 1; i++ ) {
		if ( sentClients.Get( i ) != 0 ) {
			allSent = false;
			break;
		}
	}

#ifdef SD_SUPPORT_REPEATER
	for ( int i = 0; i < numRepeaterClients; i++ ) {
		if ( sentRepeaterClients.Get( i ) != 0 ) {
			allSent = false;
			break;
		}
	}
#endif // SD_SUPPORT_REPEATER

	if ( allSent ) {
		return true;
	}

	// expire 2 second old messages
	if ( gameLocal.time - GetTime() > 2000 ) {
		return true;
	}

	return false;
}

/*
================
sdUnreliableEntityNetEvent::GetSent
================
*/
bool sdUnreliableEntityNetEvent::GetSent( int clientTo ) const {
	return sentClients.Get( clientTo ) != 0;
}

#ifdef SD_SUPPORT_REPEATER
/*
================
sdUnreliableEntityNetEvent::SetRepeaterSent
================
*/
void sdUnreliableEntityNetEvent::SetRepeaterSent( int clientTo ) {
	if ( clientTo >= numRepeaterClients ) {
		assert( false );
		return;
	}
	sentRepeaterClients.Set( clientTo );
}

/*
================
sdUnreliableEntityNetEvent::GetRepeaterSent
================
*/
bool sdUnreliableEntityNetEvent::GetRepeaterSent( int clientTo ) const {
	if ( clientTo >= numRepeaterClients ) {
		return true;
	}
	return sentRepeaterClients.Get( clientTo ) != 0;
}
#endif // SD_SUPPORT_REPEATER

/*
===============================================================================

	idGameLocal

===============================================================================
*/

/*
================
idGameLocal::InitAsyncNetwork
================
*/
void idGameLocal::InitAsyncNetwork( void ) {	
	entityNetEventQueue.Clear();
	savedEntityNetEventQueue.Clear();

	numEntityDefBits	= idMath::BitsForInteger( declEntityDefType.Num() + 1 );
	numSkinDeclBits		= idMath::BitsForInteger( declSkinType.Num() + 1 );
	numDamageDeclBits	= idMath::BitsForInteger( declDamageType.Num() + 1 );
	numInvItemBits		= idMath::BitsForInteger( declInvItemType.Num() + 1 );
	numDeployObjectBits = idMath::BitsForInteger( declDeployableObjectType.Num() + 1 );
	numPlayerClassBits	= idMath::BitsForInteger( declPlayerClassType.Num() + 1 );
	numClientIndexBits	= idMath::BitsForInteger( MAX_CLIENTS + 1 );

	realClientTime = 0;

	targetTimers.Clear();
	targetTimerLookup.Clear();
	numServerTimers = 0;

	if ( isClient ) {
		SetupGameStateBase( GetNetworkInfo( localClientNum ) );
		SetupEntityStateBase( GetNetworkInfo( localClientNum ) );
	}

	SetupGameStateBase( GetNetworkInfo( ASYNC_DEMO_CLIENT_INDEX ) );
	SetupEntityStateBase( GetNetworkInfo( ASYNC_DEMO_CLIENT_INDEX ) );
}

/*
================
idGameLocal::ShutdownAsyncNetwork
================
*/
void idGameLocal::ShutdownAsyncNetwork( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		ShutdownClientNetworkState( i );
	}
	ShutdownClientNetworkState( ASYNC_DEMO_CLIENT_INDEX );
#ifdef SD_SUPPORT_REPEATER
	ShutdownClientNetworkState( REPEATER_CLIENT_INDEX );
	ShutdownRepeatersNetworkStates();
#endif // SD_SUPPORT_REPEATER

	for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		sdNetworkStateObject& object = GetGameStateObject( ( extNetworkStateMode_t )i );
		object.Clear();
	}

	snapshotAllocator.Shutdown();
	entityNetEventAllocator.Shutdown();
	unreliableEntityNetEventAllocator.Shutdown();
}

/*
================
idGameLocal::InitLocalClient
================
*/
void idGameLocal::InitLocalClient( int clientNum, bool server ) {
	isServer = server;
	isClient = !server;
	localClientNum = clientNum;
#ifdef SD_SUPPORT_REPEATER
	serverIsRepeater = clientNum == REPEATER_CLIENT_INDEX;
#else
	serverIsRepeater = false;
#endif // SD_SUPPORT_REPEATER

	if ( isClient ) {
		SetupGameStateBase( GetNetworkInfo( clientNum ) );
		SetupEntityStateBase( GetNetworkInfo( clientNum ) );
	}
	sdFireTeamManager::GetInstance().Clear();
}

/*
================
idGameLocal::OnServerShutdown
================
*/
void idGameLocal::OnServerShutdown() {
#ifndef _XENON
	sdnet.StopGameSession( true );

	if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
#if !defined( SD_DEMO_BUILD )
		sdnet.FlushStats( true );
#endif /* !SD_DEMO_BUILD */
		sdnet.SignOutDedicated();
	}
#endif

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		ServerClientDisconnect( i );
	}

	reservedClientSlots.Clear();
	isServer = false;
}

/*
================
idGameLocal::ReserveClientSlot
================
*/
void idGameLocal::ReserveClientSlot( const sdNetClientId& netClientId ) {
	CheckForExpiredReservedSlots();

	if ( reservedClientSlots.Num() == MAX_CLIENTS ) {
		// find the oldest one and remove it
		int oldestIndex = 0;
		int oldestTime = reservedClientSlots[0].time;
		for ( int i = 1; i < reservedClientSlots.Num(); i++ ) {
			if ( reservedClientSlots[i].time < oldestTime ) {
				oldestIndex = i;
				oldestTime = reservedClientSlots[i].time;
			}
		}
		reservedClientSlots.RemoveIndexFast( oldestIndex );
	}

	reservedSlot_t* reservedSlot = reservedClientSlots.Alloc();
	reservedSlot->netClientId = netClientId;
	reservedSlot->time = sys->Milliseconds();
}

/*
================
idGameLocal::CheckForExpiredReservedSlots
================
*/
void idGameLocal::CheckForExpiredReservedSlots( void ) {
	int currentTime = sys->Milliseconds();
	int i = 0;
	while ( i < reservedClientSlots.Num() ) {
		if ( reservedClientSlots[i].time < currentTime - RESERVEDCLIENTSLOT_TIMEOUT ) {
			reservedClientSlots.RemoveIndexFast( i );
			continue;
		}
		i++;
	}
}

/*
================
idGameLocal::GetMaxPrivateClients
================
*/
int idGameLocal::GetMaxPrivateClients( void ) {
	const int MAX_RANKED_PRIVATE_SLOTS = 4;
	int max = si_privateClients.GetInteger();
	if ( networkSystem->IsRankedServer() ) {
		if ( max > MAX_RANKED_PRIVATE_SLOTS ) {
			max = MAX_RANKED_PRIVATE_SLOTS;
			si_privateClients.SetInteger( MAX_RANKED_PRIVATE_SLOTS );
		}
	}

	return max;
}

/*
================
idGameLocal::ServerAllowClient
================
*/
allowReply_t idGameLocal::ServerAllowClient( int numClients, int numBots, const clientNetworkAddress_t& address, const sdNetClientId& netClientId, const char *guid, const char *password, allowFailureReason_t& reason ) {
	clientGUIDLookup_t lookup;
	lookup.ip	= ( *( int* )address.ip );
	lookup.pbid	= 0;
	lookup.clientId = netClientId;

	sdGUIDFile::banState_t banState = guidFile.CheckForBan( lookup );
	switch ( banState ) {
		case sdGUIDFile::BS_PERM_BAN:
			reason = ALLOWFAIL_PERMBAN;
			return ALLOW_NO;
		case sdGUIDFile::BS_TEMP_BAN:
			reason = ALLOWFAIL_TEMPBAN;
			return ALLOW_NO;
		case sdGUIDFile::BS_NOT_BANNED:
			break;
	}

	if ( serverInfo.GetInt( "si_pure" ) && !rules->IsPureReady() ) {
		reason = ALLOWFAIL_SERVERSPAWNING;
		return ALLOW_NOTYET;
	}

	int maxPlayers = si_maxPlayers.GetInteger();
	if ( maxPlayers == 0 ) {
		reason = ALLOWFAIL_SERVERSPAWNING;
		return ALLOW_NOTYET;
	}

	int maxPrivatePlayers = GetMaxPrivateClients();
	if ( maxPrivatePlayers > maxPlayers ) {
		maxPrivatePlayers = maxPlayers;
	}
	int maxRegularPlayers = maxPlayers - maxPrivatePlayers;

	// check if there is a reserved slot for this client, expire old slots first
	CheckForExpiredReservedSlots();

	int numReservedSlots = 0;
	if ( !networkSystem->IsRankedServer() ) {
		numReservedSlots = Min( reservedClientSlots.Num(), net_serverMaxReservedClientSlots.GetInteger() );
	}
	int numAvailableSlots = maxRegularPlayers - numReservedSlots;

	bool isPrivateClient = false;
	const char* privatePass = g_privatePassword.GetString();
	if ( *privatePass != '\0' && idStr::Cmp( privatePass, password ) == 0 ) {
		isPrivateClient = true;
		numAvailableSlots = maxPlayers; // with private password, any slot is available
	} else if ( numReservedSlots > 0 ) {
		for ( int i = 0; i < reservedClientSlots.Num(); i++ ) {
			if ( reservedClientSlots[ i ].netClientId == netClientId ) {
				reservedClientSlots.RemoveIndexFast( i );
				numAvailableSlots = maxRegularPlayers; // with reserved id, any slot is available, bar the private slots
				break;
			}
		}
	}

	if ( numClients >= numAvailableSlots ) {
		if ( numClients >= maxPrivatePlayers ) {
			reason = ALLOWFAIL_SERVERFULL;
			return ALLOW_NOTYET;
		}
		reason = ALLOWFAIL_INVALIDPASSWORD;
		return ALLOW_BADPASS;
	}

	if ( !isPrivateClient ) {
		if ( si_needPass.GetBool() ) {
			const char* pass = g_password.GetString();
			if ( *pass == '\0' ) {
				// Gordon: FIXME: This is rubbish
				gameLocal.Warning( "si_needPass is set but g_password is empty" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say si_needPass is set but g_password is empty" );
				// avoids silent misconfigured state
				reason = ALLOWFAIL_SERVERMISCONFIGURED;
				return ALLOW_NOTYET;
			}

			if ( idStr::Cmp( pass, password ) != 0 ) {
				reason = ALLOWFAIL_INVALIDPASSWORD;
				Printf( "Rejecting client %s from IP '%i.%i.%i.%i': invalid password\n", guid, address.ip[ 0 ], address.ip[ 1 ], address.ip[ 2 ], address.ip[ 3 ] );
				return ALLOW_BADPASS;
			}
		}
	}

	if ( ( numClients + numBots ) >= numAvailableSlots ) {
		// We need to kick a bot to let the player in
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			if ( player->IsType( idBot::Type ) ) {
				networkSystem->ServerKickClient( i, "", false );
				break;
			}
		}
	}

	return ALLOW_YES;
}

/*
================
idGameLocal::ServerClientConnect
================
*/
void idGameLocal::ServerClientConnect( int clientNum ) {
	Printf( "client %d connected.\n", clientNum );

	if ( !clientConnected[ clientNum ] ) {
		clientConnected[ clientNum ] = true;
		clientRanks[ clientNum ].calculated = false;
		clientRanks[ clientNum ].rank = -1;
		clientStatsRequestsPending = true;
#if !defined( SD_DEMO_BUILD )
		clientStatsList[ clientNum ].SetNum( 0, false );
		clientStatsHash[ clientNum ].Clear();
#endif /* !SD_DEMO_BUILD */
		clientCompaintCount[ clientNum ] = 0;
		clientUniqueComplaints[ clientNum ].SetNum( 0, false );
		clientLastBanIndexReceived[ clientNum ] = -1;

		clientGUIDLookup_t lookup;

		clientNetworkAddress_t netAddr;
		networkSystem->ServerGetClientNetworkInfo( clientNum, netAddr );

		lookup.ip = ( *( int* )netAddr.ip );
		lookup.pbid = 0;

		networkSystem->ServerGetClientNetId( clientNum, lookup.clientId );

		gameLocal.guidFile.AuthUser( clientNum, lookup );

#ifndef _XENON
		if ( sdnet.HasGameSession() ) {
			sdnet.ServerClientConnect( clientNum );
			sdnet.UpdateGameSession( false, false );
		}
#endif
	}

	sdInstanceCollector< sdTransport > vehicles( true );
	for ( int i = 0; i < vehicles.Num(); i++ ) {
		vehicles[ i ]->GetPositionManager().ResetBan( clientNum );
	}
}

/*
================
idGameLocal::ServerClientBegin
================
*/
void idGameLocal::ServerClientBegin( int clientNum, bool isBot ) {
	if ( isServer ) { // client needs to get this straight away
		sdReliableServerMessage ruleMsg( GAME_RELIABLE_SMESSAGE_RULES_DATA );
		ruleMsg.WriteLong( sdGameRules::EVENT_CREATE );
		ruleMsg.WriteLong( rules->GetType()->typeNum );
		ruleMsg.Send( sdReliableMessageClientInfo( clientNum ) );

		rules->WriteInitialReliableMessages( sdReliableMessageClientInfo( clientNum ) );
	}

	idPlayer* player = GetClient( clientNum );
	if ( player == NULL ) {
		// spawn the player
		SpawnPlayer( clientNum, isBot );
		player = GetClient( clientNum );
	}

	if ( player != NULL ) {
		sdProficiencyManager::GetInstance().RestoreProficiency( player );
	}
	sdGlobalStatsTracker::GetInstance().Restore( clientNum );
	SetupFixedClientRank( clientNum );

	if ( clientNum == localClientNum ) {
		rules->EnterGame( GetLocalPlayer() );
	}
}

/*
================
idGameLocal::SetClientNum
================
*/
void idGameLocal::SetClientNum( int clientNum, bool server ) {
	InitLocalClient( clientNum, server );
}

/*
================
idGameLocal::ShutdownClientNetworkState
================
*/
void idGameLocal::ShutdownClientNetworkState( int clientNum ) {
	GetNetworkInfo( clientNum ).Reset();
}

/*
================
idGameLocal::ServerClientDisconnect
================
*/
void idGameLocal::ServerClientDisconnect( int clientNum ) {
	idPlayer* disconnectClient = GetClient( clientNum );

	if ( disconnectClient != NULL ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( i == clientNum ) {
				continue;
			}

			idPlayer* player = GetClient( i );
			if ( player == NULL ) {
				continue;
			}
			if ( player->GetSpectateClient() != disconnectClient ) {
				continue;
			}

			player->SpectateCycle( true );

			if ( player->GetSpectateClient() != disconnectClient ) {
				continue;
			}

			player->SetSpectateClient( player );
		}

		if ( !disconnectClient->userInfo.isBot ) {
			sdProficiencyManager::GetInstance().CacheProficiency( disconnectClient );
		}
		disconnectClient->PostEventMS( &EV_Remove, 0 );
	}

	if ( !clientConnected[ clientNum ] ) {
		return;
	}

	Printf( "client %d disconnected.\n", clientNum );

	clientConnected[ clientNum ] = false;
#if !defined( SD_DEMO_BUILD )
	clientStatsList[ clientNum ].SetNum( 0, false );
	clientStatsHash[ clientNum ].Clear();
#endif // SD_DEMO_BUILD
	clientRanks[ clientNum ].calculated = false;
	clientRanks[ clientNum ].rank = -1;
	clientLastBanIndexReceived[ clientNum ] = -1;
	clientMuteMask[ clientNum ].Clear();
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientMuteMask[ i ].Clear( clientNum );
	}

	for ( int i = 0; i < endGameStats.Num(); i++ ) {
		endGameStats[ i ].data[ clientNum ] = -1.f;
	}

	if ( botThreadData.GetGameWorldState() != NULL ) {
		botThreadData.InitClientInfo( clientNum, true, true );
	}

	ShutdownClientNetworkState( clientNum );

	for ( int i = 0; i < declVehicleScriptDefType.Num(); i++ ) {
		const sdDeclVehicleScript* script = declVehicleScriptDefType.LocalFindByIndex( i, false );
		script->ResetCameraMode( clientNum );
	}

	rules->DisconnectClient( clientNum );

#ifndef _XENON
	if ( sdnet.HasGameSession() ) {
		sdnet.ServerClientDisconnect( clientNum );
		sdnet.UpdateGameSession( false, false );
	}
#endif
}

/*
================
idGameLocal::ServerWriteInitialReliableMessages

  Send reliable messages to initialize the client game up to a certain initial state.
================
*/
void idGameLocal::ServerWriteInitialReliableMessages( int clientNum ) {
	WriteInitialReliableMessages( sdReliableMessageClientInfo( clientNum ) );
}

/*
================
idGameLocal::WriteInitialReliableMessages
================
*/
void idGameLocal::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) {
	assert( !target.SendToAll() );

	if ( target.SendToClients() ) {
		idPlayer* player = GetClient( target.GetClientNum() );
		if ( player == NULL ) {
			Error( "idGameLocal::ServerWriteInitialReliableMessages Client '%d' was NULL", target.GetClientNum() );
		}
	}

	SendPauseInfo( target );

	for ( int i = 0; i < targetTimers.Num(); i++ ) {
		sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_CREATEPLAYERTARGETTIMER );
		outMsg.WriteString( targetTimers[ i ].name );
		outMsg.WriteShort( i );
		outMsg.Send( target );
	}

	int count = 0;
	int totalCount = 0;
	int totalBits = 0;
	int batchCount = 0;
		




	count = networkedEntities.Num();
	totalCount = count;
	batchCount = 0;

	idEntity* ent = networkedEntities.Next();
	
	totalBits = 0;
	while ( count > 0 ) {
		const int MAX_COMBINED_CREATE = 128;
		int use = Min( count, MAX_COMBINED_CREATE );

		batchCount++;

		sdReliableServerMessage entMsg( GAME_RELIABLE_SMESSAGE_MULTI_CREATE_ENT );
		entMsg.WriteLong( use );

		for ( int j = 0; j < use; j++ ) {
			ent->WriteCreateEvent( &entMsg, target );
			ent = ent->networkNode.Next();
		}

		entMsg.Send( target );

		totalBits += entMsg.GetNumBitsWritten();
		count -= use;
	}

//	gameLocal.Printf( "idGameLocal::ServerWriteInitialReliableMessages Writing %d Entities Using %d bits in %d batches\n", totalCount, totalBits, batchCount );





	// send all saved events
	totalCount = 0;
	batchCount = 0;

	sdEntityNetEvent* start = savedEntityNetEventQueue.Next();
	while ( start != NULL ) {
		count = 0;

		const int maxEventSizeTotal = MAX_GAME_MESSAGE_SIZE - 16;

		sdEntityNetEvent* last = start;

		int totalSize = 0;
		while ( last != NULL ) {
			totalSize += last->GetTotalSize();
			if ( totalSize >= maxEventSizeTotal ) {
				break;
			}
			count++;
			totalCount++;
			last = last->GetNode().Next();
		}

		assert( count > 0 );

		batchCount++;

		sdReliableServerMessage evtMsg( GAME_RELIABLE_SMESSAGE_MULTI_ENTITY_EVENT );
		evtMsg.WriteLong( count );

		while ( start != last ) {
			idEntity* temp = gameLocal.EntityForSpawnId( start->GetSpawnId() );
			if ( temp == NULL ) {
				gameLocal.Warning( "idGameLocal::ServerWriteInitialReliableMessages NULL entity" );
			}

			start->Write( evtMsg );
			start = start->GetNode().Next();
		}

		evtMsg.Send( target );

		totalBits += evtMsg.GetNumBitsWritten();
	}

//	gameLocal.Printf( "idGameLocal::ServerWriteInitialReliableMessages Writing %d Saved Events Using %d bits in %d batches\n", totalCount, totalBits, batchCount );





	for ( idEntity* ent = networkedEntities.Next(); ent; ent = ent->networkNode.Next() ) {
		ent->WriteInitialReliableMessages( target );
	}

	for ( int i = 0; i < NUM_DEPLOY_REQUESTS; i++ ) {
		if ( !deployRequests[ i ] ) {
			continue;
		}
		deployRequests[ i ]->WriteCreateEvent( i, target );
	}

	sdTaskManager::GetInstance().WriteInitialReliableMessages( target );
	sdFireTeamManager::GetInstance().WriteInitialReliableMessages( target );
	sdObjectiveManager::GetInstance().WriteInitialReliableMessages( target );
	sdUserGroupManager::GetInstance().WriteNetworkData( target );

	SendEndGameStats( target );
}

/*
================
idGameLocal::ClientWriteGameState
================
*/
void idGameLocal::ClientWriteGameState( idFile* file ) {
	file->WriteBool( isPaused );
	file->WriteInt( time );
	file->WriteInt( timeOffset );

	assert( rules != NULL );
	file->WriteInt( rules->GetType()->typeNum );

	file->WriteInt( targetTimers.Num() );
	for ( int i = 0; i < targetTimers.Num(); i++ ) {
		file->WriteString( targetTimers[ i ].name );
		file->WriteInt( targetTimers[ i ].serverHandle );
		if ( localClientNum != ASYNC_DEMO_CLIENT_INDEX ) {
			file->WriteInt( targetTimers[ i ].endTimes[ localClientNum ] );
		}
	}

	file->WriteInt( networkedEntities.Num() );
	for ( idEntity* ent = networkedEntities.Next(); ent; ent = ent->networkNode.Next() ) {
		ent->WriteCreationData( file );
		ASYNC_SECURITY_WRITE_FILE( file )
	}

	// Gordon: now seperate so the client can create all entities before reading this data
	for ( idEntity* ent = networkedEntities.Next(); ent; ent = ent->networkNode.Next() ) {
		ent->WriteDemoBaseData( file );
		ASYNC_SECURITY_WRITE_FILE( file )
	}

	file->WriteInt( entityNetEventQueue.Num() );
	for ( sdEntityNetEvent* event = entityNetEventQueue.Next(); event; event = event->GetNode().Next() ) {
		event->Write( file );
		ASYNC_SECURITY_WRITE_FILE( file )
	}

	for ( int i = 0; i < NUM_DEPLOY_REQUESTS; i++ ) {
		if ( !deployRequests[ i ] ) {
			file->WriteBool( false );
			continue;
		}
		file->WriteBool( true );
		deployRequests[ i ]->Write( file );
	}

	sdTaskManager::GetInstance().Write( file );
	sdFireTeamManager::GetInstance().Write( file );
	sdUserGroupManager::GetInstance().Write( file );
}

/*
================
idGameLocal::ClientReadGameState
================
*/
void idGameLocal::ClientReadGameState( idFile* file ) {
	file->ReadBool( isPaused );
	file->ReadInt( time );
	file->ReadInt( timeOffset );

	int rulesTypeNum;
	file->ReadInt( rulesTypeNum );
	SetRules( idClass::GetType( rulesTypeNum ) );

	ClearTargetTimers();

	int count;
	file->ReadInt( count );

	targetTimers.SetNum( count );
	numServerTimers = 0;
	for ( int i = 0; i < count; i++ ) {
		int serverHandle;

		file->ReadString( targetTimers[ i ].name );
		file->ReadInt( serverHandle );
		targetTimers[ i ].serverHandle = serverHandle;
		if ( localClientNum != ASYNC_DEMO_CLIENT_INDEX ) {
			file->ReadInt( targetTimers[ i ].endTimes[ localClientNum ] );
		}

		if ( serverHandle != -1 ) {
			numServerTimers = Max( serverHandle + 1, numServerTimers );
			if ( numServerTimers > targetTimerLookup.Num() ) {
				targetTimerLookup.AssureSize( numServerTimers, -1 );
			}
			targetTimerLookup[ serverHandle ] = i;
		}
	}

	idList< idEntity* > creationEnts;

	file->ReadInt( count );

	creationEnts.SetNum( count );

	for ( int i = 0; i < count; i++ ) {
		creationEnts[ i ] = idEntity::FromCreationData( file );

		if ( !creationEnts[ i ] ) {
			gameLocal.Error( "idGameLocal::ClientReadGameState Failed to spawn entity from demo" );
		}

		ASYNC_SECURITY_READ_FILE( file )
	}

	// Gordon: moved this out of the above loop so all entities are spawned before they read their creation data
	// as they may want to link to an existing entity further down the chain
	for ( int i = 0; i < count; i++ ) {
		creationEnts[ i ]->LoadDemoBaseData( file );

		ASYNC_SECURITY_READ_FILE( file )
	}

	file->ReadInt( count );
	for ( int i = 0; i < count; i++ ) {
		sdEntityNetEvent* event = entityNetEventAllocator.Alloc();
		event->GetNode().AddToEnd( entityNetEventQueue );
		event->Read( file );
		
		ASYNC_SECURITY_READ_FILE( file )
	}

	ClearDeployRequests();
	for ( int i = 0; i < NUM_DEPLOY_REQUESTS; i++ ) {
		bool temp;
		file->ReadBool( temp );
		if ( !temp ) {
			continue;
		}

		deployRequests[ i ] = new sdDeployRequest( file );
	}

	sdTaskManager::GetInstance().Read( file );
	sdFireTeamManager::GetInstance().Read( file );
	sdUserGroupManager::GetInstance().Read( file );
}

/*
================
idGameLocal::FreeEntityNetworkEvents
================
*/
void idGameLocal::FreeEntityNetworkEvents( const idEntity *ent, int eventId ) {

	sdEntityNetEvent* next;
	for ( sdEntityNetEvent* event = savedEntityNetEventQueue.Next(); event; event = next ) {
		next = event->GetNode().Next();

		if ( eventId == -1 || event->GetEvent() == eventId ) {

			idEntity* eventEnt = gameLocal.EntityForSpawnId( event->GetSpawnId() );
			if ( eventEnt == ent ) {				
				event->GetNode().Remove();
				entityNetEventAllocator.Free( event );
			}
		}
	}
}

/*
================
idGameLocal::SaveEntityNetworkEvent
================
*/
void idGameLocal::SaveEntityNetworkEvent( const sdEntityNetEvent& oldEvent ) {
	sdEntityNetEvent* event;
	event = entityNetEventAllocator.Alloc();
	event->Create( oldEvent );
	event->GetNode().AddToEnd( savedEntityNetEventQueue );
}

/*
================
idGameLocal::SendUnreliableEntityNetworkEvent
================
*/
void idGameLocal::SendUnreliableEntityNetworkEvent( const idEntity *ent, int eventId, const idBitMsg *msg ) {
	sdUnreliableEntityNetEvent* event;
	event = unreliableEntityNetEventAllocator.Alloc();
	event->Create( ent, eventId, false, msg );
	event->ClearSent();
	event->GetUnreliableNode().AddToEnd( unreliableEntityNetEventQueue );
}

/*
================
idGameLocal::SendUnreliableEntityNetworkEvent
================
*/
void idGameLocal::SendUnreliableEntityNetworkEvent( const sdUnreliableEntityNetEvent& oldEvent ) {
	sdUnreliableEntityNetEvent* event;
	event = unreliableEntityNetEventAllocator.Alloc();
	event->Create( oldEvent );
	event->ClearSent();
	event->GetUnreliableNode().AddToEnd( unreliableEntityNetEventQueue );
}

/*
================
idGameLocal::WriteUnreliableEntityEvents
================
*/
void idGameLocal::WriteUnreliableEntityNetEvents( int clientNum, bool repeaterClient, idBitMsg &msg ) {
	// write unreliable entity network events to the snapshot
	// first count them up
	int numEvents = 0;
	sdUnreliableEntityNetEvent* event;
	sdUnreliableEntityNetEvent* nextEvent;
	for ( event = unreliableEntityNetEventQueue.Next(); event != NULL; event = nextEvent ) {
		nextEvent = event->GetUnreliableNode().Next();

#ifdef SD_SUPPORT_REPEATER
		if ( repeaterClient ) {
			if ( !event->GetRepeaterSent( clientNum ) ) {
				numEvents++;
			}
		} else
#endif // SD_SUPPORT_REPEATER
		{
			if ( !event->GetSent( clientNum ) ) {
				numEvents++;
			}
		}
	}

	// now write them
	msg.WriteLong( numEvents );
	for ( event = unreliableEntityNetEventQueue.Next(); event != NULL; event = nextEvent ) {
		nextEvent = event->GetUnreliableNode().Next();

#ifdef SD_SUPPORT_REPEATER
		if ( repeaterClient ) {
			if ( !event->GetRepeaterSent( clientNum ) ) {
				event->Write( msg );
				event->SetRepeaterSent( clientNum );
			}
		} else
#endif // SD_SUPPORT_REPEATER
		{
			if ( !event->GetSent( clientNum ) ) {
				event->Write( msg );
				event->SetSent( clientNum );
			}
		}

		// check if the event has expired
		if ( event->HasExpired() ) {
			event->GetUnreliableNode().Remove();
			unreliableEntityNetEventAllocator.Free( event );
		}
	}
}

/*
================
idGameLocal::FreeSnapshotsOlderThanSequence
================
*/
void idGameLocal::FreeSnapshotsOlderThanSequence( clientNetworkInfo_t& nwInfo, int sequence ) {
	snapshot_t* nextSnapshot;

	snapshot_t* lastSnapshot = NULL;
	for ( snapshot_t* snapshot = nwInfo.snapshots; snapshot; snapshot = nextSnapshot ) {
		nextSnapshot = snapshot->next;
		if ( snapshot->sequence < sequence ) {

			if ( lastSnapshot ) {
				lastSnapshot->next = snapshot->next;
			} else {
				nwInfo.snapshots = snapshot->next;
			}

			for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
				sdEntityState* nextState;
				for ( sdEntityState* state = snapshot->firstEntityState[ i ]; state; state = nextState ) {
					nextState = state->next;

					idEntity* ent = EntityForSpawnId( state->GetSpawnId() );
					if ( ent ) {
						state->next = ent->freeStates[ i ];
						ent->freeStates[ i ] = state;
					} else {
						FreeNetworkState( state );
					}
				}
			}

			for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
				if ( snapshot->gameStates[ i ] == NULL ) {
					continue;
				}

				sdNetworkStateObject& obj = GetGameStateObject( ( extNetworkStateMode_t )i );
				snapshot->gameStates[ i ]->next = obj.freeStates;
				obj.freeStates = snapshot->gameStates[ i ];
			}

			snapshotAllocator.Free( snapshot );
		} else {
			lastSnapshot = snapshot;
		}
	}
}

/*
================
idGameLocal::ApplySnapshot
================
*/
bool idGameLocal::ApplySnapshot( clientNetworkInfo_t& nwInfo, int sequence ) {
	snapshot_t *snapshot, *lastSnapshot, *nextSnapshot;

	FreeSnapshotsOlderThanSequence( nwInfo, sequence );

	for ( lastSnapshot = NULL, snapshot = nwInfo.snapshots; snapshot; snapshot = nextSnapshot ) {
		nextSnapshot = snapshot->next;
		if ( snapshot->sequence == sequence ) {
			if ( lastSnapshot ) {
				lastSnapshot->next = nextSnapshot;
			} else {
				nwInfo.snapshots = nextSnapshot;
			}

			for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
				sdEntityState* next;
				for ( sdEntityState* state = snapshot->firstEntityState[ i ]; state; state = next ) {
					next = state->next;

					idEntity* ent = EntityForSpawnId( state->GetSpawnId() );
					if ( ent ) {
						sdEntityState* oldState = nwInfo.states[ ent->entityNumber ][ i ];
						oldState->next = ent->freeStates[ i ];
						ent->freeStates[ i ] = oldState;

						nwInfo.states[ ent->entityNumber ][ i ] = state;
						state->next = NULL;
					} else {
						FreeNetworkState( state );
					}
				}
			}

			for ( int i = 0; i < snapshot->visibleEntities.Num(); i++ ) {
				nwInfo.lastMarker[ snapshot->visibleEntities[ i ] ] = snapshot->time;
			}

			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				if ( snapshot->clientUserCommands.Get( i ) ) {
					if ( nwInfo.lastUserCommand[ i ] != -1 ) {
						nwInfo.lastUserCommandDelay[ i ] = snapshot->time - nwInfo.lastUserCommand[ i ];
					} else {
						nwInfo.lastUserCommandDelay[ i ] = 0;
					}
					nwInfo.lastUserCommand[ i ] = snapshot->time;
				}
			}

			for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
				if ( !snapshot->gameStates[ i ] ) {
					continue;
				}

				sdNetworkStateObject& object = GetGameStateObject( ( extNetworkStateMode_t )i );

				sdGameState* oldState = nwInfo.gameStates[ i ];
				oldState->next = object.freeStates;
				object.freeStates = oldState;

				nwInfo.gameStates[ i ] = snapshot->gameStates[ i ];
			}

			snapshotAllocator.Free( snapshot );
			return true;
		} else {
			lastSnapshot = snapshot;
		}
	}

	return false;
}

/*
================
idGameLocal::ServerWriteSnapshot
================
*/
snapshot_t* idGameLocal::AllocateSnapshot( int sequence, clientNetworkInfo_t& nwInfo ) {
	// allocate new snapshot
	snapshot_t* snapshot = snapshotAllocator.Alloc();
	snapshot->Init( sequence, time );
	snapshot->next = nwInfo.snapshots;
	nwInfo.snapshots = snapshot;
	activeSnapshot = snapshot;
	snapshot->visibleEntities.SetNum( 0, false );
	snapshot->clientUserCommands.Clear();
	return snapshot;
}

/*
================
idGameLocal::WriteSnapshotGameStates
================
*/
void idGameLocal::WriteSnapshotGameStates( snapshot_t* snapshot, clientNetworkInfo_t& nwInfo, idBitMsg& msg ) {
//	ASYNC_SECURITY_WRITE( msg )
	WriteGameState( ENSM_TEAM, snapshot, nwInfo, msg );
//	ASYNC_SECURITY_WRITE( msg )

//	ASYNC_SECURITY_WRITE( msg )
	WriteGameState( ENSM_RULES, snapshot, nwInfo, msg );
//	ASYNC_SECURITY_WRITE( msg )
}

/*
================
idGameLocal::WriteSnapshotEntityStates
================
*/
void idGameLocal::WriteSnapshotEntityStates( snapshot_t* snapshot, clientNetworkInfo_t& nwInfo, int clientNum, idBitMsg& msg, bool useAOR ) {
	idStaticList< idEntity*, MAX_GENTITIES > visibleEntities;
	idStaticList< idEntity*, MAX_GENTITIES > broadcastEntities;

	idEntity* ent = NULL;
	if ( useAOR ) {
		for ( ent = networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
			ent->snapshotPVSFlags = PVS_VISIBLE;

			bool update = true;
			aorManager.UpdateEntityAORFlags( ent, ent->GetPhysics()->GetOrigin(), update );

			sdEntityState* baseState;
			if ( ent != snapShotPlayer && ent->entityNumber != clientNum && !update ) {
				ent->snapshotPVSFlags &= ~( PVS_VISIBLE );
			} else {
				baseState = nwInfo.states[ ent->entityNumber ][ NSM_VISIBLE ];
				if ( ent->CheckNetworkStateChanges( NSM_VISIBLE, *baseState->data ) ) {
					*visibleEntities.Alloc() = ent;
				} else {
					ent->snapshotPVSFlags &= ~( PVS_VISIBLE );
				}

				snapshot->visibleEntities.Alloc() = ent->entityNumber;
			}

			baseState = nwInfo.states[ ent->entityNumber ][ NSM_BROADCAST ];
			if ( ent->CheckNetworkStateChanges( NSM_BROADCAST, *baseState->data ) ) {
				*broadcastEntities.Alloc() = ent;
			}
		}
	} else {
		for( ent = networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
			ent->snapshotPVSFlags = PVS_VISIBLE;
			snapshot->visibleEntities.Alloc() = ent->entityNumber;

			sdEntityState* baseState;
			
			baseState = nwInfo.states[ ent->entityNumber ][ NSM_VISIBLE ];
			if ( ent->CheckNetworkStateChanges( NSM_VISIBLE, *baseState->data ) ) {
				*visibleEntities.Alloc() = ent;
			} else {
				ent->snapshotPVSFlags &= ~( PVS_VISIBLE );
			}

			baseState = nwInfo.states[ ent->entityNumber ][ NSM_BROADCAST ];
			if ( ent->CheckNetworkStateChanges( NSM_BROADCAST, *baseState->data ) ) {
				*broadcastEntities.Alloc() = ent;
			}
		}
	}

	for ( int i = 0; i < visibleEntities.Num(); i++ ) {
		idEntity* ent = visibleEntities[ i ];

		int oldBits = msg.GetNumBitsWritten();

		msg.WriteBits( ent->entityNumber, GENTITYNUM_BITS );

		sdEntityState* baseState = nwInfo.states[ ent->entityNumber ][ NSM_VISIBLE ];
		sdEntityState* newState = AllocEntityState( NSM_VISIBLE, ent );

//		ASYNC_SECURITY_WRITE( msg )
		ent->WriteNetworkState( NSM_VISIBLE, *baseState->data, *newState->data, msg );
//		ASYNC_SECURITY_WRITE( msg )

		if ( g_debugNetworkWrite.GetBool() ) {
			int bitsWritten = msg.GetNumBitsWritten() - oldBits;
			gameLocal.LogNetwork( va( "Writing Visible: '%s' '%s', %d bits\n", ent->GetType()->classname, ent->name.c_str(), bitsWritten ) );
		}

		newState->next = snapshot->firstEntityState[ NSM_VISIBLE ];
		snapshot->firstEntityState[ NSM_VISIBLE ] = newState;
	}
	msg.WriteBits( ENTITYNUM_NONE, GENTITYNUM_BITS );

	for ( int i = 0; i < broadcastEntities.Num(); i++ ) {
		idEntity* ent = broadcastEntities[ i ];

		int oldBits = msg.GetNumBitsWritten();

		msg.WriteBits( ent->entityNumber, GENTITYNUM_BITS );

		sdEntityState* baseState = nwInfo.states[ ent->entityNumber ][ NSM_BROADCAST ];
		sdEntityState* newState = AllocEntityState( NSM_BROADCAST, ent );

//		ASYNC_SECURITY_WRITE( msg )
		ent->WriteNetworkState( NSM_BROADCAST, *baseState->data, *newState->data, msg );
//		ASYNC_SECURITY_WRITE( msg )

		if ( g_debugNetworkWrite.GetBool() ) {
			int bitsWritten = msg.GetNumBitsWritten() - oldBits;
			gameLocal.LogNetwork( va( "Writing Broadcast: '%s' '%s', %d bits\n", ent->GetType()->classname, ent->name.c_str(), bitsWritten ) );
		}

		newState->next = snapshot->firstEntityState[ NSM_BROADCAST ];
		snapshot->firstEntityState[ NSM_BROADCAST ] = newState;
	}
	msg.WriteBits( ENTITYNUM_NONE, GENTITYNUM_BITS );
}

/*
================
idGameLocal::WriteSnapshotUserCmds
================
*/
void idGameLocal::WriteSnapshotUserCmds( snapshot_t* snapshot, idBitMsg& msg, int ignoreClientNum, bool useAOR ) {
	// write the latest user commands from the other clients in the PVS to the snapshot
	// written to a seperate idBitMsg, which uses a different compression strategy
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}

		idPlayer* otherPlayer = GetClient( i );
		if ( !otherPlayer ) {
			continue;
		}

		if ( useAOR && otherPlayer != snapShotPlayer ) {
			if ( otherPlayer->GetProxyEntity() == NULL ) {
				if ( otherPlayer->aorFlags & AOR_INHIBIT_USERCMDS ) {
					continue;
				}
			} else {
				if ( otherPlayer->GetProxyEntity()->aorFlags & AOR_INHIBIT_USERCMDS ) {
					continue;
				}
			}
		}

		msg.WriteBits( i, CLIENTBITS );
		networkSystem->WriteClientUserCmds( i, msg );

		// record that this client had its user command's sent with this snapshot
		snapshot->clientUserCommands.Set( i );
		if ( !IsPaused() ) {
			sdAntiLagManager::GetInstance().CreateUserCommandBranch( otherPlayer );
		}
	}
	msg.WriteBits( MAX_CLIENTS, CLIENTBITS );
}

/*
================
idGameLocal::ServerWriteSnapshot

  Write a snapshot of the current game state for the given client.
================
*/
void idGameLocal::ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, idBitMsg &ucmdmsg ) {
	int oldBitsMain = msg.GetNumBitsWritten();
	if ( g_debugNetworkWrite.GetBool() ) {
		gameLocal.LogNetwork( va( "Writing Network State for client %d\n\n", clientNum ) );
	}

	bool useAOR = net_useAOR.GetBool();

	snapShotClientIsRepeater = false; // Gordon: Main server will never have repeaters connected to it
	SetSnapShotClient( NULL );
	SetSnapShotPlayer( NULL );
	if ( clientNum != ASYNC_DEMO_CLIENT_INDEX ) {
		idPlayer* player = GetClient( clientNum );
		if ( !player ) {
			return;
		}		
		SetSnapShotClient( player );
		SetSnapShotPlayer( player->GetSpectateClient() );

		aorManager.SetClient( snapShotPlayer );
	} else {
		useAOR = false;
	}

	clientNetworkInfo_t&	nwInfo = GetNetworkInfo( clientNum );

	// free too old snapshots
	FreeSnapshotsOlderThanSequence( nwInfo, sequence - 64 );

	snapshot_t* snapshot = AllocateSnapshot( sequence, nwInfo );
	WriteSnapshotGameStates( snapshot, nwInfo, msg );
	WriteSnapshotEntityStates( snapshot, nwInfo, clientNum, msg, useAOR );
	WriteUnreliableEntityNetEvents( clientNum, false, msg );
	WriteSnapshotUserCmds( snapshot, ucmdmsg, clientNum, useAOR );

	snapShotClientIsRepeater = false;
	SetSnapShotClient( NULL );
	SetSnapShotPlayer( NULL );

	if ( g_debugNetworkWrite.GetBool() ) {
		int bitsWrittenMain = msg.GetNumBitsWritten() - oldBitsMain;
		gameLocal.LogNetwork( va( "\nTotal Bits Written: %d\nFinished Writing Network State for client %d\n\n", bitsWrittenMain, clientNum ) );
	}
}

/*
================
idGameLocal::ServerApplySnapshot
================
*/
bool idGameLocal::ServerApplySnapshot( int clientNum, int sequence ) {
	return ApplySnapshot( GetNetworkInfo( clientNum ), sequence );
}

/*
================
idGameLocal::NetworkEventWarning
================
*/
void idGameLocal::NetworkEventWarning( const sdEntityNetEvent& event, const char *fmt, ... ) {
	char buf[1024];
	int length = 0;
	va_list argptr;

	length += idStr::snPrintf( buf+length, sizeof(buf)-1-length, "event %d for entity %d %d: ", event.GetEvent(), event.GetEntityNumber(), event.GetId() );
	va_start( argptr, fmt );
	length = idStr::vsnPrintf( buf+length, sizeof(buf)-1-length, fmt, argptr );
	va_end( argptr );
//	idStr::Append( buf, sizeof(buf), "\n" ); - warning puts a \n already

	gameLocal.DWarning( buf );
}

/*
================
idGameLocal::UnreliableNetworkEventWarning
================
*/
void idGameLocal::UnreliableNetworkEventWarning( const sdUnreliableEntityNetEvent& event, const char *fmt, ... ) {
	char buf[1024];
	int length = 0;
	va_list argptr;

	length += idStr::snPrintf( buf+length, sizeof(buf)-1-length, "unreliable event %d for entity %d %d: ", event.GetEvent(), event.GetEntityNumber(), event.GetId() );
	va_start( argptr, fmt );
	length = idStr::vsnPrintf( buf+length, sizeof(buf)-1-length, fmt, argptr );
	va_end( argptr );
//	idStr::Append( buf, sizeof(buf), "\n" ); - warning puts a \n already

	gameLocal.DWarning( buf );
}

/*
================
idGameLocal::ServerSendQuickChatMessage
================
*/
void idGameLocal::ServerSendQuickChatMessage( idPlayer* player, const sdDeclQuickChat* quickChat, idPlayer* recipient, idEntity* target ) {
	if ( !quickChat ) {
		return;
	}

	if ( !isClient ) {
		if ( player != NULL ) {

            //mal: update this players chat time and what the chat was, so the bots can see it, and respond if needed.
			if ( !botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].isBot ) {
				if ( quickChat->GetType() != NULL_CHAT ) {
 					botThreadData.UpdateChats( player->entityNumber, quickChat );
				}
			}

			const char* callback = quickChat->GetCallback();

			if ( *callback ) {
				sdScriptHelper helper;
				if ( target != NULL ) {
					helper.Push( target->GetScriptObject() );
				}
				player->CallNonBlockingScriptEvent( player->scriptObject->GetFunction( callback ), helper );
			}
		}

		if ( player != NULL && recipient == NULL ) {
			if ( gameLocal.rules->MuteStatus( player ) & MF_CHAT ) {
				const sdUserGroup& userGroup = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
				if ( !userGroup.HasPermission( PF_NO_MUTE ) ) {
					player->SendLocalisedMessage( declHolder.declLocStrType[ "rules/messages/muted" ], idWStrList() );
					return;
				}
			}

			if ( !quickChat->IsTeam() ) {
				if ( si_disableGlobalChat.GetBool() ) {
					const sdUserGroup& userGroup = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
					if ( !userGroup.HasPermission( PF_NO_MUTE ) ) {
						player->SendLocalisedMessage( declHolder.declLocStrType[ "rules/messages/globalchatdisabled" ], idWStrList() );
						return;
					}
				}
			}
		}
	}

	int sendingClientNum = player == NULL ? -1 : player->entityNumber;
	idVec3 location = player == NULL ? vec3_origin : player->GetPhysics()->GetOrigin();
	
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* other = GetClient( i );
		if ( !other ) {
			continue;
		}

		if ( recipient && other != recipient ) {
			continue;
		}

		if ( !quickChat->Check( other, player ) ) {
			continue;
		} else if ( quickChat->IsTeam() && player != NULL ) {
			if ( player->GetEntityAllegiance( other ) != TA_FRIEND ) {
				continue;
			}
		}

		other->PlayQuickChat( location, sendingClientNum, quickChat, target );
	}
}

/*
================
idGameLocal::ServerProcessReliableMessage
================
*/
void idGameLocal::ServerProcessReliableMessage( int clientNum, const idBitMsg &msg ) {
	gameReliableClientMessage_t id = static_cast< gameReliableClientMessage_t >( msg.ReadBits( idMath::BitsForInteger( GAME_RELIABLE_CMESSAGE_NUM_MESSAGES ) ) );

	idPlayer* client = GetClient( clientNum );
	if ( !client ) {
		return;
	}

	switch( id ) {
		case GAME_RELIABLE_CMESSAGE_CHAT:
		case GAME_RELIABLE_CMESSAGE_TEAM_CHAT: {
		case GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT:
			wchar_t text[ 128 ];

			msg.ReadString( text, sizeof( text ) / sizeof( wchar_t ) );

			rules->ProcessChatMessage( client, id, text );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_QUICK_CHAT: {
			const sdDeclQuickChat* quickChat = declQuickChatType.SafeIndex( msg.ReadLong() );
			int spawnId = msg.ReadLong();
			client->RequestQuickChat( quickChat, spawnId );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_KILL: {
			rules->WantKilled( client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_GOD: {
			ToggleGodMode( client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_NOCLIP: {
			ToggleNoclipMode( client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_NETWORKSPAWN: {
			char classname[128];

			msg.ReadString( classname, sizeof( classname ) );

			NetworkSpawn( classname, client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_IMPULSE: {
			client->PerformImpulse( msg.ReadBits( 6 ) );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_GUISCRIPT: {
			idEntity* entity = EntityForSpawnId( msg.ReadBits( 32 ) );
			if ( !entity ) {
				break;
			}

			const int MAX_GUISCRIPT_STRING = 1024;
			char buffer[ MAX_GUISCRIPT_STRING ];
			msg.ReadString( buffer, MAX_GUISCRIPT_STRING );

			HandleGuiScriptMessage( client, entity, buffer );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_NETWORKCOMMAND: {
			idEntity* entity = EntityForSpawnId( msg.ReadBits( 32 ) );
			if ( !entity ) {
				break;
			}

			const int MAX_GUISCRIPT_STRING = 1024;
			char buffer[ MAX_GUISCRIPT_STRING ];
			msg.ReadString( buffer, MAX_GUISCRIPT_STRING );

			HandleNetworkMessage( client, entity, buffer );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_TEAMSWITCH: {
			int index = msg.ReadLong(	);
			char buffer[ 256 ];
			char buffer2[ 256 ];
			msg.ReadString( buffer, sizeof( buffer ) );

			bool useBuffer2 = false;
			if ( index == sdTeamManager::GetInstance().GetNumTeams() + 1 ) {
				msg.ReadString( buffer2, sizeof( buffer2 ) );

				// auto join team with the least number of players
				int lowest = -1;
				int clientTeamIndex = client->GetTeam() != NULL ? client->GetTeam()->GetIndex() : -1;
				for ( int i = 0; i < sdTeamManager::GetInstance().GetNumTeams(); i++ ) {
					int num = sdTeamManager::GetInstance().GetTeamByIndex( i ).GetNumPlayers();
					if ( clientTeamIndex == i ) {
						num--;
					}
					if ( num < lowest || lowest == -1 ) {
						lowest = num;
						index = i + 1;
					}
				}

				if ( index == 2 ) {
					// use password for team 2
					useBuffer2 = true;
				}
			}

			rules->SetClientTeam( client, index, false, useBuffer2 == false ? buffer : buffer2 );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_CLASSSWITCH: {
			int index = msg.ReadLong();
			int classOption = msg.ReadLong();
			const sdDeclPlayerClass* pc = declPlayerClassType.SafeIndex( index );
			if ( pc != NULL ) {
				client->ChangeClass( pc, classOption );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_CHANGEWEAPON: {
			int id = msg.ReadLong();
			if ( client->GetInventory().CanEquip( id, true ) ) {
				client->GetInventory().SetIdealWeapon( id );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_DEFAULTSPAWN: {
			client->SetSpawnPoint( NULL );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_SETSPAWNPOINT: {
			idEntity* ent = gameLocal.EntityForSpawnId( msg.ReadLong() );
			client->SetSpawnPoint( ent );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_GIVECLASS: {
			char buffer[ 128 ];
			msg.ReadString( buffer, sizeof( buffer ) );

			const sdDeclPlayerClass* pc = declPlayerClassType[ buffer ];
			if ( CheatsOk( false ) && pc ) {
				client->GetInventory().GiveClass( pc, true );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_RESPAWN: {
			if ( !client->IsSpectating() && client->IsDead() ) {
				client->ServerForceRespawn( false );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_ADMIN: {
			idCmdArgs args;
			args.AppendArg( "admin" );

			char buffer[ 256 ];
			int numArgs = msg.ReadLong();
			if ( numArgs > 8 ) { // Gordon: prevent evil people making us attempt to read a silly number of args
				numArgs = 8;
			}
			for ( int i = 0; i < numArgs; i++ ) {
				msg.ReadString( buffer, sizeof( buffer ) );
				args.AppendArg( buffer );
			}
			sdAdminSystem::GetInstance().PerformCommand( args, client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_FIRETEAM: {
			idCmdArgs args;
			args.AppendArg( "fireteam" );

			char buffer[ 256 ];
			int numArgs = msg.ReadLong();
			if ( numArgs > 8 ) { // Gordon: prevent evil people making us attempt to read a silly number of args
				numArgs = 8;
			}
			for ( int i = 0; i < numArgs; i++ ) {
				msg.ReadString( buffer, sizeof( buffer ) );
				args.AppendArg( buffer );
			}
			sdFireTeamManager::GetInstance().PerformCommand( args, client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_TASK: {
			qhandle_t handle = msg.ReadBits( sdPlayerTask::TASK_BITS + 1 ) - 1;
			sdTaskManager::GetInstance().SelectTask( client, handle );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_SELECTWEAPON: {
			char buffer[ 256 ];
			msg.ReadString( buffer, sizeof( buffer ) );
			client->SelectWeaponByName( buffer );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_VOTE: {
			sdVoteManager::GetInstance().ServerReadNetworkMessage( client, msg );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_CALLVOTE: {
			idCmdArgs args;
			args.AppendArg( "callvote" );

			char buffer[ 256 ];
			int numArgs = msg.ReadLong();
			if ( numArgs > 8 ) { // Gordon: prevent evil people making us attempt to read a silly number of args
				numArgs = 8;
			}
			for ( int i = 0; i < numArgs; i++ ) {
				msg.ReadString( buffer, sizeof( buffer ) );
				args.AppendArg( buffer );
			}
			sdVoteManager::GetInstance().PerformCommand( args, client );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_RESERVECLIENTSLOT: {
			sdNetClientId netClientId;

			netClientId.id[0] = msg.ReadLong();
			netClientId.id[1] = msg.ReadLong();

			ReserveClientSlot( netClientId );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_REQUESTSTATS: {
			sdGlobalStatsTracker::GetInstance().AddStatsRequest( client->entityNumber );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_ACKNOWLEDGESTATS: {
			sdGlobalStatsTracker::GetInstance().AcknowledgeStatsReponse( client->entityNumber );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_SETMUTESTATUS: {
			bool muteState = msg.ReadBool();
			int clientIndex = msg.ReadByte();
			if ( muteState ) {
				MutePlayerLocal( client, clientIndex );
			} else {
				UnMutePlayerLocal( client, clientIndex );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_SETSPECTATECLIENT: {
			int entityNum = msg.ReadByte();
			if ( entityNum >= 0 && entityNum < MAX_CLIENTS ) {
				client->OnSetClientSpectatee( gameLocal.GetClient( entityNum ) );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_ANTILAGDEBUG: {
			sdAntiLagManager::GetInstance().OnNetworkEvent( clientNum, msg );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_ACKNOWLEDGEBANLIST: {
			if ( clientLastBanIndexReceived[ clientNum ] != -1 ) {
				SendBanList( client );
			}
			break;
		}
		case GAME_RELIABLE_CMESSAGE_REPEATERSTATUS: {
			sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_REPEATERSTATUS );
			msg.WriteBool( gameLocal.isRepeater );
			msg.Send( sdReliableMessageClientInfo( clientNum ) );
			break;
		}
		case GAME_RELIABLE_CMESSAGE_SPECTATECOMMAND: {
			int command = msg.ReadBits( idMath::BitsForInteger( idPlayer::SPECTATE_MAX ) );

			idVec3 origin = vec3_origin;
			idAngles angles = ang_zero;
			if ( command == idPlayer::SPECTATE_POSITION ) {
				origin = msg.ReadVector();
				angles.pitch = msg.ReadFloat();
				angles.yaw = msg.ReadFloat();
				angles.roll = msg.ReadFloat();
			}

			if ( client->IsSpectator() ) {
				client->SpectateCommand( ( idPlayer::spectateCommand_t )command, origin, angles );
			}
			break;
		}
		default: {
			Warning( "Unknown client->server reliable message: %d", id );
			break;
		}
	}
}

/*
================
idGameLocal::ClientShowAOR
================
*/
void idGameLocal::ClientShowAOR( int clientNum ) const {
	if ( !net_clientShowAOR.GetInteger() ) {
		return;
	}

	if ( net_clientShowAOR.GetInteger() & 1 ) {
		gameLocal.aorManager.DebugDraw( clientNum );
	}

	if ( net_clientShowAOR.GetInteger() & 2 ) {
		gameLocal.aorManager.DebugDrawEntities( clientNum );
	}
}

/*
================
idGameLocal::ClientShowSnapshot
================
*/
void idGameLocal::ClientShowSnapshot( int clientNum ) const {
}


// Gordon: FIXME: This is a maintenance nightmare
/*
================
idGameLocal::ResetGameState
================
*/
void idGameLocal::ResetGameState( extNetworkStateMode_t mode ) {
	sdNetworkStateObject& object = GetGameStateObject( mode );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		FreeGameState( clientNetworkInfo[ i ].gameStates[ mode ] );
	}
	FreeGameState( demoClientNetworkInfo.gameStates[ mode ] );
#ifdef SD_SUPPORT_REPEATER
	FreeGameState( repeaterClientNetworkInfo.gameStates[ mode ] );
	for ( int i = 0; i < repeaterNetworkInfo.Num(); i++ ) {
		if ( repeaterNetworkInfo[ i ] == NULL ) {
			continue;
		}
		FreeGameState( repeaterNetworkInfo[ i ]->gameStates[ mode ] );
	}
#endif // SD_SUPPORT_REPEATER

	while ( object.freeStates ) {
		sdGameState* nextState = object.freeStates->next;
		FreeGameState( object.freeStates );
		object.freeStates = nextState;
	}





	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		sdGameState* newState = AllocGameState( object );
		if ( newState->data ) {
			newState->data->MakeDefault();
		}
		clientNetworkInfo[ i ].gameStates[ mode ] = newState;
	}

	sdGameState* newState = AllocGameState( object );
	if ( newState->data ) {
		newState->data->MakeDefault();
	}
	demoClientNetworkInfo.gameStates[ mode ] = newState;	

#ifdef SD_SUPPORT_REPEATER
	newState = AllocGameState( object );
	if ( newState->data ) {
		newState->data->MakeDefault();
	}
	repeaterClientNetworkInfo.gameStates[ mode ] = newState;	

	for ( int i = 0; i < repeaterNetworkInfo.Num(); i++ ) {
		if ( repeaterNetworkInfo[ i ] == NULL ) {
			continue;
		}
		sdGameState* newState = AllocGameState( object );
		if ( newState->data ) {
			newState->data->MakeDefault();
		}
		( *repeaterNetworkInfo[ i ] ).gameStates[ mode ] = newState;
	}
#endif // SD_SUPPORT_REPEATER
}

/*
================
idGameLocal::GetGameStateObject
================
*/
sdNetworkStateObject& idGameLocal::GetGameStateObject( extNetworkStateMode_t mode ) {
	switch ( mode ) {
		case ENSM_TEAM:
			return sdTeamManager::GetInstance().GetStateObject();
		case ENSM_RULES:
			return rulesStateObject;
	}

	Error( "idGameLocal::GetGameStateObject Invalid Mode" );

	return *( sdNetworkStateObject* )( NULL );
}

/*
================
idGameLocal::WriteGameState
================
*/
void idGameLocal::WriteGameState( extNetworkStateMode_t mode, snapshot_t* snapshot, clientNetworkInfo_t& nwInfo, idBitMsg& msg ) {
	sdGameState* baseState = nwInfo.gameStates[ mode ];

	const sdNetworkStateObject& object = GetGameStateObject( mode );
	if ( !object.CheckNetworkStateChanges( *baseState->data ) ) {
		msg.WriteBool( false );
		return;
	}
	msg.WriteBool( true );
	snapshot->gameStates[ mode ] = AllocGameState( object );
	object.WriteNetworkState( *baseState->data, *snapshot->gameStates[ mode ]->data, msg );
}

/*
================
idGameLocal::ReadGameState
================
*/
void idGameLocal::ReadGameState( extNetworkStateMode_t mode, snapshot_t* snapshot, const idBitMsg& msg ) {
	if ( !msg.ReadBool() ) {
		return;
	}

	sdGameState* baseState = GetNetworkInfo( localClientNum ).gameStates[ mode ];

	sdNetworkStateObject& object = GetGameStateObject( mode );
	snapshot->gameStates[ mode ] = AllocGameState( object );
	object.ReadNetworkState( *baseState->data, *snapshot->gameStates[ mode ]->data, msg );
	object.ApplyNetworkState( *snapshot->gameStates[ mode ]->data );
}

/*
================
idGameLocal::EnsureAlloced
================
*/
void idGameLocal::EnsureAlloced( int entityNum, idEntity* ent, const char* oldState, const char* currentState ) {
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES ) {
		Error( "Entitynum (%i) out of range", entityNum );
	}

	if ( !entities[ entityNum ] ) {
		Printf( "Current Entity (%s)\n", currentState );
		Printf( "=======================================\n" );
		entityDebugInfo[ entityNum ].PrintStatus( entityNum );

		if ( ent ) {
			Printf( "Previous Entity To Blame (%s)\n", oldState );
			Printf( "=======================================\n" );
			entityDebugInfo[ ent->entityNumber ].PrintStatus( ent->entityNumber );
		} else {
			Printf( "No Previous Entity To Blame\n" );
		}

		Error( "idGameLocal::EnsureAlloced State Received For Entity With No Header" );
	}
}

/*
================
idGameLocal::UpdateLagometer
================
*/
void idGameLocal::UpdateLagometer( int aheadOfServer, int numDuplicatedUsercmds ) {
	int i, j, ahead;
	for ( i = 0; i < LAGO_HEIGHT; i++ ) {
		memmove( (byte *)lagometer + LAGO_WIDTH * 4 * i, (byte *)lagometer + LAGO_WIDTH * 4 * i + 4, ( LAGO_WIDTH - 1 ) * 4 );
	}
	j = LAGO_WIDTH - 1;
	for ( i = 0; i < LAGO_HEIGHT; i++ ) {
		lagometer[i][j][0] = lagometer[i][j][1] = lagometer[i][j][2] = lagometer[i][j][3] = 0;
	}
	ahead = idMath::Rint( (float)aheadOfServer / (float)USERCMD_MSEC );
	if ( ahead >= 0 ) {
		for ( i = 2 * Max( 0, 5 - ahead ); i < 2 * 5; i++ ) {
			lagometer[i][j][1] = 255;
			lagometer[i][j][3] = 255;
		}
	} else {
		for ( i = 2 * 5; i < 2 * ( 5 + Min( 10, -ahead ) ); i++ ) {
			lagometer[i][j][0] = 255;
			lagometer[i][j][1] = 255;
			lagometer[i][j][3] = 255;
		}
	}
	for ( i = LAGO_HEIGHT - 2 * Min( 6, numDuplicatedUsercmds ); i < LAGO_HEIGHT; i++ ) {
		lagometer[i][j][0] = 255;
		if ( numDuplicatedUsercmds <= 2 ) {
			lagometer[i][j][1] = 255;
		}
		lagometer[i][j][3] = 255;
	}

	if ( renderSystem && !renderSystem->UploadImage( LAGO_IMAGE, (byte *)lagometer, LAGO_IMG_WIDTH, LAGO_IMG_HEIGHT, false, false ) ) {
		common->Printf( "lagometer: UploadImage failed. turning off net_clientLagOMeter\n" );
		net_clientLagOMeter.SetBool( false );
	}
}

/*
================
idGameLocal::ClientReadSnapshot
================
*/
bool idGameLocal::ClientReadSnapshot( int sequence, const int gameFrame, const int gameTime, const int numDuplicatedUsercmds, const int aheadOfServer, const idBitMsg &msg, const idBitMsg &ucmdmsg ) {
	snapshot_t*				snapshot;
	clientNetworkInfo_t&	nwInfo = GetNetworkInfo( localClientNum );

	g_trainingMode.SetBool( false );

	sdPhysicsEvent* physicsEvent;
	sdPhysicsEvent* nextPhysicsEvent;
	for ( physicsEvent = physicsEvents.Next(); physicsEvent; physicsEvent = nextPhysicsEvent ) {
		nextPhysicsEvent = physicsEvent->GetNode().Next();
		if ( gameTime > physicsEvent->GetCreationTime() ) {
			delete physicsEvent;
		}
	}

	if ( net_clientLagOMeter.GetBool() ) {
		UpdateLagometer( aheadOfServer, numDuplicatedUsercmds );
	}

	// clear any debug lines from a previous frame
	gameRenderWorld->DebugClearLines( time );

	// clear any debug polygons from a previous frame
	gameRenderWorld->DebugClearPolygons( time );

	idEntity* ent = NULL;
	for( ent = networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
		ent->snapshotPVSFlags = PVS_DEFERRED_VISIBLE;

		sdEntityState* state = nwInfo.states[ ent->entityNumber ][ NSM_VISIBLE ];
		if ( state == NULL ) {
			gameLocal.Warning( "idGameLocal::ClientReadSnapshot Entity '%s' has no Visible State, Likely a Client Spawned Entity", ent->name.c_str() );
			continue;
		}

		if ( state->data == NULL ) {
			continue;
		}

		// reset back to the old visible state
		ent->ResetNetworkState( NSM_VISIBLE, *state->data );
	}
	for( ent = networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
		ent->snapshotPVSFlags = PVS_DEFERRED_VISIBLE;

		sdEntityState* state = nwInfo.states[ ent->entityNumber ][ NSM_BROADCAST ];
		if ( state == NULL ) {
			gameLocal.Warning( "idGameLocal::ClientReadSnapshot Entity '%s' has no Broadcast State, Likely a Client Spawned Entity", ent->name.c_str() );
			continue;
		}

		if ( state->data == NULL ) {
			continue;
		}

		// reset back to the old broadcast state
		ent->ResetNetworkState( NSM_BROADCAST, *state->data );
	}

	// update the game time
	framenum = gameFrame;
	if ( !IsPaused() ) {
		time = gameTime - timeOffset;
		previousTime = time - networkSystem->ClientGetFrameTime();
	} else {
		timeOffset = gameTime - time;
		previousTime = time;
	}
	isNewFrame = true;
	predictionUpdateRequired = true;

	// clear the snapshot entity list
	snapshotEntities.Clear();
	snapshotVisbileEntities.Clear();

	// allocate new snapshot
	snapshot = snapshotAllocator.Alloc();
	snapshot->Init( sequence, gameTime );
	snapshot->next = nwInfo.snapshots;
	nwInfo.snapshots = snapshot;

	activeSnapshot = snapshot;

	int followPlayer = 0;
	if ( serverIsRepeater ) {
		followPlayer = msg.ReadByte() - 1;
	}

//	ASYNC_SECURITY_READ( msg )
	ReadGameState( ENSM_TEAM, snapshot, msg );
//	ASYNC_SECURITY_READ( msg )

//	ASYNC_SECURITY_READ( msg )
	ReadGameState( ENSM_RULES, snapshot, msg );
//	ASYNC_SECURITY_READ( msg )

	ent = NULL;

	const char* oldState = NULL;

	// process entity events
	ClientProcessEntityNetworkEventQueue();

	if ( serverIsRepeater ) {
		SetSnapShotClient( NULL );
		SetSnapShotPlayer( followPlayer == -1 ? NULL : GetClient( followPlayer ) );
	} else {
		idPlayer* localPlayer = GetLocalPlayer();
		if ( localPlayer ) {
			SetSnapShotClient( localPlayer );
			SetSnapShotPlayer( localPlayer->GetSpectateClient() );
		} else {
			SetSnapShotClient( NULL );
			SetSnapShotPlayer( NULL );
		}
	}

	// read visible states
	for ( int i = msg.ReadBits( GENTITYNUM_BITS ); i != ENTITYNUM_NONE; i = msg.ReadBits( GENTITYNUM_BITS ) ) {
		EnsureAlloced( i, ent, oldState, "Visible" );

		ent = entities[ i ];
		
		ent->snapshotPVSFlags &= ~( PVS_DEFERRED_VISIBLE );
		ent->snapshotPVSFlags |= PVS_VISIBLE;

		sdEntityState* newState = AllocEntityState( NSM_VISIBLE, ent );
		sdEntityState* baseState = nwInfo.states[ i ][ NSM_VISIBLE ];

//		ASYNC_SECURITY_READ( msg )
		ent->ReadNetworkState( NSM_VISIBLE, *baseState->data, *newState->data, msg );
//		ASYNC_SECURITY_READ( msg )

		ent->ApplyNetworkState( NSM_VISIBLE, *newState->data );

		newState->next = snapshot->firstEntityState[ NSM_VISIBLE ];
		snapshot->firstEntityState[ NSM_VISIBLE ] = newState;

		oldState = "Visible";
	}

	// read broadcast states
	for ( int i = msg.ReadBits( GENTITYNUM_BITS ); i != ENTITYNUM_NONE; i = msg.ReadBits( GENTITYNUM_BITS ) ) {
		EnsureAlloced( i, ent, oldState, "Broadcast" );

		ent = entities[ i ];
		
		ent->snapshotPVSFlags |= PVS_BROADCAST;

		sdEntityState* newState = AllocEntityState( NSM_BROADCAST, ent );
		sdEntityState* baseState = nwInfo.states[ i ][ NSM_BROADCAST ];

//		ASYNC_SECURITY_READ( msg )
		ent->ReadNetworkState( NSM_BROADCAST, *baseState->data, *newState->data, msg );
//		ASYNC_SECURITY_READ( msg )

		ent->ApplyNetworkState( NSM_BROADCAST, *newState->data );

		newState->next = snapshot->firstEntityState[ NSM_BROADCAST ];
		snapshot->firstEntityState[ NSM_BROADCAST ] = newState;

		oldState = "Broadcast";
	}

	if ( snapShotPlayer ) {

		// show AOR
		ClientShowAOR( localClientNum );

		// visualize the snapshot
		ClientShowSnapshot( localClientNum );
	}

	if ( SetupClientAoR() ) {
		for ( ent = networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
			// Gordon: Use evaluate position here, which for some fully predictable physics classes 
			// will return a newly evaluated positon, and not the currently last updated one
			idPhysics* physics = ent->GetPhysics();
			bool update = false;
			aorManager.UpdateEntityAORFlags( ent, physics->EvaluatePosition(), update );
			if ( !physics->AllowInhibit() ) {
				ent->aorFlags &= ~AOR_INHIBIT_PHYSICS;
			}
		}
	} else {
		for ( ent = networkedEntities.Next(); ent != NULL; ent = ent->networkNode.Next() ) {
			ent->aorFlags = 0;
		}
	}

	// read the unreliable entity network events
	int numUnreliableEvents = msg.ReadLong();
	for ( int i = 0; i < numUnreliableEvents; i++ ) {
		sdUnreliableEntityNetEvent* event = unreliableEntityNetEventAllocator.Alloc();
		event->GetUnreliableNode().AddToEnd( unreliableEntityNetEventQueue );
		event->Read( msg );

		if ( gameLocal.isRepeater ) {
			gameLocal.SendUnreliableEntityNetworkEvent( *event );
		}
	}

	// read user commands
	while ( true ) {
		int clientNum = ucmdmsg.ReadBits( CLIENTBITS );
		
		if ( clientNum == MAX_CLIENTS ) {
			break;
		}
		
		networkSystem->ReadClientUserCmds( clientNum, ucmdmsg );
	}

	return true;
}


/*
================
idGameLocal::SetupClientAoR
================
*/
bool idGameLocal::SetupClientAoR( void ) {
	if ( !net_useAOR.GetBool() ) {
		return false;
	}

	renderView_t view;
	if ( sdDemoManager::GetInstance().CalculateRenderView( &view ) ) {
		aorManager.SetPosition( view.vieworg );
		return true;
	}

	idPlayer* viewer = GetActiveViewer();
	if ( viewer != NULL ) {
		aorManager.SetClient( viewer );
		return true;
	}

	if ( serverIsRepeater ) {
		playerView.CalculateRepeaterView( view );
		aorManager.SetPosition( view.vieworg );
		return true;
	}

	return false;
}

/*
================
idGameLocal::ClientApplySnapshot
================
*/
bool idGameLocal::ClientApplySnapshot( int sequence ) {
	return ApplySnapshot( GetNetworkInfo( localClientNum ), sequence );
}

/*
================
idGameLocal::WriteClientNetworkInfo
================
*/
void idGameLocal::WriteClientNetworkInfo( idFile* file ) {
	ClientWriteGameState( file );

	clientNetworkInfo_t& nwInfo = GetNetworkInfo( localClientNum );

	SetSnapShotClient( GetLocalPlayer() );
	SetSnapShotPlayer( GetLocalPlayer() );

	for ( idEntity* ent = networkedEntities.Next(); ent; ent = ent->networkNode.Next() ) {
		file->WriteInt( ent->entityNumber );

		int num = ent->entityNumber;

		for ( int j = 0; j < NSM_NUM_MODES; j++ ) {
			if ( nwInfo.states[ num ][ j ] == NULL || nwInfo.states[ num ][ j ]->data == NULL ) {
				file->WriteBool( false );
				continue;
			}

			file->WriteBool( true );
			nwInfo.states[ num ][ j ]->data->Write( file );
			ASYNC_SECURITY_WRITE_FILE( file )
		}
		ASYNC_SECURITY_WRITE_FILE( file )

		for ( int j = 0; j < NSM_NUM_MODES; j++ ) {
			networkStateMode_t mode = ( networkStateMode_t )j;

			sdEntityState* state = AllocEntityState( mode, ent );
			if ( state->data != NULL ) {
				file->WriteBool( true );

				idBitMsg temp;
				byte buffer[ 2048 ];
				temp.InitWrite( buffer, sizeof( buffer ) );
				
				sdEntityState* defaultState = AllocEntityState( mode, ent );
				defaultState->data->MakeDefault();

				ent->WriteNetworkState( mode, *defaultState->data, *state->data, temp );

				FreeNetworkState( defaultState );

				state->data->Write( file );
			} else {
				file->WriteBool( false );
			}

			FreeNetworkState( state );
		}
		ASYNC_SECURITY_WRITE_FILE( file )
	}
	file->WriteInt( MAX_GENTITIES );

	for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		if ( nwInfo.gameStates[ i ] == NULL || nwInfo.gameStates[ i ]->data == NULL ) {
			file->WriteBool( false );
			continue;
		}

		file->WriteBool( true );
		nwInfo.gameStates[ i ]->data->Write( file );
	}

	ASYNC_SECURITY_WRITE_FILE( file )

	for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		extNetworkStateMode_t mode = ( extNetworkStateMode_t )i;

		sdNetworkStateObject& obj = GetGameStateObject( mode );
		sdGameState* state = AllocGameState( obj );
		if ( state->data != NULL ) {
			file->WriteBool( true );

			idBitMsg temp;
			byte buffer[ 2048 ];
			temp.InitWrite( buffer, sizeof( buffer ) );
			
			sdGameState* defaultState = AllocGameState( obj );
			defaultState->data->MakeDefault();

			obj.WriteNetworkState( *defaultState->data, *state->data, temp );

			FreeGameState( defaultState );

			state->data->Write( file );
		} else {
			file->WriteBool( false );
		}

		FreeGameState( state );
	}

	ASYNC_SECURITY_WRITE_FILE( file )

	// write number of snapshots so on readback we know how many to allocate
	int snapCount = 0;
	for ( snapshot_t* snapshot = nwInfo.snapshots; snapshot; snapshot = snapshot->next ) {
		snapCount++;
	}
	file->WriteInt( snapCount );


	for ( snapshot_t* snapshot = nwInfo.snapshots; snapshot; snapshot = snapshot->next ) {
		file->WriteInt( snapshot->sequence );

		for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
			int stateCount = 0;
			for ( sdEntityState* state = snapshot->firstEntityState[ i ]; state; state = state->next ) {
				stateCount++;
			}
			file->WriteInt( stateCount );

			for ( sdEntityState* state = snapshot->firstEntityState[ i ]; state; state = state->next ) {
				file->WriteInt( state->spawnId );
				state->data->Write( file );
			}
		}

		for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
			if ( !snapshot->gameStates[ i ] ) {
				file->WriteBool( false );
				continue;
			}
			file->WriteBool( true );
			snapshot->gameStates[ i ]->data->Write( file );
		}
	}

	SetSnapShotClient( NULL );
	SetSnapShotPlayer( NULL );
}

/*
================
idGameLocal::ReadClientNetworkInfo
================
*/
void idGameLocal::ReadClientNetworkInfo( idFile* file ) {
	ClientReadGameState( file );

	// force new frame
	realClientTime = 0;
	isNewFrame = true;

	SetSnapShotClient( GetLocalPlayer() );
	SetSnapShotPlayer( GetLocalPlayer() );

	clientNetworkInfo_t& nwInfo = GetNetworkInfo( localClientNum );

	while ( true ) {
		int num;
		file->ReadInt( num );
		if ( num == MAX_GENTITIES ) {
			break;
		}

		if ( num < 0 || num >= MAX_GENTITIES ) {
			Error( "idGameLocal::ReadClientNetworkInfo Tried to read out of bounds entity" );
		}

		idEntity* ent = entities[ num ];
		if ( ent == NULL ) {
			Error( "idGameLocal::ReadClientNetworkInfo Tried to Read State For Missing Entity" );
		}

		for ( int j = 0; j < NSM_NUM_MODES; j++ ) {
			bool temp;
			file->ReadBool( temp );
			if ( !temp ) {
				continue;
			}

			FreeNetworkState( nwInfo.states[ num ][ j ] );

			networkStateMode_t mode = ( networkStateMode_t )j;

			sdEntityState& newState = *AllocEntityState( mode, ent );
			if ( newState.data == NULL ) {
				Error( "idGameLocal::ReadClientNetworkInfo Failed to Alloc Network State" );
			}

			newState.data->Read( file );
			
			ASYNC_SECURITY_READ_FILE( file )
			nwInfo.states[ num ][ j ] = &newState;
		}

		ASYNC_SECURITY_READ_FILE( file )

		for ( int j = 0; j < NSM_NUM_MODES; j++ ) {
			bool temp;
			file->ReadBool( temp );
			if ( !temp ) {
				continue;
			}

			networkStateMode_t mode = ( networkStateMode_t )j;

			sdEntityState* state = AllocEntityState( mode, ent );
			assert( state->data != NULL );
			state->data->Read( file );

			ent->ApplyNetworkState( mode, *state->data );

			FreeNetworkState( state );
		}

		ASYNC_SECURITY_READ_FILE( file )
	}

	for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		FreeGameState( nwInfo.gameStates[ i ] );

		extNetworkStateMode_t mode = ( extNetworkStateMode_t )i;
		sdNetworkStateObject& stateObject = GetGameStateObject( mode );

		bool temp;
		file->ReadBool( temp );
		if ( !temp ) {
			continue;
		}

		nwInfo.gameStates[ i ] = AllocGameState( stateObject );
		nwInfo.gameStates[ i ]->data->Read( file );
	}

	ASYNC_SECURITY_READ_FILE( file )

	for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		extNetworkStateMode_t mode = ( extNetworkStateMode_t )i;
		sdNetworkStateObject& stateObject = GetGameStateObject( mode );

		bool temp;
		file->ReadBool( temp );
		if ( !temp ) {
			continue;
		}

		sdGameState* state = AllocGameState( stateObject );
		assert( state->data != NULL );
		state->data->Read( file );

		stateObject.ApplyNetworkState( *state->data );

		FreeGameState( state );
	}

	ASYNC_SECURITY_READ_FILE( file )

	FreeSnapshotsOlderThanSequence( nwInfo, 0x7FFFFFFF );
	
	int count;
	file->ReadInt( count );

	snapshot_t** lastSnapshot = &nwInfo.snapshots;

	for ( int i = 0; i < count; i++ ) {
		snapshot_t* snapshot = snapshotAllocator.Alloc();

		int sequence;
		file->ReadInt( sequence );

		snapshot->Init( sequence, -1 /* FIXME: Gordon: Clients don't use this atm, and i don't want to break demos right now */ );

		for ( int j = 0; j < NSM_NUM_MODES; j++ ) {
			int numStates;
			file->ReadInt( numStates );

			for ( int k = 0; k < numStates; k++ ) {
				int spawnId;
				file->ReadInt( spawnId );

				idEntity* ent = EntityForSpawnId( spawnId );
				if ( !ent ) {
					Error( "idGameLocal::ReadClientNetworkInfo Missing Entity for Snapshot State" );
				}

				sdEntityState* state = AllocEntityState( ( networkStateMode_t )j, ent );
				state->next = snapshot->firstEntityState[ j ];
				snapshot->firstEntityState[ j ] = state;

				if ( state->data ) {
					state->data->Read( file );
				}
			}
		}

		for ( int j = 0; j < ENSM_NUM_MODES; j++ ) {
			bool temp;
			file->ReadBool( temp );
			if ( !temp ) {
				continue;
			}
			snapshot->gameStates[ j ] = AllocGameState( GetGameStateObject( ( extNetworkStateMode_t )j ) );
			snapshot->gameStates[ j ]->data->Read( file );
		}

		*lastSnapshot = snapshot;
		lastSnapshot = &snapshot->next;
	}

	SetSnapShotClient( NULL );
	SetSnapShotPlayer( NULL );
}

/*
================
userInfo_t::ToDict
================
*/
void userInfo_t::ToDict( idDict& info ) const {
	info.Set( "ui_name", baseName.c_str() );
	info.Set( "ui_realname", rawName.c_str() );
	info.SetBool( "ui_showGun", showGun );
	info.SetBool( "ui_ignoreExplosiveWeapons", ignoreExplosiveWeapons );
	info.SetBool( "ui_autoSwitchEmptyWeapons", autoSwitchEmptyWeapons );
	info.SetBool( "ui_postArmFindBestWeapon", postArmFindBestWeapon );
	info.SetBool( "ui_advancedFlightControls", advancedFlightControls );
	info.SetBool( "ui_rememberCameraMode", rememberCameraMode );
	info.SetBool( "ui_drivingCameraFreelook", drivingCameraFreelook );
	info.SetBool( "ui_bot", isBot );
	info.SetBool( "ui_swapFlightYawAndRoll", swapFlightYawAndRoll );
}

/*
================
userInfo_t::FromDict
================
*/
void userInfo_t::FromDict( const idDict& info ) {
	baseName				= info.GetString( "ui_name" ); // doesn't include clan tag
	rawName					= info.GetString( "ui_realname" );
	name					= rawName + "^0";
	cleanName				= rawName;
	idGameLocal::CleanName( cleanName );
	wideName				= va( L"%hs", name.c_str() );
	showGun					= info.GetBool( "ui_showGun" );	
	ignoreExplosiveWeapons	= info.GetBool( "ui_ignoreExplosiveWeapons" );
	autoSwitchEmptyWeapons	= info.GetBool( "ui_autoSwitchEmptyWeapons" );
	postArmFindBestWeapon	= info.GetBool( "ui_postArmFindBestWeapon" );
	advancedFlightControls	= info.GetBool( "ui_advancedFlightControls" );
	rememberCameraMode		= info.GetBool( "ui_rememberCameraMode" );
	drivingCameraFreelook	= info.GetBool( "ui_drivingCameraFreelook" );
	isBot					= info.GetBool( "ui_bot" );
	voipReceiveGlobal		= info.GetBool( "ui_voipReceiveGlobal" );
	voipReceiveTeam			= info.GetBool( "ui_voipReceiveTeam" );
	voipReceiveFireTeam		= info.GetBool( "ui_voipReceiveFireTeam" );
	showComplaints			= info.GetBool( "ui_showComplaints" );
	swapFlightYawAndRoll	= info.GetBool( "ui_swapFlightYawAndRoll" );
}

/*
================
idGameLocal::WriteUserInfo
================
*/
void idGameLocal::WriteUserInfo( idBitMsg& msg, const idDict& info ) {
	userInfo_t userInfo;
	userInfo.FromDict( info );

	msg.WriteString( userInfo.baseName.c_str() );
	msg.WriteString( userInfo.rawName.c_str() );
	msg.WriteBool( userInfo.showGun );
	msg.WriteBool( userInfo.ignoreExplosiveWeapons );
	msg.WriteBool( userInfo.autoSwitchEmptyWeapons );
	msg.WriteBool( userInfo.postArmFindBestWeapon );
	msg.WriteBool( userInfo.advancedFlightControls );
	msg.WriteBool( userInfo.rememberCameraMode );
	msg.WriteBool( userInfo.drivingCameraFreelook );
	msg.WriteBool( userInfo.isBot );
	msg.WriteBool( userInfo.swapFlightYawAndRoll );
}

/*
================
idGameLocal::ReadUserInfo
================
*/
void idGameLocal::ReadUserInfo( const idBitMsg& msg, idDict& info ) {
	char buffer[ 256 ];
		userInfo_t userInfo;

	msg.ReadString( buffer, sizeof( buffer ) );
	userInfo.baseName = buffer;
	msg.ReadString( buffer, sizeof( buffer ) );
	userInfo.rawName = buffer;

	userInfo.showGun = msg.ReadBool();
	userInfo.ignoreExplosiveWeapons = msg.ReadBool();
	userInfo.autoSwitchEmptyWeapons = msg.ReadBool();
	userInfo.postArmFindBestWeapon = msg.ReadBool();
	userInfo.advancedFlightControls = msg.ReadBool();
	userInfo.rememberCameraMode = msg.ReadBool();
	userInfo.drivingCameraFreelook = msg.ReadBool();
	userInfo.isBot = msg.ReadBool();
	userInfo.swapFlightYawAndRoll = msg.ReadBool();

	userInfo.ToDict( info );
}

/*
================
idGameLocal::ClientProcessEntityNetworkEventQueue
================
*/
void idGameLocal::ClientProcessEntityNetworkEventQueue( void ) {
	sdEntityNetEvent* next;
	for ( sdEntityNetEvent* event = entityNetEventQueue.Next(); event; event = next ) {
		next = event->GetNode().Next();

		// only process forward, in order
		if ( event->GetTime() > time ) {
			break;
		}

		idEntity* entity = gameLocal.EntityForSpawnId( event->GetSpawnId() );

		if( !entity ) {
			NetworkEventWarning( *event, "unknown entity" );
		} else {
			if ( gameLocal.isRepeater ) { // Gordon: Save any off that we need to send to clients which connect later
				gameLocal.FreeEntityNetworkEvents( entity, event->GetEvent() );
				if ( event->ShouldSaveEvent() ) {
					gameLocal.SaveEntityNetworkEvent( *event );
				}
			}

			idBitMsg eventMsg;
			event->OutputParms( eventMsg );

			if ( !entity->ClientReceiveEvent( event->GetEvent(), event->GetTime(), eventMsg ) ) {
				NetworkEventWarning( *event, "unknown event" );
			}
		}
		
		event->GetNode().Remove();
		entityNetEventAllocator.Free( event );
	}

	// process unreliable events
	sdUnreliableEntityNetEvent* unreliableNext;
	for ( sdUnreliableEntityNetEvent* event = unreliableEntityNetEventQueue.Next(); event; event = unreliableNext ) {
		unreliableNext = event->GetUnreliableNode().Next();

		// only process forward, in order
		if ( event->GetTime() > time ) {
			break;
		}

		idEntity* entity = gameLocal.EntityForSpawnId( event->GetSpawnId() );

		if( !entity ) {
			UnreliableNetworkEventWarning( *event, "unknown entity" );
		} else {
			idBitMsg eventMsg;
			event->OutputParms( eventMsg );

			if ( !entity->ClientReceiveUnreliableEvent( event->GetEvent(), event->GetTime(), eventMsg ) ) {
				UnreliableNetworkEventWarning( *event, "unknown event" );
			}
		}
		
		event->GetUnreliableNode().Remove();
		unreliableEntityNetEventAllocator.Free( event );
	}
}

/*
================
idGameLocal::ClientReceiveEvent
================
*/
bool idGameLocal::ClientReceiveEvent( const idVec3& origin, int event, int time, const idBitMsg &msg ) {
	return true;
}

/*
================
idGameLocal::FreeNetworkState
================
*/
void idGameLocal::FreeNetworkState( sdEntityState* state ) {
	delete state;
}

/*
================
idGameLocal::FreeGameState
================
*/
void idGameLocal::FreeGameState( sdGameState* state ) {
	delete state;
}

/*
================
idGameLocal::CreateNetworkState
================
*/
void idGameLocal::CreateNetworkState( clientNetworkInfo_t& networkInfo, int entityNum ) {
	idEntity* entity = entities[ entityNum ];

	for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
		if ( !networkInfo.states[ entityNum ][ i ] ) {
			networkInfo.states[ entityNum ][ i ] = AllocEntityState( ( networkStateMode_t )i, entity );
			if ( networkInfo.states[ entityNum ][ i ]->data ) {
				networkInfo.states[ entityNum ][ i ]->data->MakeDefault();
			}
		}
	}
}

/*
================
idGameLocal::CreateNetworkState
================
*/
void idGameLocal::CreateNetworkState( int entityNum ) {
	if ( isClient ) {
		CreateNetworkState( GetNetworkInfo( localClientNum ), entityNum );
	} else {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !entities[ i ] ) {
				continue;
			}
			CreateNetworkState( GetNetworkInfo( i ), entityNum );
		}
		CreateNetworkState( demoClientNetworkInfo, entityNum );
	}

#ifdef SD_SUPPORT_REPEATER
	for ( int i = 0; i < repeaterNetworkInfo.Num(); i++ ) {
		if ( repeaterNetworkInfo[ i ] == NULL ) {
			continue;
		}

		CreateNetworkState( *repeaterNetworkInfo[ i ], entityNum );
	}
#endif // SD_SUPPORT_REPEATER
}

/*
================
idGameLocal::FreeNetworkState
================
*/
void idGameLocal::FreeNetworkState( clientNetworkInfo_t& networkInfo, int entityNum ) {
	for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
		FreeNetworkState( networkInfo.states[ entityNum ][ i ] );
		networkInfo.states[ entityNum ][ i ] = NULL;
	}
}

/*
================
idGameLocal::FreeNetworkState
================
*/
void idGameLocal::FreeNetworkState( int entityNum ) {
	if ( isClient ) {
		FreeNetworkState( GetNetworkInfo( localClientNum ), entityNum );

		// Gordon: Clear any pending events that we may have just caught this frame
		sdEntityNetEvent* next = NULL;
		for ( sdEntityNetEvent* evt = entityNetEventQueue.Next(); evt != NULL; evt = next ) {
			next = evt->GetNode().Next();

			if ( evt->GetEntityNumber() == entityNum ) {
				evt->GetNode().Remove();
				entityNetEventAllocator.Free( evt );
			}
		}
	} else {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			FreeNetworkState( GetNetworkInfo( i ), entityNum );
		}
		FreeNetworkState( demoClientNetworkInfo, entityNum );
	}

#ifdef SD_SUPPORT_REPEATER
	for ( int i = 0; i < repeaterNetworkInfo.Num(); i++ ) {
		if ( repeaterNetworkInfo[ i ] == NULL ) {
			continue;
		}

		FreeNetworkState( *repeaterNetworkInfo[ i ], entityNum );
	}
#endif // SD_SUPPORT_REPEATER
}

/*
================
idGameLocal::ClientSpawn
================
*/
void idGameLocal::ClientSpawn( int entityNum, int spawnId, int typeNum, int entityDefNumber, int mapSpawnId ) {
	idTypeInfo* typeInfo = NULL;
	
	// if there is no entity or an entity of the wrong type

	if ( entities[ entityNum ] ) {
//		Warning( "idGameLocal::ClientSpawn: New Entity Spawned In Already Occupied Position" );
		entities[ entityNum ]->ProcessEvent( &EV_Remove );
		assert( !entities[ entityNum ] );
	}

	spawnCount = spawnId; // Makes the spawn func set up the spawnId properly

	idDict args;
	args.SetInt( "spawn_entnum", entityNum );
	args.Set( "name", va( "entity%d", entityNum ) );

	idEntity* ent = NULL;

	if ( mapSpawnId != -1 ) {
		if ( mapSpawnId > mapFile->GetNumEntities() ) {
			Error( "Tried to spawn out of bounds entity number '%i' from map", mapSpawnId );
		}
		idMapEntity* mapEnt = mapFile->GetEntity( mapSpawnId );

		args.Copy( mapEnt->epairs );

		// precache any media specified in the map entity
		CacheDictionaryMedia( args );

		SpawnEntityDef( args, true, &ent, -1, mapSpawnId );
		assert( ent );
	} else if ( entityDefNumber >= 0 ) {
		if ( entityDefNumber >= declHolder.declEntityDefType.Num() ) {
			Error( "server has %d entityDefs instead of %d", entityDefNumber, declHolder.declEntityDefType.Num() );
		}
		const char* classname = declHolder.declEntityDefType.LocalFindByIndex( entityDefNumber, false )->GetName();
		args.Set( "classname", classname );

		if ( !SpawnEntityDef( args, true, &ent ) ) {
			Error( "Failed to spawn entity with classname '%s'", classname );
		}
	} else {
		idTypeInfo* t = idClass::GetType( typeNum );
		if ( !t ) {
			Error( "Unknown type number %d for entity %d with class number %d", typeNum, entityNum, entityDefNumber );
		}

		ent = CreateEntityType( *t );
		CallSpawnFuncs( ent, &args );
		if ( !entities[ entityNum ] ) {
			Error( "Failed to spawn entity of type '%s'", t->classname );
		}
		ent->PostMapSpawn();
	}
}

/*
================
idGameLocal::SetupGameStateBase
================
*/
void idGameLocal::SetupGameStateBase( clientNetworkInfo_t& networkInfo ) {
	for ( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		FreeGameState( networkInfo.gameStates[ i ] );

		sdGameState* state = AllocGameState( GetGameStateObject( ( extNetworkStateMode_t )i ) );
		if ( state->data ) {
			state->data->MakeDefault();
		}

		networkInfo.gameStates[ i ] = state;
	}
}

/*
================
idGameLocal::SetupEntityStateBase
================
*/
void idGameLocal::SetupEntityStateBase( clientNetworkInfo_t& networkInfo ) {
	idEntity* ent;
	for ( ent = networkedEntities.Next(); ent; ent = ent->networkNode.Next() ) {
		for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
			FreeNetworkState( networkInfo.states[ ent->entityNumber ][ i ] );

			sdEntityState* state = AllocEntityState( ( networkStateMode_t )i, ent );
			if ( state->data ) {
				state->data->MakeDefault();
			}

			networkInfo.states[ ent->entityNumber ][ i ] = state;
		}
	}
}

/*
================
idGameLocal::OnEntityCreateMessage
================
*/
void idGameLocal::OnEntityCreateMessage( const idBitMsg& msg ) {
	int temp			= msg.ReadBits( 32 );
	int entityNum		= EntityNumForSpawnId( temp );
	int spawnId			= SpawnNumForSpawnId( temp );

	int createType		= msg.ReadBits( 2 );
	switch ( createType ) {
		case 0: {
			int mapSpawnId = msg.ReadBits( idMath::BitsForInteger( GetNumMapEntities() ) );
			ClientSpawn( entityNum, spawnId, -1, -1, mapSpawnId );
			break;
		}
		case 1: {
			int entityDefNumber = msg.ReadBits( GetNumEntityDefBits() ) - 1;
			ClientSpawn( entityNum, spawnId, -1, entityDefNumber, -1 );
			break;
		}
		case 2: {
			int typeNum = msg.ReadBits( idClass::GetTypeNumBits() );
			ClientSpawn( entityNum, spawnId, typeNum, -1, -1 );
			break;
		}
		default:
			Error( "idGameLocal::OnEntityCreateMessage Unknown Create Type: '%d'", createType );
			break;
	}

	if ( entities[ entityNum ] == NULL ) {
		Error( "idGameLocal::OnEntityCreateMessage Failed To Create Entity" );
		return;
	}
	entities[ entityNum ]->networkNode.AddToEnd( networkedEntities );
	CreateNetworkState( entityNum );
}

/*
================
idGameLocal::HandleNewEntityEvent
================
*/
void idGameLocal::HandleNewEntityEvent( const idBitMsg &msg ) {
	sdEntityNetEvent* event = entityNetEventAllocator.Alloc();
	event->GetNode().AddToEnd( entityNetEventQueue );
	event->Read( msg );
}

/*
================
idGameLocal::ClientProcessReliableMessage
================
*/
void idGameLocal::ClientProcessReliableMessage( const idBitMsg &msg ) {
	gameLocal.isNewFrame = true;

	idPlayer* localPlayer = GetLocalPlayer();
	
	gameReliableServerMessage_t id = static_cast< gameReliableServerMessage_t >( msg.ReadBits( idMath::BitsForInteger( GAME_RELIABLE_SMESSAGE_NUM_MESSAGES ) ) );

	switch( id ) {
		case GAME_RELIABLE_SMESSAGE_DELETE_ENT: {
			int spawnId = msg.ReadBits( 32 );
			idEntity* ent = EntityForSpawnId( spawnId );
			if ( ent ) {
				ent->ProcessEvent( &EV_Remove );
			}
			break;
		}
		case GAME_RELIABLE_SMESSAGE_MULTI_CREATE_ENT: {
			int count = msg.ReadLong();
			for ( int i = 0; i < count; i++ ) {
				OnEntityCreateMessage( msg );
			}
			break;
		}
		case GAME_RELIABLE_SMESSAGE_CREATE_ENT: {
			OnEntityCreateMessage( msg );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_CHAT:
		case GAME_RELIABLE_SMESSAGE_FIRETEAM_CHAT:
		case GAME_RELIABLE_SMESSAGE_TEAM_CHAT: {
			sdGameRules::chatMode_t chatMode;
			switch ( id ) {
				case GAME_RELIABLE_SMESSAGE_CHAT:
					chatMode = sdGameRules::CHAT_MODE_SAY;
					break;
				case GAME_RELIABLE_SMESSAGE_FIRETEAM_CHAT:
					chatMode = sdGameRules::CHAT_MODE_SAY_FIRETEAM;
					break;
				case GAME_RELIABLE_SMESSAGE_TEAM_CHAT:
					chatMode = sdGameRules::CHAT_MODE_SAY_TEAM;
					break;
			}
			wchar_t text[ 128 ];
			idVec3 location = msg.ReadVector();
			int clientNum = msg.ReadChar();
			msg.ReadString( text, sizeof( text ) / sizeof( wchar_t ) );
			rules->AddChatLine( location, chatMode, clientNum, text );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_QUICK_CHAT: {
			idVec3 location = msg.ReadVector();
			int clientNum = msg.ReadChar();
			int index = msg.ReadLong();
			int spawnID = msg.ReadLong();

			idEntity* target = NULL;
			if ( spawnID != -1 ) {
				target = gameLocal.EntityForSpawnId( spawnID );
			}

			localPlayer->PlayQuickChat( location, clientNum, declQuickChatType.SafeIndex( index ), target );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_MULTI_ENTITY_EVENT: {
			int count = msg.ReadLong();
			for ( int i = 0; i < count; i++ ) {
				HandleNewEntityEvent( msg );
			}
			break;
		}
		case GAME_RELIABLE_SMESSAGE_ENTITY_EVENT: {
			HandleNewEntityEvent( msg );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_TOOLTIP: {
			const sdDeclToolTip* toolTip = declToolTipType.SafeIndex( msg.ReadLong() );
			if ( toolTip == NULL ) {
				return;
			}
			int numParms = msg.ReadLong();
			sdToolTipParms* parms = new sdToolTipParms();

			wchar_t buffer[ 128 ];
			for ( int i = 0; i < numParms; i++ ) {
				msg.ReadString( buffer, sizeof( buffer ) );
				parms->Add( buffer );
			}
			localPlayer->SpawnToolTip( toolTip, parms );
			break;
		}

		case GAME_RELIABLE_SMESSAGE_TASK: {
			sdPlayerTask::HandleMessage( msg );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_FIRETEAM: {
			sdFireTeam::HandleMessage( msg );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_CREATEPLAYERTARGETTIMER: {
			if ( isServer ) {
				return;
			}

			char buffer[ 256 ];
			msg.ReadString( buffer, sizeof( buffer ) );
			qhandle_t serverHandle = msg.ReadShort();

			qhandle_t handle = AllocTargetTimer( buffer );
			if ( targetTimers[ handle ].serverHandle == -1 ) {
				targetTimers[ handle ].serverHandle = serverHandle;
				if ( serverHandle >= targetTimerLookup.Num() ) {
					targetTimerLookup.AssureSize( serverHandle + 1, -1 );
				}
				targetTimerLookup[ serverHandle ] = handle;
				numServerTimers++;

//				assert( targetTimerLookup.Num() == numServerTimers );
			} else {
				assert( targetTimers[ handle ].serverHandle == serverHandle );
			}

			break;
		}
		case GAME_RELIABLE_SMESSAGE_NETWORKEVENT: {
			int entityId = msg.ReadLong();

			char buffer[ 512 ];
			msg.ReadString( buffer, sizeof( buffer ) );

			if ( entityId == NETWORKEVENT_RULES_ID ) {
				rules->OnNetworkEvent( buffer );
            } else if ( entityId == NETWORKEVENT_OBJECTIVE_ID ) { 	 
				sdObjectiveManager::GetInstance().OnNetworkEvent( buffer );
			} else {
				HandleNetworkEvent( EntityForSpawnId( entityId ), buffer );
			}

			break;
		}
		case GAME_RELIABLE_SMESSAGE_CREATEDEPLOYREQUEST: {
			int index = msg.ReadLong();
			
			float rotation							= msg.ReadFloat();
			idPlayer* owner							= EntityForSpawnId( msg.ReadLong() )->Cast< idPlayer >();
			const sdDeclDeployableObject* object	= declDeployableObjectType[ msg.ReadBits( GetNumDeployObjectBits() ) - 1 ];
			idVec3 position							= msg.ReadVector();
			sdTeamInfo* team 						= sdTeamManager::GetInstance().ReadTeamFromStream( msg );

			if ( deployRequests[ index ] ) {
				assert( false );
				delete deployRequests[ index ];
			}

			deployRequests[ index ]					= new sdDeployRequest( object, owner, position, rotation, team, 0 );

			break;
		}

		case GAME_RELIABLE_SMESSAGE_DELETEDEPLOYREQUEST: {
			int index = msg.ReadLong();

			assert( deployRequests[ index ] );

			delete deployRequests[ index ];
			deployRequests[ index ] = NULL;

			break;
		}

		case GAME_RELIABLE_SMESSAGE_USERGROUPS: {
			sdUserGroupManager::GetInstance().ReadNetworkData( msg );
			break;
		}

		case GAME_RELIABLE_SMESSAGE_OBITUARY: {
			int clientIndex		= msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
			int attackerIndex	= msg.ReadLong();
			int damageDeclIndex	= msg.ReadBits( gameLocal.GetNumDamageDeclBits() ) - 1;

			idPlayer* player				= GetClient( clientIndex );
			idEntity* killer				= EntityForSpawnId( attackerIndex );
			const sdDeclDamage* damageDecl	= declDamageType[ damageDeclIndex ];

			if ( player != NULL ) {
				player->Obituary( killer, damageDecl );
			}

			break;
		}

		case GAME_RELIABLE_SMESSAGE_VOTE: {
			sdVoteManager::GetInstance().ClientReadNetworkMessage( msg );
			break;
		}

		case GAME_RELIABLE_SMESSAGE_SETNEXTOBJECTIVE: {
			sdTeamInfo* team = sdTeamManager::GetInstance().ReadTeamFromStream( msg );
			int index = msg.ReadLong();
			sdObjectiveManager::GetInstance().SetNextObjective( team, index );
			break;
		}

		case GAME_RELIABLE_SMESSAGE_RECORD_DEMO: {
			StartRecordingDemo();
			break;
		}
		case GAME_RELIABLE_SMESSAGE_RULES_DATA: {
			sdGameRules::ParseNetworkMessage( msg );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_SETSPAWNPOINT: {
			int spawnId = msg.ReadLong();
			localPlayer->SetSpawnPoint( gameLocal.EntityForSpawnId( spawnId ) );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_FIRETEAM_DISBAND: {
			localPlayer->OnFireTeamDisbanded();
			break;
		}
		case GAME_RELIABLE_SMESSAGE_SESSION_ID: {
			sdnet.ProcessSessionId( msg );
			break;
		}

		case GAME_RELIABLE_SMESSAGE_STATSMESSAGE: {
			sdGlobalStatsTracker::GetInstance().OnServerStatsRequestMessage( msg );
			break;
		}

		case GAME_RELIABLE_SMESSAGE_STATSFINIHED: {
			bool completed = msg.ReadBool();
			if ( completed ) {
				sdGlobalStatsTracker::GetInstance().OnServerStatsRequestCompleted();
			} else {
				sdGlobalStatsTracker::GetInstance().OnServerStatsRequestCancelled();
			}
			break;
		}

		case GAME_RELIABLE_SMESSAGE_LOCALISEDMESSAGE: {
			idVec3 location = msg.ReadVector();
			const sdDeclLocStr* locStr = declHolder.declLocStrType.SafeIndex( msg.ReadLong() );

			wchar_t buffer[ 256 ];

			int parmCount = msg.ReadLong();
			idWStrList strings;
			strings.SetNum( parmCount );
			for ( int i = 0; i < parmCount; i++ ) {
				msg.ReadString( buffer, sizeof( buffer ) );
				strings[ i ] = buffer;
			}

			localPlayer->SendLocalisedMessage( locStr, strings );

			break;
		}

		case GAME_RELIABLE_SMESSAGE_ENDGAMESTATS: {
			int count = msg.ReadLong();
			endGameStats.SetNum( count, false );
			for ( int i = 0; i < count; i++ ) {
				int rankIndex = msg.ReadLong();
				endGameStats[ i ].rank = rankIndex == -1 ? NULL : gameLocal.declRankType.SafeIndex( rankIndex );

				char buffer[ 256 ];
				msg.ReadString( buffer, sizeof( buffer ) );
				endGameStats[ i ].name = buffer;

				endGameStats[ i ].value = msg.ReadFloat();
				float localValue = msg.ReadFloat();
				if ( localClientNum >= 0 && localClientNum < MAX_CLIENTS ) {
					endGameStats[ i ].data[ localClientNum ] = localValue;
				}
			}

			OnEndGameStatsReceived();
			break;
		}
		case GAME_RELIABLE_SMESSAGE_ADMINFEEDBACK: {
			bool success = msg.ReadBool();
			sdAdminSystem::GetInstance().SetCommandState( success ? sdAdminSystemLocal::CS_SUCCESS : sdAdminSystemLocal::CS_FAILED );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_SETCRITICALCLASS: {
			playerTeamTypes_t playerTeam = static_cast< playerTeamTypes_t >( msg.ReadChar() );
			playerClassTypes_t criticalClass = static_cast< playerClassTypes_t >( msg.ReadChar() );
			sdObjectiveManager::GetInstance().SetCriticalClass( playerTeam, criticalClass );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_MAP_RESTART: {
			OnLocalMapRestart();
			break;
		}
		case GAME_RELIABLE_SMESSAGE_PAUSEINFO: {
			bool paused = msg.ReadBool();
			int newTime = msg.ReadLong();
			int newTimeOffset = msg.ReadLong();

			isPaused = paused;
			time = newTime;
			timeOffset = newTimeOffset;
			break;
		}
#ifdef SD_SUPPORT_REPEATER
		case GAME_RELIABLE_SMESSAGE_REPEATER_CHAT: {
			char playerName[ 128 ];
			msg.ReadString( playerName, sizeof( playerName ) );

			int clientIndex = msg.ReadLong();

			wchar_t textBuffer[ 128 ];
			msg.ReadString( textBuffer, sizeof( textBuffer ) / sizeof( textBuffer[ 0 ] ) );

			rules->AddRepeaterChatLine( playerName, clientIndex, textBuffer );

			break;
		}
#endif // SD_SUPPORT_REPEATER
		case GAME_RELIABLE_SMESSAGE_BANLISTMESSAGE: {
			sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_ACKNOWLEDGEBANLIST );
			outMsg.Send();

			guidFile.ListBans( msg );
			break;
		}
		case GAME_RELIABLE_SMESSAGE_BANLISTFINISHED: {
			break;
		}
		case GAME_RELIABLE_SMESSAGE_REPEATERSTATUS: {
			bool repeaterActive = msg.ReadBool();
			if ( repeaterActive ) {
				gameLocal.Printf( "Repeater is running on this server\n" );
			} else {
				gameLocal.Printf( "Repeater is not running on this server\n" );
			}
			break;
		}
		default: {
			Warning( "idGameLocal::ClientProcessReliableMessage: Unknown reliable message: %d", id );
			break;
		}
	}
}

ID_INLINE bool ThinkNow( idEntity* ent, bool newFrame ) {
	return !( ent->snapshotPVSFlags & PVS_DEFERRED_VISIBLE ) || newFrame;
}

/*
================
idGameLocal::OnSnapshotHitch
================
*/
void idGameLocal::OnSnapshotHitch( int snapshotTime ) {
	if ( realClientTime != 0 ) {
		uiManager->OnSnapshotHitch( realClientTime - snapshotTime );
	}
	realClientTime = snapshotTime;
}

/* 
================
idGameLocal::OnClientDisconnected
================
*/
void idGameLocal::OnClientDisconnected( void ) {
	isClient = false;
	realClientTime = 0;
	time = 0;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		GetProficiencyTable( i ).Clear( true );
	}
}

/*
================
idGameLocal::ClientPrediction
================
*/
void idGameLocal::ClientPrediction( const usercmd_t* clientCmds, const usercmd_t* demoCmd ) {

	idPlayer* localPlayer = GetLocalPlayer();
	if ( localPlayer != NULL ) {
		unlock.canUnlockFrames = localPlayer->IsFPSUnlock();
	} else {
		unlock.canUnlockFrames = false;
	}

	unlock.unlockedDraw = false;
	interpolateEntities.Clear();

	idEntity* ent;

	idPlayer* player = GetLocalPlayer();

	sdDemoManagerLocal& demoManager = sdDemoManager::GetInstance();
	if ( demoManager.GetState() == sdDemoManagerLocal::DS_PAUSED ) {
		gameRenderWorld->DebugClearLines( 0 );
	}

	if ( serverIsRepeater && demoCmd != NULL ) {
		playerView.SetRepeaterUserCmd( *demoCmd );
	}

	framenum++;

	int frameTime = networkSystem->ClientGetFrameTime();

	if ( IsPaused() ) {
		msec = 0;
		previousTime = time;
		time += 0;
		timeOffset += frameTime;
	} else {
		msec = frameTime;
		previousTime = time;
		time += frameTime;
		timeOffset += 0;
	}

	int now = gameLocal.ToGuiTime( time );
	// update the real client time and the new frame flag
	if ( now > realClientTime ) {
		realClientTime = now;
		isNewFrame = true;

		if ( predictionUpdateRequired ) {
			for ( idEntity* ent = networkedEntities.Next(); ent; ent = ent->networkNode.Next() ) {
				ent->UpdatePredictionErrorDecay();
			}
			predictionUpdateRequired = false;
		}
	} else {
		isNewFrame = false;
	}

	UpdateDeploymentRequests();

	// check for local client lag
	if ( player ) {
		player->SetLagged( networkSystem->ClientGetTimeSinceLastPacket() >= net_clientMaxPrediction.GetInteger() );
	}

	gameRenderWorld->DebugClearLines( time );
	gameRenderWorld->DebugClearPolygons( time );

#ifdef CLIP_DEBUG_EXTREME
	clip.UpdateTraceLogging();
#endif
	sdProfileHelperManager::GetInstance().Update();

	if ( isNewFrame ) {
		bse->StartFrame();
	}

	// set the user commands for this frame
	memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

	// update demo state
	demoManager.RunDemoFrame( demoCmd );

	if ( !isRepeater ) {
		sdPhysicsEvent* physicsEvent;
		for ( physicsEvent = physicsEvents.Next(); physicsEvent; physicsEvent = physicsEvent->GetNode().Next() ) {
			if ( physicsEvent->GetCreationTime() != time ) {
				continue;
			}
			physicsEvent->Apply();
		}

		for ( int i = 0; i < changedEntities.Num(); i++ ) {
			idEntity* ent = changedEntities[ i ];
			if ( !ent ) {
				continue;
			}

			if ( !ent->thinkFlags || ent->NoThink() ) {
				ent->activeNode.Remove();
				ent->activeNetworkNode.Remove();
			} else {
				if ( ent->IsNetSynced() ) {
					if ( !ent->activeNetworkNode.InList() ) {
						ent->activeNetworkNode.AddToEnd( gameLocal.activeNetworkEntities );
					}
				}
				if ( !ent->activeNode.InList() ) {
					ent->activeNode.AddToEnd( gameLocal.activeEntities );
				}
			}
		}
		changedEntities.Clear();

		idEntity::ClearEntityCaches();

		if ( !isNewFrame ) {
			idEntity* nextEnt;
			for( ent = activeNetworkEntities.Next(); ent != NULL; ent = nextEnt ) {
				nextEnt = ent->activeNetworkNode.Next();
				if ( ( ent->snapshotPVSFlags & PVS_DEFERRED_VISIBLE ) == 0 ) {
					sdProfileHelper_ScopeTimer( "EntityThink", ent->spawnArgs.GetString( "classname" ), g_profileEntityThink.GetBool() );
					ent->Think();
				} else {
					ent->UpdateVisuals();
				}
			}
		} else {
			// sort the active entity list
			SortActiveEntityList();

			idEntity* nextEnt;
			for( ent = activeEntities.Next(); ent != NULL; ent = nextEnt ) {
				sdProfileHelper_ScopeTimer( "EntityThink", ent->spawnArgs.GetString( "classname" ), g_profileEntityThink.GetBool() );
				nextEnt = ent->activeNode.Next();
				ent->Think();
			}
		}

		idPlayer* viewPlayer = GetLocalViewPlayer();
		if ( viewPlayer != gameLocal.localPlayerProperties.GetActivePlayer() ) {
			gameLocal.OnLocalViewPlayerChanged();
		}
		playerView.UpdateProxyView( viewPlayer, false );

		if ( isNewFrame || demoManager.GetState() == sdDemoManagerLocal::DS_PAUSED ) {
			for( ent = postThinkEntities.Next(); ent; ent = ent->GetPostThinkNode()->Next() ) {
				ent->PostThink();
			}
		}
	}

	{
		if ( isNewFrame ) {
			rvClientEntity* cent;
			for( cent = clientSpawnedEntities.Next(); cent != NULL; cent = cent->spawnNode.Next() ) {
				cent->Think();
			}

			localPlayerProperties.UpdateHudModules();

			// Some will remove themselves
			sdEffect::UpdateDeadEffects();

			// service any pending events
			idEvent::ServiceEvents();

			sdTeamManager::GetInstance().Think();
			sdTaskManager::GetInstance().Think();
			sdObjectiveManager::GetInstance().Think();
			sdWayPointManager::GetInstance().Think();
			sdWakeManager::GetInstance().Think();
			sdAntiLagManager::GetInstance().Think();
			tireTreadManager->Think();
			footPrintManager->Think();
			sdGlobalStatsTracker::GetInstance().UpdateStatsRequests();

			if ( gameLocal.serverIsRepeater ) {
				playerView.UpdateRepeaterView();
			}

			idPlayer* viewPlayer = GetLocalViewPlayer();
			if ( viewPlayer != NULL ) {
				playerView.CalculateShake( viewPlayer );

				// fps unlock: record view position
				int index = framenum & 1;
				unlock.originlog[ index ] = viewPlayer->GetRenderView()->vieworg;
			}

			bse->EndFrame();

			UpdateLoggedDecals();

			// Run multiplayer game rules
			if ( rules ) {
				rules->Run();
			}

			// decrease deleted clip model reference counts
			clip.ThreadDeleteClipModels();

			// delete clip models without references for real
			clip.ActuallyDeleteClipModels();
		}

		demoManager.EndDemoFrame();

		if ( isNewFrame || demoManager.GetState() == sdDemoManagerLocal::DS_PAUSED ) {
			// show any debug info for this frame
			RunDebugInfo();
			D_DrawDebugLines();
		}
	}

#ifndef _XENON
	UpdateGameSession();
#endif // _XENON
}

/*
================
idGameLocal::ApplyRulesData
================
*/
void idGameLocal::ApplyRulesData( const sdEntityStateNetworkData& newState ) {
	rules->ApplyNetworkState( newState );
}

/*
================
idGameLocal::ReadRulesData
================
*/
void idGameLocal::ReadRulesData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	rules->ReadNetworkState( baseState, newState, msg );
}

/*
================
idGameLocal::WriteRulesData
================
*/
void idGameLocal::WriteRulesData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	rules->WriteNetworkState( baseState, newState, msg );
}

/*
================
idGameLocal::CheckRulesData
================
*/
bool idGameLocal::CheckRulesData( const sdEntityStateNetworkData& baseState ) const {
	return rules->CheckNetworkStateChanges( baseState );
}

/*
================
idGameLocal::CreateRulesData
================
*/
sdEntityStateNetworkData* idGameLocal::CreateRulesData( void ) const {
	if ( !rules ) {
		return NULL;
	}
	return rules->CreateNetworkStructure();
}
