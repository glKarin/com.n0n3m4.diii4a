// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclToolTip.h"
#include "../../framework/KeyInput.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclToolTip

===============================================================================
*/

/*
================
sdDeclToolTip::sdDeclToolTip
================
*/
sdDeclToolTip::sdDeclToolTip( void ) {
	sound			= NULL;
}

/*
================
sdDeclToolTip::~sdDeclToolTip
================
*/
sdDeclToolTip::~sdDeclToolTip( void ) {
	FreeData();
}

/*
================
sdDeclToolTip::DefaultDefinition
================
*/
const char* sdDeclToolTip::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"text \"MISSING TEXT\""	\
		"}\n";
}

/*
================
sdDeclToolTip::Parse
================
*/
bool sdDeclToolTip::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	timeline.Clear();
	useSoundLength = false;
	singlePlayerToolTip = false;
	unpauseWeaponSlot = -1;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "text" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'text'" );
				return false;
			}

			if ( !AddMessage( common->LocalizeText( token.c_str() ).c_str() ) ) {
				return false;
			}

		} else if ( !token.Icmp( "category" ) ) {

			if( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'category'" );
				return false;
			}

			category = token;

		} else if ( !token.Icmp( "locationIndex" ) ) {

			if ( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'locationIndex'" );
				return false;
			}

			locationIndex = token.GetIntValue();

		} else if ( !token.Icmp( "maxPlayCount" ) ) {

			if ( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'maxPlayCount'" );
				return false;
			}

			maxPlayCount = token.GetIntValue();

		} else if ( !token.Icmp( "alwaysPlay" ) ) {

			maxPlayCount = -1;

		} else if( !token.Icmp( "length" ) ) {

			if ( !src.ExpectTokenType( TT_NUMBER, 0, &token )  ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'length'" );
				return false;
			}

			if ( timeline.Num() > 0 ) {
				src.Error( "sdDeclToolTip::Parse length should be set before the tooltip timeline" );
				return false;
			}

			length = SEC2MS( token.GetFloatValue() );

		} else if( !token.Icmp( "nextShowDelay" ) ) {


			if ( !src.ExpectTokenType( TT_NUMBER, 0, &token )  ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'nextShowDelay'" );
				return false;
			}

			nextShowDelay = SEC2MS( token.GetFloatValue() );
			
		} else if( !token.Icmp( "sound" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'text'" );
				return false;
			}
			if( token.Length() > 0 ) {
				sound = declHolder.declSoundShaderType.LocalFind( token );
			}

		} else if( !token.Icmp( "icon" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclToolTip::Parse Invalid Parm For 'icon'" );
				return false;
			}

			icon = gameLocal.declMaterialType[ token.c_str() ];
			if ( !icon ) {
				src.Error( "sdDeclToolTip::Parse Invalid Icon '%s'", token.c_str() );
				return false;
			}

		} else if ( !token.Icmp( "timeline" ) ) {

			if ( !ParseTimeline( src ) ) {
				return false;
			}

		} else if ( !token.Icmp( "useSoundLength" ) ) {

			if ( timeline.Num() > 0 ) {
				src.Error( "sdDeclToolTip::Parse useSoundLength should be set before the tooltip timeline" );
				return false;
			}

			useSoundLength = true;

		} else if ( !token.Icmp( "singlePlayerToolTip" ) ) {

			singlePlayerToolTip = true;

		} else if ( !token.Icmp( "lookAtObjective" ) ) {

			lookAtObjective = true;

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclToolTip::Parse Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclToolTip::ParseTimeline
================
*/
bool sdDeclToolTip::ParseTimeline( idParser& src ) {
	idToken token;

	src.SkipUntilString( "{", &token );

	while ( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "onTime" ) ) {
		
			src.ReadToken( &token );

			int time;
			if ( token.type == TT_NUMBER ) {
				time = ( token.GetIntValue() / 100.0f ) * GetLength();
			} else if ( token.type == TT_NAME && !token.Icmp( "end" )  ) {
				time = TLTIME_END;
			} else {
				src.Error( "sdDeclToolTip::ParseTimeline number expected for 'onTime'" );
				return false;
			}

			timelinePair_t event;
			event.first = time;

			if ( timeline.Num() > 0 ) {
				timelinePair_t lastEvent = timeline.Back();
				if ( lastEvent.first > time && time != TLTIME_END ) {
					src.Error( "sdDeclToolTip::ParseTimeline time  events must be in increasing order: '%i'", time );
					return false;
				}
			}

			src.ReadToken( &token );

			if ( !token.Icmp( "guiEvent" ) ) {
				
				event.second.eventType = TL_GUIEVENT;

				if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
					src.Error( "sdDeclToolTip::ParseTimeline string expected after 'guiEvent'" );
					return false;
				}

				event.second.arg1 = token;

			} else if ( !token.Icmp( "pause" ) ) {
				event.second.eventType = TL_PAUSE;
			} else if ( !token.Icmp( "unpause" ) ) {
				event.second.eventType = TL_UNPAUSE;
			} else if ( !token.Icmp( "showInventory" ) ) {

				event.second.eventType = TL_SHOWINVENTORY;

				if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
					src.Error( "sdDeclToolTip::ParseTimeline string expected after 'guiEvent'" );
					return false;
				}

				event.second.arg1 = token;

			} else if ( !token.Icmp( "hideInventory" ) ) {
				event.second.eventType = TL_HIDEINVENTORY;
			} else if ( !token.Icmp( "waypointHighlight" ) )  {

				event.second.eventType = TL_WAYPOINTHIGHLIGHT;

				if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
					src.Error( "sdDeclToolTip::ParseTimeline string expected after 'guiEvent'" );
					return false;
				}

				event.second.arg1 = token;

			} else if ( !token.Icmp( "lookAtTask" ) ) {

				event.second.eventType = TL_LOOKATTASK;

			} else {
				src.Error( "sdDeclToolTip::ParseTimeline unexpected timeline event '%s'", token.c_str() );
				return false;
			}


			timeline.Append( event );

		} else if ( !token.Icmp( "unpauseWeaponSlot" ) ) {

			if( !src.ExpectTokenType( TT_NUMBER, 0, &token ) ) {
				src.Error( "sdDeclToolTip::ParseTimeline number expected after 'unpauseWeaponSlot'" );
				return false;
			}

			unpauseWeaponSlot = token.GetIntValue();

			if ( unpauseWeaponSlot > 9 || unpauseWeaponSlot < 0 ) {
				src.Warning( "sdDeclToolTip::ParseTimeline 0-9 expected as value for 'unpauseWeaponSlot'" );
				unpauseWeaponSlot = -1;
			}

			unpauseKeyString.SetKey( va( "_weapon%i", unpauseWeaponSlot - 1 ) );

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclToolTip::ParseTimeline Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclToolTip::AddMessage
================
*/
bool sdDeclToolTip::AddMessage( const wchar_t *text ) {
	message_t* message = new message_t;
	messages.Alloc() = message;

	idWStr buffer;
	
	const wchar_t* start = text;
	const wchar_t* p;
	for ( p = text; *p; p++ ) {
		if ( *p != L'%' ) {
			buffer += *p;
			continue;
		}

		p++;
		if ( !*p ) {
			break;
		}

		if ( *p >= L'a' && *p <= (L'a' + 9) ) {
			if ( buffer.Length() ) {
				message->blurbs.Alloc() = new sdDeclToolTipOptionText( buffer.c_str() );
				buffer.Clear();
			}

			message->blurbs.Alloc() = new sdDeclToolTipOptionParm( *p - L'a' );
		} else if ( *p == L'k' ) {
			if ( buffer.Length() ) {
				message->blurbs.Alloc() = new sdDeclToolTipOptionText( buffer.c_str() );
				buffer.Clear();
			}

			p++;
			if ( *p != L'(' ) {
				return false;
			}

			idWStr keyBuffer;
			while ( true ) {
				p++;
				if ( !*p ) {
					return false;
				}
				if ( *p == L')' ) {
					break;
				}
				keyBuffer += *p;
			}

			message->blurbs.Alloc() = new sdDeclToolTipOptionKey( va( "%ls", keyBuffer.c_str() ) );

		} else {
			buffer += *p;
			continue;
		}
	}

	if ( buffer.Length() ) {
		message->blurbs.Alloc() = new sdDeclToolTipOptionText( buffer.c_str() );
	}

	return true;
}

/*
================
sdDeclToolTip::FreeData
================
*/
void sdDeclToolTip::FreeData( void ) {
	messages.DeleteContents( true );
	lastTimeUsed	= 0;
	length			= SEC2MS( 3.5 );
	sound			= NULL;
	category		= "";
	locationIndex	= -1;
	maxPlayCount	= 3;
	nextShowDelay	= SEC2MS( 10 );
	icon			= NULL;
	lookAtObjective	= false;
}

/*
================
sdDeclToolTip::GetMessage
================
*/
void sdDeclToolTip::GetMessage( sdToolTipParms* formatting, idWStr& text ) const {
	text.Clear();

	if ( !messages.Num() ) {
		return;
	}

	int num = gameLocal.random.RandomInt( messages.Num() );

	message_t* message = messages[ num ];

	for ( int i = 0; i < message->blurbs.Num(); i++ ) {
		text += message->blurbs[ i ]->GetText( formatting );
	}
}

/*
================
sdDeclToolTip::CacheFromDict
================
*/
void sdDeclToolTip::CacheFromDict( const idDict& dict ) {
	const idKeyValue *kv;

	kv = NULL;
	while( kv = dict.MatchPrefix( "tt_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declToolTipType[ kv->GetValue() ];
		}
	}
}

/*
================
sdDeclToolTip::DumpToFile
================
*/
void sdDeclToolTip::DumpToFile( idFile* file ) const {
	sdToolTipParms parms;

	file->Printf( "Name: %s\n", GetName() );
	for ( int i = 0; i < messages.Num(); i++ ) {
		file->Printf( "Text %d: ", i + 1 );
		for ( int j = 0; j < messages[ i ]->blurbs.Num(); j++ ) {
			file->Printf( "%ls", messages[ i ]->blurbs[ j ]->GetText( &parms ) );
		}
		file->Printf( "\n" );
	}
	file->Printf( "\n" );
}

/*
================
sdDeclToolTip::Cmd_DumpTooltips_f
================
*/
void sdDeclToolTip::Cmd_DumpTooltips_f( const idCmdArgs& args ) {
	idFile* toolTipDump = fileSystem->OpenFileWrite( "tooltips.txt" );
	if ( toolTipDump == NULL ) {
		gameLocal.Warning( "Failed to open tooltips.txt" );
		return;
	}

	for ( int i = 0; i < gameLocal.declToolTipType.Num(); i++ ) {
		gameLocal.declToolTipType[ i ]->DumpToFile( toolTipDump );
	}

	fileSystem->CloseFile( toolTipDump );
}

/*
================
sdDeclToolTip::Cmd_ClearCookies_f
================
*/
void sdDeclToolTip::Cmd_ClearCookies_f( const idCmdArgs& args ) {
	for ( int i = 0; i < gameLocal.declToolTipType.Num(); i++ ) {
		gameLocal.declToolTipType[ i ]->ClearCookies();
	}
	gameLocal.Printf( "Cleared %i tooltip cookies\n", gameLocal.declToolTipType.Num() );
}

/*
================
sdDeclToolTip::SetLastTimeUsed
================
*/
void sdDeclToolTip::SetLastTimeUsed( void ) const {
	lastTimeUsed = sys->Milliseconds();
	gameLocal.SetCookieInt( va( "%s_play_count", GetName() ), GetCurrentPlayCount() + 1 );

	// don't play the tooltip again if already played in single player
	if ( singlePlayerToolTip && gameLocal.GetLocalPlayer() && gameLocal.isServer ) {
		gameLocal.SetCookieInt( va( "%s_sp_play_count", GetName() ), 1 );
	}
}

/*
================
sdDeclToolTip::GetCurrentPlayCount
================
*/
int sdDeclToolTip::GetCurrentPlayCount( void ) const {
	return gameLocal.GetCookieInt( va( "%s_play_count", GetName() ) );
}

/*
================
sdDeclToolTip::GetCurrentSinglePlayerPlayCount
================
*/
int sdDeclToolTip::GetCurrentSinglePlayerPlayCount( void ) const {
	return gameLocal.GetCookieInt( va( "%s_sp_play_count", GetName() ) );
}

/*
================
sdDeclToolTip::ClearCookies
================
*/
void sdDeclToolTip::ClearCookies( void ) const {
	if ( GetCurrentPlayCount() != 0 ) {
		gameLocal.SetCookieInt( va( "%s_play_count", GetName() ), 0 );
	}

	if ( GetCurrentSinglePlayerPlayCount() != 0 ) {
		gameLocal.SetCookieInt( va( "%s_sp_play_count", GetName() ), 0 );
	}
}


/*
===============================================================================

	sdDeclToolTipOptionKey

===============================================================================
*/

/*
================
sdDeclToolTipOptionKey::GetText
================
*/
const wchar_t* sdDeclToolTipOptionKey::GetText( sdToolTipParms* formatting ) const {
	if ( key.Length() <= 0 ) {
		return L"";
	}
	keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), key, true, cache );
	return cache.c_str();
}
