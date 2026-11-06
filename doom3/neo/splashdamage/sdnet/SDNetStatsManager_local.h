// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSTATSMANAGER_LOCAL_H__ )
#define __SDNETSTATSMANAGER_LOCAL_H__

//===============================================================
//
//	sdNetStatsManager
//
//===============================================================

#if !defined( SD_DEMO_BUILD )

#include "SDNetStatsManager.h"

class sdNetStatsManager_Local : public sdNetStatsManager {
public:
	sdNetStatsManager_Local();
	virtual					~sdNetStatsManager_Local();

	// Write a stats dictionary for a specific client
	virtual bool			WriteDictionary( const sdNetClientId& clientId, const sdNetStatKeyValList& stats );

	// Read the stats back that have already been stored for a specific client ( reconnecting clients )
	virtual bool			ReadCachedDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats );

	//
	// Online functionality
	//

	// Flush pending stats to the master
	virtual sdNetTask*		Flush();

	// Read a stats dictionary from the master
	virtual sdNetTask*		ReadDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats );
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETSTATSMANAGER_LOCAL_H__ */
