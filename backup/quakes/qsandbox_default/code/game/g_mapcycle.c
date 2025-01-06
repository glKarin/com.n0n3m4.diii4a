/*
===========================================================================
Copyright (C) 2010-2011 Manuel Wiese
Copyright (C) 2018 borg

This file is part of AfterShock source code.

AfterShock source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

AfterShock source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AfterShock source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

#define MAX_MAPCYCLECOUNT 128
#define MAX_MAPCYCLELENGTH 4096
#define MAX_MAPCYCLETOKENS 512

typedef struct mapcycle_s {
	char *maps[MAX_MAPCYCLECOUNT];
	int minplayers[MAX_MAPCYCLECOUNT];
	int maxplayers[MAX_MAPCYCLECOUNT];
	int lockarena[MAX_MAPCYCLECOUNT];
	int  mapcycleCount;
	char *allowedMaps[MAX_MAPCYCLECOUNT];
	int  allowedMapsCount;
	qboolean allAllowed;
	char mapfiles[MAX_MAPCYCLECOUNT][MAX_QPATH];
	char allowedmapfiles[MAX_MAPCYCLECOUNT][MAX_QPATH];
} mapcycle_t;

mapcycle_t mapcycle;

typedef enum {
	TOT_LPAREN,
	TOT_RPAREN,
	TOT_WORD,
	TOT_NUMBER,
	TOT_NIL,
	TOT_MAX
} tokenType_t;

#define TOKENVALUE_SIZE 64

typedef struct {
	char value[TOKENVALUE_SIZE];
	int type;
} token_t;

static int G_setTokens ( char* in, char* out, int start ) {
	int i = 0;
	while ( in[ start + i ] != ' ' ) {
		if ( in[ start + i ] == '\0' ) {
			out[i] = in[start+1];
			return MAX_MAPCYCLELENGTH;
		}
		out[i] = in[start+i];
		i++;
	}
	out[i] = '\0';
	return start+i+1;
}

/*
=================
G_setTokenType
Reads the string and gives out the type of the token
=================
*/
static int G_setTokenType ( char* value ) {
	int count = 0;
	qboolean lpar= qfalse,rpar= qfalse,number= qfalse, character = qfalse;

	while ( value[count] != '\0' ) {
		if ( value[count] == '{' )
			lpar = qtrue;
		else if ( value[count] == '}' )
			rpar = qtrue;
		else if ( value[count] >= '0' && value[count] <= '9' )
			number = qtrue;
		else if ( ( value[count] >= 'a' && value[count] <= 'z' ) || ( value[count] >= 'A' && value[count] <= 'Z' ) || (value[count] == '+' ) )
			character = qtrue;
		count++;
	}

	if ( lpar && ! ( rpar || number || character ) )
		return TOT_LPAREN;
	else if ( rpar && ! ( lpar || number || character ) )
		return TOT_RPAREN;
	else if ( number && ! ( lpar || rpar || character ) )
		return TOT_NUMBER;
	else if ( character && ! ( lpar || rpar ) )
		return TOT_WORD;
	else
		return TOT_NIL;
}

/*
=================
G_FindNextToken
Gives out the position of a token,
if the token is not found, -1 is returned
=================
*/
static int G_FindNextToken ( char *find, token_t *in, int start ) {
	int i;
	int cmp;

	for ( i = start; i < MAX_MAPCYCLETOKENS; i++ ) {
		cmp= strcmp ( in[i].value, find );
		if ( cmp == 0 )
			return i;
	}
	return -1;
}

/*
=================
G_AbeforeB
is true if the first argument is
found before the second argument
=================
*/

static qboolean G_AbeforeB ( char *A, char *B, token_t *in, int start ) {
	int a = G_FindNextToken ( A, in, start );
	int b = G_FindNextToken ( B, in, start );

	if ( b == -1 && a != -1 )
		return qtrue;
	if ( a == -1 && b != -1 )
		return qfalse;
	if ( a < b )
		return qtrue;
	else
		return qfalse;
}

/*
=================
SkippedChar
returns qtrue if the argument
is a char we should skip
=================
*/
qboolean SkippedChar ( char in ) {
	return ( in == '\n' || in == '\r' || in == ';' || in == '\t' || in == ' ' );
}

/*
=================
G_MapAvailable
search for a map in the mapfolder,
if the map is not found return qfalse
=================
*/
static qboolean G_MapAvailable ( char* map ) {
	if( !trap_FS_FOpenFile ( va ( "maps/%s.bsp",map ), NULL,FS_READ ) )
		return qfalse; //maps/MAPNAME.bsp does not exist
	
	return qtrue;
}

