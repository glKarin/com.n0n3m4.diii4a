// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "renderer/Image.h"

#include "declRenderBinding.h"
#include "framework/DeclParseHelper.h"

/*
===============================================================================

sdDeclRenderBinding

===============================================================================
*/

const char* sdDeclRenderBinding::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclRenderBinding::Parse( const char* text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	bool ret = false;
	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclRenderBinding::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "vector" )) {
			ret = ParseVector(src);
			if( !ret ) {
				src.Error( "sdDeclRenderBinding::Parse: failed to parse vector" );
			}
			break;
		}

		if( !token.Icmp( "attrib" )) {
			ret = ParseAttrib(src);
			if( !ret ) {
				src.Error( "sdDeclRenderBinding::Parse: failed to parse attrib" );
			}
			break;
		}

		if( !token.Icmp( "texture" )) {
			ret = ParseTexture(src);
			if( !ret ) {
				src.Error( "sdDeclRenderBinding::Parse: failed to parse texture" );
			}
			break;
		}

		if( !token.Icmp( "infrequent" )) {
			infrequent = 1;
			continue;
		}

		src.Warning( "sdDeclRenderBinding::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	if (ret)
		data = defaults;

	return ret;
}

void sdDeclRenderBinding::FreeData() {
	type = BT_VECTOR;

	defaults.vector[0] = defaults.vector[1] = defaults.vector[2] = defaults.vector[3] = 0.0f;

	infrequent = 0;

	data = defaults;
}

void sdDeclRenderBinding::List( void ) const {
}

bool sdDeclRenderBinding::ParseVector( idParser& src ) {
	idToken	token;
	type = BT_VECTOR;
	defaults.vector[0] = defaults.vector[1] = defaults.vector[2] = 0.0f;
	defaults.vector[3] = 1.0f;
	int i = 0;

	src.ExpectTokenString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclRenderBinding::ParseVector: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Cmp(",")) {
			continue;
		}

		src.UnreadToken(&token);
		defaults.vector[i++] = src.ParseFloat();
	}
	float last = i > 0 ? defaults.vector[i - 1] : 0.0f;
	while(i < 4)
	{
		defaults.vector[i] = last;
		i++;
	}

	return true;
}

bool sdDeclRenderBinding::ParseTexture( idParser& src ) {
	idToken	token;
	type = BT_TEXTURE;
	defaults.texture.defaultCubeMap = CF_2D;
	defaults.texture.defaultDepth = TD_DIFFUSE;
	defaults.texture.image = NULL;
	idStr parseBuffer;

	src.ExpectTokenString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclRenderBinding::ParseVector: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("cubemap")) {
			defaults.texture.defaultCubeMap = CF_NATIVE;
			continue;
		}

		if (!token.Icmp("diffuse")) {
			defaults.texture.defaultDepth = TD_DIFFUSE;
			continue;
		}

		if (!token.Icmp("specular")) {
			defaults.texture.defaultDepth = TD_SPECULAR;
			continue;
		}

		if (!token.Icmp("forceHighQuality")) {
			defaults.texture.defaultDepth = TD_HIGH_QUALITY;
			continue;
		}

		if (!token.Icmp("local")) {
			defaults.texture.defaultDepth = TD_BUMP;
			continue;
		}

		// add a leading space if not at the beginning
		if (parseBuffer[0]) {
			parseBuffer.Append(" ");
		}
		parseBuffer.Append(token.c_str());
	}

	if (!parseBuffer.IsEmpty()) {
		//karin: HARDCODE: as cube map if image name end with CubeMap
		if(defaults.texture.defaultCubeMap != CF_NATIVE)
		{
			int index = parseBuffer.Find("CubeMap", false);
			if(index != -1 && index == parseBuffer.Length() - 7)
				defaults.texture.defaultCubeMap = CF_NATIVE;
		}

		defaults.texture.image = globalImages->ImageFromFile(parseBuffer.c_str(), TF_DEFAULT, defaults.texture.defaultDepth != TD_HIGH_QUALITY, TR_CLAMP, defaults.texture.defaultDepth, defaults.texture.defaultCubeMap);
		if (!defaults.texture.image) {
			common->Warning("renderBinding '%s' image '%s' not loaded", GetName(), parseBuffer.c_str());
			defaults.texture.image = globalImages->defaultImage;
		}
	}
	else {
		common->Warning("renderBinding '%s' no image", GetName());
		defaults.texture.image = globalImages->defaultImage;
	}

	return true;
}

bool sdDeclRenderBinding::ParseAttrib( idParser& src ) {
	type = BT_ATTRIB;

	defaults.attrib	= src.ParseInt();

	return true;
}
