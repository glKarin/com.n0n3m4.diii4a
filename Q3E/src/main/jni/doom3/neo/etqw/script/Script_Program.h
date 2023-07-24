// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_PROGRAM_H__
#define __SCRIPT_PROGRAM_H__

// #define DEBUG_SCRIPTS

#include "../network/SnapshotState.h"
#include "Script_Interface.h"
#include "Script_Exporter.h"

class idScriptObject;
class idEventDef;
class idVarDef;
class idTypeDef;
class idEntity;
class idThread;
class idProgram;
class sdScriptHelper;
class sdEntityStateNetworkData;

const int MAX_STRING_LEN		= 128;
const int MAX_GLOBALS			= 460800;		// in bytes
const int MAX_STRINGS			= 1024;
const int MAX_FUNCS				= 5120;
const int MAX_STATEMENTS		= 131072;		// statement_t - 18 bytes last I checked
const int MAX_OBJECTS			= 2048;

class function_t : public sdProgram::sdFunction {
public:
						function_t();

	size_t				Allocated( void ) const;
	void				SetName( const char *name );
	const char			*Name( void ) const;
	void				Clear( void );

	virtual const char*	GetName( void ) const { return name.c_str(); }

private:
	idStr 				name;
public:
	const idEventDef	*eventdef;
	idVarDef			*def;
	const idTypeDef		*type;
	int 				firstStatement;
	int 				numStatements;
	int 				parmTotal;
	int 				locals; 			// total ints of parms + locals
	int					filenum; 			// source file defined in
	idList<int>			parmSize;
	int					virtualIndex;
};

typedef union eval_s {
	const char			*stringPtr;
	const wchar_t		*wstringPtr;
	float				_float;
	float				vector[ 3 ];
	function_t			*function;
	int 				_int;
	int 				_objectId;
} eval_t;


/***********************************************************************

idTypeDef

Contains type information for variables and functions.

***********************************************************************/

class idTypeDef : public sdProgram::sdTypeInfo {
private:
	etype_t						type;
	idStr 						name;
	int							size;

	// function types are more complex
	idTypeDef*					auxType;					// return type
	idList<idTypeDef *>			parmTypes;
	idStrList					parmNames;
	idList<const function_t *>	functions;
	idHashIndex					globalVirtuals;
	idProgram*					program;

public:
	idVarDef					*def;						// a def that points to this type

						idTypeDef( void );
						idTypeDef( const idTypeDef &other );
						idTypeDef( etype_t etype, idVarDef *edef, const char *ename, int esize, idTypeDef *aux );
						~idTypeDef( void );

	void				SetProgram( idProgram* _program ) { program = _program; }
	void				Init( etype_t etype, idVarDef *edef, const char *ename, int esize, idTypeDef *aux );

	void				operator=( const idTypeDef& other );
	size_t				Allocated( void ) const;

	bool				Inherits( const idTypeDef *basetype ) const;
	bool				MatchesType( const idTypeDef &matchtype ) const;
	bool				MatchesGlobalVirtualFunction( void ) const;
	bool				MatchesVirtualFunction( const idTypeDef &matchfunc ) const;
	void				AddFunctionParm( idTypeDef *parmtype, const char *name );
	void				AddField( idTypeDef *fieldtype, const char *name );

	void				SetName( idProgram* _program, const char *newname );
	const char			*Name( void ) const;

	etype_t				Type( void ) const;
	int					Size( void ) const;

	idTypeDef			*SuperClass( void ) const;
	
	idTypeDef			*ReturnType( void ) const;
	void				SetReturnType( idTypeDef *type );

	idTypeDef			*FieldType( void ) const;
	void				SetFieldType( idTypeDef *type );

	idTypeDef			*PointerType( void ) const;
	void				SetPointerType( idTypeDef *type );

	int					NumParameters( void ) const;
	idTypeDef			*GetParmType( int parmNumber ) const;
	const char			*GetParmName( int parmNumber ) const;

	int					NumFunctions( void ) const;
	int					GetFunctionNumber( const function_t *func ) const;
	const function_t	*GetFunction( int funcNumber ) const;
	const function_t*	GetVirtualFunction( int funcNumber ) const;
	void				AddFunction( const function_t *func, idProgram& program );

	virtual const char*						GetName( void ) const { return name.c_str(); }
	virtual const sdProgram::sdTypeInfo*	GetSuperClass( void ) const { return type == ev_object ? auxType : NULL; }
};


/***********************************************************************

idProgramTypeObject

***********************************************************************/

