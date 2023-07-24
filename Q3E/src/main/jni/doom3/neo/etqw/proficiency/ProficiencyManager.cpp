// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ProficiencyManager.h"
#include "../Player.h"
#include "../rules/GameRules.h"
#include "StatsTracker.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../roles/FireTeams.h"
#include "../roles/Tasks.h"
#include "../../idlib/PropertiesImpl.h"

idCVar g_logProficiency( "g_logProficiency", "1", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT | CVAR_RANKLOCKED, "log proficiency data" );

/*
===============================================================================

	sdProficiencyTable::sdNetworkData

===============================================================================
*/

/*
================
sdProficiencyTable::sdNetworkData::sdNetworkData
================
*/
sdProficiencyTable::sdNetworkData::sdNetworkData( void ) {
	int count = gameLocal.declProficiencyTypeType.Num();
	points.SetNum( count );
	basePoints.SetNum( count );
	spawnLevels.SetNum( count );
}

/*
================
sdProficiencyTable::sdNetworkData::MakeDefault
================
*/
void sdProficiencyTable::sdNetworkData::MakeDefault( void ) {
	for ( int i = 0; i < points.Num(); i++ ) {
		points[ i ]			= 0.f;
		basePoints[ i ]		= 0.f;
		spawnLevels[ i ]	= 0;
	}
	fixedRank = false;
	fixedRankIndex = 0;
}

/*
================
sdProficiencyTable::sdNetworkData::Write
================
*/
void sdProficiencyTable::sdNetworkData::Write( idFile* file ) const {
	for ( int i = 0; i < points.Num(); i++ ) {
		file->WriteFloat( points[ i ] );
		file->WriteFloat( basePoints[ i ] );
		file->WriteInt( spawnLevels[ i ] );
	}
	file->WriteBool( fixedRank );
	file->WriteInt( fixedRankIndex );
}

/*
================
sdProficiencyTable::sdNetworkData::Read
================
*/
void sdProficiencyTable::sdNetworkData::Read( idFile* file ) {
	for ( int i = 0; i < points.Num(); i++ ) {
		file->ReadFloat( points[ i ] );
		file->ReadFloat( basePoints[ i ] );
		file->ReadInt( spawnLevels[ i ] );
	}
	file->ReadBool( fixedRank );
	file->ReadInt( fixedRankIndex );
}

/*
===============================================================================

	sdProficiencyTable

===============================================================================
*/

/*
================
sdProficiencyTable::SetProficiency
================
*/
void sdProficiencyTable::SetProficiency( int index, float amount ) {
	if ( points[ index ] == amount ) {
		return;
	}

	points[ index ] = amount;
	UpdateXP();
	UpdateLevel( index );
}

/*
================
sdProficiencyTable::AddProficiency
================
*/
void sdProficiencyTable::AddProficiency( int index, float amount ) {
	if ( gameLocal.isClient ) {
		return;
	}

	points[ index ] += amount;
	UpdateXP();
	UpdateLevel( index );
}

/*
================
sdProficiencyTable::StoreBasePoints
================
*/
void sdProficiencyTable::StoreBasePoints( void ) {
	for ( int i = 0; i < points.Num(); i++ ) {
		basePoints[ i ] = points[ i ];
	}
}

/*
================
sdProficiencyTable::ResetToBasePoints
================
*/
void sdProficiencyTable::ResetToBasePoints( void ) {
	for ( int i = 0; i < points.Num(); i++ ) {
		points[ i ] = basePoints[ i ];
		UpdateLevel( i );
	}
	UpdateXP();
}

/*
================
sdProficiencyTable::SetSpawnLevels
================
*/
void sdProficiencyTable::SetSpawnLevels( void ) {
	spawnLevels = levels;
}

