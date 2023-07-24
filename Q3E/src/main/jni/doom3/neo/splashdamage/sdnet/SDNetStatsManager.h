// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSTATSMANAGER_H__ )
#define __SDNETSTATSMANAGER_H__

//===============================================================
//
//	sdNetStatsManager
//
//===============================================================

struct sdNetStatKeyValue {
	enum statValueType {
		SVT_INT,
		SVT_FLOAT,
		SVT_INT_MAX,
		SVT_FLOAT_MAX,
	};

	union valueData_t {
		int		i;
		float	f;
	};

	sdNetStatKeyValue( void ) {
		key = NULL;
	}

	sdNetStatKeyValue( const sdNetStatKeyValue& rhs ) {
		key = NULL;
		*this = rhs;
	}

	void operator=( const sdNetStatKeyValue& rhs ) {
		ClearString();

		if ( rhs.key != NULL ) {
			key = rhs.key->GetPool()->AllocString( rhs.key->c_str() );
		} else {
			key = NULL;
		}

		type = rhs.type;
		val = rhs.val;
	}

	~sdNetStatKeyValue( void ) {
		ClearString();
	}

	void ClearString( void ) {
		if ( key != NULL ) {
			key->GetPool()->FreeString( key );
		}
		key = NULL;
	}

	const idPoolStr*	key;
	statValueType		type;
	valueData_t			val;
};

typedef idList< sdNetStatKeyValue > sdNetStatKeyValList;

#if !defined( SD_DEMO_BUILD )

class sdNetTask;
struct sdNetClientId;

/*struct sdNetStatLeaderBoardEntry {
	unsigned int
};*/

class sdNetStatsManager {
public:
	virtual					~sdNetStatsManager() {}

	// Write a stats dictionary for a specific client
	virtual bool			WriteDictionary( const sdNetClientId& clientId, const sdNetStatKeyValList& stats ) = 0;

	// Read the stats back that have already been stored for a specific client ( reconnecting clients )
	virtual bool			ReadCachedDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats ) = 0;

	//
	// Online functionality
	//

	// Flush pending stats to the master
	virtual sdNetTask*		Flush() = 0;

	// Read a stats dictionary from the master
	virtual sdNetTask*		ReadDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats ) = 0;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETSTATSMANAGER_H__ */
