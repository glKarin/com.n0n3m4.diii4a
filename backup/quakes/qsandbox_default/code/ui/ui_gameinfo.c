// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// gameinfo.c
//

#include "ui_local.h"

//
// arena and bot info
//

#define DIRLIST_SIZE 	16384

#define MAX_MAPNAME 32

int				ui_numBots;
static char		*ui_botInfos[MAX_BOTS];

static int		ui_numArenas;
static char		*ui_arenaInfos[MAX_ARENAS];

static int		ui_numSinglePlayerArenas;
static int		ui_numSpecialSinglePlayerArenas;

static char		dirlist[DIRLIST_SIZE];
static int		allocPoint, outOfMemory;

#define POOLSIZE ( 1024 * 1024 ) * 32       //QVM_MEMORY note: use 16 for 32bit
static char		memoryPool[POOLSIZE];

/*
===============
UI_Alloc
===============
*/
void *UI_Alloc( int size ) {
	char	*p;

	if ( allocPoint + size > POOLSIZE ) {
		outOfMemory = qtrue;
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

/*
===============
UI_Free
===============
*/
void UI_Free(void *ptr) {
	// Определение смещения указателя от начала пула памяти
    int offset = (char*)ptr - memoryPool;
	
    // Ничего не делать, если указатель равен NULL
    if (ptr == NULL)
        return;

    // Очистка области памяти путем установки ее байтов в 0
    memset(ptr, 0, offset);

    // Сдвиг allocPoint, чтобы освободить эту область памяти для будущего использования
    allocPoint = offset;
}

/*
===============
UI_InitMemory
===============
*/
void UI_InitMemory( void ) {
	allocPoint = 0;
	outOfMemory = qfalse;
}

/*
===============
UI_StoreMapInfo
===============
*/
qboolean UI_StoreInfo( char *info, char **infos )
{
	//NOTE: extra space for arena number
	*infos = UI_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
	if (*infos) {
		strcpy(*infos, info);
		return qtrue;
	}

	return qfalse;
}

/*
===============
UI_ParseInfos
===============
*/
int UI_ParseInfos( char *buf, int max, char *infos[] ) {
	fileHandle_t file;
	const char* filename;
	int len;
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}

		// Hypo: check arena actually exists as a .bsp file
		// since bots also use this code, we don't want to exclude them!
		filename = Info_ValueForKey(info, "map");
		if (filename[0])
		{
			len = trap_FS_FOpenFile(va("maps/%s.bsp", filename), &file, FS_READ);
			if (len <= 0)
			{
//				trap_Print(va("Map not found (ignored): %s, code=%i\n", filename, len));
				continue;
			}

			trap_FS_FCloseFile(file);
//			trap_Print(va("Map Found: %s\n", filename));
		}

		if (UI_StoreInfo(info, &infos[count]))
			count++;
	}

	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		//trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas] );
}

/*
===============
UI_LoadUnscriptedMaps
===============
*/
static void UI_LoadUnscriptedMaps( void )
{
	int i;
	int dirlen;
	int nummaps;
	char* dirptr;

	nummaps = trap_FS_GetFileList("maps", ".bsp", dirlist, DIRLIST_SIZE );
	dirptr = dirlist;
	for (i = 0; i < nummaps;  i++, dirptr+= dirlen + 1) {
		if (ui_numArenas == MAX_ARENAS)
			break;

		dirlen = strlen(dirptr);

		COM_StripExtensionOld(dirptr, dirptr);

		if (UI_GetArenaInfoByMap( dirptr ))
			continue;

		if (UI_StoreInfo(va("\\map\\%s", dirptr), &ui_arenaInfos[ui_numArenas])) {
			trap_Print(va("Found unscripted map: %s\n", dirptr));
			ui_numArenas++;
		}
	}
}

