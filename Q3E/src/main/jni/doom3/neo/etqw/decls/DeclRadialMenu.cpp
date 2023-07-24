// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclRadialMenu.h"
#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclRadialMenu

===============================================================================
*/

/*
================
sdDeclRadialMenu::sdDeclRadialMenu
================
*/
sdDeclRadialMenu::sdDeclRadialMenu( void ) {
}

/*
================
sdDeclRadialMenu::~sdDeclRadialMenu
================
*/
sdDeclRadialMenu::~sdDeclRadialMenu( void ) {
	FreeData();
}

/*
================
sdDeclRadialMenu::DefaultDefinition
================
*/
const char* sdDeclRadialMenu::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclRadialMenu::Parse
================
*/
bool sdDeclRadialMenu::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	//sdDeclParseHelper declHelper( this, text, textLength, src );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	bool hadError = false;

	while( true ) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclRadialMenu::Parse: unexpected end of file." );
			hadError = true;
			break;
		}
		if( !token.Cmp( "}" )) {
			break;
		}

		if( !token.Icmp( "keys" )) {
			if( !ParseKeys( src, keys )) {
				src.Error( "sdDeclRadialMenu::Parse: failed to parse keys" );
				hadError = true;
				break;
			}
			continue;
		}

		if( !token.Icmp( "title" )) {
			if( !src.ReadToken( &token )) {
				src.Error( "sdDeclRadialMenu::Parse: failed to title" );
				hadError = true;
				break;
			}
			title = declHolder.FindLocStr( token.c_str() );
			continue;
		}

		if( !token.Icmp( "page" )) {
			if( !ParsePage( src )) {
				src.Error( "sdDeclRadialMenu::Parse: failed to parse page" );
				hadError = true;
				break;
			}
			continue;
		}
		if( !token.Icmp( "item" )) {
			if( !ParseItem( src )) {
				src.Error( "sdDeclRadialMenu::Parse: failed to parse item" );
				hadError = true;
				break;
			}
			continue;
		}
	}
	
	return !hadError;
}

/*
================
sdDeclRadialMenu::FreeData
================
*/
void sdDeclRadialMenu::FreeData( void ) {
	title = declHolder.FindLocStr( "default" );
	keys.Clear();
	pages.Clear();
	items.Clear();
}


/*
============
sdDeclRadialMenu::ParseKeys
============
*/
bool sdDeclRadialMenu::ParseKeys( idParser& src, idDict& keys ) {
	return keys.Parse( src );
}


/*
============
sdDeclRadialMenu::ParseItem
============
*/
bool sdDeclRadialMenu::ParseItem( idParser& src ) {
	idToken token;
	if( !src.ReadToken( &token )) {
		src.Error( "sdDeclRadialMenu::ParseItem: Unexpected end of file while parsing itemName" );
		return false;
	}
	item_t& item = items.Alloc();
	item.title = declHolder.FindLocStr( token.c_str() );

	bool success = ParseKeys( src, item.keys );

	gameLocal.CacheDictionaryMedia( item.keys );

	return success;
}


/*
============
sdDeclRadialMenu::ParsePage
============
*/
bool sdDeclRadialMenu::ParsePage( idParser& src ) {
	idToken token;
	if( !src.ReadToken( &token )) {
		src.Error( "sdDeclRadialMenu::ParsePage: Unexpected end of file while parsing declName" );
		return false;
	}
	if( token.Length() > 0 ) {
		pages.Append( gameLocal.declRadialMenuType.LocalFind( token ) );
	} else {
		gameLocal.Warning( "sdDeclRadialMenu::ParsePage: parsed an empty page in '%s'", GetName() );
	}
	
	return true;
}