static int G_findCharInString( char charin, char* string, int size ){
	int i;
	for( i = 0; i < size; i++ ){
		if( charin == string[i] )
			return i;
	}
	return -1;
}

/*
=================
G_setMapcycle
reads the tokens to set the
mapcycle
=================
*/
static void G_setMapcycle ( token_t *in, int min, int max ) {
	int lastmappos;
	qboolean lastMapAvailable = qfalse;
	int i,j;
	int lpar,rpar;

	for ( i = min; i <= max; i++ ) {
		if ( in[i].type == TOT_WORD ) {
			lastmappos = i;
			//find '(' ')' for mapfiles
			lpar = G_findCharInString('(',in[i].value,sizeof(in[i].value) );
			rpar = G_findCharInString(')',in[i].value,sizeof(in[i].value) );
			if ( lpar != -1 && rpar != -1 ){
				for( j= lpar+1; j < rpar; j++ ){
					mapcycle.mapfiles[mapcycle.mapcycleCount][j-lpar-1] = in[i].value[j]; 
				}
				mapcycle.mapfiles[mapcycle.mapcycleCount][j-lpar-1] = '\0';
				in[i].value[lpar] = '\0';
			} else {
				mapcycle.mapfiles[mapcycle.mapcycleCount][0] = '\0';
			}
			
			if ( G_MapAvailable ( in[i].value ) ) {
				mapcycle.maps[mapcycle.mapcycleCount] = in[i].value;
				mapcycle.mapcycleCount++;
				lastMapAvailable = qtrue;
			} else {
				lastMapAvailable = qfalse;
				G_Printf ( "Map %s not found\n", in[i].value );
			}
		} else if ( in[i].type == TOT_NUMBER && lastMapAvailable ) {
			if ( ( i - lastmappos ) == 1 ) {
				mapcycle.minplayers[mapcycle.mapcycleCount-1] = atoi ( in[i].value );
			} else if ( ( i - lastmappos ) == 2 ) {
				mapcycle.maxplayers[mapcycle.mapcycleCount-1] = atoi ( in[i].value );
			} else if ( ( i - lastmappos ) == 3 ) {
				mapcycle.lockarena[mapcycle.mapcycleCount-1] = atoi ( in[i].value );
			} else {
				G_Printf ( "Error: Number not assigned to map\n" );
			}
		}
	}
}

/*
=================
G_setAllowedMaps
reads the tokens to set the votable
maps
=================
*/
static void G_setAllowedMaps ( token_t *in, int min, int max ) {
	int i;
	int lpar,rpar,j;
	
	for ( i = min; i <= max; i++ ) {
		if ( in[i].type == TOT_WORD ) {
			//find '(' ')' for mapfiles
			lpar = G_findCharInString('(',in[i].value,sizeof(in[i].value) );
			rpar = G_findCharInString(')',in[i].value,sizeof(in[i].value) );
			if ( lpar != -1 && rpar != -1 ){
				for( j= lpar+1; j < rpar; j++ ){
					mapcycle.allowedmapfiles[mapcycle.allowedMapsCount][j-lpar-1] = in[i].value[j]; 
				}
				mapcycle.allowedmapfiles[mapcycle.allowedMapsCount][j-lpar-1] = '\0';
				in[i].value[lpar] = '\0';
			} else {
				mapcycle.allowedmapfiles[mapcycle.allowedMapsCount][0] = '\0';
			}
			
			if ( G_MapAvailable ( in[i].value ) ) {
				mapcycle.allowedMaps[mapcycle.allowedMapsCount] = in[i].value;
				mapcycle.allowedMapsCount++;
			} else {
				G_Printf ( "Map %s not found\n", in[i].value );
			}
		} else {
			G_Printf ( "No valid mapname %s\n", in[i].value );
		}
	}
}
/*
=================
G_getNextMapNumber
returns the next possible
mapnumber in the cycle
=================
*/
static int G_getNextMapNumber ( int i ) {
	int start,j;
	int buffer;

	if ( i == ( mapcycle.mapcycleCount-1 ) )
		start = 0;
	else
		start = i+1;

	for ( j = 0; j < mapcycle.mapcycleCount; j++ ) {
		buffer = ( start+j ) %mapcycle.mapcycleCount;
		if ( level.numPlayingClients <= mapcycle.maxplayers[buffer] && level.numPlayingClients >= mapcycle.minplayers[buffer] ) {
			return buffer;
		}
	}
	return start;
}

