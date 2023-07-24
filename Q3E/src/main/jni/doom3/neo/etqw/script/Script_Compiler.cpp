// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/Licensee.h"

#include "Script_Compiler.h"
#include "Script_Thread.h"

#define FUNCTION_PRIORITY	2
#define INT_PRIORITY		2
#define NOT_PRIORITY		5
#define TILDE_PRIORITY		5
#define TOP_PRIORITY		7

bool idCompiler::punctuationValid[ 256 ];
char *idCompiler::punctuation[] = {
	"+=", "-=", "*=", "/=", "%=", "&=", "|=", "++", "--",
	"&&", "||", "<=", ">=", "==", "!=", "::", ";",  ",",
	"~",  "!",  "*",  "/",  "%",  "(",   ")",  "-", "+",
	"=",  "[",  "]",  ".",  "<",  ">" ,  "&",  "|", ":",  NULL
};

opcode_t idCompiler::opcodes[] = {
	{ "<RETURN>", "RETURN", -1, false, &def_void, &def_void, &def_void },
		
	{ "++", "UINC_F", 1, true, &def_float, &def_void, &def_void },
	{ "++", "UINCP_F", 1, true, &def_object, &def_field, &def_float },
	{ "--", "UDEC_F", 1, true, &def_float, &def_void, &def_void },
	{ "--", "UDECP_F", 1, true, &def_object, &def_field, &def_float },

	{ "~", "COMP_F", -1, false, &def_float, &def_void, &def_float },
	
	{ "*", "MUL_F", 3, false, &def_float, &def_float, &def_float },
	{ "*", "MUL_V", 3, false, &def_vector, &def_vector, &def_float },
	{ "*", "MUL_FV", 3, false, &def_float, &def_vector, &def_vector },
	{ "*", "MUL_VF", 3, false, &def_vector, &def_float, &def_vector },
	
	{ "/", "DIV", 3, false, &def_float, &def_float, &def_float },
	{ "%", "MOD_F",	3, false, &def_float, &def_float, &def_float },
	
	{ "+", "ADD_F", 4, false, &def_float, &def_float, &def_float },
	{ "+", "ADD_V", 4, false, &def_vector, &def_vector, &def_vector },
	{ "+", "ADD_S", 4, false, &def_string, &def_string, &def_string },
	{ "+", "ADD_FS", 4, false, &def_float, &def_string, &def_string },
	{ "+", "ADD_SF", 4, false, &def_string, &def_float, &def_string },
	{ "+", "ADD_VS", 4, false, &def_vector, &def_string, &def_string },
	{ "+", "ADD_SV", 4, false, &def_string, &def_vector, &def_string },
	
	{ "-", "SUB_F", 4, false, &def_float, &def_float, &def_float },
	{ "-", "SUB_V", 4, false, &def_vector, &def_vector, &def_vector },
		
	{ "==", "EQ_B", 5, false, &def_boolean, &def_boolean, &def_boolean },
	{ "==", "EQ_F", 5, false, &def_float, &def_float, &def_boolean },
	{ "==", "EQ_V", 5, false, &def_vector, &def_vector, &def_boolean },
	{ "==", "EQ_S", 5, false, &def_string, &def_string, &def_boolean },
	{ "==", "EQ_W", 5, false, &def_wstring, &def_wstring, &def_boolean },
	{ "==", "EQ_O", 5, false, &def_object, &def_object, &def_boolean },
	
	{ "!=", "NE_B", 5, false, &def_boolean, &def_boolean, &def_boolean },
	{ "!=", "NE_F", 5, false, &def_float, &def_float, &def_boolean },
	{ "!=", "NE_V", 5, false, &def_vector, &def_vector, &def_boolean },
	{ "!=", "NE_S", 5, false, &def_string, &def_string, &def_boolean },
	{ "!=", "NE_W", 5, false, &def_wstring, &def_wstring, &def_boolean },
	{ "!=", "NE_O", 5, false, &def_object, &def_object, &def_boolean },
	
	{ "<=", "LE", 5, false, &def_float, &def_float, &def_float },
	{ ">=", "GE", 5, false, &def_float, &def_float, &def_float },
	{ "<", "LT", 5, false, &def_float, &def_float, &def_float },
	{ ">", "GT", 5, false, &def_float, &def_float, &def_float },
	
	{ ".", "INDIRECT_F", 1, false, &def_object, &def_field, &def_float },
	{ ".", "INDIRECT_V", 1, false, &def_object, &def_field, &def_vector },
	{ ".", "INDIRECT_S", 1, false, &def_object, &def_field, &def_string },
	{ ".", "INDIRECT_W", 1, false, &def_object, &def_field, &def_wstring },
	{ ".", "INDIRECT_BOOL", 1, false, &def_object, &def_field, &def_boolean },
	{ ".", "INDIRECT_OBJ", 1, false, &def_object, &def_field, &def_object },

	{ ".", "ADDRESS", 1, false, &def_object, &def_field, &def_pointer },

	{ ".", "EVENTCALL", 2, false, &def_object, &def_function, &def_void },
	{ ".", "OBJECTCALL", 2, false, &def_object, &def_function, &def_void },
	{ ".", "SYSCALL", 2, false, &def_void, &def_function, &def_void },

	{ "=", "STORE_F", 6, true, &def_float, &def_float, &def_float },
	{ "=", "STORE_V", 6, true, &def_vector, &def_vector, &def_vector },
	{ "=", "STORE_S", 6, true, &def_string, &def_string, &def_string },
	{ "=", "STORE_W", 6, true, &def_wstring, &def_wstring, &def_wstring },
	{ "=", "STORE_BOOL", 6, true, &def_boolean, &def_boolean, &def_boolean },
	{ "=", "STORE_OBJ", 6, true, &def_object, &def_object, &def_object },
	
	{ "=", "STORE_FTOS", 6, true, &def_string, &def_float, &def_string },
	{ "=", "STORE_BTOS", 6, true, &def_string, &def_boolean, &def_string },
	{ "=", "STORE_VTOS", 6, true, &def_string, &def_vector, &def_string },
	{ "=", "STORE_FTOBOOL", 6, true, &def_boolean, &def_float, &def_boolean },
	{ "=", "STORE_BOOLTOF", 6, true, &def_float, &def_boolean, &def_float },

	{ "=", "STOREP_F", 6, true, &def_pointer, &def_float, &def_float },
	{ "=", "STOREP_V", 6, true, &def_pointer, &def_vector, &def_vector },
	{ "=", "STOREP_S", 6, true, &def_pointer, &def_string, &def_string },
	{ "=", "STOREP_W", 6, true, &def_pointer, &def_wstring, &def_wstring },
	{ "=", "STOREP_FLD", 6, true, &def_pointer, &def_field, &def_field },
	{ "=", "STOREP_BOOL", 6, true, &def_pointer, &def_boolean, &def_boolean },
	{ "=", "STOREP_OBJ", 6, true, &def_pointer, &def_object, &def_object },

	{ "<=>", "STOREP_FTOS", 6, true, &def_pointer, &def_float, &def_string },
	{ "<=>", "STOREP_BTOS", 6, true, &def_pointer, &def_boolean, &def_string },
	{ "<=>", "STOREP_VTOS", 6, true, &def_pointer, &def_vector, &def_string },
	{ "<=>", "STOREP_FTOBOOL", 6, true, &def_pointer, &def_float, &def_boolean },
	{ "<=>", "STOREP_BOOLTOF", 6, true, &def_pointer, &def_boolean, &def_float },
	
	{ "*=", "UMUL_F", 6, true, &def_float, &def_float, &def_void },
	{ "*=", "UMUL_V", 6, true, &def_vector, &def_float, &def_void },
	{ "/=", "UDIV_F", 6, true, &def_float, &def_float, &def_void },
	{ "/=", "UDIV_V", 6, true, &def_vector, &def_float, &def_void },
	{ "%=", "UMOD_F", 6, true, &def_float, &def_float, &def_void },
	{ "+=", "UADD_F", 6, true, &def_float, &def_float, &def_void },
	{ "+=", "UADD_V", 6, true, &def_vector, &def_vector, &def_void },
	{ "-=", "USUB_F", 6, true, &def_float, &def_float, &def_void },
	{ "-=", "USUB_V", 6, true, &def_vector, &def_vector, &def_void },
	{ "&=", "UAND_F", 6, true, &def_float, &def_float, &def_void },
	{ "|=", "UOR_F", 6, true, &def_float, &def_float, &def_void },
	
	{ "!", "NOT_BOOL", -1, false, &def_boolean, &def_void, &def_boolean },
	{ "!", "NOT_F", -1, false, &def_float, &def_void, &def_boolean },
	{ "!", "NOT_V", -1, false, &def_vector, &def_void, &def_boolean },
	{ "!", "NOT_S", -1, false, &def_vector, &def_void, &def_boolean },
	{ "!", "NOT_OBJ", -1, false, &def_object, &def_void, &def_boolean },

	{ "<NEG_F>", "NEG_F", -1, false, &def_float, &def_void, &def_float },
	{ "<NEG_V>", "NEG_V", -1, false, &def_vector, &def_void, &def_vector },

	{ "int", "INT_F", -1, false, &def_float, &def_void, &def_float },
	
	{ "<IF>", "IF", -1, false, &def_float, &def_jumpoffset, &def_void },
	{ "<IFNOT>", "IFNOT", -1, false, &def_float, &def_jumpoffset, &def_void },
	
	// calls returns REG_RETURN
	{ "<CALL>", "CALL", -1, false, &def_function, &def_argsize, &def_void },
	{ "<THREAD>", "THREAD", -1, false, &def_function, &def_argsize, &def_void },
	{ "<GUITHREAD>", "GUITHREAD", -1, false, &def_function, &def_argsize, &def_void },
	{ "<THREAD>", "OBJTHREAD", -1, false, &def_function, &def_argsize, &def_void },
	{ "<GUITHREAD>", "GUIOBJTHREAD", -1, false, &def_function, &def_argsize, &def_void },
	
	{ "<PUSH>", "PUSH_F", -1, false, &def_float, &def_float, &def_void },
	{ "<PUSH>", "PUSH_V", -1, false, &def_vector, &def_vector, &def_void },
	{ "<PUSH>", "PUSH_S", -1, false, &def_string, &def_string, &def_void },
	{ "<PUSH>", "PUSH_W", -1, false, &def_wstring, &def_wstring, &def_void },
	{ "<PUSH>", "PUSH_OBJ", -1, false, &def_object, &def_object, &def_void },
	{ "<PUSH>", "PUSH_FTOS", -1, false, &def_string, &def_float, &def_void },
	{ "<PUSH>", "PUSH_FTOW", -1, false, &def_wstring, &def_float, &def_void },
	{ "<PUSH>", "PUSH_BTOF", -1, false, &def_float, &def_boolean, &def_void },
	{ "<PUSH>", "PUSH_FTOB", -1, false, &def_boolean, &def_float, &def_void },
	{ "<PUSH>", "PUSH_VTOS", -1, false, &def_string, &def_vector, &def_void },
	{ "<PUSH>", "PUSH_BTOS", -1, false, &def_string, &def_boolean, &def_void },
	
	{ "<GOTO>", "GOTO", -1, false, &def_jumpoffset, &def_void, &def_void },
	
	{ "&&", "AND", 7, false, &def_float, &def_float, &def_float },
	{ "&&", "AND_BOOLF", 7, false, &def_boolean, &def_float, &def_float },
	{ "&&", "AND_FBOOL", 7, false, &def_float, &def_boolean, &def_float },
	{ "&&", "AND_BOOLBOOL", 7, false, &def_boolean, &def_boolean, &def_float },
	{ "||", "OR", 7, false, &def_float, &def_float, &def_float },
	{ "||", "OR_BOOLF", 7, false, &def_boolean, &def_float, &def_float },
	{ "||", "OR_FBOOL", 7, false, &def_float, &def_boolean, &def_float },
	{ "||", "OR_BOOLBOOL", 7, false, &def_boolean, &def_boolean, &def_float },
	
	{ "&", "BITAND", 3, false, &def_float, &def_float, &def_float },
	{ "|", "BITOR", 3, false, &def_float, &def_float, &def_float },

	{ "<BREAK>", "BREAK", -1, false, &def_float, &def_void, &def_void },
	{ "<CONTINUE>", "CONTINUE", -1, false, &def_float, &def_void, &def_void },

	{ "<VIRTUAL>", "VIRTUALEVENTCALL", -1, false, &def_float, &def_void, &def_void },

	{ "<ALLOC>", "ALLOC", -1, false, &def_float, &def_void, &def_object },
	{ "<FREE>", "FREE", -1, false, &def_object, &def_void, &def_void },	

	{ NULL }
};

