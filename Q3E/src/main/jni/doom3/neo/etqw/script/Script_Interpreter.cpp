// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
#define NODEFAULT	default: assert( 0 )
#elif _WIN32
#define NODEFAULT	default: __assume( 0 )
#else
#define NODEFAULT
#endif

#include "Script_Interpreter.h"
#include "Script_Program.h"
#include "Script_Thread.h"
#include "Script_Compiler.h"
#include "Script_Helper.h"
#include "Script_ScriptObject.h"
#include "../Entity.h"

int idInterpreter::s_stackHigh = 0;

/*
================
idInterpreter::idInterpreter
================
*/
idInterpreter::idInterpreter() {
	localstack = NULL;
	localstackSize = 0;
	localstackUsed = 0;
	terminateOnExit = true;
	memset( callStack, 0, sizeof( callStack ) );
	Reset();
}

/*
================
idInterpreter::~idInterpreter
================
*/
idInterpreter::~idInterpreter( void ) {
	Reset();
}

/*
================
idInterpreter::Reset
================
*/
void idInterpreter::Reset( void ) {
	callStackDepth = 0;
	localstackUsed = 0;
	localstackBase = 0;
	program->FreeStack( localstack, localstackSize );
	localstack = NULL;
	localstackSize = 0;

	popParms = 0;

	currentFunction = NULL;
	NextInstruction( 0 );

	threadDying 	= false;
	doneProcessing	= true;
}

/*
================
idInterpreter::GetCallstackDepth
================
*/
int idInterpreter::GetCallstackDepth( void ) const {
	return callStackDepth;
}

/*
================
idInterpreter::GetCallstack
================
*/
const prstack_t *idInterpreter::GetCallstack( void ) const {
	return &callStack[ 0 ];
}

/*
================
idInterpreter::GetCurrentFunction
================
*/
const function_t *idInterpreter::GetCurrentFunction( void ) const {
	return currentFunction;
}

/*
================
idInterpreter::GetThread
================
*/
idThread *idInterpreter::GetThread( void ) const {
	return thread;
}


/*
================
idInterpreter::SetThread
================
*/
void idInterpreter::SetThread( idThread *pThread ) {
	thread = pThread;
}

/*
================
idInterpreter::CurrentLine
================
*/
int idInterpreter::CurrentLine( void ) const {
	if ( instructionPointer < 0 ) {
		return 0;
	}
	return program->GetLineNumberForStatement( instructionPointer );
}

/*
================
idInterpreter::CurrentFile
================
*/
const char *idInterpreter::CurrentFile( void ) const {
	if ( instructionPointer < 0 ) {
		return "";
	}
	return program->GetFilenameForStatement( instructionPointer );
}

/*
============
idInterpreter::StackTrace
============
*/
void idInterpreter::StackTrace( void ) const {
	const function_t	*f;
	int 				i;
	int					top;

	if ( callStackDepth == 0 ) {
		gameLocal.Printf( "<NO STACK>\n" );
		return;
	}

	top = callStackDepth;
	if ( top >= MAX_STACK_DEPTH ) {
		top = MAX_STACK_DEPTH - 1;
	}
	
	if ( !currentFunction ) {
		gameLocal.Printf( "<NO FUNCTION>\n" );
	} else {
		gameLocal.Printf( "%12s : %s\n", program->GetFilename( currentFunction->filenum ), currentFunction->Name() );
	}

	for( i = top; i >= 0; i-- ) {
		f = callStack[ i ].f;
		if ( !f ) {
			gameLocal.Printf( "<NO FUNCTION>\n" );
		} else {
			gameLocal.Printf( "%12s : %s\n", program->GetFilename( f->filenum ), f->Name() );
		}
	}
}

/*
============
idInterpreter::Error

Aborts the currently executing function
============
*/
void idInterpreter::Error( const char *fmt, ... ) const {
	va_list argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	StackTrace();

	if ( ( instructionPointer >= 0 ) && ( instructionPointer < program->NumStatements() ) ) {
		statement_t &line = program->GetStatement( instructionPointer );
		common->Error( "%s(%d): Thread '%s': %s", program->GetFilename( line.file ), line.linenumber, thread->GetThreadName(), text );
	} else {
		common->Error( "Thread '%s': %s", thread->GetThreadName(), text );
	}
}

/*
============
idInterpreter::Warning

Prints file and line number information with warning.
============
*/
void idInterpreter::Warning( const char *fmt, ... ) const {
	va_list argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	if ( ( instructionPointer >= 0 ) && ( instructionPointer < program->NumStatements() ) ) {
		statement_t &line = program->GetStatement( instructionPointer );
		common->Warning( "%s(%d): Thread '%s': %s", program->GetFilename( line.file ), line.linenumber, thread->GetThreadName(), text );
	} else {
		common->Warning( "Thread '%s' : %s", thread->GetThreadName(), text );
	}
}

