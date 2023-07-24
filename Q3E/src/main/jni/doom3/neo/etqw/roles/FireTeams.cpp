// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "FireTeams.h"
#include "../Player.h"
#include "../botai/Bot.h"
#include "../rules/GameRules.h"
#include "../rules/VoteManager.h"

#include "../../idlib/PropertiesImpl.h"

/*
===============================================================================

	sdFireTeamSystemCommand

===============================================================================
*/

/*
============
sdFireTeamSystemCommand::CommandCompletion
============
*/
void sdFireTeamSystemCommand::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
===============================================================================

	sdFireTeamSystemCommand_Join

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Join::PerformCommand
============
*/
void sdFireTeamSystemCommand_Join::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam ) {
		// TODO: PRINT
		return;
	}

	if ( cmd.Argc() < 3 ) {
		// TODO: PRINT
		return;
	}

	int fireTeamIndex;
	sdProperties::sdFromString( fireTeamIndex, cmd.Argv( 2 ) );

	if ( fireTeamIndex < 0 || fireTeamIndex >= sdTeamInfo::MAX_FIRETEAMS ) {
		// TODO: PRINT
		return;
	}

	sdTeamInfo* team = player->GetGameTeam();
	if ( !team ) {
		// TODO: PRINT
		return;
	}

	fireTeam = &team->GetFireTeam( fireTeamIndex );

	if ( fireTeam->IsPrivate() ) {
		const sdDeclToolTip* decl = gameLocal.declToolTipType[ "tooltip_fireteam_private" ];
		if ( decl == NULL ) {
			gameLocal.Warning( "sdFireTeamSystemCommand_Join::PerformCommand Invalid Tooltip" );
		} else {
			player->SendToolTip( decl );
		}
		return;
	}

	fireTeam->AddMember( player->entityNumber );
}

/*
============
sdFireTeamSystemCommand_Join::CommandCompletion
============
*/
void sdFireTeamSystemCommand_Join::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
	if ( fireTeam != NULL ) {
		return;
	}

	sdFireTeamManagerLocal& ftManager = sdFireTeamManager::GetInstance();
	for ( int i = 0; i < sdTeamInfo::MAX_FIRETEAMS; i++ ) {
		sdFireTeam* ft = ftManager.FireTeamForIndex( i );
		callback( va( "%s %s %i (\"%s\" - %i members)", args.Argv( 0 ), args.Argv( 1 ), i, ft->GetName(), ft->GetNumMembers() ) );
	}
}

/*
===============================================================================

	sdFireTeamSystemCommand_Leave

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Leave::PerformCommand
============
*/
void sdFireTeamSystemCommand_Leave::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( !fireTeam ) {
		// TODO: PRINT
		return;
	}

	fireTeam->RemoveMember( player->entityNumber );
}

/*
===============================================================================

	sdFireTeamSystemCommand_Create

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Create::PerformCommand
============
*/
void sdFireTeamSystemCommand_Create::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam ) {
		// TODO: PRINT
		return;
	}

	sdTeamInfo* team = player->GetTeam();
	if ( !team ) {
		// TODO: PRINT
		return;
	}

	for ( int i = 0; i < sdTeamInfo::MAX_FIRETEAMS; i++ ) {
		sdFireTeam& fireTeam = team->GetFireTeam( i );
		if ( fireTeam.GetNumMembers() > 0 ) {
			continue;
		}

		fireTeam.AddMember( player->entityNumber );
		fireTeam.SetPrivate( true );
		return;
	}

	// TODO: PRINT
}

/*
===============================================================================

	sdFireTeamSystemCommand_Invite

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Invite::PerformCommand
============
*/
void sdFireTeamSystemCommand_Invite::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( !fireTeam ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	idPlayer* other = gameLocal.GetClientByName( cmd.Argv( 2 ) );
	if ( !other ) {
		// TODO: PRINT
		return;
	}

	// throttle number of invites per client
	if ( fireTeam->GetLastInvite( other->entityNumber ) != 0 && fireTeam->GetLastInvite( other->entityNumber ) + SEC2MS( 10 ) > gameLocal.time ) {
		if ( player != NULL ) {
			player->SendLocalisedMessage( declHolder.declLocStrType[ "fireteam/messages/invitethrottle" ], idWStrList() );
		}
		return;
	}
	fireTeam->SetLastInvite( other->entityNumber, gameLocal.time );

	fireTeam->Invite( other );
};