/*
================
idCompiler::idCompiler
================
*/
idCompiler::idCompiler( idProgram* _program ) {
	char	**ptr;
	int		id;

	program = _program;

	// make sure we have the right # of opcodes in the table
	assert( ( sizeof( opcodes ) / sizeof( opcodes[ 0 ] ) ) == ( NUM_OPCODES + 1 ) );

	eof	= true;
	parserPtr = &parser;

	callthread			= false;
	callguithread		= false;
	lastStatementWasReturn	= false;
	loopDepth			= 0;
	eof					= false;
	braceDepth			= 0;
	immediateType		= NULL;
	basetype			= NULL;
	currentLineNumber	= 0;
	currentFileNumber	= 0;
	errorCount			= 0;
	scope				= &def_namespace;

	memset( &immediate, 0, sizeof( immediate ) );
	memset( punctuationValid, 0, sizeof( punctuationValid ) );
	for( ptr = punctuation; *ptr != NULL; ptr++ ) {
		id = parserPtr->GetPunctuationId( *ptr );
		if ( ( id >= 0 ) && ( id < 256 ) ) {
			punctuationValid[ id ] = true;
		}
	}

	int i;
	for ( i = 0; i < NUM_OPCODES; i++ ) {
		opcodes[ i ].emitCount = 0;
	}
}

/*
============
idCompiler::Error

Aborts the current file load
============
*/
void idCompiler::Error( const char *message, ... ) const {
	va_list	argptr;
	char	string[ 1024 ];

	va_start( argptr, message );
	vsprintf( string, message, argptr );
	va_end( argptr );

	throw idCompileError( string );
}

/*
============
idCompiler::Warning

Prints a warning about the current line
============
*/
void idCompiler::Warning( const char *message, ... ) const {
	va_list	argptr;
	char	string[ 1024 ];

	va_start( argptr, message );
	vsprintf( string, message, argptr );
	va_end( argptr );

	parserPtr->Warning( "%s", string );
}

/*
============
idCompiler::VirtualFunctionConstant

Creates a def for an index into a virtual function table
============
*/
idVarDef* idCompiler::VirtualFunctionConstant( idVarDef *func ) {
	eval_t eval;

	memset( &eval, 0, sizeof( eval ) );
	eval._int = func->scope->TypeDef()->GetFunctionNumber( func->value.functionPtr );
	if ( eval._int < 0 ) {
		Error( "Function '%s' not found in scope '%s'", func->Name(), scope->TypeDef()->Name() );
	}
    
	return GetImmediate( &type_virtualfunction, &eval, "" );
}

/*
============
idCompiler::GlobalVirtualFunctionConstant
============
*/
idVarDef* idCompiler::GlobalVirtualFunctionConstant( idVarDef *func ) {
	eval_t eval;

	memset( &eval, 0, sizeof( eval ) );
	eval._int = func->value.functionPtr->virtualIndex;
	if ( eval._int < 0 ) {
		Error( "Function '%s' is not a virtual function", func->Name() );
	}

	return GetImmediate( &type_virtualfunction, &eval, "" );
}

/*
============
idCompiler::SizeConstant

Creates a def for a size constant
============
*/
ID_INLINE idVarDef *idCompiler::SizeConstant( int size ) {
	eval_t eval;

	memset( &eval, 0, sizeof( eval ) );
	eval._int = size;
	return GetImmediate( &type_argsize, &eval, "" );
}

/*
============
idCompiler::JumpConstant

Creates a def for a jump constant
============
*/
ID_INLINE idVarDef *idCompiler::JumpConstant( int value ) {
	eval_t eval;

	memset( &eval, 0, sizeof( eval ) );
	eval._int = value;
	return GetImmediate( &type_jumpoffset, &eval, "" );
}

/*
============
idCompiler::JumpDef

Creates a def for a relative jump from one code location to another
============
*/
ID_INLINE idVarDef *idCompiler::JumpDef( int jumpfrom, int jumpto ) {
	return JumpConstant( jumpto - jumpfrom );
}

/*
============
idCompiler::JumpTo

Creates a def for a relative jump from current code location
============
*/
ID_INLINE idVarDef *idCompiler::JumpTo( int jumpto ) {
	return JumpDef( program->NumStatements(), jumpto );
}

/*
============
idCompiler::JumpFrom

Creates a def for a relative jump from code location to current code location
============
*/
ID_INLINE idVarDef *idCompiler::JumpFrom( int jumpfrom ) {
	return JumpDef( jumpfrom, program->NumStatements() );
}

/*
============
idCompiler::Divide
============
*/
ID_INLINE float idCompiler::Divide( float numerator, float denominator ) {
	if ( denominator == 0 ) {
		Error( "Divide by zero" );
		return 0;
	}

	return numerator / denominator;
}

/*
============
idCompiler::FindImmediate

tries to find an existing immediate with the same value
============
*/
idVarDef *idCompiler::FindImmediate( const idTypeDef *type, const eval_t *eval, const char *string ) const {
	idVarDef	*def;
	etype_t		etype;

	etype = type->Type();

	// check for a constant with the same value
	for( def = program->GetDefList( "<IMMEDIATE>" ); def != NULL; def = def->Next() ) {
		if ( def->TypeDef() != type ) {
			continue;
		}

		switch( etype ) {
		case ev_field :
			if ( *def->value.intPtr == eval->_int ) {
				return def;
			}
			break;

		case ev_argsize :
			if ( def->value.argSize == eval->_int ) {
				return def;
			}
			break;

		case ev_jumpoffset :
			if ( def->value.jumpOffset == eval->_int ) {
				return def;
			}
			break;

		case ev_object:
			if ( *def->value.objectId == eval->_objectId ) {
				return def;
			}
			break;

		case ev_string :
			if ( idStr::Cmp( def->value.stringPtr, string ) == 0 ) {
				return def;
			}
			break;

		case ev_float :
			if ( *def->value.floatPtr == eval->_float ) {
				return def;
			}
			break;

		case ev_virtualfunction :
			if ( def->value.virtualFunction == eval->_int ) {
				return def;
			}
			break;


		case ev_vector :
			if ( ( def->value.vectorPtr->x == eval->vector[ 0 ] ) && 
				( def->value.vectorPtr->y == eval->vector[ 1 ] ) && 
				( def->value.vectorPtr->z == eval->vector[ 2 ] ) ) {
				return def;
			}
			break;

		case ev_boolean:
			if ( *def->value.intPtr == eval->_int ) {
				return def;
			}
			break;

		default :
			Error( "weird immediate type" );
			break;
		}
	}

	return NULL;
}

/*
============
idCompiler::GetImmediate

returns an existing immediate with the same value, or allocates a new one
============
*/
idVarDef *idCompiler::GetImmediate( idTypeDef *type, const eval_t *eval, const char *string ) {
	idVarDef *def;

	def = FindImmediate( type, eval, string );
	if ( def ) {
		def->numUsers++;
	} else {
		// allocate a new def
		def = program->AllocDef( type, "<IMMEDIATE>", &def_namespace );
		if ( type->Type() == ev_string ) {
			def->SetString( string, true );
		} else {
			def->SetValue( *eval, true );
		}
	}

	return def;
}

/*
============
idCompiler::OptimizeOpcode

try to optimize when the operator works on constants only
============
*/
idVarDef *idCompiler::OptimizeOpcode( constOpCodePtr_t& op, varPtr_t& var_a, varPtr_t& var_b ) {
	eval_t		c;
	idTypeDef	*type;

	if ( ( var_a && var_a->settings.initialized != idVarDef::initializedConstant ) || ( var_b && var_b->settings.initialized != idVarDef::initializedConstant ) ) {
		switch ( op - opcodes ) {
			case OP_MUL_VF: {
				op = &opcodes[ OP_MUL_FV ];
				Swap( var_a, var_b );
				break;
			}
			case OP_AND_FBOOL: {
				op = &opcodes[ OP_AND_BOOLF ];
				Swap( var_a, var_b );
				break;
			}
			case OP_OR_FBOOL: {
				op = &opcodes[ OP_OR_BOOLF ];
				Swap( var_a, var_b );
				break;
			}
		}

		return NULL;
	}

	idVec3 &vec_c = *reinterpret_cast<idVec3 *>( &c.vector[ 0 ] );

	memset( &c, 0, sizeof( c ) );
	switch( op - opcodes ) {
		case OP_ADD_F:		c._float = *var_a->value.floatPtr + *var_b->value.floatPtr; type = &type_float; break;
		case OP_ADD_V:		vec_c = *var_a->value.vectorPtr + *var_b->value.vectorPtr; type = &type_vector; break;
		case OP_SUB_F:		c._float = *var_a->value.floatPtr - *var_b->value.floatPtr; type = &type_float; break;
		case OP_SUB_V:		vec_c = *var_a->value.vectorPtr - *var_b->value.vectorPtr; type = &type_vector; break;
		case OP_MUL_F:		c._float = *var_a->value.floatPtr * *var_b->value.floatPtr; type = &type_float; break;
		case OP_MUL_V:		c._float = *var_a->value.vectorPtr * *var_b->value.vectorPtr; type = &type_float; break;
		case OP_MUL_FV:		vec_c = *var_b->value.vectorPtr * *var_a->value.floatPtr; type = &type_vector; break;
		case OP_MUL_VF:		vec_c = *var_a->value.vectorPtr * *var_b->value.floatPtr; type = &type_vector; break;
		case OP_DIV_F:		c._float = Divide( *var_a->value.floatPtr, *var_b->value.floatPtr ); type = &type_float; break;
		case OP_MOD_F:		c._float = static_cast< float >( ( int )*var_a->value.floatPtr % ( int )*var_b->value.floatPtr ); type = &type_float; break;
		case OP_BITAND:		c._float = static_cast< float >( ( int )*var_a->value.floatPtr & ( int )*var_b->value.floatPtr ); type = &type_float; break;
		case OP_BITOR:		c._float = static_cast< float >( ( int )*var_a->value.floatPtr | ( int )*var_b->value.floatPtr ); type = &type_float; break;
		case OP_GE:			c._float = *var_a->value.floatPtr >= *var_b->value.floatPtr; type = &type_float; break;
		case OP_LE:			c._float = *var_a->value.floatPtr <= *var_b->value.floatPtr; type = &type_float; break;
		case OP_GT:			c._float = *var_a->value.floatPtr > *var_b->value.floatPtr; type = &type_float; break;
		case OP_LT:			c._float = *var_a->value.floatPtr < *var_b->value.floatPtr; type = &type_float; break;
		case OP_AND:		c._float = *var_a->value.floatPtr && *var_b->value.floatPtr; type = &type_float; break;
		case OP_OR:			c._float = *var_a->value.floatPtr || *var_b->value.floatPtr; type = &type_float; break;
		case OP_NOT_BOOL:	c._int = !*var_a->value.intPtr; type = &type_boolean; break;
		case OP_NOT_F:		c._float = !*var_a->value.floatPtr; type = &type_float; break;
		case OP_NOT_V:		c._float = !var_a->value.vectorPtr->x && !var_a->value.vectorPtr->y && !var_a->value.vectorPtr->z; type = &type_float; break;
		case OP_NEG_F:		c._float = -*var_a->value.floatPtr; type = &type_float; break;
		case OP_NEG_V:		vec_c = -*var_a->value.vectorPtr; type = &type_vector; break;
		case OP_INT_F:		c._float = static_cast< float >( ( int )*var_a->value.floatPtr ); type = &type_float; break;
		case OP_EQ_B:		c._float = ( *var_a->value.intPtr == *var_b->value.intPtr ); type = &type_float; break;
		case OP_EQ_F:		c._float = ( *var_a->value.floatPtr == *var_b->value.floatPtr ); type = &type_float; break;
		case OP_EQ_V:		c._float = var_a->value.vectorPtr->Compare( *var_b->value.vectorPtr ); type = &type_float; break;
		case OP_NE_B:		c._float = ( *var_a->value.intPtr != *var_b->value.intPtr ); type = &type_float; break;
		case OP_NE_F:		c._float = ( *var_a->value.floatPtr != *var_b->value.floatPtr ); type = &type_float; break;
		case OP_NE_V:		c._float = !var_a->value.vectorPtr->Compare( *var_b->value.vectorPtr ); type = &type_float; break;
		case OP_UADD_F:		c._float = *var_b->value.floatPtr + *var_a->value.floatPtr; type = &type_float; break;
		case OP_USUB_F:		c._float = *var_b->value.floatPtr - *var_a->value.floatPtr; type = &type_float; break;
		case OP_UMUL_F:		c._float = *var_b->value.floatPtr * *var_a->value.floatPtr; type = &type_float; break;
		case OP_UDIV_F:		c._float = Divide( *var_b->value.floatPtr, *var_a->value.floatPtr ); type = &type_float; break;
		case OP_UMOD_F:		c._float = static_cast< float >( ( int ) *var_b->value.floatPtr % ( int )*var_a->value.floatPtr ); type = &type_float; break;
		case OP_UOR_F:		c._float = static_cast< float >( ( int )*var_b->value.floatPtr | ( int )*var_a->value.floatPtr ); type = &type_float; break;
		case OP_UAND_F: 	c._float = static_cast< float >( ( int )*var_b->value.floatPtr & ( int )*var_a->value.floatPtr ); type = &type_float; break;
		case OP_UINC_F:		c._float = *var_a->value.floatPtr + 1; type = &type_float; break;
		case OP_UDEC_F:		c._float = *var_a->value.floatPtr - 1; type = &type_float; break;
		case OP_COMP_F:		c._float = ( float )~( int )*var_a->value.floatPtr; type = &type_float; break;
		default:			type = NULL; break;
	}

	if ( !type ) {
		return NULL;
	}

	if ( var_a ) {
		var_a->numUsers--;
		if ( var_a->numUsers <= 0 ) {
			program->FreeDef( var_a, NULL );
		}
	}
	if ( var_b ) {
		var_b->numUsers--;
		if ( var_b->numUsers <= 0 ) {
			program->FreeDef( var_b, NULL );
		}
	}

	return GetImmediate( type, &c, "" );
}

