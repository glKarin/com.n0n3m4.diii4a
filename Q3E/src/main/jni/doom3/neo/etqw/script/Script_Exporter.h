// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_EXPORTER_H__
#define __SCRIPT_EXPORTER_H__

class idVarDef;
class idTypeDef;

#define BASE_COMPILED_SCRIPT_CLASS_ALLOCATE		"sdCompiledScript_ClassBase"
#define BASE_COMPILED_SCRIPT_CLASS				"sdCompiledScript_Class"
#define BASE_COMPILED_SCRIPT_CLASS_HEADER		"CompiledScript_Class.h"
#define BASE_COMPILED_SCRIPT_CLASS_CPP			"base/CompiledScript_Class.cpp"
class sdScriptExporter {
public:
	struct objectDef_t;
	struct threadDef_t;
	struct callDef_t;

	struct functionDef_t {
		const function_t*			function;
		callDef_t*					externalCall;
		idList< const idVarDef* >	variables;
		bool						isVirtual;
	};

	struct stackVar_t {
		idTypeDef*					type;
		idStr						name;
		int							offset;
		bool						allocated;
	};

	struct namespaceDef_t {
		idStr						name;
		idList< objectDef_t* >		classes;
		idList< stackVar_t* >		globalVars;
		idList< namespaceDef_t* >	namespaces;
		namespaceDef_t*				parentNamespace;
	};

	struct constantDef_t {
		const idVarDef*				value;
	};

	struct callDef_t {
		const idTypeDef*				returnType;
		idList< const idTypeDef* >		parms;
		const idEventDef*				event;
	};

	struct threadDef_t {
		const idTypeDef*				returnType;
		idList< const idTypeDef* >		parms;
	};

	struct objectDef_t {
		idTypeDef*						type;
		idList< functionDef_t >			functions;
		idList< const idVarDef* >		fields;
		objectDef_t*					superType;
		int								baseOffset;
		idList< threadDef_t* >			threadCalls;
		idList< threadDef_t* >			guiThreadCalls;
	};
public:
					sdScriptExporter( void ) { program = NULL; tabCount = 0; }
					~sdScriptExporter( void ) { Clear( true ); }

	void			SetProgram( idProgram* _program ) { program = _program; }

	void			RegisterEventDef( const function_t& f );

	const char*		BuildGlobalName( const stackVar_t& var );
	const char*		BuildGlobalFunctionName( const function_t* function );
	const char*		BuildEventName( const char* name );
	const char*		BuildClassName( const idTypeDef* type );
	const char*		BuildFieldNameByType( etype_t type, etype_t defaultType = ev_error );
	const char*		BuildFieldName( const idTypeDef* type, etype_t defaultType = ev_error );
	const char*		BuildClassHeaderName( const idTypeDef* type, bool full );
	const char*		BuildClassCPPName( const idTypeDef* type, bool full );
	const char*		BuildConstantName( int index );

	void			AddDependency( const idTypeDef* type, idList< const idTypeDef* >& dependencies );
	void			AddDependency( const function_t* function, idList< const idTypeDef* >& dependencies, bool contents );

	int				GetVarDefNum( const idVarDef* var );
	void			AllocGlobal( const idVarDef* var );
	int				AllocConstant( const idVarDef* var );

	void			Finish( void );

	void			PrintTabs( idFile* file );

	void			WriteGlobalVariables( void );

	void			FindGlobalVariablesDependencies( namespaceDef_t* ns, idList< const idTypeDef* >& dependencies );
	void			FindGlobalFunctionDependencies( namespaceDef_t* ns, idList< const idTypeDef* >& dependencies );

	void			WriteGlobalVariablesHeader( idFile* file, const namespaceDef_t* ns );
	void			WriteGlobalVariablesCPP( idFile* file, const namespaceDef_t* ns );

	void			WriteVirtualFunctions( void );
	void			WriteClassFunctionWrappers( void );
	void			WriteSysCalls( void );
	void			WriteBuildVersion( void );
    void            WriteXCodeProjectFile( void );
	void			WriteProjectFile( void );
	void			WriteEventCalls( void );

	void			WriteNamespaceClasses( const namespaceDef_t* ns );

	void			Clear( bool finished );
	void			ClearNamespace( namespaceDef_t* ns );

	objectDef_t*	FindClass( const namespaceDef_t* ns, const idTypeDef* type );
	stackVar_t*		FindGlobal( const namespaceDef_t* ns, const idVarDef* var );
	int				FindThreadCall( const objectDef_t& obj, const function_t* function, bool guithreadcall );

	void			WriteFunctionStub( idFile* file, const functionDef_t& funcDef, idTypeDef* object, int baseparm );
	void			WriteThreadCallStub( idFile* file, const objectDef_t* obj, int index, int baseparm, bool clarify, bool includeName, bool gui );
	void			WriteThreadCallType( idFile* file, const objectDef_t* obj, int index, int baseparm, const char* name, bool gui );
	void			WriteThreadCallWrapperClass( idFile* file, const objectDef_t* obj, int index, int baseparm, bool gui );
	void			WriteFunctionWrappers( idFile* cppFile );

