// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclGUI.h"
#include "../../framework/DeclParseHelper.h"
#include "../guis/UserInterfaceManager.h"
#include "../../framework/Licensee.h"

const char sdDeclGUIProperty_Identifier[] = "sdDeclGUIProperty";
const char sdDeclGUITimeEvent_Identifier[] = "sdDeclGUITimeEvent";
const char sdDeclGUITimeline_Identifier[] = "sdDeclGUITimeline";
const char sdDeclGUIEvent_Identifier[] = "sdDeclGUIEvent";
const char sdDeclGUIWindow_Identifier[] = "sdDeclGUIWindow";

/*
===============================================================================

	sdDeclGUIWindow

===============================================================================
*/

idStrPool sdDeclGUIWindow::s_strPool;

/*
================
sdDeclGUIWindow::sdDeclGUIWindow
================
*/
sdDeclGUIWindow::sdDeclGUIWindow( void ) : properties(1), events(1), children(1)
#ifdef _DEBUG
,breakOnDraw( false ), breakOnLayout( false ) 
#endif
{
	type = s_strPool.AllocString( "window" );
}

/*
================
sdDeclGUIWindow::~sdDeclGUIWindow
================
*/
sdDeclGUIWindow::~sdDeclGUIWindow( void ) {
	properties.DeleteContents( true );
	events.DeleteContents( true );
}

