// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "GameDeclIdentifiers.h"
#include "../../framework/DeclParseHelper.h"
/*
===============================================================================

	sdDeclStringMap

===============================================================================
*/

#include "DeclStringMap.h"

/*
================
sdDeclStringMap::sdDeclStringMap
================
*/
sdDeclStringMap::sdDeclStringMap( void ) {
}

/*
================
sdDeclStringMap::~sdDeclStringMap
================
*/
sdDeclStringMap::~sdDeclStringMap( void ) {
}

/*
================
sdDeclStringMap::DefaultDefinition
================
*/
const char* sdDeclStringMap::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclStringMap::Parse
================
*/
bool sdDeclStringMap::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );
	src.UnreadToken( token );

	dict.Parse( src );

	return true;
}

/*
================
sdDeclStringMap::FreeData
================
*/
void sdDeclStringMap::FreeData( void ) {
	dict.Clear();
}

/*
================
sdDeclStringMap::GetValue
================
*/
const char* sdDeclStringMap::GetValue( const char* key ) const {
	return dict.GetString( key );
}

/*
================
sdDeclStringMap::RebuildSourceText
================
*/
void sdDeclStringMap::RebuildSourceText() {
	const char* typeName = declManager->GetDeclTypeName( GetType() );
	idStr text = va( "\n%s %s {\n", typeName, GetName() );
	for( int i = 0; i < dict.GetNumKeyVals(); i++ ) {
		const idKeyValue* kv = dict.GetKeyVal( i );
		text += va( "\t\"%s\"\t\"%s\"\n", kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
	text += "}";
	SetText( text );
}



/*
============
sdDeclStringMap::Save
============
*/
void sdDeclStringMap::Save() {
	RebuildSourceText();
	ReplaceSourceFileText();
}

/*
================
sdDeclStringMap::CacheFromDict
================
*/
void sdDeclStringMap::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	idDeclTypeInterface* iface = declManager->GetDeclType( declStringMapIdentifier );
	if ( !iface ) {
		return;
	}

	while ( kv = dict.MatchPrefix( "str_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			const sdDeclStringMap* str = static_cast< const sdDeclStringMap* >( iface->Find( kv->GetValue(), false ) );
			if ( str ) {
				game->CacheDictionaryMedia( str->GetDict() );
			}
		}
	}
}
