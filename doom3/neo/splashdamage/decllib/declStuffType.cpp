// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declStuffType.h"
#include "framework/DeclParseHelper.h"

sdDeclStuffType::sdDeclStuffType( void )
	: randomizeAngles(false),
	lodType(NULL)
{

}

const char* sdDeclStuffType::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclStuffType::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclStuffType::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "Model" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclStuffType::Parse: failed to parse Model" );
				break;
			}
			models.Append(token);
			continue;
		}

		if( !token.Icmp( "randomizeAngles" )) {
			randomizeAngles = true;
			continue;
		}

		src.Warning( "sdDeclStuffType::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

void sdDeclStuffType::FreeData( void ) {
	models.Clear();
	randomizeAngles = false;
	lodType = NULL;
}

bool sdDeclStuffType::RebuildTextSource( void ) {
	return true;
}
