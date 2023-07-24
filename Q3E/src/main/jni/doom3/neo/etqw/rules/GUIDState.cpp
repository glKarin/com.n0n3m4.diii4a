// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GUIDState.h"
#include "UserGroup.h"
#include "../Player.h"
#include "../../idlib/PropertiesImpl.h"
#include "GameRules.h"

/*
===============================================================================

	sdGUIDInfo

===============================================================================
*/

/*
============
sdGUIDInfo::sdGUIDInfo
============
*/
sdGUIDInfo::sdGUIDInfo( void ) {
	matchType	= MT_INVALID;
	banTime		= 0;
}

/*
============
sdGUIDInfo::CurrentTime
============
*/
time_t sdGUIDInfo::CurrentTime( void ) {
	sysTime_t blankTime;
	memset( &blankTime, 0, sizeof( blankTime ) );

	sysTime_t currentTime;
	sys->RealTime( &currentTime );

	return sys->TimeDiff( blankTime, currentTime );
}

/*
============
sdGUIDInfo::IsBanned
============
*/
bool sdGUIDInfo::IsBanned( void ) {
	if ( banTime == 0 ) {
		return false;
	}
	if ( banTime < 0 ) {
		return true;
	}

	return banTime > CurrentTime();
}

/*
============
sdGUIDInfo::IsFinished
============
*/
bool sdGUIDInfo::IsFinished( void ) {
	if ( IsBanned() ) {
		return false;
	}

	if ( authGroup.Length() > 0 ) {
		return false;
	}

	return true;
}

/*
============
sdGUIDInfo::GetPrintableName
============
*/
void sdGUIDInfo::GetPrintableName( idStr& name ) const {
	if ( userName.Length() > 0 ) {
		name = va( "'%s'", userName.c_str() );
	} else {
		name = "'no name'";
	}

	switch ( matchType ) {
		case MT_GUID:
			name += va( " GUID: %u.%u", id.clientId.id[ 0 ], id.clientId.id[ 1 ] );
			return;
		case MT_IP: {
			byte* temp = ( ( byte* )&id.ip );
			name += va( " IP: %u.%u.%u.%u", temp[ 0 ], temp[ 1 ], temp[ 2 ], temp[ 3 ] );
			return;
		}
		case MT_PB:
			name += va( " PB GUID: %i", id.pbid );
			return;
	}

	assert( false );
	name += " UNKNOWN";
}

/*
============
sdGUIDInfo::Ban
============
*/
void sdGUIDInfo::Ban( int length ) {
	if ( length < 0 ) {
		banTime = -1;
		return;
	}

	banTime = CurrentTime() + length;
}

/*
============
sdGUIDInfo::Match
============
*/
bool sdGUIDInfo::Match( const clientGUIDLookup_t& lookup ) {
	switch ( matchType ) {
		case MT_GUID:
			return id.clientId == lookup.clientId;
		case MT_IP:
			return id.ip == lookup.ip;
		case MT_PB:
			return id.pbid == lookup.pbid;
	}

	assert( false );
	return false;
}

/*
============
sdGUIDInfo::Write
============
*/
void sdGUIDInfo::Write( idFile* file ) const {
	file->Printf( "entry {\n" );

	switch ( matchType ) {
		case MT_GUID:
			file->Printf( "\t\"guid\"\t\"%u.%u\"\n", id.clientId.id[ 0 ], id.clientId.id[ 1 ] );
			break;
		case MT_IP: {
			byte* temp = ( ( byte* )&id.ip );
			file->Printf( "\t\"ip\"\t\"%u.%u.%u.%u\"\n", temp[ 0 ], temp[ 1 ], temp[ 2 ], temp[ 3 ] );
			break;
		}
		case MT_PB:
			file->Printf( "\t\"pbid\"\t\"%i\"\n", id.pbid );
			break;
	}

	if ( authGroup.Length() > 0 ) {
		file->Printf( "\t\"auth_group\"\t\"%s\"\n", authGroup.c_str() );
	}

	if ( userName.Length() > 0 ) {
		file->Printf( "\t\"user_name\"\t\"%s\"\n", userName.c_str() );
	}

	if ( banTime < 0 ) {
		file->Printf( "\t\"ban_time\"\t\"forever\"\n" );
	} else if ( banTime != 0 ) {
		file->Printf( "\t\"ban_time\"\t\"%i\"\n", banTime );
	}

	file->Printf( "}\n\n" );
}

/*
============
sdGUIDInfo::SetMatch
============
*/
void sdGUIDInfo::SetMatch( const clientGUIDLookup_t& lookup ) {
	SetUserName( lookup.name.c_str() );

	if ( lookup.clientId.IsValid() ) {
		SetGUID( lookup.clientId );
		return;
	}

	if ( lookup.pbid != 0 ) {
		SetPBID( lookup.pbid );
		return;
	}

	SetIP( lookup.ip );
}

/*
===============================================================================

	sdGUIDFile

===============================================================================
*/

const char* sdGUIDFile::guidFileName = "guidstates.dat";

