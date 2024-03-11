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

// Copyright (C) 2004 Id Software, Inc.
//
/*
sys_event.cpp

Event are used for scheduling tasks and for linking script commands.

*/

#include "precompiled.h"
#pragma hdrstop



#include "Event.h"
#include "../Game_local.h"

#define MAX_EVENTSPERFRAME			(10<<10)
//#define CREATE_EVENT_CODE

idCVar g_eventAliveSoftLimit(
	"g_eventAliveSoftLimit", "2048", CVAR_INTEGER | CVAR_GAME,
	"Post warning when number of events alive exceeds this limit",
	1, MAX_EVENTS
);
idCVar g_eventPerFrameSoftLimit(
	"g_eventPerFrameSoftLimit", "5120", CVAR_INTEGER | CVAR_GAME,
	"Post warning when number of events processed in single frame exceeds this limit",
	1, MAX_EVENTSPERFRAME
);
idCVar g_eventNumberPrintedOnLimit(
	"g_eventNumberPrintedOnLimit", "10", CVAR_INTEGER | CVAR_GAME,
	"How many events are printed to console when soft limit is exceeded",
	0, MAX_EVENTS + MAX_EVENTSPERFRAME
);


/***********************************************************************

  idEventDef

***********************************************************************/

idEventDef *idEventDef::eventDefList[MAX_EVENTS];
int idEventDef::numEventDefs = 0;

static bool eventError = false;
static char eventErrorMsg[ 128 ];

idEventDef::idEventDef(const char* name_, const EventArgs& args_, char returnType_, const char* description_) :
	name(name_),
	description(description_),
	returnType(returnType_),
	args(args_)
{
	Construct();
}

idEventDef::~idEventDef()
{
}

