// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "DeclSurfaceType.h"
#include "framework/DeclParseHelper.h"

/*
===============================================================================

sdDeclSurfaceType

===============================================================================
*/

const char* sdDeclSurfaceType::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclSurfaceType::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclSurfaceType::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "properties" )) {
			properties.Parse(src);
			continue;
		}

		src.Warning( "sdDeclSurfaceType::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}
	
	return true;
}

void sdDeclSurfaceType::FreeData() {
	type.Clear();
	properties.Clear();
}
