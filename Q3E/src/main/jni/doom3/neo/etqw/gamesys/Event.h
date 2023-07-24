// Copyright (C) 2007 Id Software, Inc.
//
/*
sys_event.h

Event are used for scheduling tasks and for linking script commands.
*/
#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#define D_EVENT_MAXARGS				8			// if changed, enable the CREATE_EVENT_CODE define in Event.cpp to generate switch statement for idClass::ProcessEventArgPtr.
												// running the game will then generate c:\doom\base\events.txt, the contents of which should be copied into the switch statement.

#define D_EVENT_VOID				( ( char )0 )
#define D_EVENT_INTEGER				'd'
#define D_EVENT_BOOLEAN				'b'
#define D_EVENT_HANDLE				'h'
#define D_EVENT_FLOAT				'f'
#define D_EVENT_VECTOR				'v'
#define D_EVENT_STRING				's'
#define D_EVENT_WSTRING				'w'
#define D_EVENT_ENTITY				'e'
#define	D_EVENT_ENTITY_NULL			'E'			// event can handle NULL entity pointers
#define D_EVENT_OBJECT				'o'

#ifdef _XENON
const int MAX_EVENTS_BUFFER			= 2;
const int MAX_EVENTS				= 2048 + MAX_EVENTS_BUFFER;
#else
const int MAX_EVENTS_BUFFER			= 512;
const int MAX_EVENTS				= 4096 + MAX_EVENTS_BUFFER;
#endif

class idClass;
class idTypeInfo;

// #define SD_STRICT_EVENTDEFS

#if defined( SD_PUBLIC_BUILD )
#define DOC_TEXT( x ) ""
#else
#define DOC_TEXT( x ) x
#endif // SD_PUBLIC_BUILD

class idEventDef {
public:
	typedef const char* argName_t;
	typedef const char* argDescription_t;

								idEventDef( const char* command, char returnType, const char* _description, int _numArgs, const char* _comments, ... );
#if !defined( SD_STRICT_EVENTDEFS )
								idEventDef( const char* command, const char* formatspec = NULL, char returnType = 0 );
#endif // SD_STRICT_EVENTDEFS
								
	const char*					GetName( void ) const;
	const char*					GetArgFormat( void ) const;
	unsigned int				GetFormatspecIndex( void ) const;
	char						GetReturnType( void ) const;
	int							GetEventNum( void ) const;
	int							GetNumArgs( void ) const;
	unsigned int				GetArgSize( void ) const;
	unsigned int				GetArgOffset( int arg ) const;
	const char*					GetArgName( int arg ) const { return argNames[ arg ]; }
	const char*					GetArgDescription( int arg ) const { return argDescriptions[ arg ]; }
	const char*					GetDescription( void ) const { return description; }
	const char*					GetComments( void ) const { return comments; }
	bool						GetAllowFromScript( void ) const { return allowFromScript; }

	static int					NumEventCommands( void );
	static const idEventDef*	GetEventCommand( int eventnum );
	static const idEventDef*	FindEvent( const char *name );

protected:
								idEventDef( void ) { ; }

	const char*					name;
	char						formatspec[ D_EVENT_MAXARGS ];
	unsigned int				formatspecIndex;
	int							returnType;
	int							numargs;
	unsigned int				argsize;
	unsigned int				argOffset[ D_EVENT_MAXARGS ];
	int							eventnum;
	const idEventDef*			next;
	const char*					description;
	const char*					comments;
	bool						allowFromScript;

	argName_t					argNames[ D_EVENT_MAXARGS ];
	argDescription_t			argDescriptions[ D_EVENT_MAXARGS ];

	static idEventDef *			eventDefList[ MAX_EVENTS ];
	static int					numEventDefs;

	void						InitArgs( int numargs );
	bool						PreInitChecks( const char* _command );
};

class idEventDefInternal : public idEventDef {
public:
								idEventDefInternal( const char* command, const char* formatspec = NULL, char returnType = 0 );
};

class idEventArg;

class idEvent {
private:
	const idEventDef*			eventdef;
	byte*						data;
	int							time;
	idClass*					object;
	const idTypeInfo*			typeinfo;
	int							activeEventIndex;
	bool						isGuiEvent;

	idLinkList< idEvent >		eventNode;
	idLinkList< idEvent >		eventEntityNode;

	static idDynamicBlockAlloc<byte, 16 * 1024, 256, false> eventDataAllocator;


public:
	static bool					initialized;

								idEvent( void );
								~idEvent( void );

	static idEvent*				Alloc( const idEventDef *evdef, int numargs, va_list args, bool guiEvent );
	static void					CopyArgs( const idEventDef *evdef, int numargs, va_list args, UINT_PTR data[ D_EVENT_MAXARGS ]  );
	
	static bool					ArgsMatch( char format, const idEventArg* arg );

	void						Free( void );
	void						Schedule( idClass *object, const idTypeInfo *cls, int time );
	byte*						GetData( void );

	static void					CancelEvents( const idClass *obj, const idEventDef *evdef = NULL );
	static void					ClearEventList( void );
	static void					ServiceEvents( void );
	static void					ServiceEvent( idEvent* event );
	static void					Init( void );
	static void					Shutdown( void );
	static void					ListEvents( const idCmdArgs& cmdArgs );
	
	static idLinkList< idEvent >	freeEvents;
	static idLinkList< idEvent >	eventQueue;
	static idLinkList< idEvent >	guiEventQueue;
	static idLinkList< idEvent >	frameEventQueue[ 2 ];
	static idList< idEvent* >		eventsToRun;
	static int						frameQueueIndex;
	static idEvent					eventPool[ MAX_EVENTS ];
	static int						numFreeEvents;
};

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

/*
================
idEventDef::GetArgSize
================
*/
ID_INLINE unsigned int idEventDef::GetArgSize( void ) const {
	return argsize;
}

/*
================
idEventDef::GetArgOffset
================
*/
ID_INLINE unsigned int idEventDef::GetArgOffset( int arg ) const {
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
