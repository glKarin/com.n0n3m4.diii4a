// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Script_Program.h"
#include "Script_Compiler.h"
#include "Script_Thread.h"
#include "Script_Helper.h"
#include "Script_ScriptObject.h"

#include "../../framework/Licensee.h"

const int SCRIPT_TYPE_PTR_SIZE = sizeof( int );

// simple types.  function types are dynamically allocated
idTypeDef	type_void( ev_void, &def_void, "void", 0, NULL );
idTypeDef	type_scriptevent( ev_scriptevent, &def_scriptevent, "scriptevent", SCRIPT_TYPE_PTR_SIZE, NULL );
idTypeDef	type_namespace( ev_namespace, &def_namespace, "namespace", SCRIPT_TYPE_PTR_SIZE, NULL );
idTypeDef	type_string( ev_string, &def_string, "string", MAX_STRING_LEN, NULL );
idTypeDef	type_wstring( ev_wstring, &def_wstring, "wstring", MAX_STRING_LEN * sizeof( wchar_t ), NULL );
idTypeDef	type_float( ev_float, &def_float, "float", sizeof( float ), NULL );
idTypeDef	type_vector( ev_vector, &def_vector, "vector", sizeof( idVec3 ), NULL );
idTypeDef	type_field( ev_field, &def_field, "field", SCRIPT_TYPE_PTR_SIZE, NULL );
idTypeDef	type_function( ev_function, &def_function, "function", sizeof( void * ), &type_void );
idTypeDef	type_virtualfunction( ev_virtualfunction, &def_virtualfunction, "virtual function", sizeof( int ), NULL );
idTypeDef	type_pointer( ev_pointer, &def_pointer, "pointer", sizeof( void * ), NULL );

idTypeDef	type_object( ev_object, &def_object, "object", SCRIPT_TYPE_PTR_SIZE, NULL );

idTypeDef	type_jumpoffset( ev_jumpoffset, &def_jumpoffset, "<jump>", sizeof( int ), NULL );		// only used for jump opcodes
idTypeDef	type_argsize( ev_argsize, &def_argsize, "<argsize>", sizeof( int ), NULL );				// only used for function call and thread opcodes
idTypeDef	type_boolean( ev_boolean, &def_boolean, "boolean", sizeof( int ), NULL );
idTypeDef	type_internalscriptevent( ev_internalscriptevent, &def_internalscriptevent, "virtual", SCRIPT_TYPE_PTR_SIZE/*sizeof( void * )*/, NULL );

idVarDef	def_void( &type_void );
idVarDef	def_scriptevent( &type_scriptevent );
idVarDef	def_namespace( &type_namespace );
idVarDef	def_string( &type_string );
idVarDef	def_wstring( &type_wstring );
idVarDef	def_float( &type_float );
idVarDef	def_vector( &type_vector );
idVarDef	def_field( &type_field );
idVarDef	def_function( &type_function );
idVarDef	def_virtualfunction( &type_virtualfunction );
idVarDef	def_pointer( &type_pointer );
idVarDef	def_object( &type_object );
idVarDef	def_jumpoffset( &type_jumpoffset );		// only used for jump opcodes
idVarDef	def_argsize( &type_argsize );
idVarDef	def_boolean( &type_boolean );
idVarDef	def_internalscriptevent( &type_internalscriptevent );

/***********************************************************************

  function_t

***********************************************************************/

/*
================
function_t::function_t
================
*/
function_t::function_t() {
	Clear();
}

/*
================
function_t::Allocated
================
*/
size_t function_t::Allocated( void ) const {
	return name.Allocated() + parmSize.Allocated();
}

/*
================
function_t::SetName
================
*/
void function_t::SetName( const char *name ) {
	this->name = name;
}

/*
================
function_t::Name
================
*/
const char *function_t::Name( void ) const {
	return name;
}

/*
================
function_t::Clear
================
*/
void function_t::Clear( void ) {
	eventdef		= NULL;
	def				= NULL;
	type			= NULL;
	firstStatement	= 0;
	numStatements	= 0;
	parmTotal		= 0;
	locals			= 0;
	filenum			= 0;
	name.Clear();
	parmSize.Clear();
	virtualIndex	= -1;
}

/***********************************************************************

  idTypeDef

***********************************************************************/

/*
================
idTypeDef::idTypeDef
================
*/
idTypeDef::idTypeDef( void ) {
	program = NULL;
	parmTypes.SetGranularity( 1 );
	parmNames.SetGranularity( 1 );
	functions.SetGranularity( 1 );
	globalVirtuals.SetGranularity( 1 );
}

/*
================
idTypeDef::idTypeDef
================
*/
idTypeDef::idTypeDef( const idTypeDef &other ) {
	program = NULL;

	*this = other;
}

/*
================
idTypeDef::idTypeDef
================
*/
idTypeDef::idTypeDef( etype_t etype, idVarDef *edef, const char *ename, int esize, idTypeDef *aux ) {
	name		= ename;
	type		= etype;
	def			= edef;
	size		= esize;
	auxType		= aux;
	program		= NULL;
	
	parmTypes.SetGranularity( 1 );
	parmNames.SetGranularity( 1 );
	functions.SetGranularity( 1 );
	globalVirtuals.SetGranularity( 1 );
}

/*
================
idTypeDef::~idTypeDef
================
*/
idTypeDef::~idTypeDef( void ) {
	if ( program != NULL ) {
		program->RemoveFromHash( this );
	}
}

/*
================
idTypeDef::Init
================
*/
void idTypeDef::Init( etype_t etype, idVarDef *edef, const char *ename, int esize, idTypeDef *aux ) {
	name		= ename;
	type		= etype;
	def			= edef;
	size		= esize;
	auxType		= aux;
}

/*
================
idTypeDef::operator=
================
*/
void idTypeDef::operator=( const idTypeDef& other ) {
	if ( program != NULL ) {
		SetName( NULL, "" );
	}

	type			= other.type;
	def				= other.def;
	size			= other.size;
	auxType			= other.auxType;
	parmTypes		= other.parmTypes;
	parmNames		= other.parmNames;
	functions		= other.functions;
	globalVirtuals	= other.globalVirtuals;

	SetName( other.program, other.name );
}

/*
================
idTypeDef::Allocated
================
*/
size_t idTypeDef::Allocated( void ) const {
	size_t memsize;
	int i;

	memsize = name.Allocated() + parmTypes.Allocated() + parmNames.Allocated() + functions.Allocated();
	for( i = 0; i < parmTypes.Num(); i++ ) {
		memsize += parmNames[ i ].Allocated();
	}

	return memsize;
}

/*
================
idTypeDef::Inherits

Returns true if basetype is an ancestor of this type.
================
*/
bool idTypeDef::Inherits( const idTypeDef *basetype ) const {
	idTypeDef *superType;

	if ( type != ev_object ) {
		return false;
	}

	if ( this == basetype ) {
		return true;
	}
	for( superType = auxType; superType != NULL; superType = superType->auxType ) {
		if ( superType == basetype ) {
			return true;
		}
	}

	return false;
}

/*
================
idTypeDef::MatchesType

Returns true if both types' base types and parameters match
================
*/
bool idTypeDef::MatchesType( const idTypeDef &matchtype ) const {
	int i;

	if ( this == &matchtype ) {
		return true;
	}

	if ( ( type != matchtype.type ) || ( auxType != matchtype.auxType ) ) {
		return false;
	}

	if ( parmTypes.Num() != matchtype.parmTypes.Num() ) {
		return false;
	}

	for( i = 0; i < matchtype.parmTypes.Num(); i++ ) {
		if ( parmTypes[ i ] != matchtype.parmTypes[ i ] ) {
			return false;
		}
	}

	return true;
}

