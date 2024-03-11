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
sys_event.h

Event are used for scheduling tasks and for linking script commands.
*/
#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#include "../../idlib/Lib.h"
#include <cstring>
#include "EventArgs.h"

#define D_EVENT_MAXARGS				8			// if changed, enable the CREATE_EVENT_CODE define in Event.cpp to generate switch statement for idClass::ProcessEventArgPtr.
												// running the game will then generate c:\doom\base\events.txt, the contents of which should be copied into the switch statement.

//stgatilov: this is required to make 32-bit an 64-bit scripting systems compatible on binary level
static_assert(sizeof(idVec3) == 12, "Scripting system assumes idVec3 has 12-bytes size on every platform");

#define D_EVENT_VOID				( ( char )0 )
#define D_EVENT_INTEGER				'd'
#define D_EVENT_FLOAT				'f'
#define D_EVENT_VECTOR				'v'
#define D_EVENT_STRING				's'
#define D_EVENT_ENTITY				'e'
#define	D_EVENT_ENTITY_NULL			'E'			// event can handle NULL entity pointers
#define D_EVENT_TRACE				't'

#define MAX_EVENTS					(10<<10)		// we can have so many different events (functions) overall

#define EV_RETURNS_VOID				D_EVENT_VOID

class idClass;
class idTypeInfo;

class idEventDef 
{
private:
	const char					*name;
	char					formatspec [D_EVENT_MAXARGS + 2 ];
	unsigned int				formatspecIndex;
	int							returnType;
	int							numargs;
	size_t						argsize;
	int							argOffset[ D_EVENT_MAXARGS ];
	int							eventnum;

	// description - what does this event do?
	const char*					description;
	EventArgs					args;

	const idEventDef *			next;

	static idEventDef *			eventDefList[MAX_EVENTS];
	static int					numEventDefs;

public:

	// Define a named event with the given arguments, return type and documentation
	idEventDef(const char* command, const EventArgs& args, char returnType, const char* description);

	~idEventDef();

	const char*					GetName() const;
	const char*					GetDescription() const;
	const char*					GetArgFormat() const;
	unsigned int				GetFormatspecIndex() const;
	char						GetReturnType() const;
	int							GetEventNum() const;
	int							GetNumArgs() const;
	const EventArg&				GetArg(int argIndex) const;
	const EventArgs&			GetArgs() const;
	size_t						GetArgSize() const;
	int							GetArgOffset(int arg) const;

	static int					NumEventCommands();
	static const idEventDef		*GetEventCommand( int eventnum );
	static const idEventDef		*FindEvent( const char *name );

	//#4549 sort event defs to ensure deterministic order
	static void					SortEventDefs();

private:
	// Shared constructor
	void Construct();
};

class idSaveGame;
class idRestoreGame;
typedef struct trace_s trace_t;

class idEvent {
private:
	const idEventDef			*eventdef;
	byte						*data;
	int							time;
	idClass						*object;
	const idTypeInfo			*typeinfo;

	idLinkList<idEvent>			eventNode;

	static idDynamicBlockAlloc<byte, 16 * 1024, 256> eventDataAllocator;

	friend idStr GetTraceLabel(const idEvent &evt);
public:
	static bool					initialized;

	~idEvent();

	static idEvent				*Alloc( const idEventDef *evdef, int numargs, va_list args );
	static void					CopyArgs( const idEventDef *evdef, int numargs, va_list args, intptr_t data[ D_EVENT_MAXARGS ]  );
	
	void						Free( void );
	void						Schedule( idClass *object, const idTypeInfo *cls, int time );
	byte						*GetData( void );
	void						Print();

	static void					CancelEvents( const idClass *obj, const idEventDef *evdef = NULL );
	static void					ClearEventList( void );
	static void					ServiceEvents( void );
	static void					Init( void );
	static void					Shutdown( void );

	// save games
	static void					Save( idSaveGame *savefile );					// archives object for save game file
	static void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	static void					SaveTrace( idSaveGame *savefile, const trace_t &trace );
	static void					RestoreTrace( idRestoreGame *savefile, trace_t &trace );
	
};

void Cmd_EventList_f(const idCmdArgs &args);

/*
================
idEvent::GetData
================
*/
ID_INLINE byte *idEvent::GetData( void ) {
	return data;
}

/*
================
idEventDef::GetName
================
*/
ID_INLINE const char *idEventDef::GetName( void ) const {
	return name;
}

ID_INLINE const char* idEventDef::GetDescription() const
{
	return description;
}

/*
================
idEventDef::GetArgFormat
================
*/
ID_INLINE const char *idEventDef::GetArgFormat( void ) const {
	return formatspec;
}

/*
================
idEventDef::GetFormatspecIndex
================
*/
ID_INLINE unsigned int idEventDef::GetFormatspecIndex( void ) const {
	return formatspecIndex;
}

/*
================
idEventDef::GetReturnType
================
*/
ID_INLINE char idEventDef::GetReturnType( void ) const {
	return returnType;
}

/*
================
idEventDef::GetNumArgs
================
*/
ID_INLINE int idEventDef::GetNumArgs( void ) const {
	return numargs;
}

ID_INLINE const EventArg& idEventDef::GetArg(int argIndex) const
{
	return args[argIndex];
}

ID_INLINE const EventArgs& idEventDef::GetArgs() const
{
	return args;
}

/*
================
idEventDef::GetArgSize
================
*/
ID_INLINE size_t idEventDef::GetArgSize( void ) const {
	return argsize;
}

/*
================
idEventDef::GetArgOffset
================
*/
ID_INLINE int idEventDef::GetArgOffset( int arg ) const {
	assert( ( arg >= 0 ) && ( arg < D_EVENT_MAXARGS ) );
	return argOffset[ arg ];
}

/*
================
idEventDef::GetEventNum
================
*/
ID_INLINE int idEventDef::GetEventNum( void ) const {
	return eventnum;
}

#endif /* !__SYS_EVENT_H__ */
