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
idDeclTable::TableLookup
=================
*/
float idDeclTable::TableLookup( float index ) const {
	int iIndex;
	float iFrac;
	
	int domain = values.Num() - 1;

	if ( domain <= 1 ) {
		return 1.0f;
	}

	if ( clamp ) {
		index *= (domain-1);
		if ( index >= domain - 1 ) {
			return values[domain - 1];
		} else if ( index <= 0 ) {
			return values[0];
		}
		iIndex = idMath::FtoiTrunc( index );
		iFrac = index - iIndex;
	} else {
		index *= domain;

		if ( index < 0 ) {
			index += domain * idMath::Ceil( -index / domain );
		}

		iIndex = idMath::FtoiRound( idMath::Floor( index ) );
		iFrac = index - iIndex;
		iIndex = iIndex % domain;
	}

	if ( !snap ) {
		// we duplicated the 0 index at the end at creation time, so we
		// don't need to worry about wrapping the filter
		return values[iIndex] * ( 1.0f - iFrac ) + values[iIndex + 1] * iFrac;
	}
	
	return values[iIndex];
}

/*
=================
idDeclTable::Size
=================
*/
size_t idDeclTable::Size( void ) const {
	return sizeof( idDeclTable ) + values.Allocated();
}

/*
=================
idDeclTable::FreeData
=================
*/
void idDeclTable::FreeData( void ) {
	snap = false;
	clamp = false;
	values.ClearFree();
}

/*
=================
idDeclTable::DefaultDefinition
=================
*/
const char *idDeclTable::DefaultDefinition( void ) const {
	return "{ { 0 } }";
}

/*
=================
idDeclTable::Parse
=================
*/
bool idDeclTable::Parse( const char *text, const int textLength ) {
	idLexer src;
	idToken token;
	float v;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	snap = false;
	clamp = false;
	values.Clear();

	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( token.Icmp( "snap" ) == 0 ) {
			snap = true;
		} else if ( token.Icmp( "clamp" ) == 0 ) {
			clamp = true;
		} else if ( token.Icmp( "{" ) == 0 ) {

			while ( 1 ) {
				bool errorFlag;

				v = src.ParseFloat( &errorFlag );
				if ( errorFlag ) {
					// we got something non-numeric
					MakeDefault();
					return false;
				}

				values.Append( v );

				src.ReadToken( &token );
				if ( token == "}" ) {
					break;
				}
				if ( token == "," ) {
					continue;
				}
				src.Warning( "expected comma or brace" );
				MakeDefault();
				return false;
			}

		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	// copy the 0 element to the end, so lerping doesn't
	// need to worry about the wrap case
	float val = values[0];		// template bug requires this to not be in the Append()?
	values.Append( val );

	return true;
}