/*
================
idTypeDef::MatchesVirtualFunction

Returns true if both functions' base types and parameters match
================
*/
bool idTypeDef::MatchesVirtualFunction( const idTypeDef &matchfunc ) const {
	int i;

	if ( this == &matchfunc ) {
		return true;
	}

	if ( ( type != matchfunc.type ) || ( auxType != matchfunc.auxType ) ) {
		return false;
	}

	if ( parmTypes.Num() != matchfunc.parmTypes.Num() ) {
		return false;
	}

	if ( parmTypes.Num() > 0 ) {
		if ( !parmTypes[ 0 ]->Inherits( matchfunc.parmTypes[ 0 ] ) ) {
			return false;
		}
	}

	for( i = 1; i < matchfunc.parmTypes.Num(); i++ ) {
		if ( parmTypes[ i ] != matchfunc.parmTypes[ i ] ) {
			return false;
		}
	}

	return true;
}

/*
================
idTypeDef::AddFunctionParm

Adds a new parameter for a function type.
================
*/
void idTypeDef::AddFunctionParm( idTypeDef *parmtype, const char *name ) {
	if ( type != ev_function ) {
		throw idCompileError( "idTypeDef::AddFunctionParm : tried to add parameter on non-function type" );
	}

	parmTypes.Append( parmtype );
	idStr &parmName = parmNames.Alloc();
	parmName = name;
}

/*
================
idTypeDef::AddField

Adds a new field to an object type.
================
*/
void idTypeDef::AddField( idTypeDef *fieldtype, const char *name ) {
	if ( type != ev_object ) {
		throw idCompileError( "idTypeDef::AddField : tried to add field to non-object type" );
	}

	parmTypes.Append( fieldtype );
	idStr &parmName = parmNames.Alloc();
	parmName = name;

	if ( fieldtype->FieldType()->Inherits( &type_object ) ) {
		size += type_object.Size();
	} else {
		size += fieldtype->FieldType()->Size();
	}
}

/*
================
idTypeDef::SetName
================
*/
void idTypeDef::SetName( idProgram* _program, const char* newname ) {
	bool wasInHash = false;
	if ( program != NULL ) {
		wasInHash = program->RemoveFromHash( this );
	}

	name = newname;

	if ( wasInHash ) {
		if ( _program != NULL ) {
			_program->AddToHash( this );
		}
	}
}

/*
================
idTypeDef::Name
================
*/
const char *idTypeDef::Name( void ) const {
	return name;
}

/*
================
idTypeDef::Type
================
*/
etype_t idTypeDef::Type( void ) const {
	return type;
}

/*
================
idTypeDef::Size
================
*/
int idTypeDef::Size( void ) const {
	return size;
}

/*
================
idTypeDef::SuperClass

If type is an object, then returns the object's superclass
================
*/
idTypeDef *idTypeDef::SuperClass( void ) const {
	if ( type != ev_object ) {
		throw idCompileError( "idTypeDef::SuperClass : tried to get superclass of a non-object type" );
	}

	return auxType;
}

/*
================
idTypeDef::ReturnType

If type is a function, then returns the function's return type
================
*/
idTypeDef *idTypeDef::ReturnType( void ) const {
	if ( type != ev_function ) {
		throw idCompileError( "idTypeDef::ReturnType: tried to get return type on non-function type" );
	}

	return auxType;
}

/*
================
idTypeDef::SetReturnType

If type is a function, then sets the function's return type
================
*/
void idTypeDef::SetReturnType( idTypeDef *returntype ) {
	if ( type != ev_function ) {
		throw idCompileError( "idTypeDef::SetReturnType: tried to set return type on non-function type" );
	}

	auxType = returntype;
}

/*
================
idTypeDef::FieldType

If type is a field, then returns it's type
================
*/
idTypeDef *idTypeDef::FieldType( void ) const {
	if ( type != ev_field ) {
		throw idCompileError( "idTypeDef::FieldType: tried to get field type on non-field type" );
	}

	return auxType;
}

/*
================
idTypeDef::SetFieldType

If type is a field, then sets the function's return type
================
*/
void idTypeDef::SetFieldType( idTypeDef *fieldtype ) {
	if ( type != ev_field ) {
		throw idCompileError( "idTypeDef::SetFieldType: tried to set return type on non-function type" );
	}

	auxType = fieldtype;
}

/*
================
idTypeDef::PointerType

If type is a pointer, then returns the type it points to
================
*/
idTypeDef *idTypeDef::PointerType( void ) const {
	if ( type != ev_pointer ) {
		throw idCompileError( "idTypeDef::PointerType: tried to get pointer type on non-pointer" );
	}

	return auxType;
}

/*
================
idTypeDef::SetPointerType

If type is a pointer, then sets the pointer's type
================
*/
void idTypeDef::SetPointerType( idTypeDef *pointertype ) {
	if ( type != ev_pointer ) {
		throw idCompileError( "idTypeDef::SetPointerType: tried to set type on non-pointer" );
	}

	auxType = pointertype;
}

/*
================
idTypeDef::NumParameters
================
*/
int idTypeDef::NumParameters( void ) const {
	return parmTypes.Num();
}

/*
================
idTypeDef::GetParmType
================
*/
idTypeDef *idTypeDef::GetParmType( int parmNumber ) const {
	assert( parmNumber >= 0 );
	assert( parmNumber < parmTypes.Num() );
	return parmTypes[ parmNumber ];
}

/*
================
idTypeDef::GetParmName
================
*/
const char *idTypeDef::GetParmName( int parmNumber ) const {
	assert( parmNumber >= 0 );
	assert( parmNumber < parmTypes.Num() );
	return parmNames[ parmNumber ];
}

/*
================
idTypeDef::NumFunctions
================
*/
int idTypeDef::NumFunctions( void ) const {
	return functions.Num();
}

/*
================
idTypeDef::GetFunctionNumber
================
*/
int idTypeDef::GetFunctionNumber( const function_t *func ) const {
	int i;

	for( i = 0; i < functions.Num(); i++ ) {
		if ( functions[ i ] == func ) {
			return i;
		}
	}
	return -1;
}

/*
================
idTypeDef::GetFunction
================
*/
const function_t *idTypeDef::GetFunction( int funcNumber ) const {
	assert( funcNumber >= 0 );
	assert( funcNumber < functions.Num() );
	return functions[ funcNumber ];
}

/*
================
idTypeDef::GetVirtualFunction
================
*/
const function_t *idTypeDef::GetVirtualFunction( int funcNumber ) const {
	int index = globalVirtuals.GetFirst( funcNumber );
	if ( index == -1 ) {
		return NULL;
	}
	return GetFunction( index );
}

/*
================
idTypeDef::AddFunction
================
*/
void idTypeDef::AddFunction( const function_t* func, idProgram& program ) {
	int i;

	for( i = 0; i < functions.Num(); i++ ) {
		if ( !idStr::Cmp( functions[ i ]->def->Name(), func->def->Name() ) ) {
			if ( func->def->TypeDef()->MatchesVirtualFunction( *functions[ i ]->def->TypeDef() ) ) {
				functions[ i ] = func;
				return;
			}
		}
	}
	
	int index = functions.Append( func );

	int globalVirtualIndex = program.MatchesVirtualFunction( *func->def->TypeDef() );
	if ( globalVirtualIndex != -1 ) {
		globalVirtuals.Add( globalVirtualIndex, index );
	}
}

/***********************************************************************

  idVarDef

***********************************************************************/

/*
================
idVarDef::idVarDef()
================
*/
idVarDef::idVarDef( idTypeDef *typeptr ) {
	typeDef		= typeptr;
	scope		= NULL;
	numUsers	= 0;
	settings.initialized	= idVarDef::uninitialized;
	settings.isReturn		= false;
	memset( &value, 0, sizeof( value ) );
	name		= NULL;
	next		= NULL;
}

/*
============
idVarDef::~idVarDef
============
*/
idVarDef::~idVarDef() {
	if ( name ) {
		name->RemoveDef( this );
	}
}

/*
================
idVarDef::Init
================
*/
void idVarDef::Init( idTypeDef *typeptr ) {
	typeDef		= typeptr;
	scope		= NULL;
	numUsers	= 0;
	settings.initialized	= idVarDef::uninitialized;
	settings.isReturn		= false;
	memset( &value, 0, sizeof( value ) );
	name		= NULL;
	next		= NULL;
}

/*
================
idVarDef::Clear
================
*/
void idVarDef::Clear() {
	if ( name ) {
		name->RemoveDef( this );
	}
}