/*
================
sdProficiencyTable::Clear
================
*/
void sdProficiencyTable::Clear( bool all ) {
	int count = gameLocal.declProficiencyTypeType.Num();
	points.AssureSize( count );
	basePoints.AssureSize( count );
	levels.AssureSize( count );
	spawnLevels.AssureSize( count );

	for ( int i = 0; i < count; i++ ) {
		points[ i ]			= 0;
		basePoints[ i ]		= 0;
		levels[ i ]			= 0;
		spawnLevels[ i ]	= 0;
	}

	xp						= 0.0f;
	if ( all || !fixedRank ) {
		rank				= NULL;
		fixedRank			= false;
	}

	UpdateRank();
}

/*
================
sdProficiencyTable::UpdateLevel
================
*/
void sdProficiencyTable::UpdateLevel( int index ) {
	int oldLevel = levels[ index ];

	levels[ index ] = 0;

	if ( gameLocal.serverInfoData.noProficiency ) {
		return;
	}

	const sdDeclProficiencyType* type = gameLocal.declProficiencyTypeType.LocalFindByIndex( index, true );

	float cost = 0;
	for ( int i = 0; i < type->GetNumLevels(); i++ ) {
		int thisCost = type->GetLevel( i );

		cost += thisCost;

		if ( points[ index ] >= cost ) {
			levels[ index ] = i + 1;
		} else {
			break;
		}
	}

	using namespace sdProperties;
	
	if ( levels[ index ] != oldLevel ) {
		idPlayer* player = gameLocal.GetClient( clientNum );
		if ( player == NULL ) {
			assert( false );
		} else {
			player->OnProficiencyLevelGain( type, oldLevel, levels[ index ] );
		}
	} else {
		if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" ) ) {
			if( sdProperty* property = scope->GetProperty( "proficiencyReward", PT_WSTRING ) ) {
				*property->value.wstringValue = L"";
			}
		} else {
			gameLocal.Warning( "sdProficiencyTable::UpdateLevel: Couldn't find global 'gameHud' scope in guiGlobals." );
		}
	}
}

/*
================
sdProficiencyTable::UpdateLevels
================
*/
void sdProficiencyTable::UpdateLevels( void ) {
	for ( int i = 0; i < levels.Num(); i++ ) {
		UpdateLevel( i );
	}
}

/*
================
sdProficiencyTable::UpdateRank
================
*/
void sdProficiencyTable::UpdateRank( void ) {
	if ( fixedRank ) {
		return;
	}

	const sdDeclRank* oldRank = rank;
	rank = NULL;

	int rankLevel = -1;
	int count = gameLocal.declRankType.Num();
	for ( int i = 0; i < count; i++ ) {
		const sdDeclRank* testRank = gameLocal.declRankType[ i ];

		if ( xp >= testRank->GetCost() ) {
			// because the ranks aren't guaranteed to be sorted, we need to make sure we don't set the rank to lower than the current rank
			if ( rank != NULL && testRank->GetCost() < rank->GetCost() ) {
				continue;
			}

			rank = testRank;
			rankLevel = i;
		}
	}

	idPlayer* player = gameLocal.GetClient( clientNum );
	if ( rank != oldRank && player != NULL ) {
		sdScriptHelper h1;
		h1.Push( rankLevel );
		player->scriptObject->CallNonBlockingScriptEvent( player->scriptObject->GetFunction( "OnRankChanged" ), h1 );
	}
}

/*
================
sdProficiencyTable::UpdateXP
================
*/
void sdProficiencyTable::UpdateXP( void ) {
	xp = 0;
	for ( int i = 0; i < levels.Num(); i++ ) {
		xp += points[ i ];
	}
	UpdateRank();
}

/*
================
sdProficiencyTable::sdProficiencyTable
================
*/
sdProficiencyTable::sdProficiencyTable( void ) {
	clientNum = -1;
	fixedRank = false;
	rank = NULL;
}

/*
================
sdProficiencyTable::~sdProficiencyTable
================
*/
sdProficiencyTable::~sdProficiencyTable( void ) {
}

