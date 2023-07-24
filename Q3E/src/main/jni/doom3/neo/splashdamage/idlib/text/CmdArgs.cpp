// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../../framework/CVarSystem.h"

/*
============
idCmdArgs::Args
============
*/
const char *idCmdArgs::Args(  int start, int end, bool escapeArgs ) const {
	static char cmd_args[MAX_COMMAND_STRING];
	int		i;

	if ( end < 0 ) {
		end = argc - 1;
	} else if ( end >= argc ) {
		end = argc - 1;
	}
	cmd_args[0] = '\0';
	if ( escapeArgs ) {
		strcat( cmd_args, "\"" );
	}
	for ( i = start; i <= end; i++ ) {
		if ( i > start ) {
			if ( escapeArgs ) {
				strcat( cmd_args, "\" \"" );
			} else {
				strcat( cmd_args, " " );
			}
		}
		if ( escapeArgs && strchr( argv[i], '\\' ) ) {
			char *p = argv[i];
			while ( *p != '\0' ) {
				if ( *p == '\\' ) {
					strcat( cmd_args, "\\\\" );
				} else {
					int l = idStr::Length( cmd_args );
					cmd_args[ l ] = *p;
					cmd_args[ l+1 ] = '\0';
				}
				p++;
			}
		} else {
			strcat( cmd_args, argv[i] );
		}
	}
	if ( escapeArgs ) {
		strcat( cmd_args, "\"" );
	}

	return cmd_args;
}

/*
============
idCmdArgs::TokenizeString

Parses the given string into command line tokens.
The text is copied to a separate buffer and 0 characters
are inserted in the appropriate place. The argv array
will point into this temporary buffer.
============
*/
void idCmdArgs::TokenizeString( const char *text, bool keepAsStrings ) {
	idLexer		lex;
	idToken		token, number;
	int			len, totalLen;

	// clear previous args
	argc = 0;

	if ( text == NULL ) {
		return;
	}

	lex.SetFlags( LEXFL_NOERRORS
				| LEXFL_NOWARNINGS
				| LEXFL_NOSTRINGCONCAT
				| LEXFL_ALLOWPATHNAMES
				| LEXFL_NOSTRINGESCAPECHARS
				| LEXFL_ALLOWIPADDRESSES | ( keepAsStrings ? LEXFL_ONLYSTRINGS : 0 ) );
	lex.LoadMemory( text, idStr::Length( text ), "idCmdArgs::TokenizeString" );

	totalLen = 0;

	while ( 1 ) {
		if ( argc == MAX_COMMAND_ARGS ) {
			idLib::common->Warning( "idCmdArgs::TokenizeString : MAX_COMMAND_ARGS hit" );
			return;			// this is usually something malicious
		}

		if ( !lex.ReadToken( &token ) ) {
			return;
		}

		// check for negative numbers
		if ( !keepAsStrings && ( token == "-" ) ) {
			if ( lex.CheckTokenType( TT_NUMBER, 0, &number ) ) {
				token = "-" + number;
			}
		}

		// check for cvar expansion
		if ( token == "$" ) {
			if ( !lex.ReadToken( &token ) ) {
				return;
			}
			if ( idLib::cvarSystem != NULL ) {
				token = idLib::cvarSystem->GetCVarString( token.c_str() );
			} else {
				token = "<unknown>";
			}
		}

		len = token.Length();

		if ( totalLen + len + 1 > sizeof( tokenized ) ) {
			return;			// this is usually something malicious
		}

		// regular token
		argv[argc] = tokenized + totalLen;
		argc++;

		// mac version indicated possible buffer overflow
		int len = sizeof( tokenized ) - totalLen;
		if ( len > token.Length() + 1 ) {
			len = token.Length() + 1;
		}
		idStr::Copynz( tokenized + totalLen, token.c_str(), len );

		totalLen += len + 1;
	}
}

/*
============
idCmdArgs::AppendArg
============
*/
void idCmdArgs::AppendArg( const char *text ) {
	if ( !argc ) {
		argc = 1;
		argv[ 0 ] = tokenized;
		idStr::Copynz( tokenized, text, sizeof( tokenized ) );
	} else {
		argv[ argc ] = argv[ argc - 1 ] + idStr::Length( argv[ argc - 1 ] ) + 1;
		idStr::Copynz( argv[ argc ], text, static_cast< int >( sizeof( tokenized ) ) - ( argv[ argc ] - tokenized ) );
		argc++;
	}
}

/*
============
idCmdArgs::GetArgs
============
*/
const char **idCmdArgs::GetArgs( int *_argc ) {
	*_argc = argc;
	return (const char **)&argv[0];
}

/*
============
idCmdArgs::operator=
============
*/
idCmdArgs& idCmdArgs::operator=( const idCmdArgs& rhs ) {
	argc = rhs.argc;
	memcpy( tokenized, rhs.tokenized, sizeof( tokenized ) );

	int i;
	for ( i = 0; i < argc; i++ ) {
		argv[ i ] = ( rhs.argv[ i ] - rhs.tokenized ) + tokenized;
	}

	return *this;
}