/*
================
idInterpreter::DisplayInfo
================
*/
void idInterpreter::DisplayInfo( void ) const {
	const function_t *f;
	int i;

	gameLocal.Printf( " Stack depth: %d bytes\n", localstackUsed );
	gameLocal.Printf( "  Call depth: %d\n", callStackDepth );
	gameLocal.Printf( "  Call Stack: " );

	if ( callStackDepth == 0 ) {
		gameLocal.Printf( "<NO STACK>\n" );
	} else {
		if ( !currentFunction ) {
			gameLocal.Printf( "<NO FUNCTION>\n" );
		} else {
			gameLocal.Printf( "%12s : %s\n", program->GetFilename( currentFunction->filenum ), currentFunction->Name() );
		}

		for( i = callStackDepth; i > 0; i-- ) {
			gameLocal.Printf( "              " );
			f = callStack[ i ].f;
			if ( !f ) {
				gameLocal.Printf( "<NO FUNCTION>\n" );
			} else {
				gameLocal.Printf( "%12s : %s\n", program->GetFilename( f->filenum ), f->Name() );
			}
		}
	}
}

/*
====================
idInterpreter::ThreadCall

Copies the args from the calling thread's stack
====================
*/
void idInterpreter::ThreadCall( idInterpreter *source, const function_t *func, int args ) {
	Reset();
	AssureStack( args );

	memcpy( localstack, &source->localstack[ source->localstackUsed - args ], args );

	localstackUsed = args;
	localstackBase = 0;

	EnterFunction( func, false );

	thread->SetName( currentFunction->Name() );
}

/*
================
idInterpreter::EnterObjectFunction

Calls a function on a script object.

NOTE: If this is called from within a event called by this interpreter, the function arguments will be invalid after calling this function.
================
*/
void idInterpreter::EnterObjectFunction( idScriptObject* object, const function_t *func, bool clearStack ) {
	if ( clearStack ) {
		Reset();
	}
	if ( popParms ) {
		PopParms( popParms );
		popParms = 0;
	}
	Push( object->GetHandle() );
	EnterFunction( func, false );
}

/*
====================
idInterpreter::EnterFunction

Returns the new program statement counter

NOTE: If this is called from within a event called by this interpreter, the function arguments will be invalid after calling this function.
====================
*/
void idInterpreter::EnterFunction( const function_t *func, bool clearStack ) {
	int 		c;
	prstack_t	*stack;

	if ( clearStack ) {
		Reset();
	}
	if ( popParms ) {
		PopParms( popParms );
		popParms = 0;
	}

	if ( callStackDepth >= MAX_STACK_DEPTH ) {
		Error( "call stack overflow" );
	}

	stack = &callStack[ callStackDepth ];

	stack->s			= instructionPointer + 1;	// point to the next instruction to execute
	stack->f			= currentFunction;
	stack->stackbase	= localstackBase;

	callStackDepth++;

	if ( !func ) {
		Error( "NULL function" );
	}

	currentFunction = func;
	assert( !func->eventdef );
	NextInstruction( func->firstStatement );

	// allocate space on the stack for locals
	// parms are already on stack
	c = func->locals - func->parmTotal;
	assert( c >= 0 );
	AssureStack( localstackUsed + c );

	// initialize local stack variables to zero
	memset( &localstack[ localstackUsed ], 0, c );

	localstackUsed += c;
	localstackBase = localstackUsed - func->locals;
}

/*
====================
idInterpreter::LeaveFunction
====================
*/
void idInterpreter::LeaveFunction( idVarDef *returnDef ) {
	prstack_t *stack;
	varEval_t ret;
	
	if ( callStackDepth <= 0 ) {
		Error( "prog stack underflow" );
	}

	// return value
	if ( returnDef ) {
		switch( returnDef->Type() ) {
		case ev_string:
#ifdef SCRIPT_EVENT_RETURN_CHECKS
			SetExpectedReturn( D_EVENT_STRING );
#endif // SCRIPT_EVENT_RETURN_CHECKS
			program->ReturnStringInternal( GetString( returnDef ) );
			break;

		case ev_wstring:
#ifdef SCRIPT_EVENT_RETURN_CHECKS
			SetExpectedReturn( D_EVENT_WSTRING );
#endif // SCRIPT_EVENT_RETURN_CHECKS
			program->ReturnWStringInternal( GetWString( returnDef ) );
			break;

		case ev_vector:
			ret = GetVariable( returnDef );
#ifdef SCRIPT_EVENT_RETURN_CHECKS
			SetExpectedReturn( D_EVENT_VECTOR );
#endif // SCRIPT_EVENT_RETURN_CHECKS
			program->ReturnVectorInternal( *ret.vectorPtr );
			break;

		default:
			ret = GetVariable( returnDef );
#ifdef SCRIPT_EVENT_RETURN_CHECKS
			SetExpectedReturn( D_EVENT_HANDLE );
#endif // SCRIPT_EVENT_RETURN_CHECKS
			program->ReturnIntegerInternal( *ret.intPtr );
			break;
		}
	}

	// remove locals from the stack
	PopParms( currentFunction->locals );
	assert( localstackUsed == localstackBase );

	// up stack
	callStackDepth--;
	stack = &callStack[ callStackDepth ]; 
	currentFunction = stack->f;
	localstackBase = stack->stackbase;
	NextInstruction( stack->s );

	if ( !callStackDepth ) {
		// all done
		doneProcessing = true;
		threadDying = true;
		currentFunction = NULL;
	}
}

