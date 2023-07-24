// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_INTERPRETER_H__
#define __SCRIPT_INTERPRETER_H__

#include "Script_Program.h"

#define MAX_STACK_DEPTH 	32
#define LOCALSTACK_SIZE 	6144

class idThread;
class idVarDef;

typedef struct prstack_s {
	int 				s;
	const function_t*	f;
	int 				stackbase;
} prstack_t;

class idInterpreter {
private:
	prstack_t			callStack[ MAX_STACK_DEPTH ];
	int 				callStackDepth;

	byte*				localstack;
	int					localstackSize;
	int 				localstackUsed;
	int 				localstackBase;

	const function_t*	currentFunction;
	int 				instructionPointer;

	int					popParms;

#ifdef SCRIPT_EVENT_RETURN_CHECKS
	int					expectedReturnType;
#endif // SCRIPT_EVENT_RETURN_CHECKS

	idThread*			thread;
	idProgram*			program;

	void				AssureStack( int size );

	void				PopParms( int numParms );
	void				PushString( const char *string );
	void				PushWString( const wchar_t *string );
	void				Push( int value );
	const char			*FloatToString( float value );
	const wchar_t		*FloatToWString( float value );
	void				AppendString( idVarDef *def, const char *from );
	void				SetString( idVarDef *def, const char *from );
	const char			*GetString( idVarDef *def );
	void				SetWString( idVarDef *def, const wchar_t *from );
	const wchar_t		*GetWString( idVarDef *def );
	varEval_t			GetVariable( idVarDef *def );
	idScriptObject		*GetScriptObject( int entnum ) const;
	void				NextInstruction( int position );

	void				LeaveFunction( idVarDef *returnDef );
	void				CallSysEvent( const function_t *func, int argsize );

#ifdef SCRIPT_EVENT_RETURN_CHECKS
	void				SetExpectedReturn( int type ) { expectedReturnType = type; }
#endif // SCRIPT_EVENT_RETURN_CHECKS

public:
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	int					GetExpectedReturn( void ) { return expectedReturnType; }
#endif // SCRIPT_EVENT_RETURN_CHECKS

	void				CallEvent( const function_t *func, int argsize );
	int					GetStackSize( void ) const { return localstackSize; }

	bool				doneProcessing;
	bool				threadDying;
	bool				terminateOnExit;

						idInterpreter();
						~idInterpreter( void );

	void				Init( idProgram* _program ) { program = _program; }

	void				SetThread( idThread *pThread );

	void				StackTrace( void ) const;

	int					CurrentLine( void ) const;
	const char			*CurrentFile( void ) const;

	void				Error( const char *fmt, ... ) const;
	void				Warning( const char *fmt, ... ) const;
	void				DisplayInfo( void ) const;

	void				ThreadCall( idInterpreter *source, const function_t *func, int args );
	void				EnterFunction( const function_t *func, bool clearStack );
	void				EnterObjectFunction( idScriptObject* object, const function_t *func, bool clearStack );

	bool				Execute( void );
	void				Reset( void );

	int					GetCallstackDepth( void ) const;
	const prstack_t		*GetCallstack( void ) const;
	const function_t	*GetCurrentFunction( void ) const;
	idThread			*GetThread( void ) const;

	void				PushParm( int value );
	void				PushParm( const char* string );

	static int			s_stackHigh;
};

/*
====================
idInterpreter::PopParms
====================
*/
ID_INLINE void idInterpreter::PopParms( int numParms ) {
	// pop our parms off the stack
	if ( localstackUsed < numParms ) {
		Error( "locals stack underflow" );
	}
	localstackUsed -= numParms;
}

/*
====================
idInterpreter::AssureStack
====================
*/
ID_INLINE void idInterpreter::AssureStack( int size ) {
	const int STACK_SIZE_GROW = 1024;
	if ( size <= localstackSize ) {
		return;
	}
	int newStackSize = localstackSize + STACK_SIZE_GROW;
	assert( size <= newStackSize );
	byte* newStack = program->AllocStack( newStackSize );
	if ( localstack != NULL ) {
		memcpy( newStack, localstack, localstackSize );
	}
	memset( &newStack[ localstackSize ], 0, STACK_SIZE_GROW );
	program->FreeStack( localstack, localstackSize );
	localstack = newStack;
	localstackSize = newStackSize;
}

/*
====================
idInterpreter::Push
====================
*/
ID_INLINE void idInterpreter::Push( int value ) {
	AssureStack( localstackUsed + sizeof( int ) );

	*( int * )&localstack[ localstackUsed ]	= value;
	localstackUsed += sizeof( int );
	if ( localstackUsed > s_stackHigh ) {
		s_stackHigh = localstackUsed;
	}
}

/*
====================
idInterpreter::PushParm
====================
*/
ID_INLINE void idInterpreter::PushParm( int value ) {
	Push( value );
}

