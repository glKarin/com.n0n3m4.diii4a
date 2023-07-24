// Copyright (C) 2007 Id Software, Inc.
//
/*

Base class for all game objects.  Provides fast run-time type checking and run-time
instancing of objects.

*/

#ifndef __SYS_CLASS_H__
#define __SYS_CLASS_H__

#include "Event.h"

const int NEXT_FRAME_EVENT_TIME = -999;

class idClass;
class idTypeInfo;
class idEventDef;
class idScriptObject;
class idAnimator;

extern const idEventDefInternal EV_Remove;
extern const idEventDef EV_SafeRemove;
extern const idEventDef EV_IsType;

typedef void ( idClass::*eventCallback_t )( void );

template< class Type >
struct idEventFunc {
	const idEventDef	*event;
	eventCallback_t		function;
};

// added & so gcc could compile this
#define EVENT( event, function )	{ &( event ), ( void ( idClass::* )( void ) )( &function ) },
#define END_CLASS					{ NULL, NULL } };


class idEventArg {
public:
	int			type;
	UINT_PTR	value;

	idEventArg()								{ type = D_EVENT_INTEGER; value = 0; };
	idEventArg( int data )						{ type = D_EVENT_INTEGER; value = data; };
	idEventArg( bool data )						{ type = D_EVENT_BOOLEAN; value = data; };
	idEventArg( float data )					{ type = D_EVENT_FLOAT; value = *reinterpret_cast<UINT_PTR *>( &data ); };
	idEventArg( idVec3 &data )					{ type = D_EVENT_VECTOR; value = reinterpret_cast<UINT_PTR>( &data ); };
	idEventArg( const idStr &data )				{ type = D_EVENT_STRING; value = reinterpret_cast<UINT_PTR>( data.c_str() ); };
	idEventArg( const char *data )				{ type = D_EVENT_STRING; value = reinterpret_cast<UINT_PTR>( data ); };
	idEventArg( const idWStr &data )			{ type = D_EVENT_WSTRING; value = reinterpret_cast<UINT_PTR>( data.c_str() ); };
	idEventArg( const wchar_t *data )			{ type = D_EVENT_WSTRING; value = reinterpret_cast<UINT_PTR>( data ); };
	idEventArg( const class idEntity *data )	{ type = D_EVENT_ENTITY; value = reinterpret_cast<UINT_PTR>( data ); };
	
	void SetHandle( int data )					{ type = D_EVENT_HANDLE; value = data; };
};

class idAllocError : public idException {
public:
	idAllocError( const char *text = "" ) : idException( text ) {}
};

/***********************************************************************

  idClass

***********************************************************************/

/*
================
CLASS_PROTOTYPE

This macro must be included in the definition of any subclass of idClass.
It prototypes variables used in class instanciation and type checking.
Use this on single inheritance concrete classes only.
================
*/
#define CLASS_PROTOTYPE( nameofclass )									\
public:																	\
	static	idTypeInfo						Type;						\
	static	idClass							*CreateInstance( void );	\
	virtual	idTypeInfo						*GetType( void ) const;		\
	static	idEventFunc<nameofclass>		eventCallbacks[]

/*
================
CLASS_DECLARATION

This macro must be included in the code to properly initialize variables
used in type checking and run-time instanciation.  It also defines the list
of events that the class responds to.  Take special care to ensure that the 
proper superclass is indicated or the run-time type information will be
incorrect.  Use this on concrete classes only.
================
*/

#define CLASS_DECLARATION( nameofsuperclass, nameofclass )											\
	idTypeInfo nameofclass::Type( #nameofclass, #nameofsuperclass,									\
		( idEventFunc<idClass> * )nameofclass::eventCallbacks,										\
		nameofclass::CreateInstance,																\
		( void ( idClass::* )( void ) )&nameofclass::Spawn,											\
		nameofclass::InhibitSpawn );																\
	idClass *nameofclass::CreateInstance( void ) {													\
		try {																						\
			nameofclass *ptr = new nameofclass;														\
			ptr->FindUninitializedMemory();															\
			return ptr;																				\
		}																							\
		catch( idAllocError & ) {																	\
			return NULL;																			\
		}																							\
	}																								\
	idTypeInfo *nameofclass::GetType( void ) const {												\
		return &( nameofclass::Type );																\
	}																								\
