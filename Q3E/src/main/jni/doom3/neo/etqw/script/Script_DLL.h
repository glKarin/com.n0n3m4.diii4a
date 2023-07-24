// Copyright (C) 2007 Id Software, Inc.
//
#ifndef __SCRIPT_DLL_H__
#define __SCRIPT_DLL_H__

#ifdef __linux__
#define USE_UCONTEXT
#endif

#include <setjmp.h>
#ifdef USE_UCONTEXT
#include <ucontext.h>
#include <errno.h>
#endif
#include "compiledscript/CompiledScriptInterface.h"
#include "Script_ScriptObject.h"
#include "Script_SysCall.h"

class sdDLLProgram;
class sdDLLThread;
class sdScriptInterface;

void Coroutine_Detach( sdDLLThread *self );
void Coroutine_Enter( sdDLLThread *self );

class sdDLLScriptInterface : public sdCompilerInterface {
public:
										sdDLLScriptInterface( void ) { program = NULL; }
	void								Init( sdDLLProgram* _program ) { program = _program; }

	virtual const idEventDef*			FindEvent( const char* name );
	virtual int							AllocThread( sdProcedureCall* call );
	virtual int							AllocThread( sdCompiledScript_ClassBase* object, const char* name, sdProcedureCall* call );
	virtual int							AllocGuiThread( sdProcedureCall* call );
	virtual int							AllocGuiThread( sdCompiledScript_ClassBase* object, const char* name, sdProcedureCall* call );
	virtual sdCompiledScript_ClassBase*	AllocObject( const char* name );
	virtual void						FreeObject( sdCompiledScript_ClassBase* obj );
	virtual sdCompiledScript_ClassBase*	GetObject( int handle );

	virtual idScriptObject*				GetScriptObject( int handle );
	virtual idEntity*					GetEntity( int handle );

	virtual void						Wait( float time );
	virtual void						WaitFrame( void );

	virtual float						GetReturnedFloat( void );
	virtual bool						GetReturnedBoolean( void );
	virtual sdCompiledScript_ClassBase*	GetReturnedObject( void );
	virtual const char*					GetReturnedString( void );
	virtual float*						GetReturnedVector( void );
	virtual int							GetReturnedInteger( void );
	virtual const wchar_t*				GetReturnedWString( void );

	virtual void						ReturnString( const char* value );
	virtual void						ReturnWString( const wchar_t* value );
	virtual void						ReturnVector( float* value );
	virtual void						ReturnFloat( float value );
	virtual void						ReturnBoolean( bool value );
	virtual void						ReturnObject( sdCompiledScript_ClassBase* obj );

	virtual void						SysCall( const idEventDef* event, const UINT_PTR* data );
	virtual void						EventCall( const idEventDef* event, sdCompiledScript_ClassBase* obj, const UINT_PTR* data );

private:
	void								DoSysCall( const idEventDef* event, const UINT_PTR* data );
	void								DoEventCall( const idEventDef* event, sdCompiledScript_ClassBase* obj, const UINT_PTR* data );
	sdCompiledScript_ClassBase*			DoAllocObject( const char* name );
	void								DoFreeObject( sdCompiledScript_ClassBase* instance );

private:
	sdDLLProgram*						program;
};

class sdDLLFunction : public sdProgram::sdFunction {
public:
								sdDLLFunction( sdFunctionInfo* fInfo );
	virtual const char*			GetName( void ) const { return name.c_str(); }

	int							GetNumParameters( void ) const { return numParms; }
	int							GetParmSizeTotal( void ) const { return parmSizeTotal; }
	functionWrapper_t			GetWrapper( void ) const { return wrapper; }
	functionCallback_t			GetFunction( void ) const { return function; }

private:
	idStr						name;
	functionWrapper_t			wrapper;
	functionCallback_t			function;
	int							numParms;
	int							parmSizeTotal;
};

