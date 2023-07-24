// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_PROFICIENCY_STATSTRACKER_H__
#define __GAME_PROFICIENCY_STATSTRACKER_H__

#include "../../sdnet/SDNetStatsManager.h"

enum statsRequestState_t {
	SR_EMPTY,
	SR_REQUESTING,
	SR_COMPLETED,
	SR_FAILED,
};

typedef sdHandle< int, -1 > statHandle_t;

class sdPlayerStatEntry {
public:
	struct statValue_t {
	public:
		statValue_t( int value ) {
			data.i = value;
			type = sdNetStatKeyValue::SVT_INT;
		}

		statValue_t( float value ) {
			data.f = value;
			type = sdNetStatKeyValue::SVT_FLOAT;
		}

		int GetInt( void ) const {
			assert( type == sdNetStatKeyValue::SVT_INT );
			return data.i;
		}

		float GetFloat( void ) const {
			assert( type == sdNetStatKeyValue::SVT_FLOAT );
			return data.f;
		}

		sdNetStatKeyValue::statValueType GetType( void ) const {
			return type;
		}

	private:
		sdNetStatKeyValue::statValueType	type;
		sdNetStatKeyValue::valueData_t		data;
	};

								sdPlayerStatEntry( sdNetStatKeyValue::statValueType _type ) : type( _type ) { ; }
	virtual						~sdPlayerStatEntry( void ) { ; }

	virtual void				IncreaseValue( int playerIndex, const statValue_t& value ) = 0;
	virtual void				Clear( int playerIndex ) = 0;
	virtual void				Display( const char* name ) const = 0;
	virtual void				Write( idFile* fp, int playerIndex, const char* name ) const = 0;
	virtual bool				Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const = 0;
	virtual void				SetValue( int playerIndex, const statValue_t& value ) = 0;
	virtual statValue_t			GetValue( int playerIndex ) = 0;
	virtual statValue_t			GetDeltaValue( int playerIndex ) = 0;
	virtual void				SaveBaseLine( int playerIndex ) = 0;
	sdNetStatKeyValue::statValueType GetType( void ) const { return type; }

protected:
	sdNetStatKeyValue::statValueType type;
};

class sdPlayerStatEntry_Integer : public sdPlayerStatEntry {
public:
								sdPlayerStatEntry_Integer( sdNetStatKeyValue::statValueType _type );

	virtual void				IncreaseValue( int playerIndex, const statValue_t& value );
	virtual void				Clear( int playerIndex );
	virtual void				Display( const char* name ) const;
	virtual void				Write( idFile* fp, int playerIndex, const char* name ) const;
	virtual bool				Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const;
	virtual void				SetValue( int playerIndex, const statValue_t& value );
	virtual statValue_t			GetValue( int playerIndex ) { return statValue_t( values[ playerIndex ] ); }
	virtual statValue_t			GetDeltaValue( int playerIndex ) { return statValue_t( values[ playerIndex ] - baseValues[ playerIndex ] ); }
	virtual void				SaveBaseLine( int playerIndex ) { baseValues[ playerIndex ] = values[ playerIndex ]; }

private:
	int							values[ MAX_CLIENTS ];
	int							baseValues[ MAX_CLIENTS ];
};

class sdPlayerStatEntry_Float : public sdPlayerStatEntry {
public:
								sdPlayerStatEntry_Float( sdNetStatKeyValue::statValueType _type );

	virtual void				IncreaseValue( int playerIndex, const statValue_t& value );
	virtual void				Clear( int playerIndex );
	virtual void				Display( const char* name ) const;
	virtual void				Write( idFile* fp, int playerIndex, const char* name ) const;
	virtual bool				Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const;
	virtual void				SetValue( int playerIndex, const statValue_t& value );
	virtual statValue_t			GetValue( int playerIndex ) { return statValue_t( values[ playerIndex ] ); }
	virtual statValue_t			GetDeltaValue( int playerIndex ) { return statValue_t( values[ playerIndex ] - baseValues[ playerIndex ] ); }
	virtual void				SaveBaseLine( int playerIndex ) { baseValues[ playerIndex ] = values[ playerIndex ]; }

private:
	float						values[ MAX_CLIENTS ];
	float						baseValues[ MAX_CLIENTS ];
};

class sdStatsTracker;

class sdStatsCommand {
public:
	virtual						~sdStatsCommand( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args ) = 0;
	virtual void				CommandCompletion( sdStatsTracker& tracker, const idCmdArgs& args, argCompletionCallback_t callback ) { ; }
};

class sdStatsCommand_Request : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_Request( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
};

class sdStatsCommand_Get : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_Get( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
};

class sdStatsCommand_Display : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_Display( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
	virtual void				CommandCompletion( sdStatsTracker& tracker, const idCmdArgs& args, argCompletionCallback_t callback );
};

class sdStatsCommand_ClearUserStats : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_ClearUserStats( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
};

