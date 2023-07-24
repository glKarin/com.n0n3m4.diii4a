// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_INTERFACE_H__
#define __SCRIPT_INTERFACE_H__

// #define SCRIPT_EVENT_RETURN_CHECKS

int			Script_GetSpawnId( idEntity* entity );
idEntity*	Script_EntityForSpawnId( int spawnId );

typedef enum {
	ev_error = -1, ev_void, ev_scriptevent, ev_namespace, ev_string, ev_wstring, ev_float, ev_vector, ev_field, ev_function, ev_virtualfunction, ev_pointer, ev_object, ev_jumpoffset, ev_argsize, ev_boolean, ev_internalscriptevent
} etype_t;

class idClass;
class idScriptObject;
class sdProgramThread;
class sdScriptHelper;

typedef idScriptObject* idScriptObjectPtr_t;

const int	SCRIPT_OBJECTNUM_BITS			= 11;
const int	MAX_SCRIPT_OBJECTS				= ( 1 << SCRIPT_OBJECTNUM_BITS );

class sdProgram {
public:
	class sdFunction {
	public:
		virtual const char*		GetName( void ) const = 0;
	};

	class sdTypeObject {
	public:
		virtual					~sdTypeObject( void ) { ; }
		virtual void			Clear( void ) = 0;
		virtual const char*		GetName( void ) const = 0;
		virtual etype_t 		GetVariable( const char *name, byte** data ) const = 0;
		virtual bool			ValidateCall( const sdFunction* func ) const = 0;
		virtual void			SetHandle( int handle ) = 0;
	};

	class sdTypeInfo {
	public:
		virtual const char*			GetName( void ) const = 0;
		virtual const sdTypeInfo*	GetSuperClass( void ) const = 0;
	};

								sdProgram( void );
	virtual						~sdProgram( void ) { ; }

	virtual void				ReturnStringInternal( const char* value ) = 0;
	virtual void				ReturnWStringInternal( const wchar_t* value ) = 0;
	virtual void				ReturnFloatInternal( float value ) = 0;
	virtual void				ReturnVectorInternal( const idVec3& value ) = 0;
	virtual void				ReturnEntityInternal( idEntity* value ) = 0;
	virtual void				ReturnIntegerInternal( int value ) = 0;
	virtual void				ReturnObjectInternal( idScriptObject* value ) = 0;

	static void					ReturnString( const char* value );
	static void					ReturnWString( const wchar_t* value );
	static void					ReturnFloat( float value );
	static void					ReturnVector( const idVec3& value );
	static void					ReturnEntity( idEntity* value );
	static void					ReturnInteger( int value );
	static void					ReturnBoolean( bool value );
	static void					ReturnObject( idScriptObject* value );
	static void					ReturnHandle( int value );

	virtual bool				Init( void ) = 0;
	virtual void				Restart( void ) = 0;
	virtual void				Disassemble( void ) const = 0;
	virtual void				Shutdown( void ) = 0;

	virtual bool				OnError( const char* text ) = 0;

	virtual const sdTypeInfo*	GetDefaultType( void ) const = 0;

	virtual sdProgramThread*	CreateThread( void ) = 0;
	virtual sdProgramThread*	CreateThread( const sdScriptHelper& h ) = 0;
	virtual void				FreeThread( sdProgramThread* thread ) = 0;

	virtual int					GetReturnedInteger( void ) = 0;
	virtual float				GetReturnedFloat( void ) = 0;
	virtual bool				GetReturnedBoolean( void ) = 0;
	virtual const char*			GetReturnedString( void ) = 0;
	virtual const wchar_t*		GetReturnedWString( void ) = 0;
	virtual idScriptObject*		GetReturnedObject( void ) = 0;

	virtual const sdFunction*	FindFunction( const char* name ) = 0;
	virtual const sdFunction*	FindFunction( const char* name, const sdTypeObject* object ) = 0;

	virtual const sdTypeInfo*	FindTypeInfo( const char* name ) = 0;

