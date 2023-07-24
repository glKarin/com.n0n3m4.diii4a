// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "TeamManager.h"
#include "../structures/DeployRequest.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../rules/VoteManager.h"
#include "../roles/FireTeams.h"

#include "../botai/Bot.h"	//mal: needed for the bots.
#include "../botai/BotThreadData.h"


/*
===============================================================================

	sdTeamManagerLocal

===============================================================================
*/

/*
================
sdTeamManagerGameState::MakeDefault
================
*/
void sdTeamManagerGameState::MakeDefault( void ) {
	teamData.SetNum( sdTeamManager::GetInstance().GetNumTeams() );
	for ( int i = 0; i < teamData.Num(); i++ ) {
		teamData[ i ].MakeDefault();
	}
}

/*
================
sdTeamManagerGameState::Write
================
*/
void sdTeamManagerGameState::Write( idFile* file ) const {
	for ( int i = 0; i < teamData.Num(); i++ ) {
		teamData[ i ].Write( file );
	}
}

/*
================
sdTeamManagerGameState::Read
================
*/
void sdTeamManagerGameState::Read( idFile* file ) {
	teamData.SetNum( sdTeamManager::GetInstance().GetNumTeams() );
	for ( int i = 0; i < teamData.Num(); i++ ) {
		teamData[ i ].Read( file );
	}
}


/*
================
sdTeamManagerLocal::sdTeamManagerLocal
================
*/
sdTeamManagerLocal::sdTeamManagerLocal( void ) {
	stateObject.Init( this );
}

/*
================
sdTeamManagerLocal::~sdTeamManagerLocal
================
*/
sdTeamManagerLocal::~sdTeamManagerLocal( void ) {
	Shutdown();
}

/*
================
sdTeamManagerLocal::Think
================
*/
void sdTeamManagerLocal::Think( void ) {
	int i;
	for( i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->Think();
		// Gordon: FIXME: This could be done better without polling.
		if ( teamInfo[ i ]->GetBotTeam() == GDF ) {
			botThreadData.GetGameWorldState()->gameLocalInfo.nextGDFRespawnTime = ( teamInfo[ i ]->GetTeamRespawnTime() - ( gameLocal.time / 1000 ));
		} else if ( teamInfo[ i ]->GetBotTeam() == STROGG ) {
			botThreadData.GetGameWorldState()->gameLocalInfo.nextStroggRespawnTime = ( teamInfo[ i ]->GetTeamRespawnTime() - ( gameLocal.time / 1000 ));
		}
	}
}

/*
================
sdTeamManagerLocal::Init
================
*/
void sdTeamManagerLocal::Init( void ) {
	for( int i = 0; i < gameLocal.declTeamInfoType.Num(); i++ ) {
		sdTeamInfo* newTeamInfo = new sdTeamInfo();
		newTeamInfo->PreInit( i );
		teamInfo.Alloc() = newTeamInfo;
	}
	teamBits = idMath::BitsForInteger( gameLocal.declTeamInfoType.Num() + 1 );
}

/*
================
sdTeamManagerLocal::OnNewMapLoad
================
*/
void sdTeamManagerLocal::OnNewMapLoad( void ) {
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->OnNewMapLoad();
	}
}

/*
================
sdTeamManagerLocal::OnNewScriptLoad
================
*/
void sdTeamManagerLocal::OnNewScriptLoad( void ) {
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->OnNewScriptLoad();
	}
}

/*
================
sdTeamManagerLocal::OnMapStart
================
*/
void sdTeamManagerLocal::OnMapStart( void ) {
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->OnMapStart();
	}
}

/*
================
sdTeamManagerLocal::Shutdown
================
*/
void sdTeamManagerLocal::Shutdown( void ) {
	teamInfo.DeleteContents( true );
}

/*
================
sdTeamManagerLocal::GetTeam
================
*/
sdTeamInfo* sdTeamManagerLocal::GetTeamSafe( const char* name ) {	
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		if ( idStr::Icmp( teamInfo[ i ]->GetLookupName(), name ) ) {
			continue;
		}

		return teamInfo[ i ];
	}

	return NULL;
}

/*
============
sdTeamManagerLocal::GetIndexForTeam
============
*/
int sdTeamManagerLocal::GetIndexForTeam( const char* name ) {
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		if ( idStr::Icmp( teamInfo[ i ]->GetLookupName(), name ) ) {
			continue;
		}

		return i;
	}
	return -1;
}

/*
================
sdTeamManagerLocal::GetTeam
================
*/
sdTeamInfo& sdTeamManagerLocal::GetTeam( const char* name ) {	
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		if( !idStr::Icmp( teamInfo[ i ]->GetLookupName(), name ) ) {
			return *teamInfo[ i ];
		}
	}

	gameLocal.Error( "sdTeamManagerLocal::GetTeam Invalid Team Name '%s'", name );

	return *( sdTeamInfo* )NULL;
}

/*
================
sdTeamManagerLocal::GetTeamByIndex
================
*/
sdTeamInfo& sdTeamManagerLocal::GetTeamByIndex( int index ) {
	if( index < 0 || index >= teamInfo.Num() ) {
		gameLocal.Warning( "sdTeamManagerLocal::GetTeamByIndex Index OOB %i", index );
		return *teamInfo[ 0 ];
	}

	return *teamInfo[ index ];
}

