/*
cfgscript.c - "Valve script" parsing routines
Copyright (C) 2016 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "enginecallback_menu.h"
#include "CFGScript.h"

#define CVAR_USERINFO BIT(1)


const char *cvartypes[] = { NULL, "BOOL" , "NUMBER", "LIST", "STRING" };

struct parserstate_t
{
	parserstate_t() : buf( NULL ), filename( NULL ) { token[0] = 0;}
	char *buf;
	char token[MAX_STRING];
	const char *filename;
};

/*
===================
CSCR_ExpectString

Return true if next token is pExpext and skip it
===================
*/
bool CSCR_ExpectString( parserstate_t *ps, const char *pExpect, bool skip, bool error )
{
	char *tmp = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));

	if( !stricmp( ps->token, pExpect ) )
	{
		ps->buf = tmp;
		return true;
	}

	if( skip )
		ps->buf = tmp;

	if( error )
		Con_DPrintf( "Syntax error in %s: got \"%s\" instead of \"%s\"\n", ps->filename, ps->token, pExpect );

	return false;
}

/*
===================
CSCR_ParseType

Determine script variable type
===================
*/
cvartype_t CSCR_ParseType( parserstate_t *ps )
{
	int i;

	for ( i = 1; i < T_COUNT; ++i )
	{
		if( CSCR_ExpectString( ps, cvartypes[i], false, false ) )
			return (cvartype_t)i;
	}

	Con_DPrintf( "Cannot parse %s: Bad type %s\n", ps->filename, ps->token );
	return T_NONE;
}



/*
=========================
CSCR_ParseSingleCvar
=========================
*/
bool CSCR_ParseSingleCvar( parserstate_t *ps, scrvardef_t *result )
{
	// clean linked list for list
	result->list.iCount = 0;
	result->list.pEntries = result->list.pLast = NULL;
	result->list.pArray = NULL;

	// read the name
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, result->name, sizeof( result->name ));

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		goto error;

	// read description
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, result->desc, sizeof( result->desc ));

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		goto error;

	result->type = CSCR_ParseType( ps );

	switch( result->type )
	{
	case T_BOOL:
		// bool only has description
		if( !CSCR_ExpectString( ps, "}", false, true ) )
			goto error;
		break;
	case T_NUMBER:
		// min
		ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));
		result->number.fMin = atof( ps->token );

		// max
		ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));
		result->number.fMax = atof( ps->token );

		if( !CSCR_ExpectString( ps, "}", false, true ) )
			goto error;
		break;
	case T_STRING:
		if( !CSCR_ExpectString( ps, "}", false, true ) )
			goto error;
		break;
	case T_LIST:
		while( !CSCR_ExpectString( ps, "}", true, false ) )
		{
			// char szName[128];
			char *szName = ps->token;
			char szValue[64];
			scrvarlistentry_t *entry = 0;

			// Read token for each item here

			// ExpectString already moves buffer pointer, so just read from ps->token
			// ps->buf = EngFuncs::COM_ParseFile( ps->buf, szName, sizeof( szName ));
			if( !szName[0] )
				goto error;

			ps->buf = EngFuncs::COM_ParseFile( ps->buf, szValue, sizeof( szValue ));
			if( !szValue[0] )
				goto error;

			entry = new scrvarlistentry_t;
			entry->next = NULL;
			entry->szName = StringCopy( szName );
			entry->flValue = atof( szValue );

			if( !result->list.pEntries )
				result->list.pEntries = entry;
			else
				result->list.pLast->next = entry;

			result->list.pLast = entry;
			result->list.iCount++;
		}
		break;
	default:
		goto error;
	}

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		goto error;

	// default value
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, result->value, sizeof( result->value ));

	if( !CSCR_ExpectString( ps, "}", false, true ) )
		goto error;

	if( CSCR_ExpectString( ps, "SetInfo", false, false ) )
		result->flags |= CVAR_USERINFO;

	if( !CSCR_ExpectString( ps, "}", false, true ) )
		goto error;

	if( result->type == T_LIST )
	{
		scrvarlistentry_t *entry = result->list.pEntries;

		result->list.pArray = new const char*[result->list.iCount];
		result->list.pModel = new CStringArrayModel( result->list.pArray, result->list.iCount );

		for( int i = 0; entry; entry = entry->next, i++ )
		{
			result->list.pArray[i] = L( entry->szName );
		}
	}

	return true;
