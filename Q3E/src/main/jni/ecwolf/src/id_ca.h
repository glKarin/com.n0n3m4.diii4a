#ifndef __ID_CA__
#define __ID_CA__

//===========================================================================

extern  class GameMap *map;
extern	class LevelInfo *levelInfo;

//===========================================================================

void CA_CacheMap (const class FString &mapname, bool loading);

void CA_CacheScreen (class FTexture *tex, bool noaspect=false);

#endif