/*
================
sdDeclGUIWindow::ParseEvents
================
*/
bool sdDeclGUIWindow::ParseEvents( idList< sdDeclGUIEvent* >& events, idLexer& src ) {
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;
	while ( true ) {
		if( !src.ReadToken( &token ) ) {
			src.Error( "Unexpected End of File" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		sdDeclGUIEvent* event = new sdDeclGUIEvent();
		if ( !event->Parse( src, token ) ) {
			delete event;
			return false;
		}

		events.Alloc() = event;
	}

	return true;
}

/*
================
sdDeclGUIWindow::ParseProperties
================
*/
bool sdDeclGUIWindow::ParseProperties( idList< sdDeclGUIProperty* >& properties, idLexer& src ) {
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;
	while ( true ) {
		if( !src.ReadToken( &token ) ) {
			src.Error( "Unexpected End of File" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		sdDeclGUIProperty* property = new sdDeclGUIProperty();
		if ( !property->Parse( src, token ) ) {
			delete property;
			return false;
		}

		properties.Alloc() = property;
	}

	return true;
}

/*
================
sdDeclGUITimelineHolder::ParseTimeline
================
*/
bool sdDeclGUITimelineHolder::ParseTimeline( idLexer& src ) {	

	idToken token;
	if( !src.ReadToken( &token ) ) {
		src.Error( "Unexpected End of File" );
		return false;
	}

	sdDeclGUITimeline* timeline = NULL;

	const char* name = GUI_DEFAULT_TIMELINE_NAME;
	if( token == "{" ) {
		src.UnreadToken( &token );
	} else {
		name = token.c_str();
	}

	declTimelineHash_t::Iterator iter = timelines.Find( name );
	if( iter == timelines.End() ) {
		timeline = new sdDeclGUITimeline;
		timelines.Set( name, timeline );
	} else {
		timeline = iter->second;
	}

	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}
	
	while ( true ) {
		if( !src.ReadToken( &token ) ) {
			src.Error( "Unexpected End of File" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "properties" ) ) {
			if ( !sdDeclGUIWindow::ParseProperties( timeline->GetProperties(), src ) ) {
				src.Error( "Error Parsing Properties" );
				return false;
			}
			continue;
		}

		sdDeclGUITimeEvent* timeEvent = new sdDeclGUITimeEvent();
		if ( !timeEvent->Parse( src, token ) ) {
			delete timeEvent;
			return false;
		}

		timeline->GetEvents().Alloc() = timeEvent;
	}

	return true;
}

/*
================
sdDeclGUIWindow::Parse
================
*/
bool sdDeclGUIWindow::Parse( idLexer& src, sdDeclGUI* declGui ) {
	idToken token;
	if ( !src.ReadToken( &token ) ) {
		src.Error( "Unexpected End of File" );
		return false;
	}

	if ( token.type != TT_NAME ) {
		src.Error( "Invalid Window Name '%s'", name.c_str() );
		return false;
	}

	name = token;

	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	while ( true ) {
		if( !src.ReadToken( &token ) ) {
			src.Error( "%s Unexpected End of File", name.c_str() );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

#ifdef _DEBUG
		if ( !token.Icmp( "breakOnParse" ) ) {
			assert( 0 );
			continue;
		}
#endif

		if ( !token.Icmp( "windowDef" ) ) {
			sdDeclGUIWindow* window = declGui->ParseWindow( src );
			if( window == NULL ) {
				return false;
			}
			children.Append( window->GetName() );
			continue;
		}

		if ( !token.Icmp( "breakOnDraw" ) ) {
#ifdef _DEBUG
			breakOnDraw = true;
#endif
			continue;
		}

		if ( !token.Icmp( "breakOnLayout" ) ) {
#ifdef _DEBUG
			breakOnLayout = true;
#endif
			continue;
		}

		if ( !token.Icmp( "type" ) ) {
			if ( !src.ReadToken( &token ) ) {
				src.Error( "%s Unexpected End of File", name.c_str() );
				return false;
			}
			type = s_strPool.AllocString( token.c_str() );

			src.ExpectTokenString( ";" );
		} else if ( !token.Icmp( "properties" ) ) {
			if ( !ParseProperties( properties, src ) ) {
				src.Error( "%s Error Parsing Properties", name.c_str() );
				return false;
			}
		} else if ( !token.Icmp( "events" ) ) {
			if ( !ParseEvents( events, src ) ) {
				src.Error( "%s Error Parsing Events", name.c_str() );
				return false;
			}
		} else if ( !token.Icmp( "timeline" ) ) {
			if ( !timelines.ParseTimeline( src ) ) {
				src.Error( "%s Error Parsing Timeline", name.c_str() );
				return false;
			}
		} else {
			src.Error( "%s unexpected token '%s'", name.c_str(), token.c_str() );
			return false;
		}
	}

	return true;
}


/*
============
sdDeclGUITimelineHolder::GetTimeline
============
*/
const sdDeclGUITimeline* sdDeclGUITimelineHolder::GetTimeline( int index ) const {
	return timelines.FindIndex( index )->second;
}


/*
============
sdDeclGUITimelineHolder::GetTimelineName
============
*/
const char*	sdDeclGUITimelineHolder::GetTimelineName( int index ) const {
	return timelines.FindIndex( index )->first;
}

/*
===============================================================================

	sdDeclGUITimeEvent

===============================================================================
*/

/*
================
sdDeclGUITimeEvent::sdDeclGUITimeEvent
================
*/
sdDeclGUITimeEvent::sdDeclGUITimeEvent( void ) {
	startTime = -1;
}

/*
================
sdDeclGUITimeEvent::~sdDeclGUITimeEvent
================
*/
sdDeclGUITimeEvent::~sdDeclGUITimeEvent( void ) {
}

/*
================
sdDeclGUITimeEvent::Parse
================
*/
bool sdDeclGUITimeEvent::Parse( idLexer& src, const idToken& nameToken ) {
	tokens.Clear();

	int line = src.GetLineNum();

	if ( nameToken.Icmp( "onTime" ) ) {
		src.Error( "%s Events in the Timeline Must be Called onTime", nameToken.c_str() );
		return false;
	}

	idToken token;
	if ( !src.ReadToken( &token ) ) {
		src.Error( "%s Unexpected End of File", nameToken.c_str() );
		return false;
	}

	if ( token.type != TT_NUMBER || !( token.subtype & TT_INTEGER ) ) {
		src.Error( "%s Start Time Must be an Integer", nameToken.c_str() );
		return false;
	}

	startTime = token.GetIntValue();

	if( !sdDeclGUI::ParseBracedSection( src, tokens ) ) {
		src.Error( "%s Error Parsing Text", nameToken.c_str() );
		return false;
	}

	return true;
}




/*
===============================================================================

	sdDeclGUIEvent

===============================================================================
*/

/*
================
sdDeclGUIEvent::sdDeclGUIEvent
================
*/
sdDeclGUIEvent::sdDeclGUIEvent( void ) : flags( 1 ) {
	name		= -1;
}

/*
================
sdDeclGUIEvent::~sdDeclGUIEvent
================
*/
sdDeclGUIEvent::~sdDeclGUIEvent( void ) {
}

/*
================
sdDeclGUIEvent::Parse
================
*/
bool sdDeclGUIEvent::Parse( idLexer& src, const idToken& nameToken ) {
	int line = src.GetLineNum();

	if ( nameToken.type != TT_NAME ) {
		src.Error( "'%s' Invalid Name For Event", nameToken.c_str() );
		return false;
	}

	name = nameToken.GetBinaryIndex();

	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "%s Unexpected End of File", nameToken.c_str() );
			return false;
		}

		if ( !token.Cmp( "{" ) ) {
			src.UnreadToken( &token );
			break;
		}

		flags.Alloc() = token.GetBinaryIndex();
	}

	if ( !sdDeclGUI::ParseBracedSection( src, tokens ) ) {
		src.Error( "%s Error Parsing Event Text", nameToken.c_str() );
		return false;
	}

	return true;
}

/*
===============================================================================

	sdDeclGUIProperty

===============================================================================
*/

/*
================
sdDeclGUIProperty::sdDeclGUIProperty
================
*/
sdDeclGUIProperty::sdDeclGUIProperty( void ) {
	type		= -1;
	name		= -1;
}

/*
================
sdDeclGUIProperty::~sdDeclGUIProperty
================
*/
sdDeclGUIProperty::~sdDeclGUIProperty( void ) {
}

/*
================
sdDeclGUIProperty::Parse
================
*/
bool sdDeclGUIProperty::Parse( idLexer& src, const idToken& typeName ) {
	type		= typeName.GetBinaryIndex();

	idToken token;
	if ( !src.ReadToken( &token ) ) {
		src.Error( "Unexpected End of File" );
		return false;
	}

	if ( token.type != TT_NAME ) {
		src.Error( "'%s' Invalid Name for Property", token.c_str() );
		return false;
	}

	idStr nameStr = token;
	name = token.GetBinaryIndex();

	if ( !src.ReadToken( &token ) ) {
		src.Error( "Unexpected End of File" );
		return false;
	}

	if ( !token.Icmp( ";" ) ) {
		return true;
	} else if ( !token.Icmp( "=" ) ) {
		while ( true ) {
			if ( !src.ReadToken( &token ) ) {
				src.Error( "%s Unexpected End of File", nameStr.c_str() );
				return false;
			}

			if ( !token.Cmp( "}" ) ) {
				src.Error( "%s Unexpected '}'", nameStr.c_str() );
				return false;
			}

			if ( !token.Icmp( ";" ) ) {
				if ( value.Empty() ) {
					src.Error( "%s Unexpected ';'", nameStr.c_str() );
					return false;
				}
				value.Append( token.GetBinaryIndex() );
				break;
			} else {
				value.Append( token.GetBinaryIndex() );
			}
		}
		return true;
	}

	src.Error( "Unexpected Token '%s'", token.c_str() );
	return false;
}









/*
===============================================================================

	sdDeclGUI

===============================================================================
*/


idStrList sdDeclGUI::defines( 32 );

/*
================
sdDeclGUI::sdDeclGUI
================
*/
sdDeclGUI::sdDeclGUI( void ) :
	windows( 8 ),
	properties( 1 ),
	events( 1 )
#ifdef _DEBUG
 ,breakOnDraw( false ) 
#endif

{
}

/*
================
sdDeclGUI::~sdDeclGUI
================
*/
sdDeclGUI::~sdDeclGUI( void ) {
	FreeData();
}

/*
================
sdDeclGUI::DefaultDefinition
================
*/
const char* sdDeclGUI::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"\twindowDef desktop {\n"	\
		"\t}\n"						\
		"}\n";
}