/*
================
sdProficiencyTable::ApplyNetworkState
================
*/
void sdProficiencyTable::ApplyNetworkState( const sdNetworkData& newData ) {
	idPlayer* player = gameLocal.GetClient( clientNum );
	assert( player );

	for ( int i = 0; i < points.Num(); i++ ) {
		if ( newData.points[ i ] > points[ i ] ) {
			player->OnProficiencyGain( i, newData.points[ i ] - points[ i ], NULL );
		}
		SetProficiency( i, newData.points[ i ] );
		basePoints[ i ] = newData.basePoints[ i ];
		spawnLevels[ i ] = newData.spawnLevels[ i ];
	}

	if ( newData.fixedRank ) {
		SetFixedRank( gameLocal.declRankType.SafeIndex( newData.fixedRankIndex ) );
	} else {
		if ( fixedRank ) {
			fixedRank = false;
			UpdateRank();
		}
	}
}

/*
================
sdProficiencyTable::ReadNetworkState
================
*/
void sdProficiencyTable::ReadNetworkState( const sdNetworkData& baseData, sdNetworkData& newData, const idBitMsg& msg ) const {
	if ( msg.ReadBool() ) {
		for ( int i = 0; i < points.Num(); i++ ) {
			newData.points[ i ] = msg.ReadDeltaFloat( baseData.points[ i ] );
		}
	} else {
		for ( int i = 0; i < points.Num(); i++ ) {
			newData.points[ i ] = baseData.points[ i ];
		}
	}

	if ( msg.ReadBool() ) {
		for ( int i = 0; i < basePoints.Num(); i++ ) {
			newData.basePoints[ i ] = msg.ReadDeltaFloat( baseData.basePoints[ i ] );
		}
	} else {
		for ( int i = 0; i < basePoints.Num(); i++ ) {
			newData.basePoints[ i ] = baseData.basePoints[ i ];
		}
	}

	if ( msg.ReadBool() ) {
		for ( int i = 0; i < points.Num(); i++ ) {
			newData.spawnLevels[ i ] = msg.ReadDeltaLong( baseData.spawnLevels[ i ] );
		}
	} else {
		for ( int i = 0; i < points.Num(); i++ ) {
			newData.spawnLevels[ i ] = baseData.spawnLevels[ i ];
		}
	}

	if ( !baseData.fixedRank ) {
		newData.fixedRank = msg.ReadBool();
		if ( newData.fixedRank ) {
			newData.fixedRankIndex = msg.ReadLong();
		} else {
			newData.fixedRankIndex = -1;
		}
	} else {
		newData.fixedRank = true;
		newData.fixedRankIndex = baseData.fixedRankIndex;
	}
}

/*
================
sdProficiencyTable::WriteNetworkState
================
*/
void sdProficiencyTable::WriteNetworkState( const sdNetworkData& baseData, sdNetworkData& newData, idBitMsg& msg ) const {
	bool profChanged		= false;
	bool baseProfChanged	= false;
	bool spawnLevelChanged	= false;

	for ( int i = 0; i < points.Num(); i++ ) {
		newData.points[ i ] = points[ i ];
		profChanged |= newData.points[ i ] != baseData.points[ i ];
		newData.basePoints[ i ] = basePoints[ i ];
		baseProfChanged |= newData.basePoints[ i ] != baseData.basePoints[ i ];
		newData.spawnLevels[ i ] = spawnLevels[ i ];
		spawnLevelChanged |= newData.spawnLevels[ i ] != baseData.spawnLevels[ i ];
	}

	msg.WriteBool( profChanged );
	if ( profChanged ) {
		for ( int i = 0; i < points.Num(); i++ ) {
			msg.WriteDeltaFloat( baseData.points[ i ], newData.points[ i ] );
		}
	}
	msg.WriteBool( baseProfChanged );
	if ( baseProfChanged ) {
		for ( int i = 0; i < basePoints.Num(); i++ ) {
			msg.WriteDeltaFloat( baseData.basePoints[ i ], newData.basePoints[ i ] );
		}
	}
	msg.WriteBool( spawnLevelChanged );
	if ( spawnLevelChanged ) {
		for ( int i = 0; i < points.Num(); i++ ) {
			msg.WriteDeltaLong( baseData.spawnLevels[ i ], newData.spawnLevels[ i ] );
		}
	}

	if ( !baseData.fixedRank ) {
		newData.fixedRank = fixedRank;
		if ( fixedRank ) {
			msg.WriteBool( true );
			newData.fixedRankIndex = rank == NULL ? -1 : rank->Index();
			msg.WriteLong( newData.fixedRankIndex );
		} else {
			msg.WriteBool( false );
			newData.fixedRankIndex = -1;
		}
	} else {
		newData.fixedRank = true;
		newData.fixedRankIndex = baseData.fixedRankIndex;
	}
}

