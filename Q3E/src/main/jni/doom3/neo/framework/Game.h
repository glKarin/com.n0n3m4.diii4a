// karin: for compat with dhewm3
#ifndef __FRAMEWORK_GAME_H__
#define __FRAMEWORK_GAME_H__

#include "../idlib/precompiled.h"

// karin: all `BUILD_NUMBER` macro in game mod source codes replace to this `DHEWM3_BUILD_NUMBER` macro
#define DHEWM3_BUILD_NUMBER 1305

#if defined(_D3XP)
    #include "../d3xp/Game.h"
#else
    #include "../game/Game.h"
#endif

#endif /* !__FRAMEWORK_GAME_H__ */