class idProgramTypeObject : public sdProgram::sdTypeObject {
public:
									idProgramTypeObject( const idTypeDef* _type ) { type = _type; data = new byte[ type->Size() ]; }
	virtual							~idProgramTypeObject() { delete[] data; }

	const idTypeDef*				GetType( void ) const { return type; }
	byte*							GetData( void ) const { return data; }

	virtual void					Clear( void ) { memset( data, 0, type->Size() ); }
	virtual const char*				GetName( void ) const { return type->Name(); }
	virtual etype_t					GetVariable( const char *name, byte** _data ) const;
	virtual bool					ValidateCall( const sdProgram::sdFunction* func ) const;
	virtual void					SetHandle( int handle ) { ; }

private:
	const idTypeDef*				type;
	byte*							data;
};



/***********************************************************************

idCompileError

Causes the compiler to exit out of compiling the current function and
display an error message with line and file info.

***********************************************************************/

class idCompileError : public idException {
public:
	idCompileError( const char *text ) : idException( text ) {}
};

/***********************************************************************

idVarDef

Define the name, type, and location of variables, functions, and objects
defined in script.

***********************************************************************/

typedef union varEval_s {
	char*					stringPtr;
	wchar_t*				wstringPtr;
	float*					floatPtr;
	idVec3*					vectorPtr;
	function_t*				functionPtr;
	int*					intPtr;
	byte*					bytePtr;
	int*					objectId;
	int						virtualFunction;
	int						jumpOffset;
	int						stackOffset;		// offset in stack for local variables
	int						argSize;
	varEval_s*				evalPtr;
	int						ptrOffset;
} varEval_t;

class idVarDefName;

class idVarDef {
public:
	varEval_t				value;
	idVarDef*				scope; 			// function, namespace, or object the var was defined in
	int						numUsers;		// number of users if this is a constant

	typedef enum {
		uninitialized, initializedVariable, initializedConstant, stackVariable
	} initialized_t;

	struct settings_t {
		initialized_t		initialized : 3;
		bool				isReturn	: 1;
	};

	settings_t				settings;

public:
							idVarDef( idTypeDef *typeptr = NULL );
							~idVarDef();

	void					Init( idTypeDef *typeptr = NULL );
	void					Clear();

	const char *			Name( void ) const;
	const char *			GlobalName( void ) const;

	void					SetTypeDef( idTypeDef *_type ) { typeDef = _type; }
	idTypeDef *				TypeDef( void ) const { return typeDef; }
	etype_t					Type( void ) const { return ( typeDef != NULL ) ? typeDef->Type() : ev_void; }

	int						DepthOfScope( const idVarDef *otherScope ) const;

	void					SetFunction( function_t *func );
	void					SetObject( idScriptObject *object );
	void					SetValue( const eval_t &value, bool constant );
	void					SetString( const char *string, bool constant );

	idVarDef*				Next( void ) const { return next; }		// next var def with same name
	
	void					SetNext( idVarDef* def ) { next = def; }
	void					SetName( idVarDefName* _name ) { name = _name; }

	void					PrintInfo( const idProgram& program, idFile *file, int instructionPointer ) const;

private:
	idTypeDef*				typeDef;
	idVarDefName*			name;		// name of this var
	idVarDef*				next;		// next var with the same name
};

/***********************************************************************

  idVarDefName

***********************************************************************/

class idVarDefName {
public:
							idVarDefName( void ) { defs = NULL; }
							idVarDefName( const char *n ) { name = n; defs = NULL; }

	void					Init( const char* n ) { name = n; defs = NULL; }

	const char*				Name( void ) const { return name; }
	idVarDef*				GetDefs( void ) const { return defs; }

	void					AddDef( idVarDef *def );
	void					RemoveDef( idVarDef *def );

private:
	idStr					name;
	idVarDef *				defs;
};

/***********************************************************************

  Variable and type defintions

***********************************************************************/

extern	idTypeDef	type_void;
extern	idTypeDef	type_scriptevent;
extern	idTypeDef	type_namespace;
extern	idTypeDef	type_string;
extern	idTypeDef	type_wstring;
extern	idTypeDef	type_float;
extern	idTypeDef	type_vector;
extern  idTypeDef	type_field;
extern	idTypeDef	type_function;
extern	idTypeDef	type_virtualfunction;
extern  idTypeDef	type_pointer;
extern	idTypeDef	type_object;
extern	idTypeDef	type_jumpoffset;	// only used for jump opcodes
extern	idTypeDef	type_argsize;		// only used for function call and thread opcodes
extern	idTypeDef	type_boolean;
extern	idTypeDef	type_internalscriptevent;

