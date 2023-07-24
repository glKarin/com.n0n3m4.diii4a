// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PROFICIENCY_PROFICIENCYMANAGER_H__
#define __GAME_PROFICIENCY_PROFICIENCYMANAGER_H__

#include "../../sdnet/SDNetStatsManager.h"

class sdDeclProficiencyItem;
class sdUserInterfaceLocal;
class sdDeclItemPackage;
class idPlayer;
class sdDeclRank;

class sdPersistentRankInfo {
public:
	struct classValue_t {
		float value;
		float max;
	};
	class sdRankInstance {
	public:
		class sdBadge {
		public:
			idList<	classValue_t >	taskValues;
			bool					complete;
		};

		int							completeTasks;
		idList< sdBadge >			badges;
	};

	class sdBadge {
	public:
		sdBadge() : level( 0 ), alwaysAvailable( false ) {

		}
		class sdTask {
		public:
			sdTask( void ) {
				total = 0.f;				
			}

			void Clear( void ) {
				fields.SetNum( 0, false );
			}

			idStrList				fields;
			idStr					text;
			float					total;
		};

		idList< sdTask >			tasks;
		idStr						category;
		idStr						title;
		int							level;
		bool						alwaysAvailable;
	};

	bool							Parse( idParser& src );
	void							Clear( void );
	void							CreateData( const idHashIndex& hash, const sdNetStatKeyValList& list, sdRankInstance& data );
	const idList< sdBadge >&		GetBadges() const { return badges; }

private:
	bool							ParseBadge( idParser& src );
	float							FindData( const char* key, const idHashIndex& hash, const sdNetStatKeyValList& list );

	idList< sdBadge >				badges;
};

class sdProficiencyTable {
public:
	class sdNetworkData {
	public:
									sdNetworkData( void );
		void						MakeDefault( void );

		void						Write( idFile* file ) const;
		void						Read( idFile* file );

	public:
		idList< float >				points;
		idList< float >				basePoints;
		idList< int >				spawnLevels;
		bool						fixedRank;
		int							fixedRankIndex;
	};

						sdProficiencyTable( void );
						~sdProficiencyTable( void );

	void				Init( int _clientNum ) { clientNum = _clientNum; }
	void				SetProficiency( int index, float amount );
	void				AddProficiency( int index, float amount );
	void				Clear( bool all );

	float				GetXP( void ) const { return xp; }
	float				GetPoints( int index ) const { return points[ index ]; }
	float				GetPointsSinceBase( int index ) const { return points[ index ] - basePoints[ index ]; }
	void				ResetToBasePoints( void );
	int					GetLevel( int index ) const { return levels[ index ]; }
	int					GetSpawnLevel( int index ) const { return spawnLevels[ index ]; }
	const sdDeclRank*	GetRank( void ) const { return rank; }
	float				GetPercent( int index ) const;

	void				SetFixedRank( const sdDeclRank*	_rank ) { rank = _rank; fixedRank = true; }
	void				StoreBasePoints( void );
	void				SetSpawnLevels( void );

	void				ApplyNetworkState( const sdNetworkData& newData );
	void				ReadNetworkState( const sdNetworkData& baseData, sdNetworkData& newData, const idBitMsg& msg ) const;
	void				WriteNetworkState( const sdNetworkData& baseData, sdNetworkData& newData, idBitMsg& msg ) const;
	bool				CheckNetworkStateChanges( const sdNetworkData& baseData ) const;

private:
	void				UpdateXP( void );
	void				UpdateLevel( int index );
	void				UpdateLevels( void );
	void				UpdateRank( void );

private:
	int					clientNum;
	idList< float >		points;
	idList< float >		basePoints;		// points at start of map
	idList< int >		levels;
	idList< int >		spawnLevels;	// levels when the player spawned
	float				xp;
	const sdDeclRank*	rank;
	bool				fixedRank;
};

class sdProficiencyManagerLocal {
private:
	static const int MAX_CACHED_PROFICIENCY = 128;
	struct cachedProficiency_t {
		int cachedTime;
		sdNetClientId clientId;
		int ip;
		sdProficiencyTable table;
	};

public:
										sdProficiencyManagerLocal( void );
										~sdProficiencyManagerLocal( void );

public:

	void								GiveMissionProficiency( sdPlayerTask* task, float count );
	void								GiveProficiency( const sdDeclProficiencyItem* item, idPlayer* player, float scale = 1.f, sdPlayerTask* task = NULL, const char* reason = NULL );
	void								GiveProficiency( int index, float count, idPlayer* player, float scale, const char* reason );
	void								CacheProficiency( idPlayer* player );
	void								RestoreProficiency( idPlayer* player );
	void								ClearProficiency( void );
	void								StoreBasePoints( void );
	void								ResetToBasePoints( void );

	void								Init( void );
	void								Shutdown( void );

	void								DumpProficiencyData( void );
	void								LogProficiency( const char* name, float count );

	static bool							ReadRankInfo( sdPersistentRankInfo& rankInfo );

private:
	cachedProficiency_t&				FindFreeCacheSlot( void );
	cachedProficiency_t*				FindCachedProficiency( idPlayer* player );
	void								RemoveCacheEntry( cachedProficiency_t* entry );

	struct proficiencyData_t {
		idStr name;
		int count;
		float total;
	};

	idHashIndex							loggedDataHash;
	idList< proficiencyData_t >			loggedDataList;

	idStaticList< cachedProficiency_t, MAX_CACHED_PROFICIENCY > cachedTables;
};

typedef sdSingleton< sdProficiencyManagerLocal > sdProficiencyManager;


#endif // __GAME_PROFICIENCY_PROFICIENCYMANAGER_H__