/*
============
sdFireTeamSystemCommand_Invite::CommandCompletion
============
*/
void sdFireTeamSystemCommand_Invite::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
	if ( fireTeam == NULL ) {
		return;
	}

	if ( fireTeam->GetCommander() != localPlayer ) {
		return;
	}

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( gameLocal.rules->GetPlayerFireTeam( player->entityNumber ) != NULL ) {
			continue;
		}

		if ( player->GetEntityAllegiance( localPlayer ) != TA_FRIEND ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdFireTeamSystemCommand_Propose

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Propose::PerformCommand
============
*/
void sdFireTeamSystemCommand_Propose::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( !fireTeam ) {
		// TODO: PRINT
		return;
	}

	idPlayer* other = gameLocal.GetClientByName( cmd.Argv( 2 ) );
	if ( !other ) {
		// TODO: PRINT
		return;
	}

	// check outstanding proposals for player, throttle if there is too many
	if ( player != NULL && fireTeam->GetCommander() != player ) {
		int count = sdVoteManager::GetInstance().NumActiveVotes( VI_FIRETEAM_PROPOSE, player );
		if ( count > 3 ) {
			player->SendLocalisedMessage( declHolder.declLocStrType[ "fireteam/messages/proposethrottle" ], idWStrList() );
			return;
		}
	}

	fireTeam->Propose( other, player );
}