/*
============
sdProficiencyTable::CheckNetworkStateChanges
============
*/
bool sdProficiencyTable::CheckNetworkStateChanges( const sdNetworkData& baseData ) const {
	for ( int i = 0; i < points.Num(); i++ ) {
		NET_CHECK_FIELD( points[ i ], points[ i ] );
		NET_CHECK_FIELD( basePoints[ i ], basePoints[ i ] );
		NET_CHECK_FIELD( spawnLevels[ i ], spawnLevels[ i ] );
	}

	NET_CHECK_FIELD( fixedRank, fixedRank );

	return false;
}

/*
============
sdProficiencyTable::GetPercent
============
*/
float sdProficiencyTable::GetPercent( int profIndex ) const {
	const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType[ profIndex ];

	float currentCost = 0.0f;
	int maxLevel = Min( GetLevel( profIndex ) + 1, prof->GetNumLevels() );
	int levelIndex;

	for ( levelIndex = 0; levelIndex < maxLevel; levelIndex++ ) {
		currentCost += idMath::Ftoi( prof->GetLevel( levelIndex ) );
	}

	float baseCost = 0.0f;
	for ( levelIndex = 0; levelIndex < GetLevel( profIndex ); levelIndex++ ) {
		baseCost += idMath::Ftoi( prof->GetLevel( levelIndex ) );
	}

	if ( currentCost > 0.f && currentCost > baseCost ) {
		return idMath::ClampFloat( 0.f, 1.f, ( GetPoints( profIndex ) - baseCost ) / ( currentCost - baseCost ) );
	}
	return 0.f;
}

/*
===============================================================================

	sdProficiencyManagerLocal

===============================================================================
*/

/*
================
sdProficiencyManagerLocal::sdProficiencyManagerLocal
================
*/
sdProficiencyManagerLocal::sdProficiencyManagerLocal( void ) {
}

/*
================
sdProficiencyManagerLocal::~sdProficiencyManagerLocal
================
*/
sdProficiencyManagerLocal::~sdProficiencyManagerLocal( void ) {
	Shutdown();
}

/*
================
sdProficiencyManagerLocal::GiveProficiency
================
*/
void sdProficiencyManagerLocal::GiveProficiency( int index, float count, idPlayer* player, float scale, const char* reason ) {
	if ( gameLocal.rules->GetState() != sdGameRules::GS_GAMEON ) {
		return;
	}

	sdProficiencyTable& proficiencyTable = player->GetProficiencyTable();
	
	count *= scale;

	player->OnProficiencyGain( index, count, reason );
	proficiencyTable.AddProficiency( index, count );
}