/*
================
idInterpreter::CallEvent
================
*/
void idInterpreter::CallEvent( const function_t *func, int argsize ) {
	int 				i;
	int					j;
	varEval_t			var;
	int 				pos;
	int 				start;
	UINT_PTR			data[ D_EVENT_MAXARGS ];
	const idEventDef	*evdef;
	const char			*format;

	if ( !func ) {
		Error( "NULL function" );
	}

	assert( func->eventdef );
	evdef = func->eventdef;

	start = localstackUsed - argsize;
	var.intPtr = ( int * )&localstack[ start ];
	idScriptObject* object = program->GetScriptObject( *var.objectId );
	idClass* cls = object ? object->GetClass() : NULL;

#ifdef SCRIPT_EVENT_RETURN_CHECKS
	SetExpectedReturn( evdef->GetReturnType() );
#endif // SCRIPT_EVENT_RETURN_CHECKS

	if ( !cls || !cls->RespondsTo( *evdef ) ) {
		if ( cls != NULL ) {
			Warning( "Function '%s' not supported on entity '%s'", evdef->GetName(), cls->GetName() );
		} else {
			Warning( "Event call on a NULL object" );
		}

		// always return a safe value when an object doesn't exist
		switch( evdef->GetReturnType() ) {
		case D_EVENT_BOOLEAN:
		case D_EVENT_HANDLE:
			program->ReturnIntegerInternal( 0 );
			break;

		case D_EVENT_INTEGER:
		case D_EVENT_FLOAT:
			program->ReturnFloatInternal( 0 );
			break;

		case D_EVENT_VECTOR:
			program->ReturnVectorInternal( vec3_zero );
			break;

		case D_EVENT_STRING:
			program->ReturnStringInternal( "" );
			break;

		case D_EVENT_WSTRING:
			program->ReturnWStringInternal( L"" );
			break;

		case D_EVENT_ENTITY:
		case D_EVENT_ENTITY_NULL:
			program->ReturnEntityInternal( NULL );
			break;

		case D_EVENT_OBJECT:
			program->ReturnObjectInternal( NULL );
			break;
		}

		PopParms( argsize );
		return;
	}

	format = evdef->GetArgFormat();
	for( j = 0, i = 0, pos = type_object.Size(); pos < argsize && i < evdef->GetNumArgs(); i++ ) {
		switch( format[ i ] ) {
			case D_EVENT_INTEGER :
				var.intPtr = ( int * )&localstack[ start + pos ];
				data[ i ] = 0;
				data[ i ] = int( *var.floatPtr );
				break;

			case D_EVENT_BOOLEAN :
			case D_EVENT_HANDLE :
				var.intPtr = ( int * )&localstack[ start + pos ];
				data[ i ] = 0;
				data[ i ] = *var.intPtr;
				break;

			case D_EVENT_FLOAT :
				var.intPtr = ( int * )&localstack[ start + pos ];
				data[ i ] = 0;
				( *( float * )&data[ i ] ) = *var.floatPtr;
				break;

			case D_EVENT_VECTOR :
				var.intPtr = ( int * )&localstack[ start + pos ];
				( *( idVec3 ** )&data[ i ] ) = var.vectorPtr;
				break;

			case D_EVENT_STRING :
				( *( const char ** )&data[ i ] ) = ( char * )&localstack[ start + pos ];
				break;

			case D_EVENT_WSTRING :
				( *( const wchar_t ** )&data[ i ] ) = ( wchar_t * )&localstack[ start + pos ];
				break;

			case D_EVENT_ENTITY: {
				var.objectId = ( int * )&localstack[ start + pos ];
				idScriptObject* object = program->GetScriptObject( *var.objectId );
				( *( idEntity ** )&data[ i ] ) = object ? object->GetClass()->Cast< idEntity >() : NULL;
				if ( !( *( idEntity ** )&data[ i ] ) ) {
					Warning( "$null_entity passed to event '%s'!", evdef->GetName() );
				}
				break;
			}

			case D_EVENT_ENTITY_NULL: {
				var.objectId = ( int * )&localstack[ start + pos ];
				idScriptObject* object = program->GetScriptObject( *var.objectId );
				( *( idEntity ** )&data[ i ] ) = object ? object->GetClass()->Cast< idEntity >() : NULL;
				break;
			}

			case D_EVENT_OBJECT: {
				var.objectId = ( int * )&localstack[ start + pos ];
				( *( idScriptObject** )&data[ i ] ) = program->GetScriptObject( *var.objectId );
				break;
			}

			default : 
				Error( "Invalid arg format string for '%s' event.", evdef->GetName() );
				break;
		}

		pos += func->parmSize[ j++ ];
	}

	popParms = argsize;
	cls->ProcessEventArgPtr( evdef, data );

	if ( popParms ) {
		PopParms( popParms );
	}

	popParms = 0;
}