class sdDLLClassFunction : public sdProgram::sdFunction {
public:
								sdDLLClassFunction( sdClassFunctionInfo* fInfo );
	virtual const char*			GetName( void ) const { return name.c_str(); }

	int							GetNumParameters( void ) const { return numParms; }
	int							GetParmSizeTotal( void ) const { return parmSizeTotal; }
	classFunctionWrapper_t		GetWrapper( void ) const { return wrapper; }
	classFunctionCallback_t		GetFunction( void ) const { return function; }

private:
	idStr						name;
	classFunctionWrapper_t		wrapper;
	classFunctionCallback_t		function;
	int							numParms;
	int							parmSizeTotal;
};

class sdDLLClassVariable {
public:
								sdDLLClassVariable( sdClassVariableInfo* vInfo );

	const char*					GetName( void ) const { return name.c_str(); }
	variableLookup_t			GetLookup( void ) const { return lookup; }
	etype_t						GetType( void ) const { return type; }

private:
	idStr						name;
	etype_t						type;
	variableLookup_t			lookup;
};

class sdDLLProgram : public sdProgram {
public:
	static const int MAX_THREADS = 2048;

	class sdDLLClassInfo : public sdProgram::sdTypeInfo {
	public:
		virtual const char*						GetName( void ) const { return name.c_str(); }
		virtual const sdProgram::sdTypeInfo*	GetSuperClass( void ) const { return superClass; }

		idStr							name;
		sdDLLClassInfo*					superClass;
		idList< sdDLLClassFunction* >	functions;
		idHashIndex						functionHash;

		idList< sdDLLClassVariable* >	variables;
		idHashIndex						variablesHash;

		classAllocator_t				allocator;
	};

								sdDLLProgram( void );
	virtual						~sdDLLProgram( void ) { Shutdown(); }

	virtual bool				Init( void );

	virtual bool				OnError( const char* text );

	virtual void				ReturnStringInternal( const char* value );
	virtual void				ReturnWStringInternal( const wchar_t* value );
	virtual void				ReturnFloatInternal( float value );
	virtual void				ReturnVectorInternal( const idVec3& value );
	virtual void				ReturnEntityInternal( idEntity* value );
	virtual void				ReturnIntegerInternal( int value );
	virtual void				ReturnObjectInternal( idScriptObject* value );

	virtual const sdTypeInfo*	GetDefaultType( void ) const { return defaultType; }
	
	virtual int					GetReturnedInteger( void ) { return returnValue.intValue; }
	virtual float				GetReturnedFloat( void ) { return returnValue.floatValue; }
	virtual bool				GetReturnedBoolean( void ) { return returnValue.intValue != 0 ? true : false; }
	virtual const char*			GetReturnedString( void ) { return stringValue.c_str(); }
	virtual const wchar_t*		GetReturnedWString( void ) { return wstringValue.c_str(); }
	virtual idScriptObject*		GetReturnedObject( void ) { return GetScriptObject( returnValue.objectValue ); }
	virtual const idVec3*		GetReturnedVector( void ) { return &vectorValue; }

	virtual void				Restart( void );
	virtual void				Disassemble( void ) const;
	virtual void				Shutdown( void );

	virtual sdProgramThread*	CreateThread( void );
	virtual sdProgramThread*	CreateThread( const sdScriptHelper& h );
	virtual void				FreeThread( sdProgramThread* thread );

	virtual const sdFunction*	FindFunction( const char* name );
	virtual const sdFunction*	FindFunction( const char* name, const sdTypeObject* object );

	virtual sdTypeObject*		AllocType( sdTypeObject* oldType, const sdTypeInfo* type );
	virtual sdTypeObject*		AllocType( sdTypeObject* oldType, const char* typeName );
	virtual void				FreeType( sdTypeObject* oldType );

	virtual const sdProgram::sdTypeInfo*	FindTypeInfo( const char* name );
	virtual int								GetNumClasses( void ) const;
	virtual const sdProgram::sdTypeInfo*	GetClass( int index ) const;