	virtual int					GetNumClasses( void ) const = 0;
	virtual const sdTypeInfo*	GetClass( int index ) const = 0;

	virtual sdTypeObject*		AllocType( sdTypeObject* oldType, const sdTypeInfo* type ) = 0;
	virtual sdTypeObject*		AllocType( sdTypeObject* oldType, const char* typeName ) = 0;
	virtual void				FreeType( sdTypeObject* oldType ) = 0;

	virtual void				KillThread( int number ) = 0;
	virtual void				KillThread( const char* name ) = 0;

	virtual sdProgramThread*	GetCurrentThread( void ) = 0;

	virtual bool				IsValid( void ) const = 0;

	virtual void				ListThreads( void ) const = 0;
	virtual void				PruneThreads( void ) = 0;

	// Common functionality
	idScriptObject*				AllocScriptObject( idClass* cls, const sdTypeInfo* objectType );
	idScriptObject*				AllocScriptObject( idClass* cls, const char* objectType );
	void						FreeScriptObject( idScriptObjectPtr_t& object );
	void						FreeScriptObjects( void );
	void						ListScriptObjects( void );
	idScriptObject*				GetScriptObject( int handle );

	static void					ListThreads_f( const idCmdArgs& args );

private:
	idStaticList< idScriptObject*, MAX_SCRIPT_OBJECTS >	allocedObjects;
	idList< int >										freeObjectIndicies;
	int													nextScriptObjectIndex;
};

class sdProgramThread : public idClass {
public:
	ABSTRACT_PROTOTYPE( sdProgramThread );

							sdProgramThread( void ) { autoNode.SetOwner( this ); }

	virtual void			CallFunction( const sdProgram::sdFunction* function ) = 0;
	virtual void			CallFunction( idScriptObject* obj, const sdProgram::sdFunction* function ) = 0;
	virtual void			DelayedStart( int delay ) = 0;
	virtual bool			Execute( void ) = 0;
	virtual void			ManualDelete( void ) = 0;
	virtual void			ManualControl( void ) = 0;
	virtual void			EndThread( void ) = 0;
	virtual void			DoneProcessing( void ) = 0;
	virtual void			SetName( const char* name ) = 0;
	virtual void			Warning( const char* fmt, ... ) const = 0;
	virtual bool			IsWaiting( void ) const = 0;
	virtual bool			IsDying( void ) const = 0;
	virtual void			Wait( float time ) = 0;
	virtual void			WaitFrame( void ) = 0;
	virtual void			Assert( void ) = 0;

	virtual const char*		CurrentFile( void ) const = 0;
	virtual int				CurrentLine( void ) const = 0;
	virtual void			StackTrace( void ) const = 0;

	idLinkList< sdProgramThread >&	GetAutoNode( void ) { return autoNode; }
private:
	idLinkList< sdProgramThread > autoNode;
};



/***********************************************************************

idScriptVariable

Helper template that handles looking up script variables stored in objects.
If the specified variable doesn't exist, or is the wrong data type, idScriptVariable
will cause an error.

***********************************************************************/

template<class type, etype_t etype, class returnType>
class idScriptVariable {
private:
	type				*data;

public:
						idScriptVariable();
	bool				IsLinked( void ) const;
	void				Unlink( void );
	void				LinkTo( idScriptObject &obj, const char *name, bool allowFailure = false );
	idScriptVariable	&operator=( const returnType &value );
						operator returnType() const;

	returnType			operator->( void ) const { return (returnType)(*this); }
};

template<class type, etype_t etype, class returnType>
ID_INLINE idScriptVariable<type, etype, returnType>::idScriptVariable() {
	data = NULL;
}

template<class type, etype_t etype, class returnType>
ID_INLINE bool idScriptVariable<type, etype, returnType>::IsLinked( void ) const {
	return ( data != NULL );
}

template<class type, etype_t etype, class returnType>
ID_INLINE void idScriptVariable<type, etype, returnType>::Unlink( void ) {
	data = NULL;
}