error:
	if( result->type == T_LIST )
	{
		if( result->list.pArray )
			delete[] result->list.pArray;
		if( result->list.pModel )
			delete result->list.pModel;

		while( result->list.pEntries )
		{
			scrvarlistentry_t *next = result->list.pEntries->next;
			delete[] result->list.pEntries->szName;
			delete result->list.pEntries;

			result->list.pEntries = next;
		}
	}
	return false;
}

/*
======================
CSCR_ParseHeader

Check version and seek to first cvar name
======================
*/
bool CSCR_ParseHeader( parserstate_t *ps )
{
	if( !CSCR_ExpectString( ps, "VERSION", false, true ) )
		return false;

	// Parse in the version #
	// Get the first token.
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));

	if( atof( ps->token ) != 1 )
	{
		Con_DPrintf( "File %s has wrong version %s!\n", ps->filename, ps->token );
		return false;
	}

	if( !CSCR_ExpectString( ps, "DESCRIPTION", false, true ) )
		return false;

	ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));

	if( stricmp( ps->token, "INFO_OPTIONS") && stricmp( ps->token, "SERVER_OPTIONS" ) )
	{
		Con_DPrintf( "DESCRIPTION must be INFO_OPTIONS or SERVER_OPTIONS\n");
		return false;
	}

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		return false;

	return true;
}

/*
======================
CSCR_LoadDefaultCVars

Register all cvars declared in config file and set default values
======================
*/
scrvardef_t *CSCR_LoadDefaultCVars( const char *scriptfilename, int *count )
{
	int length = 0;
	char *start;
	parserstate_t state;
	bool success = false;
	scrvardef_t *list = 0, *last = 0;

	*count = 0;

	state.filename = scriptfilename;

	state.buf = (char*)EngFuncs::COM_LoadFile( scriptfilename, &length );

	start = state.buf;

	if( state.buf == 0 || length == 0)
	{
		if( start )
			EngFuncs::COM_FreeFile( start );
		return 0;
	}

	Con_DPrintf( "Reading config script file %s\n", scriptfilename );

	if( !CSCR_ParseHeader( &state ) )
	{
		Con_DPrintf( "Failed to	parse header!\n" );
		goto finish;
	}

	while( !CSCR_ExpectString( &state, "}", false, false ) )
	{
		scrvardef_t var;

		// Create a new object
		if( CSCR_ParseSingleCvar( &state, &var ) )
		{
			// Cvar_Get( var.name, var.value, var.flags, var.desc );
			scrvardef_t *entry = new scrvardef_t;
			*entry = var;

			if( !list )
			{
				list = last = entry;
			}
			else
			{
				last = last->next = entry;
			}
			(*count)++;
		}
		else
			break;

		if( *count > 1024 )
			break;
	}

	if( EngFuncs::COM_ParseFile( state.buf, state.token, sizeof( state.token )))
		Con_DPrintf( "Got extra tokens!\n" );
	else
		success = true;

finish:
	if( !success )
	{
		state.token[ sizeof( state.token ) - 1 ] = 0;
		if( start && state.buf )
			Con_DPrintf( "Parse error in %s, byte %d, token %s\n", scriptfilename, (int)( state.buf - start ), state.token );
		else
			Con_DPrintf( "Parse error in %s, token %s\n", scriptfilename, state.token );
	}
	if( start )
		EngFuncs::COM_FreeFile( start );

	return list;
}

void CSCR_FreeList( scrvardef_t *list )
{
	scrvardef_t *i = list;
	while( i )
	{
		scrvardef_t *next = i->next;

		if( i->type == T_LIST )
		{
			if( i->list.pModel )
				delete i->list.pModel;
			if( i->list.pArray )
				delete[] i->list.pArray;

			while( i->list.pEntries )
			{
				scrvarlistentry_t *next = i->list.pEntries->next;
				delete[] i->list.pEntries->szName;
				delete i->list.pEntries;

				i->list.pEntries = next;
			}
		}

		delete i;
		i = next;
	}
}