/*
================
idInterpreter::CallSysEvent
================
*/
void idInterpreter::CallSysEvent( const function_t *func, int argsize ) {
	int 				i;
	int					j;
	varEval_t			source;
	int 				pos;
	int 				start;
	UINT_PTR			data[ D_EVENT_MAXARGS ];
	const idEventDef	*evdef;
	const char			*format;
	bool				skipEvent = false;

	if ( !func ) {
		Error( "NULL function" );
	}

	assert( func->eventdef );
	evdef = func->eventdef;

	start = localstackUsed - argsize;

	format = evdef->GetArgFormat();
	for( j = 0, i = 0, pos = 0; pos < argsize && i < evdef->GetNumArgs(); i++ ) {
		switch( format[ i ] ) {
		case D_EVENT_INTEGER :
			source.intPtr = ( int * )&localstack[ start + pos ];
			*( int * )&data[ i ] = int( *source.floatPtr );
			break;

		case D_EVENT_BOOLEAN :
		case D_EVENT_HANDLE :
			source.intPtr = ( int * )&localstack[ start + pos ];
			*( int * )&data[ i ] = *source.intPtr;
			break;

		case D_EVENT_FLOAT :
			source.intPtr = ( int * )&localstack[ start + pos ];
			*( float * )&data[ i ] = *source.floatPtr;
			break;

		case D_EVENT_VECTOR :
			source.intPtr = ( int * )&localstack[ start + pos ];
			*( idVec3 ** )&data[ i ] = source.vectorPtr;
			break;

		case D_EVENT_STRING :
			*( const char ** )&data[ i ] = ( char * )&localstack[ start + pos ];
			break;

		case D_EVENT_WSTRING :
			*( const wchar_t ** )&data[ i ] = ( wchar_t * )&localstack[ start + pos ];
			break;

		case D_EVENT_ENTITY: {
			source.objectId = ( int * )&localstack[ start + pos ];

			idScriptObject* object = program->GetScriptObject( *source.objectId );
			( *( idEntity ** )&data[ i ] ) = object ? object->GetClass()->Cast< idEntity >() : NULL;
			if ( !*( idEntity ** )&data[ i ] ) {
				Warning( "$null_entity passed to event '%s'!", evdef->GetName() );
				skipEvent = true;
			}
			break;
		}

		case D_EVENT_ENTITY_NULL: {
			source.intPtr = ( int * )&localstack[ start + pos ];
			idScriptObject* object = program->GetScriptObject( *source.objectId );
			( *( idEntity ** )&data[ i ] ) = object ? object->GetClass()->Cast< idEntity >() : NULL;
			break;
		}

		case D_EVENT_OBJECT: {
			source.intPtr = ( int * )&localstack[ start + pos ];
			( *( idScriptObject ** )&data[ i ] ) = program->GetScriptObject( *source.objectId );
			break;
		}

		default :
			Error( "Invalid arg format string for '%s' event.", evdef->GetName() );
			break;
		}

		pos += func->parmSize[ j++ ];
	}

#ifdef SCRIPT_EVENT_RETURN_CHECKS
	SetExpectedReturn( evdef->GetReturnType() );
#endif // SCRIPT_EVENT_RETURN_CHECKS

	popParms = argsize;
	if ( !skipEvent ) {
		thread->ProcessEventArgPtr( evdef, data );
	}
	if ( popParms ) {
		PopParms( popParms );
	}
	popParms = 0;
}