/*
================
sdTeamManagerLocal::WriteTeamToStream
================
*/
void sdTeamManagerLocal::WriteTeamToStream( sdTeamInfo* newTeam, idFile* file ) const {
	int newIndex = newTeam ? newTeam->GetIndex() + 1 : 0;
	file->WriteInt( newIndex );
}

/*
================
sdTeamManagerLocal::WriteTeamToStream
================
*/
void sdTeamManagerLocal::WriteTeamToStream( sdTeamInfo* newTeam, idBitMsg& msg ) const {
	int newIndex = newTeam ? newTeam->GetIndex() + 1 : 0;
	msg.WriteBits( newIndex, teamBits );
}

/*
================
sdTeamManagerLocal::WriteTeamToStream
================
*/
void sdTeamManagerLocal::WriteTeamToStream( sdTeamInfo* oldTeam, sdTeamInfo* newTeam, idBitMsg& msg ) const {
	int oldIndex = oldTeam ? oldTeam->GetIndex() + 1 : 0;
	int newIndex = newTeam ? newTeam->GetIndex() + 1 : 0;

	msg.WriteDelta( oldIndex, newIndex, teamBits );
}

/*
================
sdTeamManagerLocal::ReadTeamFromStream
================
*/
sdTeamInfo* sdTeamManagerLocal::ReadTeamFromStream( idFile* file ) const {
	int teamnum;
	file->ReadInt( teamnum );
	teamnum--;

	if ( teamnum == -1 ) {
		return NULL;
	}
	return &sdTeamManager::GetInstance().GetTeamByIndex( teamnum );
}

/*
================
sdTeamManagerLocal::ReadTeamFromStream
================
*/
sdTeamInfo* sdTeamManagerLocal::ReadTeamFromStream( const idBitMsg& msg ) const {
	int teamnum = msg.ReadBits( teamBits ) - 1;
	if ( teamnum == -1 ) {
		return NULL;
	}
	return &sdTeamManager::GetInstance().GetTeamByIndex( teamnum );
}

/*
================
sdTeamManagerLocal::ReadTeamFromStream
================
*/
sdTeamInfo* sdTeamManagerLocal::ReadTeamFromStream( sdTeamInfo* oldTeam, const idBitMsg& msg ) const {
	int oldIndex = oldTeam ? oldTeam->GetIndex() + 1 : 0;

	int teamnum = msg.ReadDelta( oldIndex, teamBits ) - 1;
	if ( teamnum == -1 ) {
		return NULL;
	}
	return &sdTeamManager::GetInstance().GetTeamByIndex( teamnum );
}

/*
================
sdTeamManagerLocal::OnScriptChange
================
*/
void sdTeamManagerLocal::OnScriptChange( void ) {
	for( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->OnScriptChange();
	}
}

/*
================
sdTeamManagerLocal::ApplyNetworkState
================
*/
void sdTeamManagerLocal::ApplyNetworkState( const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdTeamManagerGameState );

	for ( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->ApplyNetworkState( newData.teamData[ i ] );
	}
}

/*
================
sdTeamManagerLocal::ReadNetworkState
================
*/
void sdTeamManagerLocal::ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdTeamManagerGameState );

	newData.teamData.SetNum( teamInfo.Num() );
	for ( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->ReadNetworkState( baseData.teamData[ i ], newData.teamData[ i ], msg );
	}
}

/*
================
sdTeamManagerLocal::WriteNetworkState
================
*/
void sdTeamManagerLocal::WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdTeamManagerGameState );

	newData.teamData.SetNum( teamInfo.Num() );
	for ( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ]->WriteNetworkState( baseData.teamData[ i ], newData.teamData[ i ], msg );
	}
}

/*
================
sdTeamManagerLocal::CheckNetworkStateChanges
================
*/
bool sdTeamManagerLocal::CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdTeamManagerGameState );

	for ( int i = 0; i < teamInfo.Num(); i++ ) {
		if ( teamInfo[ i ]->CheckNetworkStateChanges( baseData.teamData[ i ] ) ) {
			return true;
		}
	}

	return false;
}

/*
================
sdTeamManagerLocal::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdTeamManagerLocal::CreateNetworkStructure( void ) const {
	return new sdTeamManagerGameState();
}

/*
===============================================================================

	sdTeamInfo
	
===============================================================================
*/

/*
================
sdTeamInfoGameState::MakeDefault
================
*/
void sdTeamInfoGameState::MakeDefault( void ) {
	wins		= 0;

	scriptData.MakeDefault();
}

/*
================
sdTeamInfoGameState::MakeDefault
================
*/
void sdTeamInfoGameState::Write( idFile* file ) const {
	file->WriteInt( wins );

	scriptData.Write( file );
}

/*
================
sdTeamInfoGameState::Read
================
*/
void sdTeamInfoGameState::Read( idFile* file ) {
	file->ReadInt( wins );

	scriptData.Read( file );
}

extern const idEventDef EV_SyncScriptFieldBroadcast;
extern const idEventDef EV_SyncScriptFieldCallback;
extern const idEventDef EV_GetName;

