// Copyright (C) 2007 Id Software, Inc.
//
/*
sys_event.cpp

Event are used for scheduling tasks and for linking script commands.

*/

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../script/Script_Program.h" // Gordon: needed for MAX_STRING_LEN, we should move that to events.h really

//#define CREATE_EVENT_CODE

/***********************************************************************

  idEventDef

***********************************************************************/

idEventDef *idEventDef::eventDefList[MAX_EVENTS];
int idEventDef::numEventDefs = 0;

static bool eventError = false;
static char eventErrorMsg[ 128 ];


/*
================
idEventDef::PreInitChecks
================
*/
bool idEventDef::PreInitChecks( const char* _command ) {
	if ( _command == NULL ) {
		eventError = true;
		sprintf( eventErrorMsg, "idEventDef::idEventDef : command is NULL." );
		return false;
	}

	if ( idEvent::initialized ) {
		eventError = true;
		sprintf( eventErrorMsg, "idEventDef::idEventDef : idEvent::Init has already been called." );
		return false;
	}

	memset( argNames, 0, sizeof( argNames ) );
	memset( argDescriptions, 0, sizeof( argNames ) );
	memset( formatspec, 0, sizeof( formatspec ) );

	return true;
}

/*
================
idEventDef::idEventDef
================
*/
idEventDef::idEventDef( const char* _command, char _returnType, const char* _description, int _numArgs, const char* _comments, ... ) {

	allowFromScript = true;

	if ( !PreInitChecks( _command ) ) {
		return;
	}

	va_list argptr;
	va_start( argptr, _comments );

	name			= _command;
	returnType		= _returnType;
	description		= _description;
	comments		= _comments;

	if ( _numArgs > D_EVENT_MAXARGS ) {
		eventError = true;
		sprintf( eventErrorMsg, "idEventDef::idEventDef : Too many args for '%s' event.", name );
		return;
	}

	numargs = _numArgs;
	int i;
	for ( i = 0; i < numargs; i++ ) {
		// gcc 4.1 has a bad codegen problem with va_arg( argptr, char )
		formatspec[ i ] = va_arg( argptr, const char* )[0];
		argNames[ i ] = va_arg( argptr, const char* );
		argDescriptions[ i ] = va_arg( argptr, const char* );
	}

	va_end( argptr );

	InitArgs( numargs );
}

#if !defined( SD_STRICT_EVENTDEFS )
/*
================
idEventDef::idEventDef
================
*/
idEventDef::idEventDef( const char* _command, const char* _formatspec, char _returnType ) {
	allowFromScript = true;

	if ( !PreInitChecks( _command ) ) {
		return;
	}

	// Allow NULL to indicate no args, but always store it as ""
	// so we don't have to check for it.
	if ( _formatspec == NULL ) {
		_formatspec = "";
	}

	name			= _command;
	returnType		= _returnType;
	description		= NULL;
	comments		= NULL;

	numargs = idStr::Length( _formatspec );
	if ( numargs > D_EVENT_MAXARGS ) {
		eventError = true;
		sprintf( eventErrorMsg, "idEventDef::idEventDef : Too many args for '%s' event.", name );
		return;
	}

	for ( int i = 0 ; i < numargs; i++ ) {
		formatspec[ i ] = _formatspec[ i ];
	}

	InitArgs( numargs );
}
#endif // SD_STRICT_EVENTDEFS

