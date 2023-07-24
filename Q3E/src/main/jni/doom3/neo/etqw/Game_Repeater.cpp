
#include "precompiled.h"
#pragma hdrstop

#ifdef SD_SUPPORT_REPEATER

#include "rules/GameRules.h"

/*
============
idGameLocal::RepeaterClientDisconnect
============
*/
void idGameLocal::RepeaterClientDisconnect( int clientNum ) {
	if ( clientNum >= repeaterNetworkInfo.Num() ) {
		return;
	}

	ShutdownRepeatersNetworkState( clientNum );
}

/*
============
idGameLocal::RepeaterWriteInitialReliableMessages
============
*/
void idGameLocal::RepeaterWriteInitialReliableMessages( int clientNum ) {
	WriteInitialReliableMessages( sdReliableMessageClientInfoRepeater( clientNum ) );
}

/*
============
idGameLocal::RepeaterWriteSnapshot
============
*/
void idGameLocal::RepeaterWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, idBitMsg &ucmdmsg, const repeaterUserOrigin_t& userOrigin, bool clientIsRepeater ) {
	bool useAOR = net_useAOR.GetBool() && !clientIsRepeater;

	snapShotClientIsRepeater = clientIsRepeater;
	SetSnapShotClient( NULL );
	SetSnapShotPlayer( NULL );

	idPlayer* followPlayer = NULL;
	if ( !clientIsRepeater ) {
		if ( userOrigin.followClient >= 0 && userOrigin.followClient < MAX_CLIENTS ) {
			followPlayer = GetClient( userOrigin.followClient );
		}
	}
	if ( followPlayer != NULL ) {
		SetSnapShotPlayer( followPlayer );
		aorManager.SetClient( followPlayer );
		msg.WriteByte( followPlayer->entityNumber + 1 );
	} else {
		aorManager.SetPosition( userOrigin.origin );
		msg.WriteByte( 0 );
	}

	clientNetworkInfo_t&	nwInfo = GetRepeaterNetworkInfo( clientNum );

	// free too old snapshots
	FreeSnapshotsOlderThanSequence( nwInfo, sequence - 64 );

	snapshot_t* snapshot = AllocateSnapshot( sequence, nwInfo );
	WriteSnapshotGameStates( snapshot, nwInfo, msg );
	WriteSnapshotEntityStates( snapshot, nwInfo, -1, msg, useAOR );
	WriteUnreliableEntityNetEvents( clientNum, false, msg );
	WriteSnapshotUserCmds( snapshot, ucmdmsg, -1, useAOR );

	snapShotClientIsRepeater = false;
	SetSnapShotClient( NULL );
	SetSnapShotPlayer( NULL );
}