/*
===============
GameInfo_RemoveSingleFromArena

The replacement value is always the same length or shorter, so
we won't have problems with overruns in UI_Alloc()
===============
*/
static void GameInfo_RemoveSingleFromArena( char* info )
{
	char newvalue[MAX_INFO_STRING];
	char* value;
	char* single;
	char *p, *q;

	value = Info_ValueForKey(info, "type");
	if (!value[0])
		return;

	single = strstr(value, "single");
	if (!single)
		return;

	p = newvalue;
	q = value;
	while (qtrue) {
		if (q == single) {
			q += strlen("single");
			while (*q == ' ')
				q++;
		}

		*p = *q;

		if (!*p)
			break;
		p++;
		q++;
	};

//	trap_Print(va("replacement value: %s\n", newvalue));
	Info_SetValueForKey(info, "type", newvalue);
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas( void ) {
	int			numdirs;
	vmCvar_t	arenasFile;
	char		filename[128];
	char*		dirptr;
	int			i, j, n;
	int			dirlen;
	char		*type;
	char		*tag;
	int			singlePlayerNum, specialNum, otherNum;
//	char		tmpinfo[MAX_INFO_STRING];
	char*		tmpinfo;
	int			startArenaScript;
	int			swap;
	char 		bestMap[MAX_MAPNAME];
	char*		thisMap;

	ui_numArenas = 0;

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM );
	if( *arenasFile.string ) {
		UI_LoadArenasFromFile(arenasFile.string);
	}
	else {
		UI_LoadArenasFromFile("scripts/arenas.txt");
	}

	startArenaScript = ui_numArenas;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, DIRLIST_SIZE );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}

	UI_LoadUnscriptedMaps();

	trap_Print( va( "%i arenas parsed\n", ui_numArenas ) );

	if (outOfMemory) trap_Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all arenas\n");

//	trap_Print(va("Sort: %i to %i\n", startArenaScript, ui_numArenas));

	// sort the arenas we loaded from the .arena files, by mapname
	// we leave the original Id levels (and those in an overriding
	// arenas.txt) unchanged
	// not the most efficient sorting method, but we're only going to do it once
	for (i = startArenaScript - 1; i < ui_numArenas - 1; i++) {
		swap = 0;
		Q_strncpyz(bestMap, Info_ValueForKey(ui_arenaInfos[i],"map"), MAX_MAPNAME);

		if (!bestMap[0])
			continue;

		for (j = i + 1; j < ui_numArenas; j++) {
			thisMap = Info_ValueForKey(ui_arenaInfos[j], "map");

			if (!thisMap[0])
				continue;

			if (Q_stricmp(thisMap, bestMap) < 0) {
				Q_strncpyz(bestMap, thisMap, MAX_MAPNAME);
				swap = j;
			}
		}

		if (swap) {
//			trap_Print(va("swap: %s with %s\n", bestMap, Info_ValueForKey(ui_arenaInfos[i], "map")));
			tmpinfo = ui_arenaInfos[i];
			ui_arenaInfos[i] = ui_arenaInfos[swap];
			ui_arenaInfos[swap] = tmpinfo;
		}
	}

	// set initial numbers
	for( n = 0; n < ui_numArenas; n++ ) {
		Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", n ) );
	}

	// go through and count single players levels
	ui_numSinglePlayerArenas = 0;
	ui_numSpecialSinglePlayerArenas = 0;
	for( n = 0; n < ui_numArenas; n++ ) {
		// determine type
		type = Info_ValueForKey( ui_arenaInfos[n], "type" );

		// if no type specified, it will be treated as "ffa"
		if( !*type ) {
			continue;
		}

		if( strstr( type, "single" ) ) {
			// check for special single player arenas (training, final)
			tag = Info_ValueForKey( ui_arenaInfos[n], "special" );
			if( *tag ) {
				ui_numSpecialSinglePlayerArenas++;
				continue;
			}

			ui_numSinglePlayerArenas++;
		}
	}

	n = ui_numSinglePlayerArenas % ARENAS_PER_TIER;
	if( n != 0 ) {
		ui_numSinglePlayerArenas -= n;
		trap_Print( va( "%i arenas ignored to make count divisible by %i\n", n, ARENAS_PER_TIER ) );
	}

	// go through once more and assign number to the levels
	singlePlayerNum = 0;
	specialNum = singlePlayerNum + ui_numSinglePlayerArenas;
	otherNum = specialNum + ui_numSpecialSinglePlayerArenas;
	for( n = 0; n < ui_numArenas; n++ ) {
		// determine type
		type = Info_ValueForKey( ui_arenaInfos[n], "type" );

		// if no type specified, it will be treated as "ffa"
		if( *type ) {
			if( strstr( type, "single" ) ) {
				// check for special single player arenas (training, final)
				tag = Info_ValueForKey( ui_arenaInfos[n], "special" );
				if( *tag ) {
					Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", specialNum++ ) );
					continue;
				}

				// if some arenas were ignored (above), then we bypass giving them
				// a single player number, and give them an otherNum instead.
				// Id maps will always grab the single player values first because
				// they're in the legacy arenas.txt. Other multiples of ARENAS_PER_TIER
				// will also get through, so we can still add in tier packs.
				// Unfortunately, 1 map pk3's (like halven.pk3) with the "single" tag set
				// might "push" off one of the tier maps... there's nothing we can
				// do about this except provide an over-riding .arena script
				// that removes the "single" tag
				if (singlePlayerNum < ui_numSinglePlayerArenas) {
					Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", singlePlayerNum++ ) );
					continue;
				}
				else {
					trap_Print(va("..single player map \"%s\" dropped\n", Info_ValueForKey(ui_arenaInfos[n], "map")));
					GameInfo_RemoveSingleFromArena(ui_arenaInfos[n]);
				}
			}
		}	/* (*type) */

		Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", otherNum++ ) );

	}
}