/*
================
idEventDef::InitArgs
================
*/
void idEventDef::InitArgs( int numargs ) {
	// make sure the format for the args is valid, calculate the formatspecindex, and the offsets for each arg
	unsigned int bits = 0;
	argsize = 0;
	memset( argOffset, 0, sizeof( argOffset ) );
	for ( int i = 0; i < numargs; i++ ) {
		argOffset[ i ] = argsize;
		switch( formatspec[ i ] ) {
		case D_EVENT_FLOAT :
			bits |= 1 << i;
			argsize += sizeof( float );
			break;

		case D_EVENT_INTEGER :
		case D_EVENT_BOOLEAN :
		case D_EVENT_HANDLE :
			argsize += sizeof( int );
			break;

		case D_EVENT_VECTOR :
			argsize += sizeof( idVec3 );
			break;

		case D_EVENT_STRING :
			argsize += MAX_STRING_LEN;
			break;

		case D_EVENT_WSTRING :
			argsize += MAX_STRING_LEN * sizeof( wchar_t );
			break;

		case D_EVENT_OBJECT :
		case D_EVENT_ENTITY :
		case D_EVENT_ENTITY_NULL :
			argsize += sizeof( int );
			break;

		default :
			eventError = true;
			sprintf( eventErrorMsg, "idEventDef::idEventDef : Invalid arg format '%s' string for '%s' event.", formatspec, name );
			return;
			break;
		}
	}

	// calculate the formatspecindex
	formatspecIndex = ( 1 << ( numargs + D_EVENT_MAXARGS ) ) | bits;

	// go through the list of defined events and check for duplicates
	// and mismatched format strings
	eventnum = numEventDefs;
	for ( int i = 0; i < eventnum; i++ ) {
		idEventDef* ev = eventDefList[ i ];
		if ( idStr::Cmp( name, ev->name ) == 0 ) {
			eventError = true;
			sprintf( eventErrorMsg, "idEvent '%s' defined twice", name );
			return;
		}
	}

	if ( numEventDefs >= MAX_EVENTS ) {
		eventError = true;
		sprintf( eventErrorMsg, "numEventDefs >= MAX_EVENTS" );
		return;
	}
	eventDefList[ numEventDefs ] = this;
	numEventDefs++;
}

/*
================
idEventDef::NumEventCommands
================
*/
int	idEventDef::NumEventCommands( void ) {
	return numEventDefs;
}

/*
================
idEventDef::GetEventCommand
================
*/
const idEventDef *idEventDef::GetEventCommand( int eventnum ) {
	return eventDefList[ eventnum ];
}

/*
================
idEventDef::FindEvent
================
*/
const idEventDef *idEventDef::FindEvent( const char *name ) {
	idEventDef	*ev;
	int			num;
	int			i;

	assert( name );

	num = numEventDefs;
	for( i = 0; i < num; i++ ) {
		ev = eventDefList[ i ];
		if ( idStr::Cmp( name, ev->name ) == 0 ) {
			return ev;
		}
	}

	return NULL;
}


/***********************************************************************

  idEventDefInternal

***********************************************************************/

/*
================
idEventDefInternal::idEventDefInternal
================
*/
idEventDefInternal::idEventDefInternal( const char* _command, const char* _formatspec, char _returnType ) {
	allowFromScript = false;

	if ( !PreInitChecks( _command ) ) {
		return;
	}

	// Allow NULL to indicate no args, but always store it as ""
	// so we don't have to check for it.
	if ( _formatspec == NULL ) {
		_formatspec = "";
	}

	name			= _command;
	returnType		= _returnType;
	description		= NULL;
	comments		= NULL;

	numargs = idStr::Length( _formatspec );
	if ( numargs > D_EVENT_MAXARGS ) {
		eventError = true;
		sprintf( eventErrorMsg, "idEventDef::idEventDef : Too many args for '%s' event.", name );
		return;
	}

	for ( int i = 0 ; i < numargs; i++ ) {
		formatspec[ i ] = _formatspec[ i ];
	}

	InitArgs( numargs );
}

/***********************************************************************

  idEvent

***********************************************************************/

idLinkList< idEvent >	idEvent::freeEvents;
idLinkList< idEvent >	idEvent::eventQueue;
idLinkList< idEvent >	idEvent::guiEventQueue;
idLinkList< idEvent >	idEvent::frameEventQueue[ 2 ];
int						idEvent::frameQueueIndex;
idEvent					idEvent::eventPool[ MAX_EVENTS ];
int						idEvent::numFreeEvents = 0;
idList< idEvent* >		idEvent::eventsToRun;

bool idEvent::initialized = false;