class sdStatsTracker {
public:
	struct lifeStatsData_t {
	public:
										lifeStatsData_t( void ) : oldValue( 0 ), newValue( 0 ) { ; }
		const char*						GetName( void ) const;
		int								index;
		sdPlayerStatEntry::statValue_t	oldValue;
		sdPlayerStatEntry::statValue_t	newValue;
	};

								sdStatsTracker( void );
								~sdStatsTracker( void );
								
	statHandle_t				AllocStat( const char* name, sdNetStatKeyValue::statValueType type );
	statHandle_t				GetStat( const char* name ) const;
	sdPlayerStatEntry*			GetStat( statHandle_t handle ) const;
	void						DisplayStat( statHandle_t handle ) const;
	void						Restore( int playerIndex );
	void						SetStatBaseLine( int playerIndex );

	// Server
	void						CancelStatsRequest( int playerIndex );
	void						AddStatsRequest( int playerIndex );
	void						AcknowledgeStatsReponse( int playerIndex );

	// Client
	void						OnServerStatsRequestMessage( const idBitMsg& msg );
	void						OnServerStatsRequestCancelled( void );
	void						OnServerStatsRequestCompleted( void );

	void						Clear( void );
	void						Clear( int playerIndex );

	int							GetNumStats( void ) const { return stats.Num(); }
	// Gordon: This is only really used internally, so i'm leaving it unsafe, please guard this if you decide to use it externally
	const char*					GetStatName( statHandle_t handle ) const { return stats[ handle ]->GetName(); }

	void						Write( int playerIndex, const char* name );

	bool						StartStatsRequest( bool globalOnly = false );
	void						UpdateStatsRequests( void );

	statsRequestState_t			GetLocalStatsRequestState( void ) const { return requestState; }
	const sdNetStatKeyValList&	GetLocalStats( void ) const { return completeStats; }

	const idHashIndex&			GetLocalStatsHash( void ) const { return completeStatsHash; }
	
	const sdNetStatKeyValue*	GetLocalStat( const char* name ) const;
	const sdNetStatKeyValue*	GetNetStat( const char* name ) const;
	const sdNetStatKeyValue*	GetServerStat( const char* name ) const;

	void						GetTopLifeStats( idList< const lifeStatsData_t* >& improved,
												 idList< const lifeStatsData_t* >& unchanged,
												 idList< const lifeStatsData_t* >& worse ) const;

	static void					Init( void );

	static void					HandleCommand( const idCmdArgs& args );
	static void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback );

	static void					ClearLocalUserStats( sdNetUser* activeUser );
	static void					ReadLocalUserStats( sdNetUser* activeUser, sdNetStatKeyValList& list );

private:
	// Server
	void						ProcessLocalStats( int playerIndex );
	void						ProcessRemoteStats( int playerIndex );
	
	// Client
	void						UpdateStatsRequest( void );
	void						OnServerStatsRequestMessage( const sdNetStatKeyValList& list );
	void						OnStatsRequestFinished( void );

	void						WriteLocalUserStats( sdNetUser* activeUser, int playerIndex );

	sdNetStatKeyValue*			GetLocalStat( const char* name );
	lifeStatsData_t*			GetLifeStatData( const char* name );


	class sdStatEntry {
	public:
								sdStatEntry( const char* _name, sdPlayerStatEntry* _entry ) : name( _name ), entry( _entry ) { ; }
								~sdStatEntry( void ) { delete entry; }

		void					Clear( int playerIndex ) { entry->Clear( playerIndex ); }
		void					Display( void ) const { entry->Display( name ); }
		void					Write( idFile* fp, int playerIndex ) const { entry->Write( fp, playerIndex, name ); }
		bool					Write( sdNetStatKeyValList& kvList, int playerIndex, bool failOnBlank = false ) const { return entry->Write( kvList, playerIndex, name, failOnBlank ); }

		sdPlayerStatEntry*		GetEntry( void ) const { return entry; }
		const char*				GetName( void ) const { return name; }

	private:
		sdPlayerStatEntry*					entry;
		idStr								name;
	};

	sdNetTask*					requestTask;

	bool						requestedStatsValid;
	sdNetStatKeyValList			requestedStats;			// Stats from demonware
	idHashIndex					requestedStatsHash;

	bool						serverStatsValid;
	sdNetStatKeyValList			serverStats;			// Stats from server
	idHashIndex					serverStatsHash;

	sdNetStatKeyValList			completeStats;			// Combined stats
	idHashIndex					completeStatsHash;

	idList< lifeStatsData_t >	lifeStatsData;
	idHashIndex					lifeStatsDataHash;

	int							playerRequestState[ MAX_CLIENTS ];
	bool						playerRequestWaiting[ MAX_CLIENTS ];

	statsRequestState_t			requestState;

	idList< sdStatEntry* >		stats;
	idHashIndex					statHash;

	static idHashMap< sdStatsCommand* >	s_commands;
};

typedef sdSingleton< sdStatsTracker > sdGlobalStatsTracker;

#endif // __GAME_PROFICIENCY_STATSTRACKER_H__
