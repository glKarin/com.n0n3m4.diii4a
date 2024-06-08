// DukeCvar.cpp
//

#include "../Gamelib/Game_local.h"

idCVar g_ParentalLock("g_ParentalLock", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "Toggles adult content.");
idCVar g_SwearFrequency("g_SwearFrequency", "100", CVAR_GAME | CVAR_INTEGER, "The lower the less");
