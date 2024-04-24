/*
Copyright (C) 2000-2021 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef COM_GAME_H
#define COM_GAME_H

#include "qdefs.h"

typedef enum gamemode_e
{
	GAME_NORMAL,
	GAME_HIPNOTIC,
	GAME_ROGUE,
	GAME_QUOTH,
	GAME_NEHAHRA,
	GAME_NEXUIZ,
	GAME_XONOTIC,
	GAME_TRANSFUSION,
	GAME_GOODVSBAD2,
	GAME_TEU,
	GAME_BATTLEMECH,
	GAME_ZYMOTIC,
	GAME_SETHERAL,
	GAME_TENEBRAE, // full of evil hackery
	GAME_NEOTERIC,
	GAME_OPENQUARTZ, //this game sucks
	GAME_PRYDON,
	GAME_DELUXEQUAKE,
	GAME_THEHUNTED,
	GAME_DEFEATINDETAIL2,
	GAME_DARSANA,
	GAME_CONTAGIONTHEORY,
	GAME_EDU2P,
	GAME_PROPHECY,
	GAME_BLOODOMNICIDE,
	GAME_STEELSTORM, // added by motorsep
	GAME_STEELSTORM2, // added by motorsep
	GAME_SSAMMO, // added by motorsep
	GAME_STEELSTORMREVENANTS, // added by motorsep 07/19/2015
	GAME_TOMESOFMEPHISTOPHELES, // added by motorsep
	GAME_STRAPBOMB, // added by motorsep for Urre
	GAME_MOONHELM,
	GAME_VORETOURNAMENT,
	GAME_DOOMBRINGER, // added by Cloudwalk for kristus
	GAME_BATTLEMETAL, // added by Cloudwalk for Subject9x
	GAME_COUNT
}
gamemode_t;

// Master switch for some hacks/changes that eventually should become cvars.
#define IS_NEXUIZ_DERIVED(g) ((g) == GAME_NEXUIZ || (g) == GAME_XONOTIC || (g) == GAME_VORETOURNAMENT)
// Pre-csqcmodels era.
#define IS_OLDNEXUIZ_DERIVED(g) ((g) == GAME_NEXUIZ || (g) == GAME_VORETOURNAMENT)

extern gamemode_t gamemode;
extern const char *gamename;
extern const char *gamenetworkfiltername;
extern const char *gamedirname1;
extern const char *gamedirname2;
extern const char *gamescreenshotname;
extern const char *gameuserdirname;
extern char com_modname[MAX_OSPATH];

void COM_InitGameType (void);
void COM_ChangeGameTypeForGameDirs(void);

#endif