const idEventDef EV_Team_GetTitle( "getTitle", 'h', DOC_TEXT( "Returns a handle to the localized name of the team." ), 0, NULL ); 
const idEventDef EV_Team_RegisterDeployable( "registerDeployable", '\0', DOC_TEXT( "Registers an entity as belonging to this team." ), 1, "See also $event:unRegisterDeployable$.", "e", "entity", "Entity to add to the list." );
const idEventDef EV_Team_UnRegisterDeployable( "unRegisterDeployable", '\0', DOC_TEXT( "Removes an entity from this team's list." ), 1, "See also $event:registerDeployable$.", "e", "entity", "Entity to remove from the list." );
const idEventDef EV_Team_RegisterSpawnPoint( "registerSpawnPoint", '\0', DOC_TEXT( "Registers an entity as an active spawn point for this team." ), 1, "See also $event:unRegisterSpawnPoint$.", "e", "entity", "Entity to add to the list." );
const idEventDef EV_Team_UnRegisterSpawnPoint( "unRegisterSpawnPoint", '\0', DOC_TEXT( "Removes an entity from thie team's active spawn points." ), 1, "See also $event:registerSpawnPoint$.", "e", "entity", "Entity to remove from the list." );

const idEventDef EV_Team_SetTeamRearSpawn( "setTeamRearSpawn", '\0', DOC_TEXT( "Marks this entity as the rear spawn for bots to use." ), 1, NULL, "E", "entity", "Entity to mark as the rear spawn." );

CLASS_DECLARATION( idClass, sdTeamInfo )
	EVENT( EV_GetName,							sdTeamInfo::Event_GetName )
	EVENT( EV_Team_GetTitle,					sdTeamInfo::Event_GetTitle )
	EVENT( EV_Team_RegisterDeployable,			sdTeamInfo::Event_RegisterDeployable )
	EVENT( EV_Team_UnRegisterDeployable,		sdTeamInfo::Event_UnRegisterDeployable )
	EVENT( EV_Team_RegisterSpawnPoint,			sdTeamInfo::Event_RegisterSpawnPoint )
	EVENT( EV_Team_UnRegisterSpawnPoint,		sdTeamInfo::Event_UnRegisterSpawnPoint )
	EVENT( EV_SyncScriptFieldBroadcast,			sdTeamInfo::Event_SyncScriptFieldBroadcast )
	EVENT( EV_SyncScriptFieldCallback,			sdTeamInfo::Event_SyncScriptFieldCallback )

	EVENT( EV_GetKey,							sdTeamInfo::Event_GetKey )
	EVENT( EV_GetIntKey,						sdTeamInfo::Event_GetIntKey )
	EVENT( EV_GetFloatKey,						sdTeamInfo::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,						sdTeamInfo::Event_GetVectorKey )
	EVENT( EV_GetKeyWithDefault,				sdTeamInfo::Event_GetKeyWithDefault )
	EVENT( EV_GetIntKeyWithDefault,				sdTeamInfo::Event_GetIntKeyWithDefault )
	EVENT( EV_GetFloatKeyWithDefault,			sdTeamInfo::Event_GetFloatKeyWithDefault )
	EVENT( EV_GetVectorKeyWithDefault,			sdTeamInfo::Event_GetVectorKeyWithDefault )

	EVENT( EV_Team_SetTeamRearSpawn,			sdTeamInfo::Event_SetTeamRearSpawn )
END_CLASS

const char* fireTeamNames[ sdTeamInfo::MAX_FIRETEAMS ] = {
	"Alpha",
	"Beta",
	"Gamma",
	"Delta",
	"Epsilon",
	"Zeta",
	"Eta",
	"Theta"
};

/*
================
sdTeamInfo::sdTeamInfo
================
*/
sdTeamInfo::sdTeamInfo( void ) : index( -1 ), info( NULL ), passwordCVar( NULL ), lifeStatTitle( NULL ) {
	wins = 0;

	info = NULL;

	defaultClass = NULL;
	scriptObject = NULL;
	scriptThread = NULL;
	allowRespawnFunc = NULL;

	teamNextRespawnTimeFunc = NULL;

	rearSpawnBase = NULL;

	playZoneExitToolTip = NULL;
		
	fireTeams = new sdFireTeam[ MAX_FIRETEAMS ];
	for ( int i = 0; i < MAX_FIRETEAMS; i++ ) {
		fireTeams[ i ].SetDefaultName( fireTeamNames[ i ] );
		fireTeams[ i ].SetGameTeam( this );
	}

	ResetAll();
}

/*
================
sdTeamInfo::sdTeamInfo
================
*/
sdTeamInfo::~sdTeamInfo( void ) {
	delete[] fireTeams;
}

/*
================
sdTeamInfo::GetFireTeam
================
*/
sdFireTeam& sdTeamInfo::GetFireTeam( int index ) {
	return fireTeams[ index ];
}

/*
================
sdTeamInfo::GetFireTeamIndex
================
*/
int sdTeamInfo::GetFireTeamIndex( sdFireTeam* fireTeam ) const {
	if ( fireTeam == NULL ) {
		return -1;
	}

	for ( int i = 0; i < MAX_FIRETEAMS; i++ ) {
		if ( static_cast<const sdFireTeam*>( &fireTeams[ i ] ) == fireTeam ) {
			return i;
		}
	}

	return -1;
}

/*
================
sdTeamInfo::GetDefaultFireTeamName
================
*/
const char* sdTeamInfo::GetDefaultFireTeamName( int index ) {
	return fireTeamNames[ index ];
}


