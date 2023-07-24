// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DemoScript.h"
#include "DemoManager.h"

//===============================================================
//
//	Events for script
//
//===============================================================

class sdSetCameraEvent : public sdDemoScript::sdEvent {
public:
	virtual bool		Run( sdDemoScript& script ) const {
							if ( sdDemoCamera* camera = script.GetCamera( cameraName ) ) {
								sdDemoManager::GetInstance().SetActiveCamera( camera );
								return true;
							} else {
								return false;
							}
						}

	virtual bool		Parse( idParser& src ) {
							idToken token;

							if ( !src.ExpectAnyToken( &token ) ) {
								return false;
							}

							cameraName = token;

							return true;
						}

	static const char*	GetType( void ) { return "setCamera"; }

private:
	idStr				cameraName;
};

class sdClearCameraEvent : public sdDemoScript::sdEvent {
public:
	virtual bool		Run( sdDemoScript& script ) const {
							sdDemoManager::GetInstance().SetActiveCamera( NULL );
							return true;
						}

	virtual bool		Parse( idParser& src ) { return true; }

	static const char*	GetType( void ) { return "clearCamera"; }
};

class sdExecEvent : public sdDemoScript::sdEvent {
public:
	virtual bool		Run( sdDemoScript& script ) const {
							cmdSystem->BufferCommandText( CMD_EXEC_APPEND, command.c_str() );
							return true;
						}

	virtual bool		Parse( idParser& src ) {
							src.ParseRestOfLine( command );
							return true;
						}

	static const char*	GetType( void ) { return "exec"; }

private:
	idStr				command;
};

//===============================================================
//
//	sdDemoScript::sdOnTime
//
//===============================================================

/*
============
sdDemoScript::sdOnTime::Parse
============
*/
bool sdDemoScript::sdOnTime::Parse( idParser& src ) {

	time = src.ParseInt();
	
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;

	while( true ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		} else {
			sdEvent* event = sdDemoScript::CreateEvent( token.c_str() );

			if ( !event ) {
				src.Error( "sdOnTime::Parse : Unsupported event type '%s'", token.c_str() );
				return false;
			}

			if ( !event->Parse( src ) ) {
				delete event;
				return false;
			}

			events.Append( event );
		}
	}

	return true;
}

//===============================================================
//
//	sdDemoScript
//
//===============================================================

sdDemoScript::sdEventFactory sdDemoScript::eventFactory;

/*
============
sdDemoScript::InitClass
============
*/
void sdDemoScript::InitClass() {
	eventFactory.RegisterType( sdSetCameraEvent::GetType(), sdEventFactory::Allocator< sdSetCameraEvent > );
	eventFactory.RegisterType( sdClearCameraEvent::GetType(), sdEventFactory::Allocator< sdClearCameraEvent > );	
	eventFactory.RegisterType( sdExecEvent::GetType(), sdEventFactory::Allocator< sdExecEvent > );
}

/*
============
sdDemoScript::Shutdown
============
*/
void sdDemoScript::Shutdown() {
	eventFactory.Shutdown();
}

/*
============
sdDemoScript::Parse
============
*/
bool sdDemoScript::Parse( const char* fileName ) {
	idParser src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );

	idStr scriptFileName = "demos/";
	scriptFileName += fileName;

	this->fileName = fileName;
	this->fileName.StripFileExtension();

	scriptFileName.SetFileExtension( "ds" );

	src.LoadFile( scriptFileName );

	if ( !src.IsLoaded() ) {
		return false;
	}

	idToken token;

	while( true ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "camera" ) ) {
			if ( !src.ExpectAnyToken( &token ) ) {
				return false;
			}

			sdDemoCamera* camera = sdDemoManager::GetInstance().CreateCamera( token.c_str() );

			if ( !camera ) {
				src.Error( "sdDemoScript::Parse : Unsupported camera type '%s'", token.c_str() );
				return false;
			}

			if ( !camera->Parse( src ) ) {
				delete camera;
				return false;
			}

			if ( !GetCamera( camera->GetName() ) ) {
				cameras.Set( camera->GetName(), camera );
			} else {
				src.Error( "sdDemoScript::Parse : Duplicate camera name '%s'", camera->GetName() );
				delete camera;
				return false;
			}
		} else if ( !token.Icmp( "timeLine" ) ) {
			if ( !ParseTimeLine( src ) ) {
				return false;
			}
		} else {
			src.Error( "sdDemoScript::Parse : Unknown keyword '%s'", token.c_str() );
			return false;
		}
	}

	timedEvents.Sort( SortByTime );

	gameLocal.Printf( "Running demo script '%s'\n", scriptFileName.c_str() );

	Reset();

	return true;
}

/*
============
sdDemoScript::ParseTimeLine
============
*/
bool sdDemoScript::ParseTimeLine( idParser& src ) {
	
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;

	while( true ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		} else if ( !token.Icmp( "onTime" ) ) {
			sdOnTime* onTime = new sdOnTime;

			if ( !onTime->Parse( src ) ) {
				delete onTime;
				return false;
			}
            
			timedEvents.Append( onTime );
		} else {
			src.Error( "sdDemoScript::ParseTimeLine : Unknown keyword '%s'", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
============
sdDemoScript::RunFrame
============
*/
void sdDemoScript::RunFrame() {
	for ( int i = lastEvent + 1; i < timedEvents.Num(); i++ ) {
		if ( timedEvents[ i ]->GetTime() >= sdDemoManager::GetInstance().GetPreviousTime() &&
			timedEvents[ i ]->GetTime() <= sdDemoManager::GetInstance().GetTime() ) {
			timedEvents[ i ]->RunEvents( *this );

			lastEvent = i;
		}
	}
}