extern	idVarDef	def_void;
extern	idVarDef	def_scriptevent;
extern	idVarDef	def_namespace;
extern	idVarDef	def_string;
extern	idVarDef	def_wstring;
extern	idVarDef	def_float;
extern	idVarDef	def_vector;
extern	idVarDef	def_field;
extern	idVarDef	def_function;
extern	idVarDef	def_virtualfunction;
extern	idVarDef	def_pointer;
extern	idVarDef	def_object;
extern	idVarDef	def_jumpoffset;		// only used for jump opcodes
extern	idVarDef	def_argsize;		// only used for function call and thread opcodes
extern	idVarDef	def_boolean;
extern	idVarDef	def_internalscriptevent;

typedef struct statement_s {
	unsigned short	op;
	idVarDef		*a;
	idVarDef		*b;
	idVarDef		*c;
	unsigned short	linenumber;
	unsigned short	file;
#ifdef DEBUG_SCRIPTS
	int				executionCount;
#endif // DEBUG_SCRIPTS
} statement_t;

/***********************************************************************

idProgram

Handles compiling and storage of script data.  Multiple idProgram objects
would represent seperate programs with no knowledge of each other.  Scripts
meant to access shared data and functions should all be compiled by a
single idProgram.

***********************************************************************/

struct scopeReturn_t {
	idVarDef*			scope;
	idList< idVarDef* >	returns;
};

class idProgram : public sdProgram {
private:
	static const int								MAX_SCRIPT_STACK_SIZE_COUNT = 6;
	static const int								MAX_SCRIPT_STACK_SIZE = 1024 * MAX_SCRIPT_STACK_SIZE_COUNT;

	idStrList										fileList;
	idStr 											filename;
	int												filenum;

	int												numVariables;
	byte											variables[ MAX_GLOBALS ];
	idStaticList< byte, MAX_GLOBALS >				variableDefaults;
	idStaticList< function_t, MAX_FUNCS >			functions;
	idStaticList< statement_t, MAX_STATEMENTS >		statements;

	idList< byte* >									freeStacks[ MAX_SCRIPT_STACK_SIZE_COUNT ];
	
	idList< idTypeDef* >							types;
	idHashIndex										typeHash;

	idList< idVarDefName* >							varDefNames;
	idHashIndex										varDefNameHash;

	idList< function_t* >							globalVirtualFunctions;
	idList< idVarDef* >								varDefs;

	idVarDef*										sysDef;

	idList< scopeReturn_t >							scopeReturns;
	idHashIndex										scopeReturnsHash;

	idBlockAlloc< idTypeDef, 64 >					typeDefAllocator;
	idBlockAlloc< idVarDef, 256 >					varDefAllocator;
	idBlockAlloc< idVarDefName, 64 >				varDefNameAllocator;

	bool											compiled;
	bool											exporting;

	idStr											exportScriptName;

	void											CompileStats( void );

public:
	idVarDef*										returnDef;
	idVarDef*										returnStringDef;
	idTypeDef*										defaultType;

	sdScriptExporter								scriptExporter;

													idProgram();
													~idProgram();

	int												CalculateChecksum( void ) const;		// Used to ensure program code has not
																							//    changed between savegames

#ifdef SCRIPT_EVENT_RETURN_CHECKS
	int												GetExpectedReturn( void );
	void											OnEventCallReturnFailure( void );
#endif // SCRIPT_EVENT_RETURN_CHECKS

	bool											IsExporting( void ) const { return exporting; }
	void											EnableExport( void ) { exporting = true; }

	int												GetNumVarDefs( void ) const { return varDefs.Num(); }
	const idVarDef*									GetVarDef( int index ) const { return varDefs[ index ]; }

	scopeReturn_t&									GetScopeReturn( idVarDef* scope );

	byte*											AllocStack( int size );
	void											FreeStack( byte* stack, int size );

	void											Startup( const char *defaultScript );
	void											Restart( void );
	bool											CompileText( const char *source, const char *text );
	void											CompileFile( const char *filename );
#ifdef DEBUG_SCRIPTS
	void											DumpStats( void );
#endif // DEBUG_SCRIPTS
	void											BeginCompilation( void );
	void											FinishCompilation( void );
	void											DisassembleStatement( idFile *file, int instructionPointer ) const;
	void											Disassemble( void ) const;
	void											FreeData( void );
	