/*
============
sdFireTeamSystemCommand_Propose::CommandCompletion
============
*/
void sdFireTeamSystemCommand_Propose::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
	if ( fireTeam == NULL ) {
		return;
	}

	if ( fireTeam->GetCommander() == localPlayer ) {
		return;
	}

	for ( int i = 0; i < fireTeam->GetNumMembers(); i++ ) {
		idPlayer* player = fireTeam->GetMember( i );
		
		if ( player == NULL ) {
			continue;
		}

		if ( player->userInfo.isBot ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdFireTeamSystemCommand_Kick

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Kick::PerformCommand
============
*/
void sdFireTeamSystemCommand_Kick::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( !fireTeam ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	idPlayer* other = gameLocal.GetClientByName( cmd.Argv( 2 ) );
	if ( !other ) {
		// TODO: PRINT
		return;
	}

	if ( other == player ) {
		// TODO: PRINT
		return;
	}

	sdFireTeam* otherFireTeam = gameLocal.rules->GetPlayerFireTeam( other->entityNumber );
	if ( otherFireTeam != fireTeam ) {
		// TODO: PRINT
		return;
	}

	const sdDeclToolTip* decl = gameLocal.declToolTipType[ "tooltip_kick_fireteam_member" ];
	if ( decl == NULL ) {
		gameLocal.Warning( "sdFireTeamSystemCommand_Kick::AddMember Invalid Tooltip" );
	} else {
		other->SendToolTip( decl );
	}

	fireTeam->RemoveMember( other->entityNumber );
}

/*
============
sdFireTeamSystemCommand_Kick::CommandCompletion
============
*/
void sdFireTeamSystemCommand_Kick::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
	if ( fireTeam == NULL ) {
		return;
	}

	if ( fireTeam->GetCommander() != localPlayer ) {
		return;
	}

	for ( int i = 0; i < fireTeam->GetNumMembers(); i++ ) {
		idPlayer* player = fireTeam->GetMember( i );
		
		if ( player == NULL || player == localPlayer ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdFireTeamSystemCommand_Disband

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Disband::PerformCommand
============
*/
void sdFireTeamSystemCommand_Disband::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( !fireTeam ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	fireTeam->SendDisbandMessage();
	fireTeam->Disband();
}

/*
===============================================================================

	sdFireTeamSystemCommand_Promote

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Promote::PerformCommand
============
*/
void sdFireTeamSystemCommand_Promote::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( !fireTeam ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	idPlayer* other = gameLocal.GetClientByName( cmd.Argv( 2 ) );
	if ( !other ) {
		// TODO: PRINT
		return;
	}

	if ( other == player ) {
		// TODO: PRINT
		return;
	}

	sdFireTeam* otherFireTeam = gameLocal.rules->GetPlayerFireTeam( other->entityNumber );
	if ( otherFireTeam != fireTeam ) {
		// TODO: PRINT
		return;
	}

	fireTeam->PromoteMember( other->entityNumber );
}

/*
============
sdFireTeamSystemCommand_Promote::CommandCompletion
============
*/
void sdFireTeamSystemCommand_Promote::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
	if ( fireTeam == NULL ) {
		return;
	}

	if ( fireTeam->GetCommander() != localPlayer ) {
		return;
	}

	for ( int i = 0; i < fireTeam->GetNumMembers(); i++ ) {
		idPlayer* player = fireTeam->GetMember( i );
		
		if ( player == NULL || player == localPlayer ) {
			continue;
		}

		if ( player->userInfo.isBot ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdFireTeamSystemCommand_Name

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Name::PerformCommand
============
*/
void sdFireTeamSystemCommand_Name::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam == NULL ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	if ( cmd.Argc() < 3 ) {
		// TODO: PRINT
		return;
	}

	idStr name = cmd.Argv( 2 );
	idStr baseName = name;

	name.RemoveColors();
	name.StripTrailingWhiteSpace();
	name.StripLeadingWhiteSpace();
	if ( name.Length() == 0 ) {
		// TODO: PRINT
		return;
	}

	sdTeamInfo* team = player->GetGameTeam();
	if ( team == NULL ) {
		assert( false );
		return;
	}

	for ( int i = 0; i < sdTeamInfo::MAX_FIRETEAMS; i++ ) {
		sdFireTeam& teamFireTeam = team->GetFireTeam( i );

		idStr fireTeamName = teamFireTeam.GetName();
		fireTeamName.RemoveColors();
		fireTeamName.StripTrailingWhiteSpace();
		fireTeamName.StripLeadingWhiteSpace();

		if ( name.Icmp( fireTeamName.c_str() ) == 0 ) {
			// TODO: PRINT
			return;
		}

		if ( fireTeam != &teamFireTeam ) {
			if ( name.Icmp( sdTeamInfo::GetDefaultFireTeamName( i ) ) == 0 ) {
				// TODO: PRINT
				return;
			}
		}
	}

	fireTeam->SetName( baseName.c_str() );
}

/*
===============================================================================

sdFireTeamSystemCommand_Private

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Private::PerformCommand
============
*/
void sdFireTeamSystemCommand_Private::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam == NULL ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	if ( cmd.Argc() < 2 ) {
		// TODO: PRINT
		return;
	}

	fireTeam->SetPrivate( true );
}

/*
===============================================================================

sdFireTeamSystemCommand_Public

===============================================================================
*/

/*
============
sdFireTeamSystemCommand_Public::PerformCommand
============
*/
void sdFireTeamSystemCommand_Public::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam == NULL ) {
		// TODO: PRINT
		return;
	}

	if ( fireTeam->GetCommander() != player ) {
		// TODO: PRINT
		return;
	}

	if ( cmd.Argc() < 2 ) {
		// TODO: PRINT
		return;
	}

	fireTeam->SetPrivate( false );
}

/*
===============================================================================

	sdFireTeam

===============================================================================
*/

int sdFireTeam::_fireTeamBits = 0;

/*
================
sdFireTeam::sdFireTeam
================
*/
sdFireTeam::sdFireTeam( void ) : _index( -1 ), _name( "Empty" ), _private( false ) {
	memset( _lastInvite, 0, sizeof( _lastInvite ) );
}

/*
================
sdFireTeam::~sdFireTeam
================
*/
sdFireTeam::~sdFireTeam( void ) {
}

/*
================
sdFireTeam::MaxMembers
================
*/
int sdFireTeam::MaxMembers( void ) {
	return MAX_MISSION_TEAM_PLAYERS;
}

/*
================
sdFireTeam::Init
================
*/
void sdFireTeam::Init( int index ) {
	_index = index;
}

/*
================
sdFireTeam::Clear
================
*/
void sdFireTeam::Clear( void ) {
	Disband();
	memset( _lastInvite, 0, sizeof( _lastInvite ) );
}

/*
================
sdFireTeam::AddMember
================
*/
void sdFireTeam::AddMember( int clientNum ) {
	for ( int i = 0; i < _members.Num(); i++ ) {
		if ( _members[ i ] != clientNum ) {
			continue;
		}

		assert( false );
		return;
	}

	if ( _members.Num() == MaxMembers() ) {
		const sdDeclToolTip* toolTip = gameLocal.declToolTipType[ "tooltip_fireteam_full" ];
		if ( toolTip == NULL ) {
			gameLocal.Warning( "sdFireTeam::AddMember: Invalid Tooltip" );
		} else {
			idPlayer *player = gameLocal.GetClient( clientNum );
			player->SendToolTip( toolTip );
		}
		return;
	}

	idPlayer* oldLeader = GetCommander();

	_members.Alloc() = clientNum;

	gameLocal.rules->SetPlayerFireTeam( clientNum, this );

	if ( !gameLocal.isClient ) {
		idPlayer* client = gameLocal.GetClient( clientNum );
		assert( client );
		sdVoteManager::GetInstance().CancelFireTeamVotesForPlayer( client );

		if ( client != GetCommander() ) {
			client->SetActiveTask( -1 );
		}
	}

	idPlayer* newLeader = GetCommander();

	if ( newLeader != NULL && newLeader != oldLeader ) {
		// Gordon: uncomment this is we want this for creation of a fire team
		//newLeader->OnBecomeFireTeamLeader();

		// reset throttling of invites since we have a new leader
		memset( _lastInvite, 0, sizeof( _lastInvite ) );
	}

	if ( gameLocal.isServer ) {
		sdFireTeamMessage msg( this, FM_ADDMEMBER );
		msg.WriteBits( clientNum, idMath::BitsForInteger( MAX_CLIENTS ) );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdFireTeam::RemoveMember
================
*/
void sdFireTeam::RemoveMember( int clientNum ) {
	idPlayer* oldLeader = GetCommander();

	_members.Remove( clientNum );

	gameLocal.rules->SetPlayerFireTeam( clientNum, NULL );

	idPlayer* newLeader = GetCommander();

	bool leaderValid = true;
	if ( newLeader != NULL && newLeader != oldLeader ) {
		if ( newLeader->userInfo.isBot ) {
			leaderValid = false;
			for ( int i = 1; i < GetNumMembers(); i++ ) {
				newLeader = GetMember( i );

				if ( !newLeader->userInfo.isBot ) {
					int swap = _members[ 0 ];
					_members[ 0 ] = _members[ i ];
					_members[ i ] = swap;
					leaderValid = true;
					break;
				}
			}
		}

		if ( leaderValid ) {
			newLeader->OnBecomeFireTeamLeader();

			// reset throttling of invites since we have a new leader
			memset( _lastInvite, 0, sizeof( _lastInvite ) );
		}
	}

	// reset throttling of invite for the player we just removed
	_lastInvite[ clientNum ] = 0;

	if ( !leaderValid || _members.Num() == 0 ) {
		Disband();
	}

	if ( gameLocal.isServer ) {
		sdFireTeamMessage msg( this, FM_REMOVEMEMBER );
		msg.WriteBits( clientNum, idMath::BitsForInteger( MAX_CLIENTS ) );
		msg.Send( sdReliableMessageClientInfoAll() );

		if ( _members.Num() == 0 ) {
			SetName( _defaultName.c_str() );
		}
	}
}

/*
================
sdFireTeam::SetPrivate
================
*/
void sdFireTeam::SetPrivate( bool isPrivate ) {
	if ( isPrivate == _private ) {
		return;
	}

	_private = isPrivate;

	if ( gameLocal.isServer ) {
		sdFireTeamMessage msg( this, FM_SETPRIVATE );
		msg.WriteBool( isPrivate );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdFireTeam::SetName
================
*/
void sdFireTeam::SetName( const char* name ) {
	_name = name;

	if ( gameLocal.isServer ) {
		sdFireTeamMessage msg( this, FM_SETNAME );
		msg.WriteString( _name.c_str(), 32 );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdFireTeam::SetDefaultName
================
*/
void sdFireTeam::SetDefaultName( const char* name ) {
	_name = name;
	_defaultName = name;
}

/*
================
sdFireTeam::HandleMessages
================
*/
void sdFireTeam::HandleMessage( const idBitMsg& msg ) {
	if ( gameLocal.isServer ) {
		return; // listen servers should ignore these
	}

	fireteamMessageType_t type = ( fireteamMessageType_t )msg.ReadBits( idMath::BitsForInteger( FM_NUM_MESSAGES ) );
	qhandle_t handle = msg.ReadBits( _fireTeamBits );
	sdFireTeam* fireTeam = sdFireTeamManager::GetInstance().FireTeamForHandle( handle );
	if ( fireTeam ) {
		fireTeam->HandleMessage( type, msg );
	} else {
		gameLocal.Warning( "sdFireTeam::HandleMessage Invalid Handle %d", handle );
	}
}

/*
================
sdFireTeam::WriteInitialState
================
*/
void sdFireTeam::WriteInitialState( const sdReliableMessageClientInfoBase& target ) const {
	sdFireTeamMessage msg( this, FM_FULLSTATE );
	msg.WriteBool( _private );
	msg.WriteBits( _members.Num(), idMath::BitsForInteger( MaxMembers() ) );
	for ( int i = 0; i < _members.Num(); i++ ) {
		msg.WriteBits( _members[ i ], idMath::BitsForInteger( MAX_CLIENTS ) );
	}
	msg.WriteString( _name.c_str() );
	msg.Send( target );
}

/*
================
sdFireTeam::Write
================
*/
void sdFireTeam::Write( idFile* file ) const {
	file->WriteBool( _private );
	file->WriteInt( _members.Num() );
	for ( int i = 0; i < _members.Num(); i++ ) {
		file->WriteInt( _members[ i ] );
	}
	file->WriteString( _name.c_str() );
}

/*
================
sdFireTeam::Read
================
*/
void sdFireTeam::Read( idFile* file ) {
	Disband();

	file->ReadBool( _private );
	int count;
	file->ReadInt( count );
	for ( int i = 0; i < count; i++ ) {
		int temp;
		file->ReadInt( temp );
		AddMember( temp );
	}
	file->ReadString( _name );
}

/*
================
sdFireTeam::ReadInitialState
================
*/
void sdFireTeam::ReadInitialState( const idBitMsg& msg ) {
	Disband();

	_private = msg.ReadBool();
	int count = msg.ReadBits( idMath::BitsForInteger( MaxMembers() ) );
	for ( int i = 0; i < count; i++ ) {
		AddMember( msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) ) );
	}

	char buffer[ 32 ];
	msg.ReadString( buffer, sizeof( buffer ) );
	_name = buffer;
}

/*
================
sdFireTeam::HandleMessage
================
*/
void sdFireTeam::HandleMessage( fireteamMessageType_t type, const idBitMsg& msg ) {
	switch ( type ) {
		case FM_FULLSTATE: {
			ReadInitialState( msg );
			break;
		}
		case FM_PROMOTEMEMBER: {
			PromoteMember( msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) ) );
			break;
		}
		case FM_ADDMEMBER: {
			AddMember( msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) ) );
			break;
		}
		case FM_REMOVEMEMBER: {
			RemoveMember( msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) ) );
			break;
		}
		case FM_SETPRIVATE: {
			SetPrivate( msg.ReadBool() );
			break;
		}
		case FM_SETNAME: {
			char buffer[ 32 ];
			msg.ReadString( buffer, sizeof( buffer ) );
			SetName( buffer );
			break;
		}
	}
}