/*
============
idGameLocal::RepeaterAllowClient
============
*/
allowReply_t idGameLocal::RepeaterAllowClient( int numViewers, int maxViewers, const clientNetworkAddress_t& address, const sdNetClientId& netClientId, const char *guid, const char *password, allowFailureReason_t& reason, bool isRepeater ) {
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

	if ( serverInfo.GetInt( "si_pure" ) && ( rules == NULL || !rules->IsPureReady() ) ) {
		reason = ALLOWFAIL_SERVERSPAWNING;
		return ALLOW_NOTYET;
	}

	bool isPrivateClient = false;
	const char* privatePass = g_privateViewerPassword.GetString();
	if ( *privatePass != '\0' && idStr::Cmp( privatePass, password ) == 0 ) {
		isPrivateClient = true;
	} else if ( isRepeater ) {
		const char* pass = g_repeaterPassword.GetString();
		if ( *pass != '\0' ) {
			if ( idStr::Cmp( pass, password ) != 0 ) {
				reason = ALLOWFAIL_INVALIDPASSWORD;
				Printf( "Rejecting client %s from IP '%i.%i.%i.%i': invalid password\n", guid, address.ip[ 0 ], address.ip[ 1 ], address.ip[ 2 ], address.ip[ 3 ] );
				return ALLOW_BADPASS;
			}
		}
	} else if ( ri_useViewerPass.GetBool() ) {
		const char* pass = g_viewerPassword.GetString();
		if ( *pass == '\0' ) {
			// Gordon: FIXME: This is rubbish
			gameLocal.Warning( "ri_useViewerPass is set but g_password is empty" );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say ri_useViewerPass is set but g_viewerPassword is empty" );
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

	int maxPrivatePlayers = ri_privateViewers.GetInteger();
	if ( maxPrivatePlayers > maxViewers ) {
		maxPrivatePlayers = maxViewers;
	}
	int maxRegularPlayers = maxViewers - maxPrivatePlayers;
	int numAvailableSlots = maxRegularPlayers;
	if ( isPrivateClient ) {
		numAvailableSlots = maxViewers; // with private password, any slot is available
	}

	if ( numViewers >= numAvailableSlots ) {
		if ( numViewers >= maxPrivatePlayers ) {
			reason = ALLOWFAIL_SERVERFULL;
			return ALLOW_NOTYET;
		}
		reason = ALLOWFAIL_INVALIDPASSWORD;
		return ALLOW_BADPASS;
	}

	return ALLOW_YES;
}

/*
============
idGameLocal::RepeaterClientBegin
============
*/
void idGameLocal::RepeaterClientBegin( int clientNum ) {
	if ( clientNum >= repeaterNetworkInfo.Num() ) {
		int oldSize = repeaterNetworkInfo.Num();
		repeaterNetworkInfo.SetNum( clientNum + 1 );
		for ( int i = oldSize; i <= clientNum; i++ ) {
			repeaterNetworkInfo[ i ] = NULL;
		}
	}

	if ( repeaterNetworkInfo[ clientNum ] == NULL ) {
		repeaterNetworkInfo[ clientNum ] = new clientNetworkInfo_t();
	}

	SetupGameStateBase( *repeaterNetworkInfo[ clientNum ] );
	SetupEntityStateBase( *repeaterNetworkInfo[ clientNum ] );

	// client needs to get this straight away
	sdReliableServerMessage ruleMsg( GAME_RELIABLE_SMESSAGE_RULES_DATA );
	ruleMsg.WriteLong( sdGameRules::EVENT_CREATE );
	ruleMsg.WriteLong( rules->GetType()->typeNum );
	ruleMsg.Send( sdReliableMessageClientInfoRepeater( clientNum ) );
	rules->WriteInitialReliableMessages( sdReliableMessageClientInfoRepeater( clientNum ) );
}

/*
============
idGameLocal::SetRepeaterState
============
*/
void idGameLocal::SetRepeaterState( bool isRepeater ) {
	this->isRepeater = isRepeater;

	if ( net_serverDownload.GetInteger() == 3 ) {
		networkSystem->HTTPEnable( this->isServer || this->isRepeater );
	}

	UpdateRepeaterInfo();
}

/*
===========
idGameLocal::UpdateRepeaterInfo
============
*/
void idGameLocal::UpdateRepeaterInfo( void ) {
	if ( !isRepeater ) {
		return;
	}

	idDict repeaterInfo;

	BuildRepeaterInfo( repeaterInfo );

	networkSystem->RepeaterSetInfo( repeaterInfo );
}

/*
===========
idGameLocal::BuildRepeaterInfo
============
*/
void idGameLocal::BuildRepeaterInfo( idDict &repeaterInfo ) {
	repeaterInfo = serverInfo;
	repeaterInfo.SetBool( "si_tv", true );
	repeaterInfo.SetBool( "si_usePass", ri_useViewerPass.GetBool() );
	repeaterInfo.SetInt( "si_privateClients", ri_privateViewers.GetInteger() );

	const char* ri_name = cvarSystem->GetCVarString( "ri_name" );
	if ( *ri_name != '\0' ) {
		if ( *serverInfo.GetString( "ri_origName" ) == '\0' ) {
			repeaterInfo.Set( "ri_origName", serverInfo.GetString( "si_name" ) );
		}
		repeaterInfo.Set( "si_name", ri_name );
	}

	const char *si_serverURL = cvarSystem->GetCVarString( "si_serverURL" );
	if ( *si_serverURL != '\0' ) {
		if ( *serverInfo.GetString( "ri_origServerURL" ) == '\0' ) {
			repeaterInfo.Set( "ri_origServerURL", serverInfo.GetString( "si_serverURL" ) );
		}
		repeaterInfo.Set( "si_serverURL", si_serverURL );
	}

	repeaterInfo.Copy( *cvarSystem->MoveCVarsToDict( CVAR_REPEATERINFO ) );
}

/*
===========
idGameLocal::GetNumRepeaterClients
============
*/
int idGameLocal::GetNumRepeaterClients( void ) const {
	return repeaterNetworkInfo.Num();
}

/*
===========
idGameLocal::IsRepeaterClientConnected
============
*/
bool idGameLocal::IsRepeaterClientConnected( int clientNum ) const {
	return repeaterNetworkInfo[ clientNum ] != NULL;
}

/*
===========
idGameLocal::ShutdownRepeatersNetworkStates
============
*/
void idGameLocal::ShutdownRepeatersNetworkStates( void ) {
	for ( int i = 0; i < repeaterNetworkInfo.Num(); i++ ) {
		ShutdownRepeatersNetworkState( i );
	}
}

/*
===========
idGameLocal::ShutdownRepeatersNetworkState
============
*/
void idGameLocal::ShutdownRepeatersNetworkState( int clientNum ) {
	if ( repeaterNetworkInfo[ clientNum ] != NULL ) {
		repeaterNetworkInfo[ clientNum ]->Reset();
		delete repeaterNetworkInfo[ clientNum ];
		repeaterNetworkInfo[ clientNum ] = NULL;
	}
}

/*
================
idGameLocal::RepeaterApplySnapshot
================
*/
bool idGameLocal::RepeaterApplySnapshot( int clientNum, int sequence ) {
	return ApplySnapshot( GetRepeaterNetworkInfo( clientNum ), sequence );
}

/*
================
idGameLocal::RepeaterProcessReliableMessage
================
*/
void idGameLocal::RepeaterProcessReliableMessage( int clientNum, const idBitMsg &msg ) {
	gameReliableClientMessage_t id = static_cast< gameReliableClientMessage_t >( msg.ReadBits( idMath::BitsForInteger( GAME_RELIABLE_CMESSAGE_NUM_MESSAGES ) ) );

	switch( id ) {
		case GAME_RELIABLE_CMESSAGE_CHAT:
		case GAME_RELIABLE_CMESSAGE_TEAM_CHAT: {
		case GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT:
			if ( g_noTVChat.GetBool() ) {
				break;
			}

			wchar_t text[ 128 ];
			msg.ReadString( text, sizeof( text ) / sizeof( wchar_t ) );

			const idDict& clientInfo = networkSystem->RepeaterGetClientInfo( clientNum );
			idStr playerName = clientInfo.GetString( "ui_name" );
			idStr cleanName = playerName;
			cleanName.RemoveColors();
			cleanName.StripLeadingWhiteSpace();
			cleanName.StripTrailingWhiteSpace();
			if ( cleanName.IsEmpty() ) {
				playerName = "Player";
			}

			sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_REPEATER_CHAT );
			outMsg.WriteString( playerName.c_str(), 128, false );
			outMsg.WriteLong( clientNum );
			outMsg.WriteString( text, 128 );
			outMsg.Send( sdReliableMessageClientInfoRepeaterLocal() );
			break;
		}
	}
}

#endif // SD_SUPPORT_REPEATER