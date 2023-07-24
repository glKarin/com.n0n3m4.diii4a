// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

/*
===============================================================================

	sdDeclVehiclePath

===============================================================================
*/

#include "DeclVehiclePath.h"
#include "../../framework/DeclParseHelper.h"


/*
================
sdDeclVehiclePath::sdDeclVehiclePath
================
*/
sdDeclVehiclePath::sdDeclVehiclePath( void ) {
}

/*
================
sdDeclVehiclePath::~sdDeclVehiclePath
================
*/
sdDeclVehiclePath::~sdDeclVehiclePath( void ) {
}

/*
================
sdDeclVehiclePath::DefaultDefinition
================
*/
const char* sdDeclVehiclePath::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"numsamples 1\n"		\
		"( ( 0 ) )\n"			\
		"}\n";
}

/*
================
sdDeclVehiclePath::Parse
================
*/
bool sdDeclVehiclePath::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	if ( !src.ExpectTokenString( "numsamples" ) ) {
		return false;
	}
	if ( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
		return false;
	}

	int numSamples = token.GetIntValue();

	grid.SetSize( numSamples, numSamples );

	if ( !src.Parse2DMatrix( numSamples, numSamples, grid.ToFloatPtr() ) ) {
		return false;
	}

	return true;
}

/*
================
sdDeclVehiclePath::FreeData
================
*/
void sdDeclVehiclePath::FreeData( void ) {
}

/*
================
sdDeclVehiclePath::RebuildTextSource
================
*/
bool sdDeclVehiclePath::RebuildTextSource( void ) {
	int numSamples = grid.GetNumColumns();

	idStr temp;
	
	temp += va( "vehiclePath %s {\n", GetName() );

	temp += va( "\tnumsamples %i\n", grid.GetNumColumns() );

	temp += "\t(\n";

	for ( int x = 0; x < numSamples; x++ ) {
		temp += "\t\t(";

		for ( int y = 0; y < numSamples; y++ ) {
			temp += va( " %f", grid[ x ][ y ] );
		}

		temp += " )\n";
	}
	temp += "\t)\n";
	temp += "}\n";

	SetText( temp );

	return true;
}