/*
============
sdDeclGUI::ParseWindow
============
*/
sdDeclGUIWindow* sdDeclGUI::ParseWindow( idLexer& src ) {
	sdDeclGUIWindow* window = new sdDeclGUIWindow();
	if ( !window->Parse( src, this ) ) {
		delete window;
		src.Error( "Error Parsing Window" );
		return NULL;
	} else {
		windows.Alloc() = window;
	}
	return window;
}

/*
================
sdDeclGUI::Parse
================
*/
bool sdDeclGUI::Parse( const char *text, const int textLength ) {		
	idToken token;
	idParser src;
	src.SetFlags( LEXER_FLAGS ); 	
	
	bool binary = HasBinaryBuffer();
	if( !binary ) {
		// init code-generated #defines
		for( int i = 0; i < defines.Num(); i++ ) {
			src.AddDefine( defines[ i ] );
		}

		src.AddDefine( "true 1" );
		src.AddDefine( "false 0" );
	}

	sdDeclParseHelper declHelper( this, text, textLength, src );
	
	const byte* parseOutput;
	int parseOutputLength;
	declHelper.GetBinaryBuffer( parseOutput, parseOutputLength );

	idLexer binarySrc;
	if( !binarySrc.LoadMemoryBinary( parseOutput, parseOutputLength, GetFileName(), &declManager->GetGlobalTokenCache() ) ) {
		binarySrc.Error( "Couldn't load binary data." );
		return false;
	}

	binarySrc.SkipUntilString( "{", &token );

	idDict tempDict;
	while( true ) {
		if ( !binarySrc.ReadToken( &token ) ) {
			binarySrc.Error( "Unexpected End of File" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "breakOnDraw" ) ) {
#ifdef _DEBUG
			breakOnDraw = true;
#endif
			continue;
		}

		if ( !token.Icmp( "windowDef" ) ) {
			sdDeclGUIWindow* window = ParseWindow( binarySrc );
			if( window == NULL ) {
				return false;
			}
		} else if ( !token.Icmp( "properties" ) ) {
			if ( !sdDeclGUIWindow::ParseProperties( properties, binarySrc ) ) {
				binarySrc.Error( "Error Parsing Properties" );
				return false;
			}
		} else if ( !token.Icmp( "events" ) ) {
			if ( !sdDeclGUIWindow::ParseEvents( events, binarySrc ) ) {
				binarySrc.Error( "Error Parsing Events" );
				return false;
			}
		} else if ( !token.Icmp( "timeline" ) ) {
			if ( !timelines.ParseTimeline( binarySrc ) ) {
				binarySrc.Error( "Error Parsing Timeline" );
				return false;
			}
		} else if ( !token.Icmp( "sounds" ) ) {
			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing sound table" );
				return false;
			}
			sounds.SetDefaults( &tempDict );
		} else if ( !token.Icmp( "materials" ) ) {
			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing material table" );
				return false;
			}
			materials.SetDefaults( &tempDict );
		} else if ( !token.Icmp( "colors" ) ) {
			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing color table" );
				return false;
			}
			colors.SetDefaults( &tempDict );
		} else if ( !token.Icmp( "atmospheres" ) ) {
			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing atmosphere pre-cache table" );
				return false;
			}
			for( int i = 0 ; i < tempDict.GetNumKeyVals(); i++ ) {
				const idKeyValue* kv = tempDict.GetKeyVal( i );
				if( !kv->GetKey().IsEmpty() ) {
					gameLocal.declAtmosphereType[ kv->GetKey().c_str() ];
				}
			}			
		} else if ( !token.Icmp( "models" ) ) {
			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing models pre-cache table" );
				return false;
			}
			for( int i = 0 ; i < tempDict.GetNumKeyVals(); i++ ) {
				const idKeyValue* kv = tempDict.GetKeyVal( i );
				if( !kv->GetKey().IsEmpty() ) {
					renderModelManager->FindModel( kv->GetKey().c_str() );
				}
			}
		} else if ( !token.Icmp( "touchFiles" ) ) {
			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing file touch pre-cache table" );
				return false;
			}
			for( int i = 0 ; i < tempDict.GetNumKeyVals(); i++ ) {
				const idKeyValue* kv = tempDict.GetKeyVal( i );
				if( !kv->GetKey().IsEmpty() ) {
					sdFilePtr f( fileSystem->OpenFileRead( kv->GetKey().c_str(), true ) );
						if( !f.IsValid() ) {
							binarySrc.Warning( "Could not touch '%s'", kv->GetKey().c_str() );
						}
					}
				}
			}
			else if ( !token.Icmp( "touchFolders" ) ) {
				tempDict.Clear();
				if ( !tempDict.Parse( binarySrc ) ) {
					binarySrc.Error( "Error Parsing folder touch pre-cache table" );
					return false;
				}
				for( int i = 0 ; i < tempDict.GetNumKeyVals(); i++ ) {
					const idKeyValue* kv = tempDict.GetKeyVal( i );
					if( !kv->GetKey().IsEmpty() ) {
						idFileList* fl = fileSystem->ListFiles( kv->GetKey().c_str(), kv->GetValue().c_str() );
						sdStringBuilder_Heap base;

						for( int i = 0; i < fl->GetNumFiles(); i++ ) {
							base = fl->GetBasePath();
							base += "/";
							base += fl->GetFile( i );
							sdFilePtr f( fileSystem->OpenFileRead( base.c_str(), true ) );
							assert( f.IsValid() );
						}
						fileSystem->FreeFileList( fl );
					}
				}
		} else if ( !token.Icmp( "partialLoadModels" ) ) {
			// these aren't touched here and must be explicitly loaded by code
			// make sure any models referenced here are precached properly!

			tempDict.Clear();
			if ( !tempDict.Parse( binarySrc ) ) {
				binarySrc.Error( "Error Parsing partial load models pre-cache table" );
				return false;
			}
			for( int i = 0 ; i < tempDict.GetNumKeyVals(); i++ ) {
				const idKeyValue* kv = tempDict.GetKeyVal( i );
				if( !kv->GetKey().IsEmpty() ) {
					partialLoadModels.Append( kv->GetKey() );
				}
			}
		} else {
			binarySrc.Error( "Unknown token '%s'", token.c_str() );
			return false;
		}
	}

	// jrad - precaching
	const idKeyValue* kv;
	CacheMaterialDictionary( GetName(), materials );

	for( int i = 0 ; i < sounds.GetNumKeyVals(); i++ ) {
		kv = sounds.GetKeyVal( i );
		if( kv->GetValue().Length() ) {
			gameLocal.declSoundShaderType[ kv->GetValue() ];
		}		
	}

	return true;
}


