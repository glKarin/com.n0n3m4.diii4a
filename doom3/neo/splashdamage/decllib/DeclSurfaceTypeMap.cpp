// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "DeclSurfaceTypeMap.h"
#include "DeclSurfaceType.h"
#include "framework/DeclParseHelper.h"

sdDeclSurfaceTypeMap::sdDeclSurfaceTypeMap(void)
    : width(0),
      height(0)
{
}

const char* sdDeclSurfaceTypeMap::DefaultDefinition( void ) const {
    return "{  }";
}

bool sdDeclSurfaceTypeMap::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclSurfaceTypeMap::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("width")) {
			width = src.ParseInt();
			continue;
		}

		if (!token.Icmp("height")) {
			height = src.ParseInt();
			continue;
		}

		if( !token.Icmp( "rect" )) {
			if(!ParseRect(&src))
			{
				src.SkipBracedSection(false);
				break;
			}
			continue;
		}

		if( !token.Icmp( "winding" )) {
			if(!ParseWinding(&src))
			{
				src.SkipBracedSection(false);
				break;
			}
			continue;
		}

		src.Warning( "sdDeclSurfaceTypeMap::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}
	
    return true;
}

void sdDeclSurfaceTypeMap::FreeData( void ) {
    width = 0.0f;
    height = 0.0f;
    rects.Clear();
}

bool sdDeclSurfaceTypeMap::ParseRect( idParser *src ) {
	idToken token;
	if( !src->ExpectTokenString( "{" )) {
		src->Error( "sdDeclSurfaceTypeMap::ParseRect: expected {." );
		return false;
	}

	rect_t item;
	while (1) {
		if( !src->ReadToken( &token )) {
			src->Error( "sdDeclSurfaceTypeMap::ParseRect: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("surfaceType")) {
			if( !src->ReadToken(&token)) {
				src->Error( "sdDeclSurfaceTypeMap::ParseRect: failed to parse name" );
				break;
			}
			item.surfaceType = static_cast<const sdDeclSurfaceType *>(declManager->FindType(DECL_SURFACETYPE, token));
			if (!item.surfaceType) {
				src->Warning( "sdDeclSurfaceTypeMap::ParseRect: couldn't find surface type '%s'.", token.c_str() );
			}
			continue;
		}

		if (!token.Icmp("surfaceColor")) {
			src->Parse1DMatrix(3, item.surfaceColor.ToFloatPtr());
			continue;
		}

		if (!token.Icmp("coords")) {
			item.coords.SetNum(2);
			src->Parse1DMatrix(2, item.coords[0].ToFloatPtr());
			src->Parse1DMatrix(2, item.coords[1].ToFloatPtr());
			continue;
		}

		src->Warning( "sdDeclSurfaceTypeMap::ParseRect: unexpected token '%s'.", token.c_str() );
		src->SkipBracedSection(false);
		break;
	}
	rects.Append(item);

	return true;
}

bool sdDeclSurfaceTypeMap::ParseWinding( idParser *src ) {
	idToken token;
	if( !src->ExpectTokenString( "{" )) {
		src->Error( "sdDeclSurfaceTypeMap::ParseWinding: expected {." );
		return false;
	}

	rect_t item;
	while (1) {
		if( !src->ReadToken( &token )) {
			src->Error( "sdDeclSurfaceTypeMap::ParseWinding: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("surfaceType")) {
			if( !src->ReadToken(&token)) {
				src->Error( "sdDeclSurfaceTypeMap::ParseWinding: failed to parse name" );
				break;
			}
			item.surfaceType = static_cast<const sdDeclSurfaceType *>(declManager->FindType(DECL_SURFACETYPE, token));
			if (!item.surfaceType) {
				src->Warning( "sdDeclSurfaceTypeMap::ParseWinding: couldn't find surface type '%s'.", token.c_str() );
			}
			continue;
		}

		if (!token.Icmp("surfaceColor")) {
			src->Parse1DMatrix(3, item.surfaceColor.ToFloatPtr());
			continue;
		}

		if (!token.Icmp("coords")) {
			int num = src->ParseInt();
			src->ExpectTokenString("{");
			item.coords.SetNum(num);
			for(int i = 0; i < num; i++)
				src->Parse1DMatrix(2, item.coords[i].ToFloatPtr());
			src->ExpectTokenString("}");
			continue;
		}

		src->Warning( "sdDeclSurfaceTypeMap::ParseRect: unexpected token '%s'.", token.c_str() );
		src->SkipBracedSection(false);
		break;
	}
	rects.Append(item);

	return true;
}

void sdDeclSurfaceTypeMap::CacheFromDict( const idDict& dict ) {
}