idDynamicBlockAlloc< byte, 16 * 1024, 256, false >	idEvent::eventDataAllocator;

/*
================
idEvent::idEvent
================
*/
idEvent::idEvent( void ) {
	activeEventIndex = -1;
	eventEntityNode.SetOwner( this );
	eventNode.SetOwner( this );
	data		= NULL;
	Free();
}

/*
================
idEvent::~idEvent
================
*/
idEvent::~idEvent( void ) {
	Free();
}

/*
================
idEvent::ListEvents
================
*/
void idEvent::ListEvents( const idCmdArgs& args ) {
	for ( int i = 0; i < 3; i++ ) {
		idEvent* event;
		if ( i == 2 ) {
			event = eventQueue.Next();
		} else {
			event = frameEventQueue[ i ].Next();
		}

		for ( ; event; event = event->eventNode.Next() ) {
			gameLocal.Printf( "%s", event->eventdef->GetName() );

			if ( event->object ) {
				idTypeInfo* type = event->object->GetType();
				gameLocal.Printf( " %s", type->classname );

				idEntity* ent = event->object->Cast< idEntity >();
				if ( ent ) {
					gameLocal.Printf( " %s", ent->GetName() );
				}

/*				sdProgramThread* thread = event->object->Cast< sdProgramThread >();
				if ( thread ) {
					gameLocal.Printf( " %s", thread->GetName() );
					const sdProgram::sdFunction* func = thread->GetInterpreter().GetCurrentFunction();
					if ( func ) {
						gameLocal.Printf( " %s %i", func->GetName(), thread->GetInterpreter().CurrentLine() );
					}
				}*/
			}

			gameLocal.Printf( "\n" );
		}
	}
}

/*
================
idEvent::ArgsMatch
================
*/
bool idEvent::ArgsMatch( char format, const idEventArg* arg ) {
	if ( format == arg->type ) {
		return true;
	}

	if ( format == D_EVENT_ENTITY_NULL && arg->type == 'e' ) {
		return true;
	}

	// when NULL is passed in for an entity, it gets cast as an integer 0, so don't give an error when it happens
	if ( format == D_EVENT_ENTITY_NULL && arg->type == 'd' && arg->value == 0 ) {
		return true;
	}

	return false;
}

/*
================
idEvent::Alloc
================
*/
idEvent *idEvent::Alloc( const idEventDef *evdef, int numargs, va_list args, bool guiEvent ) {
	idEvent		*ev;
	size_t		size;
	const char	*format;
	idEventArg	*arg;
	byte		*dataPtr;
	int			i;
	static bool inError = false;

	if ( inError ) {
		if ( numFreeEvents == 0 ) {
			common->FatalError( "idEvent::Alloc Ran Out of Events Whilst Trying to Gracefully Exit" );
		}
	} else if ( numFreeEvents <= MAX_EVENTS_BUFFER ) { // Gordon: Keeping some spare so we can maybe shut down nicely
		inError = true;
		ListEvents( idCmdArgs() );
		gameLocal.Error( "idEvent::Alloc: No More Free Events CALL A PROGRAMMER OVER NOW DO NOT CLOSE THE GAME" );
	}

	numFreeEvents--;

	ev = freeEvents.Next();
	ev->eventNode.Remove();
	ev->isGuiEvent = guiEvent;

	ev->eventdef = evdef;

	if ( numargs != evdef->GetNumArgs() ) {
		gameLocal.Error( "idEvent::Alloc : Wrong number of args for '%s' event.", evdef->GetName() );
	}

	size = evdef->GetArgSize();
	if ( size ) {
		ev->data = eventDataAllocator.Alloc( size );
//		memset( ev->data, 0, size );
	} else {
		ev->data = NULL;
	}

	format = evdef->GetArgFormat();
	for( i = 0; i < numargs; i++ ) {
		arg = va_arg( args, idEventArg* );		
		if ( !ArgsMatch( format[ i ], arg ) ) {
			gameLocal.Error( "idEvent::Alloc : Wrong type passed in for arg # %d on '%s' event.", i, evdef->GetName() );
		}

		dataPtr = &ev->data[ evdef->GetArgOffset( i ) ];

		switch( format[ i ] ) {
		case D_EVENT_FLOAT:
		case D_EVENT_INTEGER:
		case D_EVENT_BOOLEAN:
		case D_EVENT_HANDLE:
		case D_EVENT_ENTITY:
		case D_EVENT_ENTITY_NULL:
		case D_EVENT_OBJECT:
			*reinterpret_cast< int* >( dataPtr ) = arg->value;
			break;

		case D_EVENT_VECTOR :
			if ( arg->value ) {
				*reinterpret_cast<idVec3 *>( dataPtr ) = *reinterpret_cast<const idVec3 *>( arg->value );
			}
			break;

		case D_EVENT_STRING :
			if ( arg->value ) {
				idStr::Copynz( reinterpret_cast<char *>( dataPtr ), reinterpret_cast<const char *>( arg->value ), MAX_STRING_LEN );
			}
			break;

		case D_EVENT_WSTRING :
			if ( arg->value ) {
				idWStr::Copynz( reinterpret_cast<wchar_t *>( dataPtr ), reinterpret_cast<const wchar_t *>( arg->value ), MAX_STRING_LEN );
			}
			break;

		default :
			gameLocal.Error( "idEvent::Alloc : Invalid arg format '%s' string for '%s' event.", format, evdef->GetName() );
			break;
		}
	}

	return ev;
}

