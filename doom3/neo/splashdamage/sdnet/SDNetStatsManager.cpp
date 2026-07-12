// Copyright (C) 2007 Id Software, Inc.
//

//===============================================================
//
//	sdNetStatsManager
//
//===============================================================

#if !defined( SD_DEMO_BUILD )

#include "idlib/precompiled.h"

#include "SDNetStatsManager_local.h"
#include "SDNetTask_local.h"

sdNetStatsManager_Local::sdNetStatsManager_Local() {

}

sdNetStatsManager_Local::~sdNetStatsManager_Local() {

}

	// Write a stats dictionary for a specific client
bool sdNetStatsManager_Local::WriteDictionary( const sdNetClientId& clientId, const sdNetStatKeyValList& stats ) {
	return false;
}

	// Read the stats back that have already been stored for a specific client ( reconnecting clients )
bool sdNetStatsManager_Local::ReadCachedDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats ) {
	return false;
}

	//
	// Online functionality
	//

	// Flush pending stats to the master
sdNetTask* sdNetStatsManager_Local::Flush() {
	return new sdNetTask_Flush;
}

	// Read a stats dictionary from the master
sdNetTask* sdNetStatsManager_Local::ReadDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats ) {
	return new sdNetTask_ReadDictionary(clientId, stats);
}

#endif /* !SD_DEMO_BUILD */
