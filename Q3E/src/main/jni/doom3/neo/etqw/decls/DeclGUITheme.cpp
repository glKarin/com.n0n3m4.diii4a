// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "DeclGUITheme.h"
#include "../../decllib/declTypeHolder.h"
#include "../../renderer/DeviceContext.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclGUITheme

===============================================================================
*/

/*
================
sdDeclGUITheme::sdDeclGUITheme
================
*/
sdDeclGUITheme::sdDeclGUITheme( void ) {
}

/*
================
sdDeclGUITheme::~sdDeclGUITheme
================
*/
sdDeclGUITheme::~sdDeclGUITheme( void ) {
	FreeData();
}

/*
================
sdDeclGUITheme::DefaultDefinition
================
*/
const char* sdDeclGUITheme::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"}\n";
}

/*
================
sdDeclGUITheme::Parse
================
*/
bool sdDeclGUITheme::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( LEXFL_NOSTRINGCONCAT );
 //	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString( "{", &token );

	idDict materials;
	idDict sounds;
	idDict colors;
	idDict fonts;

	while( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "sdDeclGUITheme::Parse Unexpected End of File" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "sounds" ) ) {
			sounds.Clear();
			if ( !sounds.Parse( src ) ) {
				src.Error( "sdDeclGUITheme::Parse Error Parsing sound table" );
				return false;
			}
			GetSounds().SetDefaults( &sounds );
		} else if ( !token.Icmp( "materials" ) ) {
			materials.Clear();
			if ( !materials.Parse( src ) ) {
				src.Error( "sdDeclGUITheme::Parse Error material table" );
				return false;
			}
			GetMaterials().SetDefaults( &materials );
		} else if ( !token.Icmp( "colors" ) ) {
			colors.Clear();
			if ( !colors.Parse( src ) ) {
				src.Error( "sdDeclGUITheme::Parse Error color table" );
				return false;
			}
			GetColors().SetDefaults( &colors );
		} else if ( !token.Icmp( "fonts" ) ) {
			fonts.Clear();
			if ( !fonts.Parse( src ) ) {
				src.Error( "sdDeclGUITheme::Parse Error fonts table" );
				return false;
			}
			GetFonts().SetDefaults( &fonts );
		} else {
			src.Error( "sdDeclGUITheme::Parse Unknown token '%s'", token.c_str() );
			return false;
		}
	}

	const idKeyValue* kv;
	for ( int i = 0; i < GetMaterials().GetNumKeyVals(); i++ ) {		
		kv = GetMaterials().GetKeyVal( i );
		if ( kv->GetValue().Length() ) {
			idLexer src( LEXFL_ALLOWPATHNAMES  );
			src.LoadMemory( kv->GetValue().c_str(), kv->GetValue().Length(), GetName() );
			if( src.ReadToken( &token ) ) {
				gameLocal.declMaterialType[ token.c_str() ];
			}
		}
	}
	for ( int i = 0; i < GetSounds().GetNumKeyVals(); i++ ) {
		kv = GetSounds().GetKeyVal( i );
		if ( kv->GetValue().Length() ) {
			gameLocal.declSoundShaderType[ kv->GetValue() ];
		}
	}
	for ( int i = 0; i < GetFonts().GetNumKeyVals(); i++ ) {
		kv = GetFonts().GetKeyVal( i );
		if ( kv->GetValue().Length() ) {
			qhandle_t fontHandle = deviceContext->FindFont( kv->GetValue().c_str() );
			if ( fontHandle != -1 ) {
				fontHandles.AddUnique( fontHandle );
			}
		}
	}

	return true;
}

/*
================
sdDeclGUITheme::FreeData
================
*/
void sdDeclGUITheme::FreeData( void ) {
	for ( int i = 0; i < fontHandles.Num(); i++ ) {
		deviceContext->FreeFont( fontHandles[i] );
	}
	fontHandles.Clear();

	globalMaterialTable.Clear();
	globalSoundTable.Clear();
	globalColorTable.Clear();
	globalFontsTable.Clear();
}

/*
================
sdDeclGUITheme::CacheFromDict
================
*/
void sdDeclGUITheme::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while ( kv = dict.MatchPrefix( "theme", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declGUIThemeType[ kv->GetValue() ];
		}
	}
}

/*
============
sdDeclGUITheme::GetMaterial
============
*/
const char* sdDeclGUITheme::GetMaterial( const char* material ) const {
	if ( material[ 0 ] == '\0' ) {
		return "_default";
	}

	const idKeyValue* kv = globalMaterialTable.FindKey( material );
	if ( !kv ) {
		//gameLocal.DPrintf( "UITheme: Could not find material '%s' in '%s'\n", material, GetName() );
		return "_default";
	}
	return kv->GetValue();
}

/*
============
sdDeclGUITheme::GetSound
============
*/
const char* sdDeclGUITheme::GetSound( const char* sound ) const {	
	if ( sound[ 0 ] == '\0' ) {
		return "_efault";
	}

	const idKeyValue* kv = globalSoundTable.FindKey( sound );
	if ( !kv ) {
		//gameLocal.DPrintf( "UITheme: Could not find sound '%s' in '%s'\n", sound, GetName() );
		return "default";
	}
	return kv->GetValue();
}

/*
============
sdDeclGUITheme::GetColor
============
*/
const idVec4 sdDeclGUITheme::GetColor( const char* color ) const {
	if ( color[ 0 ] == '\0' ) {
		return colorWhite;
	}

	idVec4 out;
	bool kvFound = globalColorTable.GetVec4( color, "1 1 1 1", out );
	if ( !kvFound ) {
		//gameLocal.DPrintf( "UITheme: Could not find color '%s' in '%s'\n", color, GetName() );
		return colorWhite;
	}
	return out;
}

/*
============
sdDeclGUITheme::GetFont
============
*/
const char* sdDeclGUITheme::GetFont( const char* font ) const {
	if ( font[ 0 ] == '\0' ) {
		return "_default";
	}

	const idKeyValue* kv = globalMaterialTable.FindKey( font );
	if ( !kv ) {
		gameLocal.DPrintf( "UITheme: Could not find font '%s' in '%s'\n", font, GetName() );
		return "_default";
	}
	return kv->GetValue().c_str();
}

/*
============
sdDeclGUITheme::OnReloadGUITheme
============
*/
void sdDeclGUITheme::OnReloadGUITheme( idDecl* decl ) {
	// TODO:
}