/*
====================
idInterpreter::PushParm
====================
*/
ID_INLINE void idInterpreter::PushParm( const char* string ) {
	PushString( string );
}

/*
====================
idInterpreter::PushString
====================
*/
ID_INLINE void idInterpreter::PushString( const char *string ) {
	AssureStack( localstackUsed + MAX_STRING_LEN );

	idStr::Copynz( ( char * )&localstack[ localstackUsed ], string, MAX_STRING_LEN );
	localstackUsed += MAX_STRING_LEN;
	if ( localstackUsed > s_stackHigh ) {
		s_stackHigh = localstackUsed;
	}
}

/*
====================
idInterpreter::PushWString
====================
*/
ID_INLINE void idInterpreter::PushWString( const wchar_t *string ) {
	AssureStack( localstackUsed + ( MAX_STRING_LEN * sizeof( wchar_t ) ) );

	idWStr::Copynz( ( wchar_t * )&localstack[ localstackUsed ], string, MAX_STRING_LEN );
	localstackUsed += MAX_STRING_LEN * sizeof( wchar_t );
	if ( localstackUsed > s_stackHigh ) {
		s_stackHigh = localstackUsed;
	}
}

/*
====================
idInterpreter::FloatToString
====================
*/
ID_INLINE const char *idInterpreter::FloatToString( float value ) {
	static char text[ 32 ];

	if ( value == ( float )( int )value ) {
		idStr::snPrintf( text, sizeof( text ), "%d", ( int )value );
	} else {
		idStr::snPrintf( text, sizeof( text ), "%f", value );
	}
	return text;
}

/*
====================
idInterpreter::FloatToWString
====================
*/
ID_INLINE const wchar_t *idInterpreter::FloatToWString( float value ) {
	static wchar_t text[ 32 ];

	if ( value == ( float )( int )value ) {
		idWStr::snPrintf( text, sizeof( text ) / sizeof( wchar_t ), L"%d", ( int )value );
	} else {
		idWStr::snPrintf( text, sizeof( text ) / sizeof( wchar_t ), L"%f", value );
	}
	return text;
}

/*
====================
idInterpreter::AppendString
====================
*/
ID_INLINE void idInterpreter::AppendString( idVarDef *def, const char *from ) {
	if ( def->settings.initialized == idVarDef::stackVariable ) {
		idStr::Append( ( char * )&localstack[ localstackBase + def->value.stackOffset ], MAX_STRING_LEN, from );
	} else {
		idStr::Append( def->value.stringPtr, MAX_STRING_LEN, from );
	}
}

/*
====================
idInterpreter::SetString
====================
*/
ID_INLINE void idInterpreter::SetString( idVarDef *def, const char *from ) {
	if ( def->settings.initialized == idVarDef::stackVariable ) {
		idStr::Copynz( ( char * )&localstack[ localstackBase + def->value.stackOffset ], from, MAX_STRING_LEN );
	} else {
		idStr::Copynz( def->value.stringPtr, from, MAX_STRING_LEN );
	}
}

/*
====================
idInterpreter::GetString
====================
*/
ID_INLINE const char *idInterpreter::GetString( idVarDef *def ) {
	if ( def->settings.initialized == idVarDef::stackVariable ) {
		return ( char * )&localstack[ localstackBase + def->value.stackOffset ];
	} else {
		return def->value.stringPtr;
	}
}

/*
====================
idInterpreter::SetWString
====================
*/
ID_INLINE void idInterpreter::SetWString( idVarDef *def, const wchar_t *from ) {
	if ( def->settings.initialized == idVarDef::stackVariable ) {
		idWStr::Copynz( ( wchar_t * )&localstack[ localstackBase + def->value.stackOffset ], from, MAX_STRING_LEN );
	} else {
		idWStr::Copynz( def->value.wstringPtr, from, MAX_STRING_LEN );
	}
}

/*
====================
idInterpreter::GetWString
====================
*/
ID_INLINE const wchar_t *idInterpreter::GetWString( idVarDef *def ) {
	if ( def->settings.initialized == idVarDef::stackVariable ) {
		return ( wchar_t * )&localstack[ localstackBase + def->value.stackOffset ];
	} else {
		return def->value.wstringPtr;
	}
}

/*
====================
idInterpreter::GetVariable
====================
*/
ID_INLINE varEval_t idInterpreter::GetVariable( idVarDef *def ) {
	if ( def->settings.initialized == idVarDef::stackVariable ) {
		varEval_t val;
		val.intPtr = ( int * )&localstack[ localstackBase + def->value.stackOffset ];
		return val;
	} else {
		return def->value;
	}
}

/*
====================
idInterpreter::NextInstruction
====================
*/
ID_INLINE void idInterpreter::NextInstruction( int position ) {
	// Before we execute an instruction, we increment instructionPointer,
	// therefore we need to compensate for that here.
	instructionPointer = position - 1;
}

#endif /* !__SCRIPT_INTERPRETER_H__ */