/*
============
idVarDef::Name
============
*/
const char *idVarDef::Name( void ) const {
	return name->Name();
}

/*
============
idVarDef::GlobalName
============
*/
const char *idVarDef::GlobalName( void ) const {
	if ( scope != &def_namespace ) {
		return va( "%s::%s", scope->GlobalName(), name->Name() );
	} else {
		return name->Name();
	}
}

/*
============
idVarDef::DepthOfScope
============
*/
int idVarDef::DepthOfScope( const idVarDef *otherScope ) const {
	const idVarDef *def;
	int depth;

	depth = 1;
	for( def = otherScope; def != NULL; def = def->scope ) {
		if ( def == scope ) {
			return depth;
		}
		depth++;
	}

	return 0;
}

/*
============
idVarDef::SetFunction
============
*/
void idVarDef::SetFunction( function_t *func ) {
	assert( typeDef );
	settings.initialized	= initializedConstant;
	settings.isReturn		= false;
	assert( typeDef->Type() == ev_function );
	value.functionPtr = func;
}

/*
============
idVarDef::SetObject
============
*/
void idVarDef::SetObject( idScriptObject* object ) {
	assert( typeDef );
	settings.initialized	= initializedVariable;
	settings.isReturn		= false;
	assert( typeDef->Inherits( &type_object ) );
	*value.objectId = object ? object->GetHandle() : 0;
}

/*
============
idVarDef::SetValue
============
*/
void idVarDef::SetValue( const eval_t &_value, bool constant ) {
	assert( typeDef );
	if ( constant ) {
		settings.initialized = initializedConstant;
	} else {
		settings.initialized = initializedVariable;
	}
	settings.isReturn = false;

	switch( typeDef->Type() ) {
	case ev_pointer :
	case ev_boolean :
	case ev_field :
		*value.intPtr = _value._int;
		break;

	case ev_jumpoffset :
		value.jumpOffset = _value._int;
		break;

	case ev_argsize :
		value.argSize = _value._int;
		break;

	case ev_object :
		*value.objectId = _value._objectId;
		break;

	case ev_string :
		idStr::Copynz( value.stringPtr, _value.stringPtr, MAX_STRING_LEN );
		break;

	case ev_wstring :
		idWStr::Copynz( value.wstringPtr, _value.wstringPtr, MAX_STRING_LEN );
		break;

	case ev_float :
		*value.floatPtr = _value._float;
		break;

	case ev_vector :
		value.vectorPtr->x = _value.vector[ 0 ];
		value.vectorPtr->y = _value.vector[ 1 ];
		value.vectorPtr->z = _value.vector[ 2 ];
		break;

	case ev_function :
		value.functionPtr = _value.function;
		break;

	case ev_virtualfunction :
		value.virtualFunction = _value._int;
		break;

	default :
		throw idCompileError( va( "weird type on '%s'", Name() ) );
		break;
	}
}

/*
============
idVarDef::SetString
============
*/
void idVarDef::SetString( const char *string, bool constant ) {
	if ( constant ) {
		settings.initialized = initializedConstant;
	} else {
		settings.initialized = initializedVariable;
	}
	settings.isReturn = false;
	
	assert( typeDef && ( typeDef->Type() == ev_string ) );
	idStr::Copynz( value.stringPtr, string, MAX_STRING_LEN );
}

/*
============
idVarDef::PrintInfo
============
*/
void idVarDef::PrintInfo( const idProgram& program, idFile *file, int instructionPointer ) const {
	const statement_t	*jumpst;
	int			jumpto;
	etype_t		etype;
	int			i;
	int			len;
	const char	*ch;

	if ( settings.initialized == initializedConstant ) {
		file->Printf( "const " );
	}

	etype = typeDef->Type();
	switch( etype ) {
	case ev_jumpoffset :
		jumpto = instructionPointer + value.jumpOffset;
		jumpst = &program.GetStatement( jumpto );
		file->Printf( "address %d [%s(%d)]", jumpto, program.GetFilename( jumpst->file ), jumpst->linenumber );
		break;

	case ev_function :
		if ( value.functionPtr->eventdef ) {
			file->Printf( "event %s", GlobalName() );
		} else {
			file->Printf( "function %s", GlobalName() );
		}
		break;

	case ev_field :
		file->Printf( "field %d", value.ptrOffset );
		break;

	case ev_argsize:
		file->Printf( "args %d", value.argSize );
		break;

	default:
		file->Printf( "%s ", typeDef->Name() );
		if ( settings.initialized == initializedConstant ) {
			switch( etype ) {
			case ev_string :
				file->Printf( "\"" );
				len = idStr::Length( value.stringPtr );
				ch = value.stringPtr;
				for( i = 0; i < len; i++, ch++ ) {
					if ( idStr::CharIsPrintable( *ch ) ) {
						file->Printf( "%c", *ch );
					} else if ( *ch == '\n' ) {
						file->Printf( "\\n" );
					} else {
						file->Printf( "\\x%.2x", static_cast<int>( *ch ) );
					}
				}
				file->Printf( "\"" );
				break;

			case ev_wstring :
				assert( false );/*
				file->Printf( "\"" );
				len = idStr::Length( value.stringPtr );
				ch = value.stringPtr;
				for( i = 0; i < len; i++, ch++ ) {
					if ( idStr::CharIsPrintable( *ch ) ) {
						file->Printf( "%c", *ch );
					} else if ( *ch == '\n' ) {
						file->Printf( "\\n" );
					} else {
						file->Printf( "\\x%.2x", static_cast<int>( *ch ) );
					}
				}
				file->Printf( "\"" );*/
				break;

			case ev_vector :
				file->Printf( "'%s'", value.vectorPtr->ToString() );
				break;

			case ev_float :
                file->Printf( "%f", *value.floatPtr );
				break;

			case ev_virtualfunction :
				file->Printf( "vtable[ %d ]", value.virtualFunction );
				break;

			default :
				file->Printf( "%d", *value.intPtr );
				break;
			}
		} else if ( settings.initialized == stackVariable ) {
			file->Printf( "stack[%d]", value.stackOffset );
		} else {
			file->Printf( "global[%d]", program.GetVarDefIndex( this ) );
		}
		break;
	}
}

/***********************************************************************

  idVarDef

***********************************************************************/

/*
============
idVarDefName::AddDef
============
*/
void idVarDefName::AddDef( idVarDef *def ) {
	assert( def->Next() == NULL );
	def->SetName( this );
	def->SetNext( defs );
	defs = def;
}

/*
============
idVarDefName::RemoveDef
============
*/
void idVarDefName::RemoveDef( idVarDef *def ) {
	if ( defs == def ) {
		defs = def->Next();
	} else {
		for ( idVarDef* d = defs; d->Next() != NULL; d = d->Next() ) {
			if ( d->Next() == def ) {
				d->SetNext( def->Next() );
				break;
			}
		}
	}
	def->SetNext( NULL );
	def->SetName( NULL );
}

/***********************************************************************

  idProgram

***********************************************************************/

/*
============
idProgram::AllocType
============
*/
idTypeDef *idProgram::AllocType( idTypeDef &type ) {
	idTypeDef* newtype = typeDefAllocator.Alloc();
	*newtype = type;
	
	int index = types.Append( newtype );

	AddToHash( newtype, index );

	return newtype;
}

/*
============
idProgram::AllocType
============
*/
idTypeDef *idProgram::AllocType( etype_t etype, idVarDef *edef, const char *ename, int esize, idTypeDef *aux ) {
	idTypeDef *newtype;

	newtype	= typeDefAllocator.Alloc();
	newtype->Init( etype, edef, ename, esize, aux );
	int index = types.Append( newtype );

	AddToHash( newtype, index );

	return newtype;
}

/*
============
idProgram::RemoveFromHash
============
*/
bool idProgram::RemoveFromHash( idTypeDef* type ) {
	int key = typeHash.GenerateKey( type->Name() );

	for ( int index = typeHash.GetFirst( key ); index != typeHash.NULL_INDEX; index = typeHash.GetNext( index ) ) {
		if ( types[ index ] != type ) {
			continue;
		}

		typeHash.Remove( key, index );
		return true;
	}

	return false;
}

