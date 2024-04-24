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

#include "darkplaces.h"
#include "com_game.h"

// Game mods

gamemode_t gamemode;
const char *gamename;
const char *gamenetworkfiltername; // same as gamename currently but with _ in place of spaces so that "getservers" packets parse correctly (this also means the 
const char *gamedirname1;
const char *gamedirname2;
const char *gamescreenshotname;
const char *gameuserdirname;
char com_modname[MAX_OSPATH] = "";

gamemode_t com_startupgamemode;
gamemode_t com_startupgamegroup;

typedef struct gamemode_info_s
{
	gamemode_t mode; // this gamemode
	gamemode_t group; // different games with same group can switch automatically when gamedirs change
	const char* prog_name; // not null
	const char* cmdline; // not null
	const char* gamename; // not null
	const char*	gamenetworkfiltername; // not null
	const char* gamedirname1; // not null
	const char* gamedirname2; // null
	const char* gamescreenshotname; // not nul
	const char* gameuserdirname; // not null
} gamemode_info_t;

static const gamemode_info_t gamemode_info [GAME_COUNT] =
{// game						basegame					prog_name				cmdline						gamename					gamenetworkfilername		basegame	modgame			screenshot			userdir					   // commandline option
{ GAME_NORMAL,					GAME_NORMAL,				"",						"-quake",					"DarkPlaces-Quake",			"DarkPlaces-Quake",			"id1",		NULL,			"dp",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -quake runs the game Quake (default)
{ GAME_HIPNOTIC,				GAME_NORMAL,				"hipnotic",				"-hipnotic",				"Darkplaces-Hipnotic",		"Darkplaces-Hipnotic",		"id1",		"hipnotic",		"dp",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -hipnotic runs Quake mission pack 1: The Scourge of Armagon
{ GAME_ROGUE,					GAME_NORMAL,				"rogue",				"-rogue",					"Darkplaces-Rogue",			"Darkplaces-Rogue",			"id1",		"rogue",		"dp",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -rogue runs Quake mission pack 2: The Dissolution of Eternity
{ GAME_NEHAHRA,					GAME_NORMAL,				"nehahra",				"-nehahra",					"DarkPlaces-Nehahra",		"DarkPlaces-Nehahra",		"id1",		"nehahra",		"dp",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -nehahra runs The Seal of Nehahra movie and game
{ GAME_QUOTH,					GAME_NORMAL,				"quoth",				"-quoth",					"Darkplaces-Quoth",			"Darkplaces-Quoth",			"id1",		"quoth",		"dp",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -quoth runs the Quoth mod for playing community maps made for it
{ GAME_NEXUIZ,					GAME_NEXUIZ,				"nexuiz",				"-nexuiz",					"Nexuiz",					"Nexuiz",					"data",		NULL,			"nexuiz",			"nexuiz"				}, // COMMANDLINEOPTION: Game: -nexuiz runs the multiplayer game Nexuiz
{ GAME_XONOTIC,					GAME_XONOTIC,				"xonotic",				"-xonotic",					"Xonotic",					"Xonotic",					"data",		NULL,			"xonotic",			"xonotic"				}, // COMMANDLINEOPTION: Game: -xonotic runs the multiplayer game Xonotic
{ GAME_TRANSFUSION,				GAME_TRANSFUSION,			"transfusion",			"-transfusion",				"Transfusion",				"Transfusion",				"basetf",	NULL,			"transfusion",		"transfusion"			}, // COMMANDLINEOPTION: Game: -transfusion runs Transfusion (the recreation of Blood in Quake)
{ GAME_GOODVSBAD2,				GAME_GOODVSBAD2,			"gvb2",					"-goodvsbad2",				"GoodVs.Bad2",				"GoodVs.Bad2",				"rts",		NULL,			"gvb2",				"gvb2"					}, // COMMANDLINEOPTION: Game: -goodvsbad2 runs the psychadelic RTS FPS game Good Vs Bad 2
{ GAME_TEU,						GAME_TEU,					"teu",					"-teu",						"TheEvilUnleashed",			"TheEvilUnleashed",			"baseteu",	NULL,			"teu",				"teu"					}, // COMMANDLINEOPTION: Game: -teu runs The Evil Unleashed (this option is obsolete as they are not using darkplaces)
{ GAME_BATTLEMECH,				GAME_BATTLEMECH,			"battlemech",			"-battlemech",				"Battlemech",				"Battlemech",				"base",		NULL,			"battlemech",		"battlemech"			}, // COMMANDLINEOPTION: Game: -battlemech runs the multiplayer topdown deathmatch game BattleMech
{ GAME_ZYMOTIC,					GAME_ZYMOTIC,				"zymotic",				"-zymotic",					"Zymotic",					"Zymotic",					"basezym",	NULL,			"zymotic",			"zymotic"				}, // COMMANDLINEOPTION: Game: -zymotic runs the singleplayer game Zymotic
{ GAME_SETHERAL,				GAME_SETHERAL,				"setheral",				"-setheral",				"Setheral",					"Setheral",					"data",		NULL,			"setheral",			"setheral"				}, // COMMANDLINEOPTION: Game: -setheral runs the multiplayer game Setheral
{ GAME_TENEBRAE,				GAME_NORMAL,				"tenebrae",				"-tenebrae",				"DarkPlaces-Tenebrae",		"DarkPlaces-Tenebrae",		"id1",		"tenebrae",		"dp",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -tenebrae runs the graphics test mod known as Tenebrae (some features not implemented)
{ GAME_NEOTERIC,				GAME_NORMAL,				"neoteric",				"-neoteric",				"Neoteric",					"Neoteric",					"id1",		"neobase",		"neo",				"darkplaces"			}, // COMMANDLINEOPTION: Game: -neoteric runs the game Neoteric
{ GAME_OPENQUARTZ,				GAME_NORMAL,				"openquartz",			"-openquartz",				"OpenQuartz",				"OpenQuartz",				"id1",		NULL,			"openquartz",		"darkplaces"			}, // COMMANDLINEOPTION: Game: -openquartz runs the game OpenQuartz, a standalone GPL replacement of the quake content
{ GAME_PRYDON,					GAME_NORMAL,				"prydon",				"-prydon",					"PrydonGate",				"PrydonGate",				"id1",		"prydon",		"prydon",			"darkplaces"			}, // COMMANDLINEOPTION: Game: -prydon runs the topdown point and click action-RPG Prydon Gate
{ GAME_DELUXEQUAKE,				GAME_DELUXEQUAKE,			"dq",					"-dq",						"Deluxe Quake",				"Deluxe_Quake",				"basedq",	"extradq",		"basedq",			"dq"					}, // COMMANDLINEOPTION: Game: -dq runs the game Deluxe Quake
{ GAME_THEHUNTED,				GAME_THEHUNTED,				"thehunted",			"-thehunted",				"The Hunted",				"The_Hunted",				"thdata",	NULL, 			"th",				"thehunted"				}, // COMMANDLINEOPTION: Game: -thehunted runs the game The Hunted
{ GAME_DEFEATINDETAIL2,			GAME_DEFEATINDETAIL2,		"did2",					"-did2",					"Defeat In Detail 2",		"Defeat_In_Detail_2",		"data",		NULL, 			"did2_",			"did2"					}, // COMMANDLINEOPTION: Game: -did2 runs the game Defeat In Detail 2
{ GAME_DARSANA,					GAME_DARSANA,				"darsana",				"-darsana",					"Darsana",					"Darsana",					"ddata",	NULL, 			"darsana",			"darsana"				}, // COMMANDLINEOPTION: Game: -darsana runs the game Darsana
{ GAME_CONTAGIONTHEORY,			GAME_CONTAGIONTHEORY,		"contagiontheory",		"-contagiontheory",			"Contagion Theory",			"Contagion_Theory",			"ctdata",	NULL, 			"ct",				"contagiontheory"		}, // COMMANDLINEOPTION: Game: -contagiontheory runs the game Contagion Theory
{ GAME_EDU2P,					GAME_EDU2P,					"edu2p",				"-edu2p",					"EDU2 Prototype",			"EDU2_Prototype",			"id1",		"edu2",			"edu2_p",			"edu2prototype"			}, // COMMANDLINEOPTION: Game: -edu2p runs the game Edu2 prototype
{ GAME_PROPHECY,				GAME_PROPHECY,				"prophecy",				"-prophecy",				"Prophecy",					"Prophecy",					"gamedata",	NULL,			"phcy",				"prophecy"				}, // COMMANDLINEOPTION: Game: -prophecy runs the game Prophecy
{ GAME_BLOODOMNICIDE,			GAME_BLOODOMNICIDE,			"omnicide",				"-omnicide",				"Blood Omnicide",			"Blood_Omnicide",			"kain",		NULL,			"omnicide",			"omnicide"				}, // COMMANDLINEOPTION: Game: -omnicide runs the game Blood Omnicide
{ GAME_STEELSTORM,				GAME_STEELSTORM,			"steelstorm",			"-steelstorm",				"Steel-Storm",				"Steel-Storm",				"gamedata",	NULL,			"ss",				"steelstorm"			}, // COMMANDLINEOPTION: Game: -steelstorm runs the game Steel Storm
{ GAME_STEELSTORM2,				GAME_STEELSTORM2,			"steelstorm2",			"-steelstorm2",				"Steel Storm 2",			"Steel_Storm_2",			"gamedata",	NULL,			"ss2",				"steelstorm2"			}, // COMMANDLINEOPTION: Game: -steelstorm2 runs the game Steel Storm 2
{ GAME_SSAMMO,					GAME_SSAMMO,				"steelstorm-ammo",		"-steelstormammo",			"Steel Storm A.M.M.O.",		"Steel_Storm_A.M.M.O.",		"gamedata", NULL,			"ssammo",			"steelstorm-ammo"		}, // COMMANDLINEOPTION: Game: -steelstormammo runs the game Steel Storm A.M.M.O.
{ GAME_STEELSTORMREVENANTS,		GAME_STEELSTORMREVENANTS,	"steelstorm-revenants", "-steelstormrev",			"Steel Storm: Revenants",	"Steel_Storm_Revenants",	"base", NULL,				"ssrev",			"steelstorm-revenants"	}, // COMMANDLINEOPTION: Game: -steelstormrev runs the game Steel Storm: Revenants
{ GAME_TOMESOFMEPHISTOPHELES,	GAME_TOMESOFMEPHISTOPHELES,	"tomesofmephistopheles","-tomesofmephistopheles",	"Tomes of Mephistopheles",	"Tomes_of_Mephistopheles",	"gamedata",	NULL,			"tom",				"tomesofmephistopheles"	}, // COMMANDLINEOPTION: Game: -tomesofmephistopheles runs the game Tomes of Mephistopheles
{ GAME_STRAPBOMB,				GAME_STRAPBOMB,				"strapbomb",			"-strapbomb",				"Strap-on-bomb Car",		"Strap-on-bomb_Car",		"id1",		NULL,			"strap",			"strapbomb"				}, // COMMANDLINEOPTION: Game: -strapbomb runs the game Strap-on-bomb Car
{ GAME_MOONHELM,				GAME_MOONHELM,				"moonhelm",				"-moonhelm",				"MoonHelm",					"MoonHelm",					"data",		NULL,			"mh",				"moonhelm"				}, // COMMANDLINEOPTION: Game: -moonhelm runs the game MoonHelm
{ GAME_VORETOURNAMENT,			GAME_VORETOURNAMENT,		"voretournament",		"-voretournament",			"Vore Tournament",			"Vore_Tournament",			"data",		NULL,			"voretournament",	"voretournament"		}, // COMMANDLINEOPTION: Game: -voretournament runs the multiplayer game Vore Tournament
{ GAME_DOOMBRINGER,				GAME_DOOMBRINGER,			"doombringer",			"-doombringer",				"DOOMBRINGER",				"DOOMBRINGER",				"dbdata",	NULL,			"doombringer",		"doombringer"			}, // COMMANDLINEOPTION: Game: -doombringer runs the game DOOMBRINGER
{ GAME_BATTLEMETAL,				GAME_NORMAL,				"battlemetal",			"-battlemetal",				"battlemetal",				"battlemetal",				"metaldata",	NULL,		"battlemetal",		"battlemetal"			}, // COMMANDLINEOPTION: Game: -battlemetal runs the game battleMETAL
};

static void COM_SetGameType(int index);
void COM_InitGameType (void)
{
	char name [MAX_OSPATH];
	int i;
	int index = 0;

#ifdef FORCEGAME
	COM_ToLowerString(FORCEGAME, name, sizeof (name));
#else
	// check executable filename for keywords, but do it SMARTLY - only check the last path element
	FS_StripExtension(FS_FileWithoutPath(sys.argv[0]), name, sizeof (name));
	COM_ToLowerString(name, name, sizeof (name));
#endif
	for (i = 1;i < (int)(sizeof (gamemode_info) / sizeof (gamemode_info[0]));i++)
		if (gamemode_info[i].prog_name && gamemode_info[i].prog_name[0] && strstr (name, gamemode_info[i].prog_name))
			index = i;

	// check commandline options for keywords
	for (i = 0;i < (int)(sizeof (gamemode_info) / sizeof (gamemode_info[0]));i++)
		if (Sys_CheckParm (gamemode_info[i].cmdline))
			index = i;

	com_startupgamemode = gamemode_info[index].mode;
	com_startupgamegroup = gamemode_info[index].group;
	COM_SetGameType(index);
}

void COM_ChangeGameTypeForGameDirs(void)
{
	unsigned i, gamemode_count = sizeof(gamemode_info) / sizeof(gamemode_info[0]);
	int index = -1;
	// this will not not change the gamegroup

	// first check if a base game (single gamedir) matches
	for (i = 0; i < gamemode_count; i++)
	{
		if (gamemode_info[i].group == com_startupgamegroup && !(gamemode_info[i].gamedirname2 && gamemode_info[i].gamedirname2[0]))
		{
			index = i;
			break;
		}
	}
	// now that we have a base game, see if there is a derivative game matching the startup one (two gamedirs)
	// bones_was_here: this prevents a Quake expansion (eg Rogue) getting switched to Quake,
	// and its gamedirname2 (eg "rogue") being lost from the search path, when adding a miscellaneous gamedir.
	for (i = 0; i < gamemode_count; i++)
	{
		if (gamemode_info[i].group == com_startupgamegroup && (gamemode_info[i].gamedirname2 && gamemode_info[i].gamedirname2[0]) && gamemode_info[i].mode == com_startupgamemode)
		{
			index = i;
			break;
		}
	}
	// also see if the first gamedir (from -game parm or gamedir command) matches a derivative game (two/three gamedirs)
	if (fs_numgamedirs)
	{
		for (i = 0; i < gamemode_count; i++)
		{
			if (gamemode_info[i].group == com_startupgamegroup && (gamemode_info[i].gamedirname2 && gamemode_info[i].gamedirname2[0]) && !strcasecmp(fs_gamedirs[0], gamemode_info[i].gamedirname2))
			{
				index = i;
				break;
			}
		}
	}
	// we now have a good guess at which game this is meant to be...
	if (index >= 0 && gamemode != gamemode_info[index].mode)
		COM_SetGameType(index);
}

static void COM_SetGameType(int index)
{
	static char gamenetworkfilternamebuffer[64];
	int i, t;
	if (index < 0 || index >= (int)(sizeof (gamemode_info) / sizeof (gamemode_info[0])))
		index = 0;
	gamemode = gamemode_info[index].mode;
	gamename = gamemode_info[index].gamename;
	gamenetworkfiltername = gamemode_info[index].gamenetworkfiltername;
	gamedirname1 = gamemode_info[index].gamedirname1;
	gamedirname2 = gamemode_info[index].gamedirname2;
	gamescreenshotname = gamemode_info[index].gamescreenshotname;
	gameuserdirname = gamemode_info[index].gameuserdirname;

	if (gamemode == com_startupgamemode)
	{
		if((t = Sys_CheckParm("-customgamename")) && t + 1 < sys.argc)
			gamename = gamenetworkfiltername = sys.argv[t+1];
		if((t = Sys_CheckParm("-customgamenetworkfiltername")) && t + 1 < sys.argc)
			gamenetworkfiltername = sys.argv[t+1];
		if((t = Sys_CheckParm("-customgamedirname1")) && t + 1 < sys.argc)
			gamedirname1 = sys.argv[t+1];
		if((t = Sys_CheckParm("-customgamedirname2")) && t + 1 < sys.argc)
			gamedirname2 = *sys.argv[t+1] ? sys.argv[t+1] : NULL;
		if((t = Sys_CheckParm("-customgamescreenshotname")) && t + 1 < sys.argc)
			gamescreenshotname = sys.argv[t+1];
		if((t = Sys_CheckParm("-customgameuserdirname")) && t + 1 < sys.argc)
			gameuserdirname = sys.argv[t+1];
	}

	if (gamedirname2 && gamedirname2[0])
		Con_Printf("Game is %s using base gamedirs %s %s", gamename, gamedirname1, gamedirname2);
	else
		Con_Printf("Game is %s using base gamedir %s", gamename, gamedirname1);
	for (i = 0;i < fs_numgamedirs;i++)
	{
		if (i == 0)
			Con_Printf(", with mod gamedirs");
		Con_Printf(" %s", fs_gamedirs[i]);
	}
	Con_Printf("\n");

	if (strchr(gamenetworkfiltername, ' '))
	{
		char *s;
		// if there are spaces in the game's network filter name it would
		// cause parse errors in getservers in dpmaster, so we need to replace
		// them with _ characters
		dp_strlcpy(gamenetworkfilternamebuffer, gamenetworkfiltername, sizeof(gamenetworkfilternamebuffer));
		while ((s = strchr(gamenetworkfilternamebuffer, ' ')) != NULL)
			*s = '_';
		gamenetworkfiltername = gamenetworkfilternamebuffer;
	}

	Con_Printf("gamename for server filtering: %s\n", gamenetworkfiltername);
}