/*
============
sdGUIDFile::sdGUIDFile
============
*/
sdGUIDFile::sdGUIDFile( void ) {
	fileTimestamp = -1;
}

/*
============
sdGUIDFile::sdGUIDFile
============
*/
sdGUIDFile::~sdGUIDFile( void ) {
}

/*
============
sdGUIDFile::ClearGUIDStates
============
*/
void sdGUIDFile::ClearGUIDStates( void ) {
	info.Clear();
}

/*
============
sdGUIDFile::CheckForUpdates
============
*/
void sdGUIDFile::CheckForUpdates( bool removeOldEntries ) {
	idFile* file = fileSystem->OpenFileRead( guidFileName );
	if ( !file ) {
		return;
	}

	bool changed = file->Timestamp() != fileTimestamp;
	fileTimestamp = file->Timestamp();
	fileSystem->CloseFile( file );
	if ( !changed ) {
		return;
	}

	ClearGUIDStates();

	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );

	src.LoadFile( guidFileName );

	if ( !src.IsLoaded() ) {
		return;
	}

	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "entry" ) ) {
			if ( !ParseEntry( src ) ) {
				src.Warning( "Error Parsing Entry" );
				break;
			}
		} else {
			src.Warning( "Invalid Token '%s'", token.c_str() );
			break;
		}
	}

	if ( removeOldEntries ) {
		RemoveOldEntries();
	}
}

/*
============
sdGUIDFile::IPForString
============
*/
bool sdGUIDFile::IPForString( const char* text, int& ip ) {
	ip = 0;

	int temp[ 4 ];
	if ( sscanf( text, "%u.%u.%u.%u", &temp[ 0 ], &temp[ 1 ], &temp[ 2 ], &temp[ 3 ] ) != 4 ) {
		return false;
	}

	byte iptemp[ 4 ];
	for ( int i = 0; i < 4; i++ ) {
		if ( temp[ i ] < 0 || temp[ i ] > 255 ) {
			return false;
		}
		iptemp[ i ] = temp[ i ];
	}

	ip = *( ( int* )iptemp );

	return true;
}

/*
============
sdGUIDFile::PBGUIDForString
============
*/
bool sdGUIDFile::PBGUIDForString( const char* text, int& pbguid ) {
	return sscanf( text, "%i", &pbguid ) == 1;
}

/*
============
sdGUIDFile::GUIDForString
============
*/
bool sdGUIDFile::GUIDForString( const char* text, sdNetClientId& id ) {
	if ( sscanf( text, "%u.%u", &id.id[ 0 ], &id.id[ 1 ] ) != 2 ) {
		return false;
	}
	return true;
}

/*
============
sdGUIDFile::WriteGUIDFile
============
*/
void sdGUIDFile::WriteGUIDFile( void ) {
	idFile* file = fileSystem->OpenFileWrite( guidFileName, "fs_userpath" );
	if ( !file ) {
		return;
	}

	for ( int i = 0; i < info.Num(); i++ ) {
		info[ i ].Write( file );
	}

	fileSystem->CloseFile( file );
}

/*
============
sdGUIDFile::RemoveOldEntries
============
*/
void sdGUIDFile::RemoveOldEntries( void ) {
	CheckForUpdates( false );

	bool changed = false;
	for ( int i = 0; i < info.Num(); ) {
		if ( info[ i ].IsFinished() ) {
			changed = true;
			info.RemoveIndexFast( i );
		} else {
			i++;
		}
	}

	if ( changed ) {
		WriteGUIDFile();
	}
}

/*
============
sdGUIDFile::ParseEntry
============
*/
bool sdGUIDFile::ParseEntry( idLexer& src ) {
	// parse entry
	sdGUIDInfo newInfo;

	idDict data;
	if ( !data.Parse( src ) ) {
		return false;
	}

	idStr tempString;
	if ( data.GetString( "ip", "", tempString ) ) {
		int ip;
		if ( !IPForString( tempString.c_str(), ip ) ) {
			return false;
		}
		newInfo.SetIP( ip );
	} else if ( data.GetString( "pbid", "", tempString ) ) {
		int pbguid;
		if ( !PBGUIDForString( tempString.c_str(), pbguid ) ) {
			return false;
		}
		newInfo.SetPBID( pbguid );
	} else if ( data.GetString( "guid", "", tempString ) ) {
		sdNetClientId id;
		if ( !GUIDForString( tempString.c_str(), id ) ) {
			return false;
		}
		newInfo.SetGUID( id );
	}

	newInfo.SetAuthGroup( data.GetString( "auth_group" ) );

	const char* banText = data.GetString( "ban_time" );
	if ( *banText != '\0' ) {
		if ( !idStr::Icmp( banText, "forever" ) ) {
			newInfo.BanForever();
		} else {
			int t;
			sdProperties::sdFromString( t, banText );
			newInfo.SetBanTime( t );
		}
	}

	const char* userName = data.GetString( "user_name" );
	if ( *userName != '\0' ) {
		newInfo.SetUserName( userName );
	}

	if ( newInfo.GetMatchType() == sdGUIDInfo::MT_INVALID ) {
		src.Warning( "No match type specified" );
		return true;
	}

	info.Alloc() = newInfo;

	return true;
}