	virtual bool				IsValid( void ) const { return dllHandle != 0; }

	virtual void				KillThread( int number );
	virtual void				KillThread( const char* name );

	virtual sdProgramThread*	GetCurrentThread( void );

	virtual void				ListThreads( void ) const;
	virtual void				PruneThreads( void );

	char*						AllocStack( size_t size, size_t& usedSize );
	void						FreeStack( char* stack, size_t size );

	void						OnThreadShutdown( sdDLLThread* thread );
	
private:
	void						CloseDLL( void );

private:
	idList< int >				freeThreadNums;

	idList< sdDLLClassInfo* >	classInfo;
	idHashIndex					classInfoHash;

	idList< sdDLLFunction* >	functionInfo;
	idHashIndex					functionInfoHash;

	void*						dllHandle;
	sdDLLScriptInterface		dllInterface;
	sdScriptInterface*			scriptInterface;

	idList< idList< char* > >	freeStacks;

	idBlockAlloc< sdDLLThread, 32 > threadAllocator;

	const sdTypeInfo*			defaultType;

	union returnValue_t {
		float					floatValue;
		int						intValue;
		int						objectValue;
	};

	idStr						stringValue;
	idWStr						wstringValue;
	idVec3						vectorValue;
	returnValue_t				returnValue;
};

class sdDLLTypeObject : public sdProgram::sdTypeObject {
public:
										sdDLLTypeObject( const sdDLLProgram::sdDLLClassInfo* _info );
	virtual								~sdDLLTypeObject( void ) { delete instance; }

	virtual void						Clear( void ) { ; }
	virtual const char*					GetName( void ) const { return info->name.c_str(); }
	virtual etype_t						GetVariable( const char *name, byte** data ) const;
	virtual bool						ValidateCall( const sdProgram::sdFunction* func ) const { return false; }
	virtual void						SetHandle( int handle );

	const sdDLLProgram::sdDLLClassInfo*	GetInfo( void ) const { return info; }
	sdCompiledScript_ClassBase*			GetInstance( void ) const { return instance; }

private:
	const sdDLLProgram::sdDLLClassInfo*	info;
	sdCompiledScript_ClassBase*			instance;
};

class sdDLLThread : public sdSysCallThread {
public:
	CLASS_PROTOTYPE( sdDLLThread );

	friend void Coroutine_Detach( sdDLLThread *self );
	friend void Coroutine_Enter( sdDLLThread *self );

	const static int MAX_RESUMING_COROUTINES = 32;

							sdDLLThread( void );
							~sdDLLThread( void );

	void					Init( void );
	void					Clear( void );
	
	void					Create( sdDLLProgram* _program, int _threadNum );

	virtual void			CallFunction( const sdProgram::sdFunction* function );
	virtual void			CallFunction( idScriptObject* obj, const sdProgram::sdFunction* function );
	virtual void			DelayedStart( int delay );
	virtual bool			Execute( void );
	virtual void			ManualDelete( void );
	virtual void			ManualControl( void );
	virtual void			EndThread( void );
	virtual void			DoneProcessing( void );
	virtual void			SetName( const char* name );
	virtual void			Error( const char* fmt, ... ) const;
	virtual void			Warning( const char* fmt, ... ) const;
	virtual void			EnableDebugInfo( void );
	virtual void			DisableDebugInfo( void );
	virtual bool			IsDying( void ) const { return Finished(); }
	virtual bool			IsWaiting( void ) const;

	static bool				OnError( const char* text );

	bool					Finished( void ) const { return flags.threadDying; }

	int						GetThreadTime( void ) const {
		if ( !flags.guiThread ) {
			return gameLocal.time;
		}
		return gameLocal.ToGuiTime( gameLocal.time );
	}

	virtual void			Wait( float time );
	virtual void			WaitFrame( void ) { flags.waitFrame = true; Coroutine_Detach( this ); }
	virtual void			Assert( void ) { assert( false ); }