/*
================
sdTeamInfo::PreInit
================
*/
void sdTeamInfo::PreInit( int _index ) {
	index = _index;

	const sdDeclStringMap* data = gameLocal.declTeamInfoType.LocalFindByIndex( index, false );
	lookupname = data->GetName();
}

/*
================
sdTeamInfo::OnScriptChange
================
*/
void sdTeamInfo::OnScriptChange( void ) {
	if ( scriptThread != NULL ) {
		gameLocal.program->FreeThread( scriptThread );
		scriptThread = NULL;
	}

	if ( scriptObject ) {
		gameLocal.program->FreeScriptObject( scriptObject );
	}
}

/*
================
sdTeamInfo::Reset
================
*/
void sdTeamInfo::Reset( void ) {
	deployables.Clear();
	spawnLocations.Clear();
}

/*
================
sdTeamInfo::ResetAll
================
*/
void sdTeamInfo::ResetAll( void ) {
	Reset();

	wins			= 0;
}

/*
================
sdTeamInfo::AllowRespawn
================
*/
bool sdTeamInfo::AllowRespawn( idPlayer* player ) {
	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	scriptObject->CallNonBlockingScriptEvent( allowRespawnFunc, h1 );	
	return gameLocal.program->GetReturnedInteger() != 0;
}

/*
================
sdTeamInfo::GetTeamRespawnTime
================
*/
int sdTeamInfo::GetTeamRespawnTime(  void ) {
 	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( teamNextRespawnTimeFunc, h1 );	
	return idMath::Ftoi( gameLocal.program->GetReturnedFloat() );
}

/*
================
sdTeamInfo::OnCampaignChange
================
*/
void sdTeamInfo::OnCampaignChange( void ) {
	ResetAll();
}

/*
================
sdTeamInfo::Init
================
*/
void sdTeamInfo::Init( void ) {
	info = gameLocal.declTeamInfoType[ index ];

	const idDict& dict = info->GetDict();

	title = declHolder.declLocStrType[ dict.GetString( "name" ) ];
	if ( title == NULL ) {
		gameLocal.Error( "sdTeamInfo::Init No Title Provided" );
	}

	winString = declHolder.declLocStrType[ dict.GetString( "match_win_string" ) ];
	winStringSW_Speed = declHolder.declLocStrType[ dict.GetString( "match_win_sw_speed" ) ];
	winStringSW_XP = declHolder.declLocStrType[ dict.GetString( "match_win_sw_xp" ) ];

	winMusic = dict.GetString( "snd_music_won" );

	spawnFxName				= dict.GetString( "fx_spawn", "fx/teleporterplayer" );

	allowRevive				= dict.GetBool( "allow_revive" );

	crashLandFxName			= dict.GetString( "fx_crashland" );
	crashLandThreshold		= dict.GetFloat( "crashland_threshold" );
	parachuteLandFxName		= dict.GetString( "fx_parachuteland" );
	parachuteLandThreshold	= dict.GetFloat( "parachuteland_threshold" );

	botTeam					= ( playerTeamTypes_t )dict.GetInt( "bot_team" );

	highCommandName			= declHolder.declLocStrType[ dict.GetString( "high_command_name" ) ];
	if ( highCommandName == NULL ) {
		gameLocal.Warning( "sdTeamInfo::Init No High Command Name Provided" );
	}

	defaultClass		= gameLocal.declPlayerClassType[ dict.GetString( "pc_default" ) ];
	if( !defaultClass ) {
		gameLocal.Error( "sdTeamInfo::Init No Default Class Supplied" );
	}

	proficiencyTypes.Clear();
	const idKeyValue* kv = NULL;
	while ( kv = dict.MatchPrefix( "proficiencyList", kv ) ) {
		const sdDeclProficiencyType* pType = gameLocal.declProficiencyTypeType[ kv->GetValue() ];
		if ( !pType ) {
			gameLocal.Error( "sdTeamInfo::Init Invalid Proficiency Type '%s'", kv->GetValue().c_str() );
		}
		proficiencyTypes.Alloc() = pType;
	}

	const char* passwordCVarName = dict.GetString( "cvar_password" );
	if ( *passwordCVarName ) {
		passwordCVar = cvarSystem->Find( passwordCVarName );
		if ( !passwordCVar ) {
			cvarSystem->SetCVarString( passwordCVarName, "", CVAR_NOCHEAT );
			passwordCVar = cvarSystem->Find( passwordCVarName );

			assert( passwordCVar );
		}
	} else {
		passwordCVar = NULL;
	}

	int count = dict.GetInt( "num_ratings" );
	ratings.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		const char* ratingName = dict.GetString( va( "rating_%i", i + 1 ) );

		const sdDeclRating* rating = gameLocal.declRatingType[ ratingName ];
		if ( !rating ) {
			gameLocal.Error( "sdTeamInfo::Init Failed to Load Rating '%s'", ratingName );
		}
		ratings[ i ] = rating;
	}

	playZoneExitToolTip = gameLocal.declToolTipType[ dict.GetString( "tt_leavingzone" ) ];

	// load lifeStat titles
	const char* lifeStatName = dict.GetString( "lifestat_title", "blank" );
	lifeStatTitle = declHolder.declLocStrType.LocalFind( lifeStatName );

	game->CacheDictionaryMedia( dict );
}