/*
====================
idInterpreter::Execute
====================
*/
bool idInterpreter::Execute( void ) {
	varEval_t	var_a;
	varEval_t	var_b;
	varEval_t	var_c;
	varEval_t	var;
	statement_t	*st;
	int 		runaway;
	idThread	*newThread;
	float		floatVal;
	idScriptObject* obj;
	const function_t *func;

	if ( threadDying || !currentFunction ) {
		return true;
	}

	runaway = 5000000;

	doneProcessing = false;
	while( !doneProcessing && !threadDying ) {
		instructionPointer++;

		if ( !--runaway ) {
			Error( "runaway loop error" );
		}

		// next statement
		st = &program->GetStatement( instructionPointer );
#ifdef DEBUG_SCRIPTS
		st->executionCount++;
#endif // DEBUG_SCRIPTS

		switch( st->op ) {
		case OP_RETURN:
			LeaveFunction( st->a );
			break;

		case OP_THREAD:
		case OP_GUITHREAD:
			newThread = idThread::AllocThread();
			newThread->Init( this, st->a->value.functionPtr, st->b->value.argSize, st->op == OP_GUITHREAD );
			newThread->DelayedStart( 0 );

			// return the thread number to the script
#ifdef SCRIPT_EVENT_RETURN_CHECKS
			SetExpectedReturn( D_EVENT_FLOAT );
#endif // SCRIPT_EVENT_RETURN_CHECKS
			program->ReturnFloatInternal( static_cast< float >( newThread->GetThreadNum() ) );
			PopParms( st->b->value.argSize );
			break;

		case OP_OBJTHREAD:
		case OP_GUIOBJTHREAD:
#ifdef SCRIPT_EVENT_RETURN_CHECKS
			SetExpectedReturn( D_EVENT_FLOAT );
#endif // SCRIPT_EVENT_RETURN_CHECKS

			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj != NULL ) {
				const idTypeDef* t = reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetType();

				func = t->GetFunction( st->b->value.virtualFunction );
				assert( st->c->value.functionPtr->parmTotal == func->parmTotal );
				newThread = idThread::AllocThread();
				newThread->Init( this, func, func->parmTotal, st->op == OP_GUIOBJTHREAD );
				newThread->GetAutoNode().AddToEnd( obj->GetAutoThreads() );
				
				idEntity* ent = obj->GetClass()->Cast< idEntity >();
				if ( ent != NULL ) {
					newThread->SetName( va( "%s_%s", func->type->Name(), ent->name.c_str() ) );
				}

				newThread->DelayedStart( 0 );

				// return the thread number to the script
				program->ReturnFloatInternal( static_cast< float >( newThread->GetThreadNum() ) );
			} else {
				// return a null thread to the script
				program->ReturnFloatInternal( 0.0f );
			}
			PopParms( st->c->value.functionPtr->parmTotal );
			break;

		case OP_CALL:
			EnterFunction( st->a->value.functionPtr, false );
			break;

		case OP_EVENTCALL:
			CallEvent( st->a->value.functionPtr, st->b->value.argSize );
			break;

		case OP_VIRTUALEVENTCALL: {
			int size = st->c->value.functionPtr->parmTotal + type_object.Size();

			var.bytePtr = &localstack[ localstackUsed - size ];
			obj = GetScriptObject( *var.objectId );

			func = NULL;
			if ( obj ) {
				const idTypeDef* t = reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetType();
				
				func = t->GetVirtualFunction( st->a->value.virtualFunction );
			} else {
				Warning( "Virtual function call on a NULL entity" );
			}

			if ( func ) {
				EnterFunction( func, false );
			} else {
				// return a 'safe' value
#ifdef SCRIPT_EVENT_RETURN_CHECKS
				SetExpectedReturn( D_EVENT_VECTOR );
#endif // SCRIPT_EVENT_RETURN_CHECKS
				program->ReturnVectorInternal( vec3_zero );

#ifdef SCRIPT_EVENT_RETURN_CHECKS
				SetExpectedReturn( D_EVENT_STRING );
#endif // SCRIPT_EVENT_RETURN_CHECKS
				program->ReturnStringInternal( "" );

				PopParms( size );
			}
			break;
		}

		case OP_ALLOC_TYPE: {
			idTypeDef* t = st->a->TypeDef();
			var_c = GetVariable( st->c );
			idScriptObject* object = gameLocal.program->AllocScriptObject( NULL, t->Name() );
			sdScriptHelper h1;
			object->CallNonBlockingScriptEvent( object->GetPreConstructor(), h1 );
			*var_c.objectId = object->GetHandle();
			break;
		}

		case OP_FREE_TYPE: {
			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				sdScriptHelper h1;
				obj->CallNonBlockingScriptEvent( obj->GetDestructor(), h1 );
				program->FreeScriptObject( obj );
			}
			*var_a.objectId = 0;
			break;
		}

		case OP_OBJECTCALL:	
			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				const idTypeDef* t = reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetType();
				
				func = t->GetFunction( st->b->value.virtualFunction );
				EnterFunction( func, false );
			} else {
				Warning( "Function Call on a NULL Object" );

				// return a 'safe' value
#ifdef SCRIPT_EVENT_RETURN_CHECKS
				SetExpectedReturn( D_EVENT_VECTOR );
#endif // SCRIPT_EVENT_RETURN_CHECKS
				program->ReturnVectorInternal( vec3_zero );

#ifdef SCRIPT_EVENT_RETURN_CHECKS
				SetExpectedReturn( D_EVENT_STRING );
#endif // SCRIPT_EVENT_RETURN_CHECKS
				program->ReturnStringInternal( "" );
				PopParms( st->c->value.functionPtr->parmTotal );
			}
			break;

		case OP_SYSCALL:
			CallSysEvent( st->a->value.functionPtr, st->b->value.argSize );
			break;

		case OP_IFNOT:
			var_a = GetVariable( st->a );
			if ( *var_a.intPtr == 0 ) {
				NextInstruction( instructionPointer + st->b->value.jumpOffset );
			}
			break;

		case OP_IF:
			var_a = GetVariable( st->a );
			if ( *var_a.intPtr != 0 ) {
				NextInstruction( instructionPointer + st->b->value.jumpOffset );
			}
			break;

		case OP_GOTO:
			NextInstruction( instructionPointer + st->a->value.jumpOffset );
			break;

		case OP_ADD_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = *var_a.floatPtr + *var_b.floatPtr;
			break;

		case OP_ADD_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.vectorPtr = *var_a.vectorPtr + *var_b.vectorPtr;
			break;

		case OP_ADD_S:
			SetString( st->c, GetString( st->a ) );
			AppendString( st->c, GetString( st->b ) );
			break;

		case OP_ADD_FS:
			var_a = GetVariable( st->a );
			SetString( st->c, FloatToString( *var_a.floatPtr ) );
			AppendString( st->c, GetString( st->b ) );
			break;

		case OP_ADD_SF:
			var_b = GetVariable( st->b );
			SetString( st->c, GetString( st->a ) );
			AppendString( st->c, FloatToString( *var_b.floatPtr ) );
			break;

		case OP_ADD_VS:
			var_a = GetVariable( st->a );
			SetString( st->c, var_a.vectorPtr->ToString() );
			AppendString( st->c, GetString( st->b ) );
			break;

		case OP_ADD_SV:
			var_b = GetVariable( st->b );
			SetString( st->c, GetString( st->a ) );
			AppendString( st->c, var_b.vectorPtr->ToString() );
			break;

		case OP_SUB_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = *var_a.floatPtr - *var_b.floatPtr;
			break;

		case OP_SUB_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.vectorPtr = *var_a.vectorPtr - *var_b.vectorPtr;
			break;

		case OP_MUL_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = *var_a.floatPtr * *var_b.floatPtr;
			break;

		case OP_MUL_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = *var_a.vectorPtr * *var_b.vectorPtr;
			break;

		case OP_MUL_FV:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.vectorPtr = *var_a.floatPtr * *var_b.vectorPtr;
			break;

		// Gordon: Gets optimized to the above now!
