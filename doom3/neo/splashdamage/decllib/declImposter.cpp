// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declImposter.h"
#include "framework/DeclParseHelper.h"

void sdImposterSubImage::Write( idFile_Memory &f ) {
}

bool sdImposterSubImage::Read( idParser &src ) {
	return true;
}

static void imposterInfo_t_Init(sdDeclImposter::imposterInfo_t &info) {
	info.images.Clear();
	info.material = NULL;
	info.origin.Zero();
	info.scalex = 1.0f;
	info.scaley = 1.0f;
	info.screenScale = 1.0f;
	info.tileSize = 0;
	info.numAngles = 0;
}

sdDeclImposter::sdDeclImposter( void ) {
	imposterInfo_t_Init(info);
}

	// Override from idDecl
const char* sdDeclImposter::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclImposter::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclImposter::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "material" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclImposter::Parse: failed to parse material" );
				break;
			}
			info.material = declManager->FindMaterial(token);
			continue;
		}

		if( !token.Icmp( "origin" )) {
			info.origin[0] = src.ParseFloat();
			info.origin[1] = src.ParseFloat();
			info.origin[2] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("scalex")) {
			info.scalex = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("scaley")) {
			info.scaley = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("screenScale")) {
			info.screenScale = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("numAngles")) {
			info.numAngles = src.ParseInt();
			continue;
		}

		if( !token.Icmp( "SubImage" )) {
			if( !src.ExpectTokenString( "{" )) {
				src.Error( "sdDeclImposter::Parse: SubImage expected {." );
				return false;
			}

			sdImposterSubImage item;
			int numTexCoords = 0;
			while (1) {
				if( !src.ReadToken( &token )) {
					src.Error( "sdDeclImposter::Parse: SubImage unexpected end of file." );
					break;
				}

				if (!token.Icmp("}")) {
					break;
				}

				if (!token.Icmp("min")) {
					idVec2 minValue;
					minValue[0] = src.ParseFloat();
					minValue[1] = src.ParseInt();
					item.SetMins(minValue);
					continue;
				}

				if (!token.Icmp("max")) {
					idVec2 maxValue;
					maxValue[0] = src.ParseFloat();
					maxValue[1] = src.ParseFloat();
					item.SetMaxs(maxValue);
					continue;
				}

				if (!token.Icmp("texCoord")) {
					if (numTexCoords >= 4) {
						src.Error( "sdDeclImposter::Parse: SubImage texCoord num over %d", 4 );
						return false;
					}
					idVec2 texCoord;
					texCoord[0] = src.ParseFloat();
					texCoord[1] = src.ParseFloat();
					item.SetTexCoord(numTexCoords++, texCoord);
					continue;
				}

				src.Warning( "sdDeclImposter::Parse: SubImage unexpected token '%s'.", token.c_str() );
				src.SkipBracedSection(false);
				break;
			}
			info.images.Append(item);
			continue;
		}

		src.Warning( "sdDeclImposter::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

void sdDeclImposter::FreeData( void ) {
	imposterInfo_t_Init(info);
}

void sdDeclImposter::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "imposter", kv ) ) {
		if ( kv->GetValue().Length() ) {
			declImposterType[ kv->GetValue() ];
		}
	}
}

bool sdDeclImposter::Save( void ) {
	return true;
}

void sdDeclImposter::RebuildTextSource( void ) {

}



sdDeclImposterGenerator::sdDeclImposterGenerator( void )
	: vertexColor(false),
	  numAngles(0),
	  noBump(false),
	  startAngle(0.0f),
	  screenScale(1.0f)
{
	tileSize[0] = tileSize[1] = 0;
}

const char* sdDeclImposterGenerator::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclImposterGenerator::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclImposterGenerator::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "sourceModel" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclImposterGenerator::Parse: failed to parse sourceModel" );
				break;
			}
			sourceModel = token.c_str();
			continue;
		}

		if( !token.Icmp( "outputTexture" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclImposterGenerator::Parse: failed to parse outputTexture" );
				break;
			}
			outputTexture = token.c_str();
			continue;
		}

		if (!token.Icmp("vertexColored")) {
			vertexColor = src.ParseBool();
			continue;
		}

		if (!token.Icmp("numAngles")) {
			numAngles = src.ParseInt();
			continue;
		}

		if (!token.Icmp("noBump")) {
			noBump = src.ParseBool();
			continue;
		}

		src.Warning( "sdDeclImposterGenerator::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

void sdDeclImposterGenerator::FreeData( void ) {
	vertexColor = false;
	numAngles = 0;
	noBump = false;
	startAngle = 0.0f;
	screenScale = 1.0f;
	tileSize[0] = tileSize[1] = 0;
}