/*
================
sdTeamInfo::SortPlayers
================
*/
void sdTeamInfo::SortPlayers( idStaticList< idPlayer*, MAX_CLIENTS >& list, bool wantSpectators ) {
	list.Clear();

	idStaticList< idPlayer*, MAX_CLIENTS > specList;

	int i;
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* p = gameLocal.GetClient( i );
		if( p == NULL ) {
			continue;
		}

		sdTeamInfo* team = p->GetGameTeam();
		if( ( team == NULL || *team != *this ) ) {
			if( !wantSpectators ) {
				continue;
			}
			idPlayer* specClient = p->GetSpectateClient();
			if( specClient == NULL || specClient == p ) {
				continue;
			}
			sdTeamInfo* spectatingTeam = specClient->GetGameTeam();
			if( spectatingTeam == NULL || *spectatingTeam != *this ) {
				continue;
			}
			specList.Append( p );
			continue;
		}

		list.Append( p );
	}

	list.Sort( SortPlayersFunc );

	// ensure that spectators always report last
	specList.Sort( SortPlayersFunc );
	list.Append( specList );
}

/*
================
sdTeamInfo::SortPlayersFunc
================
*/
int sdTeamInfo::SortPlayersFunc( const playerPtr_t* a, const playerPtr_t* b ) {
	return idMath::Ftoi( ( *b )->GetProficiencyTable().GetXP() ) - idMath::Ftoi( ( *a )->GetProficiencyTable().GetXP() );
}

/*
================
sdTeamInfo::Think
================
*/
void sdTeamInfo::Think( void ) {
	UpdateScript();
}

/*
================
sdTeamInfo::UpdateScript
================
*/
void sdTeamInfo::UpdateScript( void ) {
	if ( !scriptThread || gameLocal.IsPaused() ) {
		return;
	}

	// don't call script until it's done waiting
	if ( scriptThread->IsWaiting() ) {
		return;
	}

	scriptThread->Execute();
}

/*
================
sdTeamInfo::OnNewMapLoad
================
*/
void sdTeamInfo::OnNewMapLoad( void ) {
	Init();
}

/*
================
sdTeamInfo::PointInRadar
================
*/
idEntity* sdTeamInfo::PointInRadar( const idVec3& _target, radarMasks_t type, const idList< sdRadarLayer* >& list ) {
	int worldZoneIndex = gameLocal.GetWorldPlayZoneIndex( _target );
	idVec2 target = _target.ToVec2();

	for ( int index = 0; index < list.Num(); index++ ) {
		sdRadarLayer* layer = list[ index ];

		idEntity* owner = layer->GetOwner();
		if ( owner == NULL ) {
			gameLocal.Warning( "sdTeamInfo::PointInRadar radar layer with NULL owner" );
			continue;
		}

		if ( ( layer->GetMask() & type ) == 0 ) {
			continue;
		}

		const idVec2& layerOrigin = layer->GetOrigin();

		idVec2 currentDir = target - layerOrigin;

		float currentRange = currentDir.LengthSqr();
		if ( currentRange > layer->GetRange() ) {
			continue;
		}

		float maxAngle = layer->GetMaxAngle();
		if ( maxAngle < idMath::PI ) {
			currentDir.NormalizeFast();

			float angle = idMath::ACos( currentDir * layer->GetDirection() );
			if ( angle > maxAngle ) {
				continue;
			}
		}

		int radarZoneIndex = gameLocal.GetWorldPlayZoneIndex( idVec3( layerOrigin.x, layerOrigin.y, 0.f ) );
		if ( radarZoneIndex != worldZoneIndex ) {
			return NULL;
		}

		return owner;
	}

	return NULL;
}

/*
================
sdTeamInfo::PointInJammer
================
*/
idEntity* sdTeamInfo::PointInJammer( const idVec3& target, radarMasks_t type ) {
	return PointInRadar( target, type, jammerLayers );
}

/*
================
sdTeamInfo::PointInRadar
================
*/
idEntity* sdTeamInfo::PointInRadar( const idVec3& target, radarMasks_t type ) {
	return PointInRadar( target, type, radarLayers );
}

/*
================
sdTeamInfo::RegisterDeployable
================
*/
void sdTeamInfo::RegisterDeployable( idEntity* other ) {
	AddEntityToList( deployables, other );
}

/*
================
sdTeamInfo::UnRegisterDeployable
================
*/
void sdTeamInfo::UnRegisterDeployable( idEntity* other ) {
	RemoveEntityFromList( deployables, other );
}

/*
================
sdTeamInfo::RegisterSpawnPoint
================
*/
void sdTeamInfo::RegisterSpawnPoint( idEntity* other ) {
	idEntity* oldDefault = spawnLocations.Num() ? spawnLocations[ 0 ].GetEntity() : NULL;

	idStr priorityName = va( "spawn_priority_%s", lookupname.c_str() );

	int otherPriority = other->spawnArgs.GetInt( priorityName );

	int i;
	for ( i = 0; i < spawnLocations.Num(); ) {
		idEntity* loc = spawnLocations[ i ];
		if ( !loc ) {
			spawnLocations.RemoveIndex( i );
			continue;
		}

		int locPriority = loc->spawnArgs.GetInt( priorityName );
		if ( locPriority < otherPriority ) {
			idEntityPtr< idEntity > temp = other;
			spawnLocations.Insert( temp, i );
			break;
		}
		
		i++;
	}

	if ( i == spawnLocations.Num() ) {
		spawnLocations.Alloc() = other;
	}

	idEntity* newDefault = spawnLocations[ 0 ];

	if ( oldDefault != newDefault ) {
		gameLocal.localPlayerProperties.OnDefaultSpawnChanged( this, newDefault );
	}
}