/*
===============
UI_GetArenaInfoByNumber
===============
*/
const char *UI_GetArenaInfoByNumber( int num ) {
	int		n;
	char	*value;

	if( num < 0 || num >= ui_numArenas ) {
		trap_Print( va( S_COLOR_RED "Invalid arena number: %i\n", num ) );
		return NULL;
	}

	for( n = 0; n < ui_numArenas; n++ ) {
		value = Info_ValueForKey( ui_arenaInfos[n], "num" );
		if( *value && atoi(value) == num ) {
			return ui_arenaInfos[n];
		}
	}

	return NULL;
}

/*
===============
UI_GetArenaInfoByMap
===============
*/
const char *UI_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < ui_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( ui_arenaInfos[n], "map" ), map ) == 0 ) {
			return ui_arenaInfos[n];
		}
	}

	return NULL;
}

/*
===============
UI_GetSpecialArenaInfo
===============
*/
const char *UI_GetSpecialArenaInfo( const char *tag ) {
	int			n;

	for( n = 0; n < ui_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( ui_arenaInfos[n], "special" ), tag ) == 0 ) {
			return ui_arenaInfos[n];
		}
	}

	return NULL;
}

/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		//trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ui_numBots += UI_ParseInfos( buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots] );

	if (outOfMemory) trap_Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all bots\n");
}

/*
===============
UI_LoadBots
===============
*/
void UI_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[128];
//	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	ui_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		UI_LoadBotsFromFile(botsFile.string);
	}
	else {
		UI_LoadBotsFromFile("scripts/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, DIRLIST_SIZE );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadBotsFromFile(filename);
	}
	trap_Print( va( "%i bots parsed\n", ui_numBots ) );
}

/*
===============
UI_GetBotInfoByNumber
===============
*/
char *UI_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= ui_numBots ) {
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return ui_botInfos[num];
}

/*
===============
UI_GetBotInfoByName
===============
*/
char *UI_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < ui_numBots ; n++ ) {
		value = Info_ValueForKey( ui_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

/*
===============
UI_GetNumBots
===============
*/
int UI_GetNumBots( void ) {
	return ui_numBots;
}

/*
===============
UI_GetBotNumByName
===============
*/
int UI_GetBotNumByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < ui_numBots ; n++ ) {
		value = Info_ValueForKey( ui_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return n;
		}
	}

	return -1;
}

char *UI_GetBotNameByNumber( int num ) {
	char *info = UI_GetBotInfoByNumber(num);
	if (info) {
		return Info_ValueForKey( info, "name" );
	}
	return "Sarge";
}

/*
===============
UI_GetNumArenas
===============
*/
int UI_GetNumArenas( void ) {
	return ui_numArenas;
}

/*
===============
UI_InitGameinfo
===============
*/
void UI_InitGameinfo( void ) {

	UI_InitMemory();
	UI_LoadArenas();
	UI_LoadBots();

	if( (trap_Cvar_VariableValue( "fs_restrict" )) || (ui_numSpecialSinglePlayerArenas == 0 && ui_numSinglePlayerArenas == 4) ) {
		uis.demoversion = qtrue;
	}
	else {
		uis.demoversion = qfalse;
	}
}