/*
=================
G_mapChooser
=================
*/
void G_mapChooser( int num ) {
    int i,j;
    int from;
    int stringlength;
    char entry[64];
    char string[MAX_TOKEN_CHARS];
    from = rand() % mapcycle.mapcycleCount; 
    
    Com_sprintf (string, sizeof(string), "callmapvote %i ", num);
    stringlength = strlen(string);
    for (i=0;i<num;i++) {
        //j = (from+i)%mapcycle.mapcycleCount;
        j = G_getNextMapNumber(from);
        from = j;

        Com_sprintf (entry, sizeof(entry), "%s %i ",
                mapcycle.maps[j],mapcycle.lockarena[j]);
        j = strlen(entry);
        strcpy (string + stringlength, entry);
        stringlength += j;
    }
    trap_SendConsoleCommand(EXEC_APPEND,string);
}

/*
=================
G_GetMapLockArena
finds the current mapnumber and
returns the next possible
map in the cycle
=================
*/
int G_GetMapLockArena ( char *map ) {
	int i;

	if ( mapcycle.mapcycleCount == 0 )
		return 0;

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		if ( strcmp ( map, mapcycle.maps[i] ) == 0 )
			break;
	}

	if ( i == mapcycle.mapcycleCount ) {
		return 0;
	} 

	return mapcycle.lockarena[ i ];
}

/*
=================
G_GotoNextMapCycle
finds the next possible
map in the cycle
and goto it
=================
*/
void G_GotoNextMapCycle ( void ) {
    int i;

    if (mapcycle.mapcycleCount < g_mapcycleposition.integer) {
        trap_Cvar_Set("g_mapcycleposition","0");
    }
    i = G_getNextMapNumber (g_mapcycleposition.integer) ;
    if (mapcycle.mapcycleCount <= i) {
      trap_SendConsoleCommand( EXEC_APPEND, va("map_restart")) ;
      return;
    }


    trap_Cvar_Set("g_mapcycleposition",va("%i",i));
    trap_Cvar_Set("g_lockArena",va("%i", mapcycle.lockarena[ i ]));

    trap_SendConsoleCommand( EXEC_APPEND, va("map %s", mapcycle.maps[i ])) ;
}


/*
=================
G_GetNextMapCycle
finds the current mapnumber and
returns the next possible
map in the cycle
=================
*/
char *G_GetNextMapCycle ( char *map ) {

	if ( mapcycle.mapcycleCount == 0 )
		return map;

  if (mapcycle.mapcycleCount < g_mapcycleposition.integer) {
      trap_Cvar_Set("g_mapcycleposition","0");
  }

	return mapcycle.maps[G_getNextMapNumber (g_mapcycleposition.integer) ];
}


/*
=================
G_GetNextMap
finds the current mapnumber and
returns the next possible
map in the cycle
=================
*/
char *G_GetNextMap ( char *map ) {
	int i;

	if ( mapcycle.mapcycleCount == 0 )
		return map;

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		if ( strcmp ( map, mapcycle.maps[i] ) == 0 )
			break;
	}

	if ( i == mapcycle.mapcycleCount ) {
		return mapcycle.maps[G_getNextMapNumber ( ( int ) ( random() *mapcycle.mapcycleCount ) ) ];
	} 

	return mapcycle.maps[G_getNextMapNumber ( i ) ];
}

/*
=================
G_GetMapfile
finds the current mapnumber and
returns the next possible
map in the cycle
=================
*/
void G_GetMapfile ( char *map ) {
	int i;

	if ( mapcycle.mapcycleCount == 0 )
		return;

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		if ( strcmp ( map, mapcycle.maps[i] ) == 0 )
			break;
	}

	if ( i == mapcycle.mapcycleCount ) {
		for ( i = 0; i < mapcycle.allowedMapsCount; i++ ) {
			if ( strcmp ( map, mapcycle.allowedMaps[i] ) == 0 ){
				if( strcmp(mapcycle.allowedmapfiles[i],"") )
					G_LoadMapfile(mapcycle.allowedmapfiles[i]);
				return;
			}
		}
	} else {
		if( strcmp(mapcycle.mapfiles[i],"") )
			G_LoadMapfile(mapcycle.mapfiles[i]);
	}
	return;
}

