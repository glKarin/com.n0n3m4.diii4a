// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_STRUCTURES_TEAMMANAGER_H__
#define __GAME_STRUCTURES_TEAMMANAGER_H__

class sdDeployableEntity;
class sdTeamInfo;
class sdRadarInterface;

#include "../roles/RoleManager.h"

class sdFireTeam;

class sdTeamInfoGameState {
public:
										sdTeamInfoGameState( void ) { ; }

	void								MakeDefault( void );

	void								Write( idFile* file ) const;
	void								Read( idFile* file );

	int									wins;
	sdScriptObjectNetworkData			scriptData;
};

class sdTeamInfo : public idClass {
public:
	static const int					MAX_FIRETEAMS = 8;

	CLASS_PROTOTYPE( sdTeamInfo );

	typedef idPlayer* playerPtr_t;

	class sdRadarLayer {
	public:
										sdRadarLayer( void ) { range = 0.f; mask = 0; maxAngle = 360.f; }

		void							SetRange( float value ) { range = Square( value ); }
		void							SetMaxAngle( float value ) { maxAngle = DEG2RAD( value * 0.5f ); }
		void							SetMask( int value ) { mask = value; }
		void							SetOrigin( const idVec3& value ) { origin = value.ToVec2(); }
		void							SetDirection( const idVec3& value ) { direction = value.ToVec2(); }
		void							SetOwner( idEntity* ent ) { owner = ent; }

		float							GetRange( void ) const { return range; }
		float							GetMaxAngle( void ) const { return maxAngle; }
		int								GetMask( void ) const { return mask; }
		const idVec2&					GetOrigin( void ) const { return origin; }
		const idVec2&					GetDirection( void ) const { return direction; }
		idEntity*						GetOwner( void ) const { return owner; }		

	private:
		float							maxAngle;
		float							range;
		int								mask;
		idVec2							origin;
		idVec2							direction;
		idEntityPtr< idEntity >			owner;
	};
										sdTeamInfo( void );
										~sdTeamInfo( void );

	bool								operator== ( const sdTeamInfo& other ) { return ( this && ( &other ) ) && other.index == index; }
	bool								operator!= ( const sdTeamInfo& other ) { return !( *this == other ); }

	void								PreInit( int _index );
	void								Init( void );
	void								Think( void );
	void								Precache( void );
	void								SortPlayers( idStaticList< idPlayer*, MAX_CLIENTS >& list, bool wantSpectators = false );
	static int							SortPlayersFunc( const playerPtr_t* a, const playerPtr_t* b );
	void								UpdateScript( void );

	void								Event_GetName( void );
	void								Event_GetTitle( void );
	void								Event_RegisterDeployable( idEntity* other );
	void								Event_UnRegisterDeployable( idEntity* other );
	void								Event_GetDeployCategoryMax( int deployObjectCategory );
	void								Event_RegisterSpawnPoint( idEntity* other );
	void								Event_UnRegisterSpawnPoint( idEntity* other );
	void								Event_SyncScriptFieldBroadcast( const char* fieldName );
	void								Event_SyncScriptFieldCallback( const char* fieldName, const char* functionName );

	void								Event_GetKey( const char *key );
	void								Event_GetIntKey( const char *key );
	void								Event_GetFloatKey( const char *key );
	void								Event_GetVectorKey( const char *key );

	void								Event_GetKeyWithDefault( const char *key, const char* defaultvalue );
	void								Event_GetIntKeyWithDefault( const char *key, int defaultvalue );
	void								Event_GetFloatKeyWithDefault( const char *key, float defaultvalue );
	void								Event_GetVectorKeyWithDefault( const char *key, const idVec3& defaultvalue );
	void								Event_SetTeamRearSpawn( idEntity* teamRearSpawnBase );

	sdRadarLayer*						AllocRadarLayer( void );
	sdRadarLayer*						AllocJammerLayer( void );
	void								FreeRadarLayer( sdRadarLayer* layer );

	const sdDeclLocStr*					GetLifeStatTitle( void ) const;

	sdFireTeam&							GetFireTeam( int index );
	int									GetFireTeamIndex( sdFireTeam* fireTeam ) const;
	static const char*					GetDefaultFireTeamName( int index );

	int									GetTeamRespawnTime( void );

	bool								CanJoin( idPlayer* player, const char* password, const sdDeclLocStr*& reason ) const;

	const idDict&						GetDict( void ) const { return info->GetDict(); }