template<class type, etype_t etype, class returnType>
ID_INLINE idScriptVariable<type, etype, returnType> &idScriptVariable<type, etype, returnType>::operator=( const returnType &value ) {
	// check if we attempt to access the object before it's been linked

	// make sure we don't crash if we don't have a pointer
	if ( data ) {
		*data = ( type )value;
	}
	return *this;
}

template<class type, etype_t etype, class returnType>
ID_INLINE idScriptVariable<type, etype, returnType>::operator returnType() const {
	// check if we attempt to access the object before it's been linked

	// make sure we don't crash if we don't have a pointer
	if ( data ) {
		return ( const returnType )*data;
	}
	// reasonably safe value
	return ( const returnType )0;
}

/***********************************************************************

Script object variable access template instantiations

These objects will automatically handle looking up of the current value
of a variable in a script object.  They can be stored as part of a class
for up-to-date values of the variable, or can be used in functions to
sample the data for non-dynamic values.

***********************************************************************/

typedef idScriptVariable<int, ev_boolean, int>					idScriptBool;
typedef idScriptVariable<float, ev_float, float>				idScriptFloat;
typedef idScriptVariable<float, ev_float, int>					idScriptInt;
typedef idScriptVariable<idVec3, ev_vector, idVec3>				idScriptVector;
typedef idScriptVariable<idStr, ev_string, const char *>		idScriptString;
typedef idScriptVariable<idWStr, ev_wstring, const wchar_t *>	idScriptWString;
typedef idScriptVariable<int, ev_object, idEntity* >			idScriptEntity;

typedef idEntity* idEntityPtr_t;

template<>
ID_INLINE idScriptVariable< int, ev_object, idEntity* >& idScriptVariable<int, ev_object, idEntity*>::operator=( const idEntityPtr_t& value ) {
	// check if we attempt to access the object before it's been linked
	assert( data );

	// make sure we don't crash if we don't have a pointer
	if ( data ) {
		*data = Script_GetSpawnId( value );
	}
	return *this;
}

template<>
ID_INLINE idScriptVariable< int, ev_object, idEntity *>::operator idEntity*() const {
	// check if we attempt to access the object before it's been linked
//	assert( data );

	// make sure we don't crash if we don't have a pointer
	if ( data ) {
		return Script_EntityForSpawnId( *data );
	}
	return NULL;
}


const int				null_int	= 0;
const float				null_float	= 0.f;

class sdScriptObjectNetworkData : public sdEntityStateNetworkData {
public:
									sdScriptObjectNetworkData( void ) : index( 0 ) { ; }

	virtual void					MakeDefault( void );

	virtual void					Write( idFile* file ) const;
	virtual void					Read( idFile* file );

	void							Reset( void ) const { index = 0; }

	bool							IsValid( void ) const { return index < data.Num(); }

	int&							GetInt( void ) { int* temp = reinterpret_cast< int* >( &data[ index ] ); index += sizeof( int ); return *temp; }
	float&							GetFloat( void ) { float* temp = reinterpret_cast< float* >( &data[ index ] ); index += sizeof( float ); return *temp; }
	idVec3&							GetVector( void ) { idVec3* temp = reinterpret_cast< idVec3* >( &data[ index ] ); index += sizeof( idVec3 ); return *temp; }

	const int&						GetInt( void ) const { if ( !IsValid() ) { return null_int; } const int* temp = reinterpret_cast< const int* >( &data[ index ] ); index += sizeof( int ); return *temp; }
	const float&					GetFloat( void ) const { if ( !IsValid() ) { return null_float; } const float* temp = reinterpret_cast< const float* >( &data[ index ] ); index += sizeof( float ); return *temp; }
	const idVec3&					GetVector( void ) const { if ( !IsValid() ) { return vec3_zero; } const idVec3* temp = reinterpret_cast< const idVec3* >( &data[ index ] ); index += sizeof( idVec3 ); return *temp; }

	idList< byte >					data;
	mutable int						index;
};



#endif // __SCRIPT_INTERFACE_H__
