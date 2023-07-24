// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclTargetInfo.h"
#include "../gamesys/Class.h"
#include "../../decllib/declEntityDef.h"
#include "../Entity.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclTargetInfo

===============================================================================
*/

/*
================
sdDeclTargetInfo::sdDeclTargetInfo
================
*/
sdDeclTargetInfo::sdDeclTargetInfo( void ) {
}

/*
================
sdDeclTargetInfo::~sdDeclTargetInfo
================
*/
sdDeclTargetInfo::~sdDeclTargetInfo( void ) {
}

/*
================
sdDeclTargetInfo::DefaultDefinition
================
*/
const char* sdDeclTargetInfo::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclTargetInfo::ParseType
================
*/
bool sdDeclTargetInfo::ParseType( idParser& src, bool include ) {
	idToken token;
	
	if ( !src.ReadToken( &token ) ) {
		return false;
	}

	targetInfo_t info;
	info.include = include;

	if ( !token.Icmp( "class" )  ) {

		info.type = TI_CLASS;

		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		idTypeInfo* type = idClass::GetClass( token );
		if ( !type ) {
			src.Warning( "sdDeclTargetInfo::ParseType Unknown Class '%s'", token.c_str() );
			return false;
		}

		info.index = type->typeNum;

	} else if ( !token.Icmp( "reference" ) ) {

		info.type = TI_REFERENCE;

		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		const sdDeclTargetInfo* decl = gameLocal.declTargetInfoType[ token ];
		if ( !decl ) {
			src.Warning( "sdDeclTargetInfo::ParseType Reference '%s'", token.c_str() );
			return false;
		}

		info.index = decl->Index();

	} else if ( !token.Icmp( "collection" ) ) {

		info.type = TI_COLLECTION;

		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		info.collection = gameLocal.GetEntityCollection( token, true );

	} else {
		src.Warning( "sdDeclTargetInfo::ParseType Invalid Include/Exclude Type '%s'", token.c_str() );
		return false;
	}

	filters.Alloc() = info;
	return true;
}

/*
================
sdDeclTargetInfo::Parse
================
*/
bool sdDeclTargetInfo::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "sdDeclTargetInfo::Parse Unexpected end of file" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "include" ) ) {
			if ( !ParseType( src, true ) ) {
				src.Warning( "sdDeclTargetInfo::Parse Failure Parsing Type '%s'", token.c_str() );
				return false;
			}
		} else if ( !token.Icmp( "exclude" ) ) {
			if ( !ParseType( src, false ) ) {
				src.Warning( "sdDeclTargetInfo::Parse Failure Parsing Type '%s'", token.c_str() );
				return false;
			}
		} else if ( !token.Icmp( "require" ) ) {
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "sdDeclTargetInfo::Parse Failure Parsing Requirement" );
				return false;
			}

			requirements.Load( token.c_str() );
		} else {
			src.Warning( "sdDeclTargetInfo::Parse Unknown token '%s'", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdDeclTargetInfo::FreeData
================
*/
void sdDeclTargetInfo::FreeData( void ) {
	filters.Clear();
}

/*
================
sdDeclTargetInfo::FilterEntity
================
*/
bool sdDeclTargetInfo::FilterEntity( idEntity* entity ) const {
	if ( !entity ) {
		assert( false );
		return false;
	}

	bool keep = false;

	int i;
	for ( i = 0; i < filters.Num(); i++ ) {
		const targetInfo_t& targetInfo = filters[ i ];

		switch( targetInfo.type ) {
			case TI_CLASS: {
				idTypeInfo* type = idClass::GetType( targetInfo.index );
				if ( entity->IsType( *type ) ) {
					keep = targetInfo.include;
				}
				break;
			}
			case TI_REFERENCE: {
				const sdDeclTargetInfo *reference = gameLocal.declTargetInfoType[ targetInfo.index ];
				keep = (targetInfo.include == reference->FilterEntity( entity ));
				break;
			}
			case TI_COLLECTION: {
				if ( targetInfo.collection->Contains( entity ) ) {
					keep = targetInfo.include;
				}
				break;
			}
		}
	}

	if ( keep ) {
		keep = requirements.Check( entity );
	}

	return keep;
}

/*
================
sdDeclTargetInfo::CacheFromDict
================
*/
void sdDeclTargetInfo::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "ti_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declTargetInfoType[ kv->GetValue() ];
		}
	}
}
