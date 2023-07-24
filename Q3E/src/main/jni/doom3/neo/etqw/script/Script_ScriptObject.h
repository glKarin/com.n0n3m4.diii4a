// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_SCRIPTOBJECT_H__
#define __SCRIPT_SCRIPTOBJECT_H__

/***********************************************************************

idScriptObject

In-game representation of objects in scripts.  Use the idScriptVariable template
(below) to access variables.

***********************************************************************/

struct networkFieldSync_t {
	byte*									data;
	etype_t									type;
	const sdProgram::sdFunction*			callback;
	bool									broadcast;
};

struct networkFieldModeSync_t {
	idStaticList< networkFieldSync_t, 32 >	fields;
	int										size;
};

class idScriptObject {
private:
	sdProgram::sdTypeObject*		typeObject;
	sdProgram*						program;
	int								_handle;
	idClass*						_object;

	idLinkList< sdProgramThread >	autoThreads; // threads created in the script system, we will terminate them on destruction
	
public:
									idScriptObject( int handle, idClass* object, sdProgram* program );
									~idScriptObject();

	networkFieldModeSync_t			networkFields[ NSM_NUM_MODES ];

	idClass*						GetClass( void ) const { return _object; }
	int								GetHandle( void ) const { return this ? _handle : 0; }

	void							SetSynced( const char *name, bool snap );
	void							SetSyncCallback( const char *name, const char* functionName );

	void							Free( void );

	bool							SetType( const sdProgram::sdTypeInfo* newtype );
	bool							SetType( const char *typeName );

	sdProgram::sdTypeObject*		GetObject( void ) { return typeObject; }

	void							ShutdownAutoThreads( void );
	idLinkList< sdProgramThread >&	GetAutoThreads( void ) { return autoThreads; }

	void							ClearObject( void );

	bool							HasObject( void ) const { return typeObject != NULL; }

	const char*						GetTypeName( void ) const;
	const sdProgram::sdFunction*	GetSyncFunc( void ) const;
	const sdProgram::sdFunction*	GetConstructor( void ) const;
	const sdProgram::sdFunction*	GetPreConstructor( void ) const;
	const sdProgram::sdFunction*	GetDestructor( void ) const;
	const sdProgram::sdFunction*	GetFunction( const char *name ) const;

	void							CallEvent( const char* name );

	void							ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	void							ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	void							WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	bool							CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	void							SetupStateData( networkStateMode_t mode, sdScriptObjectNetworkData& state ) const;

	byte*							GetVariable( const char *name, etype_t etype ) const;

	void							CallNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper );
};

// TTimo: initially in Script_Interface.h, moved here because idScriptObject has to be defined
template<class type, etype_t etype, class returnType>
ID_INLINE void idScriptVariable<type, etype, returnType>::LinkTo( idScriptObject &obj, const char *name, bool allowFailure ) {
	data = ( type * )obj.GetVariable( name, etype );
	if ( !data && !allowFailure ) {
		gameError( "Missing '%s' field in script object '%s'", name, obj.GetTypeName() );
	}
}

#endif // __SCRIPT_SCRIPTOBJECT_H__