	int												GetFunctionIndex( const function_t* function ) const;
	int												GetTypeDefIndex( const idTypeDef* typeDef ) const;
	int												GetVarDefIndex( const idVarDef* varDef ) const;

	const char*										GetFilename( int num ) const;
	int												GetFilenum( const char *name );
	int												GetLineNumberForStatement( int index );
	const char*										GetFilenameForStatement( int index );

	idVarDef*										GetVarDefForIndex( int index ) { return varDefs[ index ]; }
	idTypeDef*										GetTypeDefForIndex( int index ) { return types[ index ]; }

	idTypeDef*										AllocType( idTypeDef &type );
	idTypeDef*										AllocType( etype_t etype, idVarDef *edef, const char *ename, int esize, idTypeDef *aux );
	idTypeDef*										GetType( idTypeDef &type, bool allocate );
	idTypeDef*										FindType( const char *name );

	bool											RemoveFromHash( idTypeDef* type );
	void											AddToHash( idTypeDef* type, int index = -1 );

	idVarDef*										AllocDef( idTypeDef *type, const char *name, idVarDef *scope );
	idVarDef*										GetDef( const idTypeDef *type, const char *name, const idVarDef *scope ) const;
	void											FreeDef( idVarDef *d, const idVarDef *scope );
	idVarDef*										FindFreeResultDef( idTypeDef *type, idVarDef *scope, const idVarDef *a, const idVarDef *b );
	idVarDef*										GetDefList( const char *name ) const;
	void											AddDefToNameList( idVarDef *def, const char *name );

	function_t*										FindEvent( const char *name );

	function_t*										FindFunctionInternal( const char *name ) const;
	function_t*										FindFunctionInternal( const char *name, const idTypeDef *type ) const;
	function_t*										FindFunctionLocal( const char *name, const idTypeDef *type ) const;

	function_t&										AllocVirtualFunction( idVarDef *def );
	function_t&										AllocFunction( idVarDef *def );
	function_t*										GetFunction( int index );
	int												GetFunctionIndex( const function_t *func );

	statement_t*									AllocStatement( void );
	statement_t&									GetStatement( int index );
	const statement_t&								GetStatement( int index ) const;
	int												NumStatements( void ) { return statements.Num(); }

	int												MatchesVirtualFunction( const idTypeDef& match ) const;
	
	int												NumFilenames( void ) { return fileList.Num( ); }


	virtual bool									OnError( const char* text );

	virtual void									KillThread( int number );
	virtual void									KillThread( const char* name );
	virtual sdProgramThread*						GetCurrentThread( void );

	virtual int 									GetReturnedInteger( void );
	virtual float									GetReturnedFloat( void );
	virtual bool									GetReturnedBoolean( void );
	virtual const char*								GetReturnedString( void );
	virtual const wchar_t*							GetReturnedWString( void );
	virtual idScriptObject*							GetReturnedObject( void );

	virtual void									ReturnFloatInternal( float value );
	virtual void									ReturnIntegerInternal( int value );
	virtual void									ReturnVectorInternal( idVec3 const &vec );
	virtual void									ReturnStringInternal( const char *string );
	virtual void									ReturnWStringInternal( const wchar_t *string );
	virtual void									ReturnEntityInternal( idEntity *ent );
	virtual void									ReturnObjectInternal( idScriptObject* object );

	virtual const sdProgram::sdTypeInfo*			FindTypeInfo( const char *name );

	virtual int										GetNumClasses( void ) const;
	virtual const sdProgram::sdTypeInfo*			GetClass( int index ) const;

	virtual bool									Init( void );
	virtual void									Shutdown( void );

	virtual sdProgramThread*						CreateThread( void );
	virtual sdProgramThread*						CreateThread( const sdScriptHelper& h );
	virtual void									FreeThread( sdProgramThread* thread );

	virtual const sdProgram::sdFunction*			FindFunction( const char* name );
	virtual const sdProgram::sdFunction*			FindFunction( const char* name, const sdProgram::sdTypeObject* object );
	virtual sdProgram::sdTypeObject*				AllocType( sdProgram::sdTypeObject* oldType, const sdProgram::sdTypeInfo* typeInfo );
	virtual sdProgram::sdTypeObject*				AllocType( sdProgram::sdTypeObject* oldType, const char* typeName );
	virtual void									FreeType( sdProgram::sdTypeObject* oldType );
	virtual bool									IsValid( void ) const { return compiled; }

	virtual const sdProgram::sdTypeInfo*			GetDefaultType( void ) const { return defaultType; }