/*
============
idCompiler::EmitOpcode

Emits a primitive statement, returning the var it places it's value in
============
*/
idVarDef *idCompiler::EmitOpcode( const opcode_t *op, idVarDef* var_a, idVarDef* var_b ) {
	statement_t	*statement;
	idVarDef	*var_c;

	var_c = OptimizeOpcode( op, var_a, var_b );
	if ( var_c ) {
		return var_c;
	}

	int code = op - opcodes;
	if ( code < OP_PUSH_F || code > OP_PUSH_BTOS ) { // Gordon: don't reuse push results until the function call has been finished
		if ( var_a && var_a->settings.isReturn ) {
			var_a->numUsers++;
		}
		if ( var_b && var_b->settings.isReturn ) {
			var_b->numUsers++;
		}
	}

	statement = program->AllocStatement();
	statement->linenumber	= currentLineNumber;
	statement->file 		= currentFileNumber;
	
	if ( ( op->type_c == &def_void ) || op->rightAssociative ) {
		// ifs, gotos, and assignments don't need vars allocated
		var_c = NULL;
	} else {
		idTypeDef* resultType = op->type_c->TypeDef();
		code = op - opcodes;
		if ( code >= OP_INDIRECT_F && code <= OP_INDIRECT_OBJ ) {
			// Gordon: if it is an indirect, grab the real type from var b
			resultType = var_b->TypeDef()->FieldType();

			assert( resultType->Type() != ev_pointer );
		}

		// allocate result space
		// try to reuse result defs as much as possible
		var_c = program->FindFreeResultDef( resultType, scope, var_a, var_b );
		// set user count back to 1, a result def needs to be used twice before it can be reused
		var_c->numUsers = 1;
	}

	statement->op	= op - opcodes;
	statement->a	= var_a;
	statement->b	= var_b;
	statement->c	= var_c;
#ifdef DEBUG_SCRIPTS
	statement->executionCount = 0;
#endif // DEBUG_SCRIPTS

	op->emitCount++;

	if ( op->rightAssociative ) {
		return var_a;
	}

	return var_c;
}

/*
============
idCompiler::EmitOpcode

Emits a primitive statement, returning the var it places it's value in
============
*/
ID_INLINE idVarDef *idCompiler::EmitOpcode( int op, idVarDef *var_a, idVarDef *var_b ) {
	return EmitOpcode( &opcodes[ op ], var_a, var_b );
}

/*
============
idCompiler::EmitPush

Emits an opcode to push the variable onto the stack.
============
*/
bool idCompiler::EmitPush( idVarDef *expression, const idTypeDef *funcArg ) {
	opcode_t *op;
	opcode_t *out;

	out = NULL;
	for( op = &opcodes[ OP_PUSH_F ]; op->name && !idStr::Cmp( op->name, "<PUSH>" ); op++ ) {
		if ( ( funcArg->Type() == op->type_a->Type() ) && ( expression->Type() == op->type_b->Type() ) ) {
			out = op;
			break;
		}
	}

	if ( !out ) {
		if ( ( expression->TypeDef() != funcArg ) && !expression->TypeDef()->Inherits( funcArg ) ) {
			return false;
		}

		out = &opcodes[ OP_PUSH_OBJ ];
	}

	EmitOpcode( out, expression, NULL );

	return true;
}

/*
==============
idCompiler::NextToken

Sets token, immediateType, and possibly immediate
==============
*/
void idCompiler::NextToken( void ) {
	int i;

	// reset our type
	immediateType = NULL;
	memset( &immediate, 0, sizeof( immediate ) );

	// Save the token's line number and filename since when we emit opcodes the current 
	// token is always the next one to be read 
	currentLineNumber = token.line;
	currentFileNumber = program->GetFilenum( parserPtr->GetFileName() );

	if ( !parserPtr->ReadToken( &token ) ) {
		eof = true;
		return;
	}

	if ( currentFileNumber != program->GetFilenum( parserPtr->GetFileName() ) ) {
		if ( ( braceDepth > 0 ) && ( token != "}" ) ) {
			// missing a closing brace.  try to give as much info as possible.
			if ( scope->Type() == ev_function ) {
				Error( "Unexpected end of file inside function '%s'.  Missing closing braces.", scope->Name() );
			} else if ( scope->Type() == ev_object ) {
				Error( "Unexpected end of file inside object '%s'.  Missing closing braces.", scope->Name() );
			} else if ( scope->Type() == ev_namespace ) {
				Error( "Unexpected end of file inside namespace '%s'.  Missing closing braces.", scope->Name() );
			} else {
				Error( "Unexpected end of file inside braced section" );
			}
		}
	}

	switch( token.type ) {
	case TT_STRING:
		// handle quoted strings as a unit
		immediateType = &type_string;
		return;

	case TT_LITERAL: {
		// handle quoted vectors as a unit
		immediateType = &type_vector;
		idLexer lex( token, token.Length(), parserPtr->GetFileName(), LEXFL_NOERRORS );
		idToken token2;
		for( i = 0; i < 3; i++ ) {
			if ( !lex.ReadToken( &token2 ) ) {
				Error( "Couldn't read vector. '%s' is not in the form of 'x y z'", token.c_str() );
			}
			if ( token2.type == TT_PUNCTUATION && token2 == "-" ) {
				if ( !lex.CheckTokenType( TT_NUMBER, 0, &token2 ) ) {
					Error( "expected a number following '-' but found '%s' in vector '%s'", token2.c_str(), token.c_str() );
				}
				immediate.vector[ i ] = -token2.GetFloatValue();
			} else if ( token2.type == TT_NUMBER ) {
				immediate.vector[ i ] = token2.GetFloatValue();
			} else {
				Error( "vector '%s' is not in the form of 'x y z'.  expected float value, found '%s'", token.c_str(), token2.c_str() );
			}
		}
		return;
	}

	case TT_NUMBER:
		immediateType = &type_float;
		immediate._float = token.GetFloatValue();
		return;

	case TT_PUNCTUATION:
		// entity names
		if ( token == "$" ) {
			immediateType = &type_object;
			parserPtr->ReadToken( &token );
			return;
		}

		if ( token == "{" ) {
			braceDepth++;
			return;
		}

		if ( token == "}" ) {
			braceDepth--;
			return;
		}

		if ( punctuationValid[ token.subtype ] ) {
			return;
		}

		Error( "Unknown punctuation '%s'", token.c_str() );
		break;

	case TT_NAME:
		if ( token == "true" ) {
			immediateType	= &type_boolean;
			immediate._int	= 1;
			return;
		}
		if ( token == "false" ) {
			immediateType	= &type_boolean;
			immediate._int	= 0;
			return;
		}
		return;

	default:
		Error( "Unknown token '%s'", token.c_str() );
	}
}

/*
=============
idCompiler::ExpectToken

Issues an Error if the current token isn't equal to string
Gets the next token
=============
*/
void idCompiler::ExpectToken( const char *string ) {
	if ( token != string ) {
		Error( "expected '%s', found '%s'", string, token.c_str() );
	}

	NextToken();
}

/*
=============
idCompiler::CheckToken

Returns true and gets the next token if the current token equals string
Returns false and does nothing otherwise
=============
*/
bool idCompiler::CheckToken( const char *string ) {
	if ( token != string ) {
		return false;
	}
		
	NextToken();
	
	return true;
}

/*
============
idCompiler::ParseName

Checks to see if the current token is a valid name
============
*/
void idCompiler::ParseName( idStr &name ) {
	if ( token.type != TT_NAME ) {
		Error( "'%s' is valid not a name (e.g. trying to use a class name as an identifier)", token.c_str() );
	}

	name = token;
	NextToken();
}

/*
============
idCompiler::SkipOutOfFunction

For error recovery, pops out of nested braces
============
*/
void idCompiler::SkipOutOfFunction( void ) {
	while( braceDepth ) {
		parserPtr->SkipBracedSection( false );
		braceDepth--;
	}
	NextToken();
}

/*
============
idCompiler::SkipToSemicolon

For error recovery
============
*/
void idCompiler::SkipToSemicolon( void ) {
	do {
		if ( CheckToken( ";" ) ) {
			return;
		}

		NextToken();
	} while( !eof );
}

/*
============
idCompiler::CheckType

Parses a variable type, including functions types
============
*/
idTypeDef *idCompiler::CheckType( void ) {
	idTypeDef *type;
	
	if ( token == "float" ) {
		type = &type_float;
	} else if ( token == "vector" ) {
		type = &type_vector;
	} else if ( token == "entity" ) {
		type = &type_object;
	} else if ( token == "string" ) {
		type = &type_string;
	} else if ( token == "wstring" ) {
		type = &type_wstring;
	} else if ( token == "void" ) {
		type = &type_void;
	} else if ( token == "object" ) {
		type = &type_object;
	} else if ( token == "boolean" ) {
		type = &type_boolean;
	} else if ( token == "handle" ) {
		type = &type_boolean;
	} else if ( token == "namespace" ) {
		type = &type_namespace;
	} else if ( token == "scriptEvent" ) {
		type = &type_scriptevent;
	} else if ( token == "virtual" ) {
		type = &type_internalscriptevent;
	} else {
		type = program->FindType( token.c_str() );
		if ( type && !type->Inherits( &type_object ) ) {
			type = NULL;
		}
	}
	
	return type;
}

