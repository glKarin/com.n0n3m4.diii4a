// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_SCRIPT_SCRIPTHELPER_H__
#define __GAME_SCRIPT_SCRIPTHELPER_H__

class idEntity;

class sdScriptHelper {
public:
	typedef struct parms_s {
		const char*		string;
		int				integer;
	} parms_t;

	typedef idStaticList< parms_t, 12 > parmsList_t;

public:
									sdScriptHelper( void );
									~sdScriptHelper( void );

	bool							Init( idScriptObject* obj, const char* functionName );
	bool							Init( idScriptObject* obj, const sdProgram::sdFunction*  _function );
	bool							Done( void ) const;

	void							Push( int value );
	void							Push( float value );
	void							Push( idScriptObject* obj );
	void							Push( sdTeamInfo* team );
	void							Push( const idVec3& vec );
	void							Push( const idAngles& angles );
	void							Push( const char* value );

	void							Run( void );

	float							GetReturnedFloat( void );
	int								GetReturnedInteger( void );
	const char*						GetReturnedString( void );

	void							Clear( bool free = true );

	idScriptObject*					GetObject( void ) const { return object; }
	const parmsList_t				GetArgs( void ) const { return args; }
	const sdProgram::sdFunction*	GetFunction( void ) const { return function; }
	int								GetSize( void ) const { return size; }

private:
	sdProgramThread*				thread;
	parmsList_t						args;
	idScriptObject*					object;
	const sdProgram::sdFunction*	function;
	int								size;
};

class sdCVarWrapper : public idClass {
public:
	CLASS_PROTOTYPE( sdCVarWrapper );

	class sdCVarWrapperCallback : public idCVarCallback {
	public:
		void						Init( sdCVarWrapper* wrapper ) { owner = wrapper; }

		virtual void				OnChanged( void ) { owner->OnChanged(); }

	private:
		sdCVarWrapper*				owner;
	};

									sdCVarWrapper( void );
									~sdCVarWrapper( void );

	void							Init( const char* cvarName, const char* defaultValue );

	virtual idScriptObject*			GetScriptObject( void ) const { return _scriptObject; }

	void							Event_GetFloatValue( void );
	void							Event_GetStringValue( void );
	void							Event_GetIntValue( void );
	void							Event_GetBoolValue( void );
	void							Event_GetVectorValue( void );

	void							Event_SetFloatValue( float value );
	void							Event_SetStringValue( const char* value );
	void							Event_SetIntValue( int value );
	void							Event_SetBoolValue( bool value );
	void							Event_SetCallback( const char* callback );
	void							Event_SetVectorValue( const idVec3& value );

	void							OnChanged( void );

private:
	idCVar*							_cvar;
	idScriptObject*					_scriptObject;
	sdCVarWrapperCallback			_callback;
	const sdProgram::sdFunction*	_callbackFunction;
};

#endif // __GAME_SCRIPT_SCRIPTHELPER_H__