void idEventDef::Construct()
{
	idEventDef		*ev;
	int				i;
	unsigned int	bits;

	assert(name != NULL);
	assert(!idEvent::initialized);
	
	for ( int i = 0; i < args.size(); i++ )
		formatspec[i] = args[i].type;
	formatspec[args.size()] = '\0';
	
	numargs = static_cast<int>(strlen(formatspec));
	assert( numargs <= D_EVENT_MAXARGS );
	if ( numargs > D_EVENT_MAXARGS ) {
		eventError = true;
		sprintf( eventErrorMsg, "idEventDef::idEventDef : Too many args for '%s' event.", name );
		return;
	}

	if (idStr::Length(description) == 0)
	{
		description = "No description";
	}

	// make sure the format for the args is valid, calculate the formatspecindex, and the offsets for each arg
	bits = 0;
	argsize = 0;
	memset( argOffset, 0, sizeof( argOffset ) );

	for( i = 0; i < numargs; i++ )
	{
		argOffset[i] = static_cast<int>(argsize);

		switch( formatspec[ i ] )
		{
		case D_EVENT_FLOAT :
			bits |= 1 << i;
			argsize += sizeof(float);
			break;

		case D_EVENT_INTEGER :
			argsize += sizeof(int);
			break;

		case D_EVENT_ENTITY :
		case D_EVENT_ENTITY_NULL :
			argsize += sizeof(idEntityPtr<idEntity>);
			break;

		case D_EVENT_VECTOR :
			argsize += sizeof(idVec3);
			break;

		case D_EVENT_STRING :
			argsize += MAX_STRING_LEN;
			break;

		case D_EVENT_TRACE :
			argsize += sizeof( trace_t ) + MAX_STRING_LEN + sizeof( bool );
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
	for( i = 0; i < eventnum; i++ ) {
		ev = eventDefList[ i ];
		if ( strcmp( name, ev->name ) == 0 ) {
			if ( strcmp( formatspec, ev->formatspec ) != 0 ) {
				eventError = true;
				sprintf( eventErrorMsg, "idEvent '%s' defined twice with same name but differing format strings ('%s'!='%s').",
					name, formatspec, ev->formatspec );
				return;
			}

			if ( ev->returnType != returnType ) {
				eventError = true;
				sprintf( eventErrorMsg, "idEvent '%s' defined twice with same name but differing return types ('%c'!='%c').",
					name, returnType, ev->returnType );
				return;
			}
			// Don't bother putting the duplicate event in list.
			eventnum = ev->eventnum;
			return;
		}
	}

	ev = this;

	if ( numEventDefs >= MAX_EVENTS ) {
		eventError = true;
		sprintf( eventErrorMsg, "numEventDefs >= MAX_EVENTS" );
		return;
	}
	eventDefList[numEventDefs] = ev;
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
		if ( strcmp( name, ev->name ) == 0 ) {
			return ev;
		}
	}

	return NULL;
}


static bool CompareEventDefs(idEventDef *a, idEventDef *b) {
	//note: idEventDef::Construct ensures that event defs have distinct names
	int cmp = idStr::Cmp(a->GetName(), b->GetName());
	return cmp < 0;
}

void idEventDef::SortEventDefs() {
	for (int i = 0; i < numEventDefs; i++)
		for (int j = 0; j < i; j++)
			if (CompareEventDefs(eventDefList[i], eventDefList[j]))
				idSwap(eventDefList[i], eventDefList[j]);
}

/***********************************************************************

  idEvent

***********************************************************************/

static idLinkList<idEvent> FreeEvents;
static int FreeEventsNum = 0;
static idLinkList<idEvent> EventQueue;
static idEvent EventPool[ MAX_EVENTS ];

bool idEvent::initialized = false;

idDynamicBlockAlloc<byte, 16 * 1024, 256>	idEvent::eventDataAllocator;

/*
================
idEvent::~idEvent()
================
*/
idEvent::~idEvent() {
	Free();
}

/*
================
idEvent::Alloc
================
*/
idEvent *idEvent::Alloc( const idEventDef *evdef, int numargs, va_list args ) {
	idEvent		*ev;
	size_t		size;
	const char	*format;
	idEventArg	*arg;
	byte		*dataPtr;
	int			i;
	const char	*materialName;

	int nonFreeNum = sizeof(EventPool) / sizeof(EventPool[0]) - FreeEventsNum;
#if _DEBUG
	//stgatilov: check that free events counter is valid
	if (nonFreeNum <= 100) {	//avoid wasting too much time
		int aliveNum = EventQueue.Num();
		assert(aliveNum == nonFreeNum || aliveNum + 1 == nonFreeNum);
	}
#endif

	if ( FreeEvents.IsListEmpty() ) {
		gameLocal.Error( "idEvent::Alloc : No more free events" );
	}
	if ( nonFreeNum >= g_eventAliveSoftLimit.GetInteger() ) {
		static int previousPrintTime = 0;
		if ( gameLocal.realClientTime - previousPrintTime > 5000 ) {	//spam every 5 seconds
			previousPrintTime = gameLocal.realClientTime;
			gameLocal.Warning( "Soft limit of alive events exceeded (%d)! Some events printed below:", nonFreeNum );
			idCmdArgs args;
			args.AppendArg("");
			args.AppendArg(g_eventNumberPrintedOnLimit.GetString());
			Cmd_EventList_f(args);
		}
	}

	ev = FreeEvents.Next();
	ev->eventNode.Remove();
	FreeEventsNum--;

	ev->eventdef = evdef;

	if ( numargs != evdef->GetNumArgs() ) {
		gameLocal.Error( "idEvent::Alloc : Wrong number of args for '%s' event.", evdef->GetName() );
	}

	size = evdef->GetArgSize();
	if ( size ) {
		ev->data = eventDataAllocator.Alloc(static_cast<int>(size));
		memset( ev->data, 0, size );
	} else {
		ev->data = NULL;
	}

	format = evdef->GetArgFormat();
	for( i = 0; i < numargs; i++ ) {
		arg = va_arg( args, idEventArg * );
		if ( format[ i ] != arg->type ) {
			// when NULL is passed in for an entity, it gets cast as an integer 0, so don't give an error when it happens
			if ( !( ( ( format[ i ] == D_EVENT_TRACE ) || ( format[ i ] == D_EVENT_ENTITY ) ) && ( arg->type == 'd' ) && ( arg->value == 0 ) ) ) {
				gameLocal.Error( "idEvent::Alloc : Wrong type passed in for arg # %d on '%s' event.", i, evdef->GetName() );
			}
		}

		dataPtr = &ev->data[ evdef->GetArgOffset( i ) ];

		switch( format[ i ] ) {
		case D_EVENT_FLOAT :
		case D_EVENT_INTEGER :
			*reinterpret_cast<int *>( dataPtr ) = arg->value;
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

		case D_EVENT_ENTITY :
		case D_EVENT_ENTITY_NULL :
			*reinterpret_cast< idEntityPtr<idEntity> * >( dataPtr ) = reinterpret_cast<idEntity *>( arg->value );
			break;

		case D_EVENT_TRACE :
			if ( arg->value ) {
				*reinterpret_cast<bool *>( dataPtr ) = true;
				*reinterpret_cast<trace_t *>( dataPtr + sizeof( bool ) ) = *reinterpret_cast<const trace_t *>( arg->value );

				// save off the material as a string since the pointer won't be valid in save games.
				// since we save off the entire trace_t structure, if the material is NULL here,
				// it will be NULL when we process it, so we don't need to save off anything in that case.
				if ( reinterpret_cast<const trace_t *>( arg->value )->c.material ) {
					materialName = reinterpret_cast<const trace_t *>( arg->value )->c.material->GetName();
					idStr::Copynz( reinterpret_cast<char *>( dataPtr + sizeof( bool ) + sizeof( trace_t ) ), materialName, MAX_STRING_LEN );
				}
			} else {
				*reinterpret_cast<bool *>( dataPtr ) = false;
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
void idEvent::CopyArgs( const idEventDef *evdef, int numargs, va_list args, intptr_t data[ D_EVENT_MAXARGS ] ) {
	int			i;
	const char	*format;
	idEventArg	*arg;

	format = evdef->GetArgFormat();
	if ( numargs != evdef->GetNumArgs() ) {
		gameLocal.Error( "idEvent::CopyArgs : Wrong number of args for '%s' event.", evdef->GetName() );
	}

	for( i = 0; i < numargs; i++ ) {
		arg = va_arg( args, idEventArg * );
		if ( format[ i ] != arg->type ) {
			// when NULL is passed in for an entity, it gets cast as an integer 0, so don't give an error when it happens
			if ( !( ( ( format[ i ] == D_EVENT_TRACE ) || ( format[ i ] == D_EVENT_ENTITY ) ) && ( arg->type == 'd' ) && ( arg->value == 0 ) ) ) {
				gameLocal.Error( "idEvent::CopyArgs : Wrong type passed in for arg # %d on '%s' event.", i, evdef->GetName() );
			}
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

	eventdef	= NULL;
	time		= 0;
	object		= NULL;
	typeinfo	= NULL;

	eventNode.SetOwner( this );
	eventNode.AddToEnd( FreeEvents );
	FreeEventsNum++;
}

/*
================
idEvent::Schedule
================
*/
void idEvent::Schedule( idClass *obj, const idTypeInfo *type, int time ) {
	idEvent *event;

	assert( initialized );
	if ( !initialized ) {
		return;
	}

	object = obj;
	typeinfo = type;

	// wraps after 24 days...like I care. ;)
	this->time = gameLocal.time + time;

	eventNode.Remove();

	event = EventQueue.Next();
	while( ( event != NULL ) && ( this->time >= event->time ) ) {
		event = event->eventNode.Next();
	}

	if ( event ) {
		eventNode.InsertBefore( event->eventNode );
	} else {
		eventNode.AddToEnd( EventQueue );
	}
}

/*
================
idEvent::CancelEvents
================
*/
void idEvent::CancelEvents( const idClass *obj, const idEventDef *evdef ) {
	idEvent *event;
	idEvent *next;

	if ( !initialized ) {
		return;
	}

	for( event = EventQueue.Next(); event != NULL; event = next ) {
		next = event->eventNode.Next();
		if ( event->object == obj ) {
			if ( !evdef || ( evdef == event->eventdef ) ) {
				event->Free();
			}
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
	FreeEvents.Clear();
	EventQueue.Clear();
	FreeEventsNum = 0;
   
	// 
	// add the events to the free list
	//
	for( i = 0; i < MAX_EVENTS; i++ ) {
		EventPool[ i ].Free();
	}
}

//stgatilov: some informative labels suitable for tracing
//ideally, it should match natvis definitions...
idStr GetTraceLabel(const idEvent &evt) {
	assert( g_tracingEnabled );
	if ( evt.eventdef == &EV_Thread_Execute ) {
		return idStr("thread: ") + static_cast<idThread*>(evt.object)->GetThreadName();
	} else {
		idStr res = evt.typeinfo->classname + idStr("::") + evt.eventdef->GetName();
		if ( evt.object->IsType( idEntity::Type ) )
			res += idStr(": ") + static_cast<idEntity*>(evt.object)->GetName();
		return res;
	}
}

/*
================
idEvent::ServiceEvents
================
*/
void idEvent::ServiceEvents( void ) {
	idEvent		*event;
	int			num;
	intptr_t	args[ D_EVENT_MAXARGS ];
	int			offset;
	int			i;
	int			numargs;
	const char	*formatspec;
	trace_t		**tracePtr;
	const idEventDef *ev;
	byte		*data;
	const char  *materialName;

	TRACE_CPU_SCOPE( "idEvent::ServiceEvents" )

	num = 0;
	while( !EventQueue.IsListEmpty() ) {
		event = EventQueue.Next();
		assert( event );

		if ( event->time > gameLocal.time ) {
			break;
		}

		TRACE_CPU_SCOPE_STR ("Service:Event", GetTraceLabel(*event) )

		// copy the data into the local args array and set up pointers
		ev = event->eventdef;
		formatspec = ev->GetArgFormat();
		numargs = ev->GetNumArgs();
		for( i = 0; i < numargs; i++ ) {
			offset = ev->GetArgOffset( i );
			data = event->data;
			switch( formatspec[ i ] ) {
			case D_EVENT_FLOAT :
			case D_EVENT_INTEGER :
				args[ i ] = *reinterpret_cast<int *>( &data[ offset ] );
				break;

			case D_EVENT_VECTOR :
				*reinterpret_cast<idVec3 **>( &args[ i ] ) = reinterpret_cast<idVec3 *>( &data[ offset ] );
				break;

			case D_EVENT_STRING :
				*reinterpret_cast<const char **>( &args[ i ] ) = reinterpret_cast<const char *>( &data[ offset ] );
				break;

			case D_EVENT_ENTITY :
			case D_EVENT_ENTITY_NULL :
				*reinterpret_cast<idEntity **>( &args[ i ] ) = reinterpret_cast< idEntityPtr<idEntity> * >( &data[ offset ] )->GetEntity();
				break;

			case D_EVENT_TRACE :
				tracePtr = reinterpret_cast<trace_t **>( &args[ i ] );
				if ( *reinterpret_cast<bool *>( &data[ offset ] ) ) {
					*tracePtr = reinterpret_cast<trace_t *>( &data[ offset + sizeof( bool ) ] );

					if ( ( *tracePtr )->c.material != NULL ) {
						// look up the material name to get the material pointer
						materialName = reinterpret_cast<const char *>( &data[ offset + sizeof( bool ) + sizeof( trace_t ) ] );
						( *tracePtr )->c.material = declManager->FindMaterial( materialName, true );
					}
				} else {
					*tracePtr = NULL;
				}
				break;

			default:
				gameLocal.Error( "idEvent::ServiceEvents : Invalid arg format '%s' string for '%s' event.", formatspec, ev->GetName() );
			}
		}

		num++;
		if ( num == g_eventPerFrameSoftLimit.GetInteger() ) {
			gameLocal.Warning( "Soft limit of %d events per frame exceeded! Some events printed below:", num );
		}
		//stgatilov: print information about events, so that mappers can debug the issue
		if ( num >= g_eventPerFrameSoftLimit.GetInteger() && num < g_eventPerFrameSoftLimit.GetInteger() + g_eventNumberPrintedOnLimit.GetInteger() ) {
			event->Print();
		}

		// the event is removed from its list so that if then object
		// is deleted, the event won't be freed twice
		event->eventNode.Remove();
		assert( event->object );
		event->object->ProcessEventArgPtr( ev, args );

		// return the event to the free list
		event->Free();

		// Don't allow ourselves to stay in here too long.  An abnormally high number
		// of events being processed is evidence of an infinite loop of events.
		if ( num > MAX_EVENTSPERFRAME ) {
			gameLocal.Error( "Event overflow.  Possible infinite loop in script." );
		}
	}

	if ( num >= g_eventPerFrameSoftLimit.GetInteger() ) {
		gameLocal.Warning( "Soft limit of events per frame exceeded (%d)! See events above.", num );
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

	//#4549: sort event definitions for savegames compatibility between different platforms
	//currently they are ordered by static initialization order, which is not defined between CPP files
	idEventDef::SortEventDefs();

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

	// say it is now shut down
	initialized = false;
}

/*
================
idEvent::Save
================
*/
void idEvent::Save( idSaveGame *savefile ) {
	char *str;
	int i;
	size_t size = 0; // grayman #3649 - initialize
	idEvent	*event;
	byte *dataPtr;
	bool validTrace;
	const char	*format;

	savefile->WriteInt( EventQueue.Num() );

	event = EventQueue.Next();
	while( event != NULL ) {
		savefile->WriteInt( event->time );
		savefile->WriteString( event->eventdef->GetName() );
		savefile->WriteString( event->typeinfo->classname );
		savefile->WriteObject( event->object );
		savefile->WriteInt(static_cast<int>(event->eventdef->GetArgSize()));
		format = event->eventdef->GetArgFormat();
		for ( i = 0, size = 0; i < event->eventdef->GetNumArgs(); ++i) {
			dataPtr = &event->data[ event->eventdef->GetArgOffset( i ) ];
			switch( format[ i ] ) {
				case D_EVENT_FLOAT :
					savefile->WriteFloat( *reinterpret_cast<float *>( dataPtr ) );
					size += sizeof( float );
					break;
				case D_EVENT_INTEGER :
					savefile->WriteInt( *reinterpret_cast<int *>( dataPtr ) );
					size += sizeof( int );
					break;
				case D_EVENT_ENTITY :
				case D_EVENT_ENTITY_NULL :
					reinterpret_cast< idEntityPtr<idEntity> * >( dataPtr )->Save(savefile);
					size += sizeof( idEntityPtr<idEntity> );
					break;
				case D_EVENT_VECTOR :
					savefile->WriteVec3( *reinterpret_cast<idVec3 *>( dataPtr ) );
					size += sizeof( idVec3 );
					break;
				case D_EVENT_TRACE :
					validTrace = *reinterpret_cast<bool *>( dataPtr );
					savefile->WriteBool( validTrace );
					size += sizeof( bool );
					if ( validTrace ) {
						size += sizeof( trace_t );
						const trace_t &t = *reinterpret_cast<trace_t *>( dataPtr + sizeof( bool ) );
						SaveTrace( savefile, t );
						if ( t.c.material ) {
							size += MAX_STRING_LEN;
							str = reinterpret_cast<char *>( dataPtr + sizeof( bool ) + sizeof( trace_t ) );
							savefile->Write( str, MAX_STRING_LEN );
						}
					}
					break;
				case D_EVENT_STRING : // grayman #3649 - wasn't being handled
					size += MAX_STRING_LEN;
					str = reinterpret_cast<char *>( dataPtr );
					savefile->Write( str, MAX_STRING_LEN );
					break;
				default:
					break;
			}
		}
		assert( size == event->eventdef->GetArgSize() );
		event = event->eventNode.Next();
	}
}

/*
================
idEvent::Restore
================
*/
void idEvent::Restore( idRestoreGame *savefile ) {
	char    *str;
	int		num, i, j, size = 0, argsize = 0; // grayman #3649 - initialize 'size'
	idStr	name;
	byte *dataPtr;
	idEvent	*event;
	const char	*format;

	savefile->ReadInt( num );

	for ( i = 0; i < num; i++ ) {
		if ( FreeEvents.IsListEmpty() ) {
			gameLocal.Error( "idEvent::Restore : No more free events" );
		}

		event = FreeEvents.Next();
		event->eventNode.Remove();
		event->eventNode.AddToEnd( EventQueue );
		FreeEventsNum--;

		savefile->ReadInt( event->time );

		// read the event name
		savefile->ReadString( name );
		event->eventdef = idEventDef::FindEvent( name );
		if ( !event->eventdef ) {
			savefile->Error( "idEvent::Restore: unknown event '%s'", name.c_str() );
		}

		// read the classtype
		savefile->ReadString( name );
		event->typeinfo = idClass::GetClass( name );
		if ( !event->typeinfo ) {
			savefile->Error( "idEvent::Restore: unknown class '%s' on event '%s'", name.c_str(), event->eventdef->GetName() );
		}

		savefile->ReadObject( event->object );

		// read the args
		savefile->ReadInt( argsize );
		if ( argsize != int(event->eventdef->GetArgSize()) ) {
			savefile->Error( "idEvent::Restore: arg size (%d) doesn't match saved arg size(%d) on event '%s'", int(event->eventdef->GetArgSize()), argsize, event->eventdef->GetName() );
		}
		if ( argsize ) {
			event->data = eventDataAllocator.Alloc( argsize );
			format = event->eventdef->GetArgFormat();
			assert( format );
			for ( j = 0, size = 0; j < event->eventdef->GetNumArgs(); ++j) {
				dataPtr = &event->data[ event->eventdef->GetArgOffset( j ) ];
				switch( format[ j ] ) {
					case D_EVENT_FLOAT :
						savefile->ReadFloat( *reinterpret_cast<float *>( dataPtr ) );
						size += sizeof( float );
						break;
					case D_EVENT_INTEGER :
						savefile->ReadInt( *reinterpret_cast<int *>( dataPtr ) );
						size += sizeof( int );
						break;
					case D_EVENT_ENTITY :
					case D_EVENT_ENTITY_NULL :
						reinterpret_cast< idEntityPtr<idEntity> * >( dataPtr )->Restore(savefile);
						size += sizeof( idEntityPtr<idEntity> );
						break;
					case D_EVENT_VECTOR :
						savefile->ReadVec3( *reinterpret_cast<idVec3 *>( dataPtr ) );
						size += sizeof( idVec3 );
						break;
					case D_EVENT_TRACE :
						savefile->ReadBool( *reinterpret_cast<bool *>( dataPtr ) );
						size += sizeof( bool );
						if ( *reinterpret_cast<bool *>( dataPtr ) ) {
							size += sizeof( trace_t );
							trace_t &t = *reinterpret_cast<trace_t *>( dataPtr + sizeof( bool ) );
							RestoreTrace( savefile,  t) ;
							if ( t.c.material ) {
								size += MAX_STRING_LEN;
								str = reinterpret_cast<char *>( dataPtr + sizeof( bool ) + sizeof( trace_t ) );
								savefile->Read( str, MAX_STRING_LEN );
							}
						}
						break;
					case D_EVENT_STRING : // grayman #3649 - wasn't being handled
						size += MAX_STRING_LEN;
						str = reinterpret_cast<char *>( dataPtr );
						savefile->Read( str, MAX_STRING_LEN );
						break;
					default:
						break;
				}
			}
			assert( size == (int)event->eventdef->GetArgSize() );
		} else {
			event->data = NULL;
		}
	}
}

/*
 ================
 idEvent::ReadTrace
 
 idRestoreGame has a ReadTrace procedure, but unfortunately idEvent wants the material
 string name at the of the data structure rather than in the middle
 ================
 */
void idEvent::RestoreTrace( idRestoreGame *savefile, trace_t &trace ) {
	savefile->ReadFloat( trace.fraction );
	savefile->ReadVec3( trace.endpos );
	savefile->ReadMat3( trace.endAxis );
	savefile->ReadInt( (int&)trace.c.type );
	savefile->ReadVec3( trace.c.point );
	savefile->ReadVec3( trace.c.normal );
	savefile->ReadFloat( trace.c.dist );
	savefile->ReadInt( trace.c.contents );
	savefile->ReadInt( (int&)trace.c.material );
	savefile->ReadInt( trace.c.contents );
	savefile->ReadInt( trace.c.modelFeature );
	savefile->ReadInt( trace.c.trmFeature );
	savefile->ReadInt( trace.c.id );
}

/*
 ================
 idEvent::WriteTrace

 idSaveGame has a WriteTrace procedure, but unfortunately idEvent wants the material
 string name at the of the data structure rather than in the middle
================
 */
void idEvent::SaveTrace( idSaveGame *savefile, const trace_t &trace ) {
	savefile->WriteFloat( trace.fraction );
	savefile->WriteVec3( trace.endpos );
	savefile->WriteMat3( trace.endAxis );
	savefile->WriteInt( trace.c.type );
	savefile->WriteVec3( trace.c.point );
	savefile->WriteVec3( trace.c.normal );
	savefile->WriteFloat( trace.c.dist );
	savefile->WriteInt( trace.c.contents );
	savefile->WriteInt( (int&)trace.c.material );
	savefile->WriteInt( trace.c.contents );
	savefile->WriteInt( trace.c.modelFeature );
	savefile->WriteInt( trace.c.trmFeature );
	savefile->WriteInt( trace.c.id );
}

//stgatilov: prints event to console
//used when getting to much events (exceed soft limits)
void idEvent::Print() {
	common->Printf("Event %s::%s on %s at %d\n",
		typeinfo ? typeinfo->classname : "[null]",
		eventdef ? eventdef->GetName() : "[null]",
		object ? (
			object->IsType(idEntity::Type) ? ((idEntity*)object)->name.c_str() :
			object->IsType(idThread::Type) ? ((idThread*)object)->GetThreadName() :
			"[unknown]"
		) : "[null]",
		time
	);
}

void Cmd_EventList_f(const idCmdArgs &args) {
	int limit = -1;
	if (args.Argc() > 1) {
		limit = atoi(args.Argv(1));
	}

	int num = EventQueue.Num();
	if (limit >= num/2)
		limit = -1;

	idHashMap<int, int> printIds;
	if (limit > 0) {
		idRandom rnd = gameLocal.random;
		//print last events (half of limit)
		for (int i = 0; i < limit/2; i++)
			printIds.Set(num - 1 - i, 0);
		//print random events (another half)
		while (printIds.Num() < limit)
			printIds.Set(rnd.RandomInt(num), 0);
	}

	int idx = 0;
	for (idLinkList<idEvent> *node = EventQueue.NextNode(); node; node = node->NextNode()) {
		if (limit < 0 || printIds.Find(idx))
			node->Owner()->Print();
		idx++;
	}
	common->Printf("Total: %d/%d events alive\n", num, MAX_EVENTS);
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
					string1 += "const intptr_t";
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