/*
============
idCompiler::ParseType

Parses a variable type, including functions types
============
*/
idTypeDef *idCompiler::ParseType( void ) {
	idTypeDef *type;
	
	type = CheckType();
	if ( !type ) {
		Error( "\"%s\" is not a type", token.c_str() );
	}

	if ( ( type == &type_scriptevent ) && ( scope != &def_namespace ) ) {
		Error( "scriptEvents can only defined in the global namespace" );
	}

	if ( ( type == &type_internalscriptevent ) && ( scope != &def_namespace ) ) {
		Error( "internalScriptEvents can only defined in the global namespace" );
	}

	if ( ( type == &type_namespace ) && ( scope->Type() != ev_namespace ) ) {
		Error( "A namespace may only be defined globally, or within another namespace" );
	}

	NextToken();
	
	return type;
}

/*
============
idCompiler::ParseImmediate

Looks for a preexisting constant
============
*/
idVarDef *idCompiler::ParseImmediate( void ) {
	idVarDef *def;

	def = GetImmediate( immediateType, &immediate, token.c_str() );
	NextToken();

	return def;
}

/*
============
idCompiler::EmitFunctionParms
============
*/
idVarDef *idCompiler::EmitFunctionParms( int op, idVarDef *func, int startarg, int startsize, idVarDef *object ) {
	idVarDef		*e;
	const idTypeDef	*type;
	const idTypeDef	*funcArg;
	idVarDef		*returnDef;
	idTypeDef		*returnType;
	int 			arg;
	int 			size;
	int				resultOp;

	type = func->TypeDef();
	if ( func->Type() != ev_function ) {
		Error( "'%s' is not a function", func->Name() );
	}

	idList< idVarDef* > args;

	// copy the parameters to the global parameter variables
	arg = startarg;
	size = startsize;
	if ( !CheckToken( ")" ) ) {
		do {
			if ( arg >= type->NumParameters() ) {
				Error( "too many parameters" );
			}

			e = GetExpression( TOP_PRIORITY );
			args.Alloc() = e;

			funcArg = type->GetParmType( arg );
			if ( !EmitPush( e, funcArg ) ) {
				idStr parmName = type->GetParmName( arg ) == NULL ? "" : va( "(%s)", type->GetParmName( arg ) );
				Error( "type mismatch on parm %i%s of call to '%s'", arg + 1, parmName.c_str(), func->Name() );
			}

			if ( funcArg->Type() == ev_object ) {
				size += type_object.Size();
			} else {
				size += funcArg->Size();
			}

			arg++;
		} while( CheckToken( "," ) );
	
		ExpectToken( ")" );
	}

	if ( arg < type->NumParameters() ) {
		Error( "too few parameters for function '%s'", func->Name() );
	}

	if ( op == OP_CALL ) {
		EmitOpcode( op, func, 0 );
	} else if ( ( op == OP_OBJECTCALL ) || ( op == OP_OBJTHREAD ) || ( op == OP_GUIOBJTHREAD ) ) {
		EmitOpcode( op, object, VirtualFunctionConstant( func ) );

		// need arg size seperate since script object may be NULL
		statement_t &statement = program->GetStatement( program->NumStatements() - 1 );
		statement.c = func;
	} else if ( op == OP_VIRTUALEVENTCALL ) {
		EmitOpcode( op, GlobalVirtualFunctionConstant( func ), NULL );

		// need arg size seperate since script object may be NULL
		statement_t &statement = program->GetStatement( program->NumStatements() - 1 );
		statement.c = func;
	} else {
		EmitOpcode( op, func, SizeConstant( size ) );
	}

	for ( int i = 0; i < args.Num(); i++ ) {
		if ( !args[ i ]->settings.isReturn ) {
			continue;
		}
		args[ i ]->numUsers++;
	}

	// we need to copy off the result into a temporary result location, so figure out the opcode
	returnType = type->ReturnType();
	if ( returnType->Type() == ev_string ) {
		resultOp = OP_STORE_S;
		returnDef = program->returnStringDef;
	} else if ( returnType->Type() == ev_wstring ) {
		resultOp = OP_STORE_W;
		returnDef = program->returnStringDef;
	} else {
		program->returnDef->SetTypeDef( returnType );
		returnDef = program->returnDef;

		switch( returnType->Type() ) {
		case ev_void :
			resultOp = OP_STORE_F;
			break;

		case ev_boolean :
			resultOp = OP_STORE_BOOL;
			break;

		case ev_float :
			resultOp = OP_STORE_F;
			break;

		case ev_vector :
			resultOp = OP_STORE_V;
			break;

		case ev_object :
			resultOp = OP_STORE_OBJ;
			break;

		default :
			Error( "Invalid return type for function '%s'", func->Name() );
			// shut up compiler
			resultOp = OP_STORE_OBJ;
			break;
		}
	}

	if ( returnType->Type() == ev_void ) {
		// don't need result space since there's no result, so just return the normal result def.
		return returnDef;
	}

	// allocate result space
	// try to reuse result defs as much as possible
	statement_t &statement = program->GetStatement( program->NumStatements() - 1 );
	idVarDef *resultDef = program->FindFreeResultDef( returnType, scope, statement.a, statement.b );
	// set user count back to 0, a result def needs to be used twice before it can be reused
	resultDef->numUsers = 0;

	EmitOpcode( resultOp, returnDef, resultDef );

	return resultDef;
}

/*
============
idCompiler::ParseFunctionCall
============
*/
idVarDef *idCompiler::ParseFunctionCall( idVarDef *funcDef ) {
	assert( funcDef );

	if ( funcDef->Type() != ev_function ) {
		Error( "'%s' is not a function", funcDef->Name() );
	}

	if ( funcDef->settings.initialized == idVarDef::uninitialized ) {
		Error( "Function '%s' has not been defined yet", funcDef->GlobalName() );
	}

	assert( funcDef->value.functionPtr );
	if ( callthread ) {
		if ( ( funcDef->settings.initialized != idVarDef::uninitialized ) && funcDef->value.functionPtr->eventdef ) {
			Error( "Built-in functions cannot be called as threads" );
		}

		int op = callguithread ? OP_GUITHREAD : OP_THREAD;

		if ( program->IsExporting() ) {
			program->scriptExporter.RegisterClassThreadCall( NULL, funcDef->value.functionPtr, callguithread );
		}

		callthread = false;
		callguithread = false;

		return EmitFunctionParms( op, funcDef, 0, 0, NULL );
	} else {
		if ( ( funcDef->settings.initialized != idVarDef::uninitialized ) ) {
			if ( funcDef->value.functionPtr->eventdef ) {
				if ( ( scope->Type() != ev_namespace ) && ( scope->scope->Type() == ev_object ) ) {
					// get the local object pointer
					idVarDef *thisdef = program->GetDef( scope->scope->TypeDef(), "self", scope );
					if ( !thisdef ) {
						Error( "No 'self' within scope" );
					}

					return ParseEventCall( thisdef, funcDef );
				} else {
					Error( "Built-in functions cannot be called without an object" );
				}
			} else if ( funcDef->value.functionPtr->virtualIndex != -1 ) {
				if ( ( scope->Type() != ev_namespace ) && ( scope->scope->Type() == ev_object ) ) {
					// get the local object pointer
					idVarDef *thisdef = program->GetDef( scope->scope->TypeDef(), "self", scope );
					if ( !thisdef ) {
						Error( "No 'self' within scope" );
					}

					return ParseVirtualEventCall( thisdef, funcDef );
				} else {
					Error( "Virtual functions cannot be called without an object" );
				}
			}
		}

		return EmitFunctionParms( OP_CALL, funcDef, 0, 0, NULL );
	}
}

/*
============
idCompiler::ParseObjectCall
============
*/
idVarDef *idCompiler::ParseObjectCall( idVarDef *object, idVarDef *func ) {
	EmitPush( object, object->TypeDef() );

	int op = OP_OBJECTCALL;
	if ( callthread ) {
		op = callguithread ? OP_GUIOBJTHREAD : OP_OBJTHREAD;

		if ( program->IsExporting() ) {
			program->scriptExporter.RegisterClassThreadCall( object->TypeDef(), func->value.functionPtr, callguithread );
		}

		callthread = false;
		callguithread = false;
	}

	return EmitFunctionParms( op, func, 1, type_object.Size(), object );
}

/*
============
idCompiler::ParseVirtualEventCall
============
*/
idVarDef *idCompiler::ParseVirtualEventCall( idVarDef *object, idVarDef *funcDef ) {
	if ( callthread ) {
		Error( "Cannot call built-in functions as a thread" );
	}

	if ( funcDef->Type() != ev_function ) {
		Error( "'%s' is not a function", funcDef->Name() );
	}

	if ( funcDef->value.functionPtr->virtualIndex == -1 ) {
		Error( "\"%s\" is not a virtual function", funcDef->Name() );
	}

	if ( object->Type() == ev_object ) {
		EmitPush( object, &type_object );
	} else {
		EmitPush( object, object->TypeDef() );
	}
	return EmitFunctionParms( OP_VIRTUALEVENTCALL, funcDef, 0, type_object.Size(), NULL );
}

/*
============
idCompiler::ParseEventCall
============
*/
idVarDef *idCompiler::ParseEventCall( idVarDef *object, idVarDef *funcDef ) {
	if ( callthread ) {
		Error( "Cannot call built-in functions as a thread" );
	}

	if ( funcDef->Type() != ev_function ) {
		Error( "'%s' is not a function", funcDef->Name() );
	}

	if ( !funcDef->value.functionPtr->eventdef ) {
		Error( "\"%s\" cannot be called with object notation", funcDef->Name() );
	}

	if ( object->Type() == ev_object ) {
		EmitPush( object, &type_object );
	} else {
		EmitPush( object, object->TypeDef() );
	}

	return EmitFunctionParms( OP_EVENTCALL, funcDef, 0, type_object.Size(), NULL );
}

/*
============
idCompiler::ParseSysObjectCall
============
*/
idVarDef *idCompiler::ParseSysObjectCall( idVarDef *funcDef ) {
	if ( callthread ) {
		Error( "Cannot call built-in functions as a thread" );
	}

	if ( funcDef->Type() != ev_function ) {
		Error( "'%s' is not a function", funcDef->Name() );
	}

	if ( !funcDef->value.functionPtr->eventdef ) {
		Error( "\"%s\" cannot be called with object notation", funcDef->Name() );
	}

	if ( !idThread::Type.RespondsTo( *funcDef->value.functionPtr->eventdef ) ) {
		Error( "\"%s\" is not callable as a 'sys' function", funcDef->Name() );
	}

	return EmitFunctionParms( OP_SYSCALL, funcDef, 0, 0, NULL );
}