/*
============
idProgram::AddToHash
============
*/
void idProgram::AddToHash( idTypeDef* type, int index ) {
	if ( index == -1 ) {
		for ( int i = 0; i < types.Num(); i++ ) {
			if ( types[ i ] != type ) {
				continue;
			}

			index = i;
			break;
		}
	}

	if ( index != -1 ) {
		type->SetProgram( this );
		int key = typeHash.GenerateKey( type->Name() );
		typeHash.Add( key, index );
	}
}

/*
============
idProgram::GetType

Returns a preexisting complex type that matches the parm, or allocates
a new one and copies it out.
============
*/
idTypeDef *idProgram::GetType( idTypeDef &type, bool allocate ) {

//	for( int index = types.Num() - 1; index >= 0; index-- ) {

	int key = typeHash.GenerateKey( type.Name() );
	for ( int index = typeHash.GetFirst( key ); index != typeHash.NULL_INDEX; index = typeHash.GetNext( index ) ) {

		if ( types[ index ]->MatchesType( type ) && !idStr::Cmp( types[ index ]->Name(), type.Name() ) ) {
			return types[ index ];
		}
	}

	if ( !allocate ) {
		return NULL;
	}

	// allocate a new one
	return AllocType( type );
}

/*
============
idProgram::FindType

Returns a preexisting complex type that matches the name, or returns NULL if not found
============
*/
idTypeDef *idProgram::FindType( const char *name ) {
	idTypeDef	*check;
	int			i;

	for( i = types.Num() - 1; i >= 0; i-- ) {
		check = types[ i ];
		if ( !idStr::Cmp( check->Name(), name ) ) {
			return check;
		}
	}

	return NULL;
}

/*
============
idProgram::GetDefList
============
*/
idVarDef *idProgram::GetDefList( const char *name ) const {
	int i, hash;

	hash = varDefNameHash.GenerateKey( name, true );
	for ( i = varDefNameHash.GetFirst( hash ); i != -1; i = varDefNameHash.GetNext( i ) ) {
		if ( idStr::Cmp( varDefNames[i]->Name(), name ) == 0 ) {
			return varDefNames[i]->GetDefs();
		}
	}
	return NULL;
}

/*
============
idProgram::AddDefToNameList
============
*/
void idProgram::AddDefToNameList( idVarDef *def, const char *name ) {
	int i, hash;

	hash = varDefNameHash.GenerateKey( name, true );
	for ( i = varDefNameHash.GetFirst( hash ); i != -1; i = varDefNameHash.GetNext( i ) ) {
		if ( idStr::Cmp( varDefNames[i]->Name(), name ) == 0 ) {
			break;
		}
	}
	if ( i == -1 ) {
		idVarDefName* varDefName = varDefNameAllocator.Alloc();
		varDefName->Init( name );
		i = varDefNames.Append( varDefName );
		varDefNameHash.Add( hash, i );
	}
	varDefNames[i]->AddDef( def );
}

/*
============
idProgram::AllocDef
============
*/
idVarDef *idProgram::AllocDef( idTypeDef *type, const char *name, idVarDef *scope ) {
	idVarDef	*def;
	idStr		element;
	idVarDef	*def_x;
	idVarDef	*def_y;
	idVarDef	*def_z;

	// allocate a new def
	def = varDefAllocator.Alloc();
	def->Init( type );
	def->scope		= scope;
	def->numUsers	= 1;
	varDefs.Alloc() = def;

	// add the def to the list with defs with this name and set the name pointer
	AddDefToNameList( def, name );

	if ( ( type->Type() == ev_vector ) || ( ( type->Type() == ev_field ) && ( type->FieldType()->Type() == ev_vector ) ) ) {
		//
		// vector
		//
		if ( !idStr::Cmp( name, RESULT_STRING ) ) {
			// <RESULT> vector defs don't need the _x, _y and _z components
			assert( scope->Type() == ev_function );
			def->value.stackOffset	= scope->value.functionPtr->locals;
			def->settings.initialized		= idVarDef::stackVariable;
			scope->value.functionPtr->locals += type->Size();
		} else if ( scope->TypeDef()->Inherits( &type_object ) ) {
			idTypeDef	newtype( ev_field, NULL, "float field", 0, &type_float );
			idTypeDef	*type = GetType( newtype, true );

			// set the value to the variable's position in the object
			def->value.ptrOffset = scope->TypeDef()->Size();

			// make automatic defs for the vectors elements
			// origin can be accessed as origin_x, origin_y, and origin_z
			sprintf( element, "%s_x", def->Name() );
			def_x = AllocDef( type, element, scope );

			sprintf( element, "%s_y", def->Name() );
			def_y = AllocDef( type, element, scope );
			def_y->value.ptrOffset = def_x->value.ptrOffset + type_float.Size();

			sprintf( element, "%s_z", def->Name() );
			def_z = AllocDef( type, element, scope );
			def_z->value.ptrOffset = def_y->value.ptrOffset + type_float.Size();
		} else {
			// make automatic defs for the vectors elements
			// origin can be accessed as origin_x, origin_y, and origin_z
			sprintf( element, "%s_x", def->Name() );
			def_x = AllocDef( &type_float, element, scope );

			sprintf( element, "%s_y", def->Name() );
			def_y = AllocDef( &type_float, element, scope );

			sprintf( element, "%s_z", def->Name() );
			def_z = AllocDef( &type_float, element, scope );

			// point the vector def to the x coordinate
			def->value					= def_x->value;
			def->settings.initialized	= def_x->settings.initialized;
			def->settings.isReturn		= def_x->settings.isReturn;
		}
	} else if ( scope->TypeDef()->Inherits( &type_object ) ) {
		//
		// object variable
		//
		// set the value to the variable's position in the object
		def->value.ptrOffset = scope->TypeDef()->Size();
	} else if ( scope->Type() == ev_function ) {
		//
		// stack variable
		//
		// since we don't know how many local variables there are,
		// we have to have them go backwards on the stack
		def->value.stackOffset			= scope->value.functionPtr->locals;
		def->settings.initialized		= idVarDef::stackVariable;

		if ( type->Inherits( &type_object ) ) {
			// objects only have their entity number on the stack, not the entire object
			scope->value.functionPtr->locals += type_object.Size();
		} else {
			scope->value.functionPtr->locals += type->Size();
		}
	} else {
		//
		// global variable
		//

		int size;
		if ( def->TypeDef()->Inherits( &type_object ) ) {
			size = type_object.Size();
		} else {
			size = def->TypeDef()->Size();
		}
		def->value.bytePtr = &variables[ numVariables ];
		numVariables += size;
		if ( numVariables > sizeof( variables ) ) {
			throw idCompileError( va( "Exceeded global memory size (%d bytes)", sizeof( variables ) ) );
		}

		memset( def->value.bytePtr, 0, size );
	}

	return def;
}

/*
============
idProgram::GetDef

If type is NULL, it will match any type
============
*/
idVarDef *idProgram::GetDef( const idTypeDef *type, const char *name, const idVarDef *scope ) const {
	idVarDef		*def;
	idVarDef		*bestDef;
	int				bestDepth;
	int				depth;

	bestDepth = 0;
	bestDef = NULL;
	for( def = GetDefList( name ); def != NULL; def = def->Next() ) {
		if ( type && ( def->Type() == ev_function ) ) {
			if ( def->value.functionPtr->virtualIndex != -1 ) {
				continue;
			}
		}

		if ( def->scope->Type() == ev_namespace ) {
			depth = def->DepthOfScope( scope );
			if ( !depth ) {
				// not in the same namespace
				continue;
			}
		} else if ( def->scope != scope ) {
			// in a different function
			continue;
		} else {
			depth = 1;
		}

		if ( !bestDef || ( depth < bestDepth ) ) {
			bestDepth = depth;
			bestDef = def;
		}
	}

	// see if the name is already in use for another type
	if ( bestDef && type && ( bestDef->TypeDef() != type ) ) {
		throw idCompileError( va( "Type mismatch on redeclaration of %s", name ) );
	}

	return bestDef;
}

