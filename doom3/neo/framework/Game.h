/**
 * karin: This header file for compat with dhewm3, because dhewm3 is not upstream of idTech4A++, but idTech4A++ used dhewm3's some features.
 * For some reason, idTech4A++ not compat dhewm3, but idTech4A++ provide some compat source file for compile dhewm3's mods, just modify little source code, can run on idTech4A++.
 * If you want to port dhewm3's mod, need edit idlib/precompiled.h, and include this file(framework/Game.h), and not XXX_game/Game.h
 */
#ifndef __FRAMEWORK_GAME_H__
#define __FRAMEWORK_GAME_H__

#include "../idlib/precompiled.h"

/**
 * karin: all `BUILD_NUMBER` macro in game mod source codes replace to this `DHEWM3_BUILD_NUMBER` macro.
 * Because Dhewm3's mod check savegame's BUILD_NUMBER >= 1305, but DOOM3 original is 1304
 * Need edit these source file:
 * gamesys/SaveGame.cpp: idGameLocal::SaveGame( idFile *f )
 * Game_local.cpp: idSaveGame::WriteBuildNumber( const int value )
 */
#define DHEWM3_BUILD_NUMBER 1305

/**
 * karin: Dhewm3's API version is 9, DOOM3 original is 8
 * But not need change it.
 */
#define DHEWM3_GAME_API_VERSION 9

#if defined(_D3XP)
    #include "../d3xp/Game.h"
#else
    #include "../game/Game.h"
#endif

#endif /* !__FRAMEWORK_GAME_H__ */