/*
================
sdProficiencyManagerLocal::GiveMissionProficiency
================
*/
void sdProficiencyManagerLocal::GiveMissionProficiency( sdPlayerTask* task, float count ) {
	assert( task != NULL );

	if ( count <= 0.f ) {
		return;
	}

	if ( !task->IsMission() ) {
		return;
	}

	idStaticList< idPlayer*, MAX_CLIENTS > playerList;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		sdFireTeam* ft = gameLocal.rules->GetPlayerFireTeam( i );
		if ( ft != NULL ) {
			idPlayer* commander = ft->GetCommander();
			if ( commander->GetActiveTaskHandle() == task->GetHandle() ) {
				*playerList.Alloc() = player;
			}
		} else {
			if ( player->GetActiveTaskHandle() == task->GetHandle() ) {
				*playerList.Alloc() = player;
			}
		}
	}

	if ( playerList.Num() == 0 ) {
		return;
	}

	sdPlayerStatEntry* stat = sdGlobalStatsTracker::GetInstance().GetStat( sdGlobalStatsTracker::GetInstance().AllocStat( "mission_bonus_given", sdNetStatKeyValue::SVT_INT ) );

	float split = 1 / ( float )playerList.Num();
	for ( int i = 0; i < playerList.Num(); i++ ) {
		idPlayer* player = playerList[ i ];

		stat->IncreaseValue( player->entityNumber, 1 );

		const sdDeclPlayerClass* pc = player->GetInventory().GetClass();
		if ( pc == NULL ) {
			continue;
		}

		if ( pc->GetNumProficiencies() < 1 ) {
			continue;
		}

		LogProficiency( "mission bonus", count * split );
		GiveProficiency( pc->GetProficiency( 0 ).index, count, player, split, "Mission Bonus" );
	}
}

/*
================
sdProficiencyManagerLocal::GiveProficiency
================
*/
void sdProficiencyManagerLocal::GiveProficiency( const sdDeclProficiencyItem* proficiency, idPlayer* player, float scale, sdPlayerTask* task, const char* reason ) {
	if ( proficiency == NULL ) {
		return;
	}

	if ( task != NULL && task->IsMission() ) {
		sdFireTeam* ft = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
		if ( ft != NULL ) {
			// XP Sharing on fireteams
			idStr bonusReason = va( "Fireteam Bonus: %s", reason );
			idPlayer* commander = ft->GetCommander();
			if ( commander->GetActiveTaskHandle() == task->GetHandle() ) {
				for ( int i = 0; i < ft->GetNumMembers(); i++ ) {
					idPlayer* other = ft->GetMember( i );
					if ( other == player ) {
						continue;
					}

					const sdDeclPlayerClass* pc = other->GetInventory().GetClass();
					if ( pc == NULL ) {
						continue;
					}

					if ( pc->GetNumProficiencies() < 1 ) {
						continue;
					}

					float count = proficiency->GetProficiencyCount() * scale;
					LogProficiency( va( "fireteam bonus:%s", proficiency->GetName() ), count );
					GiveProficiency( pc->GetProficiency( 0 ).index, proficiency->GetProficiencyCount(), other, scale, bonusReason.c_str() );
				}
			}
		} else {
			// XP Sharing on mission teams
			idStr bonusReason = va( "Mission Team Bonus: %s", reason );

			taskHandle_t handle = task->GetHandle();
			
			if ( handle == player->GetActiveTaskHandle() ) {
				for ( int i = 0; i < MAX_CLIENTS; i++ ) {
					idPlayer* other = gameLocal.GetClient( i );
					if ( other == NULL || other == player ) {
						continue;
					}

					if ( gameLocal.rules->GetPlayerFireTeam( other->entityNumber ) != NULL ) {
						continue;
					}

					if ( handle != other->GetActiveTaskHandle() ) {
						continue;
					}

					const sdDeclPlayerClass* pc = other->GetInventory().GetClass();
					if ( pc == NULL ) {
						continue;
					}

					if ( pc->GetNumProficiencies() < 1 ) {
						continue;
					}

					float count = proficiency->GetProficiencyCount() * scale;
					LogProficiency( va( "mission team bonus:%s", proficiency->GetName() ), count );
					GiveProficiency( pc->GetProficiency( 0 ).index, proficiency->GetProficiencyCount(), other, scale, bonusReason.c_str() );
				}
			}
		}
	}

	float count = proficiency->GetProficiencyCount() * scale;

	sdPlayerStatEntry* stat = proficiency->GetStat();
	if ( stat ) {
		stat->IncreaseValue( player->entityNumber, count );
	}
	LogProficiency( proficiency->GetName(), count );
	GiveProficiency( proficiency->GetProficiencyType()->Index(), proficiency->GetProficiencyCount(), player, scale, reason );
}