/*
================
idEvent::CopyArgs
================
*/
void idEvent::CopyArgs( const idEventDef *evdef, int numargs, va_list args, UINT_PTR data[ D_EVENT_MAXARGS ] ) {
	int			i;
	const char	*format;
	idEventArg	*arg;

	format = evdef->GetArgFormat();
	if ( numargs != evdef->GetNumArgs() ) {
		gameLocal.Error( "idEvent::CopyArgs : Wrong number of args for '%s' event.", evdef->GetName() );
	}

	for( i = 0; i < numargs; i++ ) {
		arg = va_arg( args, idEventArg * );		
		if ( !ArgsMatch( format[ i ], arg ) ) {
			gameLocal.Error( "idEvent::CopyArgs : Wrong type passed in for arg # %d on '%s' event.", i, evdef->GetName() );
		}

		data[ i ] = arg->value;
	}
}

/*
================
idEvent::Free
================
*/
void idEvent::Free( void ) {
	if ( data ) {
		eventDataAllocator.Free( data );
		data = NULL;
	}

	if ( activeEventIndex >= 0 ) {
		assert( activeEventIndex < eventsToRun.Num() );
		eventsToRun[ activeEventIndex ] = NULL;
		activeEventIndex = -1;
	}

	eventdef	= NULL;
	time		= 0;
	object		= NULL;
	typeinfo	= NULL;

	eventNode.AddToEnd( freeEvents );
	numFreeEvents++;

	eventEntityNode.Remove();
}

/*
================
idEvent::Schedule
================
*/
void idEvent::Schedule( idClass *obj, const idTypeInfo *type, int time ) {
	assert( initialized );

	object		= obj;
	typeinfo	= type;

	eventEntityNode.AddToEnd( obj->scheduledEvents );

	if ( time <= 0 ) {
		eventNode.AddToEnd( frameEventQueue[ frameQueueIndex ] );
		return;
	}

	// wraps after 24 days...like I care. ;)
	this->time = gameLocal.time + time;
	if ( isGuiEvent ) {
		this->time = gameLocal.ToGuiTime( this->time );
	}

	eventNode.Remove();

	idEvent* event = isGuiEvent ? guiEventQueue.Next() : eventQueue.Next();
	while( ( event != NULL ) && ( this->time >= event->time ) ) {
		event = event->eventNode.Next();
	}

	if ( event ) {
		eventNode.InsertBefore( event->eventNode );
	} else {
		eventNode.AddToEnd( isGuiEvent ? guiEventQueue : eventQueue );
	}
}