/*
============
sdDeclGUI::CacheMaterialDictionary
============
*/
void sdDeclGUI::CacheMaterialDictionary( const char* name, const idDict& materials ) {
	const idKeyValue* kv;
	idToken token;
	for( int i = 0 ; i < materials.GetNumKeyVals(); i++ ) {
		kv = materials.GetKeyVal( i );
		if( kv->GetValue().Length() ) {
			idLexer src( kv->GetValue(), kv->GetValue().Length(), "LookupMaterial", LEXFL_ALLOWPATHNAMES );
			// jrad - gross copy and paste from sdUIWindow::ParseMaterial
			while( !src.HadError() ) {
				if( !src.ReadToken( &token )) {
					break;
				}
				if( token.Icmp( "literal:" ) == 0 ) {
					continue;
				}

				if( token.Icmp( "_frame" ) == 0 ) {
					continue;
				}

				if( token.Icmp( "_st" ) == 0 ) {
					continue;
				}

				if( token.Icmp( "_3v" ) == 0 ) {
					continue;
				}

				if( token.Icmp( "_3h" ) == 0 ) {
					continue;
				}

				if( token.Icmp( "_5h" ) == 0 ) {
					continue;
				}

				if( token.Icmp( "_size" ) == 0 ) {
					src.ParseInt();
					src.ExpectTokenString( "," );
					src.ParseInt();
					continue;
				}

				if( token == "::" ) {
					continue;
				}
				break;
			}			

			if( gameLocal.declMaterialType[ token ] == NULL ) {
				gameLocal.Warning( "%s: Couldn't precache material '%s'", name, token.c_str() );
			}
		}
	}
}