/*		case OP_MUL_VF:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.vectorPtr = *var_a.vectorPtr * *var_b.floatPtr;
			break;*/

		case OP_DIV_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );

			if ( *var_b.floatPtr == 0.0f ) {
				Warning( "Divide by zero" );
				*var_c.floatPtr = idMath::INFINITY;
			} else {
				*var_c.floatPtr = *var_a.floatPtr / *var_b.floatPtr;
			}
			break;

		case OP_MOD_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable ( st->c );

			if ( *var_b.floatPtr == 0.0f ) {
				Warning( "Divide by zero" );
				*var_c.floatPtr = *var_a.floatPtr;
			} else {
				*var_c.floatPtr = static_cast< float >( static_cast<int>( *var_a.floatPtr ) % static_cast<int>( *var_b.floatPtr ) );
			}
			break;

		case OP_BITAND:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = static_cast< float >( static_cast<int>( *var_a.floatPtr ) & static_cast<int>( *var_b.floatPtr ) );
			break;

		case OP_BITOR:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = static_cast< float >( static_cast<int>( *var_a.floatPtr ) | static_cast<int>( *var_b.floatPtr ) );
			break;

		case OP_GE:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr >= *var_b.floatPtr );
			break;

		case OP_LE:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr <= *var_b.floatPtr );
			break;

		case OP_GT:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr > *var_b.floatPtr );
			break;

		case OP_LT:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr < *var_b.floatPtr );
			break;

		case OP_AND:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr != 0.0f ) && ( *var_b.floatPtr != 0.0f );
			break;

		case OP_AND_BOOLF:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.intPtr != 0 ) && ( *var_b.floatPtr != 0.0f );
			break;

/*		case OP_AND_FBOOL:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr != 0.0f ) && ( *var_b.intPtr != 0 );
			break;*/

		case OP_AND_BOOLBOOL:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.intPtr != 0 ) && ( *var_b.intPtr != 0 );
			break;

		case OP_OR:	
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr != 0.0f ) || ( *var_b.floatPtr != 0.0f );
			break;

		case OP_OR_BOOLF:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.intPtr != 0 ) || ( *var_b.floatPtr != 0.0f );
			break;