/*
================
idEvent::CancelEvents
================
*/
void idEvent::CancelEvents( const idClass* obj, const idEventDef* evdef ) {
	assert( initialized );

	if ( evdef ) {
		idEvent* next;
		for ( idEvent* ev = obj->scheduledEvents.Next(); ev; ev = next ) {
			next = ev->eventEntityNode.Next();

			if ( ev->eventdef == evdef ) {
				ev->Free();
			}
		}
	} else {		
		while ( idEvent* ev = obj->scheduledEvents.Next() ) {
			ev->Free();
		}
	}
}

/*
================
idEvent::ClearEventList
================
*/
void idEvent::ClearEventList( void ) {
	int i;

	//
	// initialize lists
	//
	freeEvents.Clear();
	numFreeEvents = 0;
	eventQueue.Clear();
	frameEventQueue[ 0 ].Clear();
	frameEventQueue[ 1 ].Clear();
	eventsToRun.SetNum( 0, false );
   
	// 
	// add the events to the free list
	//
	for( i = 0; i < MAX_EVENTS; i++ ) {
		eventPool[ i ].activeEventIndex = -1;
		eventPool[ i ].Free();
	}
}

/*
================
idEvent::ServiceEvent
================
*/
void idEvent::ServiceEvent( idEvent* event ) {
	UINT_PTR args[ D_EVENT_MAXARGS ];

	// copy the data into the local args array and set up pointers
	const idEventDef* ev = event->eventdef;
	const char*	formatspec = ev->GetArgFormat();
	int numargs = ev->GetNumArgs();
	for ( int i = 0; i < numargs; i++ ) {
		int offset = ev->GetArgOffset( i );

		byte* data = event->data;

		switch( formatspec[ i ] ) {
			case D_EVENT_FLOAT :
			case D_EVENT_INTEGER :
			case D_EVENT_BOOLEAN :
			case D_EVENT_HANDLE :
			case D_EVENT_ENTITY :
			case D_EVENT_ENTITY_NULL :
			case D_EVENT_OBJECT :
				args[ i ] = *reinterpret_cast<int *>( &data[ offset ] );
				break;

			case D_EVENT_VECTOR :
				*reinterpret_cast<idVec3 **>( &args[ i ] ) = reinterpret_cast<idVec3 *>( &data[ offset ] );
				break;

			case D_EVENT_STRING :
				*reinterpret_cast<const char **>( &args[ i ] ) = reinterpret_cast<const char *>( &data[ offset ] );
				break;

			default:
				gameLocal.Error( "idEvent::ServiceEvents : Invalid arg format '%s' string for '%s' event.", formatspec, ev->GetName() );
		}
	}

	// the event is removed from its list so that if then object
	// is deleted, the event won't be freed twice
	event->eventNode.Remove();
	assert( event->object );
	event->object->ProcessEventArgPtr( ev, args );

#if 0
	// event functions may never leave return values on the FPU stack
	// enable this code to check if any event call left values on the FPU stack
	if ( !sys->FPU_StackIsEmpty() ) {
		gameLocal.Error( "idEvent::ServiceEvents %d: %s left a value on the FPU stack", num, ev->GetName() );
	}
#endif

	// return the event to the free list
	event->Free();
}