/*
============
idProgram::FreeDef
============
*/
void idProgram::FreeDef( idVarDef *def, const idVarDef *scope ) {
	if ( def->Type() == ev_vector ) {
		idVarDef* e;

		e = GetDef( NULL, va( "%s_x", def->Name() ), scope );
		if ( e ) {
			FreeDef( e, scope );
		}

		e = GetDef( NULL, va( "%s_y", def->Name() ), scope );
		if ( e ) {
			FreeDef( e, scope );
		}

		e = GetDef( NULL, va( "%s_z", def->Name() ), scope );
		if ( e ) {
			FreeDef( e, scope );
		}
	}

	if ( def->settings.isReturn ) {
		GetScopeReturn( def->scope ).returns.RemoveFast( def );
	}

	varDefs.Remove( def );

	def->Clear();
	varDefAllocator.Free( def );
}

#ifdef SCRIPT_EVENT_RETURN_CHECKS
/*
============
idProgram::GetExpectedReturn
============
*/
int idProgram::GetExpectedReturn( void ) {
	idThread* thread = idThread::CurrentThread();
	assert( thread != NULL );
	return thread->GetInterpreter().GetExpectedReturn();
}
#endif // SCRIPT_EVENT_RETURN_CHECKS

/*
============
idProgram::GetScopeReturn
============
*/
scopeReturn_t& idProgram::GetScopeReturn( idVarDef* scope ) {
	int key = scopeReturnsHash.GenerateKey( scope );
	
	for ( int index = scopeReturnsHash.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = scopeReturnsHash.GetNext( index ) ) {
		if ( scopeReturns[ index ].scope != scope ) {
			continue;
		}

		return scopeReturns[ index ];
	}

	scopeReturnsHash.Add( key, scopeReturns.Num() );

	scopeReturn_t& r = scopeReturns.Alloc();
	r.scope = scope;
	return r;
}

/*
============
idProgram::FindFreeResultDef
============
*/
idVarDef *idProgram::FindFreeResultDef( idTypeDef *type, idVarDef *scope, const idVarDef *a, const idVarDef *b ) {
	scopeReturn_t& r = GetScopeReturn( scope );

	for ( int i = 0; i < r.returns.Num(); i++ ) {
		idVarDef* def = r.returns[ i ];

		if ( def == a || def == b ) {
			continue;
		}
		if ( def->TypeDef() != type ) {
			continue;
		}
		if ( def->numUsers <= 1 ) {
			continue;
		}
		return def;
	}

	idVarDef* newResultDef = AllocDef( type, RESULT_STRING, scope );
	newResultDef->settings.isReturn = true;

	r.returns.Alloc() = newResultDef;
	return newResultDef;
}

/*
================
idProgram::FindEvent

Searches for the specified script event function in the currently loaded script.

Returns 0 if function not found.
Returns >0 if function found.
================
*/
function_t *idProgram::FindEvent( const char *name ) {
	idVarDef	*def;

	assert( name );

	def = GetDef( NULL, name, &def_namespace );
	if ( !def ) {
		// couldn't find function
		return NULL;
	}

	if ( ( def->Type() == ev_function ) && ( def->value.functionPtr->eventdef != NULL ) ) {
		return def->value.functionPtr;
	}

	// is not a function, or is not an eventdef
	return NULL;
}

/*
================
idProgram::FindFunction

Searches for the specified function in the currently loaded script.  A full namespace should be
specified if not in the global namespace.

Returns 0 if function not found.
Returns >0 if function found.
================
*/
function_t* idProgram::FindFunctionInternal( const char *name ) const {
	int			start;
	int			pos;
	idVarDef	*namespaceDef;
	idVarDef	*def;

	assert( name );

	idStr fullname = name;
	start = 0;
	namespaceDef = &def_namespace;
	do {
		pos = fullname.Find( "::", true, start );
		if ( pos < 0 ) {
			break;
		}

		idStr namespaceName = fullname.Mid( start, pos - start );
		def = GetDef( NULL, namespaceName, namespaceDef );
		if ( !def ) {
			// couldn't find namespace
			return NULL;
		}
		namespaceDef = def;

		// skip past the ::
		start = pos + 2;
	} while( def->Type() == ev_namespace );

	idStr funcName = fullname.Right( fullname.Length() - start );
	def = GetDef( NULL, funcName, namespaceDef );
	if ( !def ) {
		// couldn't find function
		return NULL;
	}

	if ( ( def->Type() == ev_function ) && ( def->value.functionPtr->eventdef == NULL ) ) {
		return def->value.functionPtr;
	}

	// is not a function, or is an eventdef
	return NULL;
}

/*
================
idProgram::FindFunction

Searches for the specified object function in the currently loaded script.

Returns 0 if function not found.
Returns >0 if function found.
================
*/
function_t* idProgram::FindFunctionInternal( const char *name, const idTypeDef *type ) const {
	const idVarDef	*tdef;
	const idVarDef	*def;

	// look for the function
	def = NULL;
	for( tdef = type->def; tdef != &def_object; tdef = tdef->TypeDef()->SuperClass()->def ) {
		def = GetDef( NULL, name, tdef );
		if ( def ) {
			if ( def->Type() == ev_function ) {
				return def->value.functionPtr;
			} else {
				return NULL;
			}
		}
	}

	return NULL;
}

/*
================
idProgram::FindFunction

Searches for the specified object function in the currently loaded script only on the specified type.
================
*/
function_t* idProgram::FindFunctionLocal( const char *name, const idTypeDef *type ) const {
	// look for the function
	const idVarDef* def = GetDef( NULL, name, type->def );
	if ( def ) {
		if ( def->Type() == ev_function ) {
			return def->value.functionPtr;
		} else {
			return NULL;
		}
	}

	return NULL;
}

/*
================
idProgram::MatchesVirtualFunction
================
*/
int idProgram::MatchesVirtualFunction( const idTypeDef& match ) const {
	int i;

	for ( i = 0; i < globalVirtualFunctions.Num(); i++ ) {
		const function_t* function = globalVirtualFunctions[ i ];
		
		const idTypeDef& other = *function->def->TypeDef();

		if ( idStr::Cmp( match.Name(), other.Name() ) ) {
			continue;
		}

		if ( ( other.Type() != match.Type() ) || ( other.ReturnType() != match.ReturnType() ) ) {
			continue;
		}

		if ( ( other.NumParameters() + 1 ) != match.NumParameters() ) {
			continue;
		}

		int j;
		for( j = 1; j < match.NumParameters(); j++ ) {
			if ( other.GetParmType( j - 1 ) != match.GetParmType( j ) ) {
				continue;
			}
		}

		return i;
	}

	return -1;
}

/*
================
idProgram::AllocVirtualFunction
================
*/
function_t &idProgram::AllocVirtualFunction( idVarDef *def ) {
	if ( functions.Num() >= functions.Max() ) {
		throw idCompileError( va( "Exceeded maximum allowed number of functions (%d)", functions.Max() ) );
	}

	// fill in the dfunction
	function_t &func	= *functions.Alloc();
	func.eventdef		= NULL;
	func.def			= def;
	func.type			= def->TypeDef();
	func.firstStatement	= 0;
	func.numStatements	= 0;
	func.parmTotal		= 0;
	func.locals			= 0;
	func.filenum		= filenum;
	func.parmSize.SetGranularity( 1 );
	func.SetName( def->GlobalName() );
	func.virtualIndex	= globalVirtualFunctions.Append( &func );

	def->SetFunction( &func );

	return func;
}

/*
================
idProgram::AllocFunction
================
*/
function_t &idProgram::AllocFunction( idVarDef *def ) {
	if ( functions.Num() >= functions.Max() ) {
		throw idCompileError( va( "Exceeded maximum allowed number of functions (%d)", functions.Max() ) );
	}

	if ( ( functions.Num() % 20 ) == 0 ) {
		common->PacifierUpdate();
	}

	// fill in the dfunction
	function_t &func	= *functions.Alloc();
	func.eventdef		= NULL;
	func.def			= def;
	func.type			= def->TypeDef();
	func.firstStatement	= 0;
	func.numStatements	= 0;
	func.parmTotal		= 0;
	func.locals			= 0;
	func.filenum		= filenum;
	func.parmSize.SetGranularity( 1 );
	func.SetName( def->GlobalName() );
	func.virtualIndex	= -1;

	def->SetFunction( &func );

	return func;
}