class sdFireTeam_InviteFinaliser : public sdVoteFinalizer {
public:
	sdFireTeam_InviteFinaliser( idPlayer* player, sdFireTeam* fireTeam ) : _player( player ), _fireTeam( fireTeam ) {
	}

	virtual ~sdFireTeam_InviteFinaliser( void ) {
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( passed ) {
			idPlayer* player = _player;
			if ( player ) {
				sdTeamInfo* team = player->GetTeam();
				if ( team ) {
					_fireTeam->SetLastInvite( player->entityNumber, gameLocal.time );
					_fireTeam->AddMember( player->entityNumber );
				} else {
					assert( false );
				}
			}
		}
	}

	const sdFireTeam*		GetFireTeam() const { return _fireTeam; }

private:
	sdFireTeam*				_fireTeam;
	idEntityPtr< idPlayer >	_player;
};

/*
================
sdFireTeam::PromoteMember
================
*/
void sdFireTeam::PromoteMember( int clientNum ) {
	idPlayer* player = gameLocal.GetClient( clientNum );
	// client might not have picked up the spawning player before promote is called
	if ( player != NULL && player->userInfo.isBot ) {
		return;
	}

	int index = _members.FindIndex( clientNum );
	if ( index == -1 ) {
		return;
	}

	idPlayer* oldLeader = GetCommander();

	_members.RemoveIndex( index );
	_members.Insert( clientNum );

	idPlayer* newLeader = GetCommander();

	if ( newLeader != NULL && oldLeader != NULL && newLeader != oldLeader ) {
		newLeader->OnBecomeFireTeamLeader();

		// swap the fireteam task to the new leader so we don't lose it
		newLeader->SetActiveTask( oldLeader->GetActiveTaskHandle() );
		oldLeader->SetActiveTask( -1 );

		// reset throttling of invites since we have a new leader
		memset( _lastInvite, 0, sizeof( _lastInvite ) );
	}

	if ( gameLocal.isServer ) {
		sdFireTeamMessage msg( this, FM_PROMOTEMEMBER );
		msg.WriteBits( clientNum, idMath::BitsForInteger( MAX_CLIENTS ) );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdFireTeam::Invite
================
*/
void sdFireTeam::Invite( idPlayer* other ) {
	assert( other );
	assert( _members.Num() > 0 );

	// HACK: Auto-add bots
	idBot* botOther = other->Cast< idBot >();
	if ( botOther != NULL ) {
		AddMember( botOther->entityNumber );
		return;
	}

	idPlayer* leader = gameLocal.GetClient( _members[ 0 ] );
	assert( leader );

	sdFireTeam* otherFireTeam = gameLocal.rules->GetPlayerFireTeam( other->entityNumber );
	if ( otherFireTeam ) {
		// TODO: PRINT
		return;
	}

	sdTeamInfo* playerTeam = leader->GetGameTeam();
	assert( playerTeam );
	sdTeamInfo* otherTeam = other->GetGameTeam();

	if ( *playerTeam != *otherTeam ) {
		// TODO: PRINT
		return;
	}

	if ( GetNumMembers() >= MaxMembers() ) {
		// TODO: PRINT
		return;
	}

	sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
	vote->DisableFinishMessage();
	vote->MakePrivateVote( other );
	vote->Tag( VI_FIRETEAM_INVITE, other );
	vote->SetText( gameLocal.declToolTipType[ "fireteam_invite" ] );
	vote->AddTextParm( va( L"%hs", leader->userInfo.name.c_str() ) );
	vote->SetFinalizer( new sdFireTeam_InviteFinaliser( other, this ) );
	vote->Start();
}

class sdFireTeam_ProposeFinaliser : public sdVoteFinalizer {
public:
	sdFireTeam_ProposeFinaliser( idPlayer* player, sdFireTeam* fireTeam, idPlayer* leader ) : _player( player ), _fireTeam( fireTeam ), _leader( leader ) {
	}

	virtual ~sdFireTeam_ProposeFinaliser( void ) {
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( !passed ) {
			return;
		}

		idPlayer* player = _player;
		if ( !player ) {
			return;
		}

		idPlayer* leader = _leader;
		if ( !leader ) {
			return;
		}

		if ( _fireTeam->GetNumMembers() == 0 ) {
			return;
		}

		if ( _fireTeam->GetCommander() != leader ) {
			return;
		}

		_fireTeam->Invite( player );
	}

private:
	sdFireTeam*				_fireTeam;
	idEntityPtr< idPlayer >	_player;
	idEntityPtr< idPlayer >	_leader;	
};

/*
================
sdFireTeam::Propose
================
*/
void sdFireTeam::Propose( idPlayer* other, idPlayer* member ) {
	assert( other && member );
	assert( _members.Num() > 0 );

	idPlayer* leader = GetCommander();
	assert( leader );
	
	if ( leader == member ) {
		Invite( other );
		return;
	}

	sdFireTeam* otherFireTeam = gameLocal.rules->GetPlayerFireTeam( other->entityNumber );
	if ( otherFireTeam != NULL ) {
		// TODO: PRINT
		return;
	}

	sdTeamInfo* playerTeam = leader->GetGameTeam();
	assert( playerTeam );
	sdTeamInfo* otherTeam = other->GetGameTeam();

	if ( *playerTeam != *otherTeam ) {
		// TODO: PRINT
		return;
	}

	if ( GetNumMembers() >= MaxMembers() ) {
		// TODO: PRINT
		return;
	}

	sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
	vote->DisableFinishMessage();
	vote->MakePrivateVote( leader );
	vote->Tag( VI_FIRETEAM_PROPOSE, other );
	vote->SetText( gameLocal.declToolTipType[ "fireteam_propose" ] );
	vote->AddTextParm( va( L"%hs", member->userInfo.name.c_str() ) );
	vote->AddTextParm( va( L"%hs", other->userInfo.name.c_str() ) );
	vote->SetFinalizer( new sdFireTeam_ProposeFinaliser( other, this, leader ) );
	vote->Start( member );
}


class sdFireTeam_RequestFinaliser : public sdVoteFinalizer {
public:
	sdFireTeam_RequestFinaliser( idPlayer* player, sdFireTeam* fireTeam, idPlayer* leader ) : 
									_player( player ), _fireTeam( fireTeam ), _leader( leader ) {
	}

	virtual ~sdFireTeam_RequestFinaliser( void ) {
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( !passed ) {
			return;
		}

		idPlayer* player = _player;
		if ( !player ) {
			return;
		}

		idPlayer* leader = _leader;
		if ( !leader ) {
			return;
		}

		if ( _fireTeam->GetNumMembers() == 0 ) {
			return;
		}

		if ( _fireTeam->GetCommander() != leader ) {
			return;
		}

		_fireTeam->AddMember( player->entityNumber );
	}


	const sdFireTeam*		GetFireTeam() const { return _fireTeam; }

private:
	sdFireTeam*				_fireTeam;
	idEntityPtr< idPlayer >	_player;
	idEntityPtr< idPlayer >	_leader;
};


/*
================
sdFireTeam::Request
================
*/
void sdFireTeam::Request( idPlayer* other ) {
	assert( other );
	assert( _members.Num() > 0 );

	idPlayer* leader = gameLocal.GetClient( _members[ 0 ] );
	assert( leader );
	
	sdFireTeam* otherFireTeam = gameLocal.rules->GetPlayerFireTeam( other->entityNumber );
	if ( otherFireTeam ) {
		// TODO: PRINT
		return;
	}

	sdTeamInfo* playerTeam = leader->GetGameTeam();
	assert( playerTeam );
	sdTeamInfo* otherTeam = other->GetGameTeam();

	if ( *playerTeam != *otherTeam ) {
		// TODO: PRINT
		return;
	}

	sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
	vote->DisableFinishMessage();
	vote->MakePrivateVote( leader );
	vote->Tag( VI_FIRETEAM_REQUEST, other );
	vote->SetText( gameLocal.declToolTipType[ "fireteam_request" ] );
	vote->AddTextParm( va( L"%hs", other->userInfo.name.c_str() ) );
	vote->SetFinalizer( new sdFireTeam_RequestFinaliser( other, this, leader ) );
	vote->Start();
}

/*
================
sdFireTeam::Disband
================
*/
void sdFireTeam::Disband( void ) {
	while ( _members.Num() ) {
		if ( _members.Num() > 1 ) {
			RemoveMember( _members[ 1 ] );
		} else {
			// remove team leader last
			RemoveMember( _members[ 0 ] );
		}
	}

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer *player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		// cancel invite
		sdPlayerVote *vote = sdVoteManager::GetInstance().FindVote( VI_FIRETEAM_INVITE, player );
		if ( vote == NULL ) {
			continue;
		}

		sdFireTeam_InviteFinaliser *invite = static_cast<sdFireTeam_InviteFinaliser*>( vote->GetFinalizer() );
		if ( invite->GetFireTeam()->GetHandle() == _index ) {
			sdVoteManager::GetInstance().CancelVote( VI_FIRETEAM_INVITE, player );
		}

		// cancel request
		vote = sdVoteManager::GetInstance().FindVote( VI_FIRETEAM_REQUEST, player );
		if ( vote == NULL ) {
			continue;
		}

		sdFireTeam_RequestFinaliser *request = static_cast<sdFireTeam_RequestFinaliser*>( vote->GetFinalizer() );
		if ( request->GetFireTeam()->GetHandle() == _index ) {
			sdVoteManager::GetInstance().CancelVote( VI_FIRETEAM_REQUEST, player );
		}
	}
}

/*
================
sdFireTeam::SendDisbandMessage
================
*/
void sdFireTeam::SendDisbandMessage( void ) {
	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_FIRETEAM_DISBAND );

	for ( int i = 1; i < _members.Num(); i++ ) {
		idPlayer* player = GetMember( i );
		if ( player == NULL ) {
			continue;
		}
		if ( gameLocal.IsLocalPlayer( player ) ) {
			player->OnFireTeamDisbanded();
		} else {
			msg.Send( sdReliableMessageClientInfo( player->entityNumber ) );
		}
	}
}