/*
================
sdProficiencyManagerLocal::Init
================
*/
void sdProficiencyManagerLocal::Init( void ) {
}

/*
================
sdProficiencyManagerLocal::Shutdown
================
*/
void sdProficiencyManagerLocal::Shutdown( void ) {
}

/*
================
sdProficiencyManagerLocal::CacheProficiency
================
*/
void sdProficiencyManagerLocal::CacheProficiency( idPlayer* player ) {
	if ( !g_xpSave.GetBool() ) {
		return;
	}

	sdProficiencyTable& table = gameLocal.GetProficiencyTable( player->entityNumber );

	cachedProficiency_t* proficiencyTable = FindCachedProficiency( player );
	if ( proficiencyTable == NULL ) {
		proficiencyTable = &FindFreeCacheSlot();

		if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
			networkSystem->ServerGetClientNetId( player->entityNumber, proficiencyTable->clientId );
		}
		if ( !proficiencyTable->clientId.IsValid() ) {
			clientNetworkAddress_t netInfo;
			networkSystem->ServerGetClientNetworkInfo( player->entityNumber, netInfo );
			proficiencyTable->ip = *( ( int* )netInfo.ip );
		}
	}

	proficiencyTable->cachedTime = gameLocal.time;
	proficiencyTable->table = table;
}

/*
================
sdProficiencyManagerLocal::RestoreProficiency
================
*/
void sdProficiencyManagerLocal::RestoreProficiency( idPlayer* player ) {
	if ( !g_xpSave.GetBool() ) {
		return;
	}

	cachedProficiency_t* proficiencyTable = FindCachedProficiency( player );
	if ( proficiencyTable == NULL ) {
		return;
	}

	sdProficiencyTable& table = gameLocal.GetProficiencyTable( player->entityNumber );
	table = proficiencyTable->table;
	table.Init( player->entityNumber ); // Gordon: the assignment will overwrite the internal clientnum which may have changed

	RemoveCacheEntry( proficiencyTable );
}

/*
================
sdProficiencyManagerLocal::FindFreeCacheSlot
================
*/
sdProficiencyManagerLocal::cachedProficiency_t& sdProficiencyManagerLocal::FindFreeCacheSlot( void ) {
	cachedProficiency_t* table = cachedTables.Alloc();
	if ( table == NULL ) {
		table = &cachedTables[ 0 ];
		int oldest = table->cachedTime;
		for ( int i = 1; i < cachedTables.Num(); i++ ) {
			if ( cachedTables[ i ].cachedTime < oldest ) {
				table = &cachedTables[ i ];
				oldest = table->cachedTime;
			}
		}
	}

	return *table;
}

/*
================
sdProficiencyManagerLocal::FindCachedProficiency
================
*/
sdProficiencyManagerLocal::cachedProficiency_t* sdProficiencyManagerLocal::FindCachedProficiency( idPlayer* player ) {
	if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
		sdNetClientId netClientId;
		networkSystem->ServerGetClientNetId( player->entityNumber, netClientId );

		if ( netClientId.IsValid() ) {
			for ( int i = 0; i < cachedTables.Num(); i++ ) {
				if ( !cachedTables[ i ].clientId.IsValid() ) {
					continue;
				}

				if ( cachedTables[ i ].clientId != netClientId ) {
					continue;
				}

				return &cachedTables[ i ];
			}
		}
	}

	clientNetworkAddress_t netInfo;
	networkSystem->ServerGetClientNetworkInfo( player->entityNumber, netInfo );

	int ip = *( ( int* )netInfo.ip );

	for ( int i = 0; i < cachedTables.Num(); i++ ) {
		if ( cachedTables[ i ].clientId.IsValid() ) {
			continue;
		}

		if ( cachedTables[ i ].ip != ip ) {
			continue;
		}

		return &cachedTables[ i ];
	}

	return NULL;
}

/*
================
sdProficiencyManagerLocal::RemoveCacheEntry
================
*/
void sdProficiencyManagerLocal::RemoveCacheEntry( cachedProficiency_t* entry ) {
	for ( int i = 0; i < cachedTables.Num(); i++ ) {
		if ( entry == &cachedTables[ i ] ) {
			cachedTables.RemoveIndexFast( i );
			return;
		}
	}
}