idEventFunc<nameofclass> nameofclass::eventCallbacks[] = {

/*
================
ABSTRACT_PROTOTYPE

This macro must be included in the definition of any abstract subclass of idClass.
It prototypes variables used in class instanciation and type checking.
Use this on single inheritance abstract classes only.
================
*/
#define ABSTRACT_PROTOTYPE( nameofclass )								\
public:																	\
	static	idTypeInfo						Type;						\
	static	idClass							*CreateInstance( void );	\
	virtual	idTypeInfo						*GetType( void ) const;		\
	static	idEventFunc<nameofclass>		eventCallbacks[]

/*
================
ABSTRACT_DECLARATION

This macro must be included in the code to properly initialize variables
used in type checking.  It also defines the list of events that the class
responds to.  Take special care to ensure that the proper superclass is
indicated or the run-time tyep information will be incorrect.  Use this
on abstract classes only.
================
*/
#define ABSTRACT_DECLARATION( nameofsuperclass, nameofclass )										\
	idTypeInfo nameofclass::Type( #nameofclass, #nameofsuperclass,									\
		( idEventFunc<idClass> * )nameofclass::eventCallbacks,										\
		nameofclass::CreateInstance,																\
		( void ( idClass::* )( void ) )&nameofclass::Spawn,											\
		nameofclass::InhibitSpawn	);																\
	idClass *nameofclass::CreateInstance( void ) {													\
		common->Error( "Cannot instanciate abstract class %s.", #nameofclass );						\
		return NULL;																				\
	}																								\
	idTypeInfo *nameofclass::GetType( void ) const {												\
		return &( nameofclass::Type );																\
	}																								\
	idEventFunc<nameofclass> nameofclass::eventCallbacks[] = {

typedef void ( idClass::*classSpawnFunc_t )( void );

class idClass {
public:
	ABSTRACT_PROTOTYPE( idClass );

#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	void *						operator new( size_t );
	void *						operator new( size_t s, int, int, char *, int );
	void						operator delete( void * );
	void						operator delete( void *, int, int, char *, int );
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

	virtual						~idClass();

	void						Spawn( void );
	void						CallSpawn( void );
	bool						IsType( const idTypeInfo &c ) const;
	const char *				GetClassname( void ) const;
	const char *				GetSuperclass( void ) const;
	void						FindUninitializedMemory( void );

	template< typename T >
	T*							Cast( void ) { return this ? ( IsType( T::Type ) ? static_cast< T* >( this ) : NULL ) : NULL; }
	
	template< typename T >
	const T*					Cast( void ) const { return this ? ( IsType( T::Type ) ? static_cast< const T* >( this ) : NULL ) : NULL; }

	bool						RespondsTo( const idEventDef &ev ) const;

	bool						PostGUIEventMS( const idEventDef *ev, int time );

	bool						PostEventMS( const idEventDef *ev, int time );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 );
	bool						PostEventMS( const idEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 );

	bool						PostEventSec( const idEventDef *ev, float time );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 );
	bool						PostEventSec( const idEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 );

	bool						ProcessEvent( const idEventDef *ev );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 );
	bool						ProcessEvent( const idEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 );

	bool						ProcessEventArgPtr( const idEventDef *ev, const UINT_PTR *data );
	void						CancelEvents( const idEventDef *ev );

	void						Event_Remove( void );
	void						Event_SafeRemove( void );

	virtual void				OnEventRemove( void ) { ; }

	bool						IsSpawning( void ) { return spawningObjects.FindIndex( this ) != -1; }

	virtual idLinkList< idClass >*	GetInstanceNode( void ) { return NULL; }
	static bool					InhibitSpawn( const idDict& args ) { return false; }

	virtual void				BecomeActive( int flags, bool force = false ) { assert( false ); }
	virtual void				BecomeInactive( int flags, bool force = false ) { assert( false ); }
	virtual idScriptObject*		GetScriptObject( void ) const { return NULL; }
	virtual void				SetSkin( const idDeclSkin* skin ) { ; }
	virtual const char*			GetName( void ) const { return NULL; }
	virtual idAnimator*			GetAnimator( void ) { return NULL; }

	// Static functions
	static void					InitClasses( void );
	static void					ShutdownClasses( void );
	static idTypeInfo *			GetClass( const char *name );
	static void					DisplayInfo_f( const idCmdArgs &args );
	static void					ListClasses_f( const idCmdArgs &args );
	static idClass *			CreateInstance( const char *name );
	static int					GetNumTypes( void ) { return types.Num(); }
	static int					GetTypeNumBits( void ) { return typeNumBits; }
	static idTypeInfo *			GetType( int num );
	static void					ArgCompletion_ClassName( const idCmdArgs &args, void(*callback)( const char *s ) );

	static void					WikiClassTree_f( const idCmdArgs &args );
	static void					WikiClassPage_f( const idCmdArgs &args );

	idLinkList< idEvent >		scheduledEvents;

private:
	classSpawnFunc_t			CallSpawnFunc( idTypeInfo *cls );

	bool						PostEventArgs( const idEventDef *ev, int time, int numargs, bool guiEvent, ... );
	bool						ProcessEventArgs( const idEventDef *ev, int numargs, ... );

	void						Event_IsClass( int typeNumber );

	static bool					initialized;
	static idList<idTypeInfo *>	types;
	static idList<idTypeInfo *>	typenums;
	static int					typeNumBits;
	static size_t				memused;
	static int					numobjects;
	static idList< idClass* >	spawningObjects;
};

