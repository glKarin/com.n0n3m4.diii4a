/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

/*
============
idCmdArgs::operator=
============
*/
void idCmdArgs::operator=( const idCmdArgs &args ) {
	int i;

	argc = args.argc;
	memcpy( tokenized, args.tokenized, MAX_COMMAND_STRING );
	for ( i = 0; i < argc; i++ ) {
		argv[ i ] = tokenized + ( args.argv[ i ] - args.tokenized );
	}
}

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
					int l = static_cast<int>(strlen( cmd_args ));
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

	if ( !text ) {
		return;
	}

	lex.LoadMemory( text, static_cast<int>(strlen( text )), "idCmdSystemLocal::TokenizeString" );
	lex.SetFlags( LEXFL_NOERRORS
				| LEXFL_NOWARNINGS
				| LEXFL_NOSTRINGCONCAT
				| LEXFL_ALLOWPATHNAMES
				| LEXFL_NOSTRINGESCAPECHARS
				| LEXFL_ALLOWIPADDRESSES | ( keepAsStrings ? LEXFL_ONLYSTRINGS : 0 ) );

	totalLen = 0;

	while ( 1 ) {
		if ( argc == MAX_COMMAND_ARGS ) {
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
			if ( idLib::cvarSystem ) {
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

		idStr::Copynz( tokenized + totalLen, token.c_str(), sizeof( tokenized ) - totalLen );

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
		argv[ argc ] = argv[ argc-1 ] + strlen( argv[ argc-1 ] ) + 1;
        idStr::Copynz(argv[argc], text, static_cast<int>(sizeof(tokenized) - (argv[argc] - tokenized)));
		argc++;
	}
}

/*
============
idCmdArgs::GetArgs
============
*/
const char **idCmdArgs::GetArgs( int *_argc ) const {
	*_argc = argc;
	return (const char **)&argv[0];
}