	playerTeamTypes_t					GetBotTeam( void ) const { return botTeam; }
	int									GetIndex( void ) const { return index; }
	const char*							GetLookupName( void ) const { return lookupname; }
	const sdDeclLocStr*					GetTitle( void ) const { return title; }
	const sdDeclLocStr*					GetWinString( void ) const { return winString; }
	const sdDeclLocStr*					GetWinStringSW_Speed( void ) const { return winStringSW_Speed; }
	const sdDeclLocStr*					GetWinStringSW_XP( void ) const { return winStringSW_XP; }

	const char*							GetWinMusic( void ) const { return winMusic; }

	const char*							GetSpawnFxName( void ) const { return spawnFxName; }

	const char*							GetCrashLandFxName() const { return crashLandFxName; }
	float								GetCrashLandThreshold() const { return crashLandThreshold; }
	const char*							GetParachuteLandFxName() const { return parachuteLandFxName; }
	float								GetParachuteLandThreshold() const { return parachuteLandThreshold; }

	const wchar_t*						GetHighCommandName( void ) const { return highCommandName ? highCommandName->GetText() : NULL; }

	const sdDeclRating*					GetRating( int index ) const;

	void								ClearMapData( void );

	void								TryFindPrivateFireTeam( idPlayer* player );
	void								CreatePrivateFireTeam( idPlayer* player );
	bool								CreatePublicFireTeam( idPlayer* player );

	void								OnNewMapLoad( void );
	void								OnNewScriptLoad( void );
	void								OnMapStart( void );
	void								OnScriptChange( void );
	void								OnCampaignChange( void );

	void								Reset( void );
	void								ResetAll( void );

	void								ApplyNetworkState( const sdTeamInfoGameState& newData );
	void								ReadNetworkState( const sdTeamInfoGameState& baseData, sdTeamInfoGameState& newData, const idBitMsg& msg ) const;
	void								WriteNetworkState( const sdTeamInfoGameState& baseData, sdTeamInfoGameState& newData, idBitMsg& msg ) const;
	bool								CheckNetworkStateChanges( const sdTeamInfoGameState& baseData ) const;

	void								AddWin( void ) { wins++; }
	int									GetNumWins( void ) const { return wins; }

	idEntity*							PointInRadar( const idVec3& target, radarMasks_t type );
	idEntity*							PointInJammer( const idVec3& target, radarMasks_t type );

	bool								AllowRespawn( idPlayer* player );

	const sdDeclPlayerClass*			GetDefaultClass( void ) const { return defaultClass; }

										// returns defaultClass if there is no mapping between the classes
	const sdDeclPlayerClass*			GetEquivalentClass( const sdDeclPlayerClass& currentClass, const sdTeamInfo& desiredTeam ) const;

	idEntity*							GetDefaultSpawn( void ) { return spawnLocations.Num() ? spawnLocations[ 0 ].GetEntity() : NULL; }
	const idEntity*						GetDefaultSpawn( void ) const { return spawnLocations.Num() ? spawnLocations[ 0 ].GetEntity() : NULL; }

	int									GetNumSpawnLocations( void )  const { return spawnLocations.Num(); }
	idEntity*							GetSpawnLocation( int index ) { return spawnLocations[ index ]; }
	const idEntity*						GetSpawnLocation( int index ) const { return spawnLocations[ index ]; }

										// returns the main base spawn for the team
	idEntity*							GetHomeBaseSpawn( void );

	const idList< idEntityPtr< idEntity > >& GetDeployables( void ) const { return deployables; }

	void								RegisterDeployable( idEntity* other );
	void								UnRegisterDeployable( idEntity* other );

	void								RegisterSpawnPoint( idEntity* other );
	void								UnRegisterSpawnPoint( idEntity* other );

	virtual idScriptObject*				GetScriptObject( void ) const { return scriptObject; }

	bool								AllowRevive( void ) const { return allowRevive; }

	template< typename T >
	void								AddEntityToList( idList< T >& list, idEntity* other ) {
		for ( int i = 0; i < list.Num(); i++ ) { if ( list[ i ].GetEntity() == other ) { return; } }
		list.Alloc() = other;
	}

	template< typename T >
	void								RemoveEntityFromList( idList< T >& list, idEntity* other ) {
		for ( int i = 0; i < list.Num(); i++ ) {
			if ( list[ i ].GetEntity() != other ) { continue; }
			list.RemoveIndex( i );
			return;
		}
	}

	idList< const sdDeclProficiencyType* >& GetProficiencyTypes( void ) { return proficiencyTypes; }
	
	idCVar*								GetPasswordCVar( void ) const { return passwordCVar; }