/*
================
idEvent::ServiceEvents
================
*/
void idEvent::ServiceEvents( void ) {
	int oldFrameQueueIndex = frameQueueIndex;
	frameQueueIndex ^= 1;

	eventsToRun.SetNum( 0, false );

	if ( gameLocal.IsPaused() ) {
		for ( idEvent* evt = frameEventQueue[ oldFrameQueueIndex ].Next(); evt != NULL; evt = evt->eventNode.Next() ) {
			if ( evt->isGuiEvent ) {
				continue;
			}
			evt->eventNode.Remove();
			evt->eventNode.AddToEnd( frameEventQueue[ frameQueueIndex ] );
		}
	}

	for ( idEvent* evt = frameEventQueue[ oldFrameQueueIndex ].Next(); evt != NULL; evt = evt->eventNode.Next() ) {
		evt->activeEventIndex = eventsToRun.Num();
		eventsToRun.Alloc() = evt;
	}

	int now;
	
	now = gameLocal.time;
	for ( idEvent* evt = eventQueue.Next(); evt != NULL; evt = evt->eventNode.Next() ) {
		if ( evt->time > now ) {
			break;
		}

		evt->activeEventIndex = eventsToRun.Num();
		eventsToRun.Alloc() = evt;
	}

	now = gameLocal.ToGuiTime( gameLocal.time );
	for ( idEvent* evt = guiEventQueue.Next(); evt != NULL; evt = evt->eventNode.Next() ) {
		if ( evt->time > now ) {
			break;
		}

		evt->activeEventIndex = eventsToRun.Num();
		eventsToRun.Alloc() = evt;
	}

	for ( int i = 0; i < eventsToRun.Num(); i++ ) {
		if ( eventsToRun[ i ] == NULL ) {
			continue; // Gordon: event was removed by a previous event, i.e. entity removal, etc
		}
		ServiceEvent( eventsToRun[ i ] );
	}
}

/*
================
idEvent::Init
================
*/
void idEvent::Init( void ) {
	gameLocal.Printf( "Initializing event system\n" );

	if ( eventError ) {
		gameLocal.Error( "%s", eventErrorMsg );
	}

#ifdef CREATE_EVENT_CODE
	void CreateEventCallbackHandler();
	CreateEventCallbackHandler();
	gameLocal.Error( "Wrote event callback handler" );
#endif

	if ( initialized ) {
		gameLocal.Printf( "...already initialized\n" );
		ClearEventList();
		return;
	}

	ClearEventList();

	eventDataAllocator.Init();

	gameLocal.Printf( "...%i event definitions\n", idEventDef::NumEventCommands() );

	// the event system has started
	initialized = true;
}

/*
================
idEvent::Shutdown
================
*/
void idEvent::Shutdown( void ) {
	gameLocal.Printf( "Shutdown event system\n" );

	if ( !initialized ) {
		gameLocal.Printf( "...not started\n" );
		return;
	}

	ClearEventList();
	
	eventDataAllocator.Shutdown();

	// say it is now shutdown
	initialized = false;
}


#ifdef CREATE_EVENT_CODE
/*
================
CreateEventCallbackHandler
================
*/
void CreateEventCallbackHandler( void ) {
	int num;
	int count;
	int i, j, k;
	char argString[ D_EVENT_MAXARGS + 1 ];
	idStr string1;
	idStr string2;
	idFile *file;

	file = fileSystem->OpenFileWrite( "Callbacks.cpp" );

	file->Printf( "// generated file - see CREATE_EVENT_CODE\n\n" );

	for( i = 1; i <= D_EVENT_MAXARGS; i++ ) {

		file->Printf( "\t/*******************************************************\n\n\t\t%d args\n\n\t*******************************************************/\n\n", i );

		for ( j = 0; j < ( 1 << i ); j++ ) {
			for ( k = 0; k < i; k++ ) {
				argString[ k ] = j & ( 1 << k ) ? 'f' : 'i';
			}
			argString[ i ] = '\0';
			
			string1.Empty();
			string2.Empty();

			for( k = 0; k < i; k++ ) {
				if ( j & ( 1 << k ) ) {
					string1 += "const float";
					string2 += va( "*( float * )&data[ %d ]", k );
				} else {
					string1 += "const UINT_PTR";
					string2 += va( "data[ %d ]", k );
				}

				if ( k < i - 1 ) {
					string1 += ", ";
					string2 += ", ";
				}
			}

			file->Printf( "\tcase %d :\n\t\ttypedef void ( idClass::*eventCallback_%s_t )( %s );\n", ( 1 << ( i + D_EVENT_MAXARGS ) ) + j, argString, string1.c_str() );
			file->Printf( "\t\t( this->*( eventCallback_%s_t )callback )( %s );\n\t\tbreak;\n\n", argString, string2.c_str() );

		}
	}

	fileSystem->CloseFile( file );
}

#endif