/*
================
sdFireTeam::IsMember
================
*/
bool sdFireTeam::IsMember( idPlayer* other ) const {
	for ( int i = 0; i < _members.Num(); i++ ) {
		if ( _members[ i ] == other->entityNumber ) {
			return true;
		}
	}
	return false;
}


/*
===============================================================================

	sdFireTeamManagerLocal

===============================================================================
*/

/*
================
sdFireTeamManagerLocal::sdFireTeamManagerLocal
================
*/
sdFireTeamManagerLocal::sdFireTeamManagerLocal( void ) {
}

/*
================
sdFireTeamManagerLocal::~sdFireTeamManagerLocal
================
*/
sdFireTeamManagerLocal::~sdFireTeamManagerLocal( void ) {
	_commands.DeleteContents( true );
}

/*
================
sdFireTeamManagerLocal::Init
================
*/
void sdFireTeamManagerLocal::Clear( void ) {
	for ( int i = 0; i < _fireTeams.Num(); i++ ) {
		_fireTeams[ i ]->Clear();
	}
}

/*
================
sdFireTeamManagerLocal::Init
================
*/
void sdFireTeamManagerLocal::Init( void ) {
	_commands.Alloc() = new sdFireTeamSystemCommand_Join();
	_commands.Alloc() = new sdFireTeamSystemCommand_Leave();
	_commands.Alloc() = new sdFireTeamSystemCommand_Create();
	_commands.Alloc() = new sdFireTeamSystemCommand_Kick();
	_commands.Alloc() = new sdFireTeamSystemCommand_Propose();
	_commands.Alloc() = new sdFireTeamSystemCommand_Invite();
	_commands.Alloc() = new sdFireTeamSystemCommand_Disband();
	_commands.Alloc() = new sdFireTeamSystemCommand_Promote();
	_commands.Alloc() = new sdFireTeamSystemCommand_Name();
	_commands.Alloc() = new sdFireTeamSystemCommand_Private();
	_commands.Alloc() = new sdFireTeamSystemCommand_Public();

	int count = sdTeamManager::GetInstance().GetNumTeams();
	for ( int i = 0; i < count; i++ ) {
		sdTeamInfo& team = sdTeamManager::GetInstance().GetTeamByIndex( i );
		for ( int j = 0; j < sdTeamInfo::MAX_FIRETEAMS; j++ ) {
			sdFireTeam& fireTeam = team.GetFireTeam( j );
			fireTeam.Init( _fireTeams.Num() );
			_fireTeams.Alloc() = &fireTeam;
		}
	}

	sdFireTeam::SetFireTeamBits( idMath::BitsForInteger( _fireTeams.Num() ) );
}