/*
============
idCompiler::LookupDef
============
*/
idVarDef *idCompiler::LookupDef( const char *name, const idVarDef *baseobj ) {
	idVarDef	*def;
	idVarDef	*field;
	etype_t		type_b;
	etype_t		type_c;
	opcode_t	*op;

	// check if we're accessing a field
	if ( baseobj && ( baseobj->Type() == ev_object ) ) {
		const idVarDef *tdef;

		def = NULL;
		for( tdef = baseobj; tdef != &def_object; tdef = tdef->TypeDef()->SuperClass()->def ) {
			def = program->GetDef( NULL, name, tdef );
			if ( def ) {
				break;
			}
		}
	} else {
		// first look through the defs in our scope
		def = program->GetDef( NULL, name, scope );
		if ( !def ) {
			// if we're in a member function, check types local to the object
			if ( ( scope->Type() != ev_namespace ) && ( scope->scope->Type() == ev_object ) ) {
				// get the local object pointer
				idVarDef *thisdef = program->GetDef( scope->scope->TypeDef(), "self", scope );

				field = LookupDef( name, scope->scope->TypeDef()->def );
				if ( !field ) {
					Error( "Unknown value \"%s\"", name );
				}

				// type check
				type_b = field->Type();
				if ( field->Type() == ev_function ) {
					type_c = field->TypeDef()->ReturnType()->Type();
				} else {
					type_c = field->TypeDef()->FieldType()->Type();	// field access gets type from field
	                if ( CheckToken( "++" ) ) {
						if ( type_c != ev_float ) {
							Error( "Invalid type for ++" );
						}
						def = EmitOpcode( OP_UINCP_F, thisdef, field );
						return def;
					} else if ( CheckToken( "--" ) ) {
						if ( type_c != ev_float ) {
							Error( "Invalid type for --" );
						}
						def = EmitOpcode( OP_UDECP_F, thisdef, field );
						return def;
					}
				}

				op = &opcodes[ OP_INDIRECT_F ];
				while( ( op->type_a->Type() != ev_object ) 
					|| ( type_b != op->type_b->Type() ) || ( type_c != op->type_c->Type() ) ) {
					if ( ( op->priority == FUNCTION_PRIORITY ) && ( op->type_a->Type() == ev_object ) && ( op->type_c->Type() == ev_void ) && 
						( type_c != op->type_c->Type() ) ) {
						// catches object calls that return a value
						break;
					}
					op++;
					if ( !op->name || idStr::Cmp( op->name, "." ) ) {
						Error( "no valid opcode to access type '%s'", field->TypeDef()->SuperClass()->Name() );
					}
				}

				if ( ( op - opcodes ) == OP_EVENTCALL ) {
					ExpectToken( "(" );
					if ( ( field->settings.initialized != idVarDef::uninitialized ) ) {
						if ( field->value.functionPtr->eventdef ) {
							def = ParseEventCall( thisdef, field );
						} else if ( field->value.functionPtr->virtualIndex != -1 ) {
							def = ParseVirtualEventCall( thisdef, field );
						} else {
							def = ParseObjectCall( thisdef, field );
						}
					} else {
						def = ParseObjectCall( thisdef, field );
					}
				} else {
					// emit the conversion opcode
					def = EmitOpcode( op, thisdef, field );

					// field access gets type from field
					// Gordon: this is fugly, have made EmitOpcode just output a result of the right type
//					def->SetTypeDef( field->TypeDef()->FieldType() );
				}
			}
		}
	}

	return def;
}

/*
============
idCompiler::ParseValue

Returns the def for the current token
============
*/
idVarDef *idCompiler::ParseValue( void ) {
	idVarDef	*def;
	idVarDef	*namespaceDef;
	idStr		name;
	
	if ( immediateType == &type_object ) {
		if ( token.Cmp( "null_entity" ) && token.Cmp( "null" ) ) {
			Error( "$ referenced entities are deprecated: %s", token.c_str() );
		}

		// if an immediate entity ($-prefaced name) then create or lookup a def for it.
		// when entities are spawned, they'll lookup the def and point it to them.
		def = program->GetDef( &type_object, "$" + token, &def_namespace );
		if ( !def ) {
			def = program->AllocDef( &type_object, "$" + token, &def_namespace );
			if ( program->IsExporting() ) {
				program->scriptExporter.AllocGlobal( def );
			}
		}
		NextToken();
		return def;
	} else if ( immediateType ) {
		// if the token is an immediate, allocate a constant for it
		return ParseImmediate();
	}

	ParseName( name );
	def = LookupDef( name, basetype );
	if ( !def ) {
		if ( basetype && basetype->TypeDef()->Inherits( &type_object ) ) {
			def = LookupDef( name, NULL );
		}
		if ( !def ) {
			if ( basetype ) {
				Error( "%s is not a member of %s", name.c_str(), basetype->TypeDef()->Name() );
			} else {
				Error( "Unknown value \"%s\"", name.c_str() );
			}
		}
	// if namespace, then look up the variable in that namespace
	} else if ( def->Type() == ev_namespace ) {
		while( def->Type() == ev_namespace ) {
			ExpectToken( "::" );
			ParseName( name );
			namespaceDef = def;
			def = program->GetDef( NULL, name, namespaceDef );
			if ( !def ) {
				Error( "Unknown value \"%s::%s\"", namespaceDef->GlobalName(), name.c_str() );
			}
		}
		//def = LookupDef( name, basetype );
	}

	return def;
}

/*
============
idCompiler::GetTerm
============
*/
idVarDef *idCompiler::GetTerm( void ) {
	idVarDef	*e;
	int 		op;
	
	if ( !immediateType && CheckToken( "~" ) ) {
		e = GetExpression( TILDE_PRIORITY );
		switch( e->Type() ) {
		case ev_float :
			op = OP_COMP_F;
			break;

		default :
			Error( "type mismatch for ~" );

			// shut up compiler
			op = OP_COMP_F;
			break;
		}

		return EmitOpcode( op, e, 0 );
	}

	if ( !immediateType && CheckToken( "!" ) ) {
		e = GetExpression( NOT_PRIORITY );
		switch( e->Type() ) {
		case ev_boolean :
			op = OP_NOT_BOOL;
			break;

		case ev_float :
			op = OP_NOT_F;
			break;

		case ev_string :
			op = OP_NOT_S;
			break;

		case ev_vector :
			op = OP_NOT_V;
			break;

		case ev_function :
			Error( "Invalid type for !" );

			// shut up compiler
			op = OP_NOT_F;
			break;

		case ev_object:
			op = OP_NOT_OBJ;
			break;

		default :
			Error( "type mismatch for !" );

			// shut up compiler
			op = OP_NOT_F;
			break;
		}

		return EmitOpcode( op, e, 0 );
	}

	// check for negation operator
	if ( !immediateType && CheckToken( "-" ) ) {
		// constants are directly negated without an instruction
		if ( immediateType == &type_float ) {
			immediate._float = -immediate._float;
			return ParseImmediate();
		} else if ( immediateType == &type_vector ) {
			immediate.vector[0] = -immediate.vector[0];
			immediate.vector[1] = -immediate.vector[1];
			immediate.vector[2] = -immediate.vector[2];
			return ParseImmediate();
		} else {
			e = GetExpression( NOT_PRIORITY );
			switch( e->Type() ) {
			case ev_float :
				op = OP_NEG_F;
				break;

			case ev_vector :
				op = OP_NEG_V;
				break;
			default :
				Error( "type mismatch for -" );

				// shut up compiler
				op = OP_NEG_F;
				break;
			}
			return EmitOpcode( &opcodes[ op ], e, 0 );
		}
	}
	
	if ( CheckToken( "new" ) ) {
		idTypeDef* newType = ParseType();

		if ( !newType->Inherits( &type_object ) ) {
			Error( "may only new object types" );
		}

		return EmitOpcode( OP_ALLOC_TYPE, newType->def, NULL );
	}

	if ( CheckToken( "delete" ) ) {
		idVarDef* var = GetTerm();

		if ( !var->TypeDef()->Inherits( &type_object ) ) {
			Error( "may only delete object types" );
		}

		EmitOpcode( OP_FREE_TYPE, var, NULL );
		return NULL;
	}

	if ( CheckToken( "int" ) ) {
		ExpectToken( "(" );

		e = GetExpression( INT_PRIORITY );
		if ( e->Type() != ev_float ) {
			Error( "type mismatch for int()" );
		}

		ExpectToken( ")" );

		return EmitOpcode( OP_INT_F, e, 0 );
	}
	
	if ( CheckToken( "thread" ) ) {
		callthread = true;
		callguithread = false;
		e = GetExpression( FUNCTION_PRIORITY );

		if ( callthread ) {
			Error( "Invalid thread call" );
		}

		// threads return the thread number
		program->returnDef->SetTypeDef( &type_float );
		return program->returnDef;
	}
	
	if ( CheckToken( "guiThread" ) ) {
		callthread = true;
		callguithread = true;
		e = GetExpression( FUNCTION_PRIORITY );

		if ( callthread ) {
			Error( "Invalid thread call" );
		}

		// threads return the thread number
		program->returnDef->SetTypeDef( &type_float );
		return program->returnDef;
	}

	if ( !immediateType && CheckToken( "(" ) ) {
		e = GetExpression( TOP_PRIORITY );
		ExpectToken( ")" );

		return e;
	}
	
	return ParseValue();
}

/*
==============
idCompiler::TypeMatches
==============
*/
bool idCompiler::TypeMatches( etype_t type1, etype_t type2 ) const {
	return type1 == type2;
}

/*
==============
idCompiler::GetExpression
==============
*/
idVarDef *idCompiler::GetExpression( int priority ) {
	opcode_t		*op;
	opcode_t		*oldop;
	idVarDef		*e;
	idVarDef		*e2;
	const idVarDef	*oldtype;
	etype_t 		type_a;
	etype_t 		type_b;
	etype_t 		type_c;
	
	if ( priority == 0 ) {
		return GetTerm();
	}
		
	e = GetExpression( priority - 1 );
	idTypeDef* eType = e ? e->TypeDef() : NULL;
	if ( token == ";" ) {
		// save us from searching through the opcodes unneccesarily
		return e;
	}

	while( 1 ) {
		if ( ( priority == FUNCTION_PRIORITY ) && CheckToken( "(" ) ) {
			return ParseFunctionCall( e );
		}

		// has to be a punctuation
		if ( immediateType ) {
			break;
		}

		for( op = opcodes; op->name; op++ ) {
			if ( ( op->priority == priority ) && CheckToken( op->name ) ) {
				break;
			}
		}

		if ( !op->name ) {
			// next token isn't at this priority level
			break;
		}

		// unary operators act only on the left operand
		if ( op->type_b == &def_void ) {
			e = EmitOpcode( op, e, 0 );
			return e;
		}

		// preserve our base type
		oldtype = basetype;

		// field access needs scope from object
		if ( ( op->name[ 0 ] == '.' ) && e->TypeDef()->Inherits( &type_object ) ) {
			// save off what type this field is part of
			basetype = e->TypeDef()->def;
		}

		if ( op->rightAssociative ) {
			// if last statement is an indirect, change it to an address of
			if ( program->NumStatements() > 0 ) {
				statement_t &statement = program->GetStatement( program->NumStatements() - 1 );
				if ( ( statement.op >= OP_INDIRECT_F ) && ( statement.op < OP_ADDRESS ) ) {
					statement.op = OP_ADDRESS;

					// Gordon: allocate a pointer of the correct type rather than abusing the single pointer type
					idTypeDef temp = type_pointer;
					temp.SetPointerType( e->TypeDef() );

					idTypeDef* pointerType = program->GetType( temp, true );

					e->numUsers = 2; // Gordon: since we aren't actually using this one here, let it be used again

					// Gordon: This actually allocates a new var, rather than overwriting the type on the old expression, which is icky
					e = program->FindFreeResultDef( pointerType, scope, statement.a, statement.b );
					e->numUsers = 1;
					statement.c = e;
				}
			}

			e2 = GetExpression( priority );
		} else {
			e2 = GetExpression( priority - 1 );
		}

		// restore type
		basetype = oldtype;
			
		// type check
		type_a = e->Type();
		type_b = e2->Type();

		// field access gets type from field
		if ( op->name[ 0 ] == '.' ) {
			if ( ( e2->Type() == ev_function ) && e2->TypeDef()->ReturnType() ) {
				type_c = e2->TypeDef()->ReturnType()->Type();
			} else if ( e2->TypeDef()->FieldType() ) {
				type_c = e2->TypeDef()->FieldType()->Type();
			} else {
				// not a field
				type_c = ev_error;
			}
		} else {
			type_c = ev_void;
		}

		oldop = op;
		while(	!TypeMatches( type_a, op->type_a->Type() ) || 
				!TypeMatches( type_b, op->type_b->Type() ) ||
				( ( type_c != ev_void ) && !TypeMatches( type_c, op->type_c->Type() ) ) 
				) {

			if ( ( op->priority == FUNCTION_PRIORITY ) && TypeMatches( type_a, op->type_a->Type() ) && TypeMatches( type_b, op->type_b->Type() ) ) {
				break;
			}

			op++;
			if ( !op->name || idStr::Cmp( op->name, oldop->name ) ) {
				Error( "type mismatch for '%s'", oldop->name );
			}
		}

		switch( op - opcodes ) {
		case OP_SYSCALL :
			ExpectToken( "(" );
			e = ParseSysObjectCall( e2 );
			break;

		case OP_OBJECTCALL :
			ExpectToken( "(" );
			if ( ( e2->settings.initialized != idVarDef::uninitialized ) && e2->value.functionPtr->eventdef ) {
				e = ParseEventCall( e, e2 );
			} else {
				e = ParseObjectCall( e, e2 );
			}
			break;

		case OP_EVENTCALL :		
			ExpectToken( "(" );
			if ( ( e2->settings.initialized != idVarDef::uninitialized ) ) {
				if ( e2->value.functionPtr->eventdef ) {
					e = ParseEventCall( e, e2 );
				} else if ( e2->value.functionPtr->virtualIndex != -1 ) {
					e = ParseVirtualEventCall( e, e2 );
				} else {
					e = ParseObjectCall( e, e2 );
				}
			} else {
				e = ParseObjectCall( e, e2 );
			}
			break;

		default:
			if ( callthread ) {
				Error( "Expecting function call after 'thread'" );
			}

			if ( ( type_a == ev_pointer ) && ( type_b != e->TypeDef()->PointerType()->Type() ) ) {
				// FIXME: need to make a general case for this
				if ( ( op - opcodes == OP_STOREP_F ) && ( e->TypeDef()->PointerType()->Type() == ev_boolean ) ) {
					// copy from float to boolean pointer
					op = &opcodes[ OP_STOREP_FTOBOOL ];
				} else if ( ( op - opcodes == OP_STOREP_BOOL ) && ( e->TypeDef()->PointerType()->Type() == ev_float ) ) {
					// copy from boolean to float pointer
					op = &opcodes[ OP_STOREP_BOOLTOF ];
				} else if ( ( op - opcodes == OP_STOREP_F ) && ( e->TypeDef()->PointerType()->Type() == ev_string ) ) {
					// copy from float to string pointer
					op = &opcodes[ OP_STOREP_FTOS ];
				} else if ( ( op - opcodes == OP_STOREP_BOOL ) && ( e->TypeDef()->PointerType()->Type() == ev_string ) ) {
					// copy from boolean to string pointer
					op = &opcodes[ OP_STOREP_BTOS ];
				} else if ( ( op - opcodes == OP_STOREP_V ) && ( e->TypeDef()->PointerType()->Type() == ev_string ) ) {
					// copy from vector to string pointer
					op = &opcodes[ OP_STOREP_VTOS ];
				} else {
					Error( "type mismatch for '%s'", op->name );
				}
			}

			if ( op->rightAssociative ) {
				e = EmitOpcode( op, e2, e );
			} else {
				e = EmitOpcode( op, e, e2 );
			}

			if ( op - opcodes == OP_STOREP_OBJ || op - opcodes == OP_STORE_OBJ ) {
				// statement.b points to type_pointer, which is just a temporary that gets its type reassigned, so we store the real type in statement.c
				// so that we can do a type check during run time since we don't know what type the script object is at compile time because it
				// comes from an entity
				statement_t &statement = program->GetStatement( program->NumStatements() - 1 );
				statement.c = eType->def;
			}
			
			// field access gets type from field
			if ( type_c != ev_void ) {
				e->SetTypeDef( e2->TypeDef()->FieldType() );
			}
			break;
		}
	}

	return e;
}