	void			WriteNamespaceTitle( idFile* file, const namespaceDef_t* ns );
	void			WriteNamespaceEntry( idFile* file, const namespaceDef_t* ns );
	void			WriteNamespaceExit( idFile* file, const namespaceDef_t* ns );
	void			WriteNamespaceScope( idFile* file, const namespaceDef_t* ns );
	void			WriteNamespaceFunctions( const namespaceDef_t* ns, idFile* headerFile, idFile* cppFile );
	void			WriteNamespaceFunctions( const namespaceDef_t* ns );
	void			WriteNamespaceClassInit( const namespaceDef_t* ns, idFile* cppFile );
	void			WriteNamespaceFunctionInfo( const namespaceDef_t* ns, idFile* cppFile );
	void			WriteNamespaceFunctionInit( const namespaceDef_t* ns, idFile* cppFile );
	void			WriteNamespaceClassIncludes( const namespaceDef_t* ns, idFile* cppFile );

	void			WriteFunctionStartSpecial( idFile* file, const objectDef_t& cls, const function_t* function );

	void			RegisterVirtualFunction( idTypeDef* type );
	void			RegisterClass( idTypeDef* type, idTypeDef* superType );
	void			RegisterClassFunction( idTypeDef* type, const function_t* function );
	void			RegisterClassField( idTypeDef* type, idVarDef* field );
	void			RegisterClassFunctionVariable( idTypeDef* type, const function_t* function, const idVarDef* var );
	void			RegisterClassThreadCall( idTypeDef* type, const function_t* function, bool guithreadcall );
	void			RegisterExternalClassCall( functionDef_t& funcDef );
	void			RegisterExternalFunctionCall( functionDef_t& funcDef );
	int				RegisterSysCall( const function_t* function );
	int				RegisterEventCall( const function_t* function );
	
	int				RegisterCall( const function_t* function, int baseParm, idList< callDef_t* >& list );

	void			EnterNamespace( const char* name );
	void			ExitNamespace( void );

	const char*		FindFieldName( const namespaceDef_t* ns, idTypeDef* type, idVarDef* ptr );
	static int		FieldSize( const idTypeDef* typeDef );

private:
	struct eventDef_t {
		idStr		name;
	};
	idList< idTypeDef* >				virtualFunctions;
	idList< eventDef_t >				events;
	int									tabCount;

	namespaceDef_t						globalNameSpace;
	namespaceDef_t*						currentNameSpace;

	idList< callDef_t* >				externalClassCalls;
	idList< callDef_t* >				externalFunctionCalls;
	idList< callDef_t* >				sysCalls;
	idList< callDef_t* >				eventCalls;
	idList< constantDef_t >				constants;

	idProgram*							program;

	idStrList							generatedCppFiles;
	idStrList							generatedHFiles;

	void								WriteClass( const namespaceDef_t* ns, objectDef_t& cls );

public:
	namespaceDef_t&	GetGlobalNamespace( void ) { return globalNameSpace; }
};

class sdFunctionCompileState {
public:
	sdFunctionCompileState( const sdScriptExporter::functionDef_t* _func, const sdScriptExporter::namespaceDef_t* _nameSpace, idFile* _output, int initialTabCount, idProgram* _program );
	~sdFunctionCompileState();

	void				Run( void );

private:
	struct resultInfo_t {
		idVarDef*		object;
		idVarDef*		result;
	};

private:
	void				ScanForLabels( void );
	void				ScanOpCodes( void );
	void				ScanOpCode( int statement );
	bool				LookAheadStore( int statement );
	bool				IsReturn( const idVarDef* var );

	void				CheckVariable( const idVarDef* var, etype_t defaultType = ev_error );
	void				GetVariableName( const idVarDef* var, etype_t expectedType, idStr& name );
	void				PrintVariable( const idVarDef* var, etype_t expectedType );
	void				PrintField( idTypeDef* type, idVarDef* ptr );

	void				PrintTabs( void );

	sdScriptExporter::stackVar_t*	FindStackVar( const idVarDef* var );
	sdScriptExporter::stackVar_t&	AllocStackVar( const idVarDef* var );
	void						AllocateSubVars( const sdScriptExporter::stackVar_t& var );
	void						InitStackVar( sdScriptExporter::stackVar_t& var );
	void						AllocBaseStackVars( void );

private:
	const sdScriptExporter::functionDef_t*	func;
	const sdScriptExporter::namespaceDef_t*	nameSpace;
	// write the opcodes and the variable declaration to two different streams
	// then write out declarations first to the file
	idFile*				fileOutput;
	idFile_Memory*		output;
	idFile_Memory*		variables;
	idList< int >		labels;
	idList< idVarDef* >	compileStack;
	idList< resultInfo_t >	resultStack;
	idList< sdScriptExporter::stackVar_t* > stackVars;
	int					tabCount;
	idVarDef*			returnVar;
	bool				skipNextStatement;
	bool				isSpecial;
	idProgram*			program;
};

#endif // __SCRIPT_EXPORTER