/*
================
sdFireTeamManagerLocal::Think
================
*/
void sdFireTeamManagerLocal::Think( void ) {
}

/*
================
sdFireTeamManagerLocal::WriteInitialReliableMessages
================
*/
void sdFireTeamManagerLocal::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const {
	for ( int i = 0; i < _fireTeams.Num(); i++ ) {
		_fireTeams[ i ]->WriteInitialState( target );
	}
}

/*
================
sdFireTeamManagerLocal::Write
================
*/
void sdFireTeamManagerLocal::Write( idFile* file ) const {
	for ( int i = 0; i < _fireTeams.Num(); i++ ) {
		_fireTeams[ i ]->Write( file );
	}
}

/*
================
sdFireTeamManagerLocal::Read
================
*/
void sdFireTeamManagerLocal::Read( idFile* file ) {
	for ( int i = 0; i < _fireTeams.Num(); i++ ) {
		_fireTeams[ i ]->Read( file );
	}
}

/*
================
sdFireTeamManagerLocal::FireTeamForHandle
================
*/
sdFireTeam* sdFireTeamManagerLocal::FireTeamForHandle( qhandle_t handle ) {
	if ( handle < 0 || handle >= _fireTeams.Num() ) {
		return NULL;
	}

	return _fireTeams[ handle ];
}