/*
================
sdTeamInfo::UnRegisterSpawnPoint
================
*/
void sdTeamInfo::UnRegisterSpawnPoint( idEntity* other ) {
	idEntity* oldDefault = spawnLocations.Num() ? spawnLocations[ 0 ].GetEntity() : NULL;
	
	RemoveEntityFromList( spawnLocations, other );

	idEntity* newDefault = spawnLocations.Num() ? spawnLocations[ 0 ].GetEntity() : NULL;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player || !player->IsTeam( this ) ) {
			continue;
		}

		if ( player->GetSpawnPoint() == other ) {
			player->SetSpawnPoint( NULL );
		}
	}

	if ( oldDefault != newDefault ) {
		gameLocal.localPlayerProperties.OnDefaultSpawnChanged( this, newDefault );
	}

	if ( other == rearSpawnBase.GetEntity() ) {
		rearSpawnBase = NULL;
	}
}

/*
================
sdTeamInfo::ApplyNetworkState
================
*/
void sdTeamInfo::ApplyNetworkState( const sdTeamInfoGameState& newData ) {
	// update state
	wins				= newData.wins;

	scriptObject->ApplyNetworkState( NSM_BROADCAST, newData.scriptData );
}

/*
================
sdTeamInfo::ReadNetworkState
================
*/
void sdTeamInfo::ReadNetworkState( const sdTeamInfoGameState& baseData, sdTeamInfoGameState& newData, const idBitMsg& msg ) const {
	// read state
	newData.wins		= msg.ReadDeltaLong( baseData.wins );

	scriptObject->ReadNetworkState( NSM_BROADCAST, baseData.scriptData, newData.scriptData, msg );
}

/*
================
sdTeamInfo::WriteNetworkState
================
*/
void sdTeamInfo::WriteNetworkState( const sdTeamInfoGameState& baseData, sdTeamInfoGameState& newData, idBitMsg& msg ) const {
	// update state
	newData.wins		= wins;

	// write state
	msg.WriteDeltaLong( baseData.wins, newData.wins );

	scriptObject->WriteNetworkState( NSM_BROADCAST, baseData.scriptData, newData.scriptData, msg );
}

/*
================
sdTeamInfo::CheckNetworkStateChanges
================
*/
bool sdTeamInfo::CheckNetworkStateChanges( const sdTeamInfoGameState& baseData ) const {
	if ( scriptObject->CheckNetworkStateChanges( NSM_BROADCAST, baseData.scriptData ) ) {
		return true;
	}

	return ( baseData.wins != wins );
}

/*
================
sdTeamInfo::CanJoin
================
*/
bool sdTeamInfo::CanJoin( idPlayer* player, const char* password, const sdDeclLocStr*& reason ) const {
	if ( player->IsType( idBot::Type ) ) {
		return true;
	}

	if ( passwordCVar != NULL ) {
		const char* passwordString = passwordCVar->GetString();
		if ( *passwordString != '\0' ) {
			if ( idStr::Cmp( passwordString, password ) ) {
				reason = declHolder.declLocStrType[ "teams/messages/wrongpassword" ];
				return false;
			}
		}
	}
	return true;
}

/*
================
sdTeamInfo::OnNewScriptLoad
================
*/
void sdTeamInfo::OnNewScriptLoad( void ) {
	const char* scriptObjectName = info->GetDict().GetString( "scriptObject", "default" );
	scriptObject = gameLocal.program->AllocScriptObject( this, scriptObjectName );
	allowRespawnFunc = scriptObject->GetFunction( "OnAllowRespawn" );
	
	teamNextRespawnTimeFunc = scriptObject->GetFunction( "GetNextRespawnTime" );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h1 );

	sdScriptHelper h2;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetSyncFunc(), h2 );

	const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
	if ( constructor ) {
		scriptThread = gameLocal.program->CreateThread();
		scriptThread->SetName( "sdTeamInfo" );
		scriptThread->CallFunction( scriptObject, constructor );
		scriptThread->ManualControl();
		scriptThread->ManualDelete();
	}
}

/*
================
sdTeamInfo::OnMapStart
================
*/
void sdTeamInfo::OnMapStart( void ) {
	assert( scriptObject );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnMapStart" ), h1 );
}

/*
================
sdTeamInfo::Event_GetName
================
*/
void sdTeamInfo::Event_GetName( void ) {
	sdProgram::ReturnString( lookupname );
}

/*
================
sdTeamInfo::Event_GetTitle
================
*/
void sdTeamInfo::Event_GetTitle( void ) {
	sdProgram::ReturnHandle( title->Index() );
}

/*
================
sdTeamInfo::Event_RegisterDeployable
================
*/
void sdTeamInfo::Event_RegisterDeployable( idEntity* other ) {
	RegisterDeployable( other );
}

/*
================
sdTeamInfo::Event_UnRegisterDeployable
================
*/
void sdTeamInfo::Event_UnRegisterDeployable( idEntity* other ) {
	UnRegisterDeployable( other );
}