/*
================
idCompiler::PatchLoop
================
*/
void idCompiler::PatchLoop( int start, int continuePos ) {
	int			i;
	statement_t	*pos;

	pos = &program->GetStatement( start );
	for( i = start; i < program->NumStatements(); i++, pos++ ) {
		if ( pos->op == OP_BREAK ) {
			pos->op = OP_GOTO;
			pos->a = JumpFrom( i );
		} else if ( pos->op == OP_CONTINUE ) {
			pos->op = OP_GOTO;
			pos->a = JumpDef( i, continuePos );
		}
	}
}

/*
================
idCompiler::ParseReturnStatement
================
*/
void idCompiler::ParseReturnStatement( void ) {
	idVarDef	*e;
	etype_t 	type_a;
	etype_t 	type_b;
	opcode_t	*op;

	if ( CheckToken( ";" ) ) {
		if ( scope->TypeDef()->ReturnType()->Type() != ev_void ) {
			Error( "expecting return value" );
		}

		EmitOpcode( OP_RETURN, 0, 0 );
		return;
	}

	e = GetExpression( TOP_PRIORITY );
	ExpectToken( ";" );

	type_a = e->Type();
	type_b = scope->TypeDef()->ReturnType()->Type();

	if ( TypeMatches( type_a, type_b ) ) {
		EmitOpcode( OP_RETURN, e, 0 );
		return;
	}

	for( op = opcodes; op->name; op++ ) {
		if ( !idStr::Cmp( op->name, "=" ) ) {
			break;
		}
	}

	assert( op->name );

	while ( !TypeMatches( type_a, op->type_b->Type() ) || !TypeMatches( type_b, op->type_a->Type() ) ) {
		op++;
		if ( !op->name || idStr::Cmp( op->name, "=" ) ) {
			Error( "type mismatch for return value" );
		}
	}

	idTypeDef *returnType = scope->TypeDef()->ReturnType();
	if ( returnType->Type() == ev_string || returnType->Type() == ev_wstring ) {
		EmitOpcode( op, e, program->returnStringDef );
		EmitOpcode( OP_RETURN, program->returnStringDef, 0 );
	} else {
		program->returnDef->SetTypeDef( returnType );
		EmitOpcode( op, e, program->returnDef );
		EmitOpcode( OP_RETURN, program->returnDef, 0 );
	}
}
	
/*
================
idCompiler::ParseWhileStatement
================
*/
void idCompiler::ParseWhileStatement( void ) {
	idVarDef	*e;
	int			patch1;
	int			patch2;

	loopDepth++;

	ExpectToken( "(" );
	
	patch2 = program->NumStatements();
	e = GetExpression( TOP_PRIORITY );
	ExpectToken( ")" );

	if ( ( e->settings.initialized == idVarDef::initializedConstant ) && ( *e->value.intPtr != 0 ) ) {
		//FIXME: we can completely skip generation of this code in the opposite case
		ParseStatement();
		EmitOpcode( OP_GOTO, JumpTo( patch2 ), 0 );
	} else {
		patch1 = program->NumStatements();
        EmitOpcode( OP_IFNOT, e, 0 );
		ParseStatement();
		EmitOpcode( OP_GOTO, JumpTo( patch2 ), 0 );
		program->GetStatement( patch1 ).b = JumpFrom( patch1 );
	}

	// fixup breaks and continues
	PatchLoop( patch2, patch2 );

	loopDepth--;
}

/*
================
idCompiler::ParseForStatement

Form of for statement with a counter:

	a = 0;
start:					<< patch4
	if ( !( a < 10 ) ) {
		goto end;		<< patch1
	} else {
		goto process;	<< patch3
	}

increment:				<< patch2
	a = a + 1;
	goto start;			<< goto patch4

process:
	statements;
	goto increment;		<< goto patch2

end:

Form of for statement without a counter:

	a = 0;
start:					<< patch2
	if ( !( a < 10 ) ) {
		goto end;		<< patch1
	}

process:
	statements;
	goto start;			<< goto patch2

end:
================
*/
void idCompiler::ParseForStatement( void ) {
	idVarDef	*e;
	int			start;
	int			patch1;
	int			patch2;
	int			patch3;
	int			patch4;

	loopDepth++;

	start = program->NumStatements();

	ExpectToken( "(" );
	
	// init
	if ( !CheckToken( ";" ) ) {
		do {
			GetExpression( TOP_PRIORITY );
		} while( CheckToken( "," ) );

		ExpectToken( ";" );
	}

	// condition
	patch2 = program->NumStatements();

	e = GetExpression( TOP_PRIORITY );
	ExpectToken( ";" );

	//FIXME: add check for constant expression
	patch1 = program->NumStatements();
	EmitOpcode( OP_IFNOT, e, 0 );

	// counter
	if ( !CheckToken( ")" ) ) {
		patch3 = program->NumStatements();
		EmitOpcode( OP_IF, e, 0 );

		patch4 = patch2;
		patch2 = program->NumStatements();
		do {
			GetExpression( TOP_PRIORITY );
		} while( CheckToken( "," ) );
		
		ExpectToken( ")" );

		// goto patch4
		EmitOpcode( OP_GOTO, JumpTo( patch4 ), 0 );

		// fixup patch3
		program->GetStatement( patch3 ).b = JumpFrom( patch3 );
	}

	ParseStatement();

	// goto patch2
	EmitOpcode( OP_GOTO, JumpTo( patch2 ), 0 );

	// fixup patch1
	program->GetStatement( patch1 ).b = JumpFrom( patch1 );

	// fixup breaks and continues
	PatchLoop( start, patch2 );

	loopDepth--;
}

/*
================
idCompiler::ParseDoWhileStatement
================
*/
void idCompiler::ParseDoWhileStatement( void ) {
	idVarDef	*e;
	int			patch1;

	loopDepth++;

	patch1 = program->NumStatements();
	ParseStatement();
	ExpectToken( "while" );
	ExpectToken( "(" );
	e = GetExpression( TOP_PRIORITY );
	ExpectToken( ")" );
	ExpectToken( ";" );

	EmitOpcode( OP_IF, e, JumpTo( patch1 ) );

	// fixup breaks and continues
	PatchLoop( patch1, patch1 );

	loopDepth--;
}

/*
================
idCompiler::ParseIfStatement
================
*/
void idCompiler::ParseIfStatement( void ) {
	idVarDef	*e;
	int			patch1;
	int			patch2;

	ExpectToken( "(" );
	e = GetExpression( TOP_PRIORITY );
	ExpectToken( ")" );

	//FIXME: add check for constant expression
	patch1 = program->NumStatements();
	EmitOpcode( OP_IFNOT, e, 0 );

	ParseStatement();
	
	if ( CheckToken( "else" ) ) {
		patch2 = program->NumStatements();
		EmitOpcode( OP_GOTO, 0, 0 );
		program->GetStatement( patch1 ).b = JumpFrom( patch1 );
		ParseStatement();
		program->GetStatement( patch2 ).a = JumpFrom( patch2 );
	} else {
		program->GetStatement( patch1 ).b = JumpFrom( patch1 );
	}
}

/*
============
idCompiler::ParseStatement
============
*/
void idCompiler::ParseStatement( void ) {
	if ( CheckToken( ";" ) ) {
		// skip semicolons, which are harmless and ok syntax
		return;
	}

	lastStatementWasReturn = false;

	if ( CheckToken( "{" ) ) {
		while( !CheckToken( "}" ) ) {
			ParseStatement();
		}

		lastStatementWasReturn = false;

		return;
	} 

	if ( CheckToken( "return" ) ) {
		ParseReturnStatement();
		lastStatementWasReturn = true;
		return;
	}
	
	if ( CheckToken( "while" ) ) {
		ParseWhileStatement();
		return;
	}

	if ( CheckToken( "for" ) ) {
		ParseForStatement();
		return;
	}

	if ( CheckToken( "do" ) ) {
		ParseDoWhileStatement();
		return;
	}

	if ( CheckToken( "break" ) ) {
		ExpectToken( ";" );
		if ( !loopDepth ) {
			Error( "cannot break outside of a loop" );
		}
		EmitOpcode( OP_BREAK, 0, 0 );
		return;
	}

	if ( CheckToken( "continue" ) ) {
		ExpectToken( ";" );
		if ( !loopDepth ) {
			Error( "cannot contine outside of a loop" );
		}
		EmitOpcode( OP_CONTINUE, 0, 0 );
		return;
	}

	if ( CheckType() != NULL ) {
		ParseDefs();
		return;
	}

	if ( CheckToken( "if" ) ) {
		ParseIfStatement();
		return;
	}

	GetExpression( TOP_PRIORITY );
	ExpectToken(";");
}