/*
================
sdFireTeamManagerLocal::PerformCommand
================
*/
void sdFireTeamManagerLocal::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) {
	assert( idStr::Icmp( "fireteam", cmd.Argv( 0 ) ) == 0 );

	const char* command = cmd.Argv( 1 );
	for ( int i = 0; i < _commands.Num(); i++ ) {
		if ( idStr::Icmp( command, _commands[ i ]->GetName() ) ) {
			continue;
		}

		_commands[ i ]->PerformCommand( cmd, player );
		break;
	}
}

/*
============
sdFireTeamManagerLocal::CommandCompletion
============
*/
void sdFireTeamManagerLocal::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) {
	sdFireTeamManagerLocal& ftManager = sdFireTeamManager::GetInstance();

	const char* cmd = args.Argv( 1 );

	for ( int i = 0; i < ftManager._commands.Num(); i++ ) {
		if ( idStr::Icmp( cmd, ftManager._commands[ i ]->GetName() ) ) {
			continue;
		}

		ftManager._commands[ i ]->CommandCompletion( args, callback );
	}

	int cmdLength = idStr::Length( cmd );
	for ( int i = 0; i < ftManager._commands.Num(); i++ ) {
		if ( idStr::Icmpn( cmd, ftManager._commands[ i ]->GetName(), cmdLength ) ) {
			continue;
		}

		callback( va( "%s %s", args.Argv( 0 ), ftManager._commands[ i ]->GetName() ) );
	}
}
