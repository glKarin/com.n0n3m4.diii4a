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
idDeclSkin::Size
=================
*/
size_t idDeclSkin::Size( void ) const {
	return sizeof( idDeclSkin );
}

/*
================
idDeclSkin::FreeData
================
*/
void idDeclSkin::FreeData( void ) {
	mappings.ClearFree();
}

/*
================
idDeclSkin::Parse
================
*/
bool idDeclSkin::Parse( const char *text, const int textLength ) {
	idLexer src;
	idToken	token, token2;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS | LEXFL_ONLYSTRINGS );
	src.SkipUntilString( "{" );

	associatedModels.Clear();

	while (1) {
		if ( !src.ReadToken( &token ) || !token.Icmp( "}" ) ) {
			return false;
		} else if ( !src.ReadToken( &token2 ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		} else if ( !token.Icmp( "model" ) ) {
			// The list of models associated with this skin is only to guide the
			// user selection in the editor. The skin will be applied for any model
			// the entity has, regardless on whether it is in this list, or not.
			associatedModels.Append( token2 );
		} else {
			skinMapping_t	map;

			if ( !token.Icmp( "-" ) ) {
				//stgatilov: there is stupid tradition to write paths onquoted in skins =(
				src.Warning("Path with hyphens must be surrounded in double quotes");
			} else if ( !token.Icmp( "*" ) ) {
				map.from = NULL; // wildcard
			} else {
				map.from = declManager->FindMaterial( token );
			}
			map.to = declManager->FindMaterial( token2 );
			mappings.Append( map );
		}
	}
}

/*
================
idDeclSkin::SetDefaultText
================
*/
bool idDeclSkin::SetDefaultText( void ) {
	// if there exists a material with the same name
	if ( declManager->FindType( DECL_MATERIAL, GetName(), false ) ) {
		char generated[512];

		idStr::snPrintf( generated, sizeof( generated ),
						"skin %s // IMPLICITLY GENERATED\n"
						"{\n"
						"\t_default %s\n"
						"}\n", GetName(), GetName() );
		SetText( generated );
		return true;
	} else {
		return false;
	}
}

/*
================
idDeclSkin::DefaultDefinition
================
*/
const char *idDeclSkin::DefaultDefinition( void ) const {
	return
		"{\n"
		"\t\"*\"\t\"_default\"\n"
		"}\n";
}

/*
================
idDeclSkin::GetNumModelAssociations
================
*/
const int idDeclSkin::GetNumModelAssociations(void ) const {
	return associatedModels.Num(); 
}

/*
================
idDeclSkin::GetAssociatedModel
================
*/
const char *idDeclSkin::GetAssociatedModel( const int index ) const {
	if ( index >= 0 && index < associatedModels.Num() ) {
		return associatedModels[ index ];
	} else {
		return "";
	}
}

/*
===============
RemapShaderBySkin
===============
*/
const idMaterial *idDeclSkin::RemapShaderBySkin( const idMaterial *shader ) const {
	if ( !shader ) {
		return NULL;
	}

	/* // SteveL #4232: we do want to reskin collision models. Comment out this test.
	// never remap surfaces that were originally nodraw, like collision hulls
	if ( !shader->IsDrawn() ) {
		return shader;
	}
	*/

	for ( int i = 0; i < mappings.Num() ; i++ ) {
		const skinMapping_t	*map = &mappings[i];

		// NULL = wildcard match
		if ( !map->from || map->from == shader ) {
			return map->to;
		}
	}

	// didn't find a match or wildcard, so stay the same
	return shader;
}