/*
============
sdGUIDFile::BanUser
============
*/
void sdGUIDFile::BanUser( const clientGUIDLookup_t& lookup, int length ) {
	CheckForUpdates();

	int index = -1;

	for ( int i = 0; i < info.Num(); i++ ) {
		sdGUIDInfo& gInfo = info[ i ];
		if ( !gInfo.Match( lookup ) ) {
			continue;
		}

		index = i;
		break;
	}

	if ( index == -1 ) {
		index = info.Num();
		info.Alloc().SetMatch( lookup );
	}

	sdGUIDInfo& gInfo = info[ index ];
	gInfo.Ban( length );

	WriteGUIDFile();
}

/*
============
sdGUIDFile::AuthUser
============
*/
void sdGUIDFile::AuthUser( int clientNum, const clientGUIDLookup_t& lookup ) {
	CheckForUpdates();

	for ( int i = 0; i < info.Num(); i++ ) {
		sdGUIDInfo& gInfo = info[ i ];
		if ( !gInfo.Match( lookup ) ) {
			continue;
		}

		const char* authGroup = gInfo.GetAuthGroup();
		if ( !*authGroup ) {
			continue;
		}

		int group = sdUserGroupManager::GetInstance().FindGroup( authGroup );
		if ( group == -1 ) {
			continue;
		}

		gameLocal.rules->SetPlayerUserGroup( clientNum, group );
		return;
	}

	gameLocal.rules->SetPlayerUserGroup( clientNum, -1 );
}

/*
============
sdGUIDFile::AuthUser
============
*/
void sdGUIDFile::SetAutoAuth( const clientGUIDLookup_t& lookup, const char* group ) {
	CheckForUpdates();

	int index = -1;
	for ( int i = 0; i < info.Num(); i++ ) {
		sdGUIDInfo& gInfo = info[ i ];
		if ( !gInfo.Match( lookup ) ) {
			continue;
		}

		index = i;
		break;
	}

	if ( index == -1 ) {
		index = info.Num();
		info.Alloc().SetMatch( lookup );
	}

	sdGUIDInfo& gInfo = info[ index ];
	gInfo.SetAuthGroup( group );

	WriteGUIDFile();
}

/*
============
sdGUIDFile::CheckForBan
============
*/
sdGUIDFile::banState_t sdGUIDFile::CheckForBan( const clientGUIDLookup_t& lookup ) {
	CheckForUpdates();

	for ( int i = 0; i < info.Num(); i++ ) {
		sdGUIDInfo& gInfo = info[ i ];
		if ( !gInfo.Match( lookup ) ) {
			continue;
		}

		if ( !gInfo.IsBanned() ) {
			continue;
		}

		if ( gInfo.GetBanEndTime() < 0 ) {
			return BS_PERM_BAN;
		}
		return BS_TEMP_BAN;
	}
	return BS_NOT_BANNED;
}

/*
============
sdGUIDFile::ListBans
============
*/
void sdGUIDFile::ListBans( void ) {
	CheckForUpdates();

	for ( int i = 0; i < info.Num(); i++ ) {
		sdGUIDInfo& gInfo = info[ i ];
		if ( !gInfo.IsBanned() ) {
			continue;
		}

		idStr name;
		gInfo.GetPrintableName( name );
		gameLocal.Printf( "%d: %s\n", i, name.c_str() );
	}
}

/*
============
sdGUIDFile::ListBans
============
*/
void sdGUIDFile::ListBans( const idBitMsg& msg ) {
	while ( msg.GetSize() != msg.GetReadCount() ) {
		int index = msg.ReadLong();
		char buffer[ 128 ];
		msg.ReadString( buffer, sizeof( buffer ) );
		gameLocal.Printf( "%d: %s\n", index, buffer );
	}
}

/*
============
sdGUIDFile::WriteBans
============
*/
bool sdGUIDFile::WriteBans( int& startIndex, idBitMsg& msg ) {
	CheckForUpdates();

	const int MAX_WRITE_BANS = 5;
	int count = 0;
	int i = startIndex + 1;
	for ( ; i < info.Num(); i++ ) {
		sdGUIDInfo& gInfo = info[ i ];
		if ( !gInfo.IsBanned() ) {
			continue;
		}

		count++;
		msg.WriteLong( i );
		idStr name;
		gInfo.GetPrintableName( name );
		msg.WriteString( name.c_str() );

		if ( count == MAX_WRITE_BANS ) {
			startIndex = i;
			return false;
		}
	}

	startIndex = -1;
	return true;
}

/*
============
sdGUIDFile::UnBan
============
*/
void sdGUIDFile::UnBan( int index ) {
	if ( index < 0 || index >= info.Num() ) {
		return;
	}

	info[ index ].UnBan();

	WriteGUIDFile();
}