	static void				ListThreads( void );
	static void				PruneThreads( void );

	int						GetThreadNum( void ) const { return threadNum; }
	void					Call( sdProcedureCall* call, bool guiThread );

	virtual const char*		CurrentFile( void ) const { return "not available"; }
	virtual int				CurrentLine( void ) const { return -1; }
	virtual void			StackTrace( void ) const;

	// Gordon: FIXME: This is a bit silly
	class sdDLLThreadCall : public sdProcedureCall {
	public:
		void Init( const sdProgram::sdFunction* _function, const byte* _data ) {
			function	= _function;
			data		= _data;
		}

		virtual void Go( void ) {
			reinterpret_cast< const sdDLLFunction* >( function )->GetWrapper()( 
				reinterpret_cast< const sdDLLFunction* >( function )->GetFunction(),
				data );
		}

		const sdProgram::sdFunction*	function;
		const byte*						data;
	} call2;

	class sdDLLClassThreadCall : public sdProcedureCall {
	public:
		void Init( idScriptObject* _obj, const sdProgram::sdFunction* _function, const byte* _data, sdDLLProgram* _program ) {
			objHandle	= _obj->GetHandle();
			function	= _function;
			data		= _data;
			program		= _program;
		}

		virtual void Go( void ) {
			idScriptObject* obj = program->GetScriptObject( objHandle );
			if ( obj == NULL ) {
				return;
			}

			reinterpret_cast< const sdDLLClassFunction* >( function )->GetWrapper()( 
				reinterpret_cast< sdDLLTypeObject* >( obj->GetObject() )->GetInstance(),
				reinterpret_cast< const sdDLLClassFunction* >( function )->GetFunction(),
				data );
		}

		int								objHandle;
		const sdProgram::sdFunction*	function;
		const byte*						data;
		sdDLLProgram*					program;
	} call1;

	virtual void Routine( void ) {
		assert( call != NULL );
		call->Go();
		flags.threadDying = true;
	}

	void AllocStack( char* stack ) {
		FreeStack();
		storedStackSize = storedStackPointer - stack;		
		localStack = program->AllocStack( storedStackSize, actualStackSize );
		memcpy( localStack, stack, storedStackSize );
	}

	void FreeStack( void ) {
		program->FreeStack( localStack, actualStackSize );
		storedStackSize = 0;
		actualStackSize = 0;
		localStack = NULL;
	}

	void					SetCall( sdProcedureCall* _call ) { flags.threadDying = false; call = _call; flags.reset = true; if ( GetActiveThread() == this ) { Coroutine_Detach( this ); } }
	
	static void				KillThread( int number );
	static void				KillThread( const char* name );
	static sdDLLThread*		GetActiveThread( void ) { return s_current; }

	int						GetPauseEndTime( void ) const { return pauseTime; }

	static void				Call( sdDLLThread* next );

	bool					IsStackSaved( void ) const { return localStack != NULL; }

	void Reset( void ) {
		storedStackPointer = NULL;
	}

protected:
	sdDLLProgram*			program;
	int						threadNum;

	struct flags_t {
		bool				manualDelete;
		bool				manualControl;
		bool				waitFrame;
		bool				threadDying;
		bool				doneProcessing;
		bool				reset;
		bool				guiThread;
	};

	flags_t					flags;

	sdProcedureCall*		call;
	int						pauseTime;
	idStr					name;

	idLinkList< sdDLLThread > threadNode;

protected:
	char*					localStack;
#ifdef USE_UCONTEXT
	ucontext_t				context;
#else
    jmp_buf					environment;
#endif

    sdDLLThread*			caller;
	sdDLLThread*			callee;
	char*					storedStackPointer;
	size_t					storedStackSize;
	size_t					actualStackSize;

	static idLinkList< sdDLLThread > s_threads;
	static sdDLLThread* s_current;
	static sdDLLThread* s_main;
	static char*		s_errorThrown;
};

#endif // __SCRIPT_DLL_H__