/*
=================
G_mapIsVoteable
looks up in the mapcycle and
the allowed mapcycle
and returns qtrue if the map
in the argument is voteable
=================
*/
qboolean G_mapIsVoteable ( char* map ) {
	int i;
	if ( mapcycle.allAllowed && G_MapAvailable ( map ) )
		return qtrue;

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		if ( strcmp ( map, mapcycle.maps[i] ) == 0 )
			return qtrue;
	}

	for ( i = 0; i < mapcycle.allowedMapsCount; i++ ) {
		if ( strcmp ( map, mapcycle.allowedMaps[i] ) == 0 )
			return qtrue;
	}

	return qfalse;
}
/*
=================
G_drawAllowedMaps
prints out a list of all
voteable maps
=================
*/
void G_drawAllowedMaps ( gentity_t *ent ) {
	int i;
	char buffer[4001] = {'\0'};

	strcpy ( buffer,va ( "Allowed maps are: " ) );

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		strcat ( buffer, va ( "^2%s ", mapcycle.maps[i] ) );
	}
	for ( i = 0; i < mapcycle.allowedMapsCount; i++ ) {
		strcat ( buffer, va ( "^1%s ", mapcycle.allowedMaps[i] ) );
	}

	strcat ( buffer, va ( "\n" ) );

	for( i = 0; i < 4; i++) {
		char tmp = buffer[(i+1)*1000];
		buffer[(i+1)*1000] = '\0';
		trap_SendServerCommand(ent-g_entities, va ("print \"%s\"", &buffer[i*1000]));
		if(tmp == '\0') {
			break;
		}

		buffer[(i+1)*1000] = tmp;
	}
}

/*
=================
G_drawMapcycle
prints out a list of all
information in the mapcycle
=================
*/
void G_drawMapcycle ( gentity_t *ent ) {
	int i;
	char buffer[8001] = {'\0'};

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		strcat ( buffer, va ( "^3%i ^2%s ^3%i %i\n", i, mapcycle.maps[i], mapcycle.minplayers[i], mapcycle.maxplayers[i] ) );
	}
	strcat ( buffer, va ( "\n" ) );
	for ( i = 0; i < mapcycle.allowedMapsCount; i++ ) {
		strcat ( buffer, va ( "^1%s ", mapcycle.allowedMaps[i] ) );
	}

	strcat ( buffer, va ( "\n" ) );

	for( i = 0; i < 8; i++) {
		char tmp = buffer[(i+1)*1000];
		buffer[(i+1)*1000] = '\0';
		trap_SendServerCommand(ent-g_entities, va ("print \"%s\"", &buffer[i*1000]));
		if(tmp == '\0') {
			break;
		}

		buffer[(i+1)*1000] = tmp;
	}
}

/*
=================
G_sendMapcycle
prints the current working mapcycle
with original structure
useful for rcon and serverowners
=================
*/
void G_sendMapcycle( void ){
	int i;
	char buffer[MAX_MAPCYCLELENGTH];
	strcat ( buffer,va ( "mapcycle {\n" ) );
	
	if( !g_useMapcycle.integer ){
		G_Printf("no mapcycle set on this server\n");
		return;
	}

	for ( i = 0; i < mapcycle.mapcycleCount; i++ ) {
		if( !strcmp("", mapcycle.mapfiles[i]) )
			strcat ( buffer, va ( "    %s\n", mapcycle.maps[i] ) );
		else
			strcat ( buffer, va ( "    %s(%s)\n", mapcycle.maps[i], mapcycle.mapfiles[i] ) );
	}
	
	strcat( buffer, "}\n" );
	
	if( !mapcycle.allAllowed ){
	
		strcat ( buffer, "\nallowed {\n");
	
		for ( i = 0; i < mapcycle.allowedMapsCount; i++ ) {
			if( !strcmp("", mapcycle.allowedmapfiles[i]) )
				strcat ( buffer, va ( "    %s\n", mapcycle.allowedMaps[i] ) );
			else
				strcat ( buffer, va ( "    %s(%s)\n", mapcycle.allowedMaps[i], mapcycle.allowedmapfiles[i] ) );
		}

		strcat ( buffer, va ( "}\n" ) );
	}
	
	G_Printf("%s", buffer);
  
}

/*
=================
G_initMapcycle
set the init values
minplayers 0, maxplayers 64
=================
*/
static void G_initMapcycle ( void ) {
	int i;
	mapcycle.mapcycleCount = 0;
	mapcycle.allowedMapsCount = 0;
	mapcycle.allAllowed = qtrue;
	for ( i = 0; i < MAX_MAPCYCLECOUNT; i++ ) {
		mapcycle.minplayers[i] = 0;
		mapcycle.maxplayers[i] = MAX_CLIENTS;
		mapcycle.lockarena[i] = 0;
	}
}

