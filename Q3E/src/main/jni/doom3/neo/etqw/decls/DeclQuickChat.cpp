// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclQuickChat.h"
#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclQuickChat

===============================================================================
*/

/*
================
sdDeclQuickChat::sdDeclQuickChat
================
*/
sdDeclQuickChat::sdDeclQuickChat( void ) {
	requirements.Clear();
	team = false;
	fireteam = false;
	audio = NULL;
	type = -1;
}

/*
================
sdDeclQuickChat::~sdDeclQuickChat
================
*/
sdDeclQuickChat::~sdDeclQuickChat( void ) {
}

/*
================
sdDeclQuickChat::DefaultDefinition
================
*/
const char* sdDeclQuickChat::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclQuickChat::Parse
================
*/
bool sdDeclQuickChat::Parse( const char *_text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( _text, textLength, GetFileName(), GetLineNum() );	
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, _text, textLength, src );

	src.SkipUntilString( "{", &token );

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "text" ) ) {
			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			text = declHolder.FindLocStr( token );
		} else if ( !token.Icmp( "type" ) ) {
			if ( !src.ExpectTokenType( TT_NUMBER, 0, &token ) ) {
				return false;
			}

			type = token.GetIntValue();
		} else if ( !token.Icmp( "audio" ) ) {
			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			audio = declHolder.FindSoundShader( token, false );
			if ( audio == NULL ) {
				src.Warning( "sdDeclQuickChat::Parse : missing sound shader %s", token.c_str() );
				return false;
			}
		} else if ( !token.Icmp( "team" ) ) {
			team = true;
		} else if ( !token.Icmp( "fireteam" ) ) {
			fireteam = true;
		} else if ( !token.Icmp( "callback" ) ) {
			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}
			callback = token;
		} else if ( !token.Icmp( "requirements" ) ) {
			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}
			requirements.Load( token );
		} else if ( !token.Cmp( "}" ) ) {
			break;
		} else {
			src.Error( "sdDeclQuickChat::Parse Invalid Token %s", token.c_str() );
		}
	}

	return true;
}

/*
================
sdDeclQuickChat::FreeData
================
*/
void sdDeclQuickChat::FreeData( void ) {
	requirements.Clear();
	text		= NULL;
	audio		= NULL;
	team		= false;
	fireteam	= false;
	callback	= "";
}