/*
================
idProgram::AllocStatement
================
*/
statement_t *idProgram::AllocStatement( void ) {
	if ( statements.Num() >= statements.Max() ) {
		throw idCompileError( va( "Exceeded maximum allowed number of statements (%d)", statements.Max() ) );
	}
	return statements.Alloc();
}

#ifdef DEBUG_SCRIPTS
/*
==============
idProgram::DumpStats
==============
*/
void idProgram::DumpStats( void ) {
	idFile* dumpFile = fileSystem->OpenFileWrite( "scriptdump.txt" );
	if ( !dumpFile ) {
		gameLocal.Warning( "Failed to open dump file" );
		return;
	}

	idStr lastFile;

	for ( int j = 0; j < functions.Num(); j++ ) {
		if ( functions[ j ].eventdef != NULL ) {
			continue;
		}

		statement_t& firstStatement = statements[ functions[ j ].firstStatement ];
		if ( firstStatement.executionCount != 0 ) {
			continue;
		}

		const char* newFile = GetFilename( firstStatement.file );
		if ( lastFile.Cmp( newFile ) != 0 ) {
			lastFile = newFile;

			dumpFile->Printf( "\r\n======= File: %s =======\r\n", lastFile.c_str() );
		}

		dumpFile->Printf( "===== Function: %s =====\r\n", functions[ j ].Name() );

/*		for ( int k = 0; k < functions[ j ].numStatements; k++ ) {
			int i = functions[ j ].firstStatement + k;

			statement_t& s = statements[ i ];
			if ( s.executionCount != 0 ) {
				continue;
			}

			dumpFile->Printf( "line: %d \"%s\"\r\n", s.linenumber, idCompiler::opcodes[ s.op ].opname );
		}

		dumpFile->Printf( "\r\n" );*/
	}

	fileSystem->CloseFile( dumpFile );
}
#endif // DEBUG_SCRIPTS

/*
==============
idProgram::BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void idProgram::BeginCompilation( void ) {
	statement_t	*statement;

	FreeData();

	types.SetGranularity( 2048 );
	varDefs.SetGranularity( 4096 );
	varDefNames.SetGranularity( 2048 );
	scopeReturns.SetGranularity( 128 );
	scopeReturnsHash.SetGranularity( 128 );

	scopeReturns.Clear(); // if we had an error, these wont be clean yet
	scopeReturnsHash.Clear();

	try {
		// make the first statement a return for a "NULL" function
		statement = AllocStatement();
		statement->linenumber	= 0;
		statement->file 		= 0;
		statement->op			= OP_RETURN;
		statement->a			= NULL;
		statement->b			= NULL;
		statement->c			= NULL;
#ifdef DEBUG_SCRIPTS
		statement->executionCount = 0;
#endif // DEBUG_SCRIPTS

		// define NULL
		//AllocDef( &type_void, "<NULL>", &def_namespace );

		// define the return def
		returnDef = AllocDef( &type_vector, "<RETURN>", &def_namespace );

		// define the return def for strings
		returnStringDef = AllocDef( &type_string, "<RETURN>", &def_namespace );

		// define the sys object
		sysDef = AllocDef( &type_void, "sys", &def_namespace );
	}

	catch( idCompileError &err ) {
		gameLocal.Error( "%s", err.error );
	}
}

/*
==============
idProgram::DisassembleStatement
==============
*/
void idProgram::DisassembleStatement( idFile *file, int instructionPointer ) const {
	opcode_t			*op;
	const statement_t	*statement;

	statement = &statements[ instructionPointer ];
	op = &idCompiler::opcodes[ statement->op ];
	file->Printf( "%20s(%d):\t%6d: %15s\t", fileList[ statement->file ].c_str(), statement->linenumber, instructionPointer, op->opname );

	if ( statement->a ) {
		file->Printf( "\ta: " );
		statement->a->PrintInfo( *this, file, instructionPointer );
	}

	if ( statement->b ) {
		file->Printf( "\tb: " );
		statement->b->PrintInfo( *this, file, instructionPointer );
	}

	if ( statement->c ) {
		file->Printf( "\tc: " );
		statement->c->PrintInfo( *this, file, instructionPointer );
	}

	file->Printf( "\n" );
}

/*
==============
idProgram::Disassemble
==============
*/
void idProgram::Disassemble( void ) const {
	int					i;
	int					instructionPointer;
	const function_t	*func;
	idFile				*file;

	file = fileSystem->OpenFileByMode( "script/disasm.txt", FS_WRITE );

	for( i = 0; i < functions.Num(); i++ ) {
		func = &functions[ i ];
		if ( func->eventdef ) {
			// skip eventdefs
			continue;
		}

		file->Printf( "\nfunction %s() %d stack used, %d parms, %d locals {\n", func->Name(), func->locals, func->parmTotal, func->locals - func->parmTotal );

		for( instructionPointer = 0; instructionPointer < func->numStatements; instructionPointer++ ) {
			DisassembleStatement( file, func->firstStatement + instructionPointer );
		}
	
		file->Printf( "}\n" );
	}

	fileSystem->CloseFile( file );
}

/*
==============
idProgram::FinishCompilation

Called after all files are compiled to check for errors
==============
*/
void idProgram::FinishCompilation( void ) {
	compiled = true;

	types.Condense();
	varDefs.Condense();
	varDefNames.Condense();

	variableDefaults.Clear();
	variableDefaults.SetNum( numVariables );

	for( int i = 0; i < numVariables; i++ ) {
		variableDefaults[ i ] = variables[ i ];
	}

	scopeReturns.Clear();
	scopeReturnsHash.Clear();
}

/*
==============
idProgram::CompileStats

called after all files are compiled to report memory usage.
==============
*/
void idProgram::CompileStats( void ) {
	size_t	memused;
	size_t	memallocated;
	int	numdefs;
	size_t	stringspace;
	size_t funcMem;
	int	i;

	gameLocal.DPrintf( "---------- Compile stats ----------\n" );
	gameLocal.DPrintf( "Files loaded:\n" );

	stringspace = 0;
	for ( i = 0; i < fileList.Num(); i++ ) {
		gameLocal.DPrintf( "   %s\n", fileList[ i ].c_str() );
		stringspace += fileList[ i ].Allocated();
	}
	stringspace += fileList.Size();

	numdefs = varDefs.Num();
	memused = varDefAllocator.GetTotalCount() * sizeof( idVarDef );//varDefs.Num() * sizeof( idVarDef );
	memused = varDefNameAllocator.GetTotalCount() * sizeof( idVarDefName );//varDefs.Num() * sizeof( idVarDef );
	memused += typeDefAllocator.GetTotalCount() * sizeof( idTypeDef );//types.Num() * sizeof( idTypeDef );
	memused += stringspace;

	for ( i = 0; i < types.Num(); i++ ) {
		memused += types[ i ]->Allocated();
	}

	funcMem = functions.MemoryUsed();
	for ( i = 0; i < functions.Num(); i++ ) {
		funcMem += functions[ i ].Allocated();
	}

	memallocated = funcMem + memused + sizeof( idProgram );

	memused += statements.MemoryUsed();
	memused += functions.MemoryUsed();	// name and filename of functions are shared, so no need to include them
	memused += sizeof( variables );

	gameLocal.DPrintf( "\nMemory usage:\n" );
	gameLocal.DPrintf( "     Strings: %d, %d bytes\n", fileList.Num(), stringspace );
	gameLocal.DPrintf( "  Statements: %d, %d bytes\n", statements.Num(), statements.MemoryUsed() );
	gameLocal.DPrintf( "   Functions: %d, %d bytes\n", functions.Num(), funcMem );
	gameLocal.DPrintf( "   Variables: %d bytes\n", numVariables );
	gameLocal.DPrintf( "    Mem used: %d bytes\n", memused );
	gameLocal.DPrintf( " Static data: %d bytes\n", sizeof( idProgram ) );
	gameLocal.DPrintf( "   Allocated: %d bytes\n", memallocated );
	gameLocal.DPrintf( " Thread size: %d bytes\n\n", sizeof( idThread ) );
}

