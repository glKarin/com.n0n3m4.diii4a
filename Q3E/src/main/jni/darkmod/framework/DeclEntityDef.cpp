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
=================
idDeclEntityDef::Size
=================
*/
size_t idDeclEntityDef::Size( void ) const {
	return sizeof( idDeclEntityDef ) + dict.Allocated();
}

/*
================
idDeclEntityDef::FreeData
================
*/
void idDeclEntityDef::FreeData( void ) {
	dict.ClearFree();
}

/*
================
idDeclEntityDef::Parse
================
*/
bool idDeclEntityDef::Parse( const char *text, const int textLength ) {
	idLexer src;
	idToken	token, token2;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	while (1) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( token.type != TT_STRING ) {
			src.Warning( "Expected quoted string, but found '%s'", token.c_str() );
			MakeDefault();
			return false;
		}

		if ( !src.ReadToken( &token2 ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		}

		if ( dict.FindKey( token ) ) {
			src.Warning( "'%s' already defined", token.c_str() );
		}
		dict.Set( token, token2 );
	}

	// we always automatically set a "classname" key to our name
	dict.Set( "classname", GetName() );

	// "inherit" keys will cause all values from another entityDef to be copied into this one
	// if they don't conflict.  We can't have circular recursions, because each entityDef will
	// never be parsed mroe than once

	// find all of the dicts first, because copying inherited values will modify the dict
	idList<const idDeclEntityDef *> defList;

	while ( 1 ) {
		const idKeyValue *kv;
		kv = dict.MatchPrefix( "inherit", NULL );
		if ( !kv ) {
			break;
		}

		const idDeclEntityDef *copy = static_cast<const idDeclEntityDef *>( declManager->FindType( DECL_ENTITYDEF, kv->GetValue(), false ) );
		if ( !copy ) {
			src.Warning( "Unknown entityDef '%s' inherited by '%s'", kv->GetValue().c_str(), GetName() );
		} else {
			defList.Append( copy );
		}

		// delete this key/value pair
		dict.Delete( kv->GetKey() );
	}

	// now copy over the inherited key / value pairs
	for ( int i = 0 ; i < defList.Num() ; i++ ) {
		dict.SetDefaults( &defList[ i ]->dict );
	}

	// precache all referenced media
	// do this as long as we arent in modview
	if ( !( com_editors & (EDITOR_RADIANT|EDITOR_AAS|EDITOR_RUNPARTICLE) ) ) {
		game->CacheDictionaryMedia( &dict );
	}

	return true;
}

/*
================
idDeclEntityDef::DefaultDefinition
================
*/
const char *idDeclEntityDef::DefaultDefinition( void ) const {
	return
		"{\n"
	"\t"	"\"DEFAULTED\"\t\"1\"\n"
		"}";
}

/*
================
idDeclEntityDef::Print

Dumps all key/value pairs, including inherited ones
================
*/
void idDeclEntityDef::Print( void ) const {
	dict.Print();
}