/*		case OP_OR_FBOOL:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.floatPtr != 0.0f ) || ( *var_b.intPtr != 0 );
			break;*/
			
		case OP_OR_BOOLBOOL:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = ( *var_a.intPtr != 0 ) || ( *var_b.intPtr != 0 );
			break;
			
		case OP_NOT_BOOL:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.intPtr == 0 );
			break;

		case OP_NOT_F:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.floatPtr == 0.0f );
			break;

		case OP_NOT_V:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.vectorPtr == vec3_zero );
			break;

		case OP_NOT_S:
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( idStr::Length( GetString( st->a ) ) == 0 );
			break;

		case OP_NOT_OBJ:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( GetScriptObject( *var_a.objectId ) == NULL );
			break;

		case OP_NEG_F:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = -*var_a.floatPtr;
			break;

		case OP_NEG_V:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.vectorPtr = -*var_a.vectorPtr;
			break;

		case OP_INT_F:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = static_cast< float >( static_cast< int >( *var_a.floatPtr ) );
			break;

		case OP_EQ_B:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.intPtr == *var_b.intPtr );
			break;

		case OP_EQ_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.floatPtr == *var_b.floatPtr );
			break;

		case OP_EQ_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.vectorPtr == *var_b.vectorPtr );
			break;

		case OP_EQ_S:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( idStr::Cmp( GetString( st->a ), GetString( st->b ) ) == 0 );
			break;

		case OP_EQ_W:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( idWStr::Cmp( GetWString( st->a ), GetWString( st->b ) ) == 0 );
			break;

		case OP_EQ_O:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = GetScriptObject( *var_a.objectId ) == GetScriptObject( *var_b.objectId );
			break;

		case OP_NE_B:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.intPtr != *var_b.intPtr );
			break;

		case OP_NE_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.floatPtr != *var_b.floatPtr );
			break;

		case OP_NE_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( *var_a.vectorPtr != *var_b.vectorPtr );
			break;

		case OP_NE_S:
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( idStr::Cmp( GetString( st->a ), GetString( st->b ) ) != 0 );
			break;

		case OP_NE_W:
			var_c = GetVariable( st->c );
			*var_c.intPtr = ( idWStr::Cmp( GetWString( st->a ), GetWString( st->b ) ) != 0 );
			break;

		case OP_NE_O:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			var_c = GetVariable( st->c );
			*var_c.intPtr = GetScriptObject( *var_a.objectId ) != GetScriptObject( *var_b.objectId );
			break;

		case OP_UADD_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr += *var_a.floatPtr;
			break;

		case OP_UADD_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.vectorPtr += *var_a.vectorPtr;
			break;

		case OP_USUB_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr -= *var_a.floatPtr;
			break;

		case OP_USUB_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.vectorPtr -= *var_a.vectorPtr;
			break;

		case OP_UMUL_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr *= *var_a.floatPtr;
			break;

		case OP_UMUL_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.vectorPtr *= *var_a.floatPtr;
			break;

		case OP_UDIV_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );

			if ( *var_a.floatPtr == 0.0f ) {
				Warning( "Divide by zero" );
				*var_b.floatPtr = idMath::INFINITY;
			} else {
				*var_b.floatPtr = *var_b.floatPtr / *var_a.floatPtr;
			}
			break;

		case OP_UDIV_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );

			if ( *var_a.floatPtr == 0.0f ) {
				Warning( "Divide by zero" );
				var_b.vectorPtr->Set( idMath::INFINITY, idMath::INFINITY, idMath::INFINITY );
			} else {
				*var_b.vectorPtr = *var_b.vectorPtr / *var_a.floatPtr;
			}
			break;

		case OP_UMOD_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );

			if ( *var_a.floatPtr == 0.0f ) {
				Warning( "Divide by zero" );
				*var_b.floatPtr = *var_a.floatPtr;
			} else {
				*var_b.floatPtr = static_cast< float >( static_cast<int>( *var_b.floatPtr ) % static_cast<int>( *var_a.floatPtr ) );
			}
			break;

		case OP_UOR_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr = static_cast< float >( static_cast<int>( *var_b.floatPtr ) | static_cast<int>( *var_a.floatPtr ) );
			break;

		case OP_UAND_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr = static_cast< float >( static_cast<int>( *var_b.floatPtr ) & static_cast<int>( *var_a.floatPtr ) );
			break;

		case OP_UINC_F:
			var_a = GetVariable( st->a );
			( *var_a.floatPtr )++;
			break;

		case OP_UINCP_F:
			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				( *var.floatPtr )++;
			}
			break;

		case OP_UDEC_F:
			var_a = GetVariable( st->a );
			( *var_a.floatPtr )--;
			break;

		case OP_UDECP_F:
			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				( *var.floatPtr )--;
			}
			break;

		case OP_COMP_F:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			*var_c.floatPtr = static_cast< float >( ~static_cast<int>( *var_a.floatPtr ) );
			break;

		case OP_STORE_F:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr = *var_a.floatPtr;
			break;

		case OP_STORE_BOOL:	
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.intPtr = *var_a.intPtr;
			break;

		case OP_STORE_OBJ:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );

			obj = GetScriptObject( *var_a.objectId );

			if ( obj != NULL ) {
				const idTypeDef* t = reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetType();

				if ( st->c != NULL && !t->Inherits( st->c->TypeDef() ) ) {
					*var_b.objectId = 0;
				} else {
					*var_b.objectId = obj->GetHandle();
				}
			} else {
				*var_b.objectId = 0;
			}
			break;

		case OP_STORE_S:
			SetString( st->b, GetString( st->a ) );
			break;

		case OP_STORE_W:
			SetWString( st->b, GetWString( st->a ) );
			break;

		case OP_STORE_V:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.vectorPtr = *var_a.vectorPtr;
			break;

		case OP_STORE_FTOS:
			var_a = GetVariable( st->a );
			SetString( st->b, FloatToString( *var_a.floatPtr ) );
			break;

		case OP_STORE_BTOS:
			var_a = GetVariable( st->a );
			SetString( st->b, *var_a.intPtr ? "true" : "false" );
			break;

		case OP_STORE_VTOS:
			var_a = GetVariable( st->a );
			SetString( st->b, var_a.vectorPtr->ToString() );
			break;

		case OP_STORE_FTOBOOL:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			if ( *var_a.floatPtr != 0.0f ) {
				*var_b.intPtr = 1;
			} else {
				*var_b.intPtr = 0;
			}
			break;

		case OP_STORE_BOOLTOF:
			var_a = GetVariable( st->a );
			var_b = GetVariable( st->b );
			*var_b.floatPtr = *var_a.intPtr ? 1.f : 0.f;
			break;

		case OP_STOREP_F:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->floatPtr ) {
				var_a = GetVariable( st->a );
				*var_b.evalPtr->floatPtr = *var_a.floatPtr;
			}
			break;

		case OP_STOREP_FLD:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->intPtr ) {
				var_a = GetVariable( st->a );
				*var_b.evalPtr->intPtr = *var_a.intPtr;
			}
			break;

		case OP_STOREP_BOOL:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->intPtr ) {
				var_a = GetVariable( st->a );
				*var_b.evalPtr->intPtr = *var_a.intPtr;
			}
			break;

		case OP_STOREP_S:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->stringPtr ) {
				idStr::Copynz( var_b.evalPtr->stringPtr, GetString( st->a ), MAX_STRING_LEN );
			}
			break;

		case OP_STOREP_W:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->wstringPtr ) {
				idWStr::Copynz( var_b.evalPtr->wstringPtr, GetWString( st->a ), MAX_STRING_LEN );
			}
			break;

		case OP_STOREP_V:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->vectorPtr ) {
				var_a = GetVariable( st->a );
				*var_b.evalPtr->vectorPtr = *var_a.vectorPtr;
			}
			break;
		
		case OP_STOREP_FTOS:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->stringPtr ) {
				var_a = GetVariable( st->a );
				idStr::Copynz( var_b.evalPtr->stringPtr, FloatToString( *var_a.floatPtr ), MAX_STRING_LEN );
			}
			break;

		case OP_STOREP_BTOS:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->stringPtr ) {
				var_a = GetVariable( st->a );
				if ( *var_a.floatPtr != 0.0f ) {
					idStr::Copynz( var_b.evalPtr->stringPtr, "true", MAX_STRING_LEN );
				} else {
					idStr::Copynz( var_b.evalPtr->stringPtr, "false", MAX_STRING_LEN );
				}
			}
			break;

		case OP_STOREP_VTOS:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->stringPtr ) {
				var_a = GetVariable( st->a );
				idStr::Copynz( var_b.evalPtr->stringPtr, var_a.vectorPtr->ToString(), MAX_STRING_LEN );
			}
			break;

		case OP_STOREP_FTOBOOL:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->intPtr ) {
				var_a = GetVariable( st->a );
				if ( *var_a.floatPtr != 0.0f ) {
					*var_b.evalPtr->intPtr = 1;
				} else {
					*var_b.evalPtr->intPtr = 0;
				}
			}
			break;

		case OP_STOREP_BOOLTOF:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->floatPtr ) {
				var_a = GetVariable( st->a );
				*var_b.evalPtr->floatPtr = *var_a.intPtr ? 1.f : 0.f;
			}
			break;

		case OP_STOREP_OBJ:
			var_b = GetVariable( st->b );
			if ( var_b.evalPtr && var_b.evalPtr->objectId ) {
				var_a = GetVariable( st->a );
				obj = GetScriptObject( *var_a.objectId );
				
				if ( obj == NULL ) {
					*var_b.evalPtr->objectId = 0;
				} else {
					const idTypeDef* t = reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetType();
					if ( !t->Inherits( st->c->TypeDef() ) ) {
						*var_b.evalPtr->objectId = 0;
					} else {
						*var_b.evalPtr->objectId = *var_a.objectId;
					}
				}
			}
			break;

		case OP_ADDRESS:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var_c.evalPtr->bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
			} else {
				Warning( "Indirect on a NULL object" );
				var_c.evalPtr->bytePtr = NULL;
			}
			break;

		case OP_INDIRECT_F:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				*var_c.floatPtr = *var.floatPtr;
			} else {
				Warning( "Indirect on a NULL object" );
				*var_c.floatPtr = 0.0f;
			}
			break;

		case OP_INDIRECT_BOOL:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				*var_c.intPtr = *var.intPtr;
			} else {
				Warning( "Indirect on a NULL object" );
				*var_c.intPtr = 0;
			}
			break;

		case OP_INDIRECT_S:
			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				SetString( st->c, var.stringPtr );
			} else {
				Warning( "Indirect on a NULL object" );
				SetString( st->c, "" );
			}
			break;

		case OP_INDIRECT_W:
			var_a = GetVariable( st->a );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				SetWString( st->c, var.wstringPtr );
			} else {
				SetWString( st->c, L"" );
			}
			break;

		case OP_INDIRECT_V:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				*var_c.vectorPtr = *var.vectorPtr;
			} else {
				Warning( "Indirect on a NULL object" );
				var_c.vectorPtr->Zero();
			}
			break;

		case OP_INDIRECT_OBJ:
			var_a = GetVariable( st->a );
			var_c = GetVariable( st->c );
			obj = GetScriptObject( *var_a.objectId );
			if ( obj ) {
				var.bytePtr = &reinterpret_cast< idProgramTypeObject* >( obj->GetObject() )->GetData()[ st->b->value.ptrOffset ];
				*var_c.objectId = *var.objectId;
			} else {
				Warning( "Indirect on a NULL object" );
				*var_c.objectId = 0;
			}
			break;

		case OP_PUSH_F:
			var_a = GetVariable( st->a );
			Push( *reinterpret_cast<int *>( var_a.floatPtr ) );
			break;

		case OP_PUSH_FTOS:
			var_a = GetVariable( st->a );
			PushString( FloatToString( *var_a.floatPtr ) );
			break;

		case OP_PUSH_FTOW:
			var_a = GetVariable( st->a );
			PushWString( FloatToWString( *var_a.floatPtr ) );
			break;

		case OP_PUSH_BTOF:
			var_a = GetVariable( st->a );
			floatVal = *var_a.intPtr ? 1.f : 0.f;
			Push( *reinterpret_cast<int *>( &floatVal ) );
			break;

		case OP_PUSH_FTOB:
			var_a = GetVariable( st->a );
			if ( *var_a.floatPtr != 0.0f ) {
				Push( 1 );
			} else {
				Push( 0 );
			}
			break;

		case OP_PUSH_VTOS:
			var_a = GetVariable( st->a );
			PushString( var_a.vectorPtr->ToString() );
			break;

		case OP_PUSH_BTOS:
			var_a = GetVariable( st->a );
			PushString( *var_a.intPtr ? "true" : "false" );
			break;

		case OP_PUSH_S:
			PushString( GetString( st->a ) );
			break;

		case OP_PUSH_W:
			PushWString( GetWString( st->a ) );
			break;

		case OP_PUSH_V:
			var_a = GetVariable( st->a );
			Push( *reinterpret_cast<int *>( &var_a.vectorPtr->x ) );
			Push( *reinterpret_cast<int *>( &var_a.vectorPtr->y ) );
			Push( *reinterpret_cast<int *>( &var_a.vectorPtr->z ) );
			break;

		case OP_PUSH_OBJ:
			var_a = GetVariable( st->a );
			Push( *var_a.objectId );
			break;

			NODEFAULT;
		}
	}

	return threadDying;
}

/*
================
idInterpreter::GetScriptObject
================
*/
idScriptObject* idInterpreter::GetScriptObject( int objectId ) const {
	return program->GetScriptObject( objectId );
}