idCVar g_showCompileStats( "g_showCompileStats", "0", CVAR_BOOL | CVAR_GAME, "sets whether to show stats at the end of compilation or not" );

/*
================
idProgram::CompileText
================
*/
bool idProgram::CompileText( const char *source, const char *text ) {
	idCompiler	compiler( this );
	int			i;
	idVarDef	*def;

	filenum = GetFilenum( source );

	try {
		compiler.CompileFile( text, filename );

		// check to make sure all functions prototyped have code
		for( i = 0; i < varDefs.Num(); i++ ) {
			def = varDefs[ i ];
			if ( ( def->Type() == ev_function ) && ( ( def->scope->Type() == ev_namespace ) || def->scope->TypeDef()->Inherits( &type_object ) ) ) {
				if ( ( def->value.functionPtr->virtualIndex == -1 ) && ( !def->value.functionPtr->eventdef && !def->value.functionPtr->firstStatement ) ) {
					throw idCompileError( va( "function %s was not defined", def->GlobalName() ) );
				}
			}
		}
	}
	
	catch( idCompileError &err ) {
		gameLocal.Error( "%s", err.error );
	};

	defaultType = FindType( "default" );

	if ( g_showCompileStats.GetBool() ) {
		CompileStats();
	}

	return true;
}

/*
================
idProgram::CompileFile
================
*/
void idProgram::CompileFile( const char* filename ) {
	char *src;
	bool result;

	if ( fileSystem->ReadFile( filename, ( void ** )&src, NULL ) < 0 ) {
		gameLocal.Error( "Couldn't load %s", filename );
	}

	result = CompileText( filename, src );

	fileSystem->FreeFile( src );

	if ( g_disasm.GetBool() ) {
		Disassemble();
	}

	if ( !result ) {
		gameLocal.Error( "Compile failed in file %s.", filename );
	}
}

/*
================
idProgram::GetFunctionIndex
================
*/
int idProgram::GetFunctionIndex( const function_t* function ) const {
	for ( int i = 0; i < functions.Num(); i++ ) {
		if ( &functions[ i ] == function ) {
			return i;
		}
	}
	return -1;
}

/*
================
idProgram::GetTypeDefIndex
================
*/
int idProgram::GetTypeDefIndex( const idTypeDef* typeDef ) const {
	for ( int i = 0; i < types.Num(); i++ ) {
		if ( types[ i ] == typeDef ) {
			return i;
		}
	}
	return -1;
}

/*
================
idProgram::GetVarDefIndex
================
*/
int idProgram::GetVarDefIndex( const idVarDef* varDef ) const {
	for ( int i = 0; i < varDefs.Num(); i++ ) {
		if ( varDefs[ i ] == varDef ) {
			return i;
		}
	}
	return -1;
}

/*
================
idProgram::FreeData
================
*/
void idProgram::FreeData( void ) {
	// free the defs
	varDefs.Clear();
	varDefNames.Clear();
	varDefNameHash.Free();
	varDefAllocator.Shutdown();
	varDefNameAllocator.Shutdown();

	returnDef		= NULL;
	returnStringDef = NULL;
	sysDef			= NULL;

	// free any special types we've created
	types.Clear();
	typeHash.Free();
	typeDefAllocator.Shutdown();

	filenum = 0;

	numVariables = 0;
	memset( variables, 0, sizeof( variables ) );

	// clear all the strings in the functions so that it doesn't look like we're leaking memory.
	for( int i = 0; i < functions.Num(); i++ ) {
		functions[ i ].Clear();
	}

	FreeScriptObjects();

	idThread::Restart();

	filename.Clear();
	fileList.Clear();
	statements.Clear();
	globalVirtualFunctions.Clear();
	functions.Clear();

	filename = "";

	compiled = false;

	scriptExporter.Clear( false );

	for ( int i = 0; i < MAX_SCRIPT_STACK_SIZE_COUNT; i++ ) {
		freeStacks[ i ].DeleteContents( true );
	}
}

/*
================
idProgram::Startup
================
*/
void idProgram::Startup( const char* defaultScript ) {
	compiled = false;

	gameLocal.Printf( "Initializing scripts\n" );

	idThread::Restart();

	BeginCompilation();

	CompileFile( defaultScript );

	if ( exporting ) {
		scriptExporter.Finish();
	}

	FinishCompilation();

	exporting = false;
}


/*
================
idProgram::CalculateChecksum
================
*/
int idProgram::CalculateChecksum( void ) const {
	int i, result;

	typedef struct {
		unsigned short	op;
		int				a;
		int				b;
		int				c;
		unsigned short	linenumber;
		unsigned short	file;
	} statementBlock_t;

	statementBlock_t	*statementList = new statementBlock_t[ statements.Num() ];

	memset( statementList, 0, ( sizeof(statementBlock_t) * statements.Num() ) );

	// Copy info into new list, using the variable numbers instead of a pointer to the variable
	for( i = 0; i < statements.Num(); i++ ) {
		statementList[i].op = statements[i].op;

		if ( statements[ i ].a ) {
			statementList[ i ].a = GetVarDefIndex( statements[ i ].a );
		} else {
			statementList[ i ].a = -1;
		}
		if ( statements[ i ].b ) {
			statementList[ i ].b = GetVarDefIndex( statements[ i ].b );
		} else {
			statementList[ i ].b = -1;
		}
		if ( statements[ i ].c ) {
			statementList[ i ].c = GetVarDefIndex( statements[ i ].c );
		} else {
			statementList[ i ].c = -1;
		}

		statementList[i].linenumber = statements[i].linenumber;
		statementList[i].file = statements[i].file;
	}

	result = MD4_BlockChecksum( statementList, ( sizeof(statementBlock_t) * statements.Num() ) );

	delete [] statementList;

	return result;
}

/*
==============
idProgram::Restart

Restores all variables to their initial value
==============
*/
void idProgram::Restart( void ) {
	idThread::Restart();

	for( int i = 0; i < types.Num(); i++ ) {
		typeDefAllocator.Free( types[ i ] );
	}
	types.SetNum( 0, false );
	typeHash.Clear();

	for( int i = 0; i < varDefs.Num(); i++ ) {
		varDefs[ i ]->Clear();
		varDefAllocator.Free( varDefs[ i ] );
	}
	varDefs.SetNum( 0, false );

	for( int i = 0; i < functions.Num(); i++ ) {
		functions[ i ].Clear();
	}
	functions.SetNum( 0	);

	statements.SetNum( 0 );
	fileList.SetNum( 0, false );
	filename.Clear();
	
	// reset the variables to their default values
	numVariables = variableDefaults.Num();
	for( int  i = 0; i < numVariables; i++ ) {
		variables[ i ] = variableDefaults[ i ];
	}
}

/*
================
idProgram::GetFilenum
================
*/
int idProgram::GetFilenum( const char *name ) {
	if ( filename == name ) {
		return filenum;
	}

	filenum = fileList.AddUnique( name );
	filename = name;

	return filenum;
}

/*
================
idProgram::idProgram
================
*/
idProgram::idProgram() {
	compiled = false;
	exporting = false;

	scriptExporter.SetProgram( this );
}

/*
================
idProgram::~idProgram
================
*/
idProgram::~idProgram() {
	FreeData();
}

/*
================
idProgram::ReturnEntity
================
*/
void idProgram::ReturnEntityInternal( idEntity* ent ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_ENTITY && GetExpectedReturn() != D_EVENT_ENTITY_NULL ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	ReturnObjectInternal( ent ? ent->GetScriptObject() : NULL );
}

/*
================
idProgram::ReturnObject
================
*/
void idProgram::ReturnObjectInternal( idScriptObject* object ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_ENTITY && GetExpectedReturn() != D_EVENT_ENTITY_NULL && GetExpectedReturn() != D_EVENT_OBJECT ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	*returnDef->value.objectId = object ? object->GetHandle() : 0;
}