/*
================
idCompiler::ParseObjectDef
================
*/
void idCompiler::ParseObjectDef( const char *objname ) {
	idTypeDef	newtype( ev_field, NULL, "", 0, NULL );

	idVarDef* oldscope = scope;
	if ( scope->Type() != ev_namespace ) {
		Error( "Objects cannot be defined within functions or other objects" );
	}

	idTypeDef* originalType = program->FindType( objname );
	if ( !originalType ) {
		idTypeDef* parentType;

		// base type
		if ( !CheckToken( ":" ) ) {
			parentType = &type_object;
		} else {
			parentType = ParseType();
			if ( !parentType->Inherits( &type_object ) ) {
				Error( "Objects may only inherit from objects." );
			}
		}
		
		originalType		= program->AllocType( ev_object, NULL, objname, parentType == &type_object ? 0 : parentType->Size(), parentType );
		originalType->def	= program->AllocDef( originalType, objname, scope );
		
		if ( program->IsExporting() ) {
			program->scriptExporter.RegisterClass( originalType, parentType );
		}

		scope				= originalType->def;

		// inherit all the functions
		for( int i = 0; i < parentType->NumFunctions(); i++ ) {
			originalType->AddFunction( parentType->GetFunction( i ), *program );
		}
	} else {
		scope = originalType->def;
	}

	if ( CheckToken( "{" ) ) {
		while( !CheckToken( "}" ) ) {

			if ( CheckToken( ";" ) ) {
				// skip semicolons, which are harmless and ok syntax
				continue;
			}

			idTypeDef* fieldtype = ParseType();
			newtype.SetFieldType( fieldtype );

			newtype.SetName( program, va( "%s field", fieldtype->Name() ) );

			idStr name;
			ParseName( name );

			// check for a function prototype or declaraction
			if ( CheckToken( "(" ) ) {
				ParseFunctionDef( newtype.FieldType(), name );
			} else {
				idTypeDef* type = program->GetType( newtype, true );
				assert( !type->def );
				idVarDef* var = program->AllocDef( type, name, scope );

				if ( program->IsExporting() ) {
					program->scriptExporter.RegisterClassField( originalType, var );
				}

				originalType->AddField( type, name );
				ExpectToken( ";" );
			}
		}
	}

	scope = oldscope;
}

/*
============
idCompiler::ParseFunction

parse a function type
============
*/
idTypeDef *idCompiler::ParseFunction( idTypeDef *returnType, const char *name ) {
	idTypeDef	newtype( ev_function, NULL, name, type_function.Size(), returnType );
	idTypeDef	*type;
	
	if ( scope->Type() != ev_namespace ) {
		// create self pointer
		newtype.AddFunctionParm( scope->TypeDef(), "self" );
	}

	if ( !CheckToken( ")" ) ) {
		idStr parmName;
		do {
			type = ParseType();
			ParseName( parmName );
			newtype.AddFunctionParm( type, parmName );
		} while( CheckToken( "," ) );

		ExpectToken( ")" );
	}

	return program->GetType( newtype, true );
}

/*
================
idCompiler::ParseFunctionDef
================
*/
void idCompiler::ParseFunctionDef( idTypeDef *returnType, const char *name ) {
	idTypeDef		*type;
	idVarDef		*def;
	const idVarDef	*parm;
	idVarDef		*oldscope;
	int 			i;
	int 			numParms;
	const idTypeDef	*parmType;
	function_t		*func;
	statement_t		*pos;

	if ( ( scope->Type() != ev_namespace ) && !scope->TypeDef()->Inherits( &type_object ) ) {
		Error( "Functions may not be defined within other functions" );
	}

	type = ParseFunction( returnType, name );
	def = program->GetDef( type, name, scope );
	if ( !def ) {
		def = program->AllocDef( type, name, scope );
		type->def = def;

		func = &program->AllocFunction( def );
		if ( scope->TypeDef()->Inherits( &type_object ) ) {
			scope->TypeDef()->AddFunction( func, *program );
		}

		// calculate stack space used by parms
		numParms = type->NumParameters();
		func->parmSize.SetNum( numParms );
		for( i = 0; i < numParms; i++ ) {
			parmType = type->GetParmType( i );
			if ( parmType->Inherits( &type_object ) ) {
				func->parmSize[ i ] = type_object.Size();
			} else {
				func->parmSize[ i ] = parmType->Size();
			}
			func->parmTotal += func->parmSize[ i ];
		}

		// define the parms
		for( i = 0; i < numParms; i++ ) {
			if ( program->GetDef( type->GetParmType( i ), type->GetParmName( i ), def ) ) {
				Error( "'%s' defined more than once in function parameters", type->GetParmName( i ) );
			}
			parm = program->AllocDef( type->GetParmType( i ), type->GetParmName( i ), def );
		}
	} else {
		func = def->value.functionPtr;
		assert( func );
		if ( func->firstStatement ) {
			Error( "%s redeclared", def->GlobalName() );
		}
	}

	// check if this is a prototype or declaration
	if ( !CheckToken( "{" ) ) {
		// it's just a prototype, so get the ; and move on
		ExpectToken( ";" );
		return;
	}

	if ( program->IsExporting() ) {
		program->scriptExporter.RegisterClassFunction( scope->TypeDef()->Inherits( &type_object ) ? scope->TypeDef() : NULL, func );
	}

	oldscope = scope;
	scope = def;

	func->firstStatement = program->NumStatements();

	if ( !program->IsExporting() ) {
		// check if we should call the super class constructor
		if ( oldscope->TypeDef()->Inherits( &type_object ) && !idStr::Icmp( name, "init" ) ) {
			idTypeDef *superClass;
			function_t *constructorFunc = NULL;

			// find the superclass constructor
			for( superClass = oldscope->TypeDef()->SuperClass(); superClass != &type_object; superClass = superClass->SuperClass() ) {
				constructorFunc = program->FindFunctionInternal( va( "%s::init", superClass->Name() ) );
				if ( constructorFunc ) {
					break;
				}
			}

			// emit the call to the constructor
			if ( constructorFunc ) {
				idVarDef *selfDef = program->GetDef( type->GetParmType( 0 ), type->GetParmName( 0 ), def );
				assert( selfDef );
				EmitPush( selfDef, selfDef->TypeDef() );
				EmitOpcode( &opcodes[ OP_CALL ], constructorFunc->def, 0 );
			}
		}

		// check if we should call the super class constructor
		if ( oldscope->TypeDef()->Inherits( &type_object ) && !idStr::Icmp( name, "preinit" ) ) {
			idTypeDef *superClass;
			function_t *constructorFunc = NULL;

			// find the superclass constructor
			for( superClass = oldscope->TypeDef()->SuperClass(); superClass != &type_object; superClass = superClass->SuperClass() ) {
				constructorFunc = program->FindFunctionInternal( va( "%s::preinit", superClass->Name() ) );
				if ( constructorFunc ) {
					break;
				}
			}

			// emit the call to the constructor
			if ( constructorFunc ) {
				idVarDef *selfDef = program->GetDef( type->GetParmType( 0 ), type->GetParmName( 0 ), def );
				assert( selfDef );
				EmitPush( selfDef, selfDef->TypeDef() );
				EmitOpcode( &opcodes[ OP_CALL ], constructorFunc->def, 0 );
			}
		}

		// check if we should call the super class constructor
		if ( oldscope->TypeDef()->Inherits( &type_object ) && !idStr::Icmp( name, "syncFields" ) ) {
			idTypeDef *superClass;
			function_t *constructorFunc = NULL;

			// find the superclass constructor
			for( superClass = oldscope->TypeDef()->SuperClass(); superClass != &type_object; superClass = superClass->SuperClass() ) {
				constructorFunc = program->FindFunctionInternal( va( "%s::syncFields", superClass->Name() ) );
				if ( constructorFunc ) {
					break;
				}
			}

			// emit the call to the constructor
			if ( constructorFunc ) {
				idVarDef *selfDef = program->GetDef( type->GetParmType( 0 ), type->GetParmName( 0 ), def );
				assert( selfDef );
				EmitPush( selfDef, selfDef->TypeDef() );
				EmitOpcode( &opcodes[ OP_CALL ], constructorFunc->def, 0 );
			}
		}
	}

	lastStatementWasReturn = false;

	// parse regular statements
	while( !CheckToken( "}" ) ) {
		ParseStatement();
	}

	if ( !program->IsExporting() ) {
		// check if we should call the super class destructor
		if ( oldscope->TypeDef()->Inherits( &type_object ) && !idStr::Icmp( name, "destroy" ) ) {
			idTypeDef *superClass;
			function_t *destructorFunc = NULL;

			// find the superclass destructor
			for( superClass = oldscope->TypeDef()->SuperClass(); superClass != &type_object; superClass = superClass->SuperClass() ) {
				destructorFunc = program->FindFunctionInternal( va( "%s::destroy", superClass->Name() ) );
				if ( destructorFunc ) {
					break;
				}
			}

			if ( destructorFunc ) {
				if ( func->firstStatement < program->NumStatements() ) {
					// change all returns to point to the call to the destructor
					pos = &program->GetStatement( func->firstStatement );
					for( i = func->firstStatement; i < program->NumStatements(); i++, pos++ ) {
						if ( pos->op == OP_RETURN ) {
							pos->op = OP_GOTO;
							pos->a = JumpDef( i, program->NumStatements() );
						}
					}
				}

				// emit the call to the destructor
				idVarDef *selfDef = program->GetDef( type->GetParmType( 0 ), type->GetParmName( 0 ), def );
				assert( selfDef );
				EmitPush( selfDef, selfDef->TypeDef() );
				EmitOpcode( &opcodes[ OP_CALL ], destructorFunc->def, 0 );
			}
		}
	}

	if ( !lastStatementWasReturn ) {
		if ( func->type->ReturnType()->Type() != ev_void ) {
			Error( "Missing return value" );
		}

		// emit an end of statements opcode
		EmitOpcode( OP_RETURN, 0, 0 );
	}

	// record the number of statements in the function
	func->numStatements = program->NumStatements() - func->firstStatement;

	scope = oldscope;
}