/*
================
sdProficiencyManagerLocal::ClearProficiency
================
*/
void sdProficiencyManagerLocal::ClearProficiency( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		gameLocal.GetProficiencyTable( i ).Clear( false );
	}
	cachedTables.Clear();
}

/*
================
sdProficiencyManagerLocal::StoreBasePoints
================
*/
void sdProficiencyManagerLocal::StoreBasePoints( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		gameLocal.GetProficiencyTable( i ).StoreBasePoints();
	}
	for ( int i = 0; i < cachedTables.Num(); i++ ) {
		cachedTables[ i ].table.StoreBasePoints();
	}
}

/*
================
sdProficiencyManagerLocal::ResetToBasePoints
================
*/
void sdProficiencyManagerLocal::ResetToBasePoints( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		gameLocal.GetProficiencyTable( i ).ResetToBasePoints();
	}
	for ( int i = 0; i < cachedTables.Num(); i++ ) {
		cachedTables[ i ].table.ResetToBasePoints();
	}
}

/*
================
sdProficiencyManagerLocal::ReadRankInfo
================
*/
bool sdProficiencyManagerLocal::ReadRankInfo( sdPersistentRankInfo& rankInfo ) {
	rankInfo.Clear();

	idParser src( "rankinfo.txt", LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT, false );
	if ( src.IsLoaded() == 0 ) {
		return false;
	}

	if ( !rankInfo.Parse( src ) ) {
		rankInfo.Clear();
		return false;
	}

	return true;
}

/*
================
sdProficiencyManagerLocal::DumpProficiencyData
================
*/
void sdProficiencyManagerLocal::DumpProficiencyData( void ) {
	if ( !g_logProficiency.GetBool() ) {
		return;
	}

	idStr fileName = va( "logs/Proficiency Item Log - %hs - %s.csv", gameLocal.mapMetaData->GetString( "pretty_name" ), gameLocal.GetTimeText() );
	fileName.ReplaceChar( ':', '-' );

	idFile* file = fileSystem->OpenFileWrite( fileName.c_str(), "fs_userpath" );
	if ( file == NULL ) {
		loggedDataList.Clear();
		loggedDataHash.Clear();
		return;
	}

	for ( int i = 0; i < loggedDataList.Num(); i++ ) {
		file->WriteFloatString( "%s,%d,%f\r\n", loggedDataList[ i ].name.c_str(), loggedDataList[ i ].count, loggedDataList[ i ].total );
	}

	fileSystem->CloseFile( file );

	loggedDataList.Clear();
	loggedDataHash.Clear();
}

/*
================
sdProficiencyManagerLocal::LogProficiency
================
*/
void sdProficiencyManagerLocal::LogProficiency( const char* name, float count ) {
	if ( !g_logProficiency.GetBool() ) {
		return;
	}

	int key = loggedDataHash.GenerateKey( name, false );
	int index;
	for ( index = loggedDataHash.GetFirst( key ); index != loggedDataHash.NULL_INDEX; index = loggedDataHash.GetNext( index ) ) {
		if ( idStr::Icmp( name, loggedDataList[ index ].name.c_str() ) == 0 ) {
			break;
		}
	}
	if ( index == loggedDataHash.NULL_INDEX ) {
		index = loggedDataList.Num();

		proficiencyData_t& data = loggedDataList.Alloc();
		data.name = name;
		data.count = 0;
		data.total = 0.f;

		loggedDataHash.Add( key, index );
	}

	proficiencyData_t& data = loggedDataList[ index ];
	data.count++;
	data.total += count;
}

/*
===============================================================================

	sdPersistentRankInfo

===============================================================================
*/

/*
================
sdPersistentRankInfo::Clear
================
*/
void sdPersistentRankInfo::Clear( void ) {
	badges.SetNum( 0, false );
}