/*
===============
sdTeamInfo::Event_RegisterSpawnPoint
===============
*/
void sdTeamInfo::Event_RegisterSpawnPoint( idEntity* other ) {
	RegisterSpawnPoint( other );
}

/*
===============
sdTeamInfo::Event_UnRegisterSpawnPoint
===============
*/
void sdTeamInfo::Event_UnRegisterSpawnPoint( idEntity* other ) {
	UnRegisterSpawnPoint( other );
}

/*
================
sdTeamInfo::Event_SyncScriptFieldBroadcast
================
*/
void sdTeamInfo::Event_SyncScriptFieldBroadcast( const char* fieldName ) {
	scriptObject->SetSynced( fieldName, true );
}

/*
================
sdTeamInfo::Event_SyncScriptFieldCallback
================
*/
void sdTeamInfo::Event_SyncScriptFieldCallback( const char* fieldName, const char* functionName ) {
	scriptObject->SetSyncCallback( fieldName, functionName );
}

class sdFireTeam_InitialCreateFinaliser : public sdVoteFinalizer {
public:
	sdFireTeam_InitialCreateFinaliser( idPlayer* player ) : _player( player ) {
	}

	virtual ~sdFireTeam_InitialCreateFinaliser( void ) {
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( passed ) {
			idPlayer* player = _player;
			if ( player ) {
				sdTeamInfo* team = player->GetTeam();
				if ( team ) {
					team->CreatePublicFireTeam( player );
				} else {
					assert( false );
				}
			}
		}
	}

private:
	idEntityPtr< idPlayer >	_player;
};

class sdFireTeam_InitialJoinFinaliser : public sdVoteFinalizer {
public:
	sdFireTeam_InitialJoinFinaliser( idPlayer* player, sdFireTeam* fireTeam ) : _fireTeam( fireTeam ), _player( player ) {
	}

	virtual ~sdFireTeam_InitialJoinFinaliser( void ) {
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( passed ) {
			idPlayer* player = _player;
			if ( player ) {
				_fireTeam->AddMember( player->entityNumber );
			}
		}
	}

private:
	sdFireTeam*				_fireTeam;
	idEntityPtr< idPlayer >	_player;
};

/*
================
sdTeamInfo::CreatePrivateFireTeam
================
*/
void sdTeamInfo::CreatePrivateFireTeam( idPlayer* player ) {
	for ( int i = 0; i < MAX_FIRETEAMS; i++ ) {
		if ( fireTeams[ i ].GetNumMembers() == 0 ) {
			fireTeams[ i ].AddMember( player->entityNumber );
			fireTeams[ i ].SetPrivate( true );
			return;
		}
	}

	player->SendLocalisedMessage( declHolder.declLocStrType[ "fireteam/messages/noneleft" ], idWStrList() );
}

/*
================
sdTeamInfo::CreatePublicFireTeam
================
*/
bool sdTeamInfo::CreatePublicFireTeam( idPlayer* player ) {
	for ( int i = 0; i < MAX_FIRETEAMS; i++ ) {
		if ( fireTeams[ i ].GetNumMembers() == 0 ) {
			fireTeams[ i ].AddMember( player->entityNumber );
			fireTeams[ i ].SetPrivate( false );
			return true;
		}
	}

	return false;
}

/*
================
sdTeamInfo::TryFindPrivateFireTeam
================
*/
void sdTeamInfo::TryFindPrivateFireTeam( idPlayer* player ) {
	for ( int i = 0; i < MAX_FIRETEAMS; i++ ) {
		if ( fireTeams[ i ].GetNumMembers() == 0 ) {
			sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
			if ( !vote ) {
				return;
			}
			vote->DisableFinishMessage();
			vote->MakePrivateVote( player );
			vote->Tag( VI_FIRETEAM_CREATE, player );
			vote->SetText( gameLocal.declToolTipType[ "fireteam_initial_create" ] );
			vote->SetFinalizer( new sdFireTeam_InitialCreateFinaliser( player ) );
			vote->Start();
			return;
		} else if ( !fireTeams[ i ].IsPrivate() && fireTeams[ i ].GetNumMembers() != sdFireTeam::MaxMembers() ) {
			sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
			if ( !vote ) {
				return;
			}
			vote->DisableFinishMessage();
			vote->MakePrivateVote( player );
			vote->Tag( VI_FIRETEAM_JOIN, player );
			vote->SetText( gameLocal.declToolTipType[ "fireteam_initial_join" ] );
			vote->SetFinalizer( new sdFireTeam_InitialJoinFinaliser( player, &fireTeams[ i ] ) );
			vote->Start();
			return;
		}
	}
}

/*
================
sdTeamInfo::Event_GetKey
================
*/
void sdTeamInfo::Event_GetKey( const char *key ) {
	sdProgram::ReturnString( GetDict().GetString( key ) );
}

/*
================
sdTeamInfo::Event_GetIntKey
================
*/
void sdTeamInfo::Event_GetIntKey( const char *key ) {
	sdProgram::ReturnInteger( GetDict().GetInt( key ) );
}

/*
================
sdTeamInfo::Event_GetFloatKey
================
*/
void sdTeamInfo::Event_GetFloatKey( const char *key ) {
	sdProgram::ReturnFloat( GetDict().GetFloat( key ) );
}