/*
================
idProgram::Init
================
*/
bool idProgram::Init( void ) {
	Startup( SCRIPT_DEFAULT );
	return true;
}

/*
================
idProgram::Shutdown
================
*/
void idProgram::Shutdown( void ) {
	FreeData();
}

/*
================
idProgram::CreateThread
================
*/
sdProgramThread* idProgram::CreateThread( void ) {
	idThread* thread = idThread::AllocThread();
	return thread;
}

/*
================
idProgram::CreateThread
================
*/
sdProgramThread* idProgram::CreateThread( const sdScriptHelper& h ) {
	const function_t* function = reinterpret_cast< const function_t* >( h.GetFunction() );

	if ( function->parmTotal != h.GetSize() ) {
		gameLocal.Warning( "idProgram::CreateThread Function '%s' Called With Incorrect Number Of Arguments", function->GetName() );
		assert( false );
		return NULL;
	}

	idThread* thread = reinterpret_cast< idThread* >( CreateThread() );

	thread->ManualDelete();
	thread->ManualControl();

	thread->GetInterpreter().PushParm( h.GetObject()->GetHandle() );

	const sdScriptHelper::parmsList_t& args = h.GetArgs();
	for ( int i = 0; i < args.Num(); i++ ) {
		if ( args[ i ].string ) {
			thread->GetInterpreter().PushParm( args[ i ].string );
		} else {
			thread->GetInterpreter().PushParm( args[ i ].integer );
		}
	}

	thread->GetInterpreter().EnterFunction( function, false );

	return thread;
}

/*
================
idProgram::FreeThread
================
*/
void idProgram::FreeThread( sdProgramThread* thread ) {
	idThread::FreeThread( ( idThread* )thread );
}

/*
================
idProgram::FindFunction
================
*/
const sdProgram::sdFunction* idProgram::FindFunction( const char* name ) {
	return FindFunctionInternal( name );
}

/*
================
idProgram::FindFunction
================
*/
const sdProgram::sdFunction* idProgram::FindFunction( const char* name, const sdProgram::sdTypeObject* object ) {
	return FindFunctionInternal( name, reinterpret_cast< const idProgramTypeObject* >( object )->GetType() );
}

/*
================
idProgram::KillThread
================
*/
void idProgram::KillThread( int number ) {
	idThread::KillThread( number );
}

/*
================
idProgram::KillThread
================
*/
void idProgram::KillThread( const char* name ) {
	idThread::KillThread( name );
}

/*
================
idProgram::GetCurrentThread
================
*/
sdProgramThread* idProgram::GetCurrentThread( void ) {
	return idThread::CurrentThread();
}

/*
================
idProgram::AllocType
================
*/
sdProgram::sdTypeObject* idProgram::AllocType( sdProgram::sdTypeObject* oldType, const char* typeName ) {
	idTypeDef* typeDef = FindType( typeName );
	if ( typeDef == NULL ) {
		FreeType( oldType );
		gameLocal.Warning( "idProgram::AllocType: Unknown type '%s'", typeName );
		return NULL;
	}

	return AllocType( oldType, typeDef );
}

/*
================
idProgram::FreeType
================
*/
void idProgram::FreeType( sdProgram::sdTypeObject* oldType ) {
	delete oldType;
}

/*
================
idProgram::AllocType
================
*/
sdProgram::sdTypeObject* idProgram::AllocType( sdProgram::sdTypeObject* oldType, const sdProgram::sdTypeInfo* typeInfo ) {
	idProgramTypeObject* oldT = reinterpret_cast< idProgramTypeObject* >( oldType );

	assert( typeInfo );

	const idTypeDef* typeDef = reinterpret_cast< const idTypeDef* >( typeInfo );
	if ( !typeDef->Inherits( &type_object ) ) {
		FreeType( oldType );
		gameLocal.Warning( "idProgram::AllocType: Can't create object of type '%s'.  Must be an object type.", typeDef->Name() );
		return NULL;
	}

	idProgramTypeObject* newT = NULL;

	if ( oldT != NULL ) {
		if ( typeDef == oldT->GetType() ) {
			newT = oldT;
		} else {
			FreeType( oldType );
		}
	}

	if ( newT == NULL ) {
		newT = new idProgramTypeObject( typeDef );
	}

	newT->Clear();

	return newT;
}

/*
================
idProgram::FindTypeInfo
================
*/
const sdProgram::sdTypeInfo* idProgram::FindTypeInfo( const char *name ) {
	return FindType( name );
}

/*
================
idProgram::GetNumClasses
================
*/
int idProgram::GetNumClasses( void ) const {
	int count = 0;
	for ( int i = 0; i < types.Num(); i++ ) {
		if ( types[ i ]->Type() == ev_object ) {
			count++;
		}
	}
	return count;
}

/*
================
idProgram::GetClass
================
*/
const sdProgram::sdTypeInfo* idProgram::GetClass( int index ) const {
	int count = 0;
	for ( int i = 0; i < types.Num(); i++ ) {
		if ( types[ i ]->Type() == ev_object ) {
			if ( count == index ) {
				return types[ i ];
			}
			count++;
		}
	}
	return NULL;
}


/*
================
idProgram::ListThreads
================
*/
void idProgram::ListThreads( void ) const {
	idThread::ListThreads();

	int count = 0;
	int size = 0;
	for ( int i = 0; i < MAX_SCRIPT_STACK_SIZE_COUNT; i++ ) {
		int localCount = freeStacks[ i ].Num();
		size += ( ( i + 1 ) * 1024 ) * localCount;
		count += localCount;
	}

	gameLocal.Printf( "Total Free Thread Stacks: %d\n", count );
	gameLocal.Printf( "Total Free Thread Stack Size: %d\n", size );
}

/*
================
idProgram::PruneThreads
================
*/
void idProgram::PruneThreads( void ) {
	idThread::PruneThreads();
}

/*
================
idProgram::AllocStack
================
*/
byte* idProgram::AllocStack( int size ) {
	assert( ( ( size % 1024 ) == 0 ) && size <= 6144 );

	const int index = ( size / 1024 ) - 1;

	int num = freeStacks[ index ].Num();
	if ( num > 0 ) {
		byte* stack = freeStacks[ index ][ num - 1 ];
		freeStacks[ index ].SetNum( num - 1, false );
		return stack;
	}

	return new byte[ size ];
}

/*
================
idProgram::FreeStack
================
*/
void idProgram::FreeStack( byte* stack, int size ) {
	if ( stack == NULL ) {
		return;
	}

	assert( ( ( size % 1024 ) == 0 ) && size <= 6144 );

	const int index = ( size / 1024 ) - 1;

	freeStacks[ index ].Alloc() = stack;
}

/*
================
idProgram::OnError
================
*/
bool idProgram::OnError( const char* text ) {
	idThread* thread = idThread::CurrentThread();
	if ( thread == NULL ) {
		return false;
	}

	thread->Error( text );
	return true;
}

/*
================
idProgramTypeObject::GetVariable
================
*/
etype_t idProgramTypeObject::GetVariable( const char *name, byte** _data ) const {
	*_data = NULL;

	const idTypeDef* t = type;
	do {
		int pos;
		if ( t->SuperClass() != &type_object ) {
			pos = t->SuperClass()->Size();
		} else {
			pos = 0;
		}
		for( int i = 0; i < t->NumParameters(); i++ ) {
			const idTypeDef* parm = t->GetParmType( i );
			if ( !strcmp( t->GetParmName( i ), name ) ) {
				*_data = &data[ pos ];
				return parm->FieldType()->Type();
			}

			if ( parm->FieldType()->Inherits( &type_object ) ) {
				pos += type_object.Size();
			} else {
				pos += parm->FieldType()->Size();
			}
		}
		t = t->SuperClass();
	} while( t && ( t != &type_object ) );

	return ev_error;
}

/*
================
idProgramTypeObject::ValidateCall
================
*/
bool idProgramTypeObject::ValidateCall( const sdProgram::sdFunction* func ) const {
	const function_t* f = reinterpret_cast< const function_t* >( func );

	return type->Inherits( f->type->GetParmType( 0 ) );
}