	int									GetNumPlayers( bool excludeBots = false ) const;

	float								GetTotalXP( void ) const;

	const sdDeclToolTip*				GetOOBToolTip( void ) const { return playZoneExitToolTip; }
private:
	idEntity*							PointInRadar( const idVec3& target, radarMasks_t type, const idList< sdRadarLayer* >& list );

private:
	idStr								lookupname;
	const sdDeclLocStr*					title;
	const sdDeclLocStr*					winString;
	const sdDeclLocStr*					winStringSW_Speed;
	const sdDeclLocStr*					winStringSW_XP;
	int									index;
	int									wins;

	const sdDeclLocStr*					highCommandName;

	const sdDeclStringMap*				info;

	idStr								winMusic;

	idEntityPtr< idEntity >				rearSpawnBase;

	playerTeamTypes_t					botTeam;

	idStr								spawnFxName;
	const sdDeclPlayerClass*			defaultClass;

	idStr								crashLandFxName;
	float								crashLandThreshold;
	idStr								parachuteLandFxName;
	float								parachuteLandThreshold;
	
	sdRequirementContainer				playerHealthBoostRequirements;
	int									maxHealthBoost;
	int									healthBoost;

	idCVar*								passwordCVar;
	idCVar*								maxLivesCVar;

	bool								allowRevive;

	idList< sdRadarLayer* >				radarLayers;
	idList< sdRadarLayer* >				jammerLayers;
	idBlockAlloc< sdRadarLayer, 16 >	radarAllocator;

	idScriptObject*						scriptObject;
	sdProgramThread*					scriptThread;
	const sdProgram::sdFunction*		allowRespawnFunc;

	const sdProgram::sdFunction*		teamNextRespawnTimeFunc;

	idList< idEntityPtr< idEntity > >	deployables;
	idList< idEntityPtr< idEntity > >	spawnLocations;

	idList< const sdDeclRating* >		ratings;

	idList< const sdDeclProficiencyType* > proficiencyTypes;

	sdFireTeam*							fireTeams;

	const sdDeclToolTip*				playZoneExitToolTip;

	const sdDeclLocStr*					lifeStatTitle;
};

class sdTeamManagerGameState : public sdEntityStateNetworkData {
public:
							sdTeamManagerGameState( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idList< sdTeamInfoGameState >	teamData;
};

class sdTeamManagerLocal {
public:
								sdTeamManagerLocal( void );
								~sdTeamManagerLocal( void );

public:
	void						Init( void );
	void						OnNewMapLoad( void );
	void						OnNewScriptLoad( void );
	void						OnMapStart( void );
	void						Shutdown( void );
	void						Think( void );

	sdTeamInfo*					GetTeamSafe( const char* name );
	sdTeamInfo&					GetTeam( const char* name );
	sdTeamInfo&					GetTeamByIndex( int index );
	int							GetIndexForTeam( const char* name );
	int							GetNumTeams( void ) const { return teamInfo.Num(); }
	
	void						ApplyNetworkState( const sdEntityStateNetworkData& newState );
	void						ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	void						WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	bool						CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const;
	sdEntityStateNetworkData*	CreateNetworkStructure( void ) const;

	void						OnScriptChange( void );

	void						WriteTeamToStream( sdTeamInfo* newTeam, idFile* file ) const;
	sdTeamInfo*					ReadTeamFromStream( idFile* file ) const;

	void						WriteTeamToStream( sdTeamInfo* newTeam, idBitMsg& msg ) const;
	sdTeamInfo*					ReadTeamFromStream( const idBitMsg& msg ) const;

	void						WriteTeamToStream( sdTeamInfo* oldTeam, sdTeamInfo* newTeam, idBitMsg& msg ) const;
	sdTeamInfo*					ReadTeamFromStream( sdTeamInfo* oldTeam, const idBitMsg& msg ) const;

	sdNetworkStateObject&		GetStateObject( void ) { return stateObject; }

private:
	idList< sdTeamInfo* >		teamInfo;
	int							teamBits;

	sdNetworkStateObject_Generic< sdTeamManagerLocal, &sdTeamManagerLocal::ApplyNetworkState, &sdTeamManagerLocal::ReadNetworkState,
		&sdTeamManagerLocal::WriteNetworkState, &sdTeamManagerLocal::CheckNetworkStateChanges,
		&sdTeamManagerLocal::CreateNetworkStructure > stateObject;
};

typedef sdSingleton< sdTeamManagerLocal > sdTeamManager;

#endif // __GAME_STRUCTURES_TEAMMANAGER_H__
