// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declDecal.h"
#include "framework/DeclParseHelper.h"

sdDeclDecal::sdDeclDecal(void)
    : lifeTime(0.0f),
      minSize(0.0f),
      sizeDiff(0.0f),
      material(NULL)
{
    startColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
    endColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
}

const char* sdDeclDecal::DefaultDefinition( void ) const {
    return "{  }";
}

bool sdDeclDecal::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclDecal::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "material" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclDecal::Parse: failed to parse material" );
				break;
			}
			material = declManager->FindMaterial(token);
			continue;
		}

		if (!token.Icmp("lifeTime")) {
			lifeTime = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("size")) {
			minSize = src.ParseFloat(); // vec2
			src.ExpectTokenString(",");
			sizeDiff = src.ParseFloat() - minSize;
			continue;
		}

		if (!token.Icmp("gridSize")) {
			src.ParseFloat(); // vec2
			src.ExpectTokenString(",");
			src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "image" )) {
			sdBounds2D item;
			item[0][0] = src.ParseFloat();
			src.ExpectTokenString(",");
			item[0][1] = src.ParseFloat();
			src.ExpectTokenString(",");
			item[1][0] = src.ParseFloat();
			src.ExpectTokenString(",");
			item[1][1] = src.ParseFloat();
			images.Append(item);
			continue;
		}

		src.Warning( "sdDeclDecal::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}
	
    return true;
}

void sdDeclDecal::FreeData( void ) {
    lifeTime = 0.0f;
    minSize = 0.0f;
    sizeDiff = 0.0f;
    material = NULL;
    startColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
    endColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
    images.Clear();
}

void sdDeclDecal::CacheFromDict( const idDict& dict ) {
}