token_t tokens[MAX_MAPCYCLETOKENS];

/*
=================
G_LoadMapcycle
load the mapcycle-file
and parse it
=================
*/
void G_LoadMapcycle ( void ) {
	int len;
	fileHandle_t f;
	char buffer[MAX_MAPCYCLELENGTH];
	int i, charCount = 0;
	qboolean pgbreak = qfalse, lastSpace = qtrue;
	int tokenNum = 0, maxTokenNum;
	int lpar, rpar;
	qboolean allAllowed = qtrue;

	G_initMapcycle();

	len = trap_FS_FOpenFile ( g_mapcycle.string, &f, FS_READ );

	if ( !f ) {
		G_Printf ( "Mapcycle %s not found, setting g_useMapcycle to 0\n", g_mapcycle.string );
		trap_Cvar_Set ( "g_useMapcycle", "0" );
		return;
	}

	if ( len > MAX_MAPCYCLELENGTH ) {
		G_Printf ( "Mapcycle file too large, %s contains %i chars, max allowed is %i\n", g_mapcycle.string, len, MAX_MAPCYCLELENGTH );
		return;
	}

	trap_FS_Read ( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile ( f );

	COM_Compress ( buffer );

	for ( i = 0; i < MAX_MAPCYCLELENGTH; i++ ) {

		//Filter comments( start at # and end at break )
		if ( buffer[i] == '#' ) {
			while ( i < MAX_MAPCYCLELENGTH && !pgbreak ) {
				if ( buffer[i] == '\n' || buffer[i] == '\r' )
					pgbreak = qtrue;
				i++;
			}
			pgbreak = qfalse;
			lastSpace = qtrue;
			//continue;
		}

		if ( SkippedChar ( buffer[i] ) ) {
			if ( !lastSpace ) {
				buffer[charCount] = ' ';
				charCount++;
				lastSpace = qtrue;
			}
			continue;
		}

		lastSpace = qfalse;
		buffer[charCount] = buffer[i];
		charCount++;
	}

	buffer[charCount] = '\0';

	i = 0;
	while ( i < MAX_MAPCYCLELENGTH && tokenNum < MAX_MAPCYCLETOKENS ) {
		i = G_setTokens ( buffer, tokens[tokenNum].value, i );
		tokens[tokenNum].type = G_setTokenType ( tokens[tokenNum].value );
		tokenNum++;
	}
	maxTokenNum = tokenNum;

	G_Printf ( "Mapcycle parser found %i tokens\n", maxTokenNum );

	for ( tokenNum = 0; tokenNum < maxTokenNum; tokenNum++ ) {

		if ( strcmp ( tokens[tokenNum].value, "mapcycle" ) == 0 ) {
			if ( strcmp ( tokens[tokenNum+1].value, "{" ) == 0 ) {
				lpar = tokenNum+1;
				if ( G_AbeforeB ( ( char* ) "{", ( char* ) "}", tokens, tokenNum+2 ) ) {
					G_Printf ( "error: \"}\" expected at %s\n", tokens[tokenNum].value );
					break;
				}
				rpar = G_FindNextToken ( ( char* ) "}", tokens, tokenNum+2 );
				if ( rpar != -1 ) {
					G_setMapcycle ( tokens, lpar+1, rpar-1 );
					tokenNum = rpar;
				}
			}
		} else if ( strcmp ( tokens[tokenNum].value, "allowed" ) == 0 ) {
			allAllowed = qfalse;
			if ( strcmp ( tokens[tokenNum+1].value, "{" ) == 0 ) {
				lpar = tokenNum+1;
				if ( G_AbeforeB ( ( char* ) "{", ( char* ) "}", tokens, tokenNum+2 ) ) {
					G_Printf ( "error: \"}\" expected at %s\n", tokens[tokenNum].value );
					break;
				}
				rpar = G_FindNextToken ( ( char* ) "}", tokens, tokenNum+2 );
				if ( rpar != -1 ) {
					G_setAllowedMaps ( tokens, lpar+1, rpar-1 );
					tokenNum = rpar;
				}
			}
		}
	}
  if (mapcycle.mapcycleCount < g_mapcycleposition.integer) {
      trap_Cvar_Set("g_mapcycleposition","0");
  }

	mapcycle.allAllowed = allAllowed;
}