/*
================
sdDeclGUI::FreeData
================
*/
void sdDeclGUI::FreeData( void ) {
	windows.DeleteContents( true );

	properties.DeleteContents( true );
	events.DeleteContents( true );
	timelines.Clear();

	colors.Clear();
	materials.Clear();
	sounds.Clear();
	partialLoadModels.Clear();
}

/*
================
sdDeclGUI::CacheFromDict
================
*/
void sdDeclGUI::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "gui", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declGUIType[ kv->GetValue() ];
		}
	}
}

/*
============
sdDeclGUI::ParseBracedSection
============
*/
bool sdDeclGUI::ParseBracedSection( idLexer& src, idList< unsigned short >& tokens, const char* open, const char* close ) {
	idToken token;
	if( !src.ReadToken( &token ) ) {
		return false;
	}
	if( token != open ) {
		return false;
	}
	tokens.Append( token.GetBinaryIndex() );
	
	int numBraces = 1;
	while( numBraces > 0 ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}
		tokens.Append( token.GetBinaryIndex() );
		if( token.Cmp( open ) == 0 ) {
			numBraces++;
			continue;
		}
		if( token.Cmp( close ) == 0 ) {
			numBraces--;
			continue;
		}		
	}
	tokens.Condense();
	return true;
}


/*
============
sdDeclGUI::CreateConstructor
============
*/
bool sdDeclGUI::CreateConstructor( const idList<sdDeclGUIProperty*>& properties, idList<unsigned short>& tokens, idTokenCache& tokenCache ) {
	tokens.Clear();

	idToken temp;
	temp = "{";
	unsigned short openBrace = tokenCache.FindToken( temp );
	temp = "}";
	unsigned short closeBrace = tokenCache.FindToken( temp );
	temp = "=";
	unsigned short equals = tokenCache.FindToken( temp );

	idList< unsigned short > constructorTokens;
	tokens.Append( openBrace );
	
	bool hasValues = false;
	for( int i = 0; i < properties.Num(); i++ ) {
		sdDeclGUIProperty* prop = properties[ i ];
		if( prop->GetValue().Empty() ) {
			continue;
		}
		tokens.Append( prop->GetName() );
		tokens.Append( equals );
		tokens.Append( prop->GetValue() );
		hasValues = true;
	}
	tokens.Append( closeBrace );
	return hasValues;
}
