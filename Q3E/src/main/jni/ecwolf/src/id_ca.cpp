// ID_CA.C

// this has been customized for WOLF

/*
=============================================================================

Id Software Caching Manager
---------------------------

Must be started BEFORE the memory manager, because it needs to get the headers
loaded into the data segment

=============================================================================
*/

#include "actor.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "tmemory.h"
#include "wl_def.h"
#include "wl_game.h"
#include "w_wad.h"
#include "wl_main.h"
#include "wl_shade.h"

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

LevelInfo *levelInfo = NULL;
GameMap *map = NULL;


/*
=============================================================================

										CACHE MANAGER ROUTINES

=============================================================================
*/

// Deleter function for our map object. Ensures that any thinkers that may be
// referencing the map are cleaned up first. Note that this can't be done
// during the GameMap object's destructor since there can be more than one
// GameMap object.
void CA_UnloadMap(GameMap *map)
{
	// Flush pending spawn operations. Generally this does nothing, but we want
	// to ensure that defferred operations are cleared.
	AActor::FinishSpawningActors();

	thinkerList.DestroyAll();
	delete map;

	// Don't dangle a reference to the map we just unloaded.
	if(::map == map)
		::map = NULL;
}

/*
======================
=
= CA_CacheMap
=
======================
*/

void CA_CacheMap (const FString &mapname, bool loading)
{
	static TUniquePtr<GameMap, TFuncDeleter<GameMap, CA_UnloadMap> > map;
	map.Reset();

	Printf("\n");

	strncpy(gamestate.mapname, mapname, 8);
	levelInfo = &LevelInfo::Find(mapname);
	::map = map = new GameMap(mapname);
	map->LoadMap(loading);

	Printf("\n%s - %s\n\n", mapname.GetChars(), levelInfo->GetName(map).GetChars());

	CalcVisibility(gLevelVisibility);
}