/*
================
sdPersistentRankInfo::Parse
================
*/
bool sdPersistentRankInfo::Parse( idParser& src ) {
	idToken token;

	while ( true ) {
		if ( src.ReadToken( &token ) == 0 ) {
			break;
		}

		if ( token.Icmp( "badge" ) == 0 ) {
			if ( !ParseBadge( src ) ) {
				return false;
			}
		} else {
			src.Warning( "Unexpected Token: '%s'", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdPersistentRankInfo::ParseBadge
================
*/
bool sdPersistentRankInfo::ParseBadge( idParser& src ) {
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	sdBadge& badge = badges.Alloc();
	badge.category = "";
	badge.title = "";

	idToken token;

	while ( true ) {
		if ( src.ReadToken( &token ) == 0 ) {
			src.Warning( "Unexpected End of File" );
			return false;
		}

		if ( token.Icmp( "task" ) == 0 ) {
			idDict taskInfo;
			if ( !taskInfo.Parse( src ) ) {
				return false;
			}

			sdBadge::sdTask& task = badge.tasks.Alloc();
			task.Clear();
			task.total = taskInfo.GetFloat( "total" );
			task.text = taskInfo.GetString( "text" );

			const idKeyValue* match = NULL;
			while ( ( match = taskInfo.MatchPrefix( "field", match ) ) != NULL ) {
				task.fields.Alloc() = match->GetValue();
			}
			
		} else if ( token.Icmp( "category" ) == 0 ) {
			if ( src.ReadToken( &token ) == 0 ) {
				return false;
			}

			badge.category = token;
		} else if ( token.Icmp( "title" ) == 0 ) {
			if ( src.ReadToken( &token ) == 0 ) {
				return false;
			}

			badge.title = token;
		} else if ( token.Icmp( "level" ) == 0 ) {
			if ( src.ReadToken( &token ) == 0 ) {
				return false;
			}

			badge.level = token.GetIntValue();
		} else if ( token.Icmp( "alwaysAvailable" ) == 0 ) {
			badge.alwaysAvailable = true;
		}  else if ( token.Icmp( "}" ) == 0 ) {
			break;
		} else {
			src.Warning( "Unexpected Token: '%s'", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdPersistentRankInfo::FindData
================
*/
float sdPersistentRankInfo::FindData( const char* key, const idHashIndex& hash, const sdNetStatKeyValList& list ) {
	int hashkey = hash.GenerateKey( key, false );
	for ( int index = hash.GetFirst( hashkey ); index != -1; index = hash.GetNext( index ) ) {
		if ( idStr::Icmp( list[ index ].key->c_str(), key ) != 0 ) {
			continue;
		}
		
		switch ( list[ index ].type ) {
			case sdNetStatKeyValue::SVT_INT:
				return list[ index ].val.i;
			case sdNetStatKeyValue::SVT_FLOAT:
				return list[ index ].val.f;
			default:
				assert( false );
				break;
		}
	}

	return 0.f;
}

/*
================
sdPersistentRankInfo::CreateData
================
*/
void sdPersistentRankInfo::CreateData( const idHashIndex& hash, const sdNetStatKeyValList& list, sdRankInstance& data ) {
	data.completeTasks = 0;
	data.badges.SetNum( badges.Num(), false );
	for ( int i = 0; i < badges.Num(); i++ ) {
		sdBadge& badge = badges[ i ];

		data.badges[ i ].complete = true;

		data.badges[ i ].taskValues.SetNum( badge.tasks.Num(), false );
		for ( int j = 0; j < badge.tasks.Num(); j++ ) {
			sdBadge::sdTask& task = badge.tasks[ j ];

			float value = 0;
			for ( int k = 0; k < task.fields.Num(); k++ ) {
				value += FindData( task.fields[ k ].c_str(), hash, list );
			}

			data.badges[ i ].taskValues[ j ].value = value;
			data.badges[ i ].taskValues[ j ].max = task.total;
			if ( value < task.total ) {
				data.badges[ i ].complete = false;
			} else {
				data.completeTasks++;
			}
		}
	}
}