/*
================
idCompiler::ParseVariableDef
================
*/
void idCompiler::ParseVariableDef( idTypeDef *type, const char *name ) {
	idVarDef	*def, *def2;
	bool		negate;

	def = program->GetDef( type, name, scope );
	if ( def ) {
		Error( "%s redeclared", name );
	}
	
	def = program->AllocDef( type, name, scope );

	if ( scope->Type() == ev_function ) {
		idTypeDef* cls = NULL;
		if ( scope->scope->Type() != ev_namespace ) {
			cls = scope->scope->TypeDef();
		}
		if ( program->IsExporting() ) {
			program->scriptExporter.RegisterClassFunctionVariable( cls, scope->value.functionPtr, def );
		}
	} else if ( scope->Type() == ev_namespace ) {
		if ( program->IsExporting() ) {
			program->scriptExporter.AllocGlobal( def );
		}
	}

	// check for an initialization
	if ( CheckToken( "=" ) ) {
		// if a local variable in a function then write out interpreter code to initialize variable
		if ( scope->Type() == ev_function ) {
			def2 = GetExpression( TOP_PRIORITY );
			if ( ( type == &type_float ) && ( def2->TypeDef() == &type_float ) ) {
				EmitOpcode( OP_STORE_F, def2, def );
			} else if ( ( type == &type_vector ) && ( def2->TypeDef() == &type_vector ) ) {
				EmitOpcode( OP_STORE_V, def2, def );
			} else if ( ( type == &type_string ) && ( def2->TypeDef() == &type_string ) ) {
				EmitOpcode( OP_STORE_S, def2, def );
			} else if ( ( type == &type_wstring ) && ( def2->TypeDef() == &type_wstring ) ) {
				EmitOpcode( OP_STORE_W, def2, def );
			} else if ( ( type->Inherits( &type_object ) ) && ( def2->TypeDef()->Inherits( &type_object ) ) ) {
				EmitOpcode( OP_STORE_OBJ, def2, def );
				program->GetStatement( program->NumStatements() - 1 ).c = def->TypeDef()->def;
			} else if ( ( type == &type_boolean ) && ( def2->TypeDef() == &type_boolean ) ) {
				EmitOpcode( OP_STORE_BOOL, def2, def );
			} else if ( ( type == &type_string ) && ( def2->TypeDef() == &type_float ) ) {
				EmitOpcode( OP_STORE_FTOS, def2, def );
			} else if ( ( type == &type_string ) && ( def2->TypeDef() == &type_boolean ) ) {
				EmitOpcode( OP_STORE_BTOS, def2, def );
			} else if ( ( type == &type_string ) && ( def2->TypeDef() == &type_vector ) ) {
				EmitOpcode( OP_STORE_VTOS, def2, def );
			} else if ( ( type == &type_boolean ) && ( def2->TypeDef() == &type_float ) ) {
				EmitOpcode( OP_STORE_FTOBOOL, def2, def );
			} else if ( ( type == &type_float ) && ( def2->TypeDef() == &type_boolean ) ) {
				EmitOpcode( OP_STORE_BOOLTOF, def2, def );
			} else {
				Error( "bad initialization for '%s'", name );
			}
		} else {
			// global variables can only be initialized with immediate values
			negate = false;
			if ( token.type == TT_PUNCTUATION && token == "-" ) {
				negate = true;
				NextToken();
				if ( immediateType != &type_float ) {
					Error( "wrong immediate type for '-' on variable '%s'", name );
				}
			}

			if ( immediateType != type ) {
				Error( "wrong immediate type for '%s'", name );
			}

			// global variables are initialized at start up
			if ( type == &type_string ) {
				def->SetString( token, false );
			} else {
				if ( negate ) {
					immediate._float = -immediate._float;
				}
				def->SetValue( immediate, false );
			}
			NextToken();
		}
	} else if ( type == &type_string ) {
		// local strings on the stack are initialized in the interpreter
		if ( scope->Type() != ev_function ) {
			def->SetString( "", false );
		}
	} else if ( type->Inherits( &type_object ) ) {
		if ( scope->Type() != ev_function ) {
			def->SetObject( NULL );
		}
	}
}

/*
================
idCompiler::GetTypeForEventArg
================
*/
idTypeDef *idCompiler::GetTypeForEventArg( char argType ) {
	idTypeDef *type;

	switch( argType ) {
	case D_EVENT_INTEGER :
		// this will get converted to int by the interpreter
		type = &type_float;
		break;

	case D_EVENT_BOOLEAN :
	case D_EVENT_HANDLE :
		type = &type_boolean;
		break;

	case D_EVENT_FLOAT :
		type = &type_float;
		break;

	case D_EVENT_VECTOR :
		type = &type_vector;
		break;

	case D_EVENT_STRING :
		type = &type_string;
		break;

	case D_EVENT_WSTRING :
		type = &type_wstring;
		break;

	case D_EVENT_ENTITY :
	case D_EVENT_ENTITY_NULL :
		type = &type_object;
		break;

	case D_EVENT_OBJECT:
		type = &type_object;
		break;

	case D_EVENT_VOID :
		type = &type_void;
		break;

	default:
		// probably a typo
		type = NULL;
		break;
	}
	
	return type;
}

/*
================
idCompiler::ParseScriptEventDef
================
*/
void idCompiler::ParseScriptEventDef( idTypeDef *returnType, const char *name ) {
	idTypeDef		*argType;
	idTypeDef		*type;
	idStr			parmName;

	idTypeDef newtype( ev_function, NULL, name, type_function.Size(), returnType );

	ExpectToken( "(" );

	if ( !CheckToken( ")" ) ) {
		do {
			argType = ParseType();

			ParseName( parmName );

			newtype.AddFunctionParm( argType, "" );

			if ( CheckToken( ")" ) ) {
				break;
			}

			ExpectToken( "," );
		} while ( true );
	}
	ExpectToken( ";" );

	type = program->FindType( name );
	if ( type ) {
		if ( !newtype.MatchesType( *type ) ) {
			Error( "Type mismatch on redefinition of '%s'", name );
		}
	} else {
		type = program->AllocType( newtype );
		type->def = program->AllocDef( type, name, &def_namespace );
		program->AllocVirtualFunction( type->def );

		int i;
		for ( i = 0; i < type->NumParameters(); i++ ) {
			type->def->value.functionPtr->parmTotal += type->GetParmType( i )->Size();
		}
		type->def->value.functionPtr->locals = type->def->value.functionPtr->parmTotal;

		if ( program->IsExporting() ) {
			program->scriptExporter.RegisterVirtualFunction( type );
		}
	}
}

/*
================
idCompiler::ParseEventDef
================
*/
void idCompiler::ParseEventDef( idTypeDef *returnType, const char *name ) {
	const idTypeDef	*expectedType;
	idTypeDef		*argType;
	idTypeDef		*type;
	int 			i;
	int				num;
	const char		*format;
	const idEventDef *ev;
	idStr			parmName;

	ev = idEventDef::FindEvent( name );
	if ( !ev ) {
		Error( "Unknown event '%s'", name );
	}

	if ( !ev->GetAllowFromScript() ) {
		Error( "'%s' cannot be called from script", name );
	}

	// set the return type
	expectedType = GetTypeForEventArg( ev->GetReturnType() );
	if ( !expectedType ) {
		Error( "Invalid return type '%c' in definition of '%s' event.", ev->GetReturnType(), name );
	}
	if ( returnType != expectedType ) {
		Error( "Return type doesn't match internal return type '%s'", expectedType->Name() );
	}

	idTypeDef newtype( ev_function, NULL, name, type_function.Size(), returnType );

	ExpectToken( "(" );

	format = ev->GetArgFormat();
	num = ev->GetNumArgs();
	for( i = 0; i < num; i++ ) {
		expectedType = GetTypeForEventArg( format[ i ] );
		if ( !expectedType || ( expectedType == &type_void ) ) {
			Error( "Invalid parameter '%c' in definition of '%s' event.", format[ i ], name );
		}

		argType = ParseType();
		ParseName( parmName );
		if ( argType != expectedType ) {
			Error( "The type of parm %d ('%s') does not match the internal type '%s' in definition of '%s' event.", 
				i + 1, parmName.c_str(), expectedType->Name(), name );
		}

		newtype.AddFunctionParm( argType, "" );

		if ( i < num - 1 ) {
			if ( CheckToken( ")" ) ) {
				Error( "Too few parameters for event definition.  Internal definition has %d parameters.", num );
			}
			ExpectToken( "," );
		}
	}
	if ( !CheckToken( ")" ) ) {
		Error( "Too many parameters for event definition.  Internal definition has %d parameters.", num );
	}
	ExpectToken( ";" );

	type = program->FindType( name );
	if ( type ) {
		if ( !newtype.MatchesType( *type ) || ( type->def->value.functionPtr->eventdef != ev ) ) {
			Error( "Type mismatch on redefinition of '%s'", name );
		}
	} else {
		type = program->AllocType( newtype );
		type->def = program->AllocDef( type, name, &def_namespace );

		function_t &func	= program->AllocFunction( type->def );
		func.eventdef		= ev;
		func.parmSize.SetNum( num );
		for( i = 0; i < num; i++ ) {
			argType = newtype.GetParmType( i );
			func.parmTotal		+= argType->Size();
			func.parmSize[ i ]	= argType->Size();
		}

		// mark the parms as local
		func.locals	= func.parmTotal;

		if ( program->IsExporting() ) {
			program->scriptExporter.RegisterEventDef( func );
		}
	}
}

/*
================
idCompiler::ParseDefs

Called at the outer layer and when a local statement is hit
================
*/
void idCompiler::ParseDefs( void ) {
	idStr 		name;
	idTypeDef	*type;
	idVarDef	*def;
	idVarDef	*oldscope;

	if ( CheckToken( ";" ) ) {
		// skip semicolons, which are harmless and ok syntax
		return;
	}

	static int counter = 0;
	if( !networkSystem->IsDedicated() ) {
		counter++;
		if( counter % 50 == 0 ) {
			common->PacifierUpdate();
		}
	}

	idStr typeName = token;

	type = ParseType();
	if ( type == &type_scriptevent ) {
		type = ParseType();
		ParseName( name );
		ParseEventDef( type, name );
		return;
	}

	if ( type == &type_internalscriptevent ) {
		type = ParseType();
		ParseName( name );
		ParseScriptEventDef( type, name );
		return;
	}
    
	ParseName( name );

	if ( type == &type_namespace ) {
		def = program->GetDef( type, name, scope );
		if ( !def ) {
			def = program->AllocDef( type, name, scope );
		}
		if ( program->IsExporting() ) {
			program->scriptExporter.EnterNamespace( name );
		}
		ParseNamespace( def );
		if ( program->IsExporting() ) {
			program->scriptExporter.ExitNamespace();
		}
	} else if ( CheckToken( "::" ) ) {
		def = program->GetDef( NULL, name, scope );
		if ( !def ) {
			Error( "Unknown object name '%s'", name.c_str() );
		}
		ParseName( name );
		oldscope = scope;
		scope = def;

		if ( !program->FindFunctionLocal( name, def->TypeDef() ) ) {
			Error( "Function Definition Without a Prototype '%s'", name.c_str() );
		}

		ExpectToken( "(" );
		ParseFunctionDef( type, name.c_str() );
		scope = oldscope;
	} else if ( type == &type_object && scope->Type() == ev_namespace
		&& ( typeName.Cmp( "object" ) == 0 ) ) { // Gordon: bit of a hack, but this lets you create namespace functions that return entities, which is handy...
		ParseObjectDef( name.c_str() );
	} else if ( CheckToken( "(" ) ) {		// check for a function prototype or declaraction
		ParseFunctionDef( type, name.c_str() );
	} else {
		ParseVariableDef( type, name.c_str() );
		while( CheckToken( "," ) ) {
			ParseName( name );
			ParseVariableDef( type, name.c_str() );
		}
		ExpectToken( ";" );
	}
}

/*
================
idCompiler::ParseNamespace

Parses anything within a namespace definition
================
*/
void idCompiler::ParseNamespace( idVarDef *newScope ) {
	idVarDef *oldscope;

	oldscope = scope;
	if ( newScope != &def_namespace ) {
		ExpectToken( "{" );
	}

	while( !eof ) {
		scope		= newScope;
		callthread	= false;
		callguithread = false;

		if ( ( newScope != &def_namespace ) && CheckToken( "}" ) ) {
			break;
		}

		ParseDefs();
	}

	scope = oldscope;
}

/*
============
idCompiler::CompileFile

compiles the 0 terminated text, adding definitions to the program structure
============
*/
void idCompiler::CompileFile( const char *text, const char *filename ) {
	idTimer	compile_time;
	idStr	scriptFileName = filename;
	bool	error;

	compile_time.Start();

	scope				= &def_namespace;
	basetype			= NULL;
	callthread			= false;
	callguithread		= false;
	loopDepth			= 0;
	eof					= false;
	braceDepth			= 0;
	immediateType		= NULL;
	currentLineNumber	= 0;
	
	memset( &immediate, 0, sizeof( immediate ) );

	parser.SetFlags( LEXFL_ALLOWMULTICHARLITERALS );
	parser.LoadMemory( text, idStr::Length( text ), scriptFileName );
	parserPtr = &parser;

	error = false;
	try {
		// read first token
		NextToken();
		while( !eof && !error ) {
			// parse from global namespace
			ParseNamespace( &def_namespace );
		}
	}
		
	catch( idCompileError &err ) {
		idStr error = va( "Error: file %s, line %d: %s", program->GetFilename( currentFileNumber ), currentLineNumber, err.error );

		parser.FreeSource();

		throw idCompileError( error );
	}

	parser.FreeSource();

	compile_time.Stop();

	gameLocal.Printf( "Compiled '%s': %.1f ms\n", scriptFileName.c_str(), compile_time.Milliseconds() );
}