/*
================
sdTeamInfo::Event_GetVectorKey
================
*/
void sdTeamInfo::Event_GetVectorKey( const char *key ) {
	sdProgram::ReturnVector( GetDict().GetVector( key ) );
}

/*
================
sdTeamInfo::Event_GetKeyWithDefault
================
*/
void sdTeamInfo::Event_GetKeyWithDefault( const char *key, const char* defaultvalue ) {
	sdProgram::ReturnString( GetDict().GetString( key, defaultvalue ) );
}

/*
================
sdTeamInfo::Event_GetIntKeyWithDefault
================
*/
void sdTeamInfo::Event_GetIntKeyWithDefault( const char *key, int defaultvalue ) {
	sdProgram::ReturnFloat( GetDict().GetInt( key, va( "%i", defaultvalue ) ) );
}

/*
================
sdTeamInfo::Event_GetFloatKeyWithDefault
================
*/
void sdTeamInfo::Event_GetFloatKeyWithDefault( const char *key, float defaultvalue ) {
	sdProgram::ReturnFloat( GetDict().GetFloat( key, va( "%f", defaultvalue ) ) );
}

/*
================
sdTeamInfo::Event_GetVectorKeyWithDefault
================
*/
void sdTeamInfo::Event_GetVectorKeyWithDefault( const char *key, const idVec3& defaultvalue ) {
	sdProgram::ReturnVector( GetDict().GetVector( key, defaultvalue.ToString() ) );
}

/*
================
sdTeamInfo::Event_SetTeamRearSpawn
================
*/
void sdTeamInfo::Event_SetTeamRearSpawn( idEntity* teamRearSpawnBase ) {
	rearSpawnBase = teamRearSpawnBase;
}

/*
============
sdTeamInfo::GetEquivalentClass
============
*/
const sdDeclPlayerClass* sdTeamInfo::GetEquivalentClass( const sdDeclPlayerClass& currentClass, const sdTeamInfo& desiredTeam ) const {
	if( this == &desiredTeam ) {
		return &currentClass;
	}

	const idDict& dict = desiredTeam.GetDict();
	idStr key = va( "pc_team_remap_%s_%s", GetLookupName(), currentClass.GetName() );
	const char* remappedClass = dict.GetString( key, "" );
	
	if( idStr::Length( remappedClass ) == 0 ) {
		return &currentClass;
	}
	return gameLocal.declPlayerClassType.LocalFind( remappedClass, false );

}

/*
============
sdTeamInfo::AllocRadarLayer
============
*/
sdTeamInfo::sdRadarLayer* sdTeamInfo::AllocRadarLayer( void ) {
	sdRadarLayer* newLayer = radarAllocator.Alloc();
	radarLayers.Alloc() = newLayer;
	return newLayer;
}

/*
============
sdTeamInfo::AllocJammerLayer
============
*/
sdTeamInfo::sdRadarLayer* sdTeamInfo::AllocJammerLayer( void ) {
	sdRadarLayer* newLayer = radarAllocator.Alloc();
	jammerLayers.Alloc() = newLayer;
	return newLayer;
}

/*
============
sdTeamInfo::FreeRadarLayer
============
*/
void sdTeamInfo::FreeRadarLayer( sdRadarLayer* layer ) {
	int index;
	
	index = radarLayers.FindIndex( layer );
	if ( index != -1 ) {
		radarLayers.RemoveIndexFast( index );
		radarAllocator.Free( layer );
		return;
	}
	index = jammerLayers.FindIndex( layer );
	if ( index != -1 ) {
		jammerLayers.RemoveIndexFast( index );
		radarAllocator.Free( layer );
		return;
	}
}

/*
============
sdTeamInfo::GetRating
============
*/
const sdDeclRating* sdTeamInfo::GetRating( int index ) const {
	if ( index < 0 || ratings.Num() <= 0 ) {
		return NULL;
	}
	if ( index >= ratings.Num() ) {
		return ratings[ ratings.Num() - 1 ];
	}
	return ratings[ index ];
}

/*
============
sdTeamInfo::GetNumPlayers
============
*/
int sdTeamInfo::GetNumPlayers( bool excludeBots ) const {
	int count = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->GetGameTeam() != this ) {
			continue;
		}

		if ( excludeBots ) {
			if ( player->IsType( idBot::Type ) ) {
				continue;
			}
		}

		count++;
	}

	return count;
}

/*
============
sdTeamInfo::GetTotalXP
============
*/
float sdTeamInfo::GetTotalXP( void ) const {
	float total = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->GetGameTeam() != this ) {
			continue;
		}

		total += player->GetProficiencyTable().GetXP();
	}
	return total;
}

/*
============
sdTeamInfo::GetHomeBaseSpawn
============
*/
idEntity* sdTeamInfo::GetHomeBaseSpawn( void ) {
	if ( rearSpawnBase == NULL ) { //mal: oops - someone didn't set us up!
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Bot Team %s has no valid Rear Spawn Set!", ( botTeam == GDF ) ? "GDF" : "STROGG" );
		}
	}

	return rearSpawnBase;
}

/*
============
sdTeamInfo::GetLifeStatTitle
============
*/
const sdDeclLocStr* sdTeamInfo::GetLifeStatTitle( void ) const {
	return lifeStatTitle;
}