/***********************************************************************

  idTypeInfo

***********************************************************************/

typedef void ( *mediaCacheFunc_t )( const idDict& );
typedef void ( *classCallback_t )( idClass* );

template< typename T >
class sdInstanceCollector {
public:
										sdInstanceCollector( bool includeChildTypes ) { T::Type.GetInstances( *this, includeChildTypes ); }
	void								operator()( idClass* cls ) { *instances.Alloc() = reinterpret_cast< T* >( cls ); }

	int									Num( void ) const { return instances.Num(); }
	T*									operator[]( int index ) { return instances[ index ]; }

	idStaticList< T*, MAX_GENTITIES >&	GetList( void ) { return instances; }

private:
	idStaticList< T*, MAX_GENTITIES >	instances;
};

class idTypeInfo { // Gordon: public field maek meh angreh!
public:
	const char *				classname;
	const char *				superclass;
	void						( idClass::*Spawn )( void );

	idEventFunc<idClass> *		eventCallbacks;
	eventCallback_t *			eventMap;
	idTypeInfo *				super;
	idTypeInfo *				next;
	bool						freeEventMap;
	int							typeNum;
	int							lastChild;
	mutable idLinkList< idClass >		instances;

	idHierarchy<idTypeInfo>		node;

								idTypeInfo( const char *classname, 
											const char *superclass, 
											idEventFunc<idClass> *eventCallbacks, 
											idClass* ( *CreateInstance )( void ), 
											void ( idClass::*Spawn )( void ),
											bool ( *inhibitSpawn )( const idDict& dict )
											);

								~idTypeInfo();

	void						Init( void );
	void						Shutdown( void );

	idClass*					CreateInstance( void ) const;
	bool						InhibitSpawn( const idDict& args ) const;

	bool						IsType( const idTypeInfo &superclass ) const;
	bool						RespondsTo( const idEventDef &ev ) const;

	template< typename T >
	void GetInstances( T& callback, bool includeChildren ) const {
		idClass* cls = instances.Next();
		for ( ; cls; cls = cls->GetInstanceNode()->Next() ) {
			callback( cls );
		}

		if ( includeChildren ) {
			idTypeInfo* type = node.GetChild();
			for ( ; type; type = type->node.GetSibling() ) {
				type->GetInstances( callback, true );
			}
		}
	}

private:
	idClass*					( *_createInstance )( void );
	bool						( *_inhibitSpawn )( const idDict& args );
};

/*
================
idTypeInfo::IsType

Checks if the object's class is a subclass of the class defined by the 
passed in idTypeInfo.
================
*/
ID_INLINE bool idTypeInfo::IsType( const idTypeInfo &type ) const {
	return ( ( typeNum >= type.typeNum ) && ( typeNum <= type.lastChild ) );
}

/*
================
idTypeInfo::RespondsTo
================
*/
ID_INLINE bool idTypeInfo::RespondsTo( const idEventDef &ev ) const {
	assert( idEvent::initialized );
	return eventMap[ ev.GetEventNum() ] != NULL;
}

/*
================
idClass::IsType

Checks if the object's class is a subclass of the class defined by the 
passed in idTypeInfo.
================
*/
ID_INLINE bool idClass::IsType( const idTypeInfo &superclass ) const {
	idTypeInfo *subclass;

	subclass = GetType();
	return subclass->IsType( superclass );
}

/*
================
idClass::RespondsTo
================
*/
ID_INLINE bool idClass::RespondsTo( const idEventDef &ev ) const {
	const idTypeInfo *c;

	assert( idEvent::initialized );
	c = GetType();
	return c->RespondsTo( ev );
}

#endif /* !__SYS_CLASS_H__ */