	virtual void									ListThreads( void ) const;
	virtual void									PruneThreads( void );
};

/*
================
idProgram::GetStatement
================
*/
ID_INLINE statement_t &idProgram::GetStatement( int index ) {
	return statements[ index ];
}

/*
================
idProgram::GetStatement
================
*/
ID_INLINE const statement_t& idProgram::GetStatement( int index ) const {
	return statements[ index ];
}

/*
================
idProgram::GetFunction
================
*/
ID_INLINE function_t *idProgram::GetFunction( int index ) {
	return &functions[ index ];
}

/*
================
idProgram::GetFunctionIndex
================
*/
ID_INLINE int idProgram::GetFunctionIndex( const function_t *func ) {
	return func - &functions[0];
}

/*
================
idProgram::GetReturnedInteger
================
*/
ID_INLINE int idProgram::GetReturnedInteger( void ) {
	return *returnDef->value.intPtr;
}

/*
================
idProgram::GetReturnedFloat
================
*/
ID_INLINE float idProgram::GetReturnedFloat( void ) {
	return *returnDef->value.floatPtr;
}

/*
================
idProgram::GetReturnedBoolean
================
*/
ID_INLINE bool idProgram::GetReturnedBoolean( void ) {
	return ( *returnDef->value.intPtr ) != 0 ? true : false;
}

/*
================
idProgram::GetReturnedString
================
*/
ID_INLINE const char* idProgram::GetReturnedString( void ) {
	return returnStringDef->value.stringPtr;
}

/*
================
idProgram::GetReturnedWString
================
*/
ID_INLINE const wchar_t* idProgram::GetReturnedWString( void ) {
	return returnStringDef->value.wstringPtr;
}

/*
================
idProgram::GetReturnedObject
================
*/
ID_INLINE idScriptObject* idProgram::GetReturnedObject( void ) {
	return GetScriptObject( *returnDef->value.objectId );
}

#ifdef SCRIPT_EVENT_RETURN_CHECKS
/*
================
idProgram::OnEventCallReturnFailure
================
*/
ID_INLINE void idProgram::OnEventCallReturnFailure( void ) {
	#ifdef _DEBUG
		assert( false );
	#else
		gameLocal.Error( "Invalid return from script event call." );
	#endif // _DEBUG
}
#endif // SCRIPT_EVENT_RETURN_CHECKS

/*
================
idProgram::ReturnFloat
================
*/
ID_INLINE void idProgram::ReturnFloatInternal( float value ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_FLOAT && GetExpectedReturn() != D_EVENT_INTEGER ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	*returnDef->value.floatPtr = value;
}

/*
================
idProgram::ReturnInteger
================
*/
ID_INLINE void idProgram::ReturnIntegerInternal( int value ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_BOOLEAN && GetExpectedReturn() != D_EVENT_HANDLE ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	*returnDef->value.intPtr = value;
}

/*
================
idProgram::ReturnVector
================
*/
ID_INLINE void idProgram::ReturnVectorInternal( idVec3 const &vec ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_VECTOR ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	*returnDef->value.vectorPtr = vec;
}

/*
================
idProgram::ReturnStringInternal
================
*/
ID_INLINE void idProgram::ReturnStringInternal( const char *string ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_STRING ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	idStr::Copynz( returnStringDef->value.stringPtr, string, MAX_STRING_LEN );
}

/*
================
idProgram::ReturnWStringInternal
================
*/
ID_INLINE void idProgram::ReturnWStringInternal( const wchar_t *string ) {
#ifdef SCRIPT_EVENT_RETURN_CHECKS
	if ( GetExpectedReturn() != D_EVENT_WSTRING ) {
		OnEventCallReturnFailure();
	}
#endif // SCRIPT_EVENT_RETURN_CHECKS
	idWStr::Copynz( returnStringDef->value.wstringPtr, string, MAX_STRING_LEN );
}

/*
================
idProgram::GetFilename
================
*/
ID_INLINE const char *idProgram::GetFilename( int num ) const {
	return fileList[ num ];
}

/*
================
idProgram::GetLineNumberForStatement
================
*/
ID_INLINE int idProgram::GetLineNumberForStatement( int index ) {
	return statements[ index ].linenumber;
}

/*
================
idProgram::GetFilenameForStatement
================
*/
ID_INLINE const char *idProgram::GetFilenameForStatement( int index ) {
	return GetFilename( statements[ index ].file );
}

#endif /* !__SCRIPT_PROGRAM_H__ */
